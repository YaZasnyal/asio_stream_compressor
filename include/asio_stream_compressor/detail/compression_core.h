#pragma once

#include <memory>

#include <zstd.h>
#include <zstd_errors.h>

#include "compressor_statistics.h"
#include "zstd_error_condition.h"

namespace asio_stream_compressor
{
namespace detail
{

struct zstd_cstream_deleter
{
  void operator()(ZSTD_CCtx* ptr)
  {
    ZSTD_freeCStream(ptr);
  }
};

struct zstd_dstream_deleter
{
  void operator()(ZSTD_DCtx* ptr)
  {
    ZSTD_freeDStream(ptr);
  }
};

using zstd_cstream_uptr = std::unique_ptr<ZSTD_CStream, zstd_cstream_deleter>;
using zstd_dstream_uptr = std::unique_ptr<ZSTD_DStream, zstd_dstream_deleter>;

template<class Executor, class Allocator>
class compression_core : public Allocator
{
public:
  using self = compression_core<Executor, Allocator>;

  compression_core(int level, const Executor& ex, const Allocator& alloc)
      : Allocator(alloc)
      , compression_level_(level)
      , cctx_(ZSTD_createCCtx())
      , dctx_(ZSTD_createDCtx())
      , pending_read_(ex)
      , pending_write_(ex)
      , input_buf_(std::numeric_limits<size_t>::max(), *this)
      , write_buf_(std::numeric_limits<size_t>::max(), *this)
  {
    pending_read_.expires_at(neg_infin());
    pending_write_.expires_at(neg_infin());
    auto ec = set_compression_level(compression_level_);
    if (ec) {
      throw system_error(ec);
    }
  }

  compression_core(self&&) = default;

  self& operator=(self&&) = default;

  error_code zstd_cctx_set_parameter(ZSTD_cParameter param, int value) noexcept
  {
    size_t status = ZSTD_CCtx_setParameter(cctx_.get(), param, value);
    return make_error_code(ZSTD_getErrorCode(status));
  }

  error_code zstd_dctx_set_parameter(ZSTD_dParameter param, int value) noexcept
  {
    size_t status = ZSTD_DCtx_setParameter(dctx_.get(), param, value);
    return make_error_code(ZSTD_getErrorCode(status));
  }

  void zstd_cctx_reset(ZSTD_ResetDirective reset) noexcept
  {
    ZSTD_CCtx_reset(cctx_.get(), reset);
  }

  void zstd_dctx_reset(ZSTD_ResetDirective reset) noexcept
  {
    ZSTD_DCtx_reset(dctx_.get(), reset);
  }

  void reset()
  {
    ZSTD_CCtx_reset(cctx_.get(),
                    ZSTD_ResetDirective::ZSTD_reset_session_and_parameters);
    ZSTD_DCtx_reset(dctx_.get(),
                    ZSTD_ResetDirective::ZSTD_reset_session_and_parameters);
    set_compression_level(compression_level_);
    input_buf_.consume(input_buf_.size());
    write_buf_.consume(write_buf_.size());
    stats_.reset();
  }

  const Allocator& get_allocator() const noexcept
  {
    return *this;
  }

  compressor_statistics& get_statistics() noexcept
  {
    return stats_;
  }

  error_code set_compression_level(int level) noexcept
  {
    return zstd_cctx_set_parameter(ZSTD_c_compressionLevel, level);
  }

  int compression_level_;  ///< @brief compression level
  zstd_cstream_uptr cctx_;  ///< @brief Compression context
  zstd_dstream_uptr dctx_;  ///< @brief Decompression context
  /** < @brief Timer used for storing queued read operations */
  timer pending_read_;
  /** @brief Timer used for storing queued write operations */
  timer pending_write_;
  /** @brief buffer for input data from next_layer */
  asio::basic_streambuf<Allocator> input_buf_;
  /** @brief buffer for output data */
  asio::basic_streambuf<Allocator> write_buf_;

  compressor_statistics stats_;
};

}  // namespace detail
}  // namespace asio_stream_compressor
