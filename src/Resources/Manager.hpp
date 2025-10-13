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
#include <unordered_map>
#include <typeindex>

/* Local inclusions for inheritances. */
#include "ServiceInterface.hpp"
#include "ServiceProvider.hpp"

/* Local inclusions for usages. */
#include "Container.hpp"

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
	 * @extends EmEn::Resources::ServiceProvider
	 */
	class Manager final : public ServiceInterface, public ServiceProvider
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"ResourcesManagerService"};

			/**
			 * @brief Constructs the resource manager.
			 * @param primaryServices A reference to primary services.
			 * @param graphicsRenderer A reference to the graphics renderer.
			 */
			explicit
			Manager (PrimaryServices & primaryServices, Graphics::Renderer & graphicsRenderer) noexcept
				: ServiceInterface{ClassId},
				ServiceProvider{primaryServices.fileSystem(), graphicsRenderer},
				m_primaryServices{primaryServices}
			{

			}

			/** @copydoc EmEn::Resources::ServiceProvider::update() */
			bool update (const Json::Value & root) noexcept override;

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

		private:

			/** @copydoc EmEn::ServiceInterface::onInitialize() */
			bool onInitialize () noexcept override;

			/** @copydoc EmEn::ServiceInterface::onTerminate() */
			bool onTerminate () noexcept override;

			/** @copydoc EmEn::Resources::ServiceProvider::getContainerInternal() */
			[[nodiscard]]
			ContainerInterface *
			getContainerInternal (const std::type_index & typeIndex) noexcept override
			{
				const auto containerIt = m_containers.find(typeIndex);

				if ( containerIt == m_containers.end() )
				{
					Tracer::fatal(ClassId, "Container does not exist !");

					return nullptr;
				}

				return containerIt->second.get();
			}

			/** @copydoc EmEn::Resources::ServiceProvider::getContainerInternal() const */
			[[nodiscard]]
			const ContainerInterface *
			getContainerInternal (const std::type_index & typeIndex) const noexcept override
			{
				const auto containerIt = m_containers.find(typeIndex);

				if ( containerIt == m_containers.end() )
				{
					Tracer::fatal(ClassId, "Container does not exist !");

					return nullptr;
				}

				return containerIt->second.get();
			}

			/**
			 * @brief Reads resource indexes to fill local container.
			 * @return bool
			 */
			[[nodiscard]]
			bool readResourceIndexes () noexcept;

			/**
			 * @brief Parses a store JSON object to list available resources on disk.
			 * @param fileSystem A reference to the file system services.
			 * @param storesObject A reference to a JSON value.
			 * @param verbose Enable the reading verbosity in the console.
			 * @return bool
			 */
			[[nodiscard]]
			bool parseStores (const FileSystem & fileSystem, const Json::Value & storesObject, bool verbose) noexcept;

			/**
			 * @brief Returns a local store by its name.
			 * @param storeName A reference to string.
			 * @return std::shared_ptr< std::unordered_map< std::string, BaseInformation > >
			 */
			std::shared_ptr< std::unordered_map< std::string, BaseInformation > >
			getLocalStore (const std::string & storeName) noexcept
			{
				const auto it = m_localStores.find(storeName);

				if ( it == m_localStores.cend() )
				{
					return nullptr;
				}

				return it->second;
			}

			/**
			 * @brief STL streams printable object.
			 * @param out A reference to the stream output.
			 * @param obj A reference to the object to print.
			 * @return std::ostream &
			 */
			friend std::ostream & operator<< (std::ostream & out, const Manager & obj);

			/**
			 * @brief Returns whether the string buffer is JSON data or not.
			 * @param buffer A reference to a string.
			 * @return bool
			 */
			[[nodiscard]]
			static bool isJSONData (const std::string & buffer) noexcept;

			/**
			 * @brief Returns a list of resources index filepath.
			 * @param fileSystem A reference to the file system.
			 * @return std::vector< std::string >
			 */
			[[nodiscard]]
			static std::vector< std::string > getResourcesIndexFiles (const FileSystem & fileSystem) noexcept;

			static constexpr auto StoresKey{"Stores"};

			PrimaryServices & m_primaryServices;
			std::unordered_map< std::string, std::shared_ptr< std::unordered_map< std::string, BaseInformation > > > m_localStores;
			std::map< std::type_index, std::unique_ptr< ContainerInterface > > m_containers;
			mutable std::mutex m_localStoresAccess;
			bool m_verbosityEnabled{false};
			bool m_downloadingAllowed{false};
			bool m_quietConversion{false};
	};

	inline
	std::ostream &
	operator<< (std::ostream & out, const Manager & obj)
	{
		out << "Resources stores :" "\n";

		for ( const auto & [name, store] : obj.m_localStores )
		{
			out << " - " << name << " (" << store->size() << " resources)" << '\n';
		}

		return out;
	}

	/**
	 * @brief Stringifies the object.
	 * @param obj A reference to the object to print.
	 * @return std::string
	 */
	inline
	std::string
	to_string (const Manager & obj) noexcept
	{
		std::stringstream output;

		output << obj;

		return output.str();
	}
}
