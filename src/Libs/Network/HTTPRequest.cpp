/*
 * src/Libs/Network/HTTPRequest.cpp
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

#include "HTTPRequest.hpp"

/* Emeraude-Engine configuration. */
#include "emeraude_config.hpp"

/* Local inclusions. */
#include "Libs/String.hpp"

namespace EmEn::Libs::Network
{
	HTTPRequest::HTTPRequest (Version version) noexcept
		: HTTPHeaders(version)
	{

	}

	HTTPRequest::HTTPRequest (Method method, const URI & resource, Version version) noexcept
		: HTTPHeaders(version), m_method(method), m_resource(resource)
	{
		this->add(Host, to_string(resource.uriDomain().hostname()));
	}

	HTTPRequest::HTTPRequest (const std::string & rawHeaders) noexcept
	{
		if ( !this->parse(rawHeaders) )
		{
			std::cerr << __PRETTY_FUNCTION__ << ", unable to parse the HTTP request !" "\n";
		}
	}

	void
	HTTPRequest::setMethod (Method method) noexcept
	{
		m_method = method;
	}

	HTTPRequest::Method
	HTTPRequest::method () const noexcept
	{
		return m_method;
	}

	void
	HTTPRequest::setResource (const URI & resource, bool doNotAddHeader) noexcept
	{
		m_resource = resource;

		if ( !doNotAddHeader )
		{
			this->add(Host, to_string(resource.uriDomain().hostname()));
		}
	}

	const URI &
	HTTPRequest::resource () const noexcept
	{
		return m_resource;
	}

	bool
	HTTPRequest::isValid () const noexcept
	{
		if ( m_method == Method::NONE )
		{
			return false;
		}

		if ( m_resource.empty() )
		{
			return false;
		}

		return true;
	}

	bool
	HTTPRequest::parseFirstLine (const std::string & line) noexcept
	{
		const auto chunks = String::explode(line, ' ');

		if ( chunks.size() < 2 )
		{
			std::cerr << __PRETTY_FUNCTION__ << ", invalid HTTP request : " << line << "\n";

			return false;
		}

		/* Command */
		m_method = HTTPRequest::parseMethod(chunks[0]);

		if ( m_method == Method::NONE )
		{
			std::cerr << __PRETTY_FUNCTION__ << ", invalid method for the request : " << line << "\n";

			return false;
		}

		/* URL */
		m_resource = chunks[1];

		/* (Protocol version) */
		if ( chunks.size() > 2 )
		{
			this->setVersion(HTTPHeaders::parseVersion(chunks[2]));
		}

		return true;
	}

	std::string
	HTTPRequest::toString () const noexcept
	{
		std::stringstream string{};

		/* NOTE: HTTP version 0.9 have no header in response. */
		if ( m_version == Version::HTTP09 )
		{
			string << HTTPRequest::method(m_method) << " /" << m_resource.path() << Separator;
		}
		else
		{
			string << HTTPRequest::method(m_method) << " /" << m_resource.path() << ' ' << HTTPHeaders::version(m_version) << Separator;
		}

		for ( const auto & header : m_headers )
		{
			string << header.first << ": " << header.second << Separator;
		}

		string << Separator;

		return string.str();
	}

	const char *
	HTTPRequest::method (Method method) noexcept
	{
		switch ( method )
		{
			case Method::GET :
				return "GET";

			case Method::HEAD :
				return "HEAD";

			case Method::POST :
				return "POST";

			case Method::OPTIONS :
				return "OPTIONS";

			case Method::CONNECT :
				return "CONNECT";

			case Method::TRACE :
				return "TRACE";

			case Method::PUT :
				return "PUT";

			case Method::PATCH :
				return "PATCH";

			case Method::DELETE :
				return "DELETE";

			default:
				return nullptr;
		}
	}

	HTTPRequest::Method
	HTTPRequest::parseMethod (const std::string & method) noexcept
	{
		if ( method == GET )
		{
			return Method::GET;
		}

		if ( method == HEAD )
		{
			return Method::HEAD;
		}

		if ( method == POST )
		{
			return Method::POST;
		}

		if ( method == OPTIONS )
		{
			return Method::OPTIONS;
		}

		if ( method == CONNECT )
		{
			return Method::CONNECT;
		}

		if ( method == TRACE )
		{
			return Method::TRACE;
		}

		if ( method == PUT )
		{
			return Method::PUT;
		}

		if ( method == PATCH )
		{
			return Method::PATCH;
		}

		if ( method == DELETE )
		{
			return Method::DELETE;
		}

		return Method::NONE;
	}
}
