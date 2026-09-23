/*
 * src/Graphics/RenderTarget/ImposterBake.cpp
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

#include "ImposterBake.hpp"

/* STL inclusions. */
#include <array>

/* Local inclusions. */
#include "Graphics/ImposterAtlas.hpp"
#include "Graphics/Renderer.hpp"
#include "Tracer.hpp"
#include "Vulkan/Framebuffer.hpp"
#include "Vulkan/Image.hpp"
#include "Vulkan/ImageView.hpp"
#include "Vulkan/Instance.hpp"
#include "Vulkan/RenderPass.hpp"

namespace EmEn::Graphics::RenderTarget
{
	using namespace Base::Math;

	namespace
	{
		/** @brief The colour attachment and the material properties: required by the fixed MRT order, never read. */
		constexpr VkFormat DiscardedColorFormat{VK_FORMAT_R8G8B8A8_UNORM};

		/** @brief The near plane of the bake camera, in metres. */
		constexpr float BakeNearPlane{0.01F};
	}

	ImposterBake::ImposterBake (const std::string & name, uint32_t size) noexcept
		: Abstract{
			name,
			FramebufferPrecisions{8, 8, 8, 8, 32, 0, 1},
			{size, size, 1U},
			1.0F,
			RenderTargetType::Texture,
			Scenes::AVConsole::ConnexionType::Both,
			true,
			true
		}
	{
		/* The bake renders when a job asks for it, and every render is a job's. */
		this->setAutomaticRenderingState(true);

		/* ⚠️ Transparent black: the atlas reads its coverage from the albedo's alpha, and the renderer's own clear is
		 * OPAQUE — without this every empty texel of the atlas is a covered black one. */
		this->setClearColorOverride(VkClearColorValue{.float32 = {0.0F, 0.0F, 0.0F, 0.0F}});
	}

	ImposterBake::~ImposterBake () = default;

	void
	ImposterBake::configureCamera (const CartesianFrame< float > & frame, float boxSide) noexcept
	{
		const auto size = static_cast< float >(this->extent().width);

		m_viewMatrices.updateOrthographicViewProperties(size, size, BakeNearPlane, boxSide);

		this->updateDeviceFromCoordinates(frame, {});
	}

	bool
	ImposterBake::enqueue (const void * subject, const std::shared_ptr< ImposterAtlas > & atlas) noexcept
	{
		if ( subject == nullptr || atlas == nullptr || !atlas->isCreated() || atlas->size() != this->extent().width )
		{
			TraceError{ClassId} << "The imposter bake '" << this->id() << "' refuses a job: no subject, or an atlas that is not created or not " << this->extent().width << " px !";

			return false;
		}

		const std::lock_guard< std::mutex > lock{m_jobsAccess};

		m_jobs.emplace_back(Job{subject, atlas, WarmUpRenders});

		/* The front job's subject is the target's bake subject; the render thread moves it to the next job. The
		 * flag is released AFTER the subject is set: the render thread acquires it before reading the subject. */
		if ( m_jobs.size() == 1 )
		{
			this->setBakeSubject(subject);
		}

		m_hasJobs.store(true, std::memory_order_release);

		return true;
	}

	std::vector< std::shared_ptr< ImposterAtlas > >
	ImposterBake::bakedAtlases () const noexcept
	{
		const std::lock_guard< std::mutex > lock{m_jobsAccess};

		std::vector< std::shared_ptr< ImposterAtlas > > atlases;
		atlases.reserve(m_bakedAtlases.size());

		for ( const auto & weakAtlas : m_bakedAtlases )
		{
			if ( auto atlas = weakAtlas.lock(); atlas != nullptr )
			{
				atlases.emplace_back(std::move(atlas));
			}
		}

		return atlases;
	}

	size_t
	ImposterBake::pendingJobs () const noexcept
	{
		const std::lock_guard< std::mutex > lock{m_jobsAccess};

		return m_jobs.size();
	}

	void
	ImposterBake::setViewDistance (float meters) noexcept
	{
		const auto size = static_cast< float >(this->extent().width);

		m_viewMatrices.updateOrthographicViewProperties(size, size, BakeNearPlane, meters);
	}

	float
	ImposterBake::viewDistance () const noexcept
	{
		return m_viewMatrices.farPlane();
	}

	void
	ImposterBake::updateViewRangesProperties (float fovOrNear, float distanceOrFar) noexcept
	{
		const auto size = static_cast< float >(this->extent().width);

		m_viewMatrices.updateOrthographicViewProperties(size, size, fovOrNear, distanceOrFar);
	}

	const Vulkan::Framebuffer *
	ImposterBake::framebuffer () const noexcept
	{
		return m_framebuffer.get();
	}

	bool
	ImposterBake::isReadyForRendering () const noexcept
	{
		return m_isCreated && m_hasJobs.load(std::memory_order_acquire);
	}

	bool
	ImposterBake::writeCombinedImageSampler (const Vulkan::DescriptorSet & /*descriptorSet*/, uint32_t /*bindingIndex*/) const noexcept
	{
		TraceError{ClassId} << "The imposter bake '" << this->id() << "' is not a texture: sample its atlases !";

		return false;
	}

	bool
	ImposterBake::capture (Vulkan::TransferManager & /*transferManager*/, uint32_t /*layerIndex*/, bool /*keepAlpha*/, bool /*withDepthBuffer*/, bool /*withStencilBuffer*/, std::array< Base::PixelFactory::Pixmap< uint8_t >, 3 > & /*result*/) const noexcept
	{
		TraceError{ClassId} << "The imposter bake '" << this->id() << "' cannot be captured: write its atlas (ImposterAtlas::writeAlbedo()) !";

		return false;
	}

	void
	ImposterBake::recordPostRenderCompute (const Vulkan::CommandBuffer & commandBuffer) noexcept
	{
		const std::lock_guard< std::mutex > lock{m_jobsAccess};

		if ( m_jobs.empty() )
		{
			return;
		}

		auto & job = m_jobs.front();

		/* The subject was not drawn (not loaded yet): the render was empty, keep the job. */
		if ( !this->lastRenderHasContent() )
		{
			return;
		}

		/* The first renders of a subject may show it where its entity was created rather than where it was
		 * published: the logic thread publishes the scene once per tick, the render thread renders every frame. */
		if ( job.warmUpRenders > 0 )
		{
			--job.warmUpRenders;

			return;
		}

		job.atlas->recordBake(commandBuffer, *m_albedoImage, *m_normalsImage);

		m_bakedAtlases.emplace_back(job.atlas);

		TraceInfo{ClassId} << "Imposter atlas '" << job.atlas->name() << "' baked.";

		m_jobs.pop_front();

		if ( m_jobs.empty() )
		{
			this->setBakeSubject(nullptr);

			m_hasJobs.store(false, std::memory_order_release);
		}
		else
		{
			this->setBakeSubject(m_jobs.front().subject);
		}
	}

	void
	ImposterBake::updateVideoDeviceProperties (float fovOrNear, float distanceOrFar, bool /*isOrthographicProjection*/) noexcept
	{
		this->updateViewRangesProperties(fovOrNear, distanceOrFar);
	}

	void
	ImposterBake::updateDeviceFromCoordinates (const CartesianFrame< float > & worldCoordinates, const Vector< 3, float > & worldVelocity) noexcept
	{
		m_worldCoordinates = worldCoordinates;
		m_viewMatrices.updateViewCoordinates(worldCoordinates, worldVelocity);
	}

	bool
	ImposterBake::createAttachment (Renderer & renderer, VkFormat format, VkImageUsageFlags usage, const char * label, std::shared_ptr< Vulkan::Image > & image, std::shared_ptr< Vulkan::ImageView > & imageView) const noexcept
	{
		image = std::make_shared< Vulkan::Image >(
			renderer.device(),
			VK_IMAGE_TYPE_2D,
			format,
			this->extent(),
			VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | usage
		);
		image->setIdentifier(ClassId, this->id(), label);

		if ( !image->createOnHardware() )
		{
			TraceError{ClassId} << "Unable to create the " << label << " of the imposter bake '" << this->id() << "' !";

			return false;
		}

		imageView = std::make_shared< Vulkan::ImageView >(
			image,
			VK_IMAGE_VIEW_TYPE_2D,
			VkImageSubresourceRange{
				.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
				.baseMipLevel = 0,
				.levelCount = 1,
				.baseArrayLayer = 0,
				.layerCount = 1
			}
		);
		imageView->setIdentifier(ClassId, this->id(), label);

		if ( !imageView->createOnHardware() )
		{
			TraceError{ClassId} << "Unable to create the " << label << " view of the imposter bake '" << this->id() << "' !";

			return false;
		}

		return true;
	}

	bool
	ImposterBake::onCreate (Renderer & renderer) noexcept
	{
		if ( !this->createAttachment(renderer, DiscardedColorFormat, 0, "ColorImage", m_colorImage, m_colorImageView) ||
			 !this->createAttachment(renderer, ImposterAtlas::NormalFormat, VK_IMAGE_USAGE_TRANSFER_SRC_BIT, "NormalsImage", m_normalsImage, m_normalsImageView) ||
			 !this->createAttachment(renderer, DiscardedColorFormat, 0, "MaterialPropertiesImage", m_materialPropertiesImage, m_materialPropertiesImageView) ||
			 !this->createAttachment(renderer, ImposterAtlas::AlbedoFormat, VK_IMAGE_USAGE_TRANSFER_SRC_BIT, "AlbedoImage", m_albedoImage, m_albedoImageView) )
		{
			return false;
		}

		if ( !this->createDepthBuffer(renderer.device(), m_depthImage, m_depthImageView, this->id()) )
		{
			TraceError{ClassId} << "Unable to create the depth buffer of the imposter bake '" << this->id() << "' !";

			return false;
		}

		const auto renderPass = this->createRenderPass(renderer);

		if ( renderPass == nullptr )
		{
			return false;
		}

		m_framebuffer = std::make_shared< Vulkan::Framebuffer >(renderPass, this->extent());
		m_framebuffer->setIdentifier(ClassId, this->id(), "Framebuffer");

		/* The fixed MRT order of the scene pass: colour, normals, material properties, albedo, then depth. */
		m_framebuffer->addAttachment(m_colorImageView->handle());
		m_framebuffer->addAttachment(m_normalsImageView->handle());
		m_framebuffer->addAttachment(m_materialPropertiesImageView->handle());
		m_framebuffer->addAttachment(m_albedoImageView->handle());
		m_framebuffer->addAttachment(m_depthImageView->handle());

		if ( !m_framebuffer->createOnHardware() )
		{
			TraceError{ClassId} << "Unable to create the framebuffer of the imposter bake '" << this->id() << "' !";

			return false;
		}

		m_isCreated = true;

		return true;
	}

	void
	ImposterBake::onDestroy () noexcept
	{
		m_isCreated = false;

		{
			const std::lock_guard< std::mutex > lock{m_jobsAccess};

			m_jobs.clear();
			m_hasJobs.store(false, std::memory_order_release);
		}

		m_framebuffer.reset();
		m_depthImageView.reset();
		m_depthImage.reset();
		m_albedoImageView.reset();
		m_albedoImage.reset();
		m_materialPropertiesImageView.reset();
		m_materialPropertiesImage.reset();
		m_normalsImageView.reset();
		m_normalsImage.reset();
		m_colorImageView.reset();
		m_colorImage.reset();
	}

	std::shared_ptr< Vulkan::RenderPass >
	ImposterBake::createRenderPass (Renderer & renderer) const noexcept
	{
		auto renderPass = std::make_shared< Vulkan::RenderPass >(renderer.device(), 0);
		renderPass->setIdentifier(ClassId, this->id(), "RenderPass");

		Vulkan::RenderSubPass subPass{VK_PIPELINE_BIND_POINT_GRAPHICS, 0};

		/* The two attachments the atlas is copied from end in TRANSFER_SRC; the other two are never read. */
		const std::array< std::pair< VkFormat, bool >, 4 > colorAttachments{{
			{DiscardedColorFormat, false},
			{ImposterAtlas::NormalFormat, true},
			{DiscardedColorFormat, false},
			{ImposterAtlas::AlbedoFormat, true}
		}};

		uint32_t attachmentIndex = 0;

		for ( const auto & [format, kept] : colorAttachments )
		{
			renderPass->addAttachmentDescription(VkAttachmentDescription{
				.flags = 0,
				.format = format,
				.samples = VK_SAMPLE_COUNT_1_BIT,
				.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
				.storeOp = kept ? VK_ATTACHMENT_STORE_OP_STORE : VK_ATTACHMENT_STORE_OP_DONT_CARE,
				.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
				.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
				.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
				.finalLayout = kept ? VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL : VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
			});

			subPass.addColorAttachment(attachmentIndex, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

			++attachmentIndex;
		}

		renderPass->addAttachmentDescription(VkAttachmentDescription{
			.flags = 0,
			.format = m_depthImage->createInfo().format,
			.samples = VK_SAMPLE_COUNT_1_BIT,
			.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
			.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
			.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
			.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
			.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
			.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
		});

		subPass.setDepthStencilAttachment(attachmentIndex, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

		renderPass->addSubPass(subPass);

		/* The previous bake's copy READ these attachments: the clear must not overwrite them before it is done. */
		renderPass->addSubPassDependency({
			.srcSubpass = VK_SUBPASS_EXTERNAL,
			.dstSubpass = 0,
			.srcStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT,
			.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
			.srcAccessMask = 0,
			.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
			.dependencyFlags = 0
		});

		/* The attachment writes are made visible to the copy that follows in the same command buffer. */
		renderPass->addSubPassDependency({
			.srcSubpass = 0,
			.dstSubpass = VK_SUBPASS_EXTERNAL,
			.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			.dstStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT,
			.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
			.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT,
			.dependencyFlags = 0
		});

		if ( !renderPass->createOnHardware() )
		{
			TraceError{ClassId} << "Unable to create the render pass of the imposter bake '" << this->id() << "' !";

			return nullptr;
		}

		return renderPass;
	}
}
