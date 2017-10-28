#pragma once
#include "socketutil.h"

#include <map>
#include <typeindex>
#include <memory>
#include <atomic>

namespace SunNet {
	typedef NETWORK_BYTE CHANNEL_ID;

	template <class TData>
  class Channel;

	/**
	A C++ hack to allow typed classes to be put into containers. All channel classes,
	despite type, will inherit from this "interface".

	The interface contains declarations for the commonolaties of channels: their ids
	and their message size.
	*/
	class ChannelInterface {
	private:
		NETWORK_BYTE_SIZE message_size;
		CHANNEL_ID channel_id;
	public:
		ChannelInterface(NETWORK_BYTE_SIZE size, CHANNEL_ID id);
		NETWORK_BYTE_SIZE getMessageSize() { return this->message_size; }
		CHANNEL_ID getId() { return this->channel_id; }
	};

	/**
	A static class which keeps track of created channels and the mappings from
	channelid -> channel as well as channel_type -> channel

	If the user wants to add a new channel, they should do so through this class.
	This class is also the de facto way of finding channel info
	*/
	class Channels {
	private:
		static std::map<CHANNEL_ID, std::shared_ptr<ChannelInterface>> ids_to_channels;
		static std::map<std::type_index, CHANNEL_ID> types_to_ids;

		/* Used to create IDs for new channels */
		static std::atomic<CHANNEL_ID> channel_counter;
	public:

		/**
		Get the channel id from the channel with the provided type. The type
		is provided via template parameter

		@return The id for the channel with the given type
		@throws BadChannelException if no channel with the given type could be found
		*/
		template <class TChannelType>
		static CHANNEL_ID getChannelId() {
			const auto& id_it = types_to_ids.find(typeid(TChannelType));
			if (id_it == types_to_ids.end()) {
				throw BadChannelException();
			}
			else {
				return id_it->second;
			}
		}

		/**
		Get the channel from the provided channel id.

		@param id The channel id to retrieve
		@return A shared_ptr to the channel
		@throws BadChannelException if there is no channel with the given id
		*/
		static std::shared_ptr<ChannelInterface> getChannel(CHANNEL_ID id);

		/**
		Get the next id and increment the counter afterwards
		*/
		static CHANNEL_ID getNextId() { return channel_counter++; }

		/**
		Adds a new channel with the given type, automatically adding assigning an
		id to it and adding it to the proper maps. The type is given as a template
		parameter.
		*/
		template <class TChannelType>
		static void addNewChannel() {
			std::shared_ptr<ChannelInterface> channel = std::make_shared<Channel<TChannelType>>();
			Channels::ids_to_channels[channel->getId()] = channel;
			Channels::types_to_ids[typeid(TChannelType)] = channel->getId();
		}

		class BadChannelException : std::exception {};
	};


	/**
	A syntactic-sugar class making it easy to create new channels
	*/
	template <class TData>
	class Channel : public ChannelInterface {
	public:
		Channel() : ChannelInterface(sizeof(TData), Channels::getNextId()) {}
	};
}
