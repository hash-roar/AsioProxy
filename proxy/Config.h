#pragma once
#include <map>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>
#include <vector>

#include "common.h"

namespace proxy {
class Config {
 public:
  using ProxyChannelConfig = std::pair<Endpoint, Endpoint>;
  Config(io_context& io) : io_(io) {}
  ~Config() = default;

  bool parse(int argc, const char** argv);
  const DestChannelConfig& getProxyChannels() const;
  Endpoint getDestEndpoint();
  Endpoint getListenEndpoint();
  std::pair<std::string, std::string> toIpPort() const { return {"", ""}; };

 private:
  bool resolve();
  bool parseConfigFile(std::string_view file_name);
  DestChannelConfig cache_{};
  std::map<std::string, std::vector<std::pair<int, std::string>>>
      config_string_;
  io_context& io_;
};

}  // namespace proxy