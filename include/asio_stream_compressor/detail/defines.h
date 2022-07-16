#pragma once

#ifdef ASIO_STEREAM_COMPRESSOR_FLAVOUR_STANDALONE
#  include <system_error>

#  include <asio/steady_timer.hpp>
#  include <asio/streambuf.hpp>
#  include <asio/write.hpp>

// namespace fwd
namespace asio
{
}

namespace asio_compressor
{
namespace asio = ::asio;

using error_category = std::error_category;
using error_condition = std::error_condition;
using error_code = std::error_code;
using errc = std::errc;
using system_error = std::system_error;
using timer = asio::steady_timer;

const error_category& system_category()
{
  return std::system_category();
}

inline timer::time_point neg_infin()
{
  return (timer::time_point::min)();
}

inline timer::time_point pos_infin()
{
  return (timer::time_point::max)();
}

inline timer::time_point expiry(const timer& timer)
{
  return timer.expiry();
}

}  // namespace asio_compressor

#else

#  include <boost/asio/deadline_timer.hpp>
#  include <boost/asio/streambuf.hpp>
#  include <boost/asio/write.hpp>
#  include <boost/system/error_code.hpp>

// namespace fwd
namespace boost
{
namespace asio
{
}
}  // namespace boost

namespace asio_stream_compressor
{
namespace asio = ::boost::asio;

using error_category = boost::system::error_category;
using error_condition = boost::system::error_condition;
using error_code = boost::system::error_code;
using errc = boost::system::errc::errc_t;
using system_error = boost::system::system_error;
using timer = boost::asio::deadline_timer;

inline const error_category& system_category()
{
  return boost::system::system_category();
}

namespace detail
{

inline boost::asio::deadline_timer::time_type neg_infin()
{
  return boost::posix_time::neg_infin;
}

inline boost::asio::deadline_timer::time_type pos_infin()
{
  return boost::posix_time::pos_infin;
}

inline boost::asio::deadline_timer::time_type expiry(
    const boost::asio::deadline_timer& timer)
{
  return timer.expires_at();
}

}  // namespace detail
}  // namespace asio_stream_compressor

#endif
