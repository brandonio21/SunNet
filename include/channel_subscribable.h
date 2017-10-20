#pragma once

#include <functional>
#include <unordered_map>
#include <set>
#include "channels.h"
#include "socket_connection.h"
#include "channel_subscription.h"
#include "channeled_socket_connection.h"


namespace SunNet {
	/**
	A component that is able to subscribe and unsubscribe to specific SunNet channels.

	Any component which wishes to allow subscribing and unsubscribing to channels should
	inherit from this class instead of implementing it on their own. 

	Any messages that pertain to a subscription should then be sent to handleIncomingMessage,
	where the channel id will be parsed and the corresponding subscription will be executed.
	*/
	class ChannelSubscribable {
	private:
		/* Keep track of all the channel subscriptions */
		std::unordered_map < CHANNEL_ID, std::shared_ptr<ChannelSubscriptionInterface>> subscriptions;

	protected:
		/* Should be implemented by subclasses to handle when a recv() returns 0 */
		virtual void handleSocketDisconnect(ChanneledSocketConnection_p socket) = 0;

	public:
		virtual ~ChannelSubscribable();

		/**
		Reads the channel id and corresponding message from the socket. When called, the socket
		must be ready to read a message (recv() should not block). Since the channel id is read from
		the socket, also be sure that the socket is _channeled_, meaning that the channel identifier
		will be valid.

		@param socket The socket to read the incoming channel id and message from
		*/
		void handleIncomingMessage(ChanneledSocketConnection_p socket);

		/**
		Subscribe to a specific channel and provide a callback for when the channel receives a message. Note
		that since all channels are typed, the channel which you wish to subscribe to is specified via
		the template parameter.

		@param callback The callback to execute when a message is received on the channel. The first parameter
		of the callback is the sender, the second is the actual message.
		@return A subscription ID for the subscription, to be used to unsubscribe.
		*/
		template <class TSubscriptionType>
		SUBSCRIPTION_ID subscribe(std::function<void(ChanneledSocketConnection_p, std::shared_ptr<TSubscriptionType>)> callback) {
			/* First, deduce what channel we want to subscribe to */
			CHANNEL_ID channel_id = Channels::getChannelId<TSubscriptionType>();
			std::shared_ptr<ChannelSubscriptionInterface> subscription;

			/* Is there already a subscription for this channel? If not, create it*/
			auto subscription_iter = this->subscriptions.find(channel_id);
			if (subscription_iter != this->subscriptions.end()) {
				subscription = subscription_iter->second;
			}
			else {
				subscription = std::make_shared<ChannelSubscription<TSubscriptionType>>();
				this->subscriptions[channel_id] = subscription;
			}

			std::shared_ptr<ChannelSubscription<TSubscriptionType>> typed_subscription = (
				std::static_pointer_cast<ChannelSubscription<TSubscriptionType>>(subscription)
			);

			return typed_subscription->subscribe(callback);
		}

		/**
		Unsubscribe a certain subscription, no longer calling the callback when a message is received
		on that channel.

		@param id The id of the subscription to unsubscribe
		*/
		template <class TSubscriptionType>
		void unsubscribe(SUBSCRIPTION_ID id) {
			CHANNEL_ID channel_id = Channels::getChannelId<TSubscriptionType>();
			std::shared_ptr<ChannelSubscriptionInterface> subscription = this->subscriptions.at(channel_id);

			std::shared_ptr<ChannelSubscription<TSubscriptionType>> typed_subscription = (
				std::static_pointer_cast<ChannelSubscription<TSubscriptionType>>(subscription)
			);

			if (typed_subscription->unsubscribe(id)) {
				this->subscriptions.erase(channel_id);
			}
		}
	};
}
