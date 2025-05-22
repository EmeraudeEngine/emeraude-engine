/*
 * src/Libs/VertexFactory/ShapeBuilder.hpp
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

/* Emeraude-Engine configuration. */
#include "emeraude_config.hpp"

/* STL inclusions. */
#include <array>
#include <vector>
#include <iostream>
#include <type_traits>

/* Local inclusions. */
#include "Shape.hpp"
#include "ShapeBuilderOptions.hpp"

namespace EmEn::Libs::VertexFactory
{
	/**
	 * @brief The triangle construction mode enumeration.
	 */
	enum class ConstructionMode
	{
		None,
		Triangles,
		TriangleStrip,
		TriangleFan
	};

	/**
	 * @brief The shape builder construct a geometry shape by providing the structure by triangles.
	 * @tparam vertex_data_t The precision type of vertex data. Default float.
	 * @tparam index_data_t The precision type of index data. Default uint32_t.
	 */
	template< typename vertex_data_t = float, typename index_data_t = uint32_t >
	requires (std::is_floating_point_v< vertex_data_t > && std::is_unsigned_v< index_data_t > )
	class ShapeBuilder final
	{
		public:

			/**
			 * @brief Constructs a default shape builder.
			 * @param options A reference to initial builder options. Default none.
			 */
			explicit
			ShapeBuilder (const ShapeBuilderOptions< vertex_data_t > & options = {}) noexcept
				: m_options(options)
			{

			}

			/**
			 * @brief Constructs the shape builder with a targeted destination.
			 * @param destinationShape A reference to a shape to build.
			 * @param options A reference to initial builder options. Default none.
			 */
			explicit
			ShapeBuilder (Shape< vertex_data_t, index_data_t > & destinationShape, const ShapeBuilderOptions< vertex_data_t > & options = {}) noexcept
				: m_destinationShape(&destinationShape),
				m_options(options)
			{

			}

			/**
			 * @brief Copy constructor.
			 * @param copy A reference to the copied instance.
			 */
			ShapeBuilder (const ShapeBuilder & copy) noexcept = delete;

			/**
			 * @brief Move constructor.
			 * @param copy A reference to the copied instance.
			 */
			ShapeBuilder (ShapeBuilder && copy) noexcept = delete;

			/**
			 * @brief Copy assignment.
			 * @param copy A reference to the copied instance.
			 */
			ShapeBuilder & operator= (const ShapeBuilder & copy) noexcept = delete;

			/**
			 * @brief Move assignment.
			 * @param copy A reference to the copied instance.
			 */
			ShapeBuilder & operator= (ShapeBuilder && copy) noexcept = delete;

			/**
			 * @brief Destructs the shape assembler.
			 */
			~ShapeBuilder () = default;

			/**
			 * @brief Changes the destination shape.
			 * @param destinationShape A reference to a shape to build.
			 * @return void
			 */
			void
			setDestinationShape (Shape< vertex_data_t, index_data_t > & destinationShape) noexcept
			{
				if ( m_constructionMode != ConstructionMode::None )
				{
					std::cerr << "[" << __PRETTY_FUNCTION__ << "] You can't set an other Geometry while constructing !" "\n";

					return;
				}

				this->reset();

				m_destinationShape = &destinationShape;
			}

			/**
			 * @brief Returns a reference to options to parametrize the build.
			 * @return ShapeBuilderOptions< vertex_data_t > &
			 */
			[[nodiscard]]
			ShapeBuilderOptions< vertex_data_t > &
			options () noexcept
			{
				return m_options;
			}

			/**
			 * @brief Begins the shape construction.
			 * @param constructionMode The primitive type for construction. Default Triangles.
			 * @return void
			 */
			void
			beginConstruction (ConstructionMode constructionMode = ConstructionMode::Triangles) noexcept
			{
				if ( m_constructionMode != ConstructionMode::None )
				{
					std::cerr << "[" << __PRETTY_FUNCTION__ << "] A construction is already begun !" "\n";

					return;
				}

				if ( m_destinationShape == nullptr )
				{
					std::cerr << "[" << __PRETTY_FUNCTION__ << "] Unable to begin a construction on a null Geometry !" "\n";

					return;
				}

				/* Reset the current triangle building from the builder. */
				{
					this->resetCurrentTriangle();

					m_constructionMode = constructionMode;
				}

				if constexpr ( VertexFactoryDebugEnabled )
				{
					std::cout << "[DEBUG:VERTEX_FACTORY] Ready for construction ... " "\n";
				}
			}

			/**
			 * @brief Declares a new group in the geometry.
			 * @return void
			 */
			void
			newGroup () noexcept
			{
				if ( m_constructionMode == ConstructionMode::None )
				{
					std::cerr << "[" << __PRETTY_FUNCTION__ << "] Builder is not in construction mode !" "\n";

					return;
				}

				this->resetCurrentTriangle();

				m_destinationShape->newGroup();
			}

			/**
			 * @brief Creates a new vertex.
			 * @param position A reference to vector.
			 * @return void
			 */
			void
			setPosition (const Math::Vector< 3, vertex_data_t > & position) noexcept
			{
				m_triangleVertices[m_triangleVertexIndex] = position;
				m_declaredAttributes[PositionDeclared] = true;
			}

			void
			setPosition (vertex_data_t x, vertex_data_t y, vertex_data_t z) noexcept
			{
				m_triangleVertices[m_triangleVertexIndex][Math::X] = x;
				m_triangleVertices[m_triangleVertexIndex][Math::Y] = y;
				m_triangleVertices[m_triangleVertexIndex][Math::Z] = z;
				m_declaredAttributes[PositionDeclared] = true;
			}

			void
			setNormal (const Math::Vector< 3, vertex_data_t > & normal) noexcept
			{
				m_triangleNormals[m_triangleVertexIndex] = normal;
				m_declaredAttributes[NormalDeclared] = true;
			}

			void
			setNormal (vertex_data_t x, vertex_data_t y, vertex_data_t z) noexcept
			{
				m_triangleNormals[m_triangleVertexIndex][Math::X] = x;
				m_triangleNormals[m_triangleVertexIndex][Math::Y] = y;
				m_triangleNormals[m_triangleVertexIndex][Math::Z] = z;
				m_declaredAttributes[NormalDeclared] = true;
			}

			void
			setTextureCoordinates (const Math::Vector< 3, vertex_data_t > & textureCoordinates) noexcept
			{
				m_triangleTextureCoordinates[m_triangleVertexIndex] = textureCoordinates;
				m_declaredAttributes[TextureCoordinatesDeclared] = true;
			}

			void
			setTextureCoordinates (vertex_data_t u, vertex_data_t v, vertex_data_t w = 0) noexcept
			{
				m_triangleTextureCoordinates[m_triangleVertexIndex][Math::X] = u;
				m_triangleTextureCoordinates[m_triangleVertexIndex][Math::Y] = v;
				m_triangleTextureCoordinates[m_triangleVertexIndex][Math::Z] = w;
				m_declaredAttributes[TextureCoordinatesDeclared] = true;
			}

			void
			setVertexColor (const Math::Vector< 4, vertex_data_t > & vertexColor) noexcept
			{
				m_triangleVertexColors[m_triangleVertexIndex] = vertexColor;
				m_declaredAttributes[VertexColorDeclared] = true;
			}

			void
			setVertexColor (const PixelFactory::Color< vertex_data_t > & vertexColor) noexcept
			{
				m_triangleVertexColors[m_triangleVertexIndex][Math::X] = vertexColor.red();
				m_triangleVertexColors[m_triangleVertexIndex][Math::Y] = vertexColor.green();
				m_triangleVertexColors[m_triangleVertexIndex][Math::Z] = vertexColor.blue();
				m_triangleVertexColors[m_triangleVertexIndex][Math::W] = vertexColor.alpha();
				m_declaredAttributes[VertexColorDeclared] = true;
			}

			void
			setVertexColor (vertex_data_t red, vertex_data_t green, vertex_data_t blue, vertex_data_t alpha = 1) noexcept
			{
				m_triangleVertexColors[m_triangleVertexIndex][Math::X] = red;
				m_triangleVertexColors[m_triangleVertexIndex][Math::Y] = green;
				m_triangleVertexColors[m_triangleVertexIndex][Math::Z] = blue;
				m_triangleVertexColors[m_triangleVertexIndex][Math::W] = alpha;
				m_declaredAttributes[VertexColorDeclared] = true;
			}

			void
			setInfluences (const Math::Vector< 4, int32_t > & /*influences*/) noexcept
			{
				// TODO: Create a vector to store influences
				//m_triangleVertexColors[m_triangleVertexIndex] = influences;
				m_declaredAttributes[InfluencesDeclared] = true;
			}

			void
			setWeights (const Math::Vector< 4, vertex_data_t > & /*weights*/) noexcept
			{
				// TODO: Create a vector to store weights
				//m_triangleVertexColors[m_triangleVertexIndex] = weights;
				m_declaredAttributes[WeightsDeclared] = true;
			}

			bool
			newVertex () noexcept
			{
				if ( m_constructionMode == ConstructionMode::None )
				{
					std::cerr << "[" << __PRETTY_FUNCTION__ << "] There is no construction !" "\n";

					return false;
				}

				if ( !m_declaredAttributes[PositionDeclared] )
				{
					std::cerr << "[" << __PRETTY_FUNCTION__ << "] The position attribute is not declared for this vertex !" "\n";

					return false;
				}

				if ( m_options.isNormalsEnabled() && !this->checkNormal() )
				{
					std::cerr << "[" << __PRETTY_FUNCTION__ << "] The normal attribute is not declared for this vertex !" "\n";

					return false;
				}

				if ( m_options.isTextureCoordinatesEnabled() && !this->checkTextureCoordinates() )
				{
					std::cerr << "[" << __PRETTY_FUNCTION__ << "] The texture coordinates attribute is not declared for this vertex !" "\n";

					return false;
				}

				if ( m_options.isVertexColorsEnabled() && !this->checkVertexColor() )
				{
					std::cerr << "[" << __PRETTY_FUNCTION__ << "] The vertex color attribute is not declared for this vertex !" "\n";

					return false;
				}

				this->newVertexAdded();

				return true;
			}

			/**
			 * @brief Ends the construction of the shape.
			 * @return void
			 */
			void
			endConstruction () noexcept
			{
				if ( m_constructionMode == ConstructionMode::None )
				{
					std::cerr << "[" << __PRETTY_FUNCTION__ << "] There was no construction to end !" "\n";

					return;
				}

				/* Move the whole geometry above Y- axis, if requested. */
				if ( m_options.isCenteredAtBottom()  )
				{
					m_destinationShape->setCenterAtBottom(false);
				}

				/* NOTE: Should never happen, but we absolutely
				 * need valid texture coordinates. */
				if ( m_triangleTextureCoordinates.empty() )
				{
					const auto & textureCoordinatesMultiplier = m_options.textureCoordinatesMultiplier();

					if constexpr ( VertexFactoryDebugEnabled )
					{
						std::cout << "[DEBUG:VERTEX_FACTORY] Generating texture coordinates !" "\n";
					}

					m_destinationShape->generateTextureCoordinates(textureCoordinatesMultiplier[Math::X], textureCoordinatesMultiplier[Math::Y]);
				}

				/* Compute normal and tangent of every triangle.
				 * NOTE: If no normals were provided by vertex, we will compute it from triangle.
				 * Otherwise, we compute only the tangent vector for every vertex. */
				if ( m_options.isNormalsGenerationEnabled() )
				{
					/* NOTE: To work properly, the shape should have texture coordinates. */
					m_destinationShape->computeTriangleTBNSpace();

					m_destinationShape->computeVertexTBNSpace();
				}
				else
				{
					/* NOTE: To work properly, the shape should have texture coordinates. */
					m_destinationShape->computeTriangleTangent();

					m_destinationShape->computeVertexTangent();
				}

				/* NOTE: If asked with generation parameters, we flip the whole geometry. */
				if ( m_options.isGeometryFlippingEnabled() )
				{
					if constexpr ( VertexFactoryDebugEnabled )
					{
						std::cout << "[DEBUG:VERTEX_FACTORY] Flipping the shape !" "\n";
					}

					m_destinationShape->flipSurface();
				}

				/* This will processLogics the bounding box, sphere and centroid. */
				m_destinationShape->updateProperties();

				/* We reset construction variable. */
				{
					this->resetCurrentTriangle();

					m_constructionMode = ConstructionMode::None;
				}

				if constexpr ( VertexFactoryDebugEnabled )
				{
					std::cout << "[DEBUG:VERTEX_FACTORY] Construction finished !" "\n";
				}
			}

			/**
			 * @brief Returns a list of distances between vertices.
			 * @param vertices A list of vertex.
			 * @return std::vector< vertex_data_t >
			 */
			[[nodiscard]]
			static
			std::vector< vertex_data_t >
			getDistances (const std::vector< std::vector< ShapeVertex< vertex_data_t > > > & vertices) noexcept
			{
				if ( vertices.empty() )
				{
					return {};
				}

				const auto verticesCount = vertices.size();

				/* Compute vector space. */
				index_data_t space = 0;
				auto count = verticesCount - 1;

				for ( index_data_t index = 0; index < verticesCount; ++index )
				{
					space += count--;
				}

				/* Find every distance between every vertex. */
				std::vector< vertex_data_t > distances;
				distances.reserve(space);

				for ( index_data_t index = 0; index < verticesCount - 1; ++index )
				{
					for ( index_data_t subIndex = index + 1; subIndex < verticesCount; ++subIndex )
					{
						distances.emplace_back(Math::Vector< 3, vertex_data_t >::distance(vertices[index].position(), vertices[subIndex].position()));
					}
				}

				return distances;
			}

			/**
			 * @brief Returns minimum, maximum and the average distances between vertices.
			 * @param vertices A list of vertex.
			 * @return std::array< vertex_data_t, 3 >
			 */
			[[nodiscard]]
			static
			std::array< vertex_data_t, 3 >
			getDistancesAggregate (const std::vector< std::vector< ShapeVertex< vertex_data_t > > > & vertices) noexcept
			{
				const auto distances = ShapeBuilder::getDistances(vertices);

				return {
					*std::min_element(distances.cbegin(), distances.cend()),
					*std::max_element(distances.cbegin(), distances.cend()),
					std::accumulate(distances.cbegin(), distances.cend(), 0.0F) / distances.size()
				};
			}

			/**
			 * @brief Resets the current triangle.
			 * @return void
			 */
			void
			resetCurrentTriangle () noexcept
			{
				m_triangleVertexIndex = 0;
				m_triangleCount = 0;
				m_declaredAttributes.fill(false);
			}

			/**
			 * @brief Resets the construction.
			 * @return void
			 */
			void
			reset () noexcept
			{
				if ( m_constructionMode != ConstructionMode::None )
				{
					this->endConstruction();

					return;
				}

				/* Reset building data. */
				m_destinationShape = nullptr;

				this->resetCurrentTriangle();

				/* Reset building parameters. */
				m_constructionMode = ConstructionMode::None;
				m_options.reset();
			}

		private:

			/**
			 * @brief Check the normal attribute for one vertex.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			checkNormal () noexcept
			{
				if ( m_options.isGlobalNormalEnabled() )
				{
					m_triangleNormals[m_triangleVertexIndex] = m_options.globalNormal();

					return true;
				}

				if ( m_options.isNormalsGenerationEnabled() )
				{
					return false;
				}

				return m_declaredAttributes[NormalDeclared];
			}

			/**
			 * @brief Check the texture coordinates attribute for one vertex.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			checkTextureCoordinates () noexcept
			{
				if ( m_options.isTextureCoordinatesGenerationEnabled() )
				{
					m_triangleTextureCoordinates[m_triangleVertexIndex] = TextureCoordinates::generateCubicCoordinates(
						m_triangleVertices[m_triangleVertexIndex],
						m_options.isNormalsEnabled() ? m_triangleNormals[m_triangleVertexIndex] : Math::Vector< 3, vertex_data_t >::positiveZ()
					);

					return true;
				}

				return m_declaredAttributes[TextureCoordinatesDeclared];
			}

			/**
			 * @brief Check the vertex color attribute for one vertex.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			checkVertexColor () noexcept
			{
				if ( m_options.isGlobalVertexColorEnabled() )
				{
					m_triangleVertexColors[m_triangleVertexIndex] = m_options.globalVertexColor();

					return true;
				}

				if ( m_options.isVertexColorsGenerationEnabled() )
				{
					m_triangleVertexColors[m_triangleVertexIndex] = Math::Vector< 4, vertex_data_t >{m_triangleVertices[m_triangleVertexIndex].normalized(), 1};

					return true;
				}

				return m_declaredAttributes[VertexColorDeclared];
			}

			/**
			 * @brief Adds a new vertex into the incoming triangle.
			 * @return void
			 */
			void
			newVertexAdded () noexcept
			{
				++m_triangleVertexIndex;
				m_declaredAttributes.fill(false);

				/* NOTE: If we got 3 vertices, we can create a triangle. */
				if ( m_triangleVertexIndex >= 3 )
				{
					/* Creates the triangle in the shape. */
					this->createTriangle();

					/* Prepare the next triangle following the mode of construction. */
					switch ( m_constructionMode )
					{
						case ConstructionMode::Triangles :
							m_triangleVertexIndex = 0;
							break;

						case ConstructionMode::TriangleStrip :
							m_triangleVertexIndex = 2;

							if ( m_triangleCount % 2 > 0 )
							{
								m_triangleVertices[0] = m_triangleVertices[2];
								m_triangleTextureCoordinates[0] = m_triangleTextureCoordinates[2];
								m_triangleVertexColors[0] = m_triangleVertexColors[2];
								m_triangleNormals[0] = m_triangleNormals[2];
							}
							else
							{
								m_triangleVertices[1] = m_triangleVertices[2];
								m_triangleTextureCoordinates[1] = m_triangleTextureCoordinates[2];
								m_triangleVertexColors[1] = m_triangleVertexColors[2];
								m_triangleNormals[1] = m_triangleNormals[2];
							}
							break;

						case ConstructionMode::TriangleFan :
							m_triangleVertexIndex = 2;

							/* FIXME: Check this. */
							m_triangleVertices[1] = m_triangleVertices[2];
							m_triangleTextureCoordinates[1] = m_triangleTextureCoordinates[2];
							m_triangleVertexColors[1] = m_triangleVertexColors[2];
							m_triangleNormals[1] = m_triangleNormals[2];
							break;

						default:
							std::cerr << __PRETTY_FUNCTION__ << ", invalid usage !" "\n";
							break;
					}
				}
			}

			/**
			 * @brief Creates a triangle when sufficient data are available to build one.
			 * @return void
			 */
			void
			createTriangle () noexcept
			{
				ShapeTriangle< vertex_data_t > triangle{};

				const auto & textureCoordinatesMultiplier = m_options.textureCoordinatesMultiplier();

				for ( index_data_t vertexIndex = 0; vertexIndex < 3; ++vertexIndex )
				{
					index_data_t attributeIndex = 0;

					/* Vertex position, normal and texCoords. */
					{
						const auto textureCoordinates = Math::Vector< 3, vertex_data_t >(
							m_triangleTextureCoordinates[vertexIndex][Math::X] * textureCoordinatesMultiplier[Math::X],
							m_triangleTextureCoordinates[vertexIndex][Math::Y] * textureCoordinatesMultiplier[Math::Y],
							m_triangleTextureCoordinates[vertexIndex][Math::Z] * textureCoordinatesMultiplier[Math::Z]
						);

						if ( m_options.dataEconomyEnabled() )
						{
							attributeIndex = m_destinationShape->addVertex(m_triangleVertices[vertexIndex], m_triangleNormals[vertexIndex], textureCoordinates);
						}
						else
						{
							attributeIndex = m_destinationShape->saveVertex(m_triangleVertices[vertexIndex], m_triangleNormals[vertexIndex], textureCoordinates);
						}

						triangle.setVertexIndex(vertexIndex, attributeIndex);
					}

					/* Vertex color. */
					{
						if ( m_options.dataEconomyEnabled() )
						{
							attributeIndex = m_destinationShape->addVertexColor(m_triangleVertexColors[vertexIndex]);
						}
						else
						{
							attributeIndex = m_destinationShape->saveVertexColor(m_triangleVertexColors[vertexIndex]);
						}

						triangle.setVertexColorIndex(vertexIndex, attributeIndex);
					}
				}

				m_destinationShape->addTriangle(triangle);

				++m_triangleCount;
			}

			static constexpr auto PositionDeclared{0UL};
			static constexpr auto NormalDeclared{1UL};
			static constexpr auto TextureCoordinatesDeclared{2UL};
			static constexpr auto VertexColorDeclared{3UL};
			static constexpr auto InfluencesDeclared{4UL};
			static constexpr auto WeightsDeclared{5UL};

			Shape< vertex_data_t, index_data_t > * m_destinationShape{nullptr};
			std::array< Math::Vector< 3, vertex_data_t >, 3 > m_triangleVertices{};
			std::array< Math::Vector< 3, vertex_data_t >, 3 > m_triangleNormals{};
			std::array< Math::Vector< 3, vertex_data_t >, 3 > m_triangleTextureCoordinates{};
			std::array< Math::Vector< 4, vertex_data_t >, 3 > m_triangleVertexColors{};
			index_data_t m_triangleVertexIndex{0};
			index_data_t m_triangleCount{0};
			ShapeBuilderOptions< vertex_data_t > m_options;
			ConstructionMode m_constructionMode{ConstructionMode::None};
			std::array< bool, 8 > m_declaredAttributes{
				false/*PositionDeclared*/,
				false/*NormalDeclared*/,
				false/*TextureCoordinatesDeclared*/,
				false/*VertexColorDeclared*/,
				false/*InfluencesDeclared*/,
				false/*WeightsDeclared*/,
				false/*UNUSED*/,
				false/*UNUSED*/
			};
	};
}
