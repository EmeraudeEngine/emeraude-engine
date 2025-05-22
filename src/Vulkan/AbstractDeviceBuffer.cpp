/*
 * src/Vulkan/AbstractDeviceBuffer.cpp
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

#include "AbstractDeviceBuffer.hpp"

/* Local inclusions. */
#include "TransferManager.hpp"
#include "MemoryRegion.hpp"
#include "StagingBuffer.hpp"
#include "Tracer.hpp"

namespace EmEn::Vulkan
{
	using namespace EmEn::Libs;

	bool
	AbstractDeviceBuffer::writeData (TransferManager & transferManager, const MemoryRegion & memoryRegion) noexcept
	{
		if ( !this->isCreated() )
		{
			Tracer::error(ClassId, "The buffer is not created ! Use one of the Buffer::create() methods first.");

			return false;
		}

		/* Get an available staging buffer to prepare the transfer */
		const auto stagingBuffer = transferManager.getStagingBuffer(memoryRegion.bytes());

		if ( stagingBuffer == nullptr )
		{
			return false;
		}

		/* NOTE: Already locked, but gives the ability to unlock the staging buffer automatically at function exit. */
		const std::lock_guard< StagingBuffer > lock{*stagingBuffer};

		if ( !stagingBuffer->writeData(memoryRegion) )
		{
			TraceError{ClassId} << "Unable to write " << memoryRegion.bytes() << " bytes of data in the staging buffer !";

			return false;
		}

		/* Transfer the buffer data from host memory to device memory. */
		return transferManager.transfer(*stagingBuffer, *this);
	}
}
