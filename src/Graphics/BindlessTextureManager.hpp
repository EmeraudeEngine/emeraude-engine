/*
 * src/Graphics/BindlessTextureManager.hpp
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
#include <mutex>
#include <queue>

/* Third-party inclusions. */
#include <vulkan/vulkan.h>

/* Local inclusions for inheritances. */
#include "ServiceInterface.hpp"

namespace EmEn::Vulkan
{
	class Device;
	class DescriptorPool;
	class DescriptorSet;
	class DescriptorSetLayout;
	class TextureInterface;
}

namespace EmEn::Scenes
{
	class BindlessTextureSet;
}

namespace EmEn::Graphics
{
	class Renderer;

	/**
	 * @brief The bindless texture manager service.
	 * @note This manager provides a global descriptor set with arrays of textures
	 * that can be indexed dynamically in shaders using non-uniform indexing.
	 * @extends EmEn::ServiceInterface This is a service.
	 */
	class EMEN_API BindlessTextureManager final : public ServiceInterface
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"BindlessTextureManagerService"};

			/** @brief Reserved slots for global resources (IBL, environment, etc.).
			 * @warning Each slot is an index into ONE typed array: cube slots index
			 * texturesCube[] (TextureCubeBinding), 2D slots index textures2D[]
			 * (Texture2DBinding). The numbering restarts per array. */

			/* Cube array (texturesCube[]) reserved slots. */
			static constexpr uint32_t EnvironmentCubemapSlot = 0;
			static constexpr uint32_t IrradianceCubemapSlot = 1;
			static constexpr uint32_t PrefilteredCubemapSlot = 2;

			/* 2D array (textures2D[]) reserved slots. */
			static constexpr uint32_t BRDFLutSlot = 3;
			static constexpr uint32_t GrabPassSlot = 4;
			static constexpr uint32_t GrabPassDepthSlot = 5;

			/* First slot available for scene-dynamic textures (all arrays). */
			static constexpr uint32_t FirstDynamicSlot = 16;

			/** @brief Desired (uncapped) texture counts per type.
			 * @warning These are a TARGET, not the effective capacity. The descriptor table is
			 * sized at initialization from the device's update-after-bind budget — see
			 * computeCapacities() and the maxTextures*() accessors. Never use these constants to
			 * bound a slot index; use the accessors. */
			static constexpr uint32_t DesiredMaxTextures1D = 256;
			static constexpr uint32_t DesiredMaxTextures2D = 4096;
			static constexpr uint32_t DesiredMaxTextures3D = 256;
			static constexpr uint32_t DesiredMaxTexturesCube = 256;
			static constexpr uint32_t DesiredMaxTexturesCubeArray = 64;

			/** @brief Capacities used when the device budget cannot host the desired ones.
			 * @note The four secondary arrays take a fixed floor and the 2D array — by far the most
			 * used — absorbs whatever the budget leaves. On Apple/MoltenVK (budget 1024 samplers)
			 * this yields 32/768/32/128/32. */
			static constexpr uint32_t ReducedMaxTextures1D = 32;
			static constexpr uint32_t ReducedMaxTextures3D = 32;
			static constexpr uint32_t ReducedMaxTexturesCube = 128;
			static constexpr uint32_t ReducedMaxTexturesCubeArray = 32;

			/** @brief Absolute floor for any array: the reserved slots plus a few dynamic entries.
			 * @note A device that cannot host five arrays of this size cannot run the engine's
			 * bindless design at all; initialization fails loudly rather than rendering garbage. */
			static constexpr uint32_t MinTexturesPerArray = FirstDynamicSlot + 8;

			/** @brief Sampler budget left to the OTHER descriptor sets of a pipeline layout.
			 * @note The update-after-bind pipeline-layout VUIDs (03022, 03036) sum the sampler
			 * descriptors of EVERY set in pSetLayouts — including non-UAB ones such as the
			 * post-process input sets — so the bindless table must not claim the whole device
			 * budget. 32 covers the widest input set the engine builds today (SSR/RTGI resolve). */
			static constexpr uint32_t OtherSetsSamplerHeadroom = 32;

			/** @brief Binding points in the descriptor set layout. */
			static constexpr uint32_t Texture1DBinding = 0;
			static constexpr uint32_t Texture2DBinding = 1;
			static constexpr uint32_t Texture3DBinding = 2;
			static constexpr uint32_t TextureCubeBinding = 3;
			static constexpr uint32_t TextureCubeArrayBinding = 4;

			/**
			 * @brief Constructs a bindless textures manager service.
			 * @param renderer A reference to the graphics renderer.
			 */
			explicit BindlessTextureManager (Renderer & renderer) noexcept;

			/**
			 * @brief Destructs the bindless textures manager service.
			 * @note Declared here and defined out-of-line (not implicit): the class is exported
			 * (EMEN_API), which forces MSVC to instantiate the destructor at the class definition
			 * point in every TU. It destroys a std::unique_ptr< Vulkan::DescriptorSet > held by
			 * pointer to a forward-declared (incomplete) type, so the deleter needs the complete
			 * type — only visible in BindlessTextureManager.cpp. See docs/windows-export-api.md
			 * § "exported pimpl".
			 */
			~BindlessTextureManager () override;

			/**
			 * @brief Sets the device that will be used with this manager.
			 * @param device A reference to a device smart pointer.
			 * @return void
			 */
			/**
			 * @brief Sets the device that will be used with this manager.
			 * @param device A reference to a device smart pointer.
			 * @return void
			 */
			void setDevice (const std::shared_ptr< Vulkan::Device > & device) noexcept;

			/**
			 * @brief Returns the effective capacity of the 1D texture array.
			 * @note Valid after service initialization; before that, the desired capacity.
			 * @return uint32_t
			 */
			[[nodiscard]]
			uint32_t
			maxTextures1D () const noexcept
			{
				return m_maxTextures1D;
			}

			/**
			 * @brief Returns the effective capacity of the 2D texture array.
			 * @note Valid after service initialization; before that, the desired capacity.
			 * @return uint32_t
			 */
			[[nodiscard]]
			uint32_t
			maxTextures2D () const noexcept
			{
				return m_maxTextures2D;
			}

			/**
			 * @brief Returns the effective capacity of the 3D texture array.
			 * @note Valid after service initialization; before that, the desired capacity.
			 * @return uint32_t
			 */
			[[nodiscard]]
			uint32_t
			maxTextures3D () const noexcept
			{
				return m_maxTextures3D;
			}

			/**
			 * @brief Returns the effective capacity of the Cube texture array.
			 * @note Valid after service initialization; before that, the desired capacity.
			 * @return uint32_t
			 */
			[[nodiscard]]
			uint32_t
			maxTexturesCube () const noexcept
			{
				return m_maxTexturesCube;
			}

			/**
			 * @brief Returns the effective capacity of the CubeArray texture array.
			 * @note Valid after service initialization; before that, the desired capacity.
			 * @return uint32_t
			 */
			[[nodiscard]]
			uint32_t
			maxTexturesCubeArray () const noexcept
			{
				return m_maxTexturesCubeArray;
			}

			/**
			 * @brief Mirrors the active scene's bindless texture set into the descriptor table.
			 * @note This is the single entry point through which scene textures reach the GPU
			 * descriptor table. The manager READS the per-scene Scenes::BindlessTextureSet — the
			 * scene never writes the manager directly. Called every frame for the active scene
			 * (handles newly added textures and per-frame animated-texture frame swaps) and on
			 * scene activation. Dynamic slots not present in the set are simply left untouched;
			 * they are never sampled because materials/lights only reference occupied slots.
			 * @param set A reference to the active scene's bindless texture set.
			 * @param sceneTimeMS The active scene lifetime in milliseconds (for animated textures).
			 * @return void
			 */
			void syncTextureSet (const Scenes::BindlessTextureSet & set, uint32_t sceneTimeMS) const noexcept;

			/**
			 * @brief Clears, in the GPU descriptor table, the slots occupied by a scene's set.
			 * @note Called when a scene stops being active (Scenes::Manager::disableActiveScene),
			 * so the global descriptor set no longer references that scene's textures/samplers
			 * before they may be destroyed (otherwise vkDestroySampler/Image fire
			 * "currently in use by VkDescriptorSet"). Each freed dynamic slot is overwritten with
			 * an engine-owned dummy texture; the environment reserved slot is reset to the default
			 * cubemap. Hitch-free: NO device waitIdle here (overwriting bound descriptors while
			 * frames are in flight is safe via UPDATE_AFTER_BIND, and the leaving scene's textures
			 * stay alive while dormant). The drain that protects destruction is in
			 * Scenes::Manager::deleteScene, before the scene is erased.
			 * @param set A reference to the leaving scene's bindless texture set.
			 * @return void
			 */
			void clearTextureSet (const Scenes::BindlessTextureSet & set) const noexcept;

			/**
			 * @brief Updates a specific slot in the 1D texture array.
			 * @note Use this for reserved slots (environment maps, etc.).
			 * @param index The index of the slot to update.
			 * @param texture A reference to the texture interface.
			 * @return bool True if successful.
			 */
			[[nodiscard]]
			bool updateTexture1D (uint32_t index, const Vulkan::TextureInterface & texture) const noexcept;

			/**
			 * @brief Updates a specific slot in the 2D texture array.
			 * @note Use this for reserved slots (environment maps, etc.).
			 * @param index The index of the slot to update.
			 * @param texture A reference to the texture interface.
			 * @return bool True if successful.
			 */
			[[nodiscard]]
			bool updateTexture2D (uint32_t index, const Vulkan::TextureInterface & texture) const noexcept;

			/**
			 * @brief Updates a specific slot in the 3D texture array.
			 * @note Use this for reserved slots (environment maps, etc.).
			 * @param index The index of the slot to update.
			 * @param texture A reference to the texture interface.
			 * @return bool True if successful.
			 */
			[[nodiscard]]
			bool updateTexture3D (uint32_t index, const Vulkan::TextureInterface & texture) const noexcept;

			/**
			 * @brief Updates a specific slot in the cubemap texture array.
			 * @note Use this for reserved slots (environment maps, etc.).
			 * @param index The index of the slot to update.
			 * @param texture A reference to the texture interface.
			 * @return bool True if successful.
			 */
			[[nodiscard]]
			bool updateTextureCube (uint32_t index, const Vulkan::TextureInterface & texture) const noexcept;

			/**
			 * @brief Updates a specific slot in the cube array texture array.
			 * @param index The index of the slot to update.
			 * @param texture A reference to the texture interface.
			 * @return bool True if successful.
			 */
			[[nodiscard]]
			bool updateTextureCubeArray (uint32_t index, const Vulkan::TextureInterface & texture) const noexcept;

			/**
			 * @brief Updates a specific slot in the 2D texture array from a raw descriptor info.
			 * @note Use this when the texture source is not a TextureInterface (e.g., grab pass depth).
			 * @param index The index of the slot to update.
			 * @param descriptorInfo The Vulkan descriptor image info to write.
			 * @return bool True if successful.
			 */
			[[nodiscard]]
			bool updateTexture2DFromDescriptorInfo (uint32_t index, const VkDescriptorImageInfo & descriptorInfo) const noexcept;

			/**
			 * @brief Returns the descriptor set for binding during rendering.
			 * @return const Vulkan::DescriptorSet *
			 */
			[[nodiscard]]
			/**
			 * @brief Returns the descriptor set for binding during rendering.
			 * @return const Vulkan::DescriptorSet *
			 */
			[[nodiscard]]
			const Vulkan::DescriptorSet * descriptorSet () const noexcept;

			/**
			 * @brief Returns the descriptor set layout for pipeline creation.
			 * @return std::shared_ptr< Vulkan::DescriptorSetLayout >
			 */
			[[nodiscard]]
			/**
			 * @brief Returns the descriptor set layout for pipeline creation.
			 * @return std::shared_ptr< Vulkan::DescriptorSetLayout >
			 */
			[[nodiscard]]
			std::shared_ptr< Vulkan::DescriptorSetLayout > descriptorSetLayout () const noexcept;

		private:

			/** @copydoc EmEn::ServiceInterface::onInitialize() */
			bool onInitialize () noexcept override;

			/** @copydoc EmEn::ServiceInterface::onTerminate() */
			bool onTerminate () noexcept override;

			/**
			 * @brief Sizes the five texture arrays against the device's update-after-bind budget.
			 * @note A COMBINED_IMAGE_SAMPLER descriptor is charged to BOTH the sampler and the
			 * sampled-image update-after-bind limits, per set AND per stage. Desktop drivers
			 * advertise budgets in the millions and get the desired capacities; MoltenVK is capped
			 * at 1024 samplers by Metal's argument-buffer limit (sampled images stay at 1M), so it
			 * falls back to the reduced profile. Declaring more than the budget is not a validation
			 * nitpick: it is a hard Metal limit and undefined behaviour on any device.
			 * @return bool False if the device cannot host even the absolute floor.
			 */
			[[nodiscard]]
			bool computeCapacities () noexcept;

			/**
			 * @brief Creates the descriptor set layout with UPDATE_AFTER_BIND support.
			 * @return bool
			 */
			[[nodiscard]]
			bool createDescriptorSetLayout () noexcept;

			/**
			 * @brief Creates the descriptor pool with UPDATE_AFTER_BIND support.
			 * @return bool
			 */
			[[nodiscard]]
			bool createDescriptorPool () noexcept;

			/**
			 * @brief Creates the descriptor set.
			 * @return bool
			 */
			[[nodiscard]]
			bool createDescriptorSet () noexcept;

			/**
			 * @brief Writes a texture to the descriptor set at a specific binding and array index.
			 * @param binding The binding point.
			 * @param arrayIndex The index in the array.
			 * @param texture A reference to the texture interface.
			 * @return bool
			 */
			[[nodiscard]]
			bool writeTextureToDescriptorSet (uint32_t binding, uint32_t arrayIndex, const Vulkan::TextureInterface & texture) const noexcept;

			/**
			 * @brief Writes a raw descriptor info to the descriptor set at a specific binding and array index.
			 * @param binding The binding point.
			 * @param arrayIndex The index in the array.
			 * @param descriptorInfo The Vulkan descriptor image info to write.
			 * @return bool
			 */
			[[nodiscard]]
			bool writeRawToDescriptorSet (uint32_t binding, uint32_t arrayIndex, const VkDescriptorImageInfo & descriptorInfo) const noexcept;

			Renderer & m_renderer;
			/* Effective capacities, resolved at initialization from the device budget. They default
			 * to the desired values so a query made before initialization stays benign. */
			uint32_t m_maxTextures1D{DesiredMaxTextures1D};
			uint32_t m_maxTextures2D{DesiredMaxTextures2D};
			uint32_t m_maxTextures3D{DesiredMaxTextures3D};
			uint32_t m_maxTexturesCube{DesiredMaxTexturesCube};
			uint32_t m_maxTexturesCubeArray{DesiredMaxTexturesCubeArray};
			std::shared_ptr< Vulkan::Device > m_device;
			std::shared_ptr< Vulkan::DescriptorSetLayout > m_descriptorSetLayout;
			std::shared_ptr< Vulkan::DescriptorPool > m_descriptorPool;
			std::unique_ptr< Vulkan::DescriptorSet > m_descriptorSet;

			/* Thread safety for reserved-slot updates and active-scene set synchronization. */
			mutable std::mutex m_indexMutex;
	};
}
