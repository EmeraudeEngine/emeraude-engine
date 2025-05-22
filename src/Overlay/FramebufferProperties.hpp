/*
 * src/Overlay/FramebufferProperties.hpp
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
#include <cmath>
#include <string>

namespace EmEn::Overlay
{
	/**
	 * @brief Defines the framebuffer properties. This helps to create surfaces of the overlay according to HDPI usage.
	 */
	class FramebufferProperties final
	{
		public:

			/** 
			 * @brief Constructs a default framebuffer properties.
			 * @warning This will be invalid at this time, be sure to use FramebufferProperties::updateProperties().
			 */
			FramebufferProperties () noexcept = default;

			/**
			 * @brief Constructs a framebuffer properties from the available window framebuffer.
			 * @param width The framebuffer width in pixel.
			 * @param height The framebuffer height in pixel.
			 * @param scaleX The scale in X. Default 1.0.
			 * @param scaleY The scale in Y. Default 1.0.
			 */
			FramebufferProperties (uint32_t width, uint32_t height, float scaleX = 1.0F, float scaleY = 1.0F) noexcept;

			/**
			 * @brief Updates the framebuffer properties from the available window framebuffer.
			 * @note This must be called when the monitor or the OS window is resized.
			 * @param width The framebuffer width in pixel.
			 * @param height The framebuffer height in pixel.
			 * @return void
			 */
			void
			updateProperties (uint32_t width, uint32_t height) noexcept
			{
				m_width = width;
				m_height = height;

				this->updateScaledResolution();
			}

			/**
			 * @brief Updates the framebuffer properties from the available window framebuffer with new scaling.
			 * @note This must be called when the monitor or the OS window is resized.
			 * @param width The framebuffer width in pixel.
			 * @param height The framebuffer height in pixel.
			 * @param scaleX The screen scale in X. Default 1.0.
			 * @param scaleY The screen scale in Y. Default 1.0.
			 * @return void
			 */
			void
			updateProperties (uint32_t width, uint32_t height, float scaleX, float scaleY) noexcept
			{
				m_width = width;
				m_height = height;

				this->setScreenScale(scaleX, scaleY);
			}

			/**
			 * @brief Checks whether the framebuffer properties are valid.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isValid () const noexcept
			{
				return m_width * m_height != 0;
			}

			/**
			 * @brief Sets the framebuffer scaling factor for HDPI screen.
			 * @param scaleX The scale in X.
			 * @param scaleY The scale in Y.
			 * @return void
			 */
			void
			setScreenScale (float scaleX, float scaleY) noexcept
			{
				m_screenScaleX = scaleX;
				m_screenScaleY = scaleY;

				this->updateScaledResolution();
			}

			/**
			 * @brief Sets the framebuffer scaling factor for HDPI screen.
			 * @param scale The scale uniform on both axes.
			 * @return void
			 */
			void
			setScreenScale (float scale) noexcept
			{
				m_screenScaleX = scale;
				m_screenScaleY = scale;

				this->updateScaledResolution();
			}

			/**
			 * @brief Returns the framebuffer width in pixels.
			 * @return uint32_t
			 */
			[[nodiscard]]
			uint32_t
			width () const noexcept
			{
				return m_width;
			}

			/**
			 * @brief Returns the framebuffer height in pixels.
			 * @return uint32_t
			 */
			[[nodiscard]]
			uint32_t
			height () const noexcept
			{
				return m_height;
			}

			/**
			 * @brief Returns the scale to apply on X axis when using HDPI screen.
			 * @return float
			 */
			[[nodiscard]]
			float
			screenScaleX () const noexcept
			{
				return m_screenScaleX;
			}

			/**
			 * @brief Returns the scale to apply on Y axis when using HDPI screen.
			 * @return float
			 */
			[[nodiscard]]
			float
			screenScaleY () const noexcept
			{
				return m_screenScaleY;
			}

			/**
			 * @brief Returns the biggest HDPI screen scale factor between X and Y.
			 * @return float
			 */
			[[nodiscard]]
			float
			maxScreenScale () const noexcept
			{
				return std::max(m_screenScaleX, m_screenScaleY);
			}

			/**
			 * @brief Returns the inverse scale to apply on X axis when using HDPI screen.
			 * @return float
			 */
			[[nodiscard]]
			float
			inverseScreenScaleX () const noexcept
			{
				return m_inverseScreenScaleX;
			}

			/**
			 * @brief Returns the inverse scale to apply on Y axis when using HDPI screen.
			 * @return float
			 */
			[[nodiscard]]
			float
			inverseScreenScaleY () const noexcept
			{
				return m_inverseScreenScaleY;
			}

			/**
			 * @brief Returns the framebuffer width in points.
			 * @note This is the framebuffer width divided by the scale factor in X.
			 * @return float
			 */
			[[nodiscard]]
			float
			resolutionX () const noexcept
			{
				return m_resolutionX;
			}

			/**
			 * @brief Returns the framebuffer height in points.
			 * @note This is the framebuffer height divided by the scale factor in Y.
			 * @return float
			 */
			[[nodiscard]]
			float
			resolutionY () const noexcept
			{
				return m_resolutionY;
			}

			/**
			 * @brief Returns the framebuffer width in points rounded to an integer.
			 * @tparam integer_t The integer type. Default signed int.
			 * @return integer_t
			 */
			template< typename integer_t = int32_t >
			[[nodiscard]]
			integer_t
			getRoundedResolutionX () const noexcept requires (std::is_integral_v< integer_t >)
			{
				return static_cast< integer_t >(std::round(m_resolutionX));
			}

			/**
			 * @brief Returns the framebuffer height in points rounded to an integer.
			 * @tparam integer_t The integer type. Default signed int.
			 * @return integer_t
			 */
			template< typename integer_t = int32_t >
			[[nodiscard]]
			integer_t
			getRoundedResolutionY () const noexcept requires (std::is_integral_v< integer_t >)
			{
				return static_cast< integer_t >(std::round(m_resolutionY));
			}

			/**
			 * @brief Returns a width in points rounded to an integer.
			 * @tparam integer_t The integer type. Default signed int.
			 * @param width A scalar value to get a final size in screen space.
			 * @return integer_t
			 */
			template< typename integer_t = int32_t >
			[[nodiscard]]
			integer_t
			getRoundedResolutionX (float width) const noexcept requires (std::is_integral_v< integer_t >)
			{
					return static_cast< integer_t >(std::round(m_resolutionX * width));
			}

			/**
			 * @brief Returns a height in points rounded to an integer.
			 * @tparam integer_t The integer type. Default signed int.
			 * @param height A scalar value to get a final size in screen space.
			 * @return integer_t
			 */
			template< typename integer_t = int32_t >
			[[nodiscard]]
			integer_t
			getRoundedResolutionY (float height) const noexcept requires (std::is_integral_v< integer_t >)
			{
					return static_cast< integer_t >(std::round(m_resolutionY * height));
			}

			/**
			 * @brief Returns a computed width in pixels for a surface inside the framebuffer.
			 * @param surfaceWidth A scalar value to get a final size in screen space.
			 * @return uint32_t
			 */
			[[nodiscard]]
			uint32_t
			getSurfaceWidth (float surfaceWidth) const noexcept
			{
				/* NOTE: Do not remove the double round! */
				return static_cast< uint32_t >(std::round(std::round(m_resolutionX * surfaceWidth) * m_screenScaleX));
			}

			/**
			 * @brief Returns a computed width in pixels for a surface inside the framebuffer.
			 * @param surfaceHeight A scalar value to get a final size in screen space.
			 * @return uint32_t
			 */
			[[nodiscard]]
			uint32_t
			getSurfaceHeight (float surfaceHeight) const noexcept
			{
				/* NOTE: Do not remove the double round! */
				return static_cast< uint32_t >(std::round(std::round(m_resolutionY * surfaceHeight) * m_screenScaleY));
			}

			/**
			 * @brief Resets the framebuffer properties.
			 * @return void
			 */
			void reset () noexcept;

			/**
			 * @brief STL streams printable object.
			 * @param out A reference to the stream output.
			 * @param obj A reference to the object to print.
			 * @return std::ostream &
			 */
			friend std::ostream & operator<< (std::ostream & out, const FramebufferProperties & obj);

			/**
			 * @brief Stringifies the object.
			 * @param obj A reference to the object to print.
			 * @return std::string
			 */
			friend std::string to_string (const FramebufferProperties & obj) noexcept;

		private:

			/**
			 * @brief Updates the scaled resolution with factors.
			 * @return void
			 */
			void updateScaledResolution () noexcept;

			uint32_t m_width{0};
			uint32_t m_height{0};
			float m_screenScaleX{1.0F};
			float m_screenScaleY{1.0F};
			float m_inverseScreenScaleX{1.0F};
			float m_inverseScreenScaleY{1.0F};
			float m_resolutionX{0};
			float m_resolutionY{0};
	};
}
