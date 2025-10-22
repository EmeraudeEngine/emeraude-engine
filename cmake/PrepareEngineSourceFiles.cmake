# Base files
set(EMERAUDE_HEADER_FILES
    # Libs
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Libs/Algorithms/DiamondSquare.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Libs/Algorithms/Mandelbrot.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Libs/Algorithms/PerlinNoise.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Libs/Compression/LZMA/Compressor.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Libs/Compression/LZMA/Decompressor.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Libs/Compression/LZMA.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Libs/Compression/ZLIB.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Libs/Debug/ConcurrencyDetector.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Libs/Debug/Dummy.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Libs/Debug/Statistics.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Libs/GameTools/CardDeck.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Libs/GameTools/CardHand.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Libs/GameTools/Dice.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Libs/Hash/FNV1a.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Libs/Hash/Hash.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Libs/Hash/MD5.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Libs/Hash/SHA256.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Libs/Hash/SHA512.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Libs/IO/IO.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Libs/IO/ZipReader.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Libs/IO/ZipWriter.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Libs/Math/Space2D/Collisions/CircleRectangle.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Libs/Math/Space2D/Collisions/PointCircle.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Libs/Math/Space2D/Collisions/PointRectangle.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Libs/Math/Space2D/Collisions/PointTriangle.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Libs/Math/Space2D/Collisions/SamePrimitive.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Libs/Math/Space2D/Collisions/TriangleCircle.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Libs/Math/Space2D/Collisions/TriangleRectangle.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Libs/Math/Space2D/Intersections/LineCircle.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Libs/Math/Space2D/Intersections/LineLine.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Libs/Math/Space2D/Intersections/LineRectangle.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Libs/Math/Space2D/Intersections/LineTriangle.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Libs/Math/Space2D/Intersections/SegmentCircle.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Libs/Math/Space2D/Intersections/SegmentRectangle.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Libs/Math/Space2D/Intersections/SegmentSegment.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Libs/Math/Space2D/Intersections/SegmentTriangle.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Libs/Math/Space2D/AARectangle.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Libs/Math/Space2D/Circle.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Libs/Math/Space2D/Line.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Libs/Math/Space2D/Point.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Libs/Math/Space2D/Segment.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Libs/Math/Space2D/Triangle.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Libs/Math/Space3D/Collisions/PointCuboid.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Libs/Math/Space3D/Collisions/PointSphere.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Libs/Math/Space3D/Collisions/PointTriangle.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Libs/Math/Space3D/Collisions/SamePrimitive.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Libs/Math/Space3D/Collisions/SphereCuboid.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Libs/Math/Space3D/Collisions/TriangleCuboid.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Libs/Math/Space3D/Collisions/TriangleSphere.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Libs/Math/Space3D/Intersections/LineCuboid.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Libs/Math/Space3D/Intersections/LineLine.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Libs/Math/Space3D/Intersections/LineSphere.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Libs/Math/Space3D/Intersections/LineTriangle.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Libs/Math/Space3D/Intersections/SegmentCuboid.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Libs/Math/Space3D/Intersections/SegmentSegment.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Libs/Math/Space3D/Intersections/SegmentSphere.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Libs/Math/Space3D/Intersections/SegmentTriangle.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Libs/Math/Space3D/AACuboid.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Libs/Math/Space3D/Line.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Libs/Math/Space3D/Point.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Libs/Math/Space3D/Segment.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Libs/Math/Space3D/Sphere.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Libs/Math/Space3D/Triangle.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Libs/Math/Base.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Libs/Math/BezierCurve.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Libs/Math/BSpline.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Libs/Math/CartesianFrame.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Libs/Math/DeterminantAverage.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Libs/Math/LineFormula.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Libs/Math/Matrix.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Libs/Math/OrientedCuboid.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Libs/Math/Plane.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Libs/Math/Quaternion.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Libs/Math/Range.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Libs/Math/Vector.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Libs/Network/asio_throw_exception.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Libs/Network/Hostname.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Libs/Network/HTTPHeaders.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Libs/Network/HTTPRequest.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Libs/Network/HTTPResponse.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Libs/Network/Network.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Libs/Network/Query.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Libs/Network/URI.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Libs/Network/URIDomain.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Libs/Network/URL.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Libs/PixelFactory/Color.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Libs/PixelFactory/DefaultFont.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Libs/PixelFactory/FileFormatInterface.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Libs/PixelFactory/FileFormatJpeg.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Libs/PixelFactory/FileFormatPNG.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Libs/PixelFactory/FileFormatTarga.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Libs/PixelFactory/FileIO.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Libs/PixelFactory/Font.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Libs/PixelFactory/Gradient.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Libs/PixelFactory/Margin.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Libs/PixelFactory/Pixmap.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Libs/PixelFactory/Processor.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Libs/PixelFactory/TextProcessor.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Libs/PixelFactory/Types.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Libs/Time/Elapsed/Abstract.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Libs/Time/Elapsed/CPUTime.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Libs/Time/Elapsed/PrintScopeCPUTime.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Libs/Time/Elapsed/PrintScopeRealTime.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Libs/Time/Elapsed/RealTime.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Libs/Time/Elapsed/ScopeCPUTime.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Libs/Time/Elapsed/ScopeRealTime.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Libs/Time/Statistics/Abstract.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Libs/Time/Statistics/CPUTime.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Libs/Time/Statistics/RealTime.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Libs/Time/EventTrait.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Libs/Time/Time.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Libs/Time/TimedEvent.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Libs/Time/Types.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Libs/VertexFactory/FileFormatFBX.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Libs/VertexFactory/FileFormatInterface.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Libs/VertexFactory/FileFormatMD2.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Libs/VertexFactory/FileFormatMD3.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Libs/VertexFactory/FileFormatMD5.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Libs/VertexFactory/FileFormatMDL.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Libs/VertexFactory/FileFormatNative.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Libs/VertexFactory/FileFormatOBJ.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Libs/VertexFactory/FileIO.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Libs/VertexFactory/Grid.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Libs/VertexFactory/GridQuad.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Libs/VertexFactory/Normal.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Libs/VertexFactory/Shape.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Libs/VertexFactory/ShapeAssembler.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Libs/VertexFactory/ShapeBuilder.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Libs/VertexFactory/ShapeBuilderOptions.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Libs/VertexFactory/ShapeEdge.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Libs/VertexFactory/ShapeGenerator.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Libs/VertexFactory/ShapeTriangle.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Libs/VertexFactory/ShapeVertex.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Libs/VertexFactory/Silhouette.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Libs/VertexFactory/TextureCoordinates.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Libs/VertexFactory/TreeGenerator.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Libs/VertexFactory/TriangleGenerator.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Libs/VertexFactory/Types.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Libs/WaveFactory/Processor.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Libs/WaveFactory/Types.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Libs/WaveFactory/Wave.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Libs/BlobTrait.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Libs/FastJSON.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Libs/FlagArrayTrait.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Libs/FlagTrait.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Libs/KVParser.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Libs/NameableTrait.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Libs/NodeTrait.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Libs/ObservableTrait.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Libs/ObserverTrait.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Libs/Randomizer.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Libs/SourceCodeParser.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Libs/StaticVector.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Libs/std_source_location.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Libs/String.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Libs/ThreadPool.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Libs/Utility.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Libs/Variant.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Libs/Version.hpp
    # Net
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Net/CachedDownloadItem.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Net/DownloadItem.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Net/Manager.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Net/Types.hpp
    # PlatformSpecific
    ${CMAKE_CURRENT_SOURCE_DIR}/src/PlatformSpecific/Helpers.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/PlatformSpecific/SystemInfo.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/PlatformSpecific/Types.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/PlatformSpecific/UserInfo.hpp
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
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Tracer.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/User.hpp
)

set(EMERAUDE_SOURCE_FILES
    # Libs
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Libs/Compression/LZMA/Compressor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Libs/Compression/LZMA/Decompressor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Libs/Compression/LZMA.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Libs/Compression/ZLIB.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Libs/Debug/Statistics.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Libs/GameTools/CardDeck.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Libs/Hash/Hash.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Libs/Hash/MD5.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Libs/Hash/SHA256.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Libs/Hash/SHA512.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Libs/IO/IO.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Libs/IO/ZipReader.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Libs/IO/ZipWriter.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Libs/Network/Hostname.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Libs/Network/HTTPHeaders.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Libs/Network/HTTPRequest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Libs/Network/HTTPResponse.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Libs/Network/Network.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Libs/Network/Query.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Libs/Network/URI.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Libs/Network/URIDomain.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Libs/Time/Statistics/Abstract.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Libs/Time/Statistics/CPUTime.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Libs/VertexFactory/TreeGenerator.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Libs/WaveFactory/Processor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Libs/WaveFactory/Wave.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Libs/FastJSON.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Libs/KVParser.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Libs/ObservableTrait.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Libs/ObserverTrait.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Libs/SourceCodeParser.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Libs/String.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Libs/ThreadPool.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Libs/Variant.cpp
    # Net
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Net/Manager.cpp
    # PlatformSpecific
    ${CMAKE_CURRENT_SOURCE_DIR}/src/PlatformSpecific/SystemInfo.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/PlatformSpecific/UserInfo.cpp
    # ROOT
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Arguments.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/FileSystem.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/PrimaryServices.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Settings.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Tracer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/User.cpp
)

# Per-OS files
if ( UNIX AND NOT APPLE )
    message("Prepare sources for building Emeraude-Engine for Linux.")

    list(APPEND EMERAUDE_SOURCE_FILES
        # PlatformSpecific
        ${CMAKE_CURRENT_SOURCE_DIR}/src/PlatformSpecific/Helpers.linux.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/PlatformSpecific/SystemInfo.linux.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/PlatformSpecific/UserInfo.linux.cpp
    )
endif ()

if ( APPLE )
    message("Prepare sources for building Emeraude-Engine for macOS.")

    list(APPEND EMERAUDE_SOURCE_FILES
        # PlatformSpecific
        ${CMAKE_CURRENT_SOURCE_DIR}/src/PlatformSpecific/Helpers.mac.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/PlatformSpecific/SystemInfo.mac.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/PlatformSpecific/UserInfo.mac.cpp
    )
endif ()

if ( MSVC )
    message("Prepare sources for building Emeraude-Engine for Windows.")

    list(APPEND EMERAUDE_SOURCE_FILES
        # PlatformSpecific
        ${CMAKE_CURRENT_SOURCE_DIR}/src/PlatformSpecific/Helpers.windows.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/PlatformSpecific/SystemInfo.windows.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/PlatformSpecific/UserInfo.windows.cpp
    )
endif ()

if ( EMERAUDE_BUILD_SERVICES_ONLY )
    message("Prepare sources for building Emeraude-Engine as a service provider.")
else ()
    message("Prepare sources for building Emeraude-Engine.")

    # Globbing
    set(
        EMERAUDE_DIRECTORIES
        ${CMAKE_CURRENT_SOURCE_DIR}/src/Animations
        ${CMAKE_CURRENT_SOURCE_DIR}/src/Audio
        ${CMAKE_CURRENT_SOURCE_DIR}/src/AVConsole
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
        ${CMAKE_CURRENT_SOURCE_DIR}/src/PlatformSpecific/Desktop/Dialog/Message.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/PlatformSpecific/Desktop/Dialog/OpenFile.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/PlatformSpecific/Desktop/Dialog/SaveFile.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/PlatformSpecific/Desktop/Dialog/Types.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/PlatformSpecific/Desktop/Commands.hpp
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
        ${CMAKE_CURRENT_SOURCE_DIR}/src/PlatformSpecific/Desktop/Dialog/Abstract.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/PlatformSpecific/Desktop/Dialog/Message.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/PlatformSpecific/Desktop/Dialog/OpenFile.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/PlatformSpecific/Desktop/Dialog/SaveFile.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/PlatformSpecific/Desktop/Dialog/Types.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/PlatformSpecific/Desktop/Commands.cpp
        # ROOT
        ${CMAKE_CURRENT_SOURCE_DIR}/src/Core.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/CursorAtlas.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/Help.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/PlatformManager.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/Notifier.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/Window.cpp
    )
    
    # Per-OS files
    if ( UNIX AND NOT APPLE )
        message("Prepare sources for building Emeraude-Engine for Linux.")

        list(APPEND EMERAUDE_SOURCE_FILES
            # PlatformSpecific
            ${CMAKE_CURRENT_SOURCE_DIR}/src/PlatformSpecific/Desktop/Dialog/Message.linux.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/src/PlatformSpecific/Desktop/Dialog/OpenFile.linux.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/src/PlatformSpecific/Desktop/Dialog/SaveFile.linux.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/src/PlatformSpecific/Desktop/Commands.linux.cpp
            # ROOT
            ${CMAKE_CURRENT_SOURCE_DIR}/src/Window.linux.cpp
        )
    endif ()

    if ( APPLE )
        message("Prepare sources for building Emeraude-Engine for macOS.")

        list(APPEND EMERAUDE_SOURCE_FILES
            # PlatformSpecific
            ${CMAKE_CURRENT_SOURCE_DIR}/src/PlatformSpecific/Desktop/Dialog/Message.mac.mm
            ${CMAKE_CURRENT_SOURCE_DIR}/src/PlatformSpecific/Desktop/Dialog/OpenFile.mac.mm
            ${CMAKE_CURRENT_SOURCE_DIR}/src/PlatformSpecific/Desktop/Dialog/SaveFile.mac.mm
            ${CMAKE_CURRENT_SOURCE_DIR}/src/PlatformSpecific/Desktop/Commands.mac.mm
            # ROOT
            ${CMAKE_CURRENT_SOURCE_DIR}/src/Window.mac.mm
        )
    endif ()

    if ( MSVC )
        message("Prepare sources for building Emeraude-Engine for Windows.")

        list(APPEND EMERAUDE_SOURCE_FILES
            # PlatformSpecific
            ${CMAKE_CURRENT_SOURCE_DIR}/src/PlatformSpecific/Desktop/Dialog/Message.windows.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/src/PlatformSpecific/Desktop/Dialog/OpenFile.windows.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/src/PlatformSpecific/Desktop/Dialog/SaveFile.windows.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/src/PlatformSpecific/Desktop/Commands.windows.cpp
            # ROOT
            ${CMAKE_CURRENT_SOURCE_DIR}/src/Window.windows.cpp
        )
    endif ()
endif ()