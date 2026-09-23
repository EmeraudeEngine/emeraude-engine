/*
 * src/Graphics/RenderTarget/ImposterBake.hpp
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

#pragma once

/* STL inclusions. */
#include <atomic>
#include <cstdint>
#include <deque>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

/* Local inclusions for inheritances. */
#include "Graphics/RenderTarget/Abstract.hpp"

/* Local inclusions for usages. */
#include "Graphics/ViewMatrices2DUBO.hpp"
#include "Math/CartesianFrame.hpp"

/* Forward declarations. */
namespace EmEn
{
	namespace Vulkan
	{
		class Framebuffer;
		class Image;
		class ImageView;
	}

	namespace Graphics
	{
		class ImposterAtlas;
	}
}

namespace EmEn::Graphics::RenderTarget
{
	/**
	 * @brief The offscreen G-buffer an octahedral imposter is baked into: colour, normals, material properties and
	 * albedo — the scene pass's own MRT order — plus depth, under one ORTHOGRAPHIC camera.
	 * @note The scene generator emits its G-buffer outputs by COUNTING the colour attachments of the target's render
	 * pass (Saphir `SceneRendering`), so the subject is baked by the very program chain the near mesh is drawn with:
	 * alpha test, normal mapping, wind, specular antialiasing — no second implementation of any of it. Only the
	 * normals and the albedo are kept; the colour and the material properties exist because the MRT order is fixed.
	 * @note One target serves every bake of a scene, one JOB per rendered frame (owner decision 2026-09-23: the
	 * variants over consecutive frames): enqueue() hands it a subject and the atlas it fills. A job waits a couple
	 * of renders of its subject (the entity's published state must have caught up) and is then copied into its
	 * atlas in the same submission (ImposterAtlas::recordBake()).
	 * @note Registered in the scene's render-to-texture list WITHOUT the AV console: no camera is connected (the view
	 * is set by configureCamera()), and a console registration would walk every renderable instance of the scene to
	 * prepare it for this render pass (Scene::initializeRenderTarget()) — the subject alone is prepared, lazily.
	 * @extends EmEn::Graphics::RenderTarget::Abstract This is a render target.
	 */
	class EMEN_API ImposterBake final : public Abstract
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"ImposterBake"};

			/** @brief Renders of a job's subject before its copy: the entity's published state catches up meanwhile. */
			static constexpr uint32_t WarmUpRenders{2};

			/**
			 * @brief Constructs an imposter bake target.
			 * @param name The name of the target.
			 * @param size The pixel size of the square target, the atlas size.
			 */
			ImposterBake (const std::string & name, uint32_t size) noexcept;

			/**
			 * @brief Destructs the imposter bake target.
			 */
			~ImposterBake () override;

			ImposterBake (const ImposterBake & copy) noexcept = delete;
			ImposterBake (ImposterBake && copy) noexcept = delete;
			ImposterBake & operator= (const ImposterBake & copy) noexcept = delete;
			ImposterBake & operator= (ImposterBake && copy) noexcept = delete;

			/**
			 * @brief LOGIC THREAD. Places the orthographic camera. Call it before the first job.
			 * @param frame The camera frame (it looks along its -Z).
			 * @param boxSide The side of the square the camera sees, and its depth range, in metres.
			 * @return void
			 */
			void configureCamera (const Base::Math::CartesianFrame< float > & frame, float boxSide) noexcept;

			/**
			 * @brief Queues a bake. Thread-safe.
			 * @param subject The renderable instance to bake (its address: RenderTarget::Abstract::setBakeSubject()).
			 * @param atlas The atlas it fills; it must be created and of this target's size.
			 * @return bool False when the atlas does not fit the target.
			 */
			[[nodiscard]]
			bool enqueue (const void * subject, const std::shared_ptr< ImposterAtlas > & atlas) noexcept;

			/**
			 * @brief Returns the atlases baked by this target, still alive. Thread-safe.
			 * @return std::vector< std::shared_ptr< ImposterAtlas > >
			 */
			[[nodiscard]]
			std::vector< std::shared_ptr< ImposterAtlas > > bakedAtlases () const noexcept;

			/**
			 * @brief Returns the number of bakes still queued. Thread-safe.
			 * @return size_t
			 */
			[[nodiscard]]
			size_t pendingJobs () const noexcept;

			/** @copydoc EmEn::Graphics::RenderTarget::Abstract::setViewDistance() */
			void setViewDistance (float meters) noexcept override;

			/** @copydoc EmEn::Graphics::RenderTarget::Abstract::viewDistance() */
			[[nodiscard]]
			float viewDistance () const noexcept override;

			/** @copydoc EmEn::Graphics::RenderTarget::Abstract::updateViewRangesProperties() */
			void updateViewRangesProperties (float fovOrNear, float distanceOrFar) noexcept override;

			/** @copydoc EmEn::Graphics::RenderTarget::Abstract::aspectRatio() */
			[[nodiscard]]
			float
			aspectRatio () const noexcept override
			{
				return 1.0F;
			}

			/** @copydoc EmEn::Graphics::RenderTarget::Abstract::isCubemap() */
			[[nodiscard]]
			bool
			isCubemap () const noexcept override
			{
				return false;
			}

			/** @copydoc EmEn::Graphics::RenderTarget::Abstract::framebuffer() */
			[[nodiscard]]
			const Vulkan::Framebuffer * framebuffer () const noexcept override;

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

			/**
			 * @copydoc EmEn::Graphics::RenderTarget::Abstract::isReadyForRendering()
			 * @note Ready only while a job is queued: the renderer skips the target otherwise, at the cost of one
			 * atomic load.
			 */
			[[nodiscard]]
			bool isReadyForRendering () const noexcept override;

			/**
			 * @copydoc EmEn::Graphics::RenderTarget::Abstract::isRefreshedWhenContentArrives()
			 * @note Never: a bake renders when a job asks for it, not whenever some instance of the scene gets ready.
			 */
			[[nodiscard]]
			bool
			isRefreshedWhenContentArrives () const noexcept override
			{
				return false;
			}

			/** @copydoc EmEn::Scenes::AVConsole::AbstractVirtualDevice::videoType() */
			[[nodiscard]]
			Scenes::AVConsole::VideoType
			videoType () const noexcept override
			{
				return Scenes::AVConsole::VideoType::Texture;
			}

			/**
			 * @copydoc EmEn::Graphics::RenderTarget::Abstract::writeCombinedImageSampler()
			 * @note Never: the bake is read through its atlases (ImposterAtlas), not as a texture.
			 */
			[[nodiscard]]
			bool writeCombinedImageSampler (const Vulkan::DescriptorSet & descriptorSet, uint32_t bindingIndex) const noexcept override;

			/**
			 * @copydoc EmEn::Graphics::RenderTarget::Abstract::capture()
			 * @note Not supported: read the atlas (ImposterAtlas::writeAlbedo()).
			 */
			bool capture (Vulkan::TransferManager & transferManager, uint32_t layerIndex, bool keepAlpha, bool withDepthBuffer, bool withStencilBuffer, std::array< Base::PixelFactory::Pixmap< uint8_t >, 3 > & result) const noexcept override;

			/** @copydoc EmEn::Graphics::RenderTarget::Abstract::recordPostRenderCompute() */
			void recordPostRenderCompute (const Vulkan::CommandBuffer & commandBuffer) noexcept override;

		private:

			/** @brief One queued bake. */
			struct Job final
			{
				const void * subject{nullptr};
				std::shared_ptr< ImposterAtlas > atlas;
				uint32_t warmUpRenders{WarmUpRenders};
			};

			/** @copydoc EmEn::Scenes::AVConsole::AbstractVirtualDevice::updateVideoDeviceProperties() */
			void updateVideoDeviceProperties (float fovOrNear, float distanceOrFar, bool isOrthographicProjection) noexcept override;

			/** @copydoc EmEn::Scenes::AVConsole::AbstractVirtualDevice::getWorldCoordinates() */
			[[nodiscard]]
			Base::Math::CartesianFrame< float >
			getWorldCoordinates () const noexcept override
			{
				return m_worldCoordinates;
			}

			/** @copydoc EmEn::Scenes::AVConsole::AbstractVirtualDevice::updateDeviceFromCoordinates() */
			void updateDeviceFromCoordinates (const Base::Math::CartesianFrame< float > & worldCoordinates, const Base::Math::Vector< 3, float > & worldVelocity) noexcept override;

			/** @copydoc EmEn::Graphics::RenderTarget::Abstract::onCreate() */
			[[nodiscard]]
			bool onCreate (Renderer & renderer) noexcept override;

			/** @copydoc EmEn::Graphics::RenderTarget::Abstract::onDestroy() */
			void onDestroy () noexcept override;

			/** @copydoc EmEn::Graphics::RenderTarget::Abstract::createRenderPass() */
			[[nodiscard]]
			std::shared_ptr< Vulkan::RenderPass > createRenderPass (Renderer & renderer) const noexcept override;

			/**
			 * @brief Creates one colour attachment and its view.
			 * @param renderer A reference to the renderer.
			 * @param format The format.
			 * @param usage The usage flags on top of the colour attachment.
			 * @param label The label of the attachment.
			 * @param image A reference to the image smart pointer.
			 * @param imageView A reference to the image view smart pointer.
			 * @return bool
			 */
			[[nodiscard]]
			bool createAttachment (Renderer & renderer, VkFormat format, VkImageUsageFlags usage, const char * label, std::shared_ptr< Vulkan::Image > & image, std::shared_ptr< Vulkan::ImageView > & imageView) const noexcept;

			std::shared_ptr< Vulkan::Image > m_colorImage;
			std::shared_ptr< Vulkan::ImageView > m_colorImageView;
			std::shared_ptr< Vulkan::Image > m_normalsImage;
			std::shared_ptr< Vulkan::ImageView > m_normalsImageView;
			std::shared_ptr< Vulkan::Image > m_materialPropertiesImage;
			std::shared_ptr< Vulkan::ImageView > m_materialPropertiesImageView;
			std::shared_ptr< Vulkan::Image > m_albedoImage;
			std::shared_ptr< Vulkan::ImageView > m_albedoImageView;
			std::shared_ptr< Vulkan::Image > m_depthImage;
			std::shared_ptr< Vulkan::ImageView > m_depthImageView;
			std::shared_ptr< Vulkan::Framebuffer > m_framebuffer;
			ViewMatrices2DUBO m_viewMatrices;
			Base::Math::CartesianFrame< float > m_worldCoordinates;
			std::deque< Job > m_jobs;
			std::vector< std::weak_ptr< ImposterAtlas > > m_bakedAtlases;
			mutable std::mutex m_jobsAccess;
			std::atomic_bool m_hasJobs{false};
			bool m_isCreated{false};
	};
}
