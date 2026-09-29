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
#include <cstdint>
#include <cstring>
#include <memory>
#include <string>

/* Local inclusions for inheritances. */
#include "Abstract.hpp"

/* Local inclusions for usages. */
#include "Graphics/Material/BeamResource.hpp"
#include "Graphics/RenderableInstance/Unique.hpp"
#include "Math/Matrix.hpp"
#include "Math/Vector.hpp"

/* Forward declarations. */
namespace EmEn::Resources
{
	class Manager;
}

namespace EmEn::Scenes::Component
{
	/**
	 * @brief A BEAM between two points: a laser, an electric arc — Half-Life's env_beam / env_laser, made
	 * photometric. An arc amplitude of 0 draws a straight laser; above it the beam wanders and re-strikes.
	 * @note The beam is a camera-facing ribbon the vertex stage builds (Graphics::Material::BeamResource,
	 * AbstractVertexStage::enableBeamRibbon()). Its two endpoints travel as the LOCAL TRANSFORMATION of its
	 * renderable instance — the unit segment [0, 1] on x placed from start to end — published per logic tick
	 * (publishStateForRendering(), RenderableInstance::Abstract::publishTransformationMatrix()). That is what keeps a
	 * moving beam race-free and gives it a real previous model matrix, hence a real velocity.
	 * @note The endpoints are in the ENTITY's space. The end can follow another entity instead (setEndTarget()): its
	 * world position is brought into this entity's space every tick.
	 * @note An emissive overlay: unlit, additive, no depth write, no shadow, out of the ray tracing. It does not light
	 * the scene (owner decision, 2026-09-28): pair it with a light component when it must.
	 * @extends EmEn::Scenes::Component::Abstract The base class for each entity component.
	 */
	class EMEN_LEAN_API Beam final : public Abstract
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"Beam"};

			/** @brief The default number of segments along the beam (the resolution of the arc). */
			static constexpr uint32_t DefaultSegmentCount{64};

			/**
			 * @brief Constructs a beam component.
			 * @note Creates the beam's own material (one per beam: its look is per beam) and its renderable, on the
			 * shared strip geometry of this segment count.
			 * @param componentName A reference to a string.
			 * @param parentEntity A reference to the parent entity.
			 * @param resources A reference to the resource manager.
			 * @param segmentCount The number of segments along the beam. A straight laser needs 1; an arc wants
			 * enough to show its finest octave. Default 64.
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
			 * @brief Sets the start of the beam, in the entity's space.
			 * @param position A reference to a vector.
			 * @return void
			 */
			void
			setStart (const Base::Math::Vector< 3, float > & position) noexcept
			{
				m_start = position;
			}

			/**
			 * @brief Returns the start of the beam, in the entity's space.
			 * @return const Base::Math::Vector< 3, float > &
			 */
			[[nodiscard]]
			const Base::Math::Vector< 3, float > &
			start () const noexcept
			{
				return m_start;
			}

			/**
			 * @brief Sets the end of the beam, in the entity's space. Forgets an end target.
			 * @param position A reference to a vector.
			 * @return void
			 */
			void
			setEnd (const Base::Math::Vector< 3, float > & position) noexcept
			{
				m_end = position;
				m_endTarget.reset();
			}

			/**
			 * @brief Returns the end of the beam, in the entity's space (the target's position when one is followed).
			 * @return const Base::Math::Vector< 3, float > &
			 */
			[[nodiscard]]
			const Base::Math::Vector< 3, float > &
			end () const noexcept
			{
				return m_end;
			}

			/**
			 * @brief Makes the end of the beam follow another entity, every logic tick.
			 * @param target A reference to the followed entity (held weakly: the beam falls back on its last end when
			 * the target is gone).
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
			 * @brief Shows or hides the beam (a laser switched off, an arc between two strikes of a longer cycle).
			 * @param state The state.
			 * @return void
			 */
			void
			setEnabled (bool state) noexcept
			{
				m_enabled = state;
			}

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
			 * @brief Builds the unit segment placement from the endpoints: x = end − start, y and z two unit
			 * directions across the beam, origin = start. A disabled or zero-length beam collapses (zero x column: the
			 * vertex stage draws nothing).
			 * @return Base::Math::Matrix< 4, float >
			 */
			[[nodiscard]]
			Base::Math::Matrix< 4, float > computeSegmentMatrix () const noexcept;

			/**
			 * @brief Refreshes the render bounds from the endpoints, the width and the arc amplitude, and notifies the
			 * entity when they changed.
			 * @return void
			 */
			void updateBounds () noexcept;

			std::shared_ptr< Graphics::Material::BeamResource > m_material;
			std::shared_ptr< Graphics::RenderableInstance::Unique > m_renderableInstance;
			std::weak_ptr< const AbstractEntity > m_endTarget;
			Base::Math::Matrix< 4, float > m_segmentMatrix;
			Base::Math::Space3D::AACuboid< float > m_boundingBox;
			Base::Math::Space3D::Sphere< float > m_boundingSphere;
			Base::Math::Vector< 3, float > m_start;
			Base::Math::Vector< 3, float > m_end{0.0F, 0.0F, 1.0F};
			Base::Math::Vector< 3, float > m_endTargetOffset;
			bool m_enabled{true};
	};
}
