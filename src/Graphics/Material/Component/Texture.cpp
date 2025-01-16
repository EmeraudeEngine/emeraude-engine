/*
 * src/Graphics/Material/Component/Texture.cpp
 * This file is part of Emeraude-Engine
 *
 * Copyright (C) 2010-2024 - "LondNoir" <londnoir@gmail.com>
 *
 * Emeraude-Engine is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Emeraude-Engine is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Emeraude-Engine; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA  02110-1301  USA
 *
 * Complete project and additional information can be found at :
 * https://bitbucket.org/londnoir/emeraude-engine
 *
 * --- THIS IS AUTOMATICALLY GENERATED, DO NOT CHANGE ---
 */

#include "Texture.hpp"

/* STL inclusions. */
#include <utility>

/* Local inclusions. */
#include "Resources/Manager.hpp"
#include "Tracer.hpp"

namespace Emeraude::Graphics::Material::Component
{
	using namespace Libraries;
	using namespace Libraries::Math;
	using namespace Saphir;

	Texture::Texture (const char * samplerName, std::string variableName, const std::shared_ptr< TextureResource::Abstract > & texture, uint32_t UVWChannel, const Vector< 3, float > & UVWScale, bool enableAlpha) noexcept
		: m_samplerName(samplerName), m_variableName(std::move(variableName)), m_texture(texture), m_UVWScale(UVWScale), m_UVWChannel(UVWChannel), m_alphaEnabled(enableAlpha)
	{

	}

	Texture::Texture (const char * samplerName, std::string variableName, const Json::Value & data, const FillingType & fillingType, Resources::Manager & resources) noexcept
		: m_samplerName(samplerName), m_variableName(std::move(variableName))
	{
		if ( !data.isMember(JKResourceName) )
		{
			TraceError{ClassId} << "There is no '" << JKResourceName << "' key in Json structure !";

			return;
		}

		if ( !data[JKResourceName].isString() )
		{
			TraceError{ClassId} << "The key '" << JKResourceName << "' key in Json structure must be a string !";

			return;
		}

		const auto textureResourceName = data[JKResourceName].asString();

		/* Check the texture type. */
		switch ( fillingType )
		{
			case FillingType::Gradient :
				m_texture = resources.texture1Ds().getResource(textureResourceName);
				break;

			case FillingType::Texture :
				m_texture = resources.texture2Ds().getResource(textureResourceName);
				break;

			case FillingType::VolumeTexture :
				m_texture = resources.texture3Ds().getResource(textureResourceName);
				break;

			case FillingType::Cubemap :
				m_texture = resources.textureCubemaps().getResource(textureResourceName);
				break;

			case FillingType::AnimatedTexture :
				m_texture = resources.animatedTextures().getResource(textureResourceName);
				break;

			case FillingType::Value :
			case FillingType::Color :
			case FillingType::None :
			case FillingType::AlphaChannelAsValue :
				Tracer::error(ClassId, "Invalid texture type !");

				return;
		}

		if ( m_texture == nullptr )
		{
			TraceError{ClassId} << "Unable to find " << to_cstring(fillingType) << " '" << textureResourceName << "' !";

			return;
		}

		/* Check the optional UVW channel. */
		if ( data.isMember(JKChannel) )
		{
			const auto & jsonNode = data[JKChannel];

			if ( jsonNode.isNumeric() )
			{
				m_UVWChannel = jsonNode.asUInt();
			}
			else
			{
				TraceWarning{ClassId} <<
					"The '" << JKChannel << "' key in Json structure is not numeric ! "
					"Leaving UVW channel to 0 ...";
			}
		}

		/* Check the optional UVW scale. */
		if ( data.isMember(JKUVWScale) )
		{
			const auto & jsonNode = data[JKUVWScale];

			if ( jsonNode.isArray() )
			{
				for ( auto index = 0; index < 3; index++ )
				{
					if ( !jsonNode[index].isNumeric() )
					{
						TraceError{ClassId} << "Json array #" << index << " value is not numeric !";

						break;
					}

					m_UVWScale[index] = jsonNode[index].asFloat();
				}
			}
			else
			{
				TraceError{ClassId} << "The '" << JKUVWScale << "' key must be a numeric value array ! ";
			}
		}

		if ( data.isMember(JKEnableAlpha) )
		{
			const auto & jsonNode = data[JKEnableAlpha];

			if ( jsonNode.isBool() )
			{
				m_alphaEnabled = jsonNode.asBool();
			}
			else
			{
				TraceError{ClassId} << "The '" << JKEnableAlpha << "' key in Json structure is not a boolean !";
			}
		}
	}

	bool
	Texture::create (Renderer & renderer, uint32_t & binding) noexcept
	{
		m_binding = binding++;

		if ( m_texture == nullptr )
		{
			return false;
		}

		if ( m_texture->isCreated() )
		{
			return true;
		}

		return m_texture->createOnHardware(renderer);
	}

	Key
	Texture::textureType () const noexcept
	{
		using namespace Saphir::Keys;

		switch ( m_texture->type() )
		{
			case TextureResource::Type::Texture1D :
				return GLSL::Sampler1D;

			case TextureResource::Type::Texture2D :
				return GLSL::Sampler2D;

			case TextureResource::Type::Texture3D :
				return GLSL::Sampler3D;

			case TextureResource::Type::TextureCube :
				return GLSL::SamplerCube;

			case TextureResource::Type::Texture1DArray :
				return GLSL::Sampler1DArray;

			case TextureResource::Type::Texture2DArray :
				return GLSL::Sampler2DArray;

			case TextureResource::Type::TextureCubeArray :
				return GLSL::SamplerCubeArray;
		}

		Tracer::error(ClassId, "Unable to determine texture type !");

		return nullptr;
	}

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

	std::string
	to_string (const Texture & obj) noexcept
	{
		std::stringstream output;

		output << obj;

		return output.str();
	}
}
