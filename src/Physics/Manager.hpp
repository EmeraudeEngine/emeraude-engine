/*
 * src/Physics/Manager.hpp
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
#include <vector>
#include <map>
#include <array>
#include <memory>

/* Local inclusions for inheritances. */
#include "ServiceInterface.hpp"
#include "Vulkan/TransferManager.hpp"
#include "Vulkan/LayoutManager.hpp"

/* Forward declarations. */
namespace EmEn
{
	namespace Vulkan
	{
		class Instance;
		class Device;
		class DescriptorPool;
		class DescriptorSet;
		class DescriptorSetLayout;
		class CommandPool;
		class CommandBuffer;
		class PipelineLayout;
		class ComputePipeline;
	}

	class PrimaryServices;
}

namespace EmEn::Physics
{
	/**
	 * @brief The physics manager service class.
	 * @extends EmEn::ServiceInterface This is a service.
	 */
	class Manager final : public ServiceInterface
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"PhysicsManagerService"};

			/**
			 * @brief Constructs the physics manager.
			 * @param primaryServices A reference to primary services.
			 * @param instance A reference to the Vulkan instance.
			 */
			Manager (PrimaryServices & primaryServices, Vulkan::Instance & instance) noexcept
				: ServiceInterface{ClassId},
				m_primaryServices{primaryServices},
				m_vulkanInstance{instance}
			{

			}

			/**
			 * @brief Returns the reference to the transfer manager.
			 * @return Vulkan::TransferManager &
			 */
			[[nodiscard]]
			Vulkan::TransferManager &
			transferManager () noexcept
			{
				return m_transferManager;
			}

			/**
			 * @brief Returns the reference to the transfer manager.
			 * @return const Vulkan::TransferManager &
			 */
			[[nodiscard]]
			const Vulkan::TransferManager &
			transferManager () const noexcept
			{
				return m_transferManager;
			}

			/**
			 * @brief Returns the reference to the layout manager.
			 * @return Vulkan::LayoutManager &
			 */
			[[nodiscard]]
			Vulkan::LayoutManager &
			layoutManager () noexcept
			{
				return m_layoutManager;
			}

			/**
			 * @brief Returns the reference to the layout manager.
			 * @return const Vulkan::LayoutManager &
			 */
			[[nodiscard]]
			const Vulkan::LayoutManager &
			layoutManager () const noexcept
			{
				return m_layoutManager;
			}

			/**
			 * @brief Returns the selected logical device used for compute.
			 * @return std::shared_ptr< Vulkan::Device >
			 */
			[[nodiscard]]
			std::shared_ptr< Vulkan::Device >
			device () const noexcept
			{
				return m_device;
			}

			/**
			 * @brief Returns the descriptor pool.
			 * @return std::shared_ptr< Vulkan::DescriptorPool >
			 */
			[[nodiscard]]
			std::shared_ptr< Vulkan::DescriptorPool >
			descriptorPool () const noexcept
			{
				return m_descriptorPool;
			}

			/**
			 * @brief Returns or creates a graphics pipeline according to requirements.
			 * @param pipelineLayout A reference to as pipeline layout smart pointer.
			 * @return std::shared_ptr< Vulkan::ComputePipeline >
			 */
			[[nodiscard]]
			std::shared_ptr< Vulkan::ComputePipeline > getPipeline (const std::shared_ptr< Vulkan::PipelineLayout > & pipelineLayout) noexcept;

			/**
			 * @brief Returns whether the physics acceleration was enabled at startup.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isPhysicsAccelerationAvailable () const noexcept
			{
				return m_accelerationAvailable;
			}

		private:

			/** @copydoc EmEn::ServiceInterface::onInitialize() */
			bool onInitialize () noexcept override;

			/** @copydoc EmEn::ServiceInterface::onTerminate() */
			bool onTerminate () noexcept override;

			/**
			 * @brief Clears primary command buffers.
			 */
			void clearCommandBuffers () noexcept;

			PrimaryServices & m_primaryServices;
			Vulkan::Instance & m_vulkanInstance;
			std::shared_ptr< Vulkan::Device > m_device;
			Vulkan::TransferManager m_transferManager;
			Vulkan::LayoutManager m_layoutManager;
			std::vector< ServiceInterface * > m_subServicesEnabled;
			std::shared_ptr< Vulkan::DescriptorPool > m_descriptorPool;
			std::shared_ptr< Vulkan::CommandPool > m_commandPool;
			std::vector< std::shared_ptr< Vulkan::CommandBuffer > > m_commandBuffers;
			std::map< size_t, std::shared_ptr< Vulkan::PipelineLayout > > m_pipelineLayouts;
			std::map< size_t, std::shared_ptr< Vulkan::ComputePipeline > > m_pipelines;
			bool m_accelerationAvailable{false};
	};
}
