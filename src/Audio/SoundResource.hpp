/*
 * src/Audio/SoundResource.hpp
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

/* Local inclusions for inheritances. */
#include "PlayableInterface.hpp"
#include "Resources/ResourceTrait.hpp"

/* Local inclusions for usage. */
#include "Libs/WaveFactory/Wave.hpp"
#include "Resources/Container.hpp"
#include "Buffer.hpp"

namespace EmEn::Audio
{
	class SoundResource final : public PlayableInterface, public Resources::ResourceTrait
	{
		friend class Resources::Container< SoundResource >;

		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"SoundResource"};

			/** @brief Observable class unique identifier. */
			static const size_t ClassUID;

			/** @brief Defines the resource dependency complexity. */
			static constexpr auto Complexity{Resources::DepComplexity::None};

			/**
			 * @brief Constructs a sound resource.
			 * @param name The name of the resource.
			 * @param resourceFlags The resource flag bits. Default none. (Unused yet)
			 */
			explicit SoundResource (const std::string & name, uint32_t resourceFlags = 0) noexcept;

			/** @copydoc EmEn::Libs::ObservableTrait::classUID() const */
			[[nodiscard]]
			size_t
			classUID () const noexcept override
			{
				return ClassUID;
			}

			/** @copydoc EmEn::Libs::ObservableTrait::is() const */
			[[nodiscard]]
			bool
			is (size_t classUID) const noexcept override
			{
				return classUID == ClassUID;
			}

			/** @copydoc EmEn::Audio::PlayableInterface::streamable() */
			[[nodiscard]]
			size_t
			streamable () const noexcept override
			{
				return 0;
			}

			/** @copydoc EmEn::Audio::PlayableInterface::buffer() */
			[[nodiscard]]
			std::shared_ptr< const Buffer >
			buffer (size_t /*bufferIndex*/) const noexcept override
			{
				return m_buffer;
			}

			/** @copydoc EmEn::Resources::ResourceTrait::classLabel() const */
			[[nodiscard]]
			const char *
			classLabel () const noexcept override
			{
				return ClassId;
			}

			/** @copydoc EmEn::Resources::ResourceTrait::load() */
			bool load () noexcept override;

			/** @copydoc EmEn::Resources::ResourceTrait::load(const std::filesystem::path &) */
			bool load (const std::filesystem::path & filepath) noexcept override;

			/** @copydoc EmEn::Resources::ResourceTrait::load(const Json::Value &) */
			bool load (const Json::Value & data) noexcept override;

			/** @copydoc EmEn::Resources::ResourceTrait::memoryOccupied() const noexcept */
			[[nodiscard]]
			size_t
			memoryOccupied () const noexcept override
			{
				return sizeof(*this) + m_localData.bytes< size_t >();
			}

			/**
			 * @brief Returns the local data.
			 * @return const Libraries::WaveFactory::Wave< int16_t > &
			 */
			[[nodiscard]]
			const Libs::WaveFactory::Wave< int16_t > &
			localData () const noexcept
			{
				return m_localData;
			}

			/**
			 * @brief Returns the local data.
			 * @return Libraries::WaveFactory::Wave< int16_t > &
			 */
			Libs::WaveFactory::Wave< int16_t > &
			localData () noexcept
			{
				return m_localData;
			}

		private:

			/** @copydoc EmEn::Resources::ResourceTrait::onDependenciesLoaded() */
			[[nodiscard]]
			bool onDependenciesLoaded () noexcept override;

			std::shared_ptr< Buffer > m_buffer;
			Libs::WaveFactory::Wave< int16_t > m_localData;
	};
}

/* Expose the resource manager as a convenient type. */
namespace EmEn::Resources
{
	using Sounds = Container< Audio::SoundResource >;
}
