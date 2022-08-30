#include <thread>

#include <asio_stream_compressor/asio_stream_compressor.h>
#include <boost/asio.hpp>
#include <boost/asio/spawn.hpp>
#include <boost/beast/_experimental/test/stream.hpp>
#include <gtest/gtest.h>

namespace asc = asio_stream_compressor;
namespace ip = asio_stream_compressor::asio::ip;
using test_stream = boost::beast::test::stream;

class CompressorFixture : public ::testing::Test
{
public:
  inline static constexpr size_t test_timeout = 3;

  CompressorFixture()
      : ::testing::Test()
      , ctx_()
      , timer_(ctx_)
  {
  }

protected:
  void TearDown() override
  {
    if (thread_.joinable())
      thread_.join();
    GTEST_ASSERT_TRUE(finished_);
  }

  void start_ctx()
  {
    timer_.expires_from_now(boost::posix_time::seconds(test_timeout));
    timer_.async_wait(
        [&](boost::system::error_code ec)
        {
          if (!ec)
            ctx_.stop();
        });
    thread_ = std::thread([&]() { ctx_.run(); });
  }

  void test_finished()
  {
    finished_ = true;
    timer_.cancel();
    ctx_.stop();
  }

  auto use_future() -> boost::asio::use_future_t<>
  {
    return boost::asio::use_future;
  }

  boost::asio::io_context ctx_;
  boost::asio::deadline_timer timer_;
  std::thread thread_;
  /**
   * @brief finished_ - flag that indicates that test_finished() was called
   */
  bool finished_ = false;
};

TEST_F(CompressorFixture, no_connection_read)
{
  auto test = [&](auto yield)
  {
    asc::compressor<ip::tcp::socket> compressor(ctx_);
    char buf[512];
    bool exception = false;
    try {
      compressor.async_read_some(boost::asio::buffer(buf, 512), yield);
    } catch (asc::system_error ec) {
      GTEST_ASSERT_EQ(ec.code().default_error_condition().value(),
                      asc::errc::bad_file_descriptor);
      exception = true;
    }
    GTEST_ASSERT_TRUE(exception);
    test_finished();
  };
  boost::asio::spawn(test);
  start_ctx();
}

TEST_F(CompressorFixture, no_connection_read2)
{
  auto test = [&](auto yield)
  {
    asc::compressor<test_stream> reciever(ctx_);
    asc::compressor<test_stream> sender(ctx_);
    char buf[512];
    reciever.next_layer().connect(sender.next_layer());
    sender.async_write_some(boost::asio::buffer(buf, 512), yield);
    reciever.async_read_some(boost::asio::buffer(buf, 512), yield);
    test_finished();
  };
  boost::asio::spawn(test);
  start_ctx();
}

// read one big chunk
// read huge chunk (several MB)
// read multiple small chunks
// read in a single buffer
// read in multiple buffers
// read uncompressed data
// second async read
// too little bytes recieved from next_layer (write 1 byte at a time)
// connection closed

TEST(asio_stream_compressor, init_write)
{
  boost::asio::io_context ctx;
  asio_stream_compressor::compressor<ip::tcp::socket> compressor(ctx);
  char buf[512];
  compressor.async_write_some(boost::asio::buffer(buf, 512),
                              [&](auto /*ec*/, size_t /*bytes*/) {

                              });
}

// write one bug buffer
// write multiple buffers
// connection closed
