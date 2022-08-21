#pragma once

#include <spdlog/common.h>
#include <spdlog/spdlog.h>

#include <asio/io_context.hpp>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

#include "Config.h"
#include "ProxyConnection.h"
#include "common.h"

namespace proxy {

// TODO object pool to prevent heap allocation
class ProxyServer {
  using ConnectionPtrSet = std::unordered_set<std::shared_ptr<ProxyConnection>>;
  using Acceptors = std::vector<Acceptor>;

 public:
  ProxyServer(io_context& io, Config config);
  ~ProxyServer() = default;

  void destroyConnection(std::shared_ptr<ProxyConnection>);

  void start();

  const Config& getConfig() const { return config_; }
  io_context& getIo() const { return io_; };
  std::string getServerInfo();

 private:
  void doAccept(int i);
  void addConnection(std::shared_ptr<ProxyConnection>);
  void addPrint();

  uint64_t transferred_bytes_{0};
  Timer print_timer_;

  std::vector<Config::ProxyChannelConfig> channels_;
  Acceptors acceptors_{};
  Config config_;  // copy it intentionally
  std::mutex mutex_{};
  ConnectionPtrSet connections_{};
  io_context& io_;
};

}  // namespace proxy
