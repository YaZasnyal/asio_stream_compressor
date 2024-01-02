#pragma once

#include <zstd_errors.h>

#include "defines.h"

namespace asio_stream_compressor
{
/**
 * @brief The zstd_error_category class is used to distinguish
 * zstd errors.
 *
 * Zstd errors are described in ZSTD_ErrorCode enumerator
 */
class zstd_error_category_impl : public error_category
{
public:
  // error_category interface
  const char* name() const noexcept override
  {
    return "zstd_error";
  }

  std::string message(int ev) const override
  {
    return ZSTD_getErrorString(static_cast<ZSTD_ErrorCode>(ev));
  }

  error_condition default_error_condition(int ev) const noexcept override
  {
    ZSTD_ErrorCode ec = ZSTD_getErrorCode(ev);
    switch (ec) {
      case ZSTD_ErrorCode::ZSTD_error_no_error: {
        return error_condition();
      }
      case ZSTD_ErrorCode::ZSTD_error_memory_allocation: {
        return errc::not_enough_memory;
      }
      default:
        break;
    }

    return error_condition(ev, *this);
  }
};

inline const error_category& zstd_error_category()
{
  static zstd_error_category_impl instance;
  return instance;
}

inline error_code make_error_code(ZSTD_ErrorCode e)
{
  return error_code(e, zstd_error_category());
}

inline error_condition make_error_condition(ZSTD_ErrorCode e)
{
  return error_condition(e, zstd_error_category());
}

}  // namespace asio_stream_compressor

namespace std
{
template<>
struct is_error_condition_enum<ZSTD_ErrorCode> : public true_type
{
};
}  // namespace std
