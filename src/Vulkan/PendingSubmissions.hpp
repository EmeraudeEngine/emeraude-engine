/*
 * src/Vulkan/PendingSubmissions.hpp
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

#pragma once

/* Project configuration. */
#include "emeraude_export.hpp"

/* STL inclusions. */
#include <cstddef>
#include <cstdint>
#include <mutex>

/* Local inclusions for usages. */
#include "StaticVector.hpp"

namespace EmEn::Vulkan
{
	class Queue;

	/**
	 * @brief A completion point on a queue timeline: every command submitted to the queue up to this value.
	 */
	struct SubmissionPoint final
	{
		const Queue * queue{nullptr};
		uint64_t value{0};
	};

	/**
	 * @brief The submissions still writing a GPU object (a buffer, an image) that its destruction must outlive.
	 * @note An upload returns as soon as it is SUBMITTED: the object may be released while the copy still runs.
	 * The transfer records the completion point of its last submission here (Queue's timeline value, see
	 * SynchInfo::tracksCompletion()), and the object's destruction hands its handles to Device::destroyAfter()
	 * instead of destroying them in place while a point is unreached.
	 * Thread-safe: a transfer and a destruction may run on different threads.
	 */
	class EMEN_API PendingSubmissions final
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"VulkanPendingSubmissions"};

			/** @brief The number of distinct queues tracked at once. */
			static constexpr size_t MaxQueues{4};

			/** @brief The completion points of one object. */
			using Points = Base::StaticVector< SubmissionPoint, MaxQueues >;

			/**
			 * @brief Constructs an empty record.
			 */
			PendingSubmissions () noexcept = default;

			/**
			 * @brief Deleted copy constructor (a GPU object has one owner of its pending writes).
			 * @param copy A reference to the copied instance.
			 */
			PendingSubmissions (const PendingSubmissions & copy) noexcept = delete;

			/**
			 * @brief Move constructor: takes the points of the moved object.
			 * @param other A reference to the moved instance.
			 */
			PendingSubmissions (PendingSubmissions && other) noexcept
				: m_points{other.takeAll()}
			{

			}

			/**
			 * @brief Deleted copy assignment.
			 * @param copy A reference to the copied instance.
			 * @return PendingSubmissions &
			 */
			PendingSubmissions & operator= (const PendingSubmissions & copy) noexcept = delete;

			/**
			 * @brief Move assignment: takes the points of the moved object.
			 * @warning The points this record held are dropped: assign only to a record whose object has no
			 * pending write left (a freshly created or already destroyed object).
			 * @param other A reference to the moved instance.
			 * @return PendingSubmissions &
			 */
			PendingSubmissions &
			operator= (PendingSubmissions && other) noexcept
			{
				if ( this != &other )
				{
					const std::scoped_lock lock{m_mutex, other.m_mutex};

					m_points = other.m_points;
					other.m_points.clear();
				}

				return *this;
			}

			/**
			 * @brief Destructs the record.
			 */
			~PendingSubmissions () = default;

			/**
			 * @brief Records the completion point of a submission that writes the object.
			 * @note One point per queue: a later value on the same queue supersedes the earlier one (a queue
			 * completes in submission order). With MaxQueues queues already pending, the oldest point is
			 * waited for before it is replaced.
			 * @param queue A reference to the queue the submission went to.
			 * @param value The timeline value received from SynchInfo::tracksCompletion(). 0 is ignored.
			 * @return void
			 */
			void record (const Queue & queue, uint64_t value) noexcept;

			/**
			 * @brief Returns the points not reached yet and forgets every point.
			 * @note Called once by the object's destruction.
			 * @return Points
			 */
			[[nodiscard]]
			Points takeUnreached () noexcept;

			/**
			 * @brief Returns whether every point is reached.
			 * @param points A reference to points taken by takeUnreached().
			 * @return bool
			 */
			[[nodiscard]]
			static bool areReached (const Points & points) noexcept;

			/**
			 * @brief Blocks until every point is reached.
			 * @param points A reference to points taken by takeUnreached().
			 * @param timeoutNanoseconds The maximum wait per point.
			 * @return bool False when a wait failed (a timeout, a lost device).
			 */
			[[nodiscard]]
			static bool waitUntilReached (const Points & points, uint64_t timeoutNanoseconds) noexcept;

		private:

			/**
			 * @brief Returns every point and forgets them, under the lock.
			 * @return Points
			 */
			[[nodiscard]]
			Points
			takeAll () noexcept
			{
				const std::scoped_lock lock{m_mutex};

				Points points{m_points};

				m_points.clear();

				return points;
			}

			mutable std::mutex m_mutex;
			Points m_points;
	};
}
