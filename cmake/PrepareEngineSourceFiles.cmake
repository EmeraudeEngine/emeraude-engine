# Base files
set(EMERAUDE_HEADER_FILES
    # Net
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Net/CachedDownloadItem.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Net/DownloadItem.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Net/Manager.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Net/SerialPort.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Net/TCPClient.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Net/TCPServer.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Net/UDPClient.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Net/Types.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Net/WiFiScanner.hpp
    # PlatformSpecific
    ${CMAKE_CURRENT_SOURCE_DIR}/src/PlatformSpecific/StorageInfo.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/PlatformSpecific/Helpers.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/PlatformSpecific/SystemInfo.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/PlatformSpecific/Types.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/PlatformSpecific/UserInfo.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/PlatformSpecific/VideoCaptureDevice.hpp
    # ROOT
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Arguments.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Constants.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/emeraude_export.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/FileSystem.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Identification.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/PrimaryServices.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ServiceInterface.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/SettingKeys.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Settings.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/SystemNotification.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Tracer.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/User.hpp
)

set(EMERAUDE_SOURCE_FILES
    # Net
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Net/Manager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Net/TCPClient.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Net/TCPServer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Net/UDPClient.cpp
    # PlatformSpecific
    ${CMAKE_CURRENT_SOURCE_DIR}/src/PlatformSpecific/SystemInfo.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/PlatformSpecific/UserInfo.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/PlatformSpecific/VideoCaptureDevice.cpp
    # ROOT
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Arguments.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Arguments.console.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/FileSystem.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/FileSystem.console.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/PrimaryServices.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Settings.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Settings.console.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/SystemNotification.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Tracer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/User.cpp
)

# Per-OS files
if ( UNIX AND NOT APPLE )
    message("Prepare sources for building Emeraude-Engine for Linux.")

    list(APPEND EMERAUDE_SOURCE_FILES
        # Net
        ${CMAKE_CURRENT_SOURCE_DIR}/src/Net/SerialPort.linux.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/Net/WiFiScanner.linux.cpp
        # PlatformSpecific
        ${CMAKE_CURRENT_SOURCE_DIR}/src/PlatformSpecific/StorageInfo.linux.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/PlatformSpecific/Helpers.linux.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/PlatformSpecific/SystemInfo.linux.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/PlatformSpecific/UserInfo.linux.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/PlatformSpecific/VideoCaptureDevice.linux.cpp
    )
endif ()

if ( APPLE )
    message("Prepare sources for building Emeraude-Engine for macOS.")

    list(APPEND EMERAUDE_SOURCE_FILES
        # Net
        ${CMAKE_CURRENT_SOURCE_DIR}/src/Net/SerialPort.mac.mm
        ${CMAKE_CURRENT_SOURCE_DIR}/src/Net/WiFiScanner.mac.mm
        # PlatformSpecific
        ${CMAKE_CURRENT_SOURCE_DIR}/src/PlatformSpecific/StorageInfo.mac.mm
        ${CMAKE_CURRENT_SOURCE_DIR}/src/PlatformSpecific/Helpers.mac.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/PlatformSpecific/SystemInfo.mac.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/PlatformSpecific/UserInfo.mac.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/PlatformSpecific/VideoCaptureDevice.mac.mm
    )
endif ()

if ( MSVC )
    message("Prepare sources for building Emeraude-Engine for Windows.")

    list(APPEND EMERAUDE_SOURCE_FILES
        # Net
        ${CMAKE_CURRENT_SOURCE_DIR}/src/Net/SerialPort.windows.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/Net/WiFiScanner.windows.cpp
        # PlatformSpecific
        ${CMAKE_CURRENT_SOURCE_DIR}/src/PlatformSpecific/StorageInfo.windows.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/PlatformSpecific/Helpers.windows.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/PlatformSpecific/SystemInfo.windows.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/PlatformSpecific/UserInfo.windows.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/PlatformSpecific/VideoCaptureDevice.windows.cpp
    )
endif ()

if ( EMERAUDE_BUILD_SERVICES_ONLY )
    message("Prepare sources for building Emeraude-Engine as a service provider.")
else ()
    message("Prepare sources for building Emeraude-Engine.")

    # Globbing
    set(
        EMERAUDE_DIRECTORIES
        ${CMAKE_CURRENT_SOURCE_DIR}/src/AssetLoaders
        ${CMAKE_CURRENT_SOURCE_DIR}/src/Animations
        ${CMAKE_CURRENT_SOURCE_DIR}/src/Audio
        ${CMAKE_CURRENT_SOURCE_DIR}/src/Console
        ${CMAKE_CURRENT_SOURCE_DIR}/src/Graphics
        ${CMAKE_CURRENT_SOURCE_DIR}/src/Input
        ${CMAKE_CURRENT_SOURCE_DIR}/src/Overlay
        ${CMAKE_CURRENT_SOURCE_DIR}/src/Physics
        ${CMAKE_CURRENT_SOURCE_DIR}/src/Resources
        ${CMAKE_CURRENT_SOURCE_DIR}/src/Saphir
        ${CMAKE_CURRENT_SOURCE_DIR}/src/Scenes
        ${CMAKE_CURRENT_SOURCE_DIR}/src/Tool
        ${CMAKE_CURRENT_SOURCE_DIR}/src/Vulkan
    )

    foreach ( EMERAUDE_DIRECTORY ${EMERAUDE_DIRECTORIES} )
        file(GLOB_RECURSE EMERAUDE_DIRECTORY_EMERAUDE_HEADER_FILES ${EMERAUDE_DIRECTORY}/*.hpp)
        list(APPEND EMERAUDE_HEADER_FILES ${EMERAUDE_DIRECTORY_EMERAUDE_HEADER_FILES})

        file(GLOB_RECURSE EMERAUDE_DIRECTORY_EMERAUDE_SOURCE_FILES ${EMERAUDE_DIRECTORY}/*.cpp)
        list(APPEND EMERAUDE_SOURCE_FILES ${EMERAUDE_DIRECTORY_EMERAUDE_SOURCE_FILES})
    endforeach ()
    
    list(APPEND EMERAUDE_HEADER_FILES
        # PlatformSpecific
        ${CMAKE_CURRENT_SOURCE_DIR}/src/PlatformSpecific/Desktop/Dialog/Abstract.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/PlatformSpecific/Desktop/Dialog/CustomMessage.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/PlatformSpecific/Desktop/Dialog/Message.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/PlatformSpecific/Desktop/Dialog/OpenFile.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/PlatformSpecific/Desktop/Dialog/SaveFile.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/PlatformSpecific/Desktop/Dialog/TextInput.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/PlatformSpecific/Desktop/Dialog/Types.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/PlatformSpecific/Desktop/Commands.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/PlatformSpecific/Desktop/Notification.hpp
        # ROOT
        ${CMAKE_CURRENT_SOURCE_DIR}/src/Core.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/CursorAtlas.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/Help.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/PlatformManager.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/Notifier.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/Window.hpp
    )

    list(APPEND EMERAUDE_SOURCE_FILES
        # PlatformSpecific
        ${CMAKE_CURRENT_SOURCE_DIR}/src/PlatformSpecific/Desktop/Dialog/Types.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/PlatformSpecific/Desktop/Commands.cpp
        # ROOT
        ${CMAKE_CURRENT_SOURCE_DIR}/src/Core.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/Core.console.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/CursorAtlas.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/Help.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/PlatformManager.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/Notifier.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/Window.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/Window.console.cpp
    )

    # Per-OS files
    if ( UNIX AND NOT APPLE )
        message("Prepare sources for building Emeraude-Engine for Linux.")

        list(APPEND EMERAUDE_SOURCE_FILES
            # PlatformSpecific
            ${CMAKE_CURRENT_SOURCE_DIR}/src/PlatformSpecific/Desktop/Dialog/CustomMessage.linux.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/src/PlatformSpecific/Desktop/Dialog/Message.linux.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/src/PlatformSpecific/Desktop/Dialog/OpenFile.linux.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/src/PlatformSpecific/Desktop/Dialog/SaveFile.linux.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/src/PlatformSpecific/Desktop/Dialog/TextInput.linux.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/src/PlatformSpecific/Desktop/Commands.linux.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/src/PlatformSpecific/Desktop/Notification.linux.cpp
            # ROOT
            ${CMAKE_CURRENT_SOURCE_DIR}/src/Window.linux.cpp
        )
    endif ()

    if ( APPLE )
        message("Prepare sources for building Emeraude-Engine for macOS.")

        list(APPEND EMERAUDE_SOURCE_FILES
            # PlatformSpecific
            ${CMAKE_CURRENT_SOURCE_DIR}/src/PlatformSpecific/Desktop/Dialog/CustomMessage.mac.mm
            ${CMAKE_CURRENT_SOURCE_DIR}/src/PlatformSpecific/Desktop/Dialog/Message.mac.mm
            ${CMAKE_CURRENT_SOURCE_DIR}/src/PlatformSpecific/Desktop/Dialog/OpenFile.mac.mm
            ${CMAKE_CURRENT_SOURCE_DIR}/src/PlatformSpecific/Desktop/Dialog/SaveFile.mac.mm
            ${CMAKE_CURRENT_SOURCE_DIR}/src/PlatformSpecific/Desktop/Dialog/TextInput.mac.mm
            ${CMAKE_CURRENT_SOURCE_DIR}/src/PlatformSpecific/Desktop/Commands.mac.mm
            ${CMAKE_CURRENT_SOURCE_DIR}/src/PlatformSpecific/Desktop/Notification.mac.mm
            ${CMAKE_CURRENT_SOURCE_DIR}/src/Input/Manager.mac.mm
            # ROOT
            ${CMAKE_CURRENT_SOURCE_DIR}/src/Window.mac.mm
        )
    endif ()

    if ( MSVC )
        message("Prepare sources for building Emeraude-Engine for Windows.")

        list(APPEND EMERAUDE_SOURCE_FILES
            # PlatformSpecific
            ${CMAKE_CURRENT_SOURCE_DIR}/src/PlatformSpecific/Desktop/Dialog/CustomMessage.windows.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/src/PlatformSpecific/Desktop/Dialog/Message.windows.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/src/PlatformSpecific/Desktop/Dialog/OpenFile.windows.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/src/PlatformSpecific/Desktop/Dialog/SaveFile.windows.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/src/PlatformSpecific/Desktop/Dialog/TextInput.windows.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/src/PlatformSpecific/Desktop/Commands.windows.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/src/PlatformSpecific/Desktop/Notification.windows.cpp
            # ROOT
            ${CMAKE_CURRENT_SOURCE_DIR}/src/Window.windows.cpp
        )
    endif ()
endif ()