
#include <wsserver.hpp>
#include <server_ws.hpp>

using WsServer = SimpleWeb::SocketServer<SimpleWeb::WS>;

static WsServer server;
static std::thread thread;

void broadcast(std::string in) {
	for (std::shared_ptr<WsServer::Connection> conn: server.get_connections()) {
		conn->send(in);
	}
}

void startWebsocketServer(int port, int threads) {
	server.config.port = port;
	server.config.thread_pool_size = threads;
	
	auto &root = server.endpoint[".*"];
	
	root.on_open = [](std::shared_ptr<WsServer::Connection> conn) {
		std::cout << "WS Accepted" << std::endl;
		handleAccepted([conn](std::string msg) {
			conn->send(msg);
		});
	};
	
	root.on_message = [](std::shared_ptr<WsServer::Connection> conn, std::shared_ptr<WsServer::InMessage> msg) {
		std::cout << "WS Closed" << std::endl;
		handleMessage([conn](std::string resp) {
			conn->send(resp);
		}, msg->string());
	};
	
	thread = std::thread([]() {
		server.start([](unsigned short port) {
			std::cout << "Websocket server accepting on " << std::to_string(port) << std::endl;
		});
	});
}

void stopWebsocketServer() {
	server.stop();
	thread.join();
}
