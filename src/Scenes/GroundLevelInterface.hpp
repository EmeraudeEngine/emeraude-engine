/*
 * src/Scenes/GroundLevelInterface.hpp
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
 * https://github.com/EmeraudeEngine/emeraude-engine
 *
 * --- THIS IS AUTOMATICALLY GENERATED, DO NOT CHANGE ---
 */

#pragma once

/* Project configuration. */
#include "emeraude_export.hpp"

/* STL inclusions. */
#include <memory>

/* Local inclusions for usages. */
#include "Math/Space3D/AACuboid.hpp"
#include "Math/Space3D/Triangle.hpp"
#include "Math/Vector.hpp"

/* Forward declarations. */
namespace EmEn::Graphics::Renderable
{
	class Abstract;
}

namespace EmEn::Scenes
{
	/**
	 * @brief Receives the ground triangles of a region (GroundLevelInterface::visitTriangles()).
	 */
	class EMEN_API GroundTriangleVisitor
	{
		public:

			/**
			 * @brief Destructs the visitor.
			 */
			virtual ~GroundTriangleVisitor () = default;

			/**
			 * @brief Receives one triangle.
			 * @param triangle A reference to the triangle in world space, its winding normal pointing UP.
			 * @param featureId An identifier of the triangle, stable while the ground does not change (its cell and half),
			 * for the contact manifolds' feature ids.
			 * @return void
			 */
			virtual void onTriangle (const Base::Math::Space3D::Triangle< float > & triangle, uint32_t featureId) noexcept = 0;

		protected:

			/**
			 * @brief Constructs a visitor.
			 */
			GroundTriangleVisitor () noexcept = default;

			GroundTriangleVisitor (const GroundTriangleVisitor & copy) noexcept = default;
			GroundTriangleVisitor (GroundTriangleVisitor && copy) noexcept = default;
			GroundTriangleVisitor & operator= (const GroundTriangleVisitor & copy) noexcept = default;
			GroundTriangleVisitor & operator= (GroundTriangleVisitor && copy) noexcept = default;
	};

	/**
	 * @brief Interface to define a physical and visible floor in a scene.
	 */
	class EMEN_API GroundLevelInterface
	{
		public:

			/**
			 * @brief Destructs the ground interface.
			 */
			virtual ~GroundLevelInterface () = default;

			/**
			 * @brief Copy constructor (deleted: a polymorphic interface, copying would slice).
			 * @param copy A reference to the copied instance.
			 */
			GroundLevelInterface (const GroundLevelInterface & copy) noexcept = delete;

			/**
			 * @brief Move constructor (deleted: a polymorphic interface).
			 * @param copy A reference to the copied instance.
			 */
			GroundLevelInterface (GroundLevelInterface && copy) noexcept = delete;

			/**
			 * @brief Copy assignment (deleted: a polymorphic interface).
			 * @param copy A reference to the copied instance.
			 * @return GroundLevelInterface &
			 */
			GroundLevelInterface & operator= (const GroundLevelInterface & copy) noexcept = delete;

			/**
			 * @brief Move assignment (deleted: a polymorphic interface).
			 * @param copy A reference to the copied instance.
			 * @return GroundLevelInterface &
			 */
			GroundLevelInterface & operator= (GroundLevelInterface && copy) noexcept = delete;

			/**
			 * @brief Returns the ground level under the given position.
			 * @param worldPosition An absolute position.
			 * @return float
			 */
			[[nodiscard]]
			virtual float getLevelAt (const Base::Math::Vector< 3, float > & worldPosition) const noexcept = 0;

			/**
			 * @brief Visits the ground triangles under a world region — the RENDERED surface, what the physics collides
			 * with (physics overhaul P2, docs/physics-overhaul.md § 1.6).
			 * @note Only the region's X and Z extents select the cells; nothing is visited outside the ground.
			 * @note ⚠️ Not the surface getLevelAt() answers: that one interpolates a cell bilinearly, the triangles split it
			 * along its rendered diagonal.
			 * @param worldRegion A reference to the region (a body's world AABB).
			 * @param visitor A reference to the visitor, called once per triangle.
			 * @return size_t The number of triangles visited.
			 */
			[[nodiscard]]
			virtual size_t visitTriangles (const Base::Math::Space3D::AACuboid< float > & worldRegion, GroundTriangleVisitor & visitor) const noexcept = 0;

			/**
			 * @brief Returns a position where Y is completed by the level at X,Z position.
			 * @param positionX The X coordinates.
			 * @param positionZ The Z coordinates.
			 * @param deltaY A difference value to add to the Y component. Default 0.0F.
			 * @return Vector< 3, float >
			 */
			[[nodiscard]]
			virtual Base::Math::Vector< 3, float > getLevelAt (float positionX, float positionZ, float deltaY) const noexcept = 0;

			/**
			 * @brief Returns the normal vector under the given position.
			 * @param worldPosition An absolute position.
			 * @return Vector< 3, float >
			 */
			[[nodiscard]]
			virtual Base::Math::Vector< 3, float > getNormalAt (const Base::Math::Vector< 3, float > & worldPosition) const noexcept = 0;

			/**
			 * @brief Updates the ground visibility from the camera position.
			 * @note This is not frustum-culling, but help the ground to know where the point of view is located.
			 */
			virtual void updateVisibility (const Base::Math::Vector< 3, float > & worldPosition) noexcept = 0;

			/**
			 * @brief Returns a renderable the scene draws BESIDE the ground, or null.
			 * @note A terrain's DETAIL WINDOW (Renderable::TerrainResource): a mesh-shading surface around the camera
			 * where the CDLOD leaves a hole (engine item mesh-shading-surface-on-heightfield). The scene registers it
			 * as a scene visual next to the ground, lit, out of the ray-tracing lists.
			 * @return std::shared_ptr< Graphics::Renderable::Abstract >
			 */
			[[nodiscard]]
			virtual
			std::shared_ptr< Graphics::Renderable::Abstract >
			detailRenderable () const noexcept
			{
				return nullptr;
			}

		protected:

			/**
			 * @brief Constructs a ground interface.
			 */
			GroundLevelInterface () noexcept = default;
	};
}
