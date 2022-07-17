#pragma once

#include "compression_core.h"

namespace asio_stream_compressor
{
namespace detail
{

template<class Stream, class Core, class Handler, class MutableBufferSequence>
class async_read_some_operation
{
public:
  using self =
      async_read_some_operation<Stream, Core, Handler, MutableBufferSequence>;

  async_read_some_operation(Stream& stream,
                            Core& core,
                            const MutableBufferSequence& buffers,
                            Handler& handler)
      : stream_(stream)
      , core_(core)
      , buffers_(buffers)
      , handler_(handler)
  {
  }

  async_read_some_operation(self&& o)
      : stream_(o.stream_)
      , core_(o.core_)
      , buffers_(o.buffers_)
      , handler_(std::move(o.handler_))
      , state_(o.state_)
      , ec_(o.ec_)
      , bytes_written_(o.bytes_written_)
  {
  }

  void operator()(error_code ec, std::size_t bytes_transferred = std::size_t(0))
  {
    do {
      switch (state_) {
        case state::initial: {
          state_ = state::lock_next_layer;
          break;
        }

        case state::lock_next_layer: {
          if (expiry(core_.pending_read_) == neg_infin()) {
            core_.pending_read_.expires_at(pos_infin());
            state_ = state::decode_data;
            break;
          } else {
            // wait until another read_some operation is finished
            core_.pending_read_.async_wait(std::move(*this));
            return;
          }
        }

        case state::read_data_from_next_layer: {
          state_ = state::decode_data;
          auto bufs = core_.input_buf_.prepare(65535);
          stream_.next_layer().async_read_some(bufs, std::move(*this));
          return;
        }

        case state::decode_data: {
          if (ec) {
            ec_ = ec;
            core_.pending_read_.expires_at(neg_infin());
            break;
          }

          if (bytes_transferred != 0) {
            core_.input_buf_.commit(bytes_transferred);
            core_.stats_.rx_bytes_compressed.fetch_add(
                bytes_transferred, std::memory_order_relaxed);
          }

          if (!decode_data()) {
            if (ec_) {
              core_.pending_read_.expires_at(neg_infin());
              break;
            }

            state_ = state::read_data_from_next_layer;
            break;
          }

          state_ = state::pass_data_to_handler;
          break;
        }

        case state::pass_data_to_handler: {
          core_.pending_read_.expires_at(neg_infin());
          core_.stats_.rx_bytes_total.fetch_add(bytes_written_,
                                                std::memory_order_relaxed);
          handler_(ec_, bytes_written_);
          return;
        }
      }
    } while (!ec_);

    handler_(ec_, 0);
  }

private:
  bool decode_data()
  {
    bool result = false;

    for (auto out = buffers_.begin(); out != buffers_.end();) {
      ZSTD_outBuffer out_buf {out->data(), out->size(), 0};

      do {
        size_t decompression_result;
        if (core_.input_buf_.size() == 0) {
          ZSTD_inBuffer in_buf {nullptr, 0, 0};
          decompression_result =
              ZSTD_decompressStream(core_.dctx_.get(), &out_buf, &in_buf);
          break;
        } else {
          auto in = core_.input_buf_.data().begin();
          ZSTD_inBuffer in_buf {in->data(), in->size(), 0};
          decompression_result =
              ZSTD_decompressStream(core_.dctx_.get(), &out_buf, &in_buf);
          core_.input_buf_.consume(in_buf.pos);
        }

        if (ZSTD_isError(decompression_result)) {
          ec_ = make_error_code(ZSTD_getErrorCode(decompression_result));
          return false;
        }
      } while (out_buf.size != out_buf.pos);

      if (out_buf.pos != 0)
        result = true;

      bytes_written_ += out_buf.pos;
      if (out_buf.size == out_buf.pos)
        ++out;
      else
        break;
    }

    return result;
  }

  enum class state
  {
    initial,
    lock_next_layer,
    read_data_from_next_layer,
    decode_data,
    pass_data_to_handler,
  };

  Stream& stream_;
  Core& core_;
  MutableBufferSequence buffers_;
  Handler handler_;

  state state_ = state::initial;
  error_code ec_;
  size_t bytes_written_ = 0;
};

template<typename Stream, class Core>
class initiate_async_read_some
{
public:
  using executor_type = typename Stream::executor_type;

  initiate_async_read_some(Stream& stream, Core& core)
      : stream_(stream)
      , core_(core)
  {
  }
  initiate_async_read_some(const initiate_async_read_some&) = default;
  initiate_async_read_some(initiate_async_read_some&&) = default;

  executor_type get_executor() const noexcept { return stream_.get_executor(); }

  template<class Handler, class MutableBufferSequence>
  void operator()(Handler&& handler, const MutableBufferSequence& buffers) const
  {
    using read_op = async_read_some_operation<std::decay_t<decltype(stream_)>,
                                              std::decay_t<decltype(core_)>,
                                              std::decay_t<decltype(handler)>,
                                              std::decay_t<decltype(buffers)>>;
    read_op(stream_, core_, buffers, handler)(error_code(), 0);
  }

private:
  Stream& stream_;
  Core& core_;
};

}  // namespace detail
}  // namespace asio_stream_compressor
