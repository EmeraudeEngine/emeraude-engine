/*
 * src/Overlay/Elements/Button.hpp
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
#include <functional>
#include <memory>
#include <string>

/* Local inclusions for inheritances. */
#include "Overlay/ComposedSurface.hpp"
#include "Interface.hpp"

/* Local inclusions for usages. */
#include "Graphics/TextWriter.hpp"

namespace EmEn::Overlay::Elements
{
	/**
	 * @brief The button form element.
	 * @extends EmEn::Overlay::ComposedSurface This a visible element.
	 * @extends EmEn::Overlay::Elements::UIInputInterface this is a user input element.
	 */
	class Button final : public ComposedSurface, public UIInputInterface
	{
		public:

			using Function = std::function< int () >;

			/** @brief Class identifier. */
			static constexpr auto ClassId{"Button"};

			/**
			 * @brief Constructs a button.
			 * @param framebufferProperties A reference to the overlay framebuffer properties.
			 * @param name A reference to a string.
			 * @param geometry A reference to a rectangle for the surface geometry on screen. Default the whole screen.
			 * @param depth A depth value to order surface on the screen. Default 0.0.
			 */
			Button (const FramebufferProperties & framebufferProperties, const std::string & name, const Libs::Math::Space2D::AARectangle< float > & geometry = {}, float depth = 0.0F) noexcept;

			/** @copydoc EmEn::Overlay::Surface::onPointerEnter() */
			void onPointerEnter (float positionX, float positionY) noexcept override;

			/** @copydoc EmEn::Overlay::Surface::onPointerLeave() */
			void onPointerLeave (float positionX, float positionY) noexcept override;

			/** @copydoc EmEn::Overlay::Surface::onButtonPress() */
			bool onButtonPress (float positionX, float positionY, int32_t buttonNumber, int32_t modifiers) noexcept override;

			/** @copydoc EmEn::Overlay::Surface::onButtonRelease() */
			bool onButtonRelease (float positionX, float positionY, int32_t buttonNumber, int32_t modifiers) noexcept override;

			/**
			 * @brief Gives access to the TextWriter for configuration.
			 * @warning  Don't use it to write on the surface.
			 * @return Graphics::TextWriter &
			 */
			Graphics::TextWriter & textWriter () noexcept;

			/**
			 * @brief Sets the label of the button.
			 * @note The label value.
			 * @return void
			 */
			void setLabel (const std::string & label) noexcept;

			/**
			 * @brief Returns the label of the button.
			 * @return const std::string &
			 */
			[[nodiscard]]
			const std::string & label () const noexcept;

			/**
			 * @brief Sets the background image for the button.
			 * @param image A reference to an image resource smart pointer.
			 * @return void
			 */
			void setImage (const std::shared_ptr< Graphics::ImageResource > & image) noexcept;

			/**
			 * @brief Returns the background image.
			 * @return std::shared_ptr< Graphics::ImageResource >
			 */
			[[nodiscard]]
			std::shared_ptr< Graphics::ImageResource > image () const noexcept;

			/**
			 * @brief Sets a function to execute when the button is clicked.
			 * @param function A reference to a function.
			 * @return void
			 */
			void attachFunction (const Function & function) noexcept;

		private:

			/**
			 * @brief Declares the draw finished on the surface.
			 * @param pixmap A reference to a pixmap.
			 * @return bool
			 */
			bool onDrawFinished (Libs::PixelFactory::Pixmap< uint8_t > & pixmap) noexcept override;

			std::shared_ptr< Graphics::ImageResource > m_image{};
			std::string m_label{};
			Graphics::TextWriter m_textWriter{};
			Function m_function{};
			Status m_status{Status::Normal};
	};
}
