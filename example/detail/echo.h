#pragma once

#include <functional>

#include <asio_stream_compressor/asio_stream_compressor.h>
#include <boost/asio/ip/tcp.hpp>

using asio_stream_compressor::asio::ip::tcp;

class session
{
public:
  session(asio_stream_compressor::asio::io_context& io_service)
      : socket_(io_service)
  {
  }

  tcp::socket& socket() { return socket_; }

  void start()
  {
    socket_.async_read_some(
        asio_stream_compressor::asio::buffer(data_, max_length),
        std::bind(&session::handle_read,
                  this,
                  std::placeholders::_1,
                  std::placeholders::_2));
  }

  void handle_read(const asio_stream_compressor::error_code& error,
                   size_t bytes_transferred)
  {
    if (!error) {
      asio_stream_compressor::asio::async_write(
          socket_,
          asio_stream_compressor::asio::buffer(data_, bytes_transferred),
          std::bind(&session::handle_write, this, std::placeholders::_1));
    } else {
      delete this;
    }
  }

  void handle_write(const asio_stream_compressor::error_code& error)
  {
    if (!error) {
      socket_.async_read_some(
          asio_stream_compressor::asio::buffer(data_, max_length),
          std::bind(&session::handle_read,
                    this,
                    std::placeholders::_1,
                    std::placeholders::_2));
    } else {
      delete this;
    }
  }

private:
  tcp::socket socket_;
  enum
  {
    max_length = 1024
  };
  char data_[max_length];
};

class echo_server
{
public:
  echo_server(asio_stream_compressor::asio::io_context& io_context, short port)
      : io_context_(io_context)
      , acceptor_(io_context_, tcp::endpoint(tcp::v4(), port))
  {
    session* new_session = new session(io_context_);
    acceptor_.async_accept(new_session->socket(),
                           std::bind(&echo_server::handle_accept,
                                     this,
                                     new_session,
                                     std::placeholders::_1));
  }

  void handle_accept(session* new_session,
                     const asio_stream_compressor::error_code& error)
  {
    if (!error) {
      new_session->start();
      new_session = new session(io_context_);
      acceptor_.async_accept(new_session->socket(),
                             std::bind(&echo_server::handle_accept,
                                       this,
                                       new_session,
                                       std::placeholders::_1));
    } else {
      delete new_session;
    }
  }

private:
  asio_stream_compressor::asio::io_context& io_context_;
  tcp::acceptor acceptor_;
};
