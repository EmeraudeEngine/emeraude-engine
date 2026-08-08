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
 * https://github.com/EmeraudeEngine/emeraude-engine
 *
 * --- THIS IS AUTOMATICALLY GENERATED, DO NOT CHANGE ---
 */

#pragma once

/* Project configuration. */
#include "emeraude_export.hpp"

/* STL inclusions. */
#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>
#include <mutex>
#include <source_location>
#include <string>
#include <vector>

/* Local inclusions for inheritances. */
#include "FlagTrait.hpp"

/* Local inclusions for usages. */
#include "Graphics/BindlessTextureManager.hpp"
#include "Graphics/Renderable/Abstract.hpp"
#include "Graphics/SkinnedGeometryProcessor.hpp"
#include "Graphics/Types.hpp"
#include "Math/CartesianFrame.hpp"
#include "Math/Matrix.hpp"
#include "RenderContext.hpp"
#include "RenderStateTracker.hpp"
#include "Vulkan/AccelerationStructureBuilder.hpp"
#include "Vulkan/DescriptorPool.hpp"
#include "Vulkan/DescriptorSet.hpp"
#include "Vulkan/ShaderStorageBufferObject.hpp"

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
		class DeferredDestructor;
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
		class SceneInstanceTransforms;
	}
}

namespace EmEn::Graphics::RenderableInstance
{
	constexpr uint32_t MatrixBytes{Matrix4Alignment * sizeof(float)};
	constexpr bool MergePushConstants{true};

	/** @brief Renderable instance flag bits. */
	enum EMEN_API RenderableInstanceFlagBits : uint32_t // NOLINT(performance-enum-size): designed for growth — uint32_t reserves bit headroom for future flag additions.
	{
		None = 0U,
		/**
		 * @brief This flag is set when all positions (GPU instancing) are up to date.
		 * @note Used by Multiple to avoid redundant VBO uploads when local data hasn't changed.
		 */
		ArePositionsSynchronized = 1U << 0,
		/** @brief This flag is set when the renderable instance can't be loaded in the rendering system and must be removed. */
		BrokenState = 1U << 1,
		/** @brief This flag is set when the renderable instance needs to generate a shader with lighting code. */
		EnableLighting = 1U << 2,
		/** @brief This flag disables shadow casting (the instance won't be rendered in shadow maps). */
		DisableShadowCasting = 1U << 3,
		/** @brief This flag disables shadow receiving (the instance won't sample shadow maps during rendering). */
		DisableShadowReceiving = 1U << 4,
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
		DisableLightDistanceCheck = 1U << 12,
		/**
		 * @brief This flag extends the instanced (Multiple) mesh VBO with the previous model
		 * matrix per instance (+4 vec4), for motion vectors on DYNAMIC instanced renderables.
		 * @note Must be set at construction (it fixes the VBO stride). updateLocalData() copies
		 * the current model matrix into the previous slot before overwriting it (one history
		 * step per logic update). Meaningless on Unique and on sprites.
		 */
		EnableInstanceMotionHistory = 1U << 13
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
	 * @extends EmEn::Base::FlagTrait Provides flag-based state management (see RenderableInstanceFlagBits).
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
	class EMEN_API Abstract : public std::enable_shared_from_this< Abstract >, public Base::FlagTrait< uint32_t >
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
			 * @note GPU-visible skinning/RT resources are routed through the renderer's
			 * deferred destructor: an instance can die at runtime (actor death) while
			 * command buffers referencing them are still in flight.
			 */
			~Abstract () override;

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
			 * @brief Sets the lighting code generation state in shaders.
			 * @note The symmetric form of enableLighting(), for callers that materialize
			 * meshes of both kinds and carry the choice as data — a mesh whose lighting is
			 * already baked into its vertex colors on unlit materials must stay OFF, or the
			 * ambient/IBL pass (scaled by the background luminance) multiplies it by the sky.
			 * @param state The lighting state.
			 * @return Abstract *
			 */
			Abstract *
			setLightingState (bool state) noexcept
			{
				if ( state )
				{
					this->enableFlag(EnableLighting);
				}
				else
				{
					this->disableFlag(EnableLighting);
				}

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
			 * @brief Creates GPU resources for skeletal skinning (SSBO, descriptor pool, descriptor sets).
			 * @note The SSBO is split into @a sectionCount sections (one per frame in flight), each
			 * with its own descriptor set on a fixed offset/range. The pose is uploaded ONCE per
			 * rendered frame into the section of the frame being recorded, so the shadow pass and
			 * the scene pass of a same frame always skin with the SAME pose — a single-copy SSBO
			 * written from the logic thread used to desynchronize them (whole-body self-shadowing
			 * flicker on fast animations).
			 * @param device A reference to the Vulkan device.
			 * @param descriptorSetLayout The descriptor set layout (must match the pipeline layout).
			 * @param boneCount The number of bone matrices.
			 * @param sectionCount The number of buffer sections, i.e. Renderer::framesInFlight().
			 * @return bool
			 */
			bool createSkinningResources (const std::shared_ptr< Vulkan::Device > & device, const std::shared_ptr< Vulkan::DescriptorSetLayout > & descriptorSetLayout, uint32_t boneCount, uint32_t sectionCount) noexcept;

			/**
			 * @brief Stages the skinning matrices for the next rendered frame (CPU side only).
			 * @note Called from the logic thread. The GPU upload is deferred to the render thread
			 * (flushSkinningMatrices()), into the section of the frame being recorded.
			 * @param matrices The skinning matrices to stage.
			 * @return bool
			 */
			/* NOTE: Non-const — archives the previous pose for the motion-vectors double
			 * skinning (the staging interleaves {current, previous} matrices, stride 2). */
			bool updateSkinningMatrices (const std::vector< Base::Math::Matrix< 4, float > > & matrices) noexcept;

			/**
			 * @brief Returns whether skinning GPU resources are available.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			hasSkinningResources () const noexcept
			{
				return !m_skinningDescriptorSets.empty();
			}

			/**
			 * @brief Sets the frame cursor used to select the skinning SSBO section of the frame
			 * being recorded and to deduplicate the per-frame pose upload.
			 * @note Called ONCE per frame by the Renderer, on the render thread, before any
			 * command recording. Read on the same thread by flushSkinningMatrices().
			 * @param frameCursor A monotonic rendered-frame counter (NOT the cyclic in-flight index).
			 * @return void
			 */
			static
			void
			setSkinningFrameCursor (uint64_t frameCursor) noexcept
			{
				s_skinningFrameCursor = frameCursor;
			}

			/**
			 * @brief Creates the per-instance ray-tracing resources for skinned geometry:
			 * the skinned-mirror vertex buffer, the refit-able BLAS and its update scratch.
			 * @details The BLAS is initially built from the SOURCE VBO (bind pose — valid
			 * data; the mirror holds garbage until the first compute dispatch), then refit
			 * every frame from the mirror buffer. Idempotent: returns true when already created.
			 * @note Requires skinning resources (bone matrices SSBO) and an indexed or
			 * non-indexed TriangleList geometry. Two instances sharing the same geometry
			 * each get their own mirror and BLAS (different poses).
			 * @param builder A reference to the renderer's acceleration structure builder.
			 * @param geometry A reference to the instance's geometry.
			 * @return bool
			 */
			[[nodiscard]]
			bool createRTSkinnedGeometryResources (Vulkan::AccelerationStructureBuilder & builder, const Geometry::Interface & geometry) noexcept;

			/**
			 * @brief Returns whether the per-instance RT skinned geometry resources exist.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			hasRTSkinnedGeometry () const noexcept
			{
				return m_rtSkinnedBLAS != nullptr;
			}

			/**
			 * @brief Returns the per-instance refit-able BLAS (skinned geometry), or nullptr.
			 * @return const Vulkan::AccelerationStructure *
			 */
			[[nodiscard]]
			const Vulkan::AccelerationStructure *
			rtSkinnedBLAS () const noexcept
			{
				return m_rtSkinnedBLAS.get();
			}

			/**
			 * @brief Returns the BLAS refit geometry inputs (vertex data = mirror buffer).
			 * @return const std::vector< Vulkan::BLASGeometryInput > &
			 */
			[[nodiscard]]
			const std::vector< Vulkan::BLASGeometryInput > &
			rtRefitInputs () const noexcept
			{
				return m_rtRefitInputs;
			}

			/**
			 * @brief Returns the aligned device address of the BLAS update scratch buffer.
			 * @return VkDeviceAddress
			 */
			[[nodiscard]]
			VkDeviceAddress
			rtRefitScratchAddress () const noexcept
			{
				return m_rtRefitScratchAddress;
			}

			/**
			 * @brief Returns the device address of the skinned-mirror vertex buffer.
			 * @note This is what GPUMeshMetaData.vertexBufferAddress must point to for
			 * skinned instances (RT hit shading reads the CURRENT pose, not the bind pose).
			 * @return VkDeviceAddress
			 */
			[[nodiscard]]
			VkDeviceAddress
			rtMirrorBufferAddress () const noexcept
			{
				return m_rtSkinningPushConstants.dstAddress;
			}

			/**
			 * @brief Returns the ready-made push constants for the skinning mirror dispatch.
			 * @return const SkinnedGeometryProcessor::PushConstants &
			 */
			[[nodiscard]]
			const SkinnedGeometryProcessor::PushConstants &
			rtSkinningPushConstants () const noexcept
			{
				return m_rtSkinningPushConstants;
			}

			/**
			 * @brief Uploads the staged pose into the SSBO section of the frame being recorded,
			 * once per frame (deduplicated on the frame cursor), and selects the descriptor set
			 * to bind. Render thread only, called by the bind sites (scene, shadow, TBN passes)
			 * and by the RT skinned-mirror recording (SceneMetaData::recordTLASBuild) — the
			 * dedup guarantees RT and raster skin with the exact same pose.
			 * @note Uploading once per frame is the WHOLE fix: every pass of a frame then reads
			 * the same section, whatever the logic thread stages meanwhile.
			 * @return const Vulkan::DescriptorSet *
			 */
			[[nodiscard]]
			const Vulkan::DescriptorSet * flushSkinningMatrices () const noexcept;

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
			bool getReadyForRender (const Scenes::Scene & scene, const std::shared_ptr< RenderTarget::Abstract > & renderTarget, const Base::StaticVector< RenderPassType, MaxPassCount > & renderPassTypes, Renderer & renderer) noexcept;

			/**
			 * @brief Ensures TBN space debug programs are generated and cached for this instance.
			 * @note Called on-demand when TBN rendering is requested, since the flag may be
			 * enabled after the initial getReadyForRender() call.
			 * @param renderTarget The render target to prepare for.
			 * @param renderer The renderer for shader compilation.
			 * @return True if TBN programs are ready, false on failure.
			 */
			[[nodiscard]]
			bool getReadyForTBNSpace (const std::shared_ptr< RenderTarget::Abstract > & renderTarget, Renderer & renderer) noexcept;

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
			setTransformationMatrix (const Base::Math::Matrix< 4, float > & transformationMatrix) noexcept
			{
				m_transformationMatrix = transformationMatrix;

				this->enableFlag(ApplyTransformationMatrix);
			}

			/**
			 * @brief Returns the local transformation matrix.
			 * @return const Base::Math::Matrix< 4, float > &
			 */
			[[nodiscard]]
			const Base::Math::Matrix< 4, float > &
			transformationMatrix () const noexcept
			{
				return m_transformationMatrix;
			}

			/**
			 * @brief Stages this instance's transforms into the scene instance transforms SSBO and retains the slot.
			 * @note Non-instanced path only (instanced renderables carry their model matrices in a VBO).
			 * Called by Scene::prepareRender() during render list population; the retained slot is
			 * consumed by the draws recorded until the next prepareRender(). The model matrix
			 * computation mirrors Unique::pushMatricesForRendering().
			 * @param instanceTransforms A reference to the scene instance transforms manager.
			 * @param worldCoordinates A pointer to the world coordinates of the instance. nullptr means origin.
			 * @param cameraPosition A reference to the camera world position (sprite billboard orientation).
			 * @param advanceHistory Whether this staging advances the model matrix history
			 * (motion vectors). Only the PRIMARY view target staging advances it — one advance
			 * per rendered frame, so previousModel is the matrix of the previous rendered frame.
			 * Render-to-texture stagings must pass false.
			 * @return void
			 */
			void stageInstanceTransforms (Scenes::SceneInstanceTransforms & instanceTransforms, const Base::Math::CartesianFrame< float > * worldCoordinates, const Base::Math::Vector< 3, float > & cameraPosition, bool advanceHistory) noexcept;

			/**
			 * @brief Returns the instance transforms SSBO slot staged for the current render pass.
			 * @note Only meaningful on the non-instanced path, between two Scene::prepareRender() calls.
			 * @return uint32_t
			 */
			[[nodiscard]]
			uint32_t
			instanceTransformsSlot () const noexcept
			{
				return m_instanceTransformsSlot;
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
			 * @param LODLevel The desired LOD level. Default 0.
			 *
			 * @note Shadow maps use depth-only rendering without material/lighting bindings.
			 *
			 * @see render() For full scene rendering with materials.
			 * @see pushMatricesForShadowCasting() For push constant strategy.
			 * @version 0.8.35
			 */
			void castShadows (uint32_t readStateIndex, const std::shared_ptr< RenderTarget::Abstract > & renderTarget, uint32_t layerIndex, const Base::Math::CartesianFrame< float > * worldCoordinates, const Vulkan::CommandBuffer & commandBuffer, uint32_t LODLevel = 0) const noexcept;

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
			 * @param LODLevel The desired LOD level. Default 0.
			 * @param bindlessTexturesManager A pointer to the bindless textures manager for materials using automatic reflection. Can be nullptr.
			 *
			 * @todo The lightEmitter parameter should be refactored to use a smart pointer for safety.
			 *
			 * @see castShadows() For shadow map rendering.
			 * @see renderTBNSpace() For debug visualization.
			 * @version 0.8.35
			 */
			void render (uint32_t readStateIndex, const std::shared_ptr< RenderTarget::Abstract > & renderTarget, const Scenes::Component::AbstractLightEmitter * lightEmitter, RenderPassType renderPassType, uint32_t layerIndex, const Base::Math::CartesianFrame< float > * worldCoordinates, const Vulkan::CommandBuffer & commandBuffer, uint32_t LODLevel = 0, const BindlessTextureManager * bindlessTexturesManager = nullptr, const Vulkan::DescriptorSet * sceneTransformsDS = nullptr) const noexcept;

			/**
			 * @brief Draws the instance in a render target with state tracking to skip redundant binds.
			 *
			 * Same as render() but checks a RenderStateTracker before each Vulkan bind command,
			 * skipping redundant pipeline, geometry, and descriptor set binds when the state
			 * matches the previous draw call.
			 *
			 * When pipeline changes, all descriptor set tracking is invalidated (Vulkan
			 * descriptor set binding validity depends on pipeline layout compatibility).
			 *
			 * @param readStateIndex The render state-valid index to read data (for double/triple buffering).
			 * @param renderTarget A reference to the render target smart pointer.
			 * @param lightEmitter A pointer to an optional light emitter. Can be nullptr for unlit rendering.
			 * @param renderPassType The render pass type into the render target.
			 * @param layerIndex The renderable layer index (for multi-layer materials).
			 * @param worldCoordinates A pointer to the world coordinates of the instance. nullptr means origin.
			 * @param commandBuffer A reference to the command buffer recording draw commands.
			 * @param tracker A reference to the state tracker for redundant bind elimination.
			 * @param LODLevel The geometry LOD level to render.
			 * @param bindlessTexturesManager A pointer to the bindless texture manager. Can be nullptr.
			 *
			 * @see render() For the non-tracked version.
			 */
			void render (uint32_t readStateIndex, const std::shared_ptr< RenderTarget::Abstract > & renderTarget, const Scenes::Component::AbstractLightEmitter * lightEmitter, RenderPassType renderPassType, uint32_t layerIndex, const Base::Math::CartesianFrame< float > * worldCoordinates, const Vulkan::CommandBuffer & commandBuffer, RenderStateTracker & tracker, uint32_t LODLevel = 0, const BindlessTextureManager * bindlessTexturesManager = nullptr, const Vulkan::DescriptorSet * sceneTransformsDS = nullptr) const noexcept;

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
			void renderTBNSpace (uint32_t readStateIndex, const std::shared_ptr< RenderTarget::Abstract > & renderTarget, uint32_t layerIndex, const Base::Math::CartesianFrame< float > * worldCoordinates, const Vulkan::CommandBuffer & commandBuffer) const noexcept;

			/**
			 * @brief Prepares MDI shader program variants for this instance.
			 * @param scene The scene context.
			 * @param renderTarget The render target.
			 * @param renderer The renderer.
			 * @return True if MDI programs were generated successfully.
			 */
			[[nodiscard]]
			bool getReadyForMDI (const Scenes::Scene & scene, const std::shared_ptr< RenderTarget::Abstract > & renderTarget, Renderer & renderer) noexcept;

			/**
			 * @brief Resolves the MDI program variant for this instance.
			 * @param renderTarget The render target.
			 * @param layerIndex The material layer index.
			 * @return The resolved MDI program, or nullptr if not found.
			 */
			[[nodiscard]]
			std::shared_ptr< Saphir::Program > resolveMDIProgram (const std::shared_ptr< RenderTarget::Abstract > & renderTarget, uint32_t layerIndex) const noexcept;

			/**
			 * @brief Returns whether this instance is animated with frames.
			 * @note Scans EVERY layer, not just layer 0: a multi-layer mesh may animate any subset
			 * of its layers, and keying on layer 0 alone left the whole instance un-ticked whenever
			 * the first material happened to be static — which is the common case for a mesh whose
			 * layers are ordered by texture name.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isAnimated () const noexcept
			{
				const auto renderable = this->renderable();

				if ( !renderable->isReadyForInstantiation() )
				{
					return false;
				}

				const auto layerCount = renderable->layerCount();

				for ( uint32_t layerIndex = 0; layerIndex < layerCount; ++layerIndex )
				{
					if ( const auto * material = renderable->material(layerIndex); material != nullptr && material->isAnimated() )
					{
						return true;
					}
				}

				return false;
			}

			/**
			 * @brief Stores the animation time; the frame index itself is resolved PER LAYER at draw.
			 * @note ⚠️ The instance keeps the TIME, not a resolved index. Each layer's flipbook has
			 * its own frame count — a Doom map mixes 2, 3 and 4-frame animations in one mesh — so a
			 * single shared index would sample out of range on the shorter ones. Resolution happens
			 * in frameIndexFor(), called once per layer while the push constants are written.
			 * @param sceneTimeMS The current scene time.
			 * @return void
			 */
			void
			updateFrameIndex (uint32_t sceneTimeMS) noexcept
			{
				m_animationTimeMS = sceneTimeMS;
			}

			/**
			 * @brief Returns the frame index of a given layer at the stored animation time.
			 * @param layerIndex The material layer index.
			 * @return uint32_t Zero for a static layer, which is also its only valid index.
			 */
			[[nodiscard]]
			uint32_t
			frameIndexFor (uint32_t layerIndex) const noexcept
			{
				const auto * material = this->renderable()->material(layerIndex);

				if ( material == nullptr || !material->isAnimated() )
				{
					return 0;
				}

				return material->frameIndexAt(m_animationTimeMS);
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
			virtual void pushMatricesForShadowCasting (const RenderPassContext & passContext, const PushConstantContext & pushContext, const Base::Math::CartesianFrame< float > * worldCoordinates) const noexcept = 0;

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
			virtual void pushMatricesForRendering (const RenderPassContext & passContext, const PushConstantContext & pushContext, const Base::Math::CartesianFrame< float > * worldCoordinates) const noexcept = 0;

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
			 * @param LODLevel The desired LOD level.
			 * @return void
			 */
			virtual void bindInstanceModelLayer (const Vulkan::CommandBuffer & commandBuffer, uint32_t layerIndex, uint32_t LODLevel) const noexcept = 0;

			/**
			 * @brief Returns the animation time, in milliseconds, the frame indices derive from.
			 * @note NOT a frame index — that is per layer, see frameIndexFor().
			 * @return uint32_t
			 */
			[[nodiscard]]
			uint32_t
			animationTimeMS () const noexcept
			{
				return m_animationTimeMS;
			}

			/**
			 * @brief Mutex protecting local data access (e.g. VBO data in Multiple).
			 * @note Used to synchronize access between Logic thread (updating data) and Render thread (uploading to GPU).
			 */
			mutable std::mutex m_localDataAccess;

		private:

			/**
			 * @brief Reports ONCE that the sealed pipeline layout declares a descriptor set the
			 * instance cannot provide, which means the generation-time condition and the
			 * binding-time condition of that set have diverged.
			 * @note Render thread only. The draw call is skipped by the caller: binding the
			 * following sets one slot lower would corrupt every set after the missing one.
			 * @param setName The name of the missing descriptor set.
			 * @param renderTarget A reference to the render target being recorded.
			 * @return void
			 */
			void traceMissingDescriptorSet (const char * setName, const RenderTarget::Abstract & renderTarget) const noexcept;

			/**
			 * @brief Returns whether the renderable declares skeletal data but the instance does
			 * not own its skinning descriptor sets yet.
			 * @note The program cache lives on the RENDERABLE (shared by every instance of the
			 * same mesh), the skinning descriptor sets on the INSTANCE: this is what keeps a
			 * second instance from being declared ready on the first one's cached program.
			 * @return bool
			 */
			[[nodiscard]]
			bool isMissingSkinningResources () const noexcept;

			/**
			 * @brief Creates the skeletal skinning GPU resources if the renderable declares
			 * skeletal data and the instance does not own them yet.
			 * @note MUST be called before any program generation: the pipeline layout seals the
			 * PerModel set on the RENDERABLE (SkeletalDataTrait::hasSkeletalData()), while the
			 * command recording binds it on the INSTANCE (hasSkinningResources()). Creating the
			 * resources here — render thread, renderable ready — makes both the same instant.
			 * The animator itself stays with the Scenes::Component::Visual component; until it
			 * uploads a first pose, the SSBO sections hold identity matrices (bind pose).
			 * @param renderer A reference to the graphics renderer.
			 * @return bool
			 */
			bool prepareSkinningResources (Renderer & renderer) noexcept;

			/**
			 * @brief Builds a program cache key for this instance's current configuration.
			 * @param programType The type of program.
			 * @param renderPassType The render pass type.
			 * @param renderPassHandle The Vulkan render pass handle for pipeline compatibility.
			 * @param layerIndex The material layer index.
			 * @param isMDIEnabled
			 * @return Renderable::ProgramCacheKey
			 */
			[[nodiscard]]
			Renderable::ProgramCacheKey buildProgramCacheKey (Renderable::ProgramType programType, RenderPassType renderPassType, uint64_t renderPassHandle, uint32_t layerIndex, bool isMDIEnabled = false) const noexcept;

			/**
			 * @brief Resolves a program from the fast instance-local cache or falls back to the Renderable cache.
			 * @param renderTarget A reference to the render target.
			 * @param cacheKey The program cache key.
			 * @return std::shared_ptr< Saphir::Program > The resolved program, or nullptr if not found.
			 */
			[[nodiscard]]
			std::shared_ptr< Saphir::Program > resolveProgram (const std::shared_ptr< RenderTarget::Abstract > & renderTarget, const Renderable::ProgramCacheKey & cacheKey) const noexcept;

			/** @brief Entry in the instance-local resolved program cache. */
			struct ResolvedProgram final
			{
				uint64_t renderTargetId{0};
				uint64_t renderPassHandle{0};
				Renderable::ProgramType programType{Renderable::ProgramType::Rendering};
				RenderPassType renderPassType{RenderPassType::SimplePass};
				uint32_t layerIndex{0};
				std::shared_ptr< Saphir::Program > program;
			};

			/** @brief Maximum number of resolved program entries per instance. */
			static constexpr size_t MaxResolvedPrograms{16};

			const std::shared_ptr< Renderable::Abstract > m_renderable;
			Base::Math::Matrix< 4, float > m_transformationMatrix;
			/** @brief Model matrix staged at the previous rendered frame (primary view), for motion vectors. */
			Base::Math::Matrix< 4, float > m_lastModelMatrix;
			/** @brief Instance-local resolved program cache (typically 2-5 entries, linear scan). */
			mutable Base::StaticVector< ResolvedProgram, MaxResolvedPrograms > m_resolvedPrograms;
			uint32_t m_animationTimeMS{0}; /**< Animation time in ms; the per-layer frame index is derived from it. */
			/** @brief Instance transforms SSBO slot staged for the current render pass (non-instanced path). */
			uint32_t m_instanceTransformsSlot{0};
			/* Skeletal skinning GPU resources (per-instance).
			 * The SSBO holds one section per frame in flight; each descriptor set targets its
			 * section (fixed offset/range, same layout). See createSkinningResources(). */
			std::unique_ptr< Vulkan::ShaderStorageBufferObject > m_skinningSSBO;
			/** @brief Previous-pose bone matrices (motion vectors double skinning). */
			std::vector< Base::Math::Matrix< 4, float > > m_previousSkinningMatrices;
			/** @brief Interleaved {current, previous} staging reused across updates (guarded by m_skinningStagingMutex). */
			std::vector< Base::Math::Matrix< 4, float > > m_skinningStaging;
			std::shared_ptr< Vulkan::DescriptorPool > m_skinningDescriptorPool;
			/** @brief One descriptor set per SSBO section (frame in flight). */
			std::vector< std::unique_ptr< Vulkan::DescriptorSet > > m_skinningDescriptorSets;
			/** @brief Protects m_skinningStaging between the logic thread (staging) and the render thread (upload). */
			mutable std::mutex m_skinningStagingMutex;
			/* Ray-tracing skinned geometry resources (per-instance).
			 * The mirror buffer receives the compute-skinned vertices (same layout as the
			 * source VBO) and feeds both the per-frame BLAS refit and the RT hit shading. */
			std::unique_ptr< Vulkan::Buffer > m_rtSkinnedMirrorBuffer;
			std::unique_ptr< Vulkan::Buffer > m_rtRefitScratchBuffer;
			std::unique_ptr< Vulkan::AccelerationStructure > m_rtSkinnedBLAS;
			/** @brief Refit geometry inputs (vertex data = mirror buffer), fixed at creation. */
			std::vector< Vulkan::BLASGeometryInput > m_rtRefitInputs;
			/** @brief Ready-made dispatch description for the skinning mirror compute pass. */
			SkinnedGeometryProcessor::PushConstants m_rtSkinningPushConstants;
			VkDeviceAddress m_rtRefitScratchAddress{0};
			/** @brief Frame cursor of the last pose upload (render thread only). */
			mutable uint64_t m_skinningUploadedFrame{std::numeric_limits< uint64_t >::max()};
			/** @brief Aligned byte size of one SSBO section. */
			VkDeviceSize m_skinningSectionSize{0};
			/** @brief Byte size of one pose (boneCount x 2 matrices). */
			VkDeviceSize m_skinningPoseSize{0};
			/** @brief SSBO section bound for the frame being recorded (render thread only). */
			mutable uint32_t m_skinningBoundSection{0};
			/** @brief Rendered-frame cursor, set once per frame by the Renderer (render thread only). */
			static uint64_t s_skinningFrameCursor;
			/** @brief The renderer's deferred destructor (set with the skinning resources):
			 * the destructor retires the GPU-visible resources through it, because an
			 * instance can die at runtime while frames referencing them are in flight. */
			Vulkan::DeferredDestructor * m_deferredDestructor{nullptr};
			/** @brief Whether m_lastModelMatrix holds a valid previous-frame matrix (false until the first primary staging). */
			bool m_hasModelHistory{false};
			/** @brief Whether the descriptor set contract violation was already reported (render thread only, anti-spam). */
			mutable bool m_missingDescriptorSetReported{false};
	};
}
