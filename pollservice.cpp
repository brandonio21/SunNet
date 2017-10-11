#include "pollservice.h"

#include <string>


namespace SunNet {

	PollService::PollService(int timeout) : timeout(timeout) {
		this->results = std::make_shared<SocketCollection>();
	}

	PollService::PollService(const SocketConnection_p socket, int timeout) : PollService(timeout) {
		this->add_socket(socket);
	}

	void PollService::add_socket(const SocketConnection_p socket) {
		POLL_DESCRIPTOR poll_descriptor;
		poll_descriptor.events = POLLIN;
		poll_descriptor.fd = socket->socket_descriptor;

		this->descriptors.push_back(poll_descriptor);
		this->poll_descriptor_map[socket->socket_descriptor] = std::make_pair((int) this->descriptors.size() - 1, socket);
	}

	void PollService::remove_socket(const SocketConnection_p socket) {
		auto& info = this->poll_descriptor_map.find(socket->socket_descriptor);
		if (info == this->poll_descriptor_map.end()) {
			/* Socket is already gone! */
			return;
		}

		int index = info->second.first;

		/* Step 1: Erase the element from the descriptors, which will cause all elements to shift down */
		this->descriptors.erase(this->descriptors.begin() + index);
		this->poll_descriptor_map.erase(info);

		/* Step 2: Change the indices of all elements in poll_descritor_map after this one*/
		for (int i = index; i < this->descriptors.size(); i++) {
			this->poll_descriptor_map[this->descriptors[i].fd].first = i;
		}
	}

	void PollService::clear_sockets() {
		this->descriptors.clear();
		this->poll_descriptor_map.clear();
	}

	SocketCollection_p PollService::poll() {
		this->results->clear();
		int poll_return = socket_poll(this->descriptors.data(), (NUM_POLL_DESCRIPTORS) this->descriptors.size(), this->timeout);

		if (poll_return == SOCKET_ERROR) {
			throw PollException(std::to_string(get_previous_error_code()));
		}
		else if (poll_return > 0) {
			for (auto poll_iter = this->descriptors.begin(); poll_iter != this->descriptors.end(); ++poll_iter) {
				if (poll_iter->revents & (POLLIN | POLLERR | POLLNVAL | POLLHUP)) {
					auto& socket_iter = this->poll_descriptor_map.find(poll_iter->fd);
					if (socket_iter == this->poll_descriptor_map.end()) {
						/* 
						Something weird has happened. We've encountered something for a socket we are not keeping track of anymore.
						Maybe it's not a big deal? Let's just skip over it.
						*/
						continue;
					}


					SocketConnection_p ready_socket = socket_iter->second.second;

					if (!ready_socket) {
						throw InvalidSocketConnectionException("Invalid socket descriptor", poll_iter->fd);
					}

					SocketStatus status;
					if (poll_iter->revents & (POLLERR | POLLNVAL)) {
						status = SOCKET_STATUS_ERROR;
					}
					else if (poll_iter->revents & (POLLHUP)) {
						status = SOCKET_STATUS_DISCONNECT;
					}
					else {
						status = SOCKET_STATUS_NORMAL;
					}

					this->results->insert(SocketCollectionEntry{ ready_socket, status });
				}
			}
		}

		return this->results;
	}
}
