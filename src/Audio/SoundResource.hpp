/*
 * src/Audio/SoundResource.hpp
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
 * https://github.com/EmeraudeEngine/emeraude-engine
 *
 * --- THIS IS AUTOMATICALLY GENERATED, DO NOT CHANGE ---
 */

#pragma once

/* Project configuration. */
#include "emeraude_export.hpp"

/* STL inclusions. */
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>

/* Local inclusions for inheritances. */
#include "Resources/ResourceTrait.hpp"

/* Local inclusions for usages. */
#include "Buffer.hpp"
#include "Hash/FNV1a.hpp"
#include "Resources/Types.hpp"
#include "WaveFactory/Wave.hpp"

/* Forward declarations. */
namespace EmEn::Resources
{
	template< typename resource_t >
	class Container;
}

namespace EmEn::Audio
{
	class EMEN_API SoundResource final : public Resources::ResourceTrait
	{
		friend class Resources::Container< SoundResource >;

		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"SoundResource"};

			/** @brief Defines the resource dependency complexity. */
			static constexpr auto Complexity{Resources::DepComplexity::None};

			/**
			 * @brief Constructs a sound resource.
			 * @param serviceProvider A reference to the service provider.
			 * @param name The name of the resource [std::move].
			 * @param resourceFlags The resource flag bits. Default none. (Unused yet)
			 */
			SoundResource (Resources::AbstractServiceProvider & serviceProvider, const std::string & name, uint32_t resourceFlags = 0) noexcept
				: ResourceTrait{serviceProvider, name, resourceFlags}
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
				return Base::Hash::FNV1a(ClassId);
			}

			/** @copydoc EmEn::Base::ObservableTrait::classUID() const */
			[[nodiscard]]
			size_t
			classUID () const noexcept override
			{
				return getClassUID();
			}

			/** @copydoc EmEn::Base::ObservableTrait::is() const */
			[[nodiscard]]
			bool
			is (size_t classUID) const noexcept override
			{
				return classUID == getClassUID();
			}

			/**
			 * @brief Returns the OpenAL buffer of the sound.
			 * @return std::shared_ptr< const Buffer > A null pointer while the resource is not loaded.
			 */
			[[nodiscard]]
			std::shared_ptr< const Buffer >
			buffer () const noexcept
			{
				return m_buffer;
			}

			/**
			 * @brief Deletes the OpenAL buffer of this sound now (Buffer::destroy()); the sound stays, silent.
			 * @note Called by the audio manager's termination only, its device still open (owner decision 2026-10-16):
			 * the resource manager, which holds the sounds, terminates after it. The resource jobs are stopped by then
			 * and no source uses a buffer any more.
			 */
			void
			releaseBuffers () noexcept
			{
				if ( m_buffer != nullptr )
				{
					m_buffer->destroy();
				}
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
			const Base::WaveFactory::Wave< int16_t > &
			localData () const noexcept
			{
				return m_localData;
			}

			/**
			 * @brief Returns the local data.
			 * @return Libraries::WaveFactory::Wave< int16_t > &
			 */
			Base::WaveFactory::Wave< int16_t > &
			localData () noexcept
			{
				return m_localData;
			}

		private:

			/** @copydoc EmEn::Resources::ResourceTrait::onDependenciesLoaded() */
			[[nodiscard]]
			bool onDependenciesLoaded (const std::stop_token & stopToken) noexcept override;

			std::shared_ptr< Buffer > m_buffer;
			Base::WaveFactory::Wave< int16_t > m_localData;
	};
}

/* Expose the resource manager as a convenient type. */
namespace EmEn::Resources
{
	using Sounds = Container< Audio::SoundResource >;
}
