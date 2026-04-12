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
 * https://github.com/londnoir/emeraude-engine
 *
 * --- THIS IS AUTOMATICALLY GENERATED, DO NOT CHANGE ---
 */

#include "Manager.hpp"

/* Local inclusions. */
#include "Input/Manager.hpp"
#include "Input/Types.hpp"
#include "Resources/Manager.hpp"
#include "Graphics/Renderer.hpp"
#include "Graphics/RenderTarget/Abstract.hpp"
#include "Graphics/ViewMatricesInterface.hpp"
#include "Scenes/Scene.hpp"
#include "Scenes/Node.hpp"
#include "Scenes/StaticEntity.hpp"
#include "Physics/CollisionModelInterface.hpp"
#include "Physics/SphereCollisionModel.hpp"
#include "Physics/AABBCollisionModel.hpp"
#include "Libs/Math/Space3D/Intersections/SegmentCuboid.hpp"
#include "Libs/Math/Space3D/Intersections/SegmentSphere.hpp"
#include "Libs/Math/Space3D/Sphere.hpp"
#include "Vulkan/CommandBuffer.hpp"
#include "Notifier.hpp"
#include "Tracer.hpp"

namespace EmEn::Scenes::Editor
{
	using namespace Libs::Math;
	using namespace Libs::Math::Space3D;
	using namespace Input;
	using namespace Physics;

	Manager::Manager (Input::Manager & inputManager, Resources::Manager & resourceManager, Notifier & notifier) noexcept
		: KeyboardListenerInterface{false, false},
		PointerListenerInterface{false, false, false},
		m_inputManager{inputManager},
		m_resourceManager{resourceManager},
		m_notifier{notifier}
	{

	}

	Manager::~Manager ()
	{
		if ( m_active )
		{
			this->deactivate();
		}
	}

	void
	Manager::activate (Scene & scene, const Graphics::ViewMatricesInterface & viewMatrices, float viewportWidth, float viewportHeight) noexcept
	{
		if ( m_active )
		{
			return;
		}

		m_scene = &scene;
		m_viewMatrices = &viewMatrices;
		m_viewportWidth = viewportWidth;
		m_viewportHeight = viewportHeight;
		m_active = true;

		/* NOTE: Register as input listener. Pointer must be in absolute mode for clicking. */
		this->enableAbsoluteMode();
		m_inputManager.addKeyboardListener(this);
		m_inputManager.addPointerListener(this);

		/* NOTE: Unlock the pointer so the user can click freely. */
		m_inputManager.unlockPointer();

		/* NOTE: Pre-create the gizmo GPU resources. */
		if ( !this->ensureGizmoCreated() )
		{
			Tracer::warning(ClassId, "Failed to pre-create gizmo resources.");
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

		m_inputManager.removeKeyboardListener(this);
		m_inputManager.removePointerListener(this);

		/* NOTE: Destroy gizmo GPU resources. */
		m_translateGizmo.destroy();

		m_scene = nullptr;
		m_viewMatrices = nullptr;
		m_active = false;

		m_notifier.push("Editor mode deactivated.");
	}

	void
	Manager::processLogics () noexcept
	{
		if ( !m_active || m_selectedEntity == nullptr || m_viewMatrices == nullptr )
		{
			return;
		}

		/* NOTE: Update gizmo position to follow the selected entity. */
		const auto worldFrame = m_selectedEntity->getWorldCoordinates();
		m_translateGizmo.setWorldFrame(worldFrame);

		/* NOTE: Update gizmo scale for constant screen size (uses configurable ratio). */
		m_translateGizmo.updateScreenScale(m_viewMatrices->position(), m_viewMatrices->fieldOfView(), m_gizmoScreenRatio);
	}

	void
	Manager::render (const Vulkan::CommandBuffer & commandBuffer) const noexcept
	{
		if ( !m_active || m_selectedEntity == nullptr || m_viewMatrices == nullptr )
		{
			return;
		}

		/* NOTE: Render the active gizmo. */
		if ( m_translateGizmo.isCreated() )
		{
			m_translateGizmo.render(commandBuffer, *m_viewMatrices);
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
			case GizmoMode::Scale :
				/* TODO: Implement rotate and scale gizmos. */
				return false;
		}

		return false;
	}

	Segment< float >
	Manager::screenToWorldRay (float screenX, float screenY) const noexcept
	{
		const float ndcX = (2.0F * screenX / m_viewportWidth) - 1.0F;
		const float ndcY = (2.0F * screenY / m_viewportHeight) - 1.0F;

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

				case CollisionModelType::AABB :
				case CollisionModelType::Capsule :
				{
					const auto worldAABB = model->getAABB(worldFrame);

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

		m_scene->forEachStaticEntities([&testEntity] (const StaticEntity & entity) {
			testEntity(const_cast< StaticEntity & >(entity));
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
	Manager::setSelection (AbstractEntity * entity) noexcept
	{
		if ( m_selectedEntity == entity )
		{
			return;
		}

		this->clearSelection();

		m_selectedEntity = entity;

		if ( m_selectedEntity != nullptr )
		{
			/* NOTE: Position the gizmo at the entity. */
			m_translateGizmo.setWorldFrame(m_selectedEntity->getWorldCoordinates());

			m_notifier.push("Selected entity: '" + m_selectedEntity->name() + "'");
		}
	}

	void
	Manager::clearSelection () noexcept
	{
		m_selectedEntity = nullptr;
	}

	bool
	Manager::onButtonPress (float positionX, float positionY, int32_t buttonNumber, int32_t /*modifiers*/) noexcept
	{
		if ( buttonNumber != Button1Left )
		{
			return false;
		}

		/* NOTE: If a gizmo is active, test it first (priority over scene picking). */
		if ( m_selectedEntity != nullptr && m_translateGizmo.isCreated() )
		{
			const auto ray = this->screenToWorldRay(positionX, positionY);

			if ( ray.isValid() )
			{
				const auto hitAxis = m_translateGizmo.hitTest(ray);

				if ( hitAxis != Gizmo::AxisID::None )
				{
					/* TODO: Start drag operation on hitAxis. */
					TraceInfo{ClassId} << "Gizmo axis hit: " << static_cast< int >(hitAxis);

					return true;
				}
			}
		}

		/* NOTE: Gizmo not hit — fall through to scene picking. */
		if ( auto * entity = this->pickEntity(positionX, positionY); entity != nullptr )
		{
			this->setSelection(entity);
		}
		else
		{
			this->clearSelection();
		}

		return true;
	}

	bool
	Manager::onKeyPress (int32_t key, int32_t /*scancode*/, int32_t modifiers, bool repeat) noexcept
	{
		if ( repeat )
		{
			return false;
		}

		/* NOTE: Escape key deselects the current entity. */
		if ( key == KeyEscape )
		{
			if ( m_selectedEntity != nullptr )
			{
				this->clearSelection();

				return true;
			}
		}

		/* NOTE: Shift+G/R/S for gizmo mode switching. */
		if ( isKeyboardModifierPressed(ModKeyShift, modifiers) )
		{
			switch ( key )
			{
				case KeyG :
					this->setGizmoMode(GizmoMode::Translate);
					return true;

				case KeyR :
					this->setGizmoMode(GizmoMode::Rotate);
					return true;

				case KeyS :
					this->setGizmoMode(GizmoMode::Scale);
					return true;

				default :
					break;
			}
		}

		return false;
	}
}
