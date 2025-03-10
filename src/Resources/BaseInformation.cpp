/*
 * src/Resources/BaseInformation.cpp
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

#include "BaseInformation.hpp"

/* Local inclusions. */
#include "Libraries/Network/URL.hpp"
#include "Libraries/IO/IO.hpp"
#include "Libraries/String.hpp"
#include "FileSystem.hpp"
#include "Tracer.hpp"

namespace Emeraude::Resources
{
	using namespace Libraries;

	bool
	BaseInformation::parseName (const Json::Value & resourceDefinition) noexcept
	{
		if ( !resourceDefinition.isMember(NameKey) )
		{
			TraceError{ClassId} << "Resource base infos doesn't have '" << NameKey << "' key !";

			return false;
		}

		const auto & resourceName = resourceDefinition[NameKey];

		if ( !resourceName.isString() )
		{
			TraceError{ClassId} << "Key '" << NameKey << "' must be a string !";

			return false;
		}

		m_name = resourceName.asString();

		return true;
	}

	bool
	BaseInformation::parseSource (const Json::Value & resourceDefinition) noexcept
	{
		if ( !resourceDefinition.isMember(SourceKey) )
		{
			TraceInfo{ClassId} <<
				"Resource base infos doesn't have '" << SourceKey << "' key ! "
				"Assuming source is " << LocalDataString << '.';

			return true;
		}

		const auto & source = resourceDefinition[SourceKey];

		if ( !source.isString() )
		{
			TraceError{ClassId} << "Key '" << SourceKey << "' must be a string !";

			return false;
		}

		const auto sourceString = source.asString();

		m_source = to_SourceType(sourceString);

		if ( m_source == SourceType::Undefined )
		{
			TraceError{ClassId} <<
				"The value '" << sourceString << "' at '" << SourceKey << "' key is not handled ! "
				"It must be " << LocalDataString << ", " << ExternalDataString << " or " << DirectDataString << '.';

			return false;
		}

		return true;
	}

	bool
	BaseInformation::parseData (const Json::Value & resourceDefinition) noexcept
	{
		if ( !resourceDefinition.isMember(DataKey) )
		{
			TraceError{ClassId} << "Resource base infos doesn't have '" << SourceKey << "' key !";

			return false;
		}

		const auto & data = resourceDefinition[DataKey];

		switch ( m_source )
		{
			case SourceType::Undefined :
				TraceFatal{ClassId} <<
					"Resource infos '" << SourceKey << "' key is invalid ! "
					"This should never happen at this point !";

				return false;

			case SourceType::LocalData :
				if ( data.isString() )
				{
#if IS_WINDOWS
					const auto filepath = FileSystem::instance()->getFilepathFromDataDirectories(DataStores, String::replace('/', IO::Separator, data.asString()));
#else
					const auto filepath = FileSystem::instance()->getFilepathFromDataDirectories(DataStores, data.asString());
#endif
					if ( filepath.empty() )
					{
						TraceError{ClassId} << data << " for '" << DataKey << "' key (" << LocalDataString << ") point to an invalid location.";

						return false;
					}

					m_data = filepath.string();
				}
				else
				{
					TraceError{ClassId} << "Key '" << DataKey << "' (" << LocalDataString << ") must be a string !";

					return false;
				}
				break;

			case SourceType::ExternalData :
				if ( data.isString() )
				{
					const auto url = data.asString();

					if ( Network::URL::isURL(url) )
					{
						m_data = data;
					}
					else
					{
						TraceError{ClassId} << "'" << url << "' for '" << DataKey << "' key (" << ExternalDataString << ") is an invalid URL.";

						return false;
					}
				}
				else
				{
					TraceError{ClassId} << "Key '" << DataKey << "' (" << ExternalDataString << ") must be a string !";

					return false;
				}
				break;

			case SourceType::DirectData :
				if ( data.isObject() )
				{
					m_data = data;
				}
				else
				{
					TraceError{ClassId} << "Key '" << DataKey << "' (" << DirectDataString << ") must be a JSON object !";

					return false;
				}
				break;
		}

		return true;
	}

	void
	BaseInformation::updateFromDownload (const std::filesystem::path & filepath) noexcept
	{
		m_source = SourceType::LocalData;
		m_data = Json::Value(filepath.string());
	}

	bool
	BaseInformation::parse (const Json::Value & resourceDefinition) noexcept
	{
		/* 1. Check resource name. */
		if ( !this->parseName(resourceDefinition) )
		{
			return false;
		}

		/* 2. Check resource source. */
		if ( !this->parseSource(resourceDefinition) )
		{
			return false;
		}

		/* 3. Check resource data. */
		if ( !this->parseData(resourceDefinition) )
		{
			/* Invalid the resource */
			m_source = SourceType::Undefined;

			return false;
		}

		return true;
	}
}
