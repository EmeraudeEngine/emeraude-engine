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
 * https://github.com/EmeraudeEngine/emeraude-engine
 *
 * --- THIS IS AUTOMATICALLY GENERATED, DO NOT CHANGE ---
 */

#include "PostProcessor.hpp"

/* STL inclusions. */
#include <algorithm>
#include <chrono>
#include <cmath>
#include <numbers>

/* Local inclusions. */
#include "Geometry/IndexedVertexResource.hpp"
#include "GrabPass.hpp"
#include "IndirectPostProcessEffect.hpp"
#include "VertexFactory/ShapeGenerator.hpp"
#include "PostProcessStack.hpp"
#include "Renderer.hpp"
#include "Resources/Manager.hpp"
#include "Saphir/Generator/PostProcessing.hpp"
#include "SceneRenderTarget.hpp"
#include "Scenes/LightSet.hpp"
#include "Tracer.hpp"
#include "ViewMatricesInterface.hpp"
#include "Vulkan/CommandBuffer.hpp"
#include "Vulkan/DescriptorSet.hpp"
#include "Vulkan/DescriptorSetLayout.hpp"
#include "Vulkan/PipelineLayout.hpp"
#include "Vulkan/Sync/ImageMemoryBarrier.hpp"

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

	/**
	 * @brief Adapter exposing the grab pass albedo texture as a TextureInterface.
	 * @note Stack-allocated in executeIndirectPostProcessEffects(); lives for the duration of the chain.
	 */
	class GrabPassAlbedoAdapter final : public EmEn::Vulkan::TextureInterface
	{
		public:

			explicit
			GrabPassAlbedoAdapter (const EmEn::Graphics::GrabPass & grabPass) noexcept
				: m_grabPass{grabPass}
			{

			}

			[[nodiscard]]
			bool
			isCreated () const noexcept override
			{
				return m_grabPass.hasAlbedo();
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
				return m_grabPass.albedoImage();
			}

			[[nodiscard]]
			std::shared_ptr< EmEn::Vulkan::ImageView >
			imageView () const noexcept override
			{
				return m_grabPass.albedoImageView();
			}

			[[nodiscard]]
			std::shared_ptr< EmEn::Vulkan::Sampler >
			sampler () const noexcept override
			{
				return m_grabPass.albedoSampler();
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
	 * @brief Adapter exposing the grab pass velocity texture as a TextureInterface.
	 * @note Stack-allocated in executeIndirectPostProcessEffects(); lives for the duration of the chain.
	 */
	class GrabPassVelocityAdapter final : public EmEn::Vulkan::TextureInterface
	{
		public:

			explicit
			GrabPassVelocityAdapter (const EmEn::Graphics::GrabPass & grabPass) noexcept
				: m_grabPass{grabPass}
			{

			}

			[[nodiscard]]
			bool
			isCreated () const noexcept override
			{
				return m_grabPass.hasVelocity();
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
				return m_grabPass.velocityImage();
			}

			[[nodiscard]]
			std::shared_ptr< EmEn::Vulkan::ImageView >
			imageView () const noexcept override
			{
				return m_grabPass.velocityImageView();
			}

			[[nodiscard]]
			std::shared_ptr< EmEn::Vulkan::Sampler >
			sampler () const noexcept override
			{
				return m_grabPass.velocitySampler();
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
	using namespace Base::VertexFactory;

	/* Construction & lifecycle. */

	PostProcessor::PostProcessor (PrimaryServices & primaryServices, Resources::Manager & resourcesManager) noexcept
		: ServiceInterface{ClassId},
		m_primaryServices{primaryServices},
		m_resourcesManager{resourcesManager},
		m_renderer{resourcesManager.graphicsRenderer()}
	{

	}

	/* NOTE: Out-of-line so the std::unique_ptr< GrabPass > deleter sees the complete type
	 * (included above). Cannot be implicit in the EMEN_API-exported header — see the destructor
	 * declaration in PostProcessor.hpp. */
	PostProcessor::~PostProcessor () = default;

	bool
	PostProcessor::onInitialize () noexcept
	{
		/* Create the fullscreen quad geometry. */
		m_quadGeometry = std::make_shared< Geometry::IndexedVertexResource >(m_resourcesManager, "PostProcessQuad", Geometry::EnablePrimaryTextureCoordinates);

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
	PostProcessor::configure (const std::shared_ptr< RenderTarget::Abstract > & renderTarget, bool requiresHDR, bool requiresDepth, bool requiresNormals, bool requiresMaterialProperties, bool requiresAlbedo, bool requiresVelocity) noexcept
	{
		/* Cache the requirements for later use (recordBlit, recreateSceneTarget). */
		m_cachedRequiresHDR = requiresHDR;
		m_cachedRequiresDepth = requiresDepth;
		m_cachedRequiresNormals = requiresNormals;
		m_cachedRequiresMaterialProperties = requiresMaterialProperties;
		m_cachedRequiresAlbedo = requiresAlbedo;
		m_cachedRequiresVelocity = requiresVelocity;

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

		/* Albedo format: matches the scene render target's albedo MRT attachment. */
		const auto albedoFormat = m_renderer.sceneTarget() != nullptr
			? m_renderer.sceneTarget()->albedoFormat()
			: VK_FORMAT_UNDEFINED;

		/* Velocity format: matches the scene render target's velocity MRT attachment. */
		const auto velocityFormat = m_renderer.sceneTarget() != nullptr
			? m_renderer.sceneTarget()->velocityFormat()
			: VK_FORMAT_UNDEFINED;

		/* Retire the previous grab pass: in-flight command buffers may still reference
		 * its images; the deferred destructor destroys it once every frame in flight
		 * has completed — no device stall, no use-after-free. */
		if ( m_grabPass != nullptr )
		{
			m_renderer.deferredDestructor().retireObject(std::move(m_grabPass));
		}

		m_grabPass = std::make_unique< GrabPass >();

		if ( !m_grabPass->create(m_renderer, extent.width, extent.height, grabPassColorFormat, depthFormat, normalsFormat, materialPropertiesFormat, albedoFormat, velocityFormat) )
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

			/* Retire the previous per-frame descriptor sets for the same reason. */
			for ( auto & descriptorSet : m_descriptorSets )
			{
				m_renderer.deferredDestructor().retireObject(std::move(descriptorSet));
			}

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

		const auto dstColorImage = m_grabPass->image();

		/* Scene color source layout: the internal scene target image stays in
		 * COLOR_ATTACHMENT_OPTIMAL; the swap-chain image is in PRESENT_SRC_KHR after RP2 end. */
		const auto srcColorLayout = m_renderer.sceneTarget() != nullptr
			? VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
			: VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

		/* Optional G-buffer copies enabled this frame. */
		const auto srcDepthImage = m_renderer.currentSceneDepthImage();
		const auto srcNormalsImage = m_renderer.currentSceneNormalsImage();
		const auto srcMaterialPropertiesImage = m_renderer.currentSceneMaterialPropertiesImage();
		const auto srcAlbedoImage = m_renderer.currentSceneAlbedoImage();
		const auto srcVelocityImage = m_renderer.currentSceneVelocityImage();

		const bool copyDepth = srcDepthImage != nullptr && m_grabPass->hasDepth();
		const bool copyNormals = srcNormalsImage != nullptr && m_grabPass->hasNormals();
		const bool copyMaterialProperties = srcMaterialPropertiesImage != nullptr && m_grabPass->hasMaterialProperties();
		const bool copyAlbedo = srcAlbedoImage != nullptr && m_grabPass->hasAlbedo();
		const bool copyVelocity = srcVelocityImage != nullptr && m_grabPass->hasVelocity();

		/* The whole G-buffer grab is expressed as TWO batched barriers around the copies
		 * instead of one pipelineBarrier() per transition (which serialized the GPU up to
		 * ~30 times): every source goes to TRANSFER_SRC and every destination to TRANSFER_DST
		 * in a single call, then the copies run back-to-back, then a single call restores
		 * everything. The stage masks are the union of the per-image stages — per-image
		 * precision is preserved by the access masks carried by each VkImageMemoryBarrier. */
		std::vector< VkImageMemoryBarrier > barriers;
		barriers.reserve(12);

		/* === Pre-copy batch: sources -> TRANSFER_SRC, destinations -> TRANSFER_DST. === */

		barriers.push_back(Vulkan::Sync::ImageMemoryBarrier{
			*srcColorImage,
			VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
			VK_ACCESS_TRANSFER_READ_BIT,
			srcColorLayout,
			VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL
		}.get());

		barriers.push_back(Vulkan::Sync::ImageMemoryBarrier{
			*dstColorImage,
			VK_ACCESS_SHADER_READ_BIT,
			VK_ACCESS_TRANSFER_WRITE_BIT,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
		}.get());

		if ( copyDepth )
		{
			barriers.push_back(Vulkan::Sync::ImageMemoryBarrier{
				*srcDepthImage,
				VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
				VK_ACCESS_TRANSFER_READ_BIT,
				VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
				VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				VK_IMAGE_ASPECT_DEPTH_BIT
			}.get());

			barriers.push_back(Vulkan::Sync::ImageMemoryBarrier{
				*m_grabPass->depthImage(),
				VK_ACCESS_SHADER_READ_BIT,
				VK_ACCESS_TRANSFER_WRITE_BIT,
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				VK_IMAGE_ASPECT_DEPTH_BIT
			}.get());
		}

		if ( copyNormals )
		{
			barriers.push_back(Vulkan::Sync::ImageMemoryBarrier{
				*srcNormalsImage,
				VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
				VK_ACCESS_TRANSFER_READ_BIT,
				VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
				VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL
			}.get());

			barriers.push_back(Vulkan::Sync::ImageMemoryBarrier{
				*m_grabPass->normalsImage(),
				VK_ACCESS_SHADER_READ_BIT,
				VK_ACCESS_TRANSFER_WRITE_BIT,
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
			}.get());
		}

		if ( copyMaterialProperties )
		{
			barriers.push_back(Vulkan::Sync::ImageMemoryBarrier{
				*srcMaterialPropertiesImage,
				VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
				VK_ACCESS_TRANSFER_READ_BIT,
				VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
				VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL
			}.get());

			barriers.push_back(Vulkan::Sync::ImageMemoryBarrier{
				*m_grabPass->materialPropertiesImage(),
				VK_ACCESS_SHADER_READ_BIT,
				VK_ACCESS_TRANSFER_WRITE_BIT,
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
			}.get());
		}

		if ( copyAlbedo )
		{
			barriers.push_back(Vulkan::Sync::ImageMemoryBarrier{
				*srcAlbedoImage,
				VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
				VK_ACCESS_TRANSFER_READ_BIT,
				VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
				VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL
			}.get());

			barriers.push_back(Vulkan::Sync::ImageMemoryBarrier{
				*m_grabPass->albedoImage(),
				VK_ACCESS_SHADER_READ_BIT,
				VK_ACCESS_TRANSFER_WRITE_BIT,
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
			}.get());
		}

		if ( copyVelocity )
		{
			barriers.push_back(Vulkan::Sync::ImageMemoryBarrier{
				*srcVelocityImage,
				VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
				VK_ACCESS_TRANSFER_READ_BIT,
				VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
				VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL
			}.get());

			barriers.push_back(Vulkan::Sync::ImageMemoryBarrier{
				*m_grabPass->velocityImage(),
				VK_ACCESS_SHADER_READ_BIT,
				VK_ACCESS_TRANSFER_WRITE_BIT,
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
			}.get());
		}

		{
			/* Destinations were last sampled by fragment shaders; color sources were last
			 * written as color attachments; the depth source by the late fragment tests. */
			VkPipelineStageFlags preSrcStages = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;

			if ( copyDepth )
			{
				preSrcStages |= VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
			}

			commandBuffer.pipelineBarrier(barriers, preSrcStages, VK_PIPELINE_STAGE_TRANSFER_BIT);
		}

		/* === Copies, back-to-back (no barrier needed between independent transfers). === */

		/* When HDR is enabled, formats may differ (e.g. R16G16B16A16_SFLOAT source),
		 * so vkCmdBlitImage is used for format conversion. Otherwise, exact pixel copy. */
		if ( m_cachedRequiresHDR )
		{
			commandBuffer.blitImage(
				*srcColorImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				*dstColorImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				VK_FILTER_LINEAR
			);
		}
		else
		{
			commandBuffer.copyImage(
				*srcColorImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				*dstColorImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				VK_IMAGE_ASPECT_COLOR_BIT
			);
		}

		if ( copyDepth )
		{
			commandBuffer.copyImage(
				*srcDepthImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				*m_grabPass->depthImage(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				VK_IMAGE_ASPECT_DEPTH_BIT
			);
		}

		if ( copyNormals )
		{
			commandBuffer.copyImage(
				*srcNormalsImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				*m_grabPass->normalsImage(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				VK_IMAGE_ASPECT_COLOR_BIT
			);
		}

		if ( copyMaterialProperties )
		{
			commandBuffer.copyImage(
				*srcMaterialPropertiesImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				*m_grabPass->materialPropertiesImage(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				VK_IMAGE_ASPECT_COLOR_BIT
			);
		}

		if ( copyAlbedo )
		{
			commandBuffer.copyImage(
				*srcAlbedoImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				*m_grabPass->albedoImage(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				VK_IMAGE_ASPECT_COLOR_BIT
			);
		}

		if ( copyVelocity )
		{
			commandBuffer.copyImage(
				*srcVelocityImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				*m_grabPass->velocityImage(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				VK_IMAGE_ASPECT_COLOR_BIT
			);
		}

		/* === Post-copy batch: destinations -> SHADER_READ, sources restored for RP2
		 * restart or for the internal target to remain in attachment layout. === */

		barriers.clear();

		barriers.push_back(Vulkan::Sync::ImageMemoryBarrier{
			*dstColorImage,
			VK_ACCESS_TRANSFER_WRITE_BIT,
			VK_ACCESS_SHADER_READ_BIT,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
		}.get());

		barriers.push_back(Vulkan::Sync::ImageMemoryBarrier{
			*srcColorImage,
			VK_ACCESS_TRANSFER_READ_BIT,
			VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
			VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
		}.get());

		if ( copyDepth )
		{
			barriers.push_back(Vulkan::Sync::ImageMemoryBarrier{
				*m_grabPass->depthImage(),
				VK_ACCESS_TRANSFER_WRITE_BIT,
				VK_ACCESS_SHADER_READ_BIT,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
				VK_IMAGE_ASPECT_DEPTH_BIT
			}.get());

			barriers.push_back(Vulkan::Sync::ImageMemoryBarrier{
				*srcDepthImage,
				VK_ACCESS_TRANSFER_READ_BIT,
				VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
				VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
				VK_IMAGE_ASPECT_DEPTH_BIT
			}.get());
		}

		if ( copyNormals )
		{
			barriers.push_back(Vulkan::Sync::ImageMemoryBarrier{
				*m_grabPass->normalsImage(),
				VK_ACCESS_TRANSFER_WRITE_BIT,
				VK_ACCESS_SHADER_READ_BIT,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
			}.get());

			barriers.push_back(Vulkan::Sync::ImageMemoryBarrier{
				*srcNormalsImage,
				VK_ACCESS_TRANSFER_READ_BIT,
				VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
				VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
			}.get());
		}

		if ( copyMaterialProperties )
		{
			barriers.push_back(Vulkan::Sync::ImageMemoryBarrier{
				*m_grabPass->materialPropertiesImage(),
				VK_ACCESS_TRANSFER_WRITE_BIT,
				VK_ACCESS_SHADER_READ_BIT,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
			}.get());

			barriers.push_back(Vulkan::Sync::ImageMemoryBarrier{
				*srcMaterialPropertiesImage,
				VK_ACCESS_TRANSFER_READ_BIT,
				VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
				VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
			}.get());
		}

		if ( copyAlbedo )
		{
			barriers.push_back(Vulkan::Sync::ImageMemoryBarrier{
				*m_grabPass->albedoImage(),
				VK_ACCESS_TRANSFER_WRITE_BIT,
				VK_ACCESS_SHADER_READ_BIT,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
			}.get());

			barriers.push_back(Vulkan::Sync::ImageMemoryBarrier{
				*srcAlbedoImage,
				VK_ACCESS_TRANSFER_READ_BIT,
				VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
				VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
			}.get());
		}

		if ( copyVelocity )
		{
			barriers.push_back(Vulkan::Sync::ImageMemoryBarrier{
				*m_grabPass->velocityImage(),
				VK_ACCESS_TRANSFER_WRITE_BIT,
				VK_ACCESS_SHADER_READ_BIT,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
			}.get());

			barriers.push_back(Vulkan::Sync::ImageMemoryBarrier{
				*srcVelocityImage,
				VK_ACCESS_TRANSFER_READ_BIT,
				VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
				VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
			}.get());
		}

		{
			/* Destinations are sampled by fragment shaders; color sources resume as color
			 * attachments; the depth source resumes at the early fragment tests. */
			VkPipelineStageFlags postDstStages = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

			if ( copyDepth )
			{
				postDstStages |= VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
			}

			commandBuffer.pipelineBarrier(barriers, VK_PIPELINE_STAGE_TRANSFER_BIT, postDstStages);
		}
	}

	bool
	PostProcessor::executeIndirectPostProcessEffects (const Vulkan::CommandBuffer & commandBuffer, const PostProcessStack & stack, const Scenes::LightSet * lightSet, const Scenes::Component::Camera * activeCamera, float skyLuminance) const noexcept
	{
		if ( !stack.hasEffects() || m_grabPass == nullptr || !m_grabPass->isCreated() )
		{
			return false;
		}

		/* Build push constants for the effect chain. */
		static const auto startTime = std::chrono::steady_clock::now();
		const auto now = std::chrono::steady_clock::now();
		const auto elapsedTime = std::chrono::duration< float >(now - startTime).count();

		/* Duration of the previous RENDERED frame: the chain's single source of truth for
		 * anything converting the per-frame velocity G-buffer into a physical duration (the
		 * motion blur shutter angle). Clamped so the first frame and any hitch (a stall, a
		 * breakpoint, a scene load) cannot produce an absurd exposure. */
		constexpr float MinDeltaTime{1.0F / 1000.0F};
		constexpr float MaxDeltaTime{1.0F / 15.0F};

		const auto deltaTime = m_lastChainFrameTime.time_since_epoch().count() == 0 ?
			MinDeltaTime :
			std::clamp(std::chrono::duration< float >(now - m_lastChainFrameTime).count(), MinDeltaTime, MaxDeltaTime);

		m_lastChainFrameTime = now;

		const auto mainRT = m_renderer.mainRenderTarget();
		const auto & extent = mainRT->extent();
		const auto fovDeg = mainRT->viewMatrices().fieldOfView();
		const auto tanHalfFovY = std::tan(fovDeg * std::numbers::pi_v< float > / 360.0F);

		/* Execute each enabled effect in the chain.
		 * Each effect receives the output of the previous one.
		 * NOTE: The GrabPass implements TextureInterface for its COLOR resources.
		 * For depth, we use a lightweight adapter that exposes the depth resources. */
		const Vulkan::TextureInterface * currentTexture = m_grabPass.get();

		const GrabPassDepthAdapter depthAdapter{*m_grabPass};
		const Vulkan::TextureInterface * depthTexture = m_grabPass->hasDepth() ? &depthAdapter : nullptr;

		const GrabPassNormalsAdapter normalsAdapter{*m_grabPass};
		const Vulkan::TextureInterface * normalsTexture = m_grabPass->hasNormals() ? &normalsAdapter : nullptr;

		const GrabPassMaterialPropertiesAdapter materialPropertiesAdapter{*m_grabPass};
		const Vulkan::TextureInterface * materialPropertiesTexture = m_grabPass->hasMaterialProperties() ? &materialPropertiesAdapter : nullptr;

		const GrabPassAlbedoAdapter albedoAdapter{*m_grabPass};
		const Vulkan::TextureInterface * albedoTexture = m_grabPass->hasAlbedo() ? &albedoAdapter : nullptr;

		const GrabPassVelocityAdapter velocityAdapter{*m_grabPass};
		const Vulkan::TextureInterface * velocityTexture = m_grabPass->hasVelocity() ? &velocityAdapter : nullptr;

		/* Per-frame chain context: G-buffers, scene lighting, the active camera (single
		 * source of truth for the photographic options) and the frame push constants. */
		const IndirectPostProcessEffect::FrameContext context{
			.depth = depthTexture,
			.normals = normalsTexture,
			.materialProperties = materialPropertiesTexture,
			.albedo = albedoTexture,
			.velocity = velocityTexture,
			.lightSet = lightSet,
			.camera = activeCamera,
			.skyLuminance = skyLuminance,
			.projectionJitter = mainRT->viewMatrices().projectionJitter(),
			.constants = PushConstants{
				.frameWidth = static_cast< float >(extent.width),
				.frameHeight = static_cast< float >(extent.height),
				.time = elapsedTime,
				.nearPlane = m_nearPlane,
				.farPlane = m_farPlane,
				.tanHalfFovY = tanHalfFovY,
				.deltaTime = deltaTime
			}
		};

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

			/* Skip albedo-requiring effects if no albedo is available. */
			if ( effect->requiresAlbedo() && albedoTexture == nullptr )
			{
				continue;
			}

			/* Skip velocity-requiring effects if no velocity is available. */
			if ( effect->requiresVelocity() && velocityTexture == nullptr )
			{
				continue;
			}

			/* Skip ray tracing effects if RT is not available, disabled via settings, or the
			 * TLAS is not consumable yet (async build during the first frames, scene without
			 * RT geometry): drawing would use an unbound/never-written TLAS descriptor. */
			if ( effect->requiresRayTracing() && (!m_renderer.device()->rayTracingEnabled() || !m_renderer.isRayTracingSettingEnabled() || !m_renderer.isRayTracingReady()) )
			{
				continue;
			}

			/* Skip light-dependent effects if no main directional light is available. */
			if ( effect->requiresLightSet() && (lightSet == nullptr || lightSet->mainDirectionalLight() == nullptr) )
			{
				continue;
			}

			currentTexture = &effect->execute(commandBuffer, *currentTexture, context);
		}

		/* Update only the current frame's descriptor set to point to the effect chain output
		 * instead of the raw grab pass, so the single-pass render uses the processed texture.
		 * Each frame-in-flight has its own descriptor set, avoiding conflicts with pending frames. */
		const auto frameIndex = m_renderer.currentFrameIndex();
		const auto & descriptorSet = m_descriptorSets[frameIndex];

		if ( currentTexture != m_grabPass.get() && currentTexture != nullptr )
		{
			return descriptorSet->writeCombinedImageSampler(
				0,
				*currentTexture->image(),
				*currentTexture->imageView(),
				*currentTexture->sampler()
			);
		}

		/* No effects ran, restore descriptor to grab pass. */
		return descriptorSet->writeCombinedImageSampler(
			0,
			*m_grabPass->image(),
			*m_grabPass->imageView(),
			*m_grabPass->sampler()
		);
	}

	/* GPU execution — single-pass camera lens effects. */

	bool
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

			return false;
		}

		const auto program = generator.shaderProgram();

		if ( program == nullptr )
		{
			return false;
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
				.offset = {
					.x = 0,
					.y = 0
				},
				.extent = {
					.width = extent.width,
					.height = extent.height
				}
			};
			vkCmdSetScissor(commandBuffer.handle(), 0, 1, &scissor);

			/* Push constants. */
			static const auto startTime = std::chrono::steady_clock::now();
			const auto elapsedTime = std::chrono::duration< float >(std::chrono::steady_clock::now() - startTime).count();
			const auto fovDeg = renderTarget->viewMatrices().fieldOfView();
			const auto tanHalfFovY = std::tan(fovDeg * std::numbers::pi_v< float > / 360.0F);

			const PushConstants pc{
				.frameWidth = static_cast< float >(extent.width),
				.frameHeight = static_cast< float >(extent.height),
				.time = elapsedTime,
				.nearPlane = m_nearPlane,
				.farPlane = m_farPlane,
				.tanHalfFovY = tanHalfFovY,
				/* Direct (lens) chain: no temporal consumer, no frame delta needed. */
				.deltaTime = 0.0F
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

		return true;
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
