/*
 * src/Graphics/Effects/Framebuffer/MotionBlur.hpp
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
#include <vector>

/* Local inclusions for inheritances. */
#include "Graphics/IndirectPostProcessEffect.hpp"

/* Local inclusions for usages. */
#include "Graphics/IntermediateRenderTarget.hpp"

namespace EmEn::Graphics::Effects::Framebuffer
{
	/**
	 * @brief Camera and per-object motion blur, reconstructed from the velocity G-buffer.
	 * @note Implements McGuire et al., "A Reconstruction Filter for Plausible Motion Blur"
	 * (I3D 2012) in three passes: a TileMax pass reducing the velocity buffer to the dominant
	 * velocity of each KxK tile, a NeighborMax pass spreading each tile's dominant velocity to
	 * its 3x3 tile neighbourhood (so a fast object can blur BEYOND its own tile), and a
	 * full-resolution gather that walks the dominant velocity with jittered samples, weighting
	 * each by a soft depth classification (foreground/background) — the silhouette weighting
	 * follows Jimenez, "Next Generation Post Processing in Call of Duty: Advanced Warfare"
	 * (SIGGRAPH 2014), since uniform weights along the path break on hard edges.
	 * @note The blur LENGTH is photographic, not a strength slider: the active camera's shutter
	 * speed divided by the frame duration gives the SHUTTER ANGLE, i.e. the fraction of the frame
	 * the shutter stays open. 1/48 s at 24 fps is the cinematic 180-degree rule. This is what
	 * makes the result independent of the framerate.
	 * @extends EmEn::Graphics::IndirectPostProcessEffect This is a multi-pass post-process effect.
	 */
	class EMEN_API MotionBlur final : public IndirectPostProcessEffect
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"MotionBlurEffect"};

			/** @copydoc EmEn::Graphics::PostProcessEffect::label() */
			[[nodiscard]]
			const char *
			label () const noexcept override
			{
				return ClassId;
			}

			/**
			 * @brief Tile size, in pixels, of the velocity reduction (McGuire's K).
			 * @note Also the MAXIMUM blur radius: a velocity longer than one tile would reach
			 * pixels whose tile never saw it, so the reduction clamps to this. 64 px is ~2% of a
			 * 2880-wide frame; Unreal's default ceiling is 5% of the screen width, and a 90 deg/s
			 * pan at a 60 deg field of view already asks for ~72 px. A 16 px ceiling (the 1080p-era
			 * default) made the blur invisible at this resolution.
			 * @note The reduction is SEPARABLE (horizontal then vertical) precisely so this can be
			 * large: a single KxK pass would loop 4096 times in one fragment invocation, against
			 * 64 + 64 for the two passes — the same total number of fetches, but no serialized
			 * mega-loop killing occupancy.
			 */
			static constexpr uint32_t TileSize{64};

			/**
			 * @brief User-facing motion blur parameters.
			 * @note The blur length is NOT here — it comes from the camera's shutter speed.
			 */
			struct EMEN_API Parameters
			{
				/** @brief Samples walked along the dominant velocity (odd is better: one lands on the
				 * centre). Must scale with the ceiling above: 15 taps spread over a 64 px smear
				 * leave 4 px between samples, which reads as discrete ghosts rather than a streak. */
				uint32_t sampleCount{24};
				/** @brief Depth interval, in meters, over which the foreground/background classification softens. */
				float softDepthExtent{0.05F};
				/** @brief Below this dominant velocity (in pixels, AFTER the shutter scale) the
				 * gather is skipped entirely — a smear shorter than half a pixel is not worth 24
				 * texture samples. */
				float minVelocityPixels{0.5F};
				/** @brief Dead zone on the RAW per-frame velocity, in pixels, applied BEFORE the
				 * shutter scale: rejects the velocity buffer's floating-point noise floor, which a
				 * shutter angle above 1 would otherwise amplify past any post-scale gate. */
				float deadZonePixels{0.25F};
			};

			/**
			 * @brief Push constants of the two tile passes.
			 */
			struct EMEN_API TilePushConstants
			{
				float velocityTexelSizeX;
				float velocityTexelSizeY;
				float tileSize;
				/** @brief shutterSpeed / frameTime (may exceed 1 — see the gather constants).
				 * Applied HERE, before the reduction: the dominant velocity of a tile must be the
				 * one the gather will actually walk, or the tile classification and the walk
				 * disagree. */
				float shutterAngle;
				float maxBlurRadiusPixels;
				/** @brief Dead zone on the RAW per-frame velocity, in pixels, applied BEFORE the
				 * shutter scale. The velocity G-buffer has a floating-point noise floor of ~1e-4 px
				 * because the current and previous clip positions come from two differently
				 * computed matrix products — mathematically equal, not bit-equal. A shutter angle
				 * above 1 AMPLIFIES that floor, so gating after the scale is not enough: at 500 fps
				 * with a 1/60 s exposure the noise was multiplied by 8.3, crossed a post-scale
				 * gate, and the 64 px tiles spread it over the whole frame — a static camera
				 * shimmered everywhere. Sub-quarter-pixel motion per frame is below the raster's
				 * own precision anyway. */
				float deadZonePixels;
			};

			/**
			 * @brief Push constants of the gather pass.
			 */
			struct EMEN_API GatherPushConstants
			{
				float texelSizeX;
				float texelSizeY;
				float frameWidth;
				float frameHeight;
				float nearPlane;
				float farPlane;
				/** @brief shutterSpeed / frameTime — how many FRAMES of motion the exposure covers.
				 * Greater than 1 is normal and wanted (a 1/60 s exposure at 500 fps covers 8.3
				 * frames); the velocity is extrapolated linearly over that span, which is the
				 * standard approximation. Same value the tile passes used. */
				float shutterAngle;
				float maxBlurRadiusPixels;
				float softDepthExtent;
				float minVelocityPixels;
				float time;
				uint32_t sampleCount;
			};

			/**
			 * @brief Constructs a motion blur effect.
			 * @param renderer A reference to the graphics renderer.
			 */
			explicit
			MotionBlur (Renderer & renderer) noexcept
				: IndirectPostProcessEffect{renderer}
			{

			}

			/**
			 * @brief Constructs a motion blur effect.
			 * @param renderer A reference to the graphics renderer.
			 * @param parameters The initial parameters.
			 */
			MotionBlur (Renderer & renderer, const Parameters & parameters) noexcept
				: IndirectPostProcessEffect{renderer},
				m_parameters{parameters}
			{

			}

			/** @copydoc EmEn::Graphics::IndirectPostProcessEffect::create() */
			[[nodiscard]]
			bool create (uint32_t width, uint32_t height) noexcept override;

			/** @copydoc EmEn::Graphics::IndirectPostProcessEffect::destroy() */
			void destroy () noexcept override;

			/** @copydoc EmEn::Graphics::IndirectPostProcessEffect::execute() */
			[[nodiscard]]
			const Vulkan::TextureInterface & execute (const Vulkan::CommandBuffer & commandBuffer, const Vulkan::TextureInterface & inputColor, const FrameContext & context) noexcept override;

			/** @copydoc EmEn::Graphics::IndirectPostProcessEffect::requiresHDR() */
			[[nodiscard]]
			bool
			requiresHDR () const noexcept override
			{
				return true;
			}

			/** @copydoc EmEn::Graphics::IndirectPostProcessEffect::requiresDepth() */
			[[nodiscard]]
			bool
			requiresDepth () const noexcept override
			{
				return true;
			}

			/** @copydoc EmEn::Graphics::IndirectPostProcessEffect::requiresVelocity() */
			[[nodiscard]]
			bool
			requiresVelocity () const noexcept override
			{
				return true;
			}

			/**
			 * @brief Sets the motion blur parameters.
			 * @param parameters The new parameters.
			 * @return void
			 */
			void
			setParameters (const Parameters & parameters) noexcept
			{
				m_parameters = parameters;
			}

			/**
			 * @brief Returns the current motion blur parameters.
			 * @return const Parameters &
			 */
			[[nodiscard]]
			const Parameters &
			parameters () const noexcept
			{
				return m_parameters;
			}

		private:

			Parameters m_parameters;
			/* Intermediate render targets. */
			IntermediateRenderTarget m_tileMaxHTarget;
			IntermediateRenderTarget m_tileMaxTarget;
			IntermediateRenderTarget m_neighborMaxTarget;
			IntermediateRenderTarget m_outputTarget;
			/* Pipelines. */
			std::shared_ptr< Vulkan::GraphicsPipeline > m_tileMaxHPipeline;
			std::shared_ptr< Vulkan::GraphicsPipeline > m_tileMaxPipeline;
			std::shared_ptr< Vulkan::GraphicsPipeline > m_neighborMaxPipeline;
			std::shared_ptr< Vulkan::GraphicsPipeline > m_gatherPipeline;
			/* Pipeline layouts. */
			std::shared_ptr< Vulkan::PipelineLayout > m_tileMaxHLayout;
			std::shared_ptr< Vulkan::PipelineLayout > m_tileMaxLayout;
			std::shared_ptr< Vulkan::PipelineLayout > m_neighborMaxLayout;
			std::shared_ptr< Vulkan::PipelineLayout > m_gatherLayout;
			/* Descriptor sets: the velocity, depth and colour textures come from the frame
			 * context, hence one set per frame-in-flight; the tile targets are ours, hence fixed. */
			std::vector< std::unique_ptr< Vulkan::DescriptorSet > > m_tileMaxHPerFrame;
			std::unique_ptr< Vulkan::DescriptorSet > m_tileMaxDescSet;
			std::unique_ptr< Vulkan::DescriptorSet > m_neighborMaxDescSet;
			std::vector< std::unique_ptr< Vulkan::DescriptorSet > > m_gatherPerFrame;
	};
}
