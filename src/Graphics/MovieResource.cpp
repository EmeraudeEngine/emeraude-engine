/*
 * src/Graphics/MovieResource.cpp
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

#include "MovieResource.hpp"

/* STL inclusions. */
#include <array>
#include <algorithm>
#include <numeric>
#include <ranges>

/* Third-Party inclusions. */
#include "json/json.h"

/* Local inclusions. */
#include "Libs/Algorithms/PerlinNoise.hpp"
#include "Libs/ThreadPool.hpp"
#include "Libs/FastJSON.hpp"
#include "Resources/Manager.hpp"
#include "ImageResource.hpp"

namespace EmEn::Graphics
{
	using namespace Libs;

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
	MovieResource::load (Resources::AbstractServiceProvider & /*serviceProvider*/) noexcept
	{
		if ( !this->beginLoading() )
		{
			return false;
		}

		if constexpr ( IsDebug )
		{
			constexpr size_t frameCountDefault{3};
			constexpr size_t size{32};
			constexpr uint32_t debugFrameDuration{BaseTime / 3};

			constexpr std::array< PixelFactory::Color< float >, frameCountDefault > colors{
				PixelFactory::Red,
				PixelFactory::Green,
				PixelFactory::Blue
			};

			m_frames.resize(frameCountDefault);

			for ( size_t frameIndex = 0; frameIndex < frameCountDefault; frameIndex++ )
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

				m_frames[frameIndex].second = debugFrameDuration;
			}
		}
		else
		{
			constexpr size_t frameCountDefault{5};
			constexpr size_t size{32};

			m_frames.resize(frameCountDefault);

			for ( size_t frameIndex = 0; frameIndex < frameCountDefault; frameIndex++ )
			{
				if ( !m_frames[frameIndex].first.initialize(size, size, PixelFactory::ChannelMode::RGB) ||
					 !m_frames[frameIndex].first.noise(true) )
				{
					return this->setLoadSuccess(false);
				}

				m_frames[frameIndex].second = DefaultFrameDuration;
			}
		}

		return this->setLoadSuccess(true);
	}

	bool
	MovieResource::load (Resources::AbstractServiceProvider & serviceProvider, const std::filesystem::path & filepath) noexcept
	{
		/* Check for a JSON file. */
		if ( IO::getFileExtension(filepath) == "json" )
		{
			return ResourceTrait::load(serviceProvider, filepath);
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
	MovieResource::load (Resources::AbstractServiceProvider & serviceProvider, const Json::Value & data) noexcept
	{
		if ( !this->beginLoading() )
		{
			return false;
		}

		/* Checks the load in the parametric way. */
		if ( data.isMember(JKBaseFrameName) )
		{
			if ( !this->loadParametric(serviceProvider, data) )
			{
				TraceError{ClassId} << "Unable to load the animated texture with parametric key '" << JKBaseFrameName << "' !";

				return this->setLoadSuccess(false);
			}
		}
		/* Checks the load in the manual way. */
		else if ( data.isMember(JKFrames) )
		{
			if ( !this->loadManual(serviceProvider, data) )
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
		if ( const auto fps = FastJSON::getValue< uint32_t >(data, JKFrameRate).value_or(0); fps > 0 )
		{
			/* NOTE: Compute by using an FPS definition. */
			return BaseTime / fps;
		}

		if ( const auto animationDuration = FastJSON::getValue< uint32_t >(data, JKAnimationDuration).value_or(0); animationDuration > 0 )
		{
			/* NOTE: Using the duration of the whole movie. */
			return animationDuration / frameCount;
		}

		/* NOTE: Using a defined frame duration. */
		return FastJSON::getValue< uint32_t >(data, JKFrameDuration).value_or(DefaultFrameDuration);
	}

	bool
	MovieResource::loadParametric (Resources::AbstractServiceProvider & serviceProvider, const Json::Value & data) noexcept
	{
		/* Checks the base name of animation files. */
		const auto basename = FastJSON::getValue< std::string >(data, JKBaseFrameName);

		if ( !basename )
		{
			return false;
		}

		std::string replaceKey;

		const auto nWidth = MovieResource::extractCountWidth(basename.value(), replaceKey);

		if ( nWidth == 0 )
		{
			TraceError{ClassId} << "Invalid basename '" << basename.value() << "' !";

			return false;
		}

		/* Gets the frame count. */
		const auto frameCount = FastJSON::getValue< uint32_t >(data, JKFrameCount).value_or(0);

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

		m_looping = FastJSON::getValue< bool >(data, JKIsLooping).value_or(true);

		/* Gets all frames images. */
		auto * imageContainer = serviceProvider.container< ImageResource >();

		for ( uint32_t frameIndex = 0; frameIndex < frameCount; frameIndex++ )
		{
			/* Sets the number as a string. */
			const auto filename = String::replace(replaceKey, pad(std::to_string(frameIndex + 1), nWidth, '0', String::Side::Left), basename.value());

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
	MovieResource::loadManual (Resources::AbstractServiceProvider & serviceProvider, const Json::Value & data) noexcept
	{
		if ( !data.isMember(JKFrames) || !data[JKFrames].isArray() )
		{
			TraceError{ClassId} << "The '" << JKFrames << "' key  is not present or not an array in movie definition !";

			return false;
		}

		auto * images = serviceProvider.container< ImageResource >();

		for ( const auto & frame : data[JKFrames] )
		{
			if ( const auto imageResourceName = FastJSON::getValue< std::string >(frame, JKImage) )
			{
				/* NOTE: The image must be loaded synchronously here. */
				const auto imageResource = images->getResource(imageResourceName.value(), false);
				const auto duration = FastJSON::getValue< uint32_t >(frame, JKDuration).value_or(DefaultFrameDuration);

				if ( !imageResource->isLoaded() )
				{
					return false;
				}

				m_frames.emplace_back(imageResource->data(), duration);
			}
			else
			{
				TraceError{ClassId} <<
					"The '" << JKImage << "' key is not present or not a string in movie frame definition ! "
					"Skipping that frame...";
			}
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

	bool
	MovieResource::loadWaterNormals (uint32_t size, uint32_t frameCount, uint32_t frameDuration, float scale, uint32_t seed, uint32_t octaves, float persistence, float strength, ThreadPool * threadPool) noexcept
	{
		if ( size == 0 || frameCount == 0 || frameDuration == 0 || octaves == 0 )
		{
			TraceError{ClassId} << "Invalid parameters for water normals generation !";

			return false;
		}

		if ( !this->beginLoading() )
		{
			return false;
		}

		Algorithms::PerlinNoise< float > perlin{seed};

		const auto invSize = 1.0F / static_cast< float >(size);

		/* Animation drift amplitudes for circular looping path. */
		constexpr auto DriftX{0.5F};
		constexpr auto DriftY{0.35F};
		constexpr auto TwoPi{6.283185307179586F};

		m_frames.resize(frameCount);

		/* Pre-initialize all pixmap sequentially (fast, catches allocation failures early). */
		for ( uint32_t frameIndex = 0; frameIndex < frameCount; frameIndex++ )
		{
			if ( !m_frames[frameIndex].first.initialize(size, size, PixelFactory::ChannelMode::RGBA) )
			{
				TraceError{ClassId} << "Unable to initialize pixmap for water normals frame #" << frameIndex << " !";

				return this->setLoadSuccess(false);
			}

			m_frames[frameIndex].second = frameDuration;
		}

		/* Per-frame generation lambda. Each invocation allocates its own heights buffer
		 * so there is no shared mutable state between frames. */
		auto generateFrame = [&] (uint32_t frameIndex) {
			/* Compute temporal offset using a circular path for seamless looping. */
			const auto angle = TwoPi * static_cast< float >(frameIndex) / static_cast< float >(frameCount);
			const auto offsetX = std::cos(angle) * DriftX;
			const auto offsetY = std::sin(angle) * DriftY;

			/* Local height field buffer for two-pass generation. */
			std::vector< float > heights(static_cast< size_t >(size) * size);

			/* Pass 1: Generate the height field using FBM Perlin noise.
			 * 4-corner bilinear blending ensures seamless spatial tiling:
			 * sample noise at (u,v), (u-1,v), (u,v-1), (u-1,v-1) scaled by the
			 * octave period, then blend with smoothstep weights so that the
			 * left edge matches the right edge and top matches bottom. */
			for ( uint32_t row = 0; row < size; row++ )
			{
				const auto v = static_cast< float >(row) * invSize;
				const auto sv = v * v * (3.0F - 2.0F * v);

				for ( uint32_t col = 0; col < size; col++ )
				{
					const auto u = static_cast< float >(col) * invSize;
					const auto su = u * u * (3.0F - 2.0F * u);

					auto height = 0.0F;
					auto amplitude = 1.0F;
					auto frequency = 1.0F;

					for ( uint32_t octave = 0; octave < octaves; octave++ )
					{
						const auto s = scale * frequency;
						const auto z = static_cast< float >(octave) * 1.7F;

						const auto sx = u * s + offsetX;
						const auto sy = v * s + offsetY;

						const auto n00 = perlin.generate(sx,     sy,     z);
						const auto n10 = perlin.generate(sx - s, sy,     z);
						const auto n01 = perlin.generate(sx,     sy - s, z);
						const auto n11 = perlin.generate(sx - s, sy - s, z);

						const auto nx0 = n00 + su * (n10 - n00);
						const auto nx1 = n01 + su * (n11 - n01);

						height += amplitude * (nx0 + sv * (nx1 - nx0));

						amplitude *= persistence;
						frequency *= 2.0F;
					}

					heights[static_cast< size_t >(row) * size + col] = height;
				}
			}

			/* Pass 2: Compute normals from the height field using finite differences with wrapping. */
			for ( uint32_t row = 0; row < size; row++ )
			{
				const auto rowPrev = (row - 1 + size) % size;
				const auto rowNext = (row + 1) % size;

				for ( uint32_t col = 0; col < size; col++ )
				{
					const auto colPrev = (col - 1 + size) % size;
					const auto colNext = (col + 1) % size;

					const auto dx = heights[static_cast< size_t >(row) * size + colNext]
								  - heights[static_cast< size_t >(row) * size + colPrev];
					const auto dy = heights[static_cast< size_t >(rowNext) * size + col]
								  - heights[static_cast< size_t >(rowPrev) * size + col];

					/* Construct normal vector: (-dx * strength, -dy * strength, 1.0) and normalize. */
					const auto nx = -dx * strength;
					const auto ny = -dy * strength;
					constexpr auto nz = 1.0F;

					const auto invLen = 1.0F / std::sqrt(nx * nx + ny * ny + nz * nz);

					/* Encode to RGB: normal * 0.5 + 0.5 */
					m_frames[frameIndex].first.setPixel(
						col, row,
						PixelFactory::Color< float >{
							nx * invLen * 0.5F + 0.5F,
							ny * invLen * 0.5F + 0.5F,
							nz * invLen * 0.5F + 0.5F,
							1.0F
						}
					);
				}
			}
		};

		if ( threadPool != nullptr )
		{
			threadPool->parallelFor(uint32_t{0}, frameCount, generateFrame);
		}
		else
		{
			for ( uint32_t frameIndex = 0; frameIndex < frameCount; frameIndex++ )
			{
				generateFrame(frameIndex);
			}
		}

		this->updateDuration();

		return this->setLoadSuccess(true);
	}
}
