#include "channel_subscribable.h"

namespace SunNet {

	ChannelSubscribable::~ChannelSubscribable() {
		this->subscriptions.clear();
	}

	void ChannelSubscribable::handleIncomingMessage(ChanneledSocketConnection_p socket) {
		try {
			CHANNEL_ID channel_id = socket->channeled_read_id();

			std::unique_ptr<NETWORK_BYTE[]> data = socket->channeled_read(channel_id);

			auto channel_subs = subscriptions.find(channel_id);
			if (channel_subs != this->subscriptions.end()) {
				channel_subs->second->propagate_to_handlers(socket, std::move(data));
			}
		}
		catch (ChanneledSocketConnection::ConnectionClosedException&) {
			this->handleSocketDisconnect(socket);
		}
	}
}
