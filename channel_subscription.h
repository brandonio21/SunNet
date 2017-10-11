#pragma once

#include "socketutil.h"
#include "channeled_socket_connection.h"

#include <atomic>
#include <unordered_map>
#include <functional>
#include <memory>

namespace SunNet {
	typedef unsigned int SUBSCRIPTION_ID;

	/**
	This is a C++ hack. In order to specify templated objects of any type in a data structure, we simply
	create an "interface" and make all the templated objects inherit from that interface.
	*/
	class ChannelSubscriptionInterface {
	public:
		/**
		Propagate an incoming message to all the subscription handlers. This involves type-casting
		the bytes.

		@param sender The sender of the original channeled message
		@param data A buffer of the data read
		*/
		virtual void propagate_to_handlers(ChanneledSocketConnection_p sender, std::unique_ptr<NETWORK_BYTE[]> data) = 0;
	};

	/**
	A subscription for an individual channel. A subscription consists of several callbacks, each with its own ID.
	Since a channel is typed, the subscriptions are also typed.

	The ChannelSubscription class inherits from ChannelSubscriptionInterface so that typed channel subscriptions may be stored
	in a map. (A C++ hack)
	*/
	template <typename TSubscriptionType>
	class ChannelSubscription : public ChannelSubscriptionInterface {
	private:
		std::atomic<SUBSCRIPTION_ID> subscription_counter;
		std::unordered_map<SUBSCRIPTION_ID, std::function<void(ChanneledSocketConnection_p, std::shared_ptr<TSubscriptionType>)>> subscriptions;

	public:
		ChannelSubscription() : subscription_counter(0) {}

		/**
		Associate a callback function with this channel.
		@param handler The subscription callback
		@return An ID for the subscription, useful for unsubscribing
		*/
		SUBSCRIPTION_ID subscribe(std::function<void(ChanneledSocketConnection_p, std::shared_ptr<TSubscriptionType>)> handler) {
			SUBSCRIPTION_ID subscription_id = this->subscription_counter++;
			this->subscriptions[subscription_id] = handler;

			return subscription_id;
		}

		/**
		Unsubscribe a callback from this channel.
		@param The ID of the callback to unsubscribe
		@return A boolean indicating whether all callbacks have been unsubscribed for this channel
		*/
		bool unsubscribe(SUBSCRIPTION_ID id) {
			this->subscriptions.erase(id);
			return (this->subscriptions.size() == 0);
		}

		void propagate_to_handlers(ChanneledSocketConnection_p sender, std::unique_ptr<NETWORK_BYTE[]> data) {
			/* Take ownership of the bytes */
			std::shared_ptr<NETWORK_BYTE> shared_data = std::move(data);

			/* Typecast them for our lovely subscribers */
			auto p = reinterpret_cast<typename std::shared_ptr<TSubscriptionType>::element_type*>(shared_data.get());
			std::shared_ptr<TSubscriptionType> obj(shared_data, p);

			/* 
			Pass the shared_ptr to the subscribers. If none of them store the ptr, then the bytes will
			automatically be freed at the end of this method. However, if any of them store the ptr, the bytes
			will remain in memory until they are finished.
			*/
			for (auto subscription : subscriptions) {
				subscription.second(sender, obj);
			}
		}
	};
}
