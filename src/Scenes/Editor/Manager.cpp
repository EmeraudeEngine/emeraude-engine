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

/* STL inclusions. */
#include <cmath>

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

		/* NOTE: Destroy all gizmo GPU resources. */
		m_translateGizmo.destroy();
		m_rotateGizmo.destroy();

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

		/* NOTE: Update gizmo position (and rotation in Local mode) to follow the selected entity. */
		auto worldFrame = m_selectedEntity->getWorldCoordinates();

		if ( m_transformSpace == TransformSpace::World )
		{
			/* NOTE: In World mode, keep position but reset rotation to identity. */
			worldFrame.resetRotation();
		}

		m_translateGizmo.setWorldFrame(worldFrame);
		m_rotateGizmo.setWorldFrame(worldFrame);

		/* NOTE: Update gizmo scale for constant screen size (uses configurable ratio). */
		m_translateGizmo.updateScreenScale(m_viewMatrices->position(), m_viewMatrices->fieldOfView(), m_gizmoScreenRatio);
		m_rotateGizmo.updateScreenScale(m_viewMatrices->position(), m_viewMatrices->fieldOfView(), m_gizmoScreenRatio);
	}

	void
	Manager::render (const Vulkan::CommandBuffer & commandBuffer) const noexcept
	{
		if ( !m_active || m_selectedEntity == nullptr || m_viewMatrices == nullptr )
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
				/* TODO */
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

		const float denom = aa * bb - ab * ab;

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
		/* NOTE: Handle drag if active. */
		if ( m_dragActive && m_selectedEntity != nullptr )
		{
			if ( m_gizmoMode == GizmoMode::Translate )
			{
				const float currentT = this->projectMouseOnAxis(positionX, positionY, m_dragInitialEntityPos, m_dragAxisDirection);
				float delta = (m_dragInitialT - currentT) * m_moveRatio;

				if ( m_moveStep > 0.0F )
				{
					delta = std::round(delta / m_moveStep) * m_moveStep;
				}

				const auto newPosition = m_dragInitialEntityPos + m_dragAxisDirection * delta;

				m_selectedEntity->setPosition(newPosition, Libs::Math::TransformSpace::World);
			}
			else if ( m_gizmoMode == GizmoMode::Rotate )
			{
				const float currentAngle = this->projectMouseAngleOnPlane(positionX, positionY, m_dragInitialEntityPos, m_dragAxisDirection);
				float deltaAngle = (currentAngle - m_dragInitialAngle);

				if ( m_rotateStep > 0.0F )
				{
					deltaAngle = std::round(deltaAngle / m_rotateStep) * m_rotateStep;
				}

				/* NOTE: Apply rotation around the axis, centered on the entity (not orbiting).
				 * World mode: axis is in world space, rotate orientation only (save/restore position).
				 * Local mode: axis is already in entity's local directions, use Local transform. */
				if ( m_transformSpace == TransformSpace::Local )
				{
					/* NOTE: Local mode: pass unit axis + TransformSpace::Local.
					 * The CartesianFrame will convert from local to world internally. */
					Vector< 3, float > localAxis{0.0F, 0.0F, 0.0F};

					switch ( m_dragAxis )
					{
						case Gizmo::AxisID::X : localAxis = {1.0F, 0.0F, 0.0F}; break;
						case Gizmo::AxisID::Y : localAxis = {0.0F, 1.0F, 0.0F}; break;
						case Gizmo::AxisID::Z : localAxis = {0.0F, 0.0F, 1.0F}; break;
						default : break;
					}

					m_selectedEntity->rotate(deltaAngle, localAxis, Libs::Math::TransformSpace::Local);
				}
				else
				{
					/* NOTE: World mode: save/restore position to rotate in-place. */
					const auto savedPos = m_selectedEntity->getWorldCoordinates().position();

					m_selectedEntity->rotate(deltaAngle, m_dragAxisDirection, Libs::Math::TransformSpace::World);
					m_selectedEntity->setPosition(savedPos, Libs::Math::TransformSpace::World);
				}

				/* NOTE: Update initial angle for next delta. */
				m_dragInitialAngle = currentAngle;
			}

			return true;
		}

		/* NOTE: Update gizmo hover highlight on the active gizmo. */
		if ( m_selectedEntity != nullptr )
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
						break;
				}
			}
		}

		return false;
	}

	bool
	Manager::onButtonPress (float positionX, float positionY, int32_t buttonNumber, int32_t /*modifiers*/) noexcept
	{
		if ( buttonNumber != Button1Left )
		{
			return false;
		}

		/* NOTE: If a gizmo is active, test it first (priority over scene picking). */
		Gizmo::Abstract * activeGizmo = nullptr;

		switch ( m_gizmoMode )
		{
			case GizmoMode::Translate :
				if ( m_translateGizmo.isCreated() ) { activeGizmo = &m_translateGizmo; }
				break;

			case GizmoMode::Rotate :
				if ( m_rotateGizmo.isCreated() ) { activeGizmo = &m_rotateGizmo; }
				break;

			case GizmoMode::Scale :
				break;
		}

		if ( m_selectedEntity != nullptr && activeGizmo != nullptr )
		{
			const auto ray = this->screenToWorldRay(positionX, positionY);

			if ( ray.isValid() )
			{
				const auto hitAxis = activeGizmo->hitTest(ray);

				if ( hitAxis != Gizmo::AxisID::None )
				{
					/* NOTE: Start drag operation. */
					m_dragActive = true;
					m_dragAxis = hitAxis;
					m_dragInitialEntityPos = m_selectedEntity->getWorldCoordinates().position();

					/* NOTE: Determine axis direction based on transform space. */
					const auto & entityFrame = m_selectedEntity->getWorldCoordinates();

					switch ( hitAxis )
					{
						case Gizmo::AxisID::X :
							m_dragAxisDirection = (m_transformSpace == TransformSpace::Local)
								? entityFrame.rightVector()
								: Vector< 3, float >{1.0F, 0.0F, 0.0F};
							break;

						case Gizmo::AxisID::Y :
							m_dragAxisDirection = (m_transformSpace == TransformSpace::Local)
								? entityFrame.downwardVector()
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
						m_dragInitialT = this->projectMouseOnAxis(positionX, positionY, m_dragInitialEntityPos, m_dragAxisDirection);
					}
					else if ( m_gizmoMode == GizmoMode::Rotate )
					{
						m_dragInitialAngle = this->projectMouseAngleOnPlane(positionX, positionY, m_dragInitialEntityPos, m_dragAxisDirection);
					}

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
	Manager::onButtonRelease (float /*positionX*/, float /*positionY*/, int32_t buttonNumber, int32_t /*modifiers*/) noexcept
	{
		if ( buttonNumber != Button1Left )
		{
			return false;
		}

		if ( m_dragActive )
		{
			m_dragActive = false;
			m_dragAxis = Gizmo::AxisID::None;

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

		/* NOTE: Escape key deselects the current entity. */
		if ( key == KeyEscape )
		{
			if ( m_selectedEntity != nullptr )
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
							m_transformSpace = TransformSpace::Local;
							m_notifier.push("Transform space: Local");
							break;

						case TransformSpace::Parent :
							m_transformSpace = TransformSpace::Local;
							m_notifier.push("Transform space: Local");
							break;
					}

					return true;
				}

				/* Shift+R/S: gizmo mode switching. */
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
