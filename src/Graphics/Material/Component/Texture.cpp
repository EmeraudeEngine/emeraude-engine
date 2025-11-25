/*
 * src/Graphics/Material/Component/Texture.cpp
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

#include "Texture.hpp"

/* STL inclusions. */
#include <utility>

/* Local inclusions. */
#include "Resources/Manager.hpp"
#include "Graphics/TextureResource/Texture1D.hpp"
#include "Graphics/TextureResource/Texture2D.hpp"
#include "Graphics/TextureResource/Texture3D.hpp"
#include "Graphics/TextureResource/TextureCubemap.hpp"
#include "Graphics/TextureResource/AnimatedTexture2D.hpp"
#include "Tracer.hpp"

namespace EmEn::Graphics::Material::Component
{
	using namespace Libs;
	using namespace Libs::Math;
	using namespace Saphir;

	Texture::Texture (const char * samplerName, std::string variableName, const Json::Value & data, const FillingType & fillingType, Resources::AbstractServiceProvider & serviceProvider) noexcept
		: m_samplerName{samplerName},
		m_variableName{std::move(variableName)}
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
				m_textureResource = serviceProvider.container< TextureResource::Texture1D >()->getResource(textureResourceName);
				break;

			case FillingType::Texture :
				m_textureResource = serviceProvider.container< TextureResource::Texture2D >()->getResource(textureResourceName);
				break;

			case FillingType::VolumeTexture :
				m_textureResource = serviceProvider.container< TextureResource::Texture3D >()->getResource(textureResourceName);
				break;

			case FillingType::Cubemap :
				m_textureResource = serviceProvider.container< TextureResource::TextureCubemap >()->getResource(textureResourceName);
				break;

			case FillingType::AnimatedTexture :
				m_textureResource = serviceProvider.container< TextureResource::AnimatedTexture2D >()->getResource(textureResourceName);
				break;

			case FillingType::Value :
			case FillingType::Color :
			case FillingType::None :
			case FillingType::AlphaChannelAsValue :
				Tracer::error(ClassId, "Invalid texture type !");

				return;
		}

		if ( m_textureResource == nullptr )
		{
			TraceError{ClassId} << "Unable to find " << to_cstring(fillingType) << " '" << textureResourceName << "' !";

			return;
		}

		/* NOTE: Make a copy for the texture interface smart-point. */
		m_texture = m_textureResource;

		/* Check the optional UVW channel. */
		if ( data.isMember(JKChannel) )
		{
			if ( const auto & jsonNode = data[JKChannel]; jsonNode.isNumeric() )
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
			if ( const auto & jsonNode = data[JKUVWScale]; jsonNode.isArray() )
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
			if ( const auto & jsonNode = data[JKEnableAlpha]; jsonNode.isBool() )
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

		/* NOTE: A texture component must have the texture interface! */
		if ( m_texture == nullptr )
		{
			return false;
		}

		/* NOTE: A texture component can only create a texture
		 * on hardware if a texture resource is used. */
		if ( m_textureResource != nullptr )
		{
			if ( m_textureResource->isCreated() )
			{
				return true;
			}

			return m_textureResource->createTexture(renderer);
		}

		return m_texture->isCreated();
	}

	Key
	Texture::textureType () const noexcept
	{
		using namespace Saphir::Keys;

		switch ( m_texture->type() )
		{
			case Vulkan::TextureType::Texture1D :
				return GLSL::Sampler1D;

			case Vulkan::TextureType::Texture2D :
				return GLSL::Sampler2D;

			case Vulkan::TextureType::Texture3D :
				return GLSL::Sampler3D;

			case Vulkan::TextureType::TextureCube :
				return GLSL::SamplerCube;

			case Vulkan::TextureType::Texture1DArray :
				return GLSL::Sampler1DArray;

			case Vulkan::TextureType::Texture2DArray :
				return GLSL::Sampler2DArray;

			case Vulkan::TextureType::TextureCubeArray :
				return GLSL::SamplerCubeArray;
		}

		Tracer::error(ClassId, "Unable to determine texture type !");

		return nullptr;
	}
}
