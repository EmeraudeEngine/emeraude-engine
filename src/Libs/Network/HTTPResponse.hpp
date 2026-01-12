/*
 * src/Libs/Network/HTTPResponse.hpp
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

/* STL inclusions. */
#include <iostream>

/* Local inclusions for usages. */
#include "HTTPHeaders.hpp"

namespace EmEn::Libs::Network
{
	/**
	 * @brief The HTTPResponse class.
	 * @extends EmEn::Libs::Network::HTTPHeaders
	 */
	class HTTPResponse final : public HTTPHeaders
	{
		public:

			static constexpr auto Date{"Date"};
			static constexpr auto Server{"Server"};
			static constexpr auto LastModified{"Last-Modified"};
			static constexpr auto ETag{"DateETag"};
			static constexpr auto AcceptRanges{"Accept-Ranges"};
			static constexpr auto ContentLength{"Content-Length"};
			static constexpr auto Connection {"Connection"};
			static constexpr auto ContentType {"Content-Type"};
			static constexpr auto TransferEncoding{"Transfer-Encoding"};
			static constexpr auto ContentEncoding {"Content-Encoding"};

			/**
			 * @brief Constructs a default HTTP response.
			 */
			HTTPResponse () noexcept
				: HTTPHeaders{Version::HTTP11}
			{

			}

			/**
			 * @brief Constructs an HTTP response.
			 * @param version The HTTP version.
			 */
			explicit
			HTTPResponse (Version version) noexcept
				: HTTPHeaders{version}
			{

			}

			/**
			 * @brief Constructs an HTTP response.
			 * @param code The code of the HTTP response.
			 * @param text The text of the HTTP response [std::move].
			 * @param version The HTTP version. Default HTTP 1.1.
			 */
			HTTPResponse (int code, std::string text, Version version = Version::HTTP11) noexcept
				: HTTPHeaders{version},
				m_codeResponse{code},
				m_textResponse{std::move(text)}
			{

			}

			/**
			 * @brief Constructs an HTTP response from raw headers.
			 * @param rawHeaders A reference to string.
			 * @return void
			 */
			explicit
			HTTPResponse (const std::string & rawHeaders) noexcept
			{
				if ( !this->parse(rawHeaders) )
				{
					std::cerr << "HTTPResponse(), unable to parse the HTTP response !" "\n";
				}
			}

			/** @copydoc EmEn::Libs::Network::HTTPHeaders::isValid() */
			[[nodiscard]]
			bool isValid () const noexcept override;

			/** @copydoc EmEn::Libs::Network::HTTPHeaders::toString() */
			[[nodiscard]]
			std::string toString () const noexcept override;

			/**
			 * @brief Sets the HTTP code response.
			 * @param code The code value.
			 * @return void
			 */
			void
			setCodeResponse (int code) noexcept
			{
				m_codeResponse = code;
			}

			/**
			 * @brief Sets the HTTP text response.
			 * @param text The code text.
			 * @return void
			 */
			void
			setTextResponse (const std::string & text) noexcept
			{
				m_textResponse = text;
			}

			/**
			 * @brief Returns the HTTP code response.
			 * @return int
			 */
			[[nodiscard]]
			int
			codeResponse () const noexcept
			{
				return m_codeResponse;
			}

			/**
			 * @brief Returns the HTTP text response.
			 * @return const std::string &
			 */
			[[nodiscard]]
			const std::string &
			textResponse () const noexcept
			{
				return m_textResponse;
			}

		private:

			/** @copydoc EmEn::Libs::Network::HTTPHeaders::parseFirstLine() */
			bool parseFirstLine (const std::string & line) noexcept override;

			int m_codeResponse{0};
			std::string m_textResponse;
	};
}
