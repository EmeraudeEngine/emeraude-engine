/*
 * src/Scenes/Editor/Manager.cpp
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

#include "Manager.hpp"

/* Project configuration. */
#include "emeraude_config.hpp"

/* STL inclusions. */
#include <algorithm>
#include <cmath>
#include <ranges>

/* Third-party inclusions. */
#ifdef IMGUI_ENABLED
#include "imgui.h"
#endif

/* Local inclusions. */
#include "Graphics/RenderTarget/Abstract.hpp"
#include "Graphics/Renderer.hpp"
#include "Graphics/ViewMatricesInterface.hpp"
#include "Input/Manager.hpp"
#include "Input/Types.hpp"
#include "Math/Space3D/Collisions/PointCuboid.hpp"
#include "Math/Space3D/Collisions/PointSphere.hpp"
#include "Math/Space3D/Intersections/SegmentCuboid.hpp"
#include "Math/Space3D/Intersections/SegmentSphere.hpp"
#include "Math/Space3D/Sphere.hpp"
#include "Notifier.hpp"
#include "Physics/BoxCollisionModel.hpp"
#include "Physics/CollisionModelInterface.hpp"
#include "Physics/SphereCollisionModel.hpp"
#include "Resources/Manager.hpp"
#include "Scenes/Node.hpp"
#include "Scenes/Scene.hpp"
#include "Scenes/StaticEntity.hpp"
#include "Tracer.hpp"
#include "Vulkan/CommandBuffer.hpp"

namespace EmEn::Scenes::Editor
{
	using namespace Base::Math;
	using namespace Base::Math::Space3D;
	using namespace Input;
	using namespace Physics;

	namespace
	{
		/**
		 * @brief Returns the shared owner of a picked entity (both concrete entity types share themselves).
		 * @param entity A pointer to the entity.
		 * @return std::shared_ptr< AbstractEntity >
		 */
		[[nodiscard]]
		std::shared_ptr< AbstractEntity >
		shareEntity (AbstractEntity * entity) noexcept
		{
			if ( auto * node = dynamic_cast< Node * >(entity) )
			{
				return node->shared_from_this();
			}

			if ( auto * staticEntity = dynamic_cast< StaticEntity * >(entity) )
			{
				return staticEntity->shared_from_this();
			}

			return nullptr;
		}

		/**
		 * @brief Returns whether one of the entity's ancestors is in a set (a node only: a static entity has none).
		 * @param entity A reference to the entity.
		 * @param entities The set.
		 * @return bool
		 */
		[[nodiscard]]
		bool
		hasSelectedAncestor (const AbstractEntity & entity, const std::vector< std::shared_ptr< AbstractEntity > > & entities) noexcept
		{
			const auto * node = dynamic_cast< const Node * >(&entity);

			if ( node == nullptr )
			{
				return false;
			}

			for ( auto parent = node->parent(); parent != nullptr; parent = parent->parent() )
			{
				if ( std::ranges::any_of(entities, [&parent] (const auto & selected) { return selected.get() == parent.get(); }) )
				{
					return true;
				}
			}

			return false;
		}
	}

	Manager::Manager (Input::Manager & inputManager, Resources::Manager & resourceManager, Notifier & notifier) noexcept
		: KeyboardListenerInterface{false, false},
		PointerListenerInterface{false, false, false},
		m_inputManager{inputManager},
		m_resourceManager{resourceManager},
		m_notifier{notifier}
	{
		/* NOTE: Register once. Listening is controlled by enableKeyboardListening/enablePointerListening. */
		m_inputManager.addKeyboardListener(this);
		m_inputManager.addPointerListener(this);

#ifdef IMGUI_ENABLED
		m_panel = [this] {
			this->drawDefaultPanel();
		};
#endif
	}

	void
	Manager::setPanel (std::function< void () > drawFunction) noexcept
	{
		m_panel = std::move(drawFunction);
	}

	void
	Manager::drawDefaultPanel () noexcept
	{
#ifdef IMGUI_ENABLED
		ImGui::SetNextWindowPos(ImVec2{16.0F, 16.0F}, ImGuiCond_FirstUseEver);

		/* The content grows and shrinks with the selection: the window follows it. */
		if ( !ImGui::Begin("Scene editor", nullptr, ImGuiWindowFlags_AlwaysAutoResize) )
		{
			ImGui::End();

			return;
		}

		/* The four tools: the Selection state, or the Transformation state with one gizmo. */
		const EditorState state = m_state;
		const GizmoMode gizmoMode = m_gizmoMode;

		const auto toolButton = [] (const char * label, const char * shortcut, bool current) noexcept {
			if ( current )
			{
				ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
			}

			const bool pressed = ImGui::Button(label);

			if ( current )
			{
				ImGui::PopStyleColor();
			}

			if ( ImGui::IsItemHovered() )
			{
				ImGui::SetTooltip("%s", shortcut);
			}

			return pressed;
		};

		if ( toolButton("Select", "Shift+Q", state == EditorState::Selection) )
		{
			this->setState(EditorState::Selection);
		}

		ImGui::SameLine();

		if ( toolButton("Move", "Shift+T", state == EditorState::Transformation && gizmoMode == GizmoMode::Translate) )
		{
			this->setGizmoMode(GizmoMode::Translate);
			this->setState(EditorState::Transformation);
		}

		ImGui::SameLine();

		if ( toolButton("Rotate", "Shift+R", state == EditorState::Transformation && gizmoMode == GizmoMode::Rotate) )
		{
			this->setGizmoMode(GizmoMode::Rotate);
			this->setState(EditorState::Transformation);
		}

		ImGui::SameLine();

		if ( toolButton("Scale", "Shift+S", state == EditorState::Transformation && gizmoMode == GizmoMode::Scale) )
		{
			this->setGizmoMode(GizmoMode::Scale);
			this->setState(EditorState::Transformation);
		}

		/* The transform space (Shift+G). */
		{
			int space = m_transformSpace == TransformSpace::World ? 1 : 0;

			ImGui::TextUnformatted("Space");
			ImGui::SameLine();

			const bool localPressed = ImGui::RadioButton("Local", &space, 0);

			ImGui::SameLine();

			const bool worldPressed = ImGui::RadioButton("World", &space, 1);

			if ( localPressed || worldPressed )
			{
				this->setTransformSpace(space == 1 ? TransformSpace::World : TransformSpace::Local);
			}
		}

		ImGui::Separator();

		/* The selection: every entity, the active one (the last selected) marked. */
		const auto entities = this->selection();

		if ( entities.empty() )
		{
			ImGui::TextDisabled("Nothing selected (click an entity; Shift+click adds or removes).");
		}
		else
		{
			ImGui::Text("%zu selected", entities.size());

			for ( size_t index = 0; index < entities.size(); ++index )
			{
				const bool active = index + 1 == entities.size();

				if ( active )
				{
					ImGui::BulletText("%s (active)", entities[index]->name().c_str());
				}
				else
				{
					ImGui::BulletText("%s", entities[index]->name().c_str());
				}
			}

			/* The active entity's world transform, read-only: the gizmo transforms. */
			ImGui::Separator();

			const auto frame = entities.back()->getWorldCoordinates();
			const auto & position = frame.position();
			const auto & scaling = frame.scalingFactor();

			ImGui::Text("Position  %.3f  %.3f  %.3f", static_cast< double >(position[X]), static_cast< double >(position[Y]), static_cast< double >(position[Z]));
			/* ZYX Tait-Bryan angles (Quaternion::eulerAngles()). ⚠️ NOT CartesianFrame::getPitchAngle()/getYawAngle()/
			 * getRollAngle(): those are the angles between the backward axis and -Z/+X/+Y (180/90/90 for an untouched
			 * entity), whatever their names say. */
			const auto euler = frame.toQuaternion().eulerAngles();

			ImGui::Text("Rotation  %.1f  %.1f  %.1f deg", static_cast< double >(Degree(euler[X])), static_cast< double >(Degree(euler[Y])), static_cast< double >(Degree(euler[Z])));
			ImGui::Text("Scale     %.3f  %.3f  %.3f", static_cast< double >(scaling[X]), static_cast< double >(scaling[Y]), static_cast< double >(scaling[Z]));

			if ( entities.size() > 1 )
			{
				const auto pivot = selectionPivot(entities);

				ImGui::TextDisabled("Centre    %.3f  %.3f  %.3f", static_cast< double >(pivot[X]), static_cast< double >(pivot[Y]), static_cast< double >(pivot[Z]));
			}
		}

		ImGui::End();
#endif
	}

	Manager::~Manager ()
	{
		if ( m_active )
		{
			this->deactivate();
		}

		m_inputManager.removeKeyboardListener(this);
		m_inputManager.removePointerListener(this);
	}

	void
	Manager::activate (Scene & scene, const Graphics::ViewMatricesInterface & viewMatrices) noexcept
	{
		if ( m_active )
		{
			return;
		}

		m_scene = &scene;
		m_viewMatrices = &viewMatrices;
		m_active = true;

		/* NOTE: Enable input listening. The editor is permanently registered
		 * as a listener; activation just enables the filtering flags. */
		this->enableAbsoluteMode();
		this->enableKeyboardListening(true);
		this->enablePointerListening(true);

		/* NOTE: Unlock the pointer so the user can click freely. */
		m_inputManager.unlockPointer();

		/* NOTE: Pre-create ALL gizmo GPU resources so they are uploaded before first render. */
		{
			auto & renderer = m_resourceManager.graphicsRenderer();
			const auto renderTarget = renderer.mainRenderTarget();

			if ( !m_translateGizmo.isCreated() && !m_translateGizmo.create(renderer, m_resourceManager, renderTarget) )
			{
				Tracer::warning(ClassId, "Failed to pre-create translate gizmo.");
			}

			if ( !m_rotateGizmo.isCreated() && !m_rotateGizmo.create(renderer, m_resourceManager, renderTarget) )
			{
				Tracer::warning(ClassId, "Failed to pre-create rotate gizmo.");
			}

			if ( !m_scaleGizmo.isCreated() && !m_scaleGizmo.create(renderer, m_resourceManager, renderTarget) )
			{
				Tracer::warning(ClassId, "Failed to pre-create scale gizmo.");
			}
		}

		m_notifier.push("Editor mode activated.");
	}

	void
	Manager::deactivate () noexcept
	{
		if ( !m_active )
		{
			return;
		}

		this->clearSelection();

		this->enableKeyboardListening(false);
		this->enablePointerListening(false);

		/* NOTE: Destroy all gizmo GPU resources. */
		m_translateGizmo.destroy();
		m_rotateGizmo.destroy();
		m_scaleGizmo.destroy();

		m_scene = nullptr;
		m_viewMatrices = nullptr;
		m_active = false;

		m_notifier.push("Editor mode deactivated.");
	}

	void
	Manager::processLogics () noexcept
	{
		if ( !m_active || m_viewMatrices == nullptr )
		{
			m_gizmoShown = false;

			return;
		}

		const auto entities = this->selection();

		/* The gizmo is a tool on the selection, not its marker (the outline marks it): Transformation state only. */
		if ( m_state != EditorState::Transformation || entities.empty() )
		{
			m_gizmoShown = false;

			return;
		}

		/* NOTE: The gizmo sits at the centre of the selection, oriented like the ACTIVE entity in Local mode. */
		auto worldFrame = entities.back()->getWorldCoordinates();
		worldFrame.setPosition(selectionPivot(entities));

		if ( m_transformSpace == TransformSpace::World )
		{
			/* NOTE: In World mode, keep position but reset rotation to identity. */
			worldFrame.resetRotation();
		}

		m_translateGizmo.setWorldFrame(worldFrame);
		m_rotateGizmo.setWorldFrame(worldFrame);
		m_scaleGizmo.setWorldFrame(worldFrame);

		/* NOTE: Update gizmo scale for constant screen size (uses configurable ratio).
		 * ⚠️ ViewMatricesInterface::fieldOfView() is in DEGREES, updateScreenScale() takes radians. Fed raw until
		 * Sep 2026, tan(42.5 rad) = -11.3 at 85°: a NEGATIVE scale that point-mirrored the whole gizmo (X and Y arrows
		 * reversed on screen, so a drag moved against the arrow held) and made its size swing with the FOV. */
		const auto fieldOfView = Radian(m_viewMatrices->fieldOfView());

		m_translateGizmo.updateScreenScale(m_viewMatrices->position(), fieldOfView, m_gizmoScreenRatio);
		m_rotateGizmo.updateScreenScale(m_viewMatrices->position(), fieldOfView, m_gizmoScreenRatio);
		m_scaleGizmo.updateScreenScale(m_viewMatrices->position(), fieldOfView, m_gizmoScreenRatio);

		m_gizmoShown = true;
	}

	void
	Manager::render (const Vulkan::CommandBuffer & commandBuffer) const noexcept
	{
		if ( !m_active || !m_gizmoShown || m_viewMatrices == nullptr )
		{
			return;
		}

		/* NOTE: Render the active gizmo based on current mode. */
		switch ( m_gizmoMode )
		{
			case GizmoMode::Translate :
				if ( m_translateGizmo.isCreated() )
				{
					m_translateGizmo.render(commandBuffer, *m_viewMatrices);
				}
				break;

			case GizmoMode::Rotate :
				if ( m_rotateGizmo.isCreated() )
				{
					m_rotateGizmo.render(commandBuffer, *m_viewMatrices);
				}
				break;

			case GizmoMode::Scale :
				if ( m_scaleGizmo.isCreated() )
				{
					m_scaleGizmo.render(commandBuffer, *m_viewMatrices);
				}
				break;
		}
	}

	void
	Manager::setGizmoMode (GizmoMode mode) noexcept
	{
		if ( m_gizmoMode == mode )
		{
			return;
		}

		m_gizmoMode = mode;

		/* NOTE: Ensure the gizmo for the new mode is ready. */
		if ( !this->ensureGizmoCreated() )
		{
			Tracer::warning(ClassId, "Failed to create gizmo for new mode.");
		}

		TraceInfo{ClassId} << "Gizmo mode: " << static_cast< int >(mode);
	}

	bool
	Manager::ensureGizmoCreated () noexcept
	{
		auto & renderer = m_resourceManager.graphicsRenderer();

		switch ( m_gizmoMode )
		{
			case GizmoMode::Translate :
			{
				if ( !m_translateGizmo.isCreated() )
				{
					return m_translateGizmo.create(renderer, m_resourceManager, renderer.mainRenderTarget());
				}

				return true;
			}

			case GizmoMode::Rotate :
			{
				if ( !m_rotateGizmo.isCreated() )
				{
					return m_rotateGizmo.create(renderer, m_resourceManager, renderer.mainRenderTarget());
				}

				return true;
			}

			case GizmoMode::Scale :
				/* TODO: Implement scale gizmo. */
				return false;
		}

		return false;
	}

	Segment< float >
	Manager::screenToWorldRay (float screenX, float screenY) const noexcept
	{
		/* NOTE: Pointer events are dispatched in PHYSICAL framebuffer pixels on every platform
		 * (see Core::updatePointerScaling()), so the NDC conversion must use the render target
		 * extent — not the logical window size, which differs by the content scale on HiDPI.
		 * Queried at event time so window resizes are followed. */
		const auto renderTarget = m_resourceManager.graphicsRenderer().mainRenderTarget();

		if ( renderTarget == nullptr )
		{
			return {};
		}

		const auto & extent = renderTarget->extent();

		if ( extent.width == 0 || extent.height == 0 )
		{
			return {};
		}

		const float ndcX = (2.0F * screenX / static_cast< float >(extent.width)) - 1.0F;
		const float ndcY = (2.0F * screenY / static_cast< float >(extent.height)) - 1.0F;

		const auto & projMatrix = m_viewMatrices->projectionMatrix();
		const auto & viewMatrix = m_viewMatrices->viewMatrix(false, 0);
		const auto inverseVP = (projMatrix * viewMatrix).inverse();

		const Vector< 4, float > nearClip{ndcX, ndcY, 0.0F, 1.0F};
		const Vector< 4, float > farClip{ndcX, ndcY, 1.0F, 1.0F};

		auto nearWorld = inverseVP * nearClip;
		auto farWorld = inverseVP * farClip;

		if ( std::abs(nearWorld[3]) > std::numeric_limits< float >::epsilon() )
		{
			nearWorld = nearWorld / nearWorld[3];
		}

		if ( std::abs(farWorld[3]) > std::numeric_limits< float >::epsilon() )
		{
			farWorld = farWorld / farWorld[3];
		}

		return Segment< float >{
			Vector< 3, float >{nearWorld[0], nearWorld[1], nearWorld[2]},
			Vector< 3, float >{farWorld[0], farWorld[1], farWorld[2]}
		};
	}

	AbstractEntity *
	Manager::pickEntity (float screenX, float screenY) const noexcept
	{
		if ( m_scene == nullptr || m_viewMatrices == nullptr )
		{
			return nullptr;
		}

		const auto ray = this->screenToWorldRay(screenX, screenY);

		if ( !ray.isValid() )
		{
			return nullptr;
		}

		const auto & cameraPos = m_viewMatrices->position();

		AbstractEntity * closestEntity = nullptr;
		float closestDistance = std::numeric_limits< float >::max();

		auto testEntity = [&] (AbstractEntity & entity)
		{
			if ( !entity.hasCollisionModel() )
			{
				return;
			}

			const auto * model = entity.collisionModel();
			const auto worldFrame = entity.getWorldCoordinates();

			switch ( model->modelType() )
			{
				case CollisionModelType::Sphere :
				{
					const Sphere< float > worldSphere{model->getRadius(), worldFrame.position()};

					/* NOTE: A volume containing the point of view (typically the entity carrying
					 * the active camera) would catch every click at near-zero distance. */
					if ( isColliding(cameraPos, worldSphere) )
					{
						return;
					}

					if ( isIntersecting(ray, worldSphere) )
					{
						const float distance = (worldFrame.position() - cameraPos).length();

						if ( distance < closestDistance )
						{
							closestDistance = distance;
							closestEntity = &entity;
						}
					}
				}
					break;

				case CollisionModelType::Box :
				case CollisionModelType::Capsule :
				{
					const auto worldAABB = model->getAABB(worldFrame);

					/* NOTE: A volume containing the point of view (typically the entity carrying
					 * the active camera) would catch every click at near-zero distance. */
					if ( isColliding(cameraPos, worldAABB) )
					{
						return;
					}

					Point< float > hitPoint;

					if ( isIntersecting(ray, worldAABB, hitPoint) )
					{
						const float distance = (hitPoint - cameraPos).length();

						if ( distance < closestDistance )
						{
							closestDistance = distance;
							closestEntity = &entity;
						}
					}
				}
					break;

				case CollisionModelType::Point :
					break;
			}
		};

		m_scene->forEachStaticEntities([&testEntity] (StaticEntity & entity) {
			testEntity(entity);
		});

		if ( const auto & rootNode = m_scene->root(); rootNode != nullptr )
		{
			std::function< void(Node &) > traverseNodes = [&] (Node & node)
			{
				testEntity(node);

				for ( auto & child : node.children() | std::views::values )
				{
					if ( child != nullptr )
					{
						traverseNodes(*child);
					}
				}
			};

			traverseNodes(*rootNode);
		}

		return closestEntity;
	}

	void
	Manager::setState (EditorState state) noexcept
	{
		if ( m_state == state )
		{
			return;
		}

		m_state = state;

		m_notifier.push(state == EditorState::Selection ? "Editor: selection" : "Editor: transformation");
	}

	std::vector< std::shared_ptr< AbstractEntity > >
	Manager::selection () const noexcept
	{
		const std::scoped_lock lock{m_selectionAccess};

		std::vector< std::shared_ptr< AbstractEntity > > entities;
		entities.reserve(m_selection.size());

		for ( const auto & held : m_selection )
		{
			if ( auto entity = held.lock(); entity != nullptr )
			{
				entities.emplace_back(std::move(entity));
			}
		}

		return entities;
	}

	std::shared_ptr< AbstractEntity >
	Manager::activeEntity () const noexcept
	{
		const std::scoped_lock lock{m_selectionAccess};

		for ( const auto & held : std::views::reverse(m_selection) )
		{
			if ( auto entity = held.lock(); entity != nullptr )
			{
				return entity;
			}
		}

		return nullptr;
	}

	void
	Manager::setSelection (AbstractEntity * entity) noexcept
	{
		auto shared = shareEntity(entity);

		if ( shared == nullptr )
		{
			this->clearSelection();

			return;
		}

		{
			const std::scoped_lock lock{m_selectionAccess};

			m_selection.clear();
			m_selection.emplace_back(shared);
		}

		this->publishSelection();

		m_notifier.push("Selected entity: '" + shared->name() + "'");
	}

	void
	Manager::toggleSelection (AbstractEntity * entity) noexcept
	{
		const auto shared = shareEntity(entity);

		if ( shared == nullptr )
		{
			return;
		}

		bool removed = false;
		size_t count = 0;

		{
			const std::scoped_lock lock{m_selectionAccess};

			/* The dead are dropped on the way. */
			const auto sizeBefore = m_selection.size();

			std::erase_if(m_selection, [&shared] (const auto & held) { return held.lock() == shared; });

			removed = m_selection.size() < sizeBefore;

			std::erase_if(m_selection, [] (const auto & held) { return held.expired(); });

			if ( !removed )
			{
				/* Last = active: the entity just added becomes the one the Local axes follow. */
				m_selection.emplace_back(shared);
			}

			count = m_selection.size();
		}

		this->publishSelection();

		m_notifier.push((removed ? "Deselected '" : "Added '") + shared->name() + "' (" + std::to_string(count) + " selected)");
	}

	void
	Manager::clearSelection () noexcept
	{
		{
			const std::scoped_lock lock{m_selectionAccess};

			m_selection.clear();
		}

		this->publishSelection();
	}

	void
	Manager::publishSelection () noexcept
	{
		/* The scene outlines the selection (Graphics::SelectionOutline), in one step. */
		if ( m_scene != nullptr )
		{
			m_scene->setHighlightedEntities(this->selection());
		}
	}

	Vector< 3, float >
	Manager::selectionPivot (const std::vector< std::shared_ptr< AbstractEntity > > & entities) noexcept
	{
		Vector< 3, float > sum;

		for ( const auto & entity : entities )
		{
			sum += entity->getWorldCoordinates().position();
		}

		return sum / static_cast< float >(entities.size());
	}

	void
	Manager::beginDrag () noexcept
	{
		const auto entities = this->selection();

		m_dragTargets.clear();

		if ( entities.empty() )
		{
			return;
		}

		m_dragPivot = selectionPivot(entities);

		for ( const auto & entity : entities )
		{
			if ( hasSelectedAncestor(*entity, entities) )
			{
				continue;
			}

			const auto & frame = entity->getWorldCoordinates();

			m_dragTargets.push_back({entity, frame.position(), frame.scalingFactor()});
		}
	}

	float
	Manager::projectMouseOnAxis (float screenX, float screenY, const Vector< 3, float > & axisOrigin, const Vector< 3, float > & axisDirection) const noexcept
	{
		const auto ray = this->screenToWorldRay(screenX, screenY);

		if ( !ray.isValid() )
		{
			return 0.0F;
		}

		/* NOTE: Closest point between two lines (axis line and mouse ray).
		 * Axis: P(t) = axisOrigin + t * axisDir
		 * Ray:  Q(s) = rayStart  + s * rayDir
		 * Solve for t that minimizes distance. */
		const auto rayDir = (ray.endPoint() - ray.startPoint()).normalize();
		const auto c = ray.startPoint() - axisOrigin;

		const float aa = Vector< 3, float >::dotProduct(axisDirection, axisDirection);
		const float ab = Vector< 3, float >::dotProduct(axisDirection, rayDir);
		const float bb = Vector< 3, float >::dotProduct(rayDir, rayDir);
		const float ca = Vector< 3, float >::dotProduct(c, axisDirection);
		const float cb = Vector< 3, float >::dotProduct(c, rayDir);

		const float denom = (aa * bb) - (ab * ab);

		if ( std::abs(denom) < 0.0001F )
		{
			return 0.0F;
		}

		return (cb * ab - ca * bb) / denom;
	}

	float
	Manager::projectMouseAngleOnPlane (float screenX, float screenY, const Vector< 3, float > & planeOrigin, const Vector< 3, float > & planeNormal) const noexcept
	{
		const auto ray = this->screenToWorldRay(screenX, screenY);

		if ( !ray.isValid() )
		{
			return 0.0F;
		}

		/* NOTE: Intersect ray with the rotation plane.
		 * Plane: dot(P - planeOrigin, planeNormal) = 0
		 * Ray: P = rayStart + t * rayDir
		 * t = dot(planeOrigin - rayStart, planeNormal) / dot(rayDir, planeNormal) */
		const auto rayDir = (ray.endPoint() - ray.startPoint()).normalize();
		const float denom = Vector< 3, float >::dotProduct(rayDir, planeNormal);

		if ( std::abs(denom) < 0.0001F )
		{
			return 0.0F;
		}

		const float t = Vector< 3, float >::dotProduct(planeOrigin - ray.startPoint(), planeNormal) / denom;
		const auto hitPoint = ray.startPoint() + rayDir * t;

		/* NOTE: Project hit point into the plane's 2D coordinate system.
		 * Build two orthogonal axes on the plane. */
		const auto localHit = hitPoint - planeOrigin;

		/* NOTE: Choose a reference direction not parallel to the normal. */
		Vector< 3, float > refDir{1.0F, 0.0F, 0.0F};

		if ( std::abs(Vector< 3, float >::dotProduct(refDir, planeNormal)) > 0.9F )
		{
			refDir = Vector< 3, float >{0.0F, 1.0F, 0.0F};
		}

		const auto u = Vector< 3, float >::crossProduct(planeNormal, refDir).normalize();
		const auto v = Vector< 3, float >::crossProduct(planeNormal, u).normalize();

		const float x = Vector< 3, float >::dotProduct(localHit, u);
		const float y = Vector< 3, float >::dotProduct(localHit, v);

		return std::atan2(y, x);
	}

	bool
	Manager::onPointerMove (float positionX, float positionY) noexcept
	{
		/* NOTE: Handle drag if active. Every drag target moves around the pivot captured at the drag start. */
		if ( m_dragActive )
		{
			m_dragMoved = true;

			if ( m_gizmoMode == GizmoMode::Translate )
			{
				const float currentT = this->projectMouseOnAxis(positionX, positionY, m_dragPivot, m_dragAxisDirection);
				float delta = (m_dragInitialT - currentT) * m_moveRatio;

				if ( m_moveStep > 0.0F )
				{
					delta = std::round(delta / m_moveStep) * m_moveStep;
				}

				/* Absolute from the drag start: no accumulation. */
				const auto offset = m_dragAxisDirection * delta;

				for ( const auto & target : m_dragTargets )
				{
					if ( const auto entity = target.entity.lock(); entity != nullptr )
					{
						entity->setPosition(target.initialPosition + offset, Base::Math::TransformSpace::World);
					}
				}
			}
			else if ( m_gizmoMode == GizmoMode::Rotate )
			{
				const float currentAngle = this->projectMouseAngleOnPlane(positionX, positionY, m_dragPivot, m_dragAxisDirection);
				float deltaAngle = (currentAngle - m_dragInitialAngle);

				if ( m_rotateStep > 0.0F )
				{
					deltaAngle = std::round(deltaAngle / m_rotateStep) * m_rotateStep;
				}

				/* NOTE: Each entity turns ON ITSELF around the axis (rotate() + its position restored), then its offset from
				 * the pivot turns by the same matrix CartesianFrame::rotate() uses: a group orbits its centre, a single
				 * entity (offset 0) turns in place as before.
				 * ⚠️ Node::rotate(World) turns the node's LOCAL frame, i.e. in its parent's space: exact for a child of the
				 * root node only (docs/todo: editor-world-rotation-of-nested-nodes). */
				const auto rotation = Matrix< 3, float >::rotation(deltaAngle, m_dragAxisDirection);

				/* One entity in Local mode keeps the exact local-space path (a nested node included). */
				const bool localSingle = m_transformSpace == TransformSpace::Local && m_dragTargets.size() == 1;

				for ( const auto & target : m_dragTargets )
				{
					const auto entity = target.entity.lock();

					if ( entity == nullptr )
					{
						continue;
					}

					if ( localSingle )
					{
						Vector< 3, float > localAxis{0.0F, 0.0F, 0.0F};

						switch ( m_dragAxis )
						{
							case Gizmo::AxisID::X : localAxis = {1.0F, 0.0F, 0.0F}; break;
							case Gizmo::AxisID::Y : localAxis = {0.0F, 1.0F, 0.0F}; break;
							case Gizmo::AxisID::Z : localAxis = {0.0F, 0.0F, 1.0F}; break;
							default : break;
						}

						entity->rotate(deltaAngle, localAxis, Base::Math::TransformSpace::Local);

						continue;
					}

					const auto position = entity->getWorldCoordinates().position();

					entity->rotate(deltaAngle, m_dragAxisDirection, Base::Math::TransformSpace::World);
					entity->setPosition(m_dragPivot + rotation * (position - m_dragPivot), Base::Math::TransformSpace::World);
				}

				/* NOTE: Update initial angle for next delta. */
				m_dragInitialAngle = currentAngle;
			}
			else if ( m_gizmoMode == GizmoMode::Scale )
			{
				/* NOTE: Absolute scale from horizontal mouse delta.
				 * Right = bigger, Left = smaller. ~300px = double size. */
				const float pixelDelta = (positionX - m_dragInitialMouseX) - (positionY - m_dragInitialT);
				const float factor = std::max(0.01F, 1.0F + (pixelDelta * 0.003F * m_moveRatio));

				for ( const auto & target : m_dragTargets )
				{
					const auto entity = target.entity.lock();

					if ( entity == nullptr )
					{
						continue;
					}

					auto newScaling = target.initialScaling;
					const auto offset = target.initialPosition - m_dragPivot;
					Vector< 3, float > newOffset;

					/* The entity scales on its own axis; its offset from the pivot along the dragged direction. */
					switch ( m_dragAxis )
					{
						case Gizmo::AxisID::X : newScaling[0] = target.initialScaling[0] * factor; break;
						case Gizmo::AxisID::Y : newScaling[1] = target.initialScaling[1] * factor; break;
						case Gizmo::AxisID::Z : newScaling[2] = target.initialScaling[2] * factor; break;
						default : newScaling = target.initialScaling * factor; break;
					}

					if ( m_dragAxis == Gizmo::AxisID::X || m_dragAxis == Gizmo::AxisID::Y || m_dragAxis == Gizmo::AxisID::Z )
					{
						newOffset = offset + m_dragAxisDirection * (Vector< 3, float >::dotProduct(offset, m_dragAxisDirection) * (factor - 1.0F));
					}
					else
					{
						newOffset = offset * factor;
					}

					entity->setScalingFactor(newScaling);
					entity->setPosition(m_dragPivot + newOffset, Base::Math::TransformSpace::World);
				}
			}

			return true;
		}

		/* NOTE: Update gizmo hover highlight on the active gizmo. */
		if ( m_gizmoShown )
		{
			const auto ray = this->screenToWorldRay(positionX, positionY);

			if ( ray.isValid() )
			{
				switch ( m_gizmoMode )
				{
					case GizmoMode::Translate :
						if ( m_translateGizmo.isCreated() )
						{
							m_translateGizmo.setHighlightedAxis(m_translateGizmo.hitTest(ray));
						}
						break;

					case GizmoMode::Rotate :
						if ( m_rotateGizmo.isCreated() )
						{
							m_rotateGizmo.setHighlightedAxis(m_rotateGizmo.hitTest(ray));
						}
						break;

					case GizmoMode::Scale :
						if ( m_scaleGizmo.isCreated() )
						{
							m_scaleGizmo.setHighlightedAxis(m_scaleGizmo.hitTest(ray));
						}
						break;
				}
			}
		}

		return false;
	}

	bool
	Manager::onButtonPress (float positionX, float positionY, int32_t buttonNumber, int32_t modifiers) noexcept
	{
		if ( buttonNumber != Button1Left )
		{
			return false;
		}

		/* NOTE: Selection state: a click selects, Shift+click adds or removes, a click on nothing clears. */
		if ( m_state == EditorState::Selection )
		{
			auto * entity = this->pickEntity(positionX, positionY);

			if ( isKeyboardModifierPressed(ModKeyShift, modifiers) )
			{
				if ( entity != nullptr )
				{
					this->toggleSelection(entity);
				}
			}
			else if ( entity != nullptr )
			{
				this->setSelection(entity);
			}
			else
			{
				this->clearSelection();
			}

			return true;
		}

		/* NOTE: Transformation state: only the gizmo reacts; a click away from it changes nothing (owner decision). */
		const Gizmo::Abstract * activeGizmo = nullptr;

		switch ( m_gizmoMode )
		{
			case GizmoMode::Translate :
				if ( m_translateGizmo.isCreated() ) { activeGizmo = &m_translateGizmo; }
				break;

			case GizmoMode::Rotate :
				if ( m_rotateGizmo.isCreated() ) { activeGizmo = &m_rotateGizmo; }
				break;

			case GizmoMode::Scale :
				if ( m_scaleGizmo.isCreated() ) { activeGizmo = &m_scaleGizmo; }
				break;
		}

		const auto active = this->activeEntity();

		if ( !m_gizmoShown || active == nullptr || activeGizmo == nullptr )
		{
			return true;
		}

		const auto ray = this->screenToWorldRay(positionX, positionY);

		if ( !ray.isValid() )
		{
			return true;
		}

		const auto hitAxis = activeGizmo->hitTest(ray);

		if ( hitAxis == Gizmo::AxisID::None )
		{
			return true;
		}

		/* NOTE: Start drag operation, around the centre of the selection. */
		this->beginDrag();

		if ( m_dragTargets.empty() )
		{
			return true;
		}

		m_dragActive = true;
		m_dragMoved = false;
		m_dragStartX = positionX;
		m_dragStartY = positionY;
		m_dragAxis = hitAxis;

		/* NOTE: Determine axis direction based on transform space: Local follows the ACTIVE entity. */
		const auto & entityFrame = active->getWorldCoordinates();

		switch ( hitAxis )
		{
			case Gizmo::AxisID::X :
				m_dragAxisDirection = (m_transformSpace == TransformSpace::Local)
					? entityFrame.rightVector()
					: Vector< 3, float >{1.0F, 0.0F, 0.0F};
				break;

			case Gizmo::AxisID::Y :
				m_dragAxisDirection = (m_transformSpace == TransformSpace::Local)
					? entityFrame.localYAxis()
					: Vector< 3, float >{0.0F, 1.0F, 0.0F};
				break;

			case Gizmo::AxisID::Z :
				m_dragAxisDirection = (m_transformSpace == TransformSpace::Local)
					? entityFrame.backwardVector()
					: Vector< 3, float >{0.0F, 0.0F, 1.0F};
				break;

			default :
				break;
		}

		/* NOTE: Initialize mode-specific drag state. */
		if ( m_gizmoMode == GizmoMode::Translate )
		{
			m_dragInitialT = this->projectMouseOnAxis(positionX, positionY, m_dragPivot, m_dragAxisDirection);
		}
		else if ( m_gizmoMode == GizmoMode::Rotate )
		{
			m_dragInitialAngle = this->projectMouseAngleOnPlane(positionX, positionY, m_dragPivot, m_dragAxisDirection);
		}
		else if ( m_gizmoMode == GizmoMode::Scale )
		{
			m_dragInitialMouseX = positionX;
			m_dragInitialT = positionY;
		}

		return true;
	}

	bool
	Manager::onButtonRelease (float /*positionX*/, float /*positionY*/, int32_t buttonNumber, int32_t /*modifiers*/) noexcept
	{
		if ( buttonNumber != Button1Left )
		{
			return false;
		}

		if ( m_dragActive )
		{
			m_dragActive = false;
			m_dragMoved = false;
			m_dragAxis = Gizmo::AxisID::None;
			m_dragTargets.clear();

			return true;
		}

		return false;
	}

	bool
	Manager::onKeyPress (int32_t key, int32_t /*scancode*/, int32_t modifiers, bool repeat) noexcept
	{
		if ( repeat )
		{
			return false;
		}

		/* NOTE: Escape key clears the selection. */
		if ( key == KeyEscape )
		{
			if ( !this->selection().empty() )
			{
				this->clearSelection();

				return true;
			}
		}

		/* NOTE: Shift+key shortcuts for editor controls. */
		if ( isKeyboardModifierPressed(ModKeyShift, modifiers) )
		{
			switch ( key )
			{
				/* Shift+G: cycle transform space (Local → World → Parent → Local). */
				case KeyG :
				{
					switch ( m_transformSpace )
					{
						case TransformSpace::Local :
							m_transformSpace = TransformSpace::World;
							m_notifier.push("Transform space: World");
							break;

						case TransformSpace::World :
						case TransformSpace::Parent :
							m_transformSpace = TransformSpace::Local;
							m_notifier.push("Transform space: Local");
							break;
					}

					return true;
				}

				/* Shift+Q: the Selection state. */
				case KeyQ :
					this->setState(EditorState::Selection);
					return true;

				/* Shift+T/R/S: the Transformation state, with that gizmo. */
				case KeyT :
					this->setGizmoMode(GizmoMode::Translate);
					this->setState(EditorState::Transformation);
					return true;

				case KeyR :
					this->setGizmoMode(GizmoMode::Rotate);
					this->setState(EditorState::Transformation);
					return true;

				case KeyS :
					this->setGizmoMode(GizmoMode::Scale);
					this->setState(EditorState::Transformation);
					return true;

				default :
					break;
			}
		}

		return false;
	}
}
