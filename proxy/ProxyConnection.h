#pragma once

#include <spdlog/common.h>
#include <spdlog/spdlog.h>

#include <array>
#include <atomic>
#include <cstdint>
#include <memory>
#include <string>

#include "common.h"

namespace proxy {

class ProxyServer;
class ProxyConnection : public std::enable_shared_from_this<ProxyConnection> {
 public:
  ProxyConnection(Socket socket, Endpoint dest, ProxyServer& server);
  ~ProxyConnection() {
    spdlog::info("connection id:{} connection finally close here", conn_id_);
  };
  // connect

  void start();
  std::string getConnInfo() const;
  uint64_t id() const { return conn_id_; }
  uint64_t transferred() const { return transferred_; }

  void close();
 private:
  // void printError();  implement it in anonymous space
  void readClient();
  void readServer();
  void addTimer();

  Socket client_socket_;
  Socket server_socket_;
  TimePoint start_time_;
  uint64_t conn_id_;
  uint64_t transferred_{0};  // to be accurate use atomic
  Endpoint dest_;
  Timer deadline_timer_;
  ProxyServer& server_;
  std::atomic_bool closed_{false};
  static std::atomic_uint64_t sConnectionNum;

  // buffer from client to server
  std::array<char, kBufferSize> buffer1_{};
  // buffer from server to client
  std::array<char, kBufferSize> buffer2_{};
};
}  // namespace proxy