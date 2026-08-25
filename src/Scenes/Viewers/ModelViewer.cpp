/*
 * src/Scenes/Viewers/ModelViewer.cpp
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
 */

#include "ModelViewer.hpp"

/* STL inclusions. */
#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <string>
#include <thread>

/* Local inclusions. */
#include "Graphics/Geometry/IndexedVertexResource.hpp"
#include "Graphics/Material/StandardResource.hpp"
#include "Graphics/Renderable/MeshResource.hpp"
#include "IO/IO.hpp"
#include "Math/Space3D/AACuboid.hpp"
#include "Resources/Manager.hpp"
#include "Scenes/Component/Camera.hpp"
#include "Scenes/Component/Visual.hpp"
#include "Scenes/Loaders/Interface.hpp"
#include "Scenes/Loaders/LoaderOptions.hpp"
#include "Scenes/Loaders/SceneData.hpp"
#include "Scenes/Manager.hpp"
#include "Scenes/Scene.hpp"
#include "Scenes/SceneDataConsumer.hpp"
#include "Scenes/Toolkit.hpp"
#include "Tracer.hpp"
#include "VertexFactory/FileIO.hpp"

namespace EmEn::Scenes::Viewers
{
	using namespace Base;
	using namespace Base::Math;
	using namespace Graphics;

	bool
	ModelViewer::handlesFile (const Manager & sceneManager, const std::filesystem::path & filepath) noexcept
	{
		if ( sceneManager.createSceneLoader(filepath) != nullptr )
		{
			return true;
		}

		return VertexFactory::FileIO::isReadableExtension(IO::getFileExtension(filepath, true));
	}

	std::shared_ptr< Scene >
	ModelViewer::createScene (const std::filesystem::path & filepath) noexcept
	{
		const auto loader = m_sceneManager.createSceneLoader(filepath);

		if ( loader == nullptr && !VertexFactory::FileIO::isReadableExtension(IO::getFileExtension(filepath, true)) )
		{
			TraceError{ClassId} << "No loader handles the file '" << IO::toU8String(filepath) << "' !";

			return nullptr;
		}

		/* NOTE: A leftover viewer scene blocks the name, remove it first. */
		if ( m_sceneManager.hasSceneNamed(SceneName) && !m_sceneManager.deleteScene(SceneName) )
		{
			return nullptr;
		}

		const auto scene = m_sceneManager.newScene(SceneName, SceneBoundary);

		if ( scene == nullptr )
		{
			return nullptr;
		}

		/* NOTE: Stays empty on the raw geometry path, which makes the
		 * readiness check over its meshes trivially true. */
		Loaders::SceneData sceneData;

		if ( loader != nullptr )
		{
			/* NOTE: Showcase reflection level, the asset is displayed for itself. */
			Loaders::LoaderOptions loaderOptions;
			loaderOptions.environmentReflectionIntensity = 1.0F;

			loader->setOptions(loaderOptions);

			/* NOTE: Synchronous import on the calling thread, intended for reasonably sized assets. */
			if ( !loader->load(filepath, sceneData) )
			{
				TraceError{ClassId} << "Unable to import the file '" << IO::toU8String(filepath) << "' !";

				m_sceneManager.deleteScene(SceneName);

				return nullptr;
			}

			SceneDataConsumer consumer;

			if ( !consumer.build(sceneData, *scene) )
			{
				TraceError{ClassId} << "Unable to build the scene from the file '" << IO::toU8String(filepath) << "' !";

				m_sceneManager.deleteScene(SceneName);

				return nullptr;
			}
		}
		else if ( !this->importGeometry(filepath, *scene) )
		{
			TraceError{ClassId} << "Unable to import the geometry file '" << IO::toU8String(filepath) << "' !";

			m_sceneManager.deleteScene(SceneName);

			return nullptr;
		}

		/* NOTE: Neutral lighting recipe : a soft ambient and one warm key light. */
		auto & lightSet = scene->lightSet();
		lightSet.enable();
		lightSet.setAmbientLightColor({0.4F, 0.4F, 0.45F, 1.0F});
		lightSet.setAmbientLightIntensity(200.0F);

		Toolkit toolkit{m_settings, m_resourceManager, scene};
		toolkit.setCursor(600.0F, 1000.0F, 600.0F);
		toolkit.generateDirectionalLight< StaticEntity >("KeyLight", {1.0F, 0.98F, 0.92F, 1.0F}, 100000.0F);

		/* NOTE: A cool fill light opposite the key keeps the shadow
		 * side readable while the orbit passes behind the model. */
		toolkit.setCursor(-600.0F, 400.0F, -600.0F);
		toolkit.generateDirectionalLight< StaticEntity >("FillLight", {0.9F, 0.95F, 1.0F, 1.0F}, 15000.0F);

		/* NOTE: The loaders enqueue mesh resources on the thread pool, the entity extents
		 * publish only once the resources are loaded. The wait is bounded : on a timeout,
		 * the camera starts on the fallback framing and the dolly recovers the view. */
		Space3D::AACuboid< float > modelBox;

		for ( uint32_t waitedMS = 0; waitedMS < ExtentsWaitBudgetMS; waitedMS += 10U )
		{
			modelBox.reset();

			scene->forEachStaticEntities([&modelBox] (const StaticEntity & staticEntity) {
				const auto entityBox = staticEntity.getWorldRenderBoundingBox();

				if ( entityBox.isValid() )
				{
					modelBox.merge(entityBox);
				}

				return true;
			});

			const auto allMeshesReady = std::ranges::all_of(sceneData.meshes, [] (const auto & meshDescriptor) {
				return meshDescriptor.renderable == nullptr || meshDescriptor.renderable->isReadyForInstantiation();
			});

			if ( modelBox.isValid() && allMeshesReady )
			{
				break;
			}

			std::this_thread::sleep_for(std::chrono::milliseconds(10));
		}

		auto target = Vector< 3, float >{0.0F, 0.0F, 0.0F};
		auto radius = 2.0F;

		if ( modelBox.isValid() )
		{
			target = modelBox.centroid();
			radius = std::max(0.5F * modelBox.highestLength(), 0.01F);
		}
		else
		{
			TraceWarning{ClassId} << "The imported content published no extents in time, using the fallback framing.";
		}

		/* NOTE: The camera node stays a direct child of the scene root, the
		 * orbit controller positions it in parent space. */
		const auto cameraNode = scene->root()->createChild("ViewerCamera");

		if ( cameraNode == nullptr )
		{
			m_sceneManager.deleteScene(SceneName);

			return nullptr;
		}

		const auto camera = cameraNode->componentBuilder< Component::Camera >("ViewerCamera")
			.asPrimary()
			.build();

		/* NOTE: The neutral lighting uses photometric intensities, the HDR camera brings
		 * the frame back to a readable range. The exposure is MANUAL and paired with the
		 * key light (sunny sixteen : 100 000 lux, f/16, 1/100 s, ISO 100) : the automatic
		 * metering averages the whole frame, and a small lit model over the black void
		 * would be crushed to white. */
		camera->setFocalLength(ViewerFocalLength);
		camera->enableHDR(true);
		camera->setAutoExposure(false);
		camera->setAperture(16.0F);
		camera->setShutterSpeed(1.0F / 100.0F);
		camera->setSensitivity(100.0F);
		camera->setDistance(std::max(100.0F, radius * 20.0F));

		/* NOTE: Distance fitting the model bounding sphere inside the vertical field
		 * of view, from a three-quarter view slightly above the model. */
		const auto fieldOfView = Radian(camera->fieldOfView());
		const auto framingDistance = FramingMargin * radius / std::sin(0.5F * fieldOfView);

		auto & orbitController = scene->orbitController();
		orbitController.setDistanceLimits(std::max(radius * 0.05F, 0.01F), std::min(radius * 20.0F, SceneBoundary * 0.9F));
		orbitController.setTarget(target);
		orbitController.setOrientation(0.785F, 0.3F);
		orbitController.setDistance(framingDistance);
		orbitController.controlNode(cameraNode);

		return scene;
	}

	bool
	ModelViewer::importGeometry (const std::filesystem::path & filepath, Scene & scene) noexcept
	{
		/* NOTE: Each viewing session gets its own resource names, a resource
		 * container always returns an existing resource for a known name. */
		static std::atomic< uint32_t > s_sessionCount{0};

		const auto suffix = std::to_string(++s_sessionCount);

		const auto geometry = m_resourceManager.container< Geometry::IndexedVertexResource >()->getOrCreateResource("+ModelViewerGeometry" + suffix, [filepath] (Geometry::IndexedVertexResource & geometryResource) {
			return geometryResource.load(filepath);
		});

		/* NOTE: A raw geometry file carries no material, the mesh wears
		 * a neutral clay : mid-grey albedo, dull, dielectric. */
		const auto material = m_resourceManager.container< Material::StandardResource >()->getOrCreateResource("+ModelViewerMaterial" + suffix, [] (Material::StandardResource & materialResource) {
			if ( !materialResource.setAlbedoComponent(PixelFactory::Color< float >{0.5F, 0.5F, 0.5F, 1.0F}) )
			{
				return materialResource.setManualLoadSuccess(false);
			}

			if ( !materialResource.setRoughnessComponent(0.65F) || !materialResource.setMetalnessComponent(0.0F) )
			{
				return materialResource.setManualLoadSuccess(false);
			}

			return materialResource.setManualLoadSuccess(true);
		});

		const auto mesh = m_resourceManager.container< Renderable::MeshResource >()->getOrCreateResource("+ModelViewerMesh" + suffix, [geometry, material] (Renderable::MeshResource & meshResource) {
			return meshResource.load(geometry, material);
		});

		if ( mesh == nullptr )
		{
			return false;
		}

		const auto modelEntity = scene.createStaticEntity("Model");

		if ( modelEntity == nullptr )
		{
			return false;
		}

		/* NOTE: The lit path is explicit, exactly like the scene data consumer does :
		 * an unlit instance would send its raw [0,1] albedo through the photometric
		 * exposure and read black. */
		modelEntity->componentBuilder< Component::Visual >("Model")
			.setup([] (Component::Visual & component) {
				component.getRenderableInstance()->setLightingState(true);
			})
			.build(mesh);

		return true;
	}
}
