/*
 * src/Libs/VertexFactory/ShapeSplitter.hpp
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
#include <cstdint>
#include <iostream>
#include <queue>
#include <type_traits>
#include <unordered_map>
#include <vector>

/* Local inclusions. */
#include "Libs/Math/Plane.hpp"
#include "Shape.hpp"
#include "ShapeProcessor.hpp"

namespace EmEn::Libs::VertexFactory
{
	/**
	 * @brief The result of a shape split operation.
	 * @tparam vertex_data_t The precision type of vertex data. Default float.
	 * @tparam index_data_t The precision type of index data. Default uint32_t.
	 */
	template< typename vertex_data_t = float, typename index_data_t = uint32_t >
	requires (std::is_floating_point_v< vertex_data_t > && std::is_unsigned_v< index_data_t >)
	struct SplitResult final
	{
		std::vector< Shape< vertex_data_t, index_data_t > > frontParts;
		std::vector< Shape< vertex_data_t, index_data_t > > backParts;
		bool wasSplit{false};
	};

	/**
	 * @brief Splits a shape by a plane and optionally extracts connected components.
	 * @note This algorithm only cuts geometry — it does NOT cap/seal the open surfaces along the cutting plane.
	 * @tparam vertex_data_t The precision type of vertex data. Default float.
	 * @tparam index_data_t The precision type of index data. Default uint32_t.
	 */
	template< typename vertex_data_t = float, typename index_data_t = uint32_t >
	requires (std::is_floating_point_v< vertex_data_t > && std::is_unsigned_v< index_data_t >)
	class ShapeSplitter final
	{
		public:

			/**
			 * @brief Constructs a splitter for the given source shape and cutting plane.
			 * @param source A const reference to the source shape. Must remain valid for the lifetime of this object.
			 * @param plane The cutting plane.
			 * @param epsilon Tolerance for vertex-on-plane classification. Default 1e-5.
			 */
			ShapeSplitter (const Shape< vertex_data_t, index_data_t > & source, const Math::Plane< vertex_data_t > & plane, vertex_data_t epsilon = static_cast< vertex_data_t >(1e-5), bool sealCut = false) noexcept
				: m_source(source), m_plane(plane), m_epsilon(epsilon), m_sealCut(sealCut)
			{

			}

			/** @brief Copy constructor. */
			ShapeSplitter (const ShapeSplitter &) noexcept = delete;

			/** @brief Move constructor. */
			ShapeSplitter (ShapeSplitter &&) noexcept = delete;

			/** @brief Copy assignment. */
			ShapeSplitter & operator= (const ShapeSplitter &) noexcept = delete;

			/** @brief Move assignment. */
			ShapeSplitter & operator= (ShapeSplitter &&) noexcept = delete;

			/** @brief Destructor. */
			~ShapeSplitter () = default;

			/**
			 * @brief Performs the split operation.
			 * @return SplitResult< vertex_data_t, index_data_t >
			 */
			[[nodiscard]]
			SplitResult< vertex_data_t, index_data_t >
			split () const noexcept
			{
				SplitResult< vertex_data_t, index_data_t > result;

				if ( m_source.empty() )
				{
					return result;
				}

				Shape< vertex_data_t, index_data_t > frontShape;
				Shape< vertex_data_t, index_data_t > backShape;

				const bool didSplit = this->splitByPlane(frontShape, backShape);

				result.wasSplit = didSplit;

				if ( !frontShape.empty() )
				{
					result.frontParts.emplace_back(std::move(frontShape));
				}

				if ( !backShape.empty() )
				{
					result.backParts.emplace_back(std::move(backShape));
				}

				return result;
			}

			/**
			 * @brief Extracts disconnected components from a shape into separate shapes.
			 * @param source The source shape to decompose.
			 * @return std::vector< Shape< vertex_data_t, index_data_t > >
			 */
			[[nodiscard]]
			static
			std::vector< Shape< vertex_data_t, index_data_t > >
			extractConnectedComponents (const Shape< vertex_data_t, index_data_t > & source) noexcept
			{
				if ( source.empty() )
				{
					return {};
				}

				const auto & triangles = source.triangles();
				const auto triangleCount = triangles.size();

				/* Build vertex → triangle adjacency. */
				std::unordered_map< index_data_t, std::vector< size_t > > vertexToTriangles;

				for ( size_t triIdx = 0; triIdx < triangleCount; ++triIdx )
				{
					const auto & tri = triangles[triIdx];

					vertexToTriangles[tri.vertexIndex(0)].push_back(triIdx);
					vertexToTriangles[tri.vertexIndex(1)].push_back(triIdx);
					vertexToTriangles[tri.vertexIndex(2)].push_back(triIdx);
				}

				/* BFS to find connected components. */
				std::vector< bool > visited(triangleCount, false);
				std::vector< std::vector< size_t > > components;

				for ( size_t startTri = 0; startTri < triangleCount; ++startTri )
				{
					if ( visited[startTri] )
					{
						continue;
					}

					auto & component = components.emplace_back();

					std::queue< size_t > frontier;
					frontier.push(startTri);
					visited[startTri] = true;

					while ( !frontier.empty() )
					{
						const auto currentTri = frontier.front();
						frontier.pop();

						component.push_back(currentTri);

						const auto & tri = triangles[currentTri];

						for ( index_data_t v = 0; v < 3; ++v )
						{
							const auto vertIdx = tri.vertexIndex(v);
							const auto it = vertexToTriangles.find(vertIdx);

							if ( it == vertexToTriangles.end() )
							{
								continue;
							}

							for ( const auto neighborTri : it->second )
							{
								if ( !visited[neighborTri] )
								{
									visited[neighborTri] = true;
									frontier.push(neighborTri);
								}
							}
						}
					}
				}

				if ( components.size() == 1 )
				{
					return { source };
				}

				std::vector< Shape< vertex_data_t, index_data_t > > result;
				result.reserve(components.size());

				const auto groupLookup = buildGroupLookup(source);

				for ( const auto & component : components )
				{
					auto sortedIndices = component;

					std::ranges::sort(sortedIndices);

					result.emplace_back(buildShapeFromTriangles(source, sortedIndices, groupLookup));
				}

				return result;
			}

		private:

			/* ---- Internal types ---- */

			struct EdgeKey
			{
				index_data_t low;
				index_data_t high;

				bool operator== (const EdgeKey & other) const noexcept { return low == other.low && high == other.high; }
			};

			struct EdgeKeyHash
			{
				size_t operator() (const EdgeKey & key) const noexcept
				{
					return std::hash< index_data_t >{}(key.low) ^ (std::hash< index_data_t >{}(key.high) << 1);
				}
			};

			struct InterpolatedAttributes
			{
				Math::Vector< 3, vertex_data_t > position;
				Math::Vector< 3, vertex_data_t > tangent;
				Math::Vector< 3, vertex_data_t > normal;
				Math::Vector< 3, vertex_data_t > textureCoordinates;
				Math::Vector< 4, int32_t > influences;
				Math::Vector< 4, vertex_data_t > weights;
				Math::Vector< 4, vertex_data_t > color;
			};

			struct OutputContext
			{
				Shape< vertex_data_t, index_data_t > * shape{nullptr};
				std::unordered_map< index_data_t, index_data_t > vertexCache;
				std::unordered_map< EdgeKey, index_data_t, EdgeKeyHash > edgeCache;
				std::unordered_map< index_data_t, index_data_t > colorCache;
				std::vector< std::pair< index_data_t, index_data_t > > boundaryEdges;
				int currentGroupIndex{-1};
			};

			/* ---- Static utilities ---- */

			[[nodiscard]]
			static
			EdgeKey
			makeEdgeKey (index_data_t a, index_data_t b) noexcept
			{
				return a < b ? EdgeKey{a, b} : EdgeKey{b, a};
			}

			[[nodiscard]]
			static
			std::vector< int >
			buildGroupLookup (const Shape< vertex_data_t, index_data_t > & source) noexcept
			{
				const auto & groups = source.groups();
				const auto triCount = source.triangles().size();

				std::vector< int > lookup(triCount, 0);

				for ( size_t groupIndex = 0; groupIndex < groups.size(); ++groupIndex )
				{
					const auto offset = static_cast< size_t >(groups[groupIndex].first);
					const auto count = static_cast< size_t >(groups[groupIndex].second);

					for ( size_t index = 0; index < count && (offset + index) < triCount; ++index )
					{
						lookup[offset + index] = static_cast< int >(groupIndex);
					}
				}

				return lookup;
			}

			static
			index_data_t
			getOrCopyVertex (const Shape< vertex_data_t, index_data_t > & source, index_data_t srcVertexIdx, OutputContext & ctx) noexcept
			{
				auto it = ctx.vertexCache.find(srcVertexIdx);

				if ( it != ctx.vertexCache.end() )
				{
					return it->second;
				}

				const auto & srcVertex = source.vertex(srcVertexIdx);

				auto dstIdx = ctx.shape->saveVertex(srcVertex.position(), srcVertex.normal(), srcVertex.textureCoordinates());

				ctx.shape->vertices()[dstIdx].setTangent(srcVertex.tangent());
				ctx.shape->vertices()[dstIdx].setInfluences(
					srcVertex.influences()[Math::X], srcVertex.influences()[Math::Y],
					srcVertex.influences()[Math::Z], srcVertex.influences()[Math::W]
				);
				ctx.shape->vertices()[dstIdx].setWeights(
					srcVertex.weights()[Math::X], srcVertex.weights()[Math::Y],
					srcVertex.weights()[Math::Z], srcVertex.weights()[Math::W]
				);

				ctx.vertexCache[srcVertexIdx] = dstIdx;

				return dstIdx;
			}

			static
			index_data_t
			getOrCopyColor (const Shape< vertex_data_t, index_data_t > & source, index_data_t srcColorIdx, OutputContext & ctx) noexcept
			{
				auto it = ctx.colorCache.find(srcColorIdx);

				if ( it != ctx.colorCache.end() )
				{
					return it->second;
				}

				index_data_t dstIdx = 0;

				if ( !source.vertexColors().empty() )
				{
					dstIdx = ctx.shape->saveVertexColor(source.vertexColor(srcColorIdx));
				}
				else
				{
					dstIdx = ctx.shape->saveVertexColor({});
				}

				ctx.colorCache[srcColorIdx] = dstIdx;

				return dstIdx;
			}

			static
			index_data_t
			getOrCreateIntersectionVertex (const InterpolatedAttributes & attrs, const EdgeKey & edgeKey, OutputContext & ctx) noexcept
			{
				auto it = ctx.edgeCache.find(edgeKey);

				if ( it != ctx.edgeCache.end() )
				{
					return it->second;
				}

				auto dstIdx = ctx.shape->saveVertex(attrs.position, attrs.normal, attrs.textureCoordinates);

				ctx.shape->vertices()[dstIdx].setTangent(attrs.tangent);
				ctx.shape->vertices()[dstIdx].setInfluences(
					attrs.influences[Math::X], attrs.influences[Math::Y],
					attrs.influences[Math::Z], attrs.influences[Math::W]
				);
				ctx.shape->vertices()[dstIdx].setWeights(
					attrs.weights[Math::X], attrs.weights[Math::Y],
					attrs.weights[Math::Z], attrs.weights[Math::W]
				);

				ctx.edgeCache[edgeKey] = dstIdx;

				return dstIdx;
			}

			static
			index_data_t
			getOrCreateIntersectionColor (const InterpolatedAttributes & attrs, OutputContext & ctx) noexcept
			{
				return ctx.shape->saveVertexColor(attrs.color);
			}

			static
			void
			emitTriangle (OutputContext & ctx, int groupIndex, index_data_t v0, index_data_t v1, index_data_t v2, index_data_t c0, index_data_t c1, index_data_t c2, const Math::Vector< 3, vertex_data_t > & surfaceNormal = {}, const Math::Vector< 3, vertex_data_t > & surfaceTangent = {}) noexcept
			{
				if ( groupIndex != ctx.currentGroupIndex )
				{
					if ( ctx.currentGroupIndex >= 0 && !ctx.shape->triangles().empty() )
					{
						ctx.shape->newGroup();
					}

					ctx.currentGroupIndex = groupIndex;
				}

				ShapeTriangle< vertex_data_t, index_data_t > triangle(v0, v1, v2);
				triangle.setVertexColorIndex(0, c0);
				triangle.setVertexColorIndex(1, c1);
				triangle.setVertexColorIndex(2, c2);
				triangle.setSurfaceNormal(surfaceNormal);
				triangle.setSurfaceTangent(surfaceTangent);

				ctx.shape->triangles().emplace_back(triangle);

				if ( auto & groups = ctx.shape->groups(); groups.empty() )
				{
					groups.emplace_back(0, 1);
				}
				else
				{
					++groups.back().second;
				}
			}

			static
			void
			emitSourceTriangle (const Shape< vertex_data_t, index_data_t > & source, const ShapeTriangle< vertex_data_t, index_data_t > & tri, int groupIndex, OutputContext & ctx) noexcept
			{
				const auto v0 = getOrCopyVertex(source, tri.vertexIndex(0), ctx);
				const auto v1 = getOrCopyVertex(source, tri.vertexIndex(1), ctx);
				const auto v2 = getOrCopyVertex(source, tri.vertexIndex(2), ctx);

				const auto c0 = getOrCopyColor(source, tri.vertexColorIndex(0), ctx);
				const auto c1 = getOrCopyColor(source, tri.vertexColorIndex(1), ctx);
				const auto c2 = getOrCopyColor(source, tri.vertexColorIndex(2), ctx);

				emitTriangle(ctx, groupIndex, v0, v1, v2, c0, c1, c2, tri.surfaceNormal(), tri.surfaceTangent());
			}

			[[nodiscard]]
			static
			Shape< vertex_data_t, index_data_t >
			buildShapeFromTriangles (const Shape< vertex_data_t, index_data_t > & source, const std::vector< size_t > & triangleIndices, const std::vector< int > & groupLookup) noexcept
			{
				Shape< vertex_data_t, index_data_t > output;

				OutputContext ctx{&output, {}, {}, {}, {}, -1};

				for ( const auto triIdx : triangleIndices )
				{
					const auto groupIdx = (triIdx < groupLookup.size()) ? groupLookup[triIdx] : 0;

					emitSourceTriangle(source, source.triangles()[triIdx], groupIdx, ctx);
				}

				if ( !output.empty() )
				{
					output.updateProperties();
				}

				return output;
			}

			/* ---- Instance methods ---- */

			[[nodiscard]]
			InterpolatedAttributes
			interpolateVertexAttributes (index_data_t idxA, index_data_t idxB, vertex_data_t t, index_data_t colorIdxA, index_data_t colorIdxB) const noexcept
			{
				const auto & vertA = m_source.vertex(idxA);
				const auto & vertB = m_source.vertex(idxB);

				InterpolatedAttributes result;

				result.position = vertA.position() + (vertB.position() - vertA.position()) * t;
				result.textureCoordinates = vertA.textureCoordinates() + (vertB.textureCoordinates() - vertA.textureCoordinates()) * t;
				result.normal = (vertA.normal() + (vertB.normal() - vertA.normal()) * t).normalized();
				result.tangent = (vertA.tangent() + (vertB.tangent() - vertA.tangent()) * t).normalized();
				result.weights = vertA.weights() + (vertB.weights() - vertA.weights()) * t;
				result.influences = (t < static_cast< vertex_data_t >(0.5)) ? vertA.influences() : vertB.influences();

				if ( !m_source.vertexColors().empty() )
				{
					const auto & colorA = m_source.vertexColor(colorIdxA);
					const auto & colorB = m_source.vertexColor(colorIdxB);

					result.color = colorA + (colorB - colorA) * t;
				}

				return result;
			}

			void
			handleEdgeCaseTriangle (const ShapeTriangle< vertex_data_t, index_data_t > & tri, const index_data_t vIdx[3], const index_data_t cIdx[3], const vertex_data_t dist[3], const int side[3], int groupIndex, OutputContext & frontCtx, OutputContext & backCtx, std::unordered_map< EdgeKey, InterpolatedAttributes, EdgeKeyHash > & attrCache) const noexcept
			{
				int onPlane = -1;
				int pos = -1;
				int neg = -1;

				for ( int index = 0; index < 3; ++index )
				{
					if ( side[index] == 0 )
					{
						onPlane = index;
					}
					else if ( side[index] > 0 )
					{
						pos = index;
					}
					else
					{
						neg = index;
					}
				}

				if ( onPlane != -1 && (pos == -1 || neg == -1) )
				{
					if ( pos != -1 )
					{
						emitSourceTriangle(m_source, tri, groupIndex, frontCtx);
					}
					else
					{
						emitSourceTriangle(m_source, tri, groupIndex, backCtx);
					}

					return;
				}

				if ( onPlane == -1 || pos == -1 || neg == -1 )
				{
					emitSourceTriangle(m_source, tri, groupIndex, frontCtx);
					return;
				}

				const vertex_data_t t = dist[pos] / (dist[pos] - dist[neg]);
				const auto edgeKey = makeEdgeKey(vIdx[pos], vIdx[neg]);

				if ( !attrCache.contains(edgeKey) )
				{
					attrCache[edgeKey] = this->interpolateVertexAttributes(vIdx[pos], vIdx[neg], t, cIdx[pos], cIdx[neg]);
				}

				const auto & attrs = attrCache[edgeKey];

				const auto fVOn = getOrCopyVertex(m_source, vIdx[onPlane], frontCtx);
				const auto fVPos = getOrCopyVertex(m_source, vIdx[pos], frontCtx);
				const auto fVI = getOrCreateIntersectionVertex(attrs, edgeKey, frontCtx);
				const auto fCOn = getOrCopyColor(m_source, cIdx[onPlane], frontCtx);
				const auto fCPos = getOrCopyColor(m_source, cIdx[pos], frontCtx);
				const auto fCI = getOrCreateIntersectionColor(attrs, frontCtx);

				const auto bVOn = getOrCopyVertex(m_source, vIdx[onPlane], backCtx);
				const auto bVNeg = getOrCopyVertex(m_source, vIdx[neg], backCtx);
				const auto bVI = getOrCreateIntersectionVertex(attrs, edgeKey, backCtx);
				const auto bCOn = getOrCopyColor(m_source, cIdx[onPlane], backCtx);
				const auto bCNeg = getOrCopyColor(m_source, cIdx[neg], backCtx);
				const auto bCI = getOrCreateIntersectionColor(attrs, backCtx);

				/* Record boundary edge: on-plane vertex ↔ intersection vertex. */
				frontCtx.boundaryEdges.emplace_back(fVOn, fVI);
				backCtx.boundaryEdges.emplace_back(bVOn, bVI);

				if ( (onPlane + 1) % 3 == pos )
				{
					emitTriangle(frontCtx, groupIndex, fVOn, fVPos, fVI, fCOn, fCPos, fCI);
					emitTriangle(backCtx, groupIndex, bVOn, bVI, bVNeg, bCOn, bCI, bCNeg);
				}
				else
				{
					emitTriangle(frontCtx, groupIndex, fVOn, fVI, fVPos, fCOn, fCI, fCPos);
					emitTriangle(backCtx, groupIndex, bVOn, bVNeg, bVI, bCOn, bCNeg, bCI);
				}
			}

			/**
			 * @brief Mirrors boundary loops from one shape to another by position matching.
			 * @note Uses the boundary edge vertex positions from the target to find corresponding indices.
			 * @param source The shape with correct boundary loops (typically the back part).
			 * @param target The shape to receive mirrored boundary loops (typically the front part).
			 * @param targetEdges The raw boundary edges collected for the target shape.
			 */
			static
			void
			mirrorBoundaryLoops (const Shape< vertex_data_t, index_data_t > & source, Shape< vertex_data_t, index_data_t > & target, const std::vector< std::pair< index_data_t, index_data_t > > & targetEdges) noexcept
			{
				if ( source.boundaryLoops().empty() )
				{
					return;
				}

				/* Build a spatial index of all boundary vertex positions in the target shape. */
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

				const auto posScale = static_cast< vertex_data_t >(1) / static_cast< vertex_data_t >(1e-4);

				auto quantize = [] (const Math::Vector< 3, vertex_data_t > & pos, vertex_data_t scale) -> QuantizedPos
				{
					return {
						static_cast< int32_t >(std::round(pos[Math::X] * scale)),
						static_cast< int32_t >(std::round(pos[Math::Y] * scale)),
						static_cast< int32_t >(std::round(pos[Math::Z] * scale))
					};
				};

				/* Collect all unique target boundary vertex indices and map position → index. */
				std::unordered_map< QuantizedPos, index_data_t, QuantizedPosHash > targetPosMap;

				for ( const auto & [a, b] : targetEdges )
				{
					const auto keyA = quantize(target.vertex(a).position(), posScale);
					targetPosMap.try_emplace(keyA, a);

					const auto keyB = quantize(target.vertex(b).position(), posScale);
					targetPosMap.try_emplace(keyB, b);
				}

				/* For each source loop, remap vertex indices to the target shape. */
				for ( const auto & srcLoop : source.boundaryLoops() )
				{
					BoundaryLoop< index_data_t > dstLoop;
					dstLoop.vertexIndices.reserve(srcLoop.vertexIndices.size());

					bool valid = true;

					for ( const auto srcIdx : srcLoop.vertexIndices )
					{
						const auto key = quantize(source.vertex(srcIdx).position(), posScale);
						const auto it = targetPosMap.find(key);

						if ( it == targetPosMap.end() )
						{
							valid = false;

							break;
						}

						dstLoop.vertexIndices.push_back(it->second);
					}

					if ( valid && dstLoop.vertexIndices.size() >= 3 )
					{
						target.boundaryLoops().push_back(std::move(dstLoop));
					}
				}

				target.setBoundaryLoopsAnalyzed();
			}

			/**
			 * @brief Seals boundary loops of a shape using ShapeProcessor.
			 * @param shape The shape to seal.
			 * @param capNormal The outward-facing normal for the cap surface.
			 */
			static
			void
			sealCutSurface (Shape< vertex_data_t, index_data_t > & shape, const Math::Vector< 3, vertex_data_t > & capNormal) noexcept
			{
				ShapeProcessor< vertex_data_t, index_data_t > processor{shape};
				processor.sealAllBoundaryLoops(capNormal);
			}

			/**
			 * @brief Chains collected boundary edges into ordered boundary loops and stores them in the shape.
			 * @param edges The collected boundary edge pairs (vertex index A, vertex index B).
			 * @param shape The output shape to store boundary loops into.
			 */
			static
			void
			chainBoundaryEdges (const std::vector< std::pair< index_data_t, index_data_t > > & edges, Shape< vertex_data_t, index_data_t > & shape) noexcept
			{
				if ( edges.empty() )
				{
					return;
				}

				auto packEdgeDedup = [] (index_data_t a, index_data_t b) -> uint64_t
				{
					const auto lo = std::min(a, b);
					const auto hi = std::max(a, b);

					return (static_cast< uint64_t >(lo) << 32) | static_cast< uint64_t >(hi);
				};

				/* Deduplicate boundary edges (each boundary edge appears once per adjacent triangle). */
				std::unordered_set< uint64_t > seenEdges;
				std::vector< std::pair< index_data_t, index_data_t > > uniqueEdges;
				uniqueEdges.reserve(edges.size() / 2);

				for ( const auto & [a, b] : edges )
				{
					const auto key = packEdgeDedup(a, b);

					if ( !seenEdges.contains(key) )
					{
						seenEdges.insert(key);
						uniqueEdges.emplace_back(a, b);
					}
				}

				/* Build a spatial canonical map: quantize positions to merge vertices
				 * that represent the same point but have different indices
				 * (e.g. from getOrCopyVertex vs getOrCreateIntersectionVertex). */
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

				const auto posScale = static_cast< vertex_data_t >(1) / static_cast< vertex_data_t >(1e-4);

				auto quantize = [&shape, posScale] (index_data_t idx) -> QuantizedPos
				{
					const auto & pos = shape.vertex(idx).position();

					return {
						static_cast< int32_t >(std::round(pos[Math::X] * posScale)),
						static_cast< int32_t >(std::round(pos[Math::Y] * posScale)),
						static_cast< int32_t >(std::round(pos[Math::Z] * posScale))
					};
				};

				/* Map each vertex to its canonical representative. */
				std::unordered_map< QuantizedPos, index_data_t, QuantizedPosHash > posToCanonical;
				std::unordered_map< index_data_t, index_data_t > canonicalMap;

				for ( const auto & [a, b] : uniqueEdges )
				{
					for ( const auto idx : {a, b} )
					{
						if ( !canonicalMap.contains(idx) )
						{
							const auto key = quantize(idx);
							auto [it, inserted] = posToCanonical.try_emplace(key, idx);
							canonicalMap[idx] = it->second;
						}
					}
				}

				/* Remap edges to canonical vertex indices. */
				std::unordered_set< uint64_t > remappedSeen;
				std::vector< std::pair< index_data_t, index_data_t > > canonicalEdges;

				for ( const auto & [a, b] : uniqueEdges )
				{
					const auto ca = canonicalMap[a];
					const auto cb = canonicalMap[b];

					if ( ca == cb )
					{
						continue;
					}

					const auto key = packEdgeDedup(ca, cb);

					if ( !remappedSeen.contains(key) )
					{
						remappedSeen.insert(key);
						canonicalEdges.emplace_back(ca, cb);
					}
				}

				/* Build adjacency: vertex → list of connected vertices. */
				std::unordered_map< index_data_t, std::vector< index_data_t > > adjacency;

				for ( const auto & [a, b] : canonicalEdges )
				{
					adjacency[a].push_back(b);
					adjacency[b].push_back(a);
				}

				/* Get 3D position of a canonical vertex. */
				auto getPos = [&shape] (index_data_t idx) -> const Math::Vector< 3, vertex_data_t > &
				{
					return shape.vertex(idx).position();
				};

				/* Chain edges into loops using angle-based traversal.
				 * At T-junctions (vertices with 3+ neighbors), pick the neighbor
				 * that continues most straight (smallest turning angle). */
				std::unordered_set< uint64_t > usedEdges;

				auto packEdge = [] (index_data_t a, index_data_t b) -> uint64_t
				{
					const auto lo = std::min(a, b);
					const auto hi = std::max(a, b);

					return (static_cast< uint64_t >(lo) << 32) | static_cast< uint64_t >(hi);
				};

				for ( const auto & [startVertex, neighbors] : adjacency )
				{
					bool hasUnused = false;

					for ( const auto neighbor : neighbors )
					{
						if ( !usedEdges.contains(packEdge(startVertex, neighbor)) )
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

					auto current = startVertex;
					index_data_t prev = std::numeric_limits< index_data_t >::max();
					const size_t maxSteps = canonicalEdges.size() * 2 + 1;
					size_t steps = 0;

					do
					{
						loop.vertexIndices.push_back(current);

						const auto & adj = adjacency.at(current);
						index_data_t best = std::numeric_limits< index_data_t >::max();

						/* Prefer closing the loop if close enough. */
						if ( loop.vertexIndices.size() >= 3 )
						{
							for ( const auto neighbor : adj )
							{
								if ( neighbor == startVertex && !usedEdges.contains(packEdge(current, neighbor)) )
								{
									best = neighbor;

									break;
								}
							}
						}

						/* Pick the best unused neighbor by smallest turning angle. */
						if ( best == std::numeric_limits< index_data_t >::max() )
						{
							vertex_data_t bestScore = std::numeric_limits< vertex_data_t >::lowest();

							for ( const auto neighbor : adj )
							{
								if ( usedEdges.contains(packEdge(current, neighbor)) )
								{
									continue;
								}

								if ( prev == std::numeric_limits< index_data_t >::max() )
								{
									/* First step: just pick any unused neighbor. */
									best = neighbor;

									break;
								}

								/* Compute how "straight" this continuation is.
								 * dot(incoming_dir, outgoing_dir) → 1.0 = perfectly straight. */
								const auto incoming = (getPos(current) - getPos(prev)).normalized();
								const auto outgoing = (getPos(neighbor) - getPos(current)).normalized();
								const auto straightness = Math::Vector< 3, vertex_data_t >::dotProduct(incoming, outgoing);

								if ( straightness > bestScore )
								{
									bestScore = straightness;
									best = neighbor;
								}
							}
						}

						if ( best == std::numeric_limits< index_data_t >::max() )
						{
							break;
						}

						usedEdges.insert(packEdge(current, best));
						prev = current;
						current = best;
						++steps;

					} while ( current != startVertex && steps < maxSteps );

					if ( loop.vertexIndices.size() >= 3 )
					{
						shape.boundaryLoops().push_back(std::move(loop));
					}
				}

				/* Merge adjacent loops by finding closest endpoint pairs.
				 * Tests all 4 endpoint combinations and reverses loops as needed. */
				const auto mergeTol = static_cast< vertex_data_t >(1e-3);
				auto & loops = shape.boundaryLoops();
				bool merged = true;

				while ( merged && loops.size() > 1 )
				{
					merged = false;

					for ( size_t i = 0; i < loops.size() && !merged; ++i )
					{
						auto & loopI = loops[i].vertexIndices;

						const auto iFront = shape.vertex(loopI.front()).position();
						const auto iBack = shape.vertex(loopI.back()).position();

						for ( size_t j = i + 1; j < loops.size() && !merged; ++j )
						{
							auto & loopJ = loops[j].vertexIndices;

							const auto jFront = shape.vertex(loopJ.front()).position();
							const auto jBack = shape.vertex(loopJ.back()).position();

							/* Case 1: i.back → j.front (append j to i) */
							if ( (iBack - jFront).length() < mergeTol )
							{
								loopI.insert(loopI.end(), loopJ.begin(), loopJ.end());
								loops.erase(loops.begin() + static_cast< ptrdiff_t >(j));
								merged = true;
							}
							/* Case 2: j.back → i.front (prepend j to i) */
							else if ( (jBack - iFront).length() < mergeTol )
							{
								loopJ.insert(loopJ.end(), loopI.begin(), loopI.end());
								loopI = std::move(loopJ);
								loops.erase(loops.begin() + static_cast< ptrdiff_t >(j));
								merged = true;
							}
							/* Case 3: i.back → j.back (reverse j, then append) */
							else if ( (iBack - jBack).length() < mergeTol )
							{
								std::reverse(loopJ.begin(), loopJ.end());
								loopI.insert(loopI.end(), loopJ.begin(), loopJ.end());
								loops.erase(loops.begin() + static_cast< ptrdiff_t >(j));
								merged = true;
							}
							/* Case 4: i.front → j.front (reverse j, then prepend) */
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

				shape.setBoundaryLoopsAnalyzed();
			}

			bool
			splitByPlane (Shape< vertex_data_t, index_data_t > & frontShape, Shape< vertex_data_t, index_data_t > & backShape) const noexcept
			{
				const auto & triangles = m_source.triangles();
				const auto groupLookup = buildGroupLookup(m_source);

				OutputContext frontCtx{&frontShape, {}, {}, {}, {}, -1};
				OutputContext backCtx{&backShape, {}, {}, {}, {}, -1};

				std::unordered_map< EdgeKey, InterpolatedAttributes, EdgeKeyHash > attrCache;

				bool anyFront = false;
				bool anyBack = false;

				for ( size_t triIdx = 0; triIdx < triangles.size(); ++triIdx )
				{
					const auto & tri = triangles[triIdx];
					const auto groupIdx = (triIdx < groupLookup.size()) ? groupLookup[triIdx] : 0;

					const index_data_t vIdx[3] = { tri.vertexIndex(0), tri.vertexIndex(1), tri.vertexIndex(2) };
					const index_data_t cIdx[3] = { tri.vertexColorIndex(0), tri.vertexColorIndex(1), tri.vertexColorIndex(2) };

					vertex_data_t dist[3];
					int side[3];

					for ( int index = 0; index < 3; ++index )
					{
						dist[index] = m_plane.getSignedDistanceTo(m_source.vertex(vIdx[index]).position());

						if ( dist[index] > m_epsilon )
						{
							side[index] = 1;
						}
						else if ( dist[index] < -m_epsilon )
						{
							side[index] = -1;
						}
						else
						{
							side[index] = 0;
						}
					}

					int frontCount = 0;
					int backCount = 0;

					for ( int index = 0; index < 3; ++index )
					{
						if ( side[index] >= 0 )
						{
							++frontCount;
						}

						if ( side[index] <= 0 )
						{
							++backCount;
						}
					}

					/* Case 1: Entirely on front. */
					if ( backCount == 0 )
					{
						emitSourceTriangle(m_source, tri, groupIdx, frontCtx);
						anyFront = true;

						continue;
					}

					/* Case 2: Entirely on back. */
					if ( frontCount == 0 )
					{
						emitSourceTriangle(m_source, tri, groupIdx, backCtx);
						anyBack = true;

						continue;
					}

					/* Case 3: All vertices on the plane — assign to front. */
					if ( side[0] == 0 && side[1] == 0 && side[2] == 0 )
					{
						emitSourceTriangle(m_source, tri, groupIdx, frontCtx);
						anyFront = true;

						continue;
					}

					/* Case 4: Triangle straddles the plane — clip it. */
					anyFront = true;
					anyBack = true;

					int loneLocal = -1;
					bool loneIsFront = false;

					for ( int index = 0; index < 3; ++index )
					{
						const int next = (index + 1) % 3;
						const int prev = (index + 2) % 3;

						if ( side[index] > 0 && side[next] < 0 && side[prev] < 0 )
						{
							loneLocal = index;
							loneIsFront = true;

							break;
						}

						if ( side[index] < 0 && side[next] > 0 && side[prev] > 0 )
						{
							loneLocal = index;
							loneIsFront = false;

							break;
						}
					}

					if ( loneLocal == -1 )
					{
						this->handleEdgeCaseTriangle(tri, vIdx, cIdx, dist, side, groupIdx, frontCtx, backCtx, attrCache);

						continue;
					}

					const int l = loneLocal;
					const int a = (l + 1) % 3;
					const int b = (l + 2) % 3;

					const vertex_data_t tA = dist[l] / (dist[l] - dist[a]);
					const vertex_data_t tB = dist[l] / (dist[l] - dist[b]);

					const auto edgeKeyA = makeEdgeKey(vIdx[l], vIdx[a]);
					const auto edgeKeyB = makeEdgeKey(vIdx[l], vIdx[b]);

					if ( !attrCache.contains(edgeKeyA) )
					{
						attrCache[edgeKeyA] = this->interpolateVertexAttributes(vIdx[l], vIdx[a], tA, cIdx[l], cIdx[a]);
					}

					if ( !attrCache.contains(edgeKeyB) )
					{
						attrCache[edgeKeyB] = this->interpolateVertexAttributes(vIdx[l], vIdx[b], tB, cIdx[l], cIdx[b]);
					}

					const auto & attrsA = attrCache[edgeKeyA];
					const auto & attrsB = attrCache[edgeKeyB];

					const auto frontIA = getOrCreateIntersectionVertex(attrsA, edgeKeyA, frontCtx);
					const auto frontIB = getOrCreateIntersectionVertex(attrsB, edgeKeyB, frontCtx);
					const auto backIA = getOrCreateIntersectionVertex(attrsA, edgeKeyA, backCtx);
					const auto backIB = getOrCreateIntersectionVertex(attrsB, edgeKeyB, backCtx);

					const auto frontCA = getOrCreateIntersectionColor(attrsA, frontCtx);
					const auto frontCB = getOrCreateIntersectionColor(attrsB, frontCtx);
					const auto backCA = getOrCreateIntersectionColor(attrsA, backCtx);
					const auto backCB = getOrCreateIntersectionColor(attrsB, backCtx);

					/* Record boundary edges (intersection vertex pairs on the cutting plane). */
					frontCtx.boundaryEdges.emplace_back(frontIA, frontIB);
					backCtx.boundaryEdges.emplace_back(backIA, backIB);

					if ( loneIsFront )
					{
						const auto fVL = getOrCopyVertex(m_source, vIdx[l], frontCtx);
						const auto fCL = getOrCopyColor(m_source, cIdx[l], frontCtx);

						emitTriangle(frontCtx, groupIdx, fVL, frontIA, frontIB, fCL, frontCA, frontCB);

						const auto bVA = getOrCopyVertex(m_source, vIdx[a], backCtx);
						const auto bVB = getOrCopyVertex(m_source, vIdx[b], backCtx);
						const auto bCA = getOrCopyColor(m_source, cIdx[a], backCtx);
						const auto bCB = getOrCopyColor(m_source, cIdx[b], backCtx);

						emitTriangle(backCtx, groupIdx, backIA, bVA, bVB, backCA, bCA, bCB);
						emitTriangle(backCtx, groupIdx, backIA, bVB, backIB, backCA, bCB, backCB);
					}
					else
					{
						const auto bVL = getOrCopyVertex(m_source, vIdx[l], backCtx);
						const auto bCL = getOrCopyColor(m_source, cIdx[l], backCtx);

						emitTriangle(backCtx, groupIdx, bVL, backIA, backIB, bCL, backCA, backCB);

						const auto fVA = getOrCopyVertex(m_source, vIdx[a], frontCtx);
						const auto fVB = getOrCopyVertex(m_source, vIdx[b], frontCtx);
						const auto fCA = getOrCopyColor(m_source, cIdx[a], frontCtx);
						const auto fCB = getOrCopyColor(m_source, cIdx[b], frontCtx);

						emitTriangle(frontCtx, groupIdx, frontIA, fVA, fVB, frontCA, fCA, fCB);
						emitTriangle(frontCtx, groupIdx, frontIA, fVB, frontIB, frontCA, fCB, frontCB);
					}
				}

				if ( !frontShape.empty() )
				{
					frontShape.updateProperties();
				}

				if ( !backShape.empty() )
				{
					backShape.updateProperties();
				}

				/* Build boundary loops from the BACK part (cleaner — no on-plane triangle artifacts),
				 * then mirror to the front part by position matching. */
				if ( !backShape.empty() )
				{
					chainBoundaryEdges(backCtx.boundaryEdges, backShape);

					if ( !frontShape.empty() )
					{
						mirrorBoundaryLoops(backShape, frontShape, frontCtx.boundaryEdges);
					}
				}
				else if ( !frontShape.empty() )
				{
					chainBoundaryEdges(frontCtx.boundaryEdges, frontShape);
				}

				if ( m_sealCut )
				{
					if ( !frontShape.empty() )
					{
						sealCutSurface(frontShape, m_plane.normal() * static_cast< vertex_data_t >(-1));
					}

					if ( !backShape.empty() )
					{
						sealCutSurface(backShape, m_plane.normal());
					}
				}

				return anyFront && anyBack;
			}

			/* ---- Members ---- */

			const Shape< vertex_data_t, index_data_t > & m_source;
			Math::Plane< vertex_data_t > m_plane;
			vertex_data_t m_epsilon;
			bool m_sealCut;
	};
}
