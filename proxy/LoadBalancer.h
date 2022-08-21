#include <cstddef>
#include <map>
#include <vector>

#include "common.h"

namespace proxy {

class LoadBalancer {
 public:
  LoadBalancer(const DestChannelConfig&);
  ~LoadBalancer();
  const Endpoint* getNext(Endpoint* local);

 private:
  struct ChannelInfo {
    std::vector<const Endpoint*> dest_endpoints{};
    // avoid use pair intentionally
    std::vector<int> priority{};
    size_t accumulate{0};
    size_t last_index{0};
  };
  std::map<const Endpoint*, ChannelInfo> dests_channels_{};
};
}  // namespace proxy