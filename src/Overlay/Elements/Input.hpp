/*
 * src/Overlay/Elements/Input.hpp
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
#include "Interface.hpp"

/* Local inclusions for usages. */
#include "Graphics/TextWriter.hpp"

namespace EmEn::Overlay::Elements
{
	/**
	 * @brief The input form element.
	 * @extends EmEn::Overlay::ComposedSurface This a visible element.
	 * @extends EmEn::Overlay::Elements::UIInputInterface this is a user input element.
	 */
	class Input final : public ComposedSurface, public UIInputInterface
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"Input"};

			/**
			 * @brief Constructs an input.
			 * @param framebufferProperties A reference to the overlay framebuffer properties.
			 * @param name A reference to a string.
			 * @param geometry A reference to a rectangle for the surface geometry on screen. Default the whole screen.
			 * @param depth A depth value to order surface on the screen. Default 0.0.
			 */
			Input (const FramebufferProperties & framebufferProperties, const std::string & name, const Libs::Math::Space2D::AARectangle< float > & geometry = {}, float depth = 0.0F) noexcept;

			/** @copydoc EmEn::Overlay::Surface::onKeyRelease() */
			bool onKeyRelease (int32_t key, int32_t scancode, int32_t modifiers) noexcept override;

			/** @copydoc EmEn::Overlay::Surface::onCharacterType() */
			bool onCharacterType (uint32_t unicode) noexcept override;

			/**
			 * @brief Gives access to the TextWriter for configuration.
			 * @warning Don't use it to write on the surface.
			 * @return Graphics::TextWriter &
			 */
			Graphics::TextWriter & textWriter () noexcept;

			/**
			 * @brief Sets the default color of the background of the surface.
			 * @param color A reference to the color.
			 * @return void
			 */
			void setClearColor (const Libs::PixelFactory::Color< float > & color) noexcept;

			/**
			 * @brief Returns the clear color.
			 * @return const Color< float > &
			 */
			[[nodiscard]]
			const Libs::PixelFactory::Color< float > & clearColor () const noexcept;

			/**
			 * @brief Sets the initial content of the input box.
			 * @return void
			 */
			void write (const std::string & text) noexcept;

			/**
			 * @brief Clears the content of the input box.
			 * @return void
			 */
			void clear () noexcept;

			/**
			 * @brief Returns the text currently held by the surface.
			 * @return const std::string &
			 */
			[[nodiscard]]
			const std::string & text () const noexcept;

		private:

			/**
			 * @brief Declares the draw finished on the surface.
			 * @param pixmap A reference to a pixmap.
			 * @return bool
			 */
			bool onDrawFinished (Libs::PixelFactory::Pixmap< uint8_t > & pixmap) noexcept override;

			Libs::PixelFactory::Color< float > m_clearColor = Libs::PixelFactory::Transparent;
			std::string m_text{};
			Graphics::TextWriter m_textWriter{};
	};
}
