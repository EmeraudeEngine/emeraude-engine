/*
 * src/Graphics/MovieResource.cpp
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

#include "MovieResource.hpp"

/* STL inclusions. */
#include <array>
#include <algorithm>
#include <numeric>
#include <ranges>

/* Third-Party inclusions. */
#include "json/json.h"

/* Local inclusions. */
#include "Libs/FastJSON.hpp"
#include "Resources/Manager.hpp"
#include "ImageResource.hpp"

/* Defining the resource manager class id. */
template<>
const char * const EmEn::Resources::Container< EmEn::Graphics::MovieResource >::ClassId{"MovieContainer"};

/* Defining the resource manager ClassUID. */
template<>
const size_t EmEn::Resources::Container< EmEn::Graphics::MovieResource >::ClassUID{getClassUID(ClassId)};

namespace EmEn::Graphics
{
	using namespace EmEn::Libs;

	const size_t MovieResource::ClassUID{getClassUID(ClassId)};

	/* JSON key. */
	constexpr auto JKBaseFrameName{"BaseFrameName"};
	constexpr auto JKFrameCount{"FrameCount"};
	constexpr auto JKFrameRate{"FrameRate"};
	constexpr auto JKFrameDuration{"FrameDuration"};
	constexpr auto JKIsLooping{"IsLooping"};
	constexpr auto JKAnimationDuration{"AnimationDuration"};
	constexpr auto JKFrames{"Frames"};
	constexpr auto JKDuration{"Duration"};
	constexpr auto JKImage{"Image"};

	bool
	MovieResource::load () noexcept
	{
		if ( !this->beginLoading() )
		{
			return false;
		}

		constexpr size_t imageCount{3};
		constexpr size_t size{32};

		constexpr std::array< PixelFactory::Color< float >, imageCount > colors{
			PixelFactory::Red,
			PixelFactory::Green,
			PixelFactory::Blue
		};

		m_frames.resize(3);

		for ( size_t frameIndex = 0; frameIndex < imageCount; frameIndex++ )
		{
			if ( !m_frames[frameIndex].first.initialize(size, size, PixelFactory::ChannelMode::RGB) )
			{
				TraceError{ClassId} << "Unable to load the default pixmap for frame #" << frameIndex << " !";

				return this->setLoadSuccess(false);
			}

			if ( !m_frames[frameIndex].first.fill(colors.at(frameIndex)) )
			{
				TraceError{ClassId} << "Unable to fill the default pixmap for frame #" << frameIndex << " !";

				return this->setLoadSuccess(false);
			}

			m_frames[frameIndex].second = DefaultFrameDuration;
		}

		return this->setLoadSuccess(true);
	}

	bool
	MovieResource::load (const std::filesystem::path & filepath) noexcept
	{
		/* Check for a JSON file. */
		if ( IO::getFileExtension(filepath) == "json" )
		{
			return ResourceTrait::load(filepath);
		}

		/* Tries to read a movie (mp4, mpg, avi, ...) file. */
		TraceDebug{ClassId} << "Reading movie file is not available yet !";

		if ( !this->beginLoading() )
		{
			return false;
		}

		return this->setLoadSuccess(false);
	}

	bool
	MovieResource::load (const Json::Value & data) noexcept
	{
		if ( !this->beginLoading() )
		{
			return false;
		}

		/* Checks the load in the parametric way. */
		if ( data.isMember(JKBaseFrameName) )
		{
			if ( !this->loadParametric(data) )
			{
				TraceError{ClassId} << "Unable to load the animated texture with parametric key '" << JKBaseFrameName << "' !";

				return this->setLoadSuccess(false);
			}
		}
		/* Checks the load in the manual way. */
		else if ( data.isMember(JKFrames) )
		{
			if ( !this->loadManual(data) )
			{
				TraceError{ClassId} << "Unable to load the animated texture with manual key '" << JKFrames << "' !";

				return this->setLoadSuccess(false);
			}
		}
		else
		{
			TraceError{ClassId} << "There is no '" << JKBaseFrameName << "' or '" << JKFrames << "' key in animated texture definition !";

			return this->setLoadSuccess(false);
		}

		/* Checks if there is at least one frame registered. */
		if ( m_frames.empty() )
		{
			TraceError{ClassId} << "There is no valid frame for this movie !";

			return this->setLoadSuccess(false);
		}

		return this->setLoadSuccess(true);
	}

	uint32_t
	MovieResource::extractFrameDuration (const Json::Value & data, uint32_t frameCount) noexcept
	{
		const auto fps = FastJSON::getNumber< uint32_t >(data, JKFrameRate, 0);

		if ( fps > 0 )
		{
			/* NOTE: Compute by using an FPS definition. */
			return BaseTime / fps;
		}

		const auto animationDuration = FastJSON::getNumber< uint32_t >(data, JKAnimationDuration, 0);

		if ( animationDuration > 0 )
		{
			/* NOTE: Using the duration of the whole movie. */
			return animationDuration / frameCount;
		}

		/* NOTE: Using a defined frame duration. */
		return FastJSON::getNumber< uint32_t >(data, JKFrameDuration, DefaultFrameDuration);
	}

	bool
	MovieResource::loadParametric (const Json::Value & data) noexcept
	{
		/* Checks the base name of animation files. */
		const auto basename = FastJSON::getString(data, JKBaseFrameName);

		if ( basename.empty() )
		{
			return false;
		}

		std::string replaceKey{};

		const auto nWidth = MovieResource::extractCountWidth(basename, replaceKey);

		if ( nWidth == 0 )
		{
			TraceError{ClassId} << "Invalid basename '" << basename << "' !";

			return false;
		}

		/* Gets the frame count. */
		const auto frameCount = FastJSON::getNumber< uint32_t >(data, JKFrameCount, 0);

		if ( frameCount == 0 )
		{
			Tracer::error(ClassId, "Invalid number of frame !");

			return false;
		}

		/* Gets the frame duration. */
		const auto frameDuration = MovieResource::extractFrameDuration(data, frameCount);

		if ( frameDuration == 0 )
		{
			Tracer::error(ClassId, "Invalid frame duration !");

			return false;
		}

		m_looping = FastJSON::getBoolean(data, JKIsLooping, true);

		/* Gets all frames images. */
		auto * imageContainer = Resources::Manager::instance()->container< ImageResource >();

		for ( uint32_t frameIndex = 0; frameIndex < frameCount; frameIndex++ )
		{
			/* Sets the number as a string. */
			const auto filename = String::replace(replaceKey, pad(std::to_string(frameIndex + 1), nWidth, '0', String::Side::Left), basename);

			const auto imageResource = imageContainer->getResource(filename, false);

			if ( imageResource == nullptr )
			{
				TraceError{ClassId} << "Unable to get Image '" << filename << "' or the default one.";

				return false;
			}

			/* Save frame data. */
			m_frames.emplace_back(imageResource->data(), frameDuration);
		}

		return true;
	}

	bool
	MovieResource::loadManual (const Json::Value & data) noexcept
	{
		if ( !data.isMember(JKFrames) || !data[JKFrames].isArray() )
		{
			TraceError{ClassId} << "The '" << JKFrames << "' key  is not present or not an array in movie definition !";

			return false;
		}

		auto * images = Resources::Manager::instance()->container< ImageResource >();

		for ( const auto & frame : data[JKFrames] )
		{
			const auto imageName = FastJSON::getString(frame, JKImage);

			if ( imageName.empty() )
			{
				TraceError{ClassId} <<
					"The '" << JKImage << "' key is not present or not a string in movie frame definition ! "
					"Skipping that frame...";

				continue;
			}

			/* NOTE: The image must be loaded synchronously here. */
			const auto imageResource = images->getResource(imageName, false);
			const auto duration = FastJSON::getNumber< uint32_t >(frame, JKDuration, DefaultFrameDuration);

			if ( !imageResource->isLoaded() )
			{
				return false;
			}

			m_frames.emplace_back(imageResource->data(), duration);
		}

		return true;
	}

	bool
	MovieResource::onDependenciesLoaded () noexcept
	{
		this->updateDuration();

		return true;
	}

	void
	MovieResource::updateDuration () noexcept
	{
		m_duration = std::accumulate(m_frames.cbegin(), m_frames.cend(), 0U, [] (uint32_t duration, const auto & frame) {
			return duration + frame.second;
		});
	}

	const PixelFactory::Pixmap< uint8_t > &
	MovieResource::data (size_t frameIndex) const noexcept
	{
		if ( frameIndex >= m_frames.size() )
		{
			Tracer::error(ClassId, "Frame index overflow !");

			frameIndex = 0;
		}

		return m_frames[frameIndex].first;
	}

	bool
	MovieResource::isGrayScale () const noexcept
	{
		return std::ranges::all_of(m_frames, [] (const auto & frame) {
			if ( !frame.first.isValid() )
			{
				return false;
			}

			return frame.first.isGrayScale();
		});
	}

	PixelFactory::Color< float >
	MovieResource::averageColor () const noexcept
	{
		if ( !this->isLoaded() )
		{
			return {};
		}

		const auto ratio = 1.0F / static_cast< float >(m_frames.size());

		auto red = 0.0F;
		auto green = 0.0F;
		auto blue = 0.0F;

		for ( const auto & frame : std::ranges::views::keys(m_frames) )
		{
			const auto average = frame.averageColor();

			red += average.red() * ratio;
			green += average.green() * ratio;
			blue += average.blue() * ratio;
		}

		return {red, green, blue, 1.0F};
	}

	uint32_t
	MovieResource::frameIndexAt (uint32_t timePoint) const noexcept
	{
		if ( m_duration > 0 )
		{
			const auto frameCount = static_cast< uint32_t >(m_frames.size());

			auto time = timePoint % m_duration;

			if ( !m_looping && timePoint >= m_duration )
			{
				return frameCount - 1;
			}

			for ( uint32_t index = 0; index < frameCount; index++ )
			{
				if ( m_frames[index].second >= time )
				{
					return index;
				}

				time -= m_frames[index].second;
			}
		}

		return 0;
	}

	uint32_t
	MovieResource::extractCountWidth (const std::string & basename, std::string & replaceKey) noexcept
	{
		const auto params = String::extractTags(basename, {'{', '}'}, true);

		if ( params.empty() )
		{
			return 0;
		}

		replaceKey = '{' + params[0] + '}';

		return std::stoul(params[0]);
	}
}
