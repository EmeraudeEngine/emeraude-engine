/*
 * src/Graphics/DeferredLightResolve.hpp
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

#pragma once

/* Project configuration. */
#include "emeraude_export.hpp"

/* STL inclusions. */
#include <atomic>
#include <cstdint>
#include <memory>
#include <span>
#include <vector>

/* Third-party inclusions. */
#include "volk.h"

/* Local inclusions for usages. */
#include "StaticVector.hpp"

namespace EmEn
{
	namespace Vulkan
	{
		class CommandBuffer;
		class ComputePipeline;
		class DescriptorSet;
		class DescriptorSetLayout;
		class Framebuffer;
		class GraphicsPipeline;
		class ImageView;
		class PipelineLayout;
		class RenderPass;
		class Sampler;
		class ShaderStorageBufferObject;
	}

	namespace Scenes
	{
		class LightSet;

		namespace Component
		{
			class AbstractLightEmitter;
		}
	}

	namespace Graphics
	{
		class Renderer;
		class SceneRenderTarget;
		class ViewMatricesInterface;
	}
}

namespace EmEn::Graphics
{
	/**
	 * @brief The deferred resolve of the unshadowed, unprojected point and spot lights: ONE fullscreen pass that shades
	 * them from the G-buffer, where the forward path re-draws the geometry once per light.
	 * @note Owner decision 2026-10-04 (engine docs/todo/mrt-single-pass-deferred.md). Measured on Sponza: 22 lamps cost
	 * 36 ms of forward light passes on an RTX 3070 Ti, ≈ 87 % of it GEOMETRY re-submission.
	 *
	 * The contract, three parties that must agree at every frame:
	 *  - Scenes::LightSet::isDeferredPunctualLight() picks the lights, ONCE per frame, into the snapshot prepare() builds;
	 *  - Scenes::Scene::renderOpaque() skips the forward passes of exactly those lights, for the materials whose
	 *    Material::Interface::deferredLightingEligible() answers true;
	 *  - those materials' ambient programs publish the deferred-lighting bit (DeferredLightingBit) in the material
	 *    properties G-buffer, and this resolve shades only the pixels carrying it.
	 *
	 * The BRDF is the forward light pass's (Saphir/LightGenerator.PBR.cpp, Cook-Torrance: GGX, Smith-Schlick k = (r+1)²/8,
	 * Fresnel-Schlick, kD = (1 - F)(1 - m)) with F0 = mix(0.04, albedo, m) — the eligible materials have IOR 1.5 and a
	 * neutral KHR specular. Inputs: the view-space normal (normals.rgb), the SAA-widened roughness (normals.a, owner
	 * decision: the value the reflections read), the base colour (albedo.rgb), the metalness (1 - albedo.a: the diffuse
	 * weight of a non-transmissive surface) and the position rebuilt from the depth with the frame's JITTERED projection.
	 *
	 * Placement: between the opaque and the translucent halves of the scene pass, which the renderer splits only when the
	 * snapshot is not empty (Renderer: an empty snapshot keeps the single pass, nothing changes). The G-buffer colour
	 * attachments go to SHADER_READ_ONLY and the depth to DEPTH_STENCIL_READ_ONLY around the resolve, then back.
	 *
	 * References: M. Deering et al., "The Triangle Processor and Normal Vector Shader", SIGGRAPH 1988; T. Saito &
	 * T. Takahashi, "Comprehensible Rendering of 3-D Shapes", SIGGRAPH 1990 (the G-buffer); A. Lauritzen, "Deferred
	 * Rendering for Current and Future Rendering Pipelines", SIGGRAPH 2010 course. A per-pixel loop over the snapshot,
	 * rejected by distance first (owner decision); tiled culling (J. Andersson, "DirectX 11 Rendering in Battlefield 3",
	 * GDC 2011) only if it is measured to be needed.
	 *
	 * The frame's selection (2026-10-06, owner decision): prepare() sorts every eligible light into ONE of three sets.
	 *  - INVISIBLE: its reach (a sphere, radius > 0) misses the main camera's frustum. It cannot light a pixel of this
	 *    target — the G-buffer holds only what the frustum sees — so neither the resolve nor the forward path draws it
	 *    (invisibleLights(), skipped by Scene::renderOpaque() for every opaque batch). The light-volume culling of
	 *    deferred shading: S. Hargreaves, "Deferred Shading", GDC 2004; O. Shishkovtsov, "Deferred Shading in
	 *    S.T.A.L.K.E.R.", GPU Gems 2, ch. 9, 2005. An unbounded light (radius 0) is never invisible.
	 *  - RESOLVED: the MaxLights visible lights CLOSEST to the camera, by max(0, distance - radius) (an unbounded light
	 *    ranks by its distance). The first MaxLights in the set's order used to be taken, wherever they were.
	 *  - FORWARD: the visible lights beyond MaxLights, shaded by the forward path as before (the same BRDF).
	 *
	 * TILED culling (2026-10-06, owner decision, asked by 'labyrinth': 370 lamps occluded by walls went forward): a
	 * compute pass splits the frame into TileSize × TileSize tiles, reduces each tile's view-space depth range over the
	 * pixels carrying the deferred-lighting bit, and tests every resolved light's sphere against the tile's four side
	 * planes and that range. The result is ONE BIT per light per tile (LightWordCount words: no per-tile cap, no
	 * overflow); the resolve loops over its tile's bits only, in light order. A lamp behind a wall falls beyond the
	 * tile's depth range and costs nothing there. References: J. Andersson, "DirectX 11 Rendering in Battlefield 3",
	 * GDC 2011; A. Lauritzen, "Deferred Rendering for Current and Future Rendering Pipelines", SIGGRAPH 2010.
	 */
	class EMEN_API DeferredLightResolve final
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"DeferredLightResolve"};

			/** @brief The most lights one frame resolves (the size of the per-frame light buffer): 128 until the tiled
			 * culling (2026-10-06). */
			static constexpr uint32_t MaxLights{1024};

			/** @brief The side of a culling tile, in pixels (the compute workgroup is TileSize × TileSize). */
			static constexpr uint32_t TileSize{16};

			/** @brief The 32-bit words of one tile's light mask: one bit per light. */
			static constexpr uint32_t LightWordCount{MaxLights / 32};

			static_assert(MaxLights % 32 == 0, "The tile mask packs the lights by 32.");
			static_assert(TileSize * TileSize >= LightWordCount, "One invocation clears / writes each mask word.");

			/** @brief The bit an eligible surface sets in the LOW nibble of the material-properties R channel. */
			static constexpr uint32_t DeferredLightingBit{1U};

			/**
			 * @brief Constructs the resolve.
			 * @param renderer A reference to the renderer.
			 */
			explicit DeferredLightResolve (Renderer & renderer) noexcept;

			DeferredLightResolve (const DeferredLightResolve & copy) noexcept = delete;
			DeferredLightResolve (DeferredLightResolve && copy) noexcept = delete;
			DeferredLightResolve & operator= (const DeferredLightResolve & copy) noexcept = delete;
			DeferredLightResolve & operator= (DeferredLightResolve && copy) noexcept = delete;

			/**
			 * @brief Destructs the resolve.
			 */
			~DeferredLightResolve ();

			/**
			 * @brief Takes the frame's snapshot of the deferred lights and uploads them, in view space, to the buffer of the
			 * current frame in flight.
			 * @note Call once per frame, before Scenes::Scene::renderOpaque(), which receives lights(). Makes every GPU
			 * resource the frame's record() needs: when it answers false, nothing is skipped and record() is not called.
			 * @param sceneTarget A reference to the scene render target.
			 * @param lightSet A reference to the scene's light set.
			 * @param readStateIndex The render state slot the frame draws.
			 * @param viewMatrices A reference to the main camera's view matrices.
			 * @param shadowMapsEnabled The renderer's global shadow switch.
			 * @return bool True when at least one light is resolved this frame.
			 */
			[[nodiscard]]
			bool prepare (const SceneRenderTarget & sceneTarget, const Scenes::LightSet & lightSet, uint32_t readStateIndex, const ViewMatricesInterface & viewMatrices, bool shadowMapsEnabled) noexcept;

			/**
			 * @brief Returns the frame's snapshot of the deferred lights, sorted by address (what Scene::renderOpaque() takes).
			 * @return std::span< const Scenes::Component::AbstractLightEmitter * const >
			 */
			[[nodiscard]]
			std::span< const Scenes::Component::AbstractLightEmitter * const >
			lights () const noexcept
			{
				return {m_lights.data(), m_lights.size()};
			}

			/**
			 * @brief Returns the frame's eligible lights whose reach misses the main camera's frustum, sorted by address.
			 * @note Valid for the target prepare() was given, whatever prepare() answered: Scene::renderOpaque() skips
			 * their forward passes for every opaque batch of that target. Empty before the first prepare() of a frame.
			 * @return std::span< const Scenes::Component::AbstractLightEmitter * const >
			 */
			[[nodiscard]]
			std::span< const Scenes::Component::AbstractLightEmitter * const >
			invisibleLights () const noexcept
			{
				return {m_invisibleLights.data(), m_invisibleLights.size()};
			}

			/** @brief What the last prepare() did with the eligible lights (a console snapshot; the counts are read apart). */
			struct Statistics
			{
				/** @brief The eligible lights (unshadowed, unprojected, enabled point and spot lights). */
				uint32_t eligible{0};
				/** @brief Those whose reach missed the frustum: drawn by nobody. */
				uint32_t invisible{0};
				/** @brief Those the resolve shades. */
				uint32_t resolved{0};
				/** @brief The visible ones beyond MaxLights, left to the forward passes. */
				uint32_t forward{0};
			};

			/**
			 * @brief Switches the tile culling on or off (the exactness A/B: off, every tile holds every resolved light,
			 * which is the per-pixel loop of before; the two frames must be bit-identical).
			 * @note Thread-safe, read when the next frame is recorded.
			 * @param state The state.
			 */
			void
			enableTileCulling (bool state) noexcept
			{
				m_tileCullingEnabled.store(state, std::memory_order_relaxed);
			}

			/**
			 * @brief Returns whether the tile culling is on.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isTileCullingEnabled () const noexcept
			{
				return m_tileCullingEnabled.load(std::memory_order_relaxed);
			}

			/**
			 * @brief Returns what the last prepare() did with the eligible lights.
			 * @note Thread-safe (written by the render thread, read by the console). Zeros before the first prepare(),
			 * and the last values stay while the renderer does not call prepare() (resolve switched off, no scene).
			 * @return Statistics
			 */
			[[nodiscard]]
			Statistics
			statistics () const noexcept
			{
				return {
					.eligible = m_statisticEligible.load(std::memory_order_relaxed),
					.invisible = m_statisticInvisible.load(std::memory_order_relaxed),
					.resolved = m_statisticResolved.load(std::memory_order_relaxed),
					.forward = m_statisticForward.load(std::memory_order_relaxed)
				};
			}

			/**
			 * @brief Records the resolve, OUTSIDE any render pass, after the opaque half of the scene pass.
			 * @note Leaves every G-buffer attachment back in its attachment layout, ready for the translucent half.
			 * @param commandBuffer A reference to the command buffer.
			 * @param sceneTarget A reference to the scene render target, the one prepare() was given this frame.
			 * @param readStateIndex The render state slot the frame draws.
			 * @param viewMatrices A reference to the main camera's view matrices.
			 * @return bool False when nothing was recorded (no light this frame).
			 */
			[[nodiscard]]
			bool record (const Vulkan::CommandBuffer & commandBuffer, const SceneRenderTarget & sceneTarget, uint32_t readStateIndex, const ViewMatricesInterface & viewMatrices) noexcept;

			/**
			 * @brief Returns whether a scene target carries every attachment the resolve reads.
			 * @param sceneTarget A reference to the scene render target.
			 * @return bool
			 */
			[[nodiscard]]
			static bool hasGeometryBuffer (const SceneRenderTarget & sceneTarget) noexcept;

			/**
			 * @brief Releases every GPU resource (renderer termination, scene target recreation).
			 */
			void destroy () noexcept;

		private:

			/** @brief A visible eligible light competing for the resolve. */
			struct Candidate
			{
				/** @brief The light (its published block carries everything the buffer needs). */
				const Scenes::Component::AbstractLightEmitter * light{nullptr};
				/** @brief max(0, distance to the camera - radius), the distance for an unbounded light. */
				float rank{0.0F};
				/** @brief The position in the light set's walk (points, then spots): the tie-break and the buffer order. */
				uint32_t order{0};
				/** @brief True for a spot light, false for a point light. */
				bool isSpot{false};
			};

			/**
			 * @brief Creates the sampler, the descriptor set layout, the per-frame sets and buffers and the pipeline layout.
			 * @return bool
			 */
			[[nodiscard]]
			bool createSharedResources () noexcept;

			/**
			 * @brief (Re)creates the render pass, the framebuffer and the pipeline for the scene target's colour image.
			 * @param sceneTarget A reference to the scene render target.
			 * @return bool
			 */
			[[nodiscard]]
			bool createTargetResources (const SceneRenderTarget & sceneTarget) noexcept;

			Renderer & m_renderer;
			std::shared_ptr< Vulkan::Sampler > m_sampler;
			std::shared_ptr< Vulkan::DescriptorSetLayout > m_descriptorSetLayout;
			std::vector< std::unique_ptr< Vulkan::DescriptorSet > > m_descriptorSets;
			std::vector< std::unique_ptr< Vulkan::ShaderStorageBufferObject > > m_lightBuffers;
			std::shared_ptr< Vulkan::PipelineLayout > m_pipelineLayout;
			std::shared_ptr< Vulkan::RenderPass > m_renderPass;
			std::shared_ptr< Vulkan::Framebuffer > m_framebuffer;
			std::shared_ptr< Vulkan::GraphicsPipeline > m_pipeline;
			/** @brief The tile culling pass (target independent). */
			std::unique_ptr< Vulkan::ComputePipeline > m_cullPipeline;
			/** @brief One tile-mask buffer per frame in flight, sized for the scene target (device local). */
			std::vector< std::unique_ptr< Vulkan::ShaderStorageBufferObject > > m_tileMaskBuffers;
			/** @brief The frame's resolved lights, sorted by address (render thread, MaxLights reserved once: 8 KiB, over
			 * the StaticVector bound of Allocatus Reduxus). */
			std::vector< const Scenes::Component::AbstractLightEmitter * > m_lights;
			/** @brief The frame's invisible lights, sorted by address (render thread only, capacity kept across frames). */
			std::vector< const Scenes::Component::AbstractLightEmitter * > m_invisibleLights;
			/** @brief The frame's visible eligible lights before the selection (render thread only, capacity kept). */
			std::vector< Candidate > m_candidates;
			/** @brief The scene colour view the framebuffer was made for: a recreated scene target brings a new one (a weak
			 * reference: an expired one can never compare equal to its successor, whatever address it reuses). */
			std::weak_ptr< Vulkan::ImageView > m_targetColorView;
			std::atomic< uint32_t > m_statisticEligible{0};
			std::atomic< uint32_t > m_statisticInvisible{0};
			std::atomic< uint32_t > m_statisticResolved{0};
			std::atomic< uint32_t > m_statisticForward{0};
			std::atomic< bool > m_tileCullingEnabled{true};
			uint32_t m_tileCountX{0};
			uint32_t m_tileCountY{0};
			bool m_sharedResourcesCreated{false};
	};
}
