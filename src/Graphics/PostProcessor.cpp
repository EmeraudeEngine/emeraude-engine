/*
 * src/Graphics/PostProcessor.cpp
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
 * https://github.com/londnoir/emeraude-engine
 *
 * --- THIS IS AUTOMATICALLY GENERATED, DO NOT CHANGE ---
 */

#include "PostProcessor.hpp"

/* STL inclusions. */
#include <chrono>
#include <cmath>
#include <numbers>

/* Local inclusions. */
#include "Libs/VertexFactory/ShapeGenerator.hpp"
#include "Geometry/IndexedVertexResource.hpp"
#include "IndirectPostProcessEffect.hpp"
#include "PostProcessStack.hpp"
#include "Saphir/Generator/PostProcessing.hpp"
#include "Saphir/Program.hpp"
#include "Vulkan/CommandBuffer.hpp"
#include "Vulkan/DescriptorSet.hpp"
#include "Vulkan/DescriptorSetLayout.hpp"
#include "Vulkan/PipelineLayout.hpp"
#include "Vulkan/Sync/ImageMemoryBarrier.hpp"
#include "GrabPass.hpp"
#include "Renderer.hpp"
#include "Tracer.hpp"
#include "ViewMatricesInterface.hpp"

namespace
{
	/**
	 * @brief Lightweight adapter exposing a GrabPass's depth resources as a TextureInterface.
	 * @note Stack-allocated in executeIndirectPostProcessEffects(); lives for the duration of the chain.
	 */
	class GrabPassDepthAdapter final : public EmEn::Vulkan::TextureInterface
	{
		public:

			explicit
			GrabPassDepthAdapter (const EmEn::Graphics::GrabPass & grabPass) noexcept
				: m_grabPass{grabPass}
			{

			}

			[[nodiscard]]
			bool
			isCreated () const noexcept override
			{
				return m_grabPass.hasDepth();
			}

			[[nodiscard]]
			EmEn::Vulkan::TextureType
			type () const noexcept override
			{
				return EmEn::Vulkan::TextureType::Texture2D;
			}

			[[nodiscard]]
			uint32_t
			dimensions () const noexcept override
			{
				return 2;
			}

			[[nodiscard]]
			bool
			isCubemapTexture () const noexcept override
			{
				return false;
			}

			[[nodiscard]]
			std::shared_ptr< EmEn::Vulkan::Image >
			image () const noexcept override
			{
				return m_grabPass.depthImage();
			}

			[[nodiscard]]
			std::shared_ptr< EmEn::Vulkan::ImageView >
			imageView () const noexcept override
			{
				return m_grabPass.depthImageView();
			}

			[[nodiscard]]
			std::shared_ptr< EmEn::Vulkan::Sampler >
			sampler () const noexcept override
			{
				return m_grabPass.depthSampler();
			}

			[[nodiscard]]
			bool
			request3DTextureCoordinates () const noexcept override
			{
				return false;
			}

		private:

			const EmEn::Graphics::GrabPass & m_grabPass;
	};

	/**
	 * @brief Lightweight adapter exposing a GrabPass's normals resources as a TextureInterface.
	 * @note Stack-allocated in executeIndirectPostProcessEffects(); lives for the duration of the chain.
	 */
	class GrabPassNormalsAdapter final : public EmEn::Vulkan::TextureInterface
	{
		public:

			explicit
			GrabPassNormalsAdapter (const EmEn::Graphics::GrabPass & grabPass) noexcept
				: m_grabPass{grabPass}
			{

			}

			[[nodiscard]]
			bool
			isCreated () const noexcept override
			{
				return m_grabPass.hasNormals();
			}

			[[nodiscard]]
			EmEn::Vulkan::TextureType
			type () const noexcept override
			{
				return EmEn::Vulkan::TextureType::Texture2D;
			}

			[[nodiscard]]
			uint32_t
			dimensions () const noexcept override
			{
				return 2;
			}

			[[nodiscard]]
			bool
			isCubemapTexture () const noexcept override
			{
				return false;
			}

			[[nodiscard]]
			std::shared_ptr< EmEn::Vulkan::Image >
			image () const noexcept override
			{
				return m_grabPass.normalsImage();
			}

			[[nodiscard]]
			std::shared_ptr< EmEn::Vulkan::ImageView >
			imageView () const noexcept override
			{
				return m_grabPass.normalsImageView();
			}

			[[nodiscard]]
			std::shared_ptr< EmEn::Vulkan::Sampler >
			sampler () const noexcept override
			{
				return m_grabPass.normalsSampler();
			}

			[[nodiscard]]
			bool
			request3DTextureCoordinates () const noexcept override
			{
				return false;
			}

		private:

			const EmEn::Graphics::GrabPass & m_grabPass;
	};

	/**
	 * @brief Lightweight adapter exposing a GrabPass's material properties resources as a TextureInterface.
	 * @note Stack-allocated in executeIndirectPostProcessEffects(); lives for the duration of the chain.
	 */
	class GrabPassMaterialPropertiesAdapter final : public EmEn::Vulkan::TextureInterface
	{
		public:

			explicit
			GrabPassMaterialPropertiesAdapter (const EmEn::Graphics::GrabPass & grabPass) noexcept
				: m_grabPass{grabPass}
			{

			}

			[[nodiscard]]
			bool
			isCreated () const noexcept override
			{
				return m_grabPass.hasMaterialProperties();
			}

			[[nodiscard]]
			EmEn::Vulkan::TextureType
			type () const noexcept override
			{
				return EmEn::Vulkan::TextureType::Texture2D;
			}

			[[nodiscard]]
			uint32_t
			dimensions () const noexcept override
			{
				return 2;
			}

			[[nodiscard]]
			bool
			isCubemapTexture () const noexcept override
			{
				return false;
			}

			[[nodiscard]]
			std::shared_ptr< EmEn::Vulkan::Image >
			image () const noexcept override
			{
				return m_grabPass.materialPropertiesImage();
			}

			[[nodiscard]]
			std::shared_ptr< EmEn::Vulkan::ImageView >
			imageView () const noexcept override
			{
				return m_grabPass.materialPropertiesImageView();
			}

			[[nodiscard]]
			std::shared_ptr< EmEn::Vulkan::Sampler >
			sampler () const noexcept override
			{
				return m_grabPass.materialPropertiesSampler();
			}

			[[nodiscard]]
			bool
			request3DTextureCoordinates () const noexcept override
			{
				return false;
			}

		private:

			const EmEn::Graphics::GrabPass & m_grabPass;
	};
}

namespace EmEn::Graphics
{
	using namespace Libs::VertexFactory;

	/* Construction & lifecycle. */

	PostProcessor::PostProcessor (Renderer & renderer) noexcept
		: ServiceInterface{ClassId},
		m_renderer{renderer}
	{

	}

	bool
	PostProcessor::onInitialize () noexcept
	{
		/* Create the fullscreen quad geometry. */
		m_quadGeometry = std::make_shared< Geometry::IndexedVertexResource >("PostProcessQuad", Geometry::EnablePrimaryTextureCoordinates);

		if ( !m_quadGeometry->load(ShapeGenerator::generateQuad(2.0F, 2.0F)) )
		{
			TraceError{ClassId} << "Unable to generate the fullscreen quad geometry !";

			m_quadGeometry.reset();

			return false;
		}

		/* Upload vertex/index data to GPU memory. */
		if ( !m_quadGeometry->createOnHardware(m_renderer.transferManager()) )
		{
			TraceError{ClassId} << "Unable to upload the fullscreen quad geometry to GPU !";

			m_quadGeometry.reset();

			return false;
		}

		return true;
	}

	bool
	PostProcessor::onTerminate () noexcept
	{
		m_descriptorSets.clear();

		m_quadGeometry.reset();

		if ( m_grabPass != nullptr )
		{
			m_grabPass->destroy();
			m_grabPass.reset();
		}

		return true;
	}

	/* Configuration. */

	bool
	PostProcessor::configure (const std::shared_ptr< RenderTarget::Abstract > & renderTarget, bool requiresHDR, bool requiresDepth, bool requiresNormals, bool requiresMaterialProperties) noexcept
	{
		/* Cache the requirements for later use (recordBlit, recreateSceneTarget). */
		m_cachedRequiresHDR = requiresHDR;
		m_cachedRequiresDepth = requiresDepth;
		m_cachedRequiresNormals = requiresNormals;
		m_cachedRequiresMaterialProperties = requiresMaterialProperties;

		const auto & extent = renderTarget->extent();
		const auto swapChainColorFormat = m_renderer.swapChainColorFormat();

		if ( swapChainColorFormat == VK_FORMAT_UNDEFINED )
		{
			TraceError{ClassId} << "Unable to determine the swap chain color format !";

			return false;
		}

		/* When HDR is enabled, the grab pass uses a 16-bit float format for higher precision.
		 * Otherwise, it matches the swap chain format. */
		const auto grabPassColorFormat = m_cachedRequiresHDR ? VK_FORMAT_R16G16B16A16_SFLOAT : swapChainColorFormat;

		const auto depthFormat = m_renderer.swapChainDepthStencilFormat();

		/* Normals format: matches the scene render target's normals MRT attachment. */
		const auto normalsFormat = m_renderer.sceneTarget() != nullptr
			? m_renderer.sceneTarget()->normalsFormat()
			: VK_FORMAT_UNDEFINED;

		/* Material properties format: matches the scene render target's material properties MRT attachment. */
		const auto materialPropertiesFormat = m_renderer.sceneTarget() != nullptr
			? m_renderer.sceneTarget()->materialPropertiesFormat()
			: VK_FORMAT_UNDEFINED;

		/* Ensure any in-flight command buffers referencing the old grab pass have completed
		 * before destroying its resources. This only happens during (re)configuration,
		 * not per-frame — typically once at scene load or on window resize. */
		if ( m_grabPass != nullptr )
		{
			m_renderer.device()->waitIdle("PostProcessor::configure - grab pass reconfiguration");

			m_grabPass->destroy();
			m_grabPass.reset();
		}

		m_grabPass = std::make_unique< GrabPass >();

		if ( !m_grabPass->create(m_renderer, extent.width, extent.height, grabPassColorFormat, depthFormat, normalsFormat, materialPropertiesFormat) )
		{
			TraceError{ClassId} << "Unable to create the post-processor grab pass !";

			m_grabPass.reset();

			return false;
		}

		/* Create one descriptor set per frame-in-flight so that each frame can safely
		 * update its own descriptor without conflicting with pending command buffers. */
		{
			const auto descriptorSetLayout = getDescriptorSetLayout(m_renderer.layoutManager());

			if ( descriptorSetLayout == nullptr )
			{
				TraceError{ClassId} << "Unable to get the post-processing descriptor set layout !";

				return false;
			}

			const auto frameCount = m_renderer.framesInFlight();

			m_descriptorSets.clear();
			m_descriptorSets.reserve(frameCount);

			for ( uint32_t frameIndex = 0; frameIndex < frameCount; ++frameIndex )
			{
				auto descriptorSet = std::make_unique< Vulkan::DescriptorSet >(m_renderer.descriptorPool(), descriptorSetLayout);
				descriptorSet->setIdentifier(ClassId, "PostProcessorDescriptor-F" + std::to_string(frameIndex), "DescriptorSet");

				if ( !descriptorSet->create() )
				{
					TraceError{ClassId} << "Unable to create the post-processor descriptor set for frame #" << frameIndex << " !";

					m_descriptorSets.clear();

					return false;
				}

				if ( !descriptorSet->writeCombinedImageSampler(0, *m_grabPass->image(), *m_grabPass->imageView(), *m_grabPass->sampler()) )
				{
					TraceError{ClassId} << "Unable to write the grab pass color texture to the post-processor descriptor set !";

					m_descriptorSets.clear();

					return false;
				}

				if ( m_grabPass->hasDepth() )
				{
					if ( !descriptorSet->writeCombinedImageSampler(1, *m_grabPass->depthImage(), *m_grabPass->depthImageView(), *m_grabPass->depthSampler()) )
					{
						TraceError{ClassId} << "Unable to write the grab pass depth texture to the post-processor descriptor set !";

						m_descriptorSets.clear();

						return false;
					}
				}

				if ( m_grabPass->hasNormals() )
				{
					if ( !descriptorSet->writeCombinedImageSampler(2, *m_grabPass->normalsImage(), *m_grabPass->normalsImageView(), *m_grabPass->normalsSampler()) )
					{
						TraceError{ClassId} << "Unable to write the grab pass normals texture to the post-processor descriptor set !";

						m_descriptorSets.clear();

						return false;
					}
				}

				if ( m_grabPass->hasMaterialProperties() )
				{
					if ( !descriptorSet->writeCombinedImageSampler(3, *m_grabPass->materialPropertiesImage(), *m_grabPass->materialPropertiesImageView(), *m_grabPass->materialPropertiesSampler()) )
					{
						TraceError{ClassId} << "Unable to write the grab pass material properties texture to the post-processor descriptor set !";

						m_descriptorSets.clear();

						return false;
					}
				}

				m_descriptorSets.emplace_back(std::move(descriptorSet));
			}
		}

		return true;
	}

	/* GPU execution — multi-pass scene effects. */

	void
	PostProcessor::recordBlit (const Vulkan::CommandBuffer & commandBuffer) const noexcept
	{
		if ( m_grabPass == nullptr || !m_grabPass->isCreated() )
		{
			return;
		}

		const auto srcColorImage = m_renderer.currentSceneColorImage();

		if ( srcColorImage == nullptr )
		{
			return;
		}

		const auto dstImage = m_grabPass->image();

		/* 1. Transition scene color: current layout -> TRANSFER_SRC_OPTIMAL.
		 * When using the internal scene target, the image is in COLOR_ATTACHMENT_OPTIMAL.
		 * When rendering directly to swapchain, the image is in PRESENT_SRC_KHR after RP2 end. */
		{
			const auto srcLayout = m_renderer.sceneTarget() != nullptr
				? VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
				: VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

			const Vulkan::Sync::ImageMemoryBarrier barrier{
				*srcColorImage,
				VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
				VK_ACCESS_TRANSFER_READ_BIT,
				srcLayout,
				VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL
			};

			commandBuffer.pipelineBarrier(
				barrier,
				VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
				VK_PIPELINE_STAGE_TRANSFER_BIT
			);
		}

		/* 2. Transition grab pass image: SHADER_READ_ONLY_OPTIMAL -> TRANSFER_DST_OPTIMAL. */
		{
			const Vulkan::Sync::ImageMemoryBarrier barrier{
				*dstImage,
				VK_ACCESS_SHADER_READ_BIT,
				VK_ACCESS_TRANSFER_WRITE_BIT,
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
			};

			commandBuffer.pipelineBarrier(
				barrier,
				VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
				VK_PIPELINE_STAGE_TRANSFER_BIT
			);
		}

		/* 3. Transfer scene color -> grab pass.
		 * When HDR is enabled, formats may differ (e.g. R16G16B16A16_SFLOAT source),
		 * so vkCmdBlitImage is used for format conversion. Otherwise, exact pixel copy. */
		if ( m_cachedRequiresHDR )
		{
			commandBuffer.blitImage(
				*srcColorImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				*dstImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				VK_FILTER_LINEAR
			);
		}
		else
		{
			commandBuffer.copyImage(
				*srcColorImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				*dstImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				VK_IMAGE_ASPECT_COLOR_BIT
			);
		}

		/* 4. Transition grab pass image: TRANSFER_DST_OPTIMAL -> SHADER_READ_ONLY_OPTIMAL. */
		{
			const Vulkan::Sync::ImageMemoryBarrier barrier{
				*dstImage,
				VK_ACCESS_TRANSFER_WRITE_BIT,
				VK_ACCESS_SHADER_READ_BIT,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
			};

			commandBuffer.pipelineBarrier(
				barrier,
				VK_PIPELINE_STAGE_TRANSFER_BIT,
				VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
			);
		}

		/* 5. Transition scene color: TRANSFER_SRC_OPTIMAL -> COLOR_ATTACHMENT_OPTIMAL.
		 * Ready for RP2 restart or for the internal target to remain in attachment layout. */
		{
			const Vulkan::Sync::ImageMemoryBarrier barrier{
				*srcColorImage,
				VK_ACCESS_TRANSFER_READ_BIT,
				VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
				VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
			};

			commandBuffer.pipelineBarrier(
				barrier,
				VK_PIPELINE_STAGE_TRANSFER_BIT,
				VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
			);
		}

		/* === Depth copy === */
		const auto srcDepthImage = m_renderer.currentSceneDepthImage();

		if ( srcDepthImage != nullptr && m_grabPass->hasDepth() )
		{
			const auto dstDepthImage = m_grabPass->depthImage();

			/* 6. Transition scene depth: DEPTH_STENCIL_ATTACHMENT_OPTIMAL -> TRANSFER_SRC_OPTIMAL. */
			{
				const Vulkan::Sync::ImageMemoryBarrier barrier{
					*srcDepthImage,
					VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
					VK_ACCESS_TRANSFER_READ_BIT,
					VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
					VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
					VK_IMAGE_ASPECT_DEPTH_BIT
				};

				commandBuffer.pipelineBarrier(
					barrier,
					VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
					VK_PIPELINE_STAGE_TRANSFER_BIT
				);
			}

			/* 7. Transition grab pass depth: SHADER_READ_ONLY_OPTIMAL -> TRANSFER_DST_OPTIMAL. */
			{
				const Vulkan::Sync::ImageMemoryBarrier barrier{
					*dstDepthImage,
					VK_ACCESS_SHADER_READ_BIT,
					VK_ACCESS_TRANSFER_WRITE_BIT,
					VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
					VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
					VK_IMAGE_ASPECT_DEPTH_BIT
				};

				commandBuffer.pipelineBarrier(
					barrier,
					VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
					VK_PIPELINE_STAGE_TRANSFER_BIT
				);
			}

			/* 8. Copy scene depth -> grab pass depth. */
			commandBuffer.copyImage(
				*srcDepthImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				*dstDepthImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				VK_IMAGE_ASPECT_DEPTH_BIT
			);

			/* 9. Transition grab pass depth: TRANSFER_DST_OPTIMAL -> SHADER_READ_ONLY_OPTIMAL. */
			{
				const Vulkan::Sync::ImageMemoryBarrier barrier{
					*dstDepthImage,
					VK_ACCESS_TRANSFER_WRITE_BIT,
					VK_ACCESS_SHADER_READ_BIT,
					VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
					VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
					VK_IMAGE_ASPECT_DEPTH_BIT
				};

				commandBuffer.pipelineBarrier(
					barrier,
					VK_PIPELINE_STAGE_TRANSFER_BIT,
					VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
				);
			}

			/* 10. Transition scene depth: TRANSFER_SRC_OPTIMAL -> DEPTH_STENCIL_ATTACHMENT_OPTIMAL.
			 * Ready for RP2 restart or internal target attachment layout restoration. */
			{
				const Vulkan::Sync::ImageMemoryBarrier barrier{
					*srcDepthImage,
					VK_ACCESS_TRANSFER_READ_BIT,
					VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
					VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
					VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
					VK_IMAGE_ASPECT_DEPTH_BIT
				};

				commandBuffer.pipelineBarrier(
					barrier,
					VK_PIPELINE_STAGE_TRANSFER_BIT,
					VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT
				);
			}
		}

		/* === Normals copy === */
		const auto srcNormalsImage = m_renderer.currentSceneNormalsImage();

		if ( srcNormalsImage != nullptr && m_grabPass->hasNormals() )
		{
			const auto dstNormalsImage = m_grabPass->normalsImage();

			/* 11. Transition scene normals: COLOR_ATTACHMENT_OPTIMAL -> TRANSFER_SRC_OPTIMAL. */
			{
				const Vulkan::Sync::ImageMemoryBarrier barrier{
					*srcNormalsImage,
					VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
					VK_ACCESS_TRANSFER_READ_BIT,
					VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
					VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL
				};

				commandBuffer.pipelineBarrier(
					barrier,
					VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
					VK_PIPELINE_STAGE_TRANSFER_BIT
				);
			}

			/* 12. Transition grab pass normals: SHADER_READ_ONLY_OPTIMAL -> TRANSFER_DST_OPTIMAL. */
			{
				const Vulkan::Sync::ImageMemoryBarrier barrier{
					*dstNormalsImage,
					VK_ACCESS_SHADER_READ_BIT,
					VK_ACCESS_TRANSFER_WRITE_BIT,
					VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
					VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
				};

				commandBuffer.pipelineBarrier(
					barrier,
					VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
					VK_PIPELINE_STAGE_TRANSFER_BIT
				);
			}

			/* 13. Copy scene normals -> grab pass normals (same format). */
			commandBuffer.copyImage(
				*srcNormalsImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				*dstNormalsImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				VK_IMAGE_ASPECT_COLOR_BIT
			);

			/* 14. Transition grab pass normals: TRANSFER_DST_OPTIMAL -> SHADER_READ_ONLY_OPTIMAL. */
			{
				const Vulkan::Sync::ImageMemoryBarrier barrier{
					*dstNormalsImage,
					VK_ACCESS_TRANSFER_WRITE_BIT,
					VK_ACCESS_SHADER_READ_BIT,
					VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
					VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
				};

				commandBuffer.pipelineBarrier(
					barrier,
					VK_PIPELINE_STAGE_TRANSFER_BIT,
					VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
				);
			}

			/* 15. Transition scene normals: TRANSFER_SRC_OPTIMAL -> COLOR_ATTACHMENT_OPTIMAL.
			 * Ready for RP2 restart or internal target attachment layout restoration. */
			{
				const Vulkan::Sync::ImageMemoryBarrier barrier{
					*srcNormalsImage,
					VK_ACCESS_TRANSFER_READ_BIT,
					VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
					VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
					VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
				};

				commandBuffer.pipelineBarrier(
					barrier,
					VK_PIPELINE_STAGE_TRANSFER_BIT,
					VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
				);
			}
		}

		/* === Material properties copy === */
		const auto srcMaterialPropertiesImage = m_renderer.currentSceneMaterialPropertiesImage();

		if ( srcMaterialPropertiesImage != nullptr && m_grabPass->hasMaterialProperties() )
		{
			const auto dstMaterialPropertiesImage = m_grabPass->materialPropertiesImage();

			/* 16. Transition scene material properties: COLOR_ATTACHMENT_OPTIMAL -> TRANSFER_SRC_OPTIMAL. */
			{
				const Vulkan::Sync::ImageMemoryBarrier barrier{
					*srcMaterialPropertiesImage,
					VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
					VK_ACCESS_TRANSFER_READ_BIT,
					VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
					VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL
				};

				commandBuffer.pipelineBarrier(
					barrier,
					VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
					VK_PIPELINE_STAGE_TRANSFER_BIT
				);
			}

			/* 17. Transition grab pass material properties: SHADER_READ_ONLY_OPTIMAL -> TRANSFER_DST_OPTIMAL. */
			{
				const Vulkan::Sync::ImageMemoryBarrier barrier{
					*dstMaterialPropertiesImage,
					VK_ACCESS_SHADER_READ_BIT,
					VK_ACCESS_TRANSFER_WRITE_BIT,
					VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
					VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
				};

				commandBuffer.pipelineBarrier(
					barrier,
					VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
					VK_PIPELINE_STAGE_TRANSFER_BIT
				);
			}

			/* 18. Copy scene material properties -> grab pass material properties (same format). */
			commandBuffer.copyImage(
				*srcMaterialPropertiesImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				*dstMaterialPropertiesImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				VK_IMAGE_ASPECT_COLOR_BIT
			);

			/* 19. Transition grab pass material properties: TRANSFER_DST_OPTIMAL -> SHADER_READ_ONLY_OPTIMAL. */
			{
				const Vulkan::Sync::ImageMemoryBarrier barrier{
					*dstMaterialPropertiesImage,
					VK_ACCESS_TRANSFER_WRITE_BIT,
					VK_ACCESS_SHADER_READ_BIT,
					VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
					VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
				};

				commandBuffer.pipelineBarrier(
					barrier,
					VK_PIPELINE_STAGE_TRANSFER_BIT,
					VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
				);
			}

			/* 20. Transition scene material properties: TRANSFER_SRC_OPTIMAL -> COLOR_ATTACHMENT_OPTIMAL.
			 * Ready for RP2 restart or internal target attachment layout restoration. */
			{
				const Vulkan::Sync::ImageMemoryBarrier barrier{
					*srcMaterialPropertiesImage,
					VK_ACCESS_TRANSFER_READ_BIT,
					VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
					VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
					VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
				};

				commandBuffer.pipelineBarrier(
					barrier,
					VK_PIPELINE_STAGE_TRANSFER_BIT,
					VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
				);
			}
		}
	}

	void
	PostProcessor::executeIndirectPostProcessEffects (const Vulkan::CommandBuffer & commandBuffer, const PostProcessStack & stack) const noexcept
	{
		if ( !stack.hasEffects() || m_grabPass == nullptr || !m_grabPass->isCreated() )
		{
			return;
		}

		/* Build push constants for the effect chain. */
		static const auto startTime = std::chrono::steady_clock::now();
		const auto elapsedTime = std::chrono::duration< float >(std::chrono::steady_clock::now() - startTime).count();

		const auto mainRT = m_renderer.mainRenderTarget();
		const auto & extent = mainRT->extent();
		const auto fovDeg = mainRT->viewMatrices().fieldOfView();
		const auto tanHalfFovY = std::tan(fovDeg * std::numbers::pi_v< float > / 360.0F);

		const PushConstants pc{
			static_cast< float >(extent.width),
			static_cast< float >(extent.height),
			elapsedTime,
			m_nearPlane,
			m_farPlane,
			tanHalfFovY
		};

		/* Execute each enabled effect in the chain.
		 * Each effect receives the output of the previous one.
		 * NOTE: The GrabPass implements TextureInterface for its COLOR resources.
		 * For depth, we use a lightweight adapter that exposes the depth resources. */
		const Vulkan::TextureInterface * currentTexture = m_grabPass.get();

		GrabPassDepthAdapter depthAdapter{*m_grabPass};
		const Vulkan::TextureInterface * depthTexture = m_grabPass->hasDepth() ? &depthAdapter : nullptr;

		GrabPassNormalsAdapter normalsAdapter{*m_grabPass};
		const Vulkan::TextureInterface * normalsTexture = m_grabPass->hasNormals() ? &normalsAdapter : nullptr;

		GrabPassMaterialPropertiesAdapter materialPropertiesAdapter{*m_grabPass};
		const Vulkan::TextureInterface * materialPropertiesTexture = m_grabPass->hasMaterialProperties() ? &materialPropertiesAdapter : nullptr;

		for ( const auto & effect : stack.effects() )
		{
			if ( effect == nullptr || !effect->isEnabled() )
			{
				continue;
			}

			/* Skip depth-requiring effects if no depth is available. */
			if ( effect->requiresDepth() && depthTexture == nullptr )
			{
				continue;
			}

			/* Skip HDR-requiring effects if HDR is not enabled. */
			if ( effect->requiresHDR() && !m_cachedRequiresHDR )
			{
				continue;
			}

			/* Skip normals-requiring effects if no normals are available. */
			if ( effect->requiresNormals() && normalsTexture == nullptr )
			{
				continue;
			}

			/* Skip material-properties-requiring effects if no material properties are available. */
			if ( effect->requiresMaterialProperties() && materialPropertiesTexture == nullptr )
			{
				continue;
			}

			/* Skip ray tracing effects if RT is not available or disabled via settings. */
			if ( effect->requiresRayTracing() && (!m_renderer.device()->rayTracingEnabled() || !m_renderer.isRayTracingSettingEnabled()) )
			{
				continue;
			}

			currentTexture = &effect->execute(commandBuffer, *currentTexture, depthTexture, normalsTexture, materialPropertiesTexture, pc);
		}

		/* Update only the current frame's descriptor set to point to the effect chain output
		 * instead of the raw grab pass, so the single-pass render uses the processed texture.
		 * Each frame-in-flight has its own descriptor set, avoiding conflicts with pending frames. */
		const auto frameIndex = m_renderer.currentFrameIndex();
		auto & descriptorSet = m_descriptorSets[frameIndex];

		if ( currentTexture != m_grabPass.get() && currentTexture != nullptr )
		{
			static_cast< void >(descriptorSet->writeCombinedImageSampler(
				0,
				*currentTexture->image(),
				*currentTexture->imageView(),
				*currentTexture->sampler()
			));
		}
		else
		{
			/* No effects ran, restore descriptor to grab pass. */
			static_cast< void >(descriptorSet->writeCombinedImageSampler(
				0,
				*m_grabPass->image(),
				*m_grabPass->imageView(),
				*m_grabPass->sampler()
			));
		}
	}

	/* GPU execution — single-pass camera lens effects. */

	void
	PostProcessor::executeDirectPostProcessEffects (const Vulkan::CommandBuffer & commandBuffer, const std::vector< std::shared_ptr< DirectPostProcessEffect > > & lensEffects) const noexcept
	{
		/* Generate or retrieve the shader program via the PostProcessing generator.
		 * The Renderer::m_programs cache (hash map) handles deduplication automatically.
		 * Same camera + same effects = cache hit O(1). */
		const auto renderTarget = m_renderer.mainRenderTarget();

		Saphir::Generator::PostProcessing generator{renderTarget, m_quadGeometry};

		/* Pass the current effects list to the generator. */
		generator.setEffectsList(lensEffects);

		/* Use the post-process framebuffer (single-sample). */
		if ( const auto * overlayFB = m_renderer.overlayFramebuffer(); overlayFB != nullptr )
		{
			generator.setPipelineFramebuffer(overlayFB);
		}

		if ( !generator.generateShaderProgram(m_renderer) )
		{
			TraceError{ClassId} << "Unable to generate the post-processing shader program !";

			return;
		}

		const auto program = generator.shaderProgram();

		if ( program == nullptr )
		{
			return;
		}

		/* Bind the graphics pipeline. */
		commandBuffer.bind(*program->graphicsPipeline());

		/* Set dynamic viewport and scissor based on current render target extent. */
		{
			const auto & extent = renderTarget->extent();

			const VkViewport viewport{
				.x = 0.0F,
				.y = 0.0F,
				.width = static_cast< float >(extent.width),
				.height = static_cast< float >(extent.height),
				.minDepth = 0.0F,
				.maxDepth = 1.0F
			};
			vkCmdSetViewport(commandBuffer.handle(), 0, 1, &viewport);

			const VkRect2D scissor{
				.offset = {0, 0},
				.extent = {extent.width, extent.height}
			};
			vkCmdSetScissor(commandBuffer.handle(), 0, 1, &scissor);

			/* Push constants. */
			static const auto startTime = std::chrono::steady_clock::now();
			const auto elapsedTime = std::chrono::duration< float >(std::chrono::steady_clock::now() - startTime).count();
			const auto fovDeg = renderTarget->viewMatrices().fieldOfView();
			const auto tanHalfFovY = std::tan(fovDeg * std::numbers::pi_v< float > / 360.0F);

			const PushConstants pc{
				static_cast< float >(extent.width),
				static_cast< float >(extent.height),
				elapsedTime,
				m_nearPlane,
				m_farPlane,
				tanHalfFovY
			};

			vkCmdPushConstants(
				commandBuffer.handle(),
				program->pipelineLayout()->handle(),
				VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
				0,
				sizeof(PushConstants),
				&pc
			);
		}

		/* Bind the current frame's GrabPass descriptor set. */
		commandBuffer.bind(
			*m_descriptorSets[m_renderer.currentFrameIndex()],
			*program->pipelineLayout(),
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			0
		);

		/* Bind and draw the fullscreen quad. */
		commandBuffer.bind(*m_quadGeometry, 0);
		commandBuffer.draw(*m_quadGeometry, 0, 1);
	}

	/* Static. */

	std::shared_ptr< Vulkan::DescriptorSetLayout >
	PostProcessor::getDescriptorSetLayout (Vulkan::LayoutManager & layoutManager) noexcept
	{
		auto descriptorSetLayout = layoutManager.getDescriptorSetLayout(ClassId);

		if ( descriptorSetLayout == nullptr )
		{
			descriptorSetLayout = layoutManager.prepareNewDescriptorSetLayout(ClassId);
			descriptorSetLayout->setIdentifier(ClassId, ClassId, "DescriptorSetLayout");

			descriptorSetLayout->declareCombinedImageSampler(0, VK_SHADER_STAGE_FRAGMENT_BIT);
			descriptorSetLayout->declareCombinedImageSampler(1, VK_SHADER_STAGE_FRAGMENT_BIT);
			descriptorSetLayout->declareCombinedImageSampler(2, VK_SHADER_STAGE_FRAGMENT_BIT);
			descriptorSetLayout->declareCombinedImageSampler(3, VK_SHADER_STAGE_FRAGMENT_BIT);

			if ( !layoutManager.createDescriptorSetLayout(descriptorSetLayout) )
			{
				return nullptr;
			}
		}

		return descriptorSetLayout;
	}
}
