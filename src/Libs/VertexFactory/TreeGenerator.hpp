/*
 * src/Libs/VertexFactory/TreeGenerator.hpp
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

#pragma once

/* Local inclusions for usages. */
#include "ShapeAssembler.hpp"

namespace EmEn::Libs::VertexFactory
{
	/**
	 * @brief The TreeGenerator class
	 */
	class TreeGenerator final
	{
		public:

			/**
			 * @brief Constructs a default tree generator.
			 */
			TreeGenerator () noexcept = default;

			/**
			 * @brief Sets the base size of generation.
			 * @param baseSize The radius of the trunk.
			 * @return void
			 */
			void
			setBaseSize (float baseSize) noexcept
			{
				m_baseSize = baseSize;
			}

			/**
			 * @brief Returns the base size.
			 * @return float
			 */
			[[nodiscard]]
			float
			baseSize () const noexcept
			{
				return m_baseSize;
			}

			/**
			 * @brief Sets the base height of generation.
			 * @param baseHeight The height of the trunk.
			 * @return void
			 */
			void
			setBaseHeight (float baseHeight) noexcept
			{
				m_baseHeight = baseHeight;
			}

			/**
			 * @brief Returns the base size of generation.
			 * @return float
			 */
			[[nodiscard]]
			float
			baseHeight () const noexcept
			{
				return m_baseHeight;
			}

			/**
			 * @brief setReduceFactor
			 * @param reduceFactor
			 * @return void
			 */
			void
			setReduceFactor (float reduceFactor) noexcept
			{
				m_reduceFactor = Math::clampToUnit(reduceFactor);
			}

			/**
			 * @brief reduceFactor
			 * @return float
			 */
			[[nodiscard]]
			float
			reduceFactor () const noexcept
			{
				return m_reduceFactor;
			}

			/**
			 * @brief setMaxSpreadingAngle
			 * @param maxSpreadingAngle
			 * @return void
			 */
			void
			setMaxSpreadingAngle (float maxSpreadingAngle) noexcept
			{
				m_maxSpreadingAngle = maxSpreadingAngle;
			}

			/**
			 * @brief maxSpreadingAngle
			 * @return float
			 */
			[[nodiscard]]
			float
			maxSpreadingAngle () const noexcept
			{
				return m_maxSpreadingAngle;
			}

			/**
			 * @brief setChaos
			 * @param chaos
			 * @return void
			 */
			void
			setChaos (float chaos) noexcept
			{
				m_chaos = chaos;
			}

			/**
			 * @brief chaos
			 * @return float
			 */
			[[nodiscard]]
			float
			chaos () const noexcept
			{
				return m_chaos;
			}

			/**
			 * @brief setDepth
			 * @param depth
			 * @return void
			 */
			void
			setDepth (size_t depth) noexcept
			{
				m_depth = depth;
			}

			/**
			 * @brief depth
			 * @return size_t
			 */
			[[nodiscard]]
			size_t
			depth () const noexcept
			{
				return m_depth;
			}

			/**
			 * @brief Executes the generation of the tree as a shape.
			 * @param seed The generation seed. Default 0.
			 * @return Shape< float >
			 */
			[[nodiscard]]
			Shape< float > generate (unsigned int seed = 0) noexcept;

		private:

			/**
			 * @brief generateBranch
			 * @param assembler
			 * @param size
			 * @param height
			 * @param origin
			 * @param currentDepth
			 * @param generateSubBranches
			 * @return void
			 */
			void generateBranch (ShapeAssembler< float > & assembler, float size, float height, const Math::CartesianFrame< float > & origin, size_t currentDepth, bool generateSubBranches) noexcept;

			float m_baseSize{1.0F};
			float m_baseHeight{4.0F};
			float m_reduceFactor{0.75F};
			float m_maxSpreadingAngle{15.0F};
			float m_chaos{0.1F};
			size_t m_depth{3};
		};
}
