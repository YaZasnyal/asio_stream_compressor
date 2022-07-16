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
                             Handler& handler)
      : stream_(stream)
      , core_(core)
      , buffers_(buffers)
      , handler_(handler)
  {
  }

  async_write_some_operation(self&& o)
      : stream_(o.stream_)
      , core_(o.core_)
      , buffers_(o.buffers_)
      , handler_(std::move(o.handler_))
      , state_(o.state_)
      , ec_(o.ec_)
  {
  }

  void operator()(error_code ec,
                  std::size_t /*bytes_transferred*/ = std::size_t(0))
  {
    do {
      switch (state_) {
        case state::initial: {
          state_ = state::lock_next_layer;
          break;
        }

        case state::lock_next_layer: {
          if (expiry(core_.pending_write_) == neg_infin()) {
            core_.pending_write_.expires_at(pos_infin());
            state_ = state::encode_data;
            break;
          } else {
            // wait untill another read_some operation is finished
            core_.pending_write_.async_wait(std::move(*this));
            return;
          }
        }

        case state::encode_data: {
          encode_data();
          if (ec_) {
            core_.pending_write_.expires_at(neg_infin());
            break;
          }

          state_ = state::send_data;
          break;
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
          core_.stats_.tx_bytes_total.fetch_add(buffers_.size(),
                                                std::memory_order_relaxed);
          core_.stats_.tx_bytes_compressed.fetch_add(core_.write_buf_.size(),
                                                     std::memory_order_relaxed);
          handler_(ec_, buffers_.size());
          return;
        }
      }
    } while (!ec);

    handler_(ec_, 0);
  }

private:
  void encode_data()
  {
    for (auto& in : buffers_) {
      ZSTD_inBuffer in_buf {in.data(), in.size(), 0};

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

    // flush remainig
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
    auto buf = core_.write_buf_.prepare(ZSTD_DStreamOutSize()).begin();

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

  executor_type get_executor() const noexcept { return stream_.get_executor(); }

  template<class Handler, class ConstBufferSequence>
  void operator()(Handler&& handler, const ConstBufferSequence& buffers) const
  {
    using write_op =
        async_write_some_operation<std::decay_t<decltype(stream_)>,
                                   std::decay_t<decltype(core_)>,
                                   std::decay_t<decltype(handler)>,
                                   std::decay_t<decltype(buffers)>>;
    write_op(stream_, core_, buffers, handler)(error_code(), 0);
  }

private:
  Stream& stream_;
  Core& core_;
};

}  // namespace detail
}  // namespace asio_stream_compressor
