/*
 * src/Graphics/BindlessTexturesManager.hpp
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
#include <cstdint>
#include <memory>
#include <queue>
#include <mutex>

/* Local inclusions for inheritances. */
#include "ServiceInterface.hpp"

/* Forward declarations. */
namespace EmEn::Vulkan
{
	class Device;
	class DescriptorPool;
	class DescriptorSet;
	class DescriptorSetLayout;
	class TextureInterface;
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
	class BindlessTextureManager final : public ServiceInterface
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"BindlessTextureManagerService"};

			/** @brief Reserved slots for global resources (IBL, environment, etc.). */
			static constexpr uint32_t EnvironmentCubemapSlot = 0;
			static constexpr uint32_t IrradianceCubemapSlot = 1;
			static constexpr uint32_t PrefilteredCubemapSlot = 2;
			static constexpr uint32_t BRDFLutSlot = 3;
			static constexpr uint32_t FirstDynamicSlot = 16;

			/** @brief Maximum texture counts per type. */
			static constexpr uint32_t MaxTextures1D = 256;
			static constexpr uint32_t MaxTextures2D = 4096;
			static constexpr uint32_t MaxTextures3D = 256;
			static constexpr uint32_t MaxTexturesCube = 256;

			/** @brief Binding points in the descriptor set layout. */
			static constexpr uint32_t Texture1DBinding = 0;
			static constexpr uint32_t Texture2DBinding = 1;
			static constexpr uint32_t Texture3DBinding = 2;
			static constexpr uint32_t TextureCubeBinding = 3;

			/**
			 * @brief Constructs a bindless textures manager service.
			 * @param renderer A reference to the graphics renderer.
			 */
			explicit BindlessTextureManager (Renderer & renderer) noexcept;

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
			 * @brief Registers a 1D texture and returns its index in the bindless array.
			 * @param texture A reference to the texture interface.
			 * @return uint32_t The index in the bindless array, or UINT32_MAX on failure.
			 */
			[[nodiscard]]
			uint32_t registerTexture1D (const Vulkan::TextureInterface & texture) noexcept;

			/**
			 * @brief Registers a 2D texture and returns its index in the bindless array.
			 * @param texture A reference to the texture interface.
			 * @return uint32_t The index in the bindless array, or UINT32_MAX on failure.
			 */
			[[nodiscard]]
			uint32_t registerTexture2D (const Vulkan::TextureInterface & texture) noexcept;

			/**
			 * @brief Registers a 3D texture and returns its index in the bindless array.
			 * @param texture A reference to the texture interface.
			 * @return uint32_t The index in the bindless array, or UINT32_MAX on failure.
			 */
			[[nodiscard]]
			uint32_t registerTexture3D (const Vulkan::TextureInterface & texture) noexcept;

			/**
			 * @brief Registers a cubemap texture and returns its index in the bindless array.
			 * @param texture A reference to the texture interface.
			 * @return uint32_t The index in the bindless array, or UINT32_MAX on failure.
			 */
			[[nodiscard]]
			uint32_t registerTextureCube (const Vulkan::TextureInterface & texture) noexcept;

			/**
			 * @brief Unregisters a 1D texture and frees its index.
			 * @param index The index of the texture to unregister.
			 * @return void
			 */
			void unregisterTexture1D (uint32_t index) noexcept;

			/**
			 * @brief Unregisters a 2D texture and frees its index.
			 * @param index The index of the texture to unregister.
			 * @return void
			 */
			void unregisterTexture2D (uint32_t index) noexcept;

			/**
			 * @brief Unregisters a 3D texture and frees its index.
			 * @param index The index of the texture to unregister.
			 * @return void
			 */
			void unregisterTexture3D (uint32_t index) noexcept;

			/**
			 * @brief Unregisters a cubemap texture and frees its index.
			 * @param index The index of the texture to unregister.
			 * @return void
			 */
			void unregisterTextureCube (uint32_t index) noexcept;

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
			 * @brief Allocates a new index from a free list or increments the counter.
			 * @param freeIndices A reference to the free indices queue.
			 * @param nextIndex A reference to the next available index counter.
			 * @param maxIndex The maximum allowed index.
			 * @return uint32_t The allocated index, or UINT32_MAX on failure.
			 */
			[[nodiscard]]
			static uint32_t allocateIndex (std::queue< uint32_t > & freeIndices, uint32_t & nextIndex, uint32_t maxIndex) noexcept;

			/**
			 * @brief Frees an index back to the free list.
			 * @param freeIndices A reference to the free indices queue.
			 * @param index The index to free.
			 * @return void
			 */
			static void freeIndex (std::queue< uint32_t > & freeIndices, uint32_t index) noexcept;

			/**
			 * @brief Writes a texture to the descriptor set at a specific binding and array index.
			 * @param binding The binding point.
			 * @param arrayIndex The index in the array.
			 * @param texture A reference to the texture interface.
			 * @return bool
			 */
			[[nodiscard]]
			bool writeTextureToDescriptorSet (uint32_t binding, uint32_t arrayIndex, const Vulkan::TextureInterface & texture) const noexcept;

			Renderer & m_renderer;
			std::shared_ptr< Vulkan::Device > m_device;
			std::shared_ptr< Vulkan::DescriptorSetLayout > m_descriptorSetLayout;
			std::shared_ptr< Vulkan::DescriptorPool > m_descriptorPool;
			std::unique_ptr< Vulkan::DescriptorSet > m_descriptorSet;

			/* Index management for dynamic allocation. */
			std::queue< uint32_t > m_freeIndices1D;
			std::queue< uint32_t > m_freeIndices2D;
			std::queue< uint32_t > m_freeIndices3D;
			std::queue< uint32_t > m_freeIndicesCube;
			uint32_t m_nextIndex1D{FirstDynamicSlot};
			uint32_t m_nextIndex2D{FirstDynamicSlot};
			uint32_t m_nextIndex3D{FirstDynamicSlot};
			uint32_t m_nextIndexCube{FirstDynamicSlot};

			/* Thread safety for index allocation. */
			mutable std::mutex m_indexMutex;
	};
}
