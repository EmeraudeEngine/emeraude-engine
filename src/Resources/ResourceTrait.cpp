/*
 * src/Resources/ResourceTrait.cpp
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

#include "Resources/ResourceTrait.hpp"

/* STL inclusions. */
#include <algorithm>

/* Local inclusions. */
#include "Libs/FastJSON.hpp"
#include "Libs/String.hpp"
#include "Manager.hpp"
#include "Tracer.hpp"

namespace EmEn::Resources
{
	using namespace Libs;

	constexpr auto TracerTag{"ResourceChain"};

	ResourceTrait::~ResourceTrait ()
	{
		/* NOTE: Check the resource status.
		 * It should be Loaded or Failed. */
		switch ( m_status )
		{
			case Status::Unloaded :
				if ( s_verboseEnabled )
				{
					TraceInfo{TracerTag} << "The resource '" << this->name() << "' (" << this << ") is destroyed with status 'Unloaded' !";
				}
				break;

			case Status::Enqueuing :
				TraceWarning{TracerTag} << "The resource '" << this->name() << "' (" << this << ") is destroyed while still enqueueing dependencies (Automatic mode) !";
				break;

			case Status::ManualEnqueuing :
				TraceWarning{TracerTag} << "The resource '" << this->name() << "' (" << this << ") is destroyed while still enqueueing dependencies (Manual mode) !";
				break;

			case Status::Loading :
				TraceError{TracerTag} << "The resource '" << this->name() << "' (" << this << ") is destroyed while still loading !";
				break;

			case Status::Loaded :
				/* NOTE: Check the parent list. It should be empty! */
				if ( !m_parentsToNotify.empty() )
				{
					TraceError{TracerTag} << "The resource '" << this->name() << "' (" << this << ") is destroyed while still having " << m_parentsToNotify.size() << " parent pointer(s) !";
				}

				/* NOTE: Check the dependency list. It should be empty! */
				if ( !m_dependenciesToWaitFor.empty() )
				{
					TraceError{TracerTag} << "The resource '" << this->name() << "' (" << this << ") is destroyed while still having " << m_dependenciesToWaitFor.size() << " dependency pointer(s) !";
				}
				break;

			case Status::Failed :
			default:
				break;
		}
	}

	bool
	ResourceTrait::initializeEnqueuing (bool manual) noexcept
	{
		if ( s_verboseEnabled )
		{
			TraceInfo{TracerTag} << "Beginning the creation of resource '" << this->name() << "' (" << this->classLabel() << ") ...";
		}

		switch ( m_status )
		{
			case Status::Unloaded :
				m_status = manual ? Status::ManualEnqueuing : Status::Enqueuing;
				[[fallthrough]];
			case Status::Enqueuing :
			case Status::ManualEnqueuing :
				return true;

			case Status::Loading :
				TraceError{TracerTag} << "The resource '" << this->name() << "' (" << this->classLabel() << ") is already loading !";

				return false;

			case Status::Loaded :
				TraceWarning{TracerTag} << "The resource '" << this->name() << "' (" << this->classLabel() << ") is already loaded !";

				return false;

			case Status::Failed :
			default:
				TraceError{TracerTag} << "The resource '" << this->name() << "' (" << this->classLabel() << ") has previously tried to be loaded, but failed !";

				return false;
		}
	}

	bool
	ResourceTrait::addDependency (const std::shared_ptr< ResourceTrait > & dependency) noexcept
	{
		const std::lock_guard< std::mutex > lock{m_dependenciesAccess};

		/* First, we check the current resource status. */
		switch ( m_status )
		{
			case Status::Unloaded :
				TraceError{TracerTag} <<
					"The resource '" << this->name() << "' (" << this->classLabel() << ") is not in loading stage ! "
					"You should call ResourceTrait::beginLoading() first.";

				return false;

			case Status::Enqueuing :
			case Status::ManualEnqueuing :
				/* The status is in the right condition to add dependency. */
				break;

			case Status::Loading :
				TraceError{TracerTag} <<
					"The resource '" << this->name() << "' (" << this->classLabel() << ") is loading !"
					"No more dependency can be added !";
				break;

			case Status::Loaded :
				TraceWarning{TracerTag} <<
					"The resource '" << this->name() << "' (" << this->classLabel() << ") is loaded !"
					"No more dependency can be added !";
				break;

			case Status::Failed :
			default:
				TraceError{TracerTag} <<
					"The resource '" << this->name() << "' (" << this->classLabel() << ") is failed !"
					"This resource should be removed.";

				return false;
		}

		if ( dependency == nullptr )
		{
			Tracer::error(TracerTag, "The dependency pointer is null !");

			m_status = Status::Failed;

			return false;
		}

		/* NOTE: If the dependency is already loaded, we skip it... */
		if ( dependency->isLoaded() )
		{
			if ( s_verboseEnabled )
			{
				TraceInfo{TracerTag} << "Resource dependency '" << dependency->name() << "' (" << dependency->classLabel() << ") is already loaded.";
			}

			return true;
		}

		/* NOTE: If the dependency is already present, we also skip it... */
		if ( std::ranges::find(std::as_const(m_dependenciesToWaitFor), dependency) != m_dependenciesToWaitFor.cend() )
		{
			if ( s_verboseEnabled )
			{
				TraceInfo{TracerTag} << "Resource dependency '" << dependency->name() << "' (" << dependency->classLabel() << ") is already in the queue.";
			}

			return true;
		}

		/* NOTE: Adds the dependency to wait for being loaded ... */
		m_dependenciesToWaitFor.push_back(dependency);

		/* ... then set this resource as the parent of the dependency (double-link). */
		dependency->m_parentsToNotify.push_back(this->shared_from_this());

		if ( s_verboseEnabled )
		{
			TraceInfo{TracerTag} <<
				"Resource dependency '" << dependency->name() << "' (" << dependency->classLabel() << ") "
				"added to resource '" << this->name() << "' (" << this->classLabel() << "). "
				"Dependency count : " << m_dependenciesToWaitFor.size() << ".";
		}

		return true;
	}

	void
	ResourceTrait::dependencyLoaded (const std::shared_ptr< ResourceTrait > & dependency) noexcept
	{
		if ( s_verboseEnabled )
		{
			TraceInfo{TracerTag} <<
				"The dependency '" << dependency->name() << "' (" << dependency->classLabel() << ") "
				"is loaded from resource '" << this->name() << "' (" << this->classLabel() << ") !";
		}

		/* NOTE: Removes the loaded resource from dependencies. */
		std::erase(m_dependenciesToWaitFor, dependency);

		/* Launch an overall check for dependency loading. */
		this->checkDependencies();
	}

	void
	ResourceTrait::checkDependencies () noexcept
	{
		const std::lock_guard< std::mutex > lock{m_dependenciesAccess};

		/* NOTE: First, we check the current resource status. */
		switch ( m_status )
		{
			/* For these statuses, there is no need to check dependencies now. */
			case Status::Unloaded :
			case Status::Enqueuing :
			case Status::ManualEnqueuing :
				if ( s_verboseEnabled )
				{
					TraceInfo{TracerTag} << "The resource '" << this->name() << "' (" << this->classLabel() << ") still enqueuing dependencies !";
				}
				break;

			/* This is the state where we want to know if dependencies are loaded. */
			case Status::Loading :
			{
				/* NOTE: If any of the dependencies are in a loading state. */
				if ( std::ranges::any_of(m_dependenciesToWaitFor, [] (const auto & dependency) {return !dependency->isLoaded();}) )
				{
					return;
				}

				if ( s_verboseEnabled )
				{
					TraceInfo{TracerTag} << "The resource '" << this->name() << "' (" << this->classLabel() << ") has no more dependency to wait for loading !";
				}

				if ( this->onDependenciesLoaded() )
				{
					m_status = Status::Loaded;

					this->notify(LoadFinished, this->name());

					if ( s_verboseEnabled )
					{
						TraceSuccess{TracerTag} << "Resource '" << this->name() << "' (" << this->classLabel() << ") is successfully loaded !";
					}

					if ( !this->isTopResource() )
					{
						/* We want to notice parents the resource is loaded. */
						for ( const auto & parent : m_parentsToNotify )
						{
							parent->dependencyLoaded(this->shared_from_this());
						}

						/* Once notified, we don't need to keep tracks of parents. */
						m_parentsToNotify.clear();
					}
				}
				else
				{
					m_status = Status::Failed;

					this->notify(LoadFailed, this->name());

					if ( s_verboseEnabled )
					{
						TraceError{TracerTag} << "Resource '" << this->name() << "' (" << this->classLabel() << ") failed to load !";
					}
				}
			}
				break;

			case Status::Loaded :
				if ( !m_dependenciesToWaitFor.empty() )
				{
					TraceError{TracerTag} << "The resource '" << this->name() << "' (" << this->classLabel() << ") status is loaded, but still have " << m_dependenciesToWaitFor.size() << " dependencies.";
				}

				/* NOTE: We don't want to check again dependencies. */
				break;

			case Status::Failed :
			default:
				TraceError{TracerTag} <<
					"The resource '" << this->name() << "' (" << this->classLabel() << ") status is failed ! "
					"This resource should be removed !";
				break;
		}
	}

	bool
	ResourceTrait::setLoadSuccess (bool status) noexcept
	{
		if ( s_verboseEnabled )
		{
			TraceInfo{TracerTag} << "Ending the creation of resource '" << this->name() << "' (" << this->classLabel() << ") ...";
		}

		/* NOTE: If status is not Enqueuing, ManualEnqueuing or Loading,
		 * the resource is in an incoherent status! */
		switch ( m_status )
		{
			case Status::Unloaded :
				TraceError{TracerTag} <<
					"The resource '" << this->name() << "' (" << this->classLabel() << ") is not in a building stage ! "
					"You must call call ResourceTrait::beginLoading() before.";

				return false;

			case Status::Loaded :
				TraceError{TracerTag} << "The resource '" << this->name() << "' (" << this->classLabel() << ") is already loaded !";

				return false;

			case Status::Failed :
				TraceError{TracerTag} << "The resource '" << this->name() << "' (" << this->classLabel() << ") has previously failed to load !";

				return false;

			default:
				break;
		}

		if ( status )
		{
			/* Set the resource in the loading stage.
			 * NOTE: No more sub-resource enqueuing is possible after this point. */
			m_status = Status::Loading;

			/* We want to check every dependency status.
			 * NOTE: This will eventually fire up the 'LoadFinished' event. */
			this->checkDependencies();
		}
		else
		{
			m_status = Status::Failed;

			this->notify(LoadFailed, this->name());

			TraceError{TracerTag} << "Resource '" << this->name() << "' (" << this << ") failed to load ...";
		}

		return status;
	}

	bool
	ResourceTrait::setManualLoadSuccess (bool status) noexcept
	{
		/* Avoid it to call this method on an automatic loading resource. */
		if ( m_status != Status::ManualEnqueuing )
		{
			TraceError{TracerTag} << "Resource '" << this->name() << "' (" << this << ") is not in manual mode !";

			return false;
		}

		return this->setLoadSuccess(status);
	}

	bool
	ResourceTrait::load (ServiceProvider & serviceProvider, const std::filesystem::path & filepath) noexcept
	{
		const auto root = FastJSON::getRootFromFile(filepath);

		if ( !root )
		{
			TraceError{TracerTag} << "Unable to parse the resource file " << filepath << " !" "\n";

			/* NOTE: Set status here. */
			m_status = Status::Failed;

			this->notify(LoadFailed, this->name());

			return false;
		}

		/* Checks if additional stores before loading (optional) */
		serviceProvider.update(root.value());

		return this->load(serviceProvider, root.value());
	}

	std::string
	ResourceTrait::getResourceNameFromFilepath (const std::filesystem::path & filepath, const std::string & storeName) noexcept
	{
		const auto filename = String::right(filepath.string(), storeName + IO::Separator);

		if constexpr ( IsWindows )
		{
			/* NOTE: Resource name uses the UNIX convention. */
			return String::replace(IO::Separator, '/', String::removeFileExtension(filename));
		}
		else
		{
			return String::removeFileExtension(filename);
		}
	}
}
