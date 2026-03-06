/*
 * src/Graphics/CubemapMovieResource.hpp
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

#pragma once

/* STL inclusions. */
#include <utility>
#include <vector>

/* Third-Party inclusions. */
#include "json/json.h"

/* Local inclusions for inheritances. */
#include "Resources/ResourceTrait.hpp"

/* Local inclusions. */
#include "Libs/PixelFactory/Color.hpp"
#include "Libs/PixelFactory/Pixmap.hpp"
#include "Resources/Container.hpp"
#include "Types.hpp"

namespace EmEn::Libs { class ThreadPool; }

namespace EmEn::Graphics
{
	/**
	 * @brief The cubemap movie resource class.
	 * Each frame consists of 6 cubemap face pixmaps with a duration in milliseconds.
	 * The main resources directory is "./data-stores/CubemapMovies/".
	 * @extends EmEn::Resources::ResourceTrait This is a loadable resource.
	 */
	class CubemapMovieResource final : public Resources::ResourceTrait
	{
		friend class Resources::Container< CubemapMovieResource >;

		public:

			/** @brief A frame from the cubemap movie with 6 face pixmaps and duration in milliseconds. */
			using Frame = std::pair< CubemapPixmaps, uint32_t >;

			/** @brief Class identifier. */
			static constexpr auto ClassId{"CubemapMovieResource"};

			/** @brief Defines the resource dependency complexity. */
			static constexpr auto Complexity{Resources::DepComplexity::None};

			/**
			 * @brief Constructs a cubemap movie resource.
			 * @param name A string for the resource name [std::move].
			 * @param resourceFlags The resource flag bits. Default none. (Unused yet)
			 */
			explicit
			CubemapMovieResource (std::string name, uint32_t resourceFlags = 0) noexcept
				: ResourceTrait{std::move(name), resourceFlags}
			{

			}

			/**
			 * @brief Returns the unique identifier for this class [Thread-safe].
			 * @return size_t
			 */
			static
			size_t
			getClassUID () noexcept
			{
				return Libs::Hash::FNV1a(ClassId);
			}

			/** @copydoc EmEn::Libs::ObservableTrait::classUID() const */
			[[nodiscard]]
			size_t
			classUID () const noexcept override
			{
				return getClassUID();
			}

			/** @copydoc EmEn::Libs::ObservableTrait::is() const */
			[[nodiscard]]
			bool
			is (size_t classUID) const noexcept override
			{
				return classUID == getClassUID();
			}

			/** @copydoc EmEn::Resources::ResourceTrait::classLabel() const */
			[[nodiscard]]
			const char *
			classLabel () const noexcept override
			{
				return ClassId;
			}

			/** @copydoc EmEn::Resources::ResourceTrait::load(Resources::ServiceProvider &) */
			bool load (Resources::AbstractServiceProvider & serviceProvider) noexcept override;

			/** @copydoc EmEn::Resources::ResourceTrait::load(Resources::ServiceProvider &, const std::filesystem::path &) */
			bool load (Resources::AbstractServiceProvider & serviceProvider, const std::filesystem::path & filepath) noexcept override;

			/** @copydoc EmEn::Resources::ResourceTrait::load(Resources::Manager &, const Json::Value &) */
			bool load (Resources::AbstractServiceProvider & serviceProvider, const Json::Value & data) noexcept override;

			/** @copydoc EmEn::Resources::ResourceTrait::memoryOccupied() const noexcept */
			[[nodiscard]]
			size_t
			memoryOccupied () const noexcept override
			{
				size_t bytes = sizeof(*this);

				for ( const auto & [faces, duration] : m_frames )
				{
					for ( const auto & pixmap : faces )
					{
						bytes += pixmap.bytes< size_t >();
					}
				}

				return bytes;
			}

			/**
			 * @brief Returns the pixmap for a specific frame and face.
			 * @param frameIndex The frame number.
			 * @param faceIndex The face number (0-5).
			 * @return const Libraries::PixelFactory::Pixmap< uint8_t > &
			 */
			[[nodiscard]]
			const Libs::PixelFactory::Pixmap< uint8_t > & data (size_t frameIndex, size_t faceIndex) const noexcept;

			/**
			 * @brief Returns the frames from the cubemap movie.
			 * @return const std::vector< Frame > &
			 */
			[[nodiscard]]
			const std::vector< Frame > &
			frames () const noexcept
			{
				return m_frames;
			}

			/**
			 * @brief Returns the cube size (width/height of one face).
			 * @note Returns the width of the first face of the first frame.
			 * @return uint32_t
			 */
			[[nodiscard]]
			uint32_t
			cubeSize () const noexcept
			{
				return m_frames.empty() ? 0 : static_cast< uint32_t >(m_frames[0].first[0].width());
			}

			/**
			 * @brief Returns whether frames are all gray scale.
			 * @return bool
			 */
			[[nodiscard]]
			bool isGrayScale () const noexcept;

			/**
			 * @brief Generates procedural Voronoi caustic animation frames for cubemap projection.
			 * @param faceSize The resolution of each cubemap face in pixels (e.g. 256).
			 * @param frameCount The number of animation frames to generate.
			 * @param frameDuration The duration of each frame in milliseconds.
			 * @param scale The Voronoi cell density. Higher values produce finer patterns. Default 4.0.
			 * @param seed The random seed for the Voronoi noise. Default 0.
			 * @param baseIntensity The background luminosity in [0,1]. Default 0.7.
			 * @param causticIntensity The caustic line luminosity in [0,1]. Default 1.0.
			 * @param threadPool A pointer to a thread pool. Default sequential loading.
			 * @return bool
			 */
			bool loadCaustics (uint32_t faceSize, uint32_t frameCount, uint32_t frameDuration, float scale = 4.0F, uint32_t seed = 0, float baseIntensity = 0.7F, float causticIntensity = 1.0F, Libs::ThreadPool * threadPool = nullptr) noexcept;

			/**
			 * @brief Generates procedural refractive caustic animation frames for cubemap projection.
			 * @note Uses multi-octave Perlin noise with sinusoidal domain warping to simulate
			 *       light concentration through a wavy water surface. Produces soft, organic
			 *       patterns unlike the sharp cell-edge patterns of Voronoi caustics.
			 * @param faceSize The resolution of each cubemap face in pixels (e.g. 256).
			 * @param frameCount The number of animation frames to generate.
			 * @param frameDuration The duration of each frame in milliseconds.
			 * @param scale The pattern density. Higher values produce finer caustics. Default 3.0.
			 * @param seed The random seed for the Perlin noise. Default 0.
			 * @param octaves The number of noise octaves for detail richness. Default 3.
			 * @param baseIntensity The minimum luminosity in dark areas [0,1]. Default 0.15.
			 * @param peakIntensity The maximum luminosity at caustic concentrations. Can exceed 1.0 for HDR. Default 1.4.
			 * @param threadPool A pointer to a thread pool. Default sequential loading.
			 * @return bool
			 */
			bool loadRefractiveCaustics (uint32_t faceSize, uint32_t frameCount, uint32_t frameDuration, float scale = 3.0F, uint32_t seed = 0, uint32_t octaves = 3, float baseIntensity = 0.15F, float peakIntensity = 1.4F, Libs::ThreadPool * threadPool = nullptr) noexcept;

			/**
			 * @brief Returns the average color of the cubemap movie.
			 * @return Libraries::PixelFactory::Color< float >
			 */
			[[nodiscard]]
			Libs::PixelFactory::Color< float > averageColor () const noexcept;

			/**
			 * @brief Returns the duration in milliseconds.
			 * @return uint32_t
			 */
			[[nodiscard]]
			uint32_t
			duration () const noexcept
			{
				return m_duration;
			}

			/**
			 * @brief Returns the number of frames.
			 * @return uint32_t
			 */
			[[nodiscard]]
			uint32_t
			frameCount () const noexcept
			{
				return static_cast< uint32_t >(m_frames.size());
			}

			/**
			 * @brief Returns the index of the frame at a specific time.
			 * @param timePoint A time point in milliseconds.
			 * @return uint32_t
			 */
			[[nodiscard]]
			uint32_t frameIndexAt (uint32_t timePoint) const noexcept;

			/**
			 * @brief Sets whether the animation is looping.
			 * @param state The state.
			 * @return void
			 */
			void
			setLoopState (bool state) noexcept
			{
				m_looping = state;
			}

			/**
			 * @brief Returns whether the animation is looping.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isLooping () const noexcept
			{
				return m_looping;
			}

		private:

			/** @copydoc EmEn::Resources::ResourceTrait::onDependenciesLoaded() */
			[[nodiscard]]
			bool onDependenciesLoaded () noexcept override;

			/**
			 * @brief Loads a cubemap movie based on a numerical sequence of cubemap resources.
			 * @param serviceProvider A reference to the resource manager through a service provider.
			 * @param data A reference to a JSON value.
			 * @return bool
			 */
			bool loadParametric (Resources::AbstractServiceProvider & serviceProvider, const Json::Value & data) noexcept;

			/**
			 * @brief Loads a manual version of a cubemap movie.
			 * @param serviceProvider A reference to the resource manager through a service provider.
			 * @param data A reference to a JSON value.
			 * @return bool
			 */
			bool loadManual (Resources::AbstractServiceProvider & serviceProvider, const Json::Value & data) noexcept;

			/**
			 * @brief Updates the full duration of the cubemap movie.
			 * @return void
			 */
			void updateDuration () noexcept;

			/**
			 * @brief Returns the frame duration from the JSON resource description.
			 * @param data A reference to a JSON node.
			 * @param frameCount The animation frame count for calculation.
			 * @return uint32_t
			 */
			[[nodiscard]]
			static uint32_t extractFrameDuration (const Json::Value & data, uint32_t frameCount) noexcept;

			/**
			 * @brief Extracts the count width from a basename pattern.
			 * @param basename The base name pattern.
			 * @param replaceKey The replaceable key output.
			 * @return uint32_t
			 */
			static uint32_t extractCountWidth (const std::string & basename, std::string & replaceKey) noexcept;

			static constexpr uint32_t BaseTime{1000};
			static constexpr uint32_t DefaultFrameDuration{BaseTime / 30};

			std::vector< Frame > m_frames;
			uint32_t m_duration{0};
			bool m_looping{true};
	};
}

/* Expose the resource manager as a convenient type. */
namespace EmEn::Resources
{
	using CubemapMovies = Container< Graphics::CubemapMovieResource >;
}
