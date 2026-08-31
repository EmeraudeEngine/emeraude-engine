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
#include <limits>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <functional>
#include <ranges>
#include <string>
#include <thread>

/* Local inclusions. */
#include "Animations/AnimationClipResource.hpp"
#include "Animations/SkeletalAnimator.hpp"
#include "Graphics/Geometry/IndexedVertexResource.hpp"
#include "Graphics/Material/StandardResource.hpp"
#include "Graphics/Renderable/MeshResource.hpp"
#include "Graphics/Renderable/SkeletalDataTrait.hpp"
#include "Graphics/Renderable/SkyBoxResource.hpp"
#include "IO/IO.hpp"
#include "Math/Space3D/AACuboid.hpp"
#include "Resources/Manager.hpp"
#include "Scenes/Component/Camera.hpp"
#include "Scenes/Component/NodeAnimation.hpp"
#include "Scenes/Component/Visual.hpp"
#include "Scenes/Loaders/Interface.hpp"
#include "Scenes/Loaders/LoaderOptions.hpp"
#include "Scenes/Loaders/SceneData.hpp"
#include "Scenes/Manager.hpp"
#include "Scenes/Scene.hpp"
#include "Scenes/SceneDataConsumer.hpp"
#include "Scenes/Toolkit.hpp"
#include "SettingKeys.hpp"
#include "Settings.hpp"
#include "Tracer.hpp"
#include "VertexFactory/FileIO.hpp"

namespace EmEn::Scenes::Viewers
{
	using namespace Base;
	using namespace Base::Math;
	using namespace Graphics;

	namespace
	{
		/**
		 * @brief Visits every component of a viewer scene, static entities and node hierarchy alike.
		 * @warning The walk holds each entity's component mutex, which is NOT recursive: a visitor
		 * must never call back into the entity's component API. Driving an animator is safe, adding
		 * or enabling a component is not.
		 */
		void
		forEachSceneComponent (const Scene & scene, const std::function< void (Component::Abstract &) > & visit) noexcept
		{
			scene.forEachStaticEntities([&visit] (const StaticEntity & entity) {
				const_cast< StaticEntity & >(entity).forEachComponent([&visit] (Component::Abstract & component) {
					visit(component);

					return true;
				});

				return true;
			});

			const std::function< void (const std::shared_ptr< Node > &) > walk = [&visit, &walk] (const std::shared_ptr< Node > & node) {
				if ( node == nullptr )
				{
					return;
				}

				node->forEachComponent([&visit] (Component::Abstract & component) {
					visit(component);

					return true;
				});

				for ( const auto & child : node->children() | std::views::values )
				{
					walk(child);
				}
			};

			walk(scene.root());
		}
	}

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

			/* ⚠️ The state the asset APPEARS in is decided HERE and nowhere else. A skeletal animator is
			 * created LAZILY, on the Visual component's first logic cycle, so nothing built at scene time
			 * can stop one that does not exist yet — the intent has to be stated on the RENDERABLE. The
			 * viewer shows the asset at rest and lets the user walk the clips from there.
			 * ⚠️ This mutates a CACHED resource: the flag survives for every later instance of the same
			 * asset in this session, viewer or not. */
			for ( const auto & meshDescriptor : sceneData.meshes )
			{
				if ( auto * skeletalData = dynamic_cast< Renderable::SkeletalDataTrait * >(meshDescriptor.renderable.get()) )
				{
					skeletalData->enableAutoPlayFirstClip(false);
				}
			}

			/* ⚠️ Both lists are read and DEDUPLICATED by name: one glTF animation driving skin joints AND
			 * plain nodes comes out of the loader SPLIT into two clips sharing a name, and the user must
			 * see a single entry for it — applyAnimation() then drives whichever evaluators answer.
			 * ⚠️ What is kept is the CLIP's own name, never the resource key: the loaders prefix their
			 * keys ("FBX:<stem>/Animation/<clip>") while both animators index on clip().name(). Feeding a
			 * resource key to play() looks up something that never existed and returns false, silently. */
			const auto collectNames = [this] (const auto & clips) {
				for ( const auto & clip : clips )
				{
					if ( clip == nullptr )
					{
						continue;
					}

					const auto & clipName = clip->clip().name();

					if ( std::ranges::find(m_clipNames, clipName) == m_clipNames.end() )
					{
						m_clipNames.push_back(clipName);
					}
				}
			};

			collectNames(sceneData.animationClips);
			collectNames(sceneData.nodeAnimationClips);

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

		/* NOTE: An environment behind the model, and the IBL that comes with it. Without one, a
		 * reflective, transmissive, clearcoat, sheen or iridescent material has nothing to reflect
		 * and the asset simply cannot be judged — the whole point of a model viewer. A sky with a
		 * clear horizon line is what the Khronos normal/tangent tests ask for by name, the horizon
		 * being the readable feature in a curved reflection.
		 * ⚠️ The DIRECT lighting below stays manual on purpose: deriving it from the sky
		 * (applyBackgroundLighting) would make every sky change the exposure of the subject, and
		 * the viewer's exposure is deliberately fixed (see the camera setup further down). The
		 * background feeds the reflections, not the key light. */
		this->installBackground(*scene);

		/* ⚠️ AFTER the background, and that order is the whole point: a background installs its own
		 * cubemap as the scene's environment, so an explicit override has to come second. Two
		 * independent axes — what is BEHIND the subject and what the subject REFLECTS — which is
		 * what lets a session reproduce the Khronos references' black backdrop with a bright studio
		 * reflection. */
		this->installEnvironmentCubemap(*scene);

		/* NOTE: Neutral lighting recipe : a soft ambient and one warm key light.
		 * The flat ambient is a floor for the case where no background resource is available; the
		 * sky irradiance dominates it by two orders of magnitude when one is.
		 * ⚠️ It is a SETTING because 200 lux of flat ambient washes out a sheen rim or an
		 * iridescence fringe — exactly what the tests Khronos shoots on black are measuring. */
		auto & lightSet = scene->lightSet();
		lightSet.enable();
		lightSet.setAmbientLightColor({0.4F, 0.4F, 0.45F, 1.0F});
		lightSet.setAmbientLightIntensity(m_settings.getOrSetDefault< float >(ViewerAmbientIntensityKey, DefaultViewerAmbientIntensity));

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
			/* ⚠️ The floor used to be 0.01F — one CENTIMETRE — which silently re-framed every
			 * sub-centimetre asset as if it were a centimetre across. The Khronos
			 * MetalRoughSpheresNoTextures has a radius of 0.00035 m: it was framed 28 times too
			 * far away before anything else clipped it. The floor exists only to keep a degenerate
			 * (zero-extent) box from producing a zero distance, so it belongs at the smallest
			 * positive value, not at a scale. */
			radius = std::max(0.5F * modelBox.highestLength(), std::numeric_limits< float >::min());
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

		/* ⚠️⚠️ THE NEAR PLANE IS DERIVED FROM THIS, and it used to be a constant 0.1 m for every
		 * scene — so a subject smaller than a decimetre sat inside the near plane and rendered
		 * NOTHING.
		 *
		 * One PERCENT of the radius, and not the framing distance minus the radius, because the
		 * orbit controller's lower limit lets the user dolly to 5 % of the radius — i.e. INSIDE the
		 * subject, deliberately. There is no positive "nearest surface point" at that distance, so
		 * the only sane rule is a small positive fraction of the subject's own scale: it clips
		 * nothing the user can reach, and it scales with the asset instead of assuming a decimetre.
		 * The far side of the same coin: telling the camera the true scale also stops a large scene
		 * spending its depth precision in the first ten centimetres (conventional Z, no
		 * reversed-Z). */
		camera->setNearestObjectDistance(radius * 0.01F);

		auto & orbitController = scene->orbitController();
		/* ⚠️ The lower limit was `max(radius * 0.05F, 0.01F)` — the same centimetre assumption, and
		 * `OrbitController::setDistance()` CLAMPS to these limits, so a millimetric subject could
		 * never be approached at all. The relative term is the intended behaviour (dolly to 5 % of
		 * the radius, inside the subject if the user insists); the absolute one was a scale-blind
		 * guard, and the controller already floors the minimum at float epsilon on its own. */
		orbitController.setDistanceLimits(radius * 0.05F, std::min(radius * 20.0F, SceneBoundary * 0.9F));
		orbitController.setTarget(target);
		orbitController.setOrientation(0.785F, 0.3F);
		orbitController.setDistance(framingDistance);
		orbitController.controlNode(cameraNode);

		return scene;
	}

	void
	ModelViewer::installBackground (Scene & scene) noexcept
	{
		const auto backgroundName = m_settings.getOrSetDefault< std::string >(ViewerBackgroundKey, DefaultViewerBackground);

		if ( backgroundName.empty() )
		{
			return;
		}

		auto * skyBoxes = m_resourceManager.container< Renderable::SkyBoxResource >();

		if ( skyBoxes == nullptr )
		{
			return;
		}

		/* NOTE: The resource belongs to the consumer's data store, so a missing name is a
		 * configuration matter, never a viewer failure: the model still opens, against a void. */
		const auto skyBox = skyBoxes->getResource(backgroundName);

		if ( skyBox == nullptr )
		{
			TraceWarning{ClassId} <<
				"No skybox resource named '" << backgroundName << "' (setting '" << ViewerBackgroundKey << "'). "
				"The model is displayed without an environment: reflections, transmission and clearcoat "
				"will have nothing to reflect.";

			return;
		}

		scene.setBackground(skyBox);
	}

	void
	ModelViewer::installEnvironmentCubemap (Scene & scene) noexcept
	{
		const auto cubemapName = m_settings.getOrSetDefault< std::string >(ViewerEnvironmentCubemapKey, DefaultViewerEnvironmentCubemap);

		/* Empty is the default and means "keep whatever the background installed" — the behaviour
		 * every session had before this setting existed. */
		if ( cubemapName.empty() )
		{
			return;
		}

		auto * cubemaps = m_resourceManager.container< TextureResource::TextureCubemap >();

		if ( cubemaps == nullptr )
		{
			return;
		}

		const auto cubemap = cubemaps->getResource(cubemapName);

		/* NOTE: Same contract as the background: the resource belongs to the consumer's data store,
		 * so an unknown name is a configuration matter and never a viewer failure. The subject keeps
		 * reflecting whatever the background installed. */
		if ( cubemap == nullptr )
		{
			TraceWarning{ClassId} <<
				"No cubemap resource named '" << cubemapName << "' (setting '" << ViewerEnvironmentCubemapKey << "'). "
				"The subject keeps reflecting the background's environment.";

			return;
		}

		scene.setEnvironmentCubemap(cubemap);
	}


	void
	ModelViewer::applyAnimation (Scene & scene, const std::vector< std::string > & clipNames, size_t animationIndex) noexcept
	{
		/* ⚠️ Index 0 is NO animation — the rest pose — and a cycle wraps back to it. */
		const auto stopping = animationIndex == 0 || clipNames.empty();
		const auto clipName =
			stopping ?
			std::string{} :
			clipNames[(animationIndex - 1) % clipNames.size()];

		/* An asset may need EITHER evaluator, or BOTH at once. Counting what was ASKED against what
		 * ANSWERED is what turns a silent no-op into a diagnosable one.
		 * ⚠️ The two counters are NOT redundant: a Visual has no animator until its first logic cycle,
		 * so nothing being asked is legitimate right after a load — warning on `answered == 0` alone
		 * would cry wolf every time. */
		size_t asked = 0;
		size_t answered = 0;

		forEachSceneComponent(scene, [&clipName, stopping, &asked, &answered] (Component::Abstract & component) {
			if ( auto * visual = dynamic_cast< Component::Visual * >(&component) )
			{
				/* ⚠️ May legitimately be null — see above. Index 0 needs no animator anyway: the
				 * renderable's auto-play flag already leaves a fresh asset at rest. */
				if ( auto * animator = visual->skeletalAnimator() )
				{
					asked++;

					if ( stopping )
					{
						animator->stop();

						answered++;
					}
					else if ( animator->play(clipName) )
					{
						answered++;
					}
				}

				return;
			}

			if ( auto * nodeAnimation = dynamic_cast< Component::NodeAnimation * >(&component) )
			{
				asked++;

				if ( stopping )
				{
					nodeAnimation->stop();

					answered++;
				}
				else if ( nodeAnimation->play(clipName) )
				{
					answered++;
				}
			}
		});

		if ( !stopping && asked > 0 && answered == 0 )
		{
			TraceWarning{ClassId} << "None of the " << asked << " evaluator(s) reached knows the clip '" << clipName << "' !";
		}
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
