/*
 * src/Vulkan/GPUProfiler.cpp
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
 */

#include "GPUProfiler.hpp"

/* STL inclusions. */
#include <algorithm>
#include <array>
#include <cstdio>
#include <limits>

/* Local inclusions. */
#include "Tracer.hpp"
#include "Vulkan/CommandBuffer.hpp"
#include "Vulkan/Device.hpp"
#include "Vulkan/PhysicalDevice.hpp"

namespace EmEn::Vulkan
{
	GPUProfiler::GPUProfiler (const std::shared_ptr< Device > & device, uint32_t frameCount) noexcept
		: AbstractDeviceDependentObject{device}
	{
		m_frames.resize(frameCount);
		m_openScopes.reserve(16);
		m_accumulated.reserve(MaxScopesPerFrame);

		for ( auto & frame : m_frames )
		{
			frame.records.reserve(MaxScopesPerFrame);
		}
	}

	bool
	GPUProfiler::createOnHardware () noexcept
	{
		const auto device = this->device();

		if ( device == nullptr )
		{
			Tracer::error(ClassId, "No device to create the query pools !");

			return false;
		}

		const auto physicalDevice = device->physicalDevice();
		const auto & limits = physicalDevice->propertiesVK10().limits;

		if ( limits.timestampPeriod <= 0.0F )
		{
			Tracer::warning(ClassId, "The device does not support timestamp queries. The GPU profiler stays disabled.");

			return false;
		}

		const auto graphicsFamilyIndex = device->getGraphicsFamilyIndex();
		const auto & queueFamilies = physicalDevice->queueFamilyPropertiesVK11();

		if ( graphicsFamilyIndex >= queueFamilies.size() )
		{
			Tracer::error(ClassId, "The graphics queue family index is out of bounds !");

			return false;
		}

		const auto timestampValidBits = queueFamilies[graphicsFamilyIndex].queueFamilyProperties.timestampValidBits;

		if ( timestampValidBits == 0 )
		{
			Tracer::warning(ClassId, "The graphics queue does not support timestamps. The GPU profiler stays disabled.");

			return false;
		}

		m_timestampPeriodNS = static_cast< double >(limits.timestampPeriod);
		m_timestampMask = timestampValidBits >= 64 ? ~0ULL : (1ULL << timestampValidBits) - 1ULL;

		VkQueryPoolCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
		createInfo.pNext = nullptr;
		createInfo.flags = 0;
		createInfo.queryType = VK_QUERY_TYPE_TIMESTAMP;
		createInfo.queryCount = MaxScopesPerFrame * 2;
		createInfo.pipelineStatistics = 0;

		for ( auto & frame : m_frames )
		{
			if ( vkCreateQueryPool(device->handle(), &createInfo, nullptr, &frame.pool) != VK_SUCCESS )
			{
				Tracer::error(ClassId, "Unable to create a timestamp query pool !");

				this->destroyFromHardware();

				return false;
			}
		}

		m_usable = true;

		TraceSuccess{ClassId} <<
			"GPU profiler ready: " << m_frames.size() << " query pools of " << createInfo.queryCount << " timestamps, "
			"period " << m_timestampPeriodNS << " ns/tick, " << timestampValidBits << " valid bits.";

		return true;
	}

	bool
	GPUProfiler::destroyFromHardware () noexcept
	{
		m_usable = false;

		const auto device = this->device();

		if ( device == nullptr )
		{
			return false;
		}

		for ( auto & frame : m_frames )
		{
			if ( frame.pool != VK_NULL_HANDLE )
			{
				vkDestroyQueryPool(device->handle(), frame.pool, nullptr);

				frame.pool = VK_NULL_HANDLE;
			}

			frame.records.clear();
			frame.queryCount = 0;
			frame.submitted = false;
		}

		return true;
	}

	void
	GPUProfiler::harvest (uint32_t frameSlot) noexcept
	{
		if ( !m_usable || frameSlot >= m_frames.size() )
		{
			return;
		}

		auto & frame = m_frames[frameSlot];

		if ( !frame.submitted || frame.queryCount == 0 )
		{
			return;
		}

		/* The caller just waited this slot's in-flight fence: every query of the previous
		 * submission is available, the read cannot stall (no WAIT flag on purpose). */
		std::array< uint64_t, MaxScopesPerFrame * 2 > results{};

		const auto result = vkGetQueryPoolResults(
			this->device()->handle(),
			frame.pool,
			0,
			frame.queryCount,
			frame.queryCount * sizeof(uint64_t),
			results.data(),
			sizeof(uint64_t),
			VK_QUERY_RESULT_64_BIT
		);

		frame.submitted = false;

		if ( result != VK_SUCCESS )
		{
			/* VK_NOT_READY would mean the fence contract above was broken. */
			TraceError{ClassId} << "vkGetQueryPoolResults() failed (" << result << ") for the frame slot #" << frameSlot << " !";

			return;
		}

		const std::lock_guard< std::mutex > lock{m_accumulatedAccess};

		for ( const auto & record : frame.records )
		{
			const auto begin = results[record.beginQuery];
			const auto end = results[record.endQuery];

			/* The subtraction is masked to the queue's valid bits: a counter wrap inside
			 * the frame yields the correct duration instead of a huge bogus sample. */
			const auto ticks = (end - begin) & m_timestampMask;
			const auto milliseconds = static_cast< float >(static_cast< double >(ticks) * m_timestampPeriodNS * 1e-6);

			this->accumulate(record.label, record.depth, milliseconds);
		}
	}

	void
	GPUProfiler::beginFrame (const CommandBuffer & commandBuffer, uint32_t frameSlot) noexcept
	{
		if ( !m_usable || frameSlot >= m_frames.size() )
		{
			return;
		}

		m_currentSlot = frameSlot;

		auto & frame = m_frames[frameSlot];
		frame.records.clear();
		frame.queryCount = 0;

		m_openScopes.clear();

		/* NOTE: Must be recorded outside a render pass. */
		vkCmdResetQueryPool(commandBuffer.handle(), frame.pool, 0, MaxScopesPerFrame * 2);

		this->beginScope(commandBuffer, "Frame");
	}

	void
	GPUProfiler::endFrame (const CommandBuffer & commandBuffer) noexcept
	{
		if ( !m_usable )
		{
			return;
		}

		auto & frame = m_frames[m_currentSlot];

		/* Close every scope left open (the root "Frame" scope in the nominal case). */
		while ( !m_openScopes.empty() )
		{
			this->endScope(commandBuffer);
		}

		frame.submitted = frame.queryCount > 0;
	}

	void
	GPUProfiler::beginScope (const CommandBuffer & commandBuffer, const char * label, const char * subLabel) noexcept
	{
		if ( !m_usable )
		{
			return;
		}

		auto & frame = m_frames[m_currentSlot];

		if ( frame.records.size() >= MaxScopesPerFrame )
		{
			if ( !m_scopeOverflowTraced )
			{
				TraceWarning{ClassId} << "More than " << MaxScopesPerFrame << " scopes in one frame: the extra scopes are dropped.";

				m_scopeOverflowTraced = true;
			}

			/* Keep the open/close bookkeeping balanced: the matching endScope() must pop. */
			m_openScopes.emplace_back(std::numeric_limits< uint32_t >::max());

			return;
		}

		auto & record = frame.records.emplace_back();

		if ( subLabel != nullptr )
		{
			std::snprintf(record.label, LabelCapacity, "%s/%s", label, subLabel);
		}
		else
		{
			std::snprintf(record.label, LabelCapacity, "%s", label);
		}

		record.beginQuery = frame.queryCount++;
		record.endQuery = frame.queryCount++;
		record.depth = static_cast< uint32_t >(m_openScopes.size());

		m_openScopes.emplace_back(static_cast< uint32_t >(frame.records.size() - 1));

		vkCmdWriteTimestamp(commandBuffer.handle(), VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, frame.pool, record.beginQuery);
	}

	void
	GPUProfiler::endScope (const CommandBuffer & commandBuffer) noexcept
	{
		if ( !m_usable || m_openScopes.empty() )
		{
			return;
		}

		const auto recordIndex = m_openScopes.back();
		m_openScopes.pop_back();

		/* A dropped (overflowed) scope has no query to close. */
		if ( recordIndex == std::numeric_limits< uint32_t >::max() )
		{
			return;
		}

		const auto & record = m_frames[m_currentSlot].records[recordIndex];

		vkCmdWriteTimestamp(commandBuffer.handle(), VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, m_frames[m_currentSlot].pool, record.endQuery);
	}

	std::vector< GPUProfiler::Timing >
	GPUProfiler::snapshot () const noexcept
	{
		std::vector< Timing > timings;

		const std::lock_guard< std::mutex > lock{m_accumulatedAccess};

		timings.reserve(m_accumulated.size());

		for ( const auto & [label, stats] : m_accumulated )
		{
			timings.emplace_back(Timing{
				.label = label,
				.lastMS = stats.lastMS,
				.averageMS = stats.averageMS,
				.maximumMS = stats.maximumMS,
				.sampleCount = stats.sampleCount,
				.depth = stats.depth
			});
		}

		return timings;
	}

	void
	GPUProfiler::resetStatistics () noexcept
	{
		const std::lock_guard< std::mutex > lock{m_accumulatedAccess};

		m_accumulated.clear();
	}

	void
	GPUProfiler::accumulate (const char * label, uint32_t depth, float milliseconds) noexcept
	{
		/* Linear scan: the accumulator holds at most MaxScopesPerFrame entries and the
		 * first-seen order IS the display order (matches the command stream). */
		const auto entry = std::ranges::find_if(m_accumulated, [label] (const auto & item) {
			return item.first == label;
		});

		if ( entry == m_accumulated.end() )
		{
			m_accumulated.emplace_back(label, Accumulated{
				.lastMS = milliseconds,
				.averageMS = milliseconds,
				.maximumMS = milliseconds,
				.sampleCount = 1,
				.depth = depth
			});

			return;
		}

		auto & stats = entry->second;
		stats.lastMS = milliseconds;
		stats.averageMS += (milliseconds - stats.averageMS) * AverageAlpha;
		stats.maximumMS = std::max(stats.maximumMS, milliseconds);
		stats.sampleCount++;
		stats.depth = depth;
	}
}