/*
 * src/Libs/Network/Network.hpp
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

/* Engine configuration file. */
#include "emeraude_config.hpp"

/* STL inclusions. */
#include <string>
#include <filesystem>

/* Third party inclusions. */
#ifdef ASIO_ENABLED
/* NOTE: asio_throw_exception.hpp MUST be included before asio.hpp.
 * It defines ASIO_STANDALONE, ASIO_NO_EXCEPTIONS, and provides
 * the custom throw_exception() function required when exceptions are disabled. */
#include "asio_throw_exception.hpp"
#include "asio.hpp"
#endif

/* Local inclusions for usages. */
#include "URI.hpp"

namespace EmEn::Libs::Network
{
	/* Forward declarations. */
	class HTTPResponse;

	/**
	 * @brief Checks whether a connexion is available.
	 * @return bool
	 */
	[[nodiscard]]
	bool hasInternetConnexion () noexcept;

	/**
	 * @brief Downloads a file from a URI.
	 * @param uri A reference to a URI.
	 * @param filepath A reference to a filesystem path.
	 * @param verbose Show transaction information.
	 * @return bool
	 */
	bool download (const URI & uri, const std::filesystem::path & filepath, bool verbose = false) noexcept;

	/**
	 * @brief Creates a GET request header in the string format from a URI.
	 * @param uri A reference to a URI.
	 * @return std::string
	 */
	[[nodiscard]]
	std::string getGETHeaderRequest (const URI & uri) noexcept;

#ifdef ASIO_ENABLED
	/**
	 * @brief extractHeaders
	 * @param buffer
	 * @param delimiter
	 * @return HTTPResponse
	 */
	[[nodiscard]]
	HTTPResponse extractHeaders (asio::streambuf & buffer, const std::string & delimiter) noexcept;
#endif
}
