/*
 * src/Graphics/MovieResource.hpp
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

/* Third-Party inclusions. */
#include "json/json.h"

/* Local inclusions for inheritances. */
#include "Resources/ResourceTrait.hpp"

/* Local inclusions. */
#include "Libs/PixelFactory/Color.hpp"
#include "Libs/PixelFactory/Pixmap.hpp"
#include "Resources/Container.hpp"

namespace EmEn::Graphics
{
	/**
	 * @brief The movie resource class.
	 * @todo FIXME: There is a bug when unloading this resource at the engine shutdown.
	 * The main resources directory is "./data-stores/Movies/".
	 * @extends EmEn::Resources::ResourceTrait This is a loadable resource.
	 */
	class MovieResource final : public Resources::ResourceTrait
	{
		friend class Resources::Container< MovieResource >;

		public:

			/** @brief A frame from the movie with duration in milliseconds. */
			using Frame = std::pair< Libs::PixelFactory::Pixmap< uint8_t >, uint32_t >;

			/** @brief Class identifier. */
			static constexpr auto ClassId{"MovieResource"};

			/** @brief Defines the resource dependency complexity. */
			static constexpr auto Complexity{Resources::DepComplexity::None};

			/**
			 * @brief Constructs a movie resource.
			 * @param name A string for the resource name [std::move].
			 * @param resourceFlags The resource flag bits. Default none. (Unused yet)
			 */
			explicit
			MovieResource (std::string name, uint32_t resourceFlags = 0) noexcept
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

				for ( const auto & pixmap : m_frames | std::views::keys )
				{
					bytes += pixmap.bytes< size_t >();
				}

				return bytes;
			}

			/**
			 * @brief Returns the pixmap.
			 * @param frameIndex The frame number.
			 * @return const Libraries::PixelFactory::Pixmap< uint8_t > &
			 */
			[[nodiscard]]
			const Libs::PixelFactory::Pixmap< uint8_t > & data (size_t frameIndex) const noexcept;

			/**
			 * @brief Returns the frames from the movie.
			 * @return const std::vector< Frame > &
			 */
			[[nodiscard]]
			const std::vector< Frame > &
			frames () const noexcept
			{
				return m_frames;
			}

			/**
			 * @brief Returns the width of the movie.
			 * @note Returns the width of the first frame.
			 * @return uint32_t
			 */
			[[nodiscard]]
			uint32_t
			width () const noexcept
			{
				return m_frames.empty() ? 0 : static_cast< uint32_t >(m_frames[0].first.width());
			}

			/**
			 * @brief Returns the height of the movie.
			 * @note Returns the height of the first frame.
			 * @return size_t
			 */
			[[nodiscard]]
			uint32_t
			height () const noexcept
			{
				return m_frames.empty() ? 0 : static_cast< uint32_t >(m_frames[0].first.height());
			}

			/**
			 * @brief Returns whether frames are all gray scale.
			 * @return bool
			 */
			[[nodiscard]]
			bool isGrayScale () const noexcept;

			/**
			 * @brief Returns the average color of the movie.
			 * @return Libraries::PixelFactory::Color< float >
			 */
			[[nodiscard]]
			Libs::PixelFactory::Color< float > averageColor () const noexcept;

			/**
			 * @brief Returns the duration in milliseconds.
			 * @return size_t
			 */
			[[nodiscard]]
			uint32_t
			duration () const noexcept
			{
				return m_duration;
			}

			/**
			 * @brief Returns the number of frames.
			 * @return size_t
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
			 * @brief Loads a movie based on a numerical sequence of files.
			 * @param serviceProvider A reference to the resource manager through a service provider.
			 * @param data A reference to a JSON value.
			 * @return bool
			 */
			bool loadParametric (Resources::AbstractServiceProvider & serviceProvider, const Json::Value & data) noexcept;

			/**
			 * @brief Loads a manual version of a movie.
			 * @param serviceProvider A reference to the resource manager through a service provider.
			 * @param data A reference to a JSON value.
			 * @return bool
			 */
			bool loadManual (Resources::AbstractServiceProvider & serviceProvider, const Json::Value & data) noexcept;

			/**
			 * @brief Updates the full duration of the movie.
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
			 * @brief extractCountWidth
			 * @param basename
			 * @param replaceKey
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
	using Movies = Container< Graphics::MovieResource >;
}
