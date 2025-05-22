/*
 * src/Overlay/Surface.cpp
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

#include "Surface.hpp"

/* Local inclusions. */
#include "Vulkan/Image.hpp"
#include "Vulkan/ImageView.hpp"
#include "Vulkan/Sampler.hpp"
#include "Vulkan/MemoryRegion.hpp"
#include "Graphics/Renderer.hpp"
#include "Manager.hpp"
#include "Tracer.hpp"

namespace EmEn::Overlay
{
	using namespace EmEn::Libs;
	using namespace EmEn::Libs::Math;
	using namespace EmEn::Libs::PixelFactory;
	using namespace EmEn::Graphics;
	using namespace EmEn::Vulkan;

	Surface::Surface (const FramebufferProperties & framebufferProperties, const std::string & name, const Space2D::AARectangle< float > & geometry, float depth, bool visible) noexcept
		: NameableTrait(name),
		m_framebufferProperties(framebufferProperties),
		m_rectangle(geometry),
		m_depth(depth)
	{
		m_flags[IsVisible] = visible;

		this->updateModelMatrix();
	}

	void
	Surface::setGeometry (const Space2D::AARectangle< float > & rectangle) noexcept
	{
		m_rectangle = rectangle;

		/* NOTE: The texture must be resized. */
		this->invalidate();
	}

	void
	Surface::setPosition (float xPosition, float yPosition) noexcept
	{
		m_rectangle.setLeft(xPosition);
		m_rectangle.setTop(yPosition);

		this->updateModelMatrix();
	}

	void
	Surface::setSize (float width, float height) noexcept
	{
		m_rectangle.setWidth(width);
		m_rectangle.setHeight(height);

		/* NOTE: The texture must be resized. */
		this->invalidate();
	}

	void
	Surface::setDepth (float depth) noexcept
	{
		m_depth = depth;

		this->updateModelMatrix();
	}

	void
	Surface::move (float deltaX, float deltaY) noexcept
	{
		m_rectangle.move(deltaX, deltaY);

		this->updateModelMatrix();
	}

	bool
	Surface::isBelowPoint (float positionX, float positionY) const noexcept
	{
		/* NOTE: Check on X axis */
		{
			const auto screenWidth = static_cast< float >(m_framebufferProperties.width());

			const auto positionXa = screenWidth * m_rectangle.left();

			if ( positionX < positionXa )
			{
				return false;
			}

			const auto positionXb = screenWidth * m_rectangle.right();

			if ( positionX > positionXb )
			{
				return false;
			}
		}

		/* NOTE: Check on Y axis */
		{
			const auto screenHeight = static_cast< float >(m_framebufferProperties.height());

			const auto positionYa = screenHeight * m_rectangle.top();

			if ( positionY < positionYa )
			{
				return false;
			}

			const auto positionYb = screenHeight * m_rectangle.bottom();

			if ( positionY > positionYb )
			{
				return false;
			}
		}

		return true;
	}

	bool
	Surface::isEventBlocked (float screenX, float screenY) const noexcept
	{
		/* The test is not required at all. */
		if ( !this->isBlockingEvent() )
		{
			return false;
		}

		/* NOTE: The alpha testing is disabled, so whatever the position is, it's blocked. */
		if ( !this->isBlockingEventWithAlphaTest() )
		{
			return true;
		}

		/* Get the pixel coordinates on the surface. */
		const auto surfaceX = static_cast< uint32_t >(screenX - (static_cast< float >(m_framebufferProperties.width()) * m_rectangle.left()));
		const auto surfaceY = static_cast< uint32_t >(screenY - (static_cast< float >(m_framebufferProperties.height()) * m_rectangle.top()));

		/* Get that pixel color from the pixmap. */
		const auto pixelColor = m_frontLocalData.safePixel(surfaceX, surfaceY);
		const auto blocked = pixelColor.alpha() > m_alphaThreshold;

		return blocked;
	}

	void
	Surface::updateModelMatrix () noexcept
	{
		const auto xPosition = (-1.0F + m_rectangle.width()) + (m_rectangle.left() * 2.0F);
		const auto yPosition = (-1.0F + m_rectangle.height()) + (m_rectangle.top() * 2.0F);

		m_modelMatrix.reset();
		m_modelMatrix *= Matrix< 4, float >::translation(xPosition, yPosition, m_depth);
		m_modelMatrix *= Matrix< 4, float >::scaling(m_rectangle.width(), m_rectangle.height(), 1.0F);
	}

	bool
	Surface::createOnHardware (Renderer & renderer) noexcept
	{
		const auto & framebuffer = this->framebufferProperties();
		const auto & geometry = this->geometry();

		const auto textureWidth = framebuffer.getSurfaceWidth(geometry.width());
		const auto textureHeight = framebuffer.getSurfaceHeight(geometry.height());

		if ( !m_backLocalData.initialize(textureWidth, textureHeight, ChannelMode::RGBA) )
		{
			TraceError{ClassId} << "Unable to initialize a " << textureWidth << "x" << textureHeight << "px pixmap for the surface '" << this->name() << "' !";

			return false;
		}

		/* NOTE: We only need one sampler. */
		if ( m_sampler == nullptr || !m_sampler->isCreated() )
		{
			if ( !this->getSampler(renderer) )
			{
				return false;
			}
		}

		if ( !this->createImage(renderer) || !this->createImageView() || !this->createDescriptorSet(renderer) )
		{
			this->clearBackFramebuffer();

			return false;
		}

		/* NOTE: At first creation, we swap automatically. */
		m_flags[ReadyToSwap] = true;

		return this->swapFramebuffers();
	}

	bool
	Surface::destroyFromHardware () noexcept
	{
		/* NOTE: Cleaning the back buffer. */
		this->clearBackFramebuffer();

		/* NOTE: Cleaning the front buffer. */
		{
			if ( m_frontDescriptorSet != nullptr )
			{
				m_frontDescriptorSet->destroy();
				m_frontDescriptorSet.reset();
			}

			if ( m_frontImageView != nullptr )
			{
				m_frontImageView->destroyFromHardware();
				m_frontImageView.reset();
			}

			if ( m_frontImage != nullptr )
			{
				m_frontImage->destroyFromHardware();
				m_frontImage.reset();
			}
		}

		if ( m_sampler != nullptr )
		{
			m_sampler->destroyFromHardware();
			m_sampler.reset();
		}

		return true;
	}

	bool
	Surface::updateVideoMemory (Renderer & renderer) noexcept
	{
		if ( !m_framebufferAccess.try_lock() )
		{
			return true;
		}

		if ( !this->isVideoMemorySizeValid() )
		{
			this->updateModelMatrix();

			if ( !this->updatePhysicalRepresentation(renderer) )
			{
				TraceError{ClassId} << "Unable to update the physical representation of surface '" << this->name() << " !";

				m_framebufferAccess.unlock();

				return false;
			}

			/* NOTE: The texture size is ok, but now the content is invalid. */
			m_flags[VideoMemorySizeValid] = true;
			m_flags[VideoMemoryUpToDate] = false;

			this->onSurfaceReadyForUsage();
		}

		/* NOTE: Updating the front framebuffer when ready for the application. */
		if ( m_frontImage != nullptr && !this->isVideoMemoryUpToDate() )
		{
			const MemoryRegion memoryRegion{
				m_frontLocalData.data().data(),
				m_frontLocalData.bytes()
			};

			if ( !m_frontImage->writeData(renderer.transferManager(), memoryRegion) )
			{
				TraceError{ClassId} << "Unable to update the content of surface '" << this->name() << " !";

				m_framebufferAccess.unlock();

				return false;
			}

			m_flags[VideoMemoryUpToDate] = true;
		}

		m_framebufferAccess.unlock();

		return true;
	}

	bool
	Surface::getSampler (Renderer & renderer) noexcept
	{
		m_sampler = renderer.getSampler(0, 0);
		m_sampler->setIdentifier(ClassId, this->name(), "Sampler");

		if ( m_sampler == nullptr )
		{
			TraceError{ClassId} << "Unable to get a sampler for the surface '" << this->name() << "' !";

			return false;
		}

		return true;
	}

	bool
	Surface::createImage (Renderer & renderer) noexcept
	{
		if ( !m_backLocalData.isValid() )
		{
			TraceError{ClassId} << "The back framebuffer local pixmap is invalid for the surface '" << this->name() << "' ! Unable to create the image for the GPU.";

			return false;
		}

		if ( m_backImage != nullptr && m_backImage->isCreated() )
		{
			TraceError{ClassId} << "The back framebuffer image is already created for the surface '" << this->name() << "' ! Destroy it before.";

			return false;
		}

		m_backImage = std::make_shared< Image >(
			renderer.device(),
			VK_IMAGE_TYPE_2D,
			Image::getFormat< uint8_t >(m_backLocalData.colorCount()),
			VkExtent3D{m_backLocalData.width(), m_backLocalData.height(), 1U},
			VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
			VK_IMAGE_LAYOUT_UNDEFINED,
			0,
			1,
			1,
			VK_SAMPLE_COUNT_1_BIT,
			VK_IMAGE_TILING_OPTIMAL
		);
		m_backImage->setIdentifier(ClassId, this->name(), "Image");

		if ( !m_backImage->create(renderer.transferManager(), m_backLocalData) )
		{
			TraceError{ClassId} << "Unable to create the back framebuffer image for the surface '" << this->name() << "' !";

			m_backImage.reset();

			return false;
		}

		return true;
	}

	bool
	Surface::createImageView () noexcept
	{
		if ( m_backImageView != nullptr && m_backImageView->isCreated() )
		{
			TraceError{ClassId} << "The back framebuffer image view is already created for the surface '" << this->name() << "' ! Destroy it before.";

			return false;
		}

		m_backImageView = std::make_shared< ImageView >(
			m_backImage,
			VK_IMAGE_VIEW_TYPE_2D,
			VkImageSubresourceRange{
				.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
				.baseMipLevel = 0,
				.levelCount = m_backImage->createInfo().mipLevels,
				.baseArrayLayer = 0,
				.layerCount = m_backImage->createInfo().arrayLayers
			}
		);
		m_backImageView->setIdentifier(ClassId, this->name(), "ImageView");

		if ( !m_backImageView->createOnHardware() )
		{
			TraceError{ClassId} << "Unable to create the back framebuffer image view for the surface '" << this->name() << "' !";

			return false;
		}

		return true;
	}

	bool
	Surface::createDescriptorSet (Renderer & renderer) noexcept
	{
		const auto descriptorSetLayout = Manager::getDescriptorSetLayout(renderer.layoutManager());

		if ( descriptorSetLayout == nullptr )
		{
			TraceError{ClassId} << "Unable to get the back framebuffer overlay descriptor set layout for the surface '" << this->name() << "' !";

			return false;
		}

		m_backDescriptorSet = std::make_unique< DescriptorSet >(renderer.descriptorPool(), descriptorSetLayout);
		m_backDescriptorSet->setIdentifier(ClassId, this->name(), "DescriptorSet");

		if ( !m_backDescriptorSet->create() )
		{
			m_backDescriptorSet.reset();

			TraceError{ClassId} << "Unable to create the surface descriptor set for the surface '" << this->name() << "' !";

			return false;
		}

		if ( !m_backDescriptorSet->writeCombinedImageSampler(0, *m_backImage, *m_backImageView, *m_sampler) )
		{
			TraceError{ClassId} << "Unable to write to the surface descriptor set of the surface '" << this->name() << "' !";

			return false;
		}

		return true;
	}

	bool
	Surface::swapFramebuffers () noexcept
	{
		const std::lock_guard< std::mutex > lock{m_framebufferAccess};

		if ( !m_flags[ReadyToSwap] )
		{
			TraceWarning{ClassId} << "The surface '" << this->name() << "' is not ready to swap !";

			return false;
		}

		if ( m_backImage == nullptr || m_backImageView == nullptr || m_backDescriptorSet == nullptr )
		{
			TraceError{ClassId} << "The surface '" << this->name() << "' back framebuffer is invalid !";

			return false;
		}

		/* NOTE: Swap the local data. */
		std::swap(m_backLocalData, m_frontLocalData);

		/* NOTE: Swap the GPU resource pointers. */
		m_backImage.swap(m_frontImage);
		m_backImageView.swap(m_frontImageView);
		m_backDescriptorSet.swap(m_frontDescriptorSet);

		m_flags[ReadyToSwap] = false;

		return true;
	}

	void
	Surface::clearBackFramebuffer () noexcept
	{
		if ( m_backDescriptorSet != nullptr )
		{
			m_backDescriptorSet->destroy();
			m_backDescriptorSet.reset();
		}

		if ( m_backImageView != nullptr )
		{
			m_backImageView->destroyFromHardware();
			m_backImageView.reset();
		}

		if ( m_backImage != nullptr )
		{
			m_backImage->destroyFromHardware();
			m_backImage.reset();
		}
	}

	bool
	Surface::updatePhysicalRepresentation (Renderer & renderer) noexcept
	{
		const auto & framebuffer = this->framebufferProperties();
		const auto & geometry = this->geometry();

		const auto textureWidth = framebuffer.getSurfaceWidth(geometry.width());
		const auto textureHeight = framebuffer.getSurfaceHeight(geometry.height());

		/* NOTE: If the back buffer is different, we prepare it before the swap. */
		if ( m_backLocalData.width() != textureWidth || m_backLocalData.height() != textureHeight )
		{
			if ( !m_backLocalData.initialize(textureWidth, textureHeight, ChannelMode::RGBA) )
			{
				TraceError{ClassId} << "Unable to resize the pixmap for the surface '" << this->name() << "' !";

				return false;
			}

			this->clearBackFramebuffer();

			if ( !this->createImage(renderer) || !this->createImageView() || !this->createDescriptorSet(renderer) )
			{
				this->clearBackFramebuffer();

				return false;
			}
		}

		/* NOTE: Prepare to swap manually or automatically. */
		m_flags[ReadyToSwap] = true;

		if ( m_flags[AutoSwapEnabled] )
		{
			return this->swapFramebuffers();
		}

		return true;
	}

	std::ostream &
	operator<< (std::ostream & out, const Surface & obj)
	{
		return out << "Surface '" << obj.name() << "' [depth:" << obj.depth() << "] " << obj.geometry() <<
			"Model matrix : " << obj.modelMatrix();
	}

	std::string
	to_string (const Surface & obj)
	{
		std::stringstream output;

		output << obj;

		return output.str();
	}
}
