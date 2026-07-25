/*
 * src/Graphics/ViewMatricesInterface.hpp
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

/* Local inclusions for usages. */
#include "Frustum.hpp"
#include "Math/CartesianFrame.hpp"
#include "PixelFactory/Color.hpp"

/* Forward declarations. */
namespace EmEn
{
	namespace Vulkan
	{
		class DescriptorSet;
	}

	namespace Graphics
	{
		class Renderer;
	}
}

namespace EmEn::Graphics
{
	/** 
	 * @brief Defines an abstract way to describe a view with coordinates and matrices to use with Vulkan.
	 */
	class EMEN_API ViewMatricesInterface
	{
		public:

			/**
			 * @brief Copy constructor.
			 * @param copy A reference to the copied instance.
			 */
			ViewMatricesInterface (const ViewMatricesInterface & copy) noexcept = delete;

			/**
			 * @brief Move constructor.
			 * @param copy A reference to the copied instance.
			 */
			ViewMatricesInterface (ViewMatricesInterface && copy) noexcept = delete;

			/**
			 * @brief Copy assignment.
			 * @param copy A reference to the copied instance.
			 * @return ViewMatricesInterface &
			 */
			ViewMatricesInterface & operator= (const ViewMatricesInterface & copy) noexcept = delete;

			/**
			 * @brief Move assignment.
			 * @param copy A reference to the copied instance.
			 * @return ViewMatricesInterface &
			 */
			ViewMatricesInterface & operator= (ViewMatricesInterface && copy) noexcept = delete;
			
			/**
			 * @brief Destructs the view matrices interface.
			 */
			virtual ~ViewMatricesInterface () = default;

			/**
			 * @brief Returns the projection matrix.
			 * @return const Matrix< 4, float > &
			 */
			[[nodiscard]]
			virtual const Base::Math::Matrix< 4, float > & projectionMatrix () const noexcept = 0;

			/**
			 * @brief Returns the projection matrix.
			 * @note When a sub-pixel jitter is active (TAA), this returns the JITTERED matrix, i.e.
			 * the one the frame is actually rasterized with. Anything feeding a velocity clip
			 * position must use unjitteredProjectionMatrix() instead.
			 * @param readStateIndex The render state-valid index to read data.
			 * @return const Matrix< 4, float > &
			 */
			[[nodiscard]]
			virtual const Base::Math::Matrix< 4, float > & projectionMatrix (uint32_t readStateIndex) const noexcept = 0;

			/**
			 * @brief Returns the projection matrix without any sub-pixel jitter (TAA).
			 * @note The TAA jitter is applied to gl_Position through a per-draw push constant, never
			 * through a matrix: any matrix a velocity consumer reads (the InstanceTransforms SSBO
			 * header, the pushed view projection of the paths that jitter in the shader) MUST come
			 * from here. Views that do not support jitter serve the same matrix as projectionMatrix().
			 * @param readStateIndex The render state-valid index to read data.
			 * @return const Matrix< 4, float > &
			 */
			[[nodiscard]]
			virtual
			const Base::Math::Matrix< 4, float > &
			unjitteredProjectionMatrix (uint32_t readStateIndex) const noexcept
			{
				return this->projectionMatrix(readStateIndex);
			}

			/**
			 * @brief Returns the view matrix.
			 * @param infinity Gives the view matrix for infinite view.
			 * @param viewIndex The index of the matrix for the cubemap view.
			 * @return const Matrix< 4, float > &
			 */
			[[nodiscard]]
			virtual const Base::Math::Matrix< 4, float > & viewMatrix (bool infinity, size_t viewIndex) const noexcept = 0;

			/**
			 * @brief Returns the view matrix.
			 * @param readStateIndex The render state-valid index to read data.
			 * @param infinity Gives the view matrix for infinite view.
			 * @param viewIndex The index of the matrix for the cubemap view.
			 * @return const Matrix< 4, float > &
			 */
			[[nodiscard]]
			virtual const Base::Math::Matrix< 4, float > & viewMatrix (uint32_t readStateIndex, bool infinity, size_t viewIndex) const noexcept = 0;

			/**
			 * @brief Returns the view matrix used by the previously rendered frame.
			 * @note Frame-history contract for temporal effects (reprojection, accumulation).
			 * Only meaningful after archiveStateAfterRendering() has been called at least once;
			 * views that do not keep a history return an identity matrix. Consumers MUST handle
			 * their own first-frame invalidation (this contract does not signal validity).
			 * @return const Matrix< 4, float > &
			 */
			[[nodiscard]]
			virtual
			const Base::Math::Matrix< 4, float > &
			previousViewMatrix () const noexcept
			{
				static const Base::Math::Matrix< 4, float > noHistory{};

				return noHistory;
			}

			/**
			 * @brief Returns the projection matrix used by the previously rendered frame.
			 * @note Frame-history contract for temporal effects. See previousViewMatrix().
			 * @return const Matrix< 4, float > &
			 */
			[[nodiscard]]
			virtual
			const Base::Math::Matrix< 4, float > &
			previousProjectionMatrix () const noexcept
			{
				static const Base::Math::Matrix< 4, float > noHistory{};

				return noHistory;
			}

			/**
			 * @brief Sets the sub-pixel projection jitter for the frame being rendered (temporal anti-aliasing).
			 * @note Called by the Renderer ONCE per rendered frame, on the render thread, BEFORE the
			 * view UBO upload and before any consumer reads projectionMatrix(readStateIndex). The offset
			 * is expressed in NDC units (2 * pixelOffset / viewportSize). Only the main view keeps and
			 * applies a jitter; the default implementation ignores it (shadow maps, cubemaps and
			 * render-to-texture targets must NEVER be jittered).
			 * @param ndcOffset The jitter offset in normalized device coordinates.
			 * @return void
			 */
			virtual
			void
			setProjectionJitter (const Base::Math::Vector< 2, float > & /*ndcOffset*/) noexcept
			{
				/* Default: this view does not support projection jitter. */
			}

			/**
			 * @brief Disables the projection jitter (temporal anti-aliasing inactive).
			 * @note Restores the clean projection path at zero per-draw cost. Default implementation is a no-op.
			 * @return void
			 */
			virtual
			void
			disableProjectionJitter () noexcept
			{
				/* Default: this view does not support projection jitter. */
			}

			/**
			 * @brief Returns the projection jitter applied to the frame being rendered.
			 * @note Zero when jitter is disabled or unsupported.
			 * @return const Base::Math::Vector< 2, float > &
			 */
			[[nodiscard]]
			virtual
			const Base::Math::Vector< 2, float > &
			projectionJitter () const noexcept
			{
				static const Base::Math::Vector< 2, float > noJitter{};

				return noJitter;
			}

			/**
			 * @brief Returns the position of the point of view.
			 * @return const Base::Math::Vector< 3, float > &
			 */
			[[nodiscard]]
			virtual const Base::Math::Vector< 3, float > & position () const noexcept = 0;

			/**
			 * @brief Returns the position of the point of view.
			 * @param readStateIndex The render state-valid index to read data.
			 * @return const Base::Math::Vector< 3, float > &
			 */
			[[nodiscard]]
			virtual const Base::Math::Vector< 3, float > & position (uint32_t readStateIndex) const noexcept = 0;

			/**
			 * @brief Returns the const access to the frustum for object clipping.
			 * @param viewIndex The index of the frustum for the cubemap view.
			 * @return Frustum
			 */
			[[nodiscard]]
			virtual const Frustum & frustum (size_t viewIndex) const noexcept = 0;

			/**
			 * @brief Returns the const access to the frustum for object clipping.
			 * @param readStateIndex The render state-valid index to read data.
			 * @param viewIndex The index of the frustum for the cubemap view.
			 * @return Frustum
			 */
			[[nodiscard]]
			virtual const Frustum & frustum (uint32_t readStateIndex, size_t viewIndex) const noexcept = 0;

			/**
			 * @brief Returns the aspect ratio of the view.
			 * @return float
			 */
			[[nodiscard]]
			virtual float getAspectRatio () const noexcept = 0;

			/**
			 * @brief Returns the field of view of the perspective projection matrix.
			 * @return float
			 */
			[[nodiscard]]
			virtual float fieldOfView () const noexcept = 0;

			/**
			 * @brief Returns the near clipping plane distance.
			 * @return float
			 */
			[[nodiscard]]
			virtual float nearPlane () const noexcept = 0;

			/**
			 * @brief Returns the far clipping plane distance.
			 * @return float
			 */
			[[nodiscard]]
			virtual float farPlane () const noexcept = 0;

			/**
			 * @brief Returns the view descriptor set.
			 * @return const Vulkan::DescriptorSet *
			 */
			[[nodiscard]]
			virtual const Vulkan::DescriptorSet * descriptorSet () const noexcept = 0;

			/**
			 * @brief Updates view properties with a perspective projection.
			 * @note This should be called when the viewport changes.
			 * @param width The width of the viewport.
			 * @param height The height of the viewport.
			 * @param distance The maximal distance of the viewport for perspective calculation.
			 * @param fov The field of view in degrees.
			 * @return void
			 */
			virtual void updatePerspectiveViewProperties (float width, float height, float fov, float distance) noexcept = 0;

			/**
			 * @brief Updates view properties with an orthographic projection.
			 * @note This should be called when the viewport changes.
			 * @param width The width of the viewport.
			 * @param height The height of the viewport.
			 * @param nearDistance The minimal distance of the viewport for orthographic calculation.
			 * @param farDistance The maximal distance of the viewport for perspective calculation.
			 * @return void
			 */
			virtual void updateOrthographicViewProperties (float width, float height, float nearDistance, float farDistance) noexcept = 0;

			/**
			 * @brief Updates the view coordinates. This should be called everytime the point of view moves.
			 * @param coordinates The absolute coordinates of the camera responsible for this view.
			 * @param velocity A vector representing a velocity applied to the camera for special effect.
			 * @return void
			 */
			virtual void updateViewCoordinates (const Base::Math::CartesianFrame< float > & coordinates, const Base::Math::Vector< 3, float > & velocity) noexcept = 0;

			/**
			 * @brief Updates optional ambient color and intensity.
			 * @param color A reference to a color.
			 * @param intensity The light intensity.
			 * @return void
			 */
			virtual void updateAmbientLightProperties (const Base::PixelFactory::Color< float > & color, float intensity) noexcept = 0;

			/**
			 * @brief Creates a buffer in the video memory.
			 * @param renderer A reference to the renderer.
			 * @param instanceID A reference to a string.
			 * @return bool
			 */
			virtual bool create (Renderer & renderer, const std::string & instanceID) noexcept = 0;

			/**
			 * @brief Copies local data for a stable render.
			 * @note This must be done at the end of the logic loop.
			 * @param writeStateIndex The render state-free index to write data.
			 * @return void
			 */
			virtual void publishStateForRendering (uint32_t writeStateIndex) noexcept = 0;

			/**
			 * @brief Archives the view state consumed by the frame that was just recorded.
			 * @note Frame-history contract for temporal effects. Called by the Renderer ONCE per
			 * rendered frame, on the render thread, AFTER the frame's command buffer is recorded —
			 * so that during the recording of frame N, previousViewMatrix()/previousProjectionMatrix()
			 * still expose the state of frame N-1. This is distinct from the logic/render
			 * double-buffering (publishStateForRendering): state indices track logic ticks,
			 * NOT rendered frames. Default implementation keeps no history.
			 * @param readStateIndex The render state-valid index the frame was rendered with.
			 * @return void
			 */
			virtual
			void
			archiveStateAfterRendering (uint32_t /*readStateIndex*/) noexcept
			{
				/* Default: this view keeps no frame history. */
			}

			/**
			 * @brief Updates the video memory.
			 * @note This is done just before a rendering.
			 * @param readStateIndex The render state-valid index to read data.
			 * @return bool
			 */
			[[nodiscard]]
			virtual bool updateVideoMemory (uint32_t readStateIndex) const noexcept = 0;

			/**
			 * @brief Destroys buffer in the video memory.
			 * @return void
			 */
			virtual void destroy () noexcept = 0;

			/**
			 * @brief Computes the 8 corners of a frustum in world space.
			 * @note The corners are computed by transforming NDC cube corners through the inverse view-projection matrix.
			 * @param inverseViewProjection The inverse of the view-projection matrix.
			 * @return std::array< Base::Math::Vector< 3, float >, 8 > The frustum corners in world space.
			 */
			[[nodiscard]]
			static std::array< Base::Math::Vector< 3, float >, 8 > computeFrustumCornersWorld (const Base::Math::Matrix< 4, float > & inverseViewProjection) noexcept;

			/**
			 * @brief Computes the 8 corners of this view's frustum in world space.
			 * @note Uses the current projection and view matrices to compute frustum corners.
			 * @return std::array< Base::Math::Vector< 3, float >, 8 > The frustum corners in world space.
			 */
			[[nodiscard]]
			std::array< Base::Math::Vector< 3, float >, 8 >
			getFrustumCornersWorld () const noexcept
			{
				const auto viewProjection = this->projectionMatrix() * this->viewMatrix(false, 0);

				return ViewMatricesInterface::computeFrustumCornersWorld(viewProjection.inverse());
			}

		protected:

			/**
			 * @brief Constructs a view matrices interface.
			 */
			ViewMatricesInterface () noexcept = default;
	};
}
