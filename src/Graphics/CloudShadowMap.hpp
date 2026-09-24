/*
 * src/Graphics/CloudShadowMap.hpp
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
#include "Math/Matrix.hpp"

namespace EmEn::Graphics
{
	/**
	 * @brief The volumetric clouds' shadow on the world: a Beer shadow map seen from the main sun.
	 * @note A Beer shadow map (S. Hillaire, *Physically Based Sky, Atmosphere and Cloud Rendering in
	 * Frostbite*, SIGGRAPH 2016) stores, per texel of a map seen along the light: **R** the depth along
	 * the light where the clouds START, **G** their mean extinction, **B** their total optical depth.
	 * A receiver at depth `d` sees `exp(-min(B, G · max(d - R, 0)))` — 1 above the clouds, the whole
	 * optical depth under them, and a ramp INSIDE one: a treetop in a low cloud is half shadowed, not
	 * black, which a single transmittance per texel cannot express.
	 * @note The map's frame belongs to the LIGHT (Scenes::Component::DirectionalLight::updateCloudShadow(),
	 * logic thread, published with the light block): the lit shaders and this pass read the SAME matrix
	 * for a frame. Depths are measured from the camera's plane, so a half float carries them.
	 * @note Recorded by the Renderer BEFORE the scene pass, on the frame's command buffer; the render
	 * pass dependencies of its IntermediateRenderTarget order it against the previous frame's reads and
	 * make it visible to this frame's fragment shaders. Owned by the scene's Scenes::CloudSet.
	 * @note Stage 2 lot 1 (owner decision, 2026-09-24): the lit MATERIALS read it. The volumetric
	 * scattering, the clouds among themselves and the ray-traced lanes do not yet.
	 * @extends EmEn::Graphics::IndirectPostProcessEffect For the fullscreen-pass helpers only: this is a
	 * component (EffectSlot::Internal), never filed in a chain.
	 */
	class EMEN_API CloudShadowMap final : public IndirectPostProcessEffect
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"CloudShadowMap"};

			/** @brief How far along the light, either side of the camera's plane, clouds are looked for, in metres. */
			static constexpr auto DepthRange{2000.0F};

			/** @brief Steps across the clouds a texel's ray crosses. */
			static constexpr uint32_t StepCount{48};

			/** @brief The per-frame uniform block (std140). */
			struct EMEN_API ShadowBlock
			{
				/** @brief (u, v, depth, 1) -> world, column-major: the inverse of the light's cloud shadow matrix. */
				std::array< float, 16 > mapToWorld{};
				/** @brief xyz = the light's direction of propagation, w = cloud count. */
				std::array< float, 4 > lightDirection{};
				/** @brief x = depth range either side of the camera plane (m), y = step count. */
				std::array< float, 4 > parameters{};
				/** @brief The clouds. */
				std::array< CloudBlock, MaxCloudVolumes > clouds{};
			};

			static_assert(sizeof(ShadowBlock) == 64 + 2 * 16 + MaxCloudVolumes * 112, "ShadowBlock must match the GLSL std140 block !");

			/**
			 * @brief Constructs a cloud shadow map.
			 * @param renderer A reference to the graphics renderer.
			 * @param resolution The side of the map, in texels.
			 * @param coverage The side of the map, in metres.
			 */
			CloudShadowMap (Renderer & renderer, uint32_t resolution, float coverage) noexcept
				: IndirectPostProcessEffect{renderer},
				m_resolution{resolution},
				m_coverage{coverage}
			{

			}

			/** @copydoc EmEn::Graphics::IndirectPostProcessEffect::slot() */
			[[nodiscard]]
			EffectSlot
			slot () const noexcept override
			{
				return EffectSlot::Internal;
			}

			/** @copydoc EmEn::Graphics::PostProcessEffect::label() */
			[[nodiscard]]
			const char *
			label () const noexcept override
			{
				return ClassId;
			}

			/**
			 * @copydoc EmEn::Graphics::IndirectPostProcessEffect::create()
			 * @note The frame extent is ignored: the map has its own resolution.
			 */
			[[nodiscard]]
			bool create (uint32_t width, uint32_t height) noexcept override;

			/** @copydoc EmEn::Graphics::IndirectPostProcessEffect::destroy() */
			void destroy () noexcept override;

			/**
			 * @brief Records the map for this frame.
			 * @param commandBuffer A reference to the frame's command buffer, OUTSIDE any render pass.
			 * @param clouds The clouds, as the view march reads them.
			 * @param cloudCount How many of them are valid.
			 * @param worldToMap The light's published cloud shadow matrix.
			 * @return bool
			 */
			bool record (const Vulkan::CommandBuffer & commandBuffer, const std::array< CloudBlock, MaxCloudVolumes > & clouds, uint32_t cloudCount, const Base::Math::Matrix< 4, float > & worldToMap) noexcept;

			/**
			 * @brief Returns the map, to register in the scene's bindless 2D array.
			 * @return std::shared_ptr< IntermediateRenderTarget >
			 */
			[[nodiscard]]
			std::shared_ptr< IntermediateRenderTarget >
			target () const noexcept
			{
				return m_target;
			}

			/**
			 * @brief Returns the side of the map, in texels.
			 * @return uint32_t
			 */
			[[nodiscard]]
			uint32_t
			resolution () const noexcept
			{
				return m_resolution;
			}

			/**
			 * @brief Returns the side of the map, in metres.
			 * @return float
			 */
			[[nodiscard]]
			float
			coverage () const noexcept
			{
				return m_coverage;
			}

		private:

			std::shared_ptr< IntermediateRenderTarget > m_target{std::make_shared< IntermediateRenderTarget >()};
			std::shared_ptr< Vulkan::PipelineLayout > m_layout;
			std::shared_ptr< Vulkan::GraphicsPipeline > m_pipeline;
			std::vector< std::unique_ptr< Vulkan::DescriptorSet > > m_perFrame;
			std::vector< std::unique_ptr< Vulkan::UniformBufferObject > > m_frameUBOs;
			uint32_t m_resolution;
			float m_coverage;
	};
}
