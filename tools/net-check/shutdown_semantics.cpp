/* Why doesn't close() wake a parked receive() on macOS? Probe shutdown() semantics. */

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cerrno>
#include <chrono>
#include <cstring>
#include <iostream>
#include <thread>

using Clock = std::chrono::steady_clock;

static long
elapsed (Clock::time_point start)
{
	return std::chrono::duration_cast< std::chrono::milliseconds >(Clock::now() - start).count();
}

static void
probe (const char * label, int type, bool connectIt, bool listenIt)
{
	const auto fd = socket(AF_INET, type, 0);

	sockaddr_in address{};
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = 0;

	if ( bind(fd, reinterpret_cast< sockaddr * >(&address), sizeof(address)) != 0 )
	{
		std::cout << label << ": bind failed\n";
		close(fd);
		return;
	}

	if ( listenIt )
	{
		listen(fd, 4);
	}

	if ( connectIt )
	{
		socklen_t length = sizeof(address);
		getsockname(fd, reinterpret_cast< sockaddr * >(&address), &length);
		address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
		connect(fd, reinterpret_cast< sockaddr * >(&address), sizeof(address));
	}

	long took = 0;
	int selectResult = 0;

	std::thread parked([fd, &took, &selectResult] {
		fd_set readFds;
		FD_ZERO(&readFds);
		FD_SET(fd, &readFds);

		timeval tv{};
		tv.tv_sec = 3;

		const auto start = Clock::now();
		selectResult = select(fd + 1, &readFds, nullptr, nullptr, &tv);
		took = elapsed(start);
	});

	std::this_thread::sleep_for(std::chrono::milliseconds(200));

	errno = 0;
	const auto shutdownResult = ::shutdown(fd, SHUT_RDWR);
	const auto shutdownErrno = errno;

	parked.join();
	close(fd);

	std::cout << label
		<< ": shutdown() -> " << shutdownResult
		<< " (" << ( shutdownResult == 0 ? "ok" : std::strerror(shutdownErrno) ) << ")"
		<< ", select returned " << selectResult << " after " << took << " ms"
		<< ( took < 1000 ? "   <= WOKEN" : "   <= NOT woken, full timeout" )
		<< '\n';
}

int
main ()
{
	std::cout << "shutdown() wakeup semantics on this kernel\n\n";

	probe("UDP, bound, unconnected  ", SOCK_DGRAM, false, false);
	probe("UDP, bound, connected    ", SOCK_DGRAM, true, false);
	probe("TCP, bound, listening    ", SOCK_STREAM, false, true);

	return 0;
}
