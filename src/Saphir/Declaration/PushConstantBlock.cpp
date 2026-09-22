/*
 * src/Saphir/Declaration/PushConstantBlock.cpp
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

#include "PushConstantBlock.hpp"

/* STL inclusions. */
#include <algorithm>
#include <ranges>
#include <sstream>

/* Local inclusions. */
#include "BaseUtility.hpp"
#include "Tracer.hpp"

namespace EmEn::Saphir::Declaration
{
	using namespace Base;
	using namespace Saphir::Keys;

	uint32_t
	PushConstantBlock::bytes () const noexcept
	{
		uint32_t size = 0;

		for ( const auto & structure : this->structureDeclaration() | std::views::values )
		{
			size += structure.bytes();
		}

		/* Push-constant blocks use std430 layout: each member starts at an offset aligned to its
		 * base alignment (e.g. a mat4 aligns to 16). Summing raw member sizes underestimates the
		 * block size whenever a smaller member precedes a mat4 — e.g. the MDI block
		 * (uint, uint, mat4, float): naive sum 76 vs real layout 84 (8 bytes of padding before the
		 * mat4). A too-small VkPushConstantRange then trips VUID-VkGraphicsPipelineCreateInfo-layout
		 * (the SPIR-V block is larger than the declared range). For the scalar/vector/matrix types
		 * used in push constants, std430 and std140 base alignments coincide, so base_alignment_std140
		 * is reused here. */
		for ( const auto & pushConstant : this->members() | std::views::values )
		{
			const auto alignment = base_alignment_std140(pushConstant.type());

			if ( alignment > 0 )
			{
				const auto remainder = size % alignment;

				if ( remainder != 0 )
				{
					size += alignment - remainder;
				}
			}

			/* ⚠️ The member's OWN std430 size, not its padded one: Member::PushConstant::bytes() goes
			 * through size_bytes(), which rounds a vec2 or a vec3 up to 16 bytes. Summed here, it put
			 * every member after the TAA jitter (a vec2) 8 bytes later than the SPIR-V block does — the
			 * range only grew, so nothing failed, until a heightfield pushed its node at the offset this
			 * computed (96) while the shader read it at 80: every terrain vertex went to garbage,
			 * without a single validation message (2026-09-22). */
			switch ( pushConstant.type() )
			{
				case VariableType::FloatVector2 :
				case VariableType::UIntVector2 :
				case VariableType::SIntVector2 :
				case VariableType::BooleanVector2 :
					size += 8U * std::max(1U, pushConstant.arraySize());
					break;

				case VariableType::FloatVector3 :
				case VariableType::UIntVector3 :
				case VariableType::SIntVector3 :
				case VariableType::BooleanVector3 :
					/* std430 keeps a vec3 ARRAY on a 16-byte stride; the last element is 12 bytes. */
					size += (16U * (std::max(1U, pushConstant.arraySize()) - 1U)) + 12U;
					break;

				default :
					size += pushConstant.bytes();
					break;
			}
		}

		if ( this->arraySize() > 1U )
		{
			size *= this->arraySize();
		}

		return size;
	}

	bool
	PushConstantBlock::addMember (VariableType type, Key name) noexcept
	{
		if ( Utility::contains(m_members, name) )
		{
			TraceError{ClassId} << "This push constant block has already a member named '" << name << "' !";

			return false;
		}

		m_members.emplace_back(
			std::piecewise_construct,
			std::forward_as_tuple(name),
			std::forward_as_tuple(type, name, 0)
		);

		return true;
	}

	bool
	PushConstantBlock::addArrayMember (VariableType type, Key name, uint32_t arraySize) noexcept
	{
		if ( Utility::contains(m_members, name) )
		{
			TraceError{ClassId} << "This push constant block has already a member named '" << name << "' !";

			return false;
		}

		if ( arraySize == 0 )
		{
			return false;
		}

		m_members.emplace_back(
			std::piecewise_construct,
			std::forward_as_tuple(name),
			std::forward_as_tuple(type, name, arraySize)
		);

		return true;
	}

	std::string
	PushConstantBlock::sourceCode () const noexcept
	{
		std::stringstream code;

		/* Default Std430 */
		code << GLSL::Layout << " (" << GLSL::PushConstant << ") " << GLSL::Uniform << ' ' << this->name() << "\n" "{" "\n";

		for ( const auto & pushConstant : m_members | std::views::values )
		{
			code << '\t' <<  pushConstant.sourceCode();
		}

		code << '}';

		if ( !this->instanceName().empty() )
		{
			code << ' ' << this->instanceName();
		}

		code << ";" "\n";

		return code.str();
	}
}
