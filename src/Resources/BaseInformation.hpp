/*
 * src/Resources/BaseInformation.hpp
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
	/** @brief This structure represents a resource definition in a store. It holds the way to load it. */
	class BaseInformation final
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"ResourcesBaseInformation"};

			/**
			 * @brief Default constructor.
			 */
			BaseInformation () = default;

			/**
			 * @brief Returns false if the source is Undefined.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isValid () const noexcept
			{
				return m_source != SourceType::Undefined;
			}

			/**
			 * @brief Returns the name of the resource.
			 * @return const std::string &
			 */
			[[nodiscard]]
			const std::string &
			name () const noexcept
			{
				return m_name;
			}

			/**
			 * @brief Returns where the resource comes from.
			 * @return SourceType
			 */
			[[nodiscard]]
			SourceType
			sourceType () const noexcept
			{
				return m_source;
			}

			/**
			 * @brief Returns the resource data as JSON.
			 * @return const Json::Value &
			 */
			[[nodiscard]]
			const Json::Value &
			data () const noexcept
			{
				return m_data;
			}

			/**
			 * @brief Updates resource information from a downloaded file.
			 * @param filepath A reference to a filesystem path.
			 */
			void updateFromDownload (const std::filesystem::path & filepath) noexcept;

			/**
			 * @brief Parses a JSON node to extract resource information.
			 * @param fileSystem A reference to the file system service.
			 * @param resourceDefinition A reference to a JSON node.
			 * @return bool
			 */
			bool parse (const FileSystem & fileSystem, const Json::Value & resourceDefinition) noexcept;

		private:

			/**
			 * @brief Gets the resource name from the JSON node.
			 * @param resourceDefinition A reference to a JSON node.
			 * @return bool
			 */
			bool parseName (const Json::Value & resourceDefinition) noexcept;

			/**
			 * @brief Gets the resource source type from the JSON node.
			 * @param resourceDefinition A reference to a JSON node.
			 * @return bool
			 */
			bool parseSource (const Json::Value & resourceDefinition) noexcept;

			/**
			 * @brief Gets the resource data from the JSON node.
			 * @param fileSystem A reference to the file system service.
			 * @param resourceDefinition A reference to a JSON node.
			 * @return bool
			 */
			bool parseData (const FileSystem & fileSystem, const Json::Value & resourceDefinition) noexcept;

			/* JSON keys */
			static constexpr auto NameKey{"Name"};
			static constexpr auto SourceKey{"Source"};
			static constexpr auto DataKey{"Data"};

			std::string m_name;
			SourceType m_source{SourceType::Undefined};
			Json::Value m_data;
	};
}
