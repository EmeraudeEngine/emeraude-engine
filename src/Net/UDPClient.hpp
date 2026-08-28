/*
 * src/Net/UDPClient.hpp
 * This file is part of Emeraude-Engine
 *
 * Copyright (C) 2010-2026 - Sébastien Léon Claude Christian Bémelmans "LondNoir" <londnoir@gmail.com>
 *
 * Emeraude-Engine is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * Emeraude-Engine is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with Emeraude-Engine; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 * Complete project and additional information can be found at :
 * https://github.com/EmeraudeEngine/emeraude-engine
 *
 * --- THIS IS AUTOMATICALLY GENERATED, DO NOT CHANGE ---
 */

#pragma once

/* Project configuration. */
#include "emeraude_export.hpp"

/* STL inclusions. */
#include <atomic>
#include <cstdint>
#include <map>
#include <memory>
#include <shared_mutex>
#include <string>
#include <utility>
#include <vector>

namespace EmEn::Net
{
	/**
	 * @brief Represents a single SSDP device response.
	 */
	struct EMEN_LEAN_API SSDPDevice
	{
		std::string address;
		uint16_t port{0};
		std::map< std::string, std::string > headers;
	};

	/**
	 * @brief Delivery information attached to a received datagram.
	 * @note Produced by the receive() overload taking this structure. It answers the
	 * question the sender address alone cannot: which of the local addresses the
	 * datagram was aimed at, and through which interface it came in. On a multi-homed
	 * host (VPN, container bridges) the same multicast answer arrives several times,
	 * and only the interface index tells the copies apart.
	 */
	struct EMEN_LEAN_API DatagramInfo
	{
		std::string senderAddress;			/* Source IPv4 address, dotted-decimal. */
		std::string destinationAddress;		/* Address the datagram was sent TO. Empty when the stack did not report it. */
		uint32_t interfaceIndex{0};			/* Index of the receiving interface, 0 when unavailable. */
		uint16_t senderPort{0};				/* Source port. */
		bool multicast{false};				/* The destination address belongs to 224.0.0.0/4. */
		bool timedOut{false};				/* The wait ended with NO datagram: a timeout, or a concurrent close(). */
	};

	/**
	 * @brief Cross-platform UDP client for sending/receiving datagrams and SSDP discovery.
	 * @note Uses raw BSD sockets on Linux/macOS and WinSock on Windows.
	 * Provides both a stateful socket (open/bind/send/receive/close) and a
	 * self-contained static SSDP discovery method.
	 * @note IPv4 only, multicast included.
	 */
	class EMEN_LEAN_API UDPClient final
	{
		public:

			static constexpr int DefaultSSDPTimeoutSeconds{5};

			UDPClient () noexcept = default;

			/** @brief Destructor closes the socket if still open. */
			~UDPClient () noexcept;

			/** @brief Non-copyable. */
			UDPClient (const UDPClient &) = delete;

			UDPClient & operator= (const UDPClient &) = delete;

			/** @brief Movable. */
			UDPClient (UDPClient && other) noexcept;

			UDPClient & operator= (UDPClient && other) noexcept;

			/**
			 * @brief Opens a UDP socket.
			 * @note Enables SO_REUSEADDR, and SO_REUSEPORT on POSIX, before any bind()
			 * can happen — both options are inert once the socket is bound. Also arms the
			 * ancillary data read by the receive() overload taking a DatagramInfo, which
			 * must be in place before the first datagram is queued.
			 * @warning POSIX only: because SO_REUSEPORT is enabled, two sockets bound to
			 * the same unicast port no longer collide with EADDRINUSE; the kernel spreads
			 * incoming unicast datagrams between them instead. Multicast is unaffected,
			 * every member socket receives its copy.
			 * @return bool True if the socket was created successfully.
			 */
			bool open () noexcept;

			/**
			 * @brief Binds the socket to a local port for receiving.
			 * @param port The local port to bind to.
			 * @param address The local address to bind to (empty or "0.0.0.0" for any).
			 * @note To receive multicast, bind to any address: binding to a single
			 * interface address filters out group traffic on several stacks.
			 * @return bool True if binding succeeded.
			 */
			bool bind (uint16_t port, const std::string & address = {}) noexcept;

			/**
			 * @brief Closes the socket.
			 * @note Drops the recorded multicast memberships, mirroring the kernel which
			 * releases them when the socket goes away.
			 * @note Interrupts a receive() parked on another thread and waits for it to
			 * return before invalidating the handle. The wait is bounded by one poll slice
			 * (50 ms), NOT by the timeout that receive() was given.
			 * @note Safe on a moved-from instance, where there is nothing left to close.
			 * @return void
			 */
			void close () noexcept;

			/**
			 * @brief Returns whether the socket is currently open.
			 * @return bool
			 */
			[[nodiscard]]
			bool isOpen () const noexcept;

			/**
			 * @brief Sends raw data to a remote host.
			 * @param host The destination IP address or hostname.
			 * @param port The destination port.
			 * @param data Pointer to the data buffer.
			 * @param length Number of bytes to send.
			 * @return int Number of bytes sent, or -1 on error.
			 */
			int send (const std::string & host, uint16_t port, const void * data, size_t length) noexcept;

			/**
			 * @brief Sends a string to a remote host.
			 * @param host The destination IP address or hostname.
			 * @param port The destination port.
			 * @param data The string to send.
			 * @return int Number of bytes sent, or -1 on error.
			 */
			int send (const std::string & host, uint16_t port, const std::string & data) noexcept;

			/**
			 * @brief Receives data from the socket.
			 * @param buffer The buffer to read into.
			 * @param maxLength Maximum number of bytes to receive.
			 * @param senderAddress [out] The sender's IP address.
			 * @param senderPort [out] The sender's port.
			 * @param timeoutMs Receive timeout in milliseconds. 0 = NON-BLOCKING: the call returns 0 at once when no datagram is queued (it never parks the thread).
			 * @param timedOut [out] Optional. Set to true when the wait ended with NO datagram.
			 * ⚠️ Without it, a return of 0 is AMBIGUOUS: a zero-length datagram is legal in UDP and
			 * also returns 0. Measured through app_system's JS path, a timeout and a received 0-byte
			 * datagram were byte-for-byte indistinguishable. It does NOT separate a timeout from a
			 * concurrent close() - both mean "nothing arrived"; a closed socket is observable through
			 * isOpen(), and the next call returns -1.
			 * @return int Number of bytes received (0 for a zero-length datagram), 0 if nothing
			 * arrived - use @a timedOut to tell those apart - or -1 on error.
			 */
			int receive (void * buffer, size_t maxLength, std::string & senderAddress, uint16_t & senderPort, uint32_t timeoutMs = 0, bool * timedOut = nullptr) noexcept;

			/**
			 * @brief Receives data from the socket along with its delivery information.
			 * @note The ancillary data is armed by open(), not here: a datagram already
			 * queued when the option is set comes back with a valid destination address
			 * but an interface index of 0. The plain receive() overload is unaffected.
			 * @warning Windows requires the socket to be bound before this overload works.
			 * @param buffer The buffer to read into.
			 * @param maxLength Maximum number of bytes to receive.
			 * @param info [out] The delivery information of the received datagram.
			 * @param timeoutMs Receive timeout in milliseconds. 0 = NON-BLOCKING: the call returns 0 at once when no datagram is queued (it never parks the thread).
			 * @note @a info is ALWAYS reset by this call, and carries `timedOut` when the wait ended
			 * with no datagram - which is what tells a timeout from a legal zero-length datagram,
			 * both of which return 0.
			 * @return int Number of bytes received (0 for a zero-length datagram), 0 if nothing
			 * arrived - see `info.timedOut` - or -1 on error.
			 */
			int receive (void * buffer, size_t maxLength, DatagramInfo & info, uint32_t timeoutMs = 0) noexcept;

			/**
			 * @brief Receives data as a string from the socket.
			 * @param maxLength Maximum number of bytes to receive.
			 * @param senderAddress [out] The sender's IP address.
			 * @param senderPort [out] The sender's port.
			 * @param timeoutMs Receive timeout in milliseconds. 0 = NON-BLOCKING: the call returns 0 at once when no datagram is queued (it never parks the thread).
			 * @note ⚠️ This overload CANNOT tell a timeout from a zero-length datagram: both give an
			 * empty string. Use the buffer overload with @a timedOut when the difference matters.
			 * @return std::string The data received (may be empty if no data available).
			 */
			[[nodiscard]]
			std::string receiveString (size_t maxLength, std::string & senderAddress, uint16_t & senderPort, uint32_t timeoutMs = 0) noexcept;

			/**
			 * @brief Retrieves the local address and port the socket is bound to.
			 * @param address [out] The bound IP address.
			 * @param port [out] The bound port.
			 * @return bool True if the address was retrieved successfully.
			 */
			bool getLocalAddress (std::string & address, uint16_t & port) const noexcept;

			/**
			 * @brief Enables or disables the SO_BROADCAST socket option.
			 * @param enable True to enable broadcast, false to disable.
			 * @return bool True if the option was set successfully.
			 */
			bool setBroadcast (bool enable) noexcept;

			/**
			 * @brief Joins an IPv4 multicast group on a given local interface.
			 * @note Idempotent by design: joining a group already joined on the same
			 * interface returns true without touching the socket. A consumer re-walking
			 * the interface list on a timer can therefore re-join blindly, where the
			 * kernel would have answered EADDRINUSE.
			 * @warning Windows requires the socket to be bound before joining. POSIX is
			 * permissive; bind first on every platform to keep the behaviour uniform.
			 * @warning The kernel caps the number of memberships per socket (20 by default
			 * on Linux, net.ipv4.igmp_max_memberships). Joining every interface of a host
			 * running containers or VPNs can reach it, and the join then fails.
			 * @param groupAddress The multicast group, dotted-decimal (e.g. "224.0.0.251").
			 * @param interfaceAddress Local interface IP to join on, dotted-decimal. Empty =
			 * INADDR_ANY, the kernel picks. NOTE: an interface *address*, never a name —
			 * "192.168.1.42", not "eth0". Use NetworkInterfaces::enumerateMulticastCapable()
			 * to obtain the candidates.
			 * @return bool True on success, or when already a member.
			 */
			bool joinMulticastGroup (const std::string & groupAddress, const std::string & interfaceAddress = {}) noexcept;

			/**
			 * @brief Leaves an IPv4 multicast group on a given local interface.
			 * @note Tolerates a group that was never joined: teardown paths call this
			 * blindly, so an unknown membership returns true without touching the socket.
			 * @param groupAddress The multicast group, dotted-decimal.
			 * @param interfaceAddress The interface the group was joined on, dotted-decimal.
			 * Must match the value passed to joinMulticastGroup(). An interface *address*,
			 * never a name.
			 * @return bool True on success, or when not a member.
			 */
			bool leaveMulticastGroup (const std::string & groupAddress, const std::string & interfaceAddress = {}) noexcept;

			/**
			 * @brief Sets the TTL of outgoing multicast datagrams.
			 * @note The default of 1 keeps the traffic on the local link. mDNS/DNS-SD
			 * mandates 255, which also lets the receiver detect forged off-link packets.
			 * @param ttl The time-to-live, in hops.
			 * @return bool True if the option was set successfully.
			 */
			bool setMulticastTTL (uint8_t ttl) noexcept;

			/**
			 * @brief Enables or disables the loopback of outgoing multicast datagrams.
			 * @note Enabled by default on most stacks. Required for several processes of
			 * the same host to see each other, and to exercise multicast on one machine.
			 * @param enable True to receive back the datagrams sent by this host.
			 * @return bool True if the option was set successfully.
			 */
			bool setMulticastLoopback (bool enable) noexcept;

			/**
			 * @brief Selects the local interface used to send multicast datagrams.
			 * @note Outbound only. It does not affect the groups joined for reception,
			 * which carry their own interface.
			 * @param interfaceAddress Local interface IP, dotted-decimal. Empty restores
			 * INADDR_ANY, letting the routing table decide. An interface *address*, never
			 * a name.
			 * @return bool True if the option was set successfully.
			 */
			bool setMulticastInterface (const std::string & interfaceAddress = {}) noexcept;

			/**
			 * @brief Performs an SSDP M-SEARCH and collects responses (self-contained, no instance state needed).
			 * @note Creates a temporary socket internally. Uses raw UDP multicast.
			 * @param searchTarget The ST header value (e.g., "ssdp:all", "urn:schemas-upnp-org:device:Printer:1").
			 * @param timeoutSeconds How long to listen for responses.
			 * @return std::vector< SSDPDevice > The discovered devices.
			 */
			[[nodiscard]]
			static std::vector< SSDPDevice > ssdpDiscover (const std::string & searchTarget, int timeoutSeconds = DefaultSSDPTimeoutSeconds) noexcept;

		private:

			/**
			 * @brief Enables the ancillary data carrying the datagram destination address.
			 * @note Called by open(). The receive() overload taking a DatagramInfo retries
			 * it, for the case of a stack that refused the option at creation time.
			 * @return bool True if the socket now reports the delivery information.
			 */
			bool enableDatagramInfo () noexcept;

			/* ⚠️ Guards the socket handle for the whole duration of a syscall. Without it,
			 * close() from another thread while receive() is in select()/recvfrom() lets the
			 * kernel recycle the descriptor: the receiver then reads from an unrelated file.
			 * Same two-phase close as TCPClient — the receiver is woken, then close()
			 * invalidates the handle only once every in-flight call released the lock.
			 * Held by pointer so the class stays movable. */
			std::unique_ptr< std::shared_mutex > m_handleMutex{std::make_unique< std::shared_mutex >()};

			/* ⚠️ What actually wakes a parked receive() — NOT shutdown(), unlike TCPClient.
			 * A datagram socket is unconnected, and POSIX makes shutdown() fail with ENOTCONN
			 * there (Winsock: WSAENOTCONN), waking nobody. Linux is the lenient outlier, which
			 * is why this went unnoticed: measured on macOS 26, close() waited out the full
			 * receive() timeout instead of returning, and since UDPModule binds close() as a
			 * SYNCHRONOUS method, that stall lands on the renderer's main thread.
			 * So receive() polls in slices and watches this flag rather than trusting the
			 * kernel for the wake-up. Held by pointer, like the mutex, to stay movable. */
			std::unique_ptr< std::atomic_bool > m_closing{std::make_unique< std::atomic_bool >(false)};

			/* Joined multicast groups, as (group, interface) pairs in network byte order.
			 * Held to make join/leave idempotent without having to read errno. */
			std::vector< std::pair< uint32_t, uint32_t > > m_multicastMemberships;
#ifdef _WIN32
			uintptr_t m_socket{~uintptr_t{0}};  /* SOCKET (UINT_PTR), INVALID_SOCKET = ~0. */
#else
			int m_fd{-1};
#endif
			bool m_datagramInfoEnabled{false};
	};
}
