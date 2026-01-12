/*
 * src/Libs/Network/HTTPRequest.hpp
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
 * https://github.com/londnoir/emeraude-engine
 *
 * --- THIS IS AUTOMATICALLY GENERATED, DO NOT CHANGE ---
 */

#pragma once

/* Local inclusions for usages. */
#include "HTTPHeaders.hpp"
#include "URI.hpp"

#if IS_WINDOWS
#undef DELETE
#endif

namespace EmEn::Libs::Network
{
	/**
	 * @brief The HTTPRequest class.
	 * @extends EmEn::Libs::Network::HTTPHeaders
	 */
	class HTTPRequest final : public HTTPHeaders
	{
		public:

			/**
			 * @brief The HTTP method enumeration.
			 */
			enum class Method
			{
				GET,
				HEAD,
				POST,
				OPTIONS,
				CONNECT,
				TRACE,
				PUT,
				PATCH,
				DELETE,
				NONE
			};

			static constexpr auto GET = "GET";
			static constexpr auto HEAD = "HEAD";
			static constexpr auto POST = "POST";
			static constexpr auto OPTIONS = "OPTIONS";
			static constexpr auto CONNECT = "CONNECT";
			static constexpr auto TRACE = "TRACE";
			static constexpr auto PUT = "PUT";
			static constexpr auto PATCH = "PATCH";
			static constexpr auto DELETE = "DELETE";

			/* HTTP/1.0 */
			static constexpr auto Host = "Host";
			static constexpr auto Referer = "Referer";
			static constexpr auto UserAgent = "User-Agent";
			static constexpr auto Date = "Date";
			static constexpr auto Server = "Server";
			static constexpr auto ContentType = "Content-Type";
			static constexpr auto ContentLength = "Content-Length";
			static constexpr auto Expires = "Expires";
			static constexpr auto LastModified = "Last-Modified";
			/* HTTP/1.1 */
			static constexpr auto Connection = "Connection";
			static constexpr auto Accept = "Accept";
			static constexpr auto AcceptCharset = "Accept-Charset";
			static constexpr auto AcceptLanguage = "Accept-Language";
			static constexpr auto AcceptEncoding = "Accept-Encoding";
			static constexpr auto Trailer = "Trailer";
			static constexpr auto TE = "TE";

			/**
			 * @brief Constructs an HTTP request.
			 * @param version The HTTP version. Default HTTP 1.1.
			 */
			explicit
			HTTPRequest (Version version = Version::HTTP11) noexcept
				: HTTPHeaders{version}
			{

			}

			/**
			 * @brief Constructs an HTTP request.
			 * @param method The method of the request.
			 * @param resource The resource of the request.
			 * @param version The HTTP version. Default HTTP 1.1.
			 */
			HTTPRequest (Method method, const URI & resource, Version version = Version::HTTP11) noexcept
				: HTTPHeaders{version},
				m_method{method},
				m_resource{resource}
			{
				this->add(Host, to_string(resource.uriDomain().hostname()));
			}

			/**
			 * @brief Constructs an HTTP request from a raw string.
			 * @param rawHeaders A reference to string.
			 */
			explicit
			HTTPRequest (const std::string & rawHeaders) noexcept
			{
				if ( !this->parse(rawHeaders) )
				{
					std::cerr << "HTTPRequest::HTTPRequest(), unable to parse the HTTP request !" "\n";
				}
			}

			/** @copydoc EmEn::Libs::Network::HTTPHeaders::isValid() */
			[[nodiscard]]
			bool isValid () const noexcept override;

			/** @copydoc EmEn::Libs::Network::HTTPHeaders::toString() */
			[[nodiscard]]
			std::string toString () const noexcept override;

			/**
			 * @brief Sets the request method.
			 * @param method The method of the request.
			 * @return void
			 */
			void
			setMethod (Method method) noexcept
			{
				m_method = method;
			}

			/**
			 * @brief Returns the request method.
			 * @return Method
			 */
			[[nodiscard]]
			Method
			method () const noexcept
			{
				return m_method;
			}

			/**
			 * @brief Sets the request resource.
			 * @param resource A reference to a URI.
			 * @param doNotAddHeader Disable the automatic adding of "Host:" header.
			 * @return void
			 */
			void
			setResource (const URI & resource, bool doNotAddHeader = false) noexcept
			{
				m_resource = resource;

				if ( !doNotAddHeader )
				{
					this->add(Host, to_string(resource.uriDomain().hostname()));
				}
			}

			/**
			 * @brief Returns the request resource.
			 * @return const URI &
			 */
			[[nodiscard]]
			const URI &
			resource () const noexcept
			{
				return m_resource;
			}

			/**
			 * @brief Converts an enum type method to a C-String.
			 * @param method A method enumeration value.
			 * @return const char *
			 */
			[[nodiscard]]
			static const char * method (Method method) noexcept;

			/**
			 * @brief Converts a C-String type method to an enum value.
			 * @param method A reference to a string.
			 * @return Method
			 */
			[[nodiscard]]
			static Method parseMethod (const std::string & method) noexcept;

		private:

			/** @copydoc EmEn::Libs::Network::HTTPHeaders::parseFirstLine() */
			bool parseFirstLine (const std::string & line) noexcept override;

			Method m_method{Method::NONE};
			URI m_resource{};
	};
}
