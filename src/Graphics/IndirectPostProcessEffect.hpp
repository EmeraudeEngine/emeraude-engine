/*
 * src/Graphics/IndirectPostProcessEffect.hpp
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
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

/* Local inclusions for inheritances. */
#include "PostProcessEffect.hpp"

/* Local inclusions for usages. */
#include "EffectSlot.hpp"

/* Local inclusions for usages. */
#include "Math/Vector.hpp"
#include "PostProcessor.hpp"
#include "StaticVector.hpp"
#include "Vulkan/DescriptorSet.hpp"

namespace EmEn
{
	namespace Vulkan
	{
		class CommandBuffer;
		class DescriptorSetLayout;
		class GraphicsPipeline;
		class PipelineLayout;
		class ShaderModule;
		class TextureInterface;
		class UniformBufferObject;
	}

	namespace Scenes
	{
		class LightSet;
		class ParticipatingMedium;

		namespace Component
		{
			class Camera;
		}
	}

	namespace Graphics
	{
		class IntermediateRenderTarget;
		class Renderer;
	}
}

namespace EmEn::Graphics
{
	/**
	 * @brief Abstract interface for multi-pass post-processing effects.
	 * @note Each effect manages its own intermediate render targets, render passes, and pipelines.
	 * Effects receive an input texture and produce an output texture, forming a chain.
	 * Provides shared infrastructure for fullscreen pass rendering (pipeline creation,
	 * pass recording, vertex shader, descriptor set layouts) to eliminate duplication.
	 * @extends EmEn::Graphics::PostProcessEffect This is a post-process effect.
	 */
	class PostProcessStack;

	class EMEN_API IndirectPostProcessEffect : public PostProcessEffect
	{
		public:

			/**
			 * @brief Copy constructor.
			 * @param copy A reference to the copied instance.
			 */
			IndirectPostProcessEffect (const IndirectPostProcessEffect & copy) noexcept = delete;

			/**
			 * @brief Move constructor.
			 * @param copy A reference to the copied instance.
			 */
			IndirectPostProcessEffect (IndirectPostProcessEffect && copy) noexcept = delete;

			/**
			 * @brief Copy assignment.
			 * @param copy A reference to the copied instance.
			 * @return IndirectPostProcessEffect &
			 */
			IndirectPostProcessEffect & operator= (const IndirectPostProcessEffect & copy) noexcept = delete;

			/**
			 * @brief Move assignment.
			 * @param copy A reference to the copied instance.
			 * @return IndirectPostProcessEffect &
			 */
			IndirectPostProcessEffect & operator= (IndirectPostProcessEffect && copy) noexcept = delete;

			/**
			 * @brief Destructs the indirect post-process effect.
			 */
			~IndirectPostProcessEffect () override = default;

			/**
			 * @brief Returns whether this effect requires a depth buffer input.
			 * @return bool
			 */
			[[nodiscard]]
			virtual
			bool
			requiresDepth () const noexcept
			{
				return false;
			}

			/**
			 * @brief Returns whether this effect requires HDR input data.
			 * @return bool
			 */
			[[nodiscard]]
			virtual
			bool
			requiresHDR () const noexcept
			{
				return false;
			}

			/**
			 * @brief Returns whether this effect requires a normals buffer input.
			 * @return bool
			 */
			[[nodiscard]]
			virtual
			bool
			requiresNormals () const noexcept
			{
				return false;
			}

			/**
			 * @brief Returns whether this effect requires a material properties buffer input.
			 * @return bool
			 */
			[[nodiscard]]
			virtual
			bool
			requiresMaterialProperties () const noexcept
			{
				return false;
			}

			/**
			 * @brief Returns whether this effect requires an albedo buffer input.
			 * @note Requiring albedo implies normals AND material properties: the shader generator
			 * infers the MRT layout from the color attachment count with a fixed order
			 * (color, normals, material properties, albedo).
			 * @return bool
			 */
			[[nodiscard]]
			virtual
			bool
			requiresAlbedo () const noexcept
			{
				return false;
			}

			/**
			 * @brief Returns whether this effect requires a velocity buffer input.
			 * @return bool
			 */
			[[nodiscard]]
			virtual
			bool
			requiresVelocity () const noexcept
			{
				return false;
			}

			/**
			 * @brief Returns whether this effect requires the sub-pixel projection jitter (temporal anti-aliasing).
			 * @note When any effect in the active stack returns true, the Renderer advances a Halton (2,3)
			 * jitter sequence once per rendered frame and applies it to the main view projection
			 * (ViewMatricesInterface::setProjectionJitter()). Implies requiresVelocity() in practice —
			 * a temporal effect without motion vectors would smear.
			 * @return bool
			 */
			[[nodiscard]]
			virtual
			bool
			requiresJitter () const noexcept
			{
				return false;
			}

			/**
			 * @brief Returns whether this effect requires ray tracing (TLAS) to function.
			 * @note Effects returning true are skipped when RT is not available on the device.
			 * @return bool
			 */
			[[nodiscard]]
			virtual
			bool
			requiresRayTracing () const noexcept
			{
				return false;
			}

			/**
			 * @brief Returns whether this effect requires the scene light set to function.
			 * @note Effects returning true are skipped when no main directional light is available.
			 * @return bool
			 */
			[[nodiscard]]
			virtual
			bool
			requiresLightSet () const noexcept
			{
				return false;
			}

			/**
			 * @brief Creates GPU resources for this effect.
			 * @param width The framebuffer width.
			 * @param height The framebuffer height.
			 * @return bool
			 */
			[[nodiscard]]
			virtual bool create (uint32_t width, uint32_t height) noexcept = 0;

			/**
			 * @brief Destroys GPU resources for this effect.
			 * @return void
			 */
			virtual void destroy () noexcept = 0;

			/**
			 * @brief Recreates GPU resources after a resize.
			 * @note Default implementation calls destroy() then create(). Override only if
			 * the effect needs partial recreation (e.g., keeping adaptation state).
			 * @param width The new framebuffer width.
			 * @param height The new framebuffer height.
			 * @return bool
			 */
			[[nodiscard]]
			virtual bool resize (uint32_t width, uint32_t height) noexcept;

			/**
			 * @brief Returns whether this effect must execute AFTER the tone mapping (HDR resolve).
			 * @note Phase contract for the chain ordering: LDR effects (antialiasing, sharpening)
			 * operate on display-referred values and misbehave on linear HDR input (posterization,
			 * halo streaks). The camera-driven photographic effects (DepthOfField, ToneMapping)
			 * are inserted BEFORE the first effect returning true. Default: false (scene/HDR effect).
			 * @return bool
			 */
			[[nodiscard]]
			virtual
			bool
			runsAfterToneMapping () const noexcept
			{
				return this->slot() == EffectSlot::PostToneMapping;
			}

			/**
			 * @brief Returns the CONCEPT this effect implements, hence its place in the chain.
			 * @note ⚠️ PURE VIRTUAL on purpose: an effect that does not state where it belongs
			 * has no defensible place in a chain whose order is a correctness property (an
			 * ambient occlusion applied before the indirect diffuse occludes the wrong term, a
			 * reflection sampled before it reflects an unlit world). An application effect the
			 * engine has no concept for answers EffectSlot::Custom.
			 * @return EffectSlot
			 */
			[[nodiscard]]
			virtual EffectSlot slot () const noexcept = 0;

			/** @copydoc EmEn::Graphics::PostProcessEffect::enable() */
			void enable (bool state) noexcept override;

			/**
			 * @brief Returns whether this effect holds the GPU resources it needs to record.
			 * @note ⚠️ AN EFFECT THAT IS NOT CREATED MUST NEVER BE RECORDED. Its render targets,
			 * pipelines and per-frame descriptor sets are empty containers, and the recording code
			 * indexes them without checking — RTGI's `m_tracePerFrame[frameIndex]` on an EMPTY
			 * vector is an out-of-bounds read, not a null dereference a test would have caught, and
			 * it segfaults on the first rendered frame. It happened twice for two different reasons:
			 * a stack installed without `createAll()` (a demo wiring mistake), and a `resize()`
			 * whose `create()` half failed after the `destroy()` half had already run.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isCreated () const noexcept
			{
				return m_created;
			}

			/**
			 * @brief Records whether the last create/resize attempt succeeded.
			 * @warning ⚠️ INTERNAL — PostProcessStack is the only caller, because it is the only
			 * object that drives the whole lifecycle (createAll, resizeAll, destroyAll, and the
			 * photographic effects it materializes itself).
			 * @param state The new state.
			 * @return void
			 */
			void
			setCreatedFlag (bool state) noexcept
			{
				m_created = state;
			}

			/**
			 * @brief Called by PostProcessStack when the effect enters or leaves a stack.
			 * @warning ⚠️ INTERNAL — the stack is the only caller. A RAW pointer, cleared by the
			 * stack on removal and in its destructor while it still holds the effect alive: an
			 * effect may outlive its stack (a demo keeps shared_ptr copies to toggle it), so it
			 * must never be left pointing at a dead one.
			 * @param stack A pointer to the owning stack, nullptr on removal.
			 * @return void
			 */
			void setOwnerStack (PostProcessStack * stack) noexcept;

			/**
			 * @brief Per-frame context shared by the whole effect chain.
			 * @note Groups the G-buffer inputs, the scene lighting, the ACTIVE CAMERA (single
			 * source of truth for the photographic options — physical camera model) and the
			 * frame push constants. Texture/scene pointers may be null when unavailable.
			 */
			struct EMEN_API FrameContext
			{
				const Vulkan::TextureInterface * depth{nullptr};
				const Vulkan::TextureInterface * normals{nullptr};
				const Vulkan::TextureInterface * materialProperties{nullptr};
				const Vulkan::TextureInterface * albedo{nullptr};
				const Vulkan::TextureInterface * velocity{nullptr};
				const Scenes::LightSet * lightSet{nullptr};
				const Scenes::Component::Camera * camera{nullptr};
				/* Luminance of the scene background, in nits (cd/m²), or 0 when the scene has no
				 * background. THE SKY IS A LIGHT SOURCE: the ray-traced GI turns an escaping ray
				 * into this radiance, which is what fills shadows in daylight (bounces alone
				 * light a scene like the Moon). The cubemap itself needs no plumbing — the
				 * BindlessTextureManager keeps the active scene's environment cubemap in a
				 * reserved slot — so this scalar also acts as the "there is a sky" flag, since
				 * that slot falls back to the engine default cubemap. */
				float skyLuminance{0.0F};
				/* The scene's participating medium — the ONE atmosphere every volumetric consumer
				 * integrates. A POINTER, not a value: this header forward-declares its Scenes
				 * neighbours and includes no Scenes header, and a value would drag one in through
				 * the whole post-process chain.
				 * ⚠️ Null is the "this scene declares no medium" flag, exactly the role skyLuminance
				 * plays for the sky — and it is also what every existing scene gets, since the Scene
				 * default is a vacuum. An effect must behave as it did before when it reads null.
				 * ⚠️ The medium had no owner before Aug 2026: it lived inside AtmosphericFog's
				 * private Parameters while AtmosphericFog was used by ONE demo and VolumetricLight by
				 * EIGHT, so nothing could share it. */
				const Scenes::ParticipatingMedium * medium{nullptr};
				/* Sub-pixel projection jitter of the frame being rendered, in NDC units. Zero when
				 * no effect requires jitter. Needed by the TAA resolve to sample the source at pixel
				 * centers; no history counterpart is exposed because nothing in the chain has to
				 * undo a previous-frame offset (the jitter never travels through a matrix). */
				Base::Math::Vector< 2, float > projectionJitter{};
				PostProcessor::PushConstants constants{};
			};

			/**
			 * @brief Executes the effect for the current frame.
			 * @note Called outside any active render pass. The effect manages its own render passes.
			 * OVERLAY effects (producesOverlay() == true) are never executed through this method:
			 * the PostProcessor drives them via recordOverlayPasses() and folds their application
			 * into a shared combine pass. The default implementation exists only for them and is
			 * a traced passthrough.
			 * @param commandBuffer A reference to the active command buffer.
			 * @param inputColor The input color texture to process.
			 * @param context The per-frame chain context (G-buffers, light set, active camera, constants).
			 * @return const Vulkan::TextureInterface & The output texture to pass to the next effect.
			 */
			[[nodiscard]]
			virtual const Vulkan::TextureInterface & execute (const Vulkan::CommandBuffer & commandBuffer, const Vulkan::TextureInterface & inputColor, const FrameContext & context) noexcept;

			/* ---- Overlay protocol (combine pass) ----
			 * An OVERLAY effect computes its result in its own working targets (trace, blur)
			 * but does NOT apply it to the chain color itself: the PostProcessor gathers the
			 * contiguous overlay effects of the chain and applies them all in ONE generated
			 * full-resolution combine pass, reproducing the exact sequential math while paying
			 * a single full-res read/write instead of one per effect. */

			/**
			 * @brief Returns whether this effect participates in the shared combine pass.
			 * @return bool
			 */
			[[nodiscard]]
			virtual
			bool
			producesOverlay () const noexcept
			{
				return false;
			}

			/**
			 * @brief Returns whether the effect's UPSTREAM passes sample the chain color.
			 * @note The combine group is FLUSHED before such an effect runs, so it reads the
			 * up-to-date color (exact sequential semantics): SSGI gathers radiance from the
			 * chain color, LensFlare thresholds it, SSR builds its pyramid from it. RTGI only
			 * falls back to it when the albedo G-buffer is missing — hence the context.
			 * @param context The per-frame chain context.
			 * @return bool
			 */
			[[nodiscard]]
			virtual
			bool
			readsChainColorUpstream (const FrameContext & /*context*/) const noexcept
			{
				return false;
			}

			/**
			 * @brief Records the effect's internal passes (trace, blur...) WITHOUT the apply.
			 * @note Only meaningful for overlay effects. The input color is the color of the
			 * CURRENT COMBINE GROUP (flushed beforehand when readsChainColorUpstream()).
			 * @param commandBuffer A reference to the active command buffer.
			 * @param inputColor The group input color texture.
			 * @param context The per-frame chain context.
			 * @return void
			 */
			virtual
			void
			recordOverlayPasses (const Vulkan::CommandBuffer & /*commandBuffer*/, const Vulkan::TextureInterface & /*inputColor*/, const FrameContext & /*context*/) noexcept
			{

			}

			/** @brief One texture consumed by an effect's combine snippet. */
			struct EMEN_API CombineSamplerInput
			{
				/** @brief Sampler name suffix: declared in GLSL as sampler2D <prefix><NameSuffix>. */
				const char * nameSuffix{nullptr};
				const Vulkan::TextureInterface * texture{nullptr};
			};

			/**
			 * @brief What an overlay effect contributes to the generated combine shader.
			 * @note The snippet operates on the running color variable `em_Color` (vec4) and
			 * reads `vUV`. Its samplers are declared as `<prefix><NameSuffix>`; the shared
			 * context samplers are `emDepth`, `emNormals`, `emMaterialProps`, `emAlbedo`.
			 * Per-frame scalars land in the combine UBO as `<prefix>Dynamics0/1` (vec4 each).
			 * Every identifier the snippet declares must be prefixed to avoid collisions.
			 */
			struct EMEN_API CombineContribution
			{
				/** @brief Lowercase GLSL identifier prefix, unique per effect type (e.g. "rtao"). */
				const char * prefix{nullptr};
				/** @brief The effect's own textures (blur output, pyramid...). */
				Base::StaticVector< CombineSamplerInput, 4 > samplers;
				/** @brief Shared context samplers required by the snippet. */
				bool needsDepth{false};
				bool needsNormals{false};
				bool needsMaterialProperties{false};
				bool needsAlbedo{false};
				/** @brief Per-frame scalar slots, exposed as vec4 UBO members. */
				Base::StaticVector< Base::Math::Vector< 4, float >, 2 > dynamics;
				/** @brief The GLSL application snippet (main() body fragment). */
				std::string code;
			};

			/**
			 * @brief Returns this frame's combine contribution (textures, dynamics, GLSL snippet).
			 * @note Only meaningful for overlay effects, and only after recordOverlayPasses().
			 * @param context The per-frame chain context.
			 * @return CombineContribution
			 */
			[[nodiscard]]
			virtual
			CombineContribution
			combineContribution (const FrameContext & /*context*/) const noexcept
			{
				return {};
			}

			/* ---- Shared denoise protocol (phase E) ----
			 * An overlay effect whose working chain is "trace → separable blur H → blur V"
			 * can delegate the blur pair to the PostProcessor's shared DenoisePass: the
			 * group's blurs run as TWO multi-render-target passes (one H, one V) instead of
			 * two per effect. Each effect keeps its EXACT kernel through a GLSL snippet. */

			/**
			 * @brief Returns whether this effect delegates its separable blur to the shared denoise pass.
			 * @note When true, the PostProcessor drives the effect through
			 * recordPreDenoisePasses() / denoiseContribution() / recordPostDenoisePasses()
			 * instead of recordOverlayPasses().
			 * @return bool
			 */
			[[nodiscard]]
			virtual
			bool
			usesSharedDenoise () const noexcept
			{
				return false;
			}

			/**
			 * @brief Records the passes PRECEDING the shared blur (trace, pyramid...).
			 * @param commandBuffer A reference to the active command buffer.
			 * @param inputColor The group input color texture.
			 * @param context The per-frame chain context.
			 * @return void
			 */
			virtual
			void
			recordPreDenoisePasses (const Vulkan::CommandBuffer & /*commandBuffer*/, const Vulkan::TextureInterface & /*inputColor*/, const FrameContext & /*context*/) noexcept
			{

			}

			/**
			 * @brief What an effect contributes to the shared denoise pass.
			 * @note The snippet computes `vec4 <prefix>Result` from its source sampler
			 * `<prefix>Src` (the trace output on the H pass, the H target on the V pass),
			 * the pass direction `emDenoiseDir` (vec2, texel-normalized axis), the shared
			 * guides (`emDepth`, `emNormals`, fetched by the snippet as needed) and its
			 * `emDyn.<prefix>Dynamics0` vec4. All declared identifiers must be prefixed.
			 * The H and V targets are EFFECT-OWNED (same extent across the group; formats
			 * may differ) — the blurV target remains what combineContribution() exposes.
			 */
			struct EMEN_API DenoiseContribution
			{
				/** @brief Lowercase GLSL identifier prefix, unique per effect type. */
				const char * prefix{nullptr};
				/** @brief The blur input (trace/extract output). */
				const Vulkan::TextureInterface * source{nullptr};
				/** @brief Horizontal pass output (effect-owned). */
				IntermediateRenderTarget * targetH{nullptr};
				/** @brief Vertical pass output (effect-owned). */
				IntermediateRenderTarget * targetV{nullptr};
				/** @brief Shared context guides required by the snippet. */
				bool needsDepth{false};
				bool needsNormals{false};
				/** @brief Per-frame scalar slot, exposed as a vec4 UBO member. */
				Base::Math::Vector< 4, float > dynamics{};
				/** @brief The GLSL kernel snippet (assigns `vec4 <prefix>Result`). */
				std::string code;
			};

			/**
			 * @brief Returns this frame's denoise contribution.
			 * @note Only meaningful when usesSharedDenoise() is true, after recordPreDenoisePasses().
			 * @param context The per-frame chain context.
			 * @return DenoiseContribution
			 */
			[[nodiscard]]
			virtual
			DenoiseContribution
			denoiseContribution (const FrameContext & /*context*/) const noexcept
			{
				return {};
			}

			/**
			 * @brief Records the passes FOLLOWING the shared blur (temporal resolve, history copies...).
			 * @param commandBuffer A reference to the active command buffer.
			 * @param context The per-frame chain context.
			 * @return void
			 */
			virtual
			void
			recordPostDenoisePasses (const Vulkan::CommandBuffer & /*commandBuffer*/, const FrameContext & /*context*/) noexcept
			{

			}

		protected:

			/**
			 * @brief Constructs an indirect post-process effect.
			 * @param renderer A reference to the graphics renderer.
			 */
			explicit
			IndirectPostProcessEffect (Renderer & renderer) noexcept
				: m_renderer{renderer}
			{

			}

			[[nodiscard]]
			Renderer &
			renderer () noexcept
			{
				return m_renderer;
			}

			[[nodiscard]]
			const Renderer &
			renderer () const noexcept
			{
				return m_renderer;
			}

			/* ---- Shared fullscreen pass infrastructure ---- */

			/**
			 * @brief The fullscreen triangle vertex shader source shared by all effects.
			 * @note Uses gl_VertexIndex to generate a full-screen triangle (3 vertices, no VBO).
			 */
			static constexpr auto FullscreenVertexShaderSource = R"GLSL(
#version 450

layout(location = 0) out vec2 vUV;

void main()
{
	vUV = vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2);
	gl_Position = vec4(vUV * 2.0 - 1.0, 0.0, 1.0);
}
)GLSL";

			/**
			 * @brief Returns the compiled fullscreen vertex shader module (cached by ShaderManager).
			 * @return std::shared_ptr< Vulkan::ShaderModule >
			 */
			[[nodiscard]]
			std::shared_ptr< Vulkan::ShaderModule > getFullscreenVertexShader () const noexcept;

			/**
			 * @brief Creates a standard fullscreen graphics pipeline.
			 * @note Configures: empty vertex input, triangle list, dynamic viewport/scissor,
			 * no culling, no depth test, no blending, single color attachment, RGBA write mask.
			 * @param tracerTag The tracer tag for debug identification.
			 * @param name The pipeline name for debug identification.
			 * @param vertexModule The vertex shader module.
			 * @param fragmentModule The fragment shader module.
			 * @param pipelineLayout The pipeline layout.
			 * @param target The intermediate render target defining the render pass.
			 * @return std::shared_ptr< Vulkan::GraphicsPipeline >
			 */
			[[nodiscard]]
			std::shared_ptr< Vulkan::GraphicsPipeline > createFullscreenPipeline (const char * tracerTag, const std::string & name, const std::shared_ptr< Vulkan::ShaderModule > & vertexModule, const std::shared_ptr< Vulkan::ShaderModule > & fragmentModule, const std::shared_ptr< Vulkan::PipelineLayout > & pipelineLayout, const IntermediateRenderTarget & target) const noexcept;

			/**
			 * @brief Records a fullscreen pass into a command buffer.
			 * @note Performs: beginRenderPass, bind pipeline, set viewport/scissor,
			 * push constants, bind descriptor set, draw(3,1), endRenderPass.
			 * @param commandBuffer A reference to the active command buffer.
			 * @param target The intermediate render target to render into.
			 * @param pipeline The graphics pipeline to use.
			 * @param pipelineLayout The pipeline layout for push constants and descriptor binding.
			 * @param descriptorSet The descriptor set to bind.
			 * @param pushConstants Pointer to the push constants data.
			 * @param pushConstantsSize Size of the push constants data in bytes.
			 * @param bindlessSet The global bindless descriptor set, bound at set 1 when the
			 * pass reads the reserved IBL/environment slots. Default none.
			 * @return void
			 */
			static void recordFullscreenPass (const Vulkan::CommandBuffer & commandBuffer, const IntermediateRenderTarget & target, const Vulkan::GraphicsPipeline & pipeline, const Vulkan::PipelineLayout & pipelineLayout, const Vulkan::DescriptorSet & descriptorSet, const void * pushConstants, uint32_t pushConstantsSize, const Vulkan::DescriptorSet * bindlessSet = nullptr) noexcept;

			/* ---- Shared descriptor set layout helpers ---- */

			/**
			 * @brief Returns a shared descriptor set layout with N combined image samplers and M uniform buffers.
			 * @note Bindings are laid out samplers first: samplers at [0 .. samplerCount-1],
			 * then uniform buffers at [samplerCount .. samplerCount+uniformBufferCount-1].
			 * Effects whose per-frame parameters exceed the 128-byte Vulkan push constant
			 * minimum guarantee (maxPushConstantsSize) MUST use a uniform buffer instead.
			 * @param samplerCount The number of combined image samplers (1, 2, 3, etc.).
			 * @param uniformBufferCount The number of uniform buffers. Default 0 (samplers only).
			 * @return std::shared_ptr< Vulkan::DescriptorSetLayout >
			 */
			[[nodiscard]]
			std::shared_ptr< Vulkan::DescriptorSetLayout > getInputLayout (uint32_t samplerCount, uint32_t uniformBufferCount = 0) const noexcept;

			/**
			 * @brief Allocates per-frame uniform buffers (one per frame-in-flight).
			 * @note Host-visible buffers intended for parameters rewritten every frame;
			 * pair each buffer with the matching per-frame descriptor set. Returns an
			 * empty vector on failure.
			 * @param size The size of one buffer in bytes.
			 * @param classId The class identifier for debug tracing.
			 * @param baseName The base name for buffer identification.
			 * @return std::vector< std::unique_ptr< Vulkan::UniformBufferObject > >
			 */
			[[nodiscard]]
			std::vector< std::unique_ptr< Vulkan::UniformBufferObject > > createPerFrameUniformBuffers (VkDeviceSize size, const char * classId, const std::string & baseName) const noexcept;

			/**
			 * @brief Writes CPU data into a host-visible uniform buffer.
			 * @param uniformBufferObject A reference to the uniform buffer.
			 * @param data Pointer to the source data.
			 * @param size Size of the data in bytes.
			 * @return bool
			 */
			[[nodiscard]]
			static bool updateUniformBufferData (const Vulkan::UniformBufferObject & uniformBufferObject, const void * data, size_t size) noexcept;

			/**
			 * @brief Allocates per-frame descriptor sets (one per frame-in-flight).
			 * @param layout The descriptor set layout.
			 * @param classId The class identifier for debug tracing.
			 * @param baseName The base name for descriptor set identification.
			 * @return std::vector< std::unique_ptr< Vulkan::DescriptorSet > >
			 */
			[[nodiscard]]
			std::vector< std::unique_ptr< Vulkan::DescriptorSet > > createPerFrameDescriptorSets (const std::shared_ptr< Vulkan::DescriptorSetLayout > & layout, const char * classId, const std::string & baseName) const noexcept;

		private:

			Renderer & m_renderer;
			/** @brief The stack holding this effect, nullptr while it belongs to none.
			 * @note ⚠️ RAW, and safe because of who clears it: PostProcessStack clears it on
			 * removal and in its own destructor, at a point where it still owns a shared_ptr to
			 * this effect. An effect outliving its stack (a demo keeping copies to toggle it)
			 * therefore never holds a dangling one. */
			PostProcessStack * m_ownerStack{nullptr};
			/** @brief Whether the last create/resize attempt succeeded — see isCreated(). */
			bool m_created{false};
	};
}
