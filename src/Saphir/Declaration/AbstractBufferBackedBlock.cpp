/*
 * src/Saphir/Declaration/AbstractBufferBackedBlock.cpp
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
 * https://github.com/londnoir/emeraude-engine
 *
 * --- THIS IS AUTOMATICALLY GENERATED, DO NOT CHANGE ---
 */

#include "AbstractBufferBackedBlock.hpp"

/* STL inclusions. */
#include <sstream>

/* Local inclusions. */
#include <ranges>


#include "Libs/Utility.hpp"
#include "Tracer.hpp"

namespace EmEn::Saphir::Declaration
{
	using namespace Libs;
	using namespace Saphir::Keys;

	constexpr auto TracerTag{"BufferBackedBlock"};

	bool
	AbstractBufferBackedBlock::isValid () const noexcept
	{
		if ( this->name() == nullptr )
		{
			return false;
		}

		if ( this->instanceName().empty() )
		{
			return false;
		}

		if ( m_members.empty() )
		{
			return false;
		}

		return true;
	}

	uint32_t
	AbstractBufferBackedBlock::bytes () const noexcept
	{
		uint32_t currentOffset = 0;

		/* NOTE: Structure declarations are embedded types, not direct members.
		 * They contribute to size only when referenced by a member. */
		for ( const auto & structure: this->structureDeclaration() | std::views::values )
		{
			currentOffset += structure.bytes();
		}

		/* Calculate size with proper std140 alignment. */
		for ( const auto & member: this->members() | std::views::values )
		{
			const auto memberType = member.type();
			const auto memberSize = member.bytes();

			/* Apply alignment only for std140 layout. */
			if ( m_memoryLayout == MemoryLayout::Std140 )
			{
				uint32_t alignment = base_alignment_std140(memberType);

				/* For arrays in std140, each element is rounded up to vec4 alignment (16 bytes). */
				if ( member.arraySize() > 0 && alignment < 16 )
				{
					alignment = 16;
				}

				/* Align current offset to the member's base alignment. */
				if ( alignment > 0 )
				{
					const uint32_t remainder = currentOffset % alignment;

					if ( remainder != 0 )
					{
						currentOffset += alignment - remainder;
					}
				}
			}

			currentOffset += memberSize;
		}

		/* For block arrays, multiply by array size. */
		if ( this->arraySize() > 1U )
		{
			currentOffset *= this->arraySize();
		}

		return currentOffset;
	}

	bool
	AbstractBufferBackedBlock::addMember (VariableType type, Key name, Key layout) noexcept
	{
		if ( Utility::contains(m_members, name) )
		{
			TraceError{TracerTag} << "This buffer backed block has already a member named '" << name << "' !";

			return false;
		}

		m_members.emplace_back(
			std::piecewise_construct,
			std::forward_as_tuple(name),
			std::forward_as_tuple(type, name, layout, 0)
		);

		return true;
	}

	bool
	AbstractBufferBackedBlock::addMember (const Structure & structure, Key layout) noexcept
	{
		const auto * name = structure.instanceName().c_str();

		if ( Utility::contains(m_members, name) )
		{
			TraceError{TracerTag} << "This buffer backed block has already a member named '" << name << "' !";

			return false;
		}

		if ( !this->addStructureDeclaration(structure.name(), structure) )
		{
			return false;
		}

		m_members.emplace_back(
			std::piecewise_construct,
			std::forward_as_tuple(name),
			std::forward_as_tuple(VariableType::Structure, name, layout, 0)
		);

		return true;
	}

	bool
	AbstractBufferBackedBlock::addArrayMember (VariableType type, Key name, uint32_t arraySize, Key layout) noexcept
	{
		if ( Utility::contains(m_members, name) )
		{
			TraceError{TracerTag} << "This buffer backed block has already a member named '" << name << "' !";

			return false;
		}

		if ( arraySize == 0 )
		{
			return false;
		}

		m_members.emplace_back(
			std::piecewise_construct,
			std::forward_as_tuple(name),
			std::forward_as_tuple(type, name, layout, arraySize)
		);

		return true;
	}

	bool
	AbstractBufferBackedBlock::addArrayMember (const Structure & structure, Key name, uint32_t arraySize, Key layout) noexcept
	{
		if ( Utility::contains(m_members, name) )
		{
			TraceError{TracerTag} << "This buffer backed block has already a member named '" << name << "' !";

			return false;
		}

		if ( arraySize == 0 )
		{
			return false;
		}

		/* NOTE: Copy the structure for declaration.
		 * Go to UniformBlock::sourceCode () for generation. */
		if ( !this->addStructureDeclaration(structure.name(), structure) )
		{
			return false;
		}

		/* NOTE: Wizardy for structures, which is not a regular buffer backed block member. */
		m_members.emplace_back(
			std::piecewise_construct,
			std::forward_as_tuple(structure.name()),
			std::forward_as_tuple(VariableType::Structure, name, layout, arraySize)
		);

		return true;
	}

	std::string
	AbstractBufferBackedBlock::getLayoutQualifier () const noexcept
	{
		std::stringstream code{};

		code << GLSL::Layout << " (";

		switch ( m_matrixStorageOrder )
		{
			case MatrixStorageOrder::Default :
				/* Nothing ... */
				break;

			case MatrixStorageOrder::ColumnMajor :
				code << GLSL::ColumnMajor;
				break;

			case MatrixStorageOrder::RowMajor :
				code << GLSL::RowMajor;
				break;
		}

		switch ( m_memoryLayout )
		{
			case MemoryLayout::Shared :
				code << GLSL::Shared << ", ";
				break;

			case MemoryLayout::Packed :
				code << GLSL::Packed << ", ";
				break;

			case MemoryLayout::Std140 :
				code << GLSL::Std140 << ", ";
				break;

			case MemoryLayout::Std430 :
				code << GLSL::Std430 << ", ";
				break;
		}

		code << GLSL::Set << " = " << m_set << ", " << GLSL::Binding << " = " << m_binding << ") ";

		return code.str();
	}
}
