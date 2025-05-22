/*
 * src/Overlay/ComposedSurface.hpp
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
#include <cstdint>
#include <array>
#include <memory>
#include <string>

/* Local inclusions for inheritances. */
#include "Surface.hpp"

/* Local inclusions for usages. */
#include "Layouts/Main.hpp"
#include "Libs/Math/Space2D/AARectangle.hpp"
#include "Libs/Math/Vector.hpp"
#include "Libs/PixelFactory/Pixmap.hpp"
#include "Graphics/Geometry/IndexedVertexResource.hpp"
#include "Vulkan/DescriptorSet.hpp"
#include "Vulkan/Image.hpp"

/* Forward declarations. */
namespace EmEn::Graphics
{
	class Renderer;
	class ImageResource;
}

namespace EmEn::Overlay
{
	enum class Position : uint8_t
	{
		Center,
		Bottom,
		Top,
		Left,
		Right,
		BottomLeft,
		BottomRight,
		TopLeft,
		TopRight
	};

	/**
	 * @brief The base class of all overlay surfaces.
	 * @extends EmEn::Overlay::Surface This is a surface.
	 */
	class ComposedSurface : public Surface
	{
		friend class UIScreen;

		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"ComposedSurface"};

			/**
			 * @brief Constructs a composed surface.
			 * @param framebufferProperties A reference to the overlay framebuffer properties.
			 * @param name A reference to a string.
			 * @param geometry A reference to a rectangle for the surface geometry on screen. Default the whole screen.
			 * @param depth A depth value to order surface on the screen. Default 0.0.
			 */
			ComposedSurface (const FramebufferProperties & framebufferProperties, const std::string & name, const Libs::Math::Space2D::AARectangle< float > & geometry = {}, float depth = 0.0F) noexcept;

			/**
			 * @brief Sets the dimension of the surface.
			 * @note This implies a call to Surface::resize().
			 * @param width
			 * @param height
			 * @return void
			 */
			void setGeometry (float width, float height) noexcept;

			/**
			 * @brief Integer version of the function that set the dimensions of the surface in pixels.
			 * @note This implies a call to Surface::resize().
			 * @param width
			 * @param height
			 * @return void
			 */
			void setGeometry (unsigned int width, unsigned int height) noexcept;

			/**
			 * @brief Sets the lower left point of the surface on screen.
			 * @param positionX
			 * @param positionY
			 * @return void
			 */
			//void setPosition (float positionX, float positionY) noexcept;

			/**
			 * @brief Integer version of the function that set the lower left point of the surface on screen in pixels.
			 * @param positionX
			 * @param positionY
			 * @return void
			 */
			void setPosition (int positionX, int positionY) noexcept;

			/**
			 * @brief Sets a predefined position for the surface on the screen.
			 * @param position
			 * @return void
			 */
			void setPosition (Position position) noexcept;

			/**
			 * @brief Returns the root layout.
			 * @return Layouts::Main &
			 */
			[[nodiscard]]
			Layouts::Main & rootLayout () noexcept;

			/**
			 * @brief Returns the root layout.
			 * @return const Layouts::Main &
			 */
			[[nodiscard]]
			const Layouts::Main & rootLayout () const noexcept;

			/**
			 * @brief Marks the surface ready for a video memory processLogics.
			 * @return void
			 */
			void drawFinished () noexcept;

			/**
			 * @brief Returns the size in graphics API metrics of the surface.
			 * @return std::array< float, 2 >
			 */
			[[nodiscard]]
			std::array< float, 2 > realSize () const noexcept;

			/**
			 * @brief Returns the position of the top/left border of the surface in graphics API metrics on the screen.
			 * @return std::array< float, 2 >
			 */
			[[nodiscard]]
			std::array< float, 2 > offsets () const noexcept;

			/**
			 * @brief Returns the texture offsets.
			 * @return const Libs::Math::Vector< 2, float > &
			 */
			[[nodiscard]]
			const Libs::Math::Vector< 2, float > & textureOffset () const noexcept;

		protected:

			/**
			 * @brief setTextureOffsetX
			 * @param offsetX
			 * @return void
			 */
			void setTextureOffsetX (float offsetX) noexcept;

			/**
			 * @brief setTextureOffsetY
			 * @param offsetY
			 * @return void
			 */
			void setTextureOffsetY (float offsetY) noexcept;

			/**
			 * @brief setTextureOffset
			 * @param offset
			 * @return void
			 */
			void setTextureOffset (const Libs::Math::Vector< 2, float > & offset) noexcept;

			/**
			 * @brief This method it's called on first surface initialization and every time the geometry changes.
			 * @param pixmap A reference to a pixmap.
			 * @return bool
			 */
			virtual bool onResize (const Libs::PixelFactory::Pixmap< uint8_t > & pixmap) noexcept;

			/**
			 * @brief Callback before Surface::drawFinished() function is executed.
			 * @note If returns false, the main function will abort.
			 * @param pixmap A reference to a pixmap.
			 * @return bool
			 */
			virtual bool onDrawFinished (Libs::PixelFactory::Pixmap< uint8_t > & pixmap) noexcept;

		private:

			/**
			 * @brief Creates or updates the vertex data of the surface.
			 * @return bool
			 */
			bool updateGeometry () noexcept;

			/**
			 * @brief Creates or updates the texture of the surface.
			 * @param screenWidth The screen width.
			 * @param screenHeight The screen height.
			 * @return bool
			 */
			bool updateTexture (float screenWidth, float screenHeight) noexcept;

			/**
			 * @brief Resizes the surface.
			 * @return bool
			 */
			bool resize () noexcept;

			/* Flag names. */
			static constexpr auto CapturePointerEvents{0UL};
			static constexpr auto CaptureKeyboardEvents{1UL};
			static constexpr auto IsPointerOver{2UL};
			static constexpr auto IsFocused{3UL};

			Layouts::Main m_rootLayout{};
			Libs::Math::Space2D::AARectangle< float > m_rectangle{0.0F, 0.0F, 1.0F, 1.0F};
			std::unique_ptr< Graphics::Geometry::IndexedVertexResource > m_geometry{};
			std::shared_ptr< Vulkan::Image > m_texture{};
			Libs::PixelFactory::Pixmap< uint8_t > m_data{};
			Libs::Math::Vector< 2, float > m_textureProportion{1.0F, 1.0F};
			Libs::Math::Vector< 2, float > m_textureOffset{};
			std::unique_ptr< Vulkan::DescriptorSet > m_descriptorSet{};
			std::array< bool, 8 > m_flags{
				false/*CapturePointerEvents*/,
				false/*CaptureKeyboardEvents*/,
				false/*IsPointerOver*/,
				false/*IsFocused*/,
				false/*UNUSED*/,
				false/*UNUSED*/,
				false/*UNUSED*/,
				false/*UNUSED*/
			};
	};
}
