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
#include "Resources/Container.hpp"
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
		m_environmentCubemap = std::move(defaultCubemapResource);

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

		if ( !data.isMember(JKCubemap) || !data[JKCubemap].isString() )
		{
			TraceError{ClassId} << "The '" << JKCubemap << "' key is not present or not a string in '" << this->name() << "' Json file ! ";

			return this->setLoadSuccess(false);
		}

		const auto textureName = data[JKCubemap].asString();

		/* Photometric part of the manifest: luminance (⚠️ TWO consumers — the material emission
		 * below AND the IBL scale, see AbstractBackground::parsePhotometry()), average color,
		 * ambient illuminance and celestial bodies. */
		if ( !this->parsePhotometry(data) )
		{
			return this->setLoadSuccess(false);
		}

		const auto luminance = this->luminance();

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

		/* Store the cubemap for environment IBL access. */
		m_environmentCubemap = std::move(cubemapResource);

		return this->setLoadSuccess(true);
	}

	bool
	SkyBoxResource::load (const std::shared_ptr< Interface > & material) noexcept
	{
		if ( !this->beginLoading() )
		{
			return false;
		}

		/* NOTE: The caller owns the material, so its emissive strength is its own business —
		 * but the BACKGROUND luminance (the IBL scale) must still be coherent: without further
		 * information, this custom sky claims the daylight default. Call setLuminance() after
		 * this load for a non-daylight sky. */
		this->setLuminance(DefaultSkyLuminance);

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
		/* The default ambient illuminance follows the MEASURED sky, not the uniform-dome pi
		 * (owner decision, Jul 2026): the cubemap pixels are final here. An explicit
		 * "AmbientIlluminance" manifest key bypasses this entirely. */
		if ( m_environmentCubemap != nullptr )
		{
			this->setAmbientIlluminanceFactor(m_environmentCubemap->hemisphereIlluminanceFactor());
		}

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
