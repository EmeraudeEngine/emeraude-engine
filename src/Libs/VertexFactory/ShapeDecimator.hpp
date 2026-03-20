/*
 * src/Libs/VertexFactory/ShapeDecimator.hpp
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
#include <array>
#include <cmath>
#include <cstdint>
#include <functional>
#include <limits>
#include <queue>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <vector>

/* Local inclusions. */
#include "Shape.hpp"
#include "ShapeProcessor.hpp"
#include "Libs/PixelFactory/Pixmap.hpp"
#include "Libs/ThreadPool.hpp"

namespace EmEn::Libs::VertexFactory
{
	/**
	 * @brief Mesh decimation using Quadric Error Metrics (Garland & Heckbert).
	 * @note Reduces polygon density while preserving overall shape, UV seams, and boundary edges.
	 * @tparam vertex_data_t The precision type of vertex data. Default float.
	 * @tparam index_data_t The precision type of index data. Default uint32_t.
	 */
	template< typename vertex_data_t = float, typename index_data_t = uint32_t >
	requires (std::is_floating_point_v< vertex_data_t > && std::is_unsigned_v< index_data_t >)
	class ShapeDecimator final
	{
		public:

			/**
			 * @brief Constructs a decimator for the given shape.
			 * @note The source shape is preserved for normal map baking (correct normals).
			 * A position-only dedup copy is created internally for QEM mesh connectivity.
			 * @param source The source high-poly shape (with correct normals/UVs).
			 * @param ratio Decimation ratio: 0.0 = maximum reduction, 1.0 = no reduction.
			 * @param boundaryPenaltyWeight Penalty multiplier for boundary/UV seam edges. Default 1000.
			 * @param normalMapResolution Resolution of the baked normal map (0 = disabled). Default 0.
			 */
			explicit
			ShapeDecimator (const Shape< vertex_data_t, index_data_t > & source, vertex_data_t ratio = static_cast< vertex_data_t >(0.5), vertex_data_t boundaryPenaltyWeight = static_cast< vertex_data_t >(1000), uint32_t normalMapResolution = 0, ThreadPool * threadPool = nullptr) noexcept
				: m_source(source),
				m_bakeSource(source),
				m_ratio(std::clamp(ratio, static_cast< vertex_data_t >(0), static_cast< vertex_data_t >(1))),
				m_boundaryPenaltyWeight(boundaryPenaltyWeight),
				m_normalMapResolution(normalMapResolution),
				m_threadPool(threadPool)
			{

			}

			/** @brief Copy constructor. */
			ShapeDecimator (const ShapeDecimator &) noexcept = delete;

			/** @brief Move constructor. */
			ShapeDecimator (ShapeDecimator &&) noexcept = delete;

			/** @brief Copy assignment. */
			ShapeDecimator & operator= (const ShapeDecimator &) noexcept = delete;

			/** @brief Move assignment. */
			ShapeDecimator & operator= (ShapeDecimator &&) noexcept = delete;

			/** @brief Destructor. */
			~ShapeDecimator () = default;

			/**
			 * @brief Performs the decimation and returns a new reduced shape.
			 * @return Shape< vertex_data_t, index_data_t > The decimated shape.
			 */
			[[nodiscard]]
			Shape< vertex_data_t, index_data_t >
			decimate () const noexcept
			{
				if ( m_source.empty() || m_ratio >= static_cast< vertex_data_t >(1) )
				{
					return m_source;
				}

				/* Create a position-only deduped copy for QEM connectivity.
				 * The original source (m_bakeSource) is preserved for normal map baking. */
				auto workShape = m_source;
				ShapeProcessor< vertex_data_t, index_data_t > connProcessor{workShape};
				connProcessor.deduplicateVertices(false, false);

				const auto srcTriCount = workShape.triangles().size();
				const auto targetTriCount = std::max(size_t{4}, static_cast< size_t >(std::round(static_cast< vertex_data_t >(srcTriCount) * m_ratio)));

				/* Initialize mutable working data. */
				std::vector< VertexData > vertices;
				std::vector< TriangleData > triangles;

				initializeData(vertices, triangles, workShape);

				/* Compute initial quadrics. */
				computeInitialQuadrics(vertices, triangles);

				/* Detect and penalize boundaries and UV seams. */
				applyBoundaryAndSeamPenalties(vertices, triangles);

				/* Build initial collapse candidates. */
				auto queue = buildCollapseQueue(vertices, triangles);

				/* Iterative edge collapse. */
				size_t liveTriCount = triangles.size();

				while ( liveTriCount > targetTriCount && !queue.empty() )
				{
					auto candidate = queue.top();
					queue.pop();

					/* Lazy validation: skip stale entries. */
					if ( vertices[candidate.v0].removed || vertices[candidate.v1].removed )
					{
						continue;
					}

					if ( candidate.genV0 != vertices[candidate.v0].generation || candidate.genV1 != vertices[candidate.v1].generation )
					{
						continue;
					}

					/* Topology check: link condition. */
					if ( !checkLinkCondition(candidate.v0, candidate.v1, vertices, triangles) )
					{
						continue;
					}

					/* Normal flip check. */
					if ( checkNormalFlip(candidate.v0, candidate.v1, candidate.optimalPos, vertices, triangles) )
					{
						continue;
					}

					/* Perform the collapse: v0 survives, v1 is removed. */
					const auto v0 = candidate.v0;
					const auto v1 = candidate.v1;

					vertices[v0].position = candidate.optimalPos;
					vertices[v0].quadric += vertices[v1].quadric;

					/* Update triangles: replace v1 with v0. */
					for ( const auto triIdx : vertices[v1].adjacentTris )
					{
						auto & tri = triangles[triIdx];

						if ( tri.removed )
						{
							continue;
						}

						/* Replace v1 with v0. */
						for ( int i = 0; i < 3; ++i )
						{
							if ( tri.v[i] == v1 )
							{
								tri.v[i] = v0;
							}
						}

						/* Check for degenerate triangle (two identical indices). */
						if ( tri.v[0] == tri.v[1] || tri.v[1] == tri.v[2] || tri.v[0] == tri.v[2] )
						{
							tri.removed = true;
							--liveTriCount;

							continue;
						}

						/* Add this triangle to v0's adjacency if not already there. */
						vertices[v0].adjacentTris.insert(triIdx);
					}

					/* Merge neighbor sets. */
					for ( const auto neighbor : vertices[v1].neighbors )
					{
						if ( neighbor != v0 && !vertices[neighbor].removed )
						{
							vertices[v0].neighbors.insert(neighbor);
							vertices[neighbor].neighbors.erase(v1);
							vertices[neighbor].neighbors.insert(v0);
						}
					}

					/* Remove v0 from its own neighbor list and remove v1. */
					vertices[v0].neighbors.erase(v1);
					vertices[v0].adjacentTris.erase(vertices[v0].adjacentTris.end(), vertices[v0].adjacentTris.end());

					/* Clean up v0's adjacentTris: remove dead triangles. */
					{
						std::unordered_set< size_t > cleanTris;

						for ( const auto triIdx : vertices[v0].adjacentTris )
						{
							if ( !triangles[triIdx].removed )
							{
								cleanTris.insert(triIdx);
							}
						}

						vertices[v0].adjacentTris = std::move(cleanTris);
					}

					vertices[v1].removed = true;
					++vertices[v0].generation;

					/* Re-queue edges from v0 to its neighbors. */
					for ( const auto neighbor : vertices[v0].neighbors )
					{
						if ( vertices[neighbor].removed )
						{
							continue;
						}

						const auto combined = vertices[v0].quadric + vertices[neighbor].quadric;
						const auto optPos = computeOptimalPosition(combined, vertices[v0].position, vertices[neighbor].position);
						const auto cost = evaluateQuadric(combined, optPos);

						queue.push({cost, v0, neighbor, optPos, vertices[v0].generation, vertices[neighbor].generation});
					}
				}

				auto output = buildOutputShape(vertices, triangles, workShape);

				/* Normal map baking: generate lightmap UVs, then bake high-poly normals. */
				if ( m_normalMapResolution > 0 )
				{
					ShapeProcessor< vertex_data_t, index_data_t > processor{output};
					processor.generateLightmapUV();

					m_normalMap = bakeNormalMap(output, m_normalMapResolution);
				}

				return output;
			}

			/**
			 * @brief Returns the baked normal map (valid only after decimate() with normalMapResolution > 0).
			 * @return const PixelFactory::Pixmap< uint8_t > &
			 */
			[[nodiscard]]
			const PixelFactory::Pixmap< uint8_t > &
			normalMap () const noexcept
			{
				return m_normalMap;
			}

		private:

			/* ---- Internal types ---- */

			/**
			 * @brief Symmetric 4x4 quadric error matrix stored as 10 floats.
			 */
			struct Quadric
			{
				std::array< vertex_data_t, 10 > q{};

				Quadric & operator+= (const Quadric & other) noexcept
				{
					for ( size_t i = 0; i < 10; ++i )
					{
						q[i] += other.q[i];
					}

					return *this;
				}

				[[nodiscard]]
				Quadric operator+ (const Quadric & other) const noexcept
				{
					Quadric result = *this;
					result += other;

					return result;
				}
			};

			struct VertexData
			{
				Quadric quadric;
				Math::Vector< 3, vertex_data_t > position;
				std::unordered_set< size_t > adjacentTris;
				std::unordered_set< index_data_t > neighbors;
				index_data_t srcIndex{0};
				uint32_t generation{0};
				bool removed{false};
			};

			struct TriangleData
			{
				index_data_t v[3]{0, 0, 0};
				size_t srcTriIndex{0};
				bool removed{false};
			};

			struct CollapseCandidate
			{
				vertex_data_t cost{0};
				index_data_t v0{0};
				index_data_t v1{0};
				Math::Vector< 3, vertex_data_t > optimalPos;
				uint32_t genV0{0};
				uint32_t genV1{0};

				bool operator> (const CollapseCandidate & other) const noexcept
				{
					return cost > other.cost;
				}
			};

			using CollapseQueue = std::priority_queue< CollapseCandidate, std::vector< CollapseCandidate >, std::greater< CollapseCandidate > >;

			/* ---- Initialization ---- */

			void
			initializeData (std::vector< VertexData > & vertices, std::vector< TriangleData > & triangles, const Shape< vertex_data_t, index_data_t > & workShape) const noexcept
			{
				const auto & srcVerts = workShape.vertices();
				const auto & srcTris = workShape.triangles();

				/* Assumes the source shape has been pre-deduped with position-only dedup
				 * (ShapeProcessor::deduplicateVertices(false, false)) to build proper
				 * mesh connectivity. Without this, OBJ-style meshes with per-face vertices
				 * have no shared edges and the QEM cannot operate. */
				vertices.resize(srcVerts.size());

				for ( size_t i = 0; i < srcVerts.size(); ++i )
				{
					vertices[i].position = srcVerts[i].position();
					vertices[i].srcIndex = static_cast< index_data_t >(i);
				}

				triangles.resize(srcTris.size());

				for ( size_t t = 0; t < srcTris.size(); ++t )
				{
					const auto & tri = srcTris[t];

					triangles[t].v[0] = tri.vertexIndex(0);
					triangles[t].v[1] = tri.vertexIndex(1);
					triangles[t].v[2] = tri.vertexIndex(2);
					triangles[t].srcTriIndex = t;

					/* Skip degenerate triangles. */
					if ( triangles[t].v[0] == triangles[t].v[1] || triangles[t].v[1] == triangles[t].v[2] || triangles[t].v[0] == triangles[t].v[2] )
					{
						triangles[t].removed = true;

						continue;
					}

					for ( int i = 0; i < 3; ++i )
					{
						const auto vIdx = triangles[t].v[i];

						vertices[vIdx].adjacentTris.insert(t);

						for ( int j = 0; j < 3; ++j )
						{
							if ( i != j )
							{
								vertices[vIdx].neighbors.insert(triangles[t].v[j]);
							}
						}
					}
				}
			}

			/* ---- Quadric computation ---- */

			[[nodiscard]]
			static
			Quadric
			computePlaneQuadric (const Math::Vector< 3, vertex_data_t > & normal, vertex_data_t d) noexcept
			{
				const auto a = normal[Math::X];
				const auto b = normal[Math::Y];
				const auto c = normal[Math::Z];

				return Quadric{{a * a, a * b, a * c, a * d, b * b, b * c, b * d, c * c, c * d, d * d}};
			}

			void
			computeInitialQuadrics (std::vector< VertexData > & vertices, const std::vector< TriangleData > & triangles) const noexcept
			{
				for ( size_t t = 0; t < triangles.size(); ++t )
				{
					const auto & tri = triangles[t];
					const auto & p0 = vertices[tri.v[0]].position;
					const auto & p1 = vertices[tri.v[1]].position;
					const auto & p2 = vertices[tri.v[2]].position;

					const auto edge1 = p1 - p0;
					const auto edge2 = p2 - p0;
					const auto cross = Math::Vector< 3, vertex_data_t >::crossProduct(edge1, edge2);
					const auto area = cross.length() * static_cast< vertex_data_t >(0.5);

					if ( area < static_cast< vertex_data_t >(1e-10) )
					{
						continue;
					}

					const auto normal = cross.normalized();
					const auto d = -Math::Vector< 3, vertex_data_t >::dotProduct(normal, p0);

					auto Q = computePlaneQuadric(normal, d);

					/* Weight by triangle area. */
					for ( auto & val : Q.q )
					{
						val *= area;
					}

					vertices[tri.v[0]].quadric += Q;
					vertices[tri.v[1]].quadric += Q;
					vertices[tri.v[2]].quadric += Q;
				}
			}

			/* ---- Boundary and UV seam detection ---- */

			[[nodiscard]]
			static
			uint64_t
			packEdgeKey (index_data_t a, index_data_t b) noexcept
			{
				const auto lo = std::min(a, b);
				const auto hi = std::max(a, b);

				return (static_cast< uint64_t >(lo) << 32) | static_cast< uint64_t >(hi);
			}

			void
			applyBoundaryAndSeamPenalties (std::vector< VertexData > & vertices, const std::vector< TriangleData > & triangles) const noexcept
			{
				/* Count edge occurrences to find boundaries. */
				std::unordered_map< uint64_t, size_t > edgeCounts;
				std::unordered_map< uint64_t, std::array< index_data_t, 2 > > edgeVerts;

				for ( const auto & tri : triangles )
				{
					for ( int i = 0; i < 3; ++i )
					{
						const auto a = tri.v[i];
						const auto b = tri.v[(i + 1) % 3];
						const auto key = packEdgeKey(a, b);

						++edgeCounts[key];
						edgeVerts[key] = {a, b};
					}
				}

				/* Apply penalty to boundary edges. */
				for ( const auto & [key, count] : edgeCounts )
				{
					if ( count != 1 )
					{
						continue;
					}

					const auto & [a, b] = edgeVerts[key];
					const auto & posA = vertices[a].position;
					const auto & posB = vertices[b].position;

					const auto edgeDir = (posB - posA).normalized();

					/* Find the adjacent triangle to get its normal. */
					Math::Vector< 3, vertex_data_t > triNormal{0, 1, 0};

					for ( const auto triIdx : vertices[a].adjacentTris )
					{
						const auto & tri = triangles[triIdx];
						bool hasA = false;
						bool hasB = false;

						for ( int i = 0; i < 3; ++i )
						{
							if ( tri.v[i] == a )
							{
								hasA = true;
							}

							if ( tri.v[i] == b )
							{
								hasB = true;
							}
						}

						if ( hasA && hasB )
						{
							const auto & p0 = vertices[tri.v[0]].position;
							const auto & p1 = vertices[tri.v[1]].position;
							const auto & p2 = vertices[tri.v[2]].position;

							triNormal = Math::Vector< 3, vertex_data_t >::crossProduct(p1 - p0, p2 - p0).normalized();

							break;
						}
					}

					/* Perpendicular plane to the boundary edge. */
					const auto perpNormal = Math::Vector< 3, vertex_data_t >::crossProduct(edgeDir, triNormal).normalized();
					const auto d = -Math::Vector< 3, vertex_data_t >::dotProduct(perpNormal, posA);

					auto penalty = computePlaneQuadric(perpNormal, d);

					for ( auto & val : penalty.q )
					{
						val *= m_boundaryPenaltyWeight;
					}

					vertices[a].quadric += penalty;
					vertices[b].quadric += penalty;
				}

				/* Detect UV seam vertices and apply penalties. */
				const auto & srcVerts = m_source.vertices();

				for ( size_t v = 0; v < vertices.size(); ++v )
				{
					if ( vertices[v].adjacentTris.size() < 2 )
					{
						continue;
					}

					Math::Vector< 3, vertex_data_t > firstTC;
					bool isSeam = false;
					bool hasFirst = false;

					for ( const auto triIdx : vertices[v].adjacentTris )
					{
						const auto & srcTri = m_source.triangles()[triIdx];

						for ( int i = 0; i < 3; ++i )
						{
							if ( triangles[triIdx].v[i] == static_cast< index_data_t >(v) )
							{
								const auto & tc = srcVerts[srcTri.vertexIndex(i)].textureCoordinates();

								if ( !hasFirst )
								{
									firstTC = tc;
									hasFirst = true;
								}
								else if ( (tc - firstTC).lengthSquared() > static_cast< vertex_data_t >(1e-6) )
								{
									isSeam = true;
								}

								break;
							}
						}

						if ( isSeam )
						{
							break;
						}
					}

					if ( isSeam )
					{
						/* Add a large penalty to prevent collapsing UV seam vertices. */
						Quadric penalty{};

						for ( auto & val : penalty.q )
						{
							val = m_boundaryPenaltyWeight;
						}

						vertices[v].quadric += penalty;
					}
				}
			}

			/* ---- Optimal position and quadric evaluation ---- */

			[[nodiscard]]
			static
			vertex_data_t
			evaluateQuadric (const Quadric & Q, const Math::Vector< 3, vertex_data_t > & v) noexcept
			{
				/* Q = [a11 a12 a13 a14]   indices: [0 1 2 3]
				 *     [a12 a22 a23 a24]            [1 4 5 6]
				 *     [a13 a23 a33 a34]            [2 5 7 8]
				 *     [a14 a24 a34 a44]            [3 6 8 9]
				 *
				 * v^T Q v = (extended with w=1) */
				const auto x = v[Math::X];
				const auto y = v[Math::Y];
				const auto z = v[Math::Z];

				return Q.q[0] * x * x + static_cast< vertex_data_t >(2) * Q.q[1] * x * y + static_cast< vertex_data_t >(2) * Q.q[2] * x * z + static_cast< vertex_data_t >(2) * Q.q[3] * x
					 + Q.q[4] * y * y + static_cast< vertex_data_t >(2) * Q.q[5] * y * z + static_cast< vertex_data_t >(2) * Q.q[6] * y
					 + Q.q[7] * z * z + static_cast< vertex_data_t >(2) * Q.q[8] * z
					 + Q.q[9];
			}

			[[nodiscard]]
			static
			Math::Vector< 3, vertex_data_t >
			computeOptimalPosition (const Quadric & Q, const Math::Vector< 3, vertex_data_t > & vA, const Math::Vector< 3, vertex_data_t > & vB) noexcept
			{
				/* Try to solve the 3x3 system from the upper-left of Q.
				 * | a11 a12 a13 | | x |   | -a14 |
				 * | a12 a22 a23 | | y | = | -a24 |
				 * | a13 a23 a33 | | z |   | -a34 | */
				const auto a = Q.q[0], b = Q.q[1], c = Q.q[2], d = Q.q[3];
				const auto e = Q.q[4], f = Q.q[5], g = Q.q[6];
				const auto h = Q.q[7], i = Q.q[8];

				const auto det = a * (e * h - f * f) - b * (b * h - f * c) + c * (b * f - e * c);

				if ( std::abs(det) > static_cast< vertex_data_t >(1e-10) )
				{
					const auto invDet = static_cast< vertex_data_t >(1) / det;

					const auto rx = -d, ry = -g, rz = -i;

					const auto x = invDet * (rx * (e * h - f * f) + ry * (c * f - b * h) + rz * (b * f - c * e));
					const auto y = invDet * (rx * (c * f - b * h) + ry * (a * h - c * c) + rz * (b * c - a * f));
					const auto z = invDet * (rx * (b * f - c * e) + ry * (b * c - a * f) + rz * (a * e - b * b));

					return {x, y, z};
				}

				/* Fallback: pick the best among midpoint, vA, vB. */
				const auto mid = (vA + vB) * static_cast< vertex_data_t >(0.5);

				const auto costA = evaluateQuadric(Q, vA);
				const auto costB = evaluateQuadric(Q, vB);
				const auto costMid = evaluateQuadric(Q, mid);

				if ( costMid <= costA && costMid <= costB )
				{
					return mid;
				}

				return (costA <= costB) ? vA : vB;
			}

			/* ---- Topology checks ---- */

			[[nodiscard]]
			static
			bool
			checkLinkCondition (index_data_t v0, index_data_t v1, const std::vector< VertexData > & vertices, const std::vector< TriangleData > & triangles) noexcept
			{
				/* Count shared neighbors (the "link" of the edge). */
				size_t sharedCount = 0;

				for ( const auto neighbor : vertices[v0].neighbors )
				{
					if ( neighbor != v1 && vertices[v1].neighbors.contains(neighbor) )
					{
						++sharedCount;
					}
				}

				/* For a manifold interior edge: exactly 2 shared neighbors.
				 * For a boundary edge: exactly 1 shared neighbor.
				 * Allow both cases. */
				return sharedCount <= 2;
			}

			[[nodiscard]]
			static
			bool
			checkNormalFlip (index_data_t v0, index_data_t v1, const Math::Vector< 3, vertex_data_t > & newPos, const std::vector< VertexData > & vertices, const std::vector< TriangleData > & triangles) noexcept
			{
				/* Check all triangles that will survive the collapse (adjacent to v0 or v1,
				 * not shared between them). Verify that no triangle normal flips. */
				auto checkTriangles = [&] (index_data_t vKeep, index_data_t vRemove) -> bool
				{
					for ( const auto triIdx : vertices[vKeep].adjacentTris )
					{
						const auto & tri = triangles[triIdx];

						if ( tri.removed )
						{
							continue;
						}

						/* Skip triangles shared by both v0 and v1 (they'll be removed). */
						bool hasRemove = false;

						for ( int i = 0; i < 3; ++i )
						{
							if ( tri.v[i] == vRemove )
							{
								hasRemove = true;

								break;
							}
						}

						if ( hasRemove )
						{
							continue;
						}

						/* Compute old and new normals. */
						Math::Vector< 3, vertex_data_t > oldPositions[3];
						Math::Vector< 3, vertex_data_t > newPositions[3];

						for ( int i = 0; i < 3; ++i )
						{
							oldPositions[i] = vertices[tri.v[i]].position;
							newPositions[i] = (tri.v[i] == vKeep) ? newPos : vertices[tri.v[i]].position;
						}

						const auto oldNormal = Math::Vector< 3, vertex_data_t >::crossProduct(
							oldPositions[1] - oldPositions[0], oldPositions[2] - oldPositions[0]);

						const auto newNormal = Math::Vector< 3, vertex_data_t >::crossProduct(
							newPositions[1] - newPositions[0], newPositions[2] - newPositions[0]);

						if ( Math::Vector< 3, vertex_data_t >::dotProduct(oldNormal, newNormal) < 0 )
						{
							return true; /* Flip detected. */
						}
					}

					return false;
				};

				return checkTriangles(v0, v1) || checkTriangles(v1, v0);
			}

			/* ---- Priority queue ---- */

			[[nodiscard]]
			CollapseQueue
			buildCollapseQueue (const std::vector< VertexData > & vertices, const std::vector< TriangleData > & triangles) const noexcept
			{
				CollapseQueue queue;
				std::unordered_set< uint64_t > processedEdges;

				for ( size_t t = 0; t < triangles.size(); ++t )
				{
					const auto & tri = triangles[t];

					for ( int i = 0; i < 3; ++i )
					{
						const auto a = tri.v[i];
						const auto b = tri.v[(i + 1) % 3];
						const auto key = packEdgeKey(a, b);

						if ( processedEdges.contains(key) )
						{
							continue;
						}

						processedEdges.insert(key);

						const auto combined = vertices[a].quadric + vertices[b].quadric;
						const auto optPos = computeOptimalPosition(combined, vertices[a].position, vertices[b].position);
						const auto cost = evaluateQuadric(combined, optPos);

						queue.push({cost, a, b, optPos, vertices[a].generation, vertices[b].generation});
					}
				}

				return queue;
			}

			/* ---- Output shape construction ---- */

			[[nodiscard]]
			Shape< vertex_data_t, index_data_t >
			buildOutputShape (const std::vector< VertexData > & vertices, const std::vector< TriangleData > & triangles, const Shape< vertex_data_t, index_data_t > & workShape) const noexcept
			{
				Shape< vertex_data_t, index_data_t > output;

				/* Build vertex compaction map. */
				std::unordered_map< index_data_t, index_data_t > vertexMap;

				for ( size_t t = 0; t < triangles.size(); ++t )
				{
					if ( triangles[t].removed )
					{
						continue;
					}

					for ( int i = 0; i < 3; ++i )
					{
						const auto srcIdx = triangles[t].v[i];

						if ( vertexMap.contains(srcIdx) )
						{
							continue;
						}

						const auto & srcVertex = workShape.vertices()[vertices[srcIdx].srcIndex];
						const auto & newPos = vertices[srcIdx].position;

						const auto dstIdx = output.saveVertex(newPos, srcVertex.normal(), srcVertex.textureCoordinates());
						output.vertices()[dstIdx].setTangent(srcVertex.tangent());

						vertexMap[srcIdx] = dstIdx;
					}
				}

				/* Emit surviving triangles. */
				for ( size_t t = 0; t < triangles.size(); ++t )
				{
					if ( triangles[t].removed )
					{
						continue;
					}

					const auto & tri = triangles[t];

					const auto dv0 = vertexMap[tri.v[0]];
					const auto dv1 = vertexMap[tri.v[1]];
					const auto dv2 = vertexMap[tri.v[2]];

					const auto c0 = output.saveVertexColor({});
					const auto c1 = output.saveVertexColor({});
					const auto c2 = output.saveVertexColor({});

					ShapeTriangle< vertex_data_t, index_data_t > outTri(dv0, dv1, dv2);
					outTri.setVertexColorIndex(0, c0);
					outTri.setVertexColorIndex(1, c1);
					outTri.setVertexColorIndex(2, c2);

					output.triangles().emplace_back(outTri);

					auto & groups = output.groups();

					if ( groups.empty() )
					{
						groups.emplace_back(0, 1);
					}
					else
					{
						++groups.back().second;
					}
				}

				if ( !output.empty() )
				{
					output.computeTriangleNormal();
					output.computeTriangleTangent();
					output.computeVertexNormal();
					output.computeVertexTangent();

					/* Verify normal orientation against the source.
					 * If the majority of computed normals disagree with source normals
					 * (e.g. due to Y-flip changing winding), flip all normals. */
					size_t agree = 0;
					size_t disagree = 0;
					const auto sampleCount = std::min(size_t{100}, output.vertices().size());

					for ( size_t i = 0; i < sampleCount; ++i )
					{
						const auto sampleIdx = i * output.vertices().size() / sampleCount;
						const auto & computedNormal = output.vertices()[sampleIdx].normal();

						/* Find closest source vertex by position. */
						const auto & pos = output.vertices()[sampleIdx].position();
						vertex_data_t bestDist = std::numeric_limits< vertex_data_t >::max();
						Math::Vector< 3, vertex_data_t > srcNormal;

						for ( const auto & srcVert : m_source.vertices() )
						{
							const auto dist = (srcVert.position() - pos).lengthSquared();

							if ( dist < bestDist )
							{
								bestDist = dist;
								srcNormal = srcVert.normal();
							}
						}

						if ( Math::Vector< 3, vertex_data_t >::dotProduct(computedNormal, srcNormal) > 0 )
						{
							++agree;
						}
						else
						{
							++disagree;
						}
					}

					if ( disagree > agree )
					{
						for ( auto & vert : output.vertices() )
						{
							vert.setNormal(vert.normal() * static_cast< vertex_data_t >(-1));
						}

						for ( auto & tri : output.triangles() )
						{
							tri.setSurfaceNormal(tri.surfaceNormal() * static_cast< vertex_data_t >(-1));
						}
					}

					output.updateProperties();
				}

				return output;
			}

			/* ---- Members ---- */

			/* ---- Normal map baking ---- */

			[[nodiscard]]
			PixelFactory::Pixmap< uint8_t >
			bakeNormalMap (const Shape< vertex_data_t, index_data_t > & lowPoly, uint32_t resolution) const noexcept
			{
				using namespace PixelFactory;

				Pixmap< uint8_t > normalMap(resolution, resolution, ChannelMode::RGBA, Color< float >{0.5F, 0.5F, 1.0F, 1.0F});

				const auto & lowTris = lowPoly.triangles();
				const auto & lowVerts = lowPoly.vertices();
				const auto & highTris = m_bakeSource.triangles();
				const auto & highVerts = m_bakeSource.vertices();

				/* Build a simple grid spatial hash over high-poly triangles for fast ray-triangle tests. */
				const auto & bbox = m_bakeSource.boundingBox();
				const auto bboxMin = bbox.minimum();
				const auto bboxMax = bbox.maximum();
				const auto bboxSize = bboxMax - bboxMin;

				constexpr size_t GridRes = 128;
				const auto cellSize = bboxSize * (static_cast< vertex_data_t >(1) / static_cast< vertex_data_t >(GridRes));

				std::vector< std::vector< size_t > > grid(GridRes * GridRes * GridRes);

				auto gridIndex = [&] (const Math::Vector< 3, vertex_data_t > & pos) -> size_t
				{
					auto cx = static_cast< size_t >(std::clamp((pos[Math::X] - bboxMin[Math::X]) / cellSize[Math::X], static_cast< vertex_data_t >(0), static_cast< vertex_data_t >(GridRes - 1)));
					auto cy = static_cast< size_t >(std::clamp((pos[Math::Y] - bboxMin[Math::Y]) / cellSize[Math::Y], static_cast< vertex_data_t >(0), static_cast< vertex_data_t >(GridRes - 1)));
					auto cz = static_cast< size_t >(std::clamp((pos[Math::Z] - bboxMin[Math::Z]) / cellSize[Math::Z], static_cast< vertex_data_t >(0), static_cast< vertex_data_t >(GridRes - 1)));

					return cx + cy * GridRes + cz * GridRes * GridRes;
				};

				for ( size_t t = 0; t < highTris.size(); ++t )
				{
					const auto & tri = highTris[t];
					const auto & p0 = highVerts[tri.vertexIndex(0)].position();
					const auto & p1 = highVerts[tri.vertexIndex(1)].position();
					const auto & p2 = highVerts[tri.vertexIndex(2)].position();

					/* Insert into all grid cells that the triangle's AABB overlaps. */
					auto triMin = p0, triMax = p0;

					for ( int axis = 0; axis < 3; ++axis )
					{
						triMin[axis] = std::min({p0[axis], p1[axis], p2[axis]});
						triMax[axis] = std::max({p0[axis], p1[axis], p2[axis]});
					}

					auto minCell = gridIndex(triMin);
					auto maxCell = gridIndex(triMax);

					size_t minCx = minCell % GridRes, minCy = (minCell / GridRes) % GridRes, minCz = minCell / (GridRes * GridRes);
					size_t maxCx = maxCell % GridRes, maxCy = (maxCell / GridRes) % GridRes, maxCz = maxCell / (GridRes * GridRes);

					for ( size_t z = minCz; z <= maxCz; ++z )
					{
						for ( size_t y = minCy; y <= maxCy; ++y )
						{
							for ( size_t x = minCx; x <= maxCx; ++x )
							{
								grid[x + y * GridRes + z * GridRes * GridRes].push_back(t);
							}
						}
					}
				}

				/* Ray-triangle intersection (Möller–Trumbore). */
				auto rayTriangleIntersect = [&highVerts] (const Math::Vector< 3, vertex_data_t > & origin, const Math::Vector< 3, vertex_data_t > & dir, const ShapeTriangle< vertex_data_t, index_data_t > & tri, vertex_data_t & outT, vertex_data_t & outU, vertex_data_t & outV) -> bool
				{
					constexpr auto eps = static_cast< vertex_data_t >(1e-7);

					const auto & v0 = highVerts[tri.vertexIndex(0)].position();
					const auto & v1 = highVerts[tri.vertexIndex(1)].position();
					const auto & v2 = highVerts[tri.vertexIndex(2)].position();

					const auto e1 = v1 - v0;
					const auto e2 = v2 - v0;
					const auto h = Math::Vector< 3, vertex_data_t >::crossProduct(dir, e2);
					const auto a = Math::Vector< 3, vertex_data_t >::dotProduct(e1, h);

					if ( std::abs(a) < eps )
					{
						return false;
					}

					const auto f = static_cast< vertex_data_t >(1) / a;
					const auto s = origin - v0;
					outU = f * Math::Vector< 3, vertex_data_t >::dotProduct(s, h);

					if ( outU < 0 || outU > 1 )
					{
						return false;
					}

					const auto q = Math::Vector< 3, vertex_data_t >::crossProduct(s, e1);
					outV = f * Math::Vector< 3, vertex_data_t >::dotProduct(dir, q);

					if ( outV < 0 || outU + outV > 1 )
					{
						return false;
					}

					outT = f * Math::Vector< 3, vertex_data_t >::dotProduct(e2, q);

					return true;
				};

				/* Build a 2D grid over UV space for fast texel → triangle lookup. */
				constexpr size_t UVGridRes = 64;
				std::vector< std::vector< size_t > > uvGrid(UVGridRes * UVGridRes);

				for ( size_t t = 0; t < lowTris.size(); ++t )
				{
					const auto & tri = lowTris[t];
					const auto & tc0 = lowVerts[tri.vertexIndex(0)].textureCoordinates();
					const auto & tc1 = lowVerts[tri.vertexIndex(1)].textureCoordinates();
					const auto & tc2 = lowVerts[tri.vertexIndex(2)].textureCoordinates();

					auto uvMinU = std::min({tc0[Math::X], tc1[Math::X], tc2[Math::X]});
					auto uvMaxU = std::max({tc0[Math::X], tc1[Math::X], tc2[Math::X]});
					auto uvMinV = std::min({tc0[Math::Y], tc1[Math::Y], tc2[Math::Y]});
					auto uvMaxV = std::max({tc0[Math::Y], tc1[Math::Y], tc2[Math::Y]});

					auto cellMinU = static_cast< size_t >(std::clamp(uvMinU * static_cast< vertex_data_t >(UVGridRes), static_cast< vertex_data_t >(0), static_cast< vertex_data_t >(UVGridRes - 1)));
					auto cellMaxU = static_cast< size_t >(std::clamp(uvMaxU * static_cast< vertex_data_t >(UVGridRes), static_cast< vertex_data_t >(0), static_cast< vertex_data_t >(UVGridRes - 1)));
					auto cellMinV = static_cast< size_t >(std::clamp(uvMinV * static_cast< vertex_data_t >(UVGridRes), static_cast< vertex_data_t >(0), static_cast< vertex_data_t >(UVGridRes - 1)));
					auto cellMaxV = static_cast< size_t >(std::clamp(uvMaxV * static_cast< vertex_data_t >(UVGridRes), static_cast< vertex_data_t >(0), static_cast< vertex_data_t >(UVGridRes - 1)));

					for ( size_t cv = cellMinV; cv <= cellMaxV; ++cv )
					{
						for ( size_t cu = cellMinU; cu <= cellMaxU; ++cu )
						{
							uvGrid[cv * UVGridRes + cu].push_back(t);
						}
					}
				}

				/* For each texel, find the corresponding low-poly triangle, ray-cast to high-poly.
				 * Parallelized by row — each row writes to independent pixels. */
				const auto invRes = static_cast< vertex_data_t >(1) / static_cast< vertex_data_t >(resolution);

				auto processRow = [&] (uint32_t py)
				{
					for ( uint32_t px = 0; px < resolution; ++px )
					{
						const auto u = (static_cast< vertex_data_t >(px) + static_cast< vertex_data_t >(0.5)) * invRes;
						const auto v = (static_cast< vertex_data_t >(py) + static_cast< vertex_data_t >(0.5)) * invRes;

						/* Find which low-poly triangle contains this UV using the grid. */
						bool found = false;
						Math::Vector< 3, vertex_data_t > worldPos;
						Math::Vector< 3, vertex_data_t > lowNormal;
						Math::Vector< 3, vertex_data_t > lowTangent;
						Math::Vector< 3, vertex_data_t > lowBitangent;

						const auto cellU = static_cast< size_t >(std::clamp(u * static_cast< vertex_data_t >(UVGridRes), static_cast< vertex_data_t >(0), static_cast< vertex_data_t >(UVGridRes - 1)));
						const auto cellV = static_cast< size_t >(std::clamp(v * static_cast< vertex_data_t >(UVGridRes), static_cast< vertex_data_t >(0), static_cast< vertex_data_t >(UVGridRes - 1)));

						for ( const auto t : uvGrid[cellV * UVGridRes + cellU] )
						{
							if ( found )
							{
								break;
							}
							const auto & tri = lowTris[t];
							const auto & tc0 = lowVerts[tri.vertexIndex(0)].textureCoordinates();
							const auto & tc1 = lowVerts[tri.vertexIndex(1)].textureCoordinates();
							const auto & tc2 = lowVerts[tri.vertexIndex(2)].textureCoordinates();

							/* Barycentric coordinates in UV space. */
							const auto d00 = (tc1[Math::X] - tc0[Math::X]) * (tc1[Math::X] - tc0[Math::X]) + (tc1[Math::Y] - tc0[Math::Y]) * (tc1[Math::Y] - tc0[Math::Y]);
							const auto d01 = (tc1[Math::X] - tc0[Math::X]) * (tc2[Math::X] - tc0[Math::X]) + (tc1[Math::Y] - tc0[Math::Y]) * (tc2[Math::Y] - tc0[Math::Y]);
							const auto d11 = (tc2[Math::X] - tc0[Math::X]) * (tc2[Math::X] - tc0[Math::X]) + (tc2[Math::Y] - tc0[Math::Y]) * (tc2[Math::Y] - tc0[Math::Y]);
							const auto d20 = (u - tc0[Math::X]) * (tc1[Math::X] - tc0[Math::X]) + (v - tc0[Math::Y]) * (tc1[Math::Y] - tc0[Math::Y]);
							const auto d21 = (u - tc0[Math::X]) * (tc2[Math::X] - tc0[Math::X]) + (v - tc0[Math::Y]) * (tc2[Math::Y] - tc0[Math::Y]);

							const auto denom = d00 * d11 - d01 * d01;

							if ( std::abs(denom) < static_cast< vertex_data_t >(1e-10) )
							{
								continue;
							}

							const auto baryV = (d11 * d20 - d01 * d21) / denom;
							const auto baryW = (d00 * d21 - d01 * d20) / denom;
							const auto baryU = static_cast< vertex_data_t >(1) - baryV - baryW;

							if ( baryU < 0 || baryV < 0 || baryW < 0 )
							{
								continue;
							}

							/* Interpolate 3D position and TBN from the low-poly. */
							const auto & p0 = lowVerts[tri.vertexIndex(0)].position();
							const auto & p1 = lowVerts[tri.vertexIndex(1)].position();
							const auto & p2 = lowVerts[tri.vertexIndex(2)].position();

							worldPos = p0 * baryU + p1 * baryV + p2 * baryW;

							const auto & n0 = lowVerts[tri.vertexIndex(0)].normal();
							const auto & n1 = lowVerts[tri.vertexIndex(1)].normal();
							const auto & n2 = lowVerts[tri.vertexIndex(2)].normal();

							lowNormal = (n0 * baryU + n1 * baryV + n2 * baryW).normalized();

							const auto & t0 = lowVerts[tri.vertexIndex(0)].tangent();
							const auto & t1 = lowVerts[tri.vertexIndex(1)].tangent();
							const auto & t2 = lowVerts[tri.vertexIndex(2)].tangent();

							lowTangent = (t0 * baryU + t1 * baryV + t2 * baryW).normalized();
							lowBitangent = Math::Vector< 3, vertex_data_t >::crossProduct(lowNormal, lowTangent).normalized();

							found = true;
						}

						if ( !found )
						{
							continue;
						}

						/* Ray-cast along the low-poly normal (both directions) to find the high-poly surface. */
						vertex_data_t bestDist = std::numeric_limits< vertex_data_t >::max();
						Math::Vector< 3, vertex_data_t > highNormal = lowNormal;
						bool hit = false;

						/* Check cells around the world position. */
						const auto cellIdx = gridIndex(worldPos);
						const size_t cx = cellIdx % GridRes;
						const size_t cy = (cellIdx / GridRes) % GridRes;
						const size_t cz = cellIdx / (GridRes * GridRes);

						const size_t searchRadius = 1;
						const size_t sMinX = (cx > searchRadius) ? cx - searchRadius : 0;
						const size_t sMinY = (cy > searchRadius) ? cy - searchRadius : 0;
						const size_t sMinZ = (cz > searchRadius) ? cz - searchRadius : 0;
						const size_t sMaxX = std::min(cx + searchRadius, GridRes - 1);
						const size_t sMaxY = std::min(cy + searchRadius, GridRes - 1);
						const size_t sMaxZ = std::min(cz + searchRadius, GridRes - 1);

						for ( size_t sz = sMinZ; sz <= sMaxZ; ++sz )
						{
							for ( size_t sy = sMinY; sy <= sMaxY; ++sy )
							{
								for ( size_t sx = sMinX; sx <= sMaxX; ++sx )
								{
									for ( const auto highTriIdx : grid[sx + sy * GridRes + sz * GridRes * GridRes] )
									{
										const auto & highTri = highTris[highTriIdx];
										vertex_data_t tHit, uHit, vHit;

										/* Try both directions along the normal. */
										if ( rayTriangleIntersect(worldPos, lowNormal, highTri, tHit, uHit, vHit) && std::abs(tHit) < bestDist )
										{
											bestDist = std::abs(tHit);

											const auto & hn0 = highVerts[highTri.vertexIndex(0)].normal();
											const auto & hn1 = highVerts[highTri.vertexIndex(1)].normal();
											const auto & hn2 = highVerts[highTri.vertexIndex(2)].normal();

											highNormal = (hn0 * (static_cast< vertex_data_t >(1) - uHit - vHit) + hn1 * uHit + hn2 * vHit).normalized();
											hit = true;
										}

										const auto negDir = lowNormal * static_cast< vertex_data_t >(-1);

										if ( rayTriangleIntersect(worldPos, negDir, highTri, tHit, uHit, vHit) && std::abs(tHit) < bestDist )
										{
											bestDist = std::abs(tHit);

											const auto & hn0 = highVerts[highTri.vertexIndex(0)].normal();
											const auto & hn1 = highVerts[highTri.vertexIndex(1)].normal();
											const auto & hn2 = highVerts[highTri.vertexIndex(2)].normal();

											highNormal = (hn0 * (static_cast< vertex_data_t >(1) - uHit - vHit) + hn1 * uHit + hn2 * vHit).normalized();
											hit = true;
										}
									}
								}
							}
						}

						/* Convert high-poly normal to tangent space. */
						Math::Vector< 3, vertex_data_t > tsNormal;

						if ( hit )
						{
							tsNormal = {
								Math::Vector< 3, vertex_data_t >::dotProduct(highNormal, lowTangent),
								Math::Vector< 3, vertex_data_t >::dotProduct(highNormal, lowBitangent),
								Math::Vector< 3, vertex_data_t >::dotProduct(highNormal, lowNormal)
							};

							tsNormal = tsNormal.normalized();
						}
						else
						{
							tsNormal = {0, 0, 1}; /* Default: no detail (flat). */
						}

						/* Encode to [0,255]: tangent-space normal [-1,1] → [0,1] → [0,255]. */
						const Color< float > normalColor{
							tsNormal[Math::X] * 0.5F + 0.5F,
							tsNormal[Math::Y] * 0.5F + 0.5F,
							tsNormal[Math::Z] * 0.5F + 0.5F,
							1.0F
						};

						normalMap.setPixel(px, py, normalColor);
					}
				};

				if ( m_threadPool != nullptr )
				{
					m_threadPool->parallelFor(uint32_t{0}, resolution, processRow);
				}
				else
				{
					for ( uint32_t py = 0; py < resolution; ++py )
					{
						processRow(py);
					}
				}

				/* Dilation pass: extend baked pixels into empty space to eliminate UV seams.
				 * Empty pixels have the default normal (0.5, 0.5, 1.0, 1.0) = RGB(128, 128, 255). */
				dilateNormalMap(normalMap, 8);

				return normalMap;
			}

			/**
			 * @brief Dilates a normal map by extending baked texels into empty space.
			 * @param normalMap The normal map to dilate in-place.
			 * @param iterations Number of dilation passes (each extends by 1 pixel).
			 */
			static
			void
			dilateNormalMap (PixelFactory::Pixmap< uint8_t > & normalMap, uint32_t iterations) noexcept
			{
				const auto width = normalMap.width();
				const auto height = normalMap.height();
				const auto colorCount = normalMap.colorCount();

				/* Detect empty pixels: RGB close to (128, 128, 255) = default flat normal. */
				std::vector< bool > filled(static_cast< size_t >(width) * height, false);
				auto & data = normalMap.data();

				for ( uint32_t y = 0; y < height; ++y )
				{
					for ( uint32_t x = 0; x < width; ++x )
					{
						const auto offset = (static_cast< size_t >(y) * width + x) * colorCount;
						const auto r = data[offset];
						const auto g = data[offset + 1];
						const auto b = data[offset + 2];

						/* A pixel is "filled" if it differs from the default (128, 128, 255). */
						filled[static_cast< size_t >(y) * width + x] = !(r == 128 && g == 128 && b == 255);
					}
				}

				/* Offsets for 8-connected neighbors. */
				constexpr int dx[8] = {-1, 0, 1, -1, 1, -1, 0, 1};
				constexpr int dy[8] = {-1, -1, -1, 0, 0, 1, 1, 1};

				for ( uint32_t iter = 0; iter < iterations; ++iter )
				{
					std::vector< bool > newFilled = filled;
					auto dataCopy = data;

					for ( uint32_t y = 0; y < height; ++y )
					{
						for ( uint32_t x = 0; x < width; ++x )
						{
							const auto idx = static_cast< size_t >(y) * width + x;

							if ( filled[idx] )
							{
								continue;
							}

							/* Find the first filled neighbor. */
							for ( int n = 0; n < 8; ++n )
							{
								const auto nx = static_cast< int >(x) + dx[n];
								const auto ny = static_cast< int >(y) + dy[n];

								if ( nx < 0 || nx >= static_cast< int >(width) || ny < 0 || ny >= static_cast< int >(height) )
								{
									continue;
								}

								const auto nIdx = static_cast< size_t >(ny) * width + static_cast< size_t >(nx);

								if ( filled[nIdx] )
								{
									const auto srcOffset = nIdx * colorCount;
									const auto dstOffset = idx * colorCount;

									for ( uint32_t c = 0; c < colorCount; ++c )
									{
										dataCopy[dstOffset + c] = data[srcOffset + c];
									}

									newFilled[idx] = true;

									break;
								}
							}
						}
					}

					data = std::move(dataCopy);
					filled = std::move(newFilled);
				}
			}

			const Shape< vertex_data_t, index_data_t > & m_source;
			const Shape< vertex_data_t, index_data_t > & m_bakeSource;
			vertex_data_t m_ratio;
			vertex_data_t m_boundaryPenaltyWeight;
			uint32_t m_normalMapResolution{0};
			ThreadPool * m_threadPool{nullptr};
			mutable PixelFactory::Pixmap< uint8_t > m_normalMap;
	};
}
