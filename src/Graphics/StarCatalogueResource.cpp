/*
 * src/Graphics/StarCatalogueResource.cpp
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

#include "StarCatalogueResource.hpp"

/* STL inclusions. */
#include <cmath>
#include <fstream>
#include <numbers>
#include <optional>

/* Local inclusions. */
#include "BlobTrait.hpp"
#include "FastJSON.hpp"
#include "IO/IO.hpp"
#include "Tracer.hpp"

namespace EmEn::Graphics
{
	namespace
	{
		/**
		 * @brief Reads one canonical entry [ra_deg, dec_deg, vmag, bv, id, ...].
		 * @param entry A reference to the entry.
		 * @return std::optional< StarCatalogueResource::Star > Nothing for a malformed or out-of-range entry.
		 */
		[[nodiscard]]
		std::optional< StarCatalogueResource::Star >
		readStar (const Json::Value & entry) noexcept
		{
			if ( !entry.isArray() || entry.size() < 4 )
			{
				return std::nullopt;
			}

			const auto rightAscension = Base::FastJSON::asValue< double >(entry[0]);
			const auto declination = Base::FastJSON::asValue< double >(entry[1]);
			const auto magnitude = Base::FastJSON::asValue< float >(entry[2]);
			const auto colour = Base::FastJSON::asValue< float >(entry[3]);

			if ( !rightAscension.has_value() || !declination.has_value() || !magnitude.has_value() || !colour.has_value() )
			{
				return std::nullopt;
			}

			/* A NaN fails every comparison, so it is out of range too. */
			const auto within = [] (auto value, auto low, auto high) {
				return value >= low && value <= high;
			};

			if ( !within(*rightAscension, 0.0, 360.0) || !within(*declination, -90.0, 90.0) ||
				!within(*magnitude, StarCatalogueResource::MinimumMagnitude, StarCatalogueResource::MaximumMagnitude) ||
				!within(*colour, StarCatalogueResource::MinimumColourIndex, StarCatalogueResource::MaximumColourIndex) )
			{
				return std::nullopt;
			}

			const auto alpha = *rightAscension * (std::numbers::pi / 180.0);
			const auto delta = *declination * (std::numbers::pi / 180.0);

			return StarCatalogueResource::Star{
				.direction = {
					static_cast< float >(std::cos(delta) * std::cos(alpha)),
					static_cast< float >(std::cos(delta) * std::sin(alpha)),
					static_cast< float >(std::sin(delta))
				},
				.visualMagnitude = *magnitude,
				.colourIndex = *colour
			};
		}
	}

	bool
	StarCatalogueResource::load () noexcept
	{
		if ( !this->beginLoading() )
		{
			return false;
		}

		m_stars.clear();

		return this->setLoadSuccess(true);
	}

	bool
	StarCatalogueResource::load (const std::filesystem::path & filepath) noexcept
	{
		if ( !this->beginLoading() )
		{
			return false;
		}

		std::ifstream file;

		if ( !Base::IO::openInputFile(filepath, file, std::ios::in) )
		{
			TraceError{ClassId} << "Unable to open the star catalogue '" << filepath << "' !";

			return this->setLoadSuccess(false);
		}

		Json::Value root;
		const Json::CharReaderBuilder readerBuilder;
		std::string errors;

		if ( !Json::parseFromStream(readerBuilder, file, &root, &errors) )
		{
			TraceError{ClassId} << "Failed to parse the star catalogue '" << filepath << "': " << errors;

			return this->setLoadSuccess(false);
		}

		return this->setLoadSuccess(this->parse(root));
	}

	bool
	StarCatalogueResource::load (const Json::Value & data) noexcept
	{
		if ( !this->beginLoading() )
		{
			return false;
		}

		return this->setLoadSuccess(this->parse(data));
	}

	bool
	StarCatalogueResource::parse (const Json::Value & data) noexcept
	{
		if ( !data.isObject() )
		{
			TraceError{ClassId} << "The star catalogue '" << this->name() << "' is not a JSON object !";

			return false;
		}

		const auto format = Base::FastJSON::getValue< std::string >(data, "format");
		const auto version = Base::FastJSON::getValue< int >(data, "version");

		if ( format.value_or("") != FormatName || version.value_or(0) != FormatVersion )
		{
			TraceError{ClassId} << "The star catalogue '" << this->name() << "' is not an " << FormatName << " v" << FormatVersion << " document !";

			return false;
		}

		if ( !data.isMember("stars") || !data["stars"].isArray() )
		{
			TraceError{ClassId} << "The star catalogue '" << this->name() << "' has no 'stars' array !";

			return false;
		}

		const auto & entries = data["stars"];

		m_stars.clear();
		m_stars.reserve(entries.size());
		m_refusedCount = 0;

		for ( const auto & entry : entries )
		{
			if ( auto star = readStar(entry); star.has_value() )
			{
				m_stars.push_back(*star);
			}
			else
			{
				++m_refusedCount;
			}
		}

		m_catalogueName = Base::FastJSON::getValue< std::string >(data, "name").value_or(this->name());
		m_credit = Base::FastJSON::getValue< std::string >(data, "credit").value_or("");

		if ( m_refusedCount > 0 )
		{
			TraceWarning{ClassId} << "The star catalogue '" << this->name() << "' refused " << m_refusedCount << " malformed or out-of-range entries.";
		}

		TraceInfo{ClassId} << "Star catalogue '" << m_catalogueName << "': " << m_stars.size() << " stars.";

		return true;
	}
}
