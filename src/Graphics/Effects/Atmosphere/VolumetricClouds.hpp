/*
 * src/Graphics/Effects/Atmosphere/VolumetricClouds.hpp
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
#include "Graphics/Effects/Shared/CloudVolumeGLSL.hpp"
#include "Graphics/IntermediateRenderTarget.hpp"

/* Forward declarations. */
namespace EmEn
{
	namespace Vulkan
	{
		class Image;
		class ImageView;
		class Sampler;
	}

	namespace Scenes
	{
		class CloudSet;
	}
}

namespace EmEn::Graphics::Effects::Atmosphere
{
	/**
	 * @brief Draws the scene's volumetric clouds: a world-space ray march through every
	 * Scenes::Component::CloudVolume a pixel's view ray crosses.
	 * @note SCENE-DRIVEN (owner decision, 2026-09-24): an application never adds it. The stack files
	 * it the first frame the scene holds a cloud (PostProcessStack::syncSceneEffects()), and
	 * requiresCloudVolumes() keeps it out of the frame — and unallocated — while there is none.
	 * @note It composites `scene · T + L` inside its own pass (the VolumetricScattering pattern), so
	 * it emits no combine snippet. A pixel whose ray crosses no cloud box pays the box tests and
	 * nothing else.
	 * @note The march, per cloud crossed (front to back, up to 8 per pixel):
	 * - the shape's density (Graphics::CloudShapeResource, bindless 3D array) eroded at its shell by
	 *   a tileable billowy cellular noise that DRIFTS UPWARD — a boiling cumulus (Schneider & Vos,
	 *   *The Real-time Volumetric Cloudscapes of Horizon Zero Dawn*, SIGGRAPH 2015, for the remap);
	 * - empty air skipped by the shape's conservative distance channel;
	 * - the sun: an optical depth marched toward it INSIDE the cloud, times the cascaded shadow map
	 *   (the trees shadow the inside of a cloud), through a dual-lobe Henyey-Greenstein phase;
	 * - multiple scattering by the octave approximation (M. Wrenninge, C. Kulla & V. Lundqvist,
	 *   *Oz: The Great and Volumetric*, SIGGRAPH 2013 Talks): each octave halves the extinction seen
	 *   toward the sun, the energy and the anisotropy;
	 * - the ambient: the sky irradiance on top, the Lambertian ground bounce underneath, blended by
	 *   the height in the cloud (the HZD / Frostbite approximation);
	 * - the energy-conserving integration of the source over each step (S. Hillaire, *Physically
	 *   Based Sky, Atmosphere and Cloud Rendering in Frostbite*, SIGGRAPH 2016).
	 * @note Photometry: nits directly, like VolumetricScattering — the sun enters as its illuminance
	 * in lux times a phase in 1/sr, the sky as the irradiance cubemap times the sky luminance. There
	 * is no exposure or gain knob, deliberately.
	 * @note ⚠️ Stage 1 limits, by owner decision (2026-09-24): the clouds do NOT shadow the world yet
	 * (stage 2: a transmittance map read by the sun term), a cloud does not shadow another one, and
	 * the ray-traced lanes do not see them.
	 * @extends EmEn::Graphics::IndirectPostProcessEffect This is a single-pass post-process effect.
	 */
	class EMEN_API VolumetricClouds final : public IndirectPostProcessEffect
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"VolumetricCloudsEffect"};

			/** @brief The most clouds one frame draws; the others are skipped, with one warning. */
			static constexpr uint32_t MaxClouds{MaxCloudVolumes};

			/** @brief The edge of the tileable detail noise texture, in voxels. */
			static constexpr uint32_t DetailNoiseSize{32};

			/** @brief Cellular cells per period of the detail noise texture. */
			static constexpr uint32_t DetailNoiseCells{4};

			/** @copydoc EmEn::Graphics::IndirectPostProcessEffect::slot() */
			[[nodiscard]]
			EffectSlot
			slot () const noexcept override
			{
				return EffectSlot::Clouds;
			}

			/** @copydoc EmEn::Graphics::PostProcessEffect::label() */
			[[nodiscard]]
			const char *
			label () const noexcept override
			{
				return ClassId;
			}

			/**
			 * @brief The TECHNIQUE knobs of the march.
			 * @note The look of a cloud belongs to its component (Scenes::Component::CloudVolume::Look).
			 */
			struct EMEN_API Parameters
			{
				/** @brief View-ray steps across the diagonal of a cloud box. */
				uint32_t stepCount{64};
				/** @brief Steps toward the sun, per view step. */
				uint32_t lightStepCount{6};
				/** @brief Lambertian albedo of the ground under the clouds, in [0, 1]. */
				float groundAlbedo{0.2F};
			};

			/** @brief The per-frame uniform block (std140). */
			struct EMEN_API FrameBlock
			{
				/** @brief Column-major inverse view-projection of the frame that produced the depth. */
				std::array< float, 16 > inverseViewProjection{};
				/** @brief xyz = camera world position, w = cloud count. */
				std::array< float, 4 > cameraPosition{};
				/** @brief xyz = camera forward axis, w = step count. */
				std::array< float, 4 > cameraForward{};
				/** @brief xyz = direction of PROPAGATION of the sun's light, w = light step count. */
				std::array< float, 4 > sunDirection{};
				/** @brief rgb = sun colour × illuminance in lux (0 = no sun), w = shadow bias. */
				std::array< float, 4 > sunIlluminance{};
				/** @brief x = sky luminance (nits, scale of the irradiance cubemap), y = ground albedo, z = cascade count, w = unused. */
				std::array< float, 4 > ambient{};
				/** @brief The clouds. */
				std::array< CloudBlock, MaxClouds > clouds{};
			};

			static_assert(sizeof(FrameBlock) == 64 + 5 * 16 + MaxClouds * 112, "FrameBlock must match the GLSL std140 block !");

			/** @brief Why the clouds of a frame were drawn or not — the census the pass traces. */
			struct EMEN_API Census
			{
				uint32_t drawn{0};
				uint32_t withoutShapeSlot{0};
				uint32_t shapeNotOnGPU{0};
				uint32_t degenerateBox{0};
				bool overflow{false};
			};

			/**
			 * @brief Fills the shader description of every drawable cloud of a scene, from its PUBLISHED state.
			 * @note [RENDER THREAD] The ONE translation of a Scenes::Component::CloudVolume into a
			 * Graphics::CloudBlock, shared by this pass and Graphics::CloudShadowMap — the "keep the look"
			 * extinction (optical thickness over the current world height) is computed here and nowhere else.
			 * @param clouds A reference to the scene's clouds.
			 * @param readStateIndex The render state slot latched by the frame.
			 * @param time The frame time in seconds (the boiling offset).
			 * @param blocks A reference to the blocks to fill.
			 * @return Census
			 */
			[[nodiscard]]
			static Census gatherClouds (const Scenes::CloudSet & clouds, uint32_t readStateIndex, float time, std::array< CloudBlock, MaxClouds > & blocks) noexcept;

			/**
			 * @brief Constructs a volumetric clouds effect with the default technique parameters.
			 * @param renderer A reference to the graphics renderer.
			 */
			explicit
			VolumetricClouds (Renderer & renderer) noexcept
				: IndirectPostProcessEffect{renderer}
			{

			}

			/**
			 * @brief Constructs a volumetric clouds effect.
			 * @note ⚠️ Two constructors, not a `= {}` default: a default argument of a NESTED type with
			 * member initializers is refused by GCC while the enclosing class is incomplete.
			 * @param renderer A reference to the graphics renderer.
			 * @param parameters The technique parameters.
			 */
			VolumetricClouds (Renderer & renderer, const Parameters & parameters) noexcept
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

			/** @copydoc EmEn::Graphics::IndirectPostProcessEffect::requiresDepth() */
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

			/** @copydoc EmEn::Graphics::IndirectPostProcessEffect::requiresCloudVolumes() */
			[[nodiscard]]
			bool
			requiresCloudVolumes () const noexcept override
			{
				return true;
			}

			/**
			 * @brief Sets the technique parameters.
			 * @param parameters A reference to the parameters.
			 * @return void
			 */
			void
			setParameters (const Parameters & parameters) noexcept
			{
				m_parameters = parameters;
			}

			/**
			 * @brief Returns the technique parameters.
			 * @return const Parameters &
			 */
			[[nodiscard]]
			const Parameters &
			parameters () const noexcept
			{
				return m_parameters;
			}

		private:

			/** @brief One shader variant: with or without the cascaded shadow map. */
			struct Variant
			{
				std::shared_ptr< Vulkan::PipelineLayout > layout;
				std::shared_ptr< Vulkan::GraphicsPipeline > pipeline;
				std::vector< std::unique_ptr< Vulkan::DescriptorSet > > perFrame;
			};

			/**
			 * @brief Builds one shader variant.
			 * @param variant A reference to the variant to fill.
			 * @param withCascades Whether the variant samples the cascaded shadow map.
			 * @return bool
			 */
			bool createVariant (Variant & variant, bool withCascades) noexcept;

			/**
			 * @brief Grows the tileable detail noise and uploads it as a 3D texture.
			 * @return bool
			 */
			bool createDetailNoise () noexcept;

			Parameters m_parameters;
			IntermediateRenderTarget m_outputTarget;
			Variant m_withCascades;
			Variant m_withoutCascades;
			std::vector< std::unique_ptr< Vulkan::UniformBufferObject > > m_frameUBOs;
			std::vector< std::unique_ptr< Vulkan::UniformBufferObject > > m_cascadeUBOs;
			std::shared_ptr< Vulkan::Image > m_detailNoiseImage;
			std::shared_ptr< Vulkan::ImageView > m_detailNoiseView;
			std::shared_ptr< Vulkan::Sampler > m_detailNoiseSampler;
			/* The last census traced: drawn, without a shape slot, shape not on the GPU, degenerate. */
			std::array< uint32_t, 4 > m_lastCensus{UINT32_MAX, UINT32_MAX, UINT32_MAX, UINT32_MAX};
			/* One warning per effect instance, not one per frame. */
			bool m_tooManyCloudsReported{false};
	};
}