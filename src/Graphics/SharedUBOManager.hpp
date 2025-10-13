/*
 * src/Graphics/SharedUBOManager.hpp
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

namespace EmEn::Graphics
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

			/**
			 * @brief Constructs a shared UBO manager service.
			 * @param renderer A reference to the graphics renderer.
			 */
			explicit
			SharedUBOManager (Renderer & renderer) noexcept
				: ServiceInterface{ClassId},
				m_renderer{renderer}
			{

			}

			/**
			 * @brief Sets the device that will be used with this manager.
			 * @param device A reference to a device smart pointer.
			 * @return void
			 */
			void
			setDevice (const std::shared_ptr< Vulkan::Device > & device) noexcept
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
			std::shared_ptr< SharedUniformBuffer > createSharedUniformBuffer (const std::string & name, const SharedUniformBuffer::descriptor_set_creator_t & descriptorSetCreator, uint32_t uniformBlockSize, uint32_t maxElementCount = 0) noexcept;

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

		private:

			/** @copydoc EmEn::ServiceInterface::onInitialize() */
			bool onInitialize () noexcept override;

			/** @copydoc EmEn::ServiceInterface::onTerminate() */
			bool onTerminate () noexcept override;

			Renderer & m_renderer;
			std::shared_ptr< Vulkan::Device > m_device;
			std::map< std::string, std::shared_ptr< SharedUniformBuffer > > m_sharedUniformBuffers;
	};
}
