#include <functional>
#include <iostream>
#include <thread>

#include <asio_stream_compressor/asio_stream_compressor.h>
#include <boost/asio.hpp>
#include <boost/asio/spawn.hpp>

#include "detail/echo.h"

namespace asio = asio_stream_compressor::asio;
namespace ip = asio::ip;

void example(asio::io_context& ctx, boost::asio::yield_context yield)
{
  asio_stream_compressor::compressor<ip::tcp::socket> sock(ctx);

  try {
    static const size_t DATA_SIZE = 65535;
    // prepare data to send
    asio::streambuf sb;
    std::ostream os(&sb);
    for (size_t i = 0; i < DATA_SIZE; ++i) {
      os << static_cast<char>(i);
    }

    // connect
    sock.next_layer().async_connect(
        ip::tcp::endpoint(ip::make_address("127.0.0.1"), 8080), yield);

    // send data
    asio::async_write(sock, sb.data(), yield);
    sb.consume(sb.size());

    // receive data
    auto bufs = sb.prepare(DATA_SIZE);
    auto bytes = asio::async_read(sock, bufs, yield);
    sb.commit(bytes);
  } catch (std::exception& e) {
    std::cerr << e.what() << "\n";
    return;
  }

  auto& stat = sock.get_statistics();
  std::cout << "stats:" << "\n    tx_bytes_total: "
            << stat.tx_bytes_total.load()
            << "\n    tx_bytes_compressed: " << stat.tx_bytes_compressed.load()
            << "\n    rx_bytes_total: " << stat.rx_bytes_total.load()
            << "\n    rx_bytes_compressed: " << stat.rx_bytes_compressed.load()
            << std::endl;
}

auto main() -> int
{
  asio::io_context ctx;
  echo_server s(ctx, 8080);
  asio::spawn(ctx, std::bind(example, std::ref(ctx), std::placeholders::_1));
  ctx.run();

  return 0;
}
