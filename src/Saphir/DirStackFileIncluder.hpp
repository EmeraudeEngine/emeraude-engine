/*
 * src/Saphir/DirStackFileIncluder.hpp
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
#include <vector>
#include <set>

/* Local inclusions for inheritance. */
#include "glslang/Public/ShaderLang.h"

namespace EmEn::Saphir
{
	/**
	 * @brief Default includes class for normal include convention of search backward
	 * through the stack of active include paths (for nested includes).
	 * Can be overridden to customize.
	 */
	class DirStackFileIncluder final : public glslang::TShader::Includer
	{
		public :

			/**
			 * @brief Constructs a default includer.
			 */
			DirStackFileIncluder () noexcept = default;

			/**
			 * @brief Destructs the includer.
			 */
			~DirStackFileIncluder () override = default;

			/**
			 * @brief Search for a valid "local" path based on combining the stack of include
			 * directories and the nominal name of the header.
			 * @param headerName
			 * @param includerName
			 * @param inclusionDepth
			 * @return IncludeResult *
			 */
			IncludeResult * includeLocal (const char * headerName, const char * includerName, size_t inclusionDepth) override;

			/**
			 * @brief includeSystem
			 * @param headerName
			 * @param includerName
			 * @param inclusionDepth
			 * @return IncludeResult *
			 */
			IncludeResult * includeSystem (const char * headerName, const char * includerName, size_t inclusionDepth) override;

			/**
			 * @brief releaseInclude
			 * @param result
			 */
			void releaseInclude (IncludeResult * result) override;

			/**
			 * @brief Externally set directories. E.g., from a command-line -I<dir>.
			 *  - Most-recently pushed are checked first.
			 *  - All these are checked after the parse-time stack of local directories is checked.
			 *  - This only applies to the "local" form of #include.
			 *  - Makes its own copy of the path.
			 * @param directory
			 */
			void pushExternalLocalDirectory (const std::string & directory) noexcept;

			/**
			 * @brief Returns included files.
			 * @return std::set< std::string > &
			 */
			[[nodiscard]]
			const std::set< std::string > & getIncludedFiles () const noexcept;

		private:

			typedef char tUserDataElement;

			/**
			 * @brief Do actual reading of the file, filling in a new include result.
			 * @param path
			 * @param file
			 * @param length
			 * @return IncludeResult *
			 */
			static IncludeResult * newIncludeResult (const std::string & path, std::ifstream & file, int length) noexcept;

			/**
			 * @brief If no path markers, return current working directory.
			 * Otherwise, strip file name and return path leading up to it.
			 * @param path
			 * @return std::string
			 */
			[[nodiscard]]
			static std::string getDirectory (const std::string & path) noexcept;

			std::vector< std::string > m_directoryStack{};
			std::set< std::string > m_includedFiles{};
			int m_externalLocalDirectoryCount = 0;
	};
}
