#include "ProxyConnection.h"

#include <spdlog/spdlog.h>

#include <algorithm>
#include <asio/buffer.hpp>
#include <asio/connect.hpp>
#include <asio/error.hpp>
#include <asio/read.hpp>
#include <asio/write.hpp>
#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <exception>
#include <iomanip>
#include <iostream>
#include <string>
#include <string_view>
#include <system_error>

#include "Server.h"
#include "common.h"

namespace {
constexpr uint64_t kBytePerKb = 1024;
constexpr uint64_t kBytesPerMb = kBytePerKb * 1024;

std::string getAddrFromEp(const proxy::Endpoint& ep) {
  std::string result{};
  result.reserve(32);
  result.append(ep.address().to_string());
  result.append(":");
  result.append(std::to_string(ep.port()));
  return result;
}

template <typename TP1, typename TP2>
proxy::Seconds getDuration(TP1 t1, TP2 t2) {
  auto time1 = std::chrono::duration_cast<std::chrono::seconds>(t1 - t2);
  return time1;
}

void show_current_time(const char* what) {
  auto time =
      std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
  std::cout << what << std::put_time(std::localtime(&time), "%F %T") << "\n";
}

void printErr(const std::error_code& ec, uint64_t conn_id,
              std::string_view description = "") {
  switch (ec.value()) {
    case asio::error::operation_aborted: {
      // ugly code ....
      if (description == "timer") {
        break;
      }
      spdlog::trace("connection id:{} {} error:{} ", conn_id, description,
                    ec.message());
      break;
    }
    case asio::error::eof: {
      spdlog::debug("connection id:{} {} error:{} ", conn_id, description,
                    ec.message());
      break;
    }
    default:
      spdlog::warn("connection id:{} {} error:{}", conn_id, description,
                   ec.message());
  }
}

}  // namespace

using std::chrono::duration_cast;
using std::chrono::seconds;
using std::chrono::steady_clock;
namespace proxy {

std::atomic_uint64_t ProxyConnection::sConnectionNum{0};

ProxyConnection::ProxyConnection(Socket socket, Endpoint ep,
                                 ProxyServer& server)
    : client_socket_(std::move(socket)),
      dest_(ep),
      server_(server),
      server_socket_(server.getIo()),
      deadline_timer_(server.getIo()),
      start_time_(std::chrono::steady_clock::now()),
      conn_id_(sConnectionNum.fetch_add(1)) {
  spdlog::info("connection id:{} proxy connection init", conn_id_);
}

void ProxyConnection::start() {
  auto self(shared_from_this());

  // TODO add a timer here for a long connect error
  deadline_timer_.expires_after(Seconds{10});
  addTimer();
  server_socket_.async_connect(dest_, [this, self](std::error_code ec) {
    if (!ec) {
      spdlog::debug("connect to server----->{}", dest_.address().to_string());
      readClient();
      readServer();
    } else {
      printErr(ec, conn_id_, "connect to server");
      close();
    }
  });
}

// TODO more infomation here
std::string ProxyConnection::getConnInfo() const {
  char buf[128];
  auto client_addr = getAddrFromEp(client_socket_.local_endpoint());
  auto server_addr = getAddrFromEp(client_socket_.remote_endpoint());
  auto time = duration_cast<seconds>(steady_clock::now() - start_time_).count();
  auto n = std::snprintf(buf, 128,
                         "connection:%lu ,from %s to %s,last %lu seconds,has "
                         "transferred %luMb/%luKb,average: %luKb/s",
                         conn_id_, client_addr.c_str(), server_addr.c_str(),
                         time, transferred_ / kBytesPerMb,
                         (transferred_ % kBytesPerMb) / kBytePerKb,
                         (transferred_ / kBytePerKb) / time);

  if (n < 0) {
    spdlog::error("format string error");
  }

  return buf;
}

void ProxyConnection::addTimer() {
  auto self(shared_from_this());
  deadline_timer_.async_wait([this, self](std::error_code ec) {
    if (!ec) {
      // thread safe
      if (closed_.load()) {
        return;
      }
      // double check timer
      if (deadline_timer_.expiry() < steady_clock::now()) {
        spdlog::info("connection id:{} close on time out", conn_id_);
        close();
      }

    } else {
      printErr(ec, conn_id_, "timer");
    }
  });
}

void ProxyConnection::readClient() {
  auto self(shared_from_this());
  deadline_timer_.expires_after(Seconds{120});
  addTimer();
  client_socket_.async_read_some(
      asio::buffer(buffer1_),
      [this, self](std::error_code ec, std::size_t read_bytes) {
        if (!ec) {
          if (closed_.load()) {
            return;
          }
          spdlog::trace("connection id:{} read from client--->{} bytes",
                        conn_id_, read_bytes);
          server_socket_.async_send(
              asio::buffer(buffer1_.data(), read_bytes),
              [this, self, read_bytes](std::error_code ec, std::size_t length) {
                transferred_ += length;
                spdlog::trace(
                    "connection id:{} write to server--->{} bytes,which should "
                    "be-->{}",
                    conn_id_, length, read_bytes);
                if (!ec) {
                  // for thread safe
                  if (closed_.load()) {
                    return;
                  }
                  readClient();
                } else {
                  printErr(ec, conn_id_, "write to server");
                  close();
                }
              });
        } else {
          printErr(ec, conn_id_, "read from client");
          close();
        }
      });
}

void ProxyConnection::readServer() {
  auto self(shared_from_this());
  deadline_timer_.expires_after(Seconds{120});
  addTimer();
  server_socket_.async_read_some(
      asio::buffer(buffer2_),
      [this, self](std::error_code ec, std::size_t read_bytes) {
        if (!ec) {
          if (closed_.load()) {
            return;
          }
          spdlog::trace("connection id:{}read from server--->{} bytes",
                        conn_id_, read_bytes);
          client_socket_.async_write_some(
              asio::buffer(buffer2_.data(), read_bytes),
              [this, self, read_bytes](std::error_code ec, std::size_t length) {
                transferred_ += length;
                spdlog::trace(
                    "connection id:{} write to client--->{} bytes,which should "
                    "be-->{}",
                    conn_id_, length, read_bytes);
                if (!ec) {
                  if (closed_.load()) {
                    return;
                  }
                  readServer();
                } else {
                  printErr(ec, conn_id_, "write to client");
                  close();
                }
              });
          // error may happen at cancellation
        } else {
          printErr(ec, conn_id_, "read server");
          close();
        }
      });
}

void ProxyConnection::close() {
  bool dummy{false};
  if (closed_.compare_exchange_strong(dummy, true)) {
    spdlog::debug("connection id:{} connection close called", conn_id_);
    if (client_socket_.is_open()) {
      std::error_code ignore_ec{};
      client_socket_.shutdown(tcp::socket::shutdown_both, ignore_ec);
      client_socket_.close(ignore_ec);
    }
    if (server_socket_.is_open()) {
      std::error_code ignore_ec{};
      server_socket_.shutdown(tcp::socket::shutdown_both, ignore_ec);
      server_socket_.close(ignore_ec);
    }
    deadline_timer_.cancel();
    server_.destroyConnection(shared_from_this());
  }
}

}  // namespace proxy