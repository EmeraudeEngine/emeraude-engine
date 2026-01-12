/*
 * src/Scenes/Scene.debug.cpp
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

#include "Scene.hpp"

/* Local inclusions. */
#include "Libs/PixelFactory/Color.hpp"
#include "Graphics/Geometry/ResourceGenerator.hpp"
#include "Graphics/Renderable/MeshResource.hpp"
#include "Graphics/Material/BasicResource.hpp"
#include "Resources/Manager.hpp"

namespace EmEn::Scenes
{
	using namespace Libs;
	using namespace Libs::Math;
	using namespace Libs::PixelFactory;
	using namespace Graphics;

	bool
	Scene::enableCompassDisplay (Resources::Manager & resourceManager) noexcept
	{
		if ( this->compassDisplayEnabled() )
		{
			return true;
		}

		const auto materialResource = resourceManager.container< Material::BasicResource >()->getOrCreateResource("+DebugSceneMaterial", [] (Material::BasicResource & newMaterial) {
			newMaterial.enableVertexColor();

			return newMaterial.setManualLoadSuccess(true);
		});

		constexpr std::array< std::pair< Vector< 3, float >, Color< float > >, 6 > landmarks{{
			/* NOTE: X+ sphere color in bright red. */
			{{100.0F, 0.0F, 0.0F}, {1.0F, 0.0F, 0.0F, 1.0F}},
			/* NOTE: Y+ sphere color in bright green. */
			{{0.0F, 100.0F, 0.0F}, {0.0F, 1.0F, 0.0F, 1.0F}},
			/* NOTE: Z+ sphere color in bright blue. */
			{{0.0F, 0.0F, 100.0F}, {0.0F, 0.0F, 1.0F, 1.0F}},
			/* NOTE: X- sphere color in washed red. */
			{{-100.0F, 0.0F, 0.0F}, {0.2F, 0.1F, 0.1F, 1.0F}},
			/* NOTE: Y- sphere color in washed green. */
			{{0.0F, -100.0F, 0.0F}, {0.1F, 0.2F, 0.1F, 1.0F}},
			/* NOTE: Z- sphere color in washed blue. */
			{{0.0F, 0.0F, -100.0F}, {0.1F, 0.1F, 0.2F, 1.0F}}
		}};

		size_t count = 0;

		for ( const auto & [position, color] : landmarks )
		{
			const auto label = String::incrementalLabel(CompassDisplay, count);

			Geometry::ResourceGenerator generator{resourceManager, Geometry::EnableNormal | Geometry::EnableVertexColor};
			generator.parameters().setGlobalVertexColor(color);

			const auto geometryResource = generator.sphere(8.0F, 16, 8, label);

			if ( geometryResource == nullptr )
			{
				return false;
			}

			const auto meshResource = resourceManager.container< Renderable::MeshResource >()->getOrCreateResource(label, [&geometryResource, &materialResource] (Renderable::MeshResource & newMesh) {
				return newMesh.load(geometryResource, materialResource);
			});

			if ( meshResource == nullptr )
			{
				return false;
			}

			const auto meshInstance = this->createStaticEntity(label, position)->componentBuilder< Component::Visual >(label).build(meshResource);

			if ( meshInstance == nullptr )
			{
				return false;
			}

			/* NOTE: Configure the renderable instance advanced options. */
			const auto renderableInstance = meshInstance->getRenderableInstance();
			renderableInstance->setUseInfinityView(true);
			renderableInstance->disableDepthTest(true);
		}

		return true;
	}

	void
	Scene::disableCompassDisplay () noexcept
	{
		for ( int index = 0; index < 6; index++ )
		{
			this->removeStaticEntity((std::stringstream{} << CompassDisplay << index).str());
		}
	}

	bool
	Scene::compassDisplayEnabled () const noexcept
	{
		return m_staticEntities.contains((std::stringstream{} << CompassDisplay << 0).str());
	}

	bool
	Scene::toggleCompassDisplay (Resources::Manager & resourceManager) noexcept
	{
		if ( this->compassDisplayEnabled() )
		{
			this->disableCompassDisplay();

			return false;
		}

		this->enableCompassDisplay(resourceManager);

		return true;
	}

	bool
	Scene::enableGroundZeroDisplay (Resources::Manager & resourceManager) noexcept
	{
		if ( this->groundZeroDisplayEnabled() )
		{
			return true;
		}

		const auto planeSize = m_boundary * Double< float >;
		const auto planeDivision = static_cast< uint32_t >(m_boundary / 100.0F);

		const auto materialResource = resourceManager.container< Material::BasicResource >()->getOrCreateResource("+DebugSceneMaterial", [] (Material::BasicResource & newMaterial) {
			newMaterial.enableVertexColor();

			return newMaterial.setManualLoadSuccess(true);
		});

		Geometry::ResourceGenerator generator{resourceManager, Geometry::EnableNormal | Geometry::EnableVertexColor | Geometry::EnablePrimitiveRestart};
		generator.parameters().setVertexColorGenMode(VertexColorGenMode::UseGlobalColor);
		generator.parameters().setGlobalVertexColor(White);

		const auto geometryResource = generator.surface(planeSize, planeDivision, GroundZeroPlaneDisplay);

		if ( geometryResource == nullptr )
		{
			return false;
		}

		const auto meshResource = resourceManager.container< Renderable::MeshResource >()->getOrCreateResource(GroundZeroPlaneDisplay, [&geometryResource, &materialResource] (Renderable::MeshResource & newMesh) {
			return newMesh.load(geometryResource, materialResource, {PolygonMode::Line, CullingMode::None});
		});

		if ( meshResource == nullptr )
		{
			return false;
		}

		const auto meshInstance = this->createStaticEntity(GroundZeroPlaneDisplay)->componentBuilder< Component::Visual >(GroundZeroPlaneDisplay).build(meshResource);

		if ( meshInstance == nullptr )
		{
			return false;
		}

		/* NOTE: Configure the renderable instance advanced options. */
		meshInstance->getRenderableInstance()->disableDepthTest(true);

		return true;
	}

	void
	Scene::disableGroundZeroDisplay () noexcept
	{
		this->removeStaticEntity(GroundZeroPlaneDisplay);
	}

	bool
	Scene::groundZeroDisplayEnabled () const noexcept
	{
		return m_staticEntities.contains(GroundZeroPlaneDisplay);
	}

	void
	Scene::toggleGroundZeroDisplay (Resources::Manager & resourceManager) noexcept
	{
		if ( this->groundZeroDisplayEnabled() )
		{
			this->disableGroundZeroDisplay();
		}
		else
		{
			this->enableGroundZeroDisplay(resourceManager);
		}
	}

	bool
	Scene::enableBoundaryPlanesDisplay (Resources::Manager & resourceManager) noexcept
	{
		if ( this->boundaryPlanesDisplayEnabled() )
		{
			return true;
		}

		const auto planeSize = m_boundary * Double< float >;
		const auto planeDivision = static_cast< uint32_t >(m_boundary / 100.0F);

		const auto materialResource = resourceManager.container< Material::BasicResource >()->getOrCreateResource("+DebugSceneMaterial", [] (Material::BasicResource & newMaterial) {
			newMaterial.enableVertexColor();

			return newMaterial.setManualLoadSuccess(true);
		});

		const std::array< std::pair< Vector< 3, float >, Color< float > >, 6 > planes{{
			/* NOTE: X+ plane color in bright red. */
			{{m_boundary, 0.0F, 0.0F}, {1.0F, 0.0F, 0.0F, 1.0F}},
			/* NOTE: Y+ plane color in bright green. */
			{{0.0F, m_boundary, 0.0F}, {0.0F, 1.0F, 0.0F, 1.0F}},
			/* NOTE: Z+ plane color in bright blue. */
			{{0.0F, 0.0F, m_boundary}, {0.0F, 0.0F, 1.0F, 1.0F}},
			/* NOTE: X- plane color in washed red. */
			{{-m_boundary, 0.0F, 0.0F}, {0.2F, 0.1F, 0.1F, 1.0F}},
			/* NOTE: Y- plane color in washed green. */
			{{0.0F, -m_boundary, 0.0F}, {0.1F, 0.2F, 0.1F, 1.0F}},
			/* NOTE: Z- plane color in washed blue. */
			{{0.0F, 0.0F, -m_boundary}, {0.1F, 0.1F, 0.2F, 1.0F}}
		}};

		size_t count = 0;

		for ( const auto & [position, color] : planes )
		{
			const auto label = String::incrementalLabel(BoundaryPlanesDisplay, count);

			Geometry::ResourceGenerator generator{resourceManager, Geometry::EnableNormal | Geometry::EnableVertexColor | Geometry::EnablePrimitiveRestart};
			generator.parameters().setVertexColorGenMode(VertexColorGenMode::UseGlobalColor);
			generator.parameters().setGlobalVertexColor(color);

			const auto geometryResource = generator.surface(planeSize, planeDivision, label);

			if ( geometryResource == nullptr )
			{
				return false;
			}

			const auto meshResource = resourceManager.container< Renderable::MeshResource >()->getOrCreateResource(label, [&geometryResource, &materialResource] (Renderable::MeshResource & newMesh) {
				return newMesh.load(geometryResource, materialResource, {PolygonMode::Line, CullingMode::None});
			});

			if ( meshResource == nullptr )
			{
				return false;
			}

			const auto entity = this->createStaticEntity(label, position);

			if ( position[X] != 0.0F )
			{
				entity->roll(Radian(QuartRevolution< float >), TransformSpace::Local);
			}

			if ( position[Z] != 0.0F )
			{
				entity->pitch(Radian(QuartRevolution< float >), TransformSpace::Local);
			}

			const auto meshInstance = entity->componentBuilder< Component::Visual >(label).build(meshResource);

			if ( meshInstance == nullptr )
			{
				return false;
			}

			/* NOTE: Configure the renderable instance options. */
			meshInstance->getRenderableInstance()->disableDepthTest(true);
		}

		return true;
	}

	void
	Scene::disableBoundaryPlanesDisplay () noexcept
	{
		for ( int index = 0; index < 6; index++ )
		{
			this->removeStaticEntity((std::stringstream{} << BoundaryPlanesDisplay << index).str());
		}
	}

	bool
	Scene::boundaryPlanesDisplayEnabled () const noexcept
	{
		return m_staticEntities.contains((std::stringstream{} << BoundaryPlanesDisplay << 0).str());
	}

	void
	Scene::toggleBoundaryPlanesDisplay (Resources::Manager & resourceManager) noexcept
	{
		if ( this->boundaryPlanesDisplayEnabled() )
		{
			this->disableGroundZeroDisplay();
		}
		else
		{
			this->enableGroundZeroDisplay(resourceManager);
		}
	}
}
