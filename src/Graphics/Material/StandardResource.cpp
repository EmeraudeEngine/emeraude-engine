/*
 * src/Graphics/Material/StandardResource.cpp
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

#include "StandardResource.hpp"

/* STL inclusions. */
#include <algorithm>
#include <cstdlib>
#include <ranges>
#include <sstream>

/* Local inclusions. */
#include "Component/Color.hpp"
#include "Component/Interface.hpp"
#include "Component/Texture.hpp"
#include "Component/Value.hpp"
#include "GPURTMaterialData.hpp"
#include "Graphics/BindlessTextureManager.hpp"
#include "Graphics/IBLTexture.hpp"
#include "Graphics/Renderer.hpp"
#include "Graphics/Types.hpp"
#include "Helpers.hpp"
#include "FastJSON.hpp"
#include "Math/Base.hpp"
#include "PixelFactory/Color.hpp"
#include "Saphir/Code.hpp"
#include "Saphir/Declaration/Sampler.hpp"
#include "Saphir/Declaration/StageOutput.hpp"
#include "Saphir/Declaration/Types.hpp"
#include "Saphir/Declaration/UniformBlock.hpp"
#include "Saphir/FragmentShader.hpp"
#include "Saphir/Generator/Abstract.hpp"
#include "Saphir/Keys.hpp"
#include "Saphir/LightGenerator.hpp"
#include "Saphir/VertexShader.hpp"
#include "Tracer.hpp"
#include "Vulkan/DescriptorSet.hpp"
#include "Vulkan/DescriptorSetLayout.hpp"
#include "Vulkan/LayoutManager.hpp"
#include "Vulkan/TextureInterface.hpp"

namespace EmEn::Graphics::Material
{
	using namespace Base;
	using namespace Base::Math;
	using namespace Saphir;
	using namespace Saphir::Keys;
	using namespace Graphics::Material::Component;
	using namespace Vulkan;

	bool
	StandardResource::load () noexcept
	{
		if ( !this->beginLoading() )
		{
			return false;
		}

		/* Default PBR material: gray dielectric with medium roughness. */
		this->setAlbedoComponent(DefaultAlbedoColor);
		this->setRoughnessComponent(DefaultRoughness);
		this->setMetalnessComponent(DefaultMetalness);

		return this->setLoadSuccess(true);
	}

	bool
	StandardResource::parseAlbedoComponent (const Json::Value & data, Resources::AbstractServiceProvider & serviceProvider) noexcept
	{
		FillingType fillingType{};
		Json::Value componentData{};

		/* Try "Albedo" first, fallback to "Diffuse" for Standard material compatibility. */
		if ( !parseComponentBase(data, AlbedoString, fillingType, componentData, true) )
		{
			return false;
		}

		if ( fillingType == FillingType::None )
		{
			/* Fallback: try "Diffuse" key from Standard material format. */
			if ( !parseComponentBase(data, DiffuseString, fillingType, componentData, true) )
			{
				return false;
			}
		}

		switch ( fillingType )
		{
			case FillingType::Color :
			{
				const auto color = parseColorComponent(componentData);

				if ( !this->setAlbedoComponent(color) )
				{
					return false;
				}
			}
				return true;

			case FillingType::Gradient :
			case FillingType::Texture :
			case FillingType::VolumeTexture :
			case FillingType::Cubemap :
			case FillingType::AnimatedTexture :
			{
				const auto result = m_components.emplace(ComponentType::Albedo, std::make_unique< Texture >(Uniform::AlbedoSampler, SurfaceAlbedoColor, componentData, fillingType, serviceProvider));

				if ( !result.second || result.first->second == nullptr )
				{
					return false;
				}

				this->enableFlag(TextureEnabled);
				this->enableFlag(UsePrimaryTextureCoordinates);
			}
				return true;

			case FillingType::None :
			default:
				TraceError{ClassId} << "The albedo component (mandatory) is not present or invalid in PBR material '" << this->name() << "' resource JSON file ! Tried both 'Albedo' and 'Diffuse' keys.";

				return false;
		}
	}

	bool
	StandardResource::parseRoughnessComponent (const Json::Value & data, Resources::AbstractServiceProvider & serviceProvider) noexcept
	{
		FillingType fillingType{};
		Json::Value componentData{};

		if ( !parseComponentBase(data, RoughnessString, fillingType, componentData, true) )
		{
			return false;
		}

		switch ( fillingType )
		{
			case FillingType::Value :
			{
				const auto value = parseValueComponent(componentData);

				if ( !this->setRoughnessComponent(value) )
				{
					return false;
				}
			}
				return true;

			case FillingType::Gradient :
			case FillingType::Texture :
			case FillingType::VolumeTexture :
			case FillingType::Cubemap :
			case FillingType::AnimatedTexture :
			{
				const auto result = m_components.emplace(ComponentType::Roughness, std::make_unique< Texture >(Uniform::RoughnessSampler, SurfaceRoughness, componentData, fillingType, serviceProvider));

				if ( !result.second || result.first->second == nullptr )
				{
					return false;
				}

				this->enableFlag(TextureEnabled);
				this->enableFlag(UsePrimaryTextureCoordinates);

				/* NOTE: With a texture component the scalar is the MULTIPLYING factor — neutral 1.0. */
				this->setRoughness(FastJSON::getValue< float >(data[RoughnessString], JKValue).value_or(DefaultTextureFactor));
			}
				return true;

			case FillingType::None :
			{
				/* Fallback: try "Specular" key from Standard material format (inverted). */
				if ( !parseComponentBase(data, SpecularString, fillingType, componentData, true) )
				{
					return false;
				}

				switch ( fillingType )
				{
					case FillingType::Value :
					{
						/* Specular value → Roughness (inverted): high specular = low roughness. */
						const auto specularValue = parseValueComponent(componentData);

						if ( !this->setRoughnessComponent(1.0F - specularValue) )
						{
							return false;
						}
					}
						return true;

					case FillingType::Gradient :
					case FillingType::Texture :
					case FillingType::VolumeTexture :
					case FillingType::Cubemap :
					case FillingType::AnimatedTexture :
					{
						/* Specular texture → Roughness texture (inverted automatically). */
						const auto result = m_components.emplace(ComponentType::Roughness, std::make_unique< Texture >(Uniform::RoughnessSampler, SurfaceRoughness, componentData, fillingType, serviceProvider));

						if ( !result.second || result.first->second == nullptr )
						{
							return false;
						}

						this->enableFlag(TextureEnabled);
						this->enableFlag(UsePrimaryTextureCoordinates);

						/* Auto-invert since we're using a specular/gloss map as roughness source. */
						m_invertRoughness = true;

						/* NOTE: With a texture component the scalar is the MULTIPLYING factor — neutral 1.0. */
						this->setRoughness(FastJSON::getValue< float >(data[SpecularString], JKValue).value_or(DefaultTextureFactor));
					}
						return true;

					case FillingType::Color :
					{
						/* Legacy Phong specular COLOR (material merge, Lot 3). The canonical
						 * conversion is roughness = 1 - glossiness (Khronos archived spec-gloss
						 * extension; the manifest "Shininess" IS an authored GLOSSINESS [0,1] —
						 * the 813ea2ea contract, see StandardResource::specularExponentFromGlossiness()).
						 * ⚠️ The Khronos "F0 = specular color" half is DELIBERATELY not applied:
						 * it presumes spec-gloss-authored data where specular is a dielectric F0
						 * (~0.04). Legacy Phong colors are HIGHLIGHT intensities (bright greys) —
						 * mapped raw to F0 they read near-mirror. For a dielectric, the perceptual
						 * equivalent of a bright Phong highlight is the LOW ROUGHNESS the
						 * glossiness already provides; F0 stays the 0.04 dielectric default. */
						const auto shininessOpt = FastJSON::getValue< float >(data[SpecularString], JKShininess);

						this->setRoughnessComponent(shininessOpt.has_value() ? 1.0F - clampToUnit(shininessOpt.value()) : DefaultRoughness);
					}
						return true;

					case FillingType::None :
					{
						/* Last fallback: a bare Shininess value on the Specular component —
						 * an authored GLOSSINESS [0,1], complement = linear roughness (Lot 3). */
						const auto shininessOpt = FastJSON::getValue< float >(data[SpecularString], JKShininess);

						this->setRoughnessComponent(shininessOpt.has_value() ? 1.0F - clampToUnit(shininessOpt.value()) : DefaultRoughness);
					}
						return true;

					default:
						TraceError{ClassId} << "Invalid filling type for PBR material '" << this->name() << "' resource roughness component (from Specular fallback) !";

						return false;
				}
			}

			default:
				TraceError{ClassId} << "Invalid filling type for PBR material '" << this->name() << "' resource roughness component !";

				return false;
		}
	}

	bool
	StandardResource::parseMetalnessComponent (const Json::Value & data, Resources::AbstractServiceProvider & serviceProvider) noexcept
	{
		FillingType fillingType{};
		Json::Value componentData{};

		if ( !parseComponentBase(data, MetalnessString, fillingType, componentData, true) )
		{
			return false;
		}

		switch ( fillingType )
		{
			case FillingType::Value :
			{
				const auto value = parseValueComponent(componentData);

				if ( !this->setMetalnessComponent(value) )
				{
					return false;
				}
			}
				return true;

			case FillingType::Gradient :
			case FillingType::Texture :
			case FillingType::VolumeTexture :
			case FillingType::Cubemap :
			case FillingType::AnimatedTexture :
			{
				const auto result = m_components.emplace(ComponentType::Metalness, std::make_unique< Texture >(Uniform::MetalnessSampler, SurfaceMetalness, componentData, fillingType, serviceProvider));

				if ( !result.second || result.first->second == nullptr )
				{
					return false;
				}

				this->enableFlag(TextureEnabled);
				this->enableFlag(UsePrimaryTextureCoordinates);

				/* NOTE: With a texture component the scalar is the MULTIPLYING factor — neutral 1.0
				 * (DefaultMetalness would ZERO the texture out). */
				this->setMetalness(FastJSON::getValue< float >(data[MetalnessString], JKValue).value_or(DefaultTextureFactor));
			}
				return true;

			case FillingType::None :
				/* Metalness is optional, use default (dielectric). */
				this->setMetalnessComponent(DefaultMetalness);
				return true;

			default:
				TraceError{ClassId} << "Invalid filling type for PBR material '" << this->name() << "' resource metalness component !";

				return false;
		}
	}

	bool
	StandardResource::parseNormalComponent (const Json::Value & data, Resources::AbstractServiceProvider & serviceProvider) noexcept
	{
		FillingType fillingType{};
		Json::Value componentData{};

		if ( !parseComponentBase(data, NormalString, fillingType, componentData, true) )
		{
			return false;
		}

		switch ( fillingType )
		{
			case FillingType::Gradient :
			case FillingType::Texture :
			case FillingType::VolumeTexture :
			case FillingType::Cubemap :
			case FillingType::AnimatedTexture :
			{
				const auto result = m_components.emplace(ComponentType::Normal, std::make_unique< Texture >(Uniform::NormalSampler, SurfaceNormalVector, componentData, fillingType, serviceProvider));

				if ( !result.second || result.first->second == nullptr )
				{
					return false;
				}

				this->enableFlag(TextureEnabled);
				this->enableFlag(UsePrimaryTextureCoordinates);

				this->setNormalScale(FastJSON::getValue< float >(data[NormalString], JKScale).value_or(DefaultNormalScale));
			}
				return true;

			case FillingType::None :
				return true;

			default:
				TraceError{ClassId} << "Invalid filling type for PBR material '" << this->name() << "' resource normal component !";

				return false;
		}
	}

	bool
	StandardResource::parseHeightComponent (const Json::Value & data, Resources::AbstractServiceProvider & serviceProvider) noexcept
	{
		FillingType fillingType{};
		Json::Value componentData{};

		if ( !parseComponentBase(data, HeightString, fillingType, componentData, true) )
		{
			return false;
		}

		switch ( fillingType )
		{
			case FillingType::Gradient :
			case FillingType::Texture :
			case FillingType::VolumeTexture :
			case FillingType::Cubemap :
			case FillingType::AnimatedTexture :
			{
				const auto result = m_components.emplace(ComponentType::Displacement, std::make_unique< Texture >(Uniform::HeightSampler, SurfaceHeightValue, componentData, fillingType, serviceProvider));

				if ( !result.second || result.first->second == nullptr )
				{
					return false;
				}

				this->enableFlag(TextureEnabled);
				this->enableFlag(UsePrimaryTextureCoordinates);

				m_useParallaxOcclusionMapping = true;

				this->setHeightScale(FastJSON::getValue< float >(data[HeightString], JKScale).value_or(DefaultHeightScale));
			}
				return true;

			case FillingType::None :
				return true;

			default:
				TraceError{ClassId} << "Invalid filling type for PBR material '" << this->name() << "' resource height component !";

				return false;
		}
	}

	bool
	StandardResource::parseReflectionComponent (const Json::Value & data, Resources::AbstractServiceProvider & serviceProvider) noexcept
	{
		FillingType fillingType{};
		Json::Value componentData{};

		if ( !parseComponentBase(data, ReflectionString, fillingType, componentData, true) )
		{
			return false;
		}

		switch ( fillingType )
		{
			case FillingType::Automatic :
			{
				/* Use scene environment cubemap at render time. */
				this->setReflectionComponentFromEnvironmentCubemap(FastJSON::getValue< float >(componentData, JKIBLIntensity).value_or(DefaultIBLIntensity));
			}
				return true;

			case FillingType::Value :
			{
				/* Post-process only reflection: no cubemap IBL, just SSR/RTR via G-buffer reflectivity.
				 * The Amount is stored as a constant reflectivity value for the material properties output. */
				m_postProcessReflectivityAmount = FastJSON::getValue< float >(componentData, JKAmount).value_or(0.5F);
			}
				return true;

			case FillingType::Gradient :
			case FillingType::Texture :
			case FillingType::VolumeTexture :
			case FillingType::Cubemap :
			case FillingType::AnimatedTexture :
			{
				const auto result = m_components.emplace(ComponentType::Reflection, std::make_unique< Texture >(Uniform::ReflectionSampler, SurfaceReflectionColor, componentData, fillingType, serviceProvider));

				if ( !result.second || result.first->second == nullptr )
				{
					return false;
				}

				/* Explicitly authored cubemap: artistic intent, never replaced by SSR/RTR. */
				m_reflectionIsArtistic = true;

				this->enableFlag(TextureEnabled);
				this->enableFlag(UsePrimaryTextureCoordinates);

				/* D2 artistic override: the optional Amount scales the mix; the neutral 1.0
				 * default leaves it BRDF-controlled. */
				this->setReflectionAmount(FastJSON::getValue< float >(data[ReflectionString], JKAmount).value_or(DefaultReflectionAmount));
			}
				return true;

			case FillingType::None :
				return true;

			default:
				TraceError{ClassId} << "Invalid filling type for PBR material '" << this->name() << "' resource reflection component !";

				return false;
		}
	}

	bool
	StandardResource::parseRefractionComponent (const Json::Value & data, Resources::AbstractServiceProvider & serviceProvider) noexcept
	{
		FillingType fillingType{};
		Json::Value componentData{};

		if ( !parseComponentBase(data, RefractionString, fillingType, componentData, true) )
		{
			return false;
		}

		switch ( fillingType )
		{
			case FillingType::Automatic :
			{
				/* Use scene environment cubemap at render time. */
				this->setRefractionComponentFromEnvironmentCubemap(FastJSON::getValue< float >(componentData, JKIOR).value_or(DefaultIOR));
			}
				return true;

			case FillingType::Gradient :
			case FillingType::Texture :
			case FillingType::VolumeTexture :
			case FillingType::Cubemap :
			case FillingType::AnimatedTexture :
			{
				const auto result = m_components.emplace(ComponentType::Refraction, std::make_unique< Texture >(Uniform::RefractionSampler, SurfaceRefractionColor, componentData, fillingType, serviceProvider));

				if ( !result.second || result.first->second == nullptr )
				{
					return false;
				}

				this->enableFlag(TextureEnabled);
				this->enableFlag(UsePrimaryTextureCoordinates);

				this->setIOR(FastJSON::getValue< float >(data[RefractionString], JKIOR).value_or(DefaultIOR));

				/* D2 artistic override: the optional Amount scales the mix; the neutral 1.0
				 * default leaves the blend Fresnel-controlled. */
				this->setRefractionAmount(FastJSON::getValue< float >(data[RefractionString], JKAmount).value_or(DefaultRefractionAmount));
			}
				return true;

			case FillingType::None :
				return true;

			default:
				TraceError{ClassId} << "Invalid filling type for PBR material '" << this->name() << "' resource refraction component !";

				return false;
		}
	}

	bool
	StandardResource::parseAutoIlluminationComponent (const Json::Value & data, Resources::AbstractServiceProvider & serviceProvider) noexcept
	{
		FillingType fillingType{};
		Json::Value componentData{};

		if ( !parseComponentBase(data, AutoIlluminationString, fillingType, componentData, true) )
		{
			return false;
		}

		switch ( fillingType )
		{
			case FillingType::Color :
			{
				const auto color = parseColorComponent(componentData);
				const auto amount = FastJSON::getValue< float >(data[AutoIlluminationString], JKAmount).value_or(DefaultAutoIlluminationAmount);

				if ( !this->setAutoIlluminationComponent(color, amount) )
				{
					return false;
				}
			}
				return true;

			case FillingType::Gradient :
			case FillingType::Texture :
			case FillingType::VolumeTexture :
			case FillingType::Cubemap :
			case FillingType::AnimatedTexture :
			{
				const auto result = m_components.emplace(ComponentType::AutoIllumination, std::make_unique< Texture >(Uniform::AutoIlluminationSampler, SurfaceAutoIlluminationColor, componentData, fillingType, serviceProvider));

				if ( !result.second || result.first->second == nullptr )
				{
					return false;
				}

				this->enableFlag(TextureEnabled);
				this->enableFlag(UsePrimaryTextureCoordinates);

				this->setAutoIlluminationAmount(FastJSON::getValue< float >(data[AutoIlluminationString], JKAmount).value_or(DefaultAutoIlluminationAmount));
			}
				return true;

			case FillingType::None :
				/* AutoIllumination is optional. */
				return true;

			default:
				TraceError{ClassId} << "Invalid filling type for PBR material '" << this->name() << "' resource auto-illumination component !";

				return false;
		}
	}

	bool
	StandardResource::parseOpacityComponent (const Json::Value & data, Resources::AbstractServiceProvider & serviceProvider) noexcept
	{
		FillingType fillingType{};
		Json::Value componentData{};

		if ( !parseComponentBase(data, OpacityString, fillingType, componentData, true) )
		{
			return false;
		}

		switch ( fillingType )
		{
			case FillingType::Value :
			{
				/* Rule 1: a global value is a uniform transparency (blending). */
				const auto value = parseValueComponent(componentData);

				if ( !this->setOpacityComponent(value) )
				{
					return false;
				}
			}
				return true;

			case FillingType::Gradient :
			case FillingType::Texture :
			case FillingType::VolumeTexture :
			case FillingType::Cubemap :
			case FillingType::AnimatedTexture :
			{
				const auto result = m_components.emplace(ComponentType::Opacity, std::make_unique< Texture >(Uniform::OpacitySampler, SurfaceOpacityAmount, componentData, fillingType, serviceProvider));

				if ( !result.second || result.first->second == nullptr )
				{
					return false;
				}

				this->enableFlag(TextureEnabled);
				this->enableFlag(UsePrimaryTextureCoordinates);
				this->enableFlag(OpacityEnabled);

				this->setOpacity(FastJSON::getValue< float >(data[OpacityString], JKAmount).value_or(DefaultOpacity));

				/* Owner's 3-rule contract: an explicit AlphaThreshold key selects the binary
				 * CUTOUT mode (rule 2) — alpha test, the material STAYS OPAQUE. Without it the
				 * map is a grayscale per-pixel alpha scale (rule 3) — blending. */
				if ( const auto threshold = FastJSON::getValue< float >(data[OpacityString], JKAlphaThreshold) )
				{
					this->enableAlphaTest(threshold.value());
				}
				else
				{
					this->enableBlending(BlendingMode::Normal);
				}
			}
				return true;

			case FillingType::None :
				/* Opacity is optional. */
				return true;

			default:
				TraceError{ClassId} << "Invalid filling type for PBR material '" << this->name() << "' resource opacity component !";

				return false;
		}
	}

	bool
	StandardResource::parseAmbientOcclusionComponent (const Json::Value & data, Resources::AbstractServiceProvider & serviceProvider) noexcept
	{
		FillingType fillingType{};
		Json::Value componentData{};

		if ( !parseComponentBase(data, AmbientOcclusionString, fillingType, componentData, true) )
		{
			return false;
		}

		switch ( fillingType )
		{
			case FillingType::Gradient :
			case FillingType::Texture :
			case FillingType::VolumeTexture :
			case FillingType::Cubemap :
			case FillingType::AnimatedTexture :
			{
				const auto result = m_components.emplace(ComponentType::AmbientOcclusion, std::make_unique< Texture >(Uniform::AmbientOcclusionSampler, SurfaceAmbientOcclusion, componentData, fillingType, serviceProvider));

				if ( !result.second || result.first->second == nullptr )
				{
					return false;
				}

				this->enableFlag(TextureEnabled);
				this->enableFlag(UsePrimaryTextureCoordinates);

				this->setAOIntensity(FastJSON::getValue< float >(data[AmbientOcclusionString], JKAmount).value_or(DefaultAOIntensity));
			}
				return true;

			case FillingType::None :
				/* AmbientOcclusion is optional. */
				return true;

			default:
				TraceError{ClassId} << "Invalid filling type for PBR material '" << this->name() << "' resource ambient occlusion component !";

				return false;
		}
	}

	bool
	StandardResource::parseReflectivityMapComponent (const Json::Value & data, Resources::AbstractServiceProvider & serviceProvider) noexcept
	{
		FillingType fillingType{};
		Json::Value componentData{};

		if ( !parseComponentBase(data, ReflectivityMapString, fillingType, componentData, true) )
		{
			return false;
		}

		switch ( fillingType )
		{
			case FillingType::Gradient :
			case FillingType::Texture :
			case FillingType::VolumeTexture :
			case FillingType::Cubemap :
			case FillingType::AnimatedTexture :
			{
				const auto result = m_components.emplace(ComponentType::ReflectivityMap, std::make_unique< Texture >(Uniform::ReflectivityMapSampler, SurfaceReflectivityMap, componentData, fillingType, serviceProvider));

				if ( !result.second || result.first->second == nullptr )
				{
					return false;
				}

				this->enableFlag(TextureEnabled);
				this->enableFlag(UsePrimaryTextureCoordinates);
			}
				return true;

			case FillingType::None :
				/* ReflectivityMap is optional. */
				return true;

			default:
				TraceError{ClassId} << "Invalid filling type for PBR material '" << this->name() << "' resource reflectivity map component !";

				return false;
		}
	}

	bool
	StandardResource::parseClearCoatComponent (const Json::Value & data, Resources::AbstractServiceProvider & serviceProvider) noexcept
	{
		FillingType fillingType{};
		Json::Value componentData{};

		if ( !parseComponentBase(data, ClearCoatString, fillingType, componentData, true) )
		{
			return false;
		}

		switch ( fillingType )
		{
			case FillingType::Value :
			{
				const auto factor = parseValueComponent(componentData);
				const auto roughness = FastJSON::getValue< float >(data[ClearCoatString], JKRoughness).value_or(DefaultClearCoatRoughness);

				if ( !this->setClearCoatComponent(factor, roughness) )
				{
					return false;
				}
			}
				return true;

			case FillingType::Gradient :
			case FillingType::Texture :
			case FillingType::VolumeTexture :
			case FillingType::Cubemap :
			case FillingType::AnimatedTexture :
			{
				const auto result = m_components.emplace(ComponentType::ClearCoat, std::make_unique< Texture >(Uniform::ClearCoatSampler, SurfaceClearCoatFactor, componentData, fillingType, serviceProvider));

				if ( !result.second || result.first->second == nullptr )
				{
					return false;
				}

				this->enableFlag(TextureEnabled);
				this->enableFlag(UsePrimaryTextureCoordinates);

				this->setClearCoatFactor(FastJSON::getValue< float >(data[ClearCoatString], JKValue).value_or(1.0F));
				this->setClearCoatRoughness(FastJSON::getValue< float >(data[ClearCoatString], JKRoughness).value_or(DefaultClearCoatRoughness));

				/* Check for separate ClearCoatRoughness texture. */
				FillingType ccRoughnessFillingType{};
				Json::Value ccRoughnessData{};

				if ( parseComponentBase(data, ClearCoatRoughnessString, ccRoughnessFillingType, ccRoughnessData, true) && ccRoughnessFillingType != FillingType::None )
				{
					const auto roughnessResult = m_components.emplace(ComponentType::ClearCoatRoughness, std::make_unique< Texture >(Uniform::ClearCoatRoughnessSampler, SurfaceClearCoatRoughness, ccRoughnessData, ccRoughnessFillingType, serviceProvider));

					if ( !roughnessResult.second || roughnessResult.first->second == nullptr )
					{
						return false;
					}
				}
			}
				return true;

			case FillingType::None :
				/* ClearCoat is optional. */
				return true;

			default:
				TraceError{ClassId} << "Invalid filling type for PBR material '" << this->name() << "' resource clear coat component !";

				return false;
		}
	}

	bool
	StandardResource::parseSubsurfaceComponent (const Json::Value & data, Resources::AbstractServiceProvider & serviceProvider) noexcept
	{
		FillingType fillingType{};
		Json::Value componentData{};

		if ( !parseComponentBase(data, SubsurfaceString, fillingType, componentData, true) )
		{
			return false;
		}

		switch ( fillingType )
		{
			case FillingType::Value :
			{
				const auto intensity = parseValueComponent(componentData);
				const auto radius = FastJSON::getValue< float >(data[SubsurfaceString], JKRadius).value_or(DefaultSubsurfaceRadius);
				const auto & colorData = data[SubsurfaceString][JKColor];
				const auto color = colorData.isArray() ? parseColorComponent(colorData) : DefaultSubsurfaceColor;

				if ( !this->setSubsurfaceComponent(intensity, radius, color) )
				{
					return false;
				}

				/* Check for separate SubsurfaceThickness texture. */
				FillingType thicknessFillingType{};
				Json::Value thicknessData{};

				if ( parseComponentBase(data, SubsurfaceThicknessString, thicknessFillingType, thicknessData, true) && thicknessFillingType != FillingType::None )
				{
					const auto thicknessResult = m_components.emplace(ComponentType::SubsurfaceThickness, std::make_unique< Texture >(Uniform::SubsurfaceThicknessSampler, SurfaceSubsurfaceThickness, thicknessData, thicknessFillingType, serviceProvider));

					if ( !thicknessResult.second || thicknessResult.first->second == nullptr )
					{
						return false;
					}

					this->enableFlag(TextureEnabled);
					this->enableFlag(UsePrimaryTextureCoordinates);
				}
			}
				return true;

			case FillingType::Gradient :
			case FillingType::Texture :
			case FillingType::VolumeTexture :
			case FillingType::Cubemap :
			case FillingType::AnimatedTexture :
			{
				const auto result = m_components.emplace(ComponentType::Subsurface, std::make_unique< Texture >(Uniform::SubsurfaceSampler, SurfaceSubsurfaceIntensity, componentData, fillingType, serviceProvider));

				if ( !result.second || result.first->second == nullptr )
				{
					return false;
				}

				this->enableFlag(TextureEnabled);
				this->enableFlag(UsePrimaryTextureCoordinates);

				this->setSubsurfaceIntensity(FastJSON::getValue< float >(data[SubsurfaceString], JKValue).value_or(1.0F));
				this->setSubsurfaceRadius(FastJSON::getValue< float >(data[SubsurfaceString], JKRadius).value_or(DefaultSubsurfaceRadius));

				{
					const auto & colorData = data[SubsurfaceString][JKColor];
					const auto color = colorData.isArray() ? parseColorComponent(colorData) : DefaultSubsurfaceColor;
					this->setSubsurfaceColor(color);
				}

				/* Check for separate SubsurfaceThickness texture. */
				FillingType thicknessFillingType{};
				Json::Value thicknessData{};

				if ( parseComponentBase(data, SubsurfaceThicknessString, thicknessFillingType, thicknessData, true) && thicknessFillingType != FillingType::None )
				{
					const auto thicknessResult = m_components.emplace(ComponentType::SubsurfaceThickness, std::make_unique< Texture >(Uniform::SubsurfaceThicknessSampler, SurfaceSubsurfaceThickness, thicknessData, thicknessFillingType, serviceProvider));

					if ( !thicknessResult.second || thicknessResult.first->second == nullptr )
					{
						return false;
					}
				}
			}
				return true;

			case FillingType::None :
				/* Subsurface is optional. */
				return true;

			default:
				TraceError{ClassId} << "Invalid filling type for PBR material '" << this->name() << "' resource subsurface component !";

				return false;
		}
	}

	bool
	StandardResource::parseSheenComponent (const Json::Value & data, Resources::AbstractServiceProvider & serviceProvider) noexcept
	{
		FillingType fillingType{};
		Json::Value componentData{};

		if ( !parseComponentBase(data, SheenString, fillingType, componentData, true) )
		{
			return false;
		}

		switch ( fillingType )
		{
			case FillingType::Color :
			{
				const auto color = parseColorComponent(componentData);
				const auto roughness = FastJSON::getValue< float >(data[SheenString], JKRoughness).value_or(DefaultSheenRoughness);

				if ( !this->setSheenComponent(color, roughness) )
				{
					return false;
				}
			}
				return true;

			case FillingType::Gradient :
			case FillingType::Texture :
			case FillingType::VolumeTexture :
			case FillingType::Cubemap :
			case FillingType::AnimatedTexture :
			{
				const auto result = m_components.emplace(ComponentType::Sheen, std::make_unique< Texture >(Uniform::SheenSampler, SurfaceSheenColor, componentData, fillingType, serviceProvider));

				if ( !result.second || result.first->second == nullptr )
				{
					return false;
				}

				this->enableFlag(TextureEnabled);
				this->enableFlag(UsePrimaryTextureCoordinates);

				{
					const auto & colorData = data[SheenString][JKColor];
					const auto color = colorData.isArray() ? parseColorComponent(colorData) : DefaultSheenColor;
					this->setSheenColor(color);
				}

				this->setSheenRoughness(FastJSON::getValue< float >(data[SheenString], JKRoughness).value_or(DefaultSheenRoughness));
			}
				return true;

			case FillingType::None :
				/* Sheen is optional. */
				return true;

			default:
				TraceError{ClassId} << "Invalid filling type for PBR material '" << this->name() << "' resource sheen component !";

				return false;
		}
	}

	bool
	StandardResource::parseAnisotropyComponent (const Json::Value & data, Resources::AbstractServiceProvider & serviceProvider) noexcept
	{
		FillingType fillingType{};
		Json::Value componentData{};

		if ( !parseComponentBase(data, AnisotropyString, fillingType, componentData, true) )
		{
			return false;
		}

		switch ( fillingType )
		{
			case FillingType::Value :
			{
				const auto anisotropy = parseValueComponent(componentData);
				const auto rotation = FastJSON::getValue< float >(data[AnisotropyString], JKRotation).value_or(DefaultAnisotropyRotation);

				if ( !this->setAnisotropyComponent(anisotropy, rotation) )
				{
					return false;
				}
			}
				return true;

			case FillingType::Gradient :
			case FillingType::Texture :
			case FillingType::VolumeTexture :
			case FillingType::Cubemap :
			case FillingType::AnimatedTexture :
			{
				const auto result = m_components.emplace(ComponentType::Anisotropy, std::make_unique< Texture >(Uniform::AnisotropySampler, SurfaceAnisotropy, componentData, fillingType, serviceProvider));

				if ( !result.second || result.first->second == nullptr )
				{
					return false;
				}

				this->enableFlag(TextureEnabled);
				this->enableFlag(UsePrimaryTextureCoordinates);

				this->setAnisotropy(FastJSON::getValue< float >(data[AnisotropyString], JKValue).value_or(0.5F));
				this->setAnisotropyRotation(FastJSON::getValue< float >(data[AnisotropyString], JKRotation).value_or(DefaultAnisotropyRotation));
			}
				return true;

			case FillingType::None :
				/* Anisotropy is optional. */
				return true;

			default:
				TraceError{ClassId} << "Invalid filling type for PBR material '" << this->name() << "' resource anisotropy component !";

				return false;
		}
	}

	bool
	StandardResource::parseTransmissionComponent (const Json::Value & data, Resources::AbstractServiceProvider & serviceProvider) noexcept
	{
		FillingType fillingType{};
		Json::Value componentData{};

		if ( !parseComponentBase(data, TransmissionString, fillingType, componentData, true) )
		{
			return false;
		}

		switch ( fillingType )
		{
			case FillingType::Value :
			{
				const auto factor = parseValueComponent(componentData);
				const auto & attenuationColorData = data[TransmissionString][JKAttenuationColor];
				const auto attenuationColor = attenuationColorData.isArray() ? parseColorComponent(attenuationColorData) : DefaultAttenuationColor;
				const auto attenuationDistance = FastJSON::getValue< float >(data[TransmissionString], JKAttenuationDistance).value_or(DefaultAttenuationDistance);
				const auto thickness = FastJSON::getValue< float >(data[TransmissionString], JKThickness).value_or(DefaultThicknessFactor);

				if ( FastJSON::getValue< bool >(data[TransmissionString], JKScreenSpace).value_or(false) )
				{
					if ( !this->setTransmissionComponentFromGrabPass(factor, attenuationColor, attenuationDistance, thickness) )
					{
						return false;
					}
				}
				else
				{
					if ( !this->setTransmissionComponent(factor, attenuationColor, attenuationDistance, thickness) )
					{
						return false;
					}
				}
			}
				return true;

			case FillingType::Gradient :
			case FillingType::Texture :
			case FillingType::VolumeTexture :
			case FillingType::Cubemap :
			case FillingType::AnimatedTexture :
			{
				const auto result = m_components.emplace(ComponentType::Transmission, std::make_unique< Texture >(Uniform::TransmissionSampler, SurfaceTransmissionFactor, componentData, fillingType, serviceProvider));

				if ( !result.second || result.first->second == nullptr )
				{
					return false;
				}

				this->enableFlag(TextureEnabled);
				this->enableFlag(UsePrimaryTextureCoordinates);

				const auto & attenuationColorData = data[TransmissionString][JKAttenuationColor];
				const auto attenuationColor = attenuationColorData.isArray() ? parseColorComponent(attenuationColorData) : DefaultAttenuationColor;
				this->setAttenuationColor(attenuationColor);
				this->setAttenuationDistance(FastJSON::getValue< float >(data[TransmissionString], JKAttenuationDistance).value_or(DefaultAttenuationDistance));
				this->setThicknessFactor(FastJSON::getValue< float >(data[TransmissionString], JKThickness).value_or(DefaultThicknessFactor));
				this->setTransmissionFactor(FastJSON::getValue< float >(data[TransmissionString], JKValue).value_or(1.0F));
			}
				return true;

			case FillingType::None :
				/* Transmission is optional. */
				return true;

			default:
				TraceError{ClassId} << "Invalid filling type for PBR material '" << this->name() << "' resource transmission component !";

				return false;
		}
	}

	bool
	StandardResource::load (const Json::Value & data) noexcept
	{
		if ( !this->beginLoading() )
		{
			return false;
		}

		auto & serviceProvider = this->serviceProvider();

		if ( !this->parseAlbedoComponent(data, serviceProvider) )
		{
			TraceError{ClassId} << "Error while parsing the albedo component for PBR material '" << this->name() << "' resource from JSON file !" "\n" "Data : " << data;

			return this->setLoadSuccess(false);
		}

		if ( !this->parseRoughnessComponent(data, serviceProvider) )
		{
			TraceError{ClassId} << "Error while parsing the roughness component for PBR material '" << this->name() << "' resource from JSON file !" "\n" "Data : " << data;

			return this->setLoadSuccess(false);
		}

		if ( !this->parseMetalnessComponent(data, serviceProvider) )
		{
			TraceError{ClassId} << "Error while parsing the metalness component for PBR material '" << this->name() << "' resource from JSON file !" "\n" "Data : " << data;

			return this->setLoadSuccess(false);
		}

		if ( !this->parseNormalComponent(data, serviceProvider) )
		{
			TraceError{ClassId} << "Error while parsing the normal component for PBR material '" << this->name() << "' resource from JSON file !" "\n" "Data : " << data;

			return this->setLoadSuccess(false);
		}

		if ( !this->parseHeightComponent(data, serviceProvider) )
		{
			TraceError{ClassId} << "Error while parsing the height component for PBR material '" << this->name() << "' resource from JSON file !" "\n" "Data : " << data;

			return this->setLoadSuccess(false);
		}

		if ( !this->parseReflectionComponent(data, serviceProvider) )
		{
			TraceError{ClassId} << "Error while parsing the reflection component for PBR material '" << this->name() << "' resource from JSON file !" "\n" "Data : " << data;

			return this->setLoadSuccess(false);
		}

		if ( !this->parseRefractionComponent(data, serviceProvider) )
		{
			TraceError{ClassId} << "Error while parsing the refraction component for PBR material '" << this->name() << "' resource from JSON file !" "\n" "Data : " << data;

			return this->setLoadSuccess(false);
		}

		if ( !this->parseAutoIlluminationComponent(data, serviceProvider) )
		{
			TraceError{ClassId} << "Error while parsing the auto-illumination component for PBR material '" << this->name() << "' resource from JSON file !" "\n" "Data : " << data;

			return this->setLoadSuccess(false);
		}

		if ( !this->parseOpacityComponent(data, serviceProvider) )
		{
			TraceError{ClassId} << "Error while parsing the opacity component for PBR material '" << this->name() << "' resource from JSON file !" "\n" "Data : " << data;

			return this->setLoadSuccess(false);
		}

		if ( !this->parseAmbientOcclusionComponent(data, serviceProvider) )
		{
			TraceError{ClassId} << "Error while parsing the ambient occlusion component for PBR material '" << this->name() << "' resource from JSON file !" "\n" "Data : " << data;

			return this->setLoadSuccess(false);
		}

		if ( !this->parseReflectivityMapComponent(data, serviceProvider) )
		{
			TraceError{ClassId} << "Error while parsing the reflectivity map component for PBR material '" << this->name() << "' resource from JSON file !" "\n" "Data : " << data;

			return this->setLoadSuccess(false);
		}

		if ( !this->parseClearCoatComponent(data, serviceProvider) )
		{
			TraceError{ClassId} << "Error while parsing the clear coat component for PBR material '" << this->name() << "' resource from JSON file !" "\n" "Data : " << data;

			return this->setLoadSuccess(false);
		}

		if ( !this->parseSubsurfaceComponent(data, serviceProvider) )
		{
			TraceError{ClassId} << "Error while parsing the subsurface component for PBR material '" << this->name() << "' resource from JSON file !" "\n" "Data : " << data;

			return this->setLoadSuccess(false);
		}

		if ( !this->parseSheenComponent(data, serviceProvider) )
		{
			TraceError{ClassId} << "Error while parsing the sheen component for PBR material '" << this->name() << "' resource from JSON file !" "\n" "Data : " << data;

			return this->setLoadSuccess(false);
		}

		if ( !this->parseAnisotropyComponent(data, serviceProvider) )
		{
			TraceError{ClassId} << "Error while parsing the anisotropy component for PBR material '" << this->name() << "' resource from JSON file !" "\n" "Data : " << data;

			return this->setLoadSuccess(false);
		}

		if ( !this->parseTransmissionComponent(data, serviceProvider) )
		{
			TraceError{ClassId} << "Error while parsing the transmission component for PBR material '" << this->name() << "' resource from JSON file !" "\n" "Data : " << data;

			return this->setLoadSuccess(false);
		}

		if ( !this->parseIridescenceComponent(data, serviceProvider) )
		{
			TraceError{ClassId} << "Error while parsing the iridescence component for PBR material '" << this->name() << "' resource from JSON file !" "\n" "Data : " << data;

			return this->setLoadSuccess(false);
		}

		/* Parse optional dispersion value (simple float, not a full component). */
		if ( data.isMember(DispersionString) )
		{
			this->setDispersionComponent(FastJSON::getValue< float >(data, DispersionString).value_or(DefaultDispersion));
		}

		/* Parse optional specular component (KHR_materials_specular). */
		if ( !this->parseSpecularComponent(data) )
		{
			TraceError{ClassId} << "Error while parsing the specular component for PBR material '" << this->name() << "' resource from JSON file !" "\n" "Data : " << data;

			return this->setLoadSuccess(false);
		}

		/* Parse optional emissive strength value (KHR_materials_emissive_strength). */
		if ( data.isMember(EmissiveStrengthString) )
		{
			m_materialProperties[EmissiveStrengthOffset] = std::max(0.0F, FastJSON::getValue< float >(data, EmissiveStrengthString).value_or(DefaultEmissiveStrength));
		}

		if ( m_components.empty() )
		{
			TraceError{ClassId} << "No component could be read from PBR material '" << this->name() << "' resource JSON file !";

			return this->setLoadSuccess(false);
		}

		for ( const auto & component : std::ranges::views::values(m_components) )
		{
			if ( component->type() != Type::Texture )
			{
				continue;
			}

			if ( const auto textureResource = component->textureResource(); !this->addDependency(textureResource) )
			{
				TraceError{ClassId} << "Unable to link the texture '" << textureResource->name() << "' dependency to PBR material '" << this->name() << "' !";

				return this->setLoadSuccess(false);
			}
		}

		return this->setLoadSuccess(true);
	}

	bool
	StandardResource::create (Renderer & renderer) noexcept
	{
		if ( m_components.empty() )
		{
			TraceError{ClassId} << "The PBR material resource '" << this->name() << "' has no component !";

			return false;
		}

		/* Component creation (optional). */
		if ( this->usingTexture() )
		{
			/* NOTE: Starts at 1 because there is the UBO in the first place. */
			uint32_t binding = 1;

			for ( const auto & [componentType, component] : m_components )
			{
				if ( component->type() != Type::Texture )
				{
					continue;
				}

				if ( !component->create(renderer, binding))
				{
					TraceError{ClassId} << "Unable to create component '" << to_cstring(componentType) << "' of PBR material resource '" << this->name() << "' !";

					return false;
				}

				/* Check if this texture component is animated. */
				if ( component->texture() != nullptr && component->texture()->duration() > 0 )
				{
					this->enableFlag(IsAnimated);
				}
			}
		}

		const auto identifier = this->getSharedUniformBufferIdentifier();

		if ( !this->createElementInSharedBuffer(renderer, identifier) )
		{
			TraceError{ClassId} << "Unable to create the data inside the shared uniform buffer '" << identifier << "' for PBR material '" << this->name() << "' !";

			return false;
		}

		if ( !this->createDescriptorSetLayout(renderer.layoutManager(), identifier) )
		{
			TraceError{ClassId} << "Unable to create the descriptor set layout for PBR material '" << this->name() << "' !";

			return false;
		}

		if ( !this->createDescriptorSet(renderer, *m_sharedUniformBuffer->uniformBufferObject(m_sharedUBOIndex)) )
		{
			TraceError{ClassId} << "Unable to create the descriptor set for PBR material '" << this->name() << "' !";

			return false;
		}

		/* Sync each texture component's UV transform (KHR_texture_transform / JSON "UVW" keys)
		 * into its material UBO slot — the components are the single source of truth. */
		this->syncComponentUVWTransforms();

		/* Initialize the material data in the GPU. */
		if ( !this->updateVideoMemory() )
		{
			Tracer::error(ClassId, "Unable to update the initial video memory !");

			return false;
		}

		return true;
	}

	void
	StandardResource::syncComponentUVWTransforms () noexcept
	{
		static constexpr std::pair< ComponentType, size_t > slots[] = {
			{ComponentType::Albedo, AlbedoUVWTransformOffset},
			{ComponentType::Roughness, RoughnessUVWTransformOffset},
			{ComponentType::Metalness, MetalnessUVWTransformOffset},
			{ComponentType::Normal, NormalUVWTransformOffset},
			{ComponentType::AmbientOcclusion, AmbientOcclusionUVWTransformOffset},
			{ComponentType::AutoIllumination, AutoIlluminationUVWTransformOffset}
		};

		for ( const auto & [componentType, offset] : slots )
		{
			const auto componentIt = m_components.find(componentType);

			if ( componentIt == m_components.cend() || componentIt->second->type() != Component::Type::Texture )
			{
				continue;
			}

			const auto * textureComponent = static_cast< const Texture * >(componentIt->second.get());

			m_materialProperties[offset] = textureComponent->UVWScale()[0];
			m_materialProperties[offset + 1] = textureComponent->UVWScale()[1];
			m_materialProperties[offset + 2] = textureComponent->UVWOffset()[0];
			m_materialProperties[offset + 3] = textureComponent->UVWOffset()[1];
		}
	}

	bool
	StandardResource::setComponentUVWTransform (ComponentType componentType, const Base::Math::Vector< 2, float > & scale, const Base::Math::Vector< 2, float > & offset) noexcept
	{
		if ( this->isCreated() )
		{
			TraceWarning{ClassId} <<
				"The resource '" << this->name() << "' is created ! "
				"Unable to change a component UV transform.";

			return false;
		}

		const auto componentIt = m_components.find(componentType);

		if ( componentIt == m_components.cend() || componentIt->second->type() != Component::Type::Texture )
		{
			return false;
		}

		auto * textureComponent = static_cast< Texture * >(componentIt->second.get());
		textureComponent->setUVWScale({scale[0], scale[1], 1.0F});
		textureComponent->setUVWOffset({offset[0], offset[1], 0.0F});

		return true;
	}

	std::string
	StandardResource::getSharedUniformBufferIdentifier () const noexcept
	{
		std::stringstream identifier{};

		uint32_t textureCount = 0;

		for ( const auto & component : std::ranges::views::values(m_components) )
		{
			if ( component->type() == Type::Texture )
			{
				textureCount++;
			}
		}

		identifier << ClassId;

		if ( textureCount > 0 )
		{
			identifier << textureCount << "Textures";
		}
		else
		{
			identifier << "Simple";
		}

		return identifier.str();
	}

	bool
	StandardResource::createElementInSharedBuffer (Renderer & renderer, const std::string & identifier) noexcept
	{
		m_sharedUniformBuffer = this->getSharedUniformBuffer(renderer, identifier);

		if ( m_sharedUniformBuffer == nullptr )
		{
			Tracer::error(ClassId, "Unable to get the shared uniform buffer !");

			return false;
		}

		if ( !m_sharedUniformBuffer->addElement(this, m_sharedUBOIndex) )
		{
			Tracer::error(ClassId, "Unable to add the PBR material to the shared uniform buffer !");

			return false;
		}

		return true;
	}

	bool
	StandardResource::createDescriptorSetLayout (LayoutManager & layoutManager, const std::string & identifier) noexcept
	{
		m_descriptorSetLayout = layoutManager.getDescriptorSetLayout(identifier);

		if ( m_descriptorSetLayout == nullptr )
		{
			uint32_t bindingPoint = 0;

			auto newLayout = layoutManager.prepareNewDescriptorSetLayout(identifier);
			newLayout->setIdentifier(ClassId, identifier, "DescriptorSetLayout");

			/* Declare the UBO for the material properties. */
			newLayout->declareUniformBuffer(bindingPoint++, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);

			/* Declare every sampler used by the material. */
			for ( const auto & component : std::ranges::views::values(m_components) )
			{
				if ( component->type() == Type::Texture )
				{
					newLayout->declareCombinedImageSampler(bindingPoint++, VK_SHADER_STAGE_FRAGMENT_BIT);
				}
			}

			if ( !layoutManager.createDescriptorSetLayout(newLayout) )
			{
				return false;
			}

			/* NOTE: Another thread may have registered the same layout first.
			 * Re-fetch to ensure we hold the canonical instance from the manager. */
			m_descriptorSetLayout = layoutManager.getDescriptorSetLayout(identifier);

			if ( m_descriptorSetLayout == nullptr )
			{
				return false;
			}
		}

		return true;
	}

	bool
	StandardResource::createDescriptorSet (Renderer & renderer, const UniformBufferObject & uniformBufferObject) noexcept
	{
		m_descriptorSet = std::make_unique< DescriptorSet >(renderer.descriptorPool(), m_descriptorSetLayout);
		m_descriptorSet->setIdentifier(ClassId, this->name(), "DescriptorSet");

		if ( !m_descriptorSet->create() )
		{
			TraceError{ClassId} << "Unable to create the descriptor set for PBR material '" << this->name() << "' !";

			return false;
		}

		uint32_t bindingPoint = 0;

		const auto descriptorInfo = m_sharedUniformBuffer->getDescriptorInfoForElement(m_sharedUBOIndex);

		if ( !m_descriptorSet->writeUniformBuffer(bindingPoint++, descriptorInfo) )
		{
			TraceError{ClassId} << "Unable to write the uniform buffer to the descriptor set of PBR material '" << this->name() << "' !";

			return false;
		}

		for ( const auto & component : std::ranges::views::values(m_components) )
		{
			if ( component->type() != Type::Texture )
			{
				continue;
			}

			if ( !m_descriptorSet->writeCombinedImageSampler(bindingPoint++, *component->texture()) )
			{
				TraceError{ClassId} << "Unable to write the texture to the descriptor set of PBR material '" << this->name() << "' !";

				return false;
			}
		}

		return true;
	}

	bool
	StandardResource::updateVideoMemory () noexcept
	{
		if ( m_sharedUniformBuffer == nullptr )
		{
			return false;
		}

		if ( !m_sharedUniformBuffer->writeElementData(m_sharedUBOIndex, m_materialProperties.data()) )
		{
			return false;
		}

		m_videoMemoryUpdated = false;

		return true;
	}

	void
	StandardResource::destroy () noexcept
	{
		if ( m_sharedUniformBuffer != nullptr )
		{
			m_sharedUniformBuffer->removeElement(this);
		}

		/* Reset to defaults. */
		this->resetFlags();

		/* Reset member variables. */
		m_physicalSurfaceProperties.reset();
		m_components.clear();
		m_blendingMode = BlendingMode::None;
		m_materialProperties = {
			/* Albedo color (4) */
			DefaultAlbedoColor.red(), DefaultAlbedoColor.green(), DefaultAlbedoColor.blue(), DefaultAlbedoColor.alpha(),
			/* Roughness (1), Metalness (1), NormalScale (1), SpecularFactor (1) */
			DefaultRoughness, DefaultMetalness, DefaultNormalScale, DefaultSpecularFactor,
			/* IOR (1), IBLIntensity (1), AutoIlluminationAmount (1), AOIntensity (1) */
			DefaultIOR, DefaultIBLIntensity, DefaultAutoIlluminationAmount, DefaultAOIntensity,
			/* AutoIlluminationColor (4) */
			DefaultAutoIlluminationColor.red(), DefaultAutoIlluminationColor.green(), DefaultAutoIlluminationColor.blue(), DefaultAutoIlluminationColor.alpha(),
			/* ClearCoatFactor (1), ClearCoatRoughness (1), SubsurfaceIntensity (1), SubsurfaceRadius (1) */
			DefaultClearCoatFactor, DefaultClearCoatRoughness, DefaultSubsurfaceIntensity, DefaultSubsurfaceRadius,
			/* SubsurfaceColor (4) */
			DefaultSubsurfaceColor.red(), DefaultSubsurfaceColor.green(), DefaultSubsurfaceColor.blue(), DefaultSubsurfaceColor.alpha(),
			/* SheenColor (4) */
			DefaultSheenColor.red(), DefaultSheenColor.green(), DefaultSheenColor.blue(), DefaultSheenColor.alpha(),
			/* SheenRoughness (1), Anisotropy (1), AnisotropyRotation (1), TransmissionFactor (1) */
			DefaultSheenRoughness, DefaultAnisotropy, DefaultAnisotropyRotation, DefaultTransmissionFactor,
			/* AttenuationColor (4) */
			DefaultAttenuationColor.red(), DefaultAttenuationColor.green(), DefaultAttenuationColor.blue(), DefaultAttenuationColor.alpha(),
			/* AttenuationDistance (1), ThicknessFactor (1), HeightScale (1), IridescenceFactor (1) */
			DefaultAttenuationDistance, DefaultThicknessFactor, DefaultHeightScale, DefaultIridescenceFactor,
			/* IridescenceIOR (1), IridescenceThicknessMin (1), IridescenceThicknessMax (1), Dispersion (1) */
			DefaultIridescenceIOR, DefaultIridescenceThicknessMin, DefaultIridescenceThicknessMax, DefaultDispersion,
			/* SpecularColorFactor (4) */
			DefaultSpecularColor.red(), DefaultSpecularColor.green(), DefaultSpecularColor.blue(), DefaultSpecularColor.alpha(),
			/* EmissiveStrength (1), ClearCoatNormalScale (1) + padding (2) for STD140 alignment */
			DefaultEmissiveStrength, DefaultClearCoatNormalScale, 0.0F, 0.0F
		};
		m_useParallaxOcclusionMapping = false;
		m_descriptorSetLayout.reset();
		m_descriptorSet.reset();
		m_sharedUniformBuffer.reset();
		m_sharedUBOIndex = 0;
	}

	bool
	StandardResource::isComplex () const noexcept
	{
		return this->isComponentPresent(ComponentType::Reflection) || this->isComponentPresent(ComponentType::Refraction) || m_isUsingEnvironmentCubemap || m_isUsingEnvironmentCubemapForRefraction || m_isUsingEnvironmentCubemapForTransmission || m_isUsingGrabPassForTransmission || m_useParallaxOcclusionMapping;
	}

	const Physics::SurfacePhysicalProperties &
	StandardResource::surfacePhysicalProperties () const noexcept
	{
		return m_physicalSurfaceProperties;
	}

	Physics::SurfacePhysicalProperties &
	StandardResource::surfacePhysicalProperties () noexcept
	{
		return m_physicalSurfaceProperties;
	}

	uint32_t
	StandardResource::frameCount () const noexcept
	{
		if ( !this->isFlagEnabled(IsAnimated) )
		{
			return 1;
		}

		for ( const auto & component : std::ranges::views::values(m_components) )
		{
			if ( component->texture() != nullptr && component->texture()->duration() > 0 )
			{
				return component->texture()->frameCount();
			}
		}

		return 1;
	}

	uint32_t
	StandardResource::duration () const noexcept
	{
		if ( !this->isFlagEnabled(IsAnimated) )
		{
			return 0;
		}

		for ( const auto & component : std::ranges::views::values(m_components) )
		{
			if ( component->texture() != nullptr && component->texture()->duration() > 0 )
			{
				return component->texture()->duration();
			}
		}

		return 0;
	}

	uint32_t
	StandardResource::frameIndexAt (uint32_t sceneTime) const noexcept
	{
		if ( !this->isFlagEnabled(IsAnimated) )
		{
			return 0;
		}

		for ( const auto & component : std::ranges::views::values(m_components) )
		{
			if ( component->texture() != nullptr && component->texture()->duration() > 0 )
			{
				return component->texture()->frameIndexAt(sceneTime);
			}
		}

		return 0;
	}

	void
	StandardResource::enableBlending (BlendingMode mode) noexcept
	{
		if ( this->isCreated() )
		{
			TraceWarning{ClassId} << "The resource '" << this->name() << "' is already created ! Unable to enable a blending mode.";

			return;
		}

		this->enableFlag(BlendingEnabled);

		m_blendingMode = mode;
	}

	BlendingMode
	StandardResource::blendingMode () const noexcept
	{
		if ( !this->isFlagEnabled(BlendingEnabled) )
		{
			return BlendingMode::None;
		}

		return m_blendingMode;
	}

	void
	StandardResource::exportRTMaterialData (GPURTMaterialData & outData) const noexcept
	{
		outData = GPURTMaterialData{};

		/* Albedo. */
		outData.albedo[0] = m_materialProperties[AlbedoColorOffset];
		outData.albedo[1] = m_materialProperties[AlbedoColorOffset + 1];
		outData.albedo[2] = m_materialProperties[AlbedoColorOffset + 2];
		outData.albedo[3] = m_materialProperties[AlbedoColorOffset + 3];

		/* Core PBR scalars. */
		outData.roughness = m_materialProperties[RoughnessOffset];
		outData.metalness = m_materialProperties[MetalnessOffset];

		/* Smoothness/gloss source: the RT hit shading must invert the sampled texel
		 * exactly like the raster roughness component codegen does. */
		if ( m_invertRoughness )
		{
			outData.flags |= GPURTMaterialData::RoughnessTexInverted;
		}
		outData.ior = m_materialProperties[IOROffset];
		outData.specularFactor = m_materialProperties[SpecularFactorOffset];

		/* Specular color (KHR_materials_specular). */
		outData.specularColor[0] = m_materialProperties[SpecularColorOffset];
		outData.specularColor[1] = m_materialProperties[SpecularColorOffset + 1];
		outData.specularColor[2] = m_materialProperties[SpecularColorOffset + 2];
		outData.specularColor[3] = m_materialProperties[SpecularColorOffset + 3];

		/* Emission. */
		const auto autoIllumAmount = m_materialProperties[AutoIlluminationAmountOffset];

		if ( autoIllumAmount > 0.0F )
		{
			outData.emissionColor[0] = m_materialProperties[AutoIlluminationColorOffset];
			outData.emissionColor[1] = m_materialProperties[AutoIlluminationColorOffset + 1];
			outData.emissionColor[2] = m_materialProperties[AutoIlluminationColorOffset + 2];
			outData.emissionColor[3] = m_materialProperties[AutoIlluminationColorOffset + 3];
			outData.emissiveStrength = m_materialProperties[EmissiveStrengthOffset] * autoIllumAmount;
			outData.flags |= GPURTMaterialData::IsEmissive;
		}

		/* Clear coat. */
		const auto clearCoat = m_materialProperties[ClearCoatFactorOffset];

		if ( clearCoat > 0.0F )
		{
			outData.clearCoatFactor = clearCoat;
			outData.clearCoatRoughness = m_materialProperties[ClearCoatRoughnessOffset];
			outData.flags |= GPURTMaterialData::HasClearCoat;
		}

		/* Alpha-test: signal the RT trace shaders to sample the opacity at hit time. The
		 * cutoff mirrors the raster threshold (UBO slot); blending materials keep the 0.5
		 * default of the mirror, unchanged behaviour. */
		if ( this->isAlphaTest() )
		{
			outData.flags |= GPURTMaterialData::IsAlphaTest;
			outData.alphaCutoff = m_materialProperties[AlphaThresholdOffset];
		}

		/* Normal map intensity for the RT hit shading (same value the raster uses). */
		outData.normalScale = m_materialProperties[NormalScaleOffset];

		/* NOTE: Texture bindless indices are set by SceneMetaData during material collection.
		 * The textures are accessible via m_components[ComponentType::Albedo], etc. */
	}

	void
	StandardResource::collectRTTextures (std::vector< RTTextureSlot > & outSlots) const noexcept
	{
		static constexpr std::pair< ComponentType, RTTextureRole > mappings[] = {
			{ComponentType::Albedo, RTTextureRole::Albedo},
			{ComponentType::Normal, RTTextureRole::Normal},
			{ComponentType::Roughness, RTTextureRole::Roughness},
			{ComponentType::Metalness, RTTextureRole::Metalness},
			{ComponentType::AutoIllumination, RTTextureRole::Emission},
			{ComponentType::Opacity, RTTextureRole::Opacity}
		};

		for ( const auto & [compType, role] : mappings )
		{
			const auto it = m_components.find(compType);

			if ( it != m_components.end() && it->second != nullptr && it->second->type() == Component::Type::Texture )
			{
				const auto tex = it->second->texture();

				if ( tex != nullptr )
				{
					/* Carry the raster component's source channel so the RT hit shading
					 * reads the same texel channel (glTF packed metallic-roughness: G/B). */
					const auto sourceChannel = static_cast< const Texture * >(it->second.get())->sourceChannel();

					outSlots.push_back({role, sourceChannel, tex});
				}
			}
		}
	}

	std::string
	StandardResource::albedoExpression () const noexcept
	{
		/* Vertex colours are folded into a single variable at the top of the fragment shader,
		 * so every consumer reads the SAME post-modulation value (glTF COLOR_0 contract). */
		if ( this->usingVertexColors() )
		{
			return SurfaceAlbedoFinal;
		}

		const auto componentIt = m_components.find(ComponentType::Albedo);

		if ( componentIt != m_components.cend() )
		{
			return std::string{componentIt->second->variableName()};
		}

		return MaterialUB(UniformBlock::Component::AlbedoColor);
	}

	void
	StandardResource::enableVertexColor () noexcept
	{
		if ( this->isCreated() )
		{
			TraceWarning{ClassId} <<
				"The resource '" << this->name() << "' is created ! "
				"Unable to enable the vertex colour.";

			return;
		}

		this->enableFlag(UseVertexColors);
	}

	const Texture *
	StandardResource::alphaSourceTextureComponent () const noexcept
	{
		for ( const auto componentType : {ComponentType::Opacity, ComponentType::Albedo} )
		{
			const auto componentIt = m_components.find(componentType);

			if ( componentIt != m_components.cend() && componentIt->second != nullptr && componentIt->second->type() == Type::Texture )
			{
				return static_cast< const Texture * >(componentIt->second.get());
			}
		}

		return nullptr;
	}

	std::string
	StandardResource::emissionMultiplier () const noexcept
	{
		/* The UNLIT path writes the surface colour verbatim — no light pass ever runs, so nothing
		 * else would apply the material's luminance. A skybox is the canonical case: without this
		 * multiplier its cubemap texel stays in [0,1] and reads near-black once the photometric
		 * exposure is applied. Only a self-illuminating material emits. */
		if ( !this->isComponentPresent(ComponentType::AutoIllumination) )
		{
			return {};
		}

		return (std::stringstream{} << '(' << MaterialUB(UniformBlock::Component::AutoIlluminationAmount) << " * " << MaterialUB(UniformBlock::Component::EmissiveStrength) << ')').str();
	}

	bool
	StandardResource::requiresAlphaTestedShadows () const noexcept
	{
		if ( !this->isFlagEnabled(AlphaTestEnabled) )
		{
			return false;
		}

		/* A binary CUTOUT must cast a CUTOUT shadow — a grate that shadows as a solid
		 * rectangle is worse than no shadow at all (same contract as BasicResource). The
		 * shadow discard reads the SAME UBO threshold as the colour pass, so the two agree
		 * by construction. */
		return this->alphaSourceTextureComponent() != nullptr;
	}

	bool
	StandardResource::generateShadowVertexCode (const Saphir::Generator::Abstract & /*generator*/, Saphir::VertexShader & vertexShader) const noexcept
	{
		const auto * component = this->alphaSourceTextureComponent();

		if ( component == nullptr )
		{
			return true;
		}

		/* Request texture coordinates output to the fragment shader. */
		const auto * textureCoordVar = component->isVolumetricTexture()
			? ShaderVariable::Primary3DTextureCoordinates
			: ShaderVariable::Primary2DTextureCoordinates;

		if ( !vertexShader.requestSynthesizeInstruction(textureCoordVar) )
		{
			TraceError{ClassId} << "Unable to synthesize texture coordinates for the shadow vertex shader of PBR material '" << this->name() << "' !";

			return false;
		}

		return true;
	}

	bool
	StandardResource::generateShadowAlphaTestCode (const Saphir::Generator::Abstract & generator, Saphir::FragmentShader & fragmentShader) const noexcept
	{
		const auto * component = this->alphaSourceTextureComponent();

		if ( component == nullptr )
		{
			return true;
		}

		const uint32_t materialSet = generator.shaderProgram()->setIndex(SetType::PerModelLayer);

		/* The threshold is a material UBO VALUE (program-cache contract: never a shader
		 * literal), so the block must be declared in the shadow fragment shader too. */
		if ( !fragmentShader.declare(this->getUniformBlock(materialSet, 0)) )
		{
			TraceError{ClassId} << "Unable to declare the material uniform block for the shadow alpha test of PBR material '" << this->name() << "' !";

			return false;
		}

		if ( !fragmentShader.declare(Declaration::Sampler{materialSet, component->binding(), component->textureType(), component->samplerName()}) )
		{
			TraceError{ClassId} << "Unable to declare the texture sampler for the shadow alpha test of PBR material '" << this->name() << "' !";

			return false;
		}

		const auto * texCoordVariable = component->isVolumetricTexture()
			? ShaderVariable::Primary3DTextureCoordinates
			: ShaderVariable::Primary2DTextureCoordinates;

		/* Sample the alpha source: the opacity component reads its red channel, the albedo
		 * fallback reads its alpha channel — the same sources the colour pass tests. */
		const char * channel = this->isComponentPresent(ComponentType::Opacity) ? "r" : "a";

		Code{fragmentShader, Location::Top} << "const float " << SurfaceOpacityAmount << " = texture(" << component->samplerName() << ", " << texCoordVariable << ")." << channel << ";";

		/* Discard fragments below the material threshold. */
		Code{fragmentShader, Location::Output} << "if ( " << SurfaceOpacityAmount << " < " << MaterialUB(UniformBlock::Component::AlphaThreshold) << " ) { discard; }";

		return true;
	}

	bool
	StandardResource::setupLightGenerator (LightGenerator & lightGenerator) const noexcept
	{
		if ( !this->isCreated() )
		{
			TraceError{ClassId} <<
				"The PBR material '" << this->name() << "' is not created !"
				"It can't configure the light generator.";

			return false;
		}

		/* Enable PBR mode in the light generator. */
		lightGenerator.enablePBRMode();

		/* Albedo component.
		 * ⚠️ Always go through albedoExpression(): with vertex colours it names the FOLDED
		 * variable, so the BRDF shades the modulated albedo. The light generator concatenates
		 * swizzles onto this string (".rgb", ".a"), which is why it must be a NAME — never a
		 * compound expression, or the swizzle would land on the last operand and still compile. */
		{
			const auto albedo = this->albedoExpression();

			lightGenerator.declareSurfaceAlbedo(albedo);

			/* Opacity: use the albedo alpha when blending is enabled (glTF alphaMode: BLEND). */
			if ( this->isFlagEnabled(BlendingEnabled) )
			{
				lightGenerator.declareSurfaceOpacity(albedo + ".a");
			}
		}

		/* Roughness component */
		{
			const auto componentIt = m_components.find(ComponentType::Roughness);

			if ( componentIt != m_components.cend() )
			{
				/* NOTE: The variable already carries the FINAL roughness — the component codegen
				 * folds the source channel, the smoothness/gloss inversion and the material
				 * factor into its definition. Never re-apply any of them here. */
				lightGenerator.declareSurfaceRoughness(componentIt->second->variableName());
			}
			else
			{
				/* Use uniform value. */
				lightGenerator.declareSurfaceRoughness(MaterialUB(UniformBlock::Component::Roughness));
			}
		}

		/* Metalness component */
		{
			const auto componentIt = m_components.find(ComponentType::Metalness);

			if ( componentIt != m_components.cend() )
			{
				lightGenerator.declareSurfaceMetalness(componentIt->second->variableName());
			}
			else
			{
				/* Use uniform value. */
				lightGenerator.declareSurfaceMetalness(MaterialUB(UniformBlock::Component::Metalness));
			}
		}

		/* Normal component.
		 * Always declare even in ambient pass: the MRT normals attachment
		 * needs the perturbed normal for post-process effects (RTR, SSR, SSAO, RTAO). */
		{
			const auto componentIt = m_components.find(ComponentType::Normal);

			if ( componentIt != m_components.cend() )
			{
				lightGenerator.declareSurfaceNormal(componentIt->second->variableName());
			}
		}

		/* Reflection/IBL component */
		{
			const auto componentIt = m_components.find(ComponentType::Reflection);

			if ( componentIt != m_components.cend() )
			{
				/* NOTE: The amount is the D2 artistic override, a UBO VALUE (program-cache
				 * contract) whose neutral 1.0 default leaves the mix BRDF-controlled. */
				lightGenerator.declareSurfaceReflection(componentIt->second->variableName(), MaterialUB(UniformBlock::Component::ReflectionAmount));

				if ( m_reflectionIsArtistic )
				{
					lightGenerator.declareReflectionArtistic();
				}

				if ( m_reflectionSourceIsAbsolute )
				{
					lightGenerator.declareReflectionSourceAbsolute();
				}
			}
			else if ( m_isUsingEnvironmentCubemap )
			{
				/* NOTE: When using automatic reflection with bindless textures, the variable is named SurfaceReflectionColor. */
				lightGenerator.declareSurfaceReflection(SurfaceReflectionColor, "1.0");
			}
		}

		/* Refraction component (for glass-like materials) */
		{
			const auto componentIt = m_components.find(ComponentType::Refraction);

			if ( componentIt != m_components.cend() )
			{
				/* NOTE: The amount is the D2 artistic override, a UBO VALUE (program-cache
				 * contract) whose neutral 1.0 default leaves the blend Fresnel-controlled. */
				lightGenerator.declareSurfaceRefraction(componentIt->second->variableName(), MaterialUB(UniformBlock::Component::RefractionAmount), MaterialUB(UniformBlock::Component::RefractionIOR));

				if ( m_refractionSourceIsAbsolute )
				{
					lightGenerator.declareRefractionSourceAbsolute();
				}
			}
			else if ( m_isUsingEnvironmentCubemapForRefraction )
			{
				/* NOTE: When using automatic refraction with bindless textures, the variable is named SurfaceRefractionColor. */
				lightGenerator.declareSurfaceRefraction(SurfaceRefractionColor, "1.0", MaterialUB(UniformBlock::Component::RefractionIOR));
			}
		}

		/* Material IOR - affects dielectric F0 computation: F0 = ((ior-1)/(ior+1))^2 (KHR_materials_ior). */
		lightGenerator.declareSurfaceMaterialIOR(MaterialUB(UniformBlock::Component::RefractionIOR));

		/* KHR_materials_specular - scales and tints dielectric F0. */
		lightGenerator.declareSurfaceKHRSpecular(
			MaterialUB(UniformBlock::Component::SpecularFactor),
			MaterialUB(UniformBlock::Component::SpecularColorFactor)
		);

		/* IBL Intensity - controls the contribution of environment cubemaps (reflection/refraction). */
		lightGenerator.declareSurfaceIBLIntensity(MaterialUB(UniformBlock::Component::IBLIntensity));

		/* Auto-Illumination (emissive) component */
		{
			const auto componentIt = m_components.find(ComponentType::AutoIllumination);

			if ( componentIt != m_components.cend() )
			{
				lightGenerator.declareSurfaceAutoIllumination(componentIt->second->variableName(), MaterialUB(UniformBlock::Component::AutoIlluminationAmount));
			}
			else
			{
				/* Use uniform color value with amount. */
				lightGenerator.declareSurfaceAutoIllumination(MaterialUB(UniformBlock::Component::AutoIlluminationColor), MaterialUB(UniformBlock::Component::AutoIlluminationAmount));
			}
		}

		/* Emissive Strength (KHR_materials_emissive_strength). */
		lightGenerator.declareSurfaceEmissiveStrength(MaterialUB(UniformBlock::Component::EmissiveStrength));

		/* Ambient Occlusion component (texture-based only) */
		{
			const auto componentIt = m_components.find(ComponentType::AmbientOcclusion);

			if ( componentIt != m_components.cend() )
			{
				lightGenerator.declareSurfaceAmbientOcclusion(componentIt->second->variableName(), MaterialUB(UniformBlock::Component::AOIntensity));
			}
		}

		/* Opacity component: light passes must respect the surface translucency (rules 1 and
		 * 3 of the opacity contract). Cutout mode (rule 2) needs no declaration — surviving
		 * texels are opaque. */
		if ( this->isFlagEnabled(OpacityEnabled) && !this->isFlagEnabled(AlphaTestEnabled) )
		{
			const auto componentIt = m_components.find(ComponentType::Opacity);

			if ( componentIt != m_components.cend() && componentIt->second->type() == Type::Texture )
			{
				lightGenerator.declareSurfaceOpacity(componentIt->second->variableName());
			}
			else
			{
				lightGenerator.declareSurfaceOpacity(MaterialUB(UniformBlock::Component::Opacity));
			}
		}

		/* Reflectivity Map component (texture-based only) */
		{
			const auto componentIt = m_components.find(ComponentType::ReflectivityMap);

			if ( componentIt != m_components.cend() )
			{
				lightGenerator.declareSurfaceReflectivityMap(componentIt->second->variableName());
			}
			else if ( m_postProcessReflectivityAmount >= 0.0F )
			{
				/* Post-process only reflection (Value type): pass the constant as a GLSL literal. */
				lightGenerator.declareSurfaceReflectivityMap("(" + std::to_string(m_postProcessReflectivityAmount) + ")");
			}
		}

		/* Clear Coat component */
		{
			const auto componentIt = m_components.find(ComponentType::ClearCoat);

			if ( componentIt != m_components.cend() )
			{
				const auto ccRoughnessIt = m_components.find(ComponentType::ClearCoatRoughness);

				lightGenerator.declareSurfaceClearCoat(
					componentIt->second->variableName(),
					ccRoughnessIt != m_components.cend() ? ccRoughnessIt->second->variableName() : MaterialUB(UniformBlock::Component::ClearCoatRoughness)
				);
			}
			else if ( m_materialProperties[ClearCoatFactorOffset] > 0.0F )
			{
				lightGenerator.declareSurfaceClearCoat(
					MaterialUB(UniformBlock::Component::ClearCoatFactor),
					MaterialUB(UniformBlock::Component::ClearCoatRoughness)
				);
			}
		}

		/* Clear Coat Normal component */
		{
			const auto componentIt = m_components.find(ComponentType::ClearCoatNormal);

			if ( componentIt != m_components.cend() )
			{
				lightGenerator.declareSurfaceClearCoatNormal(componentIt->second->variableName());
			}
		}

		/* Subsurface Scattering component */
		{
			const auto componentIt = m_components.find(ComponentType::Subsurface);

			if ( componentIt != m_components.cend() )
			{
				lightGenerator.declareSurfaceSubsurface(
					componentIt->second->variableName(),
					MaterialUB(UniformBlock::Component::SubsurfaceColor),
					MaterialUB(UniformBlock::Component::SubsurfaceRadius)
				);

				const auto thicknessIt = m_components.find(ComponentType::SubsurfaceThickness);

				if ( thicknessIt != m_components.cend() )
				{
					lightGenerator.declareSurfaceSubsurfaceThickness(thicknessIt->second->variableName());
				}
			}
			else if ( m_materialProperties[SubsurfaceIntensityOffset] > 0.0F )
			{
				lightGenerator.declareSurfaceSubsurface(
					MaterialUB(UniformBlock::Component::SubsurfaceIntensity),
					MaterialUB(UniformBlock::Component::SubsurfaceColor),
					MaterialUB(UniformBlock::Component::SubsurfaceRadius)
				);
			}
		}

		/* Sheen component */
		{
			const auto componentIt = m_components.find(ComponentType::Sheen);

			if ( componentIt != m_components.cend() )
			{
				lightGenerator.declareSurfaceSheen(
					componentIt->second->variableName(),
					MaterialUB(UniformBlock::Component::SheenRoughness)
				);
			}
			else if ( m_materialProperties[SheenColorOffset] > 0.0F || m_materialProperties[SheenColorOffset+1] > 0.0F || m_materialProperties[SheenColorOffset+2] > 0.0F )
			{
				lightGenerator.declareSurfaceSheen(
					MaterialUB(UniformBlock::Component::SheenColor),
					MaterialUB(UniformBlock::Component::SheenRoughness)
				);
			}
		}

		/* Anisotropy component */
		{
			const auto componentIt = m_components.find(ComponentType::Anisotropy);

			if ( componentIt != m_components.cend() )
			{
				lightGenerator.declareSurfaceAnisotropy(
					componentIt->second->variableName(),
					MaterialUB(UniformBlock::Component::AnisotropyRotation)
				);

				if ( componentIt->second->type() == Component::Type::Texture )
				{
					lightGenerator.declareSurfaceAnisotropyDirection(
						componentIt->second->variableName() + "_dir"
					);
				}
			}
			else if ( m_materialProperties[AnisotropyOffset] != 0.0F )
			{
				lightGenerator.declareSurfaceAnisotropy(
					MaterialUB(UniformBlock::Component::Anisotropy),
					MaterialUB(UniformBlock::Component::AnisotropyRotation)
				);
			}
		}

		/* Transmission component */
		{
			const auto componentIt = m_components.find(ComponentType::Transmission);

			if ( componentIt != m_components.cend() || m_materialProperties[TransmissionFactorOffset] > 0.0F )
			{
				lightGenerator.declareSurfaceTransmission(
					componentIt != m_components.cend() ? componentIt->second->variableName() : MaterialUB(UniformBlock::Component::TransmissionFactor),
					std::string{SurfaceTransmissionColor},
					MaterialUB(UniformBlock::Component::AttenuationColor),
					MaterialUB(UniformBlock::Component::AttenuationDistance),
					m_isUsingDepthBasedOpacity ? std::string{"gpWaterColumnThickness"} : MaterialUB(UniformBlock::Component::ThicknessFactor),
					/* The grab pass hands over the rendered scene in nits; the environment cubemap
					 * hands over a normalized [0,1] texel. Only the latter needs to be scaled by
					 * the sky luminance downstream. */
					m_isUsingGrabPassForTransmission
				);
			}
		}

		/* Iridescence component */
		{
			const auto componentIt = m_components.find(ComponentType::Iridescence);

			if ( componentIt != m_components.cend() || m_materialProperties[IridescenceFactorOffset] > 0.0F )
			{
				lightGenerator.declareSurfaceIridescence(
					componentIt != m_components.cend() ? componentIt->second->variableName() : MaterialUB(UniformBlock::Component::IridescenceFactor),
					MaterialUB(UniformBlock::Component::IridescenceIOR),
					MaterialUB(UniformBlock::Component::IridescenceThicknessMin),
					MaterialUB(UniformBlock::Component::IridescenceThicknessMax)
				);
			}
		}

		return true;
	}

	std::string
	StandardResource::fragmentColor () const noexcept
	{
		/* For PBR, the fragment color is the albedo (base color).
		 * The actual shading is computed by the BRDF in the light generator. */
		const auto base = this->albedoExpression();

		/* Blending modes of the opacity contract (rules 1 and 3): the alpha channel carries
		 * the opacity. Cutout mode (rule 2) keeps the base alpha — surviving texels are opaque. */
		if ( this->isFlagEnabled(OpacityEnabled) && !this->isFlagEnabled(AlphaTestEnabled) )
		{
			const auto componentIt = m_components.find(ComponentType::Opacity);

			std::stringstream code;

			if ( componentIt != m_components.cend() && componentIt->second->type() == Type::Texture )
			{
				/* Rule 3: the opacity variable is the amount-scaled texel (see the fragment codegen). */
				code << "vec4((" << base << ").rgb, " << componentIt->second->variableName() << ")";
			}
			else
			{
				/* Rule 1: global opacity from the material UBO. */
				code << "vec4((" << base << ").rgb, " << MaterialUB(UniformBlock::Component::Opacity) << ")";
			}

			return code.str();
		}

		return base;
	}

	std::shared_ptr< DescriptorSetLayout >
	StandardResource::descriptorSetLayout () const noexcept
	{
		return m_descriptorSetLayout;
	}

	uint32_t
	StandardResource::UBOIndex () const noexcept
	{
		return m_sharedUBOIndex;
	}

	uint32_t
	StandardResource::UBOAlignment () const noexcept
	{
		return m_sharedUniformBuffer->blockAlignedSize();
	}

	uint32_t
	StandardResource::UBOOffset () const noexcept
	{
		return m_sharedUBOIndex * m_sharedUniformBuffer->blockAlignedSize();
	}

	const DescriptorSet *
	StandardResource::descriptorSet () const noexcept
	{
		return m_descriptorSet.get();
	}

	Declaration::UniformBlock
	StandardResource::getUniformBlock (uint32_t set, uint32_t binding) const noexcept
	{
		Declaration::UniformBlock block{set, binding, Declaration::MemoryLayout::Std140, UniformBlock::Type::PBRMaterial, UniformBlock::Material};
		block.addMember(Declaration::VariableType::FloatVector4, UniformBlock::Component::AlbedoColor);
		block.addMember(Declaration::VariableType::Float, UniformBlock::Component::Roughness);
		block.addMember(Declaration::VariableType::Float, UniformBlock::Component::Metalness);
		block.addMember(Declaration::VariableType::Float, UniformBlock::Component::NormalScale);
		block.addMember(Declaration::VariableType::Float, UniformBlock::Component::SpecularFactor);
		block.addMember(Declaration::VariableType::Float, UniformBlock::Component::RefractionIOR);
		block.addMember(Declaration::VariableType::Float, UniformBlock::Component::IBLIntensity);
		block.addMember(Declaration::VariableType::Float, UniformBlock::Component::AutoIlluminationAmount);
		block.addMember(Declaration::VariableType::Float, UniformBlock::Component::AOIntensity);
		block.addMember(Declaration::VariableType::FloatVector4, UniformBlock::Component::AutoIlluminationColor);
		block.addMember(Declaration::VariableType::Float, UniformBlock::Component::ClearCoatFactor);
		block.addMember(Declaration::VariableType::Float, UniformBlock::Component::ClearCoatRoughness);
		block.addMember(Declaration::VariableType::Float, UniformBlock::Component::SubsurfaceIntensity);
		block.addMember(Declaration::VariableType::Float, UniformBlock::Component::SubsurfaceRadius);
		block.addMember(Declaration::VariableType::FloatVector4, UniformBlock::Component::SubsurfaceColor);
		block.addMember(Declaration::VariableType::FloatVector4, UniformBlock::Component::SheenColor);
		block.addMember(Declaration::VariableType::Float, UniformBlock::Component::SheenRoughness);
		block.addMember(Declaration::VariableType::Float, UniformBlock::Component::Anisotropy);
		block.addMember(Declaration::VariableType::Float, UniformBlock::Component::AnisotropyRotation);
		block.addMember(Declaration::VariableType::Float, UniformBlock::Component::TransmissionFactor);
		block.addMember(Declaration::VariableType::FloatVector4, UniformBlock::Component::AttenuationColor);
		block.addMember(Declaration::VariableType::Float, UniformBlock::Component::AttenuationDistance);
		block.addMember(Declaration::VariableType::Float, UniformBlock::Component::ThicknessFactor);
		block.addMember(Declaration::VariableType::Float, UniformBlock::Component::HeightScale);
		block.addMember(Declaration::VariableType::Float, UniformBlock::Component::IridescenceFactor);
		block.addMember(Declaration::VariableType::Float, UniformBlock::Component::IridescenceIOR);
		block.addMember(Declaration::VariableType::Float, UniformBlock::Component::IridescenceThicknessMin);
		block.addMember(Declaration::VariableType::Float, UniformBlock::Component::IridescenceThicknessMax);
		block.addMember(Declaration::VariableType::Float, UniformBlock::Component::Dispersion);
		block.addMember(Declaration::VariableType::FloatVector4, UniformBlock::Component::SpecularColorFactor);
		block.addMember(Declaration::VariableType::Float, UniformBlock::Component::EmissiveStrength);
		block.addMember(Declaration::VariableType::Float, UniformBlock::Component::ClearCoatNormalScale);
		block.addMember(Declaration::VariableType::Float, UniformBlock::Component::Opacity);
		block.addMember(Declaration::VariableType::Float, UniformBlock::Component::AlphaThreshold);
		block.addMember(Declaration::VariableType::Float, UniformBlock::Component::ReflectionAmount);
		block.addMember(Declaration::VariableType::Float, UniformBlock::Component::RefractionAmount);
		/* Per-component UV transforms (KHR_texture_transform): vec4 = (scale.xy, offset.zw). */
		block.addMember(Declaration::VariableType::FloatVector4, UniformBlock::Component::AlbedoUVWTransform);
		block.addMember(Declaration::VariableType::FloatVector4, UniformBlock::Component::RoughnessUVWTransform);
		block.addMember(Declaration::VariableType::FloatVector4, UniformBlock::Component::MetalnessUVWTransform);
		block.addMember(Declaration::VariableType::FloatVector4, UniformBlock::Component::NormalUVWTransform);
		block.addMember(Declaration::VariableType::FloatVector4, UniformBlock::Component::AmbientOcclusionUVWTransform);
		block.addMember(Declaration::VariableType::FloatVector4, UniformBlock::Component::AutoIlluminationUVWTransform);

		return block;
	}

	bool
	StandardResource::generateVertexShaderCode (Generator::Abstract & generator, VertexShader & vertexShader) const noexcept
	{
		if ( !this->isCreated() )
		{
			TraceError{ClassId} <<
				"The PBR material '" << this->name() << "' is not created !"
				"It can't generate a vertex shader source code.";

			return false;
		}

		const auto * geometry = generator.getGeometryInterface();

		if ( !generator.highQualityEnabled() && !generator.declareMaterialUniformBlock(*this, vertexShader, 0) )
		{
			return false;
		}

		/* Check texture coordinate attributes. */
		if ( this->usingTexture() )
		{
			if ( this->usingPrimaryTextureCoordinates() && !checkPrimaryTextureCoordinates(generator, vertexShader, *this, *geometry) )
			{
				return false;
			}

			if ( this->usingSecondaryTextureCoordinates() && !checkSecondaryTextureCoordinates(generator, vertexShader, *this, *geometry) )
			{
				return false;
			}

			/* If any texture component uses volumetric textures (e.g., AnimatedTexture2D)
			 * and the geometry only has 2D UVs, add the 3D coordinate fallback. */
			if ( !this->primaryTextureCoordinatesUses3D() )
			{
				for (const auto & component : m_components | std::views::values)
				{
					const auto * texture = dynamic_cast< const Component::Texture * >(component.get());

					if ( texture != nullptr && texture->isVolumetricTexture() )
					{
						if ( !addVolumetricTextureFallback(generator, vertexShader, *geometry) )
						{
							return false;
						}

						break;
					}
				}
			}
		}

		/* Check if the material needs vertex colors. */
		if ( this->usingVertexColors() )
		{
			if ( !geometry->vertexColorEnabled() )
			{
				TraceError{ClassId} <<
					"The geometry " << geometry->name() << " has no vertex color "
					"for PBR material '" << this->name() << "' !";

				return false;
			}

			vertexShader.requestSynthesizeInstruction(ShaderVariable::PrimaryVertexColor);
		}

		/* Reflection/IBL component setup.
		 * NOTE: Also setup for automatic reflection using bindless textures.
		 * GrabPass transmission also needs varyings (PositionWorldSpace, NormalWorldSpace, CameraWorldPosition). */
		if ( this->isComponentPresent(ComponentType::Reflection) || this->isComponentPresent(ComponentType::Refraction)
			|| m_isUsingEnvironmentCubemap || m_isUsingEnvironmentCubemapForRefraction
			|| m_isUsingEnvironmentCubemapForTransmission || m_isUsingGrabPassForTransmission )
		{
			[[maybe_unused]] const auto isCubemap = generator.renderTarget()->isCubemap();

			if ( generator.highQualityEnabled() )
			{
				vertexShader.requestSynthesizeInstruction(ShaderVariable::PositionWorldSpace);

				vertexShader.requestSynthesizeInstruction(ShaderVariable::NormalWorldSpace);

				if ( this->isComponentPresent(ComponentType::Normal) )
				{
					vertexShader.requestSynthesizeInstruction(ShaderVariable::TangentToWorldMatrix);
				}

				/* NOTE: Camera world position is read directly from View UBO instead of computing inverse(ViewMatrix).
				 * IMPORTANT: PositionWorldSpace is NOT per-face data, it's shared for the whole cubemap,
				 * so we always use 'false' for isCubemap parameter. */
				vertexShader.declare(Declaration::StageOutput{generator.getNextShaderVariableLocation(), GLSL::FloatVector3, "CameraWorldPosition", GLSL::Flat});

				Code(vertexShader) <<
					"CameraWorldPosition = " << ViewUB(UniformBlock::Component::PositionWorldSpace, false) << ".xyz;";
			}
			else
			{
				vertexShader.requestSynthesizeInstruction(ShaderVariable::PositionWorldSpace, VariableScope::Local);
				vertexShader.requestSynthesizeInstruction(ShaderVariable::NormalWorldSpace, VariableScope::Local);

				if ( this->isComponentPresent(ComponentType::Reflection) || m_isUsingEnvironmentCubemap )
				{
					vertexShader.declare(Declaration::StageOutput{generator.getNextShaderVariableLocation(), GLSL::FloatVector3, ShaderVariable::ReflectionTextureCoordinates, GLSL::Smooth});

					/* NOTE: Negate Y to convert from engine Y-DOWN to cubemap Y-UP convention (same as skybox).
					 * IMPORTANT: PositionWorldSpace is NOT per-face data, it's shared for the whole cubemap,
					 * so we always use 'false' for isCubemap parameter. */
					Code(vertexShader) <<
						"vec3 reflectDir = reflect(normalize(" << ShaderVariable::PositionWorldSpace << ".xyz - " << ViewUB(UniformBlock::Component::PositionWorldSpace, false) << ".xyz), " << ShaderVariable::NormalWorldSpace << ");" << Line::End <<
						ShaderVariable::ReflectionTextureCoordinates << " = vec3(reflectDir.x, -reflectDir.y, reflectDir.z);";
				}

				if ( this->isComponentPresent(ComponentType::Refraction) || m_isUsingEnvironmentCubemapForRefraction )
				{
					vertexShader.declare(Declaration::StageOutput{generator.getNextShaderVariableLocation(), GLSL::FloatVector3, ShaderVariable::RefractionTextureCoordinates, GLSL::Smooth});

					/* NOTE: Negate Y to convert from engine Y-DOWN to cubemap Y-UP convention (same as skybox).
					 * For refraction, we use IOR ratio: eta = 1.0 / IOR (air to material).
					 * IMPORTANT: PositionWorldSpace is NOT per-face data, it's shared for the whole cubemap,
					 * so we always use 'false' for isCubemap parameter. */
					Code(vertexShader) <<
						"float eta = 1.0 / " << MaterialUB(UniformBlock::Component::RefractionIOR) << ";" << Line::End <<
						"vec3 refractDir = refract(normalize(" << ShaderVariable::PositionWorldSpace << ".xyz - " << ViewUB(UniformBlock::Component::PositionWorldSpace, false) << ".xyz), " << ShaderVariable::NormalWorldSpace << ", eta);" << Line::End <<
						ShaderVariable::RefractionTextureCoordinates << " = vec3(refractDir.x, -refractDir.y, refractDir.z);";
				}
			}
		}

		/* Parallax Occlusion Mapping vertex requirements.
		 * NOTE: POM needs TangentToWorldMatrix, PositionWorldSpace, and CameraWorldPosition in the fragment shader.
		 * If Reflection/Refraction already requested them, this is a no-op for the synthesize calls.
		 * When POM iterations is 0, POM is completely disabled - no extra vertex outputs needed. */
		if ( m_useParallaxOcclusionMapping && generator.pomIterations() > 0 )
		{
			vertexShader.requestSynthesizeInstruction(ShaderVariable::PositionWorldSpace);
			vertexShader.requestSynthesizeInstruction(ShaderVariable::TangentToWorldMatrix);

			/* CameraWorldPosition stage output (if not already declared by Reflection/Refraction). */
			if ( !this->isComponentPresent(ComponentType::Reflection) && !m_isUsingEnvironmentCubemap
				&& !this->isComponentPresent(ComponentType::Refraction) && !m_isUsingEnvironmentCubemapForRefraction )
			{
				vertexShader.declare(Declaration::StageOutput{generator.getNextShaderVariableLocation(), GLSL::FloatVector3, "CameraWorldPosition", GLSL::Flat});

				Code(vertexShader) <<
					"CameraWorldPosition = " << ViewUB(UniformBlock::Component::PositionWorldSpace, false) << ".xyz;";
			}
		}

		return true;
	}

	bool
	StandardResource::generateTextureComponentFragmentShader (ComponentType componentType, const std::function< bool (FragmentShader &, const Texture *) > & codeGenerator, FragmentShader & fragmentShader, uint32_t materialSet) const noexcept
	{
		const auto componentIt = m_components.find(componentType);

		/* NOTE: The component must exist and use a texture. */
		if ( componentIt == m_components.cend() || componentIt->second->type() != Type::Texture )
		{
			return true;
		}

		const auto * component = dynamic_cast< const Texture * >(componentIt->second.get());

		if ( !fragmentShader.declare(Declaration::Sampler{materialSet, component->binding(), component->textureType(), component->samplerName()}) )
		{
			return false;
		}

		return codeGenerator(fragmentShader, component);
	}

	const char *
	StandardResource::textCoords (const Texture * component) const noexcept
	{
		if ( component->isVolumetricTexture() )
		{
			return ShaderVariable::Primary3DTextureCoordinates;
		}

		if ( m_pomGenerationActive )
		{
			return ShaderVariable::ParallaxTextureCoordinates;
		}

		return ShaderVariable::Primary2DTextureCoordinates;
	}

	std::string
	StandardResource::transformedTexCoords (ComponentType componentType, const Texture * component) const noexcept
	{
		/* The per-component UV transform (KHR_texture_transform / JSON "UVW" keys) is a
		 * material UBO vec4 (scale.xy, offset.zw) applied UNCONDITIONALLY — the neutral
		 * value is the identity, and going through the UBO (values, never GLSL literals)
		 * respects the shader program cache contract. 2D coordinates only: volumetric
		 * textures keep the plain lookup. */
		if ( component->isVolumetricTexture() )
		{
			return textCoords(component);
		}

		const char * transformKey = nullptr;

		switch ( componentType )
		{
			case ComponentType::Albedo :
				transformKey = UniformBlock::Component::AlbedoUVWTransform;
				break;

			case ComponentType::Roughness :
				transformKey = UniformBlock::Component::RoughnessUVWTransform;
				break;

			case ComponentType::Metalness :
				transformKey = UniformBlock::Component::MetalnessUVWTransform;
				break;

			case ComponentType::Normal :
				transformKey = UniformBlock::Component::NormalUVWTransform;
				break;

			case ComponentType::AmbientOcclusion :
				transformKey = UniformBlock::Component::AmbientOcclusionUVWTransform;
				break;

			case ComponentType::AutoIllumination :
				transformKey = UniformBlock::Component::AutoIlluminationUVWTransform;
				break;

			default :
				/* No transform slot for this component: plain coordinates. */
				return textCoords(component);
		}

		std::stringstream expression;
		expression << "(" << textCoords(component) << " * " << MaterialUB(transformKey) << ".xy + " << MaterialUB(transformKey) << ".zw)";

		return expression.str();
	}

	bool
	StandardResource::generateBindlessReflectionFragmentShader (const Generator::Abstract & generator, FragmentShader & fragmentShader) const noexcept
	{
		/* NOTE: For automatic reflection with bindless textures, we use the scene's environment
		 * cubemap from the bindless array. No per-material reflection component is needed. */

		/* Get the bindless set index from the program. */
		const auto bindlessSetIndex = generator.shaderProgram()->setIndex(SetType::PerBindless);

		/* Enable the nonuniform qualifier extension. */
		fragmentShader.setExtensionBehavior(GLSL::Extension::NonUniformQualifier, GLSL::Extension::Require);

		/* Declare the global bindless cubemap array (unbounded). Several features and
		 * subsystems (materials, LightGenerator) declare it on use; a re-declaration is a
		 * silent no-op (see the Saphir declare de-duplication contract). */
		if ( !fragmentShader.declare(Declaration::Sampler{
			bindlessSetIndex,
			BindlessTextureManager::TextureCubeBinding,
			GLSL::SamplerCube,
			Bindless::TexturesCube,
			Declaration::Sampler::UnboundedArray}) )
		{
			TraceError{ClassId} << "Failed to declare bindless cubemap sampler array !";

			return false;
		}

		/* Roughness expression driving the prefiltered LOD — same source of truth as
		 * setupLightGenerator(): the roughness texture component when present (declared at
		 * Location::Top by the component generation, which already folds the source channel,
		 * inversion and factor into the variable), the material UBO value otherwise. */
		const auto roughnessExpression = [this] () -> std::string {
			const auto componentIt = m_components.find(ComponentType::Roughness);

			if ( componentIt != m_components.cend() )
			{
				return componentIt->second->variableName();
			}

			return MaterialUB(UniformBlock::Component::Roughness);
		}();

		/* The prefiltered chain maps roughness 0..1 onto its mip levels (mip 0 is an exact
		 * copy of the environment: perfect mirrors keep their sharpness). */
		const auto reflectionLOD = "clamp(" + roughnessExpression + ", 0.0, 1.0) * " + std::to_string(IBLTexture::PrefilteredMipLevels - 1) + ".0";

		/* Generate the reflection sampling code using bindless textures. */
		if ( generator.highQualityEnabled() )
		{
			if ( this->isComponentPresent(ComponentType::Normal) )
			{
				Code(fragmentShader, Location::Top) << "const vec3 reflectionNormal = normalize(" << ShaderVariable::TangentToWorldMatrix << "[0] * " << SurfaceNormalVector << ".x + " << ShaderVariable::TangentToWorldMatrix << "[1] * " << SurfaceNormalVector << ".y + " << ShaderVariable::NormalWorldSpace << " * " << SurfaceNormalVector << ".z);";
			}
			else
			{
				Code(fragmentShader, Location::Top) << "const vec3 reflectionNormal = normalize(" << ShaderVariable::NormalWorldSpace << ");";
			}

			/* NOTE: Negate Y to convert from engine Y-DOWN to cubemap Y-UP convention (same as skybox). */
			Code(fragmentShader, Location::Top) <<
				"const vec3 reflectionI = normalize(" << ShaderVariable::PositionWorldSpace << ".xyz - CameraWorldPosition);" << Line::End <<
				"const vec3 reflectDir = reflect(reflectionI, reflectionNormal);" << Line::End <<
				"const vec3 " << ShaderVariable::ReflectionTextureCoordinates << " = vec3(reflectDir.x, -reflectDir.y, reflectDir.z);" << Line::End <<
				"const vec4 " << SurfaceReflectionColor << " = textureLod(" << Bindless::TexturesCube << "[" << GLSL::Functions::NonUniformEXT << "(" << BindlessTextureManager::PrefilteredCubemapSlot << ")]" << ", " << ShaderVariable::ReflectionTextureCoordinates << ", " << reflectionLOD << ");";
		}
		else
		{
			/* Low quality: use pre-computed reflection coordinates from vertex shader.
			 * NOTE: Reflection direction was already computed in vertex shader and passed via ReflectionTextureCoordinates. */
			Code(fragmentShader, Location::Top) <<
				"const vec4 " << SurfaceReflectionColor << " = textureLod(" << Bindless::TexturesCube << "[" << GLSL::Functions::NonUniformEXT << "(" << BindlessTextureManager::PrefilteredCubemapSlot << ")]" << ", " << ShaderVariable::ReflectionTextureCoordinates << ", " << reflectionLOD << ");";
		}

		return true;
	}

	bool
	StandardResource::generateBindlessRefractionFragmentShader (const Generator::Abstract & generator, FragmentShader & fragmentShader) const noexcept
	{
		/* NOTE: For automatic refraction with bindless textures, we use the scene's environment
		 * cubemap from the bindless array. No per-material refraction component is needed. */

		/* Get the bindless set index from the program. */
		const auto bindlessSetIndex = generator.shaderProgram()->setIndex(SetType::PerBindless);

		/* Enable the nonuniform qualifier extension. */
		fragmentShader.setExtensionBehavior(GLSL::Extension::NonUniformQualifier, GLSL::Extension::Require);

		/* Declare the global bindless cubemap array (unbounded). Several features and
		 * subsystems (materials, LightGenerator) declare it on use; a re-declaration is a
		 * silent no-op (see the Saphir declare de-duplication contract). */
		if ( !fragmentShader.declare(Declaration::Sampler{
			bindlessSetIndex,
			BindlessTextureManager::TextureCubeBinding,
			GLSL::SamplerCube,
			Bindless::TexturesCube,
			Declaration::Sampler::UnboundedArray}) )
		{
			TraceError{ClassId} << "Failed to declare bindless cubemap sampler array for refraction !";

			return false;
		}

		/* Generate the refraction sampling code using bindless textures. */
		if ( generator.highQualityEnabled() )
		{
			/* High quality: compute refraction direction in fragment shader.
			 * NOTE: Reuse reflectionNormal and reflectionI if already declared by reflection (explicit component or bindless).
			 * When m_isUsingEnvironmentCubemap is true, bindless reflection is always generated before refraction. */
			const bool reflectionAlreadyDeclared = this->isComponentPresent(ComponentType::Reflection) || m_isUsingEnvironmentCubemap;

			if ( !reflectionAlreadyDeclared )
			{
				if ( this->isComponentPresent(ComponentType::Normal) )
				{
					Code(fragmentShader, Location::Top) << "const vec3 reflectionNormal = normalize(" << ShaderVariable::TangentToWorldMatrix << "[0] * " << SurfaceNormalVector << ".x + " << ShaderVariable::TangentToWorldMatrix << "[1] * " << SurfaceNormalVector << ".y + " << ShaderVariable::NormalWorldSpace << " * " << SurfaceNormalVector << ".z);";
				}
				else
				{
					Code(fragmentShader, Location::Top) << "const vec3 reflectionNormal = normalize(" << ShaderVariable::NormalWorldSpace << ");";
				}

				Code(fragmentShader, Location::Top) << "const vec3 reflectionI = normalize(" << ShaderVariable::PositionWorldSpace << ".xyz - CameraWorldPosition);";
			}

			/* NOTE: Negate Y to convert from engine Y-DOWN to cubemap Y-UP convention (same as skybox).
			 * For refraction, eta = 1.0 / IOR (air to material).
			 * When chromatic dispersion is enabled, sample R/G/B with separate IORs (Cauchy dispersion). */
			if ( m_materialProperties[DispersionOffset] > 0.0F )
			{
				/* Chromatic dispersion: 3 refraction rays with different IORs per channel. */
				Code(fragmentShader, Location::Top) <<
					"const float baseIOR = " << MaterialUB(UniformBlock::Component::RefractionIOR) << ";" << Line::End <<
					"const float dispersionSpread = (baseIOR - 1.0) * " << MaterialUB(UniformBlock::Component::Dispersion) << " / 20.0;" << Line::End <<
					"const float etaR = 1.0 / (baseIOR - dispersionSpread * 0.5);" << Line::End <<
					"const float etaG = 1.0 / baseIOR;" << Line::End <<
					"const float etaB = 1.0 / (baseIOR + dispersionSpread * 0.5);" << Line::End <<
					"const vec3 refractDirR = refract(reflectionI, reflectionNormal, etaR);" << Line::End <<
					"const vec3 refractDirG = refract(reflectionI, reflectionNormal, etaG);" << Line::End <<
					"const vec3 refractDirB = refract(reflectionI, reflectionNormal, etaB);" << Line::End <<
					"float convergR = texture(" << Bindless::TexturesCube << "[" << GLSL::Functions::NonUniformEXT << "(" << BindlessTextureManager::EnvironmentCubemapSlot << ")]" << ", vec3(refractDirR.x, -refractDirR.y, refractDirR.z)).r;" << Line::End <<
					"float convergG = texture(" << Bindless::TexturesCube << "[" << GLSL::Functions::NonUniformEXT << "(" << BindlessTextureManager::EnvironmentCubemapSlot << ")]" << ", vec3(refractDirG.x, -refractDirG.y, refractDirG.z)).g;" << Line::End <<
					"float convergB = texture(" << Bindless::TexturesCube << "[" << GLSL::Functions::NonUniformEXT << "(" << BindlessTextureManager::EnvironmentCubemapSlot << ")]" << ", vec3(refractDirB.x, -refractDirB.y, refractDirB.z)).b;" << Line::End <<
					"const vec4 " << SurfaceRefractionColor << " = vec4(convergR, convergG, convergB, 1.0);";
			}
			else
			{
				Code(fragmentShader, Location::Top) <<
					"const float refractionEta = 1.0 / " << MaterialUB(UniformBlock::Component::RefractionIOR) << ";" << Line::End <<
					"const vec3 refractDir = refract(reflectionI, reflectionNormal, refractionEta);" << Line::End <<
					"const vec3 " << ShaderVariable::RefractionTextureCoordinates << " = vec3(refractDir.x, -refractDir.y, refractDir.z);" << Line::End <<
					"const vec4 " << SurfaceRefractionColor << " = texture(" << Bindless::TexturesCube << "[" << GLSL::Functions::NonUniformEXT << "(" << BindlessTextureManager::EnvironmentCubemapSlot << ")]" << ", " << ShaderVariable::RefractionTextureCoordinates << ");";
			}
		}
		else
		{
			/* Low quality: use pre-computed refraction coordinates from vertex shader.
			 * NOTE: Refraction direction was already computed in vertex shader and passed via RefractionTextureCoordinates. */
			Code(fragmentShader, Location::Top) <<
				"const vec4 " << SurfaceRefractionColor << " = texture(" << Bindless::TexturesCube << "[" << GLSL::Functions::NonUniformEXT << "(" << BindlessTextureManager::EnvironmentCubemapSlot << ")]" << ", " << ShaderVariable::RefractionTextureCoordinates << ");";
		}

		return true;
	}

	bool
	StandardResource::generateBindlessTransmissionFragmentShader (const Generator::Abstract & generator, FragmentShader & fragmentShader) const noexcept
	{
		/* NOTE: For automatic transmission with bindless textures, we sample the scene's prefiltered
		 * environment cubemap with LOD based on roughness for frosted glass effects. */

		/* Get the bindless set index from the program. */
		const auto bindlessSetIndex = generator.shaderProgram()->setIndex(SetType::PerBindless);

		/* Enable the nonuniform qualifier extension. */
		fragmentShader.setExtensionBehavior(GLSL::Extension::NonUniformQualifier, GLSL::Extension::Require);

		/* Declare the global bindless cubemap array (unbounded). Several features and
		 * subsystems (materials, LightGenerator) declare it on use; a re-declaration is a
		 * silent no-op (see the Saphir declare de-duplication contract). */
		if ( !fragmentShader.declare(Declaration::Sampler{
			bindlessSetIndex,
			BindlessTextureManager::TextureCubeBinding,
			GLSL::SamplerCube,
			Bindless::TexturesCube,
			Declaration::Sampler::UnboundedArray}) )
		{
			TraceError{ClassId} << "Failed to declare bindless cubemap sampler array for transmission !";

			return false;
		}

		/* Generate the transmission sampling code using bindless prefiltered cubemap. */
		if ( generator.highQualityEnabled() )
		{
			/* Ensure reflectionI and reflectionNormal are available.
			 * If reflection or refraction already declared them, reuse; otherwise declare them. */
			const bool reflectionAlreadyDeclared = this->isComponentPresent(ComponentType::Reflection) || m_isUsingEnvironmentCubemap || this->isComponentPresent(ComponentType::Refraction) || m_isUsingEnvironmentCubemapForRefraction;

			if ( !reflectionAlreadyDeclared )
			{
				if ( this->isComponentPresent(ComponentType::Normal) )
				{
					Code(fragmentShader, Location::Top) << "const vec3 reflectionNormal = normalize(" << ShaderVariable::TangentToWorldMatrix << "[0] * " << SurfaceNormalVector << ".x + " << ShaderVariable::TangentToWorldMatrix << "[1] * " << SurfaceNormalVector << ".y + " << ShaderVariable::NormalWorldSpace << " * " << SurfaceNormalVector << ".z);";
				}
				else
				{
					Code(fragmentShader, Location::Top) << "const vec3 reflectionNormal = normalize(" << ShaderVariable::NormalWorldSpace << ");";
				}

				Code(fragmentShader, Location::Top) << "const vec3 reflectionI = normalize(" << ShaderVariable::PositionWorldSpace << ".xyz - CameraWorldPosition);";
			}

			/* Sample the REAL prefiltered cubemap (reserved slot 2) — the frosted-glass LOD
			 * used to read the raw environment whose mips did not even exist.
			 * NOTE: Transmission goes through the surface (not reflected), so we use the view direction
			 * but with Y flipped for cubemap convention.
			 * The LOD reads the per-pixel roughness variable when a texture component drives it
			 * (the UBO scalar is only the FACTOR in that case), the UBO value otherwise. */
			const auto componentIt = m_components.find(ComponentType::Roughness);
			const auto transmissionRoughness = componentIt != m_components.cend()
				? componentIt->second->variableName()
				: std::string{MaterialUB(UniformBlock::Component::Roughness)};

			Code(fragmentShader, Location::Top) <<
				"const vec3 transmissionDir = vec3(reflectionI.x, -reflectionI.y, reflectionI.z);" << Line::End <<
				"const float transmissionLod = clamp(" << transmissionRoughness << ", 0.0, 1.0) * " << (IBLTexture::PrefilteredMipLevels - 1) << ".0;" << Line::End <<
				"const vec3 " << SurfaceTransmissionColor << " = textureLod(" << Bindless::TexturesCube << "[" << GLSL::Functions::NonUniformEXT << "(" << BindlessTextureManager::PrefilteredCubemapSlot << ")]" << ", transmissionDir, transmissionLod).rgb;";
		}
		else
		{
			/* Low quality: sample at a fixed LOD using the reflection coordinates (view direction approximation). */
			Code(fragmentShader, Location::Top) <<
				"const vec3 transmissionDir = vec3(" << ShaderVariable::ReflectionTextureCoordinates << ".x, -" << ShaderVariable::ReflectionTextureCoordinates << ".y, " << ShaderVariable::ReflectionTextureCoordinates << ".z);" << Line::End <<
				"const vec3 " << SurfaceTransmissionColor << " = textureLod(" << Bindless::TexturesCube << "[" << GLSL::Functions::NonUniformEXT << "(" << BindlessTextureManager::PrefilteredCubemapSlot << ")]" << ", transmissionDir, 3.0).rgb;";
		}

		return true;
	}

	bool
	StandardResource::generateGrabPassTransmissionFragmentShader (const Generator::Abstract & generator, FragmentShader & fragmentShader) const noexcept
	{
		/* NOTE: Screen-space transmission using the GrabPass texture (bindless 2D slot 4).
		 * Samples the captured framebuffer with UV distortion based on IOR and surface normal
		 * to simulate real refraction of the actual scene background. */

		/* Get the bindless set index from the program. */
		const auto bindlessSetIndex = generator.shaderProgram()->setIndex(SetType::PerBindless);

		/* Enable the nonuniform qualifier extension. */
		fragmentShader.setExtensionBehavior(GLSL::Extension::NonUniformQualifier, GLSL::Extension::Require);

		/* Declare the bindless 2D texture array if not already declared.
		 * NOTE: Check if any other path already declared this binding. */
		if ( !fragmentShader.declare(Declaration::Sampler{
			bindlessSetIndex,
			BindlessTextureManager::Texture2DBinding,
			GLSL::Sampler2D,
			Bindless::Textures2D,
			Declaration::Sampler::UnboundedArray}) )
		{
			/* Declaration may fail if already declared — this is acceptable. */
		}

		if ( generator.highQualityEnabled() )
		{
			/* High quality: use reflectionNormal and reflectionI for accurate per-pixel refraction. */
			const bool reflectionAlreadyDeclared = this->isComponentPresent(ComponentType::Reflection) || m_isUsingEnvironmentCubemap
				|| this->isComponentPresent(ComponentType::Refraction) || m_isUsingEnvironmentCubemapForRefraction;

			if ( !reflectionAlreadyDeclared )
			{
				if ( this->isComponentPresent(ComponentType::Normal) )
				{
					Code(fragmentShader, Location::Top) << "const vec3 reflectionNormal = normalize(" << ShaderVariable::TangentToWorldMatrix << "[0] * " << SurfaceNormalVector << ".x + " << ShaderVariable::TangentToWorldMatrix << "[1] * " << SurfaceNormalVector << ".y + " << ShaderVariable::NormalWorldSpace << " * " << SurfaceNormalVector << ".z);";
				}
				else
				{
					Code(fragmentShader, Location::Top) << "const vec3 reflectionNormal = normalize(" << ShaderVariable::NormalWorldSpace << ");";
				}

				Code(fragmentShader, Location::Top) << "const vec3 reflectionI = normalize(" << ShaderVariable::PositionWorldSpace << ".xyz - CameraWorldPosition);";
			}

			if ( m_materialProperties[DispersionOffset] > 0.0F )
			{
				/* Chromatic dispersion: 3 GrabPass samples with different IOR offsets per channel. */
				Code(fragmentShader, Location::Top) <<
					"vec2 gpScreenUV = gl_FragCoord.xy / vec2(textureSize(" << Bindless::Textures2D << "[" << GLSL::Functions::NonUniformEXT << "(" << BindlessTextureManager::GrabPassSlot << ")]" << ", 0));" << Line::End <<
					"float baseIOR = " << MaterialUB(UniformBlock::Component::RefractionIOR) << ";" << Line::End <<
					"float gpSpread = (baseIOR - 1.0) * " << MaterialUB(UniformBlock::Component::Dispersion) << " / 20.0;" << Line::End <<
					"float gpEtaR = 1.0 / (baseIOR - gpSpread * 0.5);" << Line::End <<
					"float gpEtaG = 1.0 / baseIOR;" << Line::End <<
					"float gpEtaB = 1.0 / (baseIOR + gpSpread * 0.5);" << Line::End <<
					"vec3 gpRefractDirR = refract(reflectionI, reflectionNormal, gpEtaR);" << Line::End <<
					"vec3 gpRefractDirG = refract(reflectionI, reflectionNormal, gpEtaG);" << Line::End <<
					"vec3 gpRefractDirB = refract(reflectionI, reflectionNormal, gpEtaB);" << Line::End <<
					"float gpThickness = " << MaterialUB(UniformBlock::Component::ThicknessFactor) << ";" << Line::End <<
					"vec2 gpOffsetR = gpRefractDirR.xy * gpThickness * 0.05;" << Line::End <<
					"vec2 gpOffsetG = gpRefractDirG.xy * gpThickness * 0.05;" << Line::End <<
					"vec2 gpOffsetB = gpRefractDirB.xy * gpThickness * 0.05;" << Line::End <<
					"float convergR = texture(" << Bindless::Textures2D << "[" << GLSL::Functions::NonUniformEXT << "(" << BindlessTextureManager::GrabPassSlot << ")]" << ", clamp(gpScreenUV + gpOffsetR, vec2(0.001), vec2(0.999))).r;" << Line::End <<
					"float convergG = texture(" << Bindless::Textures2D << "[" << GLSL::Functions::NonUniformEXT << "(" << BindlessTextureManager::GrabPassSlot << ")]" << ", clamp(gpScreenUV + gpOffsetG, vec2(0.001), vec2(0.999))).g;" << Line::End <<
					"float convergB = texture(" << Bindless::Textures2D << "[" << GLSL::Functions::NonUniformEXT << "(" << BindlessTextureManager::GrabPassSlot << ")]" << ", clamp(gpScreenUV + gpOffsetB, vec2(0.001), vec2(0.999))).b;" << Line::End <<
					"const vec3 " << SurfaceTransmissionColor << " = vec3(convergR, convergG, convergB);";
			}
			else
			{
				/* Standard GrabPass refraction without dispersion. */
				Code(fragmentShader, Location::Top) <<
					"vec2 gpScreenUV = gl_FragCoord.xy / vec2(textureSize(" << Bindless::Textures2D << "[" << GLSL::Functions::NonUniformEXT << "(" << BindlessTextureManager::GrabPassSlot << ")]" << ", 0));" << Line::End <<
					"vec3 gpViewDir = reflectionI;" << Line::End <<
					"float gpEta = 1.0 / " << MaterialUB(UniformBlock::Component::RefractionIOR) << ";" << Line::End <<
					"vec3 gpRefractDir = refract(gpViewDir, reflectionNormal, gpEta);" << Line::End <<
					"vec2 gpOffset = gpRefractDir.xy * " << MaterialUB(UniformBlock::Component::ThicknessFactor) << " * 0.05;" << Line::End <<
					"vec2 gpRefractedUV = clamp(gpScreenUV + gpOffset, vec2(0.001), vec2(0.999));" << Line::End <<
					"const vec3 " << SurfaceTransmissionColor << " = texture(" << Bindless::Textures2D << "[" << GLSL::Functions::NonUniformEXT << "(" << BindlessTextureManager::GrabPassSlot << ")]" << ", gpRefractedUV).rgb;";
			}
		}
		else
		{
			/* Low quality: use a simple offset based on the normal world space and gl_FragCoord. */
			Code(fragmentShader, Location::Top) <<
				"vec2 gpScreenUV = gl_FragCoord.xy / vec2(textureSize(" << Bindless::Textures2D << "[" << GLSL::Functions::NonUniformEXT << "(" << BindlessTextureManager::GrabPassSlot << ")]" << ", 0));" << Line::End <<
				"vec2 gpOffset = " << ShaderVariable::NormalWorldSpace << ".xy * " << MaterialUB(UniformBlock::Component::ThicknessFactor) << " * 0.03;" << Line::End <<
				"vec2 gpRefractedUV = clamp(gpScreenUV + gpOffset, vec2(0.001), vec2(0.999));" << Line::End <<
				"const vec3 " << SurfaceTransmissionColor << " = texture(" << Bindless::Textures2D << "[" << GLSL::Functions::NonUniformEXT << "(" << BindlessTextureManager::GrabPassSlot << ")]" << ", gpRefractedUV).rgb;";
		}

		/* Depth-based opacity: sample the grab pass depth buffer to compute water column thickness. */
		if ( m_isUsingDepthBasedOpacity )
		{
			/* Ensure the view UBO is declared in the fragment shader (needed for near/far planes). */
			static_cast< void >(generator.declareViewUniformBlock(fragmentShader));

			Code(fragmentShader, Location::Top) <<
				Line::End <<
				"/* Depth-based water opacity */" << Line::End <<
				"float gpSceneDepth = texture(" << Bindless::Textures2D << "[" << GLSL::Functions::NonUniformEXT << "(" << BindlessTextureManager::GrabPassDepthSlot << ")]" << ", gpScreenUV).r;" << Line::End <<
				"float gpNear = " << UniformBlock::View << "." << UniformBlock::Component::ViewProperties << ".z;" << Line::End <<
				"float gpFar = " << UniformBlock::View << "." << UniformBlock::Component::ViewProperties << ".w;" << Line::End <<
				"float gpLinearSceneDepth = gpNear * gpFar / (gpFar - gpSceneDepth * (gpFar - gpNear));" << Line::End <<
				"float gpLinearWaterDepth = gpNear * gpFar / (gpFar - gl_FragCoord.z * (gpFar - gpNear));" << Line::End <<
				"float gpWaterColumnThickness = max(gpLinearSceneDepth - gpLinearWaterDepth, 0.0);";
		}

		return true;
	}

	bool
	StandardResource::generateFragmentShaderCode (Generator::Abstract & generator, LightGenerator & lightGenerator, FragmentShader & fragmentShader) const noexcept
	{
		if ( !this->isCreated() )
		{
			TraceError{ClassId} <<
				"The PBR material '" << this->name() << "' is not created !"
				"It can't generate a fragment shader source code.";

			return false;
		}

		if ( !generator.declareMaterialUniformBlock(*this, fragmentShader, 0) )
		{
			return false;
		}

		const uint32_t materialSet = generator.shaderProgram()->setIndex(SetType::PerModelLayer);

		/* Parallax Occlusion Mapping (Height component).
		 * NOTE: Must be generated FIRST, before any other texture sampling, so that
		 * all subsequent texture() calls use the parallax-displaced UVs (pomTexCoords).
		 * When POM iterations is 0, POM is completely disabled and textCoords() returns original UVs. */
		m_pomGenerationActive = m_useParallaxOcclusionMapping && generator.pomIterations() > 0;

		if ( m_pomGenerationActive )
		{
			const auto maxPOMIterations = generator.pomIterations();
			const auto minPOMIterations = std::max(maxPOMIterations / 4, 2);

			if ( !this->generateTextureComponentFragmentShader(ComponentType::Displacement, [maxPOMIterations, minPOMIterations] (FragmentShader & shader, const Texture * component) {
				/* Inline POM ray-marching directly in main().
				 * NOTE: Cannot use Declaration::Function because functions are emitted before
				 * sampler declarations in the generated GLSL, causing 'undeclared identifier' errors. */
				Code{shader, Location::Top} <<
					"vec3 pomViewDir = normalize(transpose(" << ShaderVariable::TangentToWorldMatrix << ") * (CameraWorldPosition - " << ShaderVariable::PositionWorldSpace << ".xyz));" << Line::End <<
					"/* Distance-based POM fade: full effect within 8 units, disabled beyond 18. */" << Line::End <<
					"float pomFade = 1.0 - smoothstep(8.0, 18.0, length(CameraWorldPosition - " << ShaderVariable::PositionWorldSpace << ".xyz));" << Line::End <<
					"vec2 " << ShaderVariable::ParallaxTextureCoordinates << ";" << Line::End <<
					"if (pomFade < 0.001) {" << Line::End <<
					"  " << ShaderVariable::ParallaxTextureCoordinates << " = " << ShaderVariable::Primary2DTextureCoordinates << ";" << Line::End <<
					"} else {" << Line::End <<
					"  int angleLayers = clamp(int(mix(" << std::to_string(maxPOMIterations) << ".0, " << std::to_string(minPOMIterations) << ".0, max(dot(vec3(0.0, 0.0, 1.0), pomViewDir), 0.0))), " << std::to_string(minPOMIterations) << ", " << std::to_string(maxPOMIterations) << ");" << Line::End <<
					"  int numLayers = max(int(float(angleLayers) * pomFade), " << std::to_string(minPOMIterations) << ");" << Line::End <<
					"  float layerDepth = 1.0 / float(numLayers);" << Line::End <<
					"  float currentLayerDepth = 0.0;" << Line::End <<
					"  vec2 deltaTexCoords = pomViewDir.xy * (" << MaterialUB(UniformBlock::Component::HeightScale) << " * pomFade) / float(numLayers);" << Line::End <<
					"  vec2 currentTexCoords = " << ShaderVariable::Primary2DTextureCoordinates << ";" << Line::End <<
					"  float currentDepthMapValue = 1.0 - texture(" << component->samplerName() << ", currentTexCoords).r;" << Line::End <<
					"  for (int i = 0; i < " << std::to_string(maxPOMIterations) << "; i++) {" << Line::End <<
					"	if (currentLayerDepth >= currentDepthMapValue) break;" << Line::End <<
					"	currentTexCoords -= deltaTexCoords;" << Line::End <<
					"	currentDepthMapValue = 1.0 - texture(" << component->samplerName() << ", currentTexCoords).r;" << Line::End <<
					"	currentLayerDepth += layerDepth;" << Line::End <<
					"  }" << Line::End <<
					"  vec2 prevTexCoords = currentTexCoords + deltaTexCoords;" << Line::End <<
					"  float afterDepth = currentDepthMapValue - currentLayerDepth;" << Line::End <<
					"  float beforeDepth = 1.0 - texture(" << component->samplerName() << ", prevTexCoords).r - currentLayerDepth + layerDepth;" << Line::End <<
					"  float denom = afterDepth - beforeDepth;" << Line::End <<
					"  float weight = (abs(denom) > 0.0001) ? clamp(afterDepth / denom, 0.0, 1.0) : 0.5;" << Line::End <<
					"  " << ShaderVariable::ParallaxTextureCoordinates << " = prevTexCoords * weight + currentTexCoords * (1.0 - weight);" << Line::End <<
					"}";

				return true;
			}, fragmentShader, materialSet) )
			{
				TraceError{ClassId} << "Unable to generate fragment code for the height/POM component of PBR material '" << this->name() << "' !";

				return false;
			}
		}

		/* Normal component.
		 * NOTE: Get a sample from a texture in range [0,1], convert it to a normalized range of [-1, 1].
		 * Always generated (including ambient pass) because the MRT needs the perturbed normal
		 * for post-process effects (RTR, SSR, SSAO, RTAO). */
		if ( !this->generateTextureComponentFragmentShader(ComponentType::Normal, [this] (FragmentShader & shader, const Texture * component) {
			Code{shader, Location::Top} <<
				"const vec3 " << component->variableName() << "_raw = texture(" << component->samplerName() << ", " << transformedTexCoords(ComponentType::Normal, component) << ").rgb * 2.0 - 1.0;" << Line::End <<
				"const vec3 " << component->variableName() << " = normalize(vec3(" << component->variableName() << "_raw.xy * " << MaterialUB(UniformBlock::Component::NormalScale) << ", " << component->variableName() << "_raw.z));";

			return true;
		}, fragmentShader, materialSet) )
		{
			TraceError{ClassId} << "Unable to generate fragment code for the normal component of PBR material '" << this->name() << "' !";

			return false;
		}

		/* Albedo component.
		 * The sampled texel is multiplied by the albedo colour, which acts as a TINT factor —
		 * this is what glTF baseColorFactor and FBX base_color mean when a texture is also
		 * present, and both specify the PRODUCT. The multiplication is unconditional on
		 * purpose: the shader program cache keys on the descriptor layout, never on values,
		 * so a value-dependent variant could serve one material's program to another.
		 * DefaultAlbedoColor is White precisely so this stays an identity when untouched. */
		if ( !this->generateTextureComponentFragmentShader(ComponentType::Albedo, [this] (FragmentShader & shader, const Texture * component) {
			Code{shader, Location::Top} << "const vec4 " << component->variableName() << " = texture(" << component->samplerName() << ", " << transformedTexCoords(ComponentType::Albedo, component) << ") * " << MaterialUB(UniformBlock::Component::AlbedoColor) << ";";

			return true;
		}, fragmentShader, materialSet) )
		{
			TraceError{ClassId} << "Unable to generate fragment code for the albedo component of PBR material '" << this->name() << "' !";

			return false;
		}

		/* Vertex colours MODULATE the albedo (glTF COLOR_0 contract, owner decision). Folded ONCE
		 * into a single variable, declared before every consumer: the BRDF, the fragment colour
		 * and the alpha test must all see the SAME post-modulation value — testing coverage on
		 * the raw albedo while shading the modulated one is exactly how the two diverge. */
		if ( this->usingVertexColors() )
		{
			const auto rawAlbedo = this->isComponentPresent(ComponentType::Albedo)
				? std::string{m_components.at(ComponentType::Albedo)->variableName()}
				: MaterialUB(UniformBlock::Component::AlbedoColor);

			Code{fragmentShader, Location::Top} << "const vec4 " << SurfaceAlbedoFinal << " = " << rawAlbedo << " * " << ShaderVariable::PrimaryVertexColor << ";";
		}

		/* Cutout on the albedo alpha channel (glTF alphaMode MASK without a dedicated opacity
		 * map): the albedo sample carries the coverage. The threshold is a UBO VALUE, never a
		 * shader literal (program-cache contract). A dedicated opacity component, when present,
		 * owns the test instead (see the opacity component block below). */
		if ( this->isFlagEnabled(AlphaTestEnabled) && !this->isComponentPresent(ComponentType::Opacity) && ( this->isComponentPresent(ComponentType::Albedo) || this->usingVertexColors() ) )
		{
			Code{fragmentShader, Location::Top} << "if ( " << this->albedoExpression() << ".a < " << MaterialUB(UniformBlock::Component::AlphaThreshold) << " ) { discard; }";
		}

		/* Roughness component.
		 * The source channel is a component property (glTF packed metallic-roughness: Green;
		 * grayscale maps: Red). The material scalar MULTIPLIES the texel — the glTF
		 * 'roughnessFactor * texel' contract; the neutral factor is 1.0 (DefaultTextureFactor).
		 * Inversion (smoothness/gloss source) applies to the texel BEFORE the factor, so every
		 * consumer (BRDF, IBL LOD, G-buffer) reads the final value from this single variable. */
		if ( !this->generateTextureComponentFragmentShader(ComponentType::Roughness, [this] (FragmentShader & shader, const Texture * component) {
			const std::string texel = "texture(" + std::string{component->samplerName()} + ", " + transformedTexCoords(ComponentType::Roughness, component) + ")." + component->sourceChannelSwizzle();

			Code{shader, Location::Top} << "const float " << component->variableName() << " = " << ( m_invertRoughness ? "(1.0 - " + texel + ")" : texel ) << " * " << MaterialUB(UniformBlock::Component::Roughness) << ";";

			return true;
		}, fragmentShader, materialSet) )
		{
			TraceError{ClassId} << "Unable to generate fragment code for the roughness component of PBR material '" << this->name() << "' !";

			return false;
		}

		/* Metalness component.
		 * Same contracts as roughness: source channel from the component (glTF packed
		 * metallic-roughness: Blue), material scalar as multiplying factor ('metallicFactor * texel'). */
		if ( !this->generateTextureComponentFragmentShader(ComponentType::Metalness, [this] (FragmentShader & shader, const Texture * component) {
			Code{shader, Location::Top} << "const float " << component->variableName() << " = texture(" << component->samplerName() << ", " << transformedTexCoords(ComponentType::Metalness, component) << ")." << component->sourceChannelSwizzle() << " * " << MaterialUB(UniformBlock::Component::Metalness) << ";";

			return true;
		}, fragmentShader, materialSet) )
		{
			TraceError{ClassId} << "Unable to generate fragment code for the metalness component of PBR material '" << this->name() << "' !";

			return false;
		}

		/* Reflection/IBL component.
		 * NOTE: When automatic reflection is enabled AND bindless textures are supported,
		 * use the bindless texture array instead of per-material sampler. */
		if ( m_isUsingEnvironmentCubemap && generator.bindlessTexturesEnabled() )
		{
			if ( !this->generateBindlessReflectionFragmentShader(generator, fragmentShader) )
			{
				TraceError{ClassId} << "Unable to generate bindless fragment code for the reflection component of PBR material '" << this->name() << "' !";

				return false;
			}
		}
		else if ( !this->generateTextureComponentFragmentShader(ComponentType::Reflection, [&] (FragmentShader & shader, const Texture * component) {
			if ( generator.highQualityEnabled() )
			{
				if ( this->isComponentPresent(ComponentType::Normal) )
				{
					Code(shader, Location::Top) << "const vec3 reflectionNormal = normalize(" << ShaderVariable::TangentToWorldMatrix << "[0] * " << SurfaceNormalVector << ".x + " << ShaderVariable::TangentToWorldMatrix << "[1] * " << SurfaceNormalVector << ".y + " << ShaderVariable::NormalWorldSpace << " * " << SurfaceNormalVector << ".z);";
				}
				else
				{
					Code(shader, Location::Top) << "const vec3 reflectionNormal = normalize(" << ShaderVariable::NormalWorldSpace << ");";
				}

				/* NOTE: Negate Y to convert from engine Y-DOWN to cubemap Y-UP convention (same as skybox). */
				Code(shader, Location::Top) <<
					"const vec3 reflectionI = normalize(" << ShaderVariable::PositionWorldSpace << ".xyz - CameraWorldPosition);" << Line::End <<
					"const vec3 reflectDir = reflect(reflectionI, reflectionNormal);" << Line::End <<
					"const vec3 " << ShaderVariable::ReflectionTextureCoordinates << " = vec3(reflectDir.x, -reflectDir.y, reflectDir.z);";

				/* A render-target cubemap (dynamic probe) carries a GGX-prefiltered mip
				 * chain (mip 0 = mirror, upper mips convolved per render — see
				 * Compute::ProbeConvolver): drive the LOD by the material roughness, the
				 * exact sampling contract of the bindless environment path above. The LOD
				 * clamps to the image's actual chain. Static texture cubemaps keep the
				 * plain mirror fetch: the explicit texture mode is ARTISTIC by contract. */
				if ( component->textureResource() == nullptr && component->texture() != nullptr && component->texture()->isCubemapTexture() )
				{
					/* NOTE: The component variable already carries the final roughness
					 * (channel + inversion + factor folded in by the component codegen). */
					const auto componentIt = m_components.find(ComponentType::Roughness);
					const auto roughnessExpression = componentIt != m_components.cend()
						? componentIt->second->variableName()
						: std::string{MaterialUB(UniformBlock::Component::Roughness)};

					Code(shader, Location::Top) <<
						"const vec4 " << component->variableName() << " = textureLod(" << component->samplerName() << ", " << ShaderVariable::ReflectionTextureCoordinates << ", clamp(" << roughnessExpression << ", 0.0, 1.0) * " << (IBLTexture::PrefilteredMipLevels - 1) << ".0);";
				}
				else
				{
					Code(shader, Location::Top) << "const vec4 " << component->variableName() << " = texture(" << component->samplerName() << ", " << ShaderVariable::ReflectionTextureCoordinates << ");";
				}
			}
			else
			{
				Code(shader, Location::Top) << "const vec4 " << component->variableName() << " = texture(" << component->samplerName() << ", " << ShaderVariable::ReflectionTextureCoordinates << ");";
			}

			return true;
		}, fragmentShader, materialSet) )
		{
			TraceError{ClassId} << "Unable to generate fragment code for the reflection component of PBR material '" << this->name() << "' !";

			return false;
		}

		/* Refraction component (for glass-like materials).
		 * NOTE: When automatic refraction is enabled AND bindless textures are supported,
		 * use the bindless texture array instead of per-material sampler. */
		if ( m_isUsingEnvironmentCubemapForRefraction && generator.bindlessTexturesEnabled() && !this->isComponentPresent(ComponentType::Refraction) )
		{
			if ( !this->generateBindlessRefractionFragmentShader(generator, fragmentShader) )
			{
				TraceError{ClassId} << "Unable to generate bindless fragment code for the refraction component of PBR material '" << this->name() << "' !";

				return false;
			}
		}
		else
		{
			const bool reflectionVariablesDeclared = this->isComponentPresent(ComponentType::Reflection) || (m_isUsingEnvironmentCubemap && generator.bindlessTexturesEnabled());

			if ( !this->generateTextureComponentFragmentShader(ComponentType::Refraction, [&] (FragmentShader & shader, const Texture * component) {
				if ( generator.highQualityEnabled() )
				{
					if ( this->isComponentPresent(ComponentType::Normal) )
					{
						/* Reuse reflectionNormal if already declared by reflection (explicit or bindless), otherwise declare it. */
						if ( !reflectionVariablesDeclared )
						{
							Code(shader, Location::Top) <<
								"const vec3 reflGeomN = normalize(" << ShaderVariable::NormalWorldSpace << ");" << Line::End <<
								"const vec3 reflRawT = " << ShaderVariable::TangentToWorldMatrix << "[0];" << Line::End <<
								"const vec3 reflGeomT = normalize(reflRawT - reflGeomN * dot(reflGeomN, reflRawT));" << Line::End <<
								"const vec3 reflGeomB = cross(reflGeomN, reflGeomT) * sign(dot(cross(reflGeomN, reflGeomT), " << ShaderVariable::TangentToWorldMatrix << "[1]));" << Line::End <<
								"const vec3 reflectionNormal = normalize(reflGeomT * " << SurfaceNormalVector << ".x + reflGeomB * " << SurfaceNormalVector << ".y + reflGeomN * " << SurfaceNormalVector << ".z);";
						}
					}
					else if ( !reflectionVariablesDeclared )
					{
						Code(shader, Location::Top) << "const vec3 reflectionNormal = normalize(" << ShaderVariable::NormalWorldSpace << ");";
					}

					/* NOTE: Negate Y to convert from engine Y-DOWN to cubemap Y-UP convention (same as skybox). */
					Code(shader, Location::Top) <<
						"const float eta = 1.0 / " << MaterialUB(UniformBlock::Component::RefractionIOR) << ";" << Line::End;

					/* Reuse reflectionI if already declared by reflection (explicit or bindless), otherwise declare it. */
					if ( !reflectionVariablesDeclared )
					{
						Code(shader, Location::Top) << "const vec3 reflectionI = normalize(" << ShaderVariable::PositionWorldSpace << ".xyz - CameraWorldPosition);" << Line::End;
					}

					Code(shader, Location::Top) <<
						"const vec3 refractDir = refract(reflectionI, reflectionNormal, eta);" << Line::End <<
						"const vec3 " << ShaderVariable::RefractionTextureCoordinates << " = vec3(refractDir.x, -refractDir.y, refractDir.z);" << Line::End <<
						"const vec4 " << component->variableName() << " = texture(" << component->samplerName() << ", " << ShaderVariable::RefractionTextureCoordinates << ");";
				}
				else
				{
					Code(shader, Location::Top) << "const vec4 " << component->variableName() << " = texture(" << component->samplerName() << ", " << ShaderVariable::RefractionTextureCoordinates << ");";
				}

				return true;
			}, fragmentShader, materialSet) )
			{
				TraceError{ClassId} << "Unable to generate fragment code for the refraction component of PBR material '" << this->name() << "' !";

				return false;
			}
		}

		/* Auto-Illumination (emissive) component. */
		if ( !this->generateTextureComponentFragmentShader(ComponentType::AutoIllumination, [this] (FragmentShader & shader, const Texture * component) {
			Code{shader, Location::Top} << "const vec4 " << component->variableName() << " = texture(" << component->samplerName() << ", " << transformedTexCoords(ComponentType::AutoIllumination, component) << ");";

			return true;
		}, fragmentShader, materialSet) )
		{
			TraceError{ClassId} << "Unable to generate fragment code for the auto-illumination component of PBR material '" << this->name() << "' !";

			return false;
		}

		/* Opacity component (owner's 3-rule contract). The variable is the amount-scaled texel;
		 * rule 2 (cutout) adds a discard against the UBO threshold — a VALUE, never a shader
		 * literal (program-cache contract); rule 3 (grayscale) emits no test and the alpha
		 * lands in fragmentColor() for blending. */
		if ( !this->generateTextureComponentFragmentShader(ComponentType::Opacity, [this] (FragmentShader & shader, const Texture * component) {
			Code{shader, Location::Top} << "const float " << component->variableName() << " = texture(" << component->samplerName() << ", " << textCoords(component) << ").r * " << MaterialUB(UniformBlock::Component::Opacity) << ";";

			if ( this->isFlagEnabled(AlphaTestEnabled) )
			{
				Code{shader, Location::Top} << "if ( " << component->variableName() << " < " << MaterialUB(UniformBlock::Component::AlphaThreshold) << " ) { discard; }";
			}

			return true;
		}, fragmentShader, materialSet) )
		{
			TraceError{ClassId} << "Unable to generate fragment code for the opacity component of PBR material '" << this->name() << "' !";

			return false;
		}

		/* Ambient Occlusion component (baked texture). */
		if ( !this->generateTextureComponentFragmentShader(ComponentType::AmbientOcclusion, [this] (FragmentShader & shader, const Texture * component) {
			/* NOTE: AO is typically stored in a grayscale texture (red channel). */
			Code{shader, Location::Top} << "const float " << component->variableName() << " = texture(" << component->samplerName() << ", " << transformedTexCoords(ComponentType::AmbientOcclusion, component) << ").r;";

			return true;
		}, fragmentShader, materialSet) )
		{
			TraceError{ClassId} << "Unable to generate fragment code for the ambient occlusion component of PBR material '" << this->name() << "' !";

			return false;
		}

		/* Reflectivity Map component (texture-based). */
		if ( !this->generateTextureComponentFragmentShader(ComponentType::ReflectivityMap, [this] (FragmentShader & shader, const Texture * component) {
			/* NOTE: Reflectivity is typically stored in a grayscale texture (red channel). */
			Code{shader, Location::Top} << "const float " << component->variableName() << " = texture(" << component->samplerName() << ", " << textCoords(component) << ").r;";

			return true;
		}, fragmentShader, materialSet) )
		{
			TraceError{ClassId} << "Unable to generate fragment code for the reflectivity map component of PBR material '" << this->name() << "' !";

			return false;
		}

		/* Clear Coat factor component (texture-based). */
		if ( !this->generateTextureComponentFragmentShader(ComponentType::ClearCoat, [this] (FragmentShader & shader, const Texture * component) {
			Code{shader, Location::Top} << "const float " << component->variableName() << " = texture(" << component->samplerName() << ", " << textCoords(component) << ").r;";

			return true;
		}, fragmentShader, materialSet) )
		{
			TraceError{ClassId} << "Unable to generate fragment code for the clear coat component of PBR material '" << this->name() << "' !";

			return false;
		}

		/* Clear Coat roughness component (texture-based). */
		if ( !this->generateTextureComponentFragmentShader(ComponentType::ClearCoatRoughness, [this] (FragmentShader & shader, const Texture * component) {
			Code{shader, Location::Top} << "const float " << component->variableName() << " = texture(" << component->samplerName() << ", " << textCoords(component) << ").r;";

			return true;
		}, fragmentShader, materialSet) )
		{
			TraceError{ClassId} << "Unable to generate fragment code for the clear coat roughness component of PBR material '" << this->name() << "' !";

			return false;
		}

		/* Clear Coat normal component (texture-based, KHR_materials_clearcoat). */
		if ( !this->generateTextureComponentFragmentShader(ComponentType::ClearCoatNormal, [this] (FragmentShader & shader, const Texture * component) {
			Code{shader, Location::Top} <<
				"const vec3 " << component->variableName() << "_raw = texture(" << component->samplerName() << ", " << textCoords(component) << ").rgb * 2.0 - 1.0;" << Line::End <<
				"const vec3 " << component->variableName() << " = normalize(vec3(" << component->variableName() << "_raw.xy * " << MaterialUB(UniformBlock::Component::ClearCoatNormalScale) << ", " << component->variableName() << "_raw.z));";

			return true;
		}, fragmentShader, materialSet) )
		{
			TraceError{ClassId} << "Unable to generate fragment code for the clear coat normal component of PBR material '" << this->name() << "' !";

			return false;
		}

		/* Subsurface intensity component (texture-based). */
		if ( !this->generateTextureComponentFragmentShader(ComponentType::Subsurface, [this] (FragmentShader & shader, const Texture * component) {
			Code{shader, Location::Top} << "const float " << component->variableName() << " = texture(" << component->samplerName() << ", " << textCoords(component) << ").r;";

			return true;
		}, fragmentShader, materialSet) )
		{
			TraceError{ClassId} << "Unable to generate fragment code for the subsurface component of PBR material '" << this->name() << "' !";

			return false;
		}

		/* Subsurface thickness component (texture-based). */
		if ( !this->generateTextureComponentFragmentShader(ComponentType::SubsurfaceThickness, [this] (FragmentShader & shader, const Texture * component) {
			Code{shader, Location::Top} << "const float " << component->variableName() << " = texture(" << component->samplerName() << ", " << textCoords(component) << ").r;";

			return true;
		}, fragmentShader, materialSet) )
		{
			TraceError{ClassId} << "Unable to generate fragment code for the subsurface thickness component of PBR material '" << this->name() << "' !";

			return false;
		}

		/* Sheen color component (texture-based). */
		if ( !this->generateTextureComponentFragmentShader(ComponentType::Sheen, [this] (FragmentShader & shader, const Texture * component) {
			Code{shader, Location::Top} << "const vec4 " << component->variableName() << " = texture(" << component->samplerName() << ", " << textCoords(component) << ");";

			return true;
		}, fragmentShader, materialSet) )
		{
			TraceError{ClassId} << "Unable to generate fragment code for the sheen component of PBR material '" << this->name() << "' !";

			return false;
		}

		/* Anisotropy component (texture-based, KHR_materials_anisotropy format: RG = direction, B = strength). */
		if ( !this->generateTextureComponentFragmentShader(ComponentType::Anisotropy, [this] (FragmentShader & shader, const Texture * component) {
			/* NOTE: KHR_materials_anisotropy texture format:
			 * R, G = tangent-space direction vector (encoded [0,1] -> [-1,1])
			 * B = strength factor [0,1], multiplied by UBO anisotropy value. */
			Code{shader, Location::Top} <<
				"const vec3 " << component->variableName() << "_raw = texture(" << component->samplerName() << ", " << textCoords(component) << ").rgb;" << Line::End <<
				"const vec2 " << component->variableName() << "_dir = " << component->variableName() << "_raw.rg * 2.0 - 1.0;" << Line::End <<
				"const float " << component->variableName() << " = " << MaterialUB(UniformBlock::Component::Anisotropy) << " * " << component->variableName() << "_raw.b;";

			return true;
		}, fragmentShader, materialSet) )
		{
			TraceError{ClassId} << "Unable to generate fragment code for the anisotropy component of PBR material '" << this->name() << "' !";

			return false;
		}

		/* Transmission component.
		 * GrabPass screen-space transmission takes priority over cubemap-based transmission.
		 * Both require bindless textures and high quality to be enabled (the low-quality
		 * path lacks the required fragment varyings for screen-space refraction). */
		if ( m_isUsingGrabPassForTransmission && generator.bindlessTexturesEnabled() && generator.highQualityEnabled() )
		{
			if ( !this->generateGrabPassTransmissionFragmentShader(generator, fragmentShader) )
			{
				TraceError{ClassId} << "Unable to generate GrabPass fragment code for transmission of PBR material '" << this->name() << "' !";

				return false;
			}
		}
		/* NOTE: A grab-pass transmission material FALLS BACK to the cubemap here when the
		 * grab pass is unavailable (low quality / no bindless) — without this, such a
		 * material would render with NO transmission at all in the fallback tier. */
		else if ( ( m_isUsingEnvironmentCubemapForTransmission || m_isUsingGrabPassForTransmission ) && generator.bindlessTexturesEnabled() )
		{
			if ( !this->generateBindlessTransmissionFragmentShader(generator, fragmentShader) )
			{
				TraceError{ClassId} << "Unable to generate bindless fragment code for the transmission component of PBR material '" << this->name() << "' !";

				return false;
			}
		}

		/* Transmission factor component (texture-based). */
		if ( !this->generateTextureComponentFragmentShader(ComponentType::Transmission, [this] (FragmentShader & shader, const Texture * component) {
			Code{shader, Location::Top} << "const float " << component->variableName() << " = texture(" << component->samplerName() << ", " << textCoords(component) << ").r;";

			return true;
		}, fragmentShader, materialSet) )
		{
			TraceError{ClassId} << "Unable to generate fragment code for the transmission component of PBR material '" << this->name() << "' !";

			return false;
		}

		/* Iridescence factor component (texture-based). */
		if ( !this->generateTextureComponentFragmentShader(ComponentType::Iridescence, [this] (FragmentShader & shader, const Texture * component) {
			Code{shader, Location::Top} << "const float " << component->variableName() << " = texture(" << component->samplerName() << ", " << textCoords(component) << ").r;";

			return true;
		}, fragmentShader, materialSet) )
		{
			TraceError{ClassId} << "Unable to generate fragment code for the iridescence component of PBR material '" << this->name() << "' !";

			return false;
		}

		return true;
	}

	/* ==================== Component Setters ==================== */

	bool
	StandardResource::setAlbedoComponent (const PixelFactory::Color< float > & color) noexcept
	{
		if ( this->isCreated() )
		{
			TraceWarning{ClassId} <<
				"The resource '" << this->name() << "' is created ! "
				"Unable to create or change the albedo component.";

			return false;
		}

		const auto uniform = MaterialUB(UniformBlock::Component::AlbedoColor);

		const auto result = m_components.emplace(ComponentType::Albedo, std::make_unique< Color >(uniform, color));

		if ( !result.second || result.first->second == nullptr )
		{
			return false;
		}

		this->setAlbedoColor(color);

		return true;
	}

	bool
	StandardResource::setAlbedoComponent (const std::shared_ptr< TextureResource::Abstract > & texture) noexcept
	{
		if ( this->isCreated() )
		{
			TraceWarning{ClassId} <<
				"The resource '" << this->name() << "' is created ! "
				"Unable to create or change the albedo component.";

			return false;
		}

		const auto result = m_components.emplace(ComponentType::Albedo, std::make_unique< Texture >(Uniform::AlbedoSampler, SurfaceAlbedoColor, texture));

		if ( !result.second || result.first->second == nullptr )
		{
			return false;
		}

		if ( !this->addDependency(texture) )
		{
			TraceError{ClassId} << "Unable to link the texture '" << texture->name() << "' dependency to PBR material '" << this->name() << "' for albedo component !";

			return false;
		}

		this->enableFlag(TextureEnabled);
		this->enableFlag(UsePrimaryTextureCoordinates);

		return true;
	}

	bool
	StandardResource::setRoughnessComponent (float value) noexcept
	{
		if ( this->isCreated() )
		{
			TraceWarning{ClassId} <<
				"The resource '" << this->name() << "' is created ! "
				"Unable to create or change the roughness component.";

			return false;
		}

		const auto uniform = MaterialUB(UniformBlock::Component::Roughness);

		const auto result = m_components.emplace(ComponentType::Roughness, std::make_unique< Value >(uniform));

		if ( !result.second || result.first->second == nullptr )
		{
			return false;
		}

		this->setRoughness(value);

		return true;
	}

	bool
	StandardResource::setRoughnessComponent (const std::shared_ptr< TextureResource::Abstract > & texture, float value, bool invert, Base::PixelFactory::Channel sourceChannel) noexcept
	{
		if ( this->isCreated() )
		{
			TraceWarning{ClassId} <<
				"The resource '" << this->name() << "' is created ! "
				"Unable to create or change the roughness component.";

			return false;
		}

		auto component = std::make_unique< Texture >(Uniform::RoughnessSampler, SurfaceRoughness, texture);
		component->setSourceChannel(sourceChannel);

		const auto result = m_components.emplace(ComponentType::Roughness, std::move(component));

		if ( !result.second || result.first->second == nullptr )
		{
			return false;
		}

		if ( !this->addDependency(texture) )
		{
			TraceError{ClassId} << "Unable to link the texture '" << texture->name() << "' dependency to PBR material '" << this->name() << "' for roughness component !";

			return false;
		}

		this->enableFlag(TextureEnabled);
		this->enableFlag(UsePrimaryTextureCoordinates);

		this->setRoughness(value);

		m_invertRoughness = invert;

		return true;
	}

	bool
	StandardResource::setMetalnessComponent (float value) noexcept
	{
		if ( this->isCreated() )
		{
			TraceWarning{ClassId} <<
				"The resource '" << this->name() << "' is created ! "
				"Unable to create or change the metalness component.";

			return false;
		}

		const auto uniform = MaterialUB(UniformBlock::Component::Metalness);

		const auto result = m_components.emplace(ComponentType::Metalness, std::make_unique< Value >(uniform));

		if ( !result.second || result.first->second == nullptr )
		{
			return false;
		}

		this->setMetalness(value);

		return true;
	}

	bool
	StandardResource::setMetalnessComponent (const std::shared_ptr< TextureResource::Abstract > & texture, float value, Base::PixelFactory::Channel sourceChannel) noexcept
	{
		if ( this->isCreated() )
		{
			TraceWarning{ClassId} <<
				"The resource '" << this->name() << "' is created ! "
				"Unable to create or change the metalness component.";

			return false;
		}

		auto component = std::make_unique< Texture >(Uniform::MetalnessSampler, SurfaceMetalness, texture);
		component->setSourceChannel(sourceChannel);

		const auto result = m_components.emplace(ComponentType::Metalness, std::move(component));

		if ( !result.second || result.first->second == nullptr )
		{
			return false;
		}

		if ( !this->addDependency(texture) )
		{
			TraceError{ClassId} << "Unable to link the texture '" << texture->name() << "' dependency to PBR material '" << this->name() << "' for metalness component !";

			return false;
		}

		this->enableFlag(TextureEnabled);
		this->enableFlag(UsePrimaryTextureCoordinates);

		this->setMetalness(value);

		return true;
	}

	bool
	StandardResource::setNormalComponent (const std::shared_ptr< TextureResource::Abstract > & texture, float scale) noexcept
	{
		if ( this->isCreated() )
		{
			TraceWarning{ClassId} <<
				"The resource '" << this->name() << "' is created ! "
				"Unable to create or change the normal component.";

			return false;
		}

		const auto result = m_components.emplace(ComponentType::Normal, std::make_unique< Texture >(Uniform::NormalSampler, SurfaceNormalVector, texture));

		if ( !result.second || result.first->second == nullptr )
		{
			return false;
		}

		if ( !this->addDependency(texture) )
		{
			TraceError{ClassId} << "Unable to link the texture '" << texture->name() << "' dependency to PBR material '" << this->name() << "' for normal component !";

			return false;
		}

		this->enableFlag(TextureEnabled);
		this->enableFlag(UsePrimaryTextureCoordinates);

		this->setNormalScale(scale);

		return true;
	}

	bool
	StandardResource::setHeightComponent (const std::shared_ptr< TextureResource::Abstract > & texture, float scale) noexcept
	{
		if ( this->isCreated() )
		{
			TraceWarning{ClassId} <<
				"The resource '" << this->name() << "' is created ! "
				"Unable to create or change the height component.";

			return false;
		}

		const auto result = m_components.emplace(ComponentType::Displacement, std::make_unique< Texture >(Uniform::HeightSampler, SurfaceHeightValue, texture));

		if ( !result.second || result.first->second == nullptr )
		{
			return false;
		}

		if ( !this->addDependency(texture) )
		{
			TraceError{ClassId} << "Unable to link the texture '" << texture->name() << "' dependency to PBR material '" << this->name() << "' for height component !";

			return false;
		}

		this->enableFlag(TextureEnabled);
		this->enableFlag(UsePrimaryTextureCoordinates);

		m_useParallaxOcclusionMapping = true;

		this->setHeightScale(scale);

		return true;
	}

	void
	StandardResource::setHeightScale (float value) noexcept
	{
		m_materialProperties[HeightScaleOffset] = value;

		m_videoMemoryUpdated = true;
	}

	bool
	StandardResource::setReflectionComponent (const std::shared_ptr< TextureResource::Abstract > & texture) noexcept
	{
		if ( this->isCreated() )
		{
			TraceWarning{ClassId} <<
				"The resource '" << this->name() << "' is created ! "
				"Unable to create or change the reflection component.";

			return false;
		}

		const auto result = m_components.emplace(ComponentType::Reflection, std::make_unique< Texture >(Uniform::ReflectionSampler, SurfaceReflectionColor, texture));

		if ( !result.second || result.first->second == nullptr )
		{
			return false;
		}

		if ( !this->addDependency(texture) )
		{
			TraceError{ClassId} << "Unable to link the texture '" << texture->name() << "' dependency to PBR material '" << this->name() << "' for reflection component !";

			return false;
		}

		/* Explicitly authored cubemap: artistic intent, never replaced by SSR/RTR. The
		 * render-target variant below stays overridable — a probe is scene-coherent. */
		m_reflectionIsArtistic = true;

		this->enableFlag(TextureEnabled);
		this->enableFlag(UsePrimaryTextureCoordinates);

		return true;
	}

	bool
	StandardResource::setReflectionComponentFromRenderTarget (const std::shared_ptr< Vulkan::TextureInterface > & renderTarget) noexcept
	{
		if ( this->isCreated() )
		{
			TraceWarning{ClassId} <<
				"The resource '" << this->name() << "' is created ! "
				"Unable to create or change the reflection component.";

			return false;
		}

		/* NOTE: Using TextureInterface constructor - no resource dependency tracking. */
		const auto result = m_components.emplace(ComponentType::Reflection, std::make_unique< Texture >(Uniform::ReflectionSampler, SurfaceReflectionColor, renderTarget));

		if ( !result.second || result.first->second == nullptr )
		{
			return false;
		}

		/* NOTE: No addDependency() for TextureInterface - it's not a loadable resource. */

		/* A render target is the RENDERED SCENE: an absolute luminance — the shader must not
		 * re-apply the environment luminance to it (see LightGenerator::reflectionIntensity()). */
		m_reflectionSourceIsAbsolute = true;

		this->enableFlag(TextureEnabled);
		this->enableFlag(UsePrimaryTextureCoordinates);

		return true;
	}

	bool
	StandardResource::samplesTexture (const Vulkan::TextureInterface * texture) const noexcept
	{
		if ( texture == nullptr )
		{
			return false;
		}

		for ( const auto & [componentType, component] : m_components )
		{
			if ( component != nullptr && component->texture().get() == texture )
			{
				return true;
			}
		}

		return false;
	}

	bool
	StandardResource::setReflectionComponentFromEnvironmentCubemap (float IBLIntensity) noexcept
	{
		if ( this->isCreated() )
		{
			TraceWarning{ClassId} <<
				"The resource '" << this->name() << "' is created ! "
				"Unable to create or change the reflection component.";

			return false;
		}

		m_isUsingEnvironmentCubemap = true;

		m_materialProperties[IBLIntensityOffset] = std::clamp(IBLIntensity, 0.0F, 1.0F);

		return true;
	}

	bool
	StandardResource::setPostProcessReflectivity (float amount) noexcept
	{
		if ( this->isCreated() )
		{
			TraceWarning{ClassId} <<
				"The resource '" << this->name() << "' is created ! "
				"Unable to create or change the reflection component.";

			return false;
		}

		/* NOTE: No cubemap, no sampler, no Reflection component: the value only reaches
		 * LightGenerator::declareSurfaceReflectivityMap() as a GLSL literal, which puts it in the
		 * material-properties G-buffer for SSR/RTR to read. Mirror of the "Value" filling type
		 * handled in parseReflectionComponent(). */
		m_postProcessReflectivityAmount = std::clamp(amount, 0.0F, 1.0F);

		return true;
	}

	bool
	StandardResource::setRefractionComponentFromEnvironmentCubemap (float ior) noexcept
	{
		if ( this->isCreated() )
		{
			TraceWarning{ClassId} <<
				"The resource '" << this->name() << "' is created ! "
				"Unable to create or change the refraction component.";

			return false;
		}

		m_isUsingEnvironmentCubemapForRefraction = true;

		this->setIOR(ior);

		return true;
	}

	bool
	StandardResource::setRefractionComponent (const std::shared_ptr< TextureResource::Abstract > & texture, float ior) noexcept
	{
		if ( this->isCreated() )
		{
			TraceWarning{ClassId} <<
				"The resource '" << this->name() << "' is created ! "
				"Unable to create or change the refraction component.";

			return false;
		}

		const auto result = m_components.emplace(ComponentType::Refraction, std::make_unique< Texture >(Uniform::RefractionSampler, SurfaceRefractionColor, texture));

		if ( !result.second || result.first->second == nullptr )
		{
			return false;
		}

		if ( !this->addDependency(texture) )
		{
			TraceError{ClassId} << "Unable to link the texture '" << texture->name() << "' dependency to PBR material '" << this->name() << "' for refraction component !";

			return false;
		}

		this->enableFlag(TextureEnabled);
		this->enableFlag(UsePrimaryTextureCoordinates);

		this->setIOR(ior);

		return true;
	}

	bool
	StandardResource::setRefractionComponentFromRenderTarget (const std::shared_ptr< Vulkan::TextureInterface > & renderTarget, float ior) noexcept
	{
		if ( this->isCreated() )
		{
			TraceWarning{ClassId} <<
				"The resource '" << this->name() << "' is created ! "
				"Unable to create or change the refraction component.";

			return false;
		}

		/* NOTE: Using TextureInterface constructor - no resource dependency tracking. */
		const auto result = m_components.emplace(ComponentType::Refraction, std::make_unique< Texture >(Uniform::RefractionSampler, SurfaceRefractionColor, renderTarget));

		if ( !result.second || result.first->second == nullptr )
		{
			return false;
		}

		/* NOTE: No addDependency() for TextureInterface - it's not a loadable resource. */


		/* Absolute luminance source — see the reflection variant above. */
		m_refractionSourceIsAbsolute = true;
		this->enableFlag(TextureEnabled);
		this->enableFlag(UsePrimaryTextureCoordinates);

		this->setIOR(ior);

		return true;
	}

	bool
	StandardResource::setAutoIlluminationComponent (float amount) noexcept
	{
		if ( this->isCreated() )
		{
			TraceWarning{ClassId} <<
				"The resource '" << this->name() << "' is created ! "
				"Unable to create or change the auto-illumination component.";

			return false;
		}

		const auto uniform = MaterialUB(UniformBlock::Component::AutoIlluminationColor);

		m_components[ComponentType::AutoIllumination] = std::make_unique< Value >(uniform);

		/* Value-only semantic: a WHITE emissive driven by the amount alone (PBR's default
		 * emissive color is black — behavioural parity with StandardResource, default white). */
		this->setAutoIlluminationColor(PixelFactory::White);
		this->setAutoIlluminationAmount(amount);

		return true;
	}

	bool
	StandardResource::setAutoIlluminationComponent (const PixelFactory::Color< float > & color, float amount) noexcept
	{
		if ( this->isCreated() )
		{
			TraceWarning{ClassId} <<
				"The resource '" << this->name() << "' is created ! "
				"Unable to create or change the auto-illumination component.";

			return false;
		}

		const auto uniform = MaterialUB(UniformBlock::Component::AutoIlluminationColor);

		const auto result = m_components.emplace(ComponentType::AutoIllumination, std::make_unique< Color >(uniform, color));

		if ( !result.second || result.first->second == nullptr )
		{
			return false;
		}

		this->setAutoIlluminationColor(color);
		this->setAutoIlluminationAmount(amount);

		return true;
	}

	bool
	StandardResource::setAutoIlluminationComponent (const std::shared_ptr< TextureResource::Abstract > & texture, float amount) noexcept
	{
		if ( this->isCreated() )
		{
			TraceWarning{ClassId} <<
				"The resource '" << this->name() << "' is created ! "
				"Unable to create or change the auto-illumination component.";

			return false;
		}

		const auto result = m_components.emplace(ComponentType::AutoIllumination, std::make_unique< Texture >(Uniform::AutoIlluminationSampler, SurfaceAutoIlluminationColor, texture));

		if ( !result.second || result.first->second == nullptr )
		{
			return false;
		}

		if ( !this->addDependency(texture) )
		{
			TraceError{ClassId} << "Unable to link the texture '" << texture->name() << "' dependency to PBR material '" << this->name() << "' for auto-illumination component !";

			return false;
		}

		this->enableFlag(TextureEnabled);
		this->enableFlag(UsePrimaryTextureCoordinates);

		this->setAutoIlluminationAmount(amount);

		return true;
	}

	bool
	StandardResource::setOpacityComponent (float amount) noexcept
	{
		if ( this->isCreated() )
		{
			TraceWarning{ClassId} <<
				"The resource '" << this->name() << "' is created ! "
				"Unable to create or change the opacity component.";

			return false;
		}

		/* Rule 1: a global opacity value makes the whole surface uniformly translucent. */
		this->enableFlag(OpacityEnabled);
		this->enableBlending(BlendingMode::Normal);

		this->setOpacity(amount);

		return true;
	}

	bool
	StandardResource::setOpacityComponent (const std::shared_ptr< TextureResource::Abstract > & texture, float amount) noexcept
	{
		if ( this->isCreated() )
		{
			TraceWarning{ClassId} <<
				"The resource '" << this->name() << "' is created ! "
				"Unable to create or change the opacity component.";

			return false;
		}

		const auto result = m_components.emplace(ComponentType::Opacity, std::make_unique< Texture >(Uniform::OpacitySampler, SurfaceOpacityAmount, texture));

		if ( !result.second || result.first->second == nullptr )
		{
			return false;
		}

		if ( !this->addDependency(texture) )
		{
			TraceError{ClassId} << "Unable to link the texture '" << texture->name() << "' dependency to PBR material '" << this->name() << "' for opacity component !";

			return false;
		}

		this->enableFlag(TextureEnabled);
		this->enableFlag(UsePrimaryTextureCoordinates);
		this->enableFlag(OpacityEnabled);

		/* Rule 3 (grayscale per-pixel alpha scale) by default: the material blends. Calling
		 * enableAlphaTest() afterwards switches to rule 2 (binary cutout, stays opaque). */
		this->enableBlending(BlendingMode::Normal);

		this->setOpacity(amount);

		return true;
	}

	void
	StandardResource::enableAlphaTest (float threshold) noexcept
	{
		/* Rule 2: binary CUTOUT — the fragment discards below the threshold and the material
		 * STAYS OPAQUE (opaque render list, depth write kept, no back-to-front sorting). */
		this->enableFlag(AlphaTestEnabled);
		this->disableFlag(BlendingEnabled);

		this->setAlphaThresholdToDiscard(threshold);
	}

	bool
	StandardResource::setAmbientOcclusionComponent (const std::shared_ptr< TextureResource::Abstract > & texture, float intensity) noexcept
	{
		if ( this->isCreated() )
		{
			TraceWarning{ClassId} <<
				"The resource '" << this->name() << "' is created ! "
				"Unable to create or change the ambient occlusion component.";

			return false;
		}

		const auto result = m_components.emplace(ComponentType::AmbientOcclusion, std::make_unique< Texture >(Uniform::AmbientOcclusionSampler, SurfaceAmbientOcclusion, texture));

		if ( !result.second || result.first->second == nullptr )
		{
			return false;
		}

		if ( !this->addDependency(texture) )
		{
			TraceError{ClassId} << "Unable to link the texture '" << texture->name() << "' dependency to PBR material '" << this->name() << "' for ambient occlusion component !";

			return false;
		}

		this->enableFlag(TextureEnabled);
		this->enableFlag(UsePrimaryTextureCoordinates);

		this->setAOIntensity(intensity);

		return true;
	}

	bool
	StandardResource::setReflectivityMapComponent (const std::shared_ptr< TextureResource::Abstract > & texture) noexcept
	{
		if ( this->isCreated() )
		{
			TraceWarning{ClassId} <<
				"The resource '" << this->name() << "' is created ! "
				"Unable to create or change the reflectivity map component.";

			return false;
		}

		const auto result = m_components.emplace(ComponentType::ReflectivityMap, std::make_unique< Texture >(Uniform::ReflectivityMapSampler, SurfaceReflectivityMap, texture));

		if ( !result.second || result.first->second == nullptr )
		{
			return false;
		}

		if ( !this->addDependency(texture) )
		{
			TraceError{ClassId} << "Unable to link the texture '" << texture->name() << "' dependency to PBR material '" << this->name() << "' for reflectivity map component !";

			return false;
		}

		this->enableFlag(TextureEnabled);
		this->enableFlag(UsePrimaryTextureCoordinates);

		return true;
	}

	bool
	StandardResource::setClearCoatComponent (float factor, float roughness) noexcept
	{
		if ( this->isCreated() )
		{
			TraceWarning{ClassId} <<
				"The resource '" << this->name() << "' is created ! "
				"Unable to create or change the clear coat component.";

			return false;
		}

		const auto uniform = MaterialUB(UniformBlock::Component::ClearCoatFactor);

		const auto result = m_components.emplace(ComponentType::ClearCoat, std::make_unique< Value >(uniform));

		if ( !result.second || result.first->second == nullptr )
		{
			return false;
		}

		this->setClearCoatFactor(factor);
		this->setClearCoatRoughness(roughness);

		return true;
	}

	bool
	StandardResource::setClearCoatComponent (const std::shared_ptr< TextureResource::Abstract > & texture, float roughness) noexcept
	{
		if ( this->isCreated() )
		{
			TraceWarning{ClassId} <<
				"The resource '" << this->name() << "' is created ! "
				"Unable to create or change the clear coat component.";

			return false;
		}

		const auto result = m_components.emplace(ComponentType::ClearCoat, std::make_unique< Texture >(Uniform::ClearCoatSampler, SurfaceClearCoatFactor, texture));

		if ( !result.second || result.first->second == nullptr )
		{
			return false;
		}

		if ( !this->addDependency(texture) )
		{
			TraceError{ClassId} << "Unable to link the texture '" << texture->name() << "' dependency to PBR material '" << this->name() << "' for clear coat component !";

			return false;
		}

		this->enableFlag(TextureEnabled);
		this->enableFlag(UsePrimaryTextureCoordinates);

		this->setClearCoatFactor(1.0F);
		this->setClearCoatRoughness(roughness);

		return true;
	}

	bool
	StandardResource::setClearCoatRoughnessComponent (const std::shared_ptr< TextureResource::Abstract > & texture, float factor) noexcept
	{
		if ( this->isCreated() )
		{
			TraceWarning{ClassId} <<
				"The resource '" << this->name() << "' is created ! "
				"Unable to create or change the clear coat roughness component.";

			return false;
		}

		const auto result = m_components.emplace(ComponentType::ClearCoatRoughness, std::make_unique< Texture >(Uniform::ClearCoatRoughnessSampler, SurfaceClearCoatRoughness, texture));

		if ( !result.second || result.first->second == nullptr )
		{
			return false;
		}

		if ( !this->addDependency(texture) )
		{
			TraceError{ClassId} << "Unable to link the texture '" << texture->name() << "' dependency to PBR material '" << this->name() << "' for clear coat roughness component !";

			return false;
		}

		this->enableFlag(TextureEnabled);
		this->enableFlag(UsePrimaryTextureCoordinates);

		this->setClearCoatFactor(factor);

		return true;
	}

	bool
	StandardResource::setClearCoatNormalComponent (const std::shared_ptr< TextureResource::Abstract > & texture, float scale) noexcept
	{
		if ( this->isCreated() )
		{
			TraceWarning{ClassId} <<
				"The resource '" << this->name() << "' is created ! "
				"Unable to create or change the clear coat normal component.";

			return false;
		}

		const auto result = m_components.emplace(ComponentType::ClearCoatNormal, std::make_unique< Texture >(Uniform::ClearCoatNormalSampler, SurfaceClearCoatNormal, texture));

		if ( !result.second || result.first->second == nullptr )
		{
			return false;
		}

		if ( !this->addDependency(texture) )
		{
			TraceError{ClassId} << "Unable to link the texture '" << texture->name() << "' dependency to PBR material '" << this->name() << "' for clear coat normal component !";

			return false;
		}

		this->enableFlag(TextureEnabled);
		this->enableFlag(UsePrimaryTextureCoordinates);

		this->setClearCoatNormalScale(scale);

		return true;
	}

	void
	StandardResource::setClearCoatNormalScale (float value) noexcept
	{
		m_materialProperties[ClearCoatNormalScaleOffset] = value;

		m_videoMemoryUpdated = true;
	}

	bool
	StandardResource::setSubsurfaceComponent (float intensity, float radius, const PixelFactory::Color< float > & color) noexcept
	{
		if ( this->isCreated() )
		{
			TraceWarning{ClassId} <<
				"The resource '" << this->name() << "' is created ! "
				"Unable to create or change the subsurface component.";

			return false;
		}

		const auto uniform = MaterialUB(UniformBlock::Component::SubsurfaceIntensity);

		const auto result = m_components.emplace(ComponentType::Subsurface, std::make_unique< Value >(uniform));

		if ( !result.second || result.first->second == nullptr )
		{
			return false;
		}

		this->setSubsurfaceIntensity(intensity);
		this->setSubsurfaceRadius(radius);
		this->setSubsurfaceColor(color);

		return true;
	}

	bool
	StandardResource::setSubsurfaceThicknessComponent (const std::shared_ptr< TextureResource::Abstract > & texture) noexcept
	{
		if ( this->isCreated() )
		{
			TraceWarning{ClassId} <<
				"The resource '" << this->name() << "' is created ! "
				"Unable to create or change the subsurface thickness component.";

			return false;
		}

		const auto result = m_components.emplace(ComponentType::SubsurfaceThickness, std::make_unique< Texture >(Uniform::SubsurfaceThicknessSampler, SurfaceSubsurfaceThickness, texture));

		if ( !result.second || result.first->second == nullptr )
		{
			return false;
		}

		if ( !this->addDependency(texture) )
		{
			TraceError{ClassId} << "Unable to link the texture '" << texture->name() << "' dependency to PBR material '" << this->name() << "' for subsurface thickness component !";

			return false;
		}

		this->enableFlag(TextureEnabled);
		this->enableFlag(UsePrimaryTextureCoordinates);

		return true;
	}

	bool
	StandardResource::setSheenComponent (const PixelFactory::Color< float > & color, float roughness) noexcept
	{
		if ( this->isCreated() )
		{
			TraceWarning{ClassId} <<
				"The resource '" << this->name() << "' is created ! "
				"Unable to create or change the sheen component.";

			return false;
		}

		const auto uniform = MaterialUB(UniformBlock::Component::SheenColor);

		const auto result = m_components.emplace(ComponentType::Sheen, std::make_unique< Color >(uniform, color));

		if ( !result.second || result.first->second == nullptr )
		{
			return false;
		}

		this->setSheenColor(color);
		this->setSheenRoughness(roughness);

		return true;
	}

	bool
	StandardResource::setSheenComponent (const std::shared_ptr< TextureResource::Abstract > & texture, float roughness) noexcept
	{
		if ( this->isCreated() )
		{
			TraceWarning{ClassId} <<
				"The resource '" << this->name() << "' is created ! "
				"Unable to create or change the sheen component.";

			return false;
		}

		const auto result = m_components.emplace(ComponentType::Sheen, std::make_unique< Texture >(Uniform::SheenSampler, SurfaceSheenColor, texture));

		if ( !result.second || result.first->second == nullptr )
		{
			return false;
		}

		if ( !this->addDependency(texture) )
		{
			TraceError{ClassId} << "Unable to link the texture '" << texture->name() << "' dependency to PBR material '" << this->name() << "' for sheen component !";

			return false;
		}

		this->enableFlag(TextureEnabled);
		this->enableFlag(UsePrimaryTextureCoordinates);

		this->setSheenRoughness(roughness);

		return true;
	}

	bool
	StandardResource::setAnisotropyComponent (float anisotropy, float rotation) noexcept
	{
		if ( this->isCreated() )
		{
			TraceWarning{ClassId} <<
				"The resource '" << this->name() << "' is created ! "
				"Unable to create or change the anisotropy component.";

			return false;
		}

		const auto uniform = MaterialUB(UniformBlock::Component::Anisotropy);

		const auto result = m_components.emplace(ComponentType::Anisotropy, std::make_unique< Value >(uniform));

		if ( !result.second || result.first->second == nullptr )
		{
			return false;
		}

		this->setAnisotropy(anisotropy);
		this->setAnisotropyRotation(rotation);

		return true;
	}

	bool
	StandardResource::setAnisotropyComponent (const std::shared_ptr< TextureResource::Abstract > & texture, float anisotropy, float rotation) noexcept
	{
		if ( this->isCreated() )
		{
			TraceWarning{ClassId} <<
				"The resource '" << this->name() << "' is created ! "
				"Unable to create or change the anisotropy component.";

			return false;
		}

		const auto result = m_components.emplace(ComponentType::Anisotropy, std::make_unique< Texture >(Uniform::AnisotropySampler, SurfaceAnisotropy, texture));

		if ( !result.second || result.first->second == nullptr )
		{
			return false;
		}

		if ( !this->addDependency(texture) )
		{
			TraceError{ClassId} << "Unable to link the texture '" << texture->name() << "' dependency to PBR material '" << this->name() << "' for anisotropy component !";

			return false;
		}

		this->enableFlag(TextureEnabled);
		this->enableFlag(UsePrimaryTextureCoordinates);

		this->setAnisotropy(anisotropy);
		this->setAnisotropyRotation(rotation);

		return true;
	}

	bool
	StandardResource::isComponentPresent (ComponentType componentType) const noexcept
	{
		return m_components.contains(componentType);
	}

	/* ==================== Dynamic Property Setters ==================== */

	void
	StandardResource::setAlbedoColor (const PixelFactory::Color< float > & color) noexcept
	{
		m_materialProperties[AlbedoColorOffset] = color.red();
		m_materialProperties[AlbedoColorOffset+1] = color.green();
		m_materialProperties[AlbedoColorOffset+2] = color.blue();
		m_materialProperties[AlbedoColorOffset+3] = color.alpha();

		m_videoMemoryUpdated = true;
	}

	void
	StandardResource::setRoughness (float value) noexcept
	{
		m_materialProperties[RoughnessOffset] = clampToUnit(value);

		m_videoMemoryUpdated = true;
	}

	void
	StandardResource::setMetalness (float value) noexcept
	{
		m_materialProperties[MetalnessOffset] = clampToUnit(value);

		m_videoMemoryUpdated = true;
	}

	void
	StandardResource::setNormalScale (float value) noexcept
	{
		m_materialProperties[NormalScaleOffset] = value;

		m_videoMemoryUpdated = true;
	}

	void
	StandardResource::setIOR (float value) noexcept
	{
		m_materialProperties[IOROffset] = std::clamp(value, 1.0F, 3.0F);

		m_videoMemoryUpdated = true;
	}

	void
	StandardResource::setIBLIntensity (float value) noexcept
	{
		m_materialProperties[IBLIntensityOffset] = std::clamp(value, 0.0F, 1.0F);

		m_videoMemoryUpdated = true;
	}

	void
	StandardResource::setAutoIlluminationColor (const PixelFactory::Color< float > & color) noexcept
	{
		m_materialProperties[AutoIlluminationColorOffset] = color.red();
		m_materialProperties[AutoIlluminationColorOffset+1] = color.green();
		m_materialProperties[AutoIlluminationColorOffset+2] = color.blue();
		m_materialProperties[AutoIlluminationColorOffset+3] = color.alpha();

		m_videoMemoryUpdated = true;
	}

	void
	StandardResource::setAutoIlluminationAmount (float value) noexcept
	{
		m_materialProperties[AutoIlluminationAmountOffset] = std::max(0.0F, value);

		m_videoMemoryUpdated = true;
	}

	void
	StandardResource::setOpacity (float value) noexcept
	{
		m_materialProperties[OpacityOffset] = clampToUnit(value);

		m_videoMemoryUpdated = true;
	}

	void
	StandardResource::setAlphaThresholdToDiscard (float threshold) noexcept
	{
		m_materialProperties[AlphaThresholdOffset] = clampToUnit(threshold);

		m_videoMemoryUpdated = true;
	}

	void
	StandardResource::setReflectionAmount (float value) noexcept
	{
		m_materialProperties[ReflectionAmountOffset] = clampToUnit(value);

		m_videoMemoryUpdated = true;
	}

	void
	StandardResource::setRefractionAmount (float value) noexcept
	{
		m_materialProperties[RefractionAmountOffset] = clampToUnit(value);

		m_videoMemoryUpdated = true;
	}

	void
	StandardResource::setAOIntensity (float value) noexcept
	{
		m_materialProperties[AOIntensityOffset] = clampToUnit(value);

		m_videoMemoryUpdated = true;
	}

	void
	StandardResource::setClearCoatFactor (float value) noexcept
	{
		m_materialProperties[ClearCoatFactorOffset] = clampToUnit(value);

		m_videoMemoryUpdated = true;
	}

	void
	StandardResource::setClearCoatRoughness (float value) noexcept
	{
		m_materialProperties[ClearCoatRoughnessOffset] = clampToUnit(value);

		m_videoMemoryUpdated = true;
	}

	void
	StandardResource::setSubsurfaceIntensity (float value) noexcept
	{
		m_materialProperties[SubsurfaceIntensityOffset] = clampToUnit(value);

		m_videoMemoryUpdated = true;
	}

	void
	StandardResource::setSubsurfaceRadius (float value) noexcept
	{
		m_materialProperties[SubsurfaceRadiusOffset] = std::max(0.0F, value);

		m_videoMemoryUpdated = true;
	}

	void
	StandardResource::setSubsurfaceColor (const PixelFactory::Color< float > & color) noexcept
	{
		m_materialProperties[SubsurfaceColorOffset] = color.red();
		m_materialProperties[SubsurfaceColorOffset+1] = color.green();
		m_materialProperties[SubsurfaceColorOffset+2] = color.blue();
		m_materialProperties[SubsurfaceColorOffset+3] = color.alpha();

		m_videoMemoryUpdated = true;
	}

	void
	StandardResource::setSheenColor (const PixelFactory::Color< float > & color) noexcept
	{
		m_materialProperties[SheenColorOffset] = color.red();
		m_materialProperties[SheenColorOffset+1] = color.green();
		m_materialProperties[SheenColorOffset+2] = color.blue();
		m_materialProperties[SheenColorOffset+3] = color.alpha();

		m_videoMemoryUpdated = true;
	}

	void
	StandardResource::setSheenRoughness (float value) noexcept
	{
		m_materialProperties[SheenRoughnessOffset] = clampToUnit(value);

		m_videoMemoryUpdated = true;
	}

	void
	StandardResource::setAnisotropy (float value) noexcept
	{
		m_materialProperties[AnisotropyOffset] = std::clamp(value, -1.0F, 1.0F);

		m_videoMemoryUpdated = true;
	}

	void
	StandardResource::setAnisotropyRotation (float value) noexcept
	{
		m_materialProperties[AnisotropyRotationOffset] = clampToUnit(value);

		m_videoMemoryUpdated = true;
	}

	/* ==================== Transmission Component Setters ==================== */

	bool
	StandardResource::setTransmissionComponent (float factor, const PixelFactory::Color< float > & attenuationColor, float attenuationDistance, float thickness) noexcept
	{
		if ( this->isCreated() )
		{
			TraceWarning{ClassId} <<
				"The resource '" << this->name() << "' is created ! "
				"Unable to create or change the transmission component.";

			return false;
		}

		this->setTransmissionFactor(factor);
		this->setAttenuationColor(attenuationColor);
		this->setAttenuationDistance(attenuationDistance);
		this->setThicknessFactor(thickness);

		/* Enable environment cubemap for transmission (prefiltered cubemap sampling). */
		m_isUsingEnvironmentCubemapForTransmission = true;

		return true;
	}

	bool
	StandardResource::setTransmissionComponent (const std::shared_ptr< TextureResource::Abstract > & texture, const PixelFactory::Color< float > & attenuationColor, float attenuationDistance, float thickness) noexcept
	{
		if ( this->isCreated() )
		{
			TraceWarning{ClassId} <<
				"The resource '" << this->name() << "' is created ! "
				"Unable to create or change the transmission component.";

			return false;
		}

		const auto result = m_components.emplace(ComponentType::Transmission, std::make_unique< Texture >(Uniform::TransmissionSampler, SurfaceTransmissionFactor, texture));

		if ( !result.second || result.first->second == nullptr )
		{
			return false;
		}

		if ( !this->addDependency(texture) )
		{
			TraceError{ClassId} << "Unable to link the texture '" << texture->name() << "' dependency to PBR material '" << this->name() << "' for transmission component !";

			return false;
		}

		this->enableFlag(TextureEnabled);
		this->enableFlag(UsePrimaryTextureCoordinates);

		this->setTransmissionFactor(1.0F);
		this->setAttenuationColor(attenuationColor);
		this->setAttenuationDistance(attenuationDistance);
		this->setThicknessFactor(thickness);

		/* Enable environment cubemap for transmission (prefiltered cubemap sampling). */
		m_isUsingEnvironmentCubemapForTransmission = true;

		return true;
	}

	bool
	StandardResource::setTransmissionComponentFromGrabPass (float factor, const PixelFactory::Color< float > & attenuationColor, float attenuationDistance, float thickness) noexcept
	{
		if ( this->isCreated() )
		{
			TraceWarning{ClassId} <<
				"The resource '" << this->name() << "' is created ! "
				"Unable to create or change the transmission component.";

			return false;
		}

		this->setTransmissionFactor(factor);
		this->setAttenuationColor(attenuationColor);
		this->setAttenuationDistance(attenuationDistance);
		this->setThicknessFactor(thickness);

		/* Enable GrabPass screen-space transmission (mutually exclusive with cubemap). */
		m_isUsingGrabPassForTransmission = true;
		m_isUsingEnvironmentCubemapForTransmission = false;

		return true;
	}

	void
	StandardResource::enableDepthBasedOpacity (bool state) noexcept
	{
		m_isUsingDepthBasedOpacity = state;
	}

	/* ==================== Transmission Dynamic Property Setters ==================== */

	void
	StandardResource::setTransmissionFactor (float value) noexcept
	{
		m_materialProperties[TransmissionFactorOffset] = clampToUnit(value);

		m_videoMemoryUpdated = true;
	}

	void
	StandardResource::setAttenuationColor (const PixelFactory::Color< float > & color) noexcept
	{
		m_materialProperties[AttenuationColorOffset] = color.red();
		m_materialProperties[AttenuationColorOffset+1] = color.green();
		m_materialProperties[AttenuationColorOffset+2] = color.blue();
		m_materialProperties[AttenuationColorOffset+3] = color.alpha();

		m_videoMemoryUpdated = true;
	}

	void
	StandardResource::setAttenuationDistance (float value) noexcept
	{
		m_materialProperties[AttenuationDistanceOffset] = std::max(0.0001F, value);

		m_videoMemoryUpdated = true;
	}

	void
	StandardResource::setThicknessFactor (float value) noexcept
	{
		m_materialProperties[ThicknessFactorOffset] = std::max(0.0F, value);

		m_videoMemoryUpdated = true;
	}

	/* ==================== Iridescence Component Setters ==================== */

	bool
	StandardResource::setIridescenceComponent (float factor, float ior, float thicknessMin, float thicknessMax) noexcept
	{
		if ( this->isCreated() )
		{
			TraceWarning{ClassId} <<
				"The resource '" << this->name() << "' is created ! "
				"Unable to create or change the iridescence component.";

			return false;
		}

		this->setIridescenceFactor(factor);
		this->setIridescenceIOR(ior);
		this->setIridescenceThicknessMin(thicknessMin);
		this->setIridescenceThicknessMax(thicknessMax);

		return true;
	}

	bool
	StandardResource::setIridescenceComponent (const std::shared_ptr< TextureResource::Abstract > & texture, float ior, float thicknessMin, float thicknessMax) noexcept
	{
		if ( this->isCreated() )
		{
			TraceWarning{ClassId} <<
				"The resource '" << this->name() << "' is created ! "
				"Unable to create or change the iridescence component.";

			return false;
		}

		const auto result = m_components.emplace(ComponentType::Iridescence, std::make_unique< Texture >(Uniform::IridescenceSampler, SurfaceIridescenceFactor, texture));

		if ( !result.second || result.first->second == nullptr )
		{
			return false;
		}

		if ( !this->addDependency(texture) )
		{
			TraceError{ClassId} << "Unable to link the texture '" << texture->name() << "' dependency to PBR material '" << this->name() << "' for iridescence component !";

			return false;
		}

		this->enableFlag(TextureEnabled);
		this->enableFlag(UsePrimaryTextureCoordinates);

		this->setIridescenceFactor(1.0F);
		this->setIridescenceIOR(ior);
		this->setIridescenceThicknessMin(thicknessMin);
		this->setIridescenceThicknessMax(thicknessMax);

		return true;
	}

	/* ==================== Iridescence Dynamic Property Setters ==================== */

	void
	StandardResource::setIridescenceFactor (float value) noexcept
	{
		m_materialProperties[IridescenceFactorOffset] = clampToUnit(value);

		m_videoMemoryUpdated = true;
	}

	void
	StandardResource::setIridescenceIOR (float value) noexcept
	{
		m_materialProperties[IridescenceIOROffset] = std::clamp(value, 1.0F, 2.333F);

		m_videoMemoryUpdated = true;
	}

	void
	StandardResource::setIridescenceThicknessMin (float value) noexcept
	{
		m_materialProperties[IridescenceThicknessMinOffset] = std::max(0.0F, value);

		m_videoMemoryUpdated = true;
	}

	void
	StandardResource::setIridescenceThicknessMax (float value) noexcept
	{
		m_materialProperties[IridescenceThicknessMaxOffset] = std::max(0.0F, value);

		m_videoMemoryUpdated = true;
	}

	/* ==================== Dispersion Component Setters ==================== */

	bool
	StandardResource::setDispersionComponent (float dispersion) noexcept
	{
		if ( this->isCreated() )
		{
			TraceWarning{ClassId} <<
				"The resource '" << this->name() << "' is created ! "
				"Unable to create or change the dispersion component.";

			return false;
		}

		m_materialProperties[DispersionOffset] = std::max(dispersion, 0.0F);

		return true;
	}

	/* ==================== Dispersion Dynamic Property Setters ==================== */

	void
	StandardResource::setDispersion (float value) noexcept
	{
		m_materialProperties[DispersionOffset] = std::max(value, 0.0F);

		m_videoMemoryUpdated = true;
	}

	/* ==================== Specular Component Setters (KHR_materials_specular) ==================== */

	bool
	StandardResource::setSpecularComponent (float factor, const Base::PixelFactory::Color< float > & color) noexcept
	{
		if ( this->isCreated() )
		{
			TraceWarning{ClassId} <<
				"The resource '" << this->name() << "' is created ! "
				"Unable to create or change the specular component.";

			return false;
		}

		this->setSpecularFactor(factor);
		this->setSpecularColor(color);

		return true;
	}

	/* ==================== Specular Dynamic Property Setters ==================== */

	void
	StandardResource::setSpecularFactor (float value) noexcept
	{
		m_materialProperties[SpecularFactorOffset] = std::max(0.0F, value);

		m_videoMemoryUpdated = true;
	}

	void
	StandardResource::setSpecularColor (const Base::PixelFactory::Color< float > & color) noexcept
	{
		m_materialProperties[SpecularColorOffset] = color.red();
		m_materialProperties[SpecularColorOffset + 1] = color.green();
		m_materialProperties[SpecularColorOffset + 2] = color.blue();
		m_materialProperties[SpecularColorOffset + 3] = color.alpha();

		m_videoMemoryUpdated = true;
	}

	/* ==================== Specular JSON Parsing ==================== */

	bool
	StandardResource::parseSpecularComponent (const Json::Value & data) noexcept
	{
		if ( !data.isMember(SpecularKHRString) )
		{
			/* Specular is optional - not present means defaults (factor=1.0, color=white). */
			return true;
		}

		const auto & specularData = data[SpecularKHRString];

		const auto factor = FastJSON::getValue< float >(specularData, "Factor").value_or(DefaultSpecularFactor);
		this->setSpecularFactor(factor);

		if ( specularData.isMember(JKColor) )
		{
			const auto & colorData = specularData[JKColor];
			const auto color = colorData.isArray() ? parseColorComponent(colorData) : DefaultSpecularColor;

			this->setSpecularColor(color);
		}

		return true;
	}

	/* ==================== Iridescence JSON Parsing ==================== */

	bool
	StandardResource::parseIridescenceComponent (const Json::Value & data, Resources::AbstractServiceProvider & serviceProvider) noexcept
	{
		FillingType fillingType{};
		Json::Value componentData{};

		if ( !parseComponentBase(data, IridescenceString, fillingType, componentData, true) )
		{
			return false;
		}

		switch ( fillingType )
		{
			case FillingType::Value :
			{
				const auto factor = parseValueComponent(componentData);
				const auto ior = FastJSON::getValue< float >(data[IridescenceString], "IOR").value_or(DefaultIridescenceIOR);
				const auto thicknessMin = FastJSON::getValue< float >(data[IridescenceString], "ThicknessMin").value_or(DefaultIridescenceThicknessMin);
				const auto thicknessMax = FastJSON::getValue< float >(data[IridescenceString], "ThicknessMax").value_or(DefaultIridescenceThicknessMax);

				return this->setIridescenceComponent(factor, ior, thicknessMin, thicknessMax);
			}

			case FillingType::Gradient :
			case FillingType::Texture :
			case FillingType::VolumeTexture :
			case FillingType::Cubemap :
			case FillingType::AnimatedTexture :
			{
				const auto result = m_components.emplace(ComponentType::Iridescence, std::make_unique< Texture >(Uniform::IridescenceSampler, SurfaceIridescenceFactor, componentData, fillingType, serviceProvider));

				if ( !result.second || result.first->second == nullptr )
				{
					return false;
				}

				this->enableFlag(TextureEnabled);
				this->enableFlag(UsePrimaryTextureCoordinates);

				const auto ior = FastJSON::getValue< float >(data[IridescenceString], "IOR").value_or(DefaultIridescenceIOR);
				const auto thicknessMin = FastJSON::getValue< float >(data[IridescenceString], "ThicknessMin").value_or(DefaultIridescenceThicknessMin);
				const auto thicknessMax = FastJSON::getValue< float >(data[IridescenceString], "ThicknessMax").value_or(DefaultIridescenceThicknessMax);

				this->setIridescenceFactor(FastJSON::getValue< float >(data[IridescenceString], JKValue).value_or(1.0F));
				this->setIridescenceIOR(ior);
				this->setIridescenceThicknessMin(thicknessMin);
				this->setIridescenceThicknessMax(thicknessMax);
			}
				return true;

			case FillingType::None :
				/* Iridescence is optional. */
				return true;

			default:
				TraceError{ClassId} << "Invalid filling type for PBR material '" << this->name() << "' resource iridescence component !";

				return false;
		}
	}

	/* ==================== Emissive Strength Component (KHR_materials_emissive_strength) ==================== */

	bool
	StandardResource::setEmissiveStrength (float strength) noexcept
	{
		if ( this->isCreated() )
		{
			TraceWarning{ClassId} <<
				"The resource '" << this->name() << "' is created ! "
				"Unable to change the emissive strength.";

			return false;
		}

		m_materialProperties[EmissiveStrengthOffset] = std::max(0.0F, strength);

		return true;
	}

	void
	StandardResource::setEmissiveStrengthValue (float value) noexcept
	{
		m_materialProperties[EmissiveStrengthOffset] = std::max(0.0F, value);

		m_videoMemoryUpdated = true;
	}
}
