## Important Files

- `Manager.cpp/.hpp` + `Manager.console.cpp`, `DownloadItem.hpp`, `Types.hpp` - Download manager (HTTPS, URL-keyed cache, main-thread notifications)
- `APIClient.cpp/.hpp` + `APIClient.console.cpp`, `APIRequestItem.hpp`, `Types.hpp` - Web API client (arbitrary HTTPS exchanges, in-memory responses, ticket retention)
- `UDPClient.hpp/.cpp` - UDP client, IPv4 multicast and SSDP discovery
- `NetworkInterfaces.hpp/.cpp` - Local IPv4/IPv6 address enumeration with MAC (feeds the multicast API and scripting bridges)
- `TCPClient.hpp/.cpp` - TCP client (Asio-based, blocking-with-timeout API)
- `TCPServer.hpp/.cpp` - TCP server (Asio-based, accept returns owned TCPClient)
- `SerialPort.hpp` + `.{linux,mac,windows}.cpp` - Serial port abstraction
- `WiFiScanner.hpp` + `.{linux,mac,windows}.cpp` - WiFi scanning
- Download cache: `cacheDirectory("downloads")/<hash>.<ext>` + `downloads/index.json` — owned by the manager, the only one
