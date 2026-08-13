/*
 * src/Scenes/AbstractEntity.debug.cpp
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

#include "AbstractEntity.hpp"

/* Local inclusions. */
#include "Graphics/Geometry/ResourceGenerator.hpp"
#include "Graphics/Material/StandardResource.hpp"
#include "Graphics/RenderableInstance/Abstract.hpp"
#include "Physics/CapsuleCollisionModel.hpp"
#include "Physics/CollisionModelInterface.hpp"
#include "Component/Visual.hpp"
#include "Resources/Manager.hpp"
#include "Tracer.hpp"

namespace EmEn::Scenes
{
	namespace Component
	{
		class Visual;
	}
	using namespace Base;
	using namespace Base::Math;
	using namespace Graphics;
	using namespace Physics;

	constexpr auto TracerTag{"AbstractEntity.debug"};

	constexpr auto AxisDebugName{"+EntityAxis"};
	constexpr auto VelocityDebugName{"+EntityVelocity"};
	constexpr auto BoundingShapeDebugName{"+EntityCollisionShape"};
	constexpr auto RenderBoundingBoxDebugName{"+EntityRenderBoundingBox"};
	constexpr auto CameraDebugName{"+EntityCamera"};

	/* NOTE: Visual debug helpers are UNLIT, so their colour IS their emitted radiance. This
	 * luminance anchors the [0,1] vertex colours in the photometric pipeline; without it, a
	 * helper would write its raw [0,1] colour and read black under the camera exposure. */
	constexpr auto VisualDebugLuminance{2000.0F};

	/* The axis gizmo is scaled by the collision model radius so it matches the object it describes,
	 * then overshoots it slightly: arrows buried inside the mesh are unreadable.
	 * ⚠️ This is only safe because a debug helper no longer contributes to the PHYSICAL extent
	 * (Component::Abstract::setContributesToEntityExtents). While it did, the gizmo fed its own
	 * mesh extent back into the collider, the collider grew, and the gizmo was drawn bigger still —
	 * enlarging it here would have amplified that loop instead of just showing more. */
	constexpr auto AxisDebugRadiusOvershoot{1.5F};

	void
	AbstractEntity::enableVisualDebug (Resources::Manager & resourceManager, VisualDebugType type) noexcept
	{
		if ( this->isVisualDebugEnabled(type) )
		{
			return;
		}

		const char * label = nullptr;
		std::shared_ptr< Renderable::MeshResource > meshResource;

		switch ( type )
		{
			case VisualDebugType::Axis :
				label = AxisDebugName;
				meshResource = AbstractEntity::getAxisVisualDebug(resourceManager);
				break;

			case VisualDebugType::Velocity :
				label = VelocityDebugName;
				meshResource = AbstractEntity::getVelocityVisualDebug(resourceManager);
				break;

			case VisualDebugType::CollisionShape :
				label = BoundingShapeDebugName;
				if ( m_collisionModel != nullptr )
				{
					switch ( m_collisionModel->modelType() )
					{
						case CollisionModelType::Point :
							/* Point has no visual representation, use axis instead. */
							meshResource = AbstractEntity::getAxisVisualDebug(resourceManager);
							break;

						case CollisionModelType::Sphere :
							meshResource = AbstractEntity::getBoundingSphereVisualDebug(resourceManager);
							break;

						case CollisionModelType::AABB :
						case CollisionModelType::Capsule : /* TODO: Implement capsule visual debug mesh. */
							meshResource = AbstractEntity::getBoundingBoxVisualDebug(resourceManager);
							break;
					}
				}
				break;

			case VisualDebugType::RenderBoundingBox :
				label = RenderBoundingBoxDebugName;
				meshResource = AbstractEntity::getRenderBoundingBoxVisualDebug(resourceManager);
				break;

			case VisualDebugType::Camera :
				label = CameraDebugName;
				meshResource = AbstractEntity::getCameraVisualDebug(resourceManager);
				break;
		}

		if ( meshResource == nullptr )
		{
			TraceError{TracerTag} << "Unable to get the visual debug mesh !";

			return;
		}

		/* NOTE: Create an instance of this visual debug mesh.
		 * ⚠️ setContributesToEntityExtents(false) MUST happen in setup(), which runs BEFORE the
		 * component is linked — and linking calls updateEntityProperties() straight away. A helper
		 * left contributing merges its own extent into the COLLISION model of the entity it is only
		 * supposed to describe: the axis gizmo is generated at extent 1.0, so it nearly doubled the
		 * collider of a 1 m cube, and the bounding-shape helper then drew that inflated collider —
		 * the box "growing on its own" when switching objects in the geometry-debug demo. It gates
		 * the RENDER extent too: a gizmo is debug logic and must not move a measurement of the
		 * content — a helper culled a touch early costs nothing, a falsified extent costs a session. */
		const auto meshInstance = this->componentBuilder< Component::Visual >(label)
			 .setup([] (auto & component) {
				 component.setContributesToEntityExtents(false);
				 component.getRenderableInstance()->enableLighting();
			 }).build(meshResource);

		if ( meshInstance == nullptr )
		{
			TraceError{TracerTag} << "Unable to instantiate a visual debug mesh instance !";

			return;
		}

		/* NOTE : Configure the renderable instance advanced options. */
		const auto renderableInstance = meshInstance->getRenderableInstance();

		switch ( type )
		{
			case VisualDebugType::Axis :
				if ( m_collisionModel != nullptr )
				{
					renderableInstance->setTransformationMatrix(Matrix< 4, float >::scaling(m_collisionModel->getRadius() * AxisDebugRadiusOvershoot));
				}
				else
				{
					renderableInstance->setTransformationMatrix(Matrix< 4, float >::identity());
				}
				break;

			case VisualDebugType::Velocity :
				break;

			case VisualDebugType::CollisionShape :
				if ( m_collisionModel != nullptr )
				{
					switch ( m_collisionModel->modelType() )
					{
						case CollisionModelType::Point :
							/* Point has no shape, use identity. */
							renderableInstance->setTransformationMatrix(Matrix< 4, float >::identity());
							break;

						case CollisionModelType::Sphere :
							/* Sphere is centered at local origin.
							 * ⚠️ EXACT radius, no overshoot: this helper's job is to report the
							 * collider truthfully. Only the AXIS gizmo is allowed to overshoot. */
							renderableInstance->setTransformationMatrix(Matrix< 4, float >::scaling(m_collisionModel->getRadius()));
							break;

						case CollisionModelType::AABB :
						{
							const auto worldFrame = this->getWorldCoordinates();
							const auto worldAABB = m_collisionModel->getAABB(worldFrame);

							if ( worldAABB.isValid() )
							{
								const auto inverseEntity = worldFrame.getInvertedModelMatrix();

								renderableInstance->setTransformationMatrix(
									inverseEntity *
									Matrix< 4, float >::translation(worldAABB.centroid()) *
									Matrix< 4, float >::scaling(worldAABB.width(), worldAABB.height(), worldAABB.depth())
								);
							}
						}
							break;

						case CollisionModelType::Capsule :
						{
							const auto * capsuleModel = static_cast< const CapsuleCollisionModel * >(m_collisionModel.get());
							const auto & capsule = capsuleModel->localCapsule();
							const auto center = (capsule.startPoint() + capsule.endPoint()) * 0.5F;
							const auto height = (capsule.endPoint() - capsule.startPoint()).length() + capsule.radius() * 2.0F;
							const auto diameter = capsule.radius() * 2.0F;

							renderableInstance->setTransformationMatrix(
								Matrix< 4, float >::translation(center) *
								Matrix< 4, float >::scaling(diameter, height, diameter)
							);
						}
							break;
					}
				}
				break;

			case VisualDebugType::RenderBoundingBox :
				AbstractEntity::applyRenderBoundingBoxTransform(*renderableInstance, m_renderBoundingBox);
				break;

			case VisualDebugType::Camera :
				break;
		}

		renderableInstance->disableDepthTest(false);
	}

	void
	AbstractEntity::applyRenderBoundingBoxTransform (Graphics::RenderableInstance::Abstract & renderableInstance, const Space3D::AACuboid< float > & renderBoundingBox) noexcept
	{
		/* ⚠️ Unlike the collision helper above, NOTHING is inverted here: m_renderBoundingBox is
		 * already expressed in the entity's LOCAL space, whereas the collision model answers in
		 * WORLD space and has to be mapped back. Wrapping this one in the inverse entity matrix
		 * would apply the entity transform twice. */
		if ( !renderBoundingBox.isValid() )
		{
			return;
		}

		renderableInstance.setTransformationMatrix(
			Matrix< 4, float >::translation(renderBoundingBox.centroid()) *
			Matrix< 4, float >::scaling(renderBoundingBox.width(), renderBoundingBox.height(), renderBoundingBox.depth())
		);
	}

	void
	AbstractEntity::disableVisualDebug (VisualDebugType type) noexcept
	{
		switch ( type )
		{
			case VisualDebugType::Axis :
				this->removeComponent(AxisDebugName);
				break;

			case VisualDebugType::Velocity :
				this->removeComponent(VelocityDebugName);
				break;

			case VisualDebugType::CollisionShape :
				this->removeComponent(BoundingShapeDebugName);
				break;

			case VisualDebugType::RenderBoundingBox :
				this->removeComponent(RenderBoundingBoxDebugName);
				break;

			case VisualDebugType::Camera :
				this->removeComponent(CameraDebugName);
				break;
		}
	}

	bool
	AbstractEntity::toggleVisualDebug (Resources::Manager & resourceManager, VisualDebugType type) noexcept
	{
		if ( this->isVisualDebugEnabled(type) )
		{
			this->disableVisualDebug(type);

			return false;
		}

		this->enableVisualDebug(resourceManager, type);

		return true;
	}

	bool
	AbstractEntity::isVisualDebugEnabled (VisualDebugType type) const noexcept
	{
		switch ( type )
		{
			case VisualDebugType::Axis :
				return this->containsComponent(AxisDebugName);

			case VisualDebugType::Velocity :
				return this->containsComponent(VelocityDebugName);

			case VisualDebugType::CollisionShape :
				return this->containsComponent(BoundingShapeDebugName);

			case VisualDebugType::RenderBoundingBox :
				return this->containsComponent(RenderBoundingBoxDebugName);

			case VisualDebugType::Camera :
				return this->containsComponent(CameraDebugName);
		}

		return false;
	}

	void
	AbstractEntity::updateVisualDebug () noexcept
	{
		/* Update axis. */
		if ( const auto component = this->getComponent(AxisDebugName); component != nullptr )
		{
			const auto renderableInstance = component->getRenderableInstance();

			if ( m_collisionModel != nullptr )
			{
				renderableInstance->setTransformationMatrix(Matrix< 4, float >::scaling(m_collisionModel->getRadius() * AxisDebugRadiusOvershoot));
			}
			else
			{
				renderableInstance->setTransformationMatrix(Matrix< 4, float >::identity());
			}
		}

		/* Update the RENDER extent helper. It does not depend on the collision model, so it is
		 * refreshed before the early return below. */
		if ( const auto component = this->getComponent(RenderBoundingBoxDebugName); component != nullptr )
		{
			AbstractEntity::applyRenderBoundingBoxTransform(*component->getRenderableInstance(), m_renderBoundingBox);
		}

		/* Update bounding shape. */
		if ( m_collisionModel == nullptr  )
		{
			return;
		}

		if ( const auto component = this->getComponent(BoundingShapeDebugName); component != nullptr )
		{
			const auto renderableInstance = component->getRenderableInstance();

			switch ( m_collisionModel->modelType() )
			{
				case CollisionModelType::Point :
					/* Point has no shape, use identity. */
					renderableInstance->setTransformationMatrix(Matrix< 4, float >::identity());
					break;

				case CollisionModelType::Sphere :
					/* Sphere is centered at local origin.
					 * ⚠️ EXACT radius, no overshoot: this helper's job is to report the collider
					 * truthfully. Only the AXIS gizmo is allowed to overshoot. */
					renderableInstance->setTransformationMatrix(Matrix< 4, float >::scaling(m_collisionModel->getRadius()));
					break;

				case CollisionModelType::AABB :
				{
					const auto worldFrame = this->getWorldCoordinates();
					const auto worldAABB = m_collisionModel->getAABB(worldFrame);

					if ( worldAABB.isValid() )
					{
						const auto inverseEntity = worldFrame.getInvertedModelMatrix();

						renderableInstance->setTransformationMatrix(
							inverseEntity *
							Matrix< 4, float >::translation(worldAABB.centroid()) *
							Matrix< 4, float >::scaling(worldAABB.width(), worldAABB.height(), worldAABB.depth())
						);
					}
				}
					break;

				case CollisionModelType::Capsule :
				{
					const auto * capsuleModel = static_cast< const CapsuleCollisionModel * >(m_collisionModel.get());
					const auto & capsule = capsuleModel->localCapsule();
					const auto center = (capsule.startPoint() + capsule.endPoint()) * 0.5F;
					const auto height = (capsule.endPoint() - capsule.startPoint()).length() + capsule.radius() * 2.0F;
					const auto diameter = capsule.radius() * 2.0F;

					renderableInstance->setTransformationMatrix(
						Matrix< 4, float >::translation(center) *
						Matrix< 4, float >::scaling(diameter, height, diameter)
					);
				}
					break;
			}
		}
	}

	std::shared_ptr< Material::StandardResource >
	AbstractEntity::getPlainVisualDebugMaterial (Resources::Manager & resources) noexcept
	{
		return resources.container< Material::StandardResource >()
			->getOrCreateResource("+PlainVisualDebug", [] (auto & materialResource) {
				materialResource.enableVertexColor();
				/* NOTE: A debug helper must stay readable whatever the scene lighting is,
				 * so it is unlit and carries its own radiance. */
				materialResource.enableUnlit();
				materialResource.setAutoIlluminationComponent(1.0F);
				materialResource.setEmissiveStrength(VisualDebugLuminance);

				return materialResource.setManualLoadSuccess(true);
			});
	}

	std::shared_ptr< Material::StandardResource >
	AbstractEntity::getTranslucentVisualDebugMaterial (Resources::Manager & resources) noexcept
	{
		return resources.container< Material::StandardResource >()
			->getOrCreateResource("+TranslucentVisualDebug", [] (auto & materialResource) {
				materialResource.enableVertexColor();
				/* NOTE: A debug helper must stay readable whatever the scene lighting is,
				 * so it is unlit and carries its own radiance. */
				materialResource.enableUnlit();
				materialResource.setAutoIlluminationComponent(1.0F);
				materialResource.setEmissiveStrength(VisualDebugLuminance);
				materialResource.setOpacityComponent(0.333F);

				return materialResource.setManualLoadSuccess(true);
			});
	}

	std::shared_ptr< Renderable::MeshResource >
	AbstractEntity::getAxisVisualDebug (Resources::Manager & resources) noexcept
	{
		return resources.container< Renderable::MeshResource >()
			->getOrCreateResource(AxisDebugName, [&resources] (auto & meshResource) {
				return meshResource.load(
					Geometry::ResourceGenerator{resources, Geometry::EnableNormal | Geometry::EnableVertexColor}.axis(1.0F),
					AbstractEntity::getPlainVisualDebugMaterial(resources)
				);
			});
	}

	std::shared_ptr< Renderable::MeshResource >
	AbstractEntity::getVelocityVisualDebug (Resources::Manager & resources) noexcept
	{
		return resources.container< Renderable::MeshResource >()
			->getOrCreateResource(VelocityDebugName, [&resources] (auto & meshResource) {
				return meshResource.load(
					Geometry::ResourceGenerator{resources, Geometry::EnableNormal | Geometry::EnableVertexColor}.arrow(1.0F, PointTo::PositiveZ),
					AbstractEntity::getPlainVisualDebugMaterial(resources)
				);
			});
	}

	std::shared_ptr< Renderable::MeshResource >
	AbstractEntity::getBoundingSphereVisualDebug (Resources::Manager & resources) noexcept
	{
		return resources.container< Renderable::MeshResource >()
			->getOrCreateResource("+BoundingSphere", [&resources] (auto & meshResource) {
				return meshResource.load(
					Geometry::ResourceGenerator{resources, Geometry::EnableNormal | Geometry::EnableVertexColor}.geodesicSphere(1.0F),
					AbstractEntity::getTranslucentVisualDebugMaterial(resources),
					{PolygonMode::Line, CullingMode::None}
				);
			});
	}

	std::shared_ptr< Renderable::MeshResource >
	AbstractEntity::getBoundingBoxVisualDebug (Resources::Manager & resources) noexcept
	{
		return resources.container< Renderable::MeshResource >()
			->getOrCreateResource("+BoundingBox", [&resources] (auto & meshResource) {
				return meshResource.load(
					Geometry::ResourceGenerator{resources, Geometry::EnableNormal | Geometry::EnableVertexColor}.cube(1.0F),
					AbstractEntity::getTranslucentVisualDebugMaterial(resources),
					{PolygonMode::Line, CullingMode::None}
				);
			});
	}

	std::shared_ptr< Renderable::MeshResource >
	AbstractEntity::getRenderBoundingBoxVisualDebug (Resources::Manager & resources) noexcept
	{
		return resources.container< Renderable::MeshResource >()
			->getOrCreateResource("+RenderBoundingBox", [&resources] (auto & meshResource) {
				/* A SECOND cube resource on purpose: this helper is meant to be shown next to the
				 * collision one, so it carries a flat cyan material instead of the vertex colours,
				 * and the two extents can be told apart in a single capture. */
				const auto material = resources.container< Material::StandardResource >()
					->getOrCreateResource("+RenderExtentVisualDebug", [] (auto & materialResource) {
						if ( !materialResource.setAlbedoComponent(PixelFactory::Cyan) )
						{
							return false;
						}

						/* NOTE: Same photometric contract as the other debug materials — unlit
						 * helpers must carry their own radiance or the camera exposure reads them
						 * black. setAutoIlluminationComponent() FIRST: it creates the component
						 * that setEmissiveStrength() then scales. */
						materialResource.enableUnlit();
						materialResource.setAutoIlluminationComponent(1.0F);
						materialResource.setEmissiveStrength(VisualDebugLuminance);
						materialResource.setOpacityComponent(0.333F);

						return materialResource.setManualLoadSuccess(true);
					});

				return meshResource.load(
					Geometry::ResourceGenerator{resources, Geometry::EnableNormal}.cube(1.0F, "+RenderBoundingBoxGeometry"),
					material,
					{PolygonMode::Line, CullingMode::None}
				);
			});
	}

	std::shared_ptr< Renderable::MeshResource >
	AbstractEntity::getCameraVisualDebug (Resources::Manager & resources) noexcept
	{
		return resources.container< Renderable::MeshResource >()
			->getOrCreateResource(CameraDebugName, [&resources] (auto & meshResource) {
				return meshResource.load(
					resources.container< Geometry::IndexedVertexResource >()->getResource("Items/Camera"),
					resources.container< Material::StandardResource >()->getDefaultResource()
				);
			});
	}
}
