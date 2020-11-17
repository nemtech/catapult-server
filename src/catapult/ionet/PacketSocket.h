/**
*** Copyright (c) 2016-present,
*** Jaguar0625, gimre, BloodyRookie, Tech Bureau, Corp. All rights reserved.
***
*** This file is part of Catapult.
***
*** Catapult is free software: you can redistribute it and/or modify
*** it under the terms of the GNU Lesser General Public License as published by
*** the Free Software Foundation, either version 3 of the License, or
*** (at your option) any later version.
***
*** Catapult is distributed in the hope that it will be useful,
*** but WITHOUT ANY WARRANTY; without even the implied warranty of
*** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
*** GNU Lesser General Public License for more details.
***
*** You should have received a copy of the GNU Lesser General Public License
*** along with Catapult. If not, see <http://www.gnu.org/licenses/>.
**/

#pragma once
#include "BatchPacketReader.h"
#include "ConnectResult.h"
#include "IoTypes.h"
#include "PacketSocketOptions.h"

namespace catapult {
	namespace ionet {
		struct NodeEndpoint;
		struct Packet;
	}
}

namespace catapult { namespace ionet {

	// region PacketSocket

	/// Asio socket wrapper that natively supports packets.
	/// This wrapper is threadsafe but does not prevent interleaving reads or writes.
	class PacketSocket : public PacketIo, public BatchPacketReader {
	public:
		/// Statistics about a socket.
		struct Stats {
			/// \c true if the socket is open.
			bool IsOpen;

			/// Number of unprocessed bytes.
			size_t NumUnprocessedBytes;
		};

		using StatsCallback = consumer<const Stats&>;
		using WaitForDataCallback = action;

	public:
		~PacketSocket() override = default;

	public:
		/// Retrieves statistics about this socket and passes them to \a callback.
		virtual void stats(const StatsCallback& callback) = 0;

		/// Calls \a callback when data is available for reading.
		virtual void waitForData(const WaitForDataCallback& callback) = 0;

		/// Closes the socket.
		virtual void close() = 0;

		/// Aborts the current operation and closes the socket.
		virtual void abort() = 0;

		/// Gets a buffered interface to the packet socket.
		virtual std::shared_ptr<PacketIo> buffered() = 0;
	};

	// endregion

	// region PacketSocketInfo

	/// Tuple composed of (resolved) host, public key and packet socket.
	class PacketSocketInfo {
	public:
		/// Creates an empty info.
		PacketSocketInfo();

		/// Creates an info around \a host, \a publicKey and \a pPacketSocket.
		PacketSocketInfo(const std::string& host, const Key& publicKey, const std::shared_ptr<PacketSocket>& pPacketSocket);

	public:
		/// Gets the host.
		const std::string& host() const;

		/// Gets the public key.
		const Key& publicKey() const;

		/// Gets the socket.
		const std::shared_ptr<PacketSocket>& socket() const;

	public:
		/// Returns \c true if this info is not empty.
		explicit operator bool() const;

	private:
		std::string m_host;
		Key m_publicKey;
		std::shared_ptr<PacketSocket> m_pPacketSocket;
	};

	// endregion

	// region Accept

	/// Callback for an accepted socket.
	using AcceptCallback = consumer<const PacketSocketInfo&>;

	/// Accepts a connection using \a ioContext and \a acceptor and calls \a accept on completion configuring the socket with \a options.
	void Accept(
			boost::asio::io_context& ioContext,
			boost::asio::ip::tcp::acceptor& acceptor,
			const PacketSocketOptions& options,
			const AcceptCallback& accept);

	// endregion

	// region Connect

	/// Callback for a connected socket.
	using ConnectCallback = consumer<ConnectResult, const PacketSocketInfo&>;

	/// Attempts to connect a socket to the specified \a endpoint using \a ioContext and calls \a callback on
	/// completion configuring the socket with \a options. The returned function can be used to cancel the connect.
	/// \note User callbacks passed to the connected socket are serialized.
	action Connect(
			boost::asio::io_context& ioContext,
			const PacketSocketOptions& options,
			const NodeEndpoint& endpoint,
			const ConnectCallback& callback);

	// endregion
}}
