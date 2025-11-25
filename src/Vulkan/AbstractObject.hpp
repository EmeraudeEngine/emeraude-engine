/*
 * src/Vulkan/AbstractObject.hpp
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

/* Emeraude-Engine configuration. */
#include "emeraude_config.hpp"

/* STL inclusions. */
#include <iostream>
#include <sstream>
#include <string>
#include <map>

/* Third-party inclusions. */
#include <vulkan/vulkan.h>

/* Local inclusions for inheritances. */
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
