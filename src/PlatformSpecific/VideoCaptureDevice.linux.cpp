/*
 * src/PlatformSpecific/VideoCaptureDevice.linux.cpp
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

#if IS_LINUX

/* Local inclusions. */
#include "VideoCaptureDevice.hpp"
#include "Tracer.hpp"

/* STL inclusions. */
#include <cstring>

/* System inclusions. */
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/select.h>
#include <linux/videodev2.h>

namespace EmEn::PlatformSpecific
{
	constexpr auto TracerTag{"VideoCaptureDevice"};

	VideoCaptureDevice::~VideoCaptureDevice () noexcept
	{
		this->close();
	}

	std::vector< VideoCaptureDeviceInfo >
	VideoCaptureDevice::enumerateDevices () noexcept
	{
		std::vector< VideoCaptureDeviceInfo > devices;

		for ( uint32_t i = 0; i < 10; ++i )
		{
			const std::string path = "/dev/video" + std::to_string(i);

			const int fd = ::open(path.c_str(), O_RDWR | O_NONBLOCK);

			if ( fd < 0 )
			{
				continue;
			}

			struct v4l2_capability cap{};

			if ( ::ioctl(fd, VIDIOC_QUERYCAP, &cap) == 0 )
			{
				if ( (cap.capabilities & V4L2_CAP_VIDEO_CAPTURE) != 0 )
				{
					VideoCaptureDeviceInfo info;
					info.devicePath = path;
					info.deviceName = reinterpret_cast< const char * >(cap.card);
					info.index = i;

					devices.emplace_back(std::move(info));
				}
			}

			::close(fd);
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

		/* Open the device. */
		m_fd = ::open(devicePath.c_str(), O_RDWR);

		if ( m_fd < 0 )
		{
			TraceError{TracerTag} << "Failed to open video device '" << devicePath << "': " << strerror(errno);

			return false;
		}

		/* Query capabilities. */
		struct v4l2_capability cap{};

		if ( ::ioctl(m_fd, VIDIOC_QUERYCAP, &cap) < 0 )
		{
			TraceError{TracerTag} << "Failed to query capabilities for '" << devicePath << "'.";

			::close(m_fd);
			m_fd = -1;

			return false;
		}

		if ( (cap.capabilities & V4L2_CAP_VIDEO_CAPTURE) == 0 || (cap.capabilities & V4L2_CAP_STREAMING) == 0 )
		{
			TraceError{TracerTag} << "Device '" << devicePath << "' does not support video capture with streaming.";

			::close(m_fd);
			m_fd = -1;

			return false;
		}

		/* Set format: request YUYV at desired resolution. */
		struct v4l2_format fmt{};
		fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		fmt.fmt.pix.width = requestedWidth;
		fmt.fmt.pix.height = requestedHeight;
		fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
		fmt.fmt.pix.field = V4L2_FIELD_NONE;

		if ( ::ioctl(m_fd, VIDIOC_S_FMT, &fmt) < 0 )
		{
			TraceError{TracerTag} << "Failed to set video format for '" << devicePath << "'.";

			::close(m_fd);
			m_fd = -1;

			return false;
		}

		/* Store the actual negotiated resolution. */
		m_width = fmt.fmt.pix.width;
		m_height = fmt.fmt.pix.height;

		TraceInfo{TracerTag} << "Video format set to " << m_width << "x" << m_height << " YUYV on '" << devicePath << "'.";

		/* Request 1 mmap buffer. */
		struct v4l2_requestbuffers reqbuf{};
		reqbuf.count = 1;
		reqbuf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		reqbuf.memory = V4L2_MEMORY_MMAP;

		if ( ::ioctl(m_fd, VIDIOC_REQBUFS, &reqbuf) < 0 )
		{
			TraceError{TracerTag} << "Failed to request mmap buffers for '" << devicePath << "'.";

			::close(m_fd);
			m_fd = -1;

			return false;
		}

		/* Query the buffer to get its offset and length. */
		struct v4l2_buffer buf{};
		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory = V4L2_MEMORY_MMAP;
		buf.index = 0;

		if ( ::ioctl(m_fd, VIDIOC_QUERYBUF, &buf) < 0 )
		{
			TraceError{TracerTag} << "Failed to query mmap buffer for '" << devicePath << "'.";

			::close(m_fd);
			m_fd = -1;

			return false;
		}

		m_bufferLength = buf.length;
		m_buffer = ::mmap(nullptr, buf.length, PROT_READ | PROT_WRITE, MAP_SHARED, m_fd, buf.m.offset);

		if ( m_buffer == MAP_FAILED )
		{
			TraceError{TracerTag} << "Failed to mmap buffer for '" << devicePath << "'.";

			m_buffer = nullptr;
			::close(m_fd);
			m_fd = -1;

			return false;
		}

		/* Enqueue the buffer. */
		if ( ::ioctl(m_fd, VIDIOC_QBUF, &buf) < 0 )
		{
			TraceError{TracerTag} << "Failed to enqueue initial buffer for '" << devicePath << "'.";

			::munmap(m_buffer, m_bufferLength);
			m_buffer = nullptr;
			::close(m_fd);
			m_fd = -1;

			return false;
		}

		/* Start streaming. */
		auto type = static_cast< int >(V4L2_BUF_TYPE_VIDEO_CAPTURE);

		if ( ::ioctl(m_fd, VIDIOC_STREAMON, &type) < 0 )
		{
			TraceError{TracerTag} << "Failed to start streaming on '" << devicePath << "'.";

			::munmap(m_buffer, m_bufferLength);
			m_buffer = nullptr;
			::close(m_fd);
			m_fd = -1;

			return false;
		}

		m_isOpen = true;

		TraceInfo{TracerTag} << "Video capture device '" << devicePath << "' opened successfully (" << m_width << "x" << m_height << ").";

		return true;
	}

	void
	VideoCaptureDevice::close () noexcept
	{
		if ( !m_isOpen )
		{
			return;
		}

		/* Stop streaming. */
		auto type = static_cast< int >(V4L2_BUF_TYPE_VIDEO_CAPTURE);
		::ioctl(m_fd, VIDIOC_STREAMOFF, &type);

		/* Unmap buffer. */
		if ( m_buffer != nullptr )
		{
			::munmap(m_buffer, m_bufferLength);
			m_buffer = nullptr;
			m_bufferLength = 0;
		}

		/* Close the file descriptor. */
		if ( m_fd >= 0 )
		{
			::close(m_fd);
			m_fd = -1;
		}

		m_isOpen = false;
		m_width = 0;
		m_height = 0;

		TraceInfo{TracerTag} << "Video capture device closed.";
	}

	bool
	VideoCaptureDevice::isOpen () const noexcept
	{
		return m_isOpen;
	}

	bool
	VideoCaptureDevice::captureFrame (std::vector< uint8_t > & rgbaOutput) noexcept
	{
		if ( !m_isOpen )
		{
			return false;
		}

		/* Wait for the device to be ready with select() (timeout 5 seconds). */
		fd_set fds;
		FD_ZERO(&fds);
		FD_SET(m_fd, &fds);

		struct timeval tv;
		tv.tv_sec = 5;
		tv.tv_usec = 0;

		const int ret = ::select(m_fd + 1, &fds, nullptr, nullptr, &tv);

		if ( ret <= 0 )
		{
			TraceWarning{TracerTag} << "Frame capture timeout or error (select returned " << ret << ").";

			return false;
		}

		/* Dequeue the buffer. */
		struct v4l2_buffer buf{};
		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory = V4L2_MEMORY_MMAP;

		if ( ::ioctl(m_fd, VIDIOC_DQBUF, &buf) < 0 )
		{
			TraceError{TracerTag} << "Failed to dequeue buffer: " << strerror(errno);

			return false;
		}

		/* Convert YUYV to RGBA. */
		convertYUYVtoRGBA(static_cast< const uint8_t * >(m_buffer), buf.bytesused, rgbaOutput, m_width, m_height);

		/* Re-enqueue the buffer for the next frame. */
		if ( ::ioctl(m_fd, VIDIOC_QBUF, &buf) < 0 )
		{
			TraceError{TracerTag} << "Failed to re-enqueue buffer: " << strerror(errno);

			return false;
		}

		return true;
	}
}

#endif
