/*
 * src/Resources/Manager.hpp
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
#include <cstddef>
#include <map>
#include <typeindex>

/* Local inclusions for inheritances. */
#include "ServiceInterface.hpp"

/* Local inclusions for usages. */
#include "Libs/ThreadPool.hpp"
#include "Container.hpp"
#include "Stores.hpp"

/* Forward declarations. */
namespace EmEn
{
	class PrimaryServices;
	class NetworkManager;
}

namespace EmEn::Resources
{
	/**
	 * @brief The resource manager service class.
	 * @extends EmEn::ServiceInterface The resource manager is a service.
	 */
	class Manager final : public ServiceInterface
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"ResourcesManagerService"};

			/**
			 * @brief Constructs the resource manager.
			 * @param primaryServices A reference to primary services.
			 */
			explicit Manager (PrimaryServices & primaryServices) noexcept;

			/** @copydoc EmEn::ServiceInterface::usable() */
			[[nodiscard]]
			bool
			usable () const noexcept override
			{
				return m_serviceInitialized;
			}

			/**
			 * @brief Gives access to the primary services.
			 * @return PrimaryServices &
			 */
			[[nodiscard]]
			PrimaryServices &
			primaryServices () const noexcept
			{
				return m_primaryServices;
			}

			/**
			 * @brief Sets the verbosity state for all resources.
			 * @param state The state.
			 * @return void
			 */
			void setVerbosity (bool state) noexcept;

			/**
			 * @brief Returns whether the verbosity is enabled.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			verbosityEnabled () const noexcept
			{
				return m_verbosityEnabled;
			}

			/**
			 * @brief Returns the total memory consumed by loaded resources in bytes from all containers.
			 * @return size_t
			 */
			[[nodiscard]]
			size_t memoryOccupied () const noexcept;

			/**
			 * @brief Returns the total memory consumed by loaded, but unused resources in bytes from all containers.
			 * @return size_t
			 */
			[[nodiscard]]
			size_t unusedMemoryOccupied () const noexcept;

			/**
			 * @brief Clean up every unused resource.
			 * @return size_t
			 */
			size_t unloadUnusedResources () noexcept;

			/**
			 * @brief Returns the reference to the resource store service.
			 * @return Stores &
			 */
			[[nodiscard]]
			Stores &
			stores () noexcept
			{
				return m_stores;
			}

			/**
			 * @brief Returns the reference to the resource store service.
			 * @return const Stores &
			 */
			[[nodiscard]]
			const Stores &
			stores () const noexcept
			{
				return m_stores;
			}

			/**
			 * @brief Returns a reference to the container for a specific resource type.
			 * @tparam resource_t The type of the resource (e.g., SoundResource).
			 * @return Container< resource_t > *
			 */
			template< typename resource_t >
			[[nodiscard]]
			Container< resource_t > *
			container ()
			{
				const auto it = m_containers.find(typeid(resource_t));

				if ( it == m_containers.end() )
				{
					Tracer::fatal(ClassId, "Container does not exist !");

					return nullptr;
				}

				return static_cast< Container< resource_t > * >(it->second.get());
			}

			/**
			 * @brief Returns a reference to the container for a specific resource type.
			 * @tparam resource_t The type of the resource (e.g., SoundResource).
			 * @return const Container< resource_t > *
			 */
			template< typename resource_t >
			[[nodiscard]]
			const Container< resource_t > *
			container () const
			{
				const auto it = m_containers.find(typeid(resource_t));

				if ( it == m_containers.end() )
				{
					Tracer::fatal(ClassId, "Container does not exist !");

					return nullptr;
				}

				return static_cast< const Container< resource_t > * >(it->second.get());
			}

		private:

			/** @copydoc EmEn::ServiceInterface::onInitialize() */
			bool onInitialize () noexcept override;

			/** @copydoc EmEn::ServiceInterface::onTerminate() */
			bool onTerminate () noexcept override;

			PrimaryServices & m_primaryServices;
			Stores m_stores;
			std::map< std::type_index, std::unique_ptr< ContainerInterface > > m_containers;
			bool m_serviceInitialized{false};
			bool m_verbosityEnabled{false};
			bool m_downloadingAllowed{false};
			bool m_quietConversion{false};

	};
}
