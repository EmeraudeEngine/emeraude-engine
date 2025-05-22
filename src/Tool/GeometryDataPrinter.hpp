/*
 * src/Tool/GeometryDataPrinter.hpp
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
#include <cstdint>
#include <array>
#include <string>

/* Local inclusions for inheritances. */
#include "ToolInterface.hpp"

/* Local inclusions for usages. */
#include "Graphics/Types.hpp"
#include "Libs/VertexFactory/Shape.hpp"

namespace EmEn::Tool
{
	/**
	 * @brief The geometry data printer tool.
	 * @extends EmEn::Tool::ToolInterface This is a tool interface.
	 */
	class GeometryDataPrinter final : public ToolInterface
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"GeometryDataPrinter"};

			/**
			 * @brief Constructs the geometry data printer.
			 * @param arguments A reference to the arguments.
			 */
			explicit GeometryDataPrinter (const Arguments & arguments) noexcept;

			/** @copydoc EmEn::Tool::ToolInterface::execute() */
			[[nodiscard]]
			bool execute () noexcept override;

		private:

			/**
			 * @brief Print vertices array.
			 * @return std::string
			 */
			[[nodiscard]]
			std::string printData () const noexcept;

			/* Flag names */
			static constexpr auto EnableNormals{0UL};
			static constexpr auto EnableTangentSpace{1UL};
			static constexpr auto EnableTexCoords{2UL};
			static constexpr auto Enable3DTexCoords{3UL};

			std::string m_outputFile;
			Libs::VertexFactory::Shape< float, uint32_t > m_shape;
			Graphics::ShapeType m_shapeType{Graphics::ShapeType::Cube};
			float m_baseSize{1.0F};
			float m_baseLength{4.0F};
			uint32_t m_quality{8};
			std::array< bool, 8 > m_flags{
				false/*EnableNormals*/,
				false/*EnableTangentSpace*/,
				false/*EnableTexCoords*/,
				false/*Enable3DTexCoords*/,
				false/*UNUSED*/,
				false/*UNUSED*/,
				false/*UNUSED*/
			};
	};
}
