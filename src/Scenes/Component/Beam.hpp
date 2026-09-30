/*
 * src/Scenes/Component/Beam.hpp
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
#include "Graphics/Material/BeamResource.hpp"
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
	 * @brief A BEAM along a curve: a laser, an electric arc — Half-Life's env_beam / env_laser, made photometric. An arc
	 * amplitude of 0 draws a laser; above it the beam wanders and re-strikes.
	 * @note Its centre line is a curve of the Path's kinds (Math::CurveShape, owner decision 2026-09-30): a straight
	 * start → end by default, a polyline through relays, a Bézier path, a B-spline, a Catmull-Rom. setStart() / setEnd()
	 * move its first / last point; the end can follow another entity (setEndTarget()).
	 * @note Drawn by VERTEX PULLING, like a path (Graphics::Geometry::PulledVertexResource, Saphir BeamGLSL): the curve is
	 * tessellated (chord tolerance), every segment split so that the whole beam has at least segmentCount() pieces
	 * (CurveTessellation::subdivided(): an arc needs regular stations, a laser only the corners), and each STATION carries
	 * its position, its normalized arc length and a normal that does not twist around the curve (a rotation minimizing
	 * frame): the arc's two axes. The stations are published per render state slot (RenderableInstance::PathPoints) and
	 * staged into the scene's path SSBO — two vec4 per station, (position, t) then (normal, 0).
	 * @note The arc is pinned at both ENDS only (owner decision 2026-09-30): sin(π t) over the whole length.
	 * @note An emissive overlay: unlit, additive, no depth write, no shadow, out of the ray tracing, absent from the
	 * reflection cubemaps. It does not light the scene (owner decision, 2026-09-28): pair it with a light component.
	 * @note Setters: from the logic thread or under the scene's exclusive access (a console command).
	 * @extends EmEn::Scenes::Component::Abstract The base class for each entity component.
	 */
	class EMEN_LEAN_API Beam final : public Abstract
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"Beam"};

			/** @brief The default number of segments along the beam (the resolution of the arc). */
			static constexpr uint32_t DefaultSegmentCount{64};

			/** @brief The chord tolerance of the curve tessellation, in the entity's units (1 cm for metres). */
			static constexpr float DefaultTolerance{0.01F};

			/** @brief The kind of curve the beam follows (Math::CurveShape, shared with the Path). */
			using Kind = Base::Math::CurveKind;

			/**
			 * @brief Constructs a beam component: a straight beam from (0, 0, 0) to (0, 0, 1).
			 * @note Creates the beam's own material, geometry and renderable (one per beam: its look is per beam).
			 * @param componentName A reference to a string.
			 * @param parentEntity A reference to the parent entity.
			 * @param resources A reference to the resource manager.
			 * @param segmentCount The number of segments over the whole beam, at least (1 at least). A straight laser needs
			 * 1 — and then CANNOT wander: it has no interior station; an arc wants enough to show its finest octave. Default 64.
			 */
			Beam (const std::string & componentName, const AbstractEntity & parentEntity, Resources::Manager & resources, uint32_t segmentCount = DefaultSegmentCount) noexcept;

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
			 * @note A beam is light: it has no collision extent.
			 */
			[[nodiscard]]
			const Base::Math::Space3D::AACuboid< float > &
			localBoundingBox () const noexcept override
			{
				return NullBoundingBox;
			}

			/**
			 * @copydoc EmEn::Scenes::Component::Abstract::localBoundingSphere() const
			 * @note A beam is light: it has no collision extent.
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
			void processLogics (const Scene & scene) noexcept override;

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
			 * @brief Returns the beam's material, to change its look (colour, luminance, width, arc).
			 * @return std::shared_ptr< Graphics::Material::BeamResource >
			 */
			[[nodiscard]]
			std::shared_ptr< Graphics::Material::BeamResource >
			material () const noexcept
			{
				return m_material;
			}

			/**
			 * @brief Sets the start of the beam — the first point of its curve —, in the entity's space.
			 * @param position A reference to a vector.
			 * @return void
			 */
			void setStart (const Base::Math::Vector< 3, float > & position) noexcept;

			/**
			 * @brief Returns the start of the beam (the first point of its curve), in the entity's space.
			 * @return Base::Math::Vector< 3, float >
			 */
			[[nodiscard]]
			Base::Math::Vector< 3, float >
			start () const noexcept
			{
				return m_curve.points().empty() ? Base::Math::Vector< 3, float >{} : m_curve.points().front();
			}

			/**
			 * @brief Sets the end of the beam — the last point of its curve —, in the entity's space. Forgets an end target.
			 * @param position A reference to a vector.
			 * @return void
			 */
			void setEnd (const Base::Math::Vector< 3, float > & position) noexcept;

			/**
			 * @brief Returns the end of the beam (the last point of its curve; the target's position when one is followed).
			 * @return Base::Math::Vector< 3, float >
			 */
			[[nodiscard]]
			Base::Math::Vector< 3, float >
			end () const noexcept
			{
				return m_curve.points().empty() ? Base::Math::Vector< 3, float >{} : m_curve.points().back();
			}

			/**
			 * @brief Makes the end of the beam (the last point of its curve) follow another entity, every logic tick.
			 * @param target A reference to the followed entity (held weakly: the beam keeps its last end when the target is
			 * gone).
			 * @param offset An offset from the target's origin, in the TARGET's space. Default none.
			 * @return void
			 */
			void setEndTarget (const std::shared_ptr< const AbstractEntity > & target, const Base::Math::Vector< 3, float > & offset = {}) noexcept;

			/**
			 * @brief Returns the entity the end follows, or nullptr (none, or gone).
			 * @return std::shared_ptr< const AbstractEntity >
			 */
			[[nodiscard]]
			std::shared_ptr< const AbstractEntity >
			endTarget () const noexcept
			{
				return m_endTarget.lock();
			}

			/**
			 * @brief Returns the offset from the followed entity's origin, in the target's space.
			 * @return const Base::Math::Vector< 3, float > &
			 */
			[[nodiscard]]
			const Base::Math::Vector< 3, float > &
			endTargetOffset () const noexcept
			{
				return m_endTargetOffset;
			}

			/**
			 * @brief Makes the beam follow a POLYLINE (a laser through relays), in the entity's space.
			 * @param points The points (2 at least to draw anything).
			 * @param closed Whether the last point joins the first.
			 * @return void
			 */
			void setPolyline (std::span< const Base::Math::Vector< 3, float > > points, bool closed = false) noexcept;

			/**
			 * @brief Makes the beam follow a piecewise BÉZIER path, in the entity's space.
			 * @param path The anchors, their handles (offsets from the anchor) and a curve type per span.
			 * @return void
			 */
			void setBezierPath (const Base::Math::BSpline< 3, float > & path) noexcept;

			/**
			 * @brief Makes the beam follow a uniform cubic B-SPLINE (smooth, not through its points), in the entity's space.
			 * @param controlPoints The control points.
			 * @param closed Whether the curve closes on itself.
			 * @return void
			 */
			void setUniformBSpline (std::span< const Base::Math::Vector< 3, float > > controlPoints, bool closed = false) noexcept;

			/**
			 * @brief Makes the beam follow a CATMULL-ROM spline (through every point), in the entity's space.
			 * @param points The points.
			 * @param closed Whether the curve closes on itself.
			 * @param alpha 0 uniform, 0.5 centripetal (the default: no cusp, no loop), 1 chordal.
			 * @return void
			 */
			void setCatmullRom (std::span< const Base::Math::Vector< 3, float > > points, bool closed = false, float alpha = 0.5F) noexcept;

			/**
			 * @brief Sets the chord tolerance of the curve tessellation, and re-tessellates.
			 * @param tolerance The largest distance between the curve and its polyline, in the entity's units (> 0).
			 * @return void
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
			 * @brief Returns the curve the beam follows (its kind, its points).
			 * @return const Base::Math::CurveShape< float > &
			 */
			[[nodiscard]]
			const Base::Math::CurveShape< float > &
			curve () const noexcept
			{
				return m_curve;
			}

			/**
			 * @brief Returns the number of stations drawn along the beam (segments + 1; 0 when there is nothing to draw).
			 * @note An arc wanders at the INTERIOR stations only: 2 stations (a straight single segment) cannot wander.
			 * @return uint32_t
			 */
			[[nodiscard]]
			uint32_t
			stationCount () const noexcept
			{
				return static_cast< uint32_t >(m_stations.size() / RecordsPerStation);
			}

			/**
			 * @brief Returns the length of the beam (its tessellated curve), in the entity's units.
			 * @return float
			 */
			[[nodiscard]]
			float
			length () const noexcept
			{
				return m_length;
			}

			/**
			 * @brief Returns the minimum number of segments over the whole beam, fixed at construction.
			 * @note A straight beam of ONE segment has no interior station: it cannot wander, whatever its arc amplitude.
			 * @return uint32_t
			 */
			[[nodiscard]]
			uint32_t
			segmentCount () const noexcept
			{
				return m_segmentCount;
			}

			/**
			 * @brief Shows or hides the beam (a laser switched off, an arc between two strikes of a longer cycle).
			 * @param state The state.
			 * @return void
			 */
			void setEnabled (bool state) noexcept;

			/**
			 * @brief Returns whether the beam is shown.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isEnabled () const noexcept
			{
				return m_enabled;
			}

			/** @brief The vec4 records per station in the path SSBO: (position, t), then (normal, 0). */
			static constexpr size_t RecordsPerStation{2};

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
			 * @brief Tessellates and subdivides the curve into the stations (position and t, normal), sizes the geometry,
			 * and marks them for publication.
			 * @return void
			 */
			void rebuild () noexcept;

			/**
			 * @brief Refreshes the render bounds from the stations, the width and the arc amplitude, and notifies the entity
			 * when they changed.
			 * @return void
			 */
			void updateBounds () noexcept;

			std::shared_ptr< Graphics::Material::BeamResource > m_material;
			std::shared_ptr< Graphics::Geometry::PulledVertexResource > m_geometry;
			std::shared_ptr< Graphics::RenderableInstance::Unique > m_renderableInstance;
			std::shared_ptr< Graphics::RenderableInstance::PathPoints > m_pathPoints;
			std::weak_ptr< const AbstractEntity > m_endTarget;
			Base::Math::CurveShape< float > m_curve;
			/** @brief RecordsPerStation vec4 per station, in the entity's space. */
			std::vector< Base::Math::Vector< 4, float > > m_stations;
			/** @brief What each render state slot last received (m_version when current): a slot is published on change only. */
			std::array< uint64_t, RenderStateSlotCount > m_publishedVersions{};
			Base::Math::Space3D::AACuboid< float > m_boundingBox;
			Base::Math::Space3D::Sphere< float > m_boundingSphere;
			/** @brief The stations' extent, before the width and arc margin (updateBounds()). */
			Base::Math::Vector< 3, float > m_stationsMinimum;
			Base::Math::Vector< 3, float > m_stationsMaximum;
			Base::Math::Vector< 3, float > m_endTargetOffset;
			uint64_t m_version{1};
			float m_tolerance{DefaultTolerance};
			float m_length{0.0F};
			uint32_t m_segmentCount;
			bool m_enabled{true};
	};
}
