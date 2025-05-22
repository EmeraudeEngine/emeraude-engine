/*
 * src/Vulkan/DeviceRequirements.cpp
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

#include "DeviceRequirements.hpp"

/* STL inclusions. */
#include <algorithm>
#include <iterator>

/* Local inclusions. */
#include "Libs/Math/Base.hpp"
#include "Tracer.hpp"

namespace EmEn::Vulkan
{
	using namespace EmEn::Libs;

	void
	DeviceRequirements::requireGraphicsQueues (const std::vector< float > & queues, const std::vector< float > & transferQueues) noexcept
	{
		if ( queues.empty() )
		{
			Tracer::error(ClassId, "The queue vector is empty !");

			return;
		}

		std::ranges::transform(queues, std::back_inserter(m_graphicsQueues), [] (const float priority) {
			return Math::clampToUnit< float >(priority);
		});

		/* NOTE: Transfer on graphics will use a simple graphics queue. */
		if ( transferQueues.empty() )
		{
			return;
		}

		std::ranges::transform(transferQueues, std::back_inserter(m_graphicsTransferQueues), [] (const float priority) {
			return Math::clampToUnit< float >(priority);
		});
	}

	void
	DeviceRequirements::requirePresentationQueues (const std::vector< float > & queues, VkSurfaceKHR surface, bool separate) noexcept
	{
		if ( queues.empty() )
		{
			Tracer::error(ClassId, "The queue vector is empty !");

			return;
		}

		if ( surface == VK_NULL_HANDLE )
		{
			Tracer::error(ClassId, "The surface handle must be valid to check the queue validity !");

			return;
		}

		std::ranges::transform(queues, std::back_inserter(m_presentationQueues), [] (float priority) {
			return Math::clampToUnit< float >(priority);
		});

		m_surface = surface;
		m_presentationSeparated = separate;
	}

	void
	DeviceRequirements::requireComputeQueues (const std::vector< float > & queues, const std::vector< float > & transferQueues) noexcept
	{
		if ( queues.empty() )
		{
			Tracer::error(ClassId, "The queue vector is empty !");

			return;
		}

		std::ranges::transform(queues, std::back_inserter(m_computeQueues), [] (const float priority) {
			return Math::clampToUnit< float >(priority);
		});

		/* NOTE: Transfer on graphics will use a simple graphics queue. */
		if ( transferQueues.empty() )
		{
			return;
		}

		std::ranges::transform(transferQueues, std::back_inserter(m_computeTransferQueues), [] (const float priority) {
			return Math::clampToUnit< float >(priority);
		});
	}

	void
	DeviceRequirements::requireTransferQueues (const std::vector< float > & queues) noexcept
	{
		if ( queues.empty() )
		{
			Tracer::error(ClassId, "The queue vector is empty !");

			return;
		}

		std::ranges::transform(queues, std::back_inserter(m_transferQueues), [] (const float priority) {
			return Math::clampToUnit< float >(priority);
		});
	}

	size_t
	DeviceRequirements::getRequiredQueueCount () const noexcept
	{
		size_t count = 0;

		count += m_graphicsQueues.size();
		count += m_graphicsTransferQueues.size();
		count += m_presentationQueues.size();
		count += m_computeQueues.size();
		count += m_computeTransferQueues.size();
		count += m_transferQueues.size();

		return count;
	}

	std::ostream &
	operator<< (std::ostream & out, const DeviceRequirements & obj)
	{
		auto printInfo = [&out] (const char * title, const std::vector< float > & queues) {
			out << title << " : " << queues.size();

			if ( !queues.empty() )
			{
				out << " priorities : ";

				for ( const auto & priority : queues )
				{
					out << priority << ' ';
				}
			}

			out << '\n';
		};

		printInfo("Graphics queue requested", obj.m_graphicsQueues);

		printInfo("Graphics transfer queue requested", obj.m_graphicsTransferQueues);

		printInfo("Presentation queue requested", obj.m_presentationQueues);

		printInfo("Compute queue requested", obj.m_computeQueues);

		printInfo("Compute transfer queue requested", obj.m_computeTransferQueues);

		printInfo("Transfer queue requested", obj.m_transferQueues);

		return out;
	}
}
