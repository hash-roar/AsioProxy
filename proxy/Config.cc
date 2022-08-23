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
#include <iostream>
#include <iterator>
#include <string>
#include <string_view>
#include <utility>
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

void print(const proxy::DestChannelConfig& config) {
  for (const auto& [key, vec] : config) {
    for (const auto& [pri, dest_ep] : vec) {
      std::cout << pri << " " << dest_ep.address().to_string() << ":"
                << dest_ep.port() << "\n";
    }
  }
}

}  // namespace
namespace proxy {
Endpoint Config::getDestEndpoint() {
  Resolver resolver{io_};
  return Endpoint{};
}

const DestChannelConfig& Config::getProxyChannels() const { return cache_; }

bool Config::parseConfigFile(std::string_view file_name) {
  // catch many throw here
  try {
    // parse file

    std::ifstream file{file_name.data()};
    if (!file.is_open()) {
      spdlog::error("can not open config file :{}", file_name.data());
      return false;
    }
    // parse content to array
    for (std::string line; std::getline(file, line);) {
      // if line all of  space
      if (std::all_of(line.begin(), line.end(), isspace)) {
        continue;
      }

      // line: " port  ip:port[priority] ip:port[priority] ip:port  "
      // parse it into "{"port":{{priority,"ip:port"},...}}"
      trim(line);
      vector<pair<int, string>> dests{};
      auto tokens = split(line, " ");
      if (tokens.size() < 2) {
        spdlog::error("config error line:{}", line);
      }
      std::transform(
          tokens.begin() + 1, tokens.end(), std::back_inserter(dests),
          [](const string& dest_str) -> pair<int, string> {
            if (auto itr = dest_str.find("["); itr != string::npos) {
              auto itr2 = dest_str.find("]", itr);
              auto priority =
                  std::stoi(dest_str.substr(itr + 1, itr2 - itr - 1));
              return {priority,
                      string(dest_str.begin(), dest_str.begin() + itr)};
            } else {
              return {1, dest_str};
            }
          });
      config_string_.emplace(tokens.front(), dests);
    }

    return true;
  } catch (std::exception& e) {
    spdlog::error("parse config file error:{}", e.what());
  }
  return false;
}

bool Config::resolve() {
  try {
    // resolve domain to ip
    Resolver resolver{io_};
    DestChannelConfig config{};
    for (const auto& [listen, dests_str] : config_string_) {
      auto listen_port = static_cast<asio::ip::port_type>(std::stoi(listen));
      vector<pair<int, Endpoint>> dests{};
      dests.reserve(dests_str.size());
      // int string
      for (const auto& [priority, dest_str] : dests_str) {
        auto pos = dest_str.find(':');
        if (pos == std::string::npos) {
          spdlog::warn("invalid destination:{},which has been omitted",
                       dest_str);
          continue;
        }
        std::string dest_domain(dest_str.begin(), dest_str.begin() + pos);
        std::string dest_port(dest_str.begin() + pos + 1, dest_str.end());
        auto ret = resolver.resolve(dest_domain, dest_port);
        if (ret.empty()) {
          spdlog::warn("resolve no result of{}", dest_domain);
          continue;
        }
        spdlog::debug("domain:{} resolved to:{}", dest_domain,
                      ret.begin()->endpoint().address().to_string());

        dests.emplace_back(priority, ret.begin()->endpoint());
      }

      config.emplace(Endpoint{tcp::v4(), listen_port}, std::move(dests));
    }
    cache_ = std::move(config);
    print(cache_);
    return true;
  } catch (std::exception& e) {
    spdlog::error("catch exception on resolve:{}", e.what());
  }
  return false;
}

bool Config::parse(int argc, const char** argv) {
  switch (argc) {
    case 1: {
      // TODO for development return true

      config_string_ = {
          {"8000",
           {{1, "127.0.0.1:9000"},
            {2, "localhost:9001"},
            {3, "localhost:9002"}}},
      };

      spdlog::error("no enough argument");
      return resolve();
    }
    case 2: {
      return parseConfigFile(argv[1]) && resolve();
    }
    default: {
      spdlog::warn("too many arguments");
      return false;
    }
  }
}
}  // namespace proxy