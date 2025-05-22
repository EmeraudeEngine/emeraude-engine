/*
 * src/Graphics/Material/Component/Value.hpp
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
#include "Interface.hpp"

namespace EmEn::Graphics::Material::Component
{
	/**
	 * @brief The value component type.
	 * @note The value is not hold by this object. It exists only to be the counter-part of TypeTexture and TypeColor.
	 * @extends EmEn::Graphics::Material::Component::Interface This class describe a component type.
	 */
	class Value final : public Interface
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"Value"};

			/**
			 * @brief Constructs a value component.
			 * @param variableName A string [std::move].
			 */
			explicit Value (std::string variableName) noexcept;

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
				return Type::Value;
			}

			/** @copydoc EmEn::Graphics::Material::Component::Interface::isOpaque() */
			[[nodiscard]]
			bool
			isOpaque () const noexcept override
			{
				return true;
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
			 * @brief STL streams printable object.
			 * @param out A reference to the stream output.
			 * @param obj A reference to the object to print.
			 * @return std::ostream &
			 */
			friend std::ostream & operator<< (std::ostream & out, const Value & obj);

			/**
			 * @brief Stringify the object.
			 * @param obj A reference to the object to print.
			 * @return std::string
			 */
			friend std::string to_string (const Value & obj) noexcept;

		private:

			std::string m_variableName;
	};
}
