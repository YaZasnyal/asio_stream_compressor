#include <asio_stream_compressor/asio_stream_compressor.h>
#include <boost/asio.hpp>
#include <gtest/gtest.h>

namespace ip = asio_stream_compressor::asio::ip;

// Tests in this file are mostly used to check that different combinations of
// methods are compiling correctly when instantiated.

/**
 * @brief basic_constructor testing basic constructor that creates all
 * underlying layers
 */
TEST(api, basic_constructor)
{
  boost::asio::io_context ctx;
  asio_stream_compressor::compressor<ip::tcp::socket> compressor(ctx);
  compressor.get_allocator();
}

/**
 * @brief basic_constructor testing basic constructor that creates all
 * underlying layers with provided custom allocator
 */
TEST(api, basic_constructor_with_alloc)
{
  boost::asio::io_context ctx;
  asio_stream_compressor::compressor<ip::tcp::socket> compressor(
      ctx, std::allocator<char>());
  compressor.get_allocator();
}

/**
 * @brief basic_constructor testing basic constructor that creates all
 * underlying layers and sets compression level
 */
TEST(api, basic_constructor_with_level_and)
{
  boost::asio::io_context ctx;
  asio_stream_compressor::compressor<ip::tcp::socket> compressor(ctx, 1);
  compressor.get_allocator();
}

/**
 * @brief basic_constructor testing basic constructor that creates all
 * underlying layers with provided custom allocator and sets compression level
 */
TEST(api, basic_constructor_with_level_and_alloc)
{
  boost::asio::io_context ctx;
  asio_stream_compressor::compressor<ip::tcp::socket> compressor(
      ctx, 1, std::allocator<char>());
  compressor.get_allocator();
}

/**
 * @brief checks if next layer can be accessed
 */
TEST(api, next_layer_single)
{
  boost::asio::io_context ctx;
  asio_stream_compressor::compressor<ip::tcp::socket> compressor(ctx);
  auto& next_layer = compressor.next_layer();
  static_assert(std::is_same_v<decltype(next_layer), ip::tcp::socket&>);
}

/**
 * @brief checks if next layer can be accessed
 */
TEST(api, next_layer_multiple)
{
  boost::asio::io_context ctx;
  asio_stream_compressor::compressor<
      boost::asio::buffered_stream<ip::tcp::socket>>
      compressor(ctx);
  auto& next_layer = compressor.next_layer();
  static_assert(std::is_same_v<decltype(next_layer),
                               boost::asio::buffered_stream<ip::tcp::socket>&>);
}

/**
 * @brief check statistics
 */
TEST(api, get_statistics)
{
  boost::asio::io_context ctx;
  asio_stream_compressor::compressor<ip::tcp::socket> compressor(ctx);
  auto& stats = compressor.get_statistics();
  GTEST_ASSERT_EQ(stats.rx_bytes_compressed.fetch_add(1), 0);
  GTEST_ASSERT_EQ(stats.rx_bytes_total.fetch_add(1), 0);
  GTEST_ASSERT_EQ(stats.tx_bytes_compressed.fetch_add(1), 0);
  GTEST_ASSERT_EQ(stats.tx_bytes_total.fetch_add(1), 0);

  auto slice = stats.reset();
  GTEST_ASSERT_EQ(stats.rx_bytes_compressed.load(), 0);
  GTEST_ASSERT_EQ(stats.rx_bytes_total.load(), 0);
  GTEST_ASSERT_EQ(stats.tx_bytes_compressed.load(), 0);
  GTEST_ASSERT_EQ(stats.tx_bytes_total.load(), 0);
  GTEST_ASSERT_EQ(slice.rx_bytes_compressed, 1);
  GTEST_ASSERT_EQ(slice.rx_bytes_total, 1);
  GTEST_ASSERT_EQ(slice.tx_bytes_compressed, 1);
  GTEST_ASSERT_EQ(slice.tx_bytes_total, 1);

  // check reset
  GTEST_ASSERT_EQ(stats.rx_bytes_compressed.fetch_add(1), 0);
  GTEST_ASSERT_EQ(stats.rx_bytes_total.fetch_add(1), 0);
  GTEST_ASSERT_EQ(stats.tx_bytes_compressed.fetch_add(1), 0);
  GTEST_ASSERT_EQ(stats.tx_bytes_total.fetch_add(1), 0);
  compressor.reset();
  GTEST_ASSERT_EQ(stats.rx_bytes_compressed.load(), 0);
  GTEST_ASSERT_EQ(stats.rx_bytes_total.load(), 0);
  GTEST_ASSERT_EQ(stats.tx_bytes_compressed.load(), 0);
  GTEST_ASSERT_EQ(stats.tx_bytes_total.load(), 0);
}

/**
 * @brief Set CCtx option
 */
TEST(api, set_cctx_option)
{
  boost::asio::io_context ctx;
  asio_stream_compressor::compressor<ip::tcp::socket> compressor(ctx);
  auto ec = compressor.zstd_cctx_set_parameter(
      ZSTD_cParameter::ZSTD_c_compressionLevel, -1);
  GTEST_ASSERT_FALSE(ec);
}

/**
 * @brief Set CCtx option
 */
TEST(api, set_cctx_option_exception)
{
  boost::asio::io_context ctx;
  asio_stream_compressor::compressor<ip::tcp::socket> compressor(ctx);
  auto ec = compressor.zstd_cctx_set_parameter(
      ZSTD_cParameter::ZSTD_c_strategy,
      ZSTD_cParam_getBounds(ZSTD_cParameter::ZSTD_c_strategy).upperBound + 10);
  GTEST_ASSERT_TRUE(ec);
  GTEST_ASSERT_EQ(ec.category(), asio_stream_compressor::zstd_error_category());
  GTEST_ASSERT_EQ(ec.value(), ZSTD_ErrorCode::ZSTD_error_parameter_outOfBound);
}

/**
 * @brief Set DCtx option
 */
TEST(api, set_dctx_option)
{
  boost::asio::io_context ctx;
  asio_stream_compressor::compressor<ip::tcp::socket> compressor(ctx);
  auto ec = compressor.zstd_dctx_set_parameter(
      ZSTD_dParameter::ZSTD_d_windowLogMax, 0);
  GTEST_ASSERT_FALSE(ec);
}

/**
 * @brief Set DCtx option
 */
TEST(api, set_dctx_option_exception)
{
  boost::asio::io_context ctx;
  asio_stream_compressor::compressor<ip::tcp::socket> compressor(ctx);
  auto ec = compressor.zstd_dctx_set_parameter(
      ZSTD_dParameter::ZSTD_d_windowLogMax, 12345);
  GTEST_ASSERT_TRUE(ec);
  GTEST_ASSERT_EQ(ec.category(), asio_stream_compressor::zstd_error_category());
  GTEST_ASSERT_EQ(ec.value(), ZSTD_ErrorCode::ZSTD_error_parameter_outOfBound);
}
