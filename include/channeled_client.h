#pragma once

#include "client.h"
#include "channel_subscribable.h"
#include "channeled_socket_connection.h"

namespace SunNet {

	/**
	A wrapper around a client which allows for sending on channeled connections as well as
	subscribing to channels.

	A ChanneledClient must only be used to connect to a ChanneledServer and should never be used
	to call "receive" or "send". Instead, the user should use "channeled_send" and subscribe for
	receipt.
	*/
	template <typename TSocketConnection>
	class ChanneledClient : public Client<TSocketConnection>, public ChannelSubscribable {
	protected:
		/* Handle ChannelSubscribable's disconnection logic */
		void handleSocketDisconnect(ChanneledSocketConnection_p socket) {
			this->handle_client_disconnect();
		}

		/* Handle Client's ready_to_read logic */
		void handle_client_ready_to_read() {
			/* Delegate to ChannelSubscribable */
			ChanneledSocketConnection_p channeled_con = std::static_pointer_cast<ChanneledSocketConnection>(this->connection);
			this->handleIncomingMessage(channeled_con);
		}

		/**** Handlers for ChanneledClient ****/
		virtual void handle_client_disconnect() = 0;
		virtual void handle_client_error() = 0;
		virtual void handle_poll_timeout() = 0;

	public:
		template <class ... ArgTypes>
		ChanneledClient(int poll_timeout, ArgTypes ... args) : Client(poll_timeout, args...) {}

		/**
		Send a message upon a specific channel. The channel is determined
		by the template type.

		@param message The object to send
		*/
		template <class TMessageType>
		void channeled_send(TMessageType* message) {
			ChanneledSocketConnection_p channeled_con = std::static_pointer_cast<ChanneledSocketConnection>(this->connection);
			channeled_con->channeled_send<TMessageType>(message);
		}

		/**
		Receive the channel ID from the incoming message. This will block
		until a channel id is ready to be read.

		It is preferred that the user use subscriptions for reads instead of 
		reading directly
		*/
		CHANNEL_ID channeled_read_id() {
			ChanneledSocketConnection_p channeled_con = std::static_pointer_cast<ChanneledSocketConnection>(this->connection);
			return channeled_con->channeled_read_id();
		}

		/**
		Receive the bytes from a specific channel. The amount of bytes to read
		is determined by the channel id. This will block until all bytes are
		ready to be read.

		It is preferred that the user use subscriptions for reads instead of 
		reading directly
		*/
		std::unique_ptr<NETWORK_BYTE[]> channeled_read(CHANNEL_ID id) {
			ChanneledSocketConnection_p channeled_con = std::static_pointer_cast<ChanneledSocketConnection>(this->connection);
			return channeled_con->channeled_read(id);
		}
	};
}
