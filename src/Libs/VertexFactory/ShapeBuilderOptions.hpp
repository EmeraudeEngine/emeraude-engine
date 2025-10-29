/*
 * src/Libs/VertexFactory/ShapeBuilderOptions.hpp
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

/* Engine configuration. */
#include "emeraude_config.hpp"

/* STL inclusions. */
#include <array>
#include <ostream>
#include <string>
#include <type_traits>

/* Local inclusions for usages. */
#include "Libs/Math/Vector.hpp"
#include "Libs/PixelFactory/Color.hpp"

namespace EmEn::Libs::VertexFactory
{
	/**
	 * @brief The shape builder options.
	 * @tparam vertex_data_t The type of floating point number. Default float.
	 */
	template< typename vertex_data_t = float >
	requires (std::is_floating_point_v< vertex_data_t >)
	class ShapeBuilderOptions final
	{
		public:

			/**
			 * @brief Constructs a default shape builder options structure.
			 */
			ShapeBuilderOptions () noexcept = default;

			/**
			 * @brief Constructs a shape builder options structure.
			 * @param enableNormals Enable normals attributes.
			 * @param enableTextureCoordinates Enable texture coordinates attributes.
			 * @param vertexColorsEnabled Enable vertex colors attributes.
			 * @param influencesEnabled Enable influences attributes.
			 * @param weightsEnabled Enable weights attributes.
			 */
			ShapeBuilderOptions (bool enableNormals, bool enableTextureCoordinates, bool vertexColorsEnabled, bool influencesEnabled, bool weightsEnabled) noexcept
				: m_normalsEnabled{enableNormals},
				m_textureCoordinatesEnabled{enableTextureCoordinates},
				m_vertexColorsEnabled{vertexColorsEnabled},
				m_influencesEnabled{influencesEnabled},
				m_weightsEnabled{weightsEnabled}
			{

			}

			/**
			 * @brief Returns whether normals are enabled.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isNormalsEnabled () const noexcept
			{
				return m_normalsEnabled;
			}

			/**
			 * @brief Returns whether texture coordinates are enabled.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isTextureCoordinatesEnabled () const noexcept
			{
				return m_textureCoordinatesEnabled;
			}

			/**
			 * @brief Returns whether vertex colors are enabled.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isVertexColorsEnabled () const noexcept
			{
				return m_vertexColorsEnabled;
			}

			/**
			 * @brief Returns whether influences are enabled.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isInfluencesEnabled () const noexcept
			{
				return m_influencesEnabled;
			}

			/**
			 * @brief Returns whether weights are enabled.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isWeightsEnabled () const noexcept
			{
				return m_weightsEnabled;
			}

			/**
			 * @brief Uses a global normal vector.
			 * @note This will disable the normals automatic generation.
			 * @param normal A reference to vector 3.
			 * @return void
			 */
			void
			enableGlobalNormal (const Math::Vector< 3, vertex_data_t > & normal) noexcept
			{
				m_normalsEnabled = true;
				m_globalNormalEnabled = true;
				m_normalsGenerationEnabled = false;

				m_globalNormal = normal;
			}

			/**
			 * @brief Returns whether global normal is in use.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isGlobalNormalEnabled () const noexcept
			{
				return m_globalNormalEnabled;
			}

			/**
			 * @brief Returns the global vertex color.
			 * @return const Math::Vector< 3, vertex_data_t > &
			 */
			[[nodiscard]]
			const Math::Vector< 3, vertex_data_t > &
			globalNormal () const noexcept
			{
				return m_globalNormal;
			}

			/**
			 * @brief Enables the normal generation from position.
			 * @note This will disable the global normal.
			 * @return void
			 */
			void
			enableNormalsGeneration () noexcept
			{
				m_normalsEnabled = true;
				m_globalNormalEnabled = false;
				m_normalsGenerationEnabled = true;
			}

			/**
			 * @brief Returns whether normals automatic generation is enabled.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isNormalsGenerationEnabled () const noexcept
			{
				return m_normalsGenerationEnabled;
			}

			/**
			 * @brief Enables the texture coordinates generation from position.
			 * @return void
			 */
			void
			enableTextureCoordinatesGeneration () noexcept
			{
				m_textureCoordinatesEnabled = true;
				m_textureCoordinatesGenerationEnabled = true;
			}

			/**
			 * @brief Returns whether texture coordinates automatic generation is enabled.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isTextureCoordinatesGenerationEnabled () const noexcept
			{
				return m_textureCoordinatesGenerationEnabled;
			}
		
			/**
			 * @brief Use a global vertex color.
			 * @note This will disable the vertex colors automatic generation.
			 * @param vertexColor A reference to vector 4.
			 * @return void
			 */
			void
			enableGlobalVertexColor (const Math::Vector< 4, vertex_data_t > & vertexColor) noexcept
			{
				m_textureCoordinatesEnabled = true;
				m_globalVertexColorEnabled = true;
				m_vertexColorsGenerationEnabled = false;

				m_globalVertexColor = vertexColor;
			}

			/**
			 * @brief Use a global vertex color.
			 * @note This will disable the vertex colors automatic generation.
			 * @param vertexColor A reference to a color.
			 * @return void
			 */
			void
			enableGlobalVertexColor (const PixelFactory::Color< float > & vertexColor) noexcept
			{
				this->enableGlobalVertexColor(vertexColor.toVector4< vertex_data_t >());
			}

			/**
			 * @brief Returns whether a global vertex color is in use.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isGlobalVertexColorEnabled () const noexcept
			{
				return m_globalVertexColorEnabled;
			}

			/**
			 * @brief Returns the global vertex color.
			 * @return const Math::Vector< 4, vertex_data_t > &
			 */
			[[nodiscard]]
			const Math::Vector< 4, vertex_data_t > &
			globalVertexColor () const noexcept
			{
				return m_globalVertexColor;
			}

			/**
			 * @brief Enables the vertex color generation from position.
			 * @note This will disable the global vertex color.
			 * @return void
			 */
			void
			enableVertexColorsGeneration () noexcept
			{
				m_textureCoordinatesEnabled = true;
				m_globalVertexColorEnabled = false;
				m_vertexColorsGenerationEnabled = true;
			}

			/**
			 * @brief Returns whether vertex colors automatic generation is enabled.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isVertexColorsGenerationEnabled () const noexcept
			{
				return m_vertexColorsGenerationEnabled;
			}

			/**
			 * @brief Sets uniform texture coordinates state.
			 * @param state The state.
			 * @return void
			 */
			void
			setUniformTextureCoordinates (bool state) noexcept
			{
				m_uniformTextureCoordinates = state;
			}

			/**
			 * @brief Sets texture coordinates multipliers.
			 * @param xMultiplier The multiplication factor on the X axis.
			 * @param yMultiplier The multiplication factor on the Y axis. Default Same as X.
			 * @param zMultiplier The multiplication factor on the Z axis. Default Same as Z.
			 * @return void
			 */
			void
			setTextureCoordinatesMultiplier (vertex_data_t xMultiplier, vertex_data_t yMultiplier = 0, vertex_data_t zMultiplier = 0) noexcept
			{
				m_textureCoordinatesMultiplier[Math::X] = std::abs(xMultiplier);
				m_textureCoordinatesMultiplier[Math::Y] = Utility::isZero(yMultiplier) ? m_textureCoordinatesMultiplier[Math::X] : std::abs(yMultiplier);
				m_textureCoordinatesMultiplier[Math::Z] = Utility::isZero(zMultiplier) ? m_textureCoordinatesMultiplier[Math::X] : std::abs(zMultiplier);
			}

			/**
			 * @brief Sets texture coordinates multipliers.
			 * @param multiplier A reference to a vector.
			 * @return void
			 */
			void
			setTextureCoordinatesMultiplier (const Math::Vector< 2, vertex_data_t > & multiplier) noexcept
			{
				this->setTextureCoordinatesMultiplier(multiplier[Math::X], multiplier[Math::Y]);
			}

			/**
			 * @brief Sets texture coordinates multipliers.
			 * @param multiplier A reference to a vector.
			 * @return void
			 */
			void
			setTextureCoordinatesMultiplier (const Math::Vector< 3, vertex_data_t > & multiplier) noexcept
			{
				this->setTextureCoordinatesMultiplier(multiplier[Math::X], multiplier[Math::Y], multiplier[Math::Z]);
			}

			/**
			 * @brief Returns the texture coordinates multiplier.
			 * @return const Math::Vector< 3, vertex_data_t > &
			 */
			[[nodiscard]]
			const Math::Vector< 3, vertex_data_t > &
			textureCoordinatesMultiplier () const noexcept
			{
				return m_textureCoordinatesMultiplier;
			}

			/**
			 * @brief Enables the data economy.
			 * @param state The state.
			 * @return void
			 */
			void
			enableDataEconomy (bool state) noexcept
			{
				m_dataEconomyEnabled = state;
			}

			/**
			 * @brief Returns whether the data economy is enabled.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			dataEconomyEnabled () const noexcept
			{
				return m_dataEconomyEnabled;
			}

			/**
			 * @brief Sets the center of the shape at bottom.
			 * @param state The state.
			 * @return void
			 */
			void
			setCenterAtBottom (bool state) noexcept
			{
				m_centerAtBottom = state;
			}

			/**
			 * @brief Returns whether the geometry must be centered at the bottom.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isCenteredAtBottom () const noexcept
			{
				return m_centerAtBottom;
			}

			/**
			 * @brief Enables or disables the geometry flipping at the end of the generation.
			 * @param state The state.
			 * @return void
			 */
			void
			enableGeometryFlipping (bool state) noexcept
			{
				m_flipGeometry = state;
			}

			/**
			 * @brief Returns whether the geometry flipping is enabled.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isGeometryFlippingEnabled () const noexcept
			{
				return m_flipGeometry;
			}

			/**
			 * @brief Resets the construction.
			 * @return void
			 */
			void
			reset () noexcept
			{
				m_globalVertexColor = {0.5, 0.5, 0.5, 1.0};
				m_globalNormal = {0.0, 0.0, -1.0};
				m_textureCoordinatesMultiplier = {1.0, 1.0, 1.0};

				m_normalsEnabled = false;
				m_textureCoordinatesEnabled = false;
				m_vertexColorsEnabled = false;
				m_influencesEnabled = false;
				m_weightsEnabled = false;
				m_globalNormalEnabled = false;
				m_normalsGenerationEnabled = false;
				m_textureCoordinatesGenerationEnabled = false;
				m_globalVertexColorEnabled = false;
				m_vertexColorsGenerationEnabled = false;
				m_dataEconomyEnabled = true;
				m_centerAtBottom = false;
				m_uniformTextureCoordinates = false;
				m_flipGeometry = false;
			}

			/**
			 * @brief STL streams printable object.
			 * @param out A reference to the stream output.
			 * @param obj A reference to the object to print.
			 * @return std::ostream &
			 */
			friend
			std::ostream &
			operator<< (std::ostream & out, const ShapeBuilderOptions & obj)
			{
				using namespace std;

				return out <<
					"Shape builder options:" "\n"
					"Normals enabled : " << ( obj.m_normalsEnabled ? "Yes" : "No" ) << "\n"
					"Texture coordinates enabled : " << ( obj.m_textureCoordinatesEnabled ? "Yes" : "No" ) << "\n"
					"Vertex colors enabled : " << ( obj.m_vertexColorsEnabled ? "Yes" : "No" ) << "\n"
					"Influences enabled : " << ( obj.m_influencesEnabled ? "Yes" : "No" ) << "\n"
					"Weights enabled : " << ( obj.m_weightsEnabled ? "Yes" : "No" ) << "\n"
					"Use global normal : " << ( obj.m_globalNormalEnabled ? "Yes" : "No" ) << "\n"
					"Global normal vector : " << obj.m_globalNormal << "\n"
					"Generate normals : " << ( obj.m_normalsGenerationEnabled ? "Yes" : "No" ) << "\n"
					"Use global vertex color : " << ( obj.m_globalVertexColorEnabled ? "Yes" : "No" ) << "\n"
					"Global vertex color : " << obj.m_globalVertexColor << "\n"
					"Generate vertex colors : " << ( obj.m_vertexColorsGenerationEnabled ? "Yes" : "No" ) << "\n"
					"Data economy enabled : " << ( obj.m_dataEconomyEnabled ? "Yes" : "No" ) << "\n"
					"Center at bottom : " << ( obj.m_centerAtBottom ? "Yes" : "No" ) << "\n"
					"Uniform texture coordinates : " << ( obj.m_uniformTextureCoordinates ? "Yes" : "No" ) << "\n"
					"UVW multipliers : " << obj.m_textureCoordinatesMultiplier << "\n"
				;
			}

			/**
			 * @brief Stringifies the object.
			 * @param obj A reference to the object to print.
			 * @return std::string
			 */
			friend
			std::string
			to_string (const ShapeBuilderOptions & obj) noexcept
			{
				std::stringstream output;

				output << obj;

				return output.str();
			}

		private:

			Math::Vector< 4, vertex_data_t > m_globalVertexColor{0.5, 0.5, 0.5, 1.0};
			Math::Vector< 3, vertex_data_t > m_globalNormal{0.0, 0.0, -1.0};
			Math::Vector< 3, vertex_data_t > m_textureCoordinatesMultiplier{1.0, 1.0, 1.0};
			bool m_normalsEnabled{false};
			bool m_textureCoordinatesEnabled{false};
			bool m_vertexColorsEnabled{false};
			bool m_influencesEnabled{false};
			bool m_weightsEnabled{false};
			bool m_globalNormalEnabled{false};
			bool m_normalsGenerationEnabled{false};
			bool m_textureCoordinatesGenerationEnabled{false};
			bool m_globalVertexColorEnabled{false};
			bool m_vertexColorsGenerationEnabled{false};
			bool m_dataEconomyEnabled{true};
			bool m_centerAtBottom{false};
			bool m_uniformTextureCoordinates{false};
			bool m_flipGeometry{false};
	};
}
