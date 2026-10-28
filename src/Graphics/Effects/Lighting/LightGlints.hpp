/*
 * src/Graphics/Effects/Lighting/LightGlints.hpp
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
#include <array>
#include <cstdint>
#include <memory>
#include <vector>

/* Local inclusions for inheritances. */
#include "Graphics/IndirectPostProcessEffect.hpp"

/* Local inclusions for usages. */
#include "Graphics/EffectSlot.hpp"
#include "Graphics/IntermediateRenderTarget.hpp"
#include "Vulkan/DescriptorSet.hpp"
#include "Vulkan/ShaderStorageBufferObject.hpp"

/* Forward declarations. */
namespace EmEn::Graphics
{
	class Renderer;
}

namespace EmEn::Scenes
{
	class LightSet;
}

namespace EmEn::Vulkan
{
	class CommandBuffer;
	class DescriptorSetLayout;
	class GraphicsPipeline;
	class PipelineLayout;
}

namespace EmEn::Graphics::Effects::Lighting
{
	/**
	 * @brief The LIGHT GLINTS: the visible source of every point and spot light that declares one
	 * (Scenes::Component::AbstractLightEmitter::setVisibleSourceArea()) drawn as a point of light that carries the
	 * source's light toward the camera, at any distance, without aliasing (graphics doc 43).
	 * @note Why (owner, 2026-10-28): a street lamp's lens, a few square decimetres seen at a grazing angle, is smaller
	 * than a pixel beyond some tens of metres; the rasteriser draws it on one frame and misses it on the next, and the
	 * glare multiplies the blink — a city at night flickered as the camera moved. The glint is the stars' answer
	 * (Effects::Atmosphere::StarField, graphics doc 42): a normalised Gaussian footprint of at least FootprintSigma
	 * pixels whose radiance integrates over the pixels' solid angles to the illuminance the source sends to the camera,
	 * E = I / d². The technique of a game's distant city lights (A. Courrèges, "GTA V — Graphics Study", 2015: the far
	 * lights drawn as sprites).
	 * @note The source: a SPOT's is a flat face looking along the spot's direction (a luminaire's lens), I × cos α toward
	 * the camera (α between that direction and the camera), nothing from behind; a POINT light's is a sphere (a bulb),
	 * I in every direction. The footprint widens to the source's projected size (σ = 0.4 × its side in pixels) when
	 * that is larger, up to MaxFootprintSigma.
	 * @note An OVERLAY effect in EffectSlot::LightGlints, after the temporal anti-aliasing and the stars, drawn with the
	 * UNJITTERED projection, before the camera's optics (the glare spreads a glint). ONE instanced pass at full
	 * resolution, additive. A glint is hidden by the scene in front of it: its centre and four neighbours (one pixel
	 * away) are tested against the depth buffer, their visible share scales it — what hides a lamp hides its glint.
	 * @note SCENE-DRIVEN: filed by PostProcessStack::syncSceneEffects() the first frame a light publishes a visible
	 * source; the light set is read every frame from the lights' PUBLISHED state (render thread), the enabled lights
	 * only. On a frame without a glint it records the clear and adds nothing.
	 * @extends EmEn::Graphics::IndirectPostProcessEffect This is a multi-pass post-process effect.
	 */
	class EMEN_API LightGlints final : public IndirectPostProcessEffect
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"LightGlintsEffect"};

			/** @brief The least standard deviation of a glint's footprint, in pixels: the stars' (a sampled Gaussian that
			 * sums to 1 within 1e-4 wherever it falls inside its pixel — no sub-pixel twinkling). */
			static constexpr auto FootprintSigma{0.7F};

			/** @brief The greatest standard deviation, in pixels: a source seen from up close is a glow of this size at
			 * most (its geometry shows the rest). */
			static constexpr auto MaxFootprintSigma{24.0F};

			/** @brief A glint's footprint sigma per pixel of the source's projected side. */
			static constexpr auto FootprintPerSourcePixel{0.4F};

			/** @brief The half-size of a glint's quad, in footprint sigmas: 3.5σ leaves under 0.1 % of the light outside. */
			static constexpr auto FootprintExtent{3.5F};

			/** @brief The ceiling of a glint's peak radiance in chain units (a half float stops at 65 504). */
			static constexpr auto MaxChainRadiance{60000.0F};

			/** @brief The occlusion test's tolerance: a glint is hidden by a surface nearer than its distance × (1 −
			 * OcclusionRelativeBias) − OcclusionBias (metres) — its own luminaire's lens and head are within it. */
			static constexpr auto OcclusionBias{0.25F};
			static constexpr auto OcclusionRelativeBias{0.005F};

			/** @brief The most glints a frame draws: beyond, the remaining lights are skipped (said once). */
			static constexpr uint32_t MaxGlints{65536};

			/**
			 * @brief A glint as the GPU reads it (std430).
			 */
			struct GPUGlint
			{
				/** @brief xyz: the source's position RELATIVE to the camera, metres (world axes); w: its area, m². */
				std::array< float, 4 > position;
				/** @brief xyz: the direction a spot's face looks along (unit, world axes); w: 1 for a spot, 0 for a point. */
				std::array< float, 4 > axis;
				/** @brief rgb: the light's intensity × its unit-luminance colour, candelas; w: unused. */
				std::array< float, 4 > intensity;
			};

			static_assert(sizeof(GPUGlint) == 48, "A glint is three vec4 in the shader's std430 buffer.");

			/**
			 * @brief The push constants of the glint pass (vertex and fragment stages).
			 */
			struct EMEN_API PushConstants
			{
				/** @brief The unjittered projection × the view ROTATION, column-major: a camera-relative position to clip
				 * space. */
				std::array< float, 16 > viewProjection;
				/** @brief x, y: the target's size in pixels; z, w: the tangents of the half fields of view (1 / P00,
				 * 1 / P11). */
				std::array< float, 4 > viewport;
				/** @brief x: the least footprint sigma (px), y: the quad's half-size in sigmas, z: the greatest sigma
				 * (px), w: the frame's pre-exposure. */
				std::array< float, 4 > footprint;
				/** @brief x: the occlusion bias (m), y: its relative part; z, w: unused. */
				std::array< float, 4 > occlusion;
			};

			static_assert(sizeof(PushConstants) == 112, "The glint pass push constants are a mat4 and three vec4.");
			static_assert(sizeof(PushConstants) <= 128, "Over the 128-byte Vulkan minimum guarantee of maxPushConstantsSize.");

			/**
			 * @brief Constructs the light glints.
			 * @param renderer A reference to the graphics renderer.
			 */
			explicit
			LightGlints (Renderer & renderer) noexcept
				: IndirectPostProcessEffect{renderer}
			{

			}

			/** @copydoc EmEn::Graphics::IndirectPostProcessEffect::slot() */
			[[nodiscard]]
			EffectSlot
			slot () const noexcept override
			{
				return EffectSlot::LightGlints;
			}

			/** @copydoc EmEn::Graphics::PostProcessEffect::label() */
			[[nodiscard]]
			const char *
			label () const noexcept override
			{
				return ClassId;
			}

			/** @copydoc EmEn::Graphics::IndirectPostProcessEffect::create() */
			[[nodiscard]]
			bool create (uint32_t width, uint32_t height) noexcept override;

			/** @copydoc EmEn::Graphics::IndirectPostProcessEffect::destroy() */
			void destroy () noexcept override;

			/** @copydoc EmEn::Graphics::IndirectPostProcessEffect::producesOverlay() */
			[[nodiscard]]
			bool
			producesOverlay () const noexcept override
			{
				return true;
			}

			/** @copydoc EmEn::Graphics::IndirectPostProcessEffect::recordOverlayPasses() */
			void recordOverlayPasses (const Vulkan::CommandBuffer & commandBuffer, const Vulkan::TextureInterface & inputColor, const FrameContext & context) noexcept override;

			/** @copydoc EmEn::Graphics::IndirectPostProcessEffect::combineContribution() */
			[[nodiscard]]
			CombineContribution combineContribution (const FrameContext & context) const noexcept override;

			/** @copydoc EmEn::Graphics::IndirectPostProcessEffect::requiresDepth()
			 * @note The glint pass tests each glint against the depth. */
			[[nodiscard]]
			bool
			requiresDepth () const noexcept override
			{
				return true;
			}

			/** @copydoc EmEn::Graphics::IndirectPostProcessEffect::requiresHDR() */
			[[nodiscard]]
			bool
			requiresHDR () const noexcept override
			{
				return true;
			}

		private:

			/**
			 * @brief Gathers this frame's glints from the light set into m_glints: the enabled point and spot lights that
			 * publish a visible source, camera-relative, at most MaxGlints.
			 * @note ⚠️ RENDER THREAD: the PUBLISHED state of the frame's read slot, under the light set's lock.
			 * @param lightSet A reference to the scene's light set.
			 * @param readStateIndex The render state slot latched by the frame.
			 * @param cameraPosition The camera's world position at that slot.
			 */
			void gatherGlints (const Scenes::LightSet & lightSet, uint32_t readStateIndex, const std::array< float, 3 > & cameraPosition) noexcept;

			/**
			 * @brief Makes sure this frame's glint buffer holds m_glints: grown (to the next power of two, every frame in
			 * flight) when too small — the old buffers retired through the deferred destructor —, then written.
			 * @param frameIndex The frame in flight (its fence waited).
			 * @return bool
			 */
			[[nodiscard]]
			bool uploadGlints (uint32_t frameIndex) noexcept;

			/**
			 * @brief Records the glint pass into the target: the clear, then the glints when there is a set.
			 * @param commandBuffer A reference to the command buffer.
			 * @param descriptorSet A pointer to this frame's written descriptor set, or nullptr: the target is only cleared.
			 * @param pushConstants A reference to the push constants.
			 */
			void recordGlintPass (const Vulkan::CommandBuffer & commandBuffer, const Vulkan::DescriptorSet * descriptorSet, const PushConstants & pushConstants) const noexcept;

			IntermediateRenderTarget m_target;
			/* One per frame in flight, all of m_capacity glints. */
			std::vector< std::unique_ptr< Vulkan::ShaderStorageBufferObject > > m_glintBuffers;
			std::shared_ptr< Vulkan::DescriptorSetLayout > m_inputLayout;
			std::shared_ptr< Vulkan::GraphicsPipeline > m_pipeline;
			std::shared_ptr< Vulkan::PipelineLayout > m_pipelineLayout;
			std::vector< std::unique_ptr< Vulkan::DescriptorSet > > m_perFrame;
			/* This frame's glints (render thread, capacity kept across frames). */
			std::vector< GPUGlint > m_glints;
			uint32_t m_capacity{0};
			/* The MaxGlints overflow was said (render thread). */
			bool m_overflowReported{false};
	};
}
