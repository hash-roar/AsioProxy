#include "LoadBalancer.h"

#include <algorithm>
#include <cassert>
#include <iostream>
#include <utility>
#include <vector>

#include "common.h"
#include "spdlog/spdlog.h"

namespace proxy {

LoadBalancer::LoadBalancer(const DestChannelConfig& config) {
  spdlog::debug("init load balancer now");
  assert(!config.empty());
  for (const auto& [local, dests] : config) {
    std::vector<const Endpoint*> dest_endpoints(dests.size(), nullptr);
    std::vector<int> priority(dests.size(), 1);
    std::for_each(
        dests.begin(), dests.end(),
        [&, i = 0](const std::pair<int, Endpoint>& dest_pair) mutable {
          dest_endpoints[i] = &dest_pair.second;
          priority[i] = dest_pair.first;
          i++;
        });

    dests_channels_.emplace(
        local.port(),
        ChannelInfo{std::move(dest_endpoints), std::move(priority), 0, 0});
  }
  auto& a = dests_channels_.at(8000);
  std::cout << a.last_index << "\n";
}

// throw
const Endpoint* LoadBalancer::getNext(int port) {
  auto& info = dests_channels_.at(8000);
  assert(info.dest_endpoints.front()->port() != 0);
  assert(info.dest_endpoints.size() == 3);
  assert(info.last_index < info.dest_endpoints.size());
  // 1 1 2 3 1
  // 0 0 0 2 0
  // 0 1 2 3 4

  if (info.accumulate == info.priority[info.last_index] - 1) {
    auto old_index = info.last_index;
    info.last_index = (info.last_index + 1) % info.dest_endpoints.size();
    info.accumulate = 0;
    auto t = info.dest_endpoints[old_index];
    std::cout << t->port() << "\n";
    return t;
    // 0 2-1
  } else {
    info.accumulate++;
    return info.dest_endpoints[info.last_index];
  }
}

}  // namespace proxy