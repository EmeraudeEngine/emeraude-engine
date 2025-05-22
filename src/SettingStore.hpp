/*
 * src/SettingStore.hpp
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
#include <vector>
#include <string>
#include <any>

namespace EmEn
{
	/**
	 * @brief The SettingStore class
	 */
	class SettingStore final
	{
		friend class Settings;

		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"SettingStore"};

			/**
			 * @brief Constructs a default setting store.
			 */
			SettingStore () noexcept = default;

			/**
			 * @brief Sets an "any" variable in store.
			 * @param name A reference to a string for the variable name.
			 * @param value A reference to a std::any.
			 * @return void
			 */
			void
			setVariable (const std::string & name, const std::any & value) noexcept
			{
				m_variables[name] = value;
			}

			/**
			 * @brief Sets an "any" variable in an array store.
			 * @param name A reference to a string for the variable name.
			 * @param value A reference to a std::any.
			 * @return void
			 */
			void setVariableInArray (const std::string & name, const std::any & value) noexcept;

			/**
			 * @brief Returns whether the store is empty.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			empty () const noexcept
			{
				if ( !m_variables.empty() )
				{
					return false;
				}

				if ( !m_arrays.empty() )
				{
					return false;
				}

				if ( !m_subStores.empty() )
				{
					return false;
				}

				return true;
			}

			/**
			 * @brief Checks whether a variable is present in the store.
			 * @param name A reference to a string for the variable name.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			variableExists (const std::string & name) const noexcept
			{
				return m_variables.contains(name);
			}

			/**
			 * @brief Checks whether an array is present in the store.
			 * @param name A reference to a string for the array name.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			arrayExists (const std::string & name) const noexcept
			{
				return m_arrays.contains(name);
			}

			/**
			 * @brief Checks if a sub-store is present in the store.
			 * @param storeName The name of the store.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			subStoreExists (const std::string & storeName) const noexcept
			{
				return m_subStores.contains(storeName);
			}

			/**
			 * @brief Gets or creates a sub-store.
			 * @param storeName A reference to a string for the store name.
			 * @return SettingStore &
			 */
			[[nodiscard]]
			SettingStore &
			getOrCreateSubStore (const std::string & storeName) noexcept
			{
				return m_subStores[storeName];
			}

			/**
			 * @brief Returns a sub-store pointer.
			 * @param storeName The name of the store.
			 * @return const SettingStore *
			 */
			[[nodiscard]]
			const SettingStore * getSubStorePointer (const std::string & storeName) const noexcept;

			/**
			 * @brief Returns a sub-store pointer.
			 * @param storeName The name of the store.
			 * @param createIfNotExists Enabled, this will create the sub-store if it doesn't exist.
			 * @return SettingStore *
			 */
			[[nodiscard]]
			SettingStore * getSubStorePointer (const std::string & storeName, bool createIfNotExists = true) noexcept;

			/**
			 * @brief Gets the value of a variable.
			 * @param name A reference to a string for the variable name.
			 * @return const std::any *
			 */
			[[nodiscard]]
			const std::any * getValuePointer (const std::string & name) const noexcept;

			/**
			 * @brief Gets the value of a variable. If the variable doesn't exist, it will be set.
			 * @param name A reference to a string for the variable name.
			 * @param defaultValue A reference to a string for the default value.
			 * @return const std::any *
			 */
			[[nodiscard]]
			const std::any * getValuePointerOrSet (const std::string & name, const std::string & defaultValue) noexcept;

			/**
			 * @brief Returns a const access to an array of variable.
			 * @param name A reference to a string for the array name.
			 * @return const std::vector< std::any > *
			 */
			[[nodiscard]]
			const std::vector< std::any > * getArrayPointer (const std::string & name) const noexcept;

			/**
			 * @brief Returns access to an array of variable.
			 * @param name A reference to a string for the array name.
			 * @return std::vector< std::any > *
			 */
			[[nodiscard]]
			std::vector< std::any > * getArrayPointer (const std::string & name) noexcept;

			/**
			 * @brief Clears variable from this store.
			 * @return void
			 */
			void
			clearData () noexcept
			{
				m_variables.clear();
				m_arrays.clear();
			}

			/**
			 * @brief Clears sub-stores from this store.
			 * @return void
			 */
			void
			clearSubStores () noexcept
			{
				m_subStores.clear();
			}

			/**
			 * @brief Clears the store.
			 * @note Same as calling SettingStore::clearData() and Settings::clearSubStores().
			 * @return void
			 */
			void
			clear () noexcept
			{
				this->clearData();
				this->clearSubStores();
			}

			/**
			 * @brief Returns variables.
			 * @return const std::map< std::string, std::any > &
			 */
			[[nodiscard]]
			const std::map< std::string, std::any > &
			variables () const noexcept
			{
				return m_variables;
			}

			/**
			 * @brief Returns arrays variables.
			 * @return const std::map< std::string, std::vector< std::any > > &
			 */
			[[nodiscard]]
			const std::map< std::string, std::vector< std::any > > &
			arrays () const noexcept
			{
				return m_arrays;
			}

			/**
			 * @brief Returns sub-stores.
			 * @return const std::map< std::string, SettingStore > &
			 */
			[[nodiscard]]
			const std::map< std::string, SettingStore > &
			subStores () const noexcept
			{
				return m_subStores;
			}

		private:

			std::map< std::string, std::any > m_variables;
			std::map< std::string, std::vector< std::any > > m_arrays;
			std::map< std::string, SettingStore > m_subStores;
	};
}
