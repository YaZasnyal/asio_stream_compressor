#pragma once

#include <type_traits>

#include "detail/read_operation.h"
#include "detail/write_operation.h"

namespace asio_stream_compressor
{

/**
 * @brief The Compressor class is used to transparently compress
 * and decompress data before sending or receiving it from the
 * underlying stream.
 *
 * Example:
 * @code
 * namespace ip = boost::asio::ip;
 * using error_code = boost::system::error_code;
 *
 * boost::asio::io_context ctx;
 * std::thread t([&]() {ctx.run_for(std::chrono::seconds(5));});
 *
 * asio_compressor::compressor<ip::tcp::socket> sock(ctx);
 * sock.next_layer().async_connect(
 *             ip::tcp::endpoint(ip::make_address("127.0.0.1"), 8080),
 *             boost::asio::use_future).get();
 * auto bytes = sock.async_write_some(boost::asio::buffer("hello world!", 13),
 *                                    boost::asio::use_future).get();
 * std::cout << "sent successfully: " << bytes << " bytes" << std::endl;
 * t.join();
 * @endcode
 *
 * Implements traits: AsyncReadStream, AsyncWriteStream
 */
template<class Stream, class Allocator = std::allocator<char>>
class compressor
{
public:
  using self = compressor<Stream, Allocator>;

  /// The type of the next layer.
  using next_layer_type = typename std::remove_reference<Stream>::type;
  /// The type of the executor associated with the object.
  using executor_type = typename next_layer_type::executor_type;

  /**
   * @brief compressor - constructs new compressor with underlying layers
   * @param ex - executor object
   * @param alloc - custom allocator object
   *
   * @throws system_error with zstd exception if level is incorrect
   */
  template<typename Executor>
  compressor(Executor& ex, const Allocator& alloc = Allocator()) noexcept(false)
      : next_layer_(Stream(ex))
      , core_(ZSTD_defaultCLevel(), next_layer_.get_executor(), alloc)
  {
  }

  /**
   * @brief compressor - constructs new compressor with underlying layers
   * @param ex - executor object
   * @param level - compression level
   * @param alloc - custom allocator object
   *
   * @throws system_error with zstd exception if level is incorrect
   */
  template<typename Executor>
  compressor(Executor& ex,
             int level,
             const Allocator& alloc = Allocator()) noexcept(false)
      : next_layer_(Stream(ex))
      , core_(level, next_layer_.get_executor(), alloc)
  {
  }

  /**
   * @brief compressor - constructs new compressor
   * @param arg - object that implements AsyncReadStream and AsyncWriteStream
   * traits
   * @param alloc - custom allocator
   *
   * @throws system_error with zstd exception if level is incorrect
   */
  compressor(Stream&& next_layer,
             const Allocator& alloc = Allocator()) noexcept(false)
      : next_layer_(std::move(next_layer))
      , core_(ZSTD_defaultCLevel(), next_layer_.get_executor(), alloc)
  {
  }

  /**
   * @brief compressor - constructs new compressor
   * @param arg - object that implements AsyncReadStream and AsyncWriteStream
   * traits
   * @param level - compression level
   * @param alloc - custom allocator
   *
   * @throws system_error with zstd exception if level is incorrect
   */
  compressor(Stream&& next_layer,
             int level = ZSTD_defaultCLevel(),
             const Allocator& alloc = Allocator()) noexcept(false)
      : next_layer_(std::move(next_layer))
      , core_(level, next_layer_.get_executor(), alloc)
  {
  }

  compressor(const self&) = delete;

  compressor(self&&) = default;

  self& operator=(const self&) = delete;

  self& operator=(self&&) = default;

  /**
   * @brief async_read_some starts asynchronous read operation
   *
   * @param buffers - The buffers into which the data will be read. Although the
   * buffers object may be copied as necessary, ownership of the underlying
   * buffers is retained by the caller, which must guarantee that they remain
   * valid until the completion handler is called.
   * @param token - The @ref completion_token that will be used to produce a
   * completion handler, which will be called when the read completes.
   *
   * @tparam MutableBufferSequence
   * @tparam ReadToken - completion handler with signature @code void
   * (error_code, std::size_t) @endcode
   *
   * Regardless of whether the asynchronous operation completes immediately or
   * not, the completion handler will not be invoked from within this function.
   * On immediate completion, invocation of the handler will be performed in a
   * manner equivalent to using boost::asio::post().
   *
   * If this method returns an error it is usually impossible to continue
   * operation so reset() should be called to clear internal state of the
   * compressor.
   *
   */
  template<typename MutableBufferSequence,
           typename ReadToken =
               typename asio::default_completion_token<executor_type>::type>
  auto async_read_some(
      const MutableBufferSequence& buffers,
      ReadToken&& token =
          typename asio::default_completion_token<executor_type>::type())
  {
    return asio::async_initiate<ReadToken, void(error_code, std::size_t)>(
        detail::initiate_async_read_some<self, decltype(core_)>(*this, core_),
        token,
        buffers);
  }

  /**
   * @brief async_write_some
   * @param buffers - The buffers from which the data will be read. Although the
   * buffers object may be copied as necessary, ownership of the underlying
   * buffers is retained by the caller, which must guarantee that they remain
   * valid until the completion handler is called.
   * @param token The @ref completion_token that will be used to produce a
   * completion handler, which will be called when the read completes.
   *
   * @tparam MutableBufferSequence
   * @tparam ReadToken - completion handler with signature @code void
   * (error_code, std::size_t) @endcode
   *
   * Regardless of whether the asynchronous operation completes immediately or
   * not, the completion handler will not be invoked from within this function.
   * On immediate completion, invocation of the handler will be performed in a
   * manner equivalent to using boost::asio::post().
   *
   * If this method returns an error it is usually impossible to continue
   * operation so reset() should be called to clear internal state of the
   * compressor.
   *
   * @warning number of bytes in the callback will be equal to size of the
   * provided buffers or 0 if not all of them could be transferred. This is
   * because of difficulties of mapping provided byted to compressed bytes.
   */
  template<typename ConstBufferSequence,
           typename WriteToken =
               typename asio::default_completion_token<executor_type>::type>
  auto async_write_some(
      const ConstBufferSequence& buffers,
      WriteToken&& token =
          typename asio::default_completion_token<executor_type>::type())
  {
    return asio::async_initiate<WriteToken, void(error_code, std::size_t)>(
        detail::initiate_async_write_some<self, decltype(core_)>(*this, core_),
        token,
        buffers);
  }

  /**
   * @brief next_layer returns next layer in the stack of stream layers.
   */
  next_layer_type& next_layer()
  {
    return next_layer_;
  }

  /**
   * @brief get_executor obtains the executor object that the stream uses
   * to run asynchronous operations
   */
  executor_type get_executor() const noexcept
  {
    return next_layer_.get_executor();
  }

  /**
   * @brief zstd_cctx_set_parameter - sets encoder context parameter
   * @param param - param listed in ZSTD_cParameter
   * @param value - value that is in range of ZSTD_cParam_getBounds()
   * @return error_code with zstd_error_category
   *
   * @warning calling this method after async_read/write_some may cause
   * undefined behavior. Use reset() to reset internal context and configure
   * context again.
   *
   * Example:
   * @endcode
   * if (auto ec = zstd_cctx_set_parameter(ZSTD_c_compressionLevel, 1000); ec)
   * {
   *   // process error
   * }
   * @endcode
   */
  error_code zstd_cctx_set_parameter(ZSTD_cParameter param, int value) noexcept
  {
    return core_.zstd_cctx_set_parameter(param, value);
  }

  /**
   * @brief zstd_dctx_set_parameter
   * @param param - param listed in ZSTD_cParameter
   * @param value - value that is in range of ZSTD_cParam_getBounds()
   * @return error_code with zstd_error_category
   */
  error_code zstd_dctx_set_parameter(ZSTD_dParameter param, int value) noexcept
  {
    return core_.zstd_dctx_set_parameter(param, value);
  }

  /**
   * @brief reset - resets internal structures
   *
   * This method is used to reset compressor after failures. This will
   * reset both encoder and decoder contexts and drop any remaining data
   * as if ZSTD_ResetDirective::ZSTD_reset_session_and_parameters was used.
   * This will also reset compression level to the one provided in the
   * constructor or default if it wasn't.
   *
   * @warning It is unsafe to call this function if there is an active
   * asynchronous operation in progress.
   */
  void reset()
  {
    core_.reset();
  }

  /**
   * @brief get_allocator - get compressor's allocator
   */
  const Allocator& get_allocator() const noexcept
  {
    return core_.get_allocator();
  }

  /**
   * @brief get_statistics - returns compressor's statistics
   * @return compressor_statistics object with read/write stats
   *
   * This method is thread safe
   */
  compressor_statistics& get_statistics() noexcept
  {
    return core_.get_statistics();
  }

private:
  Stream next_layer_;
  detail::compression_core<executor_type, Allocator> core_;
};

}  // namespace asio_stream_compressor
