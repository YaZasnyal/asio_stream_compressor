#pragma once

#include <atomic>

namespace asio_stream_compressor
{

/**
 * @brief The compressor_statistics class holds stats for byted been
 * passed to the underlying layers or recieved from them.
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
  using stat_type = std::atomic<size_t>;

  stat_type tx_bytes_total = ATOMIC_VAR_INIT(0);
  stat_type tx_bytes_compressed = ATOMIC_VAR_INIT(0);
  stat_type rx_bytes_total = ATOMIC_VAR_INIT(0);
  stat_type rx_bytes_compressed = ATOMIC_VAR_INIT(0);
};

}  // namespace asio_stream_compressor
