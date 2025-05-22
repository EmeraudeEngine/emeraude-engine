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

/* Engine configuration file. */
#include "emeraude_config.hpp"

/* STL inclusions. */
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
	 * @extends EmEn::Libs::ObservableTrait Each service should be observable.
	 */
	class ServiceInterface : public Libs::NameableTrait, public Libs::ObservableTrait
	{
		public:

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
			bool initialize () noexcept;

			/**
			 * @brief Starts the service and register the pointer into a service list.
			 * @note This version ensures each service in order for an automatic cleaning.
			 * @param services A reference to auto register the service for shutdown.
			 * @return bool.
			 */
			bool initialize (std::vector< ServiceInterface * > & services) noexcept;

			/**
			 * @brief Terminates the service.
			 * @return bool.
			 */
			bool terminate () noexcept;

			/**
			 * @brief Returns whether the service is up and available.
			 * @warning This function must reflect the method ServiceInterface::onInitialize() has been called!
			 * @return bool.
			 */
			[[nodiscard]]
			virtual bool usable () const noexcept = 0;

		protected:

			/**
			 * @brief Constructs a service.
			 * @param serviceInstanceName The name of the service for print information.
			 */
			explicit ServiceInterface (const std::string & serviceInstanceName) noexcept;

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
