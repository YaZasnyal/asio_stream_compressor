# asio_stream_compressor

`asio_stream_compressor` is a header-only library that allows used to add ZSTD
compression support for the asynchronous streams with easy to use wrapper.

One minute example:
```cpp
#include <boost/asio.hpp>
#include <boost/asio/spawn.hpp>
#include <asio_stream_compressor/asio_stream_compressor.h>

auto main(int, char**) -> int
{
  namespace ip = boost::asio::ip;
  asio_stream_compressor::asio::io_context ctx;
  boost::asio::spawn(
      [&](auto yield)
      {
        std::string message = "Hello world!";
        asio_stream_compressor::compressor<ip::tcp::socket> sock(ctx);

        sock.next_layer().async_connect(
            ip::tcp::endpoint(ip::make_address("127.0.0.1"), 8080), yield);
        boost::asio::async_write(
            sock, boost::asio::buffer(message.data(), message.size()), yield);
        boost::asio::async_read(
            sock, boost::asio::buffer(message.data(), message.size()), yield);
      });
  ctx.run();
  return 0;
}
```

# Requirements

* C++14 compatible compiler
* CMake >= 3.14
* Boost >= 1.70.0 or ASIO (tested with 1.22.1)
* zstd >= 1.40.0

# Usage

The library can be added to a CMake project using either `add_subdirectory` or `find_package`. Once set up, include the individual headers from the asio_stream_compressor/ directory or the convenience header:

```cpp
#include <asio_stream_compressor/asio_stream_compressor.h>
```

<details><summary><b>As a subdirectory</b></summary>
Add this repository to your project as submodule or using FetchContent

Using `FetchContent`:
```cmake
include(FetchContent)
FetchContent_Declare(external_compressor
    GIT_REPOSITORY https://github.com/YaZasnyal/asio_stream_compressor.git
    GIT_TAG <tag or branch>
    )
FetchContent_Populate(external_compressor)
add_subdirectory(${external_compressor_SOURCE_DIR} ${external_compressor_BINARY_DIR})
```

Using `submodules`:
```sh
git submodule add https://github.com/YaZasnyal/asio_stream_compressor.git <path>
```
```cmake
add_subdirectory(<path>)
```
</details>

<details><summary><b>As a CMake package</b></summary>
Clone the repository and install it.

```sh
cmake -B build -DCMAKE_INSTALL_PREFIX=/desired/installation/directory .
cmake --build build --target install
```

Locate it and link it to your target.
```cmake
find_package(asio_stream_compressor)
target_link_libraries(your_app PUBLIC asio_stream_compressor::asio_stream_compressor)
```
</details>

# CMake Options
There are some options that allow you to change the behavior of the library. This options
should be set before calling `add_subdirectory` or `find_package`.

* `asio_stream_compressor_ASIO_STANDALONE` - tells library to use standalone asio
instead of Boost.Asio
* `asio_stream_compressor_ZSTD_SHARED` - tells library to use shared version of zstd
library in its public interface.

# Building and installing

See the [BUILDING](BUILDING.md) document.

# Contributing

See the [CONTRIBUTING](CONTRIBUTING.md) document.
