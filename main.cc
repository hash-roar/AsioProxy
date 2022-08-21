#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_TRACE
#include <spdlog/common.h>
#include <spdlog/spdlog.h>

#include "Config.h"
#include "Server.h"
#include "common.h"

int main(int argc, const char** argv) {
  spdlog::set_level(spdlog::level::trace);
  proxy::io_context io{4};

  // for resolve
  proxy::Config config{io};
  if (!config.parse(argc, argv)) {
    spdlog::error("parse config error");
  }
  proxy::ProxyServer server{io, config};
  server.start();
  io.run();
  return 0;
}