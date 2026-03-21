/*
 * src/Libs/VertexFactory/XRayAnalyzer.hpp
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
#include <limits>
#include <type_traits>
#include <vector>

/* Local inclusions. */
#include "Shape.hpp"
#include "Libs/Math/CartesianFrame.hpp"
#include "Libs/Math/Space3D/AACuboid.hpp"
#include "Libs/PixelFactory/Pixmap.hpp"
#include "Libs/ThreadPool.hpp"

namespace EmEn::Libs::VertexFactory
{
	/**
	 * @brief X-Ray cross-section analyzer for shapes.
	 * @note Produces a 2D slice image (like a CT scan) at a given depth through one or more shapes.
	 * White = inside a shape, black = outside.
	 * @tparam vertex_data_t The precision type of vertex data. Default float.
	 * @tparam index_data_t The precision type of index data. Default uint32_t.
	 */
	template< typename vertex_data_t = float, typename index_data_t = uint32_t >
	requires (std::is_floating_point_v< vertex_data_t > && std::is_unsigned_v< index_data_t >)
	class XRayAnalyzer final
	{
		public:

			/**
			 * @brief A shape entry with its spatial position.
			 */
			struct ShapeEntry
			{
				const Shape< vertex_data_t, index_data_t > * shape{nullptr};
				Math::CartesianFrame< vertex_data_t > frame;
			};

			/**
			 * @brief Constructs a default X-Ray analyzer.
			 * @param threadPool Optional thread pool for parallel scanning. Default nullptr.
			 */
			explicit
			XRayAnalyzer (ThreadPool * threadPool = nullptr) noexcept
				: m_threadPool(threadPool)
			{

			}

			/** @brief Copy constructor. */
			XRayAnalyzer (const XRayAnalyzer &) noexcept = delete;

			/** @brief Move constructor. */
			XRayAnalyzer (XRayAnalyzer &&) noexcept = delete;

			/** @brief Copy assignment. */
			XRayAnalyzer & operator= (const XRayAnalyzer &) noexcept = delete;

			/** @brief Move assignment. */
			XRayAnalyzer & operator= (XRayAnalyzer &&) noexcept = delete;

			/** @brief Destructor. */
			~XRayAnalyzer () = default;

			/**
			 * @brief Adds a shape with its spatial position to the scene.
			 * @param shape A reference to the shape (must remain valid).
			 * @param frame The spatial position and orientation of the shape.
			 */
			void
			addShape (const Shape< vertex_data_t, index_data_t > & shape, const Math::CartesianFrame< vertex_data_t > & frame = {}) noexcept
			{
				m_shapes.push_back({&shape, frame});
				m_boundsValid = false;
			}

			/**
			 * @brief Sets the viewpoint for scanning.
			 * @param viewpoint The cartesian frame defining the scan direction.
			 *        forward = scan depth axis, right = image X, up = image Y.
			 */
			void
			setViewpoint (const Math::CartesianFrame< vertex_data_t > & viewpoint) noexcept
			{
				m_viewpoint = viewpoint;
				m_boundsValid = false;
			}

			/**
			 * @brief Precomputes transformed triangles and spatial grid for all shapes.
			 * @note Must be called before scan(). Only needs to be called once unless
			 * shapes or viewpoint change.
			 * @param resolution The pixel resolution for the output images.
			 */
			void
			prepare (uint32_t resolution) noexcept
			{
				if ( m_shapes.empty() )
				{
					return;
				}

				computeBounds();

				const auto sliceWidth = m_boundsMax[0] - m_boundsMin[0];
				const auto sliceHeight = m_boundsMax[1] - m_boundsMin[1];

				const auto maxExtent = std::max(sliceWidth, sliceHeight);

				m_squareMinX = m_boundsMin[0] - (maxExtent - sliceWidth) * static_cast< vertex_data_t >(0.5);
				m_squareMinY = m_boundsMin[1] - (maxExtent - sliceHeight) * static_cast< vertex_data_t >(0.5);
				m_squareSize = maxExtent;
				m_sliceDepth = m_boundsMax[2] - m_boundsMin[2];
				m_resolution = resolution;

				m_forward = m_viewpoint.forwardVector();
				m_right = m_viewpoint.rightVector();
				m_up = Math::Vector< 3, vertex_data_t >::crossProduct(m_forward, m_right).normalized();

				/* Transform all triangles to world space once. */
				m_allTriangles.clear();

				for ( const auto & entry : m_shapes )
				{
					const auto & tris = entry.shape->triangles();
					const auto & verts = entry.shape->vertices();

					for ( size_t t = 0; t < tris.size(); ++t )
					{
						TransformedTriangle tt;

						for ( int i = 0; i < 3; ++i )
						{
							tt.v[i] = transformPoint(verts[tris[t].vertexIndex(i)].position(), entry.frame);
						}

						m_allTriangles.push_back(tt);
					}
				}

				/* Build a spatial grid in view-space XY. */
				m_sliceGrid.assign(SliceGridRes * SliceGridRes, {});

				for ( size_t t = 0; t < m_allTriangles.size(); ++t )
				{
					const auto & tt = m_allTriangles[t];

					vertex_data_t minVX = std::numeric_limits< vertex_data_t >::max();
					vertex_data_t maxVX = std::numeric_limits< vertex_data_t >::lowest();
					vertex_data_t minVY = std::numeric_limits< vertex_data_t >::max();
					vertex_data_t maxVY = std::numeric_limits< vertex_data_t >::lowest();

					for ( int i = 0; i < 3; ++i )
					{
						const auto vx = Math::Vector< 3, vertex_data_t >::dotProduct(tt.v[i] - m_viewpoint.position(), m_right);
						const auto vy = Math::Vector< 3, vertex_data_t >::dotProduct(tt.v[i] - m_viewpoint.position(), m_up);

						minVX = std::min(minVX, vx);
						maxVX = std::max(maxVX, vx);
						minVY = std::min(minVY, vy);
						maxVY = std::max(maxVY, vy);
					}

					const auto gridScale = static_cast< vertex_data_t >(SliceGridRes);

					auto cellMinX = static_cast< size_t >(std::clamp((minVX - m_squareMinX) / m_squareSize * gridScale, static_cast< vertex_data_t >(0), static_cast< vertex_data_t >(SliceGridRes - 1)));
					auto cellMaxX = static_cast< size_t >(std::clamp((maxVX - m_squareMinX) / m_squareSize * gridScale, static_cast< vertex_data_t >(0), static_cast< vertex_data_t >(SliceGridRes - 1)));
					auto cellMinY = static_cast< size_t >(std::clamp((minVY - m_squareMinY) / m_squareSize * gridScale, static_cast< vertex_data_t >(0), static_cast< vertex_data_t >(SliceGridRes - 1)));
					auto cellMaxY = static_cast< size_t >(std::clamp((maxVY - m_squareMinY) / m_squareSize * gridScale, static_cast< vertex_data_t >(0), static_cast< vertex_data_t >(SliceGridRes - 1)));

					for ( size_t cy = cellMinY; cy <= cellMaxY; ++cy )
					{
						for ( size_t cx = cellMinX; cx <= cellMaxX; ++cx )
						{
							m_sliceGrid[cy * SliceGridRes + cx].push_back(t);
						}
					}
				}

				m_prepared = true;
			}

			/**
			 * @brief Produces a cross-section slice at the given normalized depth.
			 * @note prepare() must be called first.
			 * @param depth Normalized depth [0.0 = front face, 1.0 = back face] of the bounding volume.
			 * @return PixelFactory::Pixmap< uint8_t > The cross-section image (white = inside, black = outside).
			 */
			[[nodiscard]]
			PixelFactory::Pixmap< uint8_t >
			scan (vertex_data_t depth) const noexcept
			{
				using namespace PixelFactory;

				if ( !m_prepared )
				{
					return {};
				}

				const auto resX = m_resolution;
				const auto resY = m_resolution;

				Pixmap< uint8_t > output(resX, resY, ChannelMode::Grayscale, Color< float >{0.0F, 0.0F, 0.0F, 1.0F});

				const auto depthPos = m_boundsMin[2] + depth * m_sliceDepth;

				auto processRow = [&] (uint32_t py)
				{
					for ( uint32_t px = 0; px < resX; ++px )
					{
						const auto viewX = m_squareMinX + (static_cast< vertex_data_t >(px) + static_cast< vertex_data_t >(0.5)) / static_cast< vertex_data_t >(resX) * m_squareSize;
						const auto viewY = m_squareMinY + (static_cast< vertex_data_t >(py) + static_cast< vertex_data_t >(0.5)) / static_cast< vertex_data_t >(resY) * m_squareSize;

						const auto worldPoint = m_viewpoint.position() + m_right * viewX + m_up * viewY + m_forward * depthPos;

						size_t intersections = 0;

						const auto gridScale = static_cast< vertex_data_t >(SliceGridRes);
						const auto cellX = static_cast< size_t >(std::clamp((viewX - m_squareMinX) / m_squareSize * gridScale, static_cast< vertex_data_t >(0), static_cast< vertex_data_t >(SliceGridRes - 1)));
						const auto cellY = static_cast< size_t >(std::clamp((viewY - m_squareMinY) / m_squareSize * gridScale, static_cast< vertex_data_t >(0), static_cast< vertex_data_t >(SliceGridRes - 1)));

						for ( const auto triIdx : m_sliceGrid[cellY * SliceGridRes + cellX] )
						{
							const auto & tt = m_allTriangles[triIdx];

							if ( rayTriangleIntersect(worldPoint, m_forward, tt.v[0], tt.v[1], tt.v[2]) )
							{
								++intersections;
							}
						}

						if ( intersections % 2 != 0 )
						{
							output.setPixel(px, py, Color< float >{1.0F, 1.0F, 1.0F, 1.0F});
						}
					}
				};

				if ( m_threadPool != nullptr )
				{
					m_threadPool->parallelFor(uint32_t{0}, resY, processRow);
				}
				else
				{
					for ( uint32_t py = 0; py < resY; ++py )
					{
						processRow(py);
					}
				}

				return output;
			}

			/**
			 * @brief Scans all slices at once using a single ray-cast pass.
			 * @note Casts one ray per pixel (x,y), collects ALL depth intersections,
			 * then determines inside/outside for each slice depth in O(1).
			 * ~1000x faster than calling scan() per slice.
			 * @param sliceCount Number of depth slices to produce.
			 * @param writeCallback Called for each completed slice: callback(sliceIndex, pixmap).
			 *        This allows writing to disk immediately without storing all slices in memory.
			 */
			template< typename callback_t >
			void
			scanAll (uint32_t sliceCount, callback_t && writeCallback) const noexcept
			{
				if ( !m_prepared || sliceCount == 0 )
				{
					return;
				}

				const auto resX = m_resolution;
				const auto resY = m_resolution;

				/* Phase 1: Cast rays and collect intersection depths for every pixel.
				 * Store as sorted depth intervals per pixel. */
				struct PixelIntervals
				{
					std::vector< vertex_data_t > depths; /* Sorted intersection depths. */
				};

				std::vector< PixelIntervals > pixelData(static_cast< size_t >(resX) * resY);

				auto castRow = [&] (uint32_t py)
				{
					for ( uint32_t px = 0; px < resX; ++px )
					{
						const auto viewX = m_squareMinX + (static_cast< vertex_data_t >(px) + static_cast< vertex_data_t >(0.5)) / static_cast< vertex_data_t >(resX) * m_squareSize;
						const auto viewY = m_squareMinY + (static_cast< vertex_data_t >(py) + static_cast< vertex_data_t >(0.5)) / static_cast< vertex_data_t >(resY) * m_squareSize;

						const auto rayOrigin = m_viewpoint.position() + m_right * viewX + m_up * viewY + m_forward * m_boundsMin[2];

						auto & pixel = pixelData[static_cast< size_t >(py) * resX + px];

						const auto gridScale = static_cast< vertex_data_t >(SliceGridRes);
						const auto cellX = static_cast< size_t >(std::clamp((viewX - m_squareMinX) / m_squareSize * gridScale, static_cast< vertex_data_t >(0), static_cast< vertex_data_t >(SliceGridRes - 1)));
						const auto cellY = static_cast< size_t >(std::clamp((viewY - m_squareMinY) / m_squareSize * gridScale, static_cast< vertex_data_t >(0), static_cast< vertex_data_t >(SliceGridRes - 1)));

						for ( const auto triIdx : m_sliceGrid[cellY * SliceGridRes + cellX] )
						{
							const auto & tt = m_allTriangles[triIdx];
							vertex_data_t hitT;

							if ( rayTriangleIntersectT(rayOrigin, m_forward, tt.v[0], tt.v[1], tt.v[2], hitT) )
							{
								pixel.depths.push_back(hitT);
							}
						}

						if ( !pixel.depths.empty() )
						{
							std::sort(pixel.depths.begin(), pixel.depths.end());
						}
					}
				};

				if ( m_threadPool != nullptr )
				{
					m_threadPool->parallelFor(uint32_t{0}, resY, castRow);
				}
				else
				{
					for ( uint32_t py = 0; py < resY; ++py )
					{
						castRow(py);
					}
				}

				/* Phase 2: For each slice, check inside/outside per pixel using the cached depths. */
				for ( uint32_t sliceIdx = 0; sliceIdx < sliceCount; ++sliceIdx )
				{
					const auto normalizedDepth = static_cast< vertex_data_t >(sliceIdx) / static_cast< vertex_data_t >(sliceCount - 1);
					const auto sliceZ = normalizedDepth * m_sliceDepth;

					PixelFactory::Pixmap< uint8_t > output(resX, resY, PixelFactory::ChannelMode::Grayscale, PixelFactory::Color< float >{0.0F, 0.0F, 0.0F, 1.0F});

					auto fillRow = [&] (uint32_t py)
					{
						for ( uint32_t px = 0; px < resX; ++px )
						{
							const auto & pixel = pixelData[static_cast< size_t >(py) * resX + px];

							if ( pixel.depths.empty() )
							{
								continue;
							}

							/* Count how many intersections are before this depth. Odd = inside. */
							size_t count = 0;

							for ( const auto d : pixel.depths )
							{
								if ( d <= sliceZ )
								{
									++count;
								}
								else
								{
									break; /* Sorted, no need to check further. */
								}
							}

							if ( count % 2 != 0 )
							{
								output.setPixel(px, py, PixelFactory::Color< float >{1.0F, 1.0F, 1.0F, 1.0F});
							}
						}
					};

					if ( m_threadPool != nullptr )
					{
						m_threadPool->parallelFor(uint32_t{0}, resY, fillRow);
					}
					else
					{
						for ( uint32_t py = 0; py < resY; ++py )
						{
							fillRow(py);
						}
					}

					writeCallback(sliceIdx, output);
				}
			}

		private:

			/**
			 * @brief Transforms a point from shape-local space to world space using a CartesianFrame.
			 */
			[[nodiscard]]
			static
			Math::Vector< 3, vertex_data_t >
			transformPoint (const Math::Vector< 3, vertex_data_t > & point, const Math::CartesianFrame< vertex_data_t > & frame) noexcept
			{
				const auto right = frame.rightVector();
				const auto up = Math::Vector< 3, vertex_data_t >::crossProduct(frame.forwardVector(), frame.rightVector()).normalized();
				const auto forward = frame.forwardVector();

				return frame.position() + right * point[Math::X] + up * point[Math::Y] + forward * point[Math::Z];
			}

			/**
			 * @brief Ray-triangle intersection returning the hit distance t.
			 */
			[[nodiscard]]
			static
			bool
			rayTriangleIntersectT (const Math::Vector< 3, vertex_data_t > & origin, const Math::Vector< 3, vertex_data_t > & dir, const Math::Vector< 3, vertex_data_t > & v0, const Math::Vector< 3, vertex_data_t > & v1, const Math::Vector< 3, vertex_data_t > & v2, vertex_data_t & outT) noexcept
			{
				constexpr auto eps = static_cast< vertex_data_t >(1e-7);

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
				const auto u = f * Math::Vector< 3, vertex_data_t >::dotProduct(s, h);

				if ( u < 0 || u > 1 )
				{
					return false;
				}

				const auto q = Math::Vector< 3, vertex_data_t >::crossProduct(s, e1);
				const auto v = f * Math::Vector< 3, vertex_data_t >::dotProduct(dir, q);

				if ( v < 0 || u + v > 1 )
				{
					return false;
				}

				outT = f * Math::Vector< 3, vertex_data_t >::dotProduct(e2, q);

				return true;
			}

			/**
			 * @brief Ray-triangle intersection test (Möller-Trumbore). Returns true if the ray hits.
			 */
			[[nodiscard]]
			static
			bool
			rayTriangleIntersect (const Math::Vector< 3, vertex_data_t > & origin, const Math::Vector< 3, vertex_data_t > & dir, const Math::Vector< 3, vertex_data_t > & v0, const Math::Vector< 3, vertex_data_t > & v1, const Math::Vector< 3, vertex_data_t > & v2) noexcept
			{
				constexpr auto eps = static_cast< vertex_data_t >(1e-7);

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
				const auto u = f * Math::Vector< 3, vertex_data_t >::dotProduct(s, h);

				if ( u < 0 || u > 1 )
				{
					return false;
				}

				const auto q = Math::Vector< 3, vertex_data_t >::crossProduct(s, e1);
				const auto v = f * Math::Vector< 3, vertex_data_t >::dotProduct(dir, q);

				if ( v < 0 || u + v > 1 )
				{
					return false;
				}

				const auto t = f * Math::Vector< 3, vertex_data_t >::dotProduct(e2, q);

				return t > eps;
			}

			/**
			 * @brief Computes the axis-aligned bounding volume of all shapes in view space.
			 */
			void
			computeBounds () const noexcept
			{
				const auto forward = m_viewpoint.forwardVector();
				const auto right = m_viewpoint.rightVector();
				const auto up = Math::Vector< 3, vertex_data_t >::crossProduct(m_viewpoint.forwardVector(), m_viewpoint.rightVector()).normalized();

				m_boundsMin = {std::numeric_limits< vertex_data_t >::max(), std::numeric_limits< vertex_data_t >::max(), std::numeric_limits< vertex_data_t >::max()};
				m_boundsMax = {std::numeric_limits< vertex_data_t >::lowest(), std::numeric_limits< vertex_data_t >::lowest(), std::numeric_limits< vertex_data_t >::lowest()};

				for ( const auto & entry : m_shapes )
				{
					const auto & verts = entry.shape->vertices();

					for ( size_t i = 0; i < verts.size(); ++i )
					{
						const auto worldPos = transformPoint(verts[i].position(), entry.frame);
						const auto relative = worldPos - m_viewpoint.position();

						const auto vx = Math::Vector< 3, vertex_data_t >::dotProduct(relative, right);
						const auto vy = Math::Vector< 3, vertex_data_t >::dotProduct(relative, up);
						const auto vz = Math::Vector< 3, vertex_data_t >::dotProduct(relative, forward);

						m_boundsMin[0] = std::min(m_boundsMin[0], vx);
						m_boundsMin[1] = std::min(m_boundsMin[1], vy);
						m_boundsMin[2] = std::min(m_boundsMin[2], vz);
						m_boundsMax[0] = std::max(m_boundsMax[0], vx);
						m_boundsMax[1] = std::max(m_boundsMax[1], vy);
						m_boundsMax[2] = std::max(m_boundsMax[2], vz);
					}
				}

				/* Add small padding. */
				const auto pad = static_cast< vertex_data_t >(0.01);

				for ( int i = 0; i < 3; ++i )
				{
					m_boundsMin[i] -= pad;
					m_boundsMax[i] += pad;
				}

				m_boundsValid = true;
			}

			/* ---- Types ---- */

			struct TransformedTriangle
			{
				Math::Vector< 3, vertex_data_t > v[3];
			};

			static constexpr size_t SliceGridRes = 128;

			/* ---- Members ---- */

			std::vector< ShapeEntry > m_shapes;
			Math::CartesianFrame< vertex_data_t > m_viewpoint;
			ThreadPool * m_threadPool{nullptr};
			mutable Math::Vector< 3, vertex_data_t > m_boundsMin;
			mutable Math::Vector< 3, vertex_data_t > m_boundsMax;
			mutable bool m_boundsValid{false};

			/* Precomputed data (populated by prepare()). */
			mutable std::vector< TransformedTriangle > m_allTriangles;
			mutable std::vector< std::vector< size_t > > m_sliceGrid;
			mutable Math::Vector< 3, vertex_data_t > m_forward;
			mutable Math::Vector< 3, vertex_data_t > m_right;
			mutable Math::Vector< 3, vertex_data_t > m_up;
			mutable vertex_data_t m_squareMinX{0};
			mutable vertex_data_t m_squareMinY{0};
			mutable vertex_data_t m_squareSize{0};
			mutable vertex_data_t m_sliceDepth{0};
			mutable uint32_t m_resolution{0};
			mutable bool m_prepared{false};
	};
}
