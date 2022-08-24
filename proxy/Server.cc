#include "Server.h"

#include <spdlog/spdlog.h>

#include <algorithm>
#include <asio/io_context.hpp>
#include <cassert>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <exception>
#include <iostream>
#include <memory>
#include <mutex>
#include <string>
#include <system_error>

#include "ProxyConnection.h"
#include "common.h"

namespace proxy {

ProxyServer::ProxyServer(io_context& io, Config config)
    : config_(config), io_(io), print_timer_(io), sigs_(io) {
  load_balancer_ = std::make_unique<LoadBalancer>(config_.getProxyChannels());
}

void ProxyServer::start() {
  try {
    spdlog::info("server start");
    const auto& channels = config_.getProxyChannels();
    // TODO pre verification that listen_ports are valid
    for (const auto& [listen, dests] : channels) {
      auto& acceptor = acceptors_.emplace_back(io_);
      acceptor.open(listen.protocol());
      acceptor.set_option(tcp::acceptor::reuse_address(true));
      acceptor.bind(listen);
      acceptor.listen();
    }

    for (auto& acceptor : acceptors_) {
      doAccept(acceptor);
    }
    addPrint();
    initSignal();
  } catch (std::exception& e) {
    spdlog::error("catch exception on proxy server start:{}", e.what());
  } catch (...) {
    // bad error, we can do nothing
    throw;
  }
}

std::string ProxyServer::getServerInfo() {
  size_t conn_num = 0;
  {
    std::lock_guard lock{mutex_};
    conn_num = connections_.size();
  }
  char buf[128];
  auto n =
      std::snprintf(buf, sizeof(buf), "server has %lu connections", conn_num);
  if (n < 0) {
    spdlog::error("error happen when format server info");
  }
  // rvo here
  return buf;
}

void ProxyServer::initSignal() {
  sigs_.add(SIGINT);
  // sigs.add(SIGTERM);
  sigs_.async_wait([this](std::error_code ec, int sig_num) {
    spdlog::info("accept signal:{}, close server", sig_num);
    // stop acceptor
    std::for_each(acceptors_.begin(), acceptors_.end(), [](Acceptor& acceptor) {
      spdlog::trace("close acceptor of:{}", acceptor.local_endpoint().port());
      acceptor.close();
    });

    io_.stop();

    // TODO do we really need this lock?
    // std::exit(0);
    // {
    //   std::cout << "lock";
    //   // std::lock_guard lock{mutex_};
    //   for (auto& conn : connections_) {
    //     conn->close();
    //   }
    //   std::cout << "unlock";
    //   connections_.clear();
    // }
  });
}

void ProxyServer::doAccept(Acceptor& acceptor) {
  acceptor.async_accept([&acceptor, this](std::error_code ec, Socket socket) {
    if (!acceptor.is_open()) {
      return;
    }
    if (!ec) {
      const Endpoint* ep_ptr =
          load_balancer_->getNext(acceptor.local_endpoint().port());
      spdlog::trace("from local:{},choose:{}", acceptor.local_endpoint().port(),
                    ep_ptr->port());
      auto ptr =
          std::make_shared<ProxyConnection>(std::move(socket), *ep_ptr, *this);
      ptr->start();
      addConnection(ptr);
    } else {
      spdlog::error("accept error--->{}", ec.message());
    }
    doAccept(acceptor);
  });
}

void ProxyServer::addPrint() {
  print_timer_.expires_after(Seconds{60});
  print_timer_.async_wait([this](const std::error_code& ec) {
    if (!ec) {
      // ConnectionPtrSet conns{};
      // {
      //   std::lock_guard lock{mutex_};
      //   // copy here
      //   conns = connections_;
      // }

      // for (const auto& ptr : conns) {
      //   std::cout << ptr->getConnInfo() << "\n";
      // }

      addPrint();
    } else {
      spdlog::error("error at print timer:{}", ec.message());
    }
  });
}

void ProxyServer::destroyConnection(std::shared_ptr<ProxyConnection> conn) {
  {
    std::lock_guard lock{mutex_};
    transferred_bytes_ += conn->transferred();
    auto num = connections_.erase(conn);
    assert(num == 1);
  }
  spdlog::info("connection id:{} destroy in server", conn->id(),
               conn.use_count());
}

void ProxyServer::addConnection(std::shared_ptr<ProxyConnection> ptr) {
  {
    std::lock_guard lock{mutex_};
    auto [_itr, success] = connections_.insert(ptr);
    assert(success);
  }
}

}  // namespace proxy