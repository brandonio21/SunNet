#pragma once
#include "socket_connection.hpp"
#include <unordered_set>

namespace SunNet {
	enum SocketStatus {
		SOCKET_STATUS_NORMAL,
		SOCKET_STATUS_DISCONNECT,
		SOCKET_STATUS_ERROR
	};

	struct SocketCollectionEntry {
		SocketConnection_p connection;
		SocketStatus status;

		friend bool operator==(const SocketCollectionEntry& lhs, const SocketCollectionEntry& rhs) {
			return (lhs.connection == rhs.connection);
		}
	};

	struct SocketCollectionEntry_hash {
		std::size_t operator()(const SocketCollectionEntry& collection_entry) const {
			return std::hash<SOCKET>()(collection_entry.connection->socket_descriptor);
		}
	};

	typedef std::unordered_set<SocketCollectionEntry, SocketCollectionEntry_hash> SocketCollection;
	typedef std::shared_ptr<SocketCollection> SocketCollection_p;
}