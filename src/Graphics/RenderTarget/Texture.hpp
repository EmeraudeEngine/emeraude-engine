/*
 * src/Graphics/RenderTarget/Texture.hpp
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

#pragma once

/* STL inclusions. */
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>

/* Local inclusions for inheritances. */
#include "Graphics/RenderTarget/Abstract.hpp"
#include "Graphics/TextureResource/Abstract.hpp"

/* Local inclusions for usages. */
#include "Vulkan/Framebuffer.hpp"
#include "Vulkan/Image.hpp"
#include "Vulkan/ImageView.hpp"
#include "Vulkan/Sampler.hpp"
#include "Vulkan/Instance.hpp"
#include "Graphics/Renderer.hpp"
#include "Graphics/ViewMatrices2DUBO.hpp"
#include "Graphics/ViewMatrices3DUBO.hpp"

namespace EmEn::Graphics::RenderTarget
{
	/**
	 * @brief The render to texture template.
	 * @tparam view_matrices_t The type of matrix interface.
	 * @extends EmEn::Graphics::RenderTarget::Abstract This is a render target.
	 * @extends EmEn::Graphics::TextureResource::Abstract This is a usable texture.
	 */
	template< typename view_matrices_t >
	requires (std::is_base_of_v< ViewMatricesInterface, view_matrices_t >)
	class Texture final : public Abstract, public TextureResource::Abstract
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"Texture"};

			/** @brief Defines the resource dependency complexity. */
			static constexpr auto Complexity{Resources::DepComplexity::None};

			/**
			 * @brief Constructs a render to 2D texture.
			 * @param name The name of the texture for debugging.
			 * @param width The width of the texture.
			 * @param width The height of the texture.
			 * @param colorCount The number of color channels desired. Default 4.
			 */
			Texture (const std::string & name, uint32_t width, uint32_t height, uint32_t colorCount = 4) noexcept requires (std::is_same_v< view_matrices_t, ViewMatrices2DUBO >)
				: RenderTarget::Abstract{
					name,
					{colorCount, 8, 32, 0, 1},
					{width, height, 1},
					RenderTargetType::Texture,
					AVConsole::ConnexionType::Both,
					true
				},
				TextureResource::Abstract{name, 0}
			{

			}

			/**
			 * @brief Constructs a render to cubemap.
			 * @param name A reference to a string for the name of the video device.
			 * @param size The size of the cubemap.
			 * @param colorCount The number of color channels desired. Default 4.
			 */
			Texture (const std::string & name, uint32_t size, uint32_t colorCount = 4) noexcept requires (std::is_same_v< view_matrices_t, ViewMatrices3DUBO >)
				: RenderTarget::Abstract{name, {colorCount, 8, 32, 0, 1}, {size, size, 1}, RenderTargetType::Cubemap, AVConsole::ConnexionType::Both, true},
				TextureResource::Abstract{name, 0}
			{

			}

			/**
			 * @brief Constructor version for resource manager.
			 * @param name A reference to a string for the resource name.
			 */
			explicit
			Texture (const std::string & name) noexcept
				: Texture{name, 64, 4}
			{
				Tracer::warning(ClassId, "This resource are not intended to build that way !");
			}


			/** @copydoc EmEn::Libs::ObservableTrait::classUID() const */
			[[nodiscard]]
			size_t
			classUID () const noexcept override
			{
				return std::numeric_limits< size_t >::max();
			}

			/** @copydoc EmEn::Libs::ObservableTrait::is() const */
			[[nodiscard]]
			bool
			is (size_t classUID) const noexcept override
			{
				return classUID == std::numeric_limits< size_t >::max();
			}

			/** @copydoc EmEn::Resources::ResourceTrait::classLabel() const */
			[[nodiscard]]
			const char *
			classLabel () const noexcept override
			{
				return ClassId;
			}

			/** @copydoc EmEn::Graphics::RenderTarget::Abstract::aspectRatio() */
			[[nodiscard]]
			float
			aspectRatio () const noexcept override
			{
				if constexpr ( std::is_same_v< view_matrices_t, ViewMatrices3DUBO >  )
				{
					return 1.0F;
				}
				else
				{
					if ( this->extent().height == 0 )
					{
						return 0.0F;
					}

					return static_cast< float >(this->extent().width) / static_cast< float >(this->extent().height);
				}
			}

			/** @copydoc EmEn::Graphics::RenderTarget::Abstract::isCubemap() */
			[[nodiscard]]
			bool
			isCubemap () const noexcept override
			{
				if constexpr ( std::is_same_v< view_matrices_t, ViewMatrices3DUBO >  )
				{
					return true;
				}
				else
				{
					return false;
				}
			}

			/** @copydoc EmEn::Graphics::RenderTarget::Abstract::viewMatrices() const */
			[[nodiscard]]
			const ViewMatricesInterface &
			viewMatrices () const noexcept override
			{
				return m_viewMatrices;
			}

			/** @copydoc EmEn::Graphics::RenderTarget::Abstract::viewMatrices() */
			[[nodiscard]]
			ViewMatricesInterface &
			viewMatrices () noexcept override
			{
				return m_viewMatrices;
			}

			/** @copydoc EmEn::Graphics::TextureResource::Abstract::type() */
			[[nodiscard]]
			TextureResource::Type
			type () const noexcept override
			{
				if constexpr ( std::is_same_v< view_matrices_t, ViewMatrices3DUBO >  )
				{
					return TextureResource::Type::TextureCube;
				}
				else
				{
					return TextureResource::Type::Texture2D;
				}
			}

			/** @copydoc EmEn::Graphics::TextureResource::Abstract::request3DTextureCoordinates() */
			[[nodiscard]]
			bool
			request3DTextureCoordinates () const noexcept override
			{
				if constexpr ( std::is_same_v< view_matrices_t, ViewMatrices3DUBO >  )
				{
					return true;
				}
				else
				{
					return false;
				}
			}

			/** @copydoc EmEn::AVConsole::AbstractVirtualDevice::videoType() */
			[[nodiscard]]
			AVConsole::VideoType
			videoType () const noexcept override
			{
				return AVConsole::VideoType::Texture;
			}

			/** @copydoc EmEn::Graphics::RenderTarget::Abstract::framebuffer() */
			[[nodiscard]]
			const Vulkan::Framebuffer *
			framebuffer () const noexcept override
			{
				return m_framebuffer.get();
			}

			/** @copydoc EmEn::Graphics::TextureResource::Abstract::image() */
			[[nodiscard]]
			std::shared_ptr< Vulkan::Image >
			image () const noexcept override
			{
				return m_colorImage;
			}

			/** @copydoc EmEn::Graphics::TextureResource::Abstract::imageView() */
			[[nodiscard]]
			std::shared_ptr< Vulkan::ImageView >
			imageView () const noexcept override
			{
				return m_colorImageView;
			}

			/** @copydoc EmEn::Graphics::TextureResource::Abstract::sampler() */
			[[nodiscard]]
			std::shared_ptr< Vulkan::Sampler >
			sampler () const noexcept override
			{
				return m_sampler;
			}

			/** @copydoc EmEn::Graphics::TextureResource::Abstract::isCreated() */
			[[nodiscard]]
			bool
			isCreated () const noexcept override
			{
				if ( m_colorImage == nullptr || !m_colorImage->isCreated() )
				{
					return false;
				}

				if ( m_colorImageView == nullptr || !m_colorImageView->isCreated() )
				{
					return false;
				}

				if ( m_sampler == nullptr || !m_sampler->isCreated() )
				{
					return false;
				}

				return true;
			}

			/** @copydoc EmEn::Graphics::TextureResource::Abstract::createOnHardware() */
			[[nodiscard]]
			bool
			createOnHardware (Renderer & renderer) noexcept override
			{
				return this->create(renderer);
			}

			/** @copydoc EmEn::Graphics::TextureResource::Abstract::destroyFromHardware() */
			bool
			destroyFromHardware () noexcept override
			{
				return this->destroy();
			}

			/** @copydoc EmEn::Graphics::RenderTarget::Abstract::isValid() */
			[[nodiscard]]
			bool
			isValid () const noexcept override
			{
				if ( m_framebuffer == nullptr || !m_framebuffer->isCreated() )
				{
					TraceError{ClassId} << "The render target '" << this->id() << "' (Texture) framebuffer is missing or not created !";

					return false;
				}

				if ( m_colorImage == nullptr || !m_colorImage->isCreated() )
				{
					TraceError{ClassId} << "The render target '" << this->id() << "' (Texture) color image is missing or not created !";

					return false;
				}

				if ( m_colorImageView == nullptr || !m_colorImageView->isCreated() )
				{
					TraceError{ClassId} << "The render target '" << this->id() << "' (Texture) color image view is missing or not created !";

					return false;
				}

				if ( m_depthStencilImage == nullptr || !m_depthStencilImage->isCreated() )
				{
					TraceError{ClassId} << "The render target '" << this->id() << "' (Texture) depth/stencil image is missing or not created !";

					return false;
				}

				if ( m_depthImageView == nullptr || !m_depthImageView->isCreated() )
				{
					TraceError{ClassId} << "The render target '" << this->id() << "' (Texture) depth image view is missing or not created !";

					return false;
				}

				/*if ( m_stencilImageView == nullptr || !m_stencilImageView->isCreated() )
				{
					TraceError{ClassId} << "The render target '" << this->id() << "' (Texture) stencil image view is missing or not created !";

					return false;
				}*/

				if ( m_sampler == nullptr ||  !m_sampler->isCreated() )
				{
					TraceError{ClassId} << "The render target '" << this->id() << "' (Texture) sampler is missing or not created !";

					return false;
				}

				return true;
			}

			/** @copydoc EmEn::Resources::ResourceTrait::load() */
			bool
			load () noexcept override
			{
				Tracer::warning(ClassId, "This resource cannot be loaded from a storage !");

				return false;
			}

			/** @copydoc EmEn::Resources::ResourceTrait::load(const std::filesystem::path &) */
			bool
			load (const std::filesystem::path & /*filepath*/) noexcept override
			{
				return this->load();
			}

			/** @copydoc EmEn::Resources::ResourceTrait::load(const Json::Value &) */
			bool
			load (const Json::Value & /*data*/) noexcept override
			{
				return this->load();
			}

			/** @copydoc EmEn::Resources::ResourceTrait::memoryOccupied() const noexcept */
			[[nodiscard]]
			size_t
			memoryOccupied () const noexcept override
			{
				return sizeof(*this);
			}

			/** @copydoc EmEn::Graphics::TextureResource::Abstract::isGrayScale() */
			[[nodiscard]]
			bool
			isGrayScale () const noexcept override
			{
				/* FIXME: Scan the createInfo */
				return false;
			}

			/** @copydoc EmEn::Graphics::TextureResource::Abstract::averageColor() */
			[[nodiscard]]
			Libs::PixelFactory::Color< float >
			averageColor () const noexcept override
			{
				/* FIXME: Compute average color from video memory texture2Ds. */
				return Libs::PixelFactory::Grey;
			}

			/** @copydoc EmEn::Graphics::TextureResource::Abstract::dimensions() */
			[[nodiscard]]
			uint32_t
			dimensions () const noexcept override
			{
				return this->isCubemap() ? 3 : 2;
			}

			/** @copydoc EmEn::Graphics::TextureResource::Abstract::isCubemapTexture() */
			[[nodiscard]]
			bool
			isCubemapTexture () const noexcept override
			{
				return this->isCubemap();
			}

			/**
			 * @brief Gives access to the main hardware depth stencil image of the render target.
			 * @return std::shared_ptr< Vulkan::Image >
			 */
			[[nodiscard]]
			std::shared_ptr< Vulkan::Image >
			depthStencilImage () const noexcept
			{
				return m_depthStencilImage;
			}

			/**
			 * @brief Gives access to the main hardware depth image view object of the render target.
			 * @return std::shared_ptr< Vulkan::ImageView >
			 */
			[[nodiscard]]
			std::shared_ptr< Vulkan::ImageView >
			depthImageView () const noexcept
			{
				return m_depthImageView;
			}

			/**
			 * @brief Gives access to the main hardware stencil image view object of the render target.
			 * @return std::shared_ptr< Vulkan::ImageView >
			 */
			[[nodiscard]]
			std::shared_ptr< Vulkan::ImageView >
			stencilImageView () const noexcept
			{
				return m_stencilImageView;
			}

		private:

			/** @copydoc EmEn::AVConsole::AbstractVirtualDevice::updateDeviceFromCoordinates() */
			void
			updateDeviceFromCoordinates (const Libs::Math::CartesianFrame< float > & worldCoordinates, const Libs::Math::Vector< 3, float > & worldVelocity) noexcept override
			{
				m_viewMatrices.updateViewCoordinates(worldCoordinates, worldVelocity);
			}

			/** @copydoc EmEn::AVConsole::AbstractVirtualDevice::updateProperties() */
			void
			updateProperties (bool isPerspectiveProjection, float distance, float fovOrNear) noexcept override
			{
				const auto & extent = this->extent();

				if ( isPerspectiveProjection )
				{
					m_viewMatrices.updatePerspectiveViewProperties(static_cast< float >(extent.width), static_cast< float >(extent.height), distance, fovOrNear);
				}
				else
				{
					m_viewMatrices.updateOrthographicViewProperties(static_cast< float >(extent.width), static_cast< float >(extent.height), distance, fovOrNear);
				}
			}

			/** @copydoc EmEn::AVConsole::AbstractVirtualDevice::onInputDeviceConnected() */
			void
			onInputDeviceConnected (AVConsole::AVManagers & managers, AbstractVirtualDevice * /*sourceDevice*/) noexcept override
			{
				m_viewMatrices.create(managers.graphicsRenderer, this->id());
			}

			/** @copydoc EmEn::AVConsole::AbstractVirtualDevice::onSourceDisconnected() */
			void
			onInputDeviceDisconnected (AVConsole::AVManagers & /*managers*/, AbstractVirtualDevice * /*sourceDevice*/) noexcept override
			{
				m_viewMatrices.destroy();
			}

			/** @copydoc EmEn::Graphics::RenderTarget::Abstract::onCreate() */
			[[nodiscard]]
			bool
			onCreate (Renderer & renderer) noexcept override
			{
				if ( !this->createImages(renderer.device()) )
				{
					return false;
				}

				/* Create a sampler for the texture. */
				m_sampler = renderer.getSampler(0, 0);
				m_sampler->setIdentifier(ClassId, this->name(), "Sampler");

				if ( m_sampler == nullptr )
				{
					Tracer::error(ClassId, "Unable to get a sampler !");

					return false;
				}

				const auto renderPass = this->createRenderPass(renderer);

				if ( renderPass == nullptr )
				{
					return false;
				}

				return this->createFramebuffer(renderPass);
			}

			/** @copydoc EmEn::Graphics::RenderTarget::Abstract::onDestroy() */
			void
			onDestroy () noexcept override
			{
				/* First, destroy the framebuffer. */
				m_framebuffer.reset();

				/* Next, destroy the sampler. */
				m_sampler.reset();

				/* Next, destroy the image views. */
				m_stencilImageView.reset();
				m_depthImageView.reset();
				m_colorImageView.reset();

				/* Finally, destroy the images. */
				m_depthStencilImage.reset();
				m_colorImage.reset();
			}

			/** @copydoc EmEn::Graphics::RenderTarget::Abstract::createRenderPass() */
			[[nodiscard]]
			std::shared_ptr< Vulkan::RenderPass >
			createRenderPass (Renderer & renderer) const noexcept override
			{
				/* FIXME: The identifier must reflect the enabled attachments !!! */
				auto renderPass = renderer.getRenderPass(TextureRender, 0);

				if ( !renderPass->isCreated() )
				{
					/* Prepare a subPass for the render pass. */
					Vulkan::RenderSubPass subPass{VK_PIPELINE_BIND_POINT_GRAPHICS, 0};

					/* Color buffer. */
					if ( this->precisions().colorBits() > 0 )
					{
						renderPass->addAttachmentDescription(VkAttachmentDescription{
							.flags = 0,
							.format = m_colorImage->createInfo().format,
							.samples = VK_SAMPLE_COUNT_1_BIT,
							.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
							.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
							.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
							.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
							.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
							.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
						});

						subPass.addColorAttachment(0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
					}

					/* Depth/Stencil buffer. */
					if ( this->precisions().depthBits() > 0 || this->precisions().stencilBits() > 0 )
					{
						renderPass->addAttachmentDescription(VkAttachmentDescription{
							.flags = 0,
							.format = m_depthStencilImage->createInfo().format,
							.samples = VK_SAMPLE_COUNT_1_BIT,
							.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
							.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
							.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
							.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
							.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
							.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
						});

						subPass.setDepthStencilAttachment(1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
					}

					renderPass->addSubPass(subPass);

					renderPass->addSubPassDependency({
						.srcSubpass = VK_SUBPASS_EXTERNAL,
						.dstSubpass = 0,
						.srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
						.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
						.srcAccessMask = VK_ACCESS_NONE,
						.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
						.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT
					});

					renderPass->addSubPassDependency({
						.srcSubpass = 0,
						.dstSubpass = VK_SUBPASS_EXTERNAL,
						.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
						.dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
						.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
						.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT,
						.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT
					});

					if ( !renderPass->createOnHardware() )
					{
						TraceError{ClassId} << "Unable to create the render pass for texture '" << this->id() << "' !";

						return nullptr;
					}
				}

				return renderPass;
			}

			/**
			 * @brief Creates the images and the image views for each swap chain frame.
			 * @param device A reference to the graphics device smart pointer.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			createImages (const std::shared_ptr< Vulkan::Device > & device) noexcept
			{
				/* Color buffer. */
				if ( this->precisions().colorBits() > 0 )
				{
					/* Create the image for a color buffer in video memory. */
					m_colorImage = std::make_shared< Vulkan::Image >(
						device,
						VK_IMAGE_TYPE_2D,
						Vulkan::Instance::findColorFormat(device, this->precisions()),
						this->extent(),
						VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
						VK_IMAGE_LAYOUT_UNDEFINED
					);
					m_colorImage->setIdentifier(ClassId, this->name(), "Image");
					m_colorImage->setCurrentImageLayout(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

					if ( !m_colorImage->createOnHardware() )
					{
						TraceError{ClassId} << "Unable to create an image (Color buffer) for texture '" << this->id() << "' !";

						return false;
					}

					/* Create a view to exploit the image. */
					m_colorImageView = std::make_shared< Vulkan::ImageView >(
						m_colorImage,
						VK_IMAGE_VIEW_TYPE_2D,
						VkImageSubresourceRange{
							.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
							.baseMipLevel = 0,
							.levelCount = m_colorImage->createInfo().mipLevels,
							.baseArrayLayer = 0,
							.layerCount = m_colorImage->createInfo().arrayLayers
						}
					);
					m_colorImageView->setIdentifier(ClassId, this->name(), "ImageView");

					if ( !m_colorImageView->createOnHardware() )
					{
						TraceError{ClassId} << "Unable to create an image view (Color buffer) for texture '" << this->id() << "' !";

						return false;
					}
				}

				/* Depth/stencil buffer. */
				if ( this->precisions().depthBits() > 0 || this->precisions().stencilBits() > 0 )
				{
					/* Create the image for depth/stencil buffer in video memory. */
					m_depthStencilImage = std::make_shared< Vulkan::Image >(
						device,
						VK_IMAGE_TYPE_2D,
						Vulkan::Instance::findDepthStencilFormat(device, this->precisions()),
						this->extent(),
						VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
						VK_IMAGE_LAYOUT_UNDEFINED
					);
					m_depthStencilImage->setIdentifier(ClassId, this->name(), "Image");
					m_depthStencilImage->setCurrentImageLayout(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

					if ( !m_depthStencilImage->createOnHardware() )
					{
						TraceError{ClassId} << "Unable to create an image (Depth/stencil buffer) for texture '" << this->id() << "' !";

						return false;
					}

					/* Create a view to exploit the depth part of the image. */
					if ( this->precisions().depthBits() > 0 )
					{
						m_depthImageView = std::make_shared< Vulkan::ImageView >(
							m_depthStencilImage,
							VK_IMAGE_VIEW_TYPE_2D,
							VkImageSubresourceRange{
								.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
								.baseMipLevel = 0,
								.levelCount = m_depthStencilImage->createInfo().mipLevels,
								.baseArrayLayer = 0,
								.layerCount = m_depthStencilImage->createInfo().arrayLayers
							}
						);
						m_depthImageView->setIdentifier(ClassId, this->name(), "ImageView");

						if ( !m_depthImageView->createOnHardware() )
						{
							TraceError{ClassId} << "Unable to create an image view (Depth buffer) for texture '" << this->id() << "' !";

							return false;
						}
					}

					/* Create a view to exploit the stencil part of the image. */
					if ( this->precisions().stencilBits() > 0 )
					{
						m_stencilImageView = std::make_shared< Vulkan::ImageView >(
							m_depthStencilImage,
							VK_IMAGE_VIEW_TYPE_2D,
							VkImageSubresourceRange{
								.aspectMask = VK_IMAGE_ASPECT_STENCIL_BIT,
								.baseMipLevel = 0,
								.levelCount = m_depthStencilImage->createInfo().mipLevels,
								.baseArrayLayer = 0,
								.layerCount = m_depthStencilImage->createInfo().arrayLayers
							}
						);
						m_stencilImageView->setIdentifier(ClassId, this->name(), "ImageView");

						if ( !m_stencilImageView->createOnHardware() )
						{
							TraceError{ClassId} << "Unable to create an image view (Stencil buffer) for texture '" << this->id() << "' !";

							return false;
						}
					}
				}

				return true;
			}

			/**
			 * @brief Creates the framebuffer.
			 * @param renderPass A reference to the render pass smart pointer.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			createFramebuffer (const std::shared_ptr< Vulkan::RenderPass > & renderPass) noexcept
			{
				m_framebuffer = std::make_shared< Vulkan::Framebuffer >(renderPass, this->extent());
				m_framebuffer->setIdentifier(ClassId, this->name(), "Framebuffer");

				/* Color buffer. */
				if ( this->precisions().colorBits() > 0 )
				{
					m_framebuffer->addAttachment(m_colorImageView->handle());
				}

				/* Depth buffer. */
				if ( this->precisions().depthBits() > 0 )
				{
					m_framebuffer->addAttachment(m_depthImageView->handle());
				}

				/* Stencil buffer. */
				if ( this->precisions().stencilBits() > 0 )
				{
					m_framebuffer->addAttachment(m_stencilImageView->handle());
				}

				if ( !m_framebuffer->createOnHardware() )
				{
					TraceError{ClassId} << "Unable to create the framebuffer for texture '" << this->id() << "' !";

					return false;
				}

				return true;
			}

			std::shared_ptr< Vulkan::Image > m_colorImage;
			std::shared_ptr< Vulkan::Image > m_depthStencilImage;
			std::shared_ptr< Vulkan::ImageView > m_colorImageView;
			std::shared_ptr< Vulkan::ImageView > m_depthImageView;
			std::shared_ptr< Vulkan::ImageView > m_stencilImageView;
			std::shared_ptr< Vulkan::Sampler > m_sampler;
			std::shared_ptr< Vulkan::Framebuffer > m_framebuffer;
			view_matrices_t m_viewMatrices;
	};
}
