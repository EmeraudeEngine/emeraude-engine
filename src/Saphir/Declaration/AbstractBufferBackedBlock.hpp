/*
 * src/Saphir/Declaration/AbstractBufferBackedBlock.hpp
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
#include <vector>
#include <cstdint>

/* Local inclusions for inheritances. */
#include "BlockInterface.hpp"

/* Local inclusions for usages. */
#include "Member/BufferBackedBlock.hpp"
#include "Types.hpp"

namespace EmEn::Saphir::Declaration
{
	/**
	 * @brief Base class for every GLSL Buffer-Backed blocks.
	 * @extends EmEn::Saphir::Declaration::BlockInterface
	 */
	class AbstractBufferBackedBlock : public BlockInterface
	{
		public:

			/**
			 * @brief Copy constructor.
			 * @param copy A reference to the copied instance.
			 */
			AbstractBufferBackedBlock (const AbstractBufferBackedBlock & copy) noexcept = default;

			/**
			 * @brief Move constructor.
			 * @param copy A reference to the copied instance.
			 */
			AbstractBufferBackedBlock (AbstractBufferBackedBlock && copy) noexcept = default;

			/**
			 * @brief Copy assignment.
			 * @param copy A reference to the copied instance.
			 * @return AbstractBufferBackedBlock &
			 */
			AbstractBufferBackedBlock & operator= (const AbstractBufferBackedBlock & copy) noexcept = default;

			/**
			 * @brief Move assignment.
			 * @param copy A reference to the copied instance.
			 * @return AbstractBufferBackedBlock &
			 */
			AbstractBufferBackedBlock & operator= (AbstractBufferBackedBlock && copy) noexcept = default;

			/**
			 * @brief Destructs the backed buffer block.
			 */
			~AbstractBufferBackedBlock () override = default;

			/** @copydoc EmEn::Saphir::Declaration::BlockInterface::isValid() */
			[[nodiscard]]
			bool isValid () const noexcept override;

			/** @copydoc EmEn::Saphir::Declaration::BlockInterface::bytes() */
			[[nodiscard]]
			uint32_t bytes () const noexcept final;

			/**
			 * @brief Gets the set index.
			 * @return uint32_t.
			 */
			[[nodiscard]]
			uint32_t
			set () const noexcept
			{
				return m_set;
			}

			/**
			 * @brief Gets the binding point.
			 * @return uint32_t.
			 */
			[[nodiscard]]
			uint32_t
			binding () const noexcept
			{
				return m_binding;
			}

			/**
			 * @brief Sets matrix storage order.
			 * @param matrixStorageOrder
			 * @return void
			 */
			void
			setMatrixStorageOrder (MatrixStorageOrder matrixStorageOrder) noexcept
			{
				m_matrixStorageOrder = matrixStorageOrder;
			}

			/**
			 * @brief Gets matrix storage order.
			 * @return MatrixStorageOrder
			 */
			[[nodiscard]]
			MatrixStorageOrder
			matrixStorageOrder () const noexcept
			{
				return m_matrixStorageOrder;
			}

			/**
			 * @brief Adds a member to the block.
			 * @param type A C-string to set the type of the member. Use one of Keys::GLSL::* keyword.
			 * @param name A C-string to set the name of the member.
			 * @param layout The layout to use. Default nullptr.
			 */
			bool addMember (VariableType type, Key name, Key layout = nullptr) noexcept;

			/**
			 * @brief Adds a member to the block.
			 * @param structure A reference to a structure.
			 * @param layout The layout to use. Default nullptr.
			 */
			bool addMember (const Structure & structure, Key layout = nullptr) noexcept;

			/**
			 * @brief Adds an array member to the block.
			 * @param type A C-string to set the type of the member. Use one of Keys::GLSL::* keyword.
			 * @param name A C-string to set the name of the member.
			 * @param arraySize The size of the array.
			 * @param layout The layout to use. Default nullptr.
			 * @return bool
			 */
			bool addArrayMember (VariableType type, Key name, uint32_t arraySize, Key layout = nullptr) noexcept;

			/**
			 * @brief Adds an array member to the block.
			 * @param structure A reference to a structure.
			 * @param arraySize The size of the array.
			 * @param layout The layout to use. Default nullptr.
			 * @return bool
			 */
			bool addArrayMember (const Structure & structure, uint32_t arraySize, Key layout = nullptr) noexcept;

			/**
			 * @brief Returns the list of members.
			 * @return const std::vector< std::pair< Key, Member::BufferBackedBlock > > &
			 */
			[[nodiscard]]
			const std::vector< std::pair< Key, Member::BufferBackedBlock > > &
			members () const noexcept
			{
				return m_members;
			}

		protected:

			/**
			 * @brief Constructs a backed buffer block.
			 * @param set An integer to define in which set the buffer is.
			 * @param binding An integer to define at which point the buffer is bound.
			 * @param memoryLayout The type of video memory layout.
			 * @param name A C-string to set the name of the block.
			 * @param instanceName A C-string to set the name of the instance of the block. Default nullptr.
			 * @param arraySize Set the block as an array. Default 0.
			 */
			AbstractBufferBackedBlock (uint32_t set, uint32_t binding, MemoryLayout memoryLayout, Key name, Key instanceName = nullptr, uint32_t arraySize = 0) noexcept;

			/**
			 * @brief Returns the layout qualifier.
			 * @return std::string
			 */
			[[nodiscard]]
			std::string getLayoutQualifier () const noexcept;

		private:

			uint32_t m_set;
			uint32_t m_binding;
			MemoryLayout m_memoryLayout;
			MatrixStorageOrder m_matrixStorageOrder = MatrixStorageOrder::Default;
			std::vector< std::pair< Key, Member::BufferBackedBlock > > m_members{};
	};
}
