/*
 * src/Libs/VertexFactory/ShapeProcessor.hpp
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

/* STL inclusions. */
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <list>
#include <numbers>
#include <numeric>
#include <queue>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <vector>

/* Local inclusions. */
#include "Libs/Algorithms/DelaunayTriangulation.hpp"
#include "Shape.hpp"

namespace EmEn::Libs::VertexFactory
{
	/**
	 * @brief Provides geometry processing operations on a Shape (hole detection, hole filling, etc.).
	 * @note Modifies the shape in place.
	 * @tparam vertex_data_t The precision type of vertex data. Default float.
	 * @tparam index_data_t The precision type of index data. Default uint32_t.
	 */
	template< typename vertex_data_t = float, typename index_data_t = uint32_t >
	requires (std::is_floating_point_v< vertex_data_t > && std::is_unsigned_v< index_data_t >)
	class ShapeProcessor final
	{
		public:

			/**
			 * @brief Constructs a processor for the given shape.
			 * @param shape A mutable reference to the shape to process. Must remain valid for the lifetime of this object.
			 * @param positionTolerance Tolerance for considering two vertex positions as identical. Default 1e-4.
			 */
			explicit
			ShapeProcessor (Shape< vertex_data_t, index_data_t > & shape, vertex_data_t positionTolerance = static_cast< vertex_data_t >(1e-4)) noexcept
				: m_shape(shape), m_positionTolerance(positionTolerance)
			{

			}

			/** @brief Copy constructor. */
			ShapeProcessor (const ShapeProcessor &) noexcept = delete;

			/** @brief Move constructor. */
			ShapeProcessor (ShapeProcessor &&) noexcept = delete;

			/** @brief Copy assignment. */
			ShapeProcessor & operator= (const ShapeProcessor &) noexcept = delete;

			/** @brief Move assignment. */
			ShapeProcessor & operator= (ShapeProcessor &&) noexcept = delete;

			/** @brief Destructor. */
			~ShapeProcessor () = default;

			/**
			 * @brief Detects whether the shape has any holes (boundary loops).
			 * @return bool
			 */
			[[nodiscard]]
			bool
			hasBoundaryLoops () const noexcept
			{
				return !this->findBoundaryLoops().empty();
			}

			/**
			 * @brief Deduplicates vertices that share identical attributes.
			 * @note By default, compares position, normal and texture coordinates (preserves
			 * hard edges and UV seams). Set flags to false for more aggressive merging.
			 * @param keepNormals If true, vertices with different normals are kept separate. Default true.
			 * @param keepTextureCoordinates If true, vertices with different UVs are kept separate. Default true.
			 * @return size_t The number of vertices removed.
			 */
			size_t
			deduplicateVertices (bool keepNormals = true, bool keepTextureCoordinates = true) noexcept
			{
				if ( m_shape.empty() )
				{
					return 0;
				}

				const auto & oldVertices = m_shape.vertices();
				const auto oldCount = static_cast< index_data_t >(oldVertices.size());

				/* Build a hash map: quantized attributes → new compact index.
				 * Attributes included in the key depend on the keep flags. */
				struct VertexKey
				{
					int64_t px, py, pz;
					int64_t nx, ny, nz;
					int64_t tx, ty, tz;

					bool operator== (const VertexKey & other) const noexcept = default;
				};

				struct VertexKeyHash
				{
					size_t operator() (const VertexKey & k) const noexcept
					{
						size_t h = 0;
						h ^= std::hash< int64_t >{}(k.px) + 0x9e3779b9 + (h << 6) + (h >> 2);
						h ^= std::hash< int64_t >{}(k.py) + 0x9e3779b9 + (h << 6) + (h >> 2);
						h ^= std::hash< int64_t >{}(k.pz) + 0x9e3779b9 + (h << 6) + (h >> 2);
						h ^= std::hash< int64_t >{}(k.nx) + 0x9e3779b9 + (h << 6) + (h >> 2);
						h ^= std::hash< int64_t >{}(k.ny) + 0x9e3779b9 + (h << 6) + (h >> 2);
						h ^= std::hash< int64_t >{}(k.nz) + 0x9e3779b9 + (h << 6) + (h >> 2);
						h ^= std::hash< int64_t >{}(k.tx) + 0x9e3779b9 + (h << 6) + (h >> 2);
						h ^= std::hash< int64_t >{}(k.ty) + 0x9e3779b9 + (h << 6) + (h >> 2);
						h ^= std::hash< int64_t >{}(k.tz) + 0x9e3779b9 + (h << 6) + (h >> 2);

						return h;
					}
				};

				const auto scale = static_cast< vertex_data_t >(1) / m_positionTolerance;

				auto quantizeVec = [scale] (const Math::Vector< 3, vertex_data_t > & v) -> std::array< int64_t, 3 >
				{
					return {
						static_cast< int64_t >(std::round(v[Math::X] * scale)),
						static_cast< int64_t >(std::round(v[Math::Y] * scale)),
						static_cast< int64_t >(std::round(v[Math::Z] * scale))
					};
				};

				constexpr std::array< int64_t, 3 > zero{0, 0, 0};

				std::unordered_map< VertexKey, index_data_t, VertexKeyHash > uniqueMap;
				std::vector< index_data_t > remapping(oldCount);
				std::vector< ShapeVertex< vertex_data_t > > newVertices;

				newVertices.reserve(oldCount);

				for ( index_data_t i = 0; i < oldCount; ++i )
				{
					const auto & vert = oldVertices[i];

					const auto p = quantizeVec(vert.position());
					const auto n = keepNormals ? quantizeVec(vert.normal()) : zero;
					const auto t = keepTextureCoordinates ? quantizeVec(vert.textureCoordinates()) : zero;

					const VertexKey key{p[0], p[1], p[2], n[0], n[1], n[2], t[0], t[1], t[2]};

					auto [it, inserted] = uniqueMap.try_emplace(key, static_cast< index_data_t >(newVertices.size()));

					if ( inserted )
					{
						newVertices.push_back(vert);
					}

					remapping[i] = it->second;
				}

				const auto removed = oldCount - static_cast< index_data_t >(newVertices.size());

				if ( removed == 0 )
				{
					return 0;
				}

				/* Replace the vertex array. */
				m_shape.vertices() = std::move(newVertices);

				/* Remap all triangle vertex indices. */
				for ( auto & tri : m_shape.triangles() )
				{
					for ( index_data_t v = 0; v < 3; ++v )
					{
						tri.setVertexIndex(v, remapping[tri.vertexIndex(v)]);
					}
				}

				return static_cast< size_t >(removed);
			}

			/**
			 * @brief Finds all boundary loops (holes) in the shape.
			 * @return std::vector< BoundaryLoop< index_data_t > >
			 */
			[[nodiscard]]
			std::vector< BoundaryLoop< index_data_t > >
			findBoundaryLoops () const noexcept
			{
				if ( m_shape.empty() )
				{
					return {};
				}

				/* Build canonical position map: quantized position → first vertex index. */
				const auto canonicalMap = this->buildCanonicalMap();

				/* Build edge occurrence map using canonical indices. */
				const auto & triangles = m_shape.triangles();

				/* Key: canonical edge (sorted pair). Value: count and actual vertex indices. */
				struct EdgeInfo
				{
					size_t count{0};
					index_data_t actualA{0};
					index_data_t actualB{0};
				};

				std::unordered_map< uint64_t, EdgeInfo > edgeMap;

				for ( const auto & tri : triangles )
				{
					for ( index_data_t e = 0; e < 3; ++e )
					{
						const auto actualA = tri.vertexIndex(e);
						const auto actualB = tri.vertexIndex((e + 1) % 3);

						const auto canonA = canonicalMap.at(actualA);
						const auto canonB = canonicalMap.at(actualB);

						const auto edgeKey = packEdgeKey(canonA, canonB);

						auto & info = edgeMap[edgeKey];
						++info.count;

						if ( info.count == 1 )
						{
							info.actualA = actualA;
							info.actualB = actualB;
						}
					}
				}

				/* Extract boundary edges (count == 1). */
				/* Adjacency: canonical vertex → list of (next_canonical, actual_vertex_for_this_end). */
				std::unordered_map< index_data_t, std::vector< std::pair< index_data_t, index_data_t > > > adjacency;

				for ( const auto & [key, info] : edgeMap )
				{
					if ( info.count != 1 )
					{
						continue;
					}

					const auto canonA = canonicalMap.at(info.actualA);
					const auto canonB = canonicalMap.at(info.actualB);

					adjacency[canonA].emplace_back(canonB, info.actualA);
					adjacency[canonB].emplace_back(canonA, info.actualB);
				}

				if ( adjacency.empty() )
				{
					return {};
				}

				/* Chain boundary edges into loops using angle-based traversal.
				 * At T-junctions (vertices with 3+ neighbors), pick the neighbor
				 * that continues most straight (smallest turning angle). */
				std::unordered_set< uint64_t > usedEdges;
				std::vector< BoundaryLoop< index_data_t > > loops;

				auto getCanonPos = [this] (index_data_t canonIdx) -> const Math::Vector< 3, vertex_data_t > &
				{
					return m_shape.vertex(canonIdx).position();
				};

				for ( const auto & [startCanonical, neighbors] : adjacency )
				{
					bool hasUnused = false;

					for ( const auto & [neighborCanon, neighborActual] : neighbors )
					{
						if ( !usedEdges.contains(packEdgeKey(startCanonical, neighborCanon)) )
						{
							hasUnused = true;

							break;
						}
					}

					if ( !hasUnused )
					{
						continue;
					}

					BoundaryLoop< index_data_t > loop;

					auto currentCanonical = startCanonical;
					index_data_t prevCanonical = std::numeric_limits< index_data_t >::max();
					const size_t maxSteps = adjacency.size() + 1;
					size_t steps = 0;

					do
					{
						const auto & edges = adjacency.at(currentCanonical);

						index_data_t actualVertex = edges.front().second;
						loop.vertexIndices.push_back(actualVertex);

						index_data_t bestCanonical = std::numeric_limits< index_data_t >::max();

						/* First: prefer closing the loop. */
						if ( loop.vertexIndices.size() >= 3 )
						{
							for ( const auto & [neighborCanon, neighborActual] : edges )
							{
								if ( neighborCanon == startCanonical && !usedEdges.contains(packEdgeKey(currentCanonical, neighborCanon)) )
								{
									bestCanonical = neighborCanon;

									break;
								}
							}
						}

						/* Second: angle-based selection — pick the straightest continuation. */
						if ( bestCanonical == std::numeric_limits< index_data_t >::max() )
						{
							vertex_data_t bestScore = std::numeric_limits< vertex_data_t >::lowest();

							for ( const auto & [neighborCanon, neighborActual] : edges )
							{
								if ( usedEdges.contains(packEdgeKey(currentCanonical, neighborCanon)) )
								{
									continue;
								}

								if ( prevCanonical == std::numeric_limits< index_data_t >::max() )
								{
									bestCanonical = neighborCanon;

									break;
								}

								const auto incoming = (getCanonPos(currentCanonical) - getCanonPos(prevCanonical)).normalized();
								const auto outgoing = (getCanonPos(neighborCanon) - getCanonPos(currentCanonical)).normalized();
								const auto straightness = Math::Vector< 3, vertex_data_t >::dotProduct(incoming, outgoing);

								if ( straightness > bestScore )
								{
									bestScore = straightness;
									bestCanonical = neighborCanon;
								}
							}
						}

						if ( bestCanonical == std::numeric_limits< index_data_t >::max() )
						{
							break;
						}

						usedEdges.insert(packEdgeKey(currentCanonical, bestCanonical));
						prevCanonical = currentCanonical;
						currentCanonical = bestCanonical;
						++steps;

					} while ( currentCanonical != startCanonical && steps < maxSteps );

					if ( loop.vertexIndices.size() >= 3 )
					{
						loops.push_back(std::move(loop));
					}
				}

				/* Merge adjacent loops whose endpoints are close in space. */
				const auto mergeTol = m_positionTolerance * static_cast< vertex_data_t >(100);
				bool merged = true;

				while ( merged && loops.size() > 1 )
				{
					merged = false;

					for ( size_t i = 0; i < loops.size() && !merged; ++i )
					{
						auto & loopI = loops[i].vertexIndices;
						const auto iFront = m_shape.vertex(loopI.front()).position();
						const auto iBack = m_shape.vertex(loopI.back()).position();

						for ( size_t j = i + 1; j < loops.size() && !merged; ++j )
						{
							auto & loopJ = loops[j].vertexIndices;
							const auto jFront = m_shape.vertex(loopJ.front()).position();
							const auto jBack = m_shape.vertex(loopJ.back()).position();

							if ( (iBack - jFront).length() < mergeTol )
							{
								loopI.insert(loopI.end(), loopJ.begin(), loopJ.end());
								loops.erase(loops.begin() + static_cast< ptrdiff_t >(j));
								merged = true;
							}
							else if ( (jBack - iFront).length() < mergeTol )
							{
								loopJ.insert(loopJ.end(), loopI.begin(), loopI.end());
								loopI = std::move(loopJ);
								loops.erase(loops.begin() + static_cast< ptrdiff_t >(j));
								merged = true;
							}
							else if ( (iBack - jBack).length() < mergeTol )
							{
								std::reverse(loopJ.begin(), loopJ.end());
								loopI.insert(loopI.end(), loopJ.begin(), loopJ.end());
								loops.erase(loops.begin() + static_cast< ptrdiff_t >(j));
								merged = true;
							}
							else if ( (iFront - jFront).length() < mergeTol )
							{
								std::reverse(loopJ.begin(), loopJ.end());
								loopJ.insert(loopJ.end(), loopI.begin(), loopI.end());
								loopI = std::move(loopJ);
								loops.erase(loops.begin() + static_cast< ptrdiff_t >(j));
								merged = true;
							}
						}
					}
				}

				return loops;
			}

			/**
			 * @brief Fills a single boundary loop using ear-clipping triangulation.
			 * @note Handles both convex and concave boundary loops correctly by projecting
			 * the 3D loop onto a 2D plane and performing ear-clipping in 2D.
			 * @param loop The boundary loop to fill.
			 * @param expectedNormal Optional expected cap normal direction. When provided, the cap winding
			 * is determined by this vector instead of the heuristic. Pass the cutting plane normal
			 * negated for front parts, or as-is for back parts.
			 * @return bool True if the hole was successfully filled.
			 */
			bool
			sealBoundaryLoop (const BoundaryLoop< index_data_t > & loop, const Math::Vector< 3, vertex_data_t > & expectedNormal = {}, size_t minVertices = 3) noexcept
			{
				if ( loop.vertexIndices.size() < std::max(size_t{3}, minVertices) )
				{
					return false;
				}

				auto indices = loop.vertexIndices;
				const auto vertexCount = indices.size();

				/* Compute the cap normal: use expectedNormal directly if provided,
				 * otherwise fall back to Newell's method. */
				Math::Vector< 3, vertex_data_t > capNormal;
				Math::Vector< 3, vertex_data_t > avgTangent{};

				if ( expectedNormal.lengthSquared() > static_cast< vertex_data_t >(0.01) )
				{
					capNormal = expectedNormal.normalized();
				}
				else
				{
					/* Newell's method fallback. */
					Math::Vector< 3, vertex_data_t > polyNormal{};

					for ( size_t i = 0; i < vertexCount; ++i )
					{
						const auto & posA = m_shape.vertex(indices[i]).position();
						const auto & posB = m_shape.vertex(indices[(i + 1) % vertexCount]).position();

						polyNormal[Math::X] = polyNormal[Math::X] + (posA[Math::Y] - posB[Math::Y]) * (posA[Math::Z] + posB[Math::Z]);
						polyNormal[Math::Y] = polyNormal[Math::Y] + (posA[Math::Z] - posB[Math::Z]) * (posA[Math::X] + posB[Math::X]);
						polyNormal[Math::Z] = polyNormal[Math::Z] + (posA[Math::X] - posB[Math::X]) * (posA[Math::Y] + posB[Math::Y]);
					}

					capNormal = polyNormal.normalized();
				}

				for ( size_t i = 0; i < vertexCount; ++i )
				{
					avgTangent = avgTangent + m_shape.vertex(indices[i]).tangent();
				}

				avgTangent = avgTangent.normalized();

				/* Build a 2D projection basis from the cap normal. */
				const auto [axisU, axisV] = buildOrthonormalBasis(capNormal);

				/* Project all boundary vertices to 2D. */
				std::vector< Math::Vector< 2, vertex_data_t > > projected(vertexCount);

				for ( size_t i = 0; i < vertexCount; ++i )
				{
					const auto & pos = m_shape.vertex(indices[i]).position();

					projected[i] = {
						Math::Vector< 3, vertex_data_t >::dotProduct(pos, axisU),
						Math::Vector< 3, vertex_data_t >::dotProduct(pos, axisV)
					};
				}

				/* Ensure CCW winding for ear-clipping. */
				const auto signedArea = computeSignedArea2D(projected);
				const bool needsReverse = signedArea < 0;

				std::vector< size_t > order(vertexCount);

				if ( needsReverse )
				{
					for ( size_t i = 0; i < vertexCount; ++i )
					{
						order[i] = vertexCount - 1 - i;
					}
				}
				else
				{
					std::iota(order.begin(), order.end(), size_t{0});
				}

				/* Optimized ear-clipping: prev/next linked list + reflex/ear classification. */
				std::vector< size_t > prevIdx(vertexCount);
				std::vector< size_t > nextIdx(vertexCount);

				for ( size_t i = 0; i < vertexCount; ++i )
				{
					prevIdx[i] = (i + vertexCount - 1) % vertexCount;
					nextIdx[i] = (i + 1) % vertexCount;
				}

				std::vector< bool > isReflex(vertexCount, false);
				std::vector< bool > isEar(vertexCount, false);
				std::vector< bool > isActive(vertexCount, true);

				auto classifyVertex = [&] (size_t i)
				{
					const auto & pA = projected[order[prevIdx[i]]];
					const auto & pB = projected[order[i]];
					const auto & pC = projected[order[nextIdx[i]]];

					isReflex[i] = cross2D(pA, pB, pC) <= 0;
				};

				auto classifyEar = [&] (size_t i)
				{
					if ( isReflex[i] )
					{
						isEar[i] = false;

						return;
					}

					const auto & pA = projected[order[prevIdx[i]]];
					const auto & pB = projected[order[i]];
					const auto & pC = projected[order[nextIdx[i]]];

					size_t cursor = nextIdx[nextIdx[i]];

					while ( cursor != prevIdx[i] )
					{
						if ( isReflex[cursor] && isPointInTriangle2D(projected[order[cursor]], pA, pB, pC) )
						{
							isEar[i] = false;

							return;
						}

						cursor = nextIdx[cursor];
					}

					isEar[i] = true;
				};

				for ( size_t i = 0; i < vertexCount; ++i )
				{
					classifyVertex(i);
				}

				for ( size_t i = 0; i < vertexCount; ++i )
				{
					classifyEar(i);
				}

				std::vector< std::array< size_t, 3 > > earTriangles;
				earTriangles.reserve(vertexCount - 2);

				size_t remaining = vertexCount;

				while ( remaining > 2 )
				{
					size_t earIdx = vertexCount;

					for ( size_t i = 0; i < vertexCount; ++i )
					{
						if ( isActive[i] && isEar[i] )
						{
							earIdx = i;

							break;
						}
					}

					if ( earIdx == vertexCount )
					{
						break;
					}

					const auto p = prevIdx[earIdx];
					const auto n = nextIdx[earIdx];

					earTriangles.push_back({order[p], order[earIdx], order[n]});

					isActive[earIdx] = false;
					nextIdx[p] = n;
					prevIdx[n] = p;
					--remaining;

					classifyVertex(p);
					classifyEar(p);
					classifyVertex(n);
					classifyEar(n);
				}

				if ( earTriangles.empty() )
				{
					return false;
				}

				/* Create NEW cap vertices with correct normal and planar UV projection.
				 * Reusing boundary vertices would cause lighting artifacts due to
				 * mismatched normals at the seam. */

				/* Compute 2D bounding box for UV normalization. */
				auto uvMin = projected[0];
				auto uvMax = projected[0];

				for ( size_t i = 1; i < vertexCount; ++i )
				{
					uvMin[Math::X] = std::min(uvMin[Math::X], projected[i][Math::X]);
					uvMin[Math::Y] = std::min(uvMin[Math::Y], projected[i][Math::Y]);
					uvMax[Math::X] = std::max(uvMax[Math::X], projected[i][Math::X]);
					uvMax[Math::Y] = std::max(uvMax[Math::Y], projected[i][Math::Y]);
				}

				const auto uvRangeX = uvMax[Math::X] - uvMin[Math::X];
				const auto uvRangeY = uvMax[Math::Y] - uvMin[Math::Y];
				const auto uvScale = std::max(uvRangeX, uvRangeY);
				const auto invUvScale = (uvScale > static_cast< vertex_data_t >(1e-6))
					? static_cast< vertex_data_t >(1) / uvScale
					: static_cast< vertex_data_t >(1);

				/* Create cap vertices: one new vertex per boundary vertex, with capNormal and projected UVs. */
				std::vector< index_data_t > capVertexMap(vertexCount);

				for ( size_t i = 0; i < vertexCount; ++i )
				{
					const auto & srcPos = m_shape.vertex(indices[i]).position();

					const Math::Vector< 3, vertex_data_t > uvCoords{
						(projected[i][Math::X] - uvMin[Math::X]) * invUvScale,
						(projected[i][Math::Y] - uvMin[Math::Y]) * invUvScale,
						static_cast< vertex_data_t >(0)
					};

					capVertexMap[i] = m_shape.saveVertex(srcPos, capNormal, uvCoords);
					m_shape.vertices()[capVertexMap[i]].setTangent(axisU);
				}

				/* Emit the ear-clipped triangles using the new cap vertices.
				 * Reverse vertex order (CBA) so that the face normal aligns with capNormal
				 * under Vulkan's CCW front-face convention. */
				for ( const auto & [localA, localB, localC] : earTriangles )
				{
					const auto vA = capVertexMap[localA];
					const auto vB = capVertexMap[localB];
					const auto vC = capVertexMap[localC];

					const auto cA = m_shape.saveVertexColor({});
					const auto cB = m_shape.saveVertexColor({});
					const auto cC = m_shape.saveVertexColor({});

					ShapeTriangle< vertex_data_t, index_data_t > triangle(vC, vB, vA);
					triangle.setVertexColorIndex(0, cC);
					triangle.setVertexColorIndex(1, cB);
					triangle.setVertexColorIndex(2, cA);
					triangle.setSurfaceNormal(capNormal);
					triangle.setSurfaceTangent(axisU);

					m_shape.triangles().emplace_back(triangle);

					auto & groups = m_shape.groups();

					if ( groups.empty() )
					{
						groups.emplace_back(0, 1);
					}
					else
					{
						++groups.back().second;
					}
				}

				m_shape.updateProperties();

				return true;
			}

			/**
			 * @brief Finds and fills all holes in the shape.
			 * @param expectedNormal Optional expected cap normal direction for all holes.
			 * @return size_t The number of holes filled.
			 */
			size_t
			sealAllBoundaryLoops (const Math::Vector< 3, vertex_data_t > & expectedNormal = {}) noexcept
			{
				std::vector< BoundaryLoop< index_data_t > > loops;

				if ( !m_shape.boundaryLoops().empty() )
				{
					/* Use pre-computed boundary loops (e.g. from ShapeSplitter). */
					loops = m_shape.boundaryLoops();
				}
				else
				{
					/* Fallback: analyze the shape to find boundary loops. */
					loops = this->findBoundaryLoops();

					if ( loops.size() > 1 )
					{
						loops = this->mergeFragmentedLoops(loops);
					}
				}

				size_t sealed = 0;

				for ( size_t i = 0; i < loops.size(); ++i )
				{
					if ( this->sealBoundaryLoop(loops[i], expectedNormal) )
					{
						++sealed;
					}
				}

				return sealed;
			}

			/**
			 * @brief Generates lightmap-style UV coordinates with uniform pixel density.
			 * @note Each triangle is individually flattened and packed into [0,1]² UV space.
			 * UV area is proportional to 3D surface area, giving uniform texel density.
			 * Ideal for normal map baking. Produces many seams (one per triangle).
			 * @param padding Padding between triangles in UV space (fraction of total). Default 0.002.
			 * @return bool True if UV generation succeeded.
			 */
			bool
			generateLightmapUV (vertex_data_t padding = static_cast< vertex_data_t >(0.002)) noexcept
			{
				if ( m_shape.empty() )
				{
					return false;
				}

				auto & triangles = m_shape.triangles();
				auto & vertices = m_shape.vertices();
				const auto triCount = triangles.size();

				/* Step 1: Flatten each triangle to 2D and compute its bounding box. */
				struct FlatTriangle
				{
					Math::Vector< 2, vertex_data_t > uv[3];
					vertex_data_t width{0};
					vertex_data_t height{0};
					size_t triIndex{0};
				};

				std::vector< FlatTriangle > flatTris;
				flatTris.reserve(triCount);

				for ( size_t t = 0; t < triCount; ++t )
				{
					const auto & tri = triangles[t];

					const auto & p0 = vertices[tri.vertexIndex(0)].position();
					const auto & p1 = vertices[tri.vertexIndex(1)].position();
					const auto & p2 = vertices[tri.vertexIndex(2)].position();

					const auto e1 = p1 - p0;
					const auto e2 = p2 - p0;

					const auto s1 = e1.length();

					if ( s1 < static_cast< vertex_data_t >(1e-10) )
					{
						continue;
					}

					const auto s2 = Math::Vector< 3, vertex_data_t >::dotProduct(e2, e1) / s1;
					const auto crossVec = Math::Vector< 3, vertex_data_t >::crossProduct(e1, e2);
					const auto t2 = crossVec.length() / s1;

					if ( t2 < static_cast< vertex_data_t >(1e-10) )
					{
						continue;
					}

					/* 2D coordinates: q0=(0,0), q1=(s1,0), q2=(s2,t2) */
					FlatTriangle flat;
					flat.uv[0] = {0, 0};
					flat.uv[1] = {s1, 0};
					flat.uv[2] = {s2, t2};
					flat.triIndex = t;

					/* Normalize to bounding box origin. */
					auto minU = std::min({flat.uv[0][Math::X], flat.uv[1][Math::X], flat.uv[2][Math::X]});
					auto minV = std::min({flat.uv[0][Math::Y], flat.uv[1][Math::Y], flat.uv[2][Math::Y]});

					for ( int i = 0; i < 3; ++i )
					{
						flat.uv[i][Math::X] = flat.uv[i][Math::X] - minU;
						flat.uv[i][Math::Y] = flat.uv[i][Math::Y] - minV;
					}

					flat.width = std::max({flat.uv[0][Math::X], flat.uv[1][Math::X], flat.uv[2][Math::X]});
					flat.height = std::max({flat.uv[0][Math::Y], flat.uv[1][Math::Y], flat.uv[2][Math::Y]});

					flatTris.push_back(flat);
				}

				if ( flatTris.empty() )
				{
					return false;
				}

				/* Step 2: Sort by decreasing height for shelf packing. */
				std::sort(flatTris.begin(), flatTris.end(), [] (const auto & a, const auto & b)
				{
					return a.height > b.height;
				});

				/* Step 3: Shelf packing. Estimate bin width from total bounding area. */
				vertex_data_t totalArea = 0;

				for ( const auto & flat : flatTris )
				{
					totalArea += (flat.width + padding) * (flat.height + padding);
				}

				const auto binWidth = std::sqrt(totalArea) * static_cast< vertex_data_t >(1.3);

				vertex_data_t currentX = 0;
				vertex_data_t shelfY = 0;
				vertex_data_t shelfHeight = 0;
				vertex_data_t maxWidth = 0;

				std::vector< Math::Vector< 2, vertex_data_t > > offsets(flatTris.size());

				for ( size_t i = 0; i < flatTris.size(); ++i )
				{
					const auto & flat = flatTris[i];

					if ( currentX + flat.width + padding > binWidth && currentX > 0 )
					{
						shelfY += shelfHeight + padding;
						shelfHeight = 0;
						currentX = 0;
					}

					offsets[i] = {currentX, shelfY};
					currentX += flat.width + padding;
					shelfHeight = std::max(shelfHeight, flat.height);
					maxWidth = std::max(maxWidth, currentX);
				}

				const auto totalHeight = shelfY + shelfHeight;
				const auto scale = static_cast< vertex_data_t >(1) / std::max(maxWidth, totalHeight);

				/* Step 4: Duplicate vertices so each triangle has exclusive vertices,
				 * then assign the packed UV coordinates. */
				for ( size_t i = 0; i < flatTris.size(); ++i )
				{
					const auto & flat = flatTris[i];
					const auto & offset = offsets[i];
					auto & tri = triangles[flat.triIndex];

					for ( int v = 0; v < 3; ++v )
					{
						const auto oldIdx = tri.vertexIndex(v);
						const auto & srcVertex = vertices[oldIdx];

						/* Create a new vertex with the lightmap UV. */
						const Math::Vector< 2, vertex_data_t > uv{
							(flat.uv[v][Math::X] + offset[Math::X]) * scale,
							(flat.uv[v][Math::Y] + offset[Math::Y]) * scale
						};

						const auto newIdx = m_shape.saveVertex(srcVertex.position(), srcVertex.normal(), {uv[Math::X], uv[Math::Y], static_cast< vertex_data_t >(0)});
						m_shape.vertices()[newIdx].setTangent(srcVertex.tangent());

						tri.setVertexIndex(v, newIdx);
					}
				}

				/* Recompute tangent space from the new UV layout. */
				m_shape.computeTriangleTangent();
				m_shape.computeVertexTangent();

				/* Smooth vertex normals and tangents by position: after duplication, each triangle
				 * has exclusive vertices (no sharing). Average normals/tangents of vertices at the
				 * same 3D position to restore smooth interpolation across triangle boundaries. */
				smoothVertexAttributesByPosition();

				m_shape.declareTextureCoordinatesAvailable();
				m_shape.updateProperties();

				return true;
			}

			/**
			 * @brief Generates UV coordinates using Least Squares Conformal Maps (LSCM).
			 * @note Segments the mesh into charts by normal discontinuity, flattens each chart
			 * with LSCM, and packs them into [0,1]² UV space.
			 * @param seamAngleThresholdDegrees The angle threshold for seam detection. Default 66°.
			 * @return bool True if UV generation succeeded.
			 */
			bool
			generateUVUnwrap (vertex_data_t seamAngleThresholdDegrees = static_cast< vertex_data_t >(66)) noexcept
			{
				if ( m_shape.empty() )
				{
					return false;
				}

				const auto seamThreshold = seamAngleThresholdDegrees * std::numbers::pi_v< vertex_data_t > / static_cast< vertex_data_t >(180);

				/* Phase 1: Segment into charts. */
				auto charts = segmentCharts(seamThreshold);

				if ( charts.empty() )
				{
					return false;
				}

				/* Duplicate vertices shared across charts so each chart has exclusive vertices. */
				duplicateSharedVertices(charts);

				/* Phase 2: LSCM per chart. */
				std::vector< std::vector< Math::Vector< 2, vertex_data_t > > > allChartUVs;
				allChartUVs.reserve(charts.size());

				for ( const auto & chart : charts )
				{
					if ( chart.vertexIndices.size() < 3 )
					{
						allChartUVs.emplace_back(chart.vertexIndices.size(), Math::Vector< 2, vertex_data_t >{});

						continue;
					}

					const auto [pinnedA, pinnedB] = selectPinnedVertices(chart);

					const auto & posA = m_shape.vertex(chart.vertexIndices[pinnedA]).position();
					const auto & posB = m_shape.vertex(chart.vertexIndices[pinnedB]).position();
					const auto dist = (posB - posA).length();

					auto chartUVs = solveChartLSCM(chart, pinnedA, pinnedB,
						{static_cast< vertex_data_t >(0), static_cast< vertex_data_t >(0)},
						{dist, static_cast< vertex_data_t >(0)});

					allChartUVs.push_back(std::move(chartUVs));
				}

				/* Phase 3: Pack charts into [0,1]². */
				packCharts(allChartUVs);

				/* Write back UVs to shape vertices. */
				for ( size_t chartIdx = 0; chartIdx < charts.size(); ++chartIdx )
				{
					const auto & chart = charts[chartIdx];
					const auto & uvs = allChartUVs[chartIdx];

					for ( size_t localIdx = 0; localIdx < chart.vertexIndices.size(); ++localIdx )
					{
						m_shape.vertices()[chart.vertexIndices[localIdx]].setTextureCoordinates(
							Math::Vector< 2, vertex_data_t >{uvs[localIdx][Math::X], uvs[localIdx][Math::Y]});
					}
				}

				/* Recompute tangent space to match the new UV layout. */
				m_shape.computeTriangleTangent();
				m_shape.computeVertexTangent();

				m_shape.declareTextureCoordinatesAvailable();

				return true;
			}

		private:

			/* ---- UV Unwrap types ---- */

			struct SparseTriplet
			{
				size_t row;
				size_t col;
				vertex_data_t value;
			};

			struct ChartData
			{
				std::vector< index_data_t > triangleIndices;
				std::vector< index_data_t > vertexIndices;
				std::unordered_map< index_data_t, size_t > globalToLocal;
			};

			/* ---- Lightmap UV: Position-based attribute smoothing ---- */

			void
			smoothVertexAttributesByPosition () noexcept
			{
				auto & vertices = m_shape.vertices();
				const auto vertCount = vertices.size();

				if ( vertCount == 0 )
				{
					return;
				}

				/* Quantize positions to group vertices at the same location. */
				struct QuantizedPos
				{
					int32_t x, y, z;

					bool operator== (const QuantizedPos & other) const noexcept = default;
				};

				struct QuantizedPosHash
				{
					size_t operator() (const QuantizedPos & p) const noexcept
					{
						auto h = std::hash< int32_t >{}(p.x);
						h ^= std::hash< int32_t >{}(p.y) + 0x9e3779b9 + (h << 6) + (h >> 2);
						h ^= std::hash< int32_t >{}(p.z) + 0x9e3779b9 + (h << 6) + (h >> 2);

						return h;
					}
				};

				const auto scale = static_cast< vertex_data_t >(1) / m_positionTolerance;

				/* Group vertices by position. */
				std::unordered_map< QuantizedPos, std::vector< size_t >, QuantizedPosHash > groups;

				for ( size_t i = 0; i < vertCount; ++i )
				{
					const auto & pos = vertices[i].position();
					const QuantizedPos key{
						static_cast< int32_t >(std::round(pos[Math::X] * scale)),
						static_cast< int32_t >(std::round(pos[Math::Y] * scale)),
						static_cast< int32_t >(std::round(pos[Math::Z] * scale))
					};

					groups[key].push_back(i);
				}

				/* For each group, average the normals and tangents. */
				for ( const auto & [key, indices] : groups )
				{
					if ( indices.size() <= 1 )
					{
						continue;
					}

					Math::Vector< 3, vertex_data_t > avgNormal{};
					Math::Vector< 3, vertex_data_t > avgTangent{};

					for ( const auto idx : indices )
					{
						avgNormal = avgNormal + vertices[idx].normal();
						avgTangent = avgTangent + vertices[idx].tangent();
					}

					avgNormal = avgNormal.normalized();
					avgTangent = avgTangent.normalized();

					for ( const auto idx : indices )
					{
						vertices[idx].setNormal(avgNormal);
						vertices[idx].setTangent(avgTangent);
					}
				}
			}

			/* ---- UV Unwrap: Chart segmentation ---- */

			[[nodiscard]]
			std::vector< ChartData >
			segmentCharts (vertex_data_t seamThresholdRadians) const noexcept
			{
				const auto & triangles = m_shape.triangles();
				const auto triCount = triangles.size();

				/* Build edge → triangle adjacency. */
				std::unordered_map< uint64_t, std::vector< size_t > > edgeToTris;

				for ( size_t t = 0; t < triCount; ++t )
				{
					const auto & tri = triangles[t];

					for ( int i = 0; i < 3; ++i )
					{
						const auto a = tri.vertexIndex(i);
						const auto b = tri.vertexIndex((i + 1) % 3);
						const auto key = packEdgeKey(a, b);

						edgeToTris[key].push_back(t);
					}
				}

				/* Build triangle adjacency: neighbors connected by non-seam edges. */
				std::vector< std::vector< size_t > > triNeighbors(triCount);

				for ( const auto & [key, tris] : edgeToTris )
				{
					if ( tris.size() != 2 )
					{
						continue;
					}

					const auto & n0 = triangles[tris[0]].surfaceNormal();
					const auto & n1 = triangles[tris[1]].surfaceNormal();

					const auto dot = std::clamp(
						Math::Vector< 3, vertex_data_t >::dotProduct(n0, n1),
						static_cast< vertex_data_t >(-1),
						static_cast< vertex_data_t >(1));
					const auto angle = std::acos(dot);

					if ( angle <= seamThresholdRadians )
					{
						triNeighbors[tris[0]].push_back(tris[1]);
						triNeighbors[tris[1]].push_back(tris[0]);
					}
				}

				/* BFS flood-fill to assign chart IDs. */
				std::vector< int > chartId(triCount, -1);
				int currentChart = 0;

				for ( size_t t = 0; t < triCount; ++t )
				{
					if ( chartId[t] >= 0 )
					{
						continue;
					}

					std::queue< size_t > frontier;
					frontier.push(t);
					chartId[t] = currentChart;

					while ( !frontier.empty() )
					{
						const auto current = frontier.front();
						frontier.pop();

						for ( const auto neighbor : triNeighbors[current] )
						{
							if ( chartId[neighbor] < 0 )
							{
								chartId[neighbor] = currentChart;
								frontier.push(neighbor);
							}
						}
					}

					++currentChart;
				}

				/* Build ChartData for each chart. */
				std::vector< ChartData > charts(static_cast< size_t >(currentChart));

				for ( size_t t = 0; t < triCount; ++t )
				{
					auto & chart = charts[static_cast< size_t >(chartId[t])];
					chart.triangleIndices.push_back(static_cast< index_data_t >(t));

					const auto & tri = triangles[t];

					for ( int i = 0; i < 3; ++i )
					{
						const auto vIdx = tri.vertexIndex(i);

						if ( !chart.globalToLocal.contains(vIdx) )
						{
							chart.globalToLocal[vIdx] = chart.vertexIndices.size();
							chart.vertexIndices.push_back(vIdx);
						}
					}
				}

				return charts;
			}

			/* ---- UV Unwrap: Vertex duplication at seam boundaries ---- */

			void
			duplicateSharedVertices (std::vector< ChartData > & charts) noexcept
			{
				/* Find vertices that belong to multiple charts. */
				std::unordered_map< index_data_t, size_t > vertexFirstChart;

				for ( size_t c = 0; c < charts.size(); ++c )
				{
					for ( const auto vIdx : charts[c].vertexIndices )
					{
						auto [it, inserted] = vertexFirstChart.try_emplace(vIdx, c);

						if ( !inserted && it->second != c )
						{
							/* This vertex is shared — duplicate it for this chart. */
							const auto & srcVertex = m_shape.vertices()[vIdx];
							const auto newIdx = m_shape.saveVertex(srcVertex.position(), srcVertex.normal(), srcVertex.textureCoordinates());
							m_shape.vertices()[newIdx].setTangent(srcVertex.tangent());
							m_shape.saveVertexColor({});

							/* Update chart's mapping. */
							const auto localIdx = charts[c].globalToLocal[vIdx];
							charts[c].vertexIndices[localIdx] = newIdx;
							charts[c].globalToLocal.erase(vIdx);
							charts[c].globalToLocal[newIdx] = localIdx;

							/* Update triangle references in this chart. */
							for ( const auto triIdx : charts[c].triangleIndices )
							{
								auto & tri = m_shape.triangles()[triIdx];

								for ( int i = 0; i < 3; ++i )
								{
									if ( tri.vertexIndex(i) == vIdx )
									{
										tri.setVertexIndex(i, newIdx);
									}
								}
							}
						}
					}
				}
			}

			/* ---- UV Unwrap: Pinned vertex selection ---- */

			[[nodiscard]]
			std::pair< size_t, size_t >
			selectPinnedVertices (const ChartData & chart) const noexcept
			{
				/* Find boundary vertices of this chart. */
				std::unordered_map< uint64_t, size_t > edgeCounts;

				for ( const auto triIdx : chart.triangleIndices )
				{
					const auto & tri = m_shape.triangles()[triIdx];

					for ( int i = 0; i < 3; ++i )
					{
						const auto key = packEdgeKey(tri.vertexIndex(i), tri.vertexIndex((i + 1) % 3));

						++edgeCounts[key];
					}
				}

				std::unordered_set< index_data_t > boundaryVerts;

				for ( const auto & [key, count] : edgeCounts )
				{
					if ( count == 1 )
					{
						/* Unpack edge key to get vertex indices. */
						const auto a = static_cast< index_data_t >(key >> 32);
						const auto b = static_cast< index_data_t >(key & 0xFFFFFFFF);

						if ( chart.globalToLocal.contains(a) )
						{
							boundaryVerts.insert(a);
						}

						if ( chart.globalToLocal.contains(b) )
						{
							boundaryVerts.insert(b);
						}
					}
				}

				/* If no boundary, use all vertices. */
				const auto & candidates = boundaryVerts.empty() ? std::unordered_set< index_data_t >(chart.vertexIndices.begin(), chart.vertexIndices.end()) : boundaryVerts;

				/* Find the two farthest apart. */
				size_t bestA = 0;
				size_t bestB = 1;
				vertex_data_t bestDist = 0;

				for ( const auto vA : candidates )
				{
					for ( const auto vB : candidates )
					{
						if ( vA >= vB )
						{
							continue;
						}

						const auto dist = (m_shape.vertex(vA).position() - m_shape.vertex(vB).position()).lengthSquared();

						if ( dist > bestDist )
						{
							bestDist = dist;
							bestA = chart.globalToLocal.at(vA);
							bestB = chart.globalToLocal.at(vB);
						}
					}
				}

				return {bestA, bestB};
			}

			/* ---- UV Unwrap: LSCM solver ---- */

			[[nodiscard]]
			std::vector< Math::Vector< 2, vertex_data_t > >
			solveChartLSCM (const ChartData & chart, size_t pinnedA, size_t pinnedB, const Math::Vector< 2, vertex_data_t > & uvA, const Math::Vector< 2, vertex_data_t > & uvB) const noexcept
			{
				const auto vertCount = chart.vertexIndices.size();
				const auto triCount = chart.triangleIndices.size();

				/* Map local indices to free indices (skip pinned). */
				std::vector< int > localToFree(vertCount, -1);
				size_t freeCount = 0;

				for ( size_t i = 0; i < vertCount; ++i )
				{
					if ( i != pinnedA && i != pinnedB )
					{
						localToFree[i] = static_cast< int >(freeCount++);
					}
				}

				if ( freeCount == 0 )
				{
					/* Only 2 vertices — return the pinned positions. */
					std::vector< Math::Vector< 2, vertex_data_t > > result(vertCount);
					result[pinnedA] = uvA;
					result[pinnedB] = uvB;

					return result;
				}

				const auto numCols = freeCount * 2; /* u and v for each free vertex. */
				const auto numRows = triCount * 2; /* real and imaginary eq per triangle. */

				std::vector< SparseTriplet > triplets;
				triplets.reserve(numRows * 6);
				std::vector< vertex_data_t > rhs(numRows, static_cast< vertex_data_t >(0));

				for ( size_t t = 0; t < triCount; ++t )
				{
					const auto & tri = m_shape.triangles()[chart.triangleIndices[t]];

					index_data_t globalV[3];
					size_t localV[3];

					for ( int i = 0; i < 3; ++i )
					{
						globalV[i] = tri.vertexIndex(i);
						localV[i] = chart.globalToLocal.at(globalV[i]);
					}

					const auto & p0 = m_shape.vertex(globalV[0]).position();
					const auto & p1 = m_shape.vertex(globalV[1]).position();
					const auto & p2 = m_shape.vertex(globalV[2]).position();

					const auto e1 = p1 - p0;
					const auto e2 = p2 - p0;

					const auto s1 = e1.length();

					if ( s1 < static_cast< vertex_data_t >(1e-10) )
					{
						continue;
					}

					const auto s2 = Math::Vector< 3, vertex_data_t >::dotProduct(e2, e1) / s1;
					const auto crossVec = Math::Vector< 3, vertex_data_t >::crossProduct(e1, e2);
					const auto t2 = crossVec.length() / s1;

					if ( t2 < static_cast< vertex_data_t >(1e-10) )
					{
						continue;
					}

					const auto scale = static_cast< vertex_data_t >(1) / std::sqrt(s1 * t2);

					/* LSCM complex coefficients. */
					const vertex_data_t Mre[3] = {-t2 * scale, t2 * scale, 0};
					const vertex_data_t Mim[3] = {(s2 - s1) * scale, -s2 * scale, s1 * scale};

					const auto rowReal = t * 2;
					const auto rowImag = t * 2 + 1;

					for ( int k = 0; k < 3; ++k )
					{
						const auto fi = localToFree[localV[k]];

						if ( fi >= 0 )
						{
							/* Free vertex: add to matrix. */
							const auto uCol = static_cast< size_t >(fi);
							const auto vCol = freeCount + static_cast< size_t >(fi);

							triplets.push_back({rowReal, uCol, Mre[k]});
							triplets.push_back({rowReal, vCol, -Mim[k]});
							triplets.push_back({rowImag, uCol, Mim[k]});
							triplets.push_back({rowImag, vCol, Mre[k]});
						}
						else
						{
							/* Pinned vertex: move to RHS. */
							const auto & pinUV = (localV[k] == pinnedA) ? uvA : uvB;
							const auto pu = pinUV[Math::X];
							const auto pv = pinUV[Math::Y];

							rhs[rowReal] -= Mre[k] * pu - Mim[k] * pv;
							rhs[rowImag] -= Mim[k] * pu + Mre[k] * pv;
						}
					}
				}

				/* Solve least-squares with CGLS. */
				auto solution = solveCGLS(triplets, numRows, numCols, rhs);

				/* Assemble UV coordinates. */
				std::vector< Math::Vector< 2, vertex_data_t > > result(vertCount);
				result[pinnedA] = uvA;
				result[pinnedB] = uvB;

				for ( size_t i = 0; i < vertCount; ++i )
				{
					const auto fi = localToFree[i];

					if ( fi >= 0 )
					{
						result[i] = {solution[static_cast< size_t >(fi)], solution[freeCount + static_cast< size_t >(fi)]};
					}
				}

				return result;
			}

			/* ---- UV Unwrap: CGLS sparse solver ---- */

			[[nodiscard]]
			static
			std::vector< vertex_data_t >
			solveCGLS (const std::vector< SparseTriplet > & triplets, size_t numRows, size_t numCols, const std::vector< vertex_data_t > & rhs, size_t maxIterations = 1000, vertex_data_t tolerance = static_cast< vertex_data_t >(1e-8)) noexcept
			{
				std::vector< vertex_data_t > x(numCols, static_cast< vertex_data_t >(0));
				std::vector< vertex_data_t > r = rhs; /* r = b - A*x, initially b since x=0 */

				/* s = A^T * r */
				std::vector< vertex_data_t > s(numCols, static_cast< vertex_data_t >(0));

				for ( const auto & t : triplets )
				{
					s[t.col] += t.value * r[t.row];
				}

				auto p = s;

				auto dot = [] (const std::vector< vertex_data_t > & a, const std::vector< vertex_data_t > & b) -> vertex_data_t
				{
					vertex_data_t sum = 0;

					for ( size_t i = 0; i < a.size(); ++i )
					{
						sum += a[i] * b[i];
					}

					return sum;
				};

				auto gamma = dot(s, s);
				const auto gamma0 = gamma;

				if ( gamma0 < static_cast< vertex_data_t >(1e-20) )
				{
					return x;
				}

				for ( size_t iter = 0; iter < maxIterations; ++iter )
				{
					/* q = A * p */
					std::vector< vertex_data_t > q(numRows, static_cast< vertex_data_t >(0));

					for ( const auto & t : triplets )
					{
						q[t.row] += t.value * p[t.col];
					}

					const auto denom = dot(q, q);

					if ( denom < static_cast< vertex_data_t >(1e-20) )
					{
						break;
					}

					const auto alpha = gamma / denom;

					for ( size_t i = 0; i < numCols; ++i )
					{
						x[i] += alpha * p[i];
					}

					for ( size_t i = 0; i < numRows; ++i )
					{
						r[i] -= alpha * q[i];
					}

					/* s = A^T * r */
					std::fill(s.begin(), s.end(), static_cast< vertex_data_t >(0));

					for ( const auto & t : triplets )
					{
						s[t.col] += t.value * r[t.row];
					}

					const auto gammaNew = dot(s, s);

					if ( std::sqrt(gammaNew / gamma0) < tolerance )
					{
						break;
					}

					const auto beta = gammaNew / gamma;
					gamma = gammaNew;

					for ( size_t i = 0; i < numCols; ++i )
					{
						p[i] = s[i] + beta * p[i];
					}
				}

				return x;
			}

			/* ---- UV Unwrap: Chart packing ---- */

			static
			void
			packCharts (std::vector< std::vector< Math::Vector< 2, vertex_data_t > > > & allChartUVs) noexcept
			{
				const auto padding = static_cast< vertex_data_t >(0.002);

				struct ChartRect
				{
					size_t index;
					vertex_data_t width;
					vertex_data_t height;
					vertex_data_t offsetX{0};
					vertex_data_t offsetY{0};
				};

				std::vector< ChartRect > rects;
				rects.reserve(allChartUVs.size());

				/* Normalize each chart to its bounding box origin and compute dimensions. */
				for ( size_t c = 0; c < allChartUVs.size(); ++c )
				{
					auto & uvs = allChartUVs[c];

					if ( uvs.empty() )
					{
						continue;
					}

					auto minU = uvs[0][Math::X], maxU = uvs[0][Math::X];
					auto minV = uvs[0][Math::Y], maxV = uvs[0][Math::Y];

					for ( const auto & uv : uvs )
					{
						minU = std::min(minU, uv[Math::X]);
						maxU = std::max(maxU, uv[Math::X]);
						minV = std::min(minV, uv[Math::Y]);
						maxV = std::max(maxV, uv[Math::Y]);
					}

					/* Translate to origin. */
					for ( auto & uv : uvs )
					{
						uv[Math::X] = uv[Math::X] - minU;
						uv[Math::Y] = uv[Math::Y] - minV;
					}

					rects.push_back({c, maxU - minU, maxV - minV});
				}

				/* Sort by decreasing height. */
				std::sort(rects.begin(), rects.end(), [] (const auto & a, const auto & b)
				{
					return a.height > b.height;
				});

				/* Shelf packing. */
				vertex_data_t currentX = 0;
				vertex_data_t shelfY = 0;
				vertex_data_t shelfHeight = 0;
				vertex_data_t maxWidth = 0;

				for ( auto & rect : rects )
				{
					if ( currentX + rect.width + padding > static_cast< vertex_data_t >(10) ) /* Arbitrary large bin width, will be normalized later. */
					{
						shelfY += shelfHeight + padding;
						shelfHeight = 0;
						currentX = 0;
					}

					rect.offsetX = currentX;
					rect.offsetY = shelfY;

					currentX += rect.width + padding;
					shelfHeight = std::max(shelfHeight, rect.height);
					maxWidth = std::max(maxWidth, currentX);
				}

				const auto totalHeight = shelfY + shelfHeight;
				const auto scale = static_cast< vertex_data_t >(1) / std::max(maxWidth, totalHeight);

				/* Apply offset and scale to all chart UVs. */
				for ( const auto & rect : rects )
				{
					auto & uvs = allChartUVs[rect.index];

					for ( auto & uv : uvs )
					{
						uv[Math::X] = (uv[Math::X] + rect.offsetX) * scale;
						uv[Math::Y] = (uv[Math::Y] + rect.offsetY) * scale;
					}
				}
			}

			/**
			 * @brief Merges fragmented boundary loops into larger ones by connecting endpoints.
			 * @note Fragments share endpoints at T-junctions. The merge finds the closest
			 * endpoint pairs and concatenates the loops.
			 * @param loops The fragmented boundary loops.
			 * @return std::vector< BoundaryLoop< index_data_t > > Merged loops.
			 */
			[[nodiscard]]
			std::vector< BoundaryLoop< index_data_t > >
			mergeFragmentedLoops (const std::vector< BoundaryLoop< index_data_t > > & loops) const noexcept
			{
				if ( loops.size() <= 1 )
				{
					return loops;
				}

				/* Build a spatial index of loop endpoints for fast nearest-neighbor lookup. */
				struct Endpoint
				{
					size_t loopIdx;
					bool isFront; /* true = front of loop, false = back */
					Math::Vector< 3, vertex_data_t > position;
				};

				std::vector< Endpoint > endpoints;
				endpoints.reserve(loops.size() * 2);

				for ( size_t i = 0; i < loops.size(); ++i )
				{
					const auto & verts = loops[i].vertexIndices;

					if ( verts.size() < 3 )
					{
						continue;
					}

					endpoints.push_back({i, true, m_shape.vertex(verts.front()).position()});
					endpoints.push_back({i, false, m_shape.vertex(verts.back()).position()});
				}

				/* Greedy merge: repeatedly find the closest endpoint pair from different loops
				 * and concatenate them. The merge tolerance is generous (100x position tolerance). */
				const auto mergeTolerance = m_positionTolerance * static_cast< vertex_data_t >(1000);

				/* Work on copies we can modify. */
				std::vector< BoundaryLoop< index_data_t > > workLoops = loops;
				std::vector< bool > merged(workLoops.size(), false);

				bool didMerge = true;

				while ( didMerge )
				{
					didMerge = false;

					for ( size_t i = 0; i < workLoops.size(); ++i )
					{
						if ( merged[i] || workLoops[i].vertexIndices.size() < 3 )
						{
							continue;
						}

						const auto & backPosI = m_shape.vertex(workLoops[i].vertexIndices.back()).position();
						vertex_data_t bestDist = mergeTolerance;
						size_t bestJ = workLoops.size();

						for ( size_t j = 0; j < workLoops.size(); ++j )
						{
							if ( j == i || merged[j] || workLoops[j].vertexIndices.size() < 3 )
							{
								continue;
							}

							const auto & frontPosJ = m_shape.vertex(workLoops[j].vertexIndices.front()).position();
							const auto dist = (backPosI - frontPosJ).length();

							if ( dist < bestDist )
							{
								bestDist = dist;
								bestJ = j;
							}
						}

						if ( bestJ < workLoops.size() )
						{
							/* Append loop j to loop i. */
							auto & dst = workLoops[i].vertexIndices;
							const auto & src = workLoops[bestJ].vertexIndices;

							dst.insert(dst.end(), src.begin(), src.end());

							merged[bestJ] = true;
							didMerge = true;

							break; /* Restart scan after each merge. */
						}
					}
				}

				/* Collect surviving loops. */
				std::vector< BoundaryLoop< index_data_t > > result;

				for ( size_t i = 0; i < workLoops.size(); ++i )
				{
					if ( !merged[i] && workLoops[i].vertexIndices.size() >= 3 )
					{
						result.push_back(std::move(workLoops[i]));
					}
				}

				return result;
			}

			/**
			 * @brief Computes the 2D cross product (z-component) of vectors (b-a) and (c-a).
			 * @param a First point.
			 * @param b Second point.
			 * @param c Third point.
			 * @return Positive if counter-clockwise, negative if clockwise, zero if collinear.
			 */
			[[nodiscard]]
			static
			vertex_data_t
			cross2D (const Math::Vector< 2, vertex_data_t > & a, const Math::Vector< 2, vertex_data_t > & b, const Math::Vector< 2, vertex_data_t > & c) noexcept
			{
				return (b[Math::X] - a[Math::X]) * (c[Math::Y] - a[Math::Y])
					 - (b[Math::Y] - a[Math::Y]) * (c[Math::X] - a[Math::X]);
			}

			/**
			 * @brief Tests whether a point lies inside a triangle in 2D using cross products.
			 * @param point The point to test.
			 * @param a Triangle vertex A.
			 * @param b Triangle vertex B.
			 * @param c Triangle vertex C.
			 * @return bool True if the point is strictly inside the triangle.
			 */
			[[nodiscard]]
			static
			bool
			isPointInTriangle2D (const Math::Vector< 2, vertex_data_t > & point, const Math::Vector< 2, vertex_data_t > & a, const Math::Vector< 2, vertex_data_t > & b, const Math::Vector< 2, vertex_data_t > & c) noexcept
			{
				const auto d1 = cross2D(a, b, point);
				const auto d2 = cross2D(b, c, point);
				const auto d3 = cross2D(c, a, point);

				const bool hasNeg = (d1 < 0) || (d2 < 0) || (d3 < 0);
				const bool hasPos = (d1 > 0) || (d2 > 0) || (d3 > 0);

				return !(hasNeg && hasPos);
			}

			/**
			 * @brief Computes the signed area of a 2D polygon (positive = CCW, negative = CW).
			 * @param polygon A vector of 2D points forming the polygon.
			 * @return vertex_data_t The signed area.
			 */
			[[nodiscard]]
			static
			vertex_data_t
			computeSignedArea2D (const std::vector< Math::Vector< 2, vertex_data_t > > & polygon) noexcept
			{
				vertex_data_t area = 0;
				const auto count = polygon.size();

				for ( size_t i = 0; i < count; ++i )
				{
					const auto & current = polygon[i];
					const auto & next = polygon[(i + 1) % count];

					area += current[Math::X] * next[Math::Y] - next[Math::X] * current[Math::Y];
				}

				return area * static_cast< vertex_data_t >(0.5);
			}

			/**
			 * @brief Builds an orthonormal basis (U, V) perpendicular to the given normal.
			 * @param normal The normal vector (must be normalized).
			 * @return std::pair of two orthonormal vectors.
			 */
			[[nodiscard]]
			static
			std::pair< Math::Vector< 3, vertex_data_t >, Math::Vector< 3, vertex_data_t > >
			buildOrthonormalBasis (const Math::Vector< 3, vertex_data_t > & normal) noexcept
			{
				/* Pick the axis least aligned with normal to avoid degeneracy. */
				Math::Vector< 3, vertex_data_t > up;

				if ( std::abs(normal[Math::Y]) < static_cast< vertex_data_t >(0.9) )
				{
					up = {0, 1, 0};
				}
				else
				{
					up = {1, 0, 0};
				}

				auto axisU = Math::Vector< 3, vertex_data_t >::crossProduct(normal, up).normalized();
				auto axisV = Math::Vector< 3, vertex_data_t >::crossProduct(normal, axisU).normalized();

				return {axisU, axisV};
			}

			/**
			 * @brief Quantized 3D position key for spatial hashing.
			 */
			struct PositionKey
			{
				int64_t x;
				int64_t y;
				int64_t z;

				bool operator== (const PositionKey & other) const noexcept = default;
			};

			struct PositionKeyHash
			{
				size_t
				operator() (const PositionKey & key) const noexcept
				{
					auto h = std::hash< int64_t >{}(key.x);
					h ^= std::hash< int64_t >{}(key.y) << 1;
					h ^= std::hash< int64_t >{}(key.z) << 2;

					return h;
				}
			};

			/**
			 * @brief Quantizes a position into a grid-based key.
			 * @param position The 3D position.
			 * @return PositionKey
			 */
			[[nodiscard]]
			PositionKey
			quantize (const Math::Vector< 3, vertex_data_t > & position) const noexcept
			{
				const auto scale = static_cast< vertex_data_t >(1) / m_positionTolerance;

				return PositionKey{
					static_cast< int64_t >(std::round(position[Math::X] * scale)),
					static_cast< int64_t >(std::round(position[Math::Y] * scale)),
					static_cast< int64_t >(std::round(position[Math::Z] * scale))
				};
			}

			/**
			 * @brief Builds a map from each vertex index to its canonical index (first vertex at the same position).
			 * @return std::unordered_map< index_data_t, index_data_t >
			 */
			[[nodiscard]]
			std::unordered_map< index_data_t, index_data_t >
			buildCanonicalMap () const noexcept
			{
				std::unordered_map< PositionKey, index_data_t, PositionKeyHash > positionToCanonical;
				std::unordered_map< index_data_t, index_data_t > canonicalMap;

				const auto & vertices = m_shape.vertices();

				for ( index_data_t i = 0; i < static_cast< index_data_t >(vertices.size()); ++i )
				{
					const auto key = this->quantize(vertices[i].position());

					auto [it, inserted] = positionToCanonical.try_emplace(key, i);

					canonicalMap[i] = it->second;
				}

				return canonicalMap;
			}

			/**
			 * @brief Packs two canonical vertex indices into a single uint64_t key (sorted).
			 * @param a First canonical index.
			 * @param b Second canonical index.
			 * @return uint64_t
			 */
			[[nodiscard]]
			static
			uint64_t
			packEdgeKey (index_data_t a, index_data_t b) noexcept
			{
				const auto lo = std::min(a, b);
				const auto hi = std::max(a, b);

				return (static_cast< uint64_t >(lo) << 32) | static_cast< uint64_t >(hi);
			}

			/* ---- Members ---- */

			Shape< vertex_data_t, index_data_t > & m_shape;
			vertex_data_t m_positionTolerance;
	};
}
