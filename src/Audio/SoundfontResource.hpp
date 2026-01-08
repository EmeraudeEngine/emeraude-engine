/*
 * src/Audio/SoundfontResource.hpp
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

#pragma once

/* STL inclusions. */
#include <cstdint>
#include <string>
#include <vector>

/* Local inclusions for inheritances. */
#include "Resources/ResourceTrait.hpp"

/* Local inclusions for usage. */
#include "Resources/Container.hpp"

/* Forward declaration for TinySoundFont. */
struct tsf;

namespace EmEn::Audio
{
	/**
	 * @brief Resource class for SoundFont 2 (SF2) files.
	 * @extends EmEn::Resources::ResourceTrait The base resource class.
	 * @note SoundFont files contain instrument samples for high-quality MIDI rendering.
	 * When no SF2 file is loaded (neutral resource), MIDI rendering falls back to additive synthesis.
	 */
	class SoundfontResource final : public Resources::ResourceTrait
	{
		friend class Resources::Container< SoundfontResource >;

		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"SoundfontResource"};

			/** @brief Defines the resource dependency complexity. */
			static constexpr auto Complexity{Resources::DepComplexity::None};

			/**
			 * @brief Constructs a soundfont resource.
			 * @param name The name of the resource.
			 * @param resourceFlags The resource flag bits. Default none. (Unused yet)
			 */
			explicit
			SoundfontResource (const std::string & name, uint32_t resourceFlags = 0) noexcept
				: ResourceTrait{name, resourceFlags}
			{

			}

			/**
			 * @brief Destructor. Releases the TinySoundFont handle.
			 */
			~SoundfontResource () override;

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
				return sizeof(*this) + m_fileData.size();
			}

			/**
			 * @brief Returns the TinySoundFont handle for rendering.
			 * @return tsf* Pointer to TSF handle, or nullptr if no soundfont loaded.
			 * @note The handle is owned by this resource and must not be freed by the caller.
			 */
			[[nodiscard]]
			tsf *
			handle () const noexcept
			{
				return m_tsf;
			}

			/**
			 * @brief Checks if a valid soundfont is loaded.
			 * @return bool True if a soundfont is loaded and ready for use.
			 */
			[[nodiscard]]
			bool
			isValid () const noexcept
			{
				return m_tsf != nullptr;
			}

			/**
			 * @brief Returns the number of presets in the soundfont.
			 * @return int Number of presets, or 0 if no soundfont loaded.
			 */
			[[nodiscard]]
			int presetCount () const noexcept;

			/**
			 * @brief Returns the name of a preset.
			 * @param presetIndex The index of the preset (0 to presetCount()-1).
			 * @return std::string The preset name, or empty string if invalid index.
			 */
			[[nodiscard]]
			std::string presetName (int presetIndex) const noexcept;

		private:

			/** @copydoc EmEn::Resources::ResourceTrait::onDependenciesLoaded() */
			[[nodiscard]]
			bool onDependenciesLoaded () noexcept override;

			tsf * m_tsf{nullptr};
			std::vector< char > m_fileData;
	};
}

/* Expose the resource manager as a convenient type. */
namespace EmEn::Resources
{
	using Soundfonts = Container< Audio::SoundfontResource >;
}
