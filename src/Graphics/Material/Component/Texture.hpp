/*
 * src/Graphics/Material/Component/Texture.hpp
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

/* Project configuration. */
#include "emeraude_export.hpp"

/* Project configuration. */
#include "emeraude_config.hpp"

/* STL inclusions. */
#include <cstdint>
#include <memory>
#include <string>

/* Local inclusions for inheritances. */
#include "Interface.hpp"

/* Local inclusions for usages. */
#include "Graphics/TextureResource/Abstract.hpp"
#include "Graphics/Types.hpp"
#include "Math/Vector.hpp"
#include "PixelFactory/Types.hpp"
#include "Saphir/Keys.hpp"

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
	class EMEN_API Texture final : public Interface
	{
		public:

			static constexpr auto ClassId{"Texture"};

			/**
			 * @brief Constructs a texture component from a resource.
			 * @param samplerName A C-string for the name of the sampler uniform.
			 * @param variableName A string [std::move].
			 * @param texture A reference to a texture resource smart pointer.
			 * @param UVWChannel The texture channel to use on geometry. Default 0.
			 * @param UVWScale A reference to a vector to scale the texture coordinates. Default 1.0 in all directions.
			 * @param enableAlpha Enable the alpha channel for opacity/blending. Request a 4-channel texture. Default false.
			 */
			Texture (const char * samplerName, std::string variableName, const std::shared_ptr< TextureResource::Abstract > & texture, uint32_t UVWChannel = 0, const Base::Math::Vector< 3, float > & UVWScale = {1.0F, 1.0F, 1.0F}, bool enableAlpha = false) noexcept
				: m_samplerName{samplerName},
				m_variableName{std::move(variableName)},
				m_texture{texture},
				m_textureResource{texture},
				m_UVWScale{UVWScale},
				m_UVWChannel{UVWChannel},
				m_alphaEnabled{enableAlpha}
			{
				/* Convention: material variable names ending with "Color" represent
				 * perceptual color data (albedo, emissive, etc.) encoded in sRGB.
				 * The GPU will automatically convert sRGB to linear on sampling. */
				if ( m_textureResource != nullptr )
				{
					m_textureResource->enableSRGB(m_variableName.ends_with("Color"));
				}
			}

			/**
			 * @brief Constructs a texture component from a texture interface (lower-level).
			 * @param samplerName A C-string for the name of the sampler uniform.
			 * @param variableName A string [std::move].
			 * @param texture A reference to a texture interface smart pointer.
			 * @param UVWChannel The texture channel to use on geometry. Default 0.
			 * @param UVWScale A reference to a vector to scale the texture coordinates. Default 1.0 in all directions.
			 * @param enableAlpha Enable the alpha channel for opacity/blending. Request a 4-channel texture. Default false.
			 */
			Texture (const char * samplerName, std::string variableName, const std::shared_ptr< Vulkan::TextureInterface > & texture, uint32_t UVWChannel = 0, const Base::Math::Vector< 3, float > & UVWScale = {1.0F, 1.0F, 1.0F}, bool enableAlpha = false) noexcept
				: m_samplerName{samplerName},
				m_variableName{std::move(variableName)},
				m_texture{texture},
				m_textureResource{nullptr},
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
			Texture (const char * samplerName, std::string variableName, const Json::Value & data, const FillingType & fillingType, Resources::AbstractServiceProvider & serviceProvider) noexcept;

			/** @copydoc EmEn::Graphics::Material::Component::Interface::create() */
			[[nodiscard]]
			bool create (Renderer & renderer, uint32_t & binding) noexcept override;

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

			/** @copydoc EmEn::Graphics::Material::Component::Interface::texture() const noexcept */
			[[nodiscard]]
			std::shared_ptr< Vulkan::TextureInterface >
			texture () const noexcept override
			{
				return m_texture;
			}

			/** @copydoc EmEn::Graphics::Material::Component::Interface::textureResource() const noexcept */
			[[nodiscard]]
			std::shared_ptr< TextureResource::Abstract >
			textureResource () const noexcept override
			{
				return m_textureResource;
			}

			/** @copydoc EmEn::Graphics::Material::Component::Interface::getSampler() */
			[[nodiscard]]
			Saphir::Declaration::Sampler
			getSampler (uint32_t materialSet) const noexcept override
			{
				return {materialSet, this->binding(), this->textureType(), this->samplerName()};
			}

			/**
			 * @brief Sets a texture interface.
			 * @note No resource loading behavior, useful for render-to-texture.
			 * @param texture A reference to a texture interface smart pointer.
			 * @return void
			 */
			void
			setTexture (const std::shared_ptr< Vulkan::TextureInterface > & texture) noexcept
			{
				m_texture = texture;
				m_textureResource.reset();
			}

			/**
			 * @brief Sets a texture resource with a loading dependency.
			 * @param texture A reference to a texture resource smart pointer.
			 * @return void
			 */
			void
			setTextureResource (const std::shared_ptr< TextureResource::Abstract > & texture) noexcept
			{
				m_texture = texture;
				m_textureResource = texture;
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
			 * @brief Rotates the texture coordinates.
			 * @param radians The rotation in radians (KHR_texture_transform's `rotation`).
			 * @return void
			 */
			void
			setUVWRotation (float radians) noexcept
			{
				m_UVWRotation = radians;
			}

			/**
			 * @brief Returns the UVW rotation, in radians.
			 * @note KHR_texture_transform's `rotation`, applied AFTER the scale and BEFORE the
			 * offset — the extension composes its matrix as translation * rotation * scale.
			 * @return float
			 */
			[[nodiscard]]
			float
			UVWRotation () const noexcept
			{
				return m_UVWRotation;
			}

			/**
			 * @brief Sets the UVW scale.
			 * @param UVWScale A reference to a vector.
			 * @return void
			 */
			void
			setUVWScale (const Base::Math::Vector< 3, float > & UVWScale) noexcept
			{
				m_UVWScale = UVWScale;
			}

			/**
			 * @brief Offsets the texture coordinates (KHR_texture_transform 'offset').
			 * @note Applied AFTER the scale in the shader: uv * scale + offset.
			 * @param UVWOffset A reference to a vector to offset the texture coordinates.
			 * @return void
			 */
			void
			setUVWOffset (const Base::Math::Vector< 3, float > & UVWOffset) noexcept
			{
				m_UVWOffset = UVWOffset;
			}

			/**
			 * @brief Returns the texture coordinates offset.
			 * @return const Base::Math::Vector< 3, float > &
			 */
			[[nodiscard]]
			const Base::Math::Vector< 3, float > &
			UVWOffset () const noexcept
			{
				return m_UVWOffset;
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
			 * @return const Base::Math::Vector< 3, float > &
			 */
			[[nodiscard]]
			const Base::Math::Vector< 3, float > &
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
				if constexpr ( IsDebug )
				{
					if ( m_texture == nullptr )
					{
						TraceError{ClassId} << "The texture interface is nullptr!";

						return false;
					}
				}

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
			 * @brief Sets the color channel a scalar component reads from the sampled texel.
			 * @note Default is Red (grayscale/single-channel maps). Packed textures select
			 * their channel here, e.g. glTF metallic-roughness: roughness = Green, metallic = Blue.
			 * @param channel The source color channel.
			 * @return void
			 */
			void
			setSourceChannel (Base::PixelFactory::Channel channel) noexcept
			{
				m_sourceChannel = channel;
			}

			/**
			 * @brief Returns the color channel a scalar component reads from the sampled texel.
			 * @return EmEn::Base::PixelFactory::Channel
			 */
			[[nodiscard]]
			Base::PixelFactory::Channel
			sourceChannel () const noexcept
			{
				return m_sourceChannel;
			}

			/**
			 * @brief Returns the GLSL swizzle selector matching the source channel.
			 * @return const char *
			 */
			[[nodiscard]]
			const char *
			sourceChannelSwizzle () const noexcept
			{
				switch ( m_sourceChannel )
				{
					case Base::PixelFactory::Channel::Green :
						return "g";

					case Base::PixelFactory::Channel::Blue :
						return "b";

					case Base::PixelFactory::Channel::Alpha :
						return "a";

					case Base::PixelFactory::Channel::Red :
					default :
						return "r";
				}
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
			friend EMEN_API std::ostream & operator<< (std::ostream & out, const Texture & obj);

			/* JSON key. */
			static constexpr auto JKResourceName{"Name"};
			static constexpr auto JKChannel{"Channel"};
			static constexpr auto JKSourceChannel{"SourceChannel"};
			static constexpr auto JKUVWScale{"UVW"};
			static constexpr auto JKUVWOffset{"UVWOffset"};
			static constexpr auto JKEnableAlpha{"EnableAlpha"};

			const char * m_samplerName;
			std::string m_variableName;
			std::shared_ptr< Vulkan::TextureInterface > m_texture;
			std::shared_ptr< TextureResource::Abstract > m_textureResource;
			Base::Math::Vector< 3, float > m_UVWScale{1.0F, 1.0F, 1.0F};
			Base::Math::Vector< 3, float > m_UVWOffset{0.0F, 0.0F, 0.0F};
			uint32_t m_UVWChannel{0};
			uint32_t m_binding{0};
			Base::PixelFactory::Channel m_sourceChannel{Base::PixelFactory::Channel::Red};
			float m_UVWRotation{0.0F};
			bool m_alphaEnabled{false};
	};

	/**
	 * @brief Stringifies the object.
	 * @param obj A reference to the object to print.
	 * @return std::string
	 */
	EMEN_API std::string to_string (const Texture & obj) noexcept;
}
