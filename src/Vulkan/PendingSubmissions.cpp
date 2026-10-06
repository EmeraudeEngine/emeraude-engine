/*
 * src/Vulkan/PendingSubmissions.cpp
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

#include "PendingSubmissions.hpp"

/* STL inclusions. */
#include <algorithm>

/* Local inclusions. */
#include "Queue.hpp"
#include "Tracer.hpp"

namespace EmEn::Vulkan
{
	using namespace Base;

	namespace
	{
		/** @brief The wait granted to an evicted point: far above any upload, below a hung device. */
		constexpr uint64_t EvictionTimeoutNanoseconds{10'000'000'000ULL};
	}

	void
	PendingSubmissions::record (const Queue & queue, uint64_t value) noexcept
	{
		if ( value == 0 )
		{
			return;
		}

		const std::scoped_lock lock{m_mutex};

		/* NOTE: A queue completes its submissions in order: the latest value covers the earlier ones. */
		for ( auto & point : m_points )
		{
			if ( point.queue == &queue )
			{
				point.value = std::max(point.value, value);

				return;
			}
		}

		/* Make room: forget the points already reached, then wait for the oldest one if still full. */
		if ( m_points.full() )
		{
			const auto reached = std::ranges::remove_if(m_points, [] (const SubmissionPoint & point) {
				return point.queue->isReached(point.value);
			});

			m_points.erase(reached.begin(), reached.end());
		}

		if ( m_points.full() )
		{
			const auto & oldest = m_points.front();

			if ( !oldest.queue->waitUntilReached(oldest.value, EvictionTimeoutNanoseconds) )
			{
				TraceError{ClassId} << "A pending submission could not be waited for before its record was replaced !";
			}

			m_points.erase(m_points.begin());
		}

		m_points.push_back(SubmissionPoint{.queue = &queue, .value = value});
	}

	PendingSubmissions::Points
	PendingSubmissions::takeUnreached () noexcept
	{
		Points unreached;

		{
			const std::scoped_lock lock{m_mutex};

			for ( const auto & point : m_points )
			{
				if ( !point.queue->isReached(point.value) )
				{
					unreached.push_back(point);
				}
			}

			m_points.clear();
		}

		return unreached;
	}

	bool
	PendingSubmissions::areReached (const Points & points) noexcept
	{
		return std::ranges::all_of(points, [] (const SubmissionPoint & point) {
			return point.queue->isReached(point.value);
		});
	}

	bool
	PendingSubmissions::waitUntilReached (const Points & points, uint64_t timeoutNanoseconds) noexcept
	{
		auto success = true;

		for ( const auto & point : points )
		{
			if ( !point.queue->waitUntilReached(point.value, timeoutNanoseconds) )
			{
				success = false;
			}
		}

		return success;
	}
}
