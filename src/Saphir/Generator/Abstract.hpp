/*
 * src/Saphir/Generator/Abstract.hpp
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

/* STL inclusions. */
#include <algorithm>
#include <cstdint>
#include <memory>
#include <string>

/* Local inclusions for inheritances. */
#include "NameableTrait.hpp"

/* Local inclusions for usages. */
#include "Graphics/Renderable/SkeletalDataTrait.hpp"
#include "Graphics/RenderableInstance/Abstract.hpp"
#include "StaticVector.hpp"
#include "Saphir/Program.hpp"

namespace EmEn
{
	namespace Graphics
	{
		namespace RenderTarget
		{
			class Abstract;
		}

		class Renderer;
	}

	namespace Vulkan
	{
		class DescriptorSetLayout;
		class Framebuffer;
	}

	class Settings;
}

namespace EmEn::Saphir::Generator
{
	/**
	 * @enum GeneratorFlagBits
	 * @brief Generator flag bits.
	 * @note Every bit is part of the shader program cache key (folded in through
	 * Base::FlagTrait::flags() by each subclass's computeProgramCacheKey()): two generators
	 * that differ by a single flag get distinct generated programs, never a stale reuse.
	 * @version 0.9.54
	 */
	// NOLINTNEXTLINE(performance-enum-size): designed for growth — uint32_t reserves bit headroom for future flag additions.
	enum GeneratorFlagBits : uint32_t
	{
		/** @brief No flag set. */
		None = 0U,
		/** @brief Enables shader generation logging/stats (see Abstract::enableDebugging()). */
		Debug = 1U << 0,
		/** @brief Selects the expensive lighting branches (Fresnel-gated reflection, refraction,
		 * parallax occlusion mapping) over their cheap fallbacks in the single Cook-Torrance
		 * per-fragment lighting model. Meant to be driven by rendering distance; nothing lowers
		 * it yet, so every program is currently generated at full quality. */
		HighQualityEnabled = 1U << 1,
		/** @brief The renderable instance uses a per-instance model-matrix VBO (see Abstract::isInstancingEnabled()). */
		IsInstancingEnabled = 1U << 2,
		/** @brief The renderable always faces the camera (sprites), affecting TBN reconstruction. */
		IsRenderableFacingCamera = 1U << 3,
		/** @brief The renderable instance participates in lighting, so the light pass code path must be generated. */
		IsLightingEnabled = 1U << 4,
		/** @brief Materials with automatic reflection sample the global bindless texture arrays
		 * instead of getting a per-material descriptor set. */
		BindlessTexturesEnabled = 1U << 5,
		/** @brief Multi-Draw Indirect enabled: model matrix from SSBO via BDA + gl_DrawID instead of push constants. */
		IsMultiDrawIndirectEnabled = 1U << 6,
		/** @brief Skeletal animation enabled: bone matrix SSBO and vertex skinning in shaders. */
		IsSkeletalAnimationEnabled = 1U << 7,
		/** @brief Instanced motion history: the per-instance VBO carries the previous model matrix (+4 vec4). */
		IsInstanceMotionHistoryEnabled = 1U << 8,
		/** @brief The renderable uses the INFINITY view (translation-free, e.g. the sky background):
		 * its velocity must be built from the previous INFINITY view-projection, not the regular one. */
		IsUsingInfinityView = 1U << 9
	};

	/**
	 * @brief The base class for every shader program generator.
	 * @extends EmEn::Base::NameableTrait This will hold the name of the program generated.
	 * @extends EmEn::Base::FlagTrait Enables flag ability for parameters.
	 * @note Runs at shader-program creation time (scene load, or a program-cache miss), never
	 * per frame: its cost is a load-time stall, not a runtime one.
	 * @version 0.9.54
	 */
	class Abstract : public Base::NameableTrait, public Base::FlagTrait< uint32_t >
	{
		public:

			/** @brief Default GLSL version string passed to generateShaderProgram(). */
			static constexpr auto DefaultGLSLVersion{"460"};
			/** @brief Default GLSL profile string passed to generateShaderProgram(). */
			static constexpr auto DefaultGLSLProfile{"core"};

			/**
			 * @brief Copy constructor.
			 * @param copy A reference to the copied instance.
			 */
			Abstract (const Abstract & copy) noexcept = default;

			/**
			 * @brief Move constructor.
			 * @param copy A reference to the copied instance.
			 */
			Abstract (Abstract && copy) noexcept = default;

			/**
			 * @brief Assignment operator.
			 * @param copy A reference to the copied instance.
			 * @return Abstract &
			 */
			Abstract & operator= (const Abstract & copy) noexcept = default;

			/**
			 * @brief Move operator.
			 * @param copy A reference to the copied instance.
			 * @return Abstract &
			 */
			Abstract & operator= (Abstract && copy) noexcept = default;

			/** 
			 * @brief Destructs the abstract shader generator.
			 */
			~Abstract () override = default;

			/**
			 * @brief Enables debugging. This will print stats of the generated source code.
			 * @param state The state
			 * @return void
			 */
			void
			enableDebugging (bool state) noexcept
			{
				if ( state )
				{
					this->enableFlag(Debug);
				}
				else
				{
					this->disableFlag(Debug);
				}
			}

			/**
			 * @brief Returns whether the debugging is enabled.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			debuggingEnabled () const noexcept
			{
				return this->isFlagEnabled(Debug);
			}

			/**
			 * @brief Returns whether the expensive lighting branches are selected (see GeneratorFlagBits::HighQualityEnabled).
			 * @note Lighting always uses the single Cook-Torrance per-fragment model; this flag only
			 * gates its costlier branches (Fresnel-gated reflection, refraction, POM) and is meant to
			 * follow rendering distance. Nothing drives it down yet, so it is currently always true
			 * for generated scene-rendering programs.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			highQualityEnabled () const noexcept
			{
				return this->isFlagEnabled(HighQualityEnabled);
			}

			/**
			 * @brief Returns the maximum number of Parallax Occlusion Mapping iterations.
			 * @return int
			 */
			[[nodiscard]]
			int
			pomIterations () const noexcept
			{
				return m_pomIterations;
			}

			/**
			 * @brief Sets the maximum number of Parallax Occlusion Mapping iterations.
			 * @param iterations The max iteration count (0 to disable POM, otherwise clamped to [4, 64]).
			 * @return void
			 */
			void
			setPOMIterations (int iterations) noexcept
			{
				m_pomIterations = (iterations <= 0) ? 0 : std::clamp(iterations, 4, 64);
			}

			/**
			 * @brief Returns whether the renderable is using instancing.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isInstancingEnabled () const noexcept
			{
				return this->isFlagEnabled(IsInstancingEnabled);
			}

			/**
			 * @brief Returns whether the instanced VBO carries the previous model matrix (motion vectors).
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isInstanceMotionHistoryEnabled () const noexcept
			{
				return this->isFlagEnabled(IsInstanceMotionHistoryEnabled);
			}

			/**
			 * @brief Returns whether the renderable uses the infinity view (translation-free).
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isUsingInfinityView () const noexcept
			{
				return this->isFlagEnabled(IsUsingInfinityView);
			}

			/**
			 * @brief Returns whether the renderable is facing the camera like sprites.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isRenderableFacingCamera () const noexcept
			{
				return this->isFlagEnabled(IsRenderableFacingCamera);
			}

			/**
			 * @brief Returns whether bindless textures are enabled for this generator.
			 * @note When enabled, materials with automatic reflection will use the global
			 * bindless texture arrays instead of per-material descriptor sets.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			bindlessTexturesEnabled () const noexcept
			{
				return this->isFlagEnabled(BindlessTexturesEnabled);
			}

			/**
			 * @brief Enables bindless textures for this generator.
			 * @note When enabled, materials with automatic reflection will use the global
			 * bindless texture arrays instead of per-material descriptor sets.
			 * @param state The state.
			 * @return void
			 */
			void
			enableBindlessTextures (bool state) noexcept
			{
				if ( state )
				{
					this->enableFlag(BindlessTexturesEnabled);
				}
				else
				{
					this->disableFlag(BindlessTexturesEnabled);
				}
			}

			/**
			 * @brief Enables Multi-Draw Indirect mode for this generator.
			 * @note When enabled, model matrices are read from an SSBO via BDA + gl_DrawID
			 * instead of push constants. Requires GL_EXT_buffer_reference.
			 * @param state The state.
			 * @return void
			 */
			void
			enableMultiDrawIndirect (bool state) noexcept
			{
				if ( state )
				{
					this->enableFlag(IsMultiDrawIndirectEnabled);
				}
				else
				{
					this->disableFlag(IsMultiDrawIndirectEnabled);
				}
			}

			/**
			 * @brief Returns whether Multi-Draw Indirect mode is enabled.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isMultiDrawIndirectEnabled () const noexcept
			{
				return this->isFlagEnabled(IsMultiDrawIndirectEnabled);
			}

			/**
			 * @brief Returns whether skeletal animation is enabled for this generator.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isSkeletalAnimationEnabled () const noexcept
			{
				return this->isFlagEnabled(IsSkeletalAnimationEnabled);
			}

			/**
			 * @brief Returns the render target pointer.
			 * @note Returns a reference to the internal shared_ptr (no atomic refcount bump) since
			 * this is called repeatedly by generator subclasses (SceneRendering, ShadowCasting, ...)
			 * during a single shader generation pass. Safe: no subclass overrides this accessor.
			 * @return const std::shared_ptr< const Graphics::RenderTarget::Abstract > &
			 */
			[[nodiscard]]
			virtual
			const std::shared_ptr< const Graphics::RenderTarget::Abstract > &
			renderTarget () const noexcept
			{
				return m_renderTarget;
			}

			/**
			 * @brief Returns whether the generator has access to the renderable instance.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isRenderableInstanceAvailable () const noexcept
			{
				return m_renderableInstance != nullptr;
			}

			/**
			 * @brief Returns the renderable instance pointer.
			 * @note Can be nullptr, check with isRenderableInstanceAvailable() first.
			 * @return const Graphics::RenderableInstance::Abstract *
			 */
			[[nodiscard]]
			const Graphics::RenderableInstance::Abstract *
			getRenderableInstance () const noexcept
			{
				return m_renderableInstance.get();
			}

			/**
			 * @brief Returns the renderable interface pointer.
			 * @return const Graphics::Renderable::Abstract * A null pointer if no renderable instance is available.
			 */
			[[nodiscard]]
			const Graphics::Renderable::Abstract *
			getRenderable () const noexcept
			{
				if ( !this->isRenderableInstanceAvailable() )
				{
					return nullptr;
				}

				return m_renderableInstance->renderable();
			}

			/**
			 * @brief Returns the geometry interface pointer.
			 * @param LODIndex The desired LOD level. Default 0.
			 * @return const Graphics::Geometry::Interface *
			 */
			[[nodiscard]]
			const Graphics::Geometry::Interface *
			getGeometryInterface (uint32_t LODIndex = 0) const noexcept
			{
				if ( !this->isRenderableInstanceAvailable() )
				{
					return nullptr;
				}

				return m_renderableInstance->renderable()->geometry(LODIndex);
			}

			/**
			 * @brief Returns whether the generator will use a material.
			 * @note The material is provided by the constructor with a renderable instance.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			materialEnabled () const noexcept
			{
				if ( !this->isRenderableInstanceAvailable() )
				{
					return false;
				}

				return m_renderableInstance->renderable()->material(m_layerIndex) != nullptr;
			}

			/**
			 * @brief Returns the renderable instance layer index being in use.
			 * @return uint32_t The layer index passed to the constructor (0 when no renderable instance is available).
			 */
			[[nodiscard]]
			uint32_t
			layerIndex () const noexcept
			{
				return m_layerIndex;
			}

			/**
			 * @brief Returns the material interface pointer.
			 * @warning Can be a null pointer, check with GraphicsShaderGenerator::materialEnabled().
			 * @return const Graphics::Material::Interface *
			 */
			[[nodiscard]]
			const Graphics::Material::Interface *
			getMaterialInterface () const noexcept
			{
				if ( !this->isRenderableInstanceAvailable() )
				{
					return nullptr;
				}

				return m_renderableInstance->renderable()->material(m_layerIndex);
			}

			/**
			 * @brief Returns the shader program.
			 * @note Returns a reference to the internal shared_ptr (no atomic refcount bump): this
			 * is chained (e.g. `this->shaderProgram()->vertexShader()->...`) many times per shader
			 * generation pass across LightGenerator, StandardResource and the generator subclasses.
			 * @return const std::shared_ptr< Program > &
			 */
			[[nodiscard]]
			const std::shared_ptr< Program > &
			shaderProgram () const noexcept
			{
				return m_shaderProgram;
			}

			/**
			 * @brief Returns the next available location for shaders.
			 * @param increment The increment value for the next location. Default 1.
			 * @return uint32_t
			 */
			[[nodiscard]]
			uint32_t getNextShaderVariableLocation (uint32_t increment = 1) noexcept;

			/**
			 * @brief Declares the view uniform block according to input information.
			 * @note The block layout depends on the render target kind (regular / cubemap / cascaded
			 * shadow map). Reads m_shaderProgram->setIndex() and the render target: call only from
			 * onGenerateShadersCode() or later, after generateShaderProgram() has constructed the
			 * program and run prepareUniformSets() — calling it any earlier dereferences a null
			 * m_shaderProgram.
			 * @param shader A reference to a shader where to declare the uniform block.
			 * @param binding The binding point number. Default 0.
			 * @return bool
			 */
			[[nodiscard]]
			bool declareViewUniformBlock (AbstractShader & shader, uint32_t binding = 0) const noexcept;

			/**
			 * @brief Declares the material uniform block according to input information.
			 * @note Same call-timing contract as declareViewUniformBlock(): m_shaderProgram must
			 * already exist and have its set indexes prepared.
			 * @param material A reference to a material.
			 * @param shader A reference to a shader where to declare the uniform block.
			 * @param binding The binding point number. Default 0.
			 * @return bool
			 */
			[[nodiscard]]
			bool declareMaterialUniformBlock (const Graphics::Material::Interface & material, AbstractShader & shader, uint32_t binding = 0) const noexcept;

			/**
			 * @brief Declares the push constant block carrying the transform matrices, according to
			 * the render target kind and the shader program's instancing/MDI/advanced-matrices state.
			 * @note Same call-timing contract as declareViewUniformBlock(): m_shaderProgram must
			 * already exist. The exact set of members pushed (M, V, VP, or an MDI buffer-device
			 * address) varies by configuration to stay within the 128-byte Vulkan minimum guarantee
			 * for maxPushConstantsSize; see the implementation for the decision table.
			 * @param shader A reference to a shader where to declare the uniform block.
			 * @return bool
			 */
			[[nodiscard]]
			bool declareMatrixPushConstantBlock (AbstractShader & shader) const noexcept;

			/**
			 * @brief Generates the shader program.
			 * @note Looks up computeProgramCacheKey() in the renderer's program cache first. On a hit,
			 * the cached program's Set 1 (material) descriptor layout hash is checked against the
			 * current material as a safety net against key collisions; on a mismatch the mismatch is
			 * logged and the program is regenerated instead of reused. On success this sets
			 * shaderProgram() and, on an actual (re)generation, registers it in the cache.
			 * @param renderer A reference to the graphics renderer.
			 * @param GLSLVersion The GLSL version in use. Default "460".
			 * @param GLSLProfile The GLSL profile in use. Default "core".
			 * @return bool
			 */
			[[nodiscard]]
			bool generateShaderProgram (Graphics::Renderer & renderer, const std::string & GLSLVersion = DefaultGLSLVersion, const std::string & GLSLProfile = DefaultGLSLProfile) noexcept;

			/**
			 * @brief Sets an override framebuffer for pipeline creation.
			 * @note When set, createGraphicsPipeline() uses this framebuffer's render pass
			 * and sample count (forced to 1x) instead of the main render target's.
			 * This is a workaround for cases where the pipeline must target a different
			 * render pass than the one provided by the render target (e.g. overlay rendered
			 * in a single-sample post-process pass while the render target is MSAA).
			 * @todo Replace this override mechanism by introducing a dedicated RenderTarget
			 * for the post-process pass. The generator would then simply receive the correct
			 * render target, eliminating the need for conditional branching in createGraphicsPipeline().
			 * @param framebuffer A pointer to the override framebuffer (nullptr to disable).
			 * @return void
			 */
			void
			setPipelineFramebuffer (const Vulkan::Framebuffer * framebuffer) noexcept
			{
				m_pipelineFramebuffer = framebuffer;
			}

			/**
			 * @brief Returns the pipeline framebuffer override.
			 * @note See setPipelineFramebuffer() for the intent and future direction.
			 * @return const Vulkan::Framebuffer *
			 */
			[[nodiscard]]
			const Vulkan::Framebuffer *
			pipelineFramebuffer () const noexcept
			{
				return m_pipelineFramebuffer;
			}

			/**
			 * @brief Computes a unique cache key for the shader program configuration.
			 * @note This allows early lookup before shader generation to avoid redundant work.
			 * @warning Implementations must fold in flags() (see GeneratorFlagBits) along with every
			 * other input that changes the generated GLSL (render pass, material layout/flags,
			 * layer index, ...); an input left out of the hash lets two structurally different
			 * programs collide on the same key and one silently reuses the other's pipeline.
			 * @return size_t The unique hash identifying this program configuration.
			 */
			[[nodiscard]]
			virtual size_t computeProgramCacheKey () const noexcept = 0;

		protected:

			/**
			 * @brief Constructs an abstract shader program generator without a renderable instance.
			 * @note No renderable-instance-derived flag (instancing, lighting, skeletal animation, ...)
			 * is set by this overload; isRenderableInstanceAvailable() will return false.
			 * @param shaderProgramName A string for the program being generated [std::move].
			 * @param renderTarget A reference to a renderTarget smart pointer.
			 */
			Abstract (std::string shaderProgramName, const std::shared_ptr< const Graphics::RenderTarget::Abstract > & renderTarget) noexcept
				: NameableTrait{std::move(shaderProgramName)},
				m_renderTarget{renderTarget}
			{

			}

			/**
			 * @brief Constructs an abstract shader program generator with a specified renderable instance.
			 * @note Derives several GeneratorFlagBits straight from \p renderableInstance and its
			 * renderable: IsInstancingEnabled (+ IsInstanceMotionHistoryEnabled) from its per-instance
			 * VBO usage, IsRenderableFacingCamera from Renderable::Abstract::isSprite(),
			 * IsUsingInfinityView from its infinity-view state, IsLightingEnabled from its lighting
			 * state, and IsSkeletalAnimationEnabled when the renderable exposes skeletal data
			 * (SkeletalDataTrait). \p renderableInstance is dereferenced unconditionally: passing a
			 * null pointer is undefined behaviour.
			 * @param shaderProgramName A string for the program being generated [std::move].
			 * @param renderTarget A reference to a renderTarget smart pointer.
			 * @param renderableInstance A reference to a renderable instance smart pointer.
			 * @param layerIndex The renderable instance layer targeted.
			 */
			Abstract (std::string shaderProgramName, const std::shared_ptr< const Graphics::RenderTarget::Abstract > & renderTarget, const std::shared_ptr< const Graphics::RenderableInstance::Abstract > & renderableInstance, uint32_t layerIndex) noexcept
				: NameableTrait{std::move(shaderProgramName)},
				m_renderTarget{renderTarget},
				m_renderableInstance{renderableInstance},
				m_layerIndex{layerIndex}
			{
				if ( renderableInstance->useModelVertexBufferObject() )
				{
					this->enableFlag(IsInstancingEnabled);

					/* Motion history extends the per-instance VBO stride — the shader and
					 * the vertex buffer format must both know it. */
					if ( renderableInstance->isFlagEnabled(Graphics::RenderableInstance::EnableInstanceMotionHistory) )
					{
						this->enableFlag(IsInstanceMotionHistoryEnabled);
					}
				}

				if ( renderableInstance->renderable()->isSprite() )
				{
					this->enableFlag(IsRenderableFacingCamera);
				}

				/* NOTE: The infinity view drops the camera translation, so a velocity built
				 * from the REGULAR previous view-projection would be wrong by that translation
				 * -- a structural mismatch that does not cancel even on a static camera.
				 * The flag reaches the program cache key through flags(). */
				if ( renderableInstance->isUsingInfinityView() )
				{
					this->enableFlag(IsUsingInfinityView);
				}

				if ( renderableInstance->isLightingEnabled() )
				{
					this->enableFlag(IsLightingEnabled);
				}

				if ( const auto * skeletalData = dynamic_cast< const Graphics::Renderable::SkeletalDataTrait * >(renderableInstance->renderable()) )
				{
					if ( skeletalData->hasSkeletalData() )
					{
						this->enableFlag(IsSkeletalAnimationEnabled);
					}
				}
			}

			/**
			 * @brief Constructs an abstract shader program generator with a generic geometry specification.
			 * @note For generators that draw immediate/ad-hoc geometry with no persistent
			 * RenderableInstance (gizmo, overlay, post-process full-screen quad): \p topology and
			 * \p geometryFlags are forwarded as-is to Program::createVertexBufferFormat() in
			 * generateShaderProgram() to build the vertex buffer format, since there is no
			 * Geometry::Interface to query it from. No renderable-instance-derived flag is set.
			 * @param shaderProgramName A string for the program being generated [std::move].
			 * @param renderTarget A reference to a renderTarget smart pointer.
			 * @param topology The primitive topology of the geometry to build the vertex buffer format for.
			 * @param geometryFlags Bitmask of geometry attribute flags (see Graphics::Geometry) describing that geometry.
			 */
			Abstract (std::string shaderProgramName, const std::shared_ptr< const Graphics::RenderTarget::Abstract > & renderTarget, Graphics::Topology topology, uint32_t geometryFlags) noexcept
				: NameableTrait{std::move(shaderProgramName)},
				m_renderTarget{renderTarget},
				m_topology{topology},
				m_geometryFlags{geometryFlags}
			{

			}

			/**
			 * @brief Generates a minimal placeholder vertex shader stage, available to subclasses
			 * when the regular shader generation path cannot be used.
			 * @note Only outputs clip-space position (from the view uniform block when the PerView
			 * set is enabled, otherwise the raw vertex position); carries no other varying.
			 * @todo When the PerView set is disabled, the position is passed through as clip-space
			 * with no transform at all; try to use at least a model-view matrix if one is available.
			 * @param program A reference to the program being constructed.
			 * @return bool
			 */
			[[nodiscard]]
			bool generateFallBackVertexShader (Program & program) noexcept;

			/**
			 * @brief Generates a minimal placeholder fragment shader stage, available to subclasses
			 * when the regular shader generation path cannot be used.
			 * @note Always outputs solid magenta (1, 0, 1, 1), the conventional "missing shader" color.
			 * @param program A reference to the program being constructed.
			 * @return bool
			 */
			[[nodiscard]]
			bool generateFallBackFragmentShader (Program & program) noexcept;

			/**
			 * @brief Prepares the uniform sets according to incoming rendering information.
			 * @note First step of generateShaderProgram(), called right after m_shaderProgram is
			 * constructed and before onGenerateShadersCode(): declareViewUniformBlock() and
			 * declareMaterialUniformBlock() rely on the set indexes this call fills in.
			 * @param setIndexes A reference to the set indexes structure.
			 * @return void
			 */
			virtual void prepareUniformSets (SetIndexes & setIndexes) noexcept = 0;

			/**
			 * @brief Main method to generate program shaders.
			 * @note Called by generateShaderProgram() right after prepareUniformSets(); this is
			 * where subclasses build the vertex/fragment (and any other) shader stages of \p program.
			 * @param program A reference to the program being constructed.
			 * @return bool
			 */
			[[nodiscard]]
			virtual bool onGenerateShadersCode (Program & program) noexcept = 0;

			/**
			 * @brief Methods to override for generating specific program layout.
			 * @note The render target descriptor set layout is already present. Called by
			 * createDataLayout() after onGenerateShadersCode() has produced the shader stages, so the
			 * generated code and the declared data layouts are guaranteed to agree.
			 * @param renderer A reference to the renderer.
			 * @param setIndexes A reference to a set indexes structure.
			 * @param descriptorSetLayouts A reference to as a list of descriptor set layouts to complete.
			 * @param pushConstantRanges A reference to as a list of push constant ranges to complete.
			 * @return bool
			 */
			[[nodiscard]]
			virtual bool onCreateDataLayouts (Graphics::Renderer & renderer, const SetIndexes & setIndexes, Base::StaticVector< std::shared_ptr< Vulkan::DescriptorSetLayout >, 6 > & descriptorSetLayouts, Base::StaticVector< VkPushConstantRange, 4 > & pushConstantRanges) noexcept = 0;

			/**
			 * @brief Configures the graphics pipeline from child shader generators.
			 * @note Called by createGraphicsPipeline() after the pipeline's shader stages, vertex
			 * input, input assembly, tessellation and multisample states are already configured;
			 * this is where subclasses add whatever remains (rasterization, depth/stencil, blending, ...).
			 * @param program A reference to the constructed program.
			 * @param graphicsPipeline A reference to the graphics pipeline to set up.
			 * @return bool
			 */
			[[nodiscard]]
			virtual bool onGraphicsPipelineConfiguration (const Program & program, Vulkan::GraphicsPipeline & graphicsPipeline) noexcept = 0;

			/**
			 * @brief Generates push constant ranges from push constant blocks code declaration.
			 * @todo: Check for a better way to use push constant from the right shader declaration to the right stage in vulkan, maybe store push constant at program level instead of shader.
			 * @param pushConstantBlocks A reference to a vector of push constant blocks.
			 * @param pushConstantRanges A reference to a vector of push constant ranges.
			 * @param stageFlags The Vulkan shader stage(s) the produced ranges apply to.
			 * @return void
			 */
			static void generatePushConstantRanges (const Base::StaticVector< Declaration::PushConstantBlock, 4 > & pushConstantBlocks, Base::StaticVector< VkPushConstantRange, 4 > & pushConstantRanges, VkShaderStageFlags stageFlags) noexcept;

		private:

			/**
			 * @brief Creates all data description layouts for the graphics pipeline.
			 * @param renderer A reference to the renderer.
			 * @return bool
			 */
			[[nodiscard]]
			bool createDataLayout (Graphics::Renderer & renderer) noexcept;

			/**
			 * @brief Creates the final Vulkan graphics pipeline.
			 * @warning FIXME: The viewport state is configured from the render target's extent
			 * (width/height) purely to obtain those two values; this could become a dynamic
			 * pipeline state instead.
			 * @param renderer A reference to the renderer.
			 * @return bool
			 */
			[[nodiscard]]
			bool createGraphicsPipeline (Graphics::Renderer & renderer) noexcept;

			std::shared_ptr< const Graphics::RenderTarget::Abstract > m_renderTarget;
			std::shared_ptr< const Graphics::RenderableInstance::Abstract > m_renderableInstance;
			uint32_t m_layerIndex{0};
			Graphics::Topology m_topology{Graphics::Topology::TriangleList};
			uint32_t m_geometryFlags{0};
			std::shared_ptr< Program > m_shaderProgram;
			const Vulkan::Framebuffer * m_pipelineFramebuffer{nullptr}; /**< @todo Remove when a dedicated post-process RenderTarget exists. */
			uint32_t m_nextShaderVariableLocation{0};
			int m_pomIterations{16};
	};
}
