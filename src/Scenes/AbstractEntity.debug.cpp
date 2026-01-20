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
 * https://github.com/londnoir/emeraude-engine
 *
 * --- THIS IS AUTOMATICALLY GENERATED, DO NOT CHANGE ---
 */

#include "AbstractEntity.hpp"

/* Local inclusions. */
#include "Graphics/Geometry/ResourceGenerator.hpp"
#include "Graphics/RenderableInstance/Abstract.hpp"
#include "Physics/CollisionModelInterface.hpp"
#include "Physics/SphereCollisionModel.hpp"
#include "Physics/AABBCollisionModel.hpp"
#include "Physics/CapsuleCollisionModel.hpp"
#include "Component/Visual.hpp"
#include "Tracer.hpp"

namespace EmEn::Scenes
{
	namespace Component
	{
		class Visual;
	}
	using namespace Libs;
	using namespace Libs::Math;
	using namespace Graphics;
	using namespace Physics;

	constexpr auto TracerTag{"AbstractEntity.debug"};

	constexpr auto AxisDebugName{"+EntityAxis"};
	constexpr auto VelocityDebugName{"+EntityVelocity"};
	constexpr auto BoundingShapeDebugName{"+EntityBoundingShape"};
	constexpr auto CameraDebugName{"+EntityCamera"};

	void
	AbstractEntity::enableVisualDebug (Resources::Manager & resourceManager, VisualDebugType type) noexcept
	{
		if ( this->isVisualDebugEnabled(type) )
		{
			return;
		}

		const char * label = nullptr;
		std::shared_ptr< Renderable::SimpleMeshResource > meshResource;

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

			case VisualDebugType::BoundingShape :
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

		/* NOTE: Create an instance of this visual debug mesh. */
		const auto meshInstance = this->componentBuilder< Component::Visual >(label)
			 .setup([] (auto & component) {
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
					renderableInstance->setTransformationMatrix(Matrix< 4, float >::scaling(m_collisionModel->getRadius()));
				}
				else
				{
					renderableInstance->setTransformationMatrix(Matrix< 4, float >::identity());
				}
				break;

			case VisualDebugType::Velocity :
				break;

			case VisualDebugType::BoundingShape :
				if ( m_collisionModel != nullptr )
				{
					switch ( m_collisionModel->modelType() )
					{
						case CollisionModelType::Point :
							/* Point has no shape, use identity. */
							renderableInstance->setTransformationMatrix(Matrix< 4, float >::identity());
							break;

						case CollisionModelType::Sphere :
							/* Sphere is centered at local origin. */
							renderableInstance->setTransformationMatrix(Matrix< 4, float >::scaling(m_collisionModel->getRadius()));
							break;

						case CollisionModelType::AABB :
						{
							const auto * aabbModel = static_cast< const AABBCollisionModel * >(m_collisionModel.get());
							const auto & aabb = aabbModel->localAABB();

							if ( aabb.isValid() )
							{
								renderableInstance->setTransformationMatrix(
									Matrix< 4, float >::translation(aabb.centroid()) *
									Matrix< 4, float >::scaling(aabb.width(), aabb.height(), aabb.depth())
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

			case VisualDebugType::Camera :
				break;
		}

		renderableInstance->disableDepthTest(false);
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

			case VisualDebugType::BoundingShape :
				this->removeComponent(BoundingShapeDebugName);
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

			case VisualDebugType::BoundingShape :
				return this->containsComponent(BoundingShapeDebugName);

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
				renderableInstance->setTransformationMatrix(Matrix< 4, float >::scaling(m_collisionModel->getRadius()));
			}
			else
			{
				renderableInstance->setTransformationMatrix(Matrix< 4, float >::identity());
			}
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
					/* Sphere is centered at local origin. */
					renderableInstance->setTransformationMatrix(Matrix< 4, float >::scaling(m_collisionModel->getRadius()));
					break;

				case CollisionModelType::AABB :
				{
					const auto * aabbModel = static_cast< const AABBCollisionModel * >(m_collisionModel.get());
					const auto & aabb = aabbModel->localAABB();

					if ( aabb.isValid() )
					{
						renderableInstance->setTransformationMatrix(
							Matrix< 4, float >::translation(aabb.centroid()) *
							Matrix< 4, float >::scaling(aabb.width(), aabb.height(), aabb.depth())
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

	std::shared_ptr< Material::BasicResource >
	AbstractEntity::getPlainVisualDebugMaterial (Resources::Manager & resources) noexcept
	{
		return resources.container< Material::BasicResource >()->getOrCreateResource("+PlainVisualDebug", [] (Material::BasicResource & newMaterial) {
			newMaterial.enableVertexColor();

			return newMaterial.setManualLoadSuccess(true);
		});
	}

	std::shared_ptr< Material::BasicResource >
	AbstractEntity::getTranslucentVisualDebugMaterial (Resources::Manager & resources) noexcept
	{
		return resources.container< Material::BasicResource >()->getOrCreateResource("+TranslucentVisualDebug", [] (Material::BasicResource & newMaterial) {
			newMaterial.enableVertexColor();
			newMaterial.setOpacity(0.333F);

			return newMaterial.setManualLoadSuccess(true);
		});
	}

	std::shared_ptr< Renderable::SimpleMeshResource >
	AbstractEntity::getAxisVisualDebug (Resources::Manager & resources) noexcept
	{
		return resources.container< Renderable::SimpleMeshResource >()->getOrCreateResourceAsync(AxisDebugName, [&resources] (auto & newMesh) {
			/* NOTE: Get the geometry. */
			const Geometry::ResourceGenerator generator{resources, Geometry::EnableNormal | Geometry::EnableVertexColor};

			const auto geometryResource = generator.axis(1.0F);

			if ( geometryResource == nullptr )
			{
				return false;
			}

			/* NOTE: Get a plain material. */
			const auto materialResource = AbstractEntity::getPlainVisualDebugMaterial(resources);

			/* NOTE: Assemble the mesh. */
			return newMesh.load(geometryResource, materialResource);
		});
	}

	std::shared_ptr< Renderable::SimpleMeshResource >
	AbstractEntity::getVelocityVisualDebug (Resources::Manager & resources) noexcept
	{
		return resources.container< Renderable::SimpleMeshResource >()->getOrCreateResourceAsync(VelocityDebugName, [&resources] (auto & newMesh) {
			/* NOTE: Get the geometry. */
			const Geometry::ResourceGenerator generator{resources, Geometry::EnableNormal | Geometry::EnableVertexColor};

			const auto geometryResource = generator.arrow(1.0F, PointTo::PositiveZ);

			if ( geometryResource == nullptr )
			{
				return false;
			}

			/* NOTE: Get a basic material. */
			const auto materialResource = AbstractEntity::getPlainVisualDebugMaterial(resources);

			/* NOTE: Assemble the mesh. */
			return newMesh.load(geometryResource, materialResource);
		});
	}

	std::shared_ptr< Renderable::SimpleMeshResource >
	AbstractEntity::getBoundingSphereVisualDebug (Resources::Manager & resources) noexcept
	{
		return resources.container< Renderable::SimpleMeshResource >()->getOrCreateResourceAsync("+BoundingSphere", [&resources] (auto & newMesh) {
			const Geometry::ResourceGenerator generator{resources, Geometry::EnableNormal | Geometry::EnableVertexColor};

			const auto geometryResource = generator.geodesicSphere(1.0F);

			if ( geometryResource == nullptr )
			{
				return false;
			}

			const auto materialResource = AbstractEntity::getTranslucentVisualDebugMaterial(resources);

			return newMesh.load(geometryResource, materialResource, {PolygonMode::Line, CullingMode::None});
		});
	}

	std::shared_ptr< Renderable::SimpleMeshResource >
	AbstractEntity::getBoundingBoxVisualDebug (Resources::Manager & resources) noexcept
	{
		return resources.container< Renderable::SimpleMeshResource >()->getOrCreateResourceAsync("+BoundingBox", [&resources] (auto & newMesh) {
			const Geometry::ResourceGenerator generator{resources, Geometry::EnableNormal | Geometry::EnableVertexColor};

			const auto geometryResource = generator.cube(1.0F);

			if ( geometryResource == nullptr )
			{
				return false;
			}

			const auto materialResource = AbstractEntity::getTranslucentVisualDebugMaterial(resources);

			return newMesh.load(geometryResource, materialResource, {PolygonMode::Line, CullingMode::None});
		});
	}

	std::shared_ptr< Renderable::SimpleMeshResource >
	AbstractEntity::getCameraVisualDebug (Resources::Manager & resources) noexcept
	{
		return resources.container< Renderable::SimpleMeshResource >()->getOrCreateResourceAsync(CameraDebugName, [&resources] (auto & newMesh) {
			/* NOTE: Get the geometry. */
			const auto geometryResource = resources.container< Geometry::IndexedVertexResource >()->getResource("Items/Camera", false);

			/* NOTE: Get a basic material. */
			const auto materialResource = resources.container< Material::BasicResource >()->getDefaultResource();

			/* NOTE: Assemble the mesh. */
			return newMesh.load(geometryResource, materialResource);
		});
	}
}
