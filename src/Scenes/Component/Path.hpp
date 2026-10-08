/*
 * src/Scenes/Component/Path.hpp
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
#include <array>
#include <cstdint>
#include <cstring>
#include <memory>
#include <span>
#include <string>
#include <vector>

/* Local inclusions for inheritances. */
#include "Abstract.hpp"

/* Local inclusions for usages. */
#include "Constants.hpp"
#include "Graphics/Material/PathResource.hpp"
#include "Graphics/RenderableInstance/PathPoints.hpp"
#include "Graphics/RenderableInstance/Unique.hpp"
#include "Math/BSpline.hpp"
#include "Math/CurveShape.hpp"
#include "Math/Vector.hpp"

/* Forward declarations. */
namespace EmEn
{
	namespace Resources
	{
		class Manager;
	}

	namespace Graphics::Geometry
	{
		class PulledVertexResource;
	}
}

namespace EmEn::Scenes::Component
{
	/**
	 * @brief A PATH in the world: a polyline or a curve drawn as a camera-facing ribbon — a route, a trail, a zone
	 * border, an AI path (owner decisions 2026-09-29).
	 * @note Every curve is tessellated on the CPU into a polyline, adaptively, within a chord tolerance
	 * (Base::Math::CurveTessellation): a polyline as it is, a piecewise Bézier path (Math::BSpline: anchors + handles),
	 * a uniform cubic B-spline, a centripetal Catmull-Rom through its points.
	 * @note Drawn by VERTEX PULLING (Graphics::Geometry::PulledVertexResource, Saphir PathGLSL): the points are published
	 * per render state slot (RenderableInstance::PathPoints) and staged into the scene's path SSBO beside the instance's
	 * transforms; the vertex stage builds the ribbon from them — mitered joins (bevelled past the miter limit) or round
	 * joins and caps, a width in metres or in pixels (Material::PathResource).
	 * @note OPAQUE and UNLIT, depth tested and written, a solid colour emitted at a luminance in nits; no shadow, out of
	 * the ray tracing, never in a reflection cubemap nor a shadow map (the path SSBO is read on the instance-transforms
	 * path only).
	 * @note Setters: from the logic thread or under the scene's exclusive access (a console command): the points are
	 * published by publishStateForRendering() on the logic thread — only when they changed.
	 * @extends EmEn::Scenes::Component::Abstract The base class for each entity component.
	 */
	class EMEN_LEAN_API Path final : public Abstract
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"Path"};

			/** @brief The chord tolerance of the curve tessellation, in the entity's units (1 cm for metres). */
			static constexpr float DefaultTolerance{0.01F};

			/** @brief The kind of curve the points describe (Math::CurveShape, shared with the Beam). */
			using Kind = Base::Math::CurveKind;

			/**
			 * @brief Constructs a path component.
			 * @note Creates the path's own material, geometry and renderable (one per path: its look and its capacity
			 * are its own).
			 * @param componentName A reference to a string.
			 * @param parentEntity A reference to the parent entity.
			 * @param resources A reference to the resource manager.
			 */
			Path (const std::string & componentName, const AbstractEntity & parentEntity, Resources::Manager & resources) noexcept;

			/** @copydoc EmEn::Scenes::Component::Abstract::getRenderableInstance() const */
			[[nodiscard]]
			std::shared_ptr< Graphics::RenderableInstance::Abstract >
			getRenderableInstance () const noexcept override
			{
				return m_renderableInstance;
			}

			/** @copydoc EmEn::Scenes::Component::Abstract::getComponentType() */
			[[nodiscard]]
			const char *
			getComponentType () const noexcept override
			{
				return ClassId;
			}

			/** @copydoc EmEn::Scenes::Component::Abstract::isComponent() */
			[[nodiscard]]
			bool
			isComponent (const char * classID) const noexcept override
			{
				return std::strcmp(ClassId, classID) == 0;
			}

			/** @copydoc EmEn::Scenes::Component::Abstract::renderBoundingBox() const */
			[[nodiscard]]
			const Base::Math::Space3D::AACuboid< float > &
			renderBoundingBox () const noexcept override
			{
				return m_boundingBox;
			}

			/** @copydoc EmEn::Scenes::Component::Abstract::renderBoundingSphere() const */
			[[nodiscard]]
			const Base::Math::Space3D::Sphere< float > &
			renderBoundingSphere () const noexcept override
			{
				return m_boundingSphere;
			}

			/**
			 * @copydoc EmEn::Scenes::Component::Abstract::localBoundingBox() const
			 * @note A path is a drawing: it has no collision extent.
			 */
			[[nodiscard]]
			const Base::Math::Space3D::AACuboid< float > &
			localBoundingBox () const noexcept override
			{
				return NullBoundingBox;
			}

			/**
			 * @copydoc EmEn::Scenes::Component::Abstract::localBoundingSphere() const
			 * @note A path is a drawing: it has no collision extent.
			 */
			[[nodiscard]]
			const Base::Math::Space3D::Sphere< float > &
			localBoundingSphere () const noexcept override
			{
				return NullBoundingSphere;
			}

			/** @copydoc EmEn::Scenes::Component::Abstract::move() */
			void
			move (const Base::Math::CartesianFrame< float > & /*worldCoordinates*/) noexcept override
			{

			}

			/** @copydoc EmEn::Scenes::Component::Abstract::processLogics() */
			void
			processLogics (const Scene & /*scene*/) noexcept override
			{

			}

			/** @copydoc EmEn::Scenes::Component::Abstract::publishStateForRendering() */
			void publishStateForRendering (uint32_t writeStateIndex) noexcept override;

			/** @copydoc EmEn::Scenes::Component::Abstract::shouldBeRemoved() */
			[[nodiscard]]
			bool
			shouldBeRemoved () const noexcept override
			{
				return m_renderableInstance == nullptr || m_renderableInstance->isBroken();
			}

			/**
			 * @brief Returns the path's material, to change its look (colour, luminance, width, joins).
			 * @return std::shared_ptr< Graphics::Material::PathResource >
			 */
			[[nodiscard]]
			std::shared_ptr< Graphics::Material::PathResource >
			material () const noexcept
			{
				return m_material;
			}

			/**
			 * @brief Sets the points of a POLYLINE, in the entity's space.
			 * @param points The points (2 at least to draw anything).
			 * @param closed Whether the last point joins the first.
			 */
			void setPolyline (std::span< const Base::Math::Vector< 3, float > > points, bool closed = false) noexcept;

			/**
			 * @brief Sets a piecewise BÉZIER path, in the entity's space.
			 * @param path The anchors, their handles (offsets from the anchor) and a curve type per span; its segment
			 * counts are ignored (the tessellation is adaptive).
			 */
			void setBezierPath (const Base::Math::BSpline< 3, float > & path) noexcept;

			/**
			 * @brief Sets the control points of a uniform cubic B-SPLINE (smooth, not through its points), in the
			 * entity's space. An open one starts and ends on its end points.
			 * @param controlPoints The control points.
			 * @param closed Whether the curve closes on itself.
			 */
			void setUniformBSpline (std::span< const Base::Math::Vector< 3, float > > controlPoints, bool closed = false) noexcept;

			/**
			 * @brief Sets the points of a CATMULL-ROM spline (through every point), in the entity's space.
			 * @param points The points.
			 * @param closed Whether the curve closes on itself.
			 * @param alpha 0 uniform, 0.5 centripetal (the default: no cusp, no loop), 1 chordal.
			 */
			void setCatmullRom (std::span< const Base::Math::Vector< 3, float > > points, bool closed = false, float alpha = 0.5F) noexcept;

			/**
			 * @brief Sets the chord tolerance of the curve tessellation, and re-tessellates.
			 * @param tolerance The largest distance between the curve and its polyline, in the entity's units (> 0).
			 */
			void setTolerance (float tolerance) noexcept;

			/**
			 * @brief Returns the chord tolerance.
			 * @return float
			 */
			[[nodiscard]]
			float
			tolerance () const noexcept
			{
				return m_tolerance;
			}

			/**
			 * @brief Returns the kind of curve.
			 * @return Kind
			 */
			[[nodiscard]]
			Kind
			kind () const noexcept
			{
				return m_curve.kind();
			}

			/**
			 * @brief Returns whether the curve is closed.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isClosed () const noexcept
			{
				return m_curve.isClosed();
			}

			/**
			 * @brief Returns the points as given (a polyline's points, a spline's control points; the anchors of a Bézier path).
			 * @return const std::vector< Base::Math::Vector< 3, float > > &
			 */
			[[nodiscard]]
			const std::vector< Base::Math::Vector< 3, float > > &
			sourcePoints () const noexcept
			{
				return m_curve.points();
			}

			/**
			 * @brief Returns the tessellated polyline drawn (xyz in the entity's space, w the arc length).
			 * @return const std::vector< Base::Math::Vector< 4, float > > &
			 */
			[[nodiscard]]
			const std::vector< Base::Math::Vector< 4, float > > &
			polyline () const noexcept
			{
				return m_polyline;
			}

			/**
			 * @brief Returns the length of the path (the tessellated polyline), in the entity's units.
			 * @return float
			 */
			[[nodiscard]]
			float
			length () const noexcept
			{
				return m_polyline.empty() ? 0.0F : m_polyline.back()[Base::Math::W];
			}

			/**
			 * @brief Shows or hides the path.
			 * @param state The state.
			 */
			void setEnabled (bool state) noexcept;

			/**
			 * @brief Returns whether the path is shown.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isEnabled () const noexcept
			{
				return m_enabled;
			}

			/**
			 * @brief Switches the DEBUG mode: the path leaves the scene pass and is drawn ALWAYS ON TOP after the tone
			 * mapping (Graphics::PathDebugOverlay) — a display colour, translucent if its opacity says so, neither
			 * exposed nor smeared by the TAA. Its width and joins stay the material's.
			 * @param state The state.
			 */
			void setDebugMode (bool state) noexcept;

			/**
			 * @brief Returns whether the path is in debug mode.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isDebugMode () const noexcept
			{
				return m_debugMode;
			}

			/**
			 * @brief Sets the colour of the debug mode, AS DISPLAYED (sRGB), with its opacity.
			 * @param color A reference to a colour.
			 */
			void setDebugColor (const Base::PixelFactory::Color< float > & color) noexcept;

			/**
			 * @brief Returns the colour of the debug mode (sRGB, opacity).
			 * @return const Base::PixelFactory::Color< float > &
			 */
			[[nodiscard]]
			const Base::PixelFactory::Color< float > &
			debugColor () const noexcept
			{
				return m_debugColor;
			}

			/**
			 * @brief Tells the path its material's look changed (width, joins): the bounds follow the width, and the debug
			 * mode, which draws with the material's style, republishes. The material setters cannot tell the path.
			 */
			void
			markLookChanged () noexcept
			{
				++m_version;

				this->updateBounds();
			}

			/** @brief Converts a kind into its name. */
			[[nodiscard]]
			static
			const char *
			kindName (Kind kind) noexcept
			{
				return Base::Math::to_cstring(kind);
			}

		private:

			/** @copydoc EmEn::Scenes::Component::Abstract::onSuspend() */
			void onSuspend () noexcept override { }

			/** @copydoc EmEn::Scenes::Component::Abstract::onWakeup() */
			void onWakeup () noexcept override { }

			/** @copydoc EmEn::Animations::AnimatableInterface::playAnimation() */
			bool
			playAnimation (uint8_t /*animationID*/, const Base::Variant & /*value*/, size_t /*cycle*/) noexcept override
			{
				return false;
			}

			/**
			 * @brief Tessellates the source into the drawn polyline (with arc lengths), sizes the geometry, refreshes the
			 * bounds and marks the points for publication.
			 */
			void rebuild () noexcept;

			/**
			 * @brief Refreshes the render bounds from the polyline and the width, and notifies the entity when they changed.
			 */
			void updateBounds () noexcept;

			std::shared_ptr< Graphics::Material::PathResource > m_material;
			std::shared_ptr< Graphics::Geometry::PulledVertexResource > m_geometry;
			std::shared_ptr< Graphics::RenderableInstance::Unique > m_renderableInstance;
			std::shared_ptr< Graphics::RenderableInstance::PathPoints > m_pathPoints;
			Base::Math::CurveShape< float > m_curve;
			std::vector< Base::Math::Vector< 4, float > > m_polyline;
			/** @brief What each render state slot last received (m_version when current): a slot is published on change only. */
			std::array< uint64_t, RenderStateSlotCount > m_publishedVersions{};
			Base::Math::Space3D::AACuboid< float > m_boundingBox;
			Base::Math::Space3D::Sphere< float > m_boundingSphere;
			Base::PixelFactory::Color< float > m_debugColor{1.0F, 0.85F, 0.1F, 1.0F};
			uint64_t m_version{1};
			float m_tolerance{DefaultTolerance};
			bool m_enabled{true};
			bool m_debugMode{false};
	};
}
