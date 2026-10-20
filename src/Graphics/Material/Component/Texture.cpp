/*
 * src/Graphics/Material/Component/Texture.cpp
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

#include "Texture.hpp"

/* STL inclusions. */
#include <cstdint>
#include <optional>
#include <sstream>
#include <string>
#include <utility>

/* Local inclusions. */
#include "FastJSON.hpp"
#include "Graphics/ImageResource.hpp"
#include "Graphics/TextureResource/AnimatedTexture2D.hpp"
#include "Graphics/TextureResource/StoreTexture.hpp"
#include "Graphics/TextureResource/Texture1D.hpp"
#include "Graphics/TextureResource/Texture2D.hpp"
#include "Graphics/TextureResource/Texture3D.hpp"
#include "Graphics/TextureResource/TextureCubemap.hpp"
#include "Graphics/Types.hpp"
#include "Logging/Severity.hpp"
#include "Resources/Container.hpp"
#include "Resources/ResourceTrait.hpp"
#include "Resources/Types.hpp"
#include "Tracer.hpp"

namespace EmEn::Graphics::Material::Component
{
	using namespace Base;
	using namespace Base::Math;
	using namespace Saphir;

	bool
	Texture::hasExpectedColorSpace (const std::string & variableName, const std::shared_ptr< TextureResource::Abstract > & texture, PixelFactory::Channel sourceChannel) noexcept
	{
		if ( texture == nullptr )
		{
			TraceError{ClassId} << "No texture resource for the material variable '" << variableName << "' !";

			return false;
		}

		/* NOTE: The default stand-in, and an alpha read (linear in every format), fit either colour space. */
		if ( texture->name() == Resources::Default || sourceChannel == PixelFactory::Channel::Alpha )
		{
			return true;
		}

		const bool expectsSRGB = variableName.ends_with("Color");

		if ( texture->isSRGB() == expectsSRGB )
		{
			return true;
		}

		TraceError{ClassId} <<
			"The texture '" << texture->name() << "' is " << ( texture->isSRGB() ? "sRGB" : "linear" ) << ", "
			"the material variable '" << variableName << "' samples a " << ( expectsSRGB ? "colour (sRGB)" : "data (linear)" ) << " texture ! "
			"Request it with TextureResource::getStoreTexture(), or create it with that colour space.";

		return false;
	}

	Texture::Texture (const char * samplerName, std::string variableName, const Json::Value & data, const FillingType & fillingType, Resources::AbstractServiceProvider & serviceProvider, Base::PixelFactory::Channel defaultSourceChannel, bool normalMap) noexcept
		: m_samplerName{samplerName},
		m_variableName{std::move(variableName)},
		m_sourceChannel{defaultSourceChannel}
	{
		if ( !data.isObject() || !data.isMember(JKResourceName) )
		{
			TraceError{ClassId} << "There is no '" << JKResourceName << "' key in Json structure !";

			return;
		}

		const auto resourceName = FastJSON::getValue< std::string >(data, JKResourceName);

		if ( !resourceName.has_value() )
		{
			TraceError{ClassId} << "The key '" << JKResourceName << "' key in Json structure must be a string !";

			return;
		}

		const auto usage = this->parseUsage(data, normalMap);

		if ( !usage.has_value() )
		{
			return;
		}

		m_textureResource = findStoreTexture(fillingType, *resourceName, *usage, serviceProvider);

		if ( m_textureResource == nullptr )
		{
			TraceError{ClassId} << "Unable to find " << to_cstring(fillingType) << " '" << *resourceName << "' !";

			return;
		}

		/* NOTE: Make a copy for the texture interface smart-point. */
		m_texture = m_textureResource;

		this->parseSamplingOptions(data);
	}

	std::optional< TextureResource::Usage >
	Texture::parseUsage (const Json::Value & data, bool normalMap) const noexcept
	{
		/* NOTE: The colour space and the normal map flip are part of the texture's IDENTITY (owner decision
		 * 2026-10-14): they are requested here, from the usage, and getStoreTexture() hands out the variant created
		 * with them — never written on the shared store texture, where a second material would change them for the
		 * first one, or the write would race the upload on a pool worker.
		 * Convention: a material variable name ending with "Color" is a perceptual colour (albedo, emissive...)
		 * encoded in sRGB, decoded to linear by the GPU on sampling. "FlipNormalMapY": true converts a
		 * Direct3D-style normal map (+Y down) into the Khronos convention. A normal map is BC5 (graphics § 39, D8). */
		TextureResource::Usage usage{};
		usage.sRGB = m_variableName.ends_with("Color");
		usage.normalMap = normalMap;

		if ( data.isMember(JKFlipNormalMapY) )
		{
			const auto flip = FastJSON::getValue< bool >(data, JKFlipNormalMapY);

			if ( !flip.has_value() )
			{
				TraceError{ClassId} << "The key '" << JKFlipNormalMapY << "' in Json structure must be a boolean !";

				return std::nullopt;
			}

			usage.flipNormalMapY = *flip;
		}

		return usage;
	}

	std::shared_ptr< TextureResource::Abstract >
	Texture::findStoreTexture (FillingType fillingType, const std::string & resourceName, const TextureResource::Usage & usage, Resources::AbstractServiceProvider & serviceProvider) noexcept
	{
		/* NOTE: A PACKED texture (graphics § 39) is no store file: the material's planner created it with its own
		 * identity (colour space, normal flip), which the usage of one of its readers does not change — the opacity
		 * reads the A of an sRGB albedo, and alpha is linear in every format. */
		if ( fillingType == FillingType::Texture && resourceName.starts_with(PackRecipe::NamePrefix) )
		{
			auto * container = serviceProvider.container< TextureResource::Texture2D >();

			if ( container == nullptr || !container->isResourceExists(resourceName) )
			{
				TraceError{ClassId} << "The packed texture '" << resourceName << "' was not created by its material !";

				return nullptr;
			}

			return container->getResource(resourceName);
		}

		switch ( fillingType )
		{
			case FillingType::Gradient :
				return TextureResource::getStoreTexture(*serviceProvider.container< TextureResource::Texture1D >(), resourceName, usage);

			case FillingType::Texture :
				return TextureResource::getStoreTexture(*serviceProvider.container< TextureResource::Texture2D >(), resourceName, usage);

			case FillingType::VolumeTexture :
				return TextureResource::getStoreTexture(*serviceProvider.container< TextureResource::Texture3D >(), resourceName, usage);

			case FillingType::Cubemap :
				return TextureResource::getStoreTexture(*serviceProvider.container< TextureResource::TextureCubemap >(), resourceName, usage);

			case FillingType::AnimatedTexture :
				return TextureResource::getStoreTexture(*serviceProvider.container< TextureResource::AnimatedTexture2D >(), resourceName, usage);

			case FillingType::Value :
			case FillingType::Color :
			case FillingType::None :
			case FillingType::AlphaChannelAsValue :
			case FillingType::Automatic :
				break;
		}

		Tracer::error(ClassId, "Invalid texture type !");

		return nullptr;
	}

	void
	Texture::parseVector3 (const Json::Value & data, const char * key, Vector< 3, float > & target) noexcept
	{
		if ( !data.isMember(key) )
		{
			return;
		}

		const auto & jsonNode = data[key];

		if ( !jsonNode.isArray() )
		{
			TraceError{ClassId} << "The '" << key << "' key must be a numeric value array ! ";

			return;
		}

		for ( uint32_t index = 0; index < 3; index++ )
		{
			const auto value = FastJSON::asValue< float >(jsonNode[index]);

			if ( !value.has_value() )
			{
				TraceError{ClassId} << "Json array #" << index << " value is not a finite number !";

				return;
			}

			target[index] = *value;
		}
	}

	void
	Texture::parseSamplingOptions (const Json::Value & data) noexcept
	{
		/* Check the optional UVW channel. */
		if ( data.isMember(JKChannel) )
		{
			if ( const auto channel = FastJSON::asValue< uint32_t >(data[JKChannel]); channel.has_value() )
			{
				m_UVWChannel = *channel;
			}
			else
			{
				TraceWarning{ClassId} <<
					"The '" << JKChannel << "' key in Json structure is not an unsigned integer ! "
					"Leaving UVW channel to 0 ...";
			}
		}

		/* Check the optional source color channel (scalar components reading a packed texture). */
		if ( data.isMember(JKSourceChannel) )
		{
			if ( const auto channel = FastJSON::asValue< uint32_t >(data[JKSourceChannel]); channel.has_value() && *channel <= 3 )
			{
				m_sourceChannel = static_cast< Base::PixelFactory::Channel >(*channel);
			}
			else
			{
				TraceWarning{ClassId} <<
					"The '" << JKSourceChannel << "' key in Json structure is not numeric (0:R, 1:G, 2:B, 3:A) ! "
					"Leaving the component's default source channel ...";
			}
		}

		/* Check the optional UVW scale, then the offset (uv * scale + offset). */
		parseVector3(data, JKUVWScale, m_UVWScale);
		parseVector3(data, JKUVWOffset, m_UVWOffset);

		if ( data.isMember(JKEnableAlpha) )
		{
			if ( const auto enabled = FastJSON::asValue< bool >(data[JKEnableAlpha]); enabled.has_value() )
			{
				m_alphaEnabled = *enabled;
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

	std::ostream &
	operator<< (std::ostream & out, const Texture & obj)
	{
		return out << Texture::ClassId << "." "\n"
			"Texture uniform name: " << obj.m_samplerName << "\n"
			"Variable name: " << obj.m_variableName << "\n"
			"Texture type (component level): " << obj.textureType() << "\n"
			"Is volumetric texture ? (component level): " << ( obj.isVolumetricTexture() ? "yes" : "no" ) << "\n"
			"Texture resource name: " << ( obj.m_textureResource != nullptr ? obj.m_textureResource->name() : "N/A (TextureInterface)" ) << "\n"
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
