/*
 * src/Resources/ResourceTrait.cpp
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

#include "Resources/ResourceTrait.hpp"

/* Project configuration. */
#include "emeraude_platform.hpp"

/* STL inclusions. */
#include <algorithm>
#include <chrono>
#include <compare>
#include <memory>
#include <optional>
#include <ranges>
#include <unordered_set>
#include <utility>
#include <vector>

/* Local inclusions. */
#include "Any.hpp"
#include "BlobTrait.hpp"
#include "FastJSON.hpp"
#include "IO/IO.hpp"
#include "PrimaryServices.hpp"
#include "Resources/BaseInformation.hpp"
#include "Resources/Types.hpp"
#include "String.hpp"
#include "ThreadPool.hpp"
#include "Tracer.hpp"

namespace EmEn::Resources
{
	using namespace Base;

	namespace
	{
		/**
		 * @brief Returns the dependency-graph lock: it serialises every edge insertion with its cycle check
		 * (addDependency()). Lock order: this one first, then node locks (m_dependenciesAccess).
		 * @note A function-local static: constructed on first use, whatever the static initialisation order.
		 * @return Base::Mutex &
		 */
		[[nodiscard]]
		Base::Mutex &
		dependencyGraphAccess () noexcept
		{
			static Base::Mutex graphAccess;

			return graphAccess;
		}

		/**
		 * @brief Returns whether the current thread may not reload a released copy by blocking (the render thread).
		 * @return bool &
		 */
		[[nodiscard]]
		bool &
		blockingLocalDataReloadForbidden () noexcept
		{
			thread_local bool forbidden{false};

			return forbidden;
		}
	}

	constexpr auto TracerTag{"ResourceChain"};

	ResourceTrait::~ResourceTrait ()
	{
		/* NOTE: Check the resource status.
		 * It should be Loaded or Failed. */
		switch ( m_status )
		{
			case Status::Unloaded :
				if ( s_showInformation )
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
				break;
		}
	}

	bool
	ResourceTrait::initializeEnqueuing (bool manual) noexcept
	{
		if ( s_showInformation )
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
				TraceError{TracerTag} << "The resource '" << this->name() << "' (" << this->classLabel() << ") has previously tried to be loaded, but failed !";

				return false;
		}

		/* An out-of-range status. */
		return false;
	}

	bool
	ResourceTrait::addDependency (const std::shared_ptr< ResourceTrait > & dependency) noexcept
	{
		/* NOTE: Check null pointer before acquiring any locks. */
		if ( dependency == nullptr )
		{
			Tracer::error(TracerTag, "The dependency pointer is null !");

			{
				const Base::MutexLock lock{m_dependenciesAccess};

				m_status = Status::Failed;
			}

			/* NOTE: The dependencies already added and the parents are released (no link may cycle once failed). */
			this->releaseLinksAfterFailure();

			return false;
		}

		/* NOTE: A resource depending on itself is a cycle. Refused before any lock: the two-node locking below took the
		 * same mutex twice (std::scoped_lock{m, m}: undefined behaviour, 2026-10-16). */
		if ( dependency.get() == this ) [[unlikely]]
		{
			TraceError{TracerTag} << "The resource '" << this->name() << "' (" << this->classLabel() << ") cannot depend on itself !";

			{
				const Base::MutexLock lock{m_dependenciesAccess};

				m_status = Status::Failed;
			}

			this->releaseLinksAfterFailure();

			return false;
		}

		/* NOTE: A refusal that FAILS this resource releases its links once the locks below are released
		 * (releaseLinksAfterFailure() takes this resource's lock and calls the parents). */
		bool failed = false;
		bool dependencyAlreadyFailed = false;

		const auto added = [&] () noexcept -> bool {
			/* NOTE: The graph lock FIRST (lock order: graph → node): the cycle check below and the insertion are atomic
			 * against every other addDependency(). The check copies each node's list under that node's lock alone. */
			const Base::MutexLock graphLock{dependencyGraphAccess()};

			const auto createsCycle = this->wouldCreateCycle(dependency);

			/* NOTE: This resource's lock, then the dependency's: both lists change. The graph lock above serializes every
			 * two-node locking (no other path holds one node's lock while taking another's), so the order cannot invert.
			 * The two are distinct: a self-dependency was refused before. */
			const Base::MutexLock lock{m_dependenciesAccess};
			const Base::MutexLock dependencyLock{dependency->m_dependenciesAccess};

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

				/* NOTE: Past the enqueuing stage a dependency is refused (owner decision 2026-10-12): it was added anyway
				 * before, and a resource already loading or loaded never waits for it (onDependenciesLoaded() does not
				 * run again). */
				case Status::Loading :
					TraceError{TracerTag} <<
						"The resource '" << this->name() << "' (" << this->classLabel() << ") is loading ! "
						"No more dependency can be added : '" << dependency->name() << "' refused.";

					return false;

				case Status::Loaded :
					TraceError{TracerTag} <<
						"The resource '" << this->name() << "' (" << this->classLabel() << ") is loaded ! "
						"No more dependency can be added : '" << dependency->name() << "' refused.";

					return false;

				case Status::Failed :
					TraceError{TracerTag} <<
						"The resource '" << this->name() << "' (" << this->classLabel() << ") is failed !"
						"This resource should be removed.";

					return false;
			}

			/* NOTE: A dependency that ALREADY failed (an asynchronous creation that ended before this call) would be
			 * waited for forever: it is handled as a failure notification, once the locks are released. */
			if ( dependency->m_status == Status::Failed )
			{
				dependencyAlreadyFailed = true;

				return false;
			}

			/* NOTE: If the dependency is already loaded, we skip it... */
			if ( dependency->isLoaded() )
			{
				if ( s_showInformation )
				{
					TraceInfo{TracerTag} << "Resource dependency '" << dependency->name() << "' (" << dependency->classLabel() << ") is already loaded.";
				}

				return true;
			}

			/* NOTE: If the dependency is already present, we also skip it... */
			if ( std::ranges::find(std::as_const(m_dependenciesToWaitFor), dependency) != m_dependenciesToWaitFor.cend() )
			{
				if ( s_showInformation )
				{
					TraceInfo{TracerTag} << "Resource dependency '" << dependency->name() << "' (" << dependency->classLabel() << ") is already in the queue.";
				}

				return true;
			}

			/* NOTE: Check for circular dependency.
			 * We walk up the dependency's parent chain to see if 'this' resource appears.
			 * If it does, adding this dependency would create a cycle and cause a deadlock. */
			if ( createsCycle ) [[unlikely]]
			{
				TraceError{TracerTag} <<
					"Circular dependency detected ! Adding '" << dependency->name() << "' (" << dependency->classLabel() << ") "
					"as a dependency of '" << this->name() << "' (" << this->classLabel() << ") would create a cycle !";

				m_status = Status::Failed;

				failed = true;

				return false;
			}

			/* NOTE: Adds the dependency to wait for being loaded ... */
			m_dependenciesToWaitFor.push_back(dependency);

			/* ... then set this resource as the parent of the dependency (double-link). */
			dependency->m_parentsToNotify.push_back(this->shared_from_this());

			if ( s_showInformation )
			{
				TraceInfo{TracerTag} <<
					"Resource dependency '" << dependency->name() << "' (" << dependency->classLabel() << ") "
					"added to resource '" << this->name() << "' (" << this->classLabel() << "). "
					"Dependency count : " << m_dependenciesToWaitFor.size() << ".";
			}

			return true;
		}();

		if ( failed )
		{
			this->releaseLinksAfterFailure();
		}

		if ( dependencyAlreadyFailed )
		{
			return this->goOnAfterDependencyFailure(*dependency);
		}

		return added;
	}

	void
	ResourceTrait::dependencyLoaded (const std::shared_ptr< ResourceTrait > & dependency) noexcept
	{
		if ( s_showInformation )
		{
			TraceInfo{TracerTag} <<
				"The dependency '" << dependency->name() << "' (" << dependency->classLabel() << ") "
				"is loaded from resource '" << this->name() << "' (" << this->classLabel() << ") !";
		}

		/* NOTE: A resource that failed meanwhile (another dependency failed) has released its links: nothing to do. */
		if ( m_status == Status::Failed )
		{
			return;
		}

		{
			const Base::MutexLock lock{m_dependenciesAccess};

			/* NOTE: Removes the loaded resource from dependencies. */
			std::erase(m_dependenciesToWaitFor, dependency);
		}

		/* Launch an overall check for dependency loading. */
		this->checkDependencies();
	}

	void
	ResourceTrait::dependencyFailed (const std::shared_ptr< ResourceTrait > & dependency) noexcept
	{
		/* NOTE: Already failed (another dependency first): its links are released already. */
		if ( m_status == Status::Failed )
		{
			return;
		}

		{
			const Base::MutexLock lock{m_dependenciesAccess};

			std::erase(m_dependenciesToWaitFor, dependency);
		}

		if ( this->goOnAfterDependencyFailure(*dependency) )
		{
			this->checkDependencies();
		}
	}

	bool
	ResourceTrait::goOnAfterDependencyFailure (const ResourceTrait & dependency) noexcept
	{
		/* NOTE: A dependency the application's stop cancelled is no failure to report (owner decision 2026-10-15): the
		 * container's stop summarises the cancelled loads in one line. */
		const bool cancelled = dependency.loadAbandoned();

		if ( this->onDependencyFailed(dependency) )
		{
			if ( !cancelled )
			{
				TraceWarning{TracerTag} << "The resource '" << this->name() << "' (" << this->classLabel() << ") goes on without its failed dependency '" << dependency.name() << "' (" << dependency.classLabel() << ").";
			}

			return true;
		}

		if ( cancelled )
		{
			/* NOTE: Its own parents stay silent too. */
			m_loadAbandoned.store(true, std::memory_order_release);
		}
		else
		{
			TraceError{TracerTag} << "The resource '" << this->name() << "' (" << this->classLabel() << ") fails: its dependency '" << dependency.name() << "' (" << dependency.classLabel() << ") failed.";
		}

		m_status = Status::Failed;

		this->notify(LoadFailed, this->name());

		this->releaseLinksAfterFailure();

		return false;
	}

	void
	ResourceTrait::releaseLinksAfterFailure () noexcept
	{
		std::vector< std::shared_ptr< ResourceTrait > > parents;

		{
			const Base::MutexLock lock{m_dependenciesAccess};

			parents = std::exchange(m_parentsToNotify, {});

			/* NOTE: The children still loading keep a pointer to this resource until they end (their parent lists):
			 * this one no longer holds them, so nothing cycles. */
			m_dependenciesToWaitFor.clear();
		}

		if ( parents.empty() )
		{
			return;
		}

		/* NOTE: A resource with parents is owned by a shared_ptr (addDependency() took shared_from_this()). */
		const auto self = this->shared_from_this();

		for ( const auto & parent : parents )
		{
			parent->dependencyFailed(self);
		}
	}

	void
	ResourceTrait::checkDependencies () noexcept
	{
		/* NOTE: We need to track what actions to take outside the lock to avoid
		 * calling virtual methods (onDependenciesLoaded) and observer notifications
		 * while holding the mutex, which could cause deadlocks. */
		enum class Action : uint8_t
		{
			None,
			CallOnDependenciesLoaded
		};

		auto pendingAction = Action::None;

		{
			const Base::MutexLock lock{m_dependenciesAccess};

			/* NOTE: First, we check the current resource status. */
			switch ( m_status )
			{
				/* For these statuses, there is no need to check dependencies now. */
				case Status::Unloaded :
				case Status::Enqueuing :
				case Status::ManualEnqueuing :
					if ( s_showInformation )
					{
						TraceInfo{TracerTag} << "The resource '" << this->name() << "' (" << this->classLabel() << ") still enqueuing dependencies !";
					}
					break;

				/* This is the state where we want to know if dependencies are loaded. */
				case Status::Loading :
				{
					/* NOTE: Another thread already runs onDependenciesLoaded() for this resource. The last dependency
					 * completing (its thread) and the resource's own setLoadSuccess() (another thread) both reach this
					 * point with every dependency loaded: without this claim, both ran onDependenciesLoaded() at once —
					 * two Texture2D::createFromPixelData() on one texture, the second destroying the image the first was
					 * creating (vkBindImageMemory on VK_NULL_HANDLE, 2026-10-07). */
					if ( m_dependenciesFinalizing )
					{
						return;
					}

					/* NOTE: If any of the dependencies are in a loading state. */
					if ( std::ranges::any_of(m_dependenciesToWaitFor, [] (const auto & dependency) {return !dependency->isLoaded();}) )
					{
						return;
					}

					if ( s_showInformation )
					{
						TraceInfo{TracerTag} << "The resource '" << this->name() << "' (" << this->classLabel() << ") has no more dependency to wait for loading !";
					}

					/* NOTE: Mark that we need to call onDependenciesLoaded() outside the lock, and claim it. */
					m_dependenciesFinalizing = true;
					pendingAction = Action::CallOnDependenciesLoaded;
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
					TraceError{TracerTag} <<
						"The resource '" << this->name() << "' (" << this->classLabel() << ") status is failed ! "
						"This resource should be removed !";
					break;
			}
		}

		/* NOTE: Execute pending actions outside the lock to prevent deadlocks.
		 * Virtual method calls and observer notifications happen here. */
		if ( pendingAction == Action::CallOnDependenciesLoaded )
		{
			const bool success = this->onDependenciesLoaded();

			/* NOTE: We need to re-acquire the lock to update status and get parents list. */
			std::vector< std::shared_ptr< ResourceTrait > > parentsToNotifyCopy;

			{
				const Base::MutexLock lock{m_dependenciesAccess};

				m_dependenciesFinalizing = false;

				if ( success )
				{
					m_status = Status::Loaded;

					if ( s_showInformation )
					{
						TraceSuccess{TracerTag} << "Resource '" << this->name() << "' (" << this->classLabel() << ") is successfully loaded !";
					}

					/* NOTE: The list itself (the lock is held here; isTopResource() takes it). */
					if ( !m_parentsToNotify.empty() )
					{
						/* Copy parents list to notify outside lock. */
						parentsToNotifyCopy = std::exchange(m_parentsToNotify, {});
					}
				}
				else
				{
					m_status = Status::Failed;

					if ( s_showInformation )
					{
						TraceError{TracerTag} << "Resource '" << this->name() << "' (" << this->classLabel() << ") failed to load !";
					}
				}
			}

			/* NOTE: Notify observers and parents outside the lock. */
			if ( success )
			{
				this->notify(LoadFinished, this->name());

				for ( const auto & parent : parentsToNotifyCopy )
				{
					parent->dependencyLoaded(this->shared_from_this());
				}
			}
			else
			{
				this->notify(LoadFailed, this->name());

				this->releaseLinksAfterFailure();
			}
		}
	}

	bool
	ResourceTrait::setLoadSuccess (bool status) noexcept
	{
		if ( s_showInformation )
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

		/* NOTE: The last moment to add a dependency: before the loading stage. */
		if ( status && !this->onLoadFinishing() )
		{
			TraceError{TracerTag} << "Resource '" << this->name() << "' (" << this->classLabel() << ") could not finish its declarations !";

			status = false;
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

			this->releaseLinksAfterFailure();
		}

		return status;
	}

	bool
	ResourceTrait::abandonLoading () noexcept
	{
		if ( !this->beginLoading() )
		{
			return false;
		}

		/* NOTE: setLoadSuccess(false) without its error line (the load never ran: the application stopped). Set first:
		 * the parents told below read it. */
		m_loadAbandoned.store(true, std::memory_order_release);
		m_status = Status::Failed;

		this->notify(LoadFailed, this->name());

		this->releaseLinksAfterFailure();

		return true;
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
	ResourceTrait::load (const std::filesystem::path & filepath) noexcept
	{
		const auto root = FastJSON::getRootFromFile(filepath);

		if ( !root )
		{
			TraceError{TracerTag} << "Unable to parse the resource file " << filepath << " !" "\n";

			/* NOTE: Set status here. */
			m_status = Status::Failed;

			this->notify(LoadFailed, this->name());

			this->releaseLinksAfterFailure();

			return false;
		}

		/* NOTE: A resource file holds a JSON object; jsoncpp's member access, in every load (const Json::Value &),
		 * aborts on anything else. */
		if ( !root->isObject() )
		{
			TraceError{TracerTag} << "The resource file " << filepath << " does not hold a JSON object !";

			m_status = Status::Failed;

			this->notify(LoadFailed, this->name());

			this->releaseLinksAfterFailure();

			return false;
		}

		/* Checks if additional stores before loading (optional) */
		this->serviceProvider().update(*root);

		return this->load(*root);
	}

	std::string
	ResourceTrait::getResourceNameFromFilepath (const std::filesystem::path & filepath, const std::string & storeName) noexcept
	{
		const auto filename = String::right(IO::toU8String(filepath), storeName + IO::Separator);

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

	bool
	ResourceTrait::wouldCreateCycle (const std::shared_ptr< ResourceTrait > & dependency) const noexcept
	{
		/* NOTE: Iterative walk with a visited set (a diamond of shared dependencies is visited once, not once per
		 * path), shared_ptr copies so that a node released meanwhile stays alive while it is examined. */
		std::vector< std::shared_ptr< ResourceTrait > > pending{dependency};
		std::unordered_set< const ResourceTrait * > visited;

		while ( !pending.empty() )
		{
			const auto node = std::move(pending.back());
			pending.pop_back();

			if ( node.get() == this )
			{
				return true;
			}

			if ( !visited.insert(node.get()).second )
			{
				continue;
			}

			const Base::MutexLock nodeLock{node->m_dependenciesAccess};

			pending.insert(pending.end(), node->m_dependenciesToWaitFor.cbegin(), node->m_dependenciesToWaitFor.cend());
		}

		return false;
	}

	ResourceTrait::LocalDataLease
	ResourceTrait::leaseLocalData () const noexcept
	{
		auto self = this->weak_from_this().lock();

		if ( self == nullptr )
		{
			TraceError{TracerTag} << "The resource '" << this->name() << "' is not owned by a shared pointer: no lease on its local data !";

			return {};
		}

		{
			const Base::MutexLock scopeLock{m_localDataAccess};

			if ( m_localDataReleased )
			{
				return {};
			}

			++m_localDataLeases;
		}

		return LocalDataLease{std::move(self)};
	}

	void
	ResourceTrait::retainLocalData () noexcept
	{
		const Base::MutexLock scopeLock{m_localDataAccess};

		m_localDataRetained = true;
	}

	bool
	ResourceTrait::localDataRetained () const noexcept
	{
		const Base::MutexLock scopeLock{m_localDataAccess};

		return m_localDataRetained;
	}

	bool
	ResourceTrait::isLocalDataResident () const noexcept
	{
		const Base::MutexLock scopeLock{m_localDataAccess};

		return !m_localDataReleased;
	}

	void
	ResourceTrait::markLocalDataReleasable () const noexcept
	{
		if ( !this->releasesLocalData() )
		{
			return;
		}

		const Base::MutexLock scopeLock{m_localDataAccess};

		if ( !m_localDataReleasable )
		{
			m_localDataReleasable = true;
			m_localDataReleasableSince = std::chrono::steady_clock::now();
		}
	}

	bool
	ResourceTrait::releaseLocalDataIfIdle (std::chrono::steady_clock::time_point now, std::chrono::steady_clock::duration graceDelay) noexcept
	{
		if ( !this->releasesLocalData() || !this->isLoaded() )
		{
			return false;
		}

		const Base::MutexLock scopeLock{m_localDataAccess};

		if ( !m_localDataReleasable || m_localDataRetained || m_localDataReleased || m_localDataLeases > 0 || now - m_localDataReleasableSince < graceDelay )
		{
			return false;
		}

		this->onReleaseLocalData();

		m_localDataReleased = true;

		return true;
	}

	void
	ResourceTrait::raiseLocalDataLeases () const noexcept
	{
		const Base::MutexLock scopeLock{m_localDataAccess};

		++m_localDataLeases;
	}

	void
	ResourceTrait::lowerLocalDataLeases () const noexcept
	{
		const Base::MutexLock scopeLock{m_localDataAccess};

		if ( m_localDataLeases > 0 )
		{
			--m_localDataLeases;
		}
	}

	ResourceTrait::LocalDataLease
	ResourceTrait::acquireLocalData () noexcept
	{
		auto self = this->weak_from_this().lock();

		if ( self == nullptr )
		{
			TraceError{TracerTag} << "The resource '" << this->name() << "' is not owned by a shared pointer: no lease on its local data !";

			return {};
		}

		{
			Base::MutexLock lock{m_localDataAccess};

			/* NOTE: Another caller is reloading the copy: wait for its outcome. */
			m_localDataReloaded.wait(lock, [this] () EMEN_REQUIRES(m_localDataAccess) {
				return !m_localDataReloading;
			});

			if ( !m_localDataReleased )
			{
				++m_localDataLeases;

				return LocalDataLease{std::move(self)};
			}

			if ( blockingLocalDataReloadForbidden() )
			{
				TraceError{TracerTag} << "The local data of '" << this->name() << "' is released and this thread may not reload it by blocking (requestLocalData() instead) !";

				return {};
			}

			m_localDataReloading = true;
		}

		return this->completeLocalDataReload();
	}

	ResourceTrait::LocalDataLease
	ResourceTrait::requestLocalData () noexcept
	{
		auto self = this->weak_from_this().lock();

		if ( self == nullptr )
		{
			TraceError{TracerTag} << "The resource '" << this->name() << "' is not owned by a shared pointer: no lease on its local data !";

			return {};
		}

		{
			const Base::MutexLock scopeLock{m_localDataAccess};

			if ( m_localDataReloading )
			{
				return {};
			}

			if ( !m_localDataReleased )
			{
				++m_localDataLeases;

				return LocalDataLease{std::move(self)};
			}

			m_localDataReloading = true;
		}

		/* NOTE: The job holds the resource (`self`): it calls the derived class's reload, which a handle held by this base
		 * class could not keep alive through the derived destructor. The PROVIDER owns the job (submitResourceJob()):
		 * it stops and waits for it before the renderer goes (a GPU readback). A job that never runs (stopped before it
		 * started, or refused) still ends the reload in LocalDataReloadJob's destructor: acquireLocalData() waits for it. */
		if ( !m_serviceProvider.submitResourceJob([reload = std::make_shared< LocalDataReloadJob >(std::move(self))] (const std::stop_token &) {
			reload->run();
		}) )
		{
			TraceWarning{TracerTag} << "The local data of '" << this->name() << "' cannot be reloaded in the background (the resource jobs are stopped) !";
		}

		return {};
	}

	ResourceTrait::LocalDataReloadJob::~LocalDataReloadJob ()
	{
		if ( m_resource == nullptr || m_ran )
		{
			return;
		}

		{
			const Base::MutexLock scopeLock{m_resource->m_localDataAccess};

			m_resource->m_localDataReloading = false;
		}

		m_resource->m_localDataReloaded.notifyAll();
	}

	void
	ResourceTrait::LocalDataReloadJob::run () noexcept
	{
		if ( m_resource == nullptr || m_ran )
		{
			return;
		}

		m_ran = true;

		static_cast< void >(m_resource->completeLocalDataReload());
	}

	bool
	AbstractServiceProvider::submitResourceJob (std::function< void (const std::stop_token &) > job) noexcept
	{
		const auto threadPool = m_primaryServices.threadPool();

		if ( threadPool == nullptr )
		{
			return false;
		}

		return threadPool->submit(m_resourceJobs, std::move(job));
	}

	void
	AbstractServiceProvider::stopResourceJobs () noexcept
	{
		/* NOTE: waitForAll(): a discarded reload ends its reload in its destructor (LocalDataReloadJob); it is gone too
		 * when this returns. Main thread (never a pool worker: aborted). */
		m_resourceJobs.requestStop();
		m_resourceJobs.waitForAll();
	}

	void
	ResourceTrait::setLocalDataSource (const BaseInformation & source) noexcept
	{
		const Base::MutexLock scopeLock{m_localDataAccess};

		m_localDataSource = source;
	}

	bool
	ResourceTrait::hasLocalDataSource () const noexcept
	{
		const Base::MutexLock scopeLock{m_localDataAccess};

		return m_localDataSource.has_value();
	}

	void
	ResourceTrait::forbidBlockingLocalDataReload () noexcept
	{
		blockingLocalDataReloadForbidden() = true;
	}

	ResourceTrait::LocalDataLease
	ResourceTrait::completeLocalDataReload () noexcept
	{
		/* NOTE: The copy is released and m_localDataReloading is set: no lease exists, no release can run, no other
		 * reload starts — the data is this call's alone, without holding the lock through the I/O. */
		auto self = this->weak_from_this().lock();
		std::optional< BaseInformation > source;

		{
			const Base::MutexLock scopeLock{m_localDataAccess};

			source = m_localDataSource;
		}

		const char * origin = "store source";
		auto reloaded = source.has_value() && this->reloadLocalDataFromSource(*source);

		if ( !reloaded )
		{
			origin = "own source";
			reloaded = this->reloadLocalDataFromOwnSource();
		}

		if ( !reloaded )
		{
			origin = "GPU copy";
			reloaded = this->reloadLocalDataFromGPU();
		}

		LocalDataLease lease;

		{
			const Base::MutexLock scopeLock{m_localDataAccess};

			m_localDataReloading = false;

			if ( reloaded && self != nullptr )
			{
				m_localDataReleased = false;
				/* NOTE: Back to releasable: released again once its new readers are done. */
				m_localDataReleasable = true;
				m_localDataReleasableSince = std::chrono::steady_clock::now();

				++m_localDataLeases;

				lease = LocalDataLease{std::move(self)};
			}
		}

		m_localDataReloaded.notifyAll();

		if ( reloaded )
		{
			TraceInfo{TracerTag} << "The local data of '" << this->name() << "' was reloaded from its " << origin << '.';
		}
		else
		{
			TraceError{TracerTag} << "The local data of '" << this->name() << "' could not be reloaded (no usable store source, own source or GPU readback) !";
		}

		return lease;
	}
}
