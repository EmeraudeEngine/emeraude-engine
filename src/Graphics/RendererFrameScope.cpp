/*
 * src/Graphics/Renderer.cpp
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

#include "RendererFrameScope.hpp"

/* Project configuration. */
#include "emeraude_config.hpp"

/* Local inclusions. */
#include "Vulkan/Sync/Fence.hpp"
#include "Vulkan/Sync/Semaphore.hpp"
#include "Vulkan/CommandPool.hpp"
#include "Vulkan/CommandBuffer.hpp"

namespace EmEn::Graphics
{
	using namespace Vulkan;

	bool
	RendererFrameScope::initialize (const std::shared_ptr< Device > & device, uint32_t frameIndex) noexcept
	{
		const auto frameName = RendererFrameScope::getFrameName(frameIndex);

		m_frameIndex = frameIndex;

		/* NOTE: We create a rendering command pool, no individual reset for command buffer. */
		m_commandPool = std::make_shared< CommandPool >(device, device->getGraphicsFamilyIndex(), true, false, false);
		m_commandPool->setIdentifier(ClassId, frameName, "CommandPool");

		if ( !m_commandPool->createOnHardware() )
		{
			TraceError{ClassId} << "Unable to create the command pool #" << m_frameIndex << '!';

			return false;
		}

		m_inFlightFence = std::make_unique< Sync::Fence >(device, VK_FENCE_CREATE_SIGNALED_BIT);
#if IS_MACOS
		m_inFlightFence->setIdentifier(ClassId, (std::stringstream{} << "Frame" << frameIndex << "ImageInFlight").str(), "Fence");
#else
		m_inFlightFence->setIdentifier(ClassId, std::format("Frame{}ImageInFlight", frameIndex), "Fence");
#endif

		if ( !m_inFlightFence->createOnHardware() )
		{
			TraceError{ClassId} << "Unable to create a fence #" << frameIndex << " for in-flight!";

			return false;
		}

		m_imageAvailableSemaphore = std::make_unique< Sync::Semaphore >(device);
#if IS_MACOS
		m_imageAvailableSemaphore->setIdentifier(ClassId, (std::stringstream{} << "Frame" << frameIndex << "ImageAvailable").str(), "Semaphore");
#else
		m_imageAvailableSemaphore->setIdentifier(ClassId, std::format("Frame{}ImageAvailable", frameIndex), "Semaphore");
#endif

		if ( !m_imageAvailableSemaphore->createOnHardware() )
		{
			TraceError{ClassId} << "Unable to create a semaphore #" << frameIndex << " for image available!";

			return false;
		}

		m_renderFinishedSemaphore = std::make_unique< Sync::Semaphore >(device);
#if IS_MACOS
		m_renderFinishedSemaphore->setIdentifier(ClassId, (std::stringstream{} << "Frame" << frameIndex << "RenderFinished").str(), "Semaphore");
#else
		m_renderFinishedSemaphore->setIdentifier(ClassId, std::format("Frame{}RenderFinished", frameIndex), "Semaphore");
#endif

		if ( !m_renderFinishedSemaphore->createOnHardware() )
		{
			TraceError{ClassId} << "Unable to create a semaphore #" << frameIndex << " for image finished!";

			return false;
		}

		return true;
	}

	std::shared_ptr< CommandBuffer >
	RendererFrameScope::getCommandBuffer (const RenderTarget::Abstract * renderTarget) noexcept
	{
		if ( const auto commandBufferIt = m_commandBuffers.find(renderTarget); commandBufferIt != m_commandBuffers.cend() )
		{
			return commandBufferIt->second;
		}

		auto commandBuffer = std::make_shared< CommandBuffer >(m_commandPool, true);
		commandBuffer->setIdentifier(ClassId, renderTarget->id(), "CommandBuffer");

		if ( !commandBuffer->isCreated() )
		{
			TraceError{ClassId} << "Unable to create a command buffer for render target '" << renderTarget->id() << "' !";

			return {};
		}

		m_commandBuffers.emplace(renderTarget, commandBuffer);

		return commandBuffer;
	}

	void
	RendererFrameScope::declareSemaphore (const std::shared_ptr< Sync::Semaphore > & semaphore, bool primary) noexcept
	{
		const auto handle = semaphore->handle();

		if ( primary )
		{
			m_primarySemaphores.emplace_back(handle);
		}
		else
		{
			m_secondarySemaphores.emplace_back(handle);
		}
	}

	void
	RendererFrameScope::promotePrimaryToSecondary () noexcept
	{
		for ( const auto semaphore : m_primarySemaphores )
		{
			m_secondarySemaphores.emplace_back(semaphore);
		}

		m_primarySemaphores.clear();
	}

	bool
	RendererFrameScope::prepareForNewFrame () noexcept
	{
		m_primarySemaphores.clear();
		m_secondarySemaphores.clear();

		return m_commandPool->resetCommandBuffers(false);
	}
}
