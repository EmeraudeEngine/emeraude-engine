/*
 * src/Vulkan/Utility.cpp
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

#include "Utility.hpp"

/* STL inclusions. */
#include <sstream>

/* Local inclusions. */
#include "Libs/Version.hpp"

namespace EmEn::Vulkan
{
	using namespace Libs;

	const char *
	vkResultToCString (VkResult code) noexcept
	{
		switch ( code )
		{
			case VK_SUCCESS :
				return "VK_SUCCESS";

			case VK_NOT_READY :
				return "VK_NOT_READY";

			case VK_TIMEOUT :
				return "VK_TIMEOUT";

			case VK_EVENT_SET :
				return "VK_EVENT_SET";

			case VK_EVENT_RESET :
				return "VK_EVENT_RESET";

			case VK_INCOMPLETE :
				return "VK_INCOMPLETE";

			case VK_ERROR_OUT_OF_HOST_MEMORY :
				return "VK_ERROR_OUT_OF_HOST_MEMORY";

			case VK_ERROR_OUT_OF_DEVICE_MEMORY :
				return "VK_ERROR_OUT_OF_DEVICE_MEMORY";

			case VK_ERROR_INITIALIZATION_FAILED :
				return "VK_ERROR_INITIALIZATION_FAILED";

			case VK_ERROR_DEVICE_LOST :
				return "VK_ERROR_DEVICE_LOST";

			case VK_ERROR_MEMORY_MAP_FAILED :
				return "VK_ERROR_MEMORY_MAP_FAILED";

			case VK_ERROR_LAYER_NOT_PRESENT :
				return "VK_ERROR_LAYER_NOT_PRESENT";

			case VK_ERROR_EXTENSION_NOT_PRESENT :
				return "VK_ERROR_EXTENSION_NOT_PRESENT";

			case VK_ERROR_FEATURE_NOT_PRESENT :
				return "VK_ERROR_FEATURE_NOT_PRESENT";

			case VK_ERROR_INCOMPATIBLE_DRIVER :
				return "VK_ERROR_INCOMPATIBLE_DRIVER";

			case VK_ERROR_TOO_MANY_OBJECTS :
				return "VK_ERROR_TOO_MANY_OBJECTS";

			case VK_ERROR_FORMAT_NOT_SUPPORTED :
				return "VK_ERROR_FORMAT_NOT_SUPPORTED";

			case VK_ERROR_FRAGMENTED_POOL :
				return "VK_ERROR_FRAGMENTED_POOL";

			case VK_ERROR_UNKNOWN :
				return "VK_ERROR_UNKNOWN";

			case VK_ERROR_OUT_OF_POOL_MEMORY :
				return "VK_ERROR_OUT_OF_POOL_MEMORY";

			case VK_ERROR_INVALID_EXTERNAL_HANDLE :
				return "VK_ERROR_INVALID_EXTERNAL_HANDLE";

			case VK_ERROR_FRAGMENTATION :
				return "VK_ERROR_FRAGMENTATION";

			case VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS : // VK_ERROR_INVALID_DEVICE_ADDRESS_EXT(deprecated)
				return "VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS";

			case VK_PIPELINE_COMPILE_REQUIRED : // VK_PIPELINE_COMPILE_REQUIRED_EXT(deprecated)
				return "VK_PIPELINE_COMPILE_REQUIRED";

			case VK_ERROR_NOT_PERMITTED_EXT : // VK_ERROR_NOT_PERMITTED(1.4)
				return "VK_ERROR_NOT_PERMITTED";

			case VK_ERROR_SURFACE_LOST_KHR :
				return "VK_ERROR_SURFACE_LOST_KHR";

			case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR :
				return "VK_ERROR_NATIVE_WINDOW_IN_USE_KHR";

			case VK_SUBOPTIMAL_KHR :
				return "VK_SUBOPTIMAL_KHR";

			case VK_ERROR_OUT_OF_DATE_KHR :
				return "VK_ERROR_OUT_OF_DATE_KHR";

			case VK_ERROR_INCOMPATIBLE_DISPLAY_KHR :
				return "VK_ERROR_INCOMPATIBLE_DISPLAY_KHR";

			case VK_ERROR_VALIDATION_FAILED_EXT :
				return "VK_ERROR_VALIDATION_FAILED_EXT";

			case VK_ERROR_INVALID_SHADER_NV :
				return "VK_ERROR_INVALID_SHADER_NV";

			case VK_ERROR_IMAGE_USAGE_NOT_SUPPORTED_KHR :
				return "VK_ERROR_IMAGE_USAGE_NOT_SUPPORTED_KHR";

			case VK_ERROR_VIDEO_PICTURE_LAYOUT_NOT_SUPPORTED_KHR :
				return "VK_ERROR_VIDEO_PICTURE_LAYOUT_NOT_SUPPORTED_KHR";

			case VK_ERROR_VIDEO_PROFILE_OPERATION_NOT_SUPPORTED_KHR :
				return "VK_ERROR_VIDEO_PROFILE_OPERATION_NOT_SUPPORTED_KHR";

			case VK_ERROR_VIDEO_PROFILE_FORMAT_NOT_SUPPORTED_KHR :
				return "VK_ERROR_VIDEO_PROFILE_FORMAT_NOT_SUPPORTED_KHR";

			case VK_ERROR_VIDEO_PROFILE_CODEC_NOT_SUPPORTED_KHR :
				return "VK_ERROR_VIDEO_PROFILE_CODEC_NOT_SUPPORTED_KHR";

			case VK_ERROR_VIDEO_STD_VERSION_NOT_SUPPORTED_KHR :
				return "VK_ERROR_VIDEO_STD_VERSION_NOT_SUPPORTED_KHR";

			case VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT :
				return "VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT";

			case VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT :
				return "VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT";

			case VK_THREAD_IDLE_KHR :
				return "VK_THREAD_IDLE_KHR";

			case VK_THREAD_DONE_KHR :
				return "VK_THREAD_DONE_KHR";

			case VK_OPERATION_DEFERRED_KHR :
				return "VK_OPERATION_DEFERRED_KHR";

			case VK_OPERATION_NOT_DEFERRED_KHR :
				return "VK_OPERATION_NOT_DEFERRED_KHR";

			case VK_ERROR_INVALID_VIDEO_STD_PARAMETERS_KHR :
				return "VK_ERROR_INVALID_VIDEO_STD_PARAMETERS_KHR";

			case VK_ERROR_COMPRESSION_EXHAUSTED_EXT :
				return "VK_ERROR_COMPRESSION_EXHAUSTED_EXT";

			case VK_ERROR_INCOMPATIBLE_SHADER_BINARY_EXT : //VK_INCOMPATIBLE_SHADER_BINARY_EXT(1.4)
				return "VK_INCOMPATIBLE_SHADER_BINARY_EXT";

			case 1000483000 : //VK_PIPELINE_BINARY_MISSING_KHR(1.4)
				return "VK_PIPELINE_BINARY_MISSING_KHR";

			case -1000483000 : //VK_ERROR_NOT_ENOUGH_SPACE_KHR(1.4)
				return "VK_ERROR_NOT_ENOUGH_SPACE_KHR";

			default:
				return "UNKNOWN_ERROR";
		}
	}

	std::string
	getItemListAsString (const std::vector< VkLayerProperties > & validationLayers) noexcept
	{
		if ( validationLayers.empty() )
		{
			return "No validation layers available !";
		}

		std::stringstream output;

		output << "Vulkan validation layers available on the sy :" "\n";

		for ( const auto & validationLayer : validationLayers )
		{
			const Version specVersion{validationLayer.specVersion};
			const Version implVersion{validationLayer.implementationVersion};

			output << '\t' << static_cast< const char * >(validationLayer.layerName) << " (" << specVersion << "/" << implVersion << ") : " << static_cast< const char * >(validationLayer.description) << "\n";
		}

		return output.str();
	}

	std::string
	getItemListAsString (const char * type, const std::vector< VkExtensionProperties > & extensions) noexcept
	{
		std::stringstream output;

		if ( extensions.empty() )
		{
			output << "No " << type << " extensions available !";
		}
		else
		{
			output << type << " extensions available :" "\n";

			for ( const auto & extension : extensions )
			{
				output << '\t' << static_cast< const char * >(extension.extensionName) << " (" << Version{extension.specVersion} << ")" "\n";
			}
		}

		return output.str();
	}
}
