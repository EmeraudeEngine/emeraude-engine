/*
 * src/Saphir/Declaration/BlockInterface.hpp
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
#include <unordered_map>

/* Local inclusions for inheritances. */
#include "Interface.hpp"
#include "Structure.hpp"

namespace EmEn::Saphir::Declaration
{
	/**
	 * @brief Base class for GLSL interface blocks.
	 * @extends EmEn::Saphir::Declaration::Interface This is a shader code declaration.
	 *
	 * storage_qualifier block_name
	 * {
	 *		<define members here>
	 * } instance_name;
	 */
	class BlockInterface : public Interface
	{
		public:

			/**
			 * @brief Copy constructor.
			 * @param copy A reference to the copied instance.
			 */
			BlockInterface (const BlockInterface & copy) noexcept = default;

			/**
			 * @brief Move constructor.
			 * @param copy A reference to the copied instance.
			 */
			BlockInterface (BlockInterface && copy) noexcept = default;

			/**
			 * @brief Copy assignment.
			 * @param copy A reference to the copied instance.
			 * @return BlockInterface &
			 */
			BlockInterface & operator= (const BlockInterface & copy) noexcept = default;

			/**
			 * @brief Move assignment.
			 * @param copy A reference to the copied instance.
			 * @return BlockInterface &
			 */
			BlockInterface & operator= (BlockInterface && copy) noexcept = default;

			/**
			 * @brief Destructs the block interface.
			 */
			~BlockInterface () override = default;

			/** @copydoc EmEn::Saphir::Declaration::Interface::name() */
			[[nodiscard]]
			Key
			name () const noexcept final
			{
				return m_name;
			}

			/**
			 * @brief Returns the name of the instance of the block in GLSL code.
			 * @return std::string
			 */
			[[nodiscard]]
			const std::string &
			instanceName () const noexcept
			{
				return m_instanceName;
			}

			/**
			 * @brief Returns the size of the array.
			 * @return uint32_t
			 */
			[[nodiscard]]
			uint32_t
			arraySize () const noexcept
			{
				return m_arraySize;
			}

			/**
			 * @brief Returns a list of structure in use in this block.
			 * @return const std::unordered_map< Key, Structure > &
			 */
			[[nodiscard]]
			const std::unordered_map< Key, Structure > &
			structures () const noexcept
			{
				return m_structures;
			}

		protected:

			/**
			 * @brief Constructs an interface blocks.
			 * @param name A C-string to set the name of the block.
			 * @param instanceName A C-string to set the name of the instance of the block. Default nullptr.
			 * @param arraySize Set the block as an array. Default 0.
			 */
			explicit BlockInterface (Key name, Key instanceName = nullptr, uint32_t arraySize = 0) noexcept;
			
			/**
			 * @brief Adds a structure declaration in use in this block.
			 * @param name A C-string to set the name of the structure.
			 * @param structure A reference to a structure.
			 * @return bool
			 */
			bool addStructure (Key name, const Structure & structure) noexcept;

		private:

			Key m_name;
			std::string m_instanceName;
			uint32_t m_arraySize;
			std::unordered_map< Key, Structure > m_structures{};
	};
}
