/*
 * src/Graphics/RenderableInstance/Abstract.hpp
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
 * https://github.com/londnoir/emeraude-engine
 *
 * --- THIS IS AUTOMATICALLY GENERATED, DO NOT CHANGE ---
 */

#pragma once

/* STL inclusions. */
#include <cstddef>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include "Libs/std_source_location.hpp"

/* Local inclusions for inheritances. */
#include "Libs/FlagTrait.hpp"

/* Local inclusions for usages. */
#include "Graphics/Renderable/Abstract.hpp"
#include "Graphics/Types.hpp"
#include "Libs/Math/CartesianFrame.hpp"
#include "RenderContext.hpp"
#include "Graphics/BindlessTextureManager.hpp"

/* Forward declarations. */
namespace EmEn
{
	namespace Graphics
	{
		namespace RenderTarget
		{
			class Abstract;
		}

		class Renderer;
		class ViewMatricesInterface;
	}

	namespace Vulkan
	{
		class PipelineLayout;
		class GraphicsPipeline;
		class CommandBuffer;
	}

	namespace Saphir
	{
		class Program;
	}

	namespace Scenes
	{
		namespace Component
		{
			class AbstractLightEmitter;
		}

		class Scene;
	}
}

namespace EmEn::Graphics::RenderableInstance
{
	constexpr uint32_t MatrixBytes{Matrix4Alignment * sizeof(float)};
	constexpr bool MergePushConstants{true};

	/** @brief Renderable instance flag bits. */
	enum RenderableInstanceFlagBits : uint32_t
	{
		None = 0U,
		/**
		 * @brief This flag is set when the renderable instance is ready to be rendered in a 3D scene.
		 * @warning This is different from the renderable flag "IsReadyForInstantiation"!
		 */
		IsReadyToRender = 1U << 0,
		/**
		 * @brief This flag is set when all positions (GPU instancing) are up to date.
		 * @note Used by Multiple to avoid redundant VBO uploads when local data hasn't changed.
		 */
		ArePositionsSynchronized = 1U << 1,
		/** @brief This flag is set when the renderable instance can't be loaded in the rendering system and must be removed. */
		BrokenState = 1U << 2,
		/** @brief This flag is set when the renderable instance needs to generate a shader with lighting code. */
		EnableLighting = 1U << 3,
		/** @brief This flag disables shadow casting (the instance won't be rendered in shadow maps). */
		DisableShadowCasting = 1U << 4,
		/** @brief This flag disables shadow receiving (the instance won't sample shadow maps during rendering). */
		DisableShadowReceiving = 1U << 13,
		/** @brief This flag is set to update the renderable instance model matrix with rotations only. Useful for sky rendering. */
		UseInfinityView = 1U << 5,
		/**
		 * @brief This flag tells the renderer to not read the depth buffer when drawing this renderable instance.
		 * @todo Convert to Vulkan 1.3 dynamic state (VK_DYNAMIC_STATE_DEPTH_TEST_ENABLE) to avoid pipeline duplication.
		 */
		DisableDepthTest = 1U << 6,
		/**
		 * @brief This flag tells the renderer to not write in the depth buffer when drawing this renderable instance.
		 * @todo Convert to Vulkan 1.3 dynamic state (VK_DYNAMIC_STATE_DEPTH_WRITE_ENABLE) to avoid pipeline duplication.
		 */
		DisableDepthWrite = 1U << 7,
		/**
		 * @brief This flag tells the renderer to not read the stencil buffer when drawing this renderable instance.
		 * @todo Convert to Vulkan 1.3 dynamic state (VK_DYNAMIC_STATE_STENCIL_TEST_ENABLE) to avoid pipeline duplication.
		 * @note Currently unused.
		 */
		DisableStencilTest = 1U << 8,
		/**
		 * @brief This flag tells the renderer to not write in the stencil buffer when drawing this renderable instance.
		 * @todo Convert to Vulkan 1.3 dynamic state (VK_DYNAMIC_STATE_STENCIL_OP) to avoid pipeline duplication.
		 * @note Currently unused.
		 */
		DisableStencilWrite = 1U << 9,
		/** @brief [DEBUG] This flag tells the renderer to display tangent space vectors on the render instance. */
		DisplayTBNSpaceEnabled = 1U << 10,
		/** @brief This flag tells the renderable instance to need an extra transformation matrix to be applied. */
		ApplyTransformationMatrix = 1U << 11,
		/** @brief This flag tells disabling the light distance check. */
		DisableLightDistanceCheck = 1U << 12
	};

	/**
	 * @brief Defines the base of a renderable instance to draw any object in a scene.
	 *
	 * A renderable instance represents a specific instantiation of a Renderable::Interface
	 * ready for drawing in the scene. It holds render state, transformation matrices, and
	 * per-render-target shader program configurations.
	 *
	 * Key responsibilities:
	 * - Manages shader program generation for shadow casting and scene rendering
	 * - Handles push constant configuration for different rendering modes
	 * - Supports GPU instancing, skeletal animation, and multi-layer materials
	 * - Provides TBN space visualization for debugging
	 *
	 * @extends std::enable_shared_from_this Allows safe shared_ptr creation from this pointer.
	 * @extends EmEn::Libs::FlagTrait Provides flag-based state management (see RenderableInstanceFlagBits).
	 *
	 * @note Thread safety: This class uses internal mutex locking for GPU memory access.
	 * @note Clarification needed: The necessity of mutex locks (m_GPUMemoryAccess) is unclear
	 *	   and marked with [VULKAN-CPU-SYNC] in the implementation.
	 *
	 * @todo Check for renderable interface already in video memory to reduce preparation time.
	 *
	 * @see Renderable::Interface The underlying renderable data (geometry, materials).
	 * @see RenderTarget::Abstract The destination for rendering operations.
	 * @see Unique For single-instance rendering.
	 * @see Multiple For GPU-instanced rendering with multiple instances.
	 * @version 0.8.35
	 */
	class Abstract : public std::enable_shared_from_this< Abstract >, public Libs::FlagTrait< uint32_t >
	{
		public:

			/**
			 * @brief Copy constructor.
			 * @param copy A reference to the copied instance.
			 */
			Abstract (const Abstract & copy) noexcept = delete;

			/**
			 * @brief Move constructor.
			 * @param copy A reference to the copied instance.
			 */
			Abstract (Abstract && copy) noexcept = delete;

			/**
			 * @brief Copy assignment.
			 * @param copy A reference to the copied instance.
			 * @return Abstract &
			 */
			Abstract & operator= (const Abstract & copy) noexcept = delete;

			/**
			 * @brief Move assignment.
			 * @param copy A reference to the copied instance.
			 * @return Abstract &
			 */
			Abstract & operator= (Abstract && copy) noexcept = delete;

			/**
			 * @brief Destructs the renderable instance.
			 */
			~Abstract () override = default;

			/**
			 * @brief Returns whether this instance is ready to cast shadows.
			 * @param renderTarget A reference to a render target smart pointer.
			 * @return bool
			 */
			[[nodiscard]]
			bool isReadyToCastShadows (const std::shared_ptr< RenderTarget::Abstract > & renderTarget) const noexcept;

			/**
			 * @brief Returns whether this instance is ready for rendering.
			 * @param renderTarget A reference to a render target smart pointer.
			 * @return bool
			 */
			[[nodiscard]]
			bool isReadyToRender (const std::shared_ptr< RenderTarget::Abstract > & renderTarget) const noexcept;

			/**
			 * @brief Returns whether this renderable instance is unable to get ready for rendering.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isBroken () const noexcept
			{
				return this->isFlagEnabled(BrokenState);
			}

			/**
			 * @brief Enables the lighting code generation in shaders.
			 * @return Abstract *
			 */
			Abstract *
			enableLighting () noexcept
			{
				this->enableFlag(EnableLighting);

				return this;
			}

			/**
			 * @brief Returns whether the lighting code generation is enabled in shaders.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isLightingEnabled () const noexcept
			{
				return this->isFlagEnabled(EnableLighting);
			}

			/**
			 * @brief Disables shadow casting for this instance (won't be rendered in shadow maps).
			 * @return Abstract *
			 */
			Abstract *
			disableShadowCasting () noexcept
			{
				this->enableFlag(DisableShadowCasting);

				return this;
			}

			/**
			 * @brief Returns whether shadow casting is disabled for this instance.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isShadowCastingDisabled () const noexcept
			{
				return this->isFlagEnabled(DisableShadowCasting);
			}

			/**
			 * @brief Returns whether shadow casting is enabled for this instance.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isShadowCastingEnabled () const noexcept
			{
				return this->isFlagDisabled(DisableShadowCasting);
			}

			/**
			 * @brief Disables shadow receiving for this instance (won't sample shadow maps during rendering).
			 * @return Abstract *
			 */
			Abstract *
			disableShadowReceiving () noexcept
			{
				this->enableFlag(DisableShadowReceiving);

				return this;
			}

			/**
			 * @brief Returns whether shadow receiving is disabled for this instance.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isShadowReceivingDisabled () const noexcept
			{
				return this->isFlagEnabled(DisableShadowReceiving);
			}

			/**
			 * @brief Returns whether shadow receiving is enabled for this instance.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isShadowReceivingEnabled () const noexcept
			{
				return this->isFlagDisabled(DisableShadowReceiving);
			}

			/**
			 * @brief Defines whether the instance should be rendered with the infinite view matrix.
			 * @param state The state.
			 * @return Abstract *
			 */
			Abstract *
			setUseInfinityView (bool state) noexcept
			{
				if ( state )
				{
					this->enableFlag(UseInfinityView);
				}
				else
				{
					this->disableFlag(UseInfinityView);
				}

				return this;
			}

			/**
			 * @brief Returns whether the instance should be rendered with the infinite view matrix.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isUsingInfinityView () const noexcept
			{
				return this->isFlagEnabled(UseInfinityView);
			}

			/**
			 * @brief Disables the depth test with this instance.
			 * @param state The state.
			 * @return Abstract *
			 */
			Abstract *
			disableDepthTest (bool state) noexcept
			{
				if ( state )
				{
					this->enableFlag(DisableDepthTest);
				}
				else
				{
					this->disableFlag(DisableDepthTest);
				}

				return this;
			}

			/**
			 * @brief Returns whether the depth test is disabled with this instance.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isDepthTestDisabled () const noexcept
			{
				return this->isFlagEnabled(DisableDepthTest);
			}

			/**
			 * @brief Disables the depth writing with this instance.
			 * @param state The state.
			 * @return Abstract *
			 */
			Abstract *
			disableDepthWrite (bool state) noexcept
			{
				if ( state )
				{
					this->enableFlag(DisableDepthWrite);
				}
				else
				{
					this->disableFlag(DisableDepthWrite);
				}

				return this;
			}

			/**
			 * @brief Returns whether the depth writes is disabled with this instance.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isDepthWriteDisabled () const noexcept
			{
				return this->isFlagEnabled(DisableDepthWrite);
			}

			/**
			 * @brief Enables the display of TBN space.
			 * @param state The state.
			 * @return Abstract *
			 */
			Abstract *
			enableDisplayTBNSpace (bool state) noexcept
			{
				if ( state )
				{
					this->enableFlag(DisplayTBNSpaceEnabled);
				}
				else
				{
					this->disableFlag(DisplayTBNSpaceEnabled);
				}

				return this;
			}

			/**
			 * @brief Returns whether the display of TBN space is enabled.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isDisplayTBNSpaceEnabled () const noexcept
			{
				return this->isFlagEnabled(DisplayTBNSpaceEnabled);
			}

			/**
			 * @brief Disables the light distance check.
			 * @return Abstract *
			 */
			Abstract *
			disableLightDistanceCheck () noexcept
			{
				this->enableFlag(DisableLightDistanceCheck);

				return this;
			}

			/**
			 * @brief Returns the light distance check is disabled.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isLightDistanceCheckDisabled () const noexcept
			{
				return this->isFlagEnabled(DisableLightDistanceCheck);
			}

			/**
			 * @brief Returns the light distance check is enabled.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isLightDistanceCheckEnabled () const noexcept
			{
				return this->isFlagDisabled(DisableLightDistanceCheck);
			}

			/**
			 * @brief Returns the renderable pointer.
			 * @return const Renderable::Interface *
			 */
			[[nodiscard]]
			const Renderable::Abstract *
			renderable () const noexcept
			{
				return m_renderable.get();
			}

			/**
			 * @brief Prepares the renderable instance for shadow casting.
			 *
			 * Generates shadow casting shader programs for each material layer.
			 * Programs are cached per render target.
			 *
			 * @param renderTarget A reference to the shadow map render target.
			 * @param renderer A writable reference to the graphics renderer for shader generation.
			 * @return true if preparation succeeded or is pending (renderable not ready yet).
			 * @return false if an error occurred (instance marked as broken).
			 *
			 * @note Returns true immediately if renderable is not ready for instantiation.
			 *	   A loading event will trigger another call when ready.
			 *
			 * @see castShadows() To render after preparation.
			 * @version 0.8.35
			 */
			[[nodiscard]]
			bool getReadyForShadowCasting (const std::shared_ptr< RenderTarget::Abstract > & renderTarget, Renderer & renderer) noexcept;

			/**
			 * @brief Prepares the renderable instance for scene rendering.
			 *
			 * Generates shader programs for each requested render pass type and material layer.
			 * Programs are cached per render target.
			 *
			 * @param scene A reference to the scene (for lighting and environment info).
			 * @param renderTarget A reference to the render target.
			 * @param renderPassTypes A list of render pass types to prepare (e.g., Opaque, Transparent).
			 * @param renderer A writable reference to the graphics renderer for shader generation.
			 * @return true if preparation succeeded or is pending (renderable not ready yet).
			 * @return false if an error occurred (instance marked as broken).
			 *
			 * @note Returns true immediately if renderable is not ready for instantiation.
			 *	   A loading event will trigger another call when ready.
			 * @note Also generates TBN space visualization program if DisplayTBNSpaceEnabled flag is set.
			 *
			 * @see render() To render after preparation.
			 * @see getReadyForShadowCasting() For shadow map preparation.
			 * @version 0.8.35
			 */
			[[nodiscard]]
			bool getReadyForRender (const Scenes::Scene & scene, const std::shared_ptr< RenderTarget::Abstract > & renderTarget, const Libs::StaticVector< RenderPassType, MaxPassCount > & renderPassTypes, Renderer & renderer) noexcept;

			/**
			 * @brief Sets the renderable instance broken from a child class.
			 * @note This is the release version.
			 * @return void
			 */
			void
			setBroken () noexcept
			{
				this->enableFlag(BrokenState);
			}

			/**
			 * @brief Sets the renderable instance broken from a child class.
			 * @note This is the debug version.
			 * @param errorMessage Trace an error message.
			 * @param location If a message has to be traced, this passes the location. Default auto-generated by the
			 * mighty C++ STL.
			 * @return void
			 */
			void setBroken (const std::string & errorMessage, const std::source_location & location = std::source_location::current()) noexcept;

			/**
			 * @brief Sets a local transformation matrix to apply just before render.
			 * @param transformationMatrix A reference to a matrix 4x4.
			 * @return void
			 */
			void
			setTransformationMatrix (const Libs::Math::Matrix< 4, float > & transformationMatrix) noexcept
			{
				m_transformationMatrix = transformationMatrix;

				this->enableFlag(ApplyTransformationMatrix);
			}

			/**
			 * @brief Returns the local transformation matrix.
			 * @return const Libs::Math::Matrix< 4, float > &
			 */
			[[nodiscard]]
			const Libs::Math::Matrix< 4, float > &
			transformationMatrix () const noexcept
			{
				return m_transformationMatrix;
			}

			/**
			 * @brief Draws the instance into a shadow map.
			 *
			 * Renders the instance for shadow casting using a simplified pipeline:
			 * 1. Binds the shadow casting graphics pipeline
			 * 2. Optionally binds view UBO for GPU instancing
			 * 3. Configures push constants via pushMatricesForShadowCasting()
			 * 4. Issues the draw command
			 *
			 * @param readStateIndex The render state-valid index to read data (for double/triple buffering).
			 * @param renderTarget A reference to the shadow map render target.
			 * @param layerIndex The renderable layer index (for multi-layer materials).
			 * @param worldCoordinates A pointer to the world coordinates of the instance. nullptr means origin.
			 * @param commandBuffer A reference to the command buffer recording draw commands.
			 *
			 * @note Shadow maps use depth-only rendering without material/lighting bindings.
			 *
			 * @see render() For full scene rendering with materials.
			 * @see pushMatricesForShadowCasting() For push constant strategy.
			 * @version 0.8.35
			 */
			void castShadows (uint32_t readStateIndex, const std::shared_ptr< RenderTarget::Abstract > & renderTarget, uint32_t layerIndex, const Libs::Math::CartesianFrame< float > * worldCoordinates, const Vulkan::CommandBuffer & commandBuffer) const noexcept;

			/**
			 * @brief Draws the instance in a render target.
			 *
			 * Performs the full render pipeline for this instance:
			 * 1. Binds the graphics pipeline and instance resources
			 * 2. Configures push constants via pushMatricesForRendering()
			 * 3. Binds view, light, and material descriptor sets
			 * 4. Issues the draw command
			 *
			 * @param readStateIndex The render state-valid index to read data (for double/triple buffering).
			 * @param renderTarget A reference to the render target smart pointer.
			 * @param lightEmitter A pointer to an optional light emitter. Can be nullptr for unlit rendering.
			 * @param renderPassType The render pass type into the render target.
			 * @param layerIndex The renderable layer index (for multi-layer materials).
			 * @param worldCoordinates A pointer to the world coordinates of the instance. nullptr means origin.
			 * @param commandBuffer A reference to the command buffer recording draw commands.
		 * @param bindlessTexturesManager A pointer to the bindless textures manager for materials using automatic reflection. Can be nullptr.
			 *
			 * @todo The lightEmitter parameter should be refactored to use a smart pointer for safety.
			 *
			 * @see castShadows() For shadow map rendering.
			 * @see renderTBNSpace() For debug visualization.
			 * @version 0.8.35
			 */
			void render (uint32_t readStateIndex, const std::shared_ptr< RenderTarget::Abstract > & renderTarget, const Scenes::Component::AbstractLightEmitter * lightEmitter, RenderPassType renderPassType, uint32_t layerIndex, const Libs::Math::CartesianFrame< float > * worldCoordinates, const Vulkan::CommandBuffer & commandBuffer, const BindlessTextureManager * bindlessTexturesManager = nullptr) const noexcept;

			/**
			 * @brief Renders the Tangent-Bitangent-Normal space vectors for debugging.
			 *
			 * Draws colored lines representing the TBN vectors at each vertex:
			 * - Red: Tangent vector
			 * - Green: Bitangent vector
			 * - Blue: Normal vector
			 *
			 * @param readStateIndex The render state-valid index to read data (for double/triple buffering).
			 * @param renderTarget A reference to the render target smart pointer.
			 * @param layerIndex The renderable layer index.
			 * @param worldCoordinates A pointer to the world coordinates of the instance. nullptr means origin.
			 * @param commandBuffer A reference to the command buffer recording draw commands.
			 *
			 * @note Only available when DisplayTBNSpaceEnabled flag is set.
			 * @note Useful for debugging normal mapping and lighting issues.
			 *
			 * @see enableDisplayTBNSpace() To enable this visualization.
			 * @version 0.8.35
			 */
			void renderTBNSpace (uint32_t readStateIndex, const std::shared_ptr< RenderTarget::Abstract > & renderTarget, uint32_t layerIndex, const Libs::Math::CartesianFrame< float > * worldCoordinates, const Vulkan::CommandBuffer & commandBuffer) const noexcept;

			/**
			 * @brief Returns whether this instance is animated with frames.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isAnimated () const noexcept
			{
				if ( !this->renderable()->isReadyForInstantiation() )
				{
					return false;
				}

				return this->renderable()->material(0)->isAnimated();
			}

			/**
			 * @brief Updates the frame animation.
			 * @param sceneTimeMS The current scene time.
			 * @return void
			 */
			void
			updateFrameIndex (uint32_t sceneTimeMS) noexcept
			{
				m_frameIndex = this->renderable()->material(0)->frameIndexAt(sceneTimeMS);
			}

			/**
			 * @brief Returns whether the instance uses a uniform buffer object for the model matrices.
			 * @return bool
			 */
			[[nodiscard]]
			virtual bool useModelUniformBufferObject () const noexcept = 0;

			/**
			 * @brief Returns whether the instance uses a vertex buffer object for the model matrices.
			 * @return bool
			 */
			[[nodiscard]]
			virtual bool useModelVertexBufferObject () const noexcept = 0;

		protected:

			/**
			 * @brief Constructs a renderable instance.
			 * @param renderable A reference to a renderable interface smart pointer.
			 * @param flagBits The renderable instance level flags.
			 */
			Abstract (const std::shared_ptr< Renderable::Abstract > & renderable, uint32_t flagBits) noexcept
				: FlagTrait{flagBits},
				m_renderable{renderable}
			{
			}

			/**
			 * @brief Configures push constants for shadow casting.
			 *
			 * Each subclass implements a different strategy based on how it stores Model matrices:
			 * - **Unique**: Computes M from worldCoordinates, pushes M (cubemap) or MVP (classic)
			 * - **Multiple**: M is in VBO, pushes VP (classic) or nothing (cubemap)
			 *
			 * @param passContext The render pass context (command buffer, view matrices, cubemap flag).
			 * @param pushContext The push constant context (pipeline layout, stage flags, shader options).
			 * @param worldCoordinates World coordinates of the instance. nullptr means origin.
			 *
			 * @see RenderPassContext::isCubemap For cubemap vs classic rendering detection.
			 */
			virtual void pushMatricesForShadowCasting (const RenderPassContext & passContext, const PushConstantContext & pushContext, const Libs::Math::CartesianFrame< float > * worldCoordinates) const noexcept = 0;

			/**
			 * @brief Configures push constants for scene rendering.
			 *
			 * Each subclass implements a different strategy based on how it stores Model matrices:
			 * - **Unique**: Computes M from worldCoordinates, pushes M/V+M/MVP depending on mode
			 * - **Multiple**: M is in VBO, pushes V+VP/VP or nothing (cubemap)
			 *
			 * @param passContext The render pass context (command buffer, view matrices, cubemap flag).
			 * @param pushContext The push constant context (pipeline layout, stage flags, shader options).
			 * @param worldCoordinates World coordinates of the instance. nullptr means origin.
			 *
			 * @see PushConstantContext::useAdvancedMatrices For lighting mode.
			 * @see PushConstantContext::useBillboarding For sprite mode.
			 */
			virtual void
			pushMatricesForRendering (const RenderPassContext & passContext, const PushConstantContext & pushContext, const Libs::Math::CartesianFrame< float > * worldCoordinates) const noexcept = 0;

			/**
			 * @brief Returns the number of instances to draw.
			 * @note This is a more convenient named method than get the vertex count from the VBO.
			 * @return uint32_t
			 */
			[[nodiscard]]
			virtual uint32_t instanceCount () const noexcept = 0;

			/**
			 * @brief Returns whether model matrices are created in video memory.
			 * @return bool
			 */
			[[nodiscard]]
			virtual bool isModelMatricesCreated () const noexcept = 0;

			/**
			 * @brief Binds the renderable instance resources to a command buffer.
			 * @param commandBuffer A reference to a command buffer.
			 * @param layerIndex The current layer to bind.
			 * @return void
			 */
			virtual void bindInstanceModelLayer (const Vulkan::CommandBuffer & commandBuffer, uint32_t layerIndex) const noexcept = 0;

			/**
			 * @brief Mutex protecting local data access (e.g. VBO data in Multiple).
			 *
			 * @note Used to synchronize access between Logic thread (updating data) and Render thread (uploading to
			 * GPU).
			 */
			mutable std::mutex m_localDataAccess;

		private:

			/**
			 * @brief Builds a program cache key for this instance's current configuration.
			 * @param programType The type of program.
			 * @param renderPassType The render pass type.
			 * @param renderPassHandle The Vulkan render pass handle for pipeline compatibility.
			 * @param layerIndex The material layer index.
			 * @return Renderable::ProgramCacheKey
			 */
			[[nodiscard]]
			Renderable::ProgramCacheKey buildProgramCacheKey (Renderable::ProgramType programType, RenderPassType renderPassType, uint64_t renderPassHandle, uint32_t layerIndex) const noexcept;

			const std::shared_ptr< Renderable::Abstract > m_renderable;
			Libs::Math::Matrix< 4, float > m_transformationMatrix;
			uint32_t m_frameIndex{0};
	};
}
