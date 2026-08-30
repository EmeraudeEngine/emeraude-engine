/*
 * src/Graphics/Effects/Framebuffer/RTR.hpp
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
#include "Graphics/TextureResource/TextureCubemap.hpp"

/* Local inclusions for usages. */
#include "Graphics/IntermediateRenderTarget.hpp"
#include "Vulkan/ComputePipeline.hpp"
#include "Vulkan/DescriptorPool.hpp"
#include "Vulkan/DescriptorSet.hpp"
#include "Vulkan/DescriptorSetLayout.hpp"
#include "Vulkan/Image.hpp"
#include "Vulkan/ImageView.hpp"
#include "Vulkan/Sampler.hpp"
#include "Vulkan/TextureInterface.hpp"

namespace EmEn::Graphics::Effects::Framebuffer
{
	/**
	 * @brief Ray-Traced Reflections (RTR) post-processing effect.
	 * @note Uses VK_KHR_ray_query / GL_EXT_ray_query in a fragment shader to trace
	 * hardware-accelerated reflection rays against the TLAS. Fetches material data
	 * from the RT SSBOs (mesh metadata, material properties, lights).
	 * Falls back to SSR when ray tracing hardware is not available.
	 * @extends EmEn::Graphics::IndirectPostProcessEffect This is a multi-pass post-process effect.
	 */
	class EMEN_API RTR final : public IndirectPostProcessEffect
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"RTREffect"};

			/** @copydoc EmEn::Graphics::IndirectPostProcessEffect::slot()
			 * @note Reflections come after everything that lights what they reflect. */
			[[nodiscard]]
			EffectSlot
			slot () const noexcept override
			{
				return EffectSlot::Reflections;
			}

			/** @copydoc EmEn::Graphics::PostProcessEffect::label() */
			[[nodiscard]]
			const char *
			label () const noexcept override
			{
				return ClassId;
			}

			/**
			 * @brief User-facing RTR parameters.
			 */
			struct EMEN_API Parameters
			{
				float maxDistance{100.0F};
				float intensity{0.8F};
				float fadeScreenEdge{0.15F};
				/** @brief MAXIMUM bilateral blur radius in texels, reached near roughness 0.7
				 * (the per-pixel radius scales with roughness² — the GGX cone footprint).
				 * Polished surfaces pay ~1 texel whatever this value. */
				uint32_t blurRadius{12};
				float depthSigma{0.5F};
				float normalSigma{0.3F};
			};

			/**
			 * @brief Push constants for the RTR trace pass.
			 */
			struct EMEN_API TracePushConstants
			{
				std::array< float, 16 > invViewProj;
				std::array< float, 3 > invViewCol0;
				float viewPosX;
				std::array< float, 3 > invViewCol1;
				float viewPosY;
				std::array< float, 3 > invViewCol2;
				float viewPosZ;
				float maxDistance;
				float intensity;
				float fadeScreenEdge;
				uint32_t lightCount;
				/* Scene ambient term (color × intensity), added at reflection hit points so
				 * the reflected surfaces match the raster look (which includes ambient). */
				float ambientR;
				float ambientG;
				float ambientB;
				/** @brief Sky luminance (nits): scales the NORMALIZED bindless environment
				 * sources sampled at hit/miss (irradiance, prefiltered) into absolute light. */
				float skyLuminance;
				/**
				 * @brief Focal length in TRACE texels (|P[1][1]| x traceHeight / 2): the glossy cone
				 * width the trace writes per pixel is an angular footprint projected to the screen.
				 */
				float coneScale;
			};

			/**
			 * @brief Constructs a ray-tracing reflexion effect.
			 * @param renderer A reference to the graphics renderer.
			 */
			explicit
			RTR (Renderer & renderer) noexcept
				: IndirectPostProcessEffect{renderer}
			{

			}

			/**
			 * @brief Constructs a ray-tracing reflexion effect.
			 * @param renderer A reference to the graphics renderer.
			 * @param parameters The initial parameters.
			 * @param environmentCubemap A cubemap sampled when a reflection ray escapes the scene (sky, distant environment). Default none (renderer default cubemap).
			 */
			RTR (Renderer & renderer, const Parameters & parameters, const std::shared_ptr< TextureResource::TextureCubemap > & environmentCubemap = nullptr) noexcept
				: IndirectPostProcessEffect{renderer},
				m_parameters{parameters},
				m_environmentCubemap{environmentCubemap}
			{

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

			/** @copydoc EmEn::Graphics::IndirectPostProcessEffect::usesSharedDenoise() */
			[[nodiscard]]
			bool
			usesSharedDenoise () const noexcept override
			{
				return true;
			}

			/** @copydoc EmEn::Graphics::IndirectPostProcessEffect::recordPreDenoisePasses() */
			void recordPreDenoisePasses (const Vulkan::CommandBuffer & commandBuffer, const Vulkan::TextureInterface & inputColor, const FrameContext & context) noexcept override;

			/** @copydoc EmEn::Graphics::IndirectPostProcessEffect::denoiseContribution() */
			[[nodiscard]]
			DenoiseContribution denoiseContribution (const FrameContext & context) const noexcept override;

			/** @copydoc EmEn::Graphics::IndirectPostProcessEffect::combineContribution() */
			[[nodiscard]]
			CombineContribution combineContribution (const FrameContext & context) const noexcept override;

			/** @copydoc EmEn::Graphics::IndirectPostProcessEffect::requiresDepth() */
			[[nodiscard]]
			bool
			requiresDepth () const noexcept override
			{
				return true;
			}

			/** @copydoc EmEn::Graphics::IndirectPostProcessEffect::requiresNormals() */
			[[nodiscard]]
			bool
			requiresNormals () const noexcept override
			{
				return true;
			}

			/** @copydoc EmEn::Graphics::IndirectPostProcessEffect::requiresAlbedo()
			 * @note The trace pass tints the PRIMARY surface's reflection by its Fresnel
			 * COLOR — F0 = mix(vec3(0.04), albedo, metalness) — so metals reflect in their
			 * own color (gold reflects gold) instead of a colorless mirror. */
			[[nodiscard]]
			bool
			requiresAlbedo () const noexcept override
			{
				return true;
			}

			/** @copydoc EmEn::Graphics::IndirectPostProcessEffect::requiresMaterialProperties() */
			[[nodiscard]]
			bool
			requiresMaterialProperties () const noexcept override
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

			/** @copydoc EmEn::Graphics::IndirectPostProcessEffect::requiresRayTracing() */
			[[nodiscard]]
			bool
			requiresRayTracing () const noexcept override
			{
				return true;
			}

			/** @copydoc EmEn::Graphics::PostProcessEffect::providesReflections() */
			[[nodiscard]]
			bool
			providesReflections () const noexcept override
			{
				return true;
			}

			/**
			 * @brief Sets the RTR parameters.
			 * @param parameters The new parameters.
			 * @return void
			 */
			void
			setParameters (const Parameters & parameters) noexcept
			{
				m_parameters = parameters;
			}

			/**
			 * @brief Returns the current RTR parameters.
			 * @return const Parameters &
			 */
			[[nodiscard]]
			const Parameters &
			parameters () const noexcept
			{
				return m_parameters;
			}

		private:

			/**
			 * @brief Exposes one of the effect's own images (a view and its sampler) as a TextureInterface
			 * for the combine pass: the pre-convolved reflection pyramid and the glossy-cone width map.
			 * @note Holds REFERENCES to the effect's shared pointers, so it follows create()/destroy()
			 * with no bookkeeping — which is why it must be declared after the members it refers to.
			 */
			class ImageTextureAdapter final : public Vulkan::TextureInterface
			{
				public:

					ImageTextureAdapter (const std::shared_ptr< Vulkan::Image > & image, const std::shared_ptr< Vulkan::ImageView > & imageView, const std::shared_ptr< Vulkan::Sampler > & sampler) noexcept
						: m_image{image},
						m_imageView{imageView},
						m_sampler{sampler}
					{

					}

					[[nodiscard]]
					bool
					isCreated () const noexcept override
					{
						return m_image != nullptr && m_image->isCreated();
					}

					[[nodiscard]]
					Vulkan::TextureType
					type () const noexcept override
					{
						return Vulkan::TextureType::Texture2D;
					}

					[[nodiscard]]
					uint32_t
					dimensions () const noexcept override
					{
						return 2;
					}

					[[nodiscard]]
					bool
					isCubemapTexture () const noexcept override
					{
						return false;
					}

					[[nodiscard]]
					std::shared_ptr< Vulkan::Image >
					image () const noexcept override
					{
						return m_image;
					}

					[[nodiscard]]
					std::shared_ptr< Vulkan::ImageView >
					imageView () const noexcept override
					{
						return m_imageView;
					}

					[[nodiscard]]
					std::shared_ptr< Vulkan::Sampler >
					sampler () const noexcept override
					{
						return m_sampler;
					}

					[[nodiscard]]
					bool
					request3DTextureCoordinates () const noexcept override
					{
						return false;
					}

				private:

					const std::shared_ptr< Vulkan::Image > & m_image;
					const std::shared_ptr< Vulkan::ImageView > & m_imageView;
					const std::shared_ptr< Vulkan::Sampler > & m_sampler;
			};

			Parameters m_parameters;
			std::shared_ptr< TextureResource::TextureCubemap > m_environmentCubemap;
			/* IRTs: trace (half-res), blur H (half-res), blur V (half-res). */
			IntermediateRenderTarget m_traceTarget;
			IntermediateRenderTarget m_blurHTarget;
			IntermediateRenderTarget m_blurVTarget;
			/* Pipelines. */
			std::shared_ptr< Vulkan::GraphicsPipeline > m_tracePipeline;
			/* Pipeline layouts. */
			std::shared_ptr< Vulkan::PipelineLayout > m_traceLayout;
			/* Per-frame descriptor sets. */
			std::vector< std::unique_ptr< Vulkan::DescriptorSet > > m_tracePerFrame;
			/* Pre-convolved REFLECTION pyramid (glossy cone approximation): half-res base,
			 * tent-downsampled chain of the PREMULTIPLIED trace output rebuilt every frame.
			 * The composite reads it at the roughness²-driven LOD (the /confidence division
			 * renormalizes edge bleed) — an O(1) blur whatever the cone width, where the
			 * separable bilateral tops out at a few texels. */
			std::shared_ptr< Vulkan::Image > m_pyramidImage;
			std::vector< std::shared_ptr< Vulkan::ImageView > > m_pyramidMipViews;
			std::shared_ptr< Vulkan::ImageView > m_pyramidFullView;
			std::shared_ptr< Vulkan::Sampler > m_pyramidSampler;
			std::shared_ptr< Vulkan::DescriptorSetLayout > m_pyramidDSLayout;
			std::shared_ptr< Vulkan::PipelineLayout > m_pyramidPipelineLayout;
			std::unique_ptr< Vulkan::ComputePipeline > m_pyramidDownsamplePipeline;
			std::shared_ptr< Vulkan::DescriptorPool > m_pyramidDescriptorPool;
			/* Fixed sets: trace target -> mip 0, then mip k-1 -> mip k. */
			std::vector< std::unique_ptr< Vulkan::DescriptorSet > > m_pyramidSets;
			uint32_t m_pyramidMipCount{0U};
			/* Glossy cone width map (R16F, trace resolution): written by the trace pass per pixel from
			 * the hit distance, the roughness and the camera distance; read by the combine pass. */
			std::shared_ptr< Vulkan::Image > m_coneImage;
			std::shared_ptr< Vulkan::ImageView > m_coneView;
			std::shared_ptr< Vulkan::Sampler > m_coneSampler;
			/* The pyramid and the cone width map exposed as TextureInterfaces for the combine contribution. */
			ImageTextureAdapter m_pyramidTexture{m_pyramidImage, m_pyramidFullView, m_pyramidSampler};
			ImageTextureAdapter m_coneTexture{m_coneImage, m_coneView, m_coneSampler};
			/* Glossy cone controls, read from the settings ONCE at create() (a change takes
			 * effect on the next stack creation — relaunch or swap-chain recreation). The cone
			 * WIDTH is per pixel since v2 (hit distance); these shape the sharp/pyramid cross-fade
			 * and cap the LOD. See SettingKeys.hpp. */
			float m_coneBlendStart{2.0F};
			float m_coneBlendFull{6.0F};
			float m_coneMaxLod{8.0F};
			bool m_coneEnabled{true};
	};
}
