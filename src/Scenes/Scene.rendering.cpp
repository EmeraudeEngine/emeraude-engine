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
 * https://github.com/EmeraudeEngine/emeraude-engine
 *
 * --- THIS IS AUTOMATICALLY GENERATED, DO NOT CHANGE ---
 */

#include "Scene.hpp"

/* STL inclusions. */
#include <ranges>

/* Local inclusions. */
#include "Graphics/BindlessTextureManager.hpp"
#include "Graphics/MDI/BatchBuilder.hpp"
#include "Graphics/Renderable/Abstract.hpp"
#include "Graphics/RenderableInstance/RenderStateTracker.hpp"
#include "Graphics/Renderer.hpp"
#include "Graphics/Renderable/Types.hpp"
#include "NodeCrawler.hpp"
#include "Vulkan/TextureInterface.hpp"

namespace EmEn::Scenes
{
	using namespace Base;
	using namespace Base::Math;
	using namespace Graphics;

	std::shared_ptr< RenderTarget::ShadowMap< ViewMatrices2DUBO > >
	Scene::createRenderToShadowMap (const std::string & name, uint32_t resolution, float viewDistance, bool isOrthographicProjection) noexcept
	{
		const std::scoped_lock lock{m_renderToShadowMapAccess};

		if ( m_AVConsoleManager.isVideoDeviceExists(name) )
		{
			TraceError{ClassId} <<
				"A virtual video device named '" << name << "' already exists ! "
				"Render to shadow map creation canceled ...";

			return {};
		}

		/* Create the render target. */
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
		const std::scoped_lock lock{m_renderToShadowMapAccess};

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
		const std::scoped_lock lock{m_renderToShadowMapAccess};

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
		const std::scoped_lock lock{m_renderToTextureAccess};

		if ( m_AVConsoleManager.isVideoDeviceExists(name) )
		{
			TraceError{ClassId} <<
				"A virtual video device named '" << name << "' already exists ! "
				"Render to texture 2D creation canceled ...";

			return {};
		}

		/* Create the render target. */
		auto renderTarget = std::make_shared< RenderTarget::Texture< ViewMatrices2DUBO > >(name, width, height, colorCount, viewDistance, isOrthographicProjection);

		/* Historical behavior: re-rendered every frame. Callers wanting a one-shot or
		 * on-demand target flip it via setAutomaticRenderingState(false) + setRenderOutOfDate(). */
		renderTarget->setAutomaticRenderingState(true);

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
	Scene::createRenderToCubemap (const std::string & name, uint32_t size, uint32_t colorCount, float viewDistance, bool isOrthographicProjection, uint32_t colorBits) noexcept
	{
		const std::scoped_lock lock{m_renderToTextureAccess};

		if ( m_AVConsoleManager.isVideoDeviceExists(name) )
		{
			TraceError{ClassId} <<
				"A virtual video device named '" << name << "' already exists ! "
				"Render to cubemap creation canceled ...";

			return {};
		}

		/* Create the render target. */
		auto renderTarget = std::make_shared< RenderTarget::Texture< ViewMatrices3DUBO > >(name, size, colorCount, viewDistance, isOrthographicProjection, colorBits);

		/* An environment probe: continuous by default (callers flip to "once" via
		 * setAutomaticRenderingState(false) + setRenderOutOfDate()), and SUSPENDED while an
		 * enabled scene-reflection provider (SSR/RTR) covers the same job — see the
		 * reflection cost ladder in docs/reflection-pipeline.md. */
		renderTarget->setAutomaticRenderingState(true);
		renderTarget->setSuspendableByPostProcessReflections(true);

		/* GGX-prefiltered mip chain: mip 0 = native mirror render, upper mips convolved
		 * after every render so rough materials get a physically blurred reflection
		 * (textureLod by roughness — the sky IBL chain semantics). */
		renderTarget->enableGGXConvolution();

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
		const std::scoped_lock lock{m_renderToViewAccess};

		if ( m_AVConsoleManager.isVideoDeviceExists(name) )
		{
			TraceError{ClassId} << "A virtual device named '" << name << "' already exists ! Render to view creation canceled ...";

			return {};
		}

		/* Create the render target. */
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
		const std::scoped_lock lock{m_renderToViewAccess};

		/* Checks name availability. */
		if ( m_AVConsoleManager.isVideoDeviceExists(name) )
		{
			TraceError{ClassId} << "A virtual device named '" << name << "' already exists ! Render to cubic view creation canceled ...";

			return {};
		}

		/* Create the render target. */
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
	Scene::signalOnDemandRenderTargets () const noexcept
	{
		/* ⚠️ DEFERRED: this fires from getRenderableInstanceReadyForRendering(), i.e. INSIDE
		 * the Renderer's render-to-textures loop which already holds the render target list
		 * mutex — walking the lists here re-locks it on the render thread (self-deadlock,
		 * black screen, lived). The flag is consumed by beginRenderFrame(), outside any lock. */
		m_onDemandRefreshPending.store(true, std::memory_order_release);
	}

	void
	Scene::updateVideoMemory (bool shadowMapEnabled, bool renderToTextureEnabled) const noexcept
	{
		const uint32_t readStateIndex = m_frameReadStateIndex;

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

		if ( !m_lightSet.updateVideoMemory(readStateIndex, m_AVConsoleManager.graphicsRenderer().currentFrameIndex()) )
		{
			Tracer::error(ClassId, "Unable to update the light set data to the video memory !");
		}
	}

	void
	Scene::castShadows (const std::shared_ptr< RenderTarget::Abstract > & renderTarget, const Vulkan::CommandBuffer & commandBuffer) noexcept
	{
		const uint32_t readStateIndex = m_frameReadStateIndex;

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
			renderBatch.renderableInstance()->castShadows(readStateIndex, renderTarget, renderBatch.subGeometryIndex(), renderBatch.worldCoordinates(), commandBuffer, renderBatch.LODLevel());
		}
	}

	void
	Scene::beginRenderFrame () noexcept
	{
		/* Deferred on-demand refresh (owner contract): consumed here, before any render
		 * target loop holds its lock. A no-op on automatic (continuous) targets. */
		if ( m_onDemandRefreshPending.exchange(false, std::memory_order_acq_rel) )
		{
			const auto flagTarget = [] (const std::shared_ptr< Graphics::RenderTarget::Abstract > & renderTarget) {
				renderTarget->setRenderOutOfDate();
			};

			this->forEachRenderToTexture(flagTarget);
			this->forEachRenderToView(flagTarget);
		}

		m_instanceTransforms.beginFrame(m_AVConsoleManager.graphicsRenderer().currentFrameIndex());

		/* ⚠️⚠️ [ONE FRAME, ONE TRUTH] The frame latches the published state ONCE, here, and every
		 * consumer below reads the latch. It used to be loaded independently at four sites spread
		 * across the frame — the UBO upload, the shadow pass, the main pass and the TBN debug pass —
		 * separated by the fence wait and by acquireNextImage(). The logic thread publishes at its
		 * own 60 Hz cadence, so a frame straddling a publish rasterised the scene into the shadow
		 * map with tick N and into the colour buffer with tick N+1: a one-tick shadow/geometry
		 * desync that re-rolled every frame, exactly zero on a static camera and random in motion.
		 * This is only EXPRESSIBLE because updateVideoMemory() now runs after beginRenderFrame();
		 * it used to be called from Core before this function, so a latch set here would have been
		 * read before it was written. */
		m_frameReadStateIndex = m_renderStateIndex.load(std::memory_order_acquire);
	}

	bool
	Scene::prepareRender (const std::shared_ptr< RenderTarget::Abstract > & renderTarget) noexcept
	{
		m_preparedReadStateIndex = m_frameReadStateIndex;

		const auto & bindlessManager = m_AVConsoleManager.graphicsRenderer().bindlessTextureManager();

		m_preparedBindlessManager = bindlessManager.usable() ? &bindlessManager : nullptr;

		/* Cache the instance transforms descriptor set for the current frame-in-flight,
		 * consumed by the render(step) calls recorded until the next prepareRender(). */
		const auto frameIndex = m_AVConsoleManager.graphicsRenderer().currentFrameIndex();

		m_preparedInstanceTransformsDS = m_instanceTransforms.descriptorSet(frameIndex);

		const bool renderListsPopulated = this->populateRenderLists(renderTarget, m_preparedReadStateIndex);

		/* Instance transforms header: current and previous view-projection matrices of the
		 * primary view target (motion vectors). Render-to-texture/cubemap targets must not
		 * write it — they are prepared BEFORE the main view, but the gate keeps the header
		 * semantics independent from the renderer's target ordering. */
		if ( renderTarget->renderType() == RenderTargetType::View )
		{
			const auto & viewMatrices = renderTarget->viewMatrices();

			/* NOTE: UNJITTERED matrices — the velocity clip positions are built from this header
			 * and must stay jitter-free (the TAA sub-pixel offset is a per-draw push constant
			 * applied to gl_Position only).
			 * NOTE: The infinity variant serves the renderables rendered with the translation-free
			 * view (the sky background): their current clip position comes from the pushed INFINITY
			 * view, so their previous one must too, otherwise the velocity is off by the whole
			 * camera translation even on a static camera. */
			const auto & previousProjection = viewMatrices.previousProjectionMatrix();

			m_instanceTransforms.setPreviousViewProjectionMatrices(
				previousProjection * viewMatrices.previousViewMatrix(),
				previousProjection * viewMatrices.previousInfinityViewMatrix()
			);
		}

		/* Upload the staged instance transforms (header + frame-linear entries) to the
		 * current frame-in-flight SSBO. Cumulative across the prepareRender() calls of the
		 * frame, so entries staged for earlier targets (render-to-textures) stay valid. */
		if ( !m_instanceTransforms.updateVideoMemory() )
		{
			Tracer::error(ClassId, "Unable to update the instance transforms SSBO to the video memory !");
		}

		if ( !renderListsPopulated )
		{
			return false;
		}

		/* Rebuild the TLAS and RT metadata from RT-specific render lists (no frustum culling).
		 * RT effects cast rays in world space and need ALL scene geometry, not just what's on screen. */
		auto * mutableBindlessSet = bindlessManager.usable() ? &m_bindlessTextureSet : nullptr;
		m_sceneMetaData.rebuild(m_RTOpaqueList, m_RTOpaqueLightedList, mutableBindlessSet, frameIndex, renderTarget->viewMatrices().position());

		return true;
	}

	void
	Scene::renderOpaque (const std::shared_ptr< RenderTarget::Abstract > & renderTarget, const Vulkan::CommandBuffer & commandBuffer) noexcept
	{
		if ( !m_renderLists[Opaque].empty() )
		{
			auto * mdiBatchBuilder = m_AVConsoleManager.graphicsRenderer().MDIBatchBuilder();

			if ( mdiBatchBuilder != nullptr && mdiBatchBuilder->isReady() && m_renderLists[Opaque].size() > 1 )
			{
				const auto currentFrame = m_AVConsoleManager.graphicsRenderer().currentFrameIndex();
				mdiBatchBuilder->buildBatches(m_renderLists[Opaque], currentFrame, m_preparedReadStateIndex);
				mdiBatchBuilder->dispatch(renderTarget, commandBuffer, currentFrame, m_preparedReadStateIndex, m_preparedBindlessManager);

				/* Render objects skipped by MDI (sprites, InfinityView, adaptive LOD). */
				if ( mdiBatchBuilder->skippedCount() > 0 )
				{
					RenderableInstance::RenderStateTracker tracker{};

					for ( const auto & renderBatch : m_renderLists[Opaque] | std::views::values )
					{
						const auto * renderable = renderBatch.renderableInstance()->renderable();

						if ( renderable == nullptr )
						{
							continue;
						}

						const auto * geometry = renderable->geometry(0);

						const bool isSkipped = renderable->isSprite()
							|| renderBatch.renderableInstance()->isUsingInfinityView()
							|| (geometry != nullptr && geometry->isAdaptiveLOD());

						if ( isSkipped )
						{
							renderBatch.renderableInstance()->render(m_preparedReadStateIndex, renderTarget, nullptr, RenderPassType::SimplePass, renderBatch.subGeometryIndex(), renderBatch.worldCoordinates(), commandBuffer, tracker, renderBatch.LODLevel(), m_preparedBindlessManager, m_preparedInstanceTransformsDS);
						}
					}
				}
			}
			else
			{
				/* No MDI: Phase 1A tracked render for all objects. */
				RenderableInstance::RenderStateTracker tracker{};

				for ( const auto & renderBatch : m_renderLists[Opaque] | std::views::values )
				{
					renderBatch.renderableInstance()->render(m_preparedReadStateIndex, renderTarget, nullptr, RenderPassType::SimplePass, renderBatch.subGeometryIndex(), renderBatch.worldCoordinates(), commandBuffer, tracker, renderBatch.LODLevel(), m_preparedBindlessManager, m_preparedInstanceTransformsDS);
				}
			}
		}

		if ( m_lightSet.isEnabled() && !m_renderLists[OpaqueLighted].empty() )
		{
			this->renderLightedSelection(renderTarget, m_preparedReadStateIndex, commandBuffer, m_renderLists[OpaqueLighted], m_preparedBindlessManager, m_preparedInstanceTransformsDS);
		}
	}

	void
	Scene::renderTranslucent (const std::shared_ptr< RenderTarget::Abstract > & renderTarget, const Vulkan::CommandBuffer & commandBuffer) noexcept
	{
		if ( !m_renderLists[Translucent].empty() )
		{
			for ( const auto & renderBatch : m_renderLists[Translucent] | std::views::values )
			{
				renderBatch.renderableInstance()->render(m_preparedReadStateIndex, renderTarget, nullptr, RenderPassType::SimplePass, renderBatch.subGeometryIndex(), renderBatch.worldCoordinates(), commandBuffer, renderBatch.LODLevel(), m_preparedBindlessManager, m_preparedInstanceTransformsDS);
			}
		}

		if ( m_lightSet.isEnabled() && !m_renderLists[TranslucentLighted].empty() )
		{
			this->renderLightedSelection(renderTarget, m_preparedReadStateIndex, commandBuffer, m_renderLists[TranslucentLighted], m_preparedBindlessManager, m_preparedInstanceTransformsDS);
		}
	}

	void
	Scene::renderTranslucentGB (const std::shared_ptr< RenderTarget::Abstract > & renderTarget, const Vulkan::CommandBuffer & commandBuffer) noexcept
	{
		if ( !m_renderLists[TranslucentGB].empty() )
		{
			for ( const auto & renderBatch : m_renderLists[TranslucentGB] | std::views::values )
			{
				renderBatch.renderableInstance()->render(m_preparedReadStateIndex, renderTarget, nullptr, RenderPassType::SimplePass, renderBatch.subGeometryIndex(), renderBatch.worldCoordinates(), commandBuffer, renderBatch.LODLevel(), m_preparedBindlessManager, m_preparedInstanceTransformsDS);
			}
		}

		if ( m_lightSet.isEnabled() && !m_renderLists[TranslucentGBLighted].empty() )
		{
			this->renderLightedSelection(renderTarget, m_preparedReadStateIndex, commandBuffer, m_renderLists[TranslucentGBLighted], m_preparedBindlessManager, m_preparedInstanceTransformsDS);
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
		const uint32_t readStateIndex = m_frameReadStateIndex;

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
			const std::scoped_lock lock{m_staticEntitiesAccess};

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
			const std::scoped_lock lock{m_sceneNodesAccess};

			NodeCrawler< const Node > crawler{m_rootNode};

			while ( crawler.fetchNextNode() )
			{
				const auto & currentNode = crawler.currentNode();

				if ( !currentNode->isRenderable() )
				{
					continue;
				}

				const auto & worldCoordinates = currentNode->getWorldCoordinatesStateForRendering(readStateIndex);

				currentNode->forEachComponent([&] (const Component::Abstract & component) {
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
		const uint32_t nextTarget = m_renderStateIndex == 0U ? 1U : 0U;

		/* Synchronize static entities. */
		for ( const auto & staticEntity : std::ranges::views::values(m_staticEntities) )
		{
			staticEntity->publishStateForRendering(nextTarget);
		}

		/* Synchronize scene nodes. */
		{
			NodeCrawler< Node > crawler{m_rootNode};

			while ( crawler.fetchNextNode() )
			{
				crawler.currentNode()->publishStateForRendering(nextTarget);
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

	[[nodiscard]]
	uint32_t
	Scene::selectLODLevel (float distance, float objectRadius) const noexcept
	{
		if ( m_currentViewDistance <= 0.0F || distance <= 0.0F || objectRadius <= 0.0F )
		{
			return 0U;
		}

		/* Screen-space coverage: how large the object appears relative to the viewport.
		 * screenSize ∝ objectRadius / distance. When screenSize is large, use LOD 0.
		 * As screenSize shrinks, increase LOD level.
		 *
		 * LODLevel = clamp(MaxLODLevels - screenSize / threshold, 0, MaxLODLevels-1)
		 * where threshold defines the coverage at which LOD 0 transitions to LOD 1. */
		const auto screenSize = objectRadius / distance;
		const auto LODf = static_cast< float >(Renderable::MaxLODLevels) * (static_cast< float >(1) - (screenSize / m_LODScreenCoverageThreshold));

		if ( LODf <= 0.0F )
		{
			return 0U;
		}

		return std::min(static_cast< uint32_t >(LODf), Renderable::MaxLODLevels - 1);
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

		/* ⚠️ [DOUBLE-BUFFERING] Published render state, like the main render list. Harmless today for
		 * a static light (a classic shadow target's own frame is written only when the light moves,
		 * and a CSM target short-circuits both tests below), but the no-argument overloads are the
		 * LIVE logic state and have no business being read from the render thread. */
		const auto & cameraPosition = renderTarget->viewMatrices().position(readStateIndex);
		const auto & frustum = renderTarget->viewMatrices().frustum(readStateIndex, 0);
		const auto viewDistance = renderTarget->viewDistance();

		/* ⚠️⚠️ A CSM shadow map has NO meaningful camera position, so the distance test below must
		 * not be applied to it — it emptied the map completely and was the third independent reason
		 * a CSM light produced no shadow at all.
		 * For every other target the "camera" is the light itself and the test is sound. A CSM
		 * target inherits its coordinates from the light ENTITY (DirectionalLight::move() only
		 * re-anchors the classic map), while its cascades are fitted to the MAIN CAMERA's frustum:
		 * the two have nothing to do with each other. Measured on reflexion-debug: the sun entity
		 * sits at (457, 762, 457), i.e. 999 m from the scene, against a viewDistance of 500 m
		 * derived from the camera — so EVERY caster failed `distance > viewDistance` and the map
		 * was rendered empty, frame after frame.
		 * TODO(perf): this now walks every caster in the scene for a CSM target. The right filter is
		 * the union of the per-cascade frustums, which viewMatrices().frustum(index) already
		 * exposes; it is a pure optimisation and belongs in its own change, measured on a scene
		 * dense enough to show it (reflexion-debug is not). */
		const auto isCascaded = renderTarget->isCascadedShadowMap();

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
			const std::scoped_lock lock{m_staticEntitiesAccess};

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

					if ( ( !isCascaded && distance > viewDistance ) || ( !renderTarget->isCubemap() && !isCascaded && !staticEntity->isVisibleTo(frustum) ) )
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
			const std::scoped_lock lock{m_sceneNodesAccess};



			NodeCrawler< const Node > crawler{m_rootNode};

			while ( crawler.fetchNextNode() )
			{
				const auto & currentNode = crawler.currentNode();

				/* Check whether the scene node contains something to render. */
				if ( !currentNode->isRenderable() )
				{
					continue;
				}

				const auto & worldCoordinates = currentNode->getWorldCoordinatesStateForRendering(readStateIndex);

				currentNode->forEachComponent([&] (const Component::Abstract & component) {
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

					if ( ( !isCascaded && distance > viewDistance ) || ( !renderTarget->isCubemap() && !isCascaded && !currentNode->isVisibleTo(frustum) ) )
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

		/* ⚠️ The LOD here is chosen from the distance to the LIGHT, not to the camera: `distance` is
		 * measured against the SHADOW TARGET's position by the caller (see populateShadowCastingRenderList).
		 * Shadow LOD and colour LOD are therefore selected from different distances BY CONSTRUCTION —
		 * this comment used to claim the opposite ("same as main rendering"), which it never was.
		 * ⚠️ selectLODLevel() also reads m_currentViewDistance, which populateRenderLists() writes and
		 * which the shadow pass reads BEFORE it on the first frame of a scene (Renderer::renderFrame
		 * runs renderShadowMaps() before prepareRender()): the very first shadow list of a scene is
		 * built against the previous scene's leftover. It only guards a `> 0` test today. */
		const auto objectRadius = renderable->boundingSphere().radius() * renderable->uniformScale();
		const auto LODLevel = this->selectLODLevel(distance, objectRadius);
		const auto layerCount = renderable->layerCount();

		for ( uint32_t layerIndex = 0; layerIndex < layerCount; layerIndex++ )
		{
			RenderBatch::create(m_renderLists[Shadows], distance, renderableInstance, worldCoordinates, layerIndex, LODLevel);
		}
	}

	bool
	Scene::checkRenderableInstanceForRendering (const std::shared_ptr< RenderTarget::Abstract > & renderTarget, const std::shared_ptr< RenderableInstance::Abstract > & renderableInstance) noexcept
	{
		/* Excluded from THIS target (probe self-inclusion fix): a reflective subject must not
		 * be rendered into its own probe — inception feedback otherwise. */
		if ( renderTarget->isExcludedFromRendering(renderableInstance.get()) )
		{
			return true; // Continue
		}

		/* AUTO-exclusion (probe self-sampling): an instance whose material SAMPLES the render
		 * target being populated must never be rendered into it, whether or not the caller
		 * registered it in the manual exclusion list above. Sampling an image that is
		 * simultaneously the pass's color attachment is undefined behavior everywhere and a
		 * hard GPU fault on Apple Silicon: measured on basic-scenery, the feedback draw
		 * triggered a GPU recovery that discarded every in-flight command buffer
		 * (kIOGPUCommandBufferCallbackErrorInnocentVictim) and lost the device. Only texture
		 * render targets can be sampled: the renderType() test keeps the cost off the main
		 * view/shadow hot paths, the dynamic_cast resolves the TextureInterface subobject. */
		if ( const auto renderTargetType = renderTarget->renderType(); renderTargetType == RenderTargetType::Texture || renderTargetType == RenderTargetType::Cubemap )
		{
			if ( const auto * targetTexture = dynamic_cast< const Vulkan::TextureInterface * >(renderTarget.get()) )
			{
				const auto & renderable = *renderableInstance->renderable();
				const auto layerCount = renderable.layerCount();

				for ( uint32_t layerIndex = 0; layerIndex < layerCount; ++layerIndex )
				{
					const auto * material = renderable.material(layerIndex);

					if ( material != nullptr && material->samplesTexture(targetTexture) )
					{
						return true; // Continue
					}
				}
			}
		}

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
		const auto RTEnabled = m_sceneMetaData.isRayTracingEnabled();

		if ( RTEnabled )
		{
			m_RTOpaqueList.clear();
			m_RTOpaqueLightedList.clear();
		}

		/* ⚠️ [DOUBLE-BUFFERING] These MUST come from the published render state, never from the
		 * no-argument overloads: those serve the LIVE logic state, which the logic thread rewrites
		 * every tick (ViewMatrices2DUBO::updateViewCoordinates() → m_logicState.frustum.update()).
		 * Reading them here culled entities in and out of the frame on a torn frustum — visible ONLY
		 * while the camera moved, because a parked camera rewrites byte-identical data. That is the
		 * very artefact the two-state system exists to remove, reintroduced at the culling site.
		 * The comment this replaced ("the camera position doesn't move during calculation") was true
		 * of the render thread and false across the thread boundary. */
		const auto & cameraPosition = renderTarget->viewMatrices().position(readStateIndex);
		const auto & frustum = renderTarget->viewMatrices().frustum(readStateIndex, 0);
		const auto viewDistance = renderTarget->viewDistance();

		/* NOTE: Only the primary view target advances the per-instance model matrix history
		 * (motion vectors) — once per rendered frame, render-to-textures excluded. */
		const bool advanceModelHistory = renderTarget->renderType() == RenderTargetType::View;

		/* Store view distance for LOD computation in insertIntoRenderLists(). */
		m_currentViewDistance = viewDistance;

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

			/* RT list: scene visuals (ground) are always included (distance 0).
			 * ONE batch per renderable — per-sub-geometry materials are resolved in the
			 * RT trace shader via materialIndices[geometryIndex] (multi-geometry BLAS).
			 * The renderable is included if ANY of its layers is opaque or alpha-test.
			 *
			 * ⚠️ EXCEPT THE BACKGROUND. The sky is a backdrop, not geometry: it is drawn as a
			 * huge mesh enclosing the scene, so putting it in the TLAS walls the world in and
			 * NO ray can ever escape. Every ray-traced effect reads a miss as "the sky is
			 * visible in that direction" — the GI turns it into sky light, the reflections into
			 * an environment sample — and an enclosing shell silently turns that into "blocked"
			 * everywhere. Measured before this exclusion (Sponza, gallery): the ray-outcome
			 * visualization was ENTIRELY red, i.e. every single ray hit the skybox shell beyond
			 * the bounce range and contributed nothing, which is why shadows were pitch black. */
			if ( RTEnabled && component != m_sceneVisualComponents[BackgroundVisualIndex] )
			{
				const auto * renderable = renderableInstance->renderable();

				if ( renderable != nullptr )
				{
					const auto layerCount = renderable->layerCount();
					bool rtVisible = false;

					for ( uint32_t layer = 0; layer < layerCount; ++layer )
					{
						const auto * layerMaterial = renderable->material(layer);

						if ( layerMaterial != nullptr && (layerMaterial->isOpaque() || layerMaterial->isAlphaTest()) )
						{
							rtVisible = true;
							break;
						}
					}

					if ( rtVisible )
					{
						const auto isLighted = m_lightSet.isEnabled() && renderableInstance->isLightingEnabled();

						RenderBatch::create(
							isLighted ? m_RTOpaqueLightedList : m_RTOpaqueList,
							0.0F,
							renderableInstance,
							nullptr,
							0
						);
					}
				}
			}

			this->insertIntoRenderLists(renderableInstance, nullptr, 0.0F, cameraPosition, advanceModelHistory);
		}

		/* Sorting renderable objects from scene static entities. */
		{
			const std::scoped_lock lock{m_staticEntitiesAccess};

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

					/* RT list: ONE batch per renderable. Per-sub-geometry materials are
					 * looked up by the RT trace shader via materialIndices[geometryIndex]
					 * (multi-geometry BLAS). Distance-only culling, no frustum. */
					if ( RTEnabled && distance <= m_TLASDistance )
					{
						const auto * renderable = renderableInstance->renderable();

						if ( renderable != nullptr )
						{
							const auto layerCount = renderable->layerCount();
							bool RTVisible = false;

							for ( uint32_t layer = 0; layer < layerCount; ++layer )
							{
								const auto * layerMaterial = renderable->material(layer);

								if ( layerMaterial != nullptr && (layerMaterial->isOpaque() || layerMaterial->isAlphaTest()) )
								{
									RTVisible = true;

									break;
								}
							}

							if ( RTVisible )
							{
								const auto isLighted = m_lightSet.isEnabled() && renderableInstance->isLightingEnabled();

								RenderBatch::create(
									isLighted ? m_RTOpaqueLightedList : m_RTOpaqueList,
									distance,
									renderableInstance,
									&worldCoordinates,
									0
								);
							}
						}
					}

					/* Raster list: frustum culling + distance check. */
					if ( distance > viewDistance || ( !renderTarget->isCubemap() && !staticEntity->isVisibleTo(frustum) ) )
					{
						return;
					}

					this->insertIntoRenderLists(renderableInstance, &worldCoordinates, distance, cameraPosition, advanceModelHistory);
				});
			}
		}

		/* Sorting renderable objects from the scene node tree. */
		{
			/* NOTE: Prevent scene node deletion from the logic update thread to crash the rendering. */
			const std::scoped_lock lock{m_sceneNodesAccess};

			NodeCrawler< const Node > crawler{m_rootNode};

			while ( crawler.fetchNextNode() )
			{
				const auto & currentNode = crawler.currentNode();

				/* Check whether the scene node contains something to render. */
				if ( !currentNode->isRenderable() )
				{
					continue;
				}

				const auto & worldCoordinates = currentNode->getWorldCoordinatesStateForRendering(readStateIndex);

				currentNode->forEachComponent([&] (const Component::Abstract & component) {
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

					/* RT list: ONE batch per renderable. Per-sub-geometry materials are
					 * looked up by the RT trace shader via materialIndices[geometryIndex]. */
					if ( RTEnabled && distance <= m_TLASDistance )
					{
						const auto * renderable = renderableInstance->renderable();

						if ( renderable != nullptr )
						{
							const auto layerCount = renderable->layerCount();
							bool rtVisible = false;

							for ( uint32_t layer = 0; layer < layerCount; ++layer )
							{
								const auto * layerMaterial = renderable->material(layer);

								if ( layerMaterial != nullptr && (layerMaterial->isOpaque() || layerMaterial->isAlphaTest()) )
								{
									rtVisible = true;
									break;
								}
							}

							if ( rtVisible )
							{
								const auto isLighted = m_lightSet.isEnabled() && renderableInstance->isLightingEnabled();
								auto & rtList = isLighted ? m_RTOpaqueLightedList : m_RTOpaqueList;
								RenderBatch::create(rtList, distance, renderableInstance, &worldCoordinates, 0);
							}
						}
					}

					/* Raster list: frustum culling + distance check.
					 * Sprites skip frustum culling: their bounding volume is a flat quad (Z=0)
					 * that doesn't account for billboard rotation done in the vertex shader. */
					if ( distance > viewDistance )
					{
						return;
					}

					const bool isBillboardSprite = renderableInstance->renderable() != nullptr && renderableInstance->renderable()->isSprite();

					if ( !isBillboardSprite && !renderTarget->isCubemap() && !currentNode->isVisibleTo(frustum) )
					{
						return;
					}

					this->insertIntoRenderLists(renderableInstance, &worldCoordinates, distance, cameraPosition, advanceModelHistory);
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
	Scene::insertIntoRenderLists (const std::shared_ptr< RenderableInstance::Abstract > & renderableInstance, const CartesianFrame< float > * worldCoordinates, float distance, const Vector< 3, float > & cameraPosition, bool advanceModelHistory) noexcept
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

		/* Stage the instance transforms SSBO entry (non-instanced path only; instanced
		 * renderables carry their model matrices in a VBO). The instance retains its
		 * frame-linear slot for the draws recorded until the next prepareRender(). */
		if ( !renderableInstance->useModelVertexBufferObject() )
		{
			renderableInstance->stageInstanceTransforms(m_instanceTransforms, worldCoordinates, cameraPosition, advanceModelHistory);
		}

		/* Compute LOD level from screen-space coverage (distance + object size).
		 * LOD 0 = full detail (large on screen), MaxLODLevels-1 = minimum detail (small on screen). */
		const auto objectRadius = renderable->boundingSphere().radius() * renderable->uniformScale();
		const auto LODLevel = this->selectLODLevel(distance, objectRadius);

		const auto layerCount = renderable->layerCount();

		for ( uint32_t layerIndex = 0; layerIndex < layerCount; layerIndex++ )
		{
			const auto isOpaque = renderable->isOpaque(layerIndex);
			const auto needsGrabPass = renderable->requiresGrabPass(layerIndex);
			const auto isLighted = m_lightSet.isEnabled() && renderableInstance->isLightingEnabled();

			if ( isOpaque )
			{
				/* Objects with special rendering flags are order-dependent and must keep distance sorting.
				 * State-sorted key only for standard opaques without special depth/display flags. */
				const bool isSpecial = renderable->isSprite()
					|| renderableInstance->isDepthTestDisabled()
					|| renderableInstance->isDepthWriteDisabled()
					|| renderableInstance->isUsingInfinityView();

				if ( isSpecial )
				{
					RenderBatch::create(m_renderLists[isLighted ? OpaqueLighted : Opaque], distance, renderableInstance, worldCoordinates, layerIndex, LODLevel);
				}
				else
				{
					RenderBatch::createStateSorted(m_renderLists[isLighted ? OpaqueLighted : Opaque], distance, renderableInstance, worldCoordinates, layerIndex, LODLevel);
				}
			}
			else if ( needsGrabPass )
			{
				RenderBatch::create(m_renderLists[isLighted ? TranslucentGBLighted : TranslucentGB], distance * -1.0F, renderableInstance, worldCoordinates, layerIndex, LODLevel);
			}
			else
			{
				RenderBatch::create(m_renderLists[isLighted ? TranslucentLighted : Translucent], distance * -1.0F, renderableInstance, worldCoordinates, layerIndex, LODLevel);
			}
		}
	}

	void
	Scene::renderLightedSelection (const std::shared_ptr< RenderTarget::Abstract > & renderTarget, uint32_t readStateIndex, const Vulkan::CommandBuffer & commandBuffer, const RenderBatch::List & renderBatches, const BindlessTextureManager * bindlessTexturesManager, const Vulkan::DescriptorSet * sceneTransformsDS) const noexcept
	{
		/* State tracker for redundant bind elimination (lighted list is state-sorted). */
		RenderableInstance::RenderStateTracker tracker{};

		/* NOTE: Check global shadow mapping setting from the renderer. */
		const bool shadowMapsEnabled = m_AVConsoleManager.graphicsRenderer().isShadowMapsEnabled();

		/* For all objects. */
		for ( const auto & renderBatch : renderBatches | std::views::values )
		{
			const std::scoped_lock lock{m_lightSet.mutex()};

			/* Ambient pass. */
			renderBatch.renderableInstance()->render(readStateIndex, renderTarget, nullptr, RenderPassType::AmbientPass, renderBatch.subGeometryIndex(), renderBatch.worldCoordinates(), commandBuffer, tracker, renderBatch.LODLevel(), bindlessTexturesManager, sceneTransformsDS);

			/* Instance world bounding sphere, computed once for the per-light culling
			 * below. Lights are tested against the whole instance VOLUME, not just its
			 * center point: a large instance (e.g. a ground) must be lit by any light
			 * whose range overlaps it, not only by lights reaching its exact center. */
			const auto * batchCoordinates = renderBatch.worldCoordinates();

			Space3D::Sphere< float > instanceWorldSphere;

			if ( batchCoordinates != nullptr )
			{
				const auto & localSphere = renderBatch.renderableInstance()->renderable()->boundingSphere();
				const auto & scale = batchCoordinates->scalingFactor();
				const auto worldRadius = localSphere.radius() * std::max({scale[0], scale[1], scale[2]});

				instanceWorldSphere = Base::Math::Space3D::Sphere< float >{worldRadius, batchCoordinates->position()};
			}

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

				auto passType = RenderPassType::None;

				if ( useShadow && useColorProjection )
				{
					/* NOTE: A cascaded light keeps its shadow and drops the projection. The two
					 * are mutually exclusive by contract (docs/shadow-mapping.md): the light-space
					 * position is resolved per cascade in the fragment shader and cannot address a
					 * single projection texture, and the CSM light block declares no projection
					 * member. Asking for both used to name a pass whose shader cannot compile,
					 * which broke the whole renderable instance — shadow is the load-bearing half. */
					passType = light->usesCSM() ? RenderPassType::DirectionalLightPassCSM : RenderPassType::DirectionalLightPassFull;
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

				instance->render(readStateIndex, renderTarget, light.get(), passType, renderBatch.subGeometryIndex(), renderBatch.worldCoordinates(), commandBuffer, tracker, renderBatch.LODLevel(), bindlessTexturesManager, sceneTransformsDS);
			}

			/* Loop through all point lights. */
			for ( const auto & light : m_lightSet.pointLights() )
			{
				if ( !light->isEnabled() )
				{
					continue;
				}

				const auto & instance = renderBatch.renderableInstance();

				/* NOTE: If a light distance check is needed. Test against the instance
				 * world bounding sphere (sphere-vs-sphere), not just its center point. */
				if ( instance->isLightDistanceCheckEnabled() && batchCoordinates != nullptr && !light->touch(instanceWorldSphere) )
				{
					continue;
				}

				/* NOTE: Select the render pass type based on shadow and color projection state. */
				const bool useShadow = shadowMapsEnabled && light->isShadowCastingEnabled() && light->hasShadowDescriptorSet() && instance->isShadowReceivingEnabled();
				const bool useColorProjection = light->hasColorProjectionTexture();

				auto passType = RenderPassType::None;

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

				instance->render(readStateIndex, renderTarget, light.get(), passType, renderBatch.subGeometryIndex(), renderBatch.worldCoordinates(), commandBuffer, tracker, renderBatch.LODLevel(), bindlessTexturesManager, sceneTransformsDS);
			}

			/* Loop through all spotlights. */
			for ( const auto & light : m_lightSet.spotLights() )
			{
				if ( !light->isEnabled() )
				{
					continue;
				}

				const auto & instance = renderBatch.renderableInstance();

				/* NOTE: If a light distance check is needed. Test against the instance
				 * world bounding sphere (sphere-vs-sphere), not just its center point. */
				if ( instance->isLightDistanceCheckEnabled() && batchCoordinates != nullptr && !light->touch(instanceWorldSphere) )
				{
					continue;
				}

				/* NOTE: Select the render pass type based on shadow and color projection state. */
				const bool useShadow = shadowMapsEnabled && light->isShadowCastingEnabled() && light->hasShadowDescriptorSet() && instance->isShadowReceivingEnabled();
				const bool useColorProjection = light->hasColorProjectionTexture();

				auto passType = RenderPassType::None;

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

				renderBatch.renderableInstance()->render(readStateIndex, renderTarget, light.get(), passType, renderBatch.subGeometryIndex(), renderBatch.worldCoordinates(), commandBuffer, tracker, renderBatch.LODLevel(), bindlessTexturesManager, sceneTransformsDS);
			}
		}

/*#ifdef DEBUG
		if ( tracker.totalDrawCalls > 1 )
		{
			TraceInfo{ClassId} << "[MDI-1A] Lighted: " << tracker.totalDrawCalls << " draws, saved "
				<< tracker.savedPipelineBinds << " pipeline, "
				<< tracker.savedGeometryBinds << " geometry, "
				<< tracker.savedMaterialBinds << " material binds";
		}
#endif*/
	}

	void
	Scene::forEachRenderableInstance (const std::function< void (const std::shared_ptr< RenderableInstance::Abstract > & renderableInstance) > & function) const noexcept
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
			const std::scoped_lock lock{m_staticEntitiesAccess};

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
			const std::scoped_lock lock{m_sceneNodesAccess};

			NodeCrawler< const Node > crawler{m_rootNode};

			while ( crawler.fetchNextNode() )
			{
				const auto & currentNode = crawler.currentNode();

				/* Check whether the scene node contains something to render. */
				if ( !currentNode->isRenderable() )
				{
					continue;
				}

				/* Go through each entity component to update visuals. */
				currentNode->forEachComponent([&function] (const Component::Abstract & component) {
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
		const std::scoped_lock lock{m_lightSet.mutex()};

		StaticVector< RenderPassType, MaxPassCount > renderPassTypes;

		if ( !m_lightSet.isEnabled() || !renderableInstance.isLightingEnabled() )
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

				/* Cascaded variant: renderLightedSelection() picks it the moment a directional
				 * light was built with the CSM constructor, so the program must exist even
				 * though no light declares CSM at this point — the light set is not what this
				 * list is keyed on.
				 * NOTE: DirectionalLightPassFullCSM is deliberately absent. CSM and colour
				 * projection are mutually exclusive by contract (a per-cascade light-space
				 * position cannot address one projection texture, cf. docs/shadow-mapping.md),
				 * and the CSM light block carries no colour-projection member — generating it
				 * only produces a shader that cannot compile. */
				renderPassTypes.emplace_back(RenderPassType::DirectionalLightPassCSM);

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

			/* Describe the scene's environment cubemap in the bindless set; the manager writes
			 * it to the reserved slot when it syncs the active scene's set. */
			m_bindlessTextureSet.setEnvironmentCubemap(m_environmentCubemap);

			TraceSuccess{ClassId} << "Scene will use environment cubemap '" << m_environmentCubemap->name() << "' !";
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

		if ( !renderableInstance->getReadyForRender(*this, renderTarget, renderPassTypes, m_AVConsoleManager.graphicsRenderer()) )
		{
			return false;
		}

		/* CONTENT APPEARED (owner contract for on-demand render targets): this instance just
		 * became renderable — an async load materialized, the scene visibly changed. Every
		 * on-demand target (a "once" probe) is flagged for a re-bake; the app signals its own
		 * specific changes (movements) through setRenderOutOfDate() itself. */
		this->signalOnDemandRenderTargets();

		/* Generate MDI shader variants for standard opaque non-lighted objects when MDI is enabled.
		 * Sprites, InfinityView, and other special objects are excluded — they need per-object
		 * push constant handling that's incompatible with the MDI push constant layout. */
		if ( m_AVConsoleManager.graphicsRenderer().isMDIEnabled()
			&& !renderableInstance->isLightingEnabled()
			&& renderableInstance->renderable() != nullptr
			&& !renderableInstance->renderable()->isSprite()
			&& !renderableInstance->isUsingInfinityView()
			&& !renderableInstance->isDepthTestDisabled()
			&& !renderableInstance->isDepthWriteDisabled() )
		{
			static_cast< void >(renderableInstance->getReadyForMDI(*this, renderTarget, m_AVConsoleManager.graphicsRenderer()));
		}

		return true;
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
