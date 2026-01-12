/*
 * src/Saphir/DirStackFileIncluder.cpp
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

#include "DirStackFileIncluder.hpp"

/* STL inclusions. */
#include <fstream>
#include <algorithm>

namespace EmEn::Saphir
{
	using namespace glslang;

	TShader::Includer::IncludeResult *
	DirStackFileIncluder::includeLocal (const char * headerName, const char * includerName, size_t inclusionDepth)
	{
		// Discard popped include directories, and
		// initialize when at parse-time first level.
		m_directoryStack.resize(inclusionDepth + m_externalLocalDirectoryCount);

		if ( inclusionDepth == 1 )
			m_directoryStack.back() = EmEn::Saphir::DirStackFileIncluder::getDirectory(includerName);

		// Find a directory that works, using a reverse search of the include stack.
		for ( auto it = m_directoryStack.rbegin(); it != m_directoryStack.rend(); ++it )
		{
			std::string path = *it + '/' + headerName;
			std::replace(path.begin(), path.end(), '\\', '/');
			std::ifstream file(path, std::ios_base::binary | std::ios_base::ate);

			if ( file )
			{
				m_directoryStack.push_back(EmEn::Saphir::DirStackFileIncluder::getDirectory(path));
				m_includedFiles.insert(path);

				return EmEn::Saphir::DirStackFileIncluder::newIncludeResult(path, file, (int)file.tellg());
			}
		}

		return nullptr;
	}

	TShader::Includer::IncludeResult *
	DirStackFileIncluder::includeSystem (const char *, const char *, size_t)
	{
		// Search for a valid <system> path.
		// Not implemented yet; returning nullptr signals failure to find.
		return nullptr;
	}

	void
	DirStackFileIncluder::releaseInclude (IncludeResult * result)
	{
		if ( result != nullptr )
		{
			delete [] static_cast< tUserDataElement * >(result->userData);
			delete result;
		}
	}

	void
	DirStackFileIncluder::pushExternalLocalDirectory (const std::string & directory) noexcept
	{
		m_directoryStack.push_back(directory);
		m_externalLocalDirectoryCount = static_cast< int >(m_directoryStack.size());
	}

	const std::set< std::string > &
	DirStackFileIncluder::getIncludedFiles () const noexcept
	{
		return m_includedFiles;
	}

	TShader::Includer::IncludeResult *
	DirStackFileIncluder::newIncludeResult (const std::string & path, std::ifstream & file, int length) noexcept
	{
		char * content = new tUserDataElement [length];
		file.seekg(0, std::ifstream::beg);
		file.read(content, length);

		return new IncludeResult(path, content, length, content);
	}

	std::string
	DirStackFileIncluder::getDirectory (const std::string & path) noexcept
	{
		size_t last = path.find_last_of("/\\");

		return last == std::string::npos ? "." : path.substr(0, last);
	}
}
