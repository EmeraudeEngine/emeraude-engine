/*
 * src/Resources/BaseInformation.hpp
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
#include <string>
#include <filesystem>

/* Third-party inclusions. */
#ifndef JSON_USE_EXCEPTION
#define JSON_USE_EXCEPTION 0
#endif
#include "json/json.h"

/* Local inclusions for usages. */
#include "Types.hpp"

/* Forward declarations. */
namespace EmEn
{
	class FileSystem;
}

namespace EmEn::Resources
{
	/**
	 * @class BaseInformation
	 * @brief Represents a resource definition extracted from JSON, holding metadata and loading information.
	 *
	 * This class encapsulates the base information required to load a resource within the
	 * Emeraude Engine resource management system. It parses JSON resource definitions to extract:
	 * - Resource name
	 * - Source type (LocalData, ExternalData, or DirectData)
	 * - Associated data (file path, URL, or inline JSON definition)
	 *
	 * The class validates the JSON structure and ensures all required fields are present and
	 * correctly formatted. It supports three source types:
	 * - **LocalData**: Resource data is loaded from a local file path
	 * - **ExternalData**: Resource data is downloaded from an external URL
	 * - **DirectData**: Resource data is defined inline in the JSON definition
	 *
	 * @note This class is marked as final and cannot be inherited.
	 * @see EmEn::Resources::Manager
	 * @see EmEn::Resources::SourceType
	 * @see EmEn::FileSystem
	 * @since 0.8.0
	 * @version 0.8.35
	 */
	class BaseInformation final
	{
		public:

			/**
			 * @brief Class identifier string used for logging and debugging.
			 * @version 0.8.35
			 */
			static constexpr auto ClassId{"ResourcesBaseInformation"};

			/**
			 * @brief Default constructor.
			 *
			 * Initializes a BaseInformation object with undefined source type and empty name and data.
			 *
			 * @post The object is created with m_source set to SourceType::Undefined.
			 * @version 0.8.35
			 */
			BaseInformation () = default;

			/**
			 * @brief Checks if the resource information is valid.
			 *
			 * A BaseInformation object is considered valid if its source type has been successfully
			 * parsed from JSON and is not SourceType::Undefined. An invalid object indicates that
			 * parsing failed or that the object was never initialized.
			 *
			 * @return true if the source type is not Undefined, false otherwise.
			 * @note This method is marked [[nodiscard]] as ignoring its result may lead to bugs.
			 * @version 0.8.35
			 */
			[[nodiscard]]
			bool
			isValid () const noexcept
			{
				return m_source != SourceType::Undefined;
			}

			/**
			 * @brief Returns the name of the resource.
			 *
			 * The resource name is extracted from the JSON "Name" field during parsing.
			 * This identifier is used to reference the resource throughout the engine.
			 *
			 * @return Const reference to the resource name string.
			 * @pre The object should be valid (isValid() returns true).
			 * @note Returns an empty string if parsing has not been performed or failed.
			 * @version 0.8.35
			 */
			[[nodiscard]]
			const std::string &
			name () const noexcept
			{
				return m_name;
			}

			/**
			 * @brief Returns the source type of the resource.
			 *
			 * The source type indicates how the resource data should be loaded:
			 * - SourceType::LocalData: Load from a local file path
			 * - SourceType::ExternalData: Download from an external URL
			 * - SourceType::DirectData: Use inline JSON definition
			 * - SourceType::Undefined: Invalid or unparsed resource
			 *
			 * @return The SourceType enumeration value.
			 * @see EmEn::Resources::SourceType
			 * @version 0.8.35
			 */
			[[nodiscard]]
			SourceType
			sourceType () const noexcept
			{
				return m_source;
			}

			/**
			 * @brief Returns the resource data as a JSON value.
			 *
			 * The content and structure of the returned JSON depends on the source type:
			 * - **LocalData**: Contains the absolute file path as a string
			 * - **ExternalData**: Contains the URL as a string
			 * - **DirectData**: Contains the complete JSON resource definition as an object
			 *
			 * @return Const reference to the Json::Value containing the resource data.
			 * @pre The object should be valid (isValid() returns true).
			 * @note The returned reference is valid as long as this object exists.
			 * @version 0.8.35
			 */
			[[nodiscard]]
			const Json::Value &
			data () const noexcept
			{
				return m_data;
			}

			/**
			 * @brief Updates resource information after downloading an external resource.
			 *
			 * This method is called after an ExternalData resource has been successfully downloaded
			 * to the local filesystem. It converts the resource from ExternalData to LocalData and
			 * updates the data field to contain the local file path instead of the original URL.
			 *
			 * @param filepath The local filesystem path where the downloaded resource was saved.
			 * @post The source type is changed to SourceType::LocalData.
			 * @post The data field contains the file path as a string.
			 * @note This method should only be called on resources that were originally ExternalData.
			 * @version 0.8.35
			 */
			void updateFromDownload (const std::filesystem::path & filepath) noexcept;

			/**
			 * @brief Parses a JSON resource definition to extract all base information.
			 *
			 * This method orchestrates the complete parsing process by calling private helper methods
			 * in sequence:
			 * 1. Extracts and validates the resource name (parseName)
			 * 2. Extracts and validates the source type (parseSource)
			 * 3. Extracts and validates the data field (parseData)
			 *
			 * The method performs comprehensive validation and logs errors if any required field is
			 * missing or malformed. If parsing fails at any stage, the source type is set to
			 * SourceType::Undefined to invalidate the object.
			 *
			 * Expected JSON structure:
			 * @code
			 * {
			 *   "Name": "ResourceName",
			 *   "Source": "LocalData" | "ExternalData" | "DirectData",
			 *   "Data": <string or object depending on Source>
			 * }
			 * @endcode
			 *
			 * @param fileSystem Reference to the FileSystem service for path resolution and validation.
			 * @param resourceDefinition The JSON object containing the resource definition to parse.
			 * @return true if parsing succeeded and all fields are valid, false otherwise.
			 * @post If successful, the object is valid (isValid() returns true).
			 * @post If failed, m_source is set to SourceType::Undefined.
			 * @note The "Source" field is optional and defaults to "LocalData" if not present.
			 * @see parseName(), parseSource(), parseData()
			 * @version 0.8.35
			 */
			[[nodiscard]]
			bool parse (const FileSystem & fileSystem, const Json::Value & resourceDefinition) noexcept;

		private:

			/**
			 * @brief Extracts and validates the resource name from the JSON definition.
			 *
			 * This helper method checks for the presence of the "Name" key in the JSON object,
			 * validates that it is a string, and stores it in m_name. Errors are logged if
			 * the key is missing or has an invalid type.
			 *
			 * @param resourceDefinition The JSON object containing the resource definition.
			 * @return true if the name was successfully extracted, false otherwise.
			 * @post If successful, m_name contains the extracted resource name.
			 * @note This method is called first in the parsing sequence by parse().
			 * @version 0.8.35
			 */
			bool parseName (const Json::Value & resourceDefinition) noexcept;

			/**
			 * @brief Extracts and validates the resource source type from the JSON definition.
			 *
			 * This helper method checks for the "Source" key in the JSON object and converts
			 * it to a SourceType enumeration value. If the "Source" key is absent, it defaults
			 * to LocalData. The method validates that the source string is one of the accepted
			 * values: "LocalData", "ExternalData", or "DirectData".
			 *
			 * @param resourceDefinition The JSON object containing the resource definition.
			 * @return true if the source type was successfully parsed, false if invalid.
			 * @post If successful, m_source contains the parsed SourceType.
			 * @post If the "Source" key is missing, m_source is set to SourceType::LocalData.
			 * @note This method is called second in the parsing sequence by parse().
			 * @version 0.8.35
			 */
			bool parseSource (const Json::Value & resourceDefinition) noexcept;

			/**
			 * @brief Extracts and validates the resource data from the JSON definition.
			 *
			 * This helper method processes the "Data" key based on the previously parsed source type.
			 * The expected format and validation depends on m_source:
			 *
			 * - **LocalData**: "Data" must be a string containing a relative path. The method
			 *   resolves it to an absolute path using the FileSystem service and validates that
			 *   the file exists.
			 *
			 * - **ExternalData**: "Data" must be a string containing a valid URL. The method
			 *   validates the URL format using Network::URL::isURL().
			 *
			 * - **DirectData**: "Data" must be a JSON object containing the inline resource
			 *   definition.
			 *
			 * @param fileSystem Reference to the FileSystem service for path resolution.
			 * @param resourceDefinition The JSON object containing the resource definition.
			 * @return true if the data was successfully extracted and validated, false otherwise.
			 * @pre m_source must be set to a valid source type (not Undefined).
			 * @post If successful, m_data contains the validated resource data.
			 * @note This method is called last in the parsing sequence by parse().
			 * @note On Windows, forward slashes in paths are automatically converted to backslashes.
			 * @version 0.8.35
			 */
			bool parseData (const FileSystem & fileSystem, const Json::Value & resourceDefinition) noexcept;

			/**
			 * @brief JSON key name for the resource name field.
			 * @version 0.8.35
			 */
			static constexpr auto NameKey{"Name"};

			/**
			 * @brief JSON key name for the resource source type field.
			 * @version 0.8.35
			 */
			static constexpr auto SourceKey{"Source"};

			/**
			 * @brief JSON key name for the resource data field.
			 * @version 0.8.35
			 */
			static constexpr auto DataKey{"Data"};

			/**
			 * @brief The resource name extracted from the JSON "Name" field.
			 * @version 0.8.35
			 */
			std::string m_name;

			/**
			 * @brief The resource source type indicating how to load the resource.
			 *
			 * Defaults to SourceType::Undefined until successfully parsed.
			 *
			 * @version 0.8.35
			 */
			SourceType m_source{SourceType::Undefined};

			/**
			 * @brief The resource data as a JSON value.
			 *
			 * The content varies based on m_source:
			 * - LocalData: String containing absolute file path
			 * - ExternalData: String containing URL
			 * - DirectData: JSON object with inline definition
			 *
			 * @version 0.8.35
			 */
			Json::Value m_data;
	};
}
