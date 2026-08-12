/*
 * src/Saphir/Generator/SceneRendering.hpp
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

/* Local inclusions for inheritances. */
#include "Abstract.hpp"

/* Local inclusions for usages. */
#include "Graphics/RenderTarget/Abstract.hpp"
#include "Saphir/LightGenerator.hpp"
#include "SettingKeys.hpp"
#include "Vulkan/Framebuffer.hpp"
#include "Vulkan/RenderPass.hpp"

/* Forward declarations. */
namespace EmEn::Scenes
{
	class Scene;
}

namespace EmEn::Saphir::Generator
{
	/**
	 * @brief This generator builds the graphics pipeline to display a scene.
	 * @extends EmEn::Saphir::Generator::Abstract This a generator.
	 * @note There is a single lighting model (Cook-Torrance, evaluated per fragment); there is no
	 * Blinn-Phong/Gouraud path and no shader-quality user setting anymore. The quality tier is
	 * Abstract's HighQualityEnabled flag, a RENDERING decision (meant to follow distance) that
	 * this generator currently always enables (see the constructor). It is part of the program
	 * cache key, so a future distance-driven switch will produce its own program variants for free.
	 * @version 0.9.54
	 */
	class SceneRendering final : public Abstract
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"SceneRendering"};

			/**
			 * @brief Constructs a graphics shader generator for a geometry.
			 * @param shaderProgramName A reference to a string.
			 * @param renderTarget A reference to the render target smart pointer.
			 * @param renderableInstance A reference to the renderable instance smart pointer.
			 * @param layerIndex The renderable instance layer.
			 * @param scene A reference to a scene.
			 * @param renderPassType The render pass type to know which kind of render is implied.
			 * @param settings A reference to the core settings.
			 * @note Keeps a non-owning pointer to \p scene; the caller must ensure the scene outlives
			 * this generator.
			 * @todo Nothing drives HighQualityEnabled down yet: every program is generated at full
			 * quality. Intended to be lowered by rendering distance once that logic exists.
			 */
			SceneRendering (const std::string & shaderProgramName, const std::shared_ptr< const Graphics::RenderTarget::Abstract > & renderTarget, const std::shared_ptr< const Graphics::RenderableInstance::Abstract > & renderableInstance, uint32_t layerIndex, const Scenes::Scene & scene, Graphics::RenderPassType renderPassType, Settings & settings) noexcept
				: Abstract{shaderProgramName, renderTarget, renderableInstance, layerIndex},
				m_renderPassType{renderPassType},
				m_lightGenerator{settings, renderPassType},
				m_scene{&scene}
			{
				/* Quality tier — a RENDERING decision, not a user setting (the
				 * "Core/Graphics/Shader/EnableHighQuality" setting is gone). It is meant to be
				 * driven by rendering DISTANCE: a distant surface can take the cheap branches
				 * (simpler transmission, no Fresnel-gated reflection, no parallax).
				 * ⚠️ TODO: nothing drives it down yet — every program is generated at full
				 * quality. The flag is part of the program cache key, so a future distance
				 * switch produces its own program variants for free. */
				this->enableFlag(HighQualityEnabled);

				this->setPOMIterations(this->highQualityEnabled() ? settings.getOrSetDefault< int >(GraphicsTexturePOMIterationsKey, DefaultGraphicsTexturePOMIterations) : 0);

				if ( (this->materialEnabled() && this->getMaterialInterface()->useEnvironmentCubemap()) || Graphics::renderPassUsesColorProjection(renderPassType) )
				{
					this->enableBindlessTextures(true);
				}

				/* Detect whether the render target supports MRT outputs
				 * by checking the render pass color attachment count.
				 * Attachment order: [0]=color, [1]=normals, [2]=materialProperties, [3]=albedo.
				 * Each MRT attachment requires every one before it (enforced by Renderer). */
				if ( const auto * fb = renderTarget->framebuffer(); fb != nullptr )
				{
					if ( const auto & rp = fb->renderPass(); rp != nullptr )
					{
						const auto colorCount = rp->colorAttachmentCount();
						m_hasNormalsAttachment = colorCount > 1;
						m_hasMaterialPropertiesAttachment = colorCount > 2;
						m_hasAlbedoAttachment = colorCount > 3;
						m_hasVelocityAttachment = colorCount > 4;
					}
				}
			}

			/**
			 * @brief Returns the render pass type.
			 * @return Graphics::RenderPassType
			 */
			[[nodiscard]]
			Graphics::RenderPassType
			renderPassType () const noexcept
			{
				return m_renderPassType;
			}

			/** @copydoc EmEn::Saphir::Generator::Abstract::computeProgramCacheKey() */
			[[nodiscard]]
			size_t computeProgramCacheKey () const noexcept override;

			/** @copydoc EmEn::Saphir::Generator::Abstract::generatorClassId() */
			[[nodiscard]]
			const char *
			generatorClassId () const noexcept override
			{
				return ClassId;
			}

		private:

			/**
			 * @copydoc EmEn::Saphir::Generator::Abstract::prepareUniformSets()
			 * @note Must run before onCreateDataLayouts(): the descriptor sets it enables here
			 * (PerView, and conditionally PerSceneTransforms/PerLight/PerModel/PerModelLayer/
			 * PerBindless) fix the pipeline layout order that onCreateDataLayouts() then honors.
			 */
			void prepareUniformSets (SetIndexes & setIndexes) noexcept override;

			/**
			 * @copydoc EmEn::Saphir::Generator::Abstract::onGenerateShadersCode()
			 * @note Fails if lighting is requested but no scene was supplied to query light
			 * information from. Generates the vertex stage, then the fragment stage — the fragment
			 * stage reads state (m_velocityOutputsEmitted) written by the vertex stage, so the order
			 * is load-bearing.
			 */
			[[nodiscard]]
			bool onGenerateShadersCode (Program & program) noexcept override;

			/**
			 * @copydoc EmEn::Saphir::Generator::Abstract::onCreateDataLayouts()
			 * @note The PerSceneTransforms descriptor set layout is appended right after PerView,
			 * matching the set index order decided in prepareUniformSets().
			 */
			[[nodiscard]]
			bool onCreateDataLayouts (Graphics::Renderer & renderer, const SetIndexes & setIndexes, Base::StaticVector< std::shared_ptr< Vulkan::DescriptorSetLayout >, 6 > & descriptorSetLayouts, Base::StaticVector< VkPushConstantRange, 4 > & pushConstantRanges) noexcept override;

			/**
			 * @copydoc EmEn::Saphir::Generator::Abstract::onGraphicsPipelineConfiguration()
			 * @note When the render target has MRT attachments (normals/material-properties/albedo/
			 * velocity), each gets its own color-blend attachment state (requires the
			 * 'independentBlend' device feature): light passes get a zero write mask (they must never
			 * touch the G-buffer), while the ambient/simple pass writes it opaquely even when the
			 * color attachment itself blends (a translucent surface must still write full G-buffer
			 * data for the top-most surface, not a value diluted by alpha blending).
			 */
			[[nodiscard]]
			bool onGraphicsPipelineConfiguration (const Program & program, Vulkan::GraphicsPipeline & graphicsPipeline) noexcept override;

			/**
			 * @brief Returns whether the vertex shader needs separate view and model matrices instead of a single combined MVP.
			 * @note True for any complex material, for every light pass, and for the ambient/simple
			 * pass only when either the normals MRT attachment or normal mapping requires the normal
			 * matrix (view * model). Drives the advanced-rendering flag passed to
			 * Program::initVertexShader(), which is part of the program cache key.
			 * @return bool
			 */
			[[nodiscard]]
			bool isAdvancedRendering () const noexcept;

			/**
			 * @brief Returns whether a LIGHT PASS must be generated for this program.
			 * @note Three conditions, all necessary: the scene's light set is enabled, the
			 * instance asked for lighting, and the MATERIAL does not declare itself unlit.
			 * The last one is the material-level veto (MaterialFlagBits::UnlitEnabled, glTF
			 * KHR_materials_unlit semantics): content carrying its own radiance — skybox,
			 * sprite, baked lighting — must never be re-lit, whatever the instance asked for.
			 * @return bool
			 */
			[[nodiscard]]
			bool isLightingRequested () const noexcept;

			/**
			 * @brief Returns whether the program uses the scene InstanceTransforms SSBO set (SetType::PerSceneTransforms).
			 * @note False for MultiDrawIndirect (matrices come from an indirect VBO) and for cubemap
			 * targets. For an instanced program, true only when a velocity attachment is present — in
			 * that case the set is used solely for the {previousViewProjection,
			 * previousViewProjectionInfinity} header consumed by the motion-vector pass, never for the
			 * model matrices (those stay in the per-instance VBO). Otherwise true for classic AND
			 * advanced non-instanced rendering, provided the scene's instance transforms are
			 * initialized. Evaluated at prepareUniformSets() time — it seals the pipeline layout; the
			 * matrix-source decision in the vertex shader follows Program::wasInstanceTransformsEnabled()
			 * separately.
			 * @return bool
			 */
			[[nodiscard]]
			bool useInstanceTransformsSet () const noexcept;

			/**
			 * @brief Generates the vertex shader stage of the graphics pipeline.
			 * @note Sets m_velocityOutputsEmitted to record whether the velocity clip-position
			 * outputs were actually synthesized (requires a velocity attachment and the
			 * PerSceneTransforms set); generateFragmentShader() reads that flag afterwards, so this
			 * must run first (as it always does, from onGenerateShadersCode()).
			 * @param program A reference to the program being constructed.
			 * @return bool
			 */
			[[nodiscard]]
			bool generateVertexShader (Program & program) noexcept;

			/**
			 * @brief Generates the fragment shader stage of the graphics pipeline.
			 * @note Must run after generateVertexShader(): it reads m_velocityOutputsEmitted and
			 * relies on connectFromPreviousShader() to pick up the vertex stage's outputs. Emits one
			 * of three mutually exclusive output paths depending on the state — lit (Cook-Torrance,
			 * via m_lightGenerator), unlit material (KHR_materials_unlit: emission multiplies the
			 * surface color instead of being added), or no material at all (magenta placeholder).
			 * @param program A reference to the program being constructed.
			 * @return bool
			 */
			[[nodiscard]]
			bool generateFragmentShader (Program & program) noexcept;

			Graphics::RenderPassType m_renderPassType;
			LightGenerator m_lightGenerator;
			/** @brief Non-owning pointer to the scene providing light and instance-transform data; the constructor always sets it from a reference, but several call sites still defensively re-check for null. */
			const Scenes::Scene * m_scene{nullptr};
			bool m_hasNormalsAttachment{false};
			bool m_hasMaterialPropertiesAttachment{false};
			bool m_hasAlbedoAttachment{false};
			bool m_hasVelocityAttachment{false};
			/** @brief Whether the vertex shader emitted the velocity clip-position outputs. */
			bool m_velocityOutputsEmitted{false};
	};
}
