/*
 * src/Graphics/Renderable/AbstractBackground.cpp
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

#include "AbstractBackground.hpp"

/* Local inclusions. */
#include "FastJSON.hpp"
#include "Types.hpp"
#include "Resources/Container.hpp"
#include "VertexFactory/ShapeGenerator.hpp"

namespace EmEn::Graphics::Renderable
{
	using namespace Base;
	using namespace Base::Math;
	using namespace Base::PixelFactory;
	using namespace Base::VertexFactory;

	std::shared_ptr< Geometry::IndexedVertexResource >
	AbstractBackground::getSkyBoxGeometry (Resources::AbstractServiceProvider & serviceProvider) noexcept
	{
		auto * geometries = serviceProvider.container< Geometry::IndexedVertexResource >();

		if ( geometries->isResourceLoaded(SkyBoxGeometryName) )
		{
			return geometries->getResource(SkyBoxGeometryName);
		}

		ShapeBuilderOptions< float > options{};
		options.enableGeometryFlipping(true);

		const auto shape = ShapeGenerator::generateCuboid(SkySize, SkySize, SkySize, options);

		auto geometry = std::make_shared< Geometry::IndexedVertexResource >(serviceProvider, SkyBoxGeometryName);

		if ( !geometry->load(shape) )
		{
			return nullptr;
		}

		geometries->addResource(geometry);

		return geometry;
	}

	std::shared_ptr< Geometry::IndexedVertexResource >
	AbstractBackground::getSkyDomeGeometry (Resources::AbstractServiceProvider & serviceProvider) noexcept
	{
		auto * geometries = serviceProvider.container< Geometry::IndexedVertexResource >();

		if ( geometries->isResourceLoaded(SkyDomeGeometryName) )
		{
			return geometries->getResource(SkyDomeGeometryName);
		}

		ShapeBuilderOptions< float > options{};
		options.enableGeometryFlipping(true);

		const auto shape = ShapeGenerator::generateSphere< float, uint32_t >(SkySize, 16, 16, options);

		auto geometry = std::make_shared< Geometry::IndexedVertexResource >(serviceProvider, SkyDomeGeometryName);

		if ( !geometry->load(shape) )
		{
			return nullptr;
		}

		geometries->addResource(geometry);

		return geometry;
	}

	bool
	AbstractBackground::parsePhotometry (const Json::Value & data) noexcept
	{
		/* ⚠️⚠️ The luminance drives TWO consumers and BOTH must hear about it: the background
		 * material's emission (what you see when you look up) AND the scene environment luminance
		 * (View UBO, fed by Scene::refreshAmbientLightProperties()), which scales every IBL
		 * contribution. A sky's LUMINANCE belongs to the sky asset, not to the code that displays
		 * it: a night cubemap and a noon cubemap are not the same photometric object. Optional —
		 * absent means a clear day, which is what every sky implicitly claimed before this key
		 * existed. */
		this->setLuminance(std::max(FastJSON::getValue< float >(data, JKLuminance).value_or(DefaultLuminance), 0.0F));

		/* Authored (or cheated) average color. Absent: the loader keeps the average computed
		 * from the actual source. */
		if ( const auto averageColor = FastJSON::getValue< Base::PixelFactory::Color< float > >(data, JKAverageColor) )
		{
			this->setAverageColor(*averageColor);
		}

		/* Explicit ambient illuminance. Absent: derived from the luminance (E = pi * L). */
		if ( const auto ambientIlluminance = FastJSON::getValue< float >(data, JKAmbientIlluminance) )
		{
			this->setAmbientIlluminance(*ambientIlluminance);
		}

		/* Celestial bodies. Zero is legitimate (overcast sky, nebula, cave dome): the
		 * background then only provides ambiance. */
		if ( data.isMember(JKStars) )
		{
			if ( !data[JKStars].isArray() )
			{
				TraceError{TracerTag} << "The '" << JKStars << "' key must be an array in '" << this->name() << "' !";

				return false;
			}

			for ( const auto & starData : data[JKStars] )
			{
				const auto direction = FastJSON::getValue< Base::Math::Vector< 3, float > >(starData, JKDirection);

				if ( !direction.has_value() )
				{
					TraceError{TracerTag} << "A star of '" << this->name() << "' has no '" << JKDirection << "' key !";

					return false;
				}

				const auto illuminance = FastJSON::getValue< float >(starData, JKIlluminance);

				if ( !illuminance.has_value() )
				{
					TraceError{TracerTag} << "A star of '" << this->name() << "' has no '" << JKIlluminance << "' key !";

					return false;
				}

				CelestialBody star;
				star.setType(CelestialBody::parseType(FastJSON::getValue< std::string >(starData, JKType).value_or("Sun")));
				star.setDirection(*direction);
				star.setIlluminance(*illuminance);

				/* The color temperature (kelvins) is the industry-standard authoring and WINS
				 * over a direct color when both are present (owner decision). */
				if ( const auto temperature = FastJSON::getValue< float >(starData, JKTemperature) )
				{
					star.setTemperature(*temperature);
				}
				else if ( const auto color = FastJSON::getValue< Base::PixelFactory::Color< float > >(starData, JKColor) )
				{
					star.setColor(*color);
				}

				star.setAngularDiameter(FastJSON::getValue< float >(starData, JKAngularDiameter).value_or(CelestialBody::EarthlikeAngularDiameter));
				star.setInTexture(FastJSON::getValue< bool >(starData, JKInTexture).value_or(true));

				this->addStar(star);
			}
		}

		return true;
	}
}
