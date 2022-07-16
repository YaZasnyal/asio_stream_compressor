#include <iostream>
#include <thread>

#include <asio_stream_compressor/asio_stream_compressor.h>
#include <boost/asio.hpp>

#include "detail/echo.h"

auto main() -> int
{
  namespace ip = asio_stream_compressor::asio::ip;

  asio_stream_compressor::asio::io_context ctx;
  echo_server s(ctx, 8080);
  asio_stream_compressor::compressor<ip::tcp::socket> sock(ctx);

  std::thread t(
      [&]()
      {
        ctx.run_for(std::chrono::seconds(1));
        sock.next_layer().close();
        ctx.run_for(std::chrono::seconds(1));
      });

  try {
    static const size_t DATA_SIZE = 65535;
    // prepare data to send
    asio_stream_compressor::asio::streambuf sb;
    std::ostream os(&sb);
    for (size_t i = 0; i < DATA_SIZE; ++i) {
      os << static_cast<char>(i);
    }

    // connect
    sock.next_layer()
        .async_connect(ip::tcp::endpoint(ip::make_address("127.0.0.1"), 8080),
                       asio_stream_compressor::asio::use_future)
        .get();

    // send data
    auto bytes = asio_stream_compressor::asio::async_write(
                     sock, sb.data(), asio_stream_compressor::asio::use_future)
                     .get();
    std::cout << "sent successfully: " << bytes << " bytes" << std::endl;
    sb.consume(sb.size());

    // recieve data
    auto bufs = sb.prepare(DATA_SIZE);
    bytes = asio_stream_compressor::asio::async_read(
                sock, bufs, asio_stream_compressor::asio::use_future)
                .get();
    sb.commit(bytes);
    std::cout << "recieved successfully: " << bytes << " bytes" << std::endl;
  } catch (std::exception& e) {
    std::cerr << e.what() << "\n";
  }

  t.join();
  auto& stat = sock.get_statistics();
  std::cout << "stats: \n"
            << "\ttx_bytes_total: " << stat.tx_bytes_total.load()
            << "\n\ttx_bytes_compressed: " << stat.tx_bytes_compressed.load()
            << "\n\trx_bytes_total: " << stat.rx_bytes_total.load()
            << "\n\trx_bytes_compressed: " << stat.rx_bytes_compressed.load()
            << std::endl;

  return 0;
}
