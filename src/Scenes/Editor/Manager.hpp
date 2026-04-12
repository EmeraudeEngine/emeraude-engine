/*
 * src/Scenes/Editor/Manager.hpp
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
#include <cstdint>
#include <memory>

/* Local inclusions for inheritances. */
#include "Input/KeyboardListenerInterface.hpp"
#include "Input/PointerListenerInterface.hpp"

/* Local inclusions for usages. */
#include "Libs/Math/Space3D/Segment.hpp"
#include "Gizmo/Abstract.hpp"
#include "Gizmo/Translate.hpp"
#include "Gizmo/Rotate.hpp"

/* Forward declarations. */
namespace EmEn
{
	namespace Input
	{
		class Manager;
	}

	namespace Resources
	{
		class Manager;
	}

	namespace Graphics
	{
		class Renderer;
		class ViewMatricesInterface;

		namespace RenderTarget
		{
			class Abstract;
		}
	}

	namespace Vulkan
	{
		class CommandBuffer;
	}

	namespace Scenes
	{
		class Scene;
		class AbstractEntity;
	}

	class Notifier;
}

namespace EmEn::Scenes::Editor
{
	/** @brief The available gizmo editing modes. */
	enum class GizmoMode : uint8_t
	{
		Translate,
		Rotate,
		Scale
	};

	/** @brief The transform space for gizmo operations. */
	enum class TransformSpace : uint8_t
	{
		Local,
		World,
		Parent
	};

	/**
	 * @brief The scene editor manager. Handles entity selection via mouse picking and gizmo display.
	 *
	 * When activated, this manager registers itself as a keyboard and pointer listener
	 * with the input manager. Clicking on entities in the scene selects them, and a
	 * standalone gizmo is rendered at the selected entity's position.
	 *
	 * @extends EmEn::Input::KeyboardListenerInterface Listens to keyboard events when active.
	 * @extends EmEn::Input::PointerListenerInterface Listens to pointer events when active.
	 */
	class Manager final : public Input::KeyboardListenerInterface, public Input::PointerListenerInterface
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"SceneEditorManager"};

			/**
			 * @brief Constructs the scene editor manager.
			 * @param inputManager A reference to the input manager for listener registration.
			 * @param resourceManager A reference to the resource manager for gizmo mesh creation.
			 * @param notifier A reference to the system notification.
			 */
			Manager (Input::Manager & inputManager, Resources::Manager & resourceManager, Notifier & notifier) noexcept;

			/**
			 * @brief Destructs the scene editor manager. Deactivates if still active.
			 */
			~Manager () override;

			/** @brief Deleted copy/move. */
			Manager (const Manager &) noexcept = delete;
			Manager (Manager &&) noexcept = delete;
			Manager & operator= (const Manager &) noexcept = delete;
			Manager & operator= (Manager &&) noexcept = delete;

			/**
			 * @brief Activates the editor mode on a scene.
			 * @param scene A reference to the scene to edit.
			 * @param viewMatrices A reference to the view matrices for the main camera.
			 * @param viewportWidth The width of the viewport in pixels.
			 * @param viewportHeight The height of the viewport in pixels.
			 * @return void
			 */
			void activate (Scene & scene, const Graphics::ViewMatricesInterface & viewMatrices, float viewportWidth, float viewportHeight) noexcept;

			/**
			 * @brief Deactivates the editor mode, clears selection and destroys gizmos.
			 * @return void
			 */
			void deactivate () noexcept;

			/**
			 * @brief Returns whether the editor mode is active.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isActive () const noexcept
			{
				return m_active;
			}

			/**
			 * @brief Updates editor logic each frame (gizmo screen scale, hover).
			 * @return void
			 */
			void processLogics () noexcept;

			/**
			 * @brief Records the gizmo draw commands into the command buffer.
			 * @param commandBuffer The active command buffer.
			 * @return void
			 */
			void render (const Vulkan::CommandBuffer & commandBuffer) const noexcept;

			/**
			 * @brief Sets the gizmo editing mode.
			 * @param mode The gizmo mode.
			 * @return void
			 */
			void setGizmoMode (GizmoMode mode) noexcept;

			/**
			 * @brief Returns the current gizmo editing mode.
			 * @return GizmoMode
			 */
			[[nodiscard]]
			GizmoMode
			gizmoMode () const noexcept
			{
				return m_gizmoMode;
			}

			/**
			 * @brief Sets the transform space for gizmo operations.
			 * @param space The transform space.
			 * @return void
			 */
			void
			setTransformSpace (TransformSpace space) noexcept
			{
				m_transformSpace = space;
			}

			/**
			 * @brief Returns the current transform space.
			 * @return TransformSpace
			 */
			[[nodiscard]]
			TransformSpace
			transformSpace () const noexcept
			{
				return m_transformSpace;
			}

			/**
			 * @brief Returns the currently selected entity, or nullptr if none.
			 * @return AbstractEntity *
			 */
			[[nodiscard]]
			AbstractEntity *
			selectedEntity () const noexcept
			{
				return m_selectedEntity;
			}

			/**
			 * @brief Sets the gizmo screen size ratio (fraction of viewport height).
			 * @param ratio The ratio. Default is 0.025.
			 * @return void
			 */
			void
			setGizmoScreenRatio (float ratio) noexcept
			{
				m_gizmoScreenRatio = ratio;
			}

			/**
			 * @brief Returns the current gizmo screen size ratio.
			 * @return float
			 */
			[[nodiscard]]
			float
			gizmoScreenRatio () const noexcept
			{
				return m_gizmoScreenRatio;
			}

			/**
			 * @brief Sets the movement ratio for free move mode. Default 1.0.
			 * @param ratio The ratio multiplier.
			 * @return void
			 */
			void
			setMoveRatio (float ratio) noexcept
			{
				m_moveRatio = ratio;
			}

			/**
			 * @brief Sets the movement step. 0 = free move, >0 = snap to grid.
			 * @param step The step size (e.g. 0.1, 1.0, 5.0). 0 disables snapping.
			 * @return void
			 */
			void
			setMoveStep (float step) noexcept
			{
				m_moveStep = step;
			}

			/**
			 * @brief Updates the viewport dimensions (call when window is resized).
			 * @param viewportWidth The new width.
			 * @param viewportHeight The new height.
			 * @return void
			 */
			void
			updateViewport (float viewportWidth, float viewportHeight) noexcept
			{
				m_viewportWidth = viewportWidth;
				m_viewportHeight = viewportHeight;
			}

		private:

			/**
			 * @brief Builds a world-space ray segment from screen coordinates.
			 * @param screenX The X position in screen pixels.
			 * @param screenY The Y position in screen pixels.
			 * @return Libs::Math::Space3D::Segment< float > The ray from near to far plane.
			 */
			[[nodiscard]]
			Libs::Math::Space3D::Segment< float > screenToWorldRay (float screenX, float screenY) const noexcept;

			/**
			 * @brief Performs picking against all scene entities under the given screen position.
			 * @param screenX The X position in screen pixels.
			 * @param screenY The Y position in screen pixels.
			 * @return AbstractEntity * The closest hit entity, or nullptr if nothing was hit.
			 */
			[[nodiscard]]
			AbstractEntity * pickEntity (float screenX, float screenY) const noexcept;

			/**
			 * @brief Sets the selection to the given entity and shows the gizmo.
			 * @param entity A pointer to the entity to select.
			 * @return void
			 */
			void setSelection (AbstractEntity * entity) noexcept;

			/**
			 * @brief Clears the current selection and hides the gizmo.
			 * @return void
			 */
			void clearSelection () noexcept;

			/**
			 * @brief Creates the gizmo for the current mode if not already created.
			 * @return bool True if the gizmo is ready.
			 */
			[[nodiscard]]
			bool ensureGizmoCreated () noexcept;

			/* Input listener overrides. */

			/** @copydoc EmEn::Input::PointerListenerInterface::onPointerMove() */
			bool onPointerMove (float positionX, float positionY) noexcept override;

			/** @copydoc EmEn::Input::PointerListenerInterface::onButtonPress() */
			bool onButtonPress (float positionX, float positionY, int32_t buttonNumber, int32_t modifiers) noexcept override;

			/** @copydoc EmEn::Input::PointerListenerInterface::onButtonRelease() */
			bool onButtonRelease (float positionX, float positionY, int32_t buttonNumber, int32_t modifiers) noexcept override;

			/** @copydoc EmEn::Input::KeyboardListenerInterface::onKeyPress() */
			bool onKeyPress (int32_t key, int32_t scancode, int32_t modifiers, bool repeat) noexcept override;

			/**
			 * @brief Computes the closest point parameter on a world-space axis for a screen position.
			 * @param screenX Screen X coordinate.
			 * @param screenY Screen Y coordinate.
			 * @param axisOrigin The origin of the axis in world space.
			 * @param axisDirection The normalized direction of the axis in world space.
			 * @return float The t parameter along the axis (distance from origin).
			 */
			[[nodiscard]]
			float projectMouseOnAxis (float screenX, float screenY, const Libs::Math::Vector< 3, float > & axisOrigin, const Libs::Math::Vector< 3, float > & axisDirection) const noexcept;

			/**
			 * @brief Computes the angle of the mouse position projected onto a rotation plane.
			 * @param screenX Screen X coordinate.
			 * @param screenY Screen Y coordinate.
			 * @param planeOrigin The center of rotation in world space.
			 * @param planeNormal The normal of the rotation plane (the rotation axis).
			 * @return float The angle in radians.
			 */
			[[nodiscard]]
			float projectMouseAngleOnPlane (float screenX, float screenY, const Libs::Math::Vector< 3, float > & planeOrigin, const Libs::Math::Vector< 3, float > & planeNormal) const noexcept;

			/* References. */
			Input::Manager & m_inputManager;
			Resources::Manager & m_resourceManager;
			Notifier & m_notifier;

			/* Scene context. */
			Scene * m_scene{nullptr};
			const Graphics::ViewMatricesInterface * m_viewMatrices{nullptr};
			float m_viewportWidth{0.0F};
			float m_viewportHeight{0.0F};

			/* Selection state. */
			AbstractEntity * m_selectedEntity{nullptr};

			/* Gizmos. */
			Gizmo::Translate m_translateGizmo;
			Gizmo::Rotate m_rotateGizmo;

			/* Editing modes. */
			GizmoMode m_gizmoMode{GizmoMode::Translate};
			TransformSpace m_transformSpace{TransformSpace::Local};
			float m_gizmoScreenRatio{Gizmo::Abstract::DefaultScreenRatio};

			/* Drag state (shared). */
			Libs::Math::Vector< 3, float > m_dragAxisDirection;
			Libs::Math::Vector< 3, float > m_dragInitialEntityPos;
			Gizmo::AxisID m_dragAxis{Gizmo::AxisID::None};

			/* Drag state (translation). */
			float m_dragInitialT{0.0F};

			/* Drag state (rotation). */
			float m_dragInitialAngle{0.0F};

			/* Movement options. */
			float m_moveRatio{1.0F};
			float m_moveStep{0.0F};
			float m_rotateStep{0.0F};

			/* Activation. */
			bool m_active{false};
			bool m_dragActive{false};
	};
}
