/*
 * src/Graphics/RenderableInstance/Abstract.hpp
 * This file is part of Emeraude-Engine
 *
 * Copyright (C) 2010-2025 - Sébastien Léon Claude Christian Bémelmans "LondNoir" <londnoir@gmail.com>
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
#include <any>
#include <unordered_map>
#include <memory>
#include <string>
#include <vector>
#include "Libs/std_source_location.hpp"

/* Local inclusions for inheritances. */
#include "Libs/FlagTrait.hpp"
#include "Libs/ObservableTrait.hpp"
#include "Libs/ObserverTrait.hpp"

/* Local inclusions for usages. */
#include "Graphics/Renderable/Interface.hpp"
#include "Graphics/Types.hpp"
#include "RenderTargetProgramsInterface.hpp"

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
	constexpr bool MergePushConstants{true};

	/** @brief Renderable instance flag bits. */
	enum RenderableInstanceFlagBits : uint32_t
	{
		None = 0,
		/**
		 * @brief This flag is set when the renderable instance is ready to be rendered in a 3D scene.
		 * @warning This is different from the renderable flag "IsReadyForInstantiation"!
		 */
		IsReadyToRender = 1 << 0,
		/**
		 * @brief This flag is set when all positions (GPU instancing) are up to date.
		 * @todo Maybe this functionality is useless now, it was an old behavior to control update video memory buffer frequency.
		 */
		ArePositionsSynchronized = 1 << 1,
		/** @brief This flag is set when the renderable instance can't be loaded in the rendering system and must be removed. */
		BrokenState = 1 << 2,
		/** @brief This flag is set when the renderable instance uses a VBO to send locations (GPU instancing). */
		EnableInstancing = 1 << 3,
		/** @brief This flag is set when the renderable instance uses skeletal animation with an UBO. */
		EnableSkeletalAnimation = 1 << 4,
		/** @brief This flag is set when the renderable instance needs to generate a shader with lighting code. */
		EnableLighting = 1 << 5,
		/** @brief This flag is set when the renderable instance needs to generate a shader to cast/receive shadows. */
		EnableShadows = 1 << 6,
		/** @brief This flag is set to update the renderable instance model matrix with rotations only. Useful for sky rendering. */
		UseInfinityView = 1 << 7,
		/** @brief This flag is set to update the renderable instance model matrix to always face the camera. Useful for sprite rendering. */
		FacingCamera = 1 << 8,
		/** @brief This flag tells the renderer to not read the depth buffer when drawing this renderable instance. */
		DisableDepthTest = 1 << 9,
		/** @brief This flag tells the renderer to not write in the depth buffer when drawing this renderable instance. */
		DisableDepthWrite = 1 << 10,
		/** @brief This flag tells the renderer to not read the stencil buffer when drawing this renderable instance. */
		DisableStencilTest = 1 << 11,
		/** @brief This flag tells the renderer to not write in the stencil buffer when drawing this renderable instance. */
		DisableStencilWrite = 1 << 12,
		/** @brief [DEBUG] This flag tells the renderer to display tangent space vectors on the render instance. */
		DisplayTBNSpaceEnabled = 1 << 13,
		/** @brief This flag tells the renderable instance to need an extra transformation matrix to be applied. */
		ApplyTransformationMatrix = 1 << 14,
		/** @brief This flag tells disabling the light distance check. */
		DisableLightDistanceCheck = 1 << 15
	};

	/**
	 * @brief Defines the base of renderable instance to draw any object in a scene.
	 * @extends EmEn::Libs::FlagTrait A renderable instance is flag-able.
	 * @extends EmEn::Libs::ObserverTrait A renderable instance needs to observe the renderable loading.
	 * @extends EmEn::Libs::ObservableTrait A renderable instance is observable as well.
	 */
	class Abstract : public std::enable_shared_from_this< Abstract >, public Libs::FlagTrait< uint32_t >, public Libs::ObserverTrait, public Libs::ObservableTrait
	{
		public:

			/** @brief Observable class unique identifier. */
			static const size_t ClassUID;

			/** @brief Observable notification codes. */
			enum NotificationCode
			{
				/* Declares the renderable ready to go on GPU for rendering. */
				ReadyToSetupOnGPU,
				/* Enumeration boundary. */
				MaxEnum
			};

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

			/** @copydoc EmEn::Libs::ObservableTrait::classUID() const */
			[[nodiscard]]
			size_t
			classUID () const noexcept override
			{
				return ClassUID;
			}

			/** @copydoc EmEn::Libs::ObservableTrait::is() const */
			[[nodiscard]]
			bool
			is (size_t classUID) const noexcept override
			{
				return classUID == ClassUID;
			}

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
			 * @brief Returns whether the renderable instance instancing is enabled.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			instancingEnabled () const noexcept
			{
				return this->isFlagEnabled(EnableInstancing);
			}

			/**
			 * @brief Returns whether the renderable instance is using a skeletal animation uniform buffer.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			skeletalAnimationEnabled () const noexcept
			{
				return this->isFlagEnabled(EnableSkeletalAnimation);
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
			 * @brief Enables the shadowing code generation in shaders.
			 * @return Abstract *
			 */
			Abstract *
			enableShadows () noexcept
			{
				this->enableFlag(EnableShadows);

				return this;
			}

			/**
			 * @brief Returns whether the shadowing code generation is enabled in shaders.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isShadowsEnabled () const noexcept
			{
				return this->isFlagEnabled(EnableShadows);
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
			 * @brief Returns whether the instance should be rendered facing the camera.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isFacingCamera () const noexcept
			{
				return this->isFlagEnabled(FacingCamera);
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
			 * @brief Disables the depth write with this instance.
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
			 * @brief Returns the renderable pointer.
			 * @return const Renderable::Interface *
			 */
			[[nodiscard]]
			const Renderable::Interface *
			renderable () const noexcept
			{
				return m_renderable.get();
			}

			/**
			 * @brief Gets the renderable instance ready to render in a scene.
			 * @param renderTarget A reference to the render target smart pointer.
			 * @param renderer A writable reference to the graphics renderer.
			 * @return bool
			 */
			[[nodiscard]]
			bool getReadyForShadowCasting (const std::shared_ptr< RenderTarget::Abstract > & renderTarget, Renderer & renderer) noexcept;

			/**
			 * @brief Gets the renderable instance ready to render in a scene.
			 * @param scene A reference to the scene.
			 * @param renderTarget A reference to the render target smart pointer.
			 * @param renderPassTypes A reference to a list of requested render pass types.
			 * @param renderer A writable reference to the graphics renderer.
			 * @return bool
			 */
			[[nodiscard]]
			bool getReadyForRender (const Scenes::Scene & scene, const std::shared_ptr< RenderTarget::Abstract > & renderTarget, const std::vector< RenderPassType > & renderPassTypes, Renderer & renderer) noexcept;

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
			 * @param location If a message has to be traced, this passes the location. Default auto-generated by the mighty C++ STL.
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
			 * @brief Refresh graphics pipelines for a specific render target.
			 * @param renderTarget A reference to the render target smart pointer.
			 * @return bool
			 */
			bool refreshGraphicsPipelines (const std::shared_ptr< RenderTarget::Abstract > & renderTarget) noexcept;

			/**
			 * @brief Destroys graphics pipelines from the renderable instance for a specific render target.
			 * @param renderTarget A reference to the render target smart pointer.
			 * @return void
			 */
			void destroyGraphicsPipelines (const std::shared_ptr< RenderTarget::Abstract > & renderTarget) noexcept;

			/**
			 * @brief Draws the instance in a shadow map.
			 * @param renderTarget A reference to the render target smart pointer.
			 * @param layerIndex The renderable layer index.
			 * @param commandBuffer A reference to a command buffer.
			 * @return void
			 */
			void castShadows (const std::shared_ptr< RenderTarget::Abstract > & renderTarget, uint32_t layerIndex, const Vulkan::CommandBuffer & commandBuffer) const noexcept;

			/**
			 * @brief Draws the instance in a render target.
			 * @param renderTarget A reference to the render target smart pointer.
			 * @param lightEmitter A pointer to an optional light emitter. TODO: should be a smart pointer.
			 * @param renderPassType The render pass type into the render target.
			 * @param layerIndex The renderable layer index.
			 * @param commandBuffer A reference to a command buffer.
			 * @return void
			 */
			void render (const std::shared_ptr< RenderTarget::Abstract > & renderTarget, const Scenes::Component::AbstractLightEmitter * lightEmitter, RenderPassType renderPassType, uint32_t layerIndex, const Vulkan::CommandBuffer & commandBuffer) const noexcept;

			/**
			 * @brief Draws the TBN space over each vertex.
			 * @param renderTarget A reference to the render target smart pointer.
			 * @param layerIndex The renderable layer index.
			 * @param commandBuffer A reference to a command buffer.
			 * @return void
			 */
			void renderTBNSpace (const std::shared_ptr< RenderTarget::Abstract > & renderTarget, uint32_t layerIndex, const Vulkan::CommandBuffer & commandBuffer) const noexcept;

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
			 * @brief Returns the position where should be the renderable instance renderer in the world.
			 * @return Libs::Math::Vector< 3, float >
			 */
			[[nodiscard]]
			virtual Libs::Math::Vector< 3, float > worldPosition () const noexcept = 0;

		protected:

			/**
			 * @brief Constructs a renderable instance.
			 * @param renderable A reference to renderable interface smart pointer.
			 * @param flagBits The renderable instance level flags.
			 */
			Abstract (const std::shared_ptr< Renderable::Interface > & renderable, uint32_t flagBits) noexcept;

			/**
			 * @brief Configures push constant into the command buffer.
			 * @param commandBuffer A reference to the command buffer.
			 * @param pipelineLayout A reference to the pipeline layout.
			 * @param viewMatrices A reference to the view matrices.
			 * @param program A reference to the program.
			 * @return void
			 */
			virtual void pushMatrices (const Vulkan::CommandBuffer & commandBuffer, const Vulkan::PipelineLayout & pipelineLayout, const ViewMatricesInterface & viewMatrices, const Saphir::Program & program) const noexcept = 0;

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
			 * @brief Returns whether the instance use a uniform buffer object for the model matrices.
			 * @return bool
			 */
			[[nodiscard]]
			virtual bool useModelUniformBufferObject () const noexcept = 0;

			/**
			 * @brief Returns whether the instance use a vertex buffer object for the model matrices.
			 * @return bool
			 */
			[[nodiscard]]
			virtual bool useModelVertexBufferObject () const noexcept = 0;

			/**
			 * @brief Resets model matrices in video memory.
			 * @return void
			 */
			virtual void resetModelMatrices () noexcept = 0;

			/**
			 * @brief Binds the renderable instance resources to a command buffer.
			 * @param commandBuffer A reference to a command buffer.
			 * @param layerIndex The current layer to bind.
			 * @return void
			 */
			virtual void bindInstanceModelLayer (const Vulkan::CommandBuffer & commandBuffer, uint32_t layerIndex) const noexcept = 0;

			mutable std::mutex m_GPUMemoryAccess{};

		private:

			/** @copydoc EmEn::Libs::ObserverTrait::onNotification() */
			[[nodiscard]]
			bool onNotification (const ObservableTrait * observable, int notificationCode, const std::any & data) noexcept override;

			[[nodiscard]]
			RenderTargetProgramsInterface * getOrCreateRenderTargetProgramInterface (const std::shared_ptr< RenderTarget::Abstract > & renderTarget, uint32_t layerCount);

			Libs::Math::Matrix< 4, float > m_transformationMatrix;
			std::shared_ptr< Renderable::Interface > m_renderable;
			std::unordered_map< std::shared_ptr< const RenderTarget::Abstract >, std::unique_ptr< RenderTargetProgramsInterface > > m_renderTargetPrograms;
			uint32_t m_frameIndex{0};

	};
}
