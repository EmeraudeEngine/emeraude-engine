/*
 * src/Graphics/ExternalInput.cpp
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

#include "ExternalInput.hpp"

/* Local inclusions. */
#include "PrimaryServices.hpp"
#include "SettingKeys.hpp"
#include "Tracer.hpp"

/* STL inclusions. */
#include <span>

namespace EmEn::Graphics
{
	using namespace Libs;
	using namespace Libs::PixelFactory;

	constexpr auto TracerTag{"ExternalInput"};

	bool
	ExternalInput::onInitialize () noexcept
	{
		auto & settings = m_primaryServices.settings();

		if ( !settings.getOrSetDefault< bool >(VideoCaptureEnableKey, DefaultVideoCaptureEnable) )
		{
			return false;
		}

		m_defaultDeviceIndex = settings.getOrSetDefault< int32_t >(VideoCaptureDeviceIndexKey, DefaultVideoCaptureDeviceIndex);
		m_captureWidth = settings.getOrSetDefault< uint32_t >(VideoCaptureDeviceWidthKey, DefaultVideoCaptureDeviceWidth);
		m_captureHeight = settings.getOrSetDefault< uint32_t >(VideoCaptureDeviceHeightKey, DefaultVideoCaptureDeviceHeight);

		TraceInfo{TracerTag} << "External input service initialized, device index: " << m_defaultDeviceIndex << ", resolution: " << m_captureWidth << "x" << m_captureHeight << ").";

		return true;
	}

	bool
	ExternalInput::onTerminate () noexcept
	{
		this->closeDevice();
		this->clearFrames();

		return true;
	}

	std::vector< PlatformSpecific::VideoCaptureDeviceInfo >
	ExternalInput::enumerateDevices () noexcept
	{
		return PlatformSpecific::VideoCaptureDevice::enumerateDevices();
	}

	bool
	ExternalInput::openDevice (const std::string & devicePath, uint32_t width, uint32_t height) noexcept
	{
		/* Use settings values when 0 is passed. */
		if ( width == 0 )
		{
			width = m_captureWidth;
		}

		if ( height == 0 )
		{
			height = m_captureHeight;
		}

		return m_captureDevice.open(devicePath, width, height);
	}

	void
	ExternalInput::closeDevice () noexcept
	{
		m_captureDevice.close();
	}

	bool
	ExternalInput::isDeviceOpen () const noexcept
	{
		return m_captureDevice.isOpen();
	}

	bool
	ExternalInput::captureFrame () noexcept
	{
		if ( !m_captureDevice.isOpen() )
		{
			TraceWarning{TracerTag} << "Cannot capture frame: no device is open.";

			return false;
		}

		if ( !m_captureDevice.captureFrame(m_rgbaBuffer) )
		{
			return false;
		}

		/* Create a Pixmap from the RGBA buffer. */
		m_capturedFrames.emplace_back(
			m_captureDevice.width(),
			m_captureDevice.height(),
			ChannelMode::RGBA,
			std::span< const uint8_t >(m_rgbaBuffer)
		);

		return true;
	}

	const std::vector< Pixmap< uint8_t > > &
	ExternalInput::capturedFrames () const noexcept
	{
		return m_capturedFrames;
	}

	const Pixmap< uint8_t > *
	ExternalInput::lastFrame () const noexcept
	{
		if ( m_capturedFrames.empty() )
		{
			return nullptr;
		}

		return &m_capturedFrames.back();
	}

	void
	ExternalInput::clearFrames () noexcept
	{
		m_capturedFrames.clear();
	}

	size_t
	ExternalInput::frameCount () const noexcept
	{
		return m_capturedFrames.size();
	}

	int32_t
	ExternalInput::defaultDeviceIndex () const noexcept
	{
		return m_defaultDeviceIndex;
	}

	uint32_t
	ExternalInput::captureWidth () const noexcept
	{
		return m_captureWidth;
	}

	uint32_t
	ExternalInput::captureHeight () const noexcept
	{
		return m_captureHeight;
	}
}
