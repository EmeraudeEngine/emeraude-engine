/*
 * src/Resources/Stores.hpp
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
#include <map>
#include <string>
#include <vector>

/* Local inclusions for usages. */
#include "BaseInformation.hpp"

/* Forward declarations. */
namespace EmEn
{
	class FileSystem;
}

namespace EmEn::Resources
{
	/**
	 * @brief The resource stores contain by type all resources available on disk by reading an index file.
	 * This only gives the filepath to the actual resource.
	 */
	class Stores final
	{
		public:

			using Store = std::map< std::string, BaseInformation >;

			/** @brief Class identifier. */
			static constexpr auto ClassId{"ResourcesStoresService"};

			/**
			 * @brief Constructs a resource stores.
			 */
			Stores () noexcept = default;

			/**
			 * @brief Returns whether resource stores are empty.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			empty () const noexcept
			{
				return m_stores.empty();
			}

			/**
			 * @brief Returns a reference to a named store.
			 * @param storeName A reference to a string for the store name.
			 * @return const Store & store
			 */
			[[nodiscard]]
			const Store & store (const std::string & storeName) const noexcept;

			/**
			 * @brief Reads the resource index.
			 * @param fileSystem A reference to the fileSystem services.
			 * @param verbose Enable verbosity.
			 * @return bool
			 */
			bool initialize (const FileSystem & fileSystem, bool verbose) noexcept;

			/**
			 * @brief Updates resource store from another resource JSON definition.
			 * @param root The resource JSON object.
			 * @param verbose Enable verbosity. Default false.
			 * @return void
			 */
			void update (const Json::Value & root, bool verbose /*= false*/) noexcept;

			/**
			 * @brief Returns a random resource from a named store.
			 * @param storeName A reference to a string for the store name.
			 * @return std::string
			 */
			[[nodiscard]]
			std::string randomName (const std::string & storeName) const noexcept;

			/**
			 * @brief Returns whether the string buffer is JSON data or not.
			 * @param buffer A reference to a string.
			 * @return bool
			 */
			[[nodiscard]]
			static bool isJSONData (const std::string & buffer) noexcept;

		private:

			/**
			 * @brief Returns a list of resources index filepath.
			 * @param fileSystem A reference to the file system.
			 * @return std::vector< std::string >
			 */
			[[nodiscard]]
			static std::vector< std::string > getResourcesIndexFiles (const FileSystem & fileSystem) noexcept;

			/**
			 * @brief Parses a store JSON object to list available resources on disk.
			 * @param storesObject A reference to a JSON value.
			 * @param verbose Enable the reading verbosity in console.
			 * @return size_t
			 */
			[[nodiscard]]
			size_t parseStores (const Json::Value & storesObject, bool verbose) noexcept;

			/**
			* @brief STL streams printable object.
			* @param out A reference to the stream output.
			* @param obj A reference to the object to print.
			* @return std::ostream &
			*/
			friend std::ostream & operator<< (std::ostream & out, const Stores & obj);

			static constexpr auto StoresKey{"Stores"};

			std::map< std::string, Store > m_stores;
			Store m_defaultStore;
			size_t m_registeredResources{0};
	};

	inline
	std::ostream &
	operator<< (std::ostream & out, const Stores & obj)
	{
		if ( obj.m_stores.empty() )
		{
			return out << "There is no available resource store !" "\n";
		}

		out << "Resources stores :" "\n";

		for ( const auto & [name, store] : obj.m_stores )
		{
			out << " - " << name << " (" << store.size() << " resources)" << '\n';
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
	to_string (const Stores & obj) noexcept
	{
		std::stringstream output;

		output << obj;

		return output.str();
	}
}
