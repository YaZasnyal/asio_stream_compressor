#pragma once

#include <atomic>

namespace asio_stream_compressor
{

/**
 * @brief The compressor_statistics class holds stats.
 *
 * Example:
 * @code
 * auto& stats = compressor.get_statistics();
 * fmt::print("Total bytes written: {}\n",
 *            stats.tx_bytes_total.load(std::memory_order_relaxed));
 * @endcode
 */
class compressor_statistics
{
public:
  using value_type = size_t;
  using stat_type = std::atomic<value_type>;

  /**
   * @brief The value_slice struct is used to return values from reset() method.
   */
  struct value_slice
  {
    value_type tx_bytes_total = 0;
    value_type tx_bytes_compressed = 0;
    value_type rx_bytes_total = 0;
    value_type rx_bytes_compressed = 0;
  };

  /**
   * @brief reset - resets all counters back to zero and returns then as a new
   * object.
   */
  value_slice reset() noexcept
  {
    static const auto RELAXED = std::memory_order::memory_order_relaxed;
    value_slice slice;
    slice.tx_bytes_total = tx_bytes_total.exchange(0, RELAXED);
    slice.tx_bytes_compressed = tx_bytes_compressed.exchange(0, RELAXED);
    slice.rx_bytes_total = rx_bytes_total.exchange(0, RELAXED);
    slice.rx_bytes_compressed = rx_bytes_compressed.exchange(0, RELAXED);
    return slice;
  }

  /**
   * @brief tx_bytes_total - total number of bytes passed to the
   * async_write_some() method.
   */
  stat_type tx_bytes_total = ATOMIC_VAR_INIT(0);
  /**
   * @brief tx_bytes_compressed - total number of bytes sent to to the
   * underlying stream (usually socket) through async_write_some()
   */
  stat_type tx_bytes_compressed = ATOMIC_VAR_INIT(0);
  /**
   * @brief rx_bytes_total - total number of bytes written to the buffers that
   * were provided to the async_read_some() method.
   *
   * @note There can be more bytes ready to be delivered or bytes that cannot be
   * decoded yet.
   */
  stat_type rx_bytes_total = ATOMIC_VAR_INIT(0);
  /**
   * @brief rx_bytes_compressed - total number of bytes received from the
   * underlying stream (usually socket).
   *
   * @note There can be more bytes ready to be delivered.
   */
  stat_type rx_bytes_compressed = ATOMIC_VAR_INIT(0);
};

}  // namespace asio_stream_compressor
