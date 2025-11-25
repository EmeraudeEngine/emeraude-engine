/*
 * src/Scenes/AbstractEntity.debug.cpp
 * This file is part of Emeraude-Engine
 *
 * Copyright (C) 2010-2025 - Sébastien Léon Claude Christian Bémelmans "LondNoir" <londnoir@gmail.com>
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

	constexpr auto TracerTag{"AbstractEntity.debug"};

	constexpr auto AxisDebugName{"+EntityAxis"};
	constexpr auto VelocityDebugName{"+EntityVelocity"};
	constexpr auto BoundingBoxDebugName{"+EntityBoundingBox"};
	constexpr auto BoundingSphereDebugName{"+EntityBoundingSphere"};
	constexpr auto CameraDebugName{"+EntityCamera"};

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

			case VisualDebugType::BoundingBox :
				label = BoundingBoxDebugName;
				meshResource = AbstractEntity::getBoundingBoxVisualDebug(resourceManager);
				break;

			case VisualDebugType::BoundingSphere :
				label = BoundingSphereDebugName;
				meshResource = AbstractEntity::getBoundingSphereVisualDebug(resourceManager);
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
				 component.enablePhysicalProperties(false);
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
				renderableInstance->setTransformationMatrix(Matrix< 4, float >::scaling(m_boundingSphere.radius()));
				break;

			case VisualDebugType::Velocity :
				break;

			case VisualDebugType::BoundingBox :
				renderableInstance->setTransformationMatrix(
					Matrix< 4, float >::translation(m_boundingBox.centroid()) *
					Matrix< 4, float >::scaling(m_boundingBox.width(), m_boundingBox.height(), m_boundingBox.depth())
				);
				break;

			case VisualDebugType::BoundingSphere :
				renderableInstance->setTransformationMatrix(
					Matrix< 4, float >::translation(m_boundingSphere.position()) *
					Matrix< 4, float >::scaling(m_boundingSphere.radius())
				);
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

			case VisualDebugType::BoundingBox :
				this->removeComponent(BoundingBoxDebugName);
				break;

			case VisualDebugType::BoundingSphere :
				this->removeComponent(BoundingSphereDebugName);
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

			case VisualDebugType::BoundingBox :
				return this->containsComponent(BoundingBoxDebugName);

			case VisualDebugType::BoundingSphere :
				return this->containsComponent(BoundingSphereDebugName);

			case VisualDebugType::Camera :
				return this->containsComponent(CameraDebugName);
		}

		return false;
	}

	void
	AbstractEntity::updateVisualDebug () noexcept
	{
		/* Update axis. */
		{
			const auto component = this->getComponent(AxisDebugName);

			if ( component != nullptr )
			{
				const auto renderableInstance = component->getRenderableInstance();

				if ( m_boundingSphere.isValid() )
				{
					renderableInstance->setTransformationMatrix(Matrix< 4, float >::scaling(m_boundingSphere.radius()));
				}
				else
				{
					renderableInstance->setTransformationMatrix(Matrix< 4, float >::scaling(1.0F));
				}
			}
		}

		/* Update bounding box. */
		{
			const auto component = this->getComponent(BoundingBoxDebugName);

			if ( component != nullptr )
			{
				const auto renderableInstance = component->getRenderableInstance();

				renderableInstance->setTransformationMatrix(
					Matrix< 4, float >::translation(m_boundingBox.centroid()) *
					Matrix< 4, float >::scaling(m_boundingBox.width(), m_boundingBox.height(), m_boundingBox.depth())
				);
			}
		}

		/* Update bounding sphere. */
		{
			const auto component = this->getComponent(BoundingSphereDebugName);

			if ( component != nullptr )
			{
				const auto renderableInstance = component->getRenderableInstance();

				renderableInstance->setTransformationMatrix(
					Matrix< 4, float >::translation(m_boundingSphere.position()) *
					Matrix< 4, float >::scaling(m_boundingSphere.radius())
				);
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

	std::shared_ptr< Renderable::MeshResource >
	AbstractEntity::getAxisVisualDebug (Resources::Manager & resources) noexcept
	{
		return resources.container< Renderable::MeshResource >()->getOrCreateResourceAsync(AxisDebugName, [&resources] (Renderable::MeshResource & newMesh) {
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

	std::shared_ptr< Renderable::MeshResource >
	AbstractEntity::getVelocityVisualDebug (Resources::Manager & resources) noexcept
	{
		return resources.container< Renderable::MeshResource >()->getOrCreateResourceAsync(AxisDebugName, [&resources] (Renderable::MeshResource & newMesh) {
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

	std::shared_ptr< Renderable::MeshResource >
	AbstractEntity::getBoundingBoxVisualDebug (Resources::Manager & resources) noexcept
	{
		return resources.container< Renderable::MeshResource >()->getOrCreateResourceAsync(BoundingBoxDebugName, [&resources] (Renderable::MeshResource & newMesh) {
			/* NOTE: Get the geometry. */
			const Geometry::ResourceGenerator generator{resources, Geometry::EnableNormal | Geometry::EnableVertexColor};

			const auto geometryResource = generator.cube(1.0F);

			if ( geometryResource == nullptr )
			{
				return false;
			}

			/* NOTE: Get a translucent material. */
			const auto materialResource = AbstractEntity::getTranslucentVisualDebugMaterial(resources);

			/* NOTE: Assemble the mesh. */
			// TODO: Remove hard-coded wireframe !
			return newMesh.load(geometryResource, materialResource, {PolygonMode::Line, CullingMode::None});
		});
	}

	std::shared_ptr< Renderable::MeshResource >
	AbstractEntity::getBoundingSphereVisualDebug (Resources::Manager & resources) noexcept
	{
		return resources.container< Renderable::MeshResource >()->getOrCreateResourceAsync(BoundingSphereDebugName, [&resources] (Renderable::MeshResource & newMesh) {
			/* NOTE: Get the geometry. */
			const Geometry::ResourceGenerator generator{resources, Geometry::EnableNormal | Geometry::EnableVertexColor};

			const auto geometryResource = generator.geodesicSphere(1.0F);

			if ( geometryResource == nullptr )
			{
				return false;
			}

			/* NOTE: Get a basic material. */
			const auto materialResource = AbstractEntity::getTranslucentVisualDebugMaterial(resources);

			/* NOTE: Assemble the mesh. */
			// TODO: Remove hard-coded wireframe !
			return newMesh.load(geometryResource, materialResource, {PolygonMode::Line, CullingMode::None});
		});
	}

	std::shared_ptr< Renderable::MeshResource >
	AbstractEntity::getCameraVisualDebug (Resources::Manager & resources) noexcept
	{
		return resources.container< Renderable::MeshResource >()->getOrCreateResourceAsync(CameraDebugName, [&resources] (Renderable::MeshResource & newMesh) {
			/* NOTE: Get the geometry. */
			const auto geometryResource = resources.container< Geometry::IndexedVertexResource >()->getResource("Items/Camera", false);

			/* NOTE: Get a basic material. */
			const auto materialResource = resources.container< Material::BasicResource >()->getDefaultResource();

			/* NOTE: Assemble the mesh. */
			return newMesh.load(geometryResource, materialResource);
		});
	}
}
