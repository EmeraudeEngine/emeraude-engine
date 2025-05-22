/*
 * src/Graphics/Geometry/GenerationParameters.hpp
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
#include "Libs/FlagTrait.hpp"

/* Local inclusions for usages. */
#include "Libs/Math/Matrix.hpp"
#include "Libs/Math/Vector.hpp"
#include "Libs/PixelFactory/Color.hpp"
#include "Libs/VertexFactory/ShapeBuilderOptions.hpp"
#include "Graphics/Types.hpp"

namespace EmEn::Graphics::Geometry
{
	/**
	 * @brief Defines parameters to generate a geometry with the resource generator.
	 * @extends EmEn::Libs::FlagTrait Needs FlagTrait to pass it to generated geometries.
	 */
	class GenerationParameters final : public Libs::FlagTrait< uint32_t >
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"GenerationParameters"};

			/**
			 * @brief Constructs a generation parameters object.
			 * @param geometryFlagBits Flags value from GeometryFlagBits. Default 0.
			 */
			explicit GenerationParameters (uint32_t geometryFlagBits = 0) noexcept;

			/**
			 * @brief Sets the way the vertex color will be generated.
			 * @param vertexColorGenMode The mode.
			 * @return void
			 */
			void
			setVertexColorGenMode (VertexColorGenMode vertexColorGenMode) noexcept
			{
				m_vertexColorGenMode = vertexColorGenMode;
			}

			/**
			 * @brief Returns the way the vertex color will be generated.
			 * @return VertexColorGenMode
			 */
			[[nodiscard]]
			VertexColorGenMode
			vertexColorGenMode () const noexcept
			{
				return m_vertexColorGenMode;
			}

			/**
			 * @brief Sets the global vertex color for mesh generation.
			 * @note Vertex color attribute must be enabled, this method only change the color.
			 * @param globalVertexColor A reference to a PixelFactory color.
			 * @return void
			 */
			void
			setGlobalVertexColor (const Libs::PixelFactory::Color< float > & globalVertexColor) noexcept
			{
				m_globalVertexColor = globalVertexColor;

				m_flags[GlobalVertexColorSet] = true;
			}

			/**
			 * @brief Applies a transform matrix on the generated resource.
			 * @param transformMatrix A reference to a 4x4 matrix.
			 * @return void
			 */
			void
			setTransformMatrix (const Libs::Math::Matrix< 4, float > & transformMatrix) noexcept
			{
				m_transformMatrix = transformMatrix;
			}

			/**
			 * @brief Sets a texture coordinates uniform multiplier.
			 * @param multiplier The multiplication factor on each axis.
			 * @return void
			 */
			void
			setTextureCoordinatesMultiplier (float multiplier) noexcept
			{
				using namespace EmEn::Libs::Math;

				m_textureCoordinatesMultiplier[X] = multiplier;
				m_textureCoordinatesMultiplier[Y] = multiplier;
				m_textureCoordinatesMultiplier[Z] = multiplier;
			}

			/**
			 * @brief Sets 2D texture coordinates multipliers.
			 * @param xMultiplier The multiplication factor on the X axis.
			 * @param yMultiplier The multiplication factor on the Y axis.
			 * @return void
			 */
			void
			setTextureCoordinatesMultiplier (float xMultiplier, float yMultiplier) noexcept
			{
				using namespace EmEn::Libs::Math;

				m_textureCoordinatesMultiplier[X] = xMultiplier;
				m_textureCoordinatesMultiplier[Y] = yMultiplier;
				m_textureCoordinatesMultiplier[Z] = 1.0F;
			}

			/**
			 * @brief Sets 3D texture coordinates multipliers.
			 * @param xMultiplier The multiplication factor on the X axis.
			 * @param yMultiplier The multiplication factor on the Y axis.
			 * @param zMultiplier The multiplication factor on the Z axis.
			 * @return void
			 */
			void
			setTextureCoordinatesMultiplier (float xMultiplier, float yMultiplier, float zMultiplier) noexcept
			{
				using namespace EmEn::Libs::Math;

				m_textureCoordinatesMultiplier[X] = xMultiplier;
				m_textureCoordinatesMultiplier[Y] = yMultiplier;
				m_textureCoordinatesMultiplier[Z] = zMultiplier;
			}

			/**
			 * @brief Sets the geometry flags.
			 * @param geometryflags Flags value from GeometryFlagBits.
			 * @return void
			 */
			void
			setGeometryFlags (uint32_t geometryflags) noexcept
			{
				this->setFlags(geometryflags);
			}

			/**
			 * @brief Sets the center of the geometry at bottom.
			 * @param state The state.
			 * @return void
			 */
			void
			setCenterAtBottom (bool state) noexcept
			{
				m_flags[CenterAtBottom] = state;
			}

			/**
			 * @brief Sets the geometry surface flipping state.
			 * @param state The state.
			 * @return void
			 */
			void
			setFlipGeometry (bool state) noexcept
			{
				m_flags[FlipGeometry] = state;
			}

			/**
			 * @brief Returns the global vertex color.
			 * @return const Libraries::PixelFactory::Color< float > &
			 */
			[[nodiscard]]
			const Libs::PixelFactory::Color< float > &
			globalVertexColor () const noexcept
			{
				return m_globalVertexColor;
			}

			/**
			 * @brief Returns the transform matrix.
			 * @return const Libs::Math::Matrix< 4, float > &
			 */
			[[nodiscard]]
			const Libs::Math::Matrix< 4, float > &
			transformMatrix () const noexcept
			{
				return m_transformMatrix;
			}

			/**
			 * @brief Returns the texture coordinate multipliers vector.
			 * @return const Libs::Math::Vector< 3, float > &
			 */
			[[nodiscard]]
			const Libs::Math::Vector< 3, float > &
			textureCoordinatesMultiplier () const noexcept
			{
				return m_textureCoordinatesMultiplier;
			}

			/**
			 * @brief Returns the geometry flags.
			 * @return uint32_t
			 */
			[[nodiscard]]
			uint32_t
			geometryFlags () const noexcept
			{
				return this->flags();
			}

			/**
			 * @brief Returns whether the center at bottom option is enabled.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isCenteredAtBottom () const noexcept
			{
				return m_flags[CenterAtBottom];
			}

			/**
			 * @brief Returns whether the geometry surface flipping is enabled.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			flipGeometry () const noexcept
			{
				return m_flags[FlipGeometry];
			}

			/**
			 * @brief Returns a unique identifier for these parameters to complete a generated resource name.
			 * @return std::string
			 */
			[[nodiscard]]
			std::string uniqueIdentifier () const noexcept;

			/**
			 * @brief Creates and returns an option structure for the shape builder.
			 * @return Libraries::VertexFactory::ShapeBuilderOptions< float >
			 */
			[[nodiscard]]
			Libs::VertexFactory::ShapeBuilderOptions< float > getShapeBuilderOptions () const noexcept;

		private:

			/* Flag names. */
			static constexpr auto CenterAtBottom{0UL};
			static constexpr auto FlipGeometry{1UL};
			static constexpr auto GlobalVertexColorSet{2UL};

			VertexColorGenMode m_vertexColorGenMode{VertexColorGenMode::UseRandom};
			Libs::PixelFactory::Color< float > m_globalVertexColor{Libs::PixelFactory::Magenta};
			Libs::Math::Matrix< 4, float > m_transformMatrix;
			Libs::Math::Vector< 3, float > m_textureCoordinatesMultiplier{1.0F, 1.0F, 1.0F};
			std::array< bool, 8 > m_flags{
				false/*CenterAtBottom*/,
				false/*FlipGeometry*/,
				false/*GlobalVertexColorSet*/,
				false/*UNUSED*/,
				false/*UNUSED*/,
				false/*UNUSED*/,
				false/*UNUSED*/,
				false/*UNUSED*/
			};
	};
}
