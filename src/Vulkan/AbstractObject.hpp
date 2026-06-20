/*
 * src/Vulkan/AbstractObject.hpp
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
#include "emeraude_config.hpp"

/* STL inclusions. */
#include <iostream>
#include <map>
#include <sstream>
#include <string>

/* Third-party inclusions. */
#include <vulkan/vulkan.h>

/* Local inclusions for usages. */
#include "Tracer.hpp"

namespace EmEn::Vulkan
{
	/**
	 * @brief Debug policy for Vulkan object identification. This will hold a 'std::string' to help keep track of objects.
	 */
	class IdentifierDebugPolicy
	{
		public:

			/**
			 * @brief Sets an identifier to the vulkan to ease the debugging.
			 * @param classId A string pointer for the class holding the Vulkan object.
			 * @param instanceId A reference to a string for the instance identifier.
			 * @param vulkanObjectName A string pointer for the type of Vulkan object;
			 * @return void
			 */
			void
			set (const char * classId, const std::string & instanceId, const char * vulkanObjectName) noexcept
			{
				std::stringstream identifier;

				identifier << classId << '-' << instanceId << '-' << vulkanObjectName;

				m_identifier = identifier.str();
			}

			/**
			 * @brief Returns the vulkan object identifier.
			 * @return const std::string &
			 */
			[[nodiscard]]
			const std::string &
			get () const noexcept
			{
				return m_identifier;
			}

		private:

			std::string m_identifier;
	};

	/**
	 * @brief Release policy for Vulkan object identification. This helps to remove all cost from Vulkan object identification in release with code optimization.
	 */
	class IdentifierReleasePolicy
	{
		public:

			/** @brief Dummy function meant to be removed by the compiler. */
			void
			set (const char *, const std::string &, const char *) noexcept
			{

			}

			/** @brief Dummy function meant to be removed by the compiler. */
			[[nodiscard]]
			const std::string &
			get () const noexcept
			{
				static const std::string empty;

				return empty;
			}
	};

#ifdef DEBUG
	using Identifier = IdentifierDebugPolicy;
#else
	using Identifier = IdentifierReleasePolicy;
#endif

	/**
	 * @brief Base of all Vulkan API objects.
	 */
	class AbstractObject
	{
		public:

			/**
			 * @brief Copy constructor.
			 * @param copy A reference to the copied instance.
			 */
			AbstractObject (const AbstractObject & copy) noexcept = default;

			/**
			 * @brief Move constructor.
			 * @param copy A reference to the copied instance.
			 */
			AbstractObject (AbstractObject && copy) noexcept = default;

			/**
			 * @brief Copy assignment.
			 * @param copy A reference to the copied instance.
			 * @return AbstractObject &
			 */
			AbstractObject & operator= (const AbstractObject & copy) noexcept = default;

			/**
			 * @brief Move assignment.
			 * @param copy A reference to the copied instance.
			 * @return AbstractObject &
			 */
			AbstractObject & operator= (AbstractObject && copy) noexcept = default;

			/**
			 * @brief Destructs a base vulkan object.
			 */
			virtual
			~AbstractObject ()
			{
				if constexpr ( VulkanTrackingDebugEnabled )
				{
					const auto * identifier = this->identifier().empty() ? "***UNIDENTIFIED***" : this->identifier().data();

					if ( m_isCreated && !m_isDestroyed )
					{
						TraceError{"VulkanObject"} << "A Vulkan object ('" << identifier << "' " << this << ") is not correctly destroyed !";
					}

					std::cout << "[DEBUG:VK_TRACKING] A Vulkan object ('" << identifier << "' @" << this << ") destructed !" "\n";

					s_tracking.erase(this);
				}
				else
				{
					if ( m_isCreated && !m_isDestroyed )
					{
						TraceError{"VulkanObject"} << "A Vulkan object is not correctly destroyed !";
					}
				}
			}

			/**
			 * @brief Sets an identifier to the vulkan to ease the debugging.
			 * @param classId A string pointer for the class holding the Vulkan object.
			 * @param instanceId A reference to a string for the instance identifier.
			 * @param vulkanObjectName A string pointer for the type of Vulkan object;
			 * @return void
			 */
			void
			setIdentifier (const char * classId, const std::string & instanceId, const char * vulkanObjectName) noexcept
			{
				m_identifier.set(classId, instanceId, vulkanObjectName);

				if constexpr ( VulkanTrackingDebugEnabled )
				{
					s_tracking[this] = this->identifier();

					std::cout << "[DEBUG:VK_TRACKING] A Vulkan object ('" << this->identifier() << "', @" << this << ") is marked !" "\n";
				}
			}

			/**
			 * @brief Returns the vulkan object identifier.
			 * @return const std::string &
			 */
			[[nodiscard]]
			const std::string &
			identifier () const noexcept
			{
				return m_identifier.get();
			}

			/**
			 * @brief Pushes this object's identifier to Vulkan as a debug name, so validation
			 * messages and GPU captures name the object instead of showing a raw handle.
			 * @note setIdentifier() runs BEFORE the handle exists, so it can only STORE the name;
			 * this must be called from createOnHardware() AFTER the handle is created. No-op if
			 * VK_EXT_debug_utils is unavailable, the handle is null, or the name is empty (release).
			 * @param device The Vulkan device handle owning the object.
			 * @param objectType The VkObjectType of the object.
			 * @param objectHandle The object handle, reinterpreted as uint64_t.
			 * @return void
			 */
			void
			setVulkanObjectName (VkDevice device, VkObjectType objectType, uint64_t objectHandle) const noexcept
			{
				if ( device == VK_NULL_HANDLE || objectHandle == 0 )
				{
					return;
				}

				const auto & name = this->identifier();

				if ( name.empty() )
				{
					return;
				}

				const auto fpSetName = reinterpret_cast< PFN_vkSetDebugUtilsObjectNameEXT >(vkGetDeviceProcAddr(device, "vkSetDebugUtilsObjectNameEXT"));

				if ( fpSetName == nullptr )
				{
					return;
				}

				const VkDebugUtilsObjectNameInfoEXT nameInfo{
					.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
					.pNext = nullptr,
					.objectType = objectType,
					.objectHandle = objectHandle,
					.pObjectName = name.c_str()
				};

				static_cast< void >(fpSetName(device, &nameInfo));
			}

			/**
			 * @brief Returns whether the object is in video memory and usable.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isCreated () const noexcept
			{
				return m_isCreated;
			}

			static inline std::map< void *, std::string > s_tracking{};

		protected:

			/**
			 * @brief Constructs a base vulkan object.
			 */
			AbstractObject () noexcept
			{
				if constexpr ( VulkanTrackingDebugEnabled )
				{
					s_tracking[this] = "";

					std::cout << "[DEBUG:VK_TRACKING] A Vulkan object (@" << this << ") constructed !" "\n";
				}
			}

			/**
			 * @brief For development purpose, this should be called by the child class constructor if everything is OK.
			 * @return void
			 */
			void
			setCreated () noexcept
			{
				m_isCreated = true;
			}

			/**
			 * @brief For development purpose, this should be called by the child class destructor if everything is OK.
			 * @return void
			 */
			void
			setDestroyed () noexcept
			{
				m_isDestroyed = true;
			}

		private:
			
			[[no_unique_address]]
			Identifier m_identifier; /* NOTE: In release mode, this shouldn't take any memory space. */
			bool m_isCreated{false};
			bool m_isDestroyed{false};
	};
}
