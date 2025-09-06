/*
 * src/ServiceInterface.hpp
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
#include <iostream>
#include <string>
#include <vector>

/* Local inclusions for inheritances. */
#include "Libs/NameableTrait.hpp"
#include "Libs/ObservableTrait.hpp"

namespace EmEn
{
	/**
	 * @brief The service interface.
	 * @note A service cannot be duplicated and should act like a singleton.
	 * @extends EmEn::Libs::NameableTrait Each service has a name.
	 */
	class ServiceInterface : public Libs::NameableTrait
	{
		public:

			/** @brief Tracer tag for print. */
			static constexpr auto TracerTag{"ServiceInterface"};

			/**
			 * @brief Copy constructor.
			 * @note A service cannot be duplicated.
			 * @param copy A reference to the copied instance.
			 */
			ServiceInterface (const ServiceInterface & copy) noexcept = delete;

			/**
			 * @brief Move constructor.
			 * @note A service cannot be duplicated.
			 * @param copy A reference to the copied instance.
			 */
			ServiceInterface (ServiceInterface && copy) noexcept = delete;

			/**
			 * @brief Copy assignment.
			 * @note A service cannot be duplicated.
			 * @param copy A reference to the copied instance.
			 * @return ServiceInterface &
			 */
			ServiceInterface & operator= (const ServiceInterface & copy) noexcept = delete;

			/**
			 * @brief Move assignment.
			 * @note A service cannot be duplicated.
			 * @param copy A reference to the copied instance.
			 * @return ServiceInterface &
			 */
			ServiceInterface & operator= (ServiceInterface && copy) noexcept = delete;

			/**
			 * @brief Destructs the service.
			 */
			~ServiceInterface () override = default;

			/**
			 * @brief Starts the service.
			 * @return bool.
			 */
			[[nodiscard]]
			bool
			initialize () noexcept
			{
				if ( this->usable() )
				{
					/* NOTE: This should never happen! */
					std::cerr <<
						"The service '" << this->name() << "' looks like already initialized !" "\n"
						"The method ServiceInterface::usable() must dynamically report if the service has been initialized and usable !" "\n";

					return false;
				}

				return this->onInitialize();
			}

			/**
			 * @brief Starts the service and register the pointer into a service list.
			 * @note This version ensures each service in order for an automatic cleaning.
			 * @param services A reference to auto register the service for shutdown.
			 * @return bool.
			 */
			[[nodiscard]]
			bool
			initialize (std::vector< ServiceInterface * > & services) noexcept
			{
				if ( !this->initialize() )
				{
					return false;
				}

				services.emplace_back(this);

				return true;
			}

			/**
			 * @brief Terminates the service.
			 * @return bool.
			 */
			bool
			terminate () noexcept
			{
				return this->onTerminate();
			}

			/**
			 * @brief Returns whether the service is up and available.
			 * @warning This function must reflect the method ServiceInterface::onInitialize() has been called!
			 * @return bool.
			 */
			[[nodiscard]]
			virtual bool usable () const noexcept = 0;

		protected:

			/**
			 * @brief Constructs a service interface.
			 * @param serviceName A reference to a string [std::move].
			 */
			explicit
			ServiceInterface (std::string serviceName) noexcept
				: NameableTrait{std::move(serviceName)}
			{

			}

			/**
			 * @brief This method must be overridden by the final service on initialization.
			 * @return bool
			 */
			virtual bool onInitialize () noexcept = 0;

			/**
			 * @brief This method must be overridden by the final service on termination.
			 * @return bool
			 */
			virtual bool onTerminate () noexcept = 0;
	};
}
