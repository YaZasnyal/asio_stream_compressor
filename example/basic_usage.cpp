#include <iostream>
#include <thread>
#include <chrono>

#include <asio_stream_compressor/asio_stream_compressor.h>
#include <boost/asio.hpp>

#include "detail/echo.h"

auto main() -> int
{
  namespace ip = asio_stream_compressor::asio::ip;

  asio_stream_compressor::asio::io_context ctx;
  echo_server s(ctx, 8080);
  asio_stream_compressor::compressor<ip::tcp::socket> sock(ctx, -1);

  std::thread t(
      [&]()
      {
        ctx.run_for(std::chrono::seconds(3));
        sock.next_layer().close();
        ctx.run_for(std::chrono::seconds(1));
      });
//  std::thread t2(
//      [&]()
//      {
//        ctx.run_for(std::chrono::seconds(3));
//        //sock.next_layer().close();
//        //ctx.run_for(std::chrono::seconds(1));
//      });

  try {
    static const size_t DATA_SIZE = 65535;
    // prepare data to send
    asio_stream_compressor::asio::streambuf sb;
    std::ostream os(&sb);
    for (size_t i = 0; i < DATA_SIZE; ++i) {
      os << static_cast<char>(i);
    }

    using Clock = std::chrono::steady_clock;
    auto start = Clock::now();
    auto end = Clock::now();
    // connect
    sock.next_layer()
        .async_connect(ip::tcp::endpoint(ip::make_address("127.0.0.1"), 8080),
                       asio_stream_compressor::asio::use_future)
        .get();

    size_t total = 0;
    while (true) {
      // send data
      auto bytes =
          asio_stream_compressor::asio::async_write(
              sock, sb.data(), asio_stream_compressor::asio::use_future)
              .get();
      total += bytes;
      //    std::cout << "sent successfully: " << bytes << " bytes" <<
      //    std::endl;
//      sb.consume(sb.size());

      // receive data
//      auto bufs = sb.prepare(DATA_SIZE);
//      bytes = asio_stream_compressor::asio::async_read(
//                  sock, bufs, asio_stream_compressor::asio::use_future)
//                  .get();
//      sb.commit(bytes);

      end = Clock::now();
      if (std::chrono::duration_cast<std::chrono::seconds>(end - start).count() >= 2)
        break;
    }
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end-start).count();

    std::cout << "receive successfully: " << total << " bytes over "
              << elapsed << " ms" << std::endl;
  } catch (std::exception& e) {
    std::cerr << e.what() << "\n";
  }

  t.join();
  //t2.join();
  auto& stat = sock.get_statistics();
  std::cout << "stats:"
            << "\n\ttx_bytes_total: " << stat.tx_bytes_total.load()
            << "\n\ttx_bytes_compressed: " << stat.tx_bytes_compressed.load()
            << "\n\trx_bytes_total: " << stat.rx_bytes_total.load()
            << "\n\trx_bytes_compressed: " << stat.rx_bytes_compressed.load()
            << std::endl;

  return 0;
}
