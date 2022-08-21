#pragma once
// #include <asio.hpp>
#include <asio/ip/tcp.hpp>
#include <asio/steady_timer.hpp>
#include <asio/system_timer.hpp>
#include <chrono>
#include <cstddef>

#define GUARDED_BY(x) THREAD_ANNOTATION_ATTRIBUTE__(guarded_by(x))

namespace proxy {
using Endpoint = asio::ip::tcp::endpoint;
using address = asio::ip::address;
using Resolver = asio::ip::tcp::resolver;
using asio::io_context;
using query = asio::ip::tcp::resolver::query;
using Acceptor = asio::ip::tcp::acceptor;
using Socket = asio::ip::tcp::socket;
using TimePoint = std::chrono::steady_clock::time_point;
using Timer = asio::steady_timer;
using tcp = asio::ip::tcp;
using Seconds = std::chrono::seconds;

constexpr size_t kBufferSize = 4 * 1024;
}  // namespace proxy