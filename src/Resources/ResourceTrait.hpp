/*
 * src/Resources/ResourceTrait.hpp
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
#include <cstdint>
#include <string>
#include <vector>
#include <memory>
#include <mutex>
#include <filesystem>

/* Third-party inclusions. */
#ifndef JSON_USE_EXCEPTION
#define JSON_USE_EXCEPTION 0
#endif
#include "json/json.h"

/* Local inclusions for inheritances. */
#include "Libs/NameableTrait.hpp"
#include "Libs/FlagTrait.hpp"
#include "Libs/ObservableTrait.hpp"

/* Local inclusions for usages. */
#include "Types.hpp"

namespace EmEn::Resources
{
	/**
	 * @brief This class describes a loadable resource with dependencies.
	 * @note [OBS][SHARED-OBSERVABLE]
	 * @extends std::enable_shared_from_this A resource should self-replicate its smart-pointer.
	 * @extends EmEn::Libs::NameableTrait A resource is always named.
	 * @extends EmEn::Libs::FlagTrait A resource needs flags to change the behavior of resource construction.
	 * @extends EmEn::Libs::ObservableTrait A resource is observable to keep track of loading states.
	 */
	class ResourceTrait : public std::enable_shared_from_this< ResourceTrait >, public Libs::NameableTrait, public Libs::FlagTrait< uint32_t >, public Libs::ObservableTrait
	{
		public:

			/** @brief Observable notification codes. */
			enum NotificationCode
			{
				LoadFinished,
				LoadFailed,
				/* Enumeration boundary. */
				MaxEnum
			};

			/** @brief Enables resource information in the terminal. */
			inline static bool s_verboseEnabled{false};
			/** @brief Enables the resources conversion warning messages in the console. */
			inline static bool s_quietConversion{true};

			/**
			 * @brief Copy constructor.
			 * @param copy A reference to the copied instance.
			 */
			ResourceTrait (const ResourceTrait & copy) noexcept = delete;

			/**
			 * @brief Move constructor.
			 * @param copy A reference to the copied instance.
			 */
			ResourceTrait (ResourceTrait && copy) noexcept = delete;

			/**
			 * @brief Copy assignment.
			 * @param copy A reference to the copied instance.
			 * @return ResourceTrait &
			 */
			ResourceTrait & operator= (const ResourceTrait & copy) noexcept = delete;

			/**
			 * @brief Move assignment.
			 * @param copy A reference to the copied instance.
			 * @return ResourceTrait &
			 */
			ResourceTrait & operator= (ResourceTrait && copy) noexcept = delete;

			/**
			 * @brief Destructs the resource.
			 */
			~ResourceTrait () override;

			/**
			 * @brief Returns whether the resource is the top of a loadable object chain.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isTopResource () const noexcept
			{
				return m_parentsToNotify.empty();
			}

			/**
			 * @brief Returns the number of dependencies still waiting to be loaded.
			 * @return size_t
			 */
			[[nodiscard]]
			size_t
			dependencyCount () const noexcept
			{
				return m_dependenciesToWaitFor.size();
			}

			/**
			 * @brief Returns whether this resource object is newly created and no loading is currently occurring.
			 * @note This method is useful when using Container::getOrCreate() with a resource.
			 * @warning If you want to know if the resource is not yet loaded, use "!AbstractChainableLoading::isLoaded()".
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isUnloaded () const noexcept
			{
				return m_status == Status::Unloaded;
			}

			/**
			 * @brief Returns whether this resource is loading.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isLoading () const noexcept
			{
				return m_status == Status::Loading;
			}

			/**
			 * @brief Returns whether this resource is loaded.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isLoaded () const noexcept
			{
				return m_status == Status::Loaded;
			}

			/**
			 * @brief Enables the manual loading of the resource.
			 * @note This method will fail if the resource is currently loading in automatic mode.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			enableManualLoading () noexcept
			{
				return this->initializeEnqueuing(true);
			}

			/**
			 * @brief Sets this resource status in manual loading mode.
			 * @return bool
			 */
			bool setManualLoadSuccess (bool status) noexcept;

			/**
			 * @brief Returns the current status of the resource.
			 * @return Status
			 */
			[[nodiscard]]
			Status
			status () const noexcept
			{
				return m_status;
			}

			/**
			 * @brief Declares the resource is using the direct loading mode.
			 * @return void
			 */
			void
			setDirectLoadingHint () noexcept
			{
				this->enableFlag(DirectLoading);
			}

			/**
			 * @brief Returns the label of resource class.
			 * @return const char *
			 */
			[[nodiscard]]
			virtual const char * classLabel () const noexcept = 0;

			/**
			 * @brief Loads a fully functional dummy resource.
			 * @return bool
			 */
			virtual bool load () noexcept = 0;

			/**
			 * @brief Loads a resource from a disk file.
			 * @note If not overridden, this method will open the file and tries to read a JSON data and call the load(data) after.
			 * @param filepath A reference to a filesystem path.
			 * @return bool
			 */
			virtual bool load (const std::filesystem::path & filepath) noexcept;

			/**
			 * @brief Loads a resource from a JsonCPP object.
			 * @param data A reference to a JsonCPP object.
			 * @return bool
			 */
			virtual bool load (const Json::Value & data) noexcept = 0;

			/**
			 * @brief When the resource is loaded in memory, this method returns the size in bytes occupied in RAM.
			 * @return size_t
			 */
			[[nodiscard]]
			virtual
			size_t
			memoryOccupied () const noexcept
			{
				return sizeof(*this);
			}

		protected:

			/**
			 * @brief Constructs an abstract resource.
			 * @param resourceName A string for the resource name [std::move].
			 * @param initialResourceFlags The initial resource flags.
			 */
			ResourceTrait (std::string resourceName, uint32_t initialResourceFlags) noexcept
				: NameableTrait{std::move(resourceName)},
				FlagTrait{initialResourceFlags}
			{

			}

			/**
			 * @brief Returns whether the resource use direct loading mode from manager.
			 * @return Status
			 */
			[[nodiscard]]
			bool
			isDirectLoading () const noexcept
			{
				return this->isFlagEnabled(DirectLoading);
			}

			/**
			 * @brief Sets the status of this resource to Status::Loading.
			 * @warning If this method return false, you should stop the loading.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			beginLoading () noexcept
			{
				/* NOTE: The manual enqueuing is disabled. */
				return this->initializeEnqueuing(false);
			}

			/**
			 * @brief Adds a new dependency to this loading object.
			 * @note If the sub resource is already loaded, it won't be added.
			 * @param dependency A point to the dependency.
			 * @return bool
			 */
			[[nodiscard]]
			bool addDependency (const std::shared_ptr< ResourceTrait > & dependency) noexcept;

			/**
			 * @brief Sets the current status of loading by the inherited class.
			 * @note If status is Status::Loaded, this will return true.
			 * @param status Set the resource loading status.
			 * @return bool
			 */
			[[nodiscard]]
			bool setLoadSuccess (bool status) noexcept;

			/**
			 * @brief This event is called when every dependency is loaded.
			 * @note This method should be overridden if something special has to be done when loaded.
			 * @return bool
			 */
			[[nodiscard]]
			virtual
			bool
			onDependenciesLoaded () noexcept
			{
				return true;
			}

			/**
			 * @brief Extracts the resource name from a file path.
			 * @note "APP-DATA/data-stores/Movies/sub_directory/res_name.ext" => "sub_directory/res_name.ext"
			 * @param filepath A reference to a file path.
			 * @param storeName A reference to a string for the initial store name.
			 * @return std::string
			 */
			[[nodiscard]]
			static std::string getResourceNameFromFilepath (const std::filesystem::path & filepath, const std::string & storeName) noexcept;

		private:

			/**
			 * @brief This private method is called from a child resource through m_parents set to relaunch the parent resource dependencies check.
			 * @param dependency A pointer to the loaded dependency. Suitable to unlink the resource from dependency set.
			 * @return void
			 */
			void dependencyLoaded (const std::shared_ptr< ResourceTrait > & dependency) noexcept;

			/**
			 * @brief Checks this resource for every dependency below it. If everything is loaded, the method launches
			 * a parent check, or if this resource is a top level, the final call to AbstractChainableLoading::onDependenciesLoaded()
			 * to notice the resource is available.
			 * @return void
			 */
			void checkDependencies () noexcept;

			/**
			 * @brief This is the real method that begins the first stage of resource loading.
			 * @param manual Boolean that denotes if the status is Enqueuing or ManualEnqueuing.
			 * @return bool
			 */
			[[nodiscard]]
			bool initializeEnqueuing (bool manual) noexcept;

			/* Flag names. */
			static constexpr uint32_t DirectLoading = 1U << 31;

			std::vector< std::shared_ptr< ResourceTrait > > m_parentsToNotify;
			std::vector< std::shared_ptr< ResourceTrait > > m_dependenciesToWaitFor;
			Status m_status{Status::Unloaded};
			mutable std::mutex m_dependenciesAccess;
	};
}
