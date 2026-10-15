/*
 * src/Vulkan/Instance.hpp
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
#include "emeraude_platform.hpp"
#include "emeraude_export.hpp"

/* STL inclusions. */
#include <cstddef>
#include <cstdint>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>

/* Local inclusions for inheritances. */
#include "ServiceInterface.hpp"

/* Local inclusions for usages. */
#include "DebugMessenger.hpp"
#include "Graphics/FramebufferPrecisions.hpp"
#include "Graphics/RenderDocCapture.hpp"
#include "Types.hpp"

/* Forward declarations */
namespace EmEn
{
	namespace Vulkan
	{
		class PhysicalDevice;
		class Device;
	}

	class Identification;
	class PrimaryServices;
	class Window;
}

namespace EmEn::Vulkan
{
	/**
	 * @brief The Vulkan instance service class.
	 * @extends EmEn::ServiceInterface This is a service
	 */
	class EMEN_LEAN_API Instance final : public ServiceInterface
	{
		public:

			/** @brief The Vulkan API version the instance asks for. */
			static constexpr uint32_t RequestedAPIVersion{VK_API_VERSION_1_3};

			/**
			 * @brief The lowest API version a device may report to be used (engine item
			 * device-below-vulkan-1-3-reaches-vkcreatedevice, 2026-10-15): the shaders target Vulkan 1.3 / SPIR-V 1.6 and
			 * the device creation chains VkPhysicalDeviceVulkan13Features. macOS compiles its shaders for Vulkan 1.2 /
			 * SPIR-V 1.5 (MoltenVK): 1.2 there.
			 */
			static constexpr uint32_t MinimumDeviceAPIVersion{IsMacOS ? VK_API_VERSION_1_2 : VK_API_VERSION_1_3};

			/**
			 * @brief Why a physical device was not used (the startup diagnosis, the user-facing message of a refused
			 * startup: engine item incompatible-gpu-is-never-told-to-the-user).
			 */
			struct DeviceRefusal final
			{
				/** @brief The cause, in terms a user can act on. */
				enum class Reason : uint8_t
				{
					/** @brief The device reports an API version below MinimumDeviceAPIVersion: an outdated driver. */
					BelowRequiredAPIVersion,
					/** @brief The device does not advertise a required feature: the hardware (or its driver) cannot run the engine. */
					MissingFeatures,
					/** @brief vkCreateDevice() failed for another reason. */
					CreationFailed
				};

				std::string deviceName;
				std::string driverVersion;
				/** @brief The reason's detail (the missing features, the versions, the Vulkan result). */
				std::string detail;
				uint32_t vendorID{0};
				uint32_t apiVersion{0};
				Reason reason{Reason::CreationFailed};
			};

			/** @brief Class identifier. */
			static constexpr auto ClassId{"VulkanInstanceService"};

			/**
			 * @brief Command-line argument overriding the validation layers FOR ONE RUN:
			 * `--set-vk-layers=LAYER_A,LAYER_B` (or `--set-vk-layers LAYER_A,LAYER_B`). When present, it replaces
			 * Core/Video/VulkanInstance/EnableDebug and RequestedValidationLayers, and the run neither reads nor writes
			 * any validation setting. An empty value (`--set-vk-layers=`) runs without any layer.
			 */
			static constexpr auto SetValidationLayersArgument{"--set-vk-layers"};

			/** @brief The most layers --set-vk-layers accepts (a longer list is refused as a whole). */
			static constexpr size_t MaxValidationLayersFromArgument{32};

			/**
			 * @brief Constructs a Vulkan instance.
			 * @param identification A reference to the application identification.
			 * @param primaryServices A reference to primary services.
			 * @param fullscreenExclusiveEnabled Declares the application will run only in fullscreen.
			 */
			Instance (const Identification & identification, PrimaryServices & primaryServices, bool fullscreenExclusiveEnabled) noexcept
				: ServiceInterface{ClassId},
				m_identification{identification},
				m_primaryServices{primaryServices},
				m_fullscreenExclusiveEnabled{fullscreenExclusiveEnabled}
			{
				/* [VULKAN-API-SETUP] Graphics device extensions selection. */

				/* VK_KHR_SWAPCHAIN_EXTENSION_NAME = "VK_KHR_swapchain" */
				m_requiredGraphicsDeviceExtensions.emplace_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

				/* VK_KHR_MULTIVIEW_EXTENSION_NAME = "VK_KHR_multiview" */
				m_requiredGraphicsDeviceExtensions.emplace_back(VK_KHR_MULTIVIEW_EXTENSION_NAME);

				if constexpr ( IsMacOS )
				{
					/* VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME = "VK_KHR_portability_subset" */
					m_requiredGraphicsDeviceExtensions.emplace_back("VK_KHR_portability_subset");
				}

				/* NOTE: VK_EXT_non_seamless_cube_map */
				// FIXME: Check to enable "VK_EXT_non_seamless_cube_map" extension
				//m_requiredGraphicsDeviceExtensions.emplace_back(VK_EXT_NON_SEAMLESS_CUBE_MAP_EXTENSION_NAME);

				//VK_EXT_BLEND_OPERATION_ADVANCED_EXTENSION_NAME, // Fails on Intel iGPU
				//VK_EXT_FILTER_CUBIC_EXTENSION_NAME, // Fails on NVidia
				//VK_KHR_SHARED_PRESENTABLE_IMAGE_EXTENSION_NAME,

				/* NOTE: Enable dynamic state extension. */
				//VK_EXT_EXTENDED_DYNAMIC_STATE_EXTENSION_NAME,
				//VK_EXT_EXTENDED_DYNAMIC_STATE_2_EXTENSION_NAME,
				//VK_EXT_EXTENDED_DYNAMIC_STATE_3_EXTENSION_NAME,

				/* NOTE: Video decoding extensions. (To test one day ...) */
				//VK_KHR_VIDEO_QUEUE_EXTENSION_NAME,
				//VK_KHR_VIDEO_DECODE_QUEUE_EXTENSION_NAME,
				//VK_KHR_VIDEO_DECODE_H265_EXTENSION_NAME,
			}

			/**
			 * @brief Destructs the Vulkan instance: when onTerminate() was skipped (an initialization failure path), the
			 * devices, the debug messenger and the VkInstance are released here (vkDestroyInstance() used to run only
			 * from onTerminate()).
			 */
			~Instance () override;

			/** @brief Copy constructor (deleted: the VkInstance is owned). */
			Instance (const Instance & copy) noexcept = delete;

			/** @brief Move constructor (deleted). */
			Instance (Instance && copy) noexcept = delete;

			/** @brief Copy assignment (deleted). */
			Instance & operator= (const Instance & copy) noexcept = delete;

			/** @brief Move assignment (deleted). */
			Instance & operator= (Instance && copy) noexcept = delete;

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
			 * @brief Returns the Vulkan instance info structure used while initialization.
			 * @return const VkInstanceCreateInfo &
			 */
			[[nodiscard]]
			const VkInstanceCreateInfo &
			info () const noexcept
			{
				return m_createInfo;
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
			 * @warning This can be nullptr!
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
			 * @warning This can be nullptr!
			 * @return const std::shared_ptr< Device > &
			 */
			[[nodiscard]]
			const std::shared_ptr< Device > &
			computeDevice () const noexcept
			{
				return m_computeDevice;
			}

			/**
			 * @brief Returns whether the instance show information state is on.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			showInformation () const noexcept
			{
				return m_showInformation;
			}

			/**
			 * @brief Returns the Vulkan validation layer state.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isDebugModeEnabled () const noexcept
			{
				return m_debugMode;
			}

			/**
			 * @brief Returns whether the Vulkan debug messenger is active.
			 * @note Created only in debug mode AND when at least one validation layer is requested;
			 * an empty layer list with debug mode on yields object naming only (no messenger).
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isUsingDebugMessenger () const noexcept
			{
				return m_debugMode && !m_requiredValidationLayers.empty();
			}

			/**
			 * @brief Returns whether the dynamic extensions state was enabled.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isDynamicStateExtensionEnabled () const noexcept
			{
				return m_dynamicStateExtensionEnabled;
			}

			/**
			 * @brief Returns a reference to the RenderDoc capture wrapper.
			 * @return Graphics::RenderDocCapture &
			 */
			[[nodiscard]]
			Graphics::RenderDocCapture &
			renderDocCapture () noexcept
			{
				return m_renderDocCapture;
			}

			/**
			 * @brief Asks the graphics device selection to prefer a given physical device (optional).
			 * @note For an application whose other components already sit on a GPU (e.g. a Chromium GPU process pinned
			 * to an adapter LUID, whose shared textures can only be imported on that same adapter). Called BEFORE the
			 * first getGraphicsDevice(), on the main thread. The preferred device is taken only if it passes every
			 * compatibility check (queues, presentation, required features and extensions); otherwise the score decides
			 * and a warning names it. Priority: Core/Video/VulkanDevice/ForceGPU > the preferred device > the score.
			 * Without a call, the selection is unchanged. Compare the result with
			 * getGraphicsDevice()->physicalDevice()->identity().
			 * @param identity The identity of the device to prefer (LUID and/or UUID).
			 */
			void
			preferGraphicsDevice (const DeviceIdentity & identity) noexcept
			{
				m_preferredGraphicsDevice = identity;
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
			 * @brief Returns every physical device refused during the selection, and why (main thread, after
			 * getGraphicsDevice() / getComputeDevice()).
			 * @return const std::vector< DeviceRefusal > &
			 */
			[[nodiscard]]
			const std::vector< DeviceRefusal > &
			deviceRefusals () const noexcept
			{
				return m_deviceRefusals;
			}

			/**
			 * @brief Returns a list of validation layers available from Vulkan.
			 * @note This method follows the vkEnumerateDeviceLayerProperties() deprecation.
			 * @return std::vector< VkLayerProperties >
			 */
			[[nodiscard]]
			static std::vector< VkLayerProperties > getAvailableValidationLayers () noexcept;

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
			 * @param redBits The bit depth for the red component.
			 * @param greenBits The bit depth for the green component.
			 * @param blueBits The bit depth for the blue component.
			 * @param alphaBits The bit depth for the alpha component.
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
			static std::vector< const char * > getSupportedValidationLayers (const std::vector< std::string > & requestedValidationLayers, const std::vector< VkLayerProperties > & availableValidationLayers = Instance::getAvailableValidationLayers()) noexcept;

		private:

			/** @copydoc EmEn::ServiceInterface::onInitialize() */
			bool onInitialize () noexcept override;

			/** @copydoc EmEn::ServiceInterface::onTerminate() */
			bool onTerminate () noexcept override;

			/**
			 * @brief Read settings.
			 */
			void readSettings () noexcept;

			/**
			 * @brief Parses the value of --set-vk-layers (command line: a trust boundary, checked in every build).
			 * @note Items are trimmed, empty items and duplicates dropped (first occurrence kept). An item that is not a
			 * layer name ([A-Za-z0-9_], shorter than VK_MAX_EXTENSION_NAME_SIZE) or a list longer than
			 * MaxValidationLayersFromArgument refuses the WHOLE value, with a trace.
			 * @param value The raw argument value, possibly empty.
			 * @return std::optional< std::vector< std::string > > The layer names (empty = no layer), or nothing when refused.
			 */
			[[nodiscard]]
			static std::optional< std::vector< std::string > > parseValidationLayersArgument (const std::string & value) noexcept;

			/**
			 * @brief Prepares a list of a physical device available on the computer.
			 * @return bool
			 */
			[[nodiscard]]
			bool preparePhysicalDevices () noexcept;

			/** @brief A compatible physical device and the keys that rank it (see rankDevices()). */
			struct ScoredPhysicalDevice
			{
				std::shared_ptr< PhysicalDevice > device;
				size_t score{0};             /**< The mode's score (extensions, queues, type multiplier). */
				uint64_t deviceLocalMemory{0}; /**< The largest DEVICE_LOCAL heap, in bytes (VRAM). */
				size_t enumerationIndex{0};  /**< The order Vulkan listed it (the last tie-break: lower first). */
				uint32_t typeRank{0};        /**< Discrete 4, integrated 3, virtual 2, CPU 1, other 0. */
			};

			/**
			 * @brief Returns the compatible graphics-capable devices, ranked: the BEST is the LAST (see rankDevices()).
			 * @param window A pointer to the current Window. This can be nullptr.
			 * @return std::vector< ScoredPhysicalDevice >
			 */
			[[nodiscard]]
			std::vector< ScoredPhysicalDevice > getScoredGraphicsDevices (Window * window) noexcept;

			/**
			 * @brief Creates the logical graphics device on one physical device: the required features checked first (a
			 * device that lacks one is refused before vkCreateDevice()), then the optional ones and extensions requested.
			 * @param window A pointer to the current Window. This can be nullptr.
			 * @param selectedPhysicalDevice The physical device.
			 * @return std::shared_ptr< Device > nullptr when refused or when vkCreateDevice() fails (the refusal recorded).
			 */
			[[nodiscard]]
			std::shared_ptr< Device > createGraphicsDevice (Window * window, const std::shared_ptr< PhysicalDevice > & selectedPhysicalDevice) noexcept;

			/**
			 * @brief Refuses a device whose API version is below MinimumDeviceAPIVersion, with the log line a user (or a
			 * report) can act on: the device, its API and driver versions, "update the graphics driver".
			 * @param physicalDevice The physical device.
			 * @return bool True when the device is recent enough.
			 */
			[[nodiscard]]
			bool checkDeviceAPIVersion (const PhysicalDevice & physicalDevice) noexcept;

			/**
			 * @brief Records why a device was not used (deviceRefusals()).
			 * @param physicalDevice The physical device.
			 * @param reason The cause.
			 * @param detail The detail.
			 */
			void recordDeviceRefusal (const PhysicalDevice & physicalDevice, DeviceRefusal::Reason reason, std::string detail) noexcept;

			/**
			 * @brief Builds the ranking keys of a compatible device.
			 * @param device A reference to the device smart pointer.
			 * @param score The mode's score.
			 * @param enumerationIndex The device's index in m_physicalDevices.
			 * @return ScoredPhysicalDevice
			 */
			[[nodiscard]]
			static ScoredPhysicalDevice makeScoredDevice (const std::shared_ptr< PhysicalDevice > & device, size_t score, size_t enumerationIndex) noexcept;

			/**
			 * @brief Sorts the devices from the worst to the BEST (the last). RawPerformance: type rank, then VRAM, then
			 * score; the other modes: score, then type rank, then VRAM. The enumeration order breaks the last tie, so no
			 * two devices ever compare equal (the former std::map keyed by score silently dropped a device with the same
			 * score as another).
			 * @param devices A reference to the devices to sort.
			 */
			void rankDevices (std::vector< ScoredPhysicalDevice > & devices) const noexcept;

			/**
			 * @brief Configures the list of required validation layers.
			 * @note This method follows the vkEnumerateDeviceLayerProperties() deprecation.
			 */
			void configureValidationLayers () noexcept;

			/**
			 * @brief Configures the list of required instance extensions.
			 */
			void configureInstanceExtensions () noexcept;

			/**
			 * @brief Detects hybrid GPU configurations (Nvidia Optimus, etc.).
			 * @return HybridGPUConfig containing detection results.
			 */
			[[nodiscard]]
			HybridGPUConfig detectHybridGPUConfiguration () const noexcept;

			/**
			 * @brief Checks if a GPU name indicates a mobile/laptop variant.
			 * @param gpuName The GPU name string to check.
			 * @return True if the GPU name contains mobile indicators (Mobile, Laptop, Max-Q, MX).
			 */
			[[nodiscard]]
			static bool isLikelyMobileGPU (const std::string & gpuName) noexcept;

			/**
			 * @brief Checks if a GPU should be excluded in Failsafe mode.
			 * @param hybridConfig A reference to a struct.
			 * @param physicalDevice The physical device to check.
			 * @return True if the device should be excluded.
			 */
			[[nodiscard]]
			bool shouldExcludeForFailsafe (const HybridGPUConfig & hybridConfig, const std::shared_ptr< PhysicalDevice > & physicalDevice) const noexcept;

			/**
			 * @brief Modulates the device score against a running strategy.
			 * @param deviceProperties A vulkan struct for the device properties.
			 * @param score A reference to the score.
			 */
			void modulateDeviceScoring (const VkPhysicalDeviceProperties & deviceProperties, size_t & score) const noexcept;

			/**
			 * @brief Checks the physical device type for use as a specific purpose
			 * @param physicalDevice A reference to the physical device smart pointer.
			 * @param type The desired queue type.
			 * @param score A reference to a score to sort the preferred device.
			 * @return bool
			 */
			[[nodiscard]]
			bool checkDeviceCompatibility (const std::shared_ptr< PhysicalDevice > & physicalDevice, VkQueueFlagBits type, size_t & score) const noexcept;

			/**
			 * @brief Checks the physical device type for use as a graphics device with presentation supported.
			 * @param physicalDevice A reference to the physical device smart pointer.
			 * @param window A pointer to the window where rendering will be displayed.
			 * @param score A reference to a score to sort the preferred device.
			 * @return bool
			 */
			[[nodiscard]]
			bool checkDeviceCompatibility (const std::shared_ptr< PhysicalDevice > & physicalDevice, const Window * window, size_t & score) const noexcept;

			/**
			 * @brief Checks the device features presence for a specialized device selector.
			 * @param physicalDevice A reference to the physical device smart pointer.
			 * @param score A reference to a score to sort the preferred device.
			 * @return bool
			 */
			[[nodiscard]]
			bool checkDevicesFeaturesForGraphics (const std::shared_ptr< PhysicalDevice > & physicalDevice, size_t & score) const noexcept;

			/**
			 * @brief Checks the device features presence for a specialized device selector.
			 * @param physicalDevice A reference to the physical device smart pointer.
			 * @param score A reference to a score to sort the preferred device.
			 * @return bool
			 */
			[[nodiscard]]
			static bool checkDevicesFeaturesForCompute (const std::shared_ptr< PhysicalDevice > & physicalDevice, size_t & score) noexcept;

			/**
			 * @brief Enables VK_KHR_portability_subset for a logical device when its physical device advertises it, with
			 * every portability feature the physical device supports. Shared by every logical device creation.
			 * @note The specification makes the extension mandatory when advertised (VUID-VkDeviceCreateInfo-pProperties-04451:
			 * MoltenVK, some Windows drivers and software renderers), and every portability feature disabled unless the
			 * feature structure requests it: requesting exactly what the device reports is maximal and safe.
			 * @param physicalDevice A reference to the physical device the logical device is created on.
			 * @param extensions A writable reference to the device extensions the logical device will enable.
			 * @param requirements A writable reference to the device requirements (its feature chain).
			 * @return bool True when the extension was advertised and is now requested.
			 */
			[[nodiscard]]
			static bool requestPortabilitySubset (const PhysicalDevice & physicalDevice, std::vector< const char * > & extensions, DeviceRequirements & requirements) noexcept;

			/**
			 * @brief Checks the device extensions presence for a specialized device selector.
			 * @param physicalDevice A reference to the physical device smart pointer.
			 * @param requiredExtensions A reference to a list of required extensions.
			 * @param score A reference to a score to sort the preferred device.
			 * @return bool
			 */
			[[nodiscard]]
			static bool checkDeviceForRequiredExtensions (const std::shared_ptr< PhysicalDevice > & physicalDevice, const std::vector< const char * > & requiredExtensions, size_t & score) noexcept;

			const Identification & m_identification;
			PrimaryServices & m_primaryServices;
			VkInstance m_instance{VK_NULL_HANDLE};
			VkApplicationInfo m_applicationInfo{};
			VkInstanceCreateInfo m_createInfo{};
			VkDebugUtilsMessengerCreateInfoEXT m_debugCreateInfo{};
			std::unique_ptr< DebugMessenger > m_debugMessenger;
			Graphics::RenderDocCapture m_renderDocCapture;
			std::vector< std::shared_ptr< PhysicalDevice > > m_physicalDevices;
			std::shared_ptr< Device > m_graphicsDevice;
			std::shared_ptr< Device > m_computeDevice;
			/* NOTE: Owns the requested layer names: m_requiredValidationLayers points into it, so it is filled once,
			 * before the selection, and never modified while the instance lives. */
			std::vector< std::string > m_requestedValidationLayers;
			std::vector< const char * > m_requiredValidationLayers;
			std::vector< const char * > m_requiredInstanceExtensions;
			std::vector< const char * > m_requiredGraphicsDeviceExtensions;
			std::optional< DeviceIdentity > m_preferredGraphicsDevice;
			/* NOTE: Filled during the device selection (main thread), read for the startup diagnosis. */
			std::vector< DeviceRefusal > m_deviceRefusals;
			DeviceAutoSelectMode m_autoSelectMode{DeviceAutoSelectMode::Performance};
			bool m_showInformation{false};
			bool m_debugMode{false};
			bool m_validationLayersFromArgument{false};
			bool m_dynamicStateExtensionEnabled{false};
			bool m_enableFailSafe{false};
			bool m_fullscreenExclusiveEnabled{false};
			bool m_useVulkanMemoryAllocator{false};
	};
}
