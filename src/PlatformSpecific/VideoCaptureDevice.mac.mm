/*
 * src/PlatformSpecific/VideoCaptureDevice.mac.mm
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
 * https://github.com/londnoir/emeraude-engine
 *
 * --- THIS IS AUTOMATICALLY GENERATED, DO NOT CHANGE ---
 */

/* Emeraude-Engine configuration. */
#include "emeraude_config.hpp"

#if IS_MACOS

/* Local inclusions. */
#include "VideoCaptureDevice.hpp"
#include "Tracer.hpp"

/* STL inclusions. */
#include <condition_variable>
#include <mutex>
#include <vector>

/* System inclusions. */
#import <AVFoundation/AVFoundation.h>
#import <CoreMedia/CoreMedia.h>
#import <CoreVideo/CoreVideo.h>

namespace EmEn::PlatformSpecific
{
	constexpr auto TracerTag{"VideoCaptureDevice"};
}

/* Objective-C delegate that receives frames asynchronously from AVFoundation
 * and stores the latest one in a mutex-protected buffer. */
@interface FrameCaptureDelegate : NSObject <AVCaptureVideoDataOutputSampleBufferDelegate>
{
	std::mutex _frameMutex;
	std::condition_variable _frameCondition;
	std::vector< uint8_t > _latestBGRA;
	uint32_t _frameWidth;
	uint32_t _frameHeight;
	bool _hasNewFrame;
	bool _firstFrameReceived;
}

- (void)captureOutput:(AVCaptureOutput *)output
  didOutputSampleBuffer:(CMSampleBufferRef)sampleBuffer
  fromConnection:(AVCaptureConnection *)connection;

- (bool)copyLatestFrameRGBA:(std::vector< uint8_t > &)rgbaOutput
  width:(uint32_t &)outWidth
  height:(uint32_t &)outHeight;

- (bool)waitForFirstFrame:(uint32_t)timeoutMs;

@end

@implementation FrameCaptureDelegate

- (instancetype)init
{
	self = [super init];

	if ( self )
	{
		_frameWidth = 0;
		_frameHeight = 0;
		_hasNewFrame = false;
		_firstFrameReceived = false;
	}

	return self;
}

- (void)captureOutput:(AVCaptureOutput *)output
  didOutputSampleBuffer:(CMSampleBufferRef)sampleBuffer
  fromConnection:(AVCaptureConnection *)connection
{
	(void)output;
	(void)connection;

	CVPixelBufferRef pixelBuffer = CMSampleBufferGetImageBuffer(sampleBuffer);

	if ( pixelBuffer == nullptr )
	{
		return;
	}

	CVPixelBufferLockBaseAddress(pixelBuffer, kCVPixelBufferLock_ReadOnly);

	const auto width = static_cast< uint32_t >(CVPixelBufferGetWidth(pixelBuffer));
	const auto height = static_cast< uint32_t >(CVPixelBufferGetHeight(pixelBuffer));
	const auto bytesPerRow = CVPixelBufferGetBytesPerRow(pixelBuffer);
	const auto * baseAddress = static_cast< const uint8_t * >(CVPixelBufferGetBaseAddress(pixelBuffer));

	const size_t dataSize = static_cast< size_t >(width) * height * 4;

	{
		std::lock_guard< std::mutex > lock(_frameMutex);

		_latestBGRA.resize(dataSize);
		_frameWidth = width;
		_frameHeight = height;

		/* Copy row by row to handle stride/padding differences. */
		for ( uint32_t row = 0; row < height; ++row )
		{
			const auto * srcRow = baseAddress + (row * bytesPerRow);
			auto * dstRow = _latestBGRA.data() + (row * width * 4);

			std::memcpy(dstRow, srcRow, width * 4);
		}

		_hasNewFrame = true;

		if ( !_firstFrameReceived )
		{
			_firstFrameReceived = true;
			_frameCondition.notify_one();
		}
	}

	CVPixelBufferUnlockBaseAddress(pixelBuffer, kCVPixelBufferLock_ReadOnly);
}

- (bool)copyLatestFrameRGBA:(std::vector< uint8_t > &)rgbaOutput
  width:(uint32_t &)outWidth
  height:(uint32_t &)outHeight
{
	std::lock_guard< std::mutex > lock(_frameMutex);

	if ( !_hasNewFrame || _latestBGRA.empty() )
	{
		return false;
	}

	const size_t pixelCount = static_cast< size_t >(_frameWidth) * _frameHeight;

	rgbaOutput.resize(pixelCount * 4);

	/* Convert BGRA -> RGBA by swapping B and R channels. */
	for ( size_t i = 0; i < pixelCount; ++i )
	{
		const size_t offset = i * 4;

		rgbaOutput[offset + 0] = _latestBGRA[offset + 2]; /* R <- B */
		rgbaOutput[offset + 1] = _latestBGRA[offset + 1]; /* G */
		rgbaOutput[offset + 2] = _latestBGRA[offset + 0]; /* B <- R */
		rgbaOutput[offset + 3] = _latestBGRA[offset + 3]; /* A */
	}

	outWidth = _frameWidth;
	outHeight = _frameHeight;
	_hasNewFrame = false;

	return true;
}

- (bool)waitForFirstFrame:(uint32_t)timeoutMs
{
	std::unique_lock< std::mutex > lock(_frameMutex);

	return _frameCondition.wait_for(lock, std::chrono::milliseconds(timeoutMs), [self]() {
		return _firstFrameReceived;
	});
}

@end

namespace EmEn::PlatformSpecific
{
	/* Platform-specific context stored via m_platformHandle. */
	struct MacCaptureContext
	{
		AVCaptureSession * session{nil};
		AVCaptureDeviceInput * input{nil};
		AVCaptureVideoDataOutput * output{nil};
		FrameCaptureDelegate * delegate{nil};
		dispatch_queue_t captureQueue{nullptr};
	};

	VideoCaptureDevice::~VideoCaptureDevice () noexcept
	{
		this->close();
	}

	std::vector< VideoCaptureDeviceInfo >
	VideoCaptureDevice::enumerateDevices () noexcept
	{
		std::vector< VideoCaptureDeviceInfo > devices;

		@autoreleasepool
		{
			/* AVCaptureDeviceTypeExternalUnknown is deprecated since macOS 14.0 in favour of
			 * AVCaptureDeviceTypeExternal, but the latter requires a deployment target of 14.0+.
			 * Since we target macOS 12.0, we keep using the older symbol which still works. */
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
			AVCaptureDeviceDiscoverySession * discoverySession = [AVCaptureDeviceDiscoverySession
				discoverySessionWithDeviceTypes:@[
					AVCaptureDeviceTypeBuiltInWideAngleCamera,
					AVCaptureDeviceTypeExternalUnknown
				]
				mediaType:AVMediaTypeVideo
				position:AVCaptureDevicePositionUnspecified];
#pragma clang diagnostic pop
			NSArray< AVCaptureDevice * > * captureDevices = [discoverySession devices];

			uint32_t index = 0;

			for ( AVCaptureDevice * device in captureDevices )
			{
				VideoCaptureDeviceInfo info;
				info.devicePath = std::string([[device uniqueID] UTF8String]);
				info.deviceName = std::string([[device localizedName] UTF8String]);
				info.index = index++;

				devices.emplace_back(std::move(info));
			}
		}

		if ( devices.empty() )
		{
			TraceInfo{TracerTag} << "No video capture devices found on macOS.";
		}
		else
		{
			TraceInfo{TracerTag} << "Found " << devices.size() << " video capture device(s) on macOS.";
		}

		return devices;
	}

	bool
	VideoCaptureDevice::open (const std::string & devicePath, uint32_t requestedWidth, uint32_t requestedHeight) noexcept
	{
		if ( m_isOpen )
		{
			TraceWarning{TracerTag} << "Device already open. Close it first.";

			return false;
		}

		@autoreleasepool
		{
			/* Find the device by uniqueID. */
			NSString * deviceID = [NSString stringWithUTF8String:devicePath.c_str()];
			AVCaptureDevice * device = [AVCaptureDevice deviceWithUniqueID:deviceID];

			if ( device == nil )
			{
				TraceError{TracerTag} << "Failed to find video device with ID '" << devicePath << "' on macOS.";

				return false;
			}

			/* Create the capture context. */
			auto * context = new (std::nothrow) MacCaptureContext{};

			if ( context == nullptr )
			{
				TraceError{TracerTag} << "Failed to allocate MacCaptureContext.";

				return false;
			}

			/* Create input from device. */
			NSError * error = nil;
			context->input = [AVCaptureDeviceInput deviceInputWithDevice:device error:&error];

			if ( context->input == nil )
			{
				TraceError{TracerTag} << "Failed to create capture input for '" << devicePath << "': "
					<< [[error localizedDescription] UTF8String];

				delete context;

				return false;
			}

			/* Create and configure the session. */
			context->session = [[AVCaptureSession alloc] init];

			/* Select session preset based on requested resolution. */
			if ( requestedWidth <= 640 && requestedHeight <= 480 )
			{
				if ( [context->session canSetSessionPreset:AVCaptureSessionPreset640x480] )
				{
					[context->session setSessionPreset:AVCaptureSessionPreset640x480];
				}
			}
			else if ( requestedWidth <= 1280 && requestedHeight <= 720 )
			{
				if ( [context->session canSetSessionPreset:AVCaptureSessionPreset1280x720] )
				{
					[context->session setSessionPreset:AVCaptureSessionPreset1280x720];
				}
			}
			else if ( requestedWidth <= 1920 && requestedHeight <= 1080 )
			{
				if ( [context->session canSetSessionPreset:AVCaptureSessionPreset1920x1080] )
				{
					[context->session setSessionPreset:AVCaptureSessionPreset1920x1080];
				}
			}
			else
			{
				if ( [context->session canSetSessionPreset:AVCaptureSessionPresetHigh] )
				{
					[context->session setSessionPreset:AVCaptureSessionPresetHigh];
				}
			}

			/* Add input to session. */
			if ( ![context->session canAddInput:context->input] )
			{
				TraceError{TracerTag} << "Cannot add capture input to session for '" << devicePath << "'.";

				delete context;

				return false;
			}

			[context->session addInput:context->input];

			/* Create and configure video data output. */
			context->output = [[AVCaptureVideoDataOutput alloc] init];
			[context->output setAlwaysDiscardsLateVideoFrames:YES];

			/* Request BGRA pixel format. */
			NSDictionary * videoSettings = @{
				(NSString *)kCVPixelBufferPixelFormatTypeKey : @(kCVPixelFormatType_32BGRA)
			};
			[context->output setVideoSettings:videoSettings];

			/* Create delegate and serial dispatch queue. */
			context->delegate = [[FrameCaptureDelegate alloc] init];
			context->captureQueue = dispatch_queue_create("com.emeraude.videocapture", DISPATCH_QUEUE_SERIAL);

			[context->output setSampleBufferDelegate:context->delegate queue:context->captureQueue];

			/* Add output to session. */
			if ( ![context->session canAddOutput:context->output] )
			{
				TraceError{TracerTag} << "Cannot add capture output to session for '" << devicePath << "'.";

				delete context;

				return false;
			}

			[context->session addOutput:context->output];

			/* Start the capture session. */
			[context->session startRunning];

			if ( ![context->session isRunning] )
			{
				TraceError{TracerTag} << "Capture session failed to start for '" << devicePath << "'.";

				delete context;

				return false;
			}

			/* Store context and update state. */
			m_platformHandle = context;
			m_width = requestedWidth;
			m_height = requestedHeight;
			m_isOpen = true;

			/* Wait for the first frame to arrive from the camera before returning.
			 * AVFoundation's startRunning is asynchronous, so without this the first
			 * call to captureFrame() would fail with no data available. */
			if ( ![context->delegate waitForFirstFrame:3000] )
			{
				TraceWarning{TracerTag} << "Timed out waiting for first frame from '" << devicePath << "'. "
					<< "The device is open but capture may not be ready yet.";
			}

			TraceInfo{TracerTag} << "Video capture device '" << devicePath << "' opened successfully on macOS ("
				<< m_width << "x" << m_height << ").";
		}

		return true;
	}

	void
	VideoCaptureDevice::close () noexcept
	{
		if ( !m_isOpen )
		{
			return;
		}

		@autoreleasepool
		{
			auto * context = static_cast< MacCaptureContext * >(m_platformHandle);

			if ( context != nullptr )
			{
				if ( context->session != nil )
				{
					[context->session stopRunning];
				}

				/* NOTE: The session holds strong references to inputs/outputs.
				 * Stopping and releasing the session is sufficient. */

				delete context;
			}
		}

		m_platformHandle = nullptr;
		m_isOpen = false;
		m_width = 0;
		m_height = 0;

		TraceInfo{TracerTag} << "Video capture device closed on macOS.";
	}

	bool
	VideoCaptureDevice::isOpen () const noexcept
	{
		return m_isOpen;
	}

	bool
	VideoCaptureDevice::captureFrame (std::vector< uint8_t > & rgbaOutput) noexcept
	{
		if ( !m_isOpen || m_platformHandle == nullptr )
		{
			return false;
		}

		auto * context = static_cast< MacCaptureContext * >(m_platformHandle);

		uint32_t actualWidth = 0;
		uint32_t actualHeight = 0;

		if ( ![context->delegate copyLatestFrameRGBA:rgbaOutput width:actualWidth height:actualHeight] )
		{
			return false;
		}

		/* Update dimensions to reflect the actual frame size from the camera. */
		m_width = actualWidth;
		m_height = actualHeight;

		return true;
	}
}

#endif
