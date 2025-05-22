/*
 * src/Vulkan/Instance.hpp
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
#include <cstdint>
#include <cstddef>
#include <string>
#include <vector>
#include <map>
#include <array>
#include <memory>

/* Local inclusions for inheritances. */
#include "ServiceInterface.hpp"

/* Local inclusions for usages. */
#include "Graphics/FramebufferPrecisions.hpp"
#include "DebugMessenger.hpp"
#include "Types.hpp"

/* Forward declarations */
namespace EmEn
{
	namespace Vulkan
	{
		class PhysicalDevice;
		class Device;
	}

	class PrimaryServices;
	class Window;
}

namespace EmEn::Vulkan
{
	/**
	 * @brief The Vulkan instance service class.
	 * @extends EmEn::ServiceInterface This is a service
	 */
	class Instance final : public ServiceInterface
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"VulkanInstanceService"};

			/** @brief Observable class unique identifier. */
			static const size_t ClassUID;

			/**
			 * @brief Constructs the Vulkan instance.
			 * @param primaryServices A reference to primary services.
			 */
			explicit Instance (PrimaryServices & primaryServices) noexcept;

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

			/** @copydoc EmEn::ServiceInterface::usable() */
			[[nodiscard]]
			bool
			usable () const noexcept override
			{
				return m_instance != nullptr;
			}

			/**
			 * @brief Returns the Vulkan instance handle wrapped in a smart pointer.
			 * @return VkInstance
			 */
			[[nodiscard]]
			VkInstance
			handle () const noexcept
			{
				return m_instance;
			}

			/**
			 * @brief Returns a reference to the physical device smart pointer list.
			 * @return const std::vector< std::shared_ptr< PhysicalDevice > > &
			 */
			[[nodiscard]]
			const std::vector< std::shared_ptr< PhysicalDevice > > &
			physicalDevices () const noexcept
			{
				return m_physicalDevices;
			}

			/**
			 * @brief Returns a reference to the selected graphics device smart pointer.
			 * @warning This can be nullptr !
			 * @return const std::shared_ptr< Device > &
			 */
			[[nodiscard]]
			const std::shared_ptr< Device > &
			graphicsDevice () const noexcept
			{
				return m_graphicsDevice;
			}

			/**
			 * @brief Returns a reference to the selected compute device smart pointer.
			 * @warning This can be nullptr !
			 * @return const std::shared_ptr< Device > &
			 */
			[[nodiscard]]
			const std::shared_ptr< Device > &
			computeDevice () const noexcept
			{
				return m_computeDevice;
			}

			/**
			 * @brief Returns the Vulkan validation layer state.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isDebugModeEnabled () const noexcept
			{
				return m_flags[DebugMode];
			}

			/**
			 * @brief Returns whether the vulkan debug messenger is enabled.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isUsingDebugMessenger () const noexcept
			{
				return m_flags[UseDebugMessenger];
			}

			/**
			 * @brief Returns whether the dynamic state extensions was enabled.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isDynamicStateExtensionEnabled () const noexcept
			{
				return m_flags[DynamicStateExtensionEnabled];
			}

			/**
			 * @brief Returns whether textures must be checked for standard requirements like sizes being power of two.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isStandardTextureCheckEnabled () const noexcept
			{
				return m_flags[StandardTextureCheckEnabled];
			}

			/**
			 * @brief Returns a logical device with graphics capabilities.
			 * @param window A reference to a Window to check the device with presentation. Default Off-screen rendering.
			 * @return std::shared_ptr< Device >
			 */
			[[nodiscard]]
			std::shared_ptr< Device > getGraphicsDevice (Window * window = nullptr) noexcept;

			/**
			 * @brief Returns a logical device with compute capabilities.
			 * @return std::shared_ptr< Device >
			 */
			[[nodiscard]]
			std::shared_ptr< Device > getComputeDevice () noexcept;

			/**
			 * @brief Returns a list of validation layers available from Vulkan.
			 * @note This method follow the vkEnumerateDeviceLayerProperties() deprecation.
			 * @return std::vector< VkLayerProperties >
			 */
			[[nodiscard]]
			static std::vector< VkLayerProperties > getValidationLayers () noexcept;

			/**
			 * @brief Returns a list of extensions available from Vulkan.
			 * @param pLayerName Default nullptr.
			 * @return std::vector< VkExtensionProperties >
			 */
			[[nodiscard]]
			static std::vector< VkExtensionProperties > getExtensions (const char * pLayerName = nullptr) noexcept;

			/**
			 * @brief Finds a suitable color buffer format.
			 * @param device A reference to a device smart pointer.
			 * @param redBits The bit depth for red component.
			 * @param greenBits The bit depth for green component.
			 * @param blueBits The bit depth for blue component.
			 * @param alphaBits The bit depth for alpha component.
			 * @return VkFormat
			 */
			[[nodiscard]]
			static VkFormat findColorFormat (const std::shared_ptr< Vulkan::Device > & device, uint32_t redBits, uint32_t greenBits, uint32_t blueBits, uint32_t alphaBits) noexcept;

			/**
			 * @brief Finds a suitable color buffer format.
			 * @param device A reference to a device smart pointer.
			 * @param precision A reference to a framebuffer precision structure.
			 * @return VkFormat
			 */
			[[nodiscard]]
			static
			VkFormat
			findColorFormat (const std::shared_ptr< Device > & device, const Graphics::FramebufferPrecisions & precision) noexcept
			{
				return findColorFormat(device, precision.redBits(), precision.greenBits(), precision.blueBits(), precision.alphaBits());
			}

			/**
			 * @brief Finds a suitable depth/stencil buffer format.
			 * @param device A reference to a device smart pointer.
			 * @param depthBits The bit depth for depth buffer.
			 * @param stencilBits The bit depth for stencil buffer.
			 * @return VkFormat
			 */
			[[nodiscard]]
			static VkFormat findDepthStencilFormat (const std::shared_ptr< Vulkan::Device > & device, uint32_t depthBits, uint32_t stencilBits) noexcept;

			/**
			 * @brief Finds a suitable depth/stencil buffer format.
			 * @param device A reference to a device smart pointer.
			 * @param precision A reference to a framebuffer precision structure.
			 * @return VkFormat
			 */
			[[nodiscard]]
			static
			VkFormat
			findDepthStencilFormat (const std::shared_ptr< Device > & device, const Graphics::FramebufferPrecisions & precision) noexcept
			{
				return findDepthStencilFormat(device, precision.depthBits(), precision.stencilBits());
			}

			/**
			 * @brief Returns a list of supported validations layers with the current system from a requested list.
			 * @param requestedValidationLayers A reference to a vector of requested validation layer names.
			 * @param availableValidationLayers A reference to a vector of available validation layer properties.
			 * @return std::vector< const char * >
			 */
			[[nodiscard]]
			static std::vector< const char * > getSupportedValidationLayers (const std::vector< std::string > & requestedValidationLayers, const std::vector< VkLayerProperties > & availableValidationLayers = Instance::getValidationLayers()) noexcept;

		private:

			/** @copydoc EmEn::ServiceInterface::onInitialize() */
			bool onInitialize () noexcept override;

			/** @copydoc EmEn::ServiceInterface::onTerminate() */
			bool onTerminate () noexcept override;

			/**
			 * @brief Read settings.
			 * @return void
			 */
			void readSettings () noexcept;

			/**
			 * @brief Prepares a list of physical device available on the computer.
			 * @return bool
			 */
			[[nodiscard]]
			bool preparePhysicalDevices () noexcept;

			/**
			 * @brief Returns a list of a scored graphics capable devices.
			 * @param window A pointer to the current Window.
			 * @return std::map< size_t, std::shared_ptr< PhysicalDevice > >
			 */
			[[nodiscard]]
			std::map< size_t, std::shared_ptr< PhysicalDevice > > getScoredGraphicsDevices (Window * window) const noexcept;

			/**
			 * @brief Configures the list of required validation layers.
			 * @note This method follow the vkEnumerateDeviceLayerProperties() deprecation.
			 * @return void
			 */
			void setRequiredValidationLayers () noexcept;

			/**
			 * @brief Configures the list of required extensions.
			 * @return void
			 */
			void setRequiredExtensions () noexcept;

			/**
			 * @brief Modulates the device score against a running strategy.
			 * @param deviceProperties A vulkan struct for the device properties.
			 * @param runningMode The desired running mode.
			 * @param score A reference to the score.
			 * @return void
			 */
			void modulateDeviceScoring (const VkPhysicalDeviceProperties & deviceProperties, RunningMode runningMode, size_t & score) const noexcept;

			/**
			 * @brief Checks the physical device type for use as a specific purpose
			 * @param physicalDevice A reference to the physical device smart pointer.
			 * @param runningMode The desired running mode.
			 * @param type The desired queue type.
			 * @param score A reference to a score to sort preferred device.
			 * @return bool
			 */
			[[nodiscard]]
			bool checkDeviceCompatibility (const std::shared_ptr< PhysicalDevice > & physicalDevice, RunningMode runningMode, VkQueueFlagBits type, size_t & score) const noexcept;

			/**
			 * @brief Checks the physical device type for use as a graphics device with presentation supported.
			 * @param physicalDevice A reference to the physical device smart pointer.
			 * @param runningMode The desired running mode.
			 * @param window A pointer to the window where rendering will be displayed.
			 * @param score A reference to a score to sort preferred device.
			 * @return bool
			 */
			[[nodiscard]]
			bool checkDeviceCompatibility (const std::shared_ptr< PhysicalDevice > & physicalDevice, RunningMode runningMode, const Window * window, size_t & score) const noexcept;

			/**
			 * @brief Checks the device features presence for a specialized device selector.
			 * @param physicalDevice A reference to the physical device smart pointer.
			 * @param score A reference to a score to sort preferred device.
			 * @return bool
			 */
			[[nodiscard]]
			bool checkDevicesFeaturesForGraphics (const std::shared_ptr< PhysicalDevice > & physicalDevice, size_t & score) const noexcept;

			/**
			 * @brief Checks the device features presence for a specialized device selector.
			 * @param physicalDevice A reference to the physical device smart pointer.
			 * @param score A reference to a score to sort preferred device.
			 * @return bool
			 */
			[[nodiscard]]
			bool checkDevicesFeaturesForCompute (const std::shared_ptr< PhysicalDevice > & physicalDevice, size_t & score) const noexcept;

			/**
			 * @brief Checks the device extensions presence for a specialized device selector.
			 * @param physicalDevice A reference to the physical device smart pointer.
			 * @param requiredExtensions A reference to a list of required extensions.
			 * @param score A reference to a score to sort preferred device.
			 * @return bool
			 */
			[[nodiscard]]
			bool checkDeviceForRequiredExtensions (const std::shared_ptr< PhysicalDevice > & physicalDevice, const std::vector< const char * > & requiredExtensions, size_t & score) const noexcept;

			/* Flag names. */
			static constexpr auto ShowInformation{0UL};
			static constexpr auto DebugMode{1UL};
			static constexpr auto UseDebugMessenger{2UL};
			static constexpr auto DynamicStateExtensionEnabled{3UL};
			static constexpr auto StandardTextureCheckEnabled{4UL};

			PrimaryServices & m_primaryServices;
			VkInstance m_instance{VK_NULL_HANDLE};
			VkApplicationInfo m_applicationInfo{};
			VkInstanceCreateInfo m_createInfo{};
			VkDebugUtilsMessengerCreateInfoEXT m_debugCreateInfo{};
			std::unique_ptr< DebugMessenger > m_debugMessenger;
			std::vector< std::shared_ptr< PhysicalDevice > > m_physicalDevices;
			std::shared_ptr< Device > m_graphicsDevice;
			std::shared_ptr< Device > m_computeDevice;
			std::vector< std::string > m_requestedValidationLayers;
			std::vector< const char * > m_requiredValidationLayers;
			std::vector< const char * > m_requiredInstanceExtensions;
			std::vector< const char * > m_requiredGraphicsDeviceExtensions;
			std::array< bool, 8 > m_flags{
				false/*ShowInformation*/,
				false/*DebugMode*/,
				false/*UseDebugMessenger*/,
				false/*DynamicStateExtensionEnabled*/,
				false/*StandardTextureCheckEnabled*/,
				false/*UNUSED*/,
				false/*UNUSED*/,
				false/*UNUSED*/
			};
	};
}
