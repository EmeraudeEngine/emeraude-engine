/*
 * src/Vulkan/AbstractObject.hpp
 * This file is part of Emeraude-Engine
 *
 * Copyright (C) 2010-2024 - "LondNoir" <londnoir@gmail.com>
 *
 * Emeraude-Engine is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Emeraude-Engine is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Emeraude-Engine; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA  02110-1301  USA
 *
 * Complete project and additional information can be found at :
 * https://bitbucket.org/londnoir/emeraude-engine
 *
 * --- THIS IS AUTOMATICALLY GENERATED, DO NOT CHANGE ---
 */

#pragma once

/* Engine configuration file. */
#include "emeraude_config.hpp"

/* STL inclusions. */
#include <array>
#include <string>
#include <sstream>
#ifdef VK_TRACKING_ENABLED
#include <map>
#endif

/* Third-party inclusions. */
#include <vulkan/vulkan.h>

namespace Emeraude::Vulkan
{
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
			virtual ~AbstractObject ();

#ifdef VK_TRACKING_ENABLED

			/**
			 * @brief Sets an identifier to the vulkan to ease the debugging.
			 * @return void
			 */
			void setIdentifier (const std::string & identifier) noexcept;

#else

			/**
			 * @brief Sets an identifier to the vulkan to ease the debugging.
			 * @return void
			 */
			void
			setIdentifier (const std::string & identifier) noexcept
			{
				m_identifier = identifier;
			}

#endif

			/**
			 * @brief Sets an identifier to the vulkan to ease the debugging.
			 * @param classId A string pointer for the class holding the Vulkan object.
			 * @param instanceId A reference to a string for the instance of the class holding the Vulkan object.
			 * @param vulkanObjectName A string pointer for the type of Vulkan object;
			 * @return void
			 */
			void
			setIdentifier (const char * classId, const std::string & instanceId, const char * vulkanObjectName) noexcept
			{
				this->setIdentifier((std::stringstream{} << classId << '-' << instanceId << '-' << vulkanObjectName).str());
			}

			/**
			 * @brief Returns the vulkan object identifier.
			 * @return const std::string &
			 */
			[[nodiscard]]
			const std::string &
			identifier () const noexcept
			{
				return m_identifier;
			}

			/**
			 * @brief Returns whether the object is in video memory and usable.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isCreated () const noexcept
			{
				return m_flags[Created];
			}

#ifdef VK_TRACKING_ENABLED
			static std::map< void *, std::string > s_tracking;
#endif

		protected:

#ifdef VK_TRACKING_ENABLED
			/**
			 * @brief Constructs a base vulkan object.
			 */
			AbstractObject () noexcept;
#else
			/**
			 * @brief Constructs a base vulkan object.
			 */
			AbstractObject () noexcept = default;
#endif

			/**
			 * @brief For development purpose, this should be called by the child class constructor if everything is OK.
			 * @return void
			 */
			void
			setCreated () noexcept
			{
				m_flags[Created] = true;
			}

			/**
			 * @brief For development purpose, this should be called by the child class destructor if everything is OK.
			 * @return void
			 */
			void
			setDestroyed () noexcept
			{
				m_flags[Destroyed] = true;
			}

		private:

			/* Flag names. */
			static constexpr auto Created{0UL};
			static constexpr auto Destroyed{1UL};

			std::string m_identifier;
			std::array< bool, 8 > m_flags{
				false/*Created*/,
				false/*Destroyed*/,
				false/*UNUSED*/,
				false/*UNUSED*/,
				false/*UNUSED*/,
				false/*UNUSED*/,
				false/*UNUSED*/,
				false/*UNUSED*/
			};
	};
}
