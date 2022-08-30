#include <boost/asio.hpp>
#include <asio_stream_compressor/asio_stream_compressor.h>

int main() {
    namespace ip = asio_stream_compressor::asio::ip;
	asio_stream_compressor::asio::io_context ctx;
	asio_stream_compressor::compressor<ip::tcp::socket> sock(ctx);
	
	return 0;
}
