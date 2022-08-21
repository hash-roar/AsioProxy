#include "Config.h"

#include <spdlog/spdlog.h>

#include <algorithm>
#include <asio/ip/basic_endpoint.hpp>
#include <cassert>
#include <cctype>
#include <cstdio>
#include <exception>
#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>
#include <vector>

#include "common.h"

namespace {
inline void ltrim(std::string& str) {
  str.erase(str.begin(),
            std::find_if(str.begin(), str.end(),
                         [](unsigned char c) { return !std::isspace(c); }));
}
inline void rtrim(std::string& str) {
  str.erase(std::find_if(str.rbegin(), str.rend(),
                         [](unsigned char c) { return !std::isspace(c); })
                .base(),
            str.end());
}

inline void trim(std::string& str) {
  ltrim(str);
  rtrim(str);
}

std::vector<std::string> split(const std::string& text,
                               const std::string& delims) {
  std::vector<std::string> tokens;
  std::size_t start = text.find_first_not_of(delims), end = 0;

  while ((end = text.find_first_of(delims, start)) != std::string::npos) {
    tokens.push_back(text.substr(start, end - start));
    start = text.find_first_not_of(delims, end);
  }
  if (start != std::string::npos) tokens.push_back(text.substr(start));

  return tokens;
}

}  // namespace
namespace proxy {
Endpoint Config::getDestEndpoint() {
  Resolver resolver{io_};
  return Endpoint{};
}

std::vector<Config::ProxyChannelConfig> Config::getProxyChannels() const {
  if (!cache_.empty()) {
    return cache_;
  }
  Resolver resolver{io_};
  std::vector<ProxyChannelConfig> result{};
  for (const auto& [listen, dest] : dest_addrs_) {
    auto listen_port = static_cast<asio::ip::port_type>(std::stoi(listen));
    auto pos = dest.find(':');
    if (pos == std::string::npos) {
      spdlog::warn("invalid destination:{}", dest);
      continue;
    }
    std::string dest_domain(dest.begin(), dest.begin() + pos);
    std::string dest_port(dest.begin() + pos + 1, dest.end());

    auto ret = resolver.resolve(dest_domain, dest_port);
    if (ret.empty()) {
      spdlog::warn("resolve no result of{}", dest_domain);
      continue;
    }
    spdlog::debug("domain:{} resolved to:{}", dest_domain,
                  ret.begin()->endpoint().address().to_string());
    result.emplace_back(Endpoint{tcp::v4(), listen_port},
                        ret.begin()->endpoint());
  }
  cache_ = result;
  return result;
}

bool Config::parseConfigFile(std::string_view file_name) {
  // catch many throw here
  try {
    std::ifstream file{file_name.data()};
    // parse content to array
    for (std::string line; std::getline(file, line);) {
      if (std::all_of(line.begin(), line.end(), isspace)) {
        continue;
      }

      // line: " port  ip:port  "
      // parse it into "{{"port","ip:port"},{}}"
      trim(line);
      std::string listen(line.begin(), std::find_if(line.begin(), line.end(),
                                                    [](unsigned char c) {
                                                      return std::isspace(c);
                                                    }));
      std::string dest(
          std::find_if(line.rbegin(), line.rend(),
                       [](unsigned char c) { return std::isspace(c); })
              .base(),
          line.end());
      spdlog::trace("listen:[{}],dest:[{}]", listen, dest);
      dest_addrs_.emplace_back(line, dest);
    }

    return true;
  } catch (std::exception& e) {
    spdlog::error("parse config file error:{}", e.what());
    return false;
  }
}

bool Config::parse(int argc, const char** argv) {
  switch (argc) {
    case 1: {
      // TODO for development return true

      dest_addrs_ = {{"8000", "127.0.0.1:9000"},
                     {"8001", "127.0.0.1:9001"},
                     {"8002", "www.baidu.com:80"}};

      spdlog::error("no enough argument");
      return true;
    }
    case 2: {
      return parseConfigFile(argv[1]);
    }
    case 3: {
      return true;
    }
    default: {
      spdlog::warn("too many arguments");
      return false;
    }
  }
}
}  // namespace proxy