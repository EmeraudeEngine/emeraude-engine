/*
 * src/Audio/ExternalInput.cpp
 * This file is part of Emeraude-Engine
 *
 * Copyright (C) 2010-2025 - Sébastien Léon Claude Christian Bémelmans "LondNoir" <londnoir@gmail.com>
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

#include "AudioRecorder.hpp"

/* STL inclusions. */
#include <cstring>
#include <iostream>

/* Local inclusions. */
#include "Libs/WaveFactory/Wave.hpp"
#include "Manager.hpp"
#include "Libs/WaveFactory/FileIO.hpp"

namespace EmEn::Audio
{
	using namespace Libs;

	void
	AudioRecorder::start () noexcept
	{
		if ( m_device == nullptr || m_isRecording )
		{
			return;
		}

		m_samples.clear();

		alcCaptureStart(m_device);

		m_isRecording = true;

		m_process = std::thread(&AudioRecorder::recordingTask, this);
	}

	void
	AudioRecorder::stop () noexcept
	{
		if ( m_device == nullptr || !m_isRecording )
		{
			return;
		}

		alcCaptureStop(m_device);

		m_isRecording = false;
	}

	bool
	AudioRecorder::saveRecord (const std::filesystem::path & filepath) const noexcept
	{
		if ( m_isRecording )
		{
			Tracer::warning(ClassId, "The recorder is still running !");

			return false;
		}

		if ( m_samples.empty() )
		{
			Tracer::warning(ClassId, "There is no record to save !");

			return false;
		}

		WaveFactory::Wave< int16_t > wave;

		if ( !wave.initialize(m_samples, WaveFactory::Channels::Mono, m_frequency) )
		{
			Tracer::error(ClassId, "Unable to initialize wave data !");

			return false;
		}

		if ( !WaveFactory::FileIO::write(wave, filepath) )
		{
			Tracer::error(ClassId, "Unable to save the record to a file !");

			return false;
		}

		return true;
	}

	void
	AudioRecorder::recordingTask () noexcept
	{
		while ( m_isRecording )
		{
			ALCint sampleCount = 0;

			alcGetIntegerv(m_device, ALC_CAPTURE_SAMPLES, 1, &sampleCount);

			if ( sampleCount > 0 )
			{
				const auto offset = m_samples.size();

				m_samples.resize(offset + static_cast< size_t >(sampleCount));

				alcCaptureSamples(m_device, &m_samples[offset], sampleCount);
			}
		}
	}
}
