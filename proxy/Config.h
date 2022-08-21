#pragma once
#include <string>
#include <string_view>
#include <tuple>
#include <utility>

#include "common.h"

namespace proxy {
class Config {
 public:
  using ProxyChannelConfig = std::pair<Endpoint, Endpoint>;
  Config(io_context& io) : io_(io) {}
  ~Config() = default;

  bool parse(int argc, const char** argv);
  std::vector<ProxyChannelConfig> getProxyChannels() const;
  Endpoint getDestEndpoint();
  Endpoint getListenEndpoint();
  std::pair<std::string, std::string> toIpPort() const { return {"", ""}; };

 private:
  bool parseConfigFile(std::string_view file_name);
  std::vector<ProxyChannelConfig> mutable cache_{};
  std::vector<std::pair<std::string, std::string>>
      dest_addrs_{};  // {{"8081","127.0.0.1:8080"},{"7071","domain.com:7070"}}
  io_context& io_;
};

}  // namespace proxy