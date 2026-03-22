/*
 * src/Graphics/CubemapMovieResource.cpp
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

#include "CubemapMovieResource.hpp"

/* STL inclusions. */
#include <array>
#include <algorithm>
#include <cmath>
#include <numeric>
#include <ranges>

/* Third-Party inclusions. */
#include "json/json.h"

/* Local inclusions. */
#include "Libs/Algorithms/PerlinNoise.hpp"
#include "Libs/Algorithms/VoronoiNoise.hpp"
#include "Libs/ThreadPool.hpp"
#include "Libs/FastJSON.hpp"
#include "Resources/Manager.hpp"
#include "CubemapResource.hpp"

namespace EmEn::Graphics
{
	using namespace Libs;

	/* JSON keys. */
	constexpr auto JKBaseCubemapName{"BaseCubemapName"};
	constexpr auto JKFrameCount{"FrameCount"};
	constexpr auto JKFrameRate{"FrameRate"};
	constexpr auto JKFrameDuration{"FrameDuration"};
	constexpr auto JKIsLooping{"IsLooping"};
	constexpr auto JKAnimationDuration{"AnimationDuration"};
	constexpr auto JKFrames{"Frames"};
	constexpr auto JKDuration{"Duration"};
	constexpr auto JKCubemap{"Cubemap"};

	bool
	CubemapMovieResource::load () noexcept
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
				for ( size_t faceIndex = 0; faceIndex < CubemapFaceCount; faceIndex++ )
				{
					if ( !m_frames[frameIndex].first[faceIndex].initialize(size, size, PixelFactory::ChannelMode::RGBA) )
					{
						TraceError{ClassId} << "Unable to load the default pixmap for frame #" << frameIndex << ", face #" << faceIndex << " !";

						return this->setLoadSuccess(false);
					}

					if ( !m_frames[frameIndex].first[faceIndex].fill(colors.at(frameIndex)) )
					{
						TraceError{ClassId} << "Unable to fill the default pixmap for frame #" << frameIndex << ", face #" << faceIndex << " !";

						return this->setLoadSuccess(false);
					}
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
				for ( size_t faceIndex = 0; faceIndex < CubemapFaceCount; faceIndex++ )
				{
					if ( !m_frames[frameIndex].first[faceIndex].initialize(size, size, PixelFactory::ChannelMode::RGBA) ||
						 !m_frames[frameIndex].first[faceIndex].noise(true) )
					{
						return this->setLoadSuccess(false);
					}
				}

				m_frames[frameIndex].second = BaseTime / 10;
			}
		}

		return this->setLoadSuccess(true);
	}

	bool
	CubemapMovieResource::load (const std::filesystem::path & filepath) noexcept
	{
		/* Check for a JSON file. */
		if ( IO::getFileExtension(filepath) == "json" )
		{
			return ResourceTrait::load(filepath);
		}

		TraceDebug{ClassId} << "Reading cubemap movie file is not available yet !";

		if ( !this->beginLoading() )
		{
			return false;
		}

		return this->setLoadSuccess(false);
	}

	bool
	CubemapMovieResource::load (const Json::Value & data) noexcept
	{
		if ( !this->beginLoading() )
		{
			return false;
		}

		/* Checks the load in the parametric way. */
		if ( data.isMember(JKBaseCubemapName) )
		{
			if ( !this->loadParametric(data) )
			{
				TraceError{ClassId} << "Unable to load the cubemap movie with parametric key '" << JKBaseCubemapName << "' !";

				return this->setLoadSuccess(false);
			}
		}
		/* Checks the load in the manual way. */
		else if ( data.isMember(JKFrames) )
		{
			if ( !this->loadManual(data) )
			{
				TraceError{ClassId} << "Unable to load the cubemap movie with manual key '" << JKFrames << "' !";

				return this->setLoadSuccess(false);
			}
		}
		else
		{
			TraceError{ClassId} << "There is no '" << JKBaseCubemapName << "' or '" << JKFrames << "' key in cubemap movie definition !";

			return this->setLoadSuccess(false);
		}

		/* Checks if there is at least one frame registered. */
		if ( m_frames.empty() )
		{
			TraceError{ClassId} << "There is no valid frame for this cubemap movie !";

			return this->setLoadSuccess(false);
		}

		return this->setLoadSuccess(true);
	}

	uint32_t
	CubemapMovieResource::extractFrameDuration (const Json::Value & data, uint32_t frameCount) noexcept
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
	CubemapMovieResource::loadParametric (const Json::Value & data) noexcept
	{
		/* Checks the base name of cubemap animation files. */
		const auto basename = FastJSON::getValue< std::string >(data, JKBaseCubemapName);

		if ( !basename )
		{
			return false;
		}

		std::string replaceKey;

		const auto nWidth = CubemapMovieResource::extractCountWidth(basename.value(), replaceKey);

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
		const auto frameDuration = CubemapMovieResource::extractFrameDuration(data, frameCount);

		if ( frameDuration == 0 )
		{
			Tracer::error(ClassId, "Invalid frame duration !");

			return false;
		}

		m_looping = FastJSON::getValue< bool >(data, JKIsLooping).value_or(true);

		/* Gets all frames cubemaps. */
		auto * cubemapContainer = this->serviceProvider().container< CubemapResource >();

		for ( uint32_t frameIndex = 0; frameIndex < frameCount; frameIndex++ )
		{
			/* Sets the number as a string. */
			const auto cubemapName = String::replace(replaceKey, pad(std::to_string(frameIndex + 1), nWidth, '0', String::Side::Left), basename.value());

			/* NOTE: The cubemap must be loaded synchronously here. */
			const auto cubemapResource = cubemapContainer->getResource(cubemapName, false);

			if ( cubemapResource == nullptr )
			{
				TraceError{ClassId} << "Unable to get Cubemap '" << cubemapName << "' or the default one.";

				return false;
			}

			/* Save frame data by copying the 6 faces. */
			m_frames.emplace_back(cubemapResource->faces(), frameDuration);
		}

		return true;
	}

	bool
	CubemapMovieResource::loadManual (const Json::Value & data) noexcept
	{
		if ( !data.isMember(JKFrames) || !data[JKFrames].isArray() )
		{
			TraceError{ClassId} << "The '" << JKFrames << "' key is not present or not an array in cubemap movie definition !";

			return false;
		}

		auto * cubemaps = this->serviceProvider().container< CubemapResource >();

		for ( const auto & frame : data[JKFrames] )
		{
			if ( const auto cubemapResourceName = FastJSON::getValue< std::string >(frame, JKCubemap) )
			{
				/* NOTE: The cubemap must be loaded synchronously here. */
				const auto cubemapResource = cubemaps->getResource(cubemapResourceName.value(), false);
				const auto duration = FastJSON::getValue< uint32_t >(frame, JKDuration).value_or(DefaultFrameDuration);

				if ( !cubemapResource->isLoaded() )
				{
					return false;
				}

				m_frames.emplace_back(cubemapResource->faces(), duration);
			}
			else
			{
				TraceError{ClassId} <<
					"The '" << JKCubemap << "' key is not present or not a string in cubemap movie frame definition ! "
					"Skipping that frame...";
			}
		}

		return true;
	}

	bool
	CubemapMovieResource::onDependenciesLoaded () noexcept
	{
		this->updateDuration();

		return true;
	}

	void
	CubemapMovieResource::updateDuration () noexcept
	{
		m_duration = std::accumulate(m_frames.cbegin(), m_frames.cend(), 0U, [] (uint32_t duration, const auto & frame) {
			return duration + frame.second;
		});
	}

	const PixelFactory::Pixmap< uint8_t > &
	CubemapMovieResource::data (size_t frameIndex, size_t faceIndex) const noexcept
	{
		if ( frameIndex >= m_frames.size() )
		{
			Tracer::error(ClassId, "Frame index overflow !");

			frameIndex = 0;
		}

		if ( faceIndex >= CubemapFaceCount )
		{
			Tracer::error(ClassId, "Face index overflow !");

			faceIndex = 0;
		}

		return m_frames[frameIndex].first[faceIndex];
	}

	bool
	CubemapMovieResource::isGrayScale () const noexcept
	{
		return std::ranges::all_of(m_frames, [] (const auto & frame) {
			return std::ranges::all_of(frame.first, [] (const auto & pixmap) {
				if ( !pixmap.isValid() )
				{
					return false;
				}

				return pixmap.isGrayScale();
			});
		});
	}

	PixelFactory::Color< float >
	CubemapMovieResource::averageColor () const noexcept
	{
		if ( !this->isLoaded() )
		{
			return {};
		}

		const auto totalFaces = static_cast< float >(m_frames.size() * CubemapFaceCount);
		const auto ratio = 1.0F / totalFaces;

		auto red = 0.0F;
		auto green = 0.0F;
		auto blue = 0.0F;

		for ( const auto & faces : m_frames | std::views::keys )
		{
			for ( const auto & face : faces )
			{
				const auto average = face.averageColor();

				red += average.red() * ratio;
				green += average.green() * ratio;
				blue += average.blue() * ratio;
			}
		}

		return {red, green, blue, 1.0F};
	}

	uint32_t
	CubemapMovieResource::frameIndexAt (uint32_t timePoint) const noexcept
	{
		if ( m_duration > 0 )
		{
			const auto frameCount = static_cast< uint32_t >(m_frames.size());

			auto time = timePoint % m_duration;

			if ( !m_looping && timePoint >= m_duration )
			{
				return frameCount - 1;
			}

			for ( uint32_t frameIndex = 0; frameIndex < frameCount; frameIndex++ )
			{
				if ( m_frames[frameIndex].second >= time )
				{
					return frameIndex;
				}

				time -= m_frames[frameIndex].second;
			}
		}

		return 0;
	}

	uint32_t
	CubemapMovieResource::extractCountWidth (const std::string & basename, std::string & replaceKey) noexcept
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
	CubemapMovieResource::loadCaustics (uint32_t faceSize, uint32_t frameCount, uint32_t frameDuration, float scale, uint32_t seed, float baseIntensity, float causticIntensity, ThreadPool * threadPool) noexcept
	{
		if ( faceSize == 0 || frameCount == 0 || frameDuration == 0 )
		{
			TraceError{ClassId} << "Invalid parameters for caustics generation !";

			return false;
		}

		if ( !this->beginLoading() )
		{
			return false;
		}

		const Algorithms::VoronoiNoise< float > voronoi{seed};

		const auto invSize = 1.0F / static_cast< float >(faceSize);

		/* Animation drift amplitudes for circular looping path. */
		constexpr auto DriftX{0.35F};
		constexpr auto DriftY{0.25F};
		constexpr auto DriftZ{0.15F};
		constexpr auto TwoPi{6.283185307179586F};

		m_frames.resize(frameCount);

		/* Pre-initialize all pixmap sequentially (fast, catches allocation failures early). */
		for ( uint32_t frameIndex = 0; frameIndex < frameCount; frameIndex++ )
		{
			for ( size_t faceIndex = 0; faceIndex < CubemapFaceCount; faceIndex++ )
			{
				if ( !m_frames[frameIndex].first[faceIndex].initialize(faceSize, faceSize, PixelFactory::ChannelMode::RGBA) )
				{
					TraceError{ClassId} << "Unable to initialize pixmap for caustics frame #" << frameIndex << ", face #" << faceIndex << " !";

					return this->setLoadSuccess(false);
				}
			}

			m_frames[frameIndex].second = frameDuration;
		}

		/* Per-frame generation lambda. Each frame writes to its own pixmap,
		 * VoronoiNoise only reads internal tables → no shared mutable state. */
		auto generateFrame = [&] (uint32_t frameIndex) {
			/* Compute temporal offset using a circular path for seamless looping.
			 * Using sin/cos ensures frame 0 and the last frame are continuous. */
			const auto angle = TwoPi * static_cast< float >(frameIndex) / static_cast< float >(frameCount);
			const auto timeX = std::cos(angle) * DriftX;
			const auto timeY = std::sin(angle) * DriftY;
			const auto timeZ = std::cos(angle + 1.0F) * DriftZ;

			for ( size_t faceIndex = 0; faceIndex < CubemapFaceCount; faceIndex++ )
			{
				for ( uint32_t row = 0; row < faceSize; row++ )
				{
					const auto t = 2.0F * (static_cast< float >(row) + 0.5F) * invSize - 1.0F;

					for ( uint32_t col = 0; col < faceSize; col++ )
					{
						const auto s = 2.0F * (static_cast< float >(col) + 0.5F) * invSize - 1.0F;

						/* Compute the 3D direction vector for this texel based on face. */
						float dx, dy, dz;

						switch ( faceIndex )
						{
							case 0: /* PositiveX */ dx =  1.0F; dy = -t;    dz = -s;    break;
							case 1: /* NegativeX */ dx = -1.0F; dy = -t;    dz =  s;    break;
							case 2: /* PositiveY */ dx =  s;    dy =  1.0F; dz =  t;    break;
							case 3: /* NegativeY */ dx =  s;    dy = -1.0F; dz = -t;    break;
							case 4: /* PositiveZ */ dx =  s;    dy = -t;    dz =  1.0F; break;
							default: /* NegativeZ */ dx = -s;   dy = -t;    dz = -1.0F; break;
						}

						/* Normalize the direction. */
						const auto invLen = 1.0F / std::sqrt(dx * dx + dy * dy + dz * dz);
						const auto nx = dx * invLen;
						const auto ny = dy * invLen;
						const auto nz = dz * invLen;

						/* Evaluate Voronoi caustics with temporal offset.
						 * Invert: F2-F1 is small at cell edges (caustic lines),
						 * but caustic lines should be the bright part. */
						const auto causticValue = 1.0F - voronoi.caustic(
							nx * scale + timeX,
							ny * scale + timeY,
							nz * scale + timeZ
						);

						/* Map caustic value to intensity. */
						const auto intensity = baseIntensity + causticValue * (causticIntensity - baseIntensity);

						m_frames[frameIndex].first[faceIndex].setPixel(
							col, row,
							PixelFactory::Color< float >{intensity, intensity, intensity, 1.0F}
						);
					}
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

	bool
	CubemapMovieResource::loadRefractiveCaustics (uint32_t faceSize, uint32_t frameCount, uint32_t frameDuration, float scale, uint32_t seed, uint32_t octaves, float baseIntensity, float peakIntensity, ThreadPool * threadPool) noexcept
	{
		if ( faceSize == 0 || frameCount == 0 || frameDuration == 0 || octaves == 0 )
		{
			TraceError{ClassId} << "Invalid parameters for refractive caustics generation !";

			return false;
		}

		if ( !this->beginLoading() )
		{
			return false;
		}

		Algorithms::PerlinNoise< float > perlin{seed};

		const auto invSize = 1.0F / static_cast< float >(faceSize);

		/* Animation drift amplitudes for circular looping path. */
		constexpr auto DriftX{0.35F};
		constexpr auto DriftY{0.25F};
		constexpr auto DriftZ{0.15F};
		constexpr auto TwoPi{6.283185307179586F};

		m_frames.resize(frameCount);

		/* Pre-initialize all pixmaps. */
		for ( uint32_t frameIndex = 0; frameIndex < frameCount; frameIndex++ )
		{
			for ( size_t faceIndex = 0; faceIndex < CubemapFaceCount; faceIndex++ )
			{
				if ( !m_frames[frameIndex].first[faceIndex].initialize(faceSize, faceSize, PixelFactory::ChannelMode::RGBA) )
				{
					TraceError{ClassId} << "Unable to initialize pixmap for refractive caustics frame #" << frameIndex << ", face #" << faceIndex << " !";

					return this->setLoadSuccess(false);
				}
			}

			m_frames[frameIndex].second = frameDuration;
		}

		/* Per-frame generation lambda. */
		auto generateFrame = [&] (uint32_t frameIndex) {
			/* Circular time path for seamless looping. */
			const auto angle = TwoPi * static_cast< float >(frameIndex) / static_cast< float >(frameCount);
			const auto timeX = std::cos(angle) * DriftX;
			const auto timeY = std::sin(angle) * DriftY;
			const auto timeZ = std::cos(angle + 1.0F) * DriftZ;

			for ( size_t faceIndex = 0; faceIndex < CubemapFaceCount; faceIndex++ )
			{
				for ( uint32_t row = 0; row < faceSize; row++ )
				{
					const auto t = 2.0F * (static_cast< float >(row) + 0.5F) * invSize - 1.0F;

					for ( uint32_t col = 0; col < faceSize; col++ )
					{
						const auto s = 2.0F * (static_cast< float >(col) + 0.5F) * invSize - 1.0F;

						/* Compute the 3D direction vector for this texel based on face. */
						float dx, dy, dz;

						switch ( faceIndex )
						{
							case 0: dx =  1.0F; dy = -t;    dz = -s;    break;
							case 1: dx = -1.0F; dy = -t;    dz =  s;    break;
							case 2: dx =  s;    dy =  1.0F; dz =  t;    break;
							case 3: dx =  s;    dy = -1.0F; dz = -t;    break;
							case 4: dx =  s;    dy = -t;    dz =  1.0F; break;
							default: dx = -s;   dy = -t;    dz = -1.0F; break;
						}

						/* Normalize the direction. */
						const auto invLen = 1.0F / std::sqrt(dx * dx + dy * dy + dz * dz);
						auto nx = dx * invLen * scale + timeX;
						auto ny = dy * invLen * scale + timeY;
						auto nz = dz * invLen * scale + timeZ;

						/* Simulate refractive caustics through domain warping and multi-octave noise.
						 * Real underwater caustics are formed by light rays refracting through
						 * a wavy water surface, creating regions of concentrated brightness
						 * where rays converge (bright) and sparse regions where they diverge (dark).
						 *
						 * We simulate this by:
						 * 1. Domain warping: Perlin noise displaces the sample coordinates,
						 *    mimicking the deflection of light rays through a wavy surface.
						 * 2. Multi-octave evaluation: Several noise layers at different frequencies
						 *    are combined to produce both broad light patches and fine detail.
						 * 3. Power curve: Concentrates brightness into peaks, simulating the
						 *    non-linear focusing behavior of refractive caustics. */

						/* First pass: domain warping to simulate surface wave refraction. */
						const auto warpX = perlin.generate(nx + 5.2F, ny + 1.3F, nz + 3.7F);
						const auto warpY = perlin.generate(nx + 9.1F, ny + 4.6F, nz + 7.2F);
						const auto warpZ = perlin.generate(nx + 2.8F, ny + 8.4F, nz + 0.9F);

						constexpr auto WarpStrength{1.5F};
						nx += warpX * WarpStrength;
						ny += warpY * WarpStrength;
						nz += warpZ * WarpStrength;

						/* Multi-octave noise accumulation. */
						auto noiseSum = 0.0F;
						auto amplitude = 1.0F;
						auto frequency = 1.0F;
						auto maxAmplitude = 0.0F;

						for ( uint32_t octave = 0; octave < octaves; octave++ )
						{
							noiseSum += perlin.generate(nx * frequency, ny * frequency, nz * frequency) * amplitude;
							maxAmplitude += amplitude;
							amplitude *= 0.5F;
							frequency *= 2.0F;
						}

						/* Normalize to [0, 1]. */
						auto value = noiseSum / maxAmplitude;

						/* Apply power curve to concentrate brightness into peaks
						 * (simulates the non-linear focusing of refractive caustics).
						 * The square accentuates bright spots while darkening the rest. */
						value = value * value;

						/* Map to intensity range. */
						const auto intensity = baseIntensity + value * (peakIntensity - baseIntensity);

						m_frames[frameIndex].first[faceIndex].setPixel(
							col, row,
							PixelFactory::Color< float >{intensity, intensity, intensity, 1.0F}
						);
					}
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
