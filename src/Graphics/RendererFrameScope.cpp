#include "RendererFrameScope.hpp"

/* Local inclusions. */
#include "Vulkan/Sync/Semaphore.hpp"
#include "Vulkan/CommandPool.hpp"
#include "Vulkan/CommandBuffer.hpp"
#include "Tracer.hpp"

namespace EmEn::Graphics
{
	using namespace EmEn::Vulkan;

	std::string
	RendererFrameScope::getFrameName (uint32_t frameIndex) noexcept
	{
		std::stringstream frameName;

		frameName << "Frame" << frameIndex;

		return frameName.str();
	}

	bool
	RendererFrameScope::initialize (const std::shared_ptr< Device > & device, uint32_t frameIndex) noexcept
	{
		const auto frameName = RendererFrameScope::getFrameName(frameIndex);
		m_frameIndex = frameIndex;

		m_commandPool = std::make_shared< CommandPool >(device, device->getGraphicsFamilyIndex(), false, true, false);
		m_commandPool->setIdentifier(ClassId, frameName, "CommandPool");

		if ( !m_commandPool->createOnHardware() )
		{
			TraceError{ClassId} << "Unable to create the command pool #" << m_frameIndex << " !";

			return false;
		}

		m_commandBuffer = std::make_shared< CommandBuffer >(m_commandPool);
		m_commandBuffer->setIdentifier(ClassId, frameName, "CommandBuffer");

		if ( !m_commandBuffer->isCreated() )
		{
			TraceError{ClassId} << "Unable to create the command buffer #" << m_frameIndex << " !";

			return false;
		}

		return true;
	}

	void
	RendererFrameScope::declareSemaphore (const std::shared_ptr< Sync::Semaphore > & semaphore, bool primary) noexcept
	{
		const auto handle = semaphore->handle();

		if ( primary )
		{
			m_primarySemaphores.emplace_back(handle);
		}

		m_secondarySemaphores.emplace_back(handle);
	}
}
