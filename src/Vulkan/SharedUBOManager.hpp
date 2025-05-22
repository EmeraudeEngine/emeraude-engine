/*
 * src/Vulkan/SharedUBOManager.hpp
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

/* Local inclusions for inheritances. */
#include "ServiceInterface.hpp"

/* Local inclusions for usages. */
#include "SharedUniformBuffer.hpp"
#include "Types.hpp"

namespace EmEn::Vulkan
{
	/**
	 * @brief The shared UBO manager class.
	 * @extends EmEn::ServiceInterface This is a service.
	 */
	class SharedUBOManager final : public ServiceInterface
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"SharedUBOManagerService"};

			/** @brief Observable class unique identifier. */
			static const size_t ClassUID;

			/**
			 * @brief Constructs a shared UBO manager service.
			 * @param type The GPU work type.
			 */
			explicit SharedUBOManager (GPUWorkType type) noexcept;

			/**
			 * @brief Copy constructor.
			 * @param copy A reference to the copied instance.
			 */
			SharedUBOManager (const SharedUBOManager & copy) noexcept = delete;

			/**
			 * @brief Move constructor.
			 * @param copy A reference to the copied instance.
			 */
			SharedUBOManager (SharedUBOManager && copy) noexcept = delete;

			/**
			 * @brief Copy assignment.
			 * @param copy A reference to the copied instance.
			 * @return SharedUBOManager &
			 */
			SharedUBOManager & operator= (const SharedUBOManager & copy) noexcept = delete;

			/**
			 * @brief Move assignment.
			 * @param copy A reference to the copied instance.
			 * @return SharedUBOManager &
			 */
			SharedUBOManager & operator= (SharedUBOManager && copy) noexcept = delete;

			/**
			 * @brief Destructs the shared UBO manager service.
			 */
			~SharedUBOManager () override;

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
				return m_device != nullptr ? m_flags[ServiceInitialized] : false;
			}

			/**
			 * @brief Sets the device that will be used with this manager.
			 * @param device A reference to a device smart pointer.
			 * @return void
			 */
			void
			setDevice (const std::shared_ptr< Device > & device) noexcept
			{
				m_device = device;
			}

			/**
			 * @brief Creates a shared buffer uniform.
			 * @param name A reference to a string.
			 * @param uniformBlockSize The size of the uniform block.
			 * @param maxElementCount The max number of element to hold in one UBO. Default, compute the maximum according to structure size and UBO properties.
			 * @return std::shared_ptr< SharedUniformBuffer >
			 */
			[[nodiscard]]
			std::shared_ptr< SharedUniformBuffer > createSharedUniformBuffer (const std::string & name, uint32_t uniformBlockSize, uint32_t maxElementCount = 0) noexcept;

			/**
			 * @brief Creates a shared dynamic buffer uniform.
			 * @param name A reference to a string.
			 * @param descriptorSetCreator A reference to a function to define the descriptor set.
			 * @param uniformBlockSize The size of the uniform block.
			 * @param maxElementCount The max number of element to hold in one UBO. Default, compute the maximum according to structure size and UBO properties.
			 * @return std::shared_ptr< SharedUniformBuffer >
			 */
			[[nodiscard]]
			std::shared_ptr< SharedUniformBuffer > createSharedUniformBuffer (const std::string & name, const SharedUniformBuffer::DescriptorSetCreator & descriptorSetCreator, uint32_t uniformBlockSize, uint32_t maxElementCount = 0) noexcept;

			/**
			 * @brief Returns a named shared buffer uniform.
			 * @param name A reference to a string.
			 * @return std::shared_ptr< SharedUniformBuffer >
			 */
			[[nodiscard]]
			std::shared_ptr< SharedUniformBuffer > getSharedUniformBuffer (const std::string & name) const noexcept;

			/**
			 * @brief Destroys a shared uniform buffer by its pointer.
			 * @param pointer A reference to shared uniform buffer smart pointer.
			 * @return bool
			 */
			bool destroySharedUniformBuffer (const std::shared_ptr< SharedUniformBuffer > & pointer) noexcept;

			/**
			 * @brief Destroys a shared uniform buffer by its name.
			 * @param name A reference to a string.
			 * @return bool
			 */
			bool destroySharedUniformBuffer (const std::string & name) noexcept;

			/**
			 * @brief Returns the instance of the shared UBO manager.
			 * @param type The GPU work type.
			 * @return SharedUBOManager *
			 */
			[[nodiscard]]
			static SharedUBOManager *
			instance (GPUWorkType type) noexcept
			{
				return s_instances.at(static_cast< size_t >(type));
			}

		private:

			/** @copydoc EmEn::ServiceInterface::onInitialize() */
			bool onInitialize () noexcept override;

			/** @copydoc EmEn::ServiceInterface::onTerminate() */
			bool onTerminate () noexcept override;

			/* Flag names */
			static constexpr auto ServiceInitialized{0UL};

			static std::array< SharedUBOManager *, 2 > s_instances;

			std::shared_ptr< Device > m_device;
			std::map< std::string, std::shared_ptr< SharedUniformBuffer > > m_sharedUniformBuffers;
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
