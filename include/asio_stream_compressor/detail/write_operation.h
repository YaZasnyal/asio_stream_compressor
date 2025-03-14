#pragma once

#include "compression_core.h"

namespace asio_stream_compressor
{
namespace detail
{
template<class Stream, class Core, class Handler, class ConstBufferSequence>
class async_write_some_operation
{
public:
  using self =
      async_write_some_operation<Stream, Core, Handler, ConstBufferSequence>;

  async_write_some_operation(Stream& stream,
                             Core& core,
                             const ConstBufferSequence& buffers,
                             Handler&& handler)
      : stream_(stream)
      , core_(core)
      , buffers_(buffers)
      , handler_(std::forward<decltype(handler)>(handler))
  {
  }

  async_write_some_operation(self&& o)
      : stream_(o.stream_)
      , core_(o.core_)
      , buffers_(o.buffers_)
      , input_length_(o.input_length_)
      , handler_(std::move(o.handler_))
      , state_(o.state_)
      , ec_(o.ec_)
  {
  }

  void operator()(error_code ec,
                  std::size_t /*bytes_transferred*/ = std::size_t(0),
                  bool start = 0)
  {
    switch (state_) {
      case state::initial: {
        state_ = state::lock_next_layer;
        [[fallthrough]];
      }

      case state::lock_next_layer: {
        if (expiry(core_.pending_write_) == neg_infin()) {
          core_.pending_write_.expires_at(pos_infin());
          state_ = state::encode_data;
        } else {
          // wait until another read_some operation is finished
          core_.pending_write_.async_wait(std::move(*this));
          return;
        }
        [[fallthrough]];
      }

      case state::encode_data: {
        encode_data();
        if (ec_) {
          core_.pending_write_.expires_at(neg_infin());
          break;
        }

        state_ = state::send_data;
        [[fallthrough]];
      }

      case state::send_data: {
        state_ = state::pass_data_to_handler;
        asio::async_write(
            stream_.next_layer(), core_.write_buf_.data(), std::move(*this));
        return;
      }

      case state::pass_data_to_handler: {
        if (ec) {
          ec_ = ec;
          core_.pending_write_.expires_at(neg_infin());
          break;
        }

        core_.pending_write_.expires_at(neg_infin());
        core_.stats_.tx_bytes_total.fetch_add(input_length_,
                                              std::memory_order_relaxed);
        core_.stats_.tx_bytes_compressed.fetch_add(core_.write_buf_.size(),
                                                   std::memory_order_relaxed);
        core_.write_buf_.consume(core_.write_buf_.size());
        handler_(ec_, input_length_);
        return;
      }
    }

    // if this function is called directly from initiate function  we
    // should call handler_ as if it was post()'ed. So we begin a zero
    // length async read operation.
    if (start) {
      auto bufs = core_.write_buf_.prepare(0);
      stream_.next_layer().async_read_some(bufs, std::move(*this));
      return;
    }
    handler_(ec_, 0);
  }

private:
  void encode_data()
  {
    auto buffers_begin = asio::buffer_sequence_begin(buffers_);
    auto buffers_end   = asio::buffer_sequence_end(buffers_);
    for (auto it = buffers_begin; it != buffers_end; ++it) {
      auto& in = *it;
      ZSTD_inBuffer in_buf {in.data(), in.size(), 0};
      input_length_ += in.size();

      while (in_buf.pos != in_buf.size) {
        ZSTD_outBuffer out_buf = get_free_buffer();
        size_t compress_result =
            ZSTD_compressStream2(core_.cctx_.get(),
                                 &out_buf,
                                 &in_buf,
                                 ZSTD_EndDirective::ZSTD_e_continue);
        if (check_set_error(compress_result))
          return;
        core_.write_buf_.commit(out_buf.pos);
      }
    }

    // flush remaining
    size_t compress_result;
    ZSTD_inBuffer in_buf {nullptr, 0, 0};
    do {
      ZSTD_outBuffer out_buf = get_free_buffer();
      compress_result = ZSTD_compressStream2(core_.cctx_.get(),
                                             &out_buf,
                                             &in_buf,
                                             ZSTD_EndDirective::ZSTD_e_flush);
      core_.write_buf_.commit(out_buf.pos);
      if (check_set_error(compress_result))
        return;
    } while (compress_result != 0);
  }

  ZSTD_outBuffer get_free_buffer()
  {
    auto buf_sequence = core_.write_buf_.prepare(ZSTD_DStreamOutSize());
    auto buf = asio::buffer_sequence_begin(buf_sequence);
    return ZSTD_outBuffer {buf->data(), buf->size(), 0};
  }

  bool check_set_error(size_t compress_result)
  {
    if (ZSTD_isError(compress_result)) {
      ec_ = make_error_code(ZSTD_getErrorCode(compress_result));
      return true;
    }
    return false;
  }

  enum class state
  {
    initial,
    lock_next_layer,
    encode_data,
    send_data,
    pass_data_to_handler,
  };

  Stream& stream_;
  Core& core_;
  ConstBufferSequence buffers_;
  size_t input_length_ = 0;
  Handler handler_;

  state state_ = state::initial;
  error_code ec_;
};

template<typename Stream, class Core>
class initiate_async_write_some
{
public:
  using executor_type = typename Stream::executor_type;

  initiate_async_write_some(Stream& stream, Core& core)
      : stream_(stream)
      , core_(core)
  {
  }
  initiate_async_write_some(const initiate_async_write_some&) = default;
  initiate_async_write_some(initiate_async_write_some&&) = default;

  executor_type get_executor() const noexcept
  {
    return stream_.get_executor();
  }

  template<class Handler, class ConstBufferSequence>
  void operator()(Handler&& handler, const ConstBufferSequence& buffers) const
  {
    async_write_some_operation(
        stream_, core_, buffers, std::forward<decltype(handler)>(handler))(
        error_code(), 0, true);
  }

private:
  Stream& stream_;
  Core& core_;
};

}  // namespace detail
}  // namespace asio_stream_compressor
