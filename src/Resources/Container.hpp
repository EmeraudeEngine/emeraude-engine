/*
 * src/Resources/Container.hpp
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
#include <any>
#include <functional>
#include <memory>
#include <string>
#include <tuple>
#include <unordered_map>
#include <utility>

/* Local inclusions for inheritances. */
#include "Libs/NameableTrait.hpp"
#include "Libs/ObservableTrait.hpp"
#include "Libs/ObserverTrait.hpp"

/* Local inclusions for usages. */
#include "Libs/IO/IO.hpp"
#include "Net/Manager.hpp"
#include "PrimaryServices.hpp"
#include "ServiceProvider.hpp"
#include "LoadingRequest.hpp"
#include "Types.hpp"

namespace EmEn::Resources
{
	/**
	 * @brief The common interface for all resource containers.
	 * @extends EmEn::Libs::NameableTrait A container is nameable.
	 * @extends EmEn::Libs::ObservableTrait A container can be observed.
	 */
	class ContainerInterface : public Libs::NameableTrait, public Libs::ObservableTrait
	{
		public:

			/**
			 * @brief Destructs the container interface.
			 */
			~ContainerInterface () override = default;

			/**
			 * @brief Sets the verbosity state for the container.
			 * @param state The state.
			 * @return void
			 */
			virtual void setVerbosity (bool state) noexcept = 0;

			/**
			 * @brief Initializes the container.
			 * @return bool
			 */
			virtual bool initialize () noexcept = 0;

			/**
			 * @brief Destroys the container.
			 * @return bool
			 */
			virtual bool terminate () noexcept = 0;

			/**
			 * @brief Returns the total memory consumed by loaded resources in bytes from the container.
			 * @return size_t
			 */
			[[nodiscard]]
			virtual size_t memoryOccupied () const noexcept = 0;

			/**
			 * @brief Returns the total memory consumed by loaded, but unused resources in bytes from the container.
			 * @return size_t
			 */
			[[nodiscard]]
			virtual size_t unusedMemoryOccupied () const noexcept = 0;

			/**
			 * @brief Clean up every unused resource and returns the number of removed resources.
			 * @return size_t
			 */
			virtual size_t unloadUnusedResources () noexcept = 0;

			/**
			 * @brief Returns the complexity of the resource.
			 * @return DepComplexity
			 */
			[[nodiscard]]
			virtual DepComplexity complexity () const noexcept = 0;

		protected:

			/**
			 * @brief Constructs a container interface.
			 * @param name A reference to a string.
			 */
			explicit
			ContainerInterface (const std::string & name) noexcept
				: NameableTrait{name}
			{

			}
	};

	/**
	 * @brief The resource manager template is responsible for loading asynchronous resources with dependencies and hold their lifetime.
	 * @note [OBS][STATIC-OBSERVABLE]
	 * @tparam resource_t The type of resources (The resource type is checked by LoadingRequest template).
	 * @extends EmEn::Resources::ContainerInterface This is a service.
	 * @extends EmEn::Libs::ObserverTrait The manager observer resource loading.
	 */
	template< typename resource_t >
	class Container final : public ContainerInterface, public Libs::ObserverTrait
	{
		public:

			/** @brief Observable notification codes. */
			enum NotificationCode
			{
				Unknown,
				LoadingProcessStarted,
				ResourceLoaded,
				LoadingProcessFinished,
				Progress,
				/* Enumeration boundary. */
				MaxEnum
			};

			/**
			 * @brief Constructs a resource manager for a specific resource from the template parameter.
			 * @param [OBS][STATIC-OBSERVER]
			 * @param primaryServices A reference to the primary services.
			 * @param serviceProvider A reference to the resource manager as a service provider.
			 * @param store A reference to the store.
			 * @param serviceName The name of the service.
			 */
			Container (const char * serviceName, PrimaryServices & primaryServices, ServiceProvider & serviceProvider, const std::shared_ptr< std::unordered_map< std::string, BaseInformation > > & store) noexcept
				: ContainerInterface{serviceName},
				m_primaryServices{primaryServices},
				m_serviceProvider{serviceProvider},
				m_localStore{store}
			{
				this->observe(&m_primaryServices.netManager());
			}

			/**
			 * @brief Returns the unique identifier for this class [Thread-safe].
			 * @return size_t
			 */
			static
			size_t
			getClassUID () noexcept
			{
				static const size_t classUID = EmEn::Libs::Hash::FNV1a(resource_t::ClassId);

				return classUID;
			}

			/** @copydoc EmEn::Libs::ObservableTrait::classUID() const */
			[[nodiscard]]
			size_t
			classUID () const noexcept override
			{
				return getClassUID();
			}

			/** @copydoc EmEn::Libs::ObservableTrait::is() const */
			[[nodiscard]]
			bool
			is (size_t classUID) const noexcept override
			{
				return classUID == getClassUID();
			}

			/** @copydoc EmEn::Resources::ContainerInterface::setVerbosity(bool) noexcept */
			void
			setVerbosity (bool state) noexcept override
			{
				m_verboseEnabled = state;
			}

			/** @copydoc EmEn::Resources::ContainerInterface::memoryOccupied() const noexcept */
			[[nodiscard]]
			size_t
			memoryOccupied () const noexcept override
			{
				const std::lock_guard< std::mutex > scopeLock{m_resourcesAccess};

				size_t bytes = 0;

				for ( const auto & resource : m_resources | std::views::values )
				{
					bytes += resource->memoryOccupied();
				}

				return bytes;
			}

			/** @copydoc EmEn::Resources::ContainerInterface::unusedMemoryOccupied() const noexcept */
			[[nodiscard]]
			size_t
			unusedMemoryOccupied () const noexcept override
			{
				const std::lock_guard< std::mutex > scopeLock{m_resourcesAccess};

				size_t bytes = 0;

				for ( const auto & resource : m_resources | std::views::values )
				{
					if ( resource.use_count() == 1 )
					{
						bytes += resource->memoryOccupied();
					}
				}

				return bytes;
			}

			/** @copydoc EmEn::Resources::ContainerInterface::unloadUnusedResources() noexcept */
			size_t
			unloadUnusedResources () noexcept override
			{
				const std::lock_guard< std::mutex > scopeLock{m_resourcesAccess};

				size_t unloadedResources = 0;

				if ( !m_resources.empty() )
				{
					for ( auto it = m_resources.begin(); it != m_resources.end(); )
					{
						const auto links = it->second.use_count();

						if ( links > 1 )
						{
							TraceDebug{resource_t::ClassId} << it->second->name() << " is still used " << links << " times !";
						}

						if ( links == 1 )
						{
							unloadedResources++;

							/* FIXME: Fails with some animated 2d textures. */
							it = m_resources.erase(it);
						}
						else
						{
							++it;
						}
					}

					TraceInfo{resource_t::ClassId} << unloadedResources << " resource(s) unloaded !";
				}

				return unloadedResources;
			}

			/** @copydoc EmEn::Resources::ContainerInterface::unloadUnusedResources() noexcept */
			DepComplexity
			complexity () const noexcept override
			{
				return resource_t::Complexity;
			}

			/**
			 * @brief Returns whether a resource is loaded and ready to use.
			 * @param resourceName A reference to a string.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isResourceLoaded (const std::string & resourceName) const noexcept
			{
				const std::lock_guard< std::mutex > scopeLock{m_resourcesAccess};

				return m_resources.contains(resourceName);
			}

			/**
			 * @brief Returns whether a resource exists.
			 * First the container will check in loaded resources, then in available (unloaded) resources in the store.
			 * @param resourceName A reference to a string.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isResourceExists (const std::string & resourceName) const noexcept
			{
				if ( m_localStore == nullptr )
				{
					return false;
				}

				const std::lock_guard< std::mutex > scopeLock{m_resourcesAccess};

				/* First, check in live resources.
				 * NOTE: Do not use Container::isResourceLoaded() to prevent from mutex deadlock. */
				if ( m_resources.contains(resourceName) )
				{
					return true;
				}

				return m_localStore->contains(resourceName);
			}

			/**
			 * @brief Returns all resource names from the store.
			 * @return std::vector< std::string >
			 */
			std::vector< std::string >
			getResourceNames () const noexcept
			{
				if ( m_localStore == nullptr )
				{
					return {};
				}

				std::vector< std::string > names;

				for ( const auto & name : *m_localStore | std::views::keys )
				{
					names.emplace_back(name);
				}

				return names;
			}

			/**
			 * @brief Creates a new resource.
			 * @note When creating a new resource, put '+' in front of the resource name to prevent it to be overridden from a store resource.
			 * @param resourceName A string with the name of the resource.
			 * @param resourceFlags The resource construction flags. Default none.
			 * @return std::shared_ptr< resource_t >
			 */
			[[nodiscard]]
			std::shared_ptr< resource_t >
			createResource (const std::string & resourceName, uint32_t resourceFlags = 0) noexcept
			{
				const std::lock_guard< std::mutex > scopeLock{m_resourcesAccess};

				return this->createResourceUnlocked(resourceName, resourceFlags);
			}

			/**
			 * @brief Adds a resource manually constructed to the store.
			 * @note When creating a new resource, put '+' in front of the resource name to prevent it to be overridden from a store resource.
			 * @param resource The manual resource.
			 * @return bool
			 */
			bool
			addResource (const std::shared_ptr< resource_t > & resource) noexcept
			{
				const std::lock_guard< std::mutex > scopeLock{m_resourcesAccess};

				auto loadedIt = m_resources.find(resource->name());

				if ( loadedIt != m_resources.cend() )
				{
					TraceError{resource_t::ClassId} << "A resource name '" << resource->name() << "' is already present in the store !";

					return false;
				}

				m_resources.emplace(resource->name(), resource);

				return true;
			}

			/**
			 * @brief Preloads asynchronously a resource.
			 * @param resourceName A string with the name of the resource.
			 * @param asyncLoad Load the resource asynchronously. Default true.
			 * @return bool
			 */
			bool
			preloadResource (const std::string & resourceName, bool asyncLoad = true)
			{
				const std::lock_guard< std::mutex > scopeLock{m_resourcesAccess};

				/* NOTE: Do not use Container::isResourceLoaded() to prevent from mutex deadlock. */
				if ( m_resources.contains(resourceName) )
				{
					return true;
				}

				/* If not already loaded, check in store for loading. */
				if ( m_localStore == nullptr )
				{
					return false;
				}

				const auto & resourceIt = m_localStore->find(resourceName);

				if ( resourceIt == m_localStore->cend() )
				{
					return false;
				}

				return this->pushInLoadingQueue(resourceIt->second, asyncLoad) != nullptr;
			}

			/**
			 * @brief Preloads a bunch of resources.
			 * @param resourceNames A reference to a vector of strings.
			 * @return uint32_t
			 */
			uint32_t
			preloadResources (const std::vector< std::string > & resourceNames) noexcept
			{
				uint32_t errors = 0;

				for ( const auto & resourceName : resourceNames )
				{
					if ( !this->preloadResource(resourceName) )
					{
						++errors;
					}
				}

				return errors;
			}

			/**
			 * @brief Returns the default resource.
			 * @note The resource should always exist.
			 * @return std::shared_ptr< resource_t >
			 */
			[[nodiscard]]
			std::shared_ptr< resource_t >
			getDefaultResource () noexcept
			{
				const std::lock_guard< std::mutex > scopeLock{m_resourcesAccess};

				return this->getDefaultResourceUnlocked();
			}

			/**
			 * @brief Returns a resource by its name. If the resource is unloaded, a thread will take care of it unless the "asyncLoad" argument is set to "false".
			 * @note The default resource of the store will be returned if nothing was found. A warning trace will be generated.
			 * @param resourceName A reference to a string for the resource name.
			 * @param asyncLoad Load the resource asynchronously. Default true.
			 * @return std::shared_ptr< resource_t >
			 */
			[[nodiscard]]
			std::shared_ptr< resource_t >
			getResource (const std::string & resourceName, bool asyncLoad = true) noexcept
			{
				const std::lock_guard< std::mutex > scopeLock{m_resourcesAccess};

				if ( resourceName == Default )
				{
					return this->getDefaultResourceUnlocked();
				}

				/* Checks in loaded resources. */
				{
					const auto & loadedIt = m_resources.find(resourceName);

					if ( loadedIt != m_resources.cend() )
					{
						return loadedIt->second;
					}
				}

				/* If not already loaded, check in store for loading. */
				if ( m_localStore == nullptr )
				{
					TraceWarning{resource_t::ClassId} <<
						"The store is empty, unable to get '" << resourceName << "' ! "
						"Use Resource::create() function instead.";

					return this->getDefaultResourceUnlocked();
				}

				const auto & resourceIt = m_localStore->find(resourceName);

				if ( resourceIt == m_localStore->cend() )
				{
					/* The resource is definitively not present. */
					TraceWarning{resource_t::ClassId} <<
						"The resource named '" << resourceName << "' doesn't exist ! "
						"Use Resource::create() function instead.";

					return this->getDefaultResourceUnlocked();
				}

				/* Returns the smart pointer to the future loaded resource. */
				return this->pushInLoadingQueue(resourceIt->second, asyncLoad);
			}

			/**
			 * @brief Returns an existing resource or a new empty one.
			 * @param resourceName A string with the name of the resource.
			 * @param resourceFlags The resource construction flags. Default none.
			 * @param asyncLoad Load the resource asynchronously. Default true.
			 * @return std::shared_ptr< resource_t >
			 */
			[[nodiscard]]
			std::shared_ptr< resource_t >
			getOrNewResource (const std::string & resourceName, uint32_t resourceFlags = 0, bool asyncLoad = true) noexcept
			{
				const std::lock_guard< std::mutex > scopeLock{m_resourcesAccess};

				const auto alreadyLoadedResource = this->checkLoadedResource(resourceName, asyncLoad);

				if ( alreadyLoadedResource != nullptr )
				{
					return alreadyLoadedResource;
				}

				return this->createResourceUnlocked(resourceName, resourceFlags);
			}

			/**
			 * @brief Returns an existing resource or use a method to create a new one.
			 * @param resourceName A string with the name of the resource.
			 * @param createFunction A reference to a function to create the existent resource.
			 * @param resourceFlags The resource construction flags. Default none.
			 * @return std::shared_ptr< resource_t >
			 */
			[[nodiscard]]
			std::shared_ptr< resource_t >
			getOrCreateResource (const std::string & resourceName, const std::function< bool (resource_t & resource) > & createFunction, uint32_t resourceFlags = 0) noexcept
			{
				const std::lock_guard< std::mutex > scopeLock{m_resourcesAccess};

				const auto alreadyLoadedResource = this->checkLoadedResource(resourceName, false);

				if ( alreadyLoadedResource != nullptr )
				{
					return alreadyLoadedResource;
				}

				/* Creates a new resource. */
				const auto newResource = this->createResourceUnlocked(resourceName, resourceFlags);

				/* Already defines that the resource is in manual loading mode. */
				if ( !newResource->enableManualLoading() )
				{
					TraceError{resource_t::ClassId} << "The manual loading for resource '" << resourceName << "' has been already set !";

					return nullptr;
				}

				if ( !createFunction(*newResource) )
				{
					TraceError{resource_t::ClassId} << "The manual loading function for resource '" << resourceName << "' has returned an error !";

					return nullptr;
				}

				switch ( newResource->status() )
				{
					case Status::Unloaded :
					case Status::Enqueuing :
					case Status::ManualEnqueuing :
						TraceError{resource_t::ClassId} <<
							"The manual resource '" << resourceName << "' is still in creation mode!"
							"A manual loading should ends with a call to ResourceTrait::setManualLoadSuccess() or ResourceTrait::load().";

						return nullptr;

					case Status::Failed :
						TraceError{resource_t::ClassId} << "The manual resource '" << resourceName << "' has failed to load!";

						return nullptr;

					default :
						return newResource;
				}
			}

			/**
			 * @brief Returns an existing resource or use a method to create a new one asynchronously.
			 * @param resourceName A string with the name of the resource.
			 * @param createFunction A reference to a function to create the existent resource.
			 * @param resourceFlags The resource construction flags. Default none.
			 * @return std::shared_ptr< resource_t >
			 */
			[[nodiscard]]
			std::shared_ptr< resource_t >
			getOrCreateResourceAsync (const std::string & resourceName, const std::function< bool (resource_t & resource) > & createFunction, uint32_t resourceFlags = 0) noexcept
			{
				const std::lock_guard< std::mutex > scopeLock{m_resourcesAccess};

				const auto alreadyLoadedResource = this->checkLoadedResource(resourceName, true);

				if ( alreadyLoadedResource != nullptr )
				{
					return alreadyLoadedResource;
				}

				/* Creates a new resource. */
				const auto newResource = this->createResourceUnlocked(resourceName, resourceFlags);

				/* Already defines that the resource is in manual loading mode. */
				if ( !newResource->enableManualLoading() )
				{
					return nullptr;
				}

				m_primaryServices.threadPool()->enqueue([createFunction, newResource] {
					if ( createFunction(*newResource) ) {
						switch ( newResource->status() )
						{
							case Status::Unloaded :
							case Status::Enqueuing :
							case Status::ManualEnqueuing :
								TraceError{resource_t::ClassId} <<
									"The manual resource '" << newResource->name() << " is still in creation mode !"
									"A manual loading should ends with a call to ResourceTrait::setManualLoadSuccess() or ResourceTrait::load().";
								break;

							default :
								break;
						}
					}
					else
					{
						TraceError{resource_t::ClassId} << "The manual loading function has return an error !";
					}
				});

				return newResource;
			}

			/**
			 * @brief Returns a random resource from this manager.
			 * @param asyncLoad Load the resource asynchronously. Default true.
			 * @return std::shared_ptr< resource_t >
			 */
			[[nodiscard]]
			std::shared_ptr< resource_t >
			getRandomResource (bool asyncLoad = true) noexcept
			{
				if ( m_localStore == nullptr )
				{
					return nullptr;
				}

				/* NOTE: Will lock the resource mutex. */
				const auto random = Libs::Utility::quickRandom< size_t >(0, m_localStore->size());

				const auto randomResourceName = std::next(std::begin(*m_localStore), static_cast< long >(random))->first;

				return this->getResource(randomResourceName, asyncLoad);
			}

		private:

			/** @copydoc EmEn::Resources::ContainerInterface::initialize() noexcept */
			bool
			initialize () noexcept override
			{
				if ( m_localStore == nullptr )
				{
					return true;
				}

				if ( m_verboseEnabled )
				{
					TraceInfo{resource_t::ClassId} << "The resource type '" << resource_t::ClassId << "' has " << m_localStore->size() <<" entries in the local store available.";
				}

				return true;
			}

			/** @copydoc EmEn::Resources::ContainerInterface::terminate() noexcept */
			bool
			terminate () noexcept override
			{
				if ( m_localStore == nullptr )
				{
					return true;
				}

				if ( m_verboseEnabled )
				{
					TraceInfo{resource_t::ClassId} << "The resource type '" << resource_t::ClassId << "' has " << m_localStore->size() << " entries in the local store to check for unload.";
				}

				{
					const std::lock_guard< std::mutex > scopeLock{m_resourcesAccess};

					m_externalResources.clear();
					m_resources.clear();
				}

				return true;
			}

			/** @copydoc EmEn::Libs::ObserverTrait::onNotification() */
			bool
			onNotification (const ObservableTrait * observable, int notificationCode, const std::any & data) noexcept override
			{
				if ( observable->is(Net::Manager::getClassUID()) )
				{
					if ( notificationCode == Net::Manager::FileDownloaded )
					{
						const std::lock_guard< std::mutex > scopeLock{m_resourcesAccess};

						const auto downloadTicket = std::any_cast< int >(data);

						auto requestIt = m_externalResources.find(downloadTicket);

						if ( requestIt != m_externalResources.end() )
						{
							switch ( m_primaryServices.netManager().downloadStatus(downloadTicket) )
							{
								case Net::DownloadStatus::Pending :
								case Net::DownloadStatus::Transferring :
								case Net::DownloadStatus::OnHold :
									return true;

								case Net::DownloadStatus::Done :
								{
									Tracer::success(resource_t::ClassId, "Resource downloaded.");

									requestIt->second.setDownloadProcessed(m_primaryServices.fileSystem(), true);

									/* Enqueue the resource loading in the thread pool. */
									auto request = requestIt->second;

									m_primaryServices.threadPool()->enqueue([this, request] {
										this->loadingTask(request);
									});
								}
									break;

								case Net::DownloadStatus::Error :
									Tracer::error(resource_t::ClassId, "Resource failed to download.");

									requestIt->second.setDownloadProcessed(m_primaryServices.fileSystem(), true);
									break;
							}

							/* Removes the loading request. */
							m_externalResources.erase(requestIt);
						}
					}

					return true;
				}

				/* We don't know who is sending this message,
				 * so we stop listening. */
				Tracer::warning(resource_t::ClassId, "Unknown notification, stop listening to this sender.");

				return false;
			}

			/**
			 * @brief Creates a new resource.
			 * @note This version does not lock the mutex.
			 * @note When creating a new resource, put '+' in front of the resource name to prevent it to be overridden from a store resource.
			 * @param resourceName A string with the name of the resource.
			 * @param resourceFlags The resource construction flags.
			 * @return std::shared_ptr< resource_t >
			 */
			[[nodiscard]]
			std::shared_ptr< resource_t >
			createResourceUnlocked (const std::string & resourceName, uint32_t resourceFlags) noexcept
			{
				if ( resourceName == Default )
				{
					TraceError{resource_t::ClassId} << Default << "' as resource name is a reserved key !";

					return nullptr;
				}

				/* First, check in store if the resource exists. */
				if ( m_localStore != nullptr && m_localStore->contains(resourceName) )
				{
					TraceWarning{resource_t::ClassId} <<
						resource_t::ClassId << " resource named '" << resourceName << "' already exists in local store ! "
						"Use get() function instead.";

					return nullptr;
				}

				/* Checks in loaded resources. */
				{
					auto loadedIt = m_resources.find(resourceName);

					if ( loadedIt != m_resources.cend() )
					{
						TraceWarning{resource_t::ClassId} <<
							resource_t::ClassId << " resource named '" << resourceName << "' already exists in loaded resources ! "
							"Use getResource() function instead.";

						return loadedIt->second;
					}
				}

				auto result = m_resources.emplace(resourceName, std::make_shared< resource_t >(resourceName, resourceFlags));

				if ( !result.second )
				{
					TraceFatal{resource_t::ClassId} <<
						"Unable to get " << resource_t::ClassId << " resource named '" << resourceName << "' into the map. "
						"This should never happens !";

					return nullptr;
				}

				return result.first->second;
			}

			/**
			 * @brief Returns the default resource.
			 * @note This version does not lock the mutex.
			 * @return std::shared_ptr< resource_t >
			 */
			[[nodiscard]]
			std::shared_ptr< resource_t >
			getDefaultResourceUnlocked () noexcept
			{
				/* Checks in loaded resources. */
				{
					const auto & loadedIt = m_resources.find(Default);

					if ( loadedIt != m_resources.cend() )
					{
						return loadedIt->second;
					}
				}

				/* Creates and load the resource. */
				auto defaultResource = std::make_shared< resource_t >(Default);

				if ( !defaultResource->load(m_serviceProvider) )
				{
					TraceFatal{resource_t::ClassId} << "The default resource '" << resource_t::ClassId << "' can't be loaded !";

					return nullptr;
				}

				/* Save the resource in a smart pointer and return it. */
				auto result = m_resources.emplace(
					std::piecewise_construct,
					std::forward_as_tuple(Default),
					std::forward_as_tuple(defaultResource)
				);

				if ( !result.second )
				{
					return nullptr;
				}

				return defaultResource;
			}

			/**
			 * @brief Checks for a previously loaded resource and return it.
			 * @note Calling methods must lock the resource access.
			 * @param resourceName A reference to a string.
			 * @param asyncLoad Load the resource asynchronously.
			 * @return std::shared_ptr< resource_t >
			 */
			[[nodiscard]]
			std::shared_ptr< resource_t >
			checkLoadedResource (const std::string & resourceName, bool asyncLoad) noexcept
			{
				if ( resourceName == Default )
				{
					return this->getDefaultResourceUnlocked();
				}

				/* Checks in loaded resources. */
				const auto & loadedIt = m_resources.find(resourceName);

				if ( loadedIt != m_resources.cend() )
				{
					return loadedIt->second;
				}

				/* If not already loaded, check in store for loading. */
				if ( m_localStore != nullptr )
				{
					if ( const auto resourceIt = m_localStore->find(resourceName); resourceIt != m_localStore->cend() )
					{
						return this->pushInLoadingQueue(resourceIt->second, !asyncLoad);
					}
				}

				return nullptr;
			}

			/**
			 * @brief Adds a resource to the loading queue.
			 * @note Calling methods must lock the resource access.
			 * @param baseInformation A reference to the base information of the resource to be loaded.
			 * @param asyncLoad Load the resource asynchronously.
			 * @return std::shared_ptr< resource_t >
			 */
			[[nodiscard]]
			std::shared_ptr< resource_t >
			pushInLoadingQueue (const BaseInformation & baseInformation, bool asyncLoad) noexcept
			{
				const auto & name = baseInformation.name();

				/* 1. Check if a resource is not already in the loading queue ... */
				const auto & resourceIt = m_resources.find(name);

				if ( resourceIt != m_resources.cend() )
				{
					if ( m_verboseEnabled )
					{
						TraceInfo{resource_t::ClassId} << "Resource (" << resource_t::ClassId << ") '" << name << "' is already in the loading queue.";
					}

					return resourceIt->second;
				}

				/* Creates a new resource in the loading queue. */
				auto result = m_resources.emplace(
					std::piecewise_construct,
					std::forward_as_tuple(name),
					std::forward_as_tuple(new resource_t(name, 0))
				);

				if ( !result.second )
				{
					TraceError{resource_t::ClassId} << "Unable to create the resource (" << resource_t::ClassId << ") '" << name << "' !";

					return nullptr;
				}

				/* Gets a reference to the smart pointer of the new resource. */
				auto & newResource = result.first->second;

				LoadingRequest< resource_t > request{baseInformation, newResource};

				if ( asyncLoad )
				{
					/* NOTE: Check if we need to download the resource first. */
					if ( baseInformation.sourceType() == SourceType::ExternalData )
					{
						if ( !request.isDownloadable() )
						{
							return nullptr;
						}

						/* NOTE: Check the cache system before downloading. */
						const auto cacheFile = request.cacheFilepath(m_primaryServices.fileSystem());

						if ( Libs::IO::fileExists(cacheFile) )
						{
							request.setDownloadProcessed(m_primaryServices.fileSystem(), true);
						}
						else
						{
							const auto ticket = m_primaryServices.netManager().download(request.url(), cacheFile, false);

							request.setDownloadTicket(ticket);

							m_externalResources.emplace(ticket, request);
						}
					}
					else
					{
						/* Enqueue the resource loading into the thread pool. */
						m_primaryServices.threadPool()->enqueue([this, request] {
							this->loadingTask(request);
						});
					}
				}
				else
				{
					newResource->setDirectLoadingHint();

					/* Call directly the loading function on the manager thread. */
					this->loadingTask(request);
				}

				return newResource;
			}

			/**
			 * @brief Task for loading a resource on a thread.
			 * @note Value must pass the request parameter.
			 * @param request The loading request.
			 * @return void
			 */
			void
			loadingTask (LoadingRequest< resource_t > request) noexcept
			{
				/* Notify the beginning of a loading process. */
				this->notify(LoadingProcessStarted);

				const auto & infos = request.baseInformation();

				auto success = false;

				/* Load the local resource and send a notification when finished. */
				switch ( infos.sourceType() )
				{
					/* This is a local file, so we load it by using a filepath. */
					case SourceType::LocalData :
						if ( m_verboseEnabled )
						{
							TraceInfo{resource_t::ClassId} << "Loading the resource (" << resource_t::ClassId << ") '" << infos.name() << "'... [CONTAINER]";
						}

						success = request.resource()->load(m_serviceProvider, std::filesystem::path{infos.data().asString()});
						break;

					/* This is direct data with a JsonCPP way of representing the data. */
					case SourceType::DirectData :
						if ( m_verboseEnabled )
						{
							TraceInfo{resource_t::ClassId} << "Loading the resource (" << resource_t::ClassId << ") '" << infos.name() << "'... [CONTAINER]";
						}

						success = request.resource()->load(m_serviceProvider, infos.data());
						break;

					/* This should never happen! ExternalData must be processed before. */
					case SourceType::ExternalData :
						TraceError{resource_t::ClassId} << "The resource (" << resource_t::ClassId << ") '" << infos.name() << "' should be downloaded first. Unable to load it ! [CONTAINER]";
						break;

					/* This should never happen! Undefined is a bug. */
					case SourceType::Undefined :
					default:
						TraceError{resource_t::ClassId} << "The resource (" << resource_t::ClassId << ") '" << infos.name() << "' information are invalid. Unable to load it ! [CONTAINER]";
						break;
				}

				if ( success )
				{
					if ( m_verboseEnabled )
					{
						TraceSuccess{resource_t::ClassId} << "The resource (" << resource_t::ClassId << ") '" << infos.name() << "' is loaded. [CONTAINER]";
					}

					this->notify(ResourceLoaded, request.resource().get());
				}
				else
				{
					TraceError{resource_t::ClassId} << "The resource (" << resource_t::ClassId << ") '" << infos.name() << "' failed to load ! [CONTAINER]";
				}

				/* Notify the end of the loading process. */
				this->notify(LoadingProcessFinished);
			}

			PrimaryServices & m_primaryServices;
			ServiceProvider & m_serviceProvider;
			std::shared_ptr< std::unordered_map< std::string, BaseInformation > > m_localStore;
			std::unordered_map< std::string, std::shared_ptr< resource_t > > m_resources;
			std::unordered_map< int, LoadingRequest< resource_t > > m_externalResources;
			mutable std::mutex m_resourcesAccess;
			bool m_verboseEnabled{false};
	};
}
