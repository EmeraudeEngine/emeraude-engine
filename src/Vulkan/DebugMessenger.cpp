/*
 * src/Vulkan/DebugMessenger.cpp
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

#include "DebugMessenger.hpp"

/* STL inclusions. */
#include <iostream>
#include <sstream>

/* Local inclusions. */
#include "Instance.hpp"
#include "Tracer.hpp"

namespace EmEn::Vulkan
{
	using namespace Libs;

	DebugMessenger::DebugMessenger (const Instance & instance) noexcept
		: m_instance{instance},
		m_createInfo{DebugMessenger::getCreateInfo()}
	{
		auto vkCreateDebugUtilsMessengerEXT = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(m_instance.handle(), "vkCreateDebugUtilsMessengerEXT");

		if ( vkCreateDebugUtilsMessengerEXT == nullptr )
		{
			Tracer::error(ClassId, "VK_EXT_debug_utils extension unavailable ! Unable to get vkCreateDebugUtilsMessengerEXT() function.");

			return;
		}

		switch ( vkCreateDebugUtilsMessengerEXT(m_instance.handle(), &m_createInfo, nullptr, &m_handle) )
		{
			case VK_SUCCESS :
				Tracer::success(ClassId, "Vulkan debug messenger created !");
				break;

			case VK_ERROR_OUT_OF_HOST_MEMORY :
				Tracer::error(ClassId, "Unable to create the Vulkan debug messenger : Out of memory (Host) !");
				return;

			default :
				Tracer::error(ClassId, "Unable to create the Vulkan debug messenger : Unknown error !");
				return;
		}

		this->setCreated();
	}

	DebugMessenger::~DebugMessenger ()
	{
		/* NOTE: An instance must exist to clean a debug messenger properly ! */
		if ( m_handle != VK_NULL_HANDLE )
		{
			auto vkDestroyDebugUtilsMessengerEXT = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(m_instance.handle(), "vkDestroyDebugUtilsMessengerEXT");

			if ( vkDestroyDebugUtilsMessengerEXT != nullptr )
			{
				vkDestroyDebugUtilsMessengerEXT(m_instance.handle(), m_handle, nullptr);
			}

			m_handle = VK_NULL_HANDLE;
		}

		this->setDestroyed();
	}

	VkDebugUtilsMessengerCreateInfoEXT
	DebugMessenger::getCreateInfo () noexcept
	{
		VkDebugUtilsMessengerCreateInfoEXT _createInfo{};
		_createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
		_createInfo.pNext = nullptr;
		_createInfo.flags = 0;
		_createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
		_createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
		_createInfo.pfnUserCallback = DebugMessenger::debugCallback;
		_createInfo.pUserData = nullptr; // Optional

		return _createInfo;
	}

	VkBool32
	DebugMessenger::debugCallback (VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT * pCallbackData, void * /*pUserData*/) noexcept
	{
		std::stringstream output;

		output << "[VK]";

		switch ( messageType )
		{
			case VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT :
				output << "[GENERAL]";
				break;

			case VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT :
				output << "[VALIDATION]";
				break;

			case VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT :
				output << "[PERFORMANCE]";
				break;

			case VK_DEBUG_UTILS_MESSAGE_TYPE_DEVICE_ADDRESS_BINDING_BIT_EXT :
				output << "[DEVICE_ADDRESS_BINDING]";
				break;

			case VK_DEBUG_UTILS_MESSAGE_TYPE_FLAG_BITS_MAX_ENUM_EXT :
			default:
				output << "[UNDEFINED]";
				break;
		}

		auto isError = false;

		switch ( messageSeverity )
		{
			case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT :
				output << "[VERBOSE]";
				break;

			case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT :
				output << "[INFO]";
				break;

			case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT :
				isError = true;

				output << "[WARNING]";
				break;

			case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT :
				isError = true;

				output << "[ERROR]";
				break;

			case VK_DEBUG_UTILS_MESSAGE_SEVERITY_FLAG_BITS_MAX_ENUM_EXT :
			default:
				isError = true;

				output << "[UNKNOWN]";
				break;
		}

		output <<
			"[ID:" << pCallbackData->pMessageIdName << " #" << pCallbackData->messageIdNumber << "]" "\n"
			"[MESSAGE:" << pCallbackData->pMessage << "]";

		if ( isError )
		{
			std::cerr << output.str() << "\n";
		}
		else
		{
			std::cout << output.str() << "\n";
		}

		return 1;
	}
}
