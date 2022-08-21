#include "Server.h"

#include <spdlog/spdlog.h>

#include <algorithm>
#include <asio/io_context.hpp>
#include <cassert>
#include <cstddef>
#include <cstdio>
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
    : io_(io), print_timer_(io), config_(config) {}

void ProxyServer::start() {
  try {
    spdlog::info("server start");
    channels_ = config_.getProxyChannels();

    // TODO pre verification that listen_ports are valid
    for (const auto& [listen, dest] : channels_) {
      auto& acceptor = acceptors_.emplace_back(io_);
      acceptor.open(listen.protocol());
      acceptor.set_option(tcp::acceptor::reuse_address(true));
      acceptor.bind(listen);
      acceptor.listen();
    }
    for (int i = 0; i < acceptors_.size(); ++i) {
      doAccept(i);
    }
    addPrint();
  } catch (std::exception& e) {
    spdlog::error("catch exception on proxy server start:{}", e.what());
  } catch (...) {
    // bad error we can do nothing
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

void ProxyServer::doAccept(int i) {
  acceptors_[i].async_accept([i, this](std::error_code ec, Socket socket) {
    if (!acceptors_[i].is_open()) {
      return;
    }
    if (!ec) {
      auto ptr = std::make_shared<ProxyConnection>(std::move(socket),
                                                   channels_[i].second, *this);
      ptr->start();
      addConnection(ptr);
    } else {
      spdlog::error("accept error--->{}", ec.message());
    }
    doAccept(i);
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