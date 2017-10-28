#include "tcp_socket_connection.h"
#include "channeled_client.h"
#include "channeled_server.h"

#include <memory>
#include <iostream>
#include <string>
#include <cassert>

class TestServer : public SunNet::ChanneledServer<SunNet::TCPSocketConnection> {
protected:
	void handle_channeledclient_connect(SunNet::ChanneledSocketConnection_p client) {
		std::cout << "Client connected!" << std::endl;
	}

	void handle_poll_timeout() {
		std::cout << "Server poll timeout!" << std::endl;
	}

	void handle_server_connection_error() {
		std::cout << "SERVER ERROR!" << std::endl;
	}

	virtual void handle_channeledclient_error(SunNet::ChanneledSocketConnection_p client) {
		std::cout << "CLIENT ERROR!" << std::endl;
	}

	virtual void handleClientDisconnect(SunNet::ChanneledSocketConnection_p client) {
		std::cout << "CLIENT DISCONNECT!" << std::endl;
	}

	virtual void handle_server_disconnect() {
		std::cout << "SERVER DISCONNECT" << std::endl;
	}

public:
	TestServer(std::string address, std::string port, int queue_size) :
		ChanneledServer<SunNet::TCPSocketConnection>(address, port, queue_size, 50) {}
};

class TestClient : public SunNet::ChanneledClient<SunNet::TCPSocketConnection> {
public:
	TestClient(int poll_timeout) : ChanneledClient<SunNet::TCPSocketConnection>(poll_timeout) {}

	void handle_poll_timeout() {
		std::cout << "Client poll timeout" << std::endl;
	}

	void handle_client_disconnect() {
		std::cout << "Client connection disconnect" << std::endl;
	}

	void handle_client_error() {
		std::cout << "CLIENT ERROR!" << std::endl;
	}
};

class GeneralParent {
public:
	int msg;
	int msg2;
};

class GeneralMessage : public GeneralParent {
public:
	int msg2;
};

void server_callback(SunNet::ChanneledSocketConnection_p sender, std::shared_ptr<GeneralMessage> ptr) {
	std::cout << "Server received: " << ptr->msg << " and " << ptr->msg2 << std::endl;

	assert(ptr->msg == 1337 && ptr->msg2 == 8888);
	GeneralMessage response;
	response.msg = 12345678;
	response.msg2 = 98765;
	sender->channeled_send<GeneralMessage>(&response);
}

void client_callback(SunNet::ChanneledSocketConnection_p sender, std::shared_ptr<GeneralMessage> ptr) {
	std::cout << "Client received: " << ptr->msg << " and " << ptr->msg2 << std::endl;

	assert(ptr->msg == 12345678 && ptr->msg2 == 98765);
	GeneralMessage response;
	response.msg = 1337;
	response.msg2 = 8888;
	sender->channeled_send(&response);
}

int main(int argc, char* argv[]) {
  std::cout << "[MAIN] About to print infinite messages" << std::endl;

	SunNet::Channels::addNewChannel<GeneralMessage>();

	TestServer server("0.0.0.0", "9876", 5);
	server.open();
	server.serve();

  std::cout << "[MAIN] Server serving..." << std::endl;

	server.subscribe<GeneralMessage>(server_callback);

	TestClient client(50);
	client.connect("127.0.0.1", "9876");
	client.subscribe<GeneralMessage>(client_callback);

  std::cout << "[MAIN] Client connected..." << std::endl;

	GeneralMessage msg;
	msg.msg = 1337;
	msg.msg2 = 8888;

	client.channeled_send<GeneralMessage>(&msg);
  std::cout << "[MAIN] Client sent message" << std::endl;

  while (true) {
    bool has_more_updates = true;
    while (has_more_updates) {
      has_more_updates = server.poll();
    }

    has_more_updates = true;
    while (has_more_updates) {
      has_more_updates = client.poll();
    }
  }


	std::string dummy;
	std::getline(std::cin, dummy);
}
