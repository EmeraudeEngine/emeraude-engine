/*
 * src/PlatformSpecific/VideoCaptureDevice.windows.cpp
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

#if IS_WINDOWS

/* Local inclusions. */
#include "VideoCaptureDevice.hpp"
#include "Helpers.hpp"
#include "Tracer.hpp"

/* System inclusions. */
#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#include <mferror.h>

namespace EmEn::PlatformSpecific
{
	constexpr auto TracerTag{"VideoCaptureDevice"};

	/* Platform-specific context stored via m_platformHandle. */
	struct MFCaptureContext
	{
		IMFSourceReader * sourceReader{nullptr};
		IMFMediaSource * mediaSource{nullptr};
		bool isRGB32{false};
		bool comInitialized{false};
		bool mfStarted{false};
	};

	/* Initializes COM and Media Foundation. Returns true on success. */
	static bool
	initializeCOMAndMF (bool & comInitialized, bool & mfStarted) noexcept
	{
		HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);

		if ( SUCCEEDED(hr) )
		{
			comInitialized = true;
		}
		else if ( hr == RPC_E_CHANGED_MODE )
		{
			/* COM already initialized with a different threading model; that's acceptable. */
			comInitialized = false;
		}
		else
		{
			TraceError{TracerTag} << "CoInitializeEx failed (HRESULT: " << hr << ").";

			return false;
		}

		hr = MFStartup(MF_VERSION);

		if ( FAILED(hr) )
		{
			TraceError{TracerTag} << "MFStartup failed (HRESULT: " << hr << ").";

			if ( comInitialized )
			{
				CoUninitialize();
				comInitialized = false;
			}

			return false;
		}

		mfStarted = true;

		return true;
	}

	/* Shuts down Media Foundation and COM. */
	static void
	shutdownCOMAndMF (bool comInitialized, bool mfStarted) noexcept
	{
		if ( mfStarted )
		{
			MFShutdown();
		}

		if ( comInitialized )
		{
			CoUninitialize();
		}
	}

	/* Converts BGRA (RGB32) data to RGBA by swapping B and R channels. */
	static void
	convertBGRAtoRGBA (const uint8_t * bgraData, size_t pixelCount, std::vector< uint8_t > & rgbaOutput) noexcept
	{
		rgbaOutput.resize(pixelCount * 4);

		for ( size_t i = 0; i < pixelCount; ++i )
		{
			const size_t offset = i * 4;

			rgbaOutput[offset + 0] = bgraData[offset + 2]; /* R <- B */
			rgbaOutput[offset + 1] = bgraData[offset + 1]; /* G */
			rgbaOutput[offset + 2] = bgraData[offset + 0]; /* B <- R */
			rgbaOutput[offset + 3] = bgraData[offset + 3]; /* A */
		}
	}

	VideoCaptureDevice::~VideoCaptureDevice () noexcept
	{
		this->close();
	}

	std::vector< VideoCaptureDeviceInfo >
	VideoCaptureDevice::enumerateDevices () noexcept
	{
		std::vector< VideoCaptureDeviceInfo > devices;

		bool comInitialized = false;
		bool mfStarted = false;

		if ( !initializeCOMAndMF(comInitialized, mfStarted) )
		{
			return devices;
		}

		/* Create attributes to request video capture devices. */
		IMFAttributes * attributes = nullptr;
		HRESULT hr = MFCreateAttributes(&attributes, 1);

		if ( FAILED(hr) )
		{
			TraceError{TracerTag} << "MFCreateAttributes failed (HRESULT: " << hr << ").";

			shutdownCOMAndMF(comInitialized, mfStarted);

			return devices;
		}

		hr = attributes->SetGUID(MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE, MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_GUID);

		if ( FAILED(hr) )
		{
			TraceError{TracerTag} << "Failed to set device source type attribute (HRESULT: " << hr << ").";

			attributes->Release();
			shutdownCOMAndMF(comInitialized, mfStarted);

			return devices;
		}

		/* Enumerate devices. */
		IMFActivate ** activateArray = nullptr;
		UINT32 deviceCount = 0;

		hr = MFEnumDeviceSources(attributes, &activateArray, &deviceCount);
		attributes->Release();

		if ( FAILED(hr) )
		{
			TraceError{TracerTag} << "MFEnumDeviceSources failed (HRESULT: " << hr << ").";

			shutdownCOMAndMF(comInitialized, mfStarted);

			return devices;
		}

		for ( UINT32 i = 0; i < deviceCount; ++i )
		{
			VideoCaptureDeviceInfo info;
			info.index = i;

			/* Get the friendly name. */
			wchar_t * friendlyName = nullptr;
			UINT32 nameLength = 0;

			hr = activateArray[i]->GetAllocatedString(MF_DEVSOURCE_ATTRIBUTE_FRIENDLY_NAME, &friendlyName, &nameLength);

			if ( SUCCEEDED(hr) && friendlyName != nullptr )
			{
				info.deviceName = convertWideToUTF8(friendlyName);
				CoTaskMemFree(friendlyName);
			}

			/* Get the symbolic link. */
			wchar_t * symbolicLink = nullptr;
			UINT32 linkLength = 0;

			hr = activateArray[i]->GetAllocatedString(MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_SYMBOLIC_LINK, &symbolicLink, &linkLength);

			if ( SUCCEEDED(hr) && symbolicLink != nullptr )
			{
				info.devicePath = convertWideToUTF8(symbolicLink);
				CoTaskMemFree(symbolicLink);
			}

			devices.emplace_back(std::move(info));

			activateArray[i]->Release();
		}

		CoTaskMemFree(activateArray);

		if ( devices.empty() )
		{
			TraceInfo{TracerTag} << "No video capture devices found on Windows.";
		}
		else
		{
			TraceInfo{TracerTag} << "Found " << devices.size() << " video capture device(s) on Windows.";
		}

		shutdownCOMAndMF(comInitialized, mfStarted);

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

		auto * context = new (std::nothrow) MFCaptureContext{};

		if ( context == nullptr )
		{
			TraceError{TracerTag} << "Failed to allocate MFCaptureContext.";

			return false;
		}

		if ( !initializeCOMAndMF(context->comInitialized, context->mfStarted) )
		{
			delete context;

			return false;
		}

		/* Create attributes to enumerate video capture devices. */
		IMFAttributes * attributes = nullptr;
		HRESULT hr = MFCreateAttributes(&attributes, 1);

		if ( FAILED(hr) )
		{
			TraceError{TracerTag} << "MFCreateAttributes failed (HRESULT: " << hr << ").";

			shutdownCOMAndMF(context->comInitialized, context->mfStarted);
			delete context;

			return false;
		}

		hr = attributes->SetGUID(MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE, MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_GUID);

		if ( FAILED(hr) )
		{
			attributes->Release();
			shutdownCOMAndMF(context->comInitialized, context->mfStarted);
			delete context;

			return false;
		}

		/* Enumerate and find the matching device. */
		IMFActivate ** activateArray = nullptr;
		UINT32 deviceCount = 0;

		hr = MFEnumDeviceSources(attributes, &activateArray, &deviceCount);
		attributes->Release();

		if ( FAILED(hr) )
		{
			TraceError{TracerTag} << "MFEnumDeviceSources failed (HRESULT: " << hr << ").";

			shutdownCOMAndMF(context->comInitialized, context->mfStarted);
			delete context;

			return false;
		}

		/* Find the device matching devicePath. */
		IMFActivate * matchedActivate = nullptr;

		for ( UINT32 i = 0; i < deviceCount; ++i )
		{
			wchar_t * symbolicLink = nullptr;
			UINT32 linkLength = 0;

			hr = activateArray[i]->GetAllocatedString(MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_SYMBOLIC_LINK, &symbolicLink, &linkLength);

			if ( SUCCEEDED(hr) && symbolicLink != nullptr )
			{
				const std::string link = convertWideToUTF8(symbolicLink);
				CoTaskMemFree(symbolicLink);

				if ( link == devicePath )
				{
					matchedActivate = activateArray[i];
					matchedActivate->AddRef();
				}
			}
		}

		/* Release all enumerated activates. */
		for ( UINT32 i = 0; i < deviceCount; ++i )
		{
			activateArray[i]->Release();
		}

		CoTaskMemFree(activateArray);

		if ( matchedActivate == nullptr )
		{
			TraceError{TracerTag} << "Failed to find video device matching '" << devicePath << "' on Windows.";

			shutdownCOMAndMF(context->comInitialized, context->mfStarted);
			delete context;

			return false;
		}

		/* Activate the media source. */
		hr = matchedActivate->ActivateObject(IID_PPV_ARGS(&context->mediaSource));
		matchedActivate->Release();

		if ( FAILED(hr) )
		{
			TraceError{TracerTag} << "Failed to activate media source (HRESULT: " << hr << ").";

			shutdownCOMAndMF(context->comInitialized, context->mfStarted);
			delete context;

			return false;
		}

		/* Create source reader from media source. */
		IMFAttributes * readerAttributes = nullptr;
		MFCreateAttributes(&readerAttributes, 1);

		hr = MFCreateSourceReaderFromMediaSource(context->mediaSource, readerAttributes, &context->sourceReader);

		if ( readerAttributes != nullptr )
		{
			readerAttributes->Release();
		}

		if ( FAILED(hr) )
		{
			TraceError{TracerTag} << "Failed to create source reader (HRESULT: " << hr << ").";

			context->mediaSource->Shutdown();
			context->mediaSource->Release();
			shutdownCOMAndMF(context->comInitialized, context->mfStarted);
			delete context;

			return false;
		}

		/* Try to configure RGB32 output format first. */
		IMFMediaType * mediaType = nullptr;
		hr = MFCreateMediaType(&mediaType);

		if ( SUCCEEDED(hr) )
		{
			mediaType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
			mediaType->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_RGB32);
			MFSetAttributeSize(mediaType, MF_MT_FRAME_SIZE, requestedWidth, requestedHeight);

			hr = context->sourceReader->SetCurrentMediaType(
				static_cast< DWORD >(MF_SOURCE_READER_FIRST_VIDEO_STREAM), nullptr, mediaType);

			if ( SUCCEEDED(hr) )
			{
				context->isRGB32 = true;
			}
			else
			{
				/* Fallback to YUY2. */
				mediaType->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_YUY2);

				hr = context->sourceReader->SetCurrentMediaType(
					static_cast< DWORD >(MF_SOURCE_READER_FIRST_VIDEO_STREAM), nullptr, mediaType);

				if ( SUCCEEDED(hr) )
				{
					context->isRGB32 = false;
				}
			}

			mediaType->Release();
		}

		if ( FAILED(hr) )
		{
			TraceError{TracerTag} << "Failed to configure output media type (HRESULT: " << hr << ").";

			context->sourceReader->Release();
			context->mediaSource->Shutdown();
			context->mediaSource->Release();
			shutdownCOMAndMF(context->comInitialized, context->mfStarted);
			delete context;

			return false;
		}

		/* Read back the actual negotiated format to get real dimensions. */
		IMFMediaType * currentType = nullptr;
		hr = context->sourceReader->GetCurrentMediaType(
			static_cast< DWORD >(MF_SOURCE_READER_FIRST_VIDEO_STREAM), &currentType);

		if ( SUCCEEDED(hr) )
		{
			UINT32 actualWidth = 0;
			UINT32 actualHeight = 0;

			MFGetAttributeSize(currentType, MF_MT_FRAME_SIZE, &actualWidth, &actualHeight);

			m_width = actualWidth;
			m_height = actualHeight;

			currentType->Release();
		}
		else
		{
			m_width = requestedWidth;
			m_height = requestedHeight;
		}

		m_platformHandle = context;
		m_isOpen = true;

		TraceInfo{TracerTag} << "Video capture device '" << devicePath << "' opened successfully on Windows ("
			<< m_width << "x" << m_height << ", " << (context->isRGB32 ? "RGB32" : "YUY2") << ").";

		return true;
	}

	void
	VideoCaptureDevice::close () noexcept
	{
		if ( !m_isOpen )
		{
			return;
		}

		auto * context = static_cast< MFCaptureContext * >(m_platformHandle);

		if ( context != nullptr )
		{
			if ( context->sourceReader != nullptr )
			{
				context->sourceReader->Release();
			}

			if ( context->mediaSource != nullptr )
			{
				context->mediaSource->Shutdown();
				context->mediaSource->Release();
			}

			shutdownCOMAndMF(context->comInitialized, context->mfStarted);

			delete context;
		}

		m_platformHandle = nullptr;
		m_isOpen = false;
		m_width = 0;
		m_height = 0;

		TraceInfo{TracerTag} << "Video capture device closed on Windows.";
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

		auto * context = static_cast< MFCaptureContext * >(m_platformHandle);

		/* Media Foundation may return S_OK with a null sample for the first few
		 * reads (stream tick / warmup). Retry a limited number of times. */
		constexpr int MaxReadAttempts = 10;
		DWORD streamIndex = 0;
		DWORD streamFlags = 0;
		LONGLONG timestamp = 0;
		IMFSample * sample = nullptr;

		for ( int attempt = 0; attempt < MaxReadAttempts; ++attempt )
		{
			HRESULT hr = context->sourceReader->ReadSample(
				static_cast< DWORD >(MF_SOURCE_READER_FIRST_VIDEO_STREAM),
				0, &streamIndex, &streamFlags, &timestamp, &sample);

			if ( FAILED(hr) )
			{
				TraceWarning{TracerTag} << "ReadSample failed (HRESULT: " << hr << ").";

				return false;
			}

			if ( sample != nullptr )
			{
				break;
			}

			/* Null sample with S_OK: stream tick or warmup gap, retry. */
		}

		if ( sample == nullptr )
		{
			TraceWarning{TracerTag} << "ReadSample returned no sample after " << MaxReadAttempts << " attempts.";

			return false;
		}

		IMFMediaBuffer * buffer = nullptr;
		HRESULT hr = sample->GetBufferByIndex(0, &buffer);

		if ( FAILED(hr) || buffer == nullptr )
		{
			sample->Release();

			TraceWarning{TracerTag} << "Failed to get buffer from sample (HRESULT: " << hr << ").";

			return false;
		}

		BYTE * rawData = nullptr;
		DWORD maxLength = 0;
		DWORD currentLength = 0;

		hr = buffer->Lock(&rawData, &maxLength, &currentLength);

		if ( FAILED(hr) )
		{
			buffer->Release();
			sample->Release();

			TraceWarning{TracerTag} << "Failed to lock media buffer (HRESULT: " << hr << ").";

			return false;
		}

		if ( context->isRGB32 )
		{
			/* RGB32 is BGRA in memory. Convert to RGBA. */
			const size_t pixelCount = static_cast< size_t >(m_width) * m_height;

			convertBGRAtoRGBA(rawData, pixelCount, rgbaOutput);
		}
		else
		{
			/* YUY2 format. Use the shared YUYV->RGBA converter. */
			convertYUYVtoRGBA(rawData, currentLength, rgbaOutput, m_width, m_height);
		}

		buffer->Unlock();
		buffer->Release();
		sample->Release();

		return true;
	}
}

#endif
