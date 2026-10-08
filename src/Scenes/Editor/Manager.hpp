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
 * https://github.com/EmeraudeEngine/emeraude-engine
 *
 * --- THIS IS AUTOMATICALLY GENERATED, DO NOT CHANGE ---
 */

#pragma once

/* Project configuration. */
#include "emeraude_export.hpp"

/* STL inclusions. */
#include <atomic>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <vector>

/* Local inclusions for inheritances. */
#include "Input/KeyboardListenerInterface.hpp"
#include "Input/PointerListenerInterface.hpp"

/* Local inclusions for usages. */
#include "Gizmo/Abstract.hpp"
#include "Gizmo/Rotate.hpp"
#include "Gizmo/Scale.hpp"
#include "Gizmo/Translate.hpp"
#include "Math/Space3D/Segment.hpp"

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
	enum class EMEN_API GizmoMode : uint8_t
	{
		Translate,
		Rotate,
		Scale
	};

	/**
	 * @brief The two states of the editor (owner decision 2026-09-29): the selection is kept apart from the gizmo.
	 * @note Selection: a click selects (Shift+click adds or removes), no gizmo is shown. Transformation: the gizmo of the
	 * current GizmoMode (translate, rotate, scale) acts on the WHOLE selection, around the centre of the selected
	 * entities; a click away from the gizmo changes nothing.
	 */
	enum class EMEN_API EditorState : uint8_t
	{
		Selection,
		Transformation
	};

	/** @brief The transform space for gizmo operations. */
	enum class EMEN_API TransformSpace : uint8_t
	{
		Local,
		World,
		Parent
	};

	/**
	 * @brief The scene editor manager. Handles entity selection via mouse picking and gizmo display.
	 *
	 * When activated, this manager registers itself as a keyboard and pointer listener
	 * with the input manager. In the Selection state, clicking on entities selects them (the scene outlines the
	 * selection: Scenes::Scene::setHighlightedEntities()); in the Transformation state, a standalone gizmo at the
	 * centre of the selection transforms it (EditorState).
	 *
	 * @extends EmEn::Input::KeyboardListenerInterface Listens to keyboard events when active.
	 * @extends EmEn::Input::PointerListenerInterface Listens to pointer events when active.
	 */
	class EMEN_API Manager final : public Input::KeyboardListenerInterface, public Input::PointerListenerInterface
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
			 * @note Screen-to-world computations read the main render target extent at
			 * event time (physical pixels, the pointer event coordinate space), so no
			 * viewport dimensions are captured here and window resizes are followed.
			 * @param scene A reference to the scene to edit.
			 * @param viewMatrices A reference to the view matrices for the main camera.
			 */
			void activate (Scene & scene, const Graphics::ViewMatricesInterface & viewMatrices) noexcept;

			/**
			 * @brief Deactivates the editor mode, clears selection and destroys gizmos.
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
			 */
			void processLogics () noexcept;

			/**
			 * @brief Records the gizmo draw commands into the command buffer.
			 * @param commandBuffer The active command buffer.
			 */
			void render (const Vulkan::CommandBuffer & commandBuffer) const noexcept;

			/**
			 * @brief Replaces the editor PANEL, drawn while the editor is active (Core's "SceneEditorScreen").
			 * @note Render thread (the overlay pass). The engine's default is an ImGUI window — the four tools, the transform
			 * space, the selection and the active entity's transform, read-only (owner decision 2026-09-29) — compiled with
			 * IMGUI_ENABLED only. An application replaces it by its own draw function, or removes it with nullptr; every
			 * panel drives the editor through the same public API (setState(), setGizmoMode(), setTransformSpace(),
			 * selection()).
			 * @warning Call it at setup (before the editor is first activated) or from the render thread: the overlay pass
			 * calls the function without a lock.
			 * @param drawFunction The function emitting the panel's ImGUI widgets, or nullptr for no panel.
			 */
			void setPanel (std::function< void () > drawFunction) noexcept;

			/**
			 * @brief Returns whether a panel is set.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			hasPanel () const noexcept
			{
				return m_panel != nullptr;
			}

			/**
			 * @brief Draws the panel, if any.
			 * @note Render thread, inside the overlay's ImGUI frame.
			 */
			void
			drawPanel () const noexcept
			{
				if ( m_panel != nullptr )
				{
					m_panel();
				}
			}

			/**
			 * @brief Sets the editor state: selecting, or transforming the selection with the gizmo.
			 * @param state The state.
			 */
			void setState (EditorState state) noexcept;

			/**
			 * @brief Returns the editor state.
			 * @return EditorState
			 */
			[[nodiscard]]
			EditorState
			state () const noexcept
			{
				return m_state;
			}

			/**
			 * @brief Sets the gizmo editing mode.
			 * @param mode The gizmo mode.
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
			 * @brief Returns the live selected entities, in the order they were selected (the last one is the active one).
			 * @return std::vector< std::shared_ptr< AbstractEntity > >
			 */
			[[nodiscard]]
			std::vector< std::shared_ptr< AbstractEntity > > selection () const noexcept;

			/**
			 * @brief Returns the ACTIVE entity — the last one selected, whose axes the Local transform space uses — or nullptr.
			 * @return std::shared_ptr< AbstractEntity >
			 */
			[[nodiscard]]
			std::shared_ptr< AbstractEntity > activeEntity () const noexcept;

			/**
			 * @brief Sets the gizmo screen size ratio (fraction of viewport height).
			 * @param ratio The ratio. Default is Gizmo::Abstract::DefaultScreenRatio (0.3).
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
			 */
			void
			setMoveRatio (float ratio) noexcept
			{
				m_moveRatio = ratio;
			}

			/**
			 * @brief Sets the movement step. 0 = free move, >0 = snap to grid.
			 * @param step The step size (e.g. 0.1, 1.0, 5.0). 0 disables snapping.
			 */
			void
			setMoveStep (float step) noexcept
			{
				m_moveStep = step;
			}

		private:

			/**
			 * @brief Builds a world-space ray segment from screen coordinates.
			 * @param screenX The X position in screen pixels.
			 * @param screenY The Y position in screen pixels.
			 * @return Base::Math::Space3D::Segment< float > The ray from near to far plane.
			 */
			[[nodiscard]]
			Base::Math::Space3D::Segment< float > screenToWorldRay (float screenX, float screenY) const noexcept;

			/**
			 * @brief Performs picking against all scene entities under the given screen position.
			 * @param screenX The X position in screen pixels.
			 * @param screenY The Y position in screen pixels.
			 * @return AbstractEntity * The closest hit entity, or nullptr if nothing was hit.
			 */
			[[nodiscard]]
			AbstractEntity * pickEntity (float screenX, float screenY) const noexcept;

			/**
			 * @brief Replaces the selection by one entity.
			 * @param entity A pointer to the entity to select.
			 */
			void setSelection (AbstractEntity * entity) noexcept;

			/**
			 * @brief Adds an entity to the selection, or removes it when it is already selected (Shift+click).
			 * @param entity A pointer to the entity.
			 */
			void toggleSelection (AbstractEntity * entity) noexcept;

			/**
			 * @brief Clears the selection.
			 */
			void clearSelection () noexcept;

			/**
			 * @brief The engine's default panel (ImGUI): the four tools, the transform space, the selection and the active
			 * entity's transform, read-only.
			 */
			void drawDefaultPanel () noexcept;

			/**
			 * @brief Hands the selection to the scene, which outlines it.
			 */
			void publishSelection () noexcept;

			/**
			 * @brief Returns the centre the gizmo transforms around: the mean of the selected entities' world positions.
			 * @param entities The live selection (not empty).
			 * @return Base::Math::Vector< 3, float >
			 */
			[[nodiscard]]
			static Base::Math::Vector< 3, float > selectionPivot (const std::vector< std::shared_ptr< AbstractEntity > > & entities) noexcept;

			/**
			 * @brief Starts a gizmo drag: captures the pivot and the initial state of every entity it moves.
			 * @note An entity whose ancestor is also selected is left out: it follows its parent, it must not move twice.
			 */
			void beginDrag () noexcept;

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
			float projectMouseOnAxis (float screenX, float screenY, const Base::Math::Vector< 3, float > & axisOrigin, const Base::Math::Vector< 3, float > & axisDirection) const noexcept;

			/**
			 * @brief Computes the angle of the mouse position projected onto a rotation plane.
			 * @param screenX Screen X coordinate.
			 * @param screenY Screen Y coordinate.
			 * @param planeOrigin The center of rotation in world space.
			 * @param planeNormal The normal of the rotation plane (the rotation axis).
			 * @return float The angle in radians.
			 */
			[[nodiscard]]
			float projectMouseAngleOnPlane (float screenX, float screenY, const Base::Math::Vector< 3, float > & planeOrigin, const Base::Math::Vector< 3, float > & planeNormal) const noexcept;

			/* References. */
			Input::Manager & m_inputManager;
			Resources::Manager & m_resourceManager;
			Notifier & m_notifier;

			/* Scene context. */
			Scene * m_scene{nullptr};
			const Graphics::ViewMatricesInterface * m_viewMatrices{nullptr};

			/** @brief One entity a gizmo drag moves, and its state when the drag started. */
			struct DragTarget final
			{
				std::weak_ptr< AbstractEntity > entity;
				Base::Math::Vector< 3, float > initialPosition;
				Base::Math::Vector< 3, float > initialScaling;
			};

			/* Selection state: held weakly (an entity may die while selected), the last one is the active one.
			 * Written by the input (main) thread, read by the logic thread: guarded. */
			std::vector< std::weak_ptr< AbstractEntity > > m_selection;
			mutable std::mutex m_selectionAccess;
			/** @brief Whether the gizmo is drawn (Transformation state with a selection), set by processLogics(), read by render(). */
			std::atomic_bool m_gizmoShown{false};

			/* Gizmos. */
			Gizmo::Translate m_translateGizmo;
			Gizmo::Rotate m_rotateGizmo;
			Gizmo::Scale m_scaleGizmo;

			/** @brief The panel draw function (setPanel()); the default one with IMGUI_ENABLED, else none. */
			std::function< void () > m_panel;

			/* Editing modes: set by the keys (main thread) and the panel (render thread), read by the logic thread. */
			std::atomic< EditorState > m_state{EditorState::Selection};
			std::atomic< GizmoMode > m_gizmoMode{GizmoMode::Translate};
			std::atomic< TransformSpace > m_transformSpace{TransformSpace::Local};
			float m_gizmoScreenRatio{Gizmo::Abstract::DefaultScreenRatio};

			/* Drag state (shared). */
			std::vector< DragTarget > m_dragTargets;
			Base::Math::Vector< 3, float > m_dragAxisDirection;
			/** @brief The centre of the selection when the drag started: the point every entity is moved, turned and scaled around. */
			Base::Math::Vector< 3, float > m_dragPivot;
			Gizmo::AxisID m_dragAxis{Gizmo::AxisID::None};

			/* Drag state (translation). */
			float m_dragInitialT{0.0F};

			/* Drag state (rotation). */
			float m_dragInitialAngle{0.0F};

			/* Drag state (scale). */
			float m_dragInitialMouseX{0.0F};

			/* Movement options. */
			float m_moveRatio{1.0F};
			float m_moveStep{0.0F};
			float m_rotateStep{0.0F};

			/* Activation. */
			float m_dragStartX{0.0F};
			float m_dragStartY{0.0F};
			bool m_active{false};
			bool m_dragActive{false};
			bool m_dragMoved{false};
	};
}
