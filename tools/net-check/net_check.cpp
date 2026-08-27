/*
 * Cross-platform runtime check for EmEn::Net::UDPClient + NetworkInterfaces.
 *
 * Compiles the REAL engine sources out-of-tree — no engine build, no CMake, no Vulkan.
 * See README.md in this directory for the per-OS command line and for what a pass means.
 *
 * Covers the two handover checklists:
 *   emeraude-engine/docs/todo/udp-multicast-macos-verification.md
 *   emeraude-base/docs/todo/tls-stack-windows-macos-validation.md
 */

#include "Net/NetworkInterfaces.hpp"
#include "Net/UDPClient.hpp"

#ifdef _WIN32
	#ifndef NOMINMAX
	#define NOMINMAX
	#endif

	#ifndef WIN32_LEAN_AND_MEAN
	#define WIN32_LEAN_AND_MEAN
	#endif

	#include <WinSock2.h>
	#include <WS2tcpip.h>

	#ifdef _MSC_VER
		#pragma comment(lib, "ws2_32.lib")
	#endif

	using RawSocket = SOCKET;
	static constexpr RawSocket InvalidRawSocket{INVALID_SOCKET};
#else
	#include <arpa/inet.h>
	#include <netinet/in.h>
	#include <sys/socket.h>
	#include <unistd.h>

	using RawSocket = int;
	static constexpr RawSocket InvalidRawSocket{-1};
#endif

#include <array>
#include <atomic>
#include <cerrno>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <thread>
#include <vector>

using namespace EmEn::Net;
using Clock = std::chrono::steady_clock;

/* This harness opens a couple of raw sockets of its own, to check what the KERNEL accepts
 * independently of what UDPClient sends. Those few calls are the only platform-specific code here. */

static void
closeRawSocket (RawSocket sock) noexcept
{
#ifdef _WIN32
	closesocket(sock);
#else
	close(sock);
#endif
}

static std::string
lastSocketError () noexcept
{
#ifdef _WIN32
	return "WSA error " + std::to_string(WSAGetLastError());
#else
	return std::strerror(errno);
#endif
}

static bool
startupSockets () noexcept
{
#ifdef _WIN32
	WSADATA data{};

	return WSAStartup(MAKEWORD(2, 2), &data) == 0;
#else
	return true;
#endif
}

static int g_pass = 0;
static int g_fail = 0;
static int g_warn = 0;

static void
check (bool condition, const std::string & label, const std::string & detail = {})
{
	if ( condition )
	{
		++g_pass;
		std::cout << "  [ OK ] " << label;
	}
	else
	{
		++g_fail;
		std::cout << "  [FAIL] " << label;
	}

	if ( !detail.empty() )
	{
		std::cout << "  — " << detail;
	}

	std::cout << '\n';
}

static void
warn (const std::string & label, const std::string & detail = {})
{
	++g_warn;
	std::cout << "  [WARN] " << label;

	if ( !detail.empty() )
	{
		std::cout << "  — " << detail;
	}

	std::cout << '\n';
}

static void
info (const std::string & text)
{
	std::cout << "         " << text << '\n';
}

static void
title (const std::string & text)
{
	std::cout << "\n=== " << text << " ===\n";
}

static long
elapsedMs (Clock::time_point start)
{
	return std::chrono::duration_cast< std::chrono::milliseconds >(Clock::now() - start).count();
}

/* ---------------------------------------------------------------- DNS/mDNS */

/* Appends a DNS-encoded name ("_services._dns-sd._udp.local"). */
static void
appendName (std::vector< uint8_t > & packet, const std::string & name)
{
	size_t start = 0;

	while ( start < name.size() )
	{
		auto dot = name.find('.', start);

		if ( dot == std::string::npos )
		{
			dot = name.size();
		}

		const auto length = dot - start;

		packet.push_back(static_cast< uint8_t >(length));
		packet.insert(packet.end(), name.begin() + static_cast< long >(start), name.begin() + static_cast< long >(dot));

		start = dot + 1;
	}

	packet.push_back(0);
}

static std::vector< uint8_t >
buildDNSSDQuery (const std::string & serviceName)
{
	std::vector< uint8_t > packet;

	/* Header: ID 0, standard query, 1 question. */
	const uint8_t header[] = {0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0};
	packet.insert(packet.end(), std::begin(header), std::end(header));

	appendName(packet, serviceName);

	/* QTYPE = PTR (12), QCLASS = IN (1). */
	packet.push_back(0);
	packet.push_back(12);
	packet.push_back(0);
	packet.push_back(1);

	return packet;
}

/* Minimal name reader with compression-pointer support. */
static std::string
readName (const uint8_t * data, size_t size, size_t offset, size_t & consumed, int depth = 0)
{
	std::string name;
	size_t cursor = offset;
	bool jumped = false;
	consumed = 0;

	if ( depth > 8 )
	{
		return name;
	}

	while ( cursor < size )
	{
		const auto length = data[cursor];

		if ( length == 0 )
		{
			++cursor;

			if ( !jumped )
			{
				consumed = cursor - offset;
			}

			break;
		}

		if ( ( length & 0xC0 ) == 0xC0 )
		{
			if ( cursor + 1 >= size )
			{
				break;
			}

			const size_t pointer = ( static_cast< size_t >(length & 0x3F) << 8 ) | data[cursor + 1];

			if ( !jumped )
			{
				consumed = cursor + 2 - offset;
				jumped = true;
			}

			size_t inner = 0;
			name += readName(data, size, pointer, inner, depth + 1);

			break;
		}

		if ( cursor + 1 + length > size )
		{
			break;
		}

		if ( !name.empty() )
		{
			name += '.';
		}

		name.append(reinterpret_cast< const char * >(data + cursor + 1), length);
		cursor += 1 + length;
	}

	return name;
}

/* Returns the PTR/answer names of an mDNS response, best effort. */
static std::vector< std::string >
parseAnswerNames (const uint8_t * data, size_t size)
{
	std::vector< std::string > names;

	if ( size < 12 )
	{
		return names;
	}

	const uint16_t questionCount = static_cast< uint16_t >(( data[4] << 8 ) | data[5]);
	const uint16_t answerCount = static_cast< uint16_t >(( data[6] << 8 ) | data[7]);

	size_t cursor = 12;

	for ( uint16_t i = 0; i < questionCount && cursor < size; ++i )
	{
		size_t consumed = 0;
		readName(data, size, cursor, consumed);
		cursor += consumed + 4;
	}

	for ( uint16_t i = 0; i < answerCount && cursor < size; ++i )
	{
		size_t consumed = 0;
		const auto owner = readName(data, size, cursor, consumed);
		cursor += consumed;

		if ( cursor + 10 > size )
		{
			break;
		}

		const uint16_t type = static_cast< uint16_t >(( data[cursor] << 8 ) | data[cursor + 1]);
		const uint16_t dataLength = static_cast< uint16_t >(( data[cursor + 8] << 8 ) | data[cursor + 9]);
		cursor += 10;

		if ( type == 12 && cursor + dataLength <= size )
		{
			size_t inner = 0;
			names.push_back(owner + " -> " + readName(data, size, cursor, inner));
		}
		else
		{
			names.push_back(owner + " (type " + std::to_string(type) + ")");
		}

		cursor += dataLength;
	}

	return names;
}

/* ------------------------------------------------------------------- Tests */

static std::vector< NetworkInterfaces::Interface >
testInterfaceEnumeration ()
{
	title("1. NetworkInterfaces::enumerate() — AF_LINK (MAC) + IPv6 legs");

	const auto interfaces = NetworkInterfaces::enumerate();

	check(!interfaces.empty(), "enumerate() returns entries", std::to_string(interfaces.size()) + " address(es)");

	bool hasIPv4 = false;
	bool hasIPv6 = false;
	bool hasMac = false;
	bool loopbackMacEmpty = true;
	bool placeholderMac = false;
	bool indexAlwaysSet = true;
	std::map< std::string, std::string > macPerInterface;
	bool macConsistent = true;

	for ( const auto & entry : interfaces )
	{
		std::cout << "         "
			<< std::left << std::setw(12) << entry.name
			<< std::setw(6) << NetworkInterfaces::to_cstring(entry.family)
			<< std::setw(42) << entry.address
			<< " /" << std::setw(4) << static_cast< int >(entry.prefixLength)
			<< " idx=" << std::setw(4) << entry.index
			<< " mac=" << std::setw(19) << ( entry.mac.empty() ? "(none)" : entry.mac )
			<< ( entry.up ? " up" : " down" )
			<< ( entry.loopback ? " loopback" : "" )
			<< ( entry.multicastCapable ? " multicast" : "" )
			<< ( entry.scopeId != 0 ? " scope=" + std::to_string(entry.scopeId) : "" )
			<< '\n';

		if ( entry.family == NetworkInterfaces::AddressFamily::IPv4 ) { hasIPv4 = true; }
		if ( entry.family == NetworkInterfaces::AddressFamily::IPv6 ) { hasIPv6 = true; }
		if ( !entry.mac.empty() ) { hasMac = true; }
		if ( entry.loopback && !entry.mac.empty() ) { loopbackMacEmpty = false; }
		if ( entry.mac == "00:00:00:00:00:00" ) { placeholderMac = true; }
		if ( entry.index == 0 ) { indexAlwaysSet = false; }

		if ( !entry.mac.empty() )
		{
			auto [it, inserted] = macPerInterface.emplace(entry.name, entry.mac);

			if ( !inserted && it->second != entry.mac )
			{
				macConsistent = false;
			}
		}
	}

	check(hasIPv4, "IPv4 addresses present");
	check(hasIPv6, "IPv6 addresses present (AF_INET6 leg)");
	check(hasMac, "at least one hardware address resolved (AF_LINK leg)");
	check(loopbackMacEmpty, "loopback carries an EMPTY mac, not a placeholder");
	check(!placeholderMac, "no 00:00:.. placeholder anywhere");
	check(indexAlwaysSet, "every entry carries a non-zero interface index");
	check(macConsistent, "the mac is repeated identically across an interface's addresses");

	title("2. NetworkInterfaces::enumerateMulticastCapable()");

	const auto multicast = NetworkInterfaces::enumerateMulticastCapable();

	check(!multicast.empty(), "at least one multicast-capable IPv4 interface", std::to_string(multicast.size()) + " found");

	bool onlyIPv4 = true;
	bool onlyUp = true;
	bool loopbackKept = false;

	for ( const auto & entry : multicast )
	{
		std::cout << "         " << std::left << std::setw(12) << entry.name << entry.address << '\n';

		if ( entry.family != NetworkInterfaces::AddressFamily::IPv4 ) { onlyIPv4 = false; }
		if ( !entry.up || !entry.multicastCapable ) { onlyUp = false; }
		if ( entry.loopback ) { loopbackKept = true; }
	}

	check(onlyIPv4, "IPv4 only, as the multicast API requires");
	check(onlyUp, "every entry is up and multicast-capable");
	check(loopbackKept, "loopback is kept (single-machine multicast)");

	return multicast;
}

static void
testMulticastTTLByteType ()
{
	title("3. IP_MULTICAST_TTL / IP_MULTICAST_LOOP option width accepted by THIS kernel");

	const auto sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

	if ( sock == InvalidRawSocket )
	{
		warn("socket() failed, skipping", lastSocketError());
		return;
	}

	const int asInt = 255;
	const auto intResult = setsockopt(sock, IPPROTO_IP, IP_MULTICAST_TTL, reinterpret_cast< const char * >(&asInt), sizeof(asInt));
	const auto intError = lastSocketError();

	const unsigned char asByte = 255;
	const auto byteResult = setsockopt(sock, IPPROTO_IP, IP_MULTICAST_TTL, reinterpret_cast< const char * >(&asByte), sizeof(asByte));
	const auto byteError = lastSocketError();

	/* Read back with the width this platform documents, which is what UDPClient sends. */
#if defined(_WIN32)
	DWORD readBack = 0;
#elif defined(__APPLE__) || defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__) || defined(__DragonFly__)
	unsigned char readBack = 0;
#else
	int readBack = 0;
#endif

	auto length = static_cast< socklen_t >(sizeof(readBack));
	getsockopt(sock, IPPROTO_IP, IP_MULTICAST_TTL, reinterpret_cast< char * >(&readBack), &length);

	closeRawSocket(sock);

	info(std::string("setsockopt(int,   4 bytes) -> ") + ( intResult == 0 ? "accepted" : "REFUSED (" + intError + ")" ));
	info(std::string("setsockopt(uchar, 1 byte)  -> ") + ( byteResult == 0 ? "accepted" : "REFUSED (" + byteError + ")" ));

	/* ⚠️ What must hold is that the width UDPClient sends on THIS platform is honoured — not that
	 * some other width is refused. The engine picks the width each platform's manual documents
	 * (MulticastOptionValue in UDPClient.cpp): u_char on macOS/BSD, int on Linux, DWORD on Windows. */
#if defined(__APPLE__) || defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__) || defined(__DragonFly__)
	check(byteResult == 0, "the width this platform documents (uchar) is accepted");
#else
	check(intResult == 0, "the width this platform documents (int/DWORD) is accepted");
#endif

	check(readBack == 255, "TTL reads back as 255 through the documented width", "got " + std::to_string(static_cast< int >(readBack)));

	if ( intResult != 0 || byteResult != 0 )
	{
		info("=> this kernel is strict about the width. Sending the wrong one leaves the TTL at 1.");
	}
	else
	{
		warn("this kernel accepts BOTH widths", "documented width still used, leniency is not a contract");
	}
}

static void
testNonBlockingReceive ()
{
	title("4. receive(timeoutMs = 0) is non-blocking");

	UDPClient client;

	check(client.open(), "open()");
	check(client.bind(0), "bind(ephemeral)");

	std::string address;
	uint16_t port = 0;
	check(client.getLocalAddress(address, port) && port != 0, "getLocalAddress()", address + ":" + std::to_string(port));

	std::array< char, 1024 > buffer{};
	std::string sender;
	uint16_t senderPort = 0;

	const auto start = Clock::now();
	const auto received = client.receive(buffer.data(), buffer.size(), sender, senderPort, 0);
	const auto took = elapsedMs(start);

	check(received == 0, "returns 0 with no datagram queued", "returned " + std::to_string(received));
	check(took < 50, "returned immediately", std::to_string(took) + " ms");

	/* And the timeout path still parks for the requested duration. */
	const auto timedStart = Clock::now();
	client.receive(buffer.data(), buffer.size(), sender, senderPort, 300);
	const auto timedTook = elapsedMs(timedStart);

	check(timedTook >= 290 && timedTook < 420, "receive(300) waits ~300 ms", std::to_string(timedTook) + " ms");

	/* ⚠️ The tolerance above must stay TIGHT. The wait is sliced internally, and an
	 * implementation that counts nominal slices instead of measuring elapsed time overshoots by a
	 * few percent per slice — invisible at 300 ms with a loose bound, +13.8% at 3 s (measured on
	 * the first version of the sliced wait, 2026-08-28). A long wait is where it shows. */
	const auto longStart = Clock::now();
	client.receive(buffer.data(), buffer.size(), sender, senderPort, 1200);
	const auto longTook = elapsedMs(longStart);
	const auto driftPercent = ( longTook - 1200 ) * 100 / 1200;

	check(longTook >= 1180 && driftPercent <= 8, "receive(1200) does not accumulate slice drift",
		std::to_string(longTook) + " ms, drift " + std::to_string(driftPercent) + "%");
}

static void
testCloseWakesReceive ()
{
	title("5. close() from another thread wakes a parked receive()");

	UDPClient client;

	if ( !client.open() || !client.bind(0) )
	{
		warn("could not open/bind, skipping");
		return;
	}

	std::atomic_bool returned{false};
	long tookMs = 0;

	std::thread receiver([&client, &returned, &tookMs] {
		std::array< char, 1024 > buffer{};
		std::string sender;
		uint16_t senderPort = 0;

		const auto start = Clock::now();
		client.receive(buffer.data(), buffer.size(), sender, senderPort, 10000);
		tookMs = elapsedMs(start);
		returned = true;
	});

	std::this_thread::sleep_for(std::chrono::milliseconds(300));
	client.close();

	receiver.join();

	check(returned.load(), "the parked receive() returned");
	check(tookMs < 3000, "it returned promptly after close()", std::to_string(tookMs) + " ms");
	check(!client.isOpen(), "the socket is closed");

	/* ⚠️ REOPEN THE SAME INSTANCE — not a fresh one. The point is that open() clears the flag
	 * close() raised; a brand-new object has never had it raised, so it would pass this check
	 * even if open() dropped the re-arm entirely, and every subsequent receive() on a recycled
	 * socket would return instantly instead of waiting. (That is exactly what the first version
	 * of this test did, 2026-08-28.) */
	check(client.open() && client.bind(0), "reopen THE CLOSED instance");

	std::array< char, 64 > buffer{};
	std::string sender;
	uint16_t senderPort = 0;

	const auto start = Clock::now();
	client.receive(buffer.data(), buffer.size(), sender, senderPort, 300);
	const auto reopenedTook = elapsedMs(start);

	check(reopenedTook >= 290, "the reopened instance waits again (open() cleared the closing flag)", std::to_string(reopenedTook) + " ms");
}

static void
testMovedFromInstance ()
{
	title("5b. a moved-from instance is safe to use and destroy");

	UDPClient moved;

	{
		UDPClient source;
		source.open();
		source.bind(0);

		moved = std::move(source);

		/* 'source' is moved-from: these must not dereference the guards it gave away,
		 * and its destructor runs close() at the end of this scope. */
		std::array< char, 64 > buffer{};
		std::string sender;
		uint16_t senderPort = 0;

		check(source.send("127.0.0.1", 9, "x") == -1, "send() on a moved-from instance returns -1");         // NOLINT(bugprone-use-after-move)
		check(source.receive(buffer.data(), buffer.size(), sender, senderPort, 0) == -1, "receive() on a moved-from instance returns -1"); // NOLINT(bugprone-use-after-move)

		source.close();                                                                                      // NOLINT(bugprone-use-after-move)
		check(true, "close() on a moved-from instance does not crash");
	}

	check(true, "destroying a moved-from instance does not crash");
	check(moved.isOpen(), "the target kept the socket");

	/* And the moved-from one can be brought back to life. */
	UDPClient revived;
	UDPClient sink{std::move(revived)};

	check(revived.open() && revived.bind(0), "a moved-from instance can be reopened");  // NOLINT(bugprone-use-after-move)
	check(revived.isOpen(), "and it is usable again");
}

static void
testMDNS (const std::vector< NetworkInterfaces::Interface > & interfaces)
{
	title("6. mDNS — bind 5353 alongside mDNSResponder, join 224.0.0.251, real DNS-SD query");

	UDPClient client;

	check(client.open(), "open()");

	const auto bound = client.bind(5353);
	check(bound, "bind(5353) while the system mDNSResponder holds it", bound ? "SO_REUSEPORT works" : "EADDRINUSE — port sharing refused");

	if ( !bound )
	{
		warn("cannot continue the mDNS round trip without the port");
		return;
	}

	check(client.setMulticastTTL(255), "setMulticastTTL(255) — mandated by RFC 6762");
	check(client.setMulticastLoopback(true), "setMulticastLoopback(true)");

	int joined = 0;

	for ( const auto & entry : interfaces )
	{
		if ( client.joinMulticastGroup("224.0.0.251", entry.address) )
		{
			++joined;
			info("joined 224.0.0.251 on " + entry.name + " (" + entry.address + ")");
		}
		else
		{
			warn("join refused on " + entry.name + " (" + entry.address + ")", lastSocketError());
		}
	}

	check(joined > 0, "joined the mDNS group on at least one interface", std::to_string(joined) + " interface(s)");

	/* Idempotency + tolerant leave, both documented contract points. */
	if ( !interfaces.empty() )
	{
		check(client.joinMulticastGroup("224.0.0.251", interfaces.front().address), "re-joining an already-joined group returns true (idempotent)");
		check(client.leaveMulticastGroup("239.9.9.9", interfaces.front().address), "leaving a never-joined group returns true (tolerant)");
	}

	check(!client.joinMulticastGroup("224.0.0.251", "en0"), "an interface NAME is rejected (addresses only)");

	/* The real query. */
	const auto query = buildDNSSDQuery("_services._dns-sd._udp.local");
	const auto sent = client.send("224.0.0.251", 5353, query.data(), query.size());

	check(sent == static_cast< int >(query.size()), "sent the DNS-SD PTR query to 224.0.0.251:5353", std::to_string(sent) + " bytes");

	std::cout << "         listening 4 s for answers...\n";

	const auto deadline = Clock::now() + std::chrono::seconds(4);
	int datagrams = 0;
	int withDestination = 0;
	int withInterfaceIndex = 0;
	int multicastFlagged = 0;
	std::set< std::string > responders;
	std::set< std::string > services;

	while ( Clock::now() < deadline )
	{
		std::array< char, 4096 > buffer{};
		DatagramInfo datagramInfo;

		const auto received = client.receive(buffer.data(), buffer.size(), datagramInfo, 500);

		if ( received <= 0 )
		{
			continue;
		}

		++datagrams;

		if ( !datagramInfo.destinationAddress.empty() ) { ++withDestination; }
		if ( datagramInfo.interfaceIndex != 0 ) { ++withInterfaceIndex; }
		if ( datagramInfo.multicast ) { ++multicastFlagged; }

		responders.insert(datagramInfo.senderAddress);

		for ( const auto & name : parseAnswerNames(reinterpret_cast< const uint8_t * >(buffer.data()), static_cast< size_t >(received)) )
		{
			services.insert(name);
		}

		if ( datagrams <= 6 )
		{
			std::cout << "         datagram #" << datagrams
				<< " from " << datagramInfo.senderAddress << ":" << datagramInfo.senderPort
				<< " -> " << ( datagramInfo.destinationAddress.empty() ? "(unknown)" : datagramInfo.destinationAddress )
				<< " ifindex=" << datagramInfo.interfaceIndex
				<< ( datagramInfo.multicast ? " [multicast]" : " [unicast]" )
				<< " " << received << " bytes\n";
		}
	}

	check(datagrams > 0, "received mDNS traffic", std::to_string(datagrams) + " datagram(s) from " + std::to_string(responders.size()) + " host(s)");

	if ( datagrams > 0 )
	{
		check(withDestination == datagrams, "IP_RECVDSTADDR yields the destination on every datagram", std::to_string(withDestination) + "/" + std::to_string(datagrams));
		check(withInterfaceIndex == datagrams, "IP_RECVIF yields a NON-ZERO interface index on every datagram", std::to_string(withInterfaceIndex) + "/" + std::to_string(datagrams));
		check(multicastFlagged > 0, "the multicast flag is set on group-addressed datagrams", std::to_string(multicastFlagged) + "/" + std::to_string(datagrams));

		std::cout << "         responders: ";
		for ( const auto & responder : responders ) { std::cout << responder << ' '; }
		std::cout << '\n';

		int printed = 0;

		for ( const auto & service : services )
		{
			if ( printed++ >= 12 ) { break; }
			std::cout << "         service: " << service << '\n';
		}

		check(!services.empty(), "decoded service records from the answers", std::to_string(services.size()) + " record(s)");
	}
	else
	{
		warn("no answer received", "no mDNS device on this LAN, or the Local Network privacy prompt was denied");
	}

	client.close();
}

static void
testSSDP ()
{
	title("7. UDPClient::ssdpDiscover()");

	const auto start = Clock::now();
	const auto devices = UDPClient::ssdpDiscover("ssdp:all", 3);
	const auto took = elapsedMs(start);

	info("discovery took " + std::to_string(took) + " ms, " + std::to_string(devices.size()) + " device(s)");

	if ( devices.empty() )
	{
		warn("no SSDP/UPnP device answered", "not conclusive — depends on what is on this LAN");
		return;
	}

	int printed = 0;

	for ( const auto & device : devices )
	{
		if ( printed++ >= 8 ) { break; }

		std::string server;

		if ( const auto it = device.headers.find("SERVER"); it != device.headers.end() )
		{
			server = it->second;
		}

		std::cout << "         " << device.address << ":" << device.port << "  " << server << '\n';
	}

	check(true, "SSDP discovery answered", std::to_string(devices.size()) + " device(s)");
}

int
main ()
{
	if ( !startupSockets() )
	{
		std::cout << "WSAStartup() failed — cannot run.\n";

		return 1;
	}

	std::cout << "EmEn::Net runtime check — UDP multicast / mDNS / interfaces\n";

#if defined(_WIN32)
	std::cout << "platform: Windows\n";
#elif defined(__APPLE__)
	std::cout << "platform: macOS/BSD\n";
#else
	std::cout << "platform: Linux\n";
#endif

	const auto interfaces = testInterfaceEnumeration();
	testMulticastTTLByteType();
	testNonBlockingReceive();
	testCloseWakesReceive();
	testMovedFromInstance();
	testMDNS(interfaces);
	testSSDP();

	std::cout << "\n=== Summary ===\n"
		<< "  pass: " << g_pass << "\n"
		<< "  fail: " << g_fail << "\n"
		<< "  warn: " << g_warn << "\n";

	return g_fail == 0 ? 0 : 1;
}
