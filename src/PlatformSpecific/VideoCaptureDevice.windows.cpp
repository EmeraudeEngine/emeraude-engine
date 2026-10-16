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
 * https://github.com/EmeraudeEngine/emeraude-engine
 *
 * --- THIS IS AUTOMATICALLY GENERATED, DO NOT CHANGE ---
 */

#include "VideoCaptureDevice.hpp"

/* STL inclusions. */
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <utility>

/* Third-party inclusions. */
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>
#include <mfapi.h>
#include <mferror.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#include <wrl/client.h>

/* Local inclusions. */
#include "COM.windows.hpp"
#include "Helpers.hpp"
#include "Tracer.hpp"

namespace EmEn::PlatformSpecific
{
	constexpr auto TracerTag{"VideoCaptureDevice"};

	namespace
	{
		/**
		 * @brief COM, then Media Foundation, for the lifetime of this object; MFShutdown() then CoUninitialize() at its
		 * destruction, each one only when its start took a reference.
		 * @note COM already initialized with the other threading model on this thread (RPC_E_CHANGED_MODE) is acceptable:
		 * no reference taken. Any other COM failure skips MFStartup(). Thread-affine, like ComInitialization.
		 */
		class MediaFoundationScope final
		{
			public:

				/**
				 * @brief Initializes COM (single-threaded apartment) and starts Media Foundation.
				 */
				MediaFoundationScope () noexcept
					: m_comInitialization{COINIT_APARTMENTTHREADED}
				{
					const auto comResult = m_comInitialization.result();

					if ( FAILED(comResult) && comResult != RPC_E_CHANGED_MODE )
					{
						TraceError{TracerTag} << "CoInitializeEx failed (HRESULT: " << comResult << ").";

						return;
					}

					const auto startResult = MFStartup(MF_VERSION);

					if ( FAILED(startResult) )
					{
						TraceError{TracerTag} << "MFStartup failed (HRESULT: " << startResult << ").";

						return;
					}

					m_started = true;
				}

				/** @brief Deleted copy constructor. */
				MediaFoundationScope (const MediaFoundationScope & copy) noexcept = delete;

				/** @brief Deleted move constructor. */
				MediaFoundationScope (MediaFoundationScope && copy) noexcept = delete;

				/** @brief Deleted copy assignment. */
				MediaFoundationScope & operator= (const MediaFoundationScope & copy) noexcept = delete;

				/** @brief Deleted move assignment. */
				MediaFoundationScope & operator= (MediaFoundationScope && copy) noexcept = delete;

				/**
				 * @brief Shuts Media Foundation down when it was started; the COM member is destroyed after it.
				 */
				~MediaFoundationScope ()
				{
					if ( m_started )
					{
						MFShutdown();
					}
				}

				/**
				 * @brief Returns whether COM is usable and Media Foundation started.
				 * @return bool
				 */
				[[nodiscard]]
				bool
				started () const noexcept
				{
					return m_started;
				}

			private:

				ComInitialization m_comInitialization;
				bool m_started{false};
		};

		/**
		 * @brief The activation objects of the video capture devices (MFEnumDeviceSources()): every element released, then
		 * the array freed (CoTaskMemFree), by the destructor.
		 */
		class DeviceActivates final
		{
			public:

				/**
				 * @brief Enumerates the device sources.
				 * @param attributes The enumeration attributes (the source type).
				 */
				explicit
				DeviceActivates (IMFAttributes * attributes) noexcept
				{
					IMFActivate ** array = nullptr;
					UINT32 count = 0;

					m_result = MFEnumDeviceSources(attributes, &array, &count);

					/* NOTE: Owned at once (a failure leaves a null array, which the deleter accepts). */
					m_array.reset(array);

					if ( SUCCEEDED(m_result) && m_array != nullptr )
					{
						m_count = count;
					}
				}

				/** @brief Deleted copy constructor. */
				DeviceActivates (const DeviceActivates & copy) noexcept = delete;

				/** @brief Deleted move constructor. */
				DeviceActivates (DeviceActivates && copy) noexcept = delete;

				/** @brief Deleted copy assignment. */
				DeviceActivates & operator= (const DeviceActivates & copy) noexcept = delete;

				/** @brief Deleted move assignment. */
				DeviceActivates & operator= (DeviceActivates && copy) noexcept = delete;

				/**
				 * @brief Releases every activation object; the array member is freed after it.
				 */
				~DeviceActivates ()
				{
					for ( auto * activate : this->items() )
					{
						/* NOTE: Adopted without AddRef(): the ComPtr gives the enumeration's reference back. */
						Microsoft::WRL::ComPtr< IMFActivate > enumerated;
						enumerated.Attach(activate);
					}
				}

				/**
				 * @brief Returns the result of MFEnumDeviceSources().
				 * @return HRESULT
				 */
				[[nodiscard]]
				HRESULT
				result () const noexcept
				{
					return m_result;
				}

				/**
				 * @brief Returns the activation objects (none after a failed enumeration).
				 * @return std::span< IMFActivate * const >
				 */
				[[nodiscard]]
				std::span< IMFActivate * const >
				items () const noexcept
				{
					return {m_array.get(), m_count};
				}

			private:

				CoTaskMemPointer< IMFActivate * > m_array;
				UINT32 m_count{0};
				HRESULT m_result{E_FAIL};
		};

		/**
		 * @brief A media buffer locked for reading (IMFMediaBuffer::Lock()), unlocked by the destructor.
		 * @note The buffer must outlive this object: declare its owner first.
		 */
		class MediaBufferLock final
		{
			public:

				/**
				 * @brief Locks the buffer.
				 * @param buffer A reference to the buffer.
				 */
				explicit
				MediaBufferLock (IMFMediaBuffer & buffer) noexcept
					: m_buffer{&buffer}
				{
					m_result = buffer.Lock(&m_data, &m_maxLength, &m_currentLength);
				}

				/** @brief Deleted copy constructor. */
				MediaBufferLock (const MediaBufferLock & copy) noexcept = delete;

				/** @brief Deleted move constructor. */
				MediaBufferLock (MediaBufferLock && copy) noexcept = delete;

				/** @brief Deleted copy assignment. */
				MediaBufferLock & operator= (const MediaBufferLock & copy) noexcept = delete;

				/** @brief Deleted move assignment. */
				MediaBufferLock & operator= (MediaBufferLock && copy) noexcept = delete;

				/**
				 * @brief Unlocks the buffer when the lock succeeded.
				 */
				~MediaBufferLock ()
				{
					if ( SUCCEEDED(m_result) )
					{
						m_buffer->Unlock();
					}
				}

				/**
				 * @brief Returns the result of IMFMediaBuffer::Lock().
				 * @return HRESULT
				 */
				[[nodiscard]]
				HRESULT
				result () const noexcept
				{
					return m_result;
				}

				/**
				 * @brief Returns the locked bytes (null when the lock failed).
				 * @return const BYTE *
				 */
				[[nodiscard]]
				const BYTE *
				data () const noexcept
				{
					return m_data;
				}

				/**
				 * @brief Returns the number of valid bytes in the buffer.
				 * @return DWORD
				 */
				[[nodiscard]]
				DWORD
				currentLength () const noexcept
				{
					return m_currentLength;
				}

			private:

				IMFMediaBuffer * m_buffer;
				BYTE * m_data{nullptr};
				DWORD m_maxLength{0};
				DWORD m_currentLength{0};
				HRESULT m_result{E_FAIL};
		};

		/**
		 * @brief Returns a string attribute of an activation object in UTF-8 (IMFAttributes::GetAllocatedString()).
		 * @param activate A reference to the activation object.
		 * @param key The attribute key.
		 * @return std::optional< std::string >
		 */
		[[nodiscard]]
		std::optional< std::string >
		getAllocatedString (IMFActivate & activate, const GUID & key) noexcept
		{
			wchar_t * rawString = nullptr;
			UINT32 length = 0;

			const auto result = activate.GetAllocatedString(key, &rawString, &length);

			/* NOTE: Owned at once (a failure leaves a null pointer, which the deleter accepts). */
			const CoTaskMemPointer< wchar_t > string{rawString};

			if ( FAILED(result) || string == nullptr )
			{
				return std::nullopt;
			}

			return convertWideToUTF8(string.get());
		}
	}

	/* The platform-specific context (VideoCaptureDevice::m_platformContext). */
	struct VideoCaptureDevice::PlatformContext final
	{
		/** @brief Constructs the context: COM and Media Foundation started (check mediaFoundation.started()). */
		PlatformContext () noexcept = default;

		/** @brief Deleted copy constructor. */
		PlatformContext (const PlatformContext & copy) noexcept = delete;

		/** @brief Deleted move constructor. */
		PlatformContext (PlatformContext && copy) noexcept = delete;

		/** @brief Deleted copy assignment. */
		PlatformContext & operator= (const PlatformContext & copy) noexcept = delete;

		/** @brief Deleted move assignment. */
		PlatformContext & operator= (PlatformContext && copy) noexcept = delete;

		/**
		 * @brief Stops the capture in order: the reader released, the source shut down then released (member destruction),
		 * then MFShutdown() and CoUninitialize() (mediaFoundation, declared first, is destroyed last).
		 */
		~PlatformContext ()
		{
			sourceReader.Reset();

			if ( mediaSource != nullptr )
			{
				mediaSource->Shutdown();
			}
		}

		MediaFoundationScope mediaFoundation;
		Microsoft::WRL::ComPtr< IMFMediaSource > mediaSource;
		Microsoft::WRL::ComPtr< IMFSourceReader > sourceReader;
		bool isRGB32{false};
	};

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

	/* NOTE: Defined here, where PlatformContext is complete: an inline constructor instantiates the unique_ptr's
	 * deleter in every includer, where the type is incomplete (the macOS / Windows build broke on it). */
	VideoCaptureDevice::VideoCaptureDevice () noexcept = default;

	VideoCaptureDevice::~VideoCaptureDevice () noexcept
	{
		this->close();
	}

	std::vector< VideoCaptureDeviceInfo >
	VideoCaptureDevice::enumerateDevices () noexcept
	{
		std::vector< VideoCaptureDeviceInfo > devices;

		const MediaFoundationScope mediaFoundation;

		if ( !mediaFoundation.started() )
		{
			return devices;
		}

		/* Create attributes to request video capture devices. */
		Microsoft::WRL::ComPtr< IMFAttributes > attributes;
		HRESULT hr = MFCreateAttributes(attributes.GetAddressOf(), 1);

		if ( FAILED(hr) )
		{
			TraceError{TracerTag} << "MFCreateAttributes failed (HRESULT: " << hr << ").";

			return devices;
		}

		hr = attributes->SetGUID(MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE, MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_GUID);

		if ( FAILED(hr) )
		{
			TraceError{TracerTag} << "Failed to set device source type attribute (HRESULT: " << hr << ").";

			return devices;
		}

		/* Enumerate devices. */
		const DeviceActivates activates{attributes.Get()};

		if ( FAILED(activates.result()) )
		{
			TraceError{TracerTag} << "MFEnumDeviceSources failed (HRESULT: " << activates.result() << ").";

			return devices;
		}

		UINT32 index = 0;

		for ( auto * activate : activates.items() )
		{
			VideoCaptureDeviceInfo info;
			info.index = index;

			/* Get the friendly name. */
			if ( auto friendlyName = getAllocatedString(*activate, MF_DEVSOURCE_ATTRIBUTE_FRIENDLY_NAME); friendlyName.has_value() )
			{
				info.deviceName = std::move(friendlyName.value());
			}

			/* Get the symbolic link. */
			if ( auto symbolicLink = getAllocatedString(*activate, MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_SYMBOLIC_LINK); symbolicLink.has_value() )
			{
				info.devicePath = std::move(symbolicLink.value());
			}

			devices.emplace_back(std::move(info));

			++index;
		}

		if ( devices.empty() )
		{
			TraceInfo{TracerTag} << "No video capture devices found on Windows.";
		}
		else
		{
			TraceInfo{TracerTag} << "Found " << devices.size() << " video capture device(s) on Windows.";
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

		/* NOTE: Every early return below destroys the context, which stops what was started so far, in order. */
		std::unique_ptr< PlatformContext > context{new (std::nothrow) PlatformContext{}};

		if ( context == nullptr )
		{
			TraceError{TracerTag} << "Failed to allocate the capture context.";

			return false;
		}

		if ( !context->mediaFoundation.started() )
		{
			return false;
		}

		Microsoft::WRL::ComPtr< IMFActivate > matchedActivate;

		/* NOTE: The enumeration (its attributes, every activation object but the matched one) is released at the end of
		 * this block, before the media source is activated. */
		{
			/* Create attributes to enumerate video capture devices. */
			Microsoft::WRL::ComPtr< IMFAttributes > attributes;
			const HRESULT hr = MFCreateAttributes(attributes.GetAddressOf(), 1);

			if ( FAILED(hr) )
			{
				TraceError{TracerTag} << "MFCreateAttributes failed (HRESULT: " << hr << ").";

				return false;
			}

			if ( FAILED(attributes->SetGUID(MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE, MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_GUID)) )
			{
				return false;
			}

			/* Enumerate and find the matching device. */
			const DeviceActivates activates{attributes.Get()};

			if ( FAILED(activates.result()) )
			{
				TraceError{TracerTag} << "MFEnumDeviceSources failed (HRESULT: " << activates.result() << ").";

				return false;
			}

			/* Find the device matching devicePath. */
			for ( auto * activate : activates.items() )
			{
				if ( const auto link = getAllocatedString(*activate, MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_SYMBOLIC_LINK); link.has_value() && link.value() == devicePath )
				{
					/* NOTE: The ComPtr assignment takes its own reference (and gives back a previous match's: two devices
					 * with the same link leaked one AddRef()). */
					matchedActivate = activate;
				}
			}
		}

		if ( matchedActivate == nullptr )
		{
			TraceError{TracerTag} << "Failed to find video device matching '" << devicePath << "' on Windows.";

			return false;
		}

		/* Activate the media source. */
		HRESULT hr = matchedActivate->ActivateObject(IID_PPV_ARGS(context->mediaSource.GetAddressOf()));

		if ( FAILED(hr) )
		{
			TraceError{TracerTag} << "Failed to activate media source (HRESULT: " << hr << ").";

			return false;
		}

		/* Create source reader from media source. */
		Microsoft::WRL::ComPtr< IMFAttributes > readerAttributes;
		MFCreateAttributes(readerAttributes.GetAddressOf(), 1);

		hr = MFCreateSourceReaderFromMediaSource(context->mediaSource.Get(), readerAttributes.Get(), context->sourceReader.GetAddressOf());

		if ( FAILED(hr) )
		{
			TraceError{TracerTag} << "Failed to create source reader (HRESULT: " << hr << ").";

			return false;
		}

		/* Try to configure RGB32 output format first. */
		Microsoft::WRL::ComPtr< IMFMediaType > mediaType;
		hr = MFCreateMediaType(mediaType.GetAddressOf());

		if ( SUCCEEDED(hr) )
		{
			mediaType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
			mediaType->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_RGB32);
			MFSetAttributeSize(mediaType.Get(), MF_MT_FRAME_SIZE, requestedWidth, requestedHeight);

			hr = context->sourceReader->SetCurrentMediaType(
				static_cast< DWORD >(MF_SOURCE_READER_FIRST_VIDEO_STREAM), nullptr, mediaType.Get());

			if ( SUCCEEDED(hr) )
			{
				context->isRGB32 = true;
			}
			else
			{
				/* Fallback to YUY2. */
				mediaType->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_YUY2);

				hr = context->sourceReader->SetCurrentMediaType(
					static_cast< DWORD >(MF_SOURCE_READER_FIRST_VIDEO_STREAM), nullptr, mediaType.Get());

				if ( SUCCEEDED(hr) )
				{
					context->isRGB32 = false;
				}
			}
		}

		if ( FAILED(hr) )
		{
			TraceError{TracerTag} << "Failed to configure output media type (HRESULT: " << hr << ").";

			return false;
		}

		/* Read back the actual negotiated format to get real dimensions. */
		Microsoft::WRL::ComPtr< IMFMediaType > currentType;
		hr = context->sourceReader->GetCurrentMediaType(
			static_cast< DWORD >(MF_SOURCE_READER_FIRST_VIDEO_STREAM), currentType.GetAddressOf());

		if ( SUCCEEDED(hr) && currentType != nullptr )
		{
			UINT32 actualWidth = 0;
			UINT32 actualHeight = 0;

			/* NOTE: The size is taken only when the attribute is really there (its result was ignored). */
			if ( SUCCEEDED(MFGetAttributeSize(currentType.Get(), MF_MT_FRAME_SIZE, &actualWidth, &actualHeight)) )
			{
				m_width = actualWidth;
				m_height = actualHeight;
			}
			else
			{
				m_width = requestedWidth;
				m_height = requestedHeight;
			}
		}
		else
		{
			m_width = requestedWidth;
			m_height = requestedHeight;
		}

		m_platformContext = std::move(context);
		m_isOpen = true;

		TraceInfo{TracerTag} << "Video capture device '" << devicePath << "' opened successfully on Windows ("
			<< m_width << "x" << m_height << ", " << (m_platformContext->isRGB32 ? "RGB32" : "YUY2") << ").";

		return true;
	}

	void
	VideoCaptureDevice::close () noexcept
	{
		if ( !m_isOpen )
		{
			return;
		}

		/* NOTE: The context's destructor stops the capture: reader, source (Shutdown()), Media Foundation, COM. */
		m_platformContext.reset();
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
		if ( !m_isOpen || m_platformContext == nullptr )
		{
			return false;
		}

		auto * context = m_platformContext.get();

		/* Media Foundation may return S_OK with a null sample for the first few
		 * reads (stream tick / warmup). Retry a limited number of times. */
		constexpr int MaxReadAttempts = 10;
		DWORD streamIndex = 0;
		DWORD streamFlags = 0;
		LONGLONG timestamp = 0;
		Microsoft::WRL::ComPtr< IMFSample > sample;

		for ( int attempt = 0; attempt < MaxReadAttempts; ++attempt )
		{
			HRESULT hr = context->sourceReader->ReadSample(
				static_cast< DWORD >(MF_SOURCE_READER_FIRST_VIDEO_STREAM),
				0, &streamIndex, &streamFlags, &timestamp, sample.ReleaseAndGetAddressOf());

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

		/* NOTE: A device can switch its format while streaming (a capture card, a virtual camera): the frame size is
		 * read again, or the next frames were read with the old one (an over-read on the RGB32 path). */
		if ( (streamFlags & MF_SOURCE_READERF_CURRENTMEDIATYPECHANGED) != 0 )
		{
			Microsoft::WRL::ComPtr< IMFMediaType > changedType;

			if ( SUCCEEDED(context->sourceReader->GetCurrentMediaType(static_cast< DWORD >(MF_SOURCE_READER_FIRST_VIDEO_STREAM), changedType.GetAddressOf())) && changedType != nullptr )
			{
				UINT32 changedWidth = 0;
				UINT32 changedHeight = 0;

				if ( SUCCEEDED(MFGetAttributeSize(changedType.Get(), MF_MT_FRAME_SIZE, &changedWidth, &changedHeight)) )
				{
					m_width = changedWidth;
					m_height = changedHeight;
				}
			}
		}

		/* NOTE: Declared in this order: the lock is undone before the buffer is released, the buffer before the sample. */
		Microsoft::WRL::ComPtr< IMFMediaBuffer > buffer;
		const HRESULT hr = sample->GetBufferByIndex(0, buffer.GetAddressOf());

		if ( FAILED(hr) || buffer == nullptr )
		{
			TraceWarning{TracerTag} << "Failed to get buffer from sample (HRESULT: " << hr << ").";

			return false;
		}

		const MediaBufferLock lock{*buffer.Get()};

		if ( FAILED(lock.result()) )
		{
			TraceWarning{TracerTag} << "Failed to lock media buffer (HRESULT: " << lock.result() << ").";

			return false;
		}

		const BYTE * rawData = lock.data();
		const DWORD currentLength = lock.currentLength();

		if ( context->isRGB32 )
		{
			/* RGB32 is BGRA in memory. Convert to RGBA. */
			const size_t pixelCount = static_cast< size_t >(m_width) * m_height;

			/* NOTE: The conversion reads pixelCount * 4 bytes: a shorter buffer was read past its end. */
			if ( pixelCount == 0 || currentLength < pixelCount * 4 )
			{
				TraceWarning{TracerTag} << "Incomplete RGB32 frame (" << currentLength << " bytes for " << m_width << "x" << m_height << ") dropped.";

				return false;
			}

			convertBGRAtoRGBA(rawData, pixelCount, rgbaOutput);
		}
		else
		{
			/* YUY2 format. Use the shared YUYV->RGBA converter, which refuses a frame shorter than the format. */
			if ( !convertYUYVtoRGBA(rawData, currentLength, rgbaOutput, m_width, m_height) )
			{
				TraceWarning{TracerTag} << "Incomplete YUY2 frame (" << currentLength << " bytes for " << m_width << "x" << m_height << ") dropped.";

				return false;
			}
		}

		return true;
	}
}
