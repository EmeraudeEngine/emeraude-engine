/*
 * src/Vulkan/LayoutManager.hpp
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
#include <array>
#include <map>
#include <memory>
#include <string>
#include <vector>

/* Third-party inclusions. */
#include <vulkan/vulkan.h>

/* Local inclusions for inheritances. */
#include "ServiceInterface.hpp"

/* Local inclusions for usages. */
#include "Types.hpp"

/* Forward declarations. */
namespace EmEn::Vulkan
{
	class Device;
	class PipelineLayout;
	class DescriptorSetLayout;
}

namespace EmEn::Vulkan
{
	/**
	 * @brief The layout manager service. This will save any descriptor set or pipeline layout in use.
	 * @extends EmEn::ServiceInterface This is a service.
	 */
	class LayoutManager final : public ServiceInterface
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"LayoutManager"};

			/** @brief Observable class unique identifier. */
			static const size_t ClassUID;

			/**
			 * @brief Constructs a descriptor set layout manager service.
			 * @param type The GPU work type.
			 */
			explicit LayoutManager (GPUWorkType type) noexcept;

			/**
			 * @brief Copy constructor.
			 * @param copy A reference to the copied instance.
			 */
			LayoutManager (const LayoutManager & copy) noexcept = delete;

			/**
			 * @brief Move constructor.
			 * @param copy A reference to the copied instance.
			 */
			LayoutManager (LayoutManager && copy) noexcept = delete;

			/**
			 * @brief Copy assignment.
			 * @param copy A reference to the copied instance.
			 * @return LayoutManager &
			 */
			LayoutManager & operator= (const LayoutManager & copy) noexcept = delete;

			/**
			 * @brief Move assignment.
			 * @param copy A reference to the copied instance.
			 * @return TransferManager &
			 */
			LayoutManager & operator= (LayoutManager && copy) noexcept = delete;

			/**
			 * @brief Destructs the descriptor set layout manager service.
			 */
			~LayoutManager () override;

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
			bool usable () const noexcept override;

			/**
			 * @brief Sets the device used by the layout manager.
			 * @return void
			 */
			void
			setDevice (const std::shared_ptr< Device > & device) noexcept
			{
				m_device = device;
			}

			/**
			 * @brief Returns the device of the layout manager.
			 * @return std::shared_ptr< Device >
			 */
			[[nodiscard]]
			std::shared_ptr< Device >
			device () const noexcept
			{
				return m_device;
			}

			/**
			 * @brief Tries to return an existing descriptor set layout.
			 * @warning The return can be nullptr.
			 * @param UUID A reference to a string.
			 * @return std::shared_ptr< DescriptorSetLayout >
			 */
			[[nodiscard]]
			std::shared_ptr< DescriptorSetLayout > getDescriptorSetLayout (const std::string & UUID) const noexcept;

			/**
			 * @brief Prepares a new descriptor set layout.
			 * @param UUID A reference to a string.
			 * @param createFlags The Vulkan flag at descriptor set layout creation. Default None.
			 * @return std::shared_ptr< DescriptorSetLayout >
			 */
			[[nodiscard]]
			std::shared_ptr< DescriptorSetLayout > prepareNewDescriptorSetLayout (const std::string & UUID, VkDescriptorSetLayoutCreateFlags createFlags = 0) const noexcept;

			/**
			 * @brief Saves a descriptor set layout for later use.
			 * @param descriptorSetLayout A reference to the new descriptor set layout smart pointer.
			 * @return bool
			 */
			[[nodiscard]]
			bool createDescriptorSetLayout (const std::shared_ptr< DescriptorSetLayout > & descriptorSetLayout) noexcept;

			/**
			 * @brief Returns an existing pipeline layout or create a new one corresponding to parameters.
			 * @param descriptorSetLayouts A reference to a list of descriptor set layout.
			 * @param pushConstantRanges A reference to push constant range list.
			 * @param createFlags The Vulkan flag at pipeline layout creation. Default None.
			 * @return std::shared_ptr< PipelineLayout >
			 */
			[[nodiscard]]
			std::shared_ptr< PipelineLayout > getPipelineLayout (const std::vector< std::shared_ptr< DescriptorSetLayout > > & descriptorSetLayouts, const std::vector< VkPushConstantRange > & pushConstantRanges = {}, VkPipelineLayoutCreateFlags createFlags = 0) noexcept;

			/**
			 * @brief Returns the instance of the transfer manager.
			 * @param type The layout manager work type.
			 * @return LayoutManager *
			 */
			[[nodiscard]]
			static LayoutManager *
			instance (GPUWorkType type) noexcept
			{
				return s_instances.at(static_cast< size_t >(type));
			}

		private:

			/** @copydoc EmEn::ServiceInterface::onInitialize() */
			bool onInitialize () noexcept override;

			/** @copydoc EmEn::ServiceInterface::onTerminate() */
			bool onTerminate () noexcept override;

			static std::array< LayoutManager *, 2 > s_instances;

			/* Flag names */
			static constexpr auto ServiceInitialized{0UL};

			std::shared_ptr< Device > m_device;
			std::map< std::string, std::shared_ptr< DescriptorSetLayout > > m_descriptorSetLayouts;
			std::map< std::string, std::shared_ptr< PipelineLayout > > m_pipelineLayouts;
			std::array< bool, 8 > m_flags{
				false/*ServiceInitialized*/,
				false/*UNUSED*/,
				false/*UNUSED*/,
				false/*UNUSED*/,
				false/*UNUSED*/,
				false/*UNUSED*/,
				false/*UNUSED*/,
				false/*UNUSED*/
			};
	};
}
