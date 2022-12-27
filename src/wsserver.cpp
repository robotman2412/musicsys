//
// Copyright (c) 2016-2019 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/boostorg/beast
//

//------------------------------------------------------------------------------
//
// Example: WebSocket server, asynchronous
//
//------------------------------------------------------------------------------

#include "wsserver.hpp"

#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/asio/strand.hpp>
#include <algorithm>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <vector>

namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace websocket = beast::websocket; // from <boost/beast/websocket.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

//------------------------------------------------------------------------------

// Report a failure
static void fail(beast::error_code ec, char const* what)
{
	std::cerr << what << ": " << ec.message() << "\n";
}

class WsSession;

static std::vector<std::shared_ptr<WsSession>> sessions;

// Echoes back all received WebSocket messages
class WsSession : public std::enable_shared_from_this<WsSession>
{
public:
	websocket::stream<beast::tcp_stream> ws_;
	beast::flat_buffer sendBuffer;
	beast::flat_buffer recvBuffer;

	// Take ownership of the socket
	explicit
	WsSession(tcp::socket&& socket)
		: ws_(std::move(socket))
	{
		ws_.text(true);
	}

	// Start the asynchronous operation
	void
	run()
	{
		// Set suggested timeout settings for the websocket
		ws_.set_option(
			websocket::stream_base::timeout::suggested(
				beast::role_type::server));

		// Set a decorator to change the Server of the handshake
		ws_.set_option(websocket::stream_base::decorator(
			[](websocket::response_type& res)
			{
				res.set(http::field::server,
					std::string(BOOST_BEAST_VERSION_STRING) +
						" websocket-server-async");
			}));

		// Accept the websocket handshake
		ws_.async_accept(
			beast::bind_front_handler(
				&WsSession::on_accept,
				shared_from_this()));
	}

	void
	on_accept(beast::error_code ec)
	{
		if(ec) {
			fail(ec, "accept");
			for (auto iter = sessions.begin(); iter != sessions.end(); iter++) {
				if (iter->get() == this) {
					sessions.erase(iter, sessions.end());
					break;
				}
			}
			return;
		}
		
		handleAccepted([this](std::string msg) -> void {
			ws_.text(true);
			beast::error_code ec;
			beast::flat_buffer buf;
			const char *raw = msg.c_str();
			size_t len = strlen(raw);
			buf.reserve(len);
			auto w = buf.prepare(len);
			char *dst = (char *) w.data();
			memcpy(dst, raw, len);
			buf.commit(len);
			size_t size = ws_.write(buf.data(), ec);
		});
		
		// Read a message
		do_read();
	}

	void
	do_read()
	{
		// Read a message into our buffer
		ws_.async_read(
			recvBuffer,
			beast::bind_front_handler(
				&WsSession::on_read,
				shared_from_this()));
	}

	void
	on_read(
		beast::error_code ec,
		std::size_t bytes_transferred)
	{
		boost::ignore_unused(bytes_transferred);

		// This indicates that the WsSession was closed
		if(ec == websocket::error::closed) {
			std::cout << "WS closed" << std::endl;
			for (auto iter = sessions.begin(); iter != sessions.end(); iter++) {
				if (iter->get() == this) {
					sessions.erase(iter, sessions.end());
					break;
				}
			}
			return;
		}

		if(ec) {
			fail(ec, "read");
			for (auto iter = sessions.begin(); iter != sessions.end(); iter++) {
				if (iter->get() == this) {
					sessions.erase(iter, sessions.end());
					break;
				}
			}
			return;
		}

		// Echo the message
		handleMessage([this](std::string msg) -> void {
			ws_.text(true);
			beast::error_code ec;
			beast::flat_buffer buf;
			const char *raw = msg.c_str();
			size_t len = strlen(raw);
			buf.reserve(len);
			auto w = buf.prepare(len);
			char *dst = (char *) w.data();
			memcpy(dst, raw, len);
			buf.commit(len);
			size_t size = ws_.write(buf.data(), ec);
		}, beast::buffers_to_string(recvBuffer.data()));
		// ws_.text(ws_.got_text());
		// ws_.async_write(
		//     buffer_.data(),
		//     beast::bind_front_handler(
		//         &WsSession::on_write,
		//         shared_from_this()));
		recvBuffer.clear();
		do_read();
	}

	void
	on_write(
		beast::error_code ec,
		std::size_t bytes_transferred)
	{
		boost::ignore_unused(bytes_transferred);

		if(ec) {
			return fail(ec, "write");
			for (auto iter = sessions.begin(); iter != sessions.end(); iter++) {
				if (iter->get() == this) {
					sessions.erase(iter, sessions.end());
					break;
				}
			}
		}

		// Clear the buffer
		sendBuffer.consume(sendBuffer.size());
	}
};

//------------------------------------------------------------------------------

// Accepts incoming connections and launches the WsSessions
class WsListener : public std::enable_shared_from_this<WsListener>
{
	net::io_context& ioc_;
	tcp::acceptor acceptor_;

public:
	WsListener(
		net::io_context& ioc,
		tcp::endpoint endpoint)
		: ioc_(ioc)
		, acceptor_(ioc)
	{
		beast::error_code ec;

		// Open the acceptor
		acceptor_.open(endpoint.protocol(), ec);
		if(ec)
		{
			fail(ec, "open");
			return;
		}

		// Allow address reuse
		acceptor_.set_option(net::socket_base::reuse_address(true), ec);
		if(ec)
		{
			fail(ec, "set_option");
			return;
		}

		// Bind to the server address
		acceptor_.bind(endpoint, ec);
		if(ec)
		{
			fail(ec, "bind");
			return;
		}

		// Start listening for connections
		acceptor_.listen(
			net::socket_base::max_listen_connections, ec);
		if(ec)
		{
			fail(ec, "listen");
			return;
		}
	}

	// Start accepting incoming connections
	void
	run()
	{
		do_accept();
	}

private:
	void
	do_accept()
	{
		// The new connection gets its own strand
		acceptor_.async_accept(
			net::make_strand(ioc_),
			beast::bind_front_handler(
				&WsListener::on_accept,
				shared_from_this()));
	}

	void
	on_accept(beast::error_code ec, tcp::socket socket)
	{
		if(ec)
		{
			fail(ec, "accept");
		}
		else
		{
			// Create the WsSession and run it
			std::shared_ptr<WsSession> ptr = std::make_shared<WsSession>(std::move(socket));
			sessions.push_back(ptr);
			ptr->run();
		}

		// Accept another connection
		do_accept();
	}
};

//------------------------------------------------------------------------------

static net::io_context *ioc;
static std::vector<std::thread> threadList;
static std::shared_ptr<WsListener> listener;
static std::shared_ptr<std::string const> docRoot;

void broadcast(std::string in) {
	beast::flat_buffer buf;
	for (auto session: sessions) {
		try {
			const char *raw = in.c_str();
			size_t len = strlen(raw);
			buf.reserve(len);
			auto w = buf.prepare(len);
			char *dst = (char *) w.data();
			memcpy(dst, raw, len);
			buf.commit(len);
			// session->ws_.write(buf.data());
			session->ws_.async_write(buf.data(), [](auto a, auto b) {
				// LOL
			});
			buf.consume(buf.max_size());
		} catch (boost::system::system_error) {
			// LoL
		}
	}
}

void startWebsocketServer(int port, int threads) {
	// Check command line arguments.
	auto const address = net::ip::make_address("0.0.0.0");

	// The io_context is required for all I/O
	ioc = new net::io_context{threads};

	// Create and launch a listening port
	listener = std::make_shared<WsListener>(*ioc, tcp::endpoint{address, port});
	listener->run();

	// Run the I/O service on the requested number of threads
	threadList.reserve(threads);
	for(auto i = threads; i > 0; --i)
		threadList.emplace_back([] {
			ioc->run();
		});
}

void stopWebsocketServer() {
	ioc->stop();
	for (auto &thread: threadList) {
		thread.join();
	}
}
