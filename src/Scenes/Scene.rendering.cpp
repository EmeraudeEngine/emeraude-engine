/*
 * src/Scenes/Scene.rendering.cpp
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
#include "Graphics/BindlessTextureManager.hpp"
#include "Graphics/Renderable/Abstract.hpp"
#include "Graphics/Renderer.hpp"
#include "NodeCrawler.hpp"

namespace EmEn::Scenes
{
	using namespace Libs;
	using namespace Libs::Math;
	using namespace Graphics;

	std::shared_ptr< RenderTarget::ShadowMap< ViewMatrices2DUBO > >
	Scene::createRenderToShadowMap (const std::string & name, uint32_t resolution, float viewDistance, bool isOrthographicProjection) noexcept
	{
		const std::lock_guard< std::mutex > lock{m_renderToShadowMapAccess};

		if ( m_AVConsoleManager.isVideoDeviceExists(name) )
		{
			TraceError{ClassId} <<
				"A virtual video device named '" << name << "' already exists ! "
				"Render to shadow map creation canceled ...";

			return {};
		}

		/* Create the render target.
		 * TODO: Get the view distance value from settings. */
		auto renderTarget = std::make_shared< RenderTarget::ShadowMap< ViewMatrices2DUBO > >(name, resolution, viewDistance, isOrthographicProjection);

		if ( !renderTarget->createRenderTarget(m_AVConsoleManager.graphicsRenderer()) )
		{
			TraceError{ClassId} << "Unable to create the render to shadow map '" << name << "' !";

			return {};
		}

		if ( !m_AVConsoleManager.addVideoDevice(renderTarget, false) )
		{
			TraceError{ClassId} << "Unable to add the render to shadow map '" << name << "' as a virtual video device !";

			return {};
		}

		m_renderToShadowMaps.emplace(renderTarget);

		return renderTarget;
	}

	std::shared_ptr< RenderTarget::ShadowMap< ViewMatrices3DUBO > >
	Scene::createRenderToCubicShadowMap (const std::string & name, uint32_t resolution, float viewDistance) noexcept
	{
		const std::lock_guard< std::mutex > lock{m_renderToShadowMapAccess};

		if ( m_AVConsoleManager.isVideoDeviceExists(name) )
		{
			TraceError{ClassId} <<
				"A virtual video device named '" << name << "' already exists ! "
				"Render to cubic shadow map creation canceled ...";

			return {};
		}

		/* Create the render target. */
		auto renderTarget = std::make_shared< RenderTarget::ShadowMap< ViewMatrices3DUBO > >(name, resolution, viewDistance);

		if ( !renderTarget->createRenderTarget(m_AVConsoleManager.graphicsRenderer()) )
		{
			TraceError{ClassId} << "Unable to create the render to cubic shadow map '" << name << "' !";

			return {};
		}

		if ( !m_AVConsoleManager.addVideoDevice(renderTarget, false) )
		{
			TraceError{ClassId} << "Unable to add the render to cubic shadow map '" << name << "' as a virtual video device !";

			return {};
		}

		m_renderToShadowMaps.emplace(renderTarget);

		return renderTarget;
	}

	std::shared_ptr< RenderTarget::ShadowMap< ViewMatricesCascadedUBO > >
	Scene::createRenderToCascadedShadowMap (const std::string & name, uint32_t resolution, float viewDistance, uint32_t cascadeCount, float lambda) noexcept
	{
		const std::lock_guard< std::mutex > lock{m_renderToShadowMapAccess};

		if ( m_AVConsoleManager.isVideoDeviceExists(name) )
		{
			TraceError{ClassId} <<
				"A virtual video device named '" << name << "' already exists ! "
				"Render to cascaded shadow map creation canceled ...";

			return {};
		}

		/* Create the cascaded shadow map render target. */
		auto renderTarget = std::make_shared< RenderTarget::ShadowMap< ViewMatricesCascadedUBO > >(name, resolution, viewDistance, cascadeCount, lambda);

		if ( !renderTarget->createRenderTarget(m_AVConsoleManager.graphicsRenderer()) )
		{
			TraceError{ClassId} << "Unable to create the render to cascaded shadow map '" << name << "' !";

			return {};
		}

		if ( !m_AVConsoleManager.addVideoDevice(renderTarget, false) )
		{
			TraceError{ClassId} << "Unable to add the render to cascaded shadow map '" << name << "' as a virtual video device !";

			return {};
		}

		m_renderToShadowMaps.emplace(renderTarget);

		TraceSuccess{ClassId} << "Cascaded shadow map '" << name << "' (" << cascadeCount << " cascades, " << resolution << "px²) created successfully.";

		return renderTarget;
	}

	std::shared_ptr< RenderTarget::Texture< ViewMatrices2DUBO > >
	Scene::createRenderToTexture2D (const std::string & name, uint32_t width, uint32_t height, uint32_t colorCount, float viewDistance, bool isOrthographicProjection) noexcept
	{
		const std::lock_guard< std::mutex > lock{m_renderToTextureAccess};

		if ( m_AVConsoleManager.isVideoDeviceExists(name) )
		{
			TraceError{ClassId} <<
				"A virtual video device named '" << name << "' already exists ! "
				"Render to texture 2D creation canceled ...";

			return {};
		}

		/* Create the render target.
		 * TODO: Get the view distance value from settings. */
		auto renderTarget = std::make_shared< RenderTarget::Texture< ViewMatrices2DUBO > >(name, width, height, colorCount, viewDistance, isOrthographicProjection);

		if ( !renderTarget->createRenderTarget(m_AVConsoleManager.graphicsRenderer()) )
		{
			TraceError{ClassId} << "Unable to create the render to texture 2D '" << name << "' !";

			return {};
		}

		if ( !m_AVConsoleManager.addVideoDevice(renderTarget, false) )
		{
			TraceError{ClassId} << "Unable to add the render to texture 2D '" << name << "' as a virtual video device !";

			return {};
		}

		m_renderToTextures.emplace(renderTarget);

		return renderTarget;
	}

	std::shared_ptr< RenderTarget::Texture< ViewMatrices3DUBO > >
	Scene::createRenderToCubemap (const std::string & name, uint32_t size, uint32_t colorCount, float viewDistance, bool isOrthographicProjection) noexcept
	{
		const std::lock_guard< std::mutex > lock{m_renderToTextureAccess};

		if ( m_AVConsoleManager.isVideoDeviceExists(name) )
		{
			TraceError{ClassId} <<
				"A virtual video device named '" << name << "' already exists ! "
				"Render to cubemap creation canceled ...";

			return {};
		}

		/* Create the render target.
		 * TODO: Get the view distance value from settings. */
		auto renderTarget = std::make_shared< RenderTarget::Texture< ViewMatrices3DUBO > >(name, size, colorCount, viewDistance, isOrthographicProjection);

		if ( !renderTarget->createRenderTarget(m_AVConsoleManager.graphicsRenderer()) )
		{
			TraceError{ClassId} << "Unable to create the render to cubemap '" << name << "' !";

			return {};
		}

		if ( !m_AVConsoleManager.addVideoDevice(renderTarget, false) )
		{
			TraceError{ClassId} << "Unable to add the render to cubemap '" << name << "' as a virtual video device !";

			return {};
		}

		m_renderToTextures.emplace(renderTarget);

		return renderTarget;
	}

	std::shared_ptr< RenderTarget::View< ViewMatrices2DUBO > >
	Scene::createRenderToView (const std::string & name, uint32_t width, uint32_t height, const FramebufferPrecisions & precisions, float viewDistance, bool isOrthographicProjection, bool primaryDevice) noexcept
	{
		const std::lock_guard< std::mutex > lock{m_renderToViewAccess};

		if ( m_AVConsoleManager.isVideoDeviceExists(name) )
		{
			TraceError{ClassId} << "A virtual device named '" << name << "' already exists ! Render to view creation canceled ...";

			return {};
		}

		/* Create the render target.
		 * TODO: Get the view distance value from settings. */
		auto renderTarget = std::make_shared< RenderTarget::View< ViewMatrices2DUBO > >(name, width, height, precisions, viewDistance, isOrthographicProjection);

		if ( !renderTarget->createRenderTarget(m_AVConsoleManager.graphicsRenderer()) )
		{
			TraceError{ClassId} << "Unable to create the render to view '" << name << "' !";

			return {};
		}

		if ( !m_AVConsoleManager.addVideoDevice(renderTarget, primaryDevice) )
		{
			TraceError{ClassId} << "Unable to add the render to view '" << name << "' as a virtual video device !";

			return {};
		}

		m_renderToViews.emplace(renderTarget);

		return renderTarget;
	}

	std::shared_ptr< RenderTarget::View< ViewMatrices3DUBO > >
	Scene::createRenderToCubicView (const std::string & name, uint32_t size, const FramebufferPrecisions & precisions, float viewDistance, bool isOrthographicProjection, bool primaryDevice) noexcept
	{
		const std::lock_guard< std::mutex > lock{m_renderToViewAccess};

		/* Checks name availability. */
		if ( m_AVConsoleManager.isVideoDeviceExists(name) )
		{
			TraceError{ClassId} << "A virtual device named '" << name << "' already exists ! Render to cubic view creation canceled ...";

			return {};
		}

		/* Create the render target.
		 * TODO: Get the view distance value from settings. */
		auto renderTarget = std::make_shared< RenderTarget::View< ViewMatrices3DUBO > >(name, size, precisions, viewDistance, isOrthographicProjection);

		if ( !renderTarget->createRenderTarget(m_AVConsoleManager.graphicsRenderer()) )
		{
			TraceError{ClassId} << "Unable to create the render to cubic view '" << name << "' !";

			return {};
		}

		if ( !m_AVConsoleManager.addVideoDevice(renderTarget, primaryDevice) )
		{
			TraceError{ClassId} << "Unable to add the render to cubic view '" << name << "' as a virtual video device !";

			return {};
		}

		m_renderToViews.emplace(renderTarget);

		return renderTarget;
	}

	void
	Scene::updateVideoMemory (bool shadowMapEnabled, bool renderToTextureEnabled) const noexcept
	{
		const uint32_t readStateIndex = m_renderStateIndex.load(std::memory_order_acquire);

		if ( shadowMapEnabled )
		{
			if ( !m_renderToShadowMaps.empty() )
			{
				this->forEachRenderToShadowMap([readStateIndex] (const auto & renderTarget) {
					if ( !renderTarget->viewMatrices().updateVideoMemory(readStateIndex) )
					{
						TraceError{ClassId} << "Failed to update the video memory of the render target (Shadow map) from readStateIndex #" << readStateIndex << " !";
					}
				});
			}
		}

		if ( renderToTextureEnabled && !m_renderToTextures.empty() )
		{
			this->forEachRenderToTexture([readStateIndex] (const auto & renderTarget) {
				if ( !renderTarget->viewMatrices().updateVideoMemory(readStateIndex) )
				{
					TraceError{ClassId} << "Failed to update the video memory of the render target (Texture) from readStateIndex #" << readStateIndex << " !";
				}
			});
		}

		/* NOTE: There should be at least the swap chain! */
		this->forEachRenderToView([readStateIndex] (const auto & renderTarget) {
			if ( !renderTarget->viewMatrices().updateVideoMemory(readStateIndex) )
			{
				TraceError{ClassId} << "Failed to update the video memory of the render target (View) from readStateIndex #" << readStateIndex << " !";
			}
		});

		if ( !m_lightSet.updateVideoMemory() )
		{
			Tracer::error(ClassId, "Unable to update the light set data to the video memory !");
		}
	}

	void
	Scene::castShadows (const std::shared_ptr< RenderTarget::Abstract > & renderTarget, const Vulkan::CommandBuffer & commandBuffer) noexcept
	{
		const uint32_t readStateIndex = m_renderStateIndex.load(std::memory_order_acquire);

		if ( !m_lightSet.isEnabled() )
		{
			return;
		}

		/* Sort the scene according to the point of view. */
		if ( !this->populateShadowCastingRenderList(renderTarget, readStateIndex) )
		{
			/* There is nothing to shadow to cast ... */
			return;
		}

		/*TraceDebug{ClassId} <<
			"Shadow map content :" "\n"
			" - Plain objects : " << m_renderLists[Shadows].size() << "\n";*/

		for ( const auto & renderBatch : m_renderLists[Shadows] | std::views::values )
		{
			renderBatch.renderableInstance()->castShadows(readStateIndex, renderTarget, renderBatch.subGeometryIndex(), renderBatch.worldCoordinates(), commandBuffer);
		}
	}

	bool
	Scene::prepareRender (const std::shared_ptr< RenderTarget::Abstract > & renderTarget) noexcept
	{
		m_preparedReadStateIndex = m_renderStateIndex.load(std::memory_order_acquire);

		const auto & bindlessManager = m_AVConsoleManager.graphicsRenderer().bindlessTextureManager();

		m_preparedBindlessManager = bindlessManager.usable() ? &bindlessManager : nullptr;

		if ( !this->populateRenderLists(renderTarget, m_preparedReadStateIndex) )
		{
			return false;
		}

		/* Rebuild the TLAS and RT metadata from RT-specific render lists (no frustum culling).
		 * RT effects cast rays in world space and need ALL scene geometry, not just what's on screen. */
		auto * mutableBindlessManager = bindlessManager.usable() ? &m_AVConsoleManager.graphicsRenderer().bindlessTextureManager() : nullptr;
		const auto frameIndex = m_AVConsoleManager.graphicsRenderer().currentFrameIndex();
		m_sceneMetaData.rebuild(m_rtOpaqueList, m_rtOpaqueLightedList, mutableBindlessManager, frameIndex);

		return true;
	}

	void
	Scene::renderOpaque (const std::shared_ptr< RenderTarget::Abstract > & renderTarget, const Vulkan::CommandBuffer & commandBuffer) noexcept
	{
		if ( !m_renderLists[Opaque].empty() )
		{
			for ( const auto & renderBatch : m_renderLists[Opaque] | std::views::values )
			{
				renderBatch.renderableInstance()->render(m_preparedReadStateIndex, renderTarget, nullptr, RenderPassType::SimplePass, renderBatch.subGeometryIndex(), renderBatch.worldCoordinates(), commandBuffer, m_preparedBindlessManager);
			}
		}

		if ( m_lightSet.isEnabled() && !m_renderLists[OpaqueLighted].empty() )
		{
			this->renderLightedSelection(renderTarget, m_preparedReadStateIndex, commandBuffer, m_renderLists[OpaqueLighted], m_preparedBindlessManager);
		}
	}

	void
	Scene::renderTranslucent (const std::shared_ptr< RenderTarget::Abstract > & renderTarget, const Vulkan::CommandBuffer & commandBuffer) noexcept
	{
		if ( !m_renderLists[Translucent].empty() )
		{
			for ( const auto & renderBatch : m_renderLists[Translucent] | std::views::values )
			{
				renderBatch.renderableInstance()->render(m_preparedReadStateIndex, renderTarget, nullptr, RenderPassType::SimplePass, renderBatch.subGeometryIndex(), renderBatch.worldCoordinates(), commandBuffer, m_preparedBindlessManager);
			}
		}

		if ( m_lightSet.isEnabled() && !m_renderLists[TranslucentLighted].empty() )
		{
			this->renderLightedSelection(renderTarget, m_preparedReadStateIndex, commandBuffer, m_renderLists[TranslucentLighted], m_preparedBindlessManager);
		}
	}

	void
	Scene::renderTranslucentGB (const std::shared_ptr< RenderTarget::Abstract > & renderTarget, const Vulkan::CommandBuffer & commandBuffer) noexcept
	{
		if ( !m_renderLists[TranslucentGB].empty() )
		{
			for ( const auto & renderBatch : m_renderLists[TranslucentGB] | std::views::values )
			{
				renderBatch.renderableInstance()->render(m_preparedReadStateIndex, renderTarget, nullptr, RenderPassType::SimplePass, renderBatch.subGeometryIndex(), renderBatch.worldCoordinates(), commandBuffer, m_preparedBindlessManager);
			}
		}

		if ( m_lightSet.isEnabled() && !m_renderLists[TranslucentGBLighted].empty() )
		{
			this->renderLightedSelection(renderTarget, m_preparedReadStateIndex, commandBuffer, m_renderLists[TranslucentGBLighted], m_preparedBindlessManager);
		}
	}

	bool
	Scene::hasTranslucentGBObjects () const noexcept
	{
		return !m_renderLists[TranslucentGB].empty() || !m_renderLists[TranslucentGBLighted].empty();
	}

	void
	Scene::renderTBNSpace (const std::shared_ptr< RenderTarget::Abstract > & renderTarget, const Vulkan::CommandBuffer & commandBuffer) noexcept
	{
		const uint32_t readStateIndex = m_renderStateIndex.load(std::memory_order_acquire);

		auto & renderer = m_AVConsoleManager.graphicsRenderer();

		/* Scene visual components (background, ground, sea). */
		for ( const auto & component : m_sceneVisualComponents )
		{
			if ( component == nullptr )
			{
				continue;
			}

			const auto renderableInstance = component->getRenderableInstance();

			if ( renderableInstance == nullptr || !renderableInstance->isDisplayTBNSpaceEnabled() )
			{
				continue;
			}

			/* Ensure TBN programs are generated (may not exist if flag was enabled after initial setup). */
			if ( !renderableInstance->getReadyForTBNSpace(renderTarget, renderer) )
			{
				continue;
			}

			const auto layerCount = renderableInstance->renderable()->layerCount();

			for ( uint32_t layerIndex = 0; layerIndex < layerCount; layerIndex++ )
			{
				renderableInstance->renderTBNSpace(readStateIndex, renderTarget, layerIndex, nullptr, commandBuffer);
			}
		}

		/* Static entities. */
		{
			const std::lock_guard< std::mutex > lock{m_staticEntitiesAccess};

			for ( const auto & staticEntity : std::ranges::views::values(m_staticEntities) )
			{
				if ( !staticEntity->isRenderable() )
				{
					continue;
				}

				const auto & worldCoordinates = staticEntity->getWorldCoordinatesStateForRendering(readStateIndex);

				staticEntity->forEachComponent([&] (const Component::Abstract & component) {
					const auto renderableInstance = component.getRenderableInstance();

					if ( renderableInstance == nullptr || !renderableInstance->isDisplayTBNSpaceEnabled() )
					{
						return;
					}

					/* Ensure TBN programs are generated. */
					if ( !renderableInstance->getReadyForTBNSpace(renderTarget, renderer) )
					{
						return;
					}

					const auto layerCount = renderableInstance->renderable()->layerCount();

					for ( uint32_t layerIndex = 0; layerIndex < layerCount; layerIndex++ )
					{
						renderableInstance->renderTBNSpace(readStateIndex, renderTarget, layerIndex, &worldCoordinates, commandBuffer);
					}
				});
			}
		}

		/* Scene node tree. */
		{
			const std::lock_guard< std::mutex > lock{m_sceneNodesAccess};

			NodeCrawler< const Node > crawler{m_rootNode};

			std::shared_ptr< const Node > node{};

			while ( (node = crawler.nextNode()) != nullptr )
			{
				if ( !node->isRenderable() )
				{
					continue;
				}

				const auto & worldCoordinates = node->getWorldCoordinatesStateForRendering(readStateIndex);

				node->forEachComponent([&] (const Component::Abstract & component) {
					const auto renderableInstance = component.getRenderableInstance();

					if ( renderableInstance == nullptr || !renderableInstance->isDisplayTBNSpaceEnabled() )
					{
						return;
					}

					/* Ensure TBN programs are generated. */
					if ( !renderableInstance->getReadyForTBNSpace(renderTarget, renderer) )
					{
						return;
					}

					const auto layerCount = renderableInstance->renderable()->layerCount();

					for ( uint32_t layerIndex = 0; layerIndex < layerCount; layerIndex++ )
					{
						renderableInstance->renderTBNSpace(readStateIndex, renderTarget, layerIndex, &worldCoordinates, commandBuffer);
					}
				});
			}
		}
	}

	void
	Scene::publishStateForRendering () noexcept
	{
		/* TODO: Check to copy only relevant data to speed up the transfer. */
		const uint32_t nextTarget = m_renderStateIndex == 0 ? 1 : 0;

		/* Synchronize static entities. */
		for ( const auto & staticEntity : std::ranges::views::values(m_staticEntities) )
		{
			staticEntity->publishStateForRendering(nextTarget);
		}

		/* Synchronize scene nodes. */
		{
			NodeCrawler< Node > crawler{m_rootNode};

			std::shared_ptr< Node > currentNode;

			while ( (currentNode = crawler.nextNode()) != nullptr )
			{
				currentNode->publishStateForRendering(nextTarget);
			}
		}

		/* Synchronize render targets. */
		{
			this->forEachRenderToShadowMap([&nextTarget] (const auto & renderTarget){
			   renderTarget->viewMatrices().publishStateForRendering(nextTarget);
			});

			this->forEachRenderToTexture([&nextTarget] (const auto & renderTarget){
				renderTarget->viewMatrices().publishStateForRendering(nextTarget);
			});

			this->forEachRenderToView([&nextTarget] (const auto & renderTarget){
				renderTarget->viewMatrices().publishStateForRendering(nextTarget);
			});
		}

		/* NOTE: Declare the new target to read from for the rendering thread. */
		m_renderStateIndex.store(nextTarget, std::memory_order_release);
	}

	void
	Scene::registerSceneVisualComponents () noexcept
	{
		if ( m_backgroundResource != nullptr )
		{
			m_sceneVisualComponents[0] = std::make_unique< Component::Visual >("Background", *m_rootNode, m_backgroundResource);

			/* NOTE: Disables lighting model and shadows on the background.
			 * The skybox should not cast or receive shadows. */
			const auto renderableInstance = m_sceneVisualComponents[0]->getRenderableInstance();
			renderableInstance->setUseInfinityView(true);
			renderableInstance->disableDepthTest(true);
			renderableInstance->disableDepthWrite(true);
			renderableInstance->disableShadowCasting();
			renderableInstance->disableShadowReceiving();
		}

		if ( m_groundLevelRenderable != nullptr )
		{
			m_sceneVisualComponents[1] = std::make_unique< Component::Visual >("SceneGround", *m_rootNode, m_groundLevelRenderable);

			const auto renderableInstance = m_sceneVisualComponents[1]->getRenderableInstance();
			renderableInstance->enableLighting();
			renderableInstance->disableLightDistanceCheck();
			renderableInstance->enableDisplayTBNSpace(false);
		}

		if ( m_seaLevelRenderable != nullptr )
		{
			m_sceneVisualComponents[2] = std::make_unique< Component::Visual >("SeaLevel", *m_rootNode, m_seaLevelRenderable);

			const auto renderableInstance = m_sceneVisualComponents[2]->getRenderableInstance();
			renderableInstance->enableLighting();
			renderableInstance->disableLightDistanceCheck();
			renderableInstance->enableDisplayTBNSpace(false);
		}
	}

	bool
	Scene::checkRenderableInstanceForShadowCasting (const std::shared_ptr< RenderTarget::Abstract > & renderTarget, const std::shared_ptr< RenderableInstance::Abstract > & renderableInstance) const noexcept
	{
		/* NOTE: Skip instances that have shadow casting disabled. */
		if ( renderableInstance->isShadowCastingDisabled() )
		{
			return true; // Continue (skip this instance)
		}

		/* Check whether the renderable instance is ready for shadow casting. */
		if ( renderableInstance->isReadyToCastShadows(renderTarget) )
		{
			return false; // Render
		}

		/* If it still unloaded. */
		if ( !renderableInstance->renderable()->isReadyForInstantiation() )
		{
			return true; // Continue
		}

		if ( this->getRenderableInstanceReadyForShadowCasting(renderableInstance, renderTarget) )
		{
			return false; // Render
		}

		/* If the object cannot be loaded, mark it as broken! */
		renderableInstance->setBroken("Unable to get ready for shadow casting !");

		return true; // Continue
	}

	bool
	Scene::populateShadowCastingRenderList (const std::shared_ptr< RenderTarget::Abstract > & renderTarget, uint32_t readStateIndex) noexcept
	{
		/* NOTE: Clean the render list before. */
		m_renderLists[Shadows].clear();

		/* NOTE: The camera position doesn't move during calculation. */
		const auto & cameraPosition = renderTarget->viewMatrices().position();
		const auto & frustum = renderTarget->viewMatrices().frustum(0);
		const auto viewDistance = renderTarget->viewDistance();

		for ( const auto & component : m_sceneVisualComponents )
		{
			if ( component == nullptr )
			{
				continue;
			}

			const auto renderableInstance = component->getRenderableInstance();

			if ( renderableInstance == nullptr )
			{
				continue;
			}

			if ( this->checkRenderableInstanceForShadowCasting(renderTarget, renderableInstance) )
			{
				continue;
			}

			this->insertIntoShadowCastingRenderList(renderableInstance, nullptr, 0.0F);
		}

		/* Sorting renderable objects from scene static entities. */
		{
			const std::lock_guard< std::mutex > lock{m_staticEntitiesAccess};

			for ( const auto & staticEntity : std::ranges::views::values(m_staticEntities) )
			{
				/* Check whether the static entity contains something to render. */
				if ( !staticEntity->isRenderable() )
				{
					continue;
				}

				const auto & worldCoordinates = staticEntity->getWorldCoordinatesStateForRendering(readStateIndex);

				staticEntity->forEachComponent([&] (const Component::Abstract & component) {
					const auto renderableInstance = component.getRenderableInstance();

					if ( renderableInstance == nullptr )
					{
						return;
					}

					if ( this->checkRenderableInstanceForShadowCasting(renderTarget, renderableInstance) )
					{
						return;
					}

					/* Render-target distance check and frustum culling check.
				 * NOTE: CSM and cubemap shadow maps skip frustum culling because:
				 * - Cubemaps render all 6 faces covering all directions
				 * - CSM uses multiple cascade frustums; objects may be visible in any cascade */
					const auto distance = Vector< 3, float >::distance(cameraPosition, worldCoordinates.position());

					if ( distance > viewDistance || ( !renderTarget->isCubemap() && !renderTarget->isCascadedShadowMap() && !staticEntity->isVisibleTo(frustum) ) )
					{
						return;
					}

					this->insertIntoShadowCastingRenderList(renderableInstance, &worldCoordinates, distance);
				});
			}
		}

		/* Sorting renderable objects from the scene node tree. */
		{
			/* NOTE: Prevent scene node deletion from the logic update thread to crash the rendering. */
			const std::lock_guard< std::mutex > lock{m_sceneNodesAccess};

			NodeCrawler< const Node > crawler{m_rootNode};

			std::shared_ptr< const Node > node;

			while ( (node = crawler.nextNode()) != nullptr )
			{
				/* Check whether the scene node contains something to render. */
				if ( !node->isRenderable() )
				{
					continue;
				}

				const auto & worldCoordinates = node->getWorldCoordinatesStateForRendering(readStateIndex);

				node->forEachComponent([&] (const Component::Abstract & component) {
					const auto renderableInstance = component.getRenderableInstance();

					if ( renderableInstance == nullptr )
					{
						return;
					}

					if ( this->checkRenderableInstanceForShadowCasting(renderTarget, renderableInstance) )
					{
						return;
					}

					/* Render-target distance check and frustum culling check.
				 * NOTE: CSM and cubemap shadow maps skip frustum culling because:
				 * - Cubemaps render all 6 faces covering all directions
				 * - CSM uses multiple cascade frustums; objects may be visible in any cascade */
					const auto distance = Vector< 3, float >::distance(cameraPosition, worldCoordinates.position());

					if ( distance > viewDistance || ( !renderTarget->isCubemap() && !renderTarget->isCascadedShadowMap() && !node->isVisibleTo(frustum) ) )
					{
						return;
					}

					this->insertIntoShadowCastingRenderList(renderableInstance, &worldCoordinates, distance);
				});
			}
		}

		/* Return true if something can be rendered. */
		return !m_renderLists[Shadows].empty();
	}

	void
	Scene::insertIntoShadowCastingRenderList (const std::shared_ptr< RenderableInstance::Abstract > & renderableInstance, const CartesianFrame< float > * worldCoordinates, float distance) noexcept
	{
		/* This is a raw pointer to the renderable interface. */
		const auto * renderable = renderableInstance->renderable();

		if constexpr ( IsDebug )
		{
			if ( renderable == nullptr )
			{
				Tracer::error(ClassId, "The renderable interface pointer is a null !");

				return;
			}

			/* NOTE: Check whether the renderable is ready to draw.
			 * Only done in debug mode because a renderable instance ready to
			 * render implies the renderable is ready to draw. */
			if ( !renderable->isReadyForInstantiation() )
			{
				Tracer::error(ClassId, "The renderable interface is not ready !");

				return;
			}
		}

		const auto layerCount = renderable->layerCount();

		for ( uint32_t layerIndex = 0; layerIndex < layerCount; layerIndex++ )
		{
			RenderBatch::create(m_renderLists[Shadows], distance, renderableInstance, worldCoordinates, layerIndex);
		}
	}

	bool
	Scene::checkRenderableInstanceForRendering (const std::shared_ptr< RenderTarget::Abstract > & renderTarget, const std::shared_ptr< RenderableInstance::Abstract > & renderableInstance) noexcept
	{
		/* Check whether the renderable instance is ready for shadow casting. */
		if ( renderableInstance->isReadyToRender(renderTarget) )
		{
			return false; // Render
		}

		/* If it still unloaded. */
		if ( !renderableInstance->renderable()->isReadyForInstantiation() )
		{
			return true; // Continue
		}

		if ( this->getRenderableInstanceReadyForRendering(renderableInstance, renderTarget) )
		{
			return false; // Render
		}

		/* If the object cannot be loaded, mark it as broken! */
		{
			std::stringstream ss;
			ss << "Unable to get ready the renderable instance (Renderable:" << renderableInstance->renderable()->name() << "') for rendering with render-target '" << renderTarget->id() << "'";

			renderableInstance->setBroken(ss.str());
		}

		return true; // Continue
	}

	bool
	Scene::populateRenderLists (const std::shared_ptr< RenderTarget::Abstract > & renderTarget, uint32_t readStateIndex) noexcept
	{
		/* NOTE: Clean render lists before. */
		m_renderLists[Opaque].clear();
		m_renderLists[Translucent].clear();
		m_renderLists[OpaqueLighted].clear();
		m_renderLists[TranslucentLighted].clear();
		m_renderLists[TranslucentGB].clear();
		m_renderLists[TranslucentGBLighted].clear();

		/* RT render lists: all opaque geometry without frustum culling, distance-only.
		 * Only populated when ray tracing is active on the device. */
		const auto rtEnabled = m_sceneMetaData.isRayTracingEnabled();

		if ( rtEnabled )
		{
			m_rtOpaqueList.clear();
			m_rtOpaqueLightedList.clear();
		}

		/* NOTE: The camera position doesn't move during calculation. */
		const auto & cameraPosition = renderTarget->viewMatrices().position();
		const auto & frustum = renderTarget->viewMatrices().frustum(0);
		const auto viewDistance = renderTarget->viewDistance();

		for ( const auto & component : m_sceneVisualComponents )
		{
			if ( component == nullptr )
			{
				continue;
			}

			const auto renderableInstance = component->getRenderableInstance();

			if ( renderableInstance == nullptr )
			{
				continue;
			}

			if ( this->checkRenderableInstanceForRendering(renderTarget, renderableInstance) )
			{
				continue;
			}

			/* NOTE: Scene visual is the skybox or the ground, frustum culling step is not relevant here. */

			/* RT list: scene visuals (ground) are always included (distance 0). */
			if ( rtEnabled )
			{
				const auto * renderable = renderableInstance->renderable();

				if ( renderable != nullptr && renderable->isOpaque(0) )
				{
					const auto isLighted = m_lightSet.isEnabled() && renderableInstance->isLightingEnabled();
					RenderBatch::create(isLighted ? m_rtOpaqueLightedList : m_rtOpaqueList, 0.0F, renderableInstance, nullptr, 0);
				}
			}

			this->insertIntoRenderLists(renderableInstance, nullptr, 0.0F);
		}

		/* Sorting renderable objects from scene static entities. */
		{
			const std::lock_guard< std::mutex > lock{m_staticEntitiesAccess};

			for ( const auto & staticEntity : std::ranges::views::values(m_staticEntities) )
			{
				/* Check whether the static entity contains something to render. */
				if ( !staticEntity->isRenderable() )
				{
					continue;
				}

				const auto & worldCoordinates = staticEntity->getWorldCoordinatesStateForRendering(readStateIndex);

				staticEntity->forEachComponent([&] (const Component::Abstract & component) {
					const auto renderableInstance = component.getRenderableInstance();

					if ( renderableInstance == nullptr )
					{
						return;
					}

					if ( this->checkRenderableInstanceForRendering(renderTarget, renderableInstance) )
					{
						return;
					}

					const auto distance = Vector< 3, float >::distance(cameraPosition, worldCoordinates.position());

					/* RT list: distance-only culling (no frustum), opaque objects only. */
					if ( rtEnabled && distance <= m_tlasDistance )
					{
						const auto * renderable = renderableInstance->renderable();

						if ( renderable != nullptr && renderable->isOpaque(0) )
						{
							const auto isLighted = m_lightSet.isEnabled() && renderableInstance->isLightingEnabled();
							RenderBatch::create(isLighted ? m_rtOpaqueLightedList : m_rtOpaqueList, distance, renderableInstance, &worldCoordinates, 0);
						}
					}

					/* Raster list: frustum culling + distance check. */
					if ( distance > viewDistance || ( !renderTarget->isCubemap() && !staticEntity->isVisibleTo(frustum) ) )
					{
						return;
					}

					this->insertIntoRenderLists(renderableInstance, &worldCoordinates, distance);
				});
			}
		}

		/* Sorting renderable objects from the scene node tree. */
		{
			/* NOTE: Prevent scene node deletion from the logic update thread to crash the rendering. */
			const std::lock_guard< std::mutex > lock{m_sceneNodesAccess};

			NodeCrawler< const Node > crawler{m_rootNode};

			std::shared_ptr< const Node > node;

			while ( (node = crawler.nextNode()) != nullptr )
			{
				/* Check whether the scene node contains something to render. */
				if ( !node->isRenderable() )
				{
					continue;
				}

				const auto & worldCoordinates = node->getWorldCoordinatesStateForRendering(readStateIndex);

				node->forEachComponent([&] (const Component::Abstract & component) {
					const auto renderableInstance = component.getRenderableInstance();

					if ( renderableInstance == nullptr )
					{
						return;
					}

					if ( this->checkRenderableInstanceForRendering(renderTarget, renderableInstance) )
					{
						return;
					}

					const auto distance = Vector< 3, float >::distance(cameraPosition, worldCoordinates.position());

					/* RT list: distance-only culling (no frustum), opaque objects only. */
					if ( rtEnabled && distance <= m_tlasDistance )
					{
						const auto * renderable = renderableInstance->renderable();

						if ( renderable != nullptr && renderable->isOpaque(0) )
						{
							const auto isLighted = m_lightSet.isEnabled() && renderableInstance->isLightingEnabled();
							RenderBatch::create(isLighted ? m_rtOpaqueLightedList : m_rtOpaqueList, distance, renderableInstance, &worldCoordinates, 0);
						}
					}

					/* Raster list: frustum culling + distance check. */
					if ( distance > viewDistance || ( !renderTarget->isCubemap() && !node->isVisibleTo(frustum) ) )
					{
						return;
					}

					this->insertIntoRenderLists(renderableInstance, &worldCoordinates, distance);
				});
			}
		}

		/* Return true if something can be rendered. */
		constexpr std::array< uint32_t, 6 > objectTypes{Opaque, Translucent, OpaqueLighted, TranslucentLighted, TranslucentGB, TranslucentGBLighted};

		return std::ranges::any_of(objectTypes, [&] (uint32_t objectType) {
			return !m_renderLists[objectType].empty();
		});
	}

	void
	Scene::insertIntoRenderLists (const std::shared_ptr< RenderableInstance::Abstract > & renderableInstance, const CartesianFrame< float > * worldCoordinates, float distance) noexcept
	{
		/* This is a raw pointer to the renderable interface. */
		const auto * renderable = renderableInstance->renderable();

		if constexpr ( IsDebug )
		{
			if ( renderable == nullptr )
			{
				Tracer::error(ClassId, "The renderable interface pointer is a null !");

				return;
			}

			/* NOTE: Check whether the renderable is ready to draw.
			 * Only done in debug mode because a renderable instance ready to
			 * render implies the renderable is ready to draw. */
			if ( !renderable->isReadyForInstantiation() )
			{
				Tracer::error(ClassId, "The renderable interface is not ready !");

				return;
			}
		}

		const auto layerCount = renderable->layerCount();

		for ( uint32_t layerIndex = 0; layerIndex < layerCount; layerIndex++ )
		{
			const auto isOpaque = renderable->isOpaque(layerIndex);
			const auto needsGrabPass = renderable->requiresGrabPass(layerIndex);
			const auto isLighted = m_lightSet.isEnabled() && renderableInstance->isLightingEnabled();

			if ( isOpaque )
			{
				RenderBatch::create(m_renderLists[isLighted ? OpaqueLighted : Opaque], distance, renderableInstance, worldCoordinates, layerIndex);
			}
			else if ( needsGrabPass )
			{
				RenderBatch::create(m_renderLists[isLighted ? TranslucentGBLighted : TranslucentGB], distance * -1.0F, renderableInstance, worldCoordinates, layerIndex);
			}
			else
			{
				RenderBatch::create(m_renderLists[isLighted ? TranslucentLighted : Translucent], distance * -1.0F, renderableInstance, worldCoordinates, layerIndex);
			}
		}
	}

	void
	Scene::renderLightedSelection (const std::shared_ptr< RenderTarget::Abstract > & renderTarget, uint32_t readStateIndex, const Vulkan::CommandBuffer & commandBuffer, const RenderBatch::List & renderBatches, const BindlessTextureManager * bindlessTexturesManager) const noexcept
	{
		if ( m_lightSet.isUsingStaticLighting() )
		{
			for ( const auto & renderBatch : renderBatches | std::views::values )
			{
				renderBatch.renderableInstance()->render(readStateIndex, renderTarget, nullptr, RenderPassType::SimplePass, renderBatch.subGeometryIndex(), renderBatch.worldCoordinates(), commandBuffer, bindlessTexturesManager);
			}

			return;
		}

		/* NOTE: Check global shadow mapping setting from the renderer. */
		const bool shadowMapsEnabled = m_AVConsoleManager.graphicsRenderer().isShadowMapsEnabled();

		/* For all objects. */
		for ( const auto & renderBatch : renderBatches | std::views::values )
		{
			const std::lock_guard< std::mutex > lock{m_lightSet.mutex()};

			/* Ambient pass. */
			renderBatch.renderableInstance()->render(readStateIndex, renderTarget, nullptr, RenderPassType::AmbientPass, renderBatch.subGeometryIndex(), renderBatch.worldCoordinates(), commandBuffer, bindlessTexturesManager);

			/* Loop through all directional lights. */
			for ( const auto & light : m_lightSet.directionalLights() )
			{
				if ( !light->isEnabled() )
				{
					continue;
				}

				const auto & instance = renderBatch.renderableInstance();

				/* NOTE: Use shadow pass type if the light has shadow casting enabled and the instance supports shadows.
				 * CSM uses a specialized pass type for cascaded shadow map sampling.
				 * Also check the global shadow mapping setting from the renderer. */
				const bool useShadow = shadowMapsEnabled && light->isShadowCastingEnabled() && light->hasShadowDescriptorSet() && instance->isShadowReceivingEnabled();
				const bool useColorProjection = light->hasColorProjectionTexture();

				RenderPassType passType;

				if ( useShadow && useColorProjection )
				{
					passType = light->usesCSM() ? RenderPassType::DirectionalLightPassFullCSM : RenderPassType::DirectionalLightPassFull;
				}
				else if ( useShadow )
				{
					passType = light->usesCSM() ? RenderPassType::DirectionalLightPassCSM : RenderPassType::DirectionalLightPassShadowMap;
				}
				else if ( useColorProjection )
				{
					passType = RenderPassType::DirectionalLightPassColorMap;
				}
				else
				{
					passType = RenderPassType::DirectionalLightPass;
				}

				instance->render(readStateIndex, renderTarget, light.get(), passType, renderBatch.subGeometryIndex(), renderBatch.worldCoordinates(), commandBuffer, bindlessTexturesManager);
			}

			/* Loop through all point lights. */
			for ( const auto & light : m_lightSet.pointLights() )
			{
				if ( !light->isEnabled() )
				{
					continue;
				}

				const auto & instance = renderBatch.renderableInstance();

				/* NOTE: If a light distance check is needed. */
				if ( instance->isLightDistanceCheckEnabled() )
				{
					const auto * worldCoordinates = renderBatch.worldCoordinates();

					if ( worldCoordinates != nullptr && !light->touch(worldCoordinates->position()) )
					{
						continue;
					}
				}

				/* NOTE: Select the render pass type based on shadow and color projection state. */
				const bool useShadow = shadowMapsEnabled && light->isShadowCastingEnabled() && light->hasShadowDescriptorSet() && instance->isShadowReceivingEnabled();
				const bool useColorProjection = light->hasColorProjectionTexture();

				RenderPassType passType;

				if ( useShadow && useColorProjection )
				{
					passType = RenderPassType::PointLightPassFull;
				}
				else if ( useShadow )
				{
					passType = RenderPassType::PointLightPassShadowMap;
				}
				else if ( useColorProjection )
				{
					passType = RenderPassType::PointLightPassColorMap;
				}
				else
				{
					passType = RenderPassType::PointLightPass;
				}

				instance->render(readStateIndex, renderTarget, light.get(), passType, renderBatch.subGeometryIndex(), renderBatch.worldCoordinates(), commandBuffer, bindlessTexturesManager);
			}

			/* Loop through all spotlights. */
			for ( const auto & light : m_lightSet.spotLights() )
			{
				if ( !light->isEnabled() )
				{
					continue;
				}

				const auto & instance = renderBatch.renderableInstance();

				/* NOTE: If a light distance check is needed. */
				if ( instance->isLightDistanceCheckEnabled() )
				{
					const auto * worldCoordinates = renderBatch.worldCoordinates();

					if ( worldCoordinates != nullptr && !light->touch(worldCoordinates->position()) )
					{
						continue;
					}
				}

				/* NOTE: Select the render pass type based on shadow and color projection state. */
				const bool useShadow = shadowMapsEnabled && light->isShadowCastingEnabled() && light->hasShadowDescriptorSet() && instance->isShadowReceivingEnabled();
				const bool useColorProjection = light->hasColorProjectionTexture();

				RenderPassType passType;

				if ( useShadow && useColorProjection )
				{
					passType = RenderPassType::SpotLightPassFull;
				}
				else if ( useShadow )
				{
					passType = RenderPassType::SpotLightPassShadowMap;
				}
				else if ( useColorProjection )
				{
					passType = RenderPassType::SpotLightPassColorMap;
				}
				else
				{
					passType = RenderPassType::SpotLightPass;
				}

				renderBatch.renderableInstance()->render(readStateIndex, renderTarget, light.get(), passType, renderBatch.subGeometryIndex(), renderBatch.worldCoordinates(), commandBuffer, bindlessTexturesManager);
			}
		}
	}

	void
	Scene::forEachRenderableInstance (const std::function< bool (const std::shared_ptr< RenderableInstance::Abstract > & renderableInstance) > & function) const noexcept
	{
		for ( const auto & visualComponent : m_sceneVisualComponents )
		{
			if ( visualComponent == nullptr )
			{
				continue;
			}

			const auto renderableInstance = visualComponent->getRenderableInstance();

			if ( renderableInstance == nullptr )
			{
				Tracer::error(ClassId, "The scene visual renderable instance pointer is null !");

				continue;
			}

			function(renderableInstance);
		}

		/* Check renderable objects from scene static entities. */
		{
			const std::lock_guard< std::mutex > lock{m_staticEntitiesAccess};

			for ( const auto & staticEntity : std::ranges::views::values(m_staticEntities) )
			{
				/* Check whether the static entity contains something to render. */
				if ( !staticEntity->isRenderable() )
				{
					continue;
				}

				/* Go through each entity component to update visuals. */
				staticEntity->forEachComponent([&function] (const Component::Abstract & component) {
					const auto renderableInstance = component.getRenderableInstance();

					if ( renderableInstance == nullptr )
					{
						return;
					}

					function(renderableInstance);
				});
			}
		}

		/* Check renderable objects from the scene node tree. */
		{
			/* NOTE: Prevent scene node deletion from the logic update thread to crash the rendering. */
			const std::lock_guard< std::mutex > lock{m_sceneNodesAccess};

			NodeCrawler< const Node > crawler{m_rootNode};

			std::shared_ptr< const Node > node{};

			while ( (node = crawler.nextNode()) != nullptr )
			{
				/* Check whether the scene node contains something to render. */
				if ( !node->isRenderable() )
				{
					continue;
				}

				/* Go through each entity component to update visuals. */
				node->forEachComponent([&function] (const Component::Abstract & component) {
					const auto renderableInstance = component.getRenderableInstance();

					if ( renderableInstance == nullptr )
					{
						return;
					}

					function(renderableInstance);
				});
			}
		}
	}

	void
	Scene::initializeRenderTarget (const std::shared_ptr< RenderTarget::Abstract > & renderTarget) noexcept
	{
		if ( renderTarget->renderType() == RenderTargetType::ShadowMap || renderTarget->renderType() == RenderTargetType::ShadowCubemap )
		{
			TraceDebug{ClassId} << "A new shadow map is available " << to_cstring(renderTarget->renderType()) << " ! Updating renderable instances from the scene ...";

			this->forEachRenderableInstance([this, renderTarget] (const std::shared_ptr< RenderableInstance::Abstract > & renderableInstance) {
				/* NOTE: Skip instances that have shadow casting disabled. */
				if ( renderableInstance->isShadowCastingDisabled() )
				{
					return true;
				}

				if ( !this->getRenderableInstanceReadyForShadowCasting(renderableInstance, renderTarget) )
				{
					TraceError{ClassId} << "The initialization of renderable instance '" << renderableInstance->renderable()->name() << "' from shadow map '" << renderTarget->id() << "' has failed !";
				}

				return true;
			});
		}
		else
		{
			TraceDebug{ClassId} << "A new render target is available " << to_cstring(renderTarget->renderType()) << " ! Updating renderable instances from the scene ...";

			this->forEachRenderableInstance([this, renderTarget] (const std::shared_ptr< RenderableInstance::Abstract > & renderableInstance) {
				if ( !this->getRenderableInstanceReadyForRendering(renderableInstance, renderTarget) )
				{
					TraceError{ClassId} << "The initialization of renderable instance '" << renderableInstance->renderable()->name() << "' from render target '" << renderTarget->id() << "' has failed !";
				}

				return true;
			});
		}
	}

	StaticVector< RenderPassType, MaxPassCount >
	Scene::prepareRenderPassTypes (const RenderableInstance::Abstract & renderableInstance) const noexcept
	{
		const std::lock_guard< std::mutex > lock{m_lightSet.mutex()};

		StaticVector< RenderPassType, MaxPassCount > renderPassTypes;

		if ( !m_lightSet.isEnabled() || !renderableInstance.isLightingEnabled() || m_lightSet.isUsingStaticLighting() )
		{
			renderPassTypes.emplace_back(RenderPassType::SimplePass);
		}
		else
		{
			renderPassTypes.emplace_back(RenderPassType::AmbientPass);

			renderPassTypes.emplace_back(RenderPassType::DirectionalLightPass);
			renderPassTypes.emplace_back(RenderPassType::PointLightPass);
			renderPassTypes.emplace_back(RenderPassType::SpotLightPass);

			/* Color projection pass types. */
			renderPassTypes.emplace_back(RenderPassType::DirectionalLightPassColorMap);
			renderPassTypes.emplace_back(RenderPassType::PointLightPassColorMap);
			renderPassTypes.emplace_back(RenderPassType::SpotLightPassColorMap);

			if ( m_AVConsoleManager.graphicsRenderer().isShadowMapsEnabled() )
			{
				renderPassTypes.emplace_back(RenderPassType::DirectionalLightPassShadowMap);
				renderPassTypes.emplace_back(RenderPassType::PointLightPassShadowMap);
				renderPassTypes.emplace_back(RenderPassType::SpotLightPassShadowMap);

				/* Full pass types (shadow + color projection). */
				renderPassTypes.emplace_back(RenderPassType::DirectionalLightPassFull);
				renderPassTypes.emplace_back(RenderPassType::PointLightPassFull);
				renderPassTypes.emplace_back(RenderPassType::SpotLightPassFull);
			}
		}

		return renderPassTypes;
	}

	bool
	Scene::getRenderableInstanceReadyForShadowCasting (const std::shared_ptr< RenderableInstance::Abstract > & renderableInstance, const std::shared_ptr< RenderTarget::Abstract > & renderTarget) const noexcept
	{
		/* If the object is ready to shadow cast, there is nothing more to do! */
		if ( renderableInstance->isReadyToCastShadows(renderTarget) )
		{
			return true;
		}

		/* A previous try to set up the renderable instance for rendering has failed ... */
		if ( renderableInstance->isBroken() )
		{
			return false;
		}

		return renderableInstance->getReadyForShadowCasting(renderTarget, m_AVConsoleManager.graphicsRenderer());
	}

	bool
	Scene::getRenderableInstanceReadyForRendering (const std::shared_ptr< RenderableInstance::Abstract > & renderableInstance, const std::shared_ptr< RenderTarget::Abstract > & renderTarget) noexcept
	{
		/* The environment cubemap can now be fetched from the visual component. */
		if ( m_environmentCubemap != nullptr && m_sceneVisualComponents[0] != nullptr && renderableInstance == m_sceneVisualComponents[0]->getRenderableInstance() )
		{
			m_environmentCubemap = m_backgroundResource->environmentCubemap();

			/* Update the bindless textures manager with the scene's environment cubemap. */
			const auto & bindlessManager = m_AVConsoleManager.graphicsRenderer().bindlessTextureManager();

			if ( bindlessManager.usable() && bindlessManager.updateTextureCube(BindlessTextureManager::EnvironmentCubemapSlot, *m_environmentCubemap) )
			{
				TraceSuccess{ClassId} << "Scene will use environment cubemap '" << m_environmentCubemap->name() << "' !";
			}
		}

		/* If the object is ready to render, there is nothing more to do! */
		if ( renderableInstance->isReadyToRender(renderTarget) )
		{
			return true;
		}

		/* A previous try to set up the renderable instance for rendering has failed ... */
		if ( renderableInstance->isBroken() )
		{
			return false;
		}

		/* NOTE: Check how many render passes this renderable instance needs. */
		const auto renderPassTypes = this->prepareRenderPassTypes(*renderableInstance);

		if ( renderPassTypes.empty() )
		{
			renderableInstance->setBroken();

			return false;
		}

		return renderableInstance->getReadyForRender(*this, renderTarget, renderPassTypes, m_AVConsoleManager.graphicsRenderer());
	}

	void
	Scene::checkAVConsoleNotification (int notificationCode, const std::any & data) noexcept
	{
		switch ( notificationCode )
		{
			case AVConsole::Manager::VideoDeviceAdded :
				TraceDebug{ClassId} << "A new video device is available for the scene.";
				break;

			case AVConsole::Manager::VideoDeviceRemoved :
			{
				const auto device = std::any_cast< const std::shared_ptr< AVConsole::AbstractVirtualDevice > >(data);

				if ( const auto renderTarget = std::dynamic_pointer_cast< RenderTarget::Abstract >(device) )
				{
					const std::scoped_lock lock{m_renderToShadowMapAccess, m_renderToTextureAccess, m_renderToViewAccess};

					/* NOTE: If the conversion is successful, renderTarget is not null. */
					m_renderToViews.erase(renderTarget);
					m_renderToTextures.erase(renderTarget);
					m_renderToShadowMaps.erase(renderTarget);
				}

				TraceDebug{ClassId} << "A video device has been removed from the scene.";
			}
				break;

			case AVConsole::Manager::AudioDeviceAdded :
				TraceDebug{ClassId} << "A new audio device is available for the scene.";
				break;

			case AVConsole::Manager::AudioDeviceRemoved :
				TraceDebug{ClassId} << "An audio device has been removed from the scene.";
				break;

			case AVConsole::Manager::RenderToShadowMapAdded :
			case AVConsole::Manager::RenderToTextureAdded :
			case AVConsole::Manager::RenderToViewAdded :
				this->initializeRenderTarget(std::any_cast< std::shared_ptr< RenderTarget::Abstract > >(data));
				break;

			default :
				if constexpr ( ObserverDebugEnabled )
				{
					TraceDebug{ClassId} << "Event #" << notificationCode << " from a master control console ignored.";
				}
				break;
		}
	}

}
