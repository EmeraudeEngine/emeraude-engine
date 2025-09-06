/*
 * src/Graphics/Material/Component/Texture.hpp
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
#include "Libs/Math/Vector.hpp"
#include "Graphics/TextureResource/Abstract.hpp"
#include "Graphics/Types.hpp"
#include "Saphir/Keys.hpp"

/* Forward declarations */
namespace EmEn::Resources
{
	class Manager;
}

namespace EmEn::Graphics::Material::Component
{
	/**
	 * @brief The texture component type.
	 * @extends EmEn::Graphics::Material::Component::Interface This class describes a component type.
	 */
	class Texture final : public Interface
	{
		public:

			static constexpr auto ClassId{"Texture"};

			/**
			 * @brief Constructs a texture component.
			 * @param samplerName A C-string for the name of the sampler uniform.
			 * @param variableName A string [std::move].
			 * @param texture A reference to a texture resource smart pointer.
			 * @param UVWChannel The texture channel to use on geometry. Default 0.
			 * @param UVWScale A reference to a vector to scale the texture coordinates. Default 1.0 in all directions.
			 * @param enableAlpha Enable the alpha channel for opacity/blending. Request a 4-channel texture. Default false.
			 */
			Texture (const char * samplerName, std::string variableName, const std::shared_ptr< TextureResource::Abstract > & texture, uint32_t UVWChannel = 0, const Libs::Math::Vector< 3, float > & UVWScale = {1.0F, 1.0F, 1.0F}, bool enableAlpha = false) noexcept
				: m_samplerName{samplerName},
				m_variableName{std::move(variableName)},
				m_texture{texture},
				m_UVWScale{UVWScale},
				m_UVWChannel{UVWChannel},
				m_alphaEnabled{enableAlpha}
			{

			}

			/**
			 * @brief Constructs a texture component from json data.
			 * @param samplerName A C-string for the name of the sampler uniform.
			 * @param variableName A string [std::move].
			 * @param data A reference to a JSON value.
			 * @param fillingType A reference to a texture filling type.
			 * @param serviceProvider A reference to the resource manager through a service provider.
			 */
			Texture (const char * samplerName, std::string variableName, const Json::Value & data, const FillingType & fillingType, Resources::ServiceProvider & serviceProvider) noexcept;

			/** @copydoc EmEn::Graphics::Material::Component::Interface::create() */
			[[nodiscard]]
			bool create (Renderer & renderer, uint32_t & binding) noexcept override;

			/** @copydoc EmEn::Graphics::Material::Component::Interface::isCreated() */
			[[nodiscard]]
			bool
			isCreated () const noexcept override
			{
				if ( m_texture == nullptr )
				{
					return false;
				}

				return m_texture->isCreated();
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
				return Type::Texture;
			}

			/** @copydoc EmEn::Graphics::Material::Component::Interface::isOpaque() */
			[[nodiscard]]
			bool
			isOpaque () const noexcept override
			{
				return !m_alphaEnabled;
			}

			/** @copydoc EmEn::Graphics::Material::Component::Interface::textureResource() */
			[[nodiscard]]
			std::shared_ptr< TextureResource::Abstract >
			textureResource () const noexcept override
			{
				return m_texture;
			}

			/** @copydoc EmEn::Graphics::Material::Component::Interface::getSampler() */
			[[nodiscard]]
			Saphir::Declaration::Sampler
			getSampler (uint32_t materialSet) const noexcept override
			{
				return {materialSet, this->binding(), this->textureType(), this->samplerName()};
			}

			/**
			 * @brief Sets a texture.
			 * @param texture A reference to a texture resource smart pointer.
			 * @return void
			 */
			void
			setTexture (const std::shared_ptr< TextureResource::Abstract > & texture) noexcept
			{
				m_texture = texture;
			}

			/**
			 * @brief Changes the texture channel.
			 * @param UVWChannel The texture channel to use on geometry.
			 * @return void
			 */
			void
			setUVWChannel (uint32_t UVWChannel) noexcept
			{
				m_UVWChannel = UVWChannel;
			}

			/**
			 * @brief Scales the texture coordinates.
			 * @param UVWScale A reference to a vector to scale the texture coordinates.
			 * @return void
			 */
			void
			setUVWScale (const Libs::Math::Vector< 3, float > & UVWScale) noexcept
			{
				m_UVWScale = UVWScale;
			}

			/**
			 * @brief Returns the texture channel.
			 * @return uint32_t
			 */
			[[nodiscard]]
			uint32_t
			UVWChannel () const noexcept
			{
				return m_UVWChannel;
			}

			/**
			 * @brief Returns the texture coordinates scale.
			 * @return const Libs::Math::Vector< 3, float > &
			 */
			[[nodiscard]]
			const Libs::Math::Vector< 3, float > &
			UVWScale () const noexcept
			{
				return m_UVWScale;
			}

			/**
			 * @brief Returns whether the texture is volumetric and needs 3D coordinates.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isVolumetricTexture () const noexcept
			{
				return m_texture->request3DTextureCoordinates();
			}

			/**
			 * @brief Returns the GLSL type of texture.
			 * @return Saphir::Key
			 */
			[[nodiscard]]
			Saphir::Key textureType () const noexcept;

			/**
			 * @brief Enables the alpha channel of the texture for opacity/blending.
			 * @param state The state.
			 * @return void
			 */
			void
			enableAlpha (bool state) noexcept
			{
				m_alphaEnabled = state;
			}

			/**
			 * @brief Returns whether the alpha channel of the texture is used for opacity/blending.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			alphaEnabled () const noexcept
			{
				return m_alphaEnabled;
			}

			/**
			 * @brief Returns the binding point for the texture.
			 * @return uint32_t
			 */
			[[nodiscard]]
			uint32_t
			binding () const noexcept
			{
				return m_binding;
			}

			/**
			 * @brief Returns the name of the sampler uniform.
			 * @return const char *
			 */
			[[nodiscard]]
			const char *
			samplerName () const noexcept
			{
				return m_samplerName;
			}

		private:

			/**
			 * @brief STL streams printable object.
			 * @param out A reference to the stream output.
			 * @param obj A reference to the object to print.
			 * @return std::ostream &
			 */
			friend std::ostream & operator<< (std::ostream & out, const Texture & obj);

			/* JSON key. */
			static constexpr auto JKResourceName{"Name"};
			static constexpr auto JKChannel{"Channel"};
			static constexpr auto JKUVWScale{"UVW"};
			static constexpr auto JKEnableAlpha{"EnableAlpha"};

			const char * m_samplerName;
			std::string m_variableName;
			std::shared_ptr< TextureResource::Abstract > m_texture{};
			Libs::Math::Vector< 3, float > m_UVWScale{1.0F, 1.0F, 1.0F};
			uint32_t m_UVWChannel{0};
			uint32_t m_binding{0};
			bool m_alphaEnabled{false};
	};

	inline
	std::ostream &
	operator<< (std::ostream & out, const Texture & obj)
	{
		return out << Texture::ClassId << "." "\n"
			"Texture uniform name: " << obj.m_samplerName << "\n"
			"Variable name: " << obj.m_variableName << "\n"
			"Texture type (component level): " << obj.textureType() << "\n"
			"Is volumetric texture ? (component level): " << ( obj.isVolumetricTexture() ? "yes" : "no" ) << "\n"
			"Texture resource name: " << obj.m_texture->name() << "\n"
			"UVW scale: " << obj.m_UVWScale << "\n"
			"Alpha channel enabled: " << ( obj.m_alphaEnabled ? "yes" : "no" ) << "\n"
			"Binding point : " << obj.m_binding << "\n";
	}

	/**
	 * @brief Stringifies the object.
	 * @param obj A reference to the object to print.
	 * @return std::string
	 */
	inline
	std::string
	to_string (const Texture & obj) noexcept
	{
		std::stringstream output;

		output << obj;

		return output.str();
	}
}
