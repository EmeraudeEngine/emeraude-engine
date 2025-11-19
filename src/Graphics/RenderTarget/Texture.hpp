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
#include "Vulkan/TextureInterface.hpp"
#include "Graphics/RenderTarget/Abstract.hpp"

/* Local inclusions for usages. */
#include "Libs/PixelFactory/Processor.hpp"
#include "Vulkan/Instance.hpp"
#include "Vulkan/Framebuffer.hpp"
#include "Graphics/Renderer.hpp"
#include "Graphics/ViewMatrices2DUBO.hpp"
#include "Graphics/ViewMatrices3DUBO.hpp"

namespace EmEn::Graphics::RenderTarget
{
	/**
	 * @brief The render to texture template.
	 * @todo A render-to-texture being part of it's own rendering creates validation layer bugs! Example: Camera filming the TV where it shows the image.
	 * @tparam view_matrices_t The type of matrix interface.
	 * @extends EmEn::Vulkan::TextureInterface This is a texture.
	 * @extends EmEn::Graphics::RenderTarget::Abstract This is a render target.
	 */
	template< typename view_matrices_t >
	requires (std::is_base_of_v< ViewMatricesInterface, view_matrices_t >)
	class Texture final : public Vulkan::TextureInterface, public Abstract
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
			 * @param colorCount The number of color channels desired.
			 * @param viewDistance The max viewable distance in meters.
			 * @param isOrthographicProjection Set orthographic projection instead of perspective.
			 */
			Texture (const std::string & name, uint32_t width, uint32_t height, uint32_t colorCount, float viewDistance, bool isOrthographicProjection) noexcept requires (std::is_same_v< view_matrices_t, ViewMatrices2DUBO >)
				: Abstract{
					name,
					{colorCount, 8U, 32U, 0U, 1U},
					{width, height, 1U},
					viewDistance,
					RenderTargetType::Texture,
					Scenes::AVConsole::ConnexionType::Both,
					isOrthographicProjection,
					true
				}
			{

			}

			/**
			 * @brief Constructs a render to cubemap.
			 * @param name A reference to a string for the name of the video device.
			 * @param size The size of the cubemap.
			 * @param colorCount The number of color channels desired.
			 * @param viewDistance The max viewable distance in meters.
			 * @param isOrthographicProjection Set orthographic projection instead of perspective.
			 */
			Texture (const std::string & name, uint32_t size, uint32_t colorCount, float viewDistance, bool isOrthographicProjection) noexcept requires (std::is_same_v< view_matrices_t, ViewMatrices3DUBO >)
				: Abstract{
					name,
					{colorCount, 8U, 32U, 0U, 1U},
					{size, size, 1U},
					viewDistance,
					RenderTargetType::Cubemap,
					Scenes::AVConsole::ConnexionType::Both,
					isOrthographicProjection,
					true
				}
			{

			}

			/** @copydoc EmEn::Vulkan::TextureInterface::isCreated() const noexcept */
			[[nodiscard]]
			bool
			isCreated () const noexcept override
			{
				/* NOTE: Extra checks. */
				if constexpr ( IsDebug )
				{
					if ( m_colorImage == nullptr || !m_colorImage->isCreated() )
					{
						return false;
					}

					if ( m_colorImageView == nullptr || !m_colorImageView->isCreated() )
					{
						return false;
					}

					if ( this->isCubemap() )
					{
						if ( m_colorCubeImageView == nullptr || !m_colorCubeImageView->isCreated() )
						{
							return false;
						}
					}

					/*if ( m_depthStencilImage == nullptr || !m_depthStencilImage->isCreated() )
					{
						return false;
					}

					if ( m_depthImageView == nullptr || !m_depthImageView->isCreated() )
					{
						return false;
					}

					if ( m_stencilImageView == nullptr || !m_stencilImageView->isCreated() )
					{
						return false;
					}*/
				}

				if ( m_sampler == nullptr || !m_sampler->isCreated() )
				{
					return false;
				}

				if ( m_framebuffer == nullptr || !m_framebuffer->isCreated() )
				{
					return false;
				}

				return true;
			}

			/** @copydoc EmEn::Vulkan::TextureInterface::type() const noexcept */
			[[nodiscard]]
			Vulkan::TextureType
			type () const noexcept override
			{
				if constexpr ( std::is_same_v< view_matrices_t, ViewMatrices3DUBO > )
				{
					return Vulkan::TextureType::TextureCube;
				}
				else
				{
					return Vulkan::TextureType::Texture2D;
				}
			}

			/** @copydoc EmEn::Vulkan::TextureInterface::dimensions() const noexcept */
			[[nodiscard]]
			uint32_t
			dimensions () const noexcept override
			{
				if constexpr ( std::is_same_v< view_matrices_t, ViewMatrices3DUBO > )
				{
					return 3;
				}
				else
				{
					return 2;
				}
			}

			/** @copydoc EmEn::Vulkan::TextureInterface::isCubemapTexture() const noexcept */
			[[nodiscard]]
			bool
			isCubemapTexture () const noexcept override
			{
				if constexpr ( std::is_same_v< view_matrices_t, ViewMatrices3DUBO > )
				{
					return true;
				}
				else
				{
					return false;
				}
			}

			/** @copydoc EmEn::Vulkan::TextureInterface::image() const noexcept */
			[[nodiscard]]
			std::shared_ptr< Vulkan::Image >
			image () const noexcept override
			{
				return m_colorImage;
			}

			/** @copydoc EmEn::Vulkan::TextureInterface::imageView() const noexcept */
			[[nodiscard]]
			std::shared_ptr< Vulkan::ImageView >
			imageView () const noexcept override
			{
				/* NOTE: As a texture request, we give the right image view with cubemap. */
				if ( this->isCubemap() && m_colorCubeImageView != nullptr )
				{
					return m_colorCubeImageView;
				}

				return m_colorImageView;
			}

			/** @copydoc EmEn::Vulkan::TextureInterface::sampler() const noexcept */
			[[nodiscard]]
			std::shared_ptr< Vulkan::Sampler >
			sampler () const noexcept override
			{
				return m_sampler;
			}

			/** @copydoc EmEn::Vulkan::TextureInterface::request3DTextureCoordinates() const noexcept */
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

			/** @copydoc EmEn::Graphics::RenderTarget::Abstract::updateViewRangesProperties() */
			void
			updateViewRangesProperties (float fovOrNear, float distanceOrFar) noexcept override
			{
				const auto & extent = this->extent();
				const auto width = static_cast< float >(extent.width);
				const auto height = static_cast< float >(extent.width);

				if ( this->isOrthographicProjection() )
				{
					m_viewMatrices.updateOrthographicViewProperties(width, height, fovOrNear, distanceOrFar);
				}
				else
				{
					m_viewMatrices.updatePerspectiveViewProperties(width, height, fovOrNear, distanceOrFar);
				}

				this->setViewDistance(distanceOrFar);
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

			/** @copydoc EmEn::Scenes::AVConsole::AbstractVirtualDevice::videoType() */
			[[nodiscard]]
			Scenes::AVConsole::VideoType
			videoType () const noexcept override
			{
				return Scenes::AVConsole::VideoType::Texture;
			}

			/** @copydoc EmEn::Graphics::RenderTarget::Abstract::framebuffer() const */
			[[nodiscard]]
			const Vulkan::Framebuffer *
			framebuffer () const noexcept override
			{
				return m_framebuffer.get();
			}

			/** @copydoc EmEn::Graphics::RenderTarget::Abstract::isReadyForRendering() const */
			[[nodiscard]]
			bool
			isReadyForRendering () const noexcept override
			{
				return m_isReadyForRendering;
			}

			/** @copydoc EmEn::Graphics::RenderTarget::Abstract::capture() */
			[[nodiscard]]
			std::array< Libs::PixelFactory::Pixmap< uint8_t >, 3 >
			capture (Vulkan::TransferManager & transferManager, uint32_t layerIndex, bool keepAlpha, bool withDepthBuffer, bool withStencilBuffer) const noexcept override
			{
				std::array< Libs::PixelFactory::Pixmap< uint8_t >, 3 > result{};

				/* NOTE: Validate layer index for cubemaps and single-layer textures. */
				const uint32_t maxLayers = this->isCubemap() ? 6 : 1;

				if ( layerIndex >= maxLayers )
				{
					if ( maxLayers == 1 && layerIndex > 0 )
					{
						TraceWarning{ClassId} << "Single-layer texture does not support layer " << layerIndex << ". Using layer 0 instead for texture '" << this->id() << "'.";

						layerIndex = 0;
					}
					else
					{
						TraceError{ClassId} << "Invalid layer index " << layerIndex << " (max: " << maxLayers - 1 << ") for texture '" << this->id() << "' !";

						return result;
					}
				}

				/* Capture color buffer. */
				if ( m_colorImage != nullptr && m_colorImage->isCreated() )
				{
					if ( !transferManager.downloadImage(*m_colorImage, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT, result[0]) )
					{
						TraceError{ClassId} << "Failed to capture color buffer for texture '" << this->id() << "' !";
						return result;
					}

					/* Convert to RGB if alpha is not requested. */
					if ( !keepAlpha )
					{
						result[0] = Libs::PixelFactory::Processor< uint8_t >::toRGB(result[0]);
					}
				}

				/* Capture depth buffer (optional). */
				if ( withDepthBuffer && m_depthStencilImage != nullptr && m_depthStencilImage->isCreated() && this->precisions().depthBits() > 0 )
				{
					if ( !transferManager.downloadImage(*m_depthStencilImage, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_IMAGE_ASPECT_DEPTH_BIT, result[1]) )
					{
						TraceWarning{ClassId} << "Failed to capture depth buffer for texture '" << this->id() << "' !";
					}
				}

				/* Capture stencil buffer (optional). */
				if ( withStencilBuffer && m_depthStencilImage != nullptr && m_depthStencilImage->isCreated() && this->precisions().stencilBits() > 0 )
				{
					if ( !transferManager.downloadImage(*m_depthStencilImage, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_IMAGE_ASPECT_STENCIL_BIT, result[2]) )
					{
						TraceWarning{ClassId} << "Failed to capture stencil buffer for texture '" << this->id() << "' !";
					}
				}

				return result;
			}

		private:

			/** @copydoc EmEn::Scenes::AVConsole::AbstractVirtualDevice::updateVideoDeviceProperties() */
			void
			updateVideoDeviceProperties (float fovOrNear, float distanceOrFar, bool isOrthographicProjection) noexcept override
			{
				this->setOrthographicProjection(isOrthographicProjection);

				this->updateViewRangesProperties(fovOrNear, distanceOrFar);
			}

			/** @copydoc EmEn::Scenes::AVConsole::AbstractVirtualDevice::updateDeviceFromCoordinates() */
			void
			updateDeviceFromCoordinates (const Libs::Math::CartesianFrame< float > & worldCoordinates, const Libs::Math::Vector< 3, float > & worldVelocity) noexcept override
			{
				m_viewMatrices.updateViewCoordinates(worldCoordinates, worldVelocity);
			}

			/** @copydoc EmEn::Scenes::AVConsole::AbstractVirtualDevice::onInputDeviceConnected() */
			void
			onInputDeviceConnected (Scenes::AVConsole::AVManagers & managers, AbstractVirtualDevice & /*sourceDevice*/) noexcept override
			{
				m_viewMatrices.create(managers.graphicsRenderer, this->id());
			}

			/** @copydoc EmEn::Scenes::AVConsole::AbstractVirtualDevice::onInputDeviceDisconnected() */
			void
			onInputDeviceDisconnected (Scenes::AVConsole::AVManagers & /*managers*/, AbstractVirtualDevice & /*sourceDevice*/) noexcept override
			{
				m_viewMatrices.destroy();
			}

			/** @copydoc EmEn::Graphics::RenderTarget::Abstract::onCreate() */
			[[nodiscard]]
			bool
			onCreate (Renderer & renderer) noexcept override
			{
				/* NOTE: Creation of images and image views and
				 * get them ready for the render-to-texture. */
				if ( !this->createImages(renderer) )
				{
					return false;
				}

				/* NOTE: Create a sampler for the texture to be samplable in fragment shaders. */
				m_sampler = renderer.getSampler("RenderToTexture", [] (Settings & settings, VkSamplerCreateInfo & createInfo) {
					const auto magFilter = settings.getOrSetDefault< std::string >(GraphicsTextureMagFilteringKey, DefaultGraphicsTextureFiltering);
					const auto minFilter = settings.getOrSetDefault< std::string >(GraphicsTextureMinFilteringKey, DefaultGraphicsTextureFiltering);
					const auto mipmapMode = settings.getOrSetDefault< std::string >(GraphicsTextureMipFilteringKey, DefaultGraphicsTextureFiltering);
					const auto mipLevels = settings.getOrSetDefault< float >(GraphicsTextureMipMappingLevelsKey, DefaultGraphicsTextureMipMappingLevels);
					const auto anisotropyLevels = settings.getOrSetDefault< float >(GraphicsTextureAnisotropyLevelsKey, DefaultGraphicsTextureAnisotropy);

					//createInfo.flags = 0;
					createInfo.magFilter = magFilter == "linear" ? VK_FILTER_LINEAR : VK_FILTER_NEAREST;
					createInfo.minFilter = minFilter == "linear" ? VK_FILTER_LINEAR : VK_FILTER_NEAREST;
					createInfo.mipmapMode = mipmapMode == "linear" ? VK_SAMPLER_MIPMAP_MODE_LINEAR : VK_SAMPLER_MIPMAP_MODE_NEAREST;
					//createInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
					//createInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
					//createInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
					//createInfo.mipLodBias = 0.0F;
					createInfo.anisotropyEnable = anisotropyLevels > 1.0F ? VK_TRUE : VK_FALSE;
					createInfo.maxAnisotropy = anisotropyLevels;
					//createInfo.compareEnable = VK_FALSE;
					//createInfo.compareOp = VK_COMPARE_OP_ALWAYS;
					//createInfo.minLod = 0.0F;
					createInfo.maxLod = mipLevels > 0.0F ? mipLevels : VK_LOD_CLAMP_NONE;
					//createInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
					//createInfo.unnormalizedCoordinates = VK_FALSE;
				});

				if ( m_sampler == nullptr )
				{
					TraceError{ClassId} << "Unable to create a sampler for the render-to-texture '" << this->id() << "' !";

					return false;
				}

				/* NOTE: Create the render pass and the framebuffer to render into the texture. */
				const auto renderPass = this->createRenderPass(renderer);

				if ( renderPass == nullptr )
				{
					return false;
				}

				if ( !this->createFramebuffer(renderPass) )
				{
					return false;
				}

				m_isReadyForRendering = true;

				return true;
			}

			/** @copydoc EmEn::Graphics::RenderTarget::Abstract::onDestroy() */
			void
			onDestroy () noexcept override
			{
				m_isReadyForRendering = false;

				/* The main framebuffer. */
				m_framebuffer.reset();

				/* The texture sampler. */
				m_sampler.reset();

				/* The depth/stencil buffers. */
				m_stencilImageView.reset();
				m_depthImageView.reset();
				m_depthStencilImage.reset();

				/* The color buffer. */
				m_colorCubeImageView.reset();
				m_colorImageView.reset();
				m_colorImage.reset();
			}

			/**
			 * @brief Creates the images and the image views for each swap chain frame.
			 * @param renderer A reference to the graphics renderer.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			createImages (const Renderer & renderer) noexcept
			{
				const auto device = renderer.device();

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
						this->isCubemap() ? VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT : 0,
						1,
						this->isCubemap() ? 6 : 1
					);
					m_colorImage->setIdentifier(ClassId, this->id(), "Image");

					if ( !m_colorImage->createOnHardware() )
					{
						TraceError{ClassId} << "Unable to create an image (Color buffer) for texture '" << this->id() << "' !";

						return false;
					}

					/* Create a view to exploit the image. */
					m_colorImageView = std::make_shared< Vulkan::ImageView >(
						m_colorImage,
						/* NOTE: Here we use VK_IMAGE_VIEW_TYPE_2D_ARRAY instead
						 * of VK_IMAGE_VIEW_TYPE_CUBE when rendering to a cubemap for the multiview feature. */
						this->isCubemap() ? VK_IMAGE_VIEW_TYPE_2D_ARRAY : VK_IMAGE_VIEW_TYPE_2D,
						VkImageSubresourceRange{
							.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
							.baseMipLevel = 0,
							.levelCount = m_colorImage->createInfo().mipLevels,
							.baseArrayLayer = 0,
							.layerCount = m_colorImage->createInfo().arrayLayers
						}
					);
					m_colorImageView->setIdentifier(ClassId, this->id(), "ImageView");

					if ( !m_colorImageView->createOnHardware() )
					{
						TraceError{ClassId} << "Unable to create an image view (Color buffer) for texture '" << this->id() << "' !";

						return false;
					}

					/* NOTE: Perform an initial layout transition from UNDEFINED to SHADER_READ_ONLY_OPTIMAL.
					 * This allows the texture to be used immediately in descriptors/materials.
					 * The RenderPass will transition to COLOR_ATTACHMENT_OPTIMAL when rendering,
					 * then back to SHADER_READ_ONLY_OPTIMAL when done. */
					if ( !renderer.transferManager().transitionImageLayout(*m_colorImage, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) )
					{
						TraceError{ClassId} << "Unable to transition color image to SHADER_READ_ONLY_OPTIMAL for texture '" << this->id() << "' !";

						return false;
					}

					/* NOTE: Create a specific view for reading
					 * the cubemap in shaders according to the multiview feature. */
					if ( this->isCubemap() )
					{
						m_colorCubeImageView = std::make_shared< Vulkan::ImageView >(
							m_colorImage,
							VK_IMAGE_VIEW_TYPE_CUBE,
							VkImageSubresourceRange{
								.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
								.baseMipLevel = 0,
								.levelCount = m_colorImage->createInfo().mipLevels,
								.baseArrayLayer = 0,
								.layerCount = 6
							}
						);
						m_colorCubeImageView->setIdentifier(ClassId, this->id(), "CubeImageView");

						if ( !m_colorCubeImageView->createOnHardware() )
						{
							TraceError{ClassId} << "Unable to create a cube image view (Color buffer) for texture '" << this->id() << "' !";

							return false;
						}
					}
				}
				else
				{
					TraceError{ClassId} << "No color bits requested for texture '" << this->id() << "' !";

					return false;
				}

				/* Depth/stencil buffer (optional). */
				if ( this->precisions().depthBits() > 0 || this->precisions().stencilBits() > 0 )
				{
					/* Create the image for depth/stencil buffer in video memory. */
					m_depthStencilImage = std::make_shared< Vulkan::Image >(
						device,
						VK_IMAGE_TYPE_2D,
						Vulkan::Instance::findDepthStencilFormat(device, this->precisions()),
						this->extent(),
						VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
						0, // No special flags for depth/stencil cubemap arrays
						1, // Single mip level
						this->isCubemap() ? 6 : 1 // 6 layers for cubemap, 1 for regular 2D
					);
					m_depthStencilImage->setIdentifier(ClassId, this->id(), "DepthStencilImage");

					if ( !m_depthStencilImage->createOnHardware() )
					{
						TraceError{ClassId} << "Unable to create an image (Depth/stencil buffer) for texture '" << this->id() << "' !";

						return false;
					}

					/* NOTE: Set the final image layout for being usable with a material. */
					m_depthStencilImage->setCurrentImageLayout(VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

					/* Create a view to exploit the depth part of the image. */
					if ( this->precisions().depthBits() > 0 )
					{
						m_depthImageView = std::make_shared< Vulkan::ImageView >(
							m_depthStencilImage,
							this->isCubemap() ? VK_IMAGE_VIEW_TYPE_2D_ARRAY : VK_IMAGE_VIEW_TYPE_2D,
							VkImageSubresourceRange{
								.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
								.baseMipLevel = 0,
								.levelCount = m_depthStencilImage->createInfo().mipLevels,
								.baseArrayLayer = 0,
								.layerCount = m_depthStencilImage->createInfo().arrayLayers
							}
						);
						m_depthImageView->setIdentifier(ClassId, this->id(), "DepthImageView");

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
							this->isCubemap() ? VK_IMAGE_VIEW_TYPE_2D_ARRAY : VK_IMAGE_VIEW_TYPE_2D,
							VkImageSubresourceRange{
								.aspectMask = VK_IMAGE_ASPECT_STENCIL_BIT,
								.baseMipLevel = 0,
								.levelCount = m_depthStencilImage->createInfo().mipLevels,
								.baseArrayLayer = 0,
								.layerCount = m_depthStencilImage->createInfo().arrayLayers
							}
						);
						m_stencilImageView->setIdentifier(ClassId, this->id(), "StencilImageView");

						if ( !m_stencilImageView->createOnHardware() )
						{
							TraceError{ClassId} << "Unable to create an image view (Stencil buffer) for texture '" << this->id() << "' !";

							return false;
						}
					}
				}

				return true;
			}

			/** @copydoc EmEn::Graphics::RenderTarget::Abstract::createRenderPass() */
			[[nodiscard]]
			std::shared_ptr< Vulkan::RenderPass >
			createRenderPass (Renderer & renderer) const noexcept override
			{
				/* Create a new RenderPass for this texture render target. */
				auto renderPass = std::make_shared< Vulkan::RenderPass >(renderer.device(), 0);
				renderPass->setIdentifier(ClassId, this->id(), "RenderPass");

				/* Prepare a subPass for the render pass. */
				Vulkan::RenderSubPass subPass{VK_PIPELINE_BIND_POINT_GRAPHICS, 0};

				/* Color buffer. */
				if ( m_colorImage != nullptr )
				{
					renderPass->addAttachmentDescription(VkAttachmentDescription{
						.flags = 0,
						.format = m_colorImage->createInfo().format,
						.samples = VK_SAMPLE_COUNT_1_BIT,
						.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
						.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
						.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
						.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
						/* The image starts in SHADER_READ_ONLY_OPTIMAL (transitioned at creation),
						 * transitions to COLOR_ATTACHMENT_OPTIMAL during rendering,
						 * then back to SHADER_READ_ONLY_OPTIMAL when done. */
						.initialLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
						.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
					});

					subPass.addColorAttachment(0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
				}
				else
				{
					TraceError{ClassId} << "color depth image is not created for texture '" << this->id() << "' !";

					return nullptr;
				}

				/* Depth/Stencil buffer (optional). */
				if ( m_depthStencilImage != nullptr )
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
					/* Wait for fragment shader reads from previous pass to complete... */
					.srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
					/* ...before the new pass begins to write in color. */
					.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
					/* The access to wait is a shader read. */
					.srcAccessMask = VK_ACCESS_SHADER_READ_BIT,
					/* The new access will be a "write" in an attachment. */
					.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
					.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT
				});

				renderPass->addSubPassDependency({
					.srcSubpass = 0,
					.dstSubpass = VK_SUBPASS_EXTERNAL,
					/* Wait until the writing in color is finished... */
					.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
					/* ...before the next pass can read the result into its fragment shader. */
					.dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
					/* The access to make visible is the writing in the attachment. */
					.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
					/* The next access will be a shader read. */
					.dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
					.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT
				});

				/* Enable multiview for cubemap rendering (Vulkan 1.1+) */
				if ( this->isCubemap() )
				{
					renderPass->enableMultiview();
				}

				if ( !renderPass->createOnHardware() )
				{
					TraceError{ClassId} << "Unable to create the render pass for texture '" << this->id() << "' !";

					return nullptr;
				}

				return renderPass;
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
				m_framebuffer->setIdentifier(ClassId, this->id(), "Framebuffer");

				/* Attach the color buffer. */
				if ( m_colorImageView != nullptr )
				{
					m_framebuffer->addAttachment(m_colorImageView->handle());
				}
				else
				{
					TraceError{ClassId} << "The color image view is not created for texture '" << this->id() << "' !";

					return false;
				}

				/* Attach the depth buffer, if present. */
				if ( m_depthImageView != nullptr )
				{
					m_framebuffer->addAttachment(m_depthImageView->handle());
				}
				else if constexpr ( IsDebug )
				{
					if ( this->precisions().depthBits() > 0 )
					{
						TraceError{ClassId} << "The depth image view is not created for texture '" << this->id() << "', but was requested !";

						return false;
					}
				}

				/* Attach the stencil buffer, if present. */
				if ( m_stencilImageView != nullptr )
				{
					m_framebuffer->addAttachment(m_stencilImageView->handle());
				}
				else if constexpr ( IsDebug )
				{
					if ( this->precisions().stencilBits() > 0 )
					{
						TraceError{ClassId} << "The stencil image view is not created for texture '" << this->id() << "', but was requested !";

						return false;
					}
				}

				if ( !m_framebuffer->createOnHardware() )
				{
					TraceError{ClassId} << "Unable to create the framebuffer for texture '" << this->id() << "' !";

					return false;
				}

				return true;
			}

			std::shared_ptr< Vulkan::Image > m_colorImage;
			std::shared_ptr< Vulkan::ImageView > m_colorImageView;
			std::shared_ptr< Vulkan::ImageView > m_colorCubeImageView;
			std::shared_ptr< Vulkan::Image > m_depthStencilImage;
			std::shared_ptr< Vulkan::ImageView > m_depthImageView;
			std::shared_ptr< Vulkan::ImageView > m_stencilImageView;
			std::shared_ptr< Vulkan::Sampler > m_sampler;
			std::shared_ptr< Vulkan::Framebuffer > m_framebuffer;
			view_matrices_t m_viewMatrices;
			bool m_isReadyForRendering{false};
	};
}
