/*
 * src/Scenes/Component/CloudVolume.hpp
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
#include <atomic>
#include <cstdint>
#include <cstring>
#include <memory>
#include <string>

/* Local inclusions for inheritances. */
#include "Abstract.hpp"
#include "ObserverTrait.hpp"

/* Local inclusions for usages. */
#include "Constants.hpp"
#include "Math/Space3D/AACuboid.hpp"
#include "Math/Vector.hpp"
#include "PixelFactory/Color.hpp"

/* Forward declarations. */
namespace EmEn
{
	namespace Graphics
	{
		class CloudShapeResource;
	}

	namespace Scenes
	{
		class BindlessTextureSet;
		class Scene;
	}
}

namespace EmEn::Scenes::Component
{
	/**
	 * @brief A volumetric cloud: a cloud SHAPE placed in the world by its entity.
	 * @note The entity IS the cloud's frame: its position is the centre of the cloud box, its
	 * orientation turns it and its scale stretches it — so a cloud is placed, turned and resized like
	 * any other entity (Toolkit, JSON scene, console, the editor gizmo). The renderer
	 * (Graphics::Effects::Atmosphere::VolumetricClouds) reads that frame from the PUBLISHED entity
	 * state every frame, and follows it.
	 * @note ⚠️ Owner decision (2026-09-24): scaling a cloud KEEPS ITS LOOK. The look is dimensionless —
	 * @ref Look::opticalThickness is the VERTICAL optical depth τ of the cloud at full density — and
	 * the renderer derives the extinction from the cloud's current world height (σt = τ / height).
	 * A physical density would have turned a shrunk cumulus into mist (a real cumulus is ~0.05 m⁻¹,
	 * opaque over a kilometre and barely there over 15 m).
	 * @note ⚠️ A cloud is NOT SOLID. Its render box drives the culling and, by default, the collision
	 * model too (@ref localBoundingBox() follows @ref renderBoundingBox()); the entity carrying it
	 * must be made non-collidable (`AbstractEntity::setCollidable(false)`), which keeps the bounding
	 * primitives the editor picks on. Toolkit::generateCloud() does it.
	 * @extends EmEn::Scenes::Component::Abstract The base class for each entity component.
	 * @extends EmEn::Base::ObserverTrait Waits for the shape to finish loading before registering it.
	 */
	class EMEN_API CloudVolume final : public Abstract, public Base::ObserverTrait
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"CloudVolume"};

			/** @brief The bindless index of a shape that is not registered (yet). */
			static constexpr uint32_t NoShapeIndex{UINT32_MAX};

			/** @brief Animatable Interface key. */
			enum AnimationID : uint8_t
			{
				OpticalThickness,
				Erosion
			};

			/**
			 * @brief How the cloud looks. Every quantity is DIMENSIONLESS or relative to the cloud, so a
			 * scale never changes it.
			 */
			struct EMEN_API Look
			{
				/** @brief Vertical optical depth τ across the cloud's height at full density. ~10 is a
				 * thin cloud, ~50 a thick cumulus whose inside is a white-out. */
				float opticalThickness{30.0F};
				/** @brief How deeply the detail noise erodes the shell, in [0, 1]. */
				float erosion{0.55F};
				/** @brief Detail noise cells across the cloud's width. */
				float detailFrequency{4.0F};
				/** @brief The drift of the detail noise — the "boiling" of a live cumulus — in cells per second. */
				float boilingSpeed{0.04F};
				/** @brief Single-scattering albedo σs/σt, a chromaticity in [0, 1]. Water droplets absorb
				 * almost nothing in the visible range: 1 is physical. */
				Base::PixelFactory::Color< float > scatteringAlbedo{1.0F, 1.0F, 1.0F, 1.0F};
			};

			/**
			 * @brief What the render thread reads, published once per logic tick.
			 */
			struct EMEN_API RenderState
			{
				Look look;
				/** @brief The half extents of the unscaled cloud box, in metres. */
				Base::Math::Vector< 3, float > halfExtents;
			};

			/**
			 * @brief Constructs a volumetric cloud with the default look (a cumulus).
			 * @param componentName A reference to a string.
			 * @param parentEntity A reference to the parent entity.
			 * @param shape A reference to the cloud shape smart pointer. It may still be loading.
			 * @param halfExtents The half extents of the unscaled cloud box, in metres. Use
			 * Graphics::CloudShapeResource::proportionsOf() to keep the shape's voxels cubic.
			 */
			CloudVolume (const std::string & componentName, const AbstractEntity & parentEntity, const std::shared_ptr< Graphics::CloudShapeResource > & shape, const Base::Math::Vector< 3, float > & halfExtents) noexcept;

			/**
			 * @brief Constructs a volumetric cloud.
			 * @note ⚠️ Two constructors, not a `= {}` default: a default argument of a NESTED type with
			 * member initializers is refused by GCC while the enclosing class is incomplete.
			 * @param componentName A reference to a string.
			 * @param parentEntity A reference to the parent entity.
			 * @param shape A reference to the cloud shape smart pointer. It may still be loading.
			 * @param halfExtents The half extents of the unscaled cloud box, in metres.
			 * @param look The initial look.
			 */
			CloudVolume (const std::string & componentName, const AbstractEntity & parentEntity, const std::shared_ptr< Graphics::CloudShapeResource > & shape, const Base::Math::Vector< 3, float > & halfExtents, const Look & look) noexcept;

			/**
			 * @brief Destructs the volumetric cloud.
			 */
			~CloudVolume () override;

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

			/** @copydoc EmEn::Scenes::Component::Abstract::move() */
			void
			move (const Base::Math::CartesianFrame< float > & /*worldCoordinates*/) noexcept override
			{
				/* NOTE: Nothing to follow here: the renderer reads the entity's published frame. */
			}

			/** @copydoc EmEn::Scenes::Component::Abstract::processLogics() */
			void processLogics (const Scene & scene) noexcept override;

			/** @copydoc EmEn::Scenes::Component::Abstract::shouldBeRemoved() */
			[[nodiscard]]
			bool
			shouldBeRemoved () const noexcept override
			{
				return false;
			}

			/** @copydoc EmEn::Scenes::Component::Abstract::publishStateForRendering() */
			void publishStateForRendering (uint32_t writeStateIndex) noexcept override;

			/**
			 * @brief Returns the state the render thread must use for a frame.
			 * @param readStateIndex The slot the frame latched (Scene::beginRenderFrame()).
			 * @return const RenderState &
			 */
			[[nodiscard]]
			const RenderState &
			renderState (uint32_t readStateIndex) const noexcept
			{
				return m_renderStates[readStateIndex];
			}

			/**
			 * @brief Returns the cloud shape.
			 * @return std::shared_ptr< Graphics::CloudShapeResource >
			 */
			[[nodiscard]]
			std::shared_ptr< Graphics::CloudShapeResource >
			shape () const noexcept
			{
				return m_shape;
			}

			/**
			 * @brief Returns the slot of the shape in the scene's bindless 3D array, or NoShapeIndex.
			 * @note [THREAD-SAFE] Written by whichever thread finishes the shape's loading, read by the
			 * render thread: a cloud whose shape is not registered yet is simply not drawn.
			 * @return uint32_t
			 */
			[[nodiscard]]
			uint32_t
			shapeBindlessIndex () const noexcept
			{
				return m_shapeBindlessIndex.load(std::memory_order_acquire);
			}

			/**
			 * @brief Changes the look [LOGIC THREAD].
			 * @param look A reference to the look.
			 * @return void
			 */
			void
			setLook (const Look & look) noexcept
			{
				m_look = look;
			}

			/**
			 * @brief Returns the look [LOGIC THREAD].
			 * @return const Look &
			 */
			[[nodiscard]]
			const Look &
			look () const noexcept
			{
				return m_look;
			}

			/**
			 * @brief Returns the half extents of the unscaled cloud box, in metres.
			 * @return const Base::Math::Vector< 3, float > &
			 */
			[[nodiscard]]
			const Base::Math::Vector< 3, float > &
			halfExtents () const noexcept
			{
				return m_halfExtents;
			}

			/**
			 * @brief Joins a scene: registers the shape in its bindless 3D array, now or once loaded.
			 * @note Called by Scenes::CloudSet::add().
			 * @param scene A reference to the scene.
			 * @return void
			 */
			void createOnHardware (Scene & scene) noexcept;

			/**
			 * @brief Leaves the scene: frees the bindless slot.
			 * @note Called by Scenes::CloudSet::remove() and by the destructor.
			 * @return void
			 */
			void destroyFromHardware () noexcept;

		private:

			/** @copydoc EmEn::Base::ObserverTrait::onNotification() */
			[[nodiscard]]
			bool onNotification (const Base::ObservableTrait * observable, int notificationCode, const std::any & data) noexcept override;

			/** @copydoc EmEn::Scenes::Component::Abstract::onSuspend() */
			void onSuspend () noexcept override { }

			/** @copydoc EmEn::Scenes::Component::Abstract::onWakeup() */
			void onWakeup () noexcept override { }

			/** @copydoc EmEn::Animations::AnimatableInterface::playAnimation() */
			bool playAnimation (uint8_t animationID, const Base::Variant & value, size_t cycle) noexcept override;

			/**
			 * @brief Registers the shape in the scene's bindless 3D array.
			 * @return void
			 */
			void registerShape () noexcept;

			std::shared_ptr< Graphics::CloudShapeResource > m_shape;
			BindlessTextureSet * m_bindlessTextureSet{nullptr};
			Look m_look;
			Base::Math::Vector< 3, float > m_halfExtents;
			Base::Math::Space3D::AACuboid< float > m_boundingBox;
			std::array< RenderState, RenderStateSlotCount > m_renderStates{};
			std::atomic< uint32_t > m_shapeBindlessIndex{NoShapeIndex};
	};
}