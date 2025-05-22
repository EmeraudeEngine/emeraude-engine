/*
 * src/Overlay/Elements/Range.hpp
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
#include <string>

/* Local inclusions for inheritances. */
#include "Overlay/ComposedSurface.hpp"

/* Local inclusions for usages. */
#include "Graphics/TextWriter.hpp"

namespace EmEn::Overlay::Elements
{
	/**
	 * @brief The range element.
	 * @extends EmEn::Overlay::ComposedSurface This a visible element.
	 */
	class Range final : public ComposedSurface
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"Range"};

			/**
			 * @brief Constructs a range.
			 * @param framebufferProperties A reference to the overlay framebuffer properties.
			 * @param name A reference to a string.
			 * @param geometry A reference to a rectangle for the surface geometry on screen. Default the whole screen.
			 * @param depth A depth value to order surface on the screen. Default 0.0.
			 */
			Range (const FramebufferProperties & framebufferProperties, const std::string & name, const Libs::Math::Space2D::AARectangle< float > & geometry = {}, float depth = 0.0F) noexcept;

			/**
			 * @brief Gives access to the TextWriter for configuration.
			 * @warning  Don't use it to write on the surface.
			 * @return Graphics::TextWriter &
			 */
			Graphics::TextWriter &
			textWriter () noexcept
			{
				return m_textWriter;
			}

			/**
			 * @brief Sets the range value.
			 * @param value A float from 0.0 to 1.0
			 * @return void
			 */
			void
			setValue (float value) noexcept
			{
				m_value = Libs::Math::clampToUnit(value);

				this->drawFinished();
			}

			/**
			 * @brief Returns the range value between 0.0 and 1.0.
			 * @return float
			 */
			[[nodiscard]]
			float
			value () const noexcept
			{
				return m_value;
			}

		private:
		
			/**
			 * @brief 
			 * @param pixmap A reference to a pixmap.
			 * @return bool
			 */
			bool onResize (const Libs::PixelFactory::Pixmap< uint8_t > & pixmap) noexcept override;

			/**
			 * @brief 
			 * @param pixmap A reference to a pixmap.
			 * @return bool
			 */
			bool onDrawFinished (Libs::PixelFactory::Pixmap< uint8_t > & pixmap) noexcept override;

			Graphics::TextWriter m_textWriter;
			float m_value{0.0F};
	};
}
