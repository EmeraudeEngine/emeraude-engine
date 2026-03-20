/*
 * src/Libs/Algorithms/DelaunayTriangulation.hpp
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
#include <limits>
#include <type_traits>
#include <vector>

namespace EmEn::Libs::Algorithms
{
	/**
	 * @brief Constrained Delaunay Triangulation for 2D point sets with boundary constraints.
	 * @note Uses the Bowyer-Watson algorithm for Delaunay, then removes triangles outside
	 * the constrained boundary. Operates on a flat 2D point set projected from 3D.
	 * @tparam data_t The floating-point precision type. Default float.
	 */
	template< typename data_t = float >
	requires (std::is_floating_point_v< data_t >)
	class DelaunayTriangulation final
	{
		public:

			struct Point
			{
				data_t x{0};
				data_t y{0};
			};

			struct Triangle
			{
				size_t a{0};
				size_t b{0};
				size_t c{0};
			};

			/**
			 * @brief Triangulates a closed polygon defined by ordered boundary points.
			 * @note The polygon vertices must be in order (CW or CCW). The triangulation
			 * covers the interior of the polygon, handling concavities correctly.
			 * @param boundaryPoints Ordered boundary vertices forming a closed polygon.
			 * @return std::vector< Triangle > The resulting triangles (indices into boundaryPoints).
			 */
			[[nodiscard]]
			static
			std::vector< Triangle >
			triangulateBoundary (const std::vector< Point > & boundaryPoints) noexcept
			{
				const auto pointCount = boundaryPoints.size();

				if ( pointCount < 3 )
				{
					return {};
				}

				/* Step 1: Compute a super-triangle that encloses all points. */
				data_t minX = boundaryPoints[0].x;
				data_t minY = boundaryPoints[0].y;
				data_t maxX = minX;
				data_t maxY = minY;

				for ( const auto & p : boundaryPoints )
				{
					minX = std::min(minX, p.x);
					minY = std::min(minY, p.y);
					maxX = std::max(maxX, p.x);
					maxY = std::max(maxY, p.y);
				}

				const auto dx = maxX - minX;
				const auto dy = maxY - minY;
				const auto dmax = std::max(dx, dy);
				const auto midX = (minX + maxX) / static_cast< data_t >(2);
				const auto midY = (minY + maxY) / static_cast< data_t >(2);

				/* Super-triangle vertices (indices: pointCount, pointCount+1, pointCount+2). */
				std::vector< Point > allPoints = boundaryPoints;
				allPoints.push_back({midX - static_cast< data_t >(20) * dmax, midY - dmax});
				allPoints.push_back({midX + static_cast< data_t >(20) * dmax, midY - dmax});
				allPoints.push_back({midX, midY + static_cast< data_t >(20) * dmax});

				const auto superA = pointCount;
				const auto superB = pointCount + 1;
				const auto superC = pointCount + 2;

				/* Step 2: Bowyer-Watson incremental insertion. */
				std::vector< Triangle > triangles;
				triangles.push_back({superA, superB, superC});

				for ( size_t i = 0; i < pointCount; ++i )
				{
					const auto & point = allPoints[i];

					/* Find all triangles whose circumcircle contains the point. */
					std::vector< std::array< size_t, 2 > > polygon;
					std::vector< bool > bad(triangles.size(), false);

					for ( size_t t = 0; t < triangles.size(); ++t )
					{
						if ( isInsideCircumcircle(allPoints, triangles[t], point) )
						{
							bad[t] = true;
						}
					}

					/* Find the boundary of the polygonal hole. */
					for ( size_t t = 0; t < triangles.size(); ++t )
					{
						if ( !bad[t] )
						{
							continue;
						}

						const auto & tri = triangles[t];
						const std::array< size_t, 2 > edges[3] = {
							{tri.a, tri.b},
							{tri.b, tri.c},
							{tri.c, tri.a}
						};

						for ( const auto & edge : edges )
						{
							bool shared = false;

							for ( size_t other = 0; other < triangles.size(); ++other )
							{
								if ( other == t || !bad[other] )
								{
									continue;
								}

								if ( sharesEdge(triangles[other], edge[0], edge[1]) )
								{
									shared = true;

									break;
								}
							}

							if ( !shared )
							{
								polygon.push_back(edge);
							}
						}
					}

					/* Remove bad triangles. */
					size_t writeIdx = 0;

					for ( size_t t = 0; t < triangles.size(); ++t )
					{
						if ( !bad[t] )
						{
							triangles[writeIdx++] = triangles[t];
						}
					}

					triangles.resize(writeIdx);

					/* Re-triangulate the polygonal hole with the new point. */
					for ( const auto & edge : polygon )
					{
						triangles.push_back({edge[0], edge[1], i});
					}
				}

				/* Step 3: Remove triangles that reference super-triangle vertices. */
				{
					size_t writeIdx = 0;

					for ( size_t t = 0; t < triangles.size(); ++t )
					{
						const auto & tri = triangles[t];

						if ( tri.a >= pointCount || tri.b >= pointCount || tri.c >= pointCount )
						{
							continue;
						}

						triangles[writeIdx++] = tri;
					}

					triangles.resize(writeIdx);
				}

				/* Step 4: Remove triangles that lie outside the boundary polygon.
				 * A triangle is inside if its centroid is inside the boundary polygon. */
				{
					size_t writeIdx = 0;

					for ( size_t t = 0; t < triangles.size(); ++t )
					{
						const auto & tri = triangles[t];

						const Point centroid{
							(allPoints[tri.a].x + allPoints[tri.b].x + allPoints[tri.c].x) / static_cast< data_t >(3),
							(allPoints[tri.a].y + allPoints[tri.b].y + allPoints[tri.c].y) / static_cast< data_t >(3)
						};

						if ( isInsidePolygon(boundaryPoints, centroid) )
						{
							triangles[writeIdx++] = tri;
						}
					}

					triangles.resize(writeIdx);
				}

				return triangles;
			}

		private:

			/**
			 * @brief Tests if a point lies inside the circumcircle of a triangle.
			 */
			[[nodiscard]]
			static
			bool
			isInsideCircumcircle (const std::vector< Point > & points, const Triangle & tri, const Point & p) noexcept
			{
				const auto & a = points[tri.a];
				const auto & b = points[tri.b];
				const auto & c = points[tri.c];

				const auto ax = a.x - p.x;
				const auto ay = a.y - p.y;
				const auto bx = b.x - p.x;
				const auto by = b.y - p.y;
				const auto cx = c.x - p.x;
				const auto cy = c.y - p.y;

				const auto det = ax * (by * (cx * cx + cy * cy) - cy * (bx * bx + by * by))
					- ay * (bx * (cx * cx + cy * cy) - cx * (bx * bx + by * by))
					+ (ax * ax + ay * ay) * (bx * cy - by * cx);

				/* If points are CCW, det > 0 means inside. Handle both windings. */
				const auto cross = (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);

				return (cross > 0) ? (det > 0) : (det < 0);
			}

			/**
			 * @brief Tests if a triangle shares an edge with the given edge vertices.
			 */
			[[nodiscard]]
			static
			bool
			sharesEdge (const Triangle & tri, size_t e0, size_t e1) noexcept
			{
				const size_t verts[3] = {tri.a, tri.b, tri.c};

				for ( int i = 0; i < 3; ++i )
				{
					const auto va = verts[i];
					const auto vb = verts[(i + 1) % 3];

					if ( (va == e0 && vb == e1) || (va == e1 && vb == e0) )
					{
						return true;
					}
				}

				return false;
			}

			/**
			 * @brief Tests if a point is inside a polygon using ray casting.
			 */
			[[nodiscard]]
			static
			bool
			isInsidePolygon (const std::vector< Point > & polygon, const Point & point) noexcept
			{
				bool inside = false;
				const auto n = polygon.size();

				for ( size_t i = 0, j = n - 1; i < n; j = i++ )
				{
					const auto & pi = polygon[i];
					const auto & pj = polygon[j];

					if ( ((pi.y > point.y) != (pj.y > point.y)) &&
						(point.x < (pj.x - pi.x) * (point.y - pi.y) / (pj.y - pi.y) + pi.x) )
					{
						inside = !inside;
					}
				}

				return inside;
			}
	};
}
