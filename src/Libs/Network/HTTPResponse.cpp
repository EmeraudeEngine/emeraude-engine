/*
 * src/Libs/Network/HTTPResponse.cpp
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

#include "HTTPResponse.hpp"

/* Emeraude-Engine configuration. */
#include "emeraude_config.hpp"

/* STL inclusions. */
#include <iostream>

/* Local inclusions. */
#include "Libs/String.hpp"

namespace EmEn::Libs::Network
{
	HTTPResponse::HTTPResponse () noexcept
		: HTTPHeaders(Version::HTTP11)
	{

	}

	HTTPResponse::HTTPResponse (Version version) noexcept
		: HTTPHeaders(version)
	{

	}

	HTTPResponse::HTTPResponse (int code, std::string text, Version version) noexcept
		: HTTPHeaders(version), m_codeResponse(code), m_textResponse(std::move(text))
	{

	}

	HTTPResponse::HTTPResponse (const std::string & rawHeaders) noexcept
	{
		if ( !this->parse(rawHeaders) )
		{
			std::cerr << __PRETTY_FUNCTION__ << ", unable to parse the HTTP response !" "\n";
		}
	}

	void
	HTTPResponse::setCodeResponse (int code) noexcept
	{
		m_codeResponse = code;
	}

	int
	HTTPResponse::codeResponse () const noexcept
	{
		return m_codeResponse;
	}

	void
	HTTPResponse::setTextResponse (const std::string & text) noexcept
	{
		m_textResponse = text;
	}

	const std::string &
	HTTPResponse::textResponse () const noexcept
	{
		return m_textResponse;
	}

	bool
	HTTPResponse::isValid () const noexcept
	{
		if ( m_codeResponse == 0 )
		{
			return false;
		}

		if ( m_textResponse.empty() )
		{
			return false;
		}

		return true;
	}

	bool
	HTTPResponse::parseFirstLine (const std::string & line) noexcept
	{
		const auto chunks = String::explode(line, ' ', false, 2);

		if ( chunks.size() != 3 )
		{
			std::cerr << __PRETTY_FUNCTION__ << ", invalid HTTP response : " << line << "\n";

			return false;
		}

		/* Protocol version. */
		this->setVersion(HTTPHeaders::parseVersion(chunks[0]));

		/* Code-response. */
		m_codeResponse = String::toNumber< int >(chunks[1]);

		/* Text-response. */
		m_textResponse = chunks[2];

		return true;
	}

	std::string
	HTTPResponse::toString () const noexcept
	{
		/* NOTE: HTTP version 0.9 have no header in response. */
		if ( m_version == Version::HTTP09 )
		{
			return {};
		}

		std::stringstream output;

		output << HTTPHeaders::version(m_version) << ' ' << m_codeResponse << ' ' << m_textResponse << Separator;

		for ( const auto & header : m_headers )
		{
			output << header.first << ": " << header.second << Separator;
		}

		output << Separator;

		return output.str();
	}
}
