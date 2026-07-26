/*
 * src/Graphics/Renderable/SkyBoxResource.cpp
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

#include "SkyBoxResource.hpp"

/* Local inclusions. */
#include "Graphics/Material/BasicResource.hpp"
#include "Graphics/TextureResource/TextureCubemap.hpp"
#include "FastJSON.hpp"
#include "Types.hpp"

namespace EmEn::Graphics::Renderable
{
	using namespace Base;
	using namespace Base::Math;
	using namespace Base::PixelFactory;
	using namespace Graphics::Material;

	bool
	SkyBoxResource::load () noexcept
	{
		if ( !this->beginLoading() )
		{
			return false;
		}

		if ( !this->setGeometry(SkyBoxResource::getSkyBoxGeometry(this->serviceProvider())) )
		{
			return this->setLoadSuccess(false);
		}

		/* Keeps the IBL scale in step with the emission, as the JSON path does. */
		this->setLuminance(DefaultSkyLuminance);

		auto defaultCubemapResource = this->serviceProvider().container< TextureResource::TextureCubemap >()->getDefaultResource();

		const auto material = this->serviceProvider().container< BasicResource >()
			->getOrCreateResource("DefaultSkyboxMaterial", [defaultCubemapResource] (auto & materialResource) {
				if ( !materialResource.setTextureResource(defaultCubemapResource) )
				{
					return false;
				}

				/* A sky EMITS, it is not lit: full self-illumination, scaled to a physical
				 * luminance. The cubemap itself is LDR ([0,1] JPEG/PNG/Targa — the image pipeline
				 * has no HDR format), so this strength is what turns a normalized gradient into
				 * candela per square meter. ⚠️ Known limit of the LDR source: the sun disc clamps
				 * with the rest of the sky, so specular reflections of the sun are dull rather
				 * than blinding — see TODO.md, the HDR file format item. */
				materialResource.setAutoIlluminationAmount(1.0F);
				materialResource.setEmissiveStrength(DefaultSkyLuminance);

				return materialResource.setManualLoadSuccess(true);
			}, ComputePrimaryTextureCoordinates | PrimaryTextureCoordinatesUses3D);

		if ( !this->setMaterial(material) )
		{
			return this->setLoadSuccess(false);
		}

		/* Store the cubemap for environment IBL access. */
		m_environmentCubemap = defaultCubemapResource;

		return this->setLoadSuccess(true);
	}

	bool
	SkyBoxResource::load (const Json::Value & data) noexcept
	{
		if ( !this->beginLoading() )
		{
			return false;
		}

		if ( !this->setGeometry(SkyBoxResource::getSkyBoxGeometry(this->serviceProvider())) )
		{
			return this->setLoadSuccess(false);
		}

		if ( !data.isMember(JKTexture) || !data[JKTexture].isString() )
		{
			TraceError{ClassId} << "The '" << JKTexture << "' key is not present or not a string in '" << this->name() << "' Json file ! ";

			return this->setLoadSuccess(false);
		}

		const auto textureName = data[JKTexture].asString();

		/* A sky's LUMINANCE belongs to the sky asset, not to the code that displays it: a night
		 * cubemap and a noon cubemap are not the same photometric object. Optional — absent means
		 * a clear day, which is what every sky implicitly claimed before this key existed. */
		const auto luminance = std::max(FastJSON::getValue< float >(data, JKLuminance).value_or(DefaultSkyLuminance), 0.0F);

		/* ⚠️⚠️ The luminance drives TWO consumers and BOTH must hear about it: the material's
		 * emission (what you see when you look up) AND the background's luminance, which is the
		 * factor scaling every IBL contribution (`LightGenerator::setEnvironmentLuminance()`, fed
		 * from `Scene::background()->luminance()`). Nothing ever called this setter before, so the
		 * IBL scale sat on its 8000-nit daylight default in EVERY scene: a material reflecting a
		 * mere 3% of the environment then received 240 nits, against 0.1 nit of moonlit diffuse —
		 * 2400x too much, which turned Citadel's stone walls into white neon (found live with the
		 * owner, Jul 2026). Setting only the material would have fixed what the sky LOOKS like
		 * while leaving everything it LIGHTS wrong. */
		this->setLuminance(luminance);

		/* Store the cubemap for environment IBL access. */
		auto cubemapResource = this->serviceProvider().container< TextureResource::TextureCubemap >()->getResource(textureName, this->isDirectLoading());

		/* ⚠️ The luminance is part of the material IDENTITY: two manifests sharing one cubemap at
		 * different luminances must not share a material, or whichever loaded first would silently
		 * impose its exposure on the other. */
		const auto material = this->serviceProvider().container< BasicResource >()
			->getOrCreateResource(textureName + "SkyboxMaterial" + std::to_string(luminance), [cubemapResource, luminance] (auto & materialResource) {
				if ( !materialResource.setTextureResource(cubemapResource) )
				{
					return false;
				}

				/* A sky EMITS, it is not lit: full self-illumination, scaled to a physical
				 * luminance. The cubemap itself is LDR ([0,1] JPEG/PNG/Targa — the image pipeline
				 * has no HDR format), so this strength is what turns a normalized gradient into
				 * candela per square meter. ⚠️ Known limit of the LDR source: the sun disc clamps
				 * with the rest of the sky, so specular reflections of the sun are dull rather
				 * than blinding — see TODO.md, the HDR file format item. */
				materialResource.setAutoIlluminationAmount(1.0F);
				materialResource.setEmissiveStrength(luminance);

				return materialResource.setManualLoadSuccess(true);
			}, ComputePrimaryTextureCoordinates | PrimaryTextureCoordinatesUses3D);

		if ( !this->setMaterial(material) )
		{
			return this->setLoadSuccess(false);
		}

		this->setLightPosition(FastJSON::getValue< Vector< 3, float > >(data, JKLightPosition).value_or(Vector< 3, float >::origin()));

		this->setLightAmbientColor(FastJSON::getValue< Color< float > >(data, JKLightAmbientColor).value_or(Black));

		this->setLightDiffuseColor(FastJSON::getValue< Color< float > >(data, JKLightDiffuseColor).value_or(Black));

		this->setLightSpecularColor(FastJSON::getValue< Color< float > >(data, JKLightSpecularColor).value_or(Black));

		/* Store the cubemap for environment IBL access. */
		m_environmentCubemap = cubemapResource;

		return this->setLoadSuccess(true);
	}

	bool
	SkyBoxResource::load (const std::shared_ptr< Interface > & material) noexcept
	{
		if ( !this->beginLoading() )
		{
			return false;
		}

		if ( !this->setGeometry(SkyBoxResource::getSkyBoxGeometry(this->serviceProvider()) ) )
		{
			return this->setLoadSuccess(false);
		}

		if ( !this->setMaterial(material) )
		{
			return this->setLoadSuccess(false);
		}

		return this->setLoadSuccess(true);
	}

	bool
	SkyBoxResource::setGeometry (const std::shared_ptr< Geometry::Interface > & geometry) noexcept
	{
		if ( geometry == nullptr )
		{
			TraceError{ClassId} << "Geometry pointer tried to be attached to renderable object '" << this->name() << "' " << this << " is null !";

			return false;
		}

		this->setReadyForInstantiation(false);

		m_geometry = geometry;

		if ( !this->addDependency(m_geometry) )
		{
			TraceError{ClassId} << "Unable to set geometry for Skybox '" << this->name() << "' !";

			return false;
		}

		return true;
	}

	bool
	SkyBoxResource::setMaterial (const std::shared_ptr< Material::Interface > & material) noexcept
	{
		if ( material == nullptr )
		{
			TraceError{ClassId} << "Material pointer tried to be attached to renderable object '" << this->name() << "' " << this << " is null !";

			return false;
		}

		this->setReadyForInstantiation(false);

		m_material = material;

		if ( !this->addDependency(m_material) )
		{
			TraceError{ClassId} << "Unable to set material for Skybox '" << this->name() << "' !";

			return false;
		}

		return true;
	}

	bool
	SkyBoxResource::onDependenciesLoaded () noexcept
	{
		if constexpr ( IsDebug )
		{
			/* NOTE: Check the geometry resource. */
			if ( !this->geometry(0)->isCreated() )
			{
				TraceError{ClassId} << "The geometry for '" << this->name() << "' (" << this->classLabel() << ") is not created!";

				return false;
			}

			/* NOTE: Check material resource. */
			if ( !this->material(0)->isCreated() )
			{
				TraceError{ClassId} << "The material for '" << this->name() << "' (" << this->classLabel() << ") is not created!";

				return false;
			}
		}

		this->setReadyForInstantiation(true);

		return true;
	}
}
