/*
 * src/Graphics/ViewMatricesInterface.hpp
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
#include <memory>

/* Local inclusions for usages. */
#include "Libs/PixelFactory/Color.hpp"
#include "Libs/Math/CartesianFrame.hpp"
#include "Vulkan/LayoutManager.hpp"
#include "Vulkan/DescriptorSet.hpp"
#include "Frustum.hpp"

/* Forward declarations. */
namespace EmEn::Graphics
{
	class Renderer;
}

namespace EmEn::Graphics
{
	/** 
	 * @brief Defines an abstract way to describe a view with coordinates and matrices to use with Vulkan.
	 */
	class ViewMatricesInterface
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
			 * @brief Returns the descriptor set layout for this view.
			 * @param layoutManager A reference to the layout manager.
			 * @return std::shared_ptr< Vulkan::DescriptorSetLayout >
			 */
			[[nodiscard]]
			static std::shared_ptr< Vulkan::DescriptorSetLayout > getDescriptorSetLayout (Vulkan::LayoutManager & layoutManager) noexcept;
			
			/**
			 * @brief Returns the projection matrix.
			 * @return const Matrix< 4, float > &
			 */
			[[nodiscard]]
			virtual const Libs::Math::Matrix< 4, float > & projectionMatrix () const noexcept = 0;

			/**
			 * @brief Returns the projection matrix.
			 * @param readStateIndex The render state-valid index to read data.
			 * @return const Matrix< 4, float > &
			 */
			[[nodiscard]]
			virtual const Libs::Math::Matrix< 4, float > & projectionMatrix (uint32_t readStateIndex) const noexcept = 0;

			/**
			 * @brief Returns the view matrix.
			 * @param infinity Gives the view matrix for infinite view.
			 * @param index The index of the matrix for the cubemap view.
			 * @return const Matrix< 4, float > &
			 */
			[[nodiscard]]
			virtual const Libs::Math::Matrix< 4, float > & viewMatrix (bool infinity, size_t index) const noexcept = 0;

			/**
			 * @brief Returns the view matrix.
			 * @param readStateIndex The render state-valid index to read data.
			 * @param infinity Gives the view matrix for infinite view.
			 * @param index The index of the matrix for the cubemap view.
			 * @return const Matrix< 4, float > &
			 */
			[[nodiscard]]
			virtual const Libs::Math::Matrix< 4, float > & viewMatrix (uint32_t readStateIndex, bool infinity, size_t index) const noexcept = 0;

			/**
			 * @brief Returns the position of the point of view.
			 * @return const Libs::Math::Vector< 3, float > &
			 */
			[[nodiscard]]
			virtual const Libs::Math::Vector< 3, float > & position () const noexcept = 0;

			/**
			 * @brief Returns the position of the point of view.
			 * @param readStateIndex The render state-valid index to read data.
			 * @return const Libs::Math::Vector< 3, float > &
			 */
			[[nodiscard]]
			virtual const Libs::Math::Vector< 3, float > & position (uint32_t readStateIndex) const noexcept = 0;

			/**
			 * @brief Returns the const access to the frustum for object clipping.
			 * @param index The index of the frustum for the cubemap view.
			 * @return Frustum
			 */
			[[nodiscard]]
			virtual const Frustum & frustum (size_t index) const noexcept = 0;

			/**
			 * @brief Returns the const access to the frustum for object clipping.
			 * @param readStateIndex The render state-valid index to read data.
			 * @param index The index of the frustum for the cubemap view.
			 * @return Frustum
			 */
			[[nodiscard]]
			virtual const Frustum & frustum (uint32_t readStateIndex, size_t index) const noexcept = 0;

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
			 * @brief Updates view properties with a perspective projection.
			 * @note This should be called when the viewport changes.
			 * @param width The width of the viewport.
			 * @param height The height of the viewport.
			 * @param distance The maximal distance of the viewport for perspective calculation.
			 * @param fov The field of view in degrees.
			 * @return void
			 */
			virtual void updatePerspectiveViewProperties (float width, float height, float distance, float fov) noexcept = 0;

			/**
			 * @brief Updates view properties with an orthographic projection.
			 * @note This should be called when the viewport changes.
			 * @param width The width of the viewport.
			 * @param height The height of the viewport.
			 * @param farDistance The maximal distance of the viewport for perspective calculation.
			 * @param nearDistance The minimal distance of the viewport for orthographic calculation.
			 * @return void
			 */
			virtual void updateOrthographicViewProperties (float width, float height, float farDistance, float nearDistance) noexcept = 0;

			/**
			 * @brief Updates the view coordinates. This should be called everytime the point of view moves.
			 * @param coordinates The absolute coordinates of the camera responsible for this view.
			 * @param velocity A vector representing a velocity applied to the camera for special effect.
			 * @return void
			 */
			virtual void updateViewCoordinates (const Libs::Math::CartesianFrame< float > & coordinates, const Libs::Math::Vector< 3, float > & velocity) noexcept = 0;

			/**
			 * @briefs Update optional ambient color and intensity.
			 * @param color A reference to a color.
			 * @param intensity The light intensity.
			 * @return void
			 */
			virtual void updateAmbientLightProperties (const Libs::PixelFactory::Color< float > & color, float intensity) noexcept = 0;

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
			 * @brief Returns the view descriptor set.
			 * @return const Vulkan::DescriptorSet *
			 */
			[[nodiscard]]
			virtual const Vulkan::DescriptorSet * descriptorSet () const noexcept = 0;

		protected:

			/**
			 * @brief Constructs a view matrices interface.
			 */
			ViewMatricesInterface () noexcept = default;
	};
}
