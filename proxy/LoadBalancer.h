#include <cstddef>
#include <map>
#include <vector>

#include "common.h"

namespace proxy {

// load balancer will be accessed concurrently
// but we do not need lock for that there are no critical area
class LoadBalancer {
 public:
  LoadBalancer(const DestChannelConfig&);
  ~LoadBalancer() = default;
  const Endpoint* getNext(int listen);

 private:
  struct ChannelInfo {
    std::vector<const Endpoint*> dest_endpoints{};
    // avoid use pair intentionally
    std::vector<int> priority{};
    size_t accumulate{0};
    size_t last_index{0};
  };
  std::map<int, ChannelInfo> dests_channels_{};
};
}  // namespace proxy