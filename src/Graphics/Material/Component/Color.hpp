/*
 * src/Graphics/Material/Component/Color.hpp
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
#include <memory>
#include <string>

/* Local inclusions for inheritances. */
#include "Interface.hpp"

/* Local inclusions for usages. */
#include "Libs/PixelFactory/Color.hpp"

namespace EmEn::Graphics::Material::Component
{
	/**
	 * @brief The color component type.
	 * @extends EmEn::Graphics::Material::Component::Interface This class describe a component type.
	 */
	class Color : public Interface
	{
		public:

			static constexpr auto ClassId{"Color"};

			/**
			 * @brief Constructs a color component.
			 * @param variableName A string [std::move].
			 * @param color A reference to a color. Default black.
			 */
			explicit Color (std::string variableName, const Libs::PixelFactory::Color< float > & color = {}) noexcept;

			/** @copydoc EmEn::Graphics::Material::Component::Interface::create() */
			[[nodiscard]]
			bool
			create (Renderer & /*renderer*/, uint32_t & /*binding*/) noexcept override
			{
				return true;
			}

			/** @copydoc EmEn::Graphics::Material::Component::Interface::isCreated() */
			[[nodiscard]]
			bool
			isCreated () const noexcept override
			{
				return true;
			}

			/** @copydoc EmEn::Graphics::Material::Component::Interface::variableName() */
			[[nodiscard]]
			const std::string &
			variableName () const noexcept override
			{
				return m_variableName;
			}

			/** @copydoc EmEn::Graphics::Material::Component::Interface::type() */
			[[nodiscard]]
			Type
			type () const noexcept override
			{
				return Type::Color;
			}

			/** @copydoc EmEn::Graphics::Material::Component::Interface::isOpaque() */
			[[nodiscard]]
			bool
			isOpaque () const noexcept override
			{
				return m_color.alpha() >= 1.0F;
			}

			/** @copydoc EmEn::Graphics::Material::Component::Interface::textureResource() */
			[[nodiscard]]
			std::shared_ptr< TextureResource::Abstract >
			textureResource () const noexcept override
			{
				return nullptr;
			}

			/** @copydoc EmEn::Graphics::Material::Component::Interface::getSampler() */
			[[nodiscard]]
			Saphir::Declaration::Sampler
			getSampler (uint32_t /*materialSet*/) const noexcept override
			{
				return {0, 0, nullptr, nullptr};
			}

			/**
			 * @brief Sets a new color.
			 * @param color A reference to a color.
			 * @return void
			 */
			void
			setColor (const Libs::PixelFactory::Color< float > & color) noexcept
			{
				m_color = color;
			}

			/**
			 * @brief Returns the current color.
			 * @return const Libraries::PixelFactory::Color< float > &
			 */
			[[nodiscard]]
			const Libs::PixelFactory::Color< float > &
			color () const noexcept
			{
				return m_color;
			}

			/**
			 * @brief Returns the current color.
			 * @return const Libraries::PixelFactory::Color< float > &
			 */
			[[nodiscard]]
			Libs::PixelFactory::Color< float > &
			color () noexcept
			{
				return m_color;
			}

			/**
			 * @brief Changes the opacity value of the color.
			 * @param value An opacity value between 0.0 and 1.0
			 * @return void
			 */
			void
			setOpacity (float value) noexcept
			{
				m_color.setAlpha(value);
			}

			/**
			 * @brief Returns the opacity value of the color.
			 * @return float
			 */
			[[nodiscard]]
			float
			opacity () const noexcept
			{
				return m_color.alpha();
			}

			/**
			 * @brief STL streams printable object.
			 * @param out A reference to the stream output.
			 * @param obj A reference to the object to print.
			 * @return std::ostream &
			 */
			friend std::ostream & operator<< (std::ostream & out, const Color & obj);

			/**
			 * @brief Stringifies the object.
			 * @param obj A reference to the object to print.
			 * @return std::string
			 */
			friend std::string to_string (const Color & obj) noexcept;

		private:

			std::string m_variableName;
			Libs::PixelFactory::Color< float > m_color;
	};
}
