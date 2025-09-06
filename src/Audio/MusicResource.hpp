/*
 * src/Audio/MusicResource.hpp
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

/* Local inclusions. */
#include "Resources/Container.hpp"

namespace EmEn::Audio
{
	/**
	 * @brief The music resource class.
	 * @extends EmEn::Audio::PlayableInterface
	 * @extends EmEn::Resources::ResourceTrait This is a loadable resource.
	 */
	class MusicResource final : public PlayableInterface, public Resources::ResourceTrait
	{
		friend class Resources::Container< MusicResource >;

		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"MusicResource"};

			/** @brief Defines the resource dependency complexity. */
			static constexpr auto Complexity{Resources::DepComplexity::None};

			/**
			 * @brief Constructs a music resource.
			 * @param name The name of the resource.
			 * @param resourceFlags The resource flag bits. Default none. (Unused yet)
			 */
			explicit
			MusicResource (const std::string & name, uint32_t resourceFlags = 0) noexcept
				: ResourceTrait{name, resourceFlags}
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
				static const size_t classUID = EmEn::Libs::Hash::FNV1a(ClassId);

				return classUID;
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

			/** @copydoc EmEn::Audio::PlayableInterface::streamable() */
			[[nodiscard]]
			size_t
			streamable () const noexcept override
			{
				return m_buffers.size();
			}

			/** @copydoc EmEn::Audio::PlayableInterface::buffer() */
			[[nodiscard]]
			std::shared_ptr< const Buffer >
			buffer (size_t bufferIndex = 0) const noexcept override
			{
				return m_buffers.at(bufferIndex);
			}

			/** @copydoc EmEn::Resources::ResourceTrait::classLabel() const */
			[[nodiscard]]
			const char *
			classLabel () const noexcept override
			{
				return ClassId;
			}

			/** @copydoc EmEn::Resources::ResourceTrait::load(Resources::ServiceProvider &) */
			bool load (Resources::ServiceProvider & serviceProvider) noexcept override;

			/** @copydoc EmEn::Resources::ResourceTrait::load(Resources::ServiceProvider &, const std::filesystem::path &) */
			bool load (Resources::ServiceProvider & serviceProvider, const std::filesystem::path & filepath) noexcept override;

			/** @copydoc EmEn::Resources::ResourceTrait::load(Resources::Manager &, const Json::Value &) */
			bool load (Resources::ServiceProvider & serviceProvider, const Json::Value & data) noexcept override;

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

			/**
			 * @brief Returns the title of the music.
			 * @return const std::string &
			 */
			[[nodiscard]]
			const std::string &
			title () const noexcept
			{
				return m_title;
			}

			/**
			 * @brief Returns the artist of the music.
			 * @return const std::string &
			 */
			[[nodiscard]]
			const std::string &
			artist () const noexcept
			{
				return m_artist;
			}

		private:

			/** @copydoc EmEn::Resources::ResourceTrait::onDependenciesLoaded() */
			[[nodiscard]]
			bool onDependenciesLoaded () noexcept override;

			/**
			 * @brief Reads the music file metadata.
			 * @param filepath A reference to a filesystem path.
			 */
			void readMetaData (const std::filesystem::path & filepath) noexcept;

			static constexpr auto DefaultInfo{"Unknown"};

			std::vector< std::shared_ptr< Buffer > > m_buffers;
			Libs::WaveFactory::Wave< int16_t > m_localData;
			std::string m_title{DefaultInfo};
			std::string m_artist{DefaultInfo};
	};
}

/* Expose the resource manager as a convenient type. */
namespace EmEn::Resources
{
	using Musics = Container< Audio::MusicResource >;
}
