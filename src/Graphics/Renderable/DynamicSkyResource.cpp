/*
 * src/Graphics/Renderable/DynamicSkyResource.cpp
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

#include "DynamicSkyResource.hpp"

/* Project configuration. */
#include "emeraude_config.hpp"

/* STL inclusions. */
#include <algorithm>
#include <array>
#include <functional>
#include <iterator>
#include <memory>
#include <optional>
#include <string>

/* Third-party inclusions. */
#include "json/json.h"

/* Local inclusions. */
#include "BlobTrait.hpp"
#include "FastJSON.hpp"
#include "Graphics/Geometry/IndexedVertexResource.hpp"
#include "Graphics/Geometry/Interface.hpp"
#include "Graphics/Material/Interface.hpp"
#include "Graphics/Material/SkyAtmosphereResource.hpp"
#include "Graphics/Renderable/Types.hpp"
#include "Graphics/SkyAtmosphere.hpp"
#include "Graphics/TextureResource/StoreTexture.hpp"
#include "Graphics/TextureResource/Texture2D.hpp"
#include "Logging/Severity.hpp"
#include "Math/Vector.hpp"
#include "PixelFactory/Color.hpp"
#include "Resources/Container.hpp"
#include "Tracer.hpp"

namespace EmEn::Graphics::Renderable
{
	using namespace Base;
	using namespace Base::Math;
	using namespace Base::PixelFactory;

	namespace
	{
		/** @brief The artistic keys of the sky material, read before the material is created. */
		struct SkyLook
		{
			std::optional< Color< float > > tint;
			std::optional< float > intensity;
			std::optional< float > saturation;
			std::optional< bool > starDisc;
			std::optional< float > limbDarkening;
		};
	}

	bool
	DynamicSkyResource::load () noexcept
	{
		return this->load(AtmosphereParameters::earth());
	}

	bool
	DynamicSkyResource::load (const AtmosphereParameters & parameters) noexcept
	{
		if ( !this->beginLoading() )
		{
			return false;
		}

		/* ABSOLUTE radiance (SkyEnvironment): the sky's environment cubemap is in nits, so the scale every consumer applies
		 * to a cubemap sample is 1 (owner decision 2026-10-25). */
		this->setLuminance(AbsoluteLuminance);

		return this->setLoadSuccess(this->build(parameters, nullptr));
	}

	bool
	DynamicSkyResource::load (const Json::Value & data) noexcept
	{
		if ( !this->beginLoading() )
		{
			return false;
		}

		const auto planet = FastJSON::getValue< std::string >(data, JKPlanet).value_or("Earth");
		auto parameters = AtmosphereParameters::preset(planet);

		if ( !parameters.has_value() )
		{
			TraceError{ClassId} << "The planet '" << planet << "' of dynamic sky '" << this->name() << "' is not a known atmosphere preset !";

			return this->setLoadSuccess(false);
		}

		/* Inline values over the preset: an alien sky is data. */
		if ( data.isMember(JKAtmosphere) )
		{
			const auto & atmosphere = data[JKAtmosphere];

			if ( !atmosphere.isObject() )
			{
				TraceError{ClassId} << "The '" << JKAtmosphere << "' key of dynamic sky '" << this->name() << "' must be an object !";

				return this->setLoadSuccess(false);
			}

			const auto overrideScalar = [&atmosphere] (const char * key, float & value) {
				if ( const auto found = FastJSON::getValue< float >(atmosphere, key) )
				{
					value = found.value();
				}
			};

			const auto overrideVector = [&atmosphere] (const char * key, Vector< 3, float > & value) {
				if ( const auto found = FastJSON::getValue< Vector< 3, float > >(atmosphere, key) )
				{
					value = found.value();
				}
			};

			overrideScalar(JKGroundRadius, parameters->groundRadius);
			overrideScalar(JKTopRadius, parameters->topRadius);
			overrideVector(JKRayleighScattering, parameters->rayleighScattering);
			overrideScalar(JKRayleighScaleHeight, parameters->rayleighScaleHeight);
			overrideVector(JKMieScattering, parameters->mieScattering);
			overrideVector(JKMieAbsorption, parameters->mieAbsorption);
			overrideScalar(JKMieScaleHeight, parameters->mieScaleHeight);
			overrideVector(JKAbsorptionExtinction, parameters->absorptionExtinction);
			overrideScalar(JKAbsorptionLayerWidth, parameters->absorptionLayerWidth);
			overrideVector(JKGroundAlbedo, parameters->groundAlbedo);

			/* One anisotropy for the three channels, or one each. */
			if ( const auto g = FastJSON::getValue< float >(atmosphere, JKMiePhaseG) )
			{
				parameters->miePhaseG = {g.value(), g.value(), g.value()};
			}
			else
			{
				overrideVector(JKMiePhaseG, parameters->miePhaseG);
			}

			if ( parameters->topRadius <= parameters->groundRadius )
			{
				TraceError{ClassId} << "The atmosphere of dynamic sky '" << this->name() << "' has its top under its ground !";

				return this->setLoadSuccess(false);
			}
		}

		if ( !this->parsePhotometry(data) )
		{
			return this->setLoadSuccess(false);
		}

		/* ABSOLUTE radiance (see load(parameters)): a "Luminance" key means nothing for a physical sky. */
		if ( data.isMember(JKLuminance) )
		{
			TraceWarning{ClassId} << "The dynamic sky '" << this->name() << "' ignores its 'Luminance': a physical sky is absolute radiance (luminance 1).";
		}

		this->setLuminance(AbsoluteLuminance);

		return this->setLoadSuccess(this->build(parameters.value(), &data));
	}

	bool
	DynamicSkyResource::build (const AtmosphereParameters & parameters, const Json::Value * data) noexcept
	{
		if ( !this->setGeometry(getSkyBoxGeometry(this->serviceProvider())) )
		{
			return false;
		}

		SkyLook look;

		if ( data != nullptr )
		{
			look.tint = FastJSON::getValue< Color< float > >(*data, Material::SkyAtmosphereResource::JKTint);
			look.intensity = FastJSON::getValue< float >(*data, Material::SkyAtmosphereResource::JKIntensity);
			look.saturation = FastJSON::getValue< float >(*data, Material::SkyAtmosphereResource::JKSaturation);
			look.starDisc = FastJSON::getValue< bool >(*data, Material::SkyAtmosphereResource::JKStarDisc);
			look.limbDarkening = FastJSON::getValue< float >(*data, Material::SkyAtmosphereResource::JKLimbDarkening);
		}

		/* The moons' surfaces (sRGB store textures, their identity fixed before they load). */
		std::array< std::shared_ptr< TextureResource::Texture2D >, Material::SkyAtmosphereResource::MaxMoons > moonSurfaces{};

		if ( data != nullptr && data->isMember(JKMoons) )
		{
			const auto & moons = (*data)[JKMoons];

			if ( !moons.isArray() )
			{
				TraceError{ClassId} << "The '" << JKMoons << "' key of dynamic sky '" << this->name() << "' must be an array !";

				return false;
			}

			if ( moons.size() > Material::SkyAtmosphereResource::MaxMoons )
			{
				TraceWarning{ClassId} << "The dynamic sky '" << this->name() << "' declares " << moons.size() << " moons: the first " << Material::SkyAtmosphereResource::MaxMoons << " are drawn.";
			}

			auto & textures = *this->serviceProvider().container< TextureResource::Texture2D >();

			for ( Json::ArrayIndex index = 0; index < std::min(moons.size(), static_cast< Json::ArrayIndex >(Material::SkyAtmosphereResource::MaxMoons)); ++index )
			{
				const auto & moon = moons[index];

				if ( !moon.isObject() )
				{
					TraceError{ClassId} << "The moon #" << index << " of dynamic sky '" << this->name() << "' must be an object !";

					return false;
				}

				if ( !moon.isMember(JKSurface) )
				{
					continue;
				}

				const auto surfaceName = FastJSON::getValue< std::string >(moon, JKSurface);

				if ( !surfaceName.has_value() || surfaceName->empty() )
				{
					TraceError{ClassId} << "The surface of moon #" << index << " of dynamic sky '" << this->name() << "' must be a texture name !";

					return false;
				}

				auto surface = TextureResource::getStoreTexture(textures, surfaceName.value(), TextureResource::Usage{.sRGB = true});

				/* An absent name answers the default resource: never drawn as a moon. */
				if ( surface == nullptr || surface->name() == Resources::Default )
				{
					TraceError{ClassId} << "The surface '" << surfaceName.value() << "' of moon #" << index << " of dynamic sky '" << this->name() << "' is not a texture of the store !";

					return false;
				}

				/* index < MaxMoons: the loop's bound. */
				*std::next(moonSurfaces.begin(), index) = std::move(surface);
			}
		}

		auto atmosphere = std::make_shared< SkyAtmosphere >(this->name(), parameters);

		const auto material = this->serviceProvider().container< Material::SkyAtmosphereResource >()
			->getOrCreateResource(this->name() + "SkyMaterial", [atmosphere, look, moonSurfaces] (auto & materialResource) {
				if ( !materialResource.setAtmosphere(atmosphere) )
				{
					return false;
				}

				for ( uint32_t index = 0; const auto & surface : moonSurfaces )
				{
					if ( surface != nullptr && !materialResource.setMoonSurface(index, surface) )
					{
						return false;
					}

					++index;
				}

				materialResource.setGrading(look.tint.value_or(White), look.intensity.value_or(1.0F));
				materialResource.setSaturation(look.saturation.value_or(1.0F));
				materialResource.enableStarDisc(look.starDisc.value_or(true));
				materialResource.setLimbDarkening(look.limbDarkening.value_or(Material::SkyAtmosphereResource::DefaultLimbDarkening));

				return materialResource.setManualLoadSuccess(true);
			});

		if ( material == nullptr || !this->setMaterial(material) )
		{
			return false;
		}

		m_skyMaterial = material;
		/* A material of the same name that already existed draws ITS atmosphere: the sky follows it. */
		m_atmosphere = material->atmosphere() != nullptr ? material->atmosphere() : std::move(atmosphere);

		return true;
	}

	bool
	DynamicSkyResource::setGeometry (const std::shared_ptr< Geometry::Interface > & geometry) noexcept
	{
		if ( geometry == nullptr )
		{
			TraceError{ClassId} << "Geometry pointer tried to be attached to renderable object '" << this->name() << "' " << this << " is null !";

			return false;
		}

		this->setReadyForInstantiation(false);

		m_geometry = geometry;

		return this->addDependency(m_geometry);
	}

	bool
	DynamicSkyResource::setMaterial (const std::shared_ptr< Material::Interface > & material) noexcept
	{
		if ( material == nullptr )
		{
			TraceError{ClassId} << "Material pointer tried to be attached to renderable object '" << this->name() << "' " << this << " is null !";

			return false;
		}

		this->setReadyForInstantiation(false);

		m_material = material;

		return this->addDependency(m_material);
	}

	bool
	DynamicSkyResource::onDependenciesLoaded () noexcept
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
