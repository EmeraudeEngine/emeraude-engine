/*
 * src/Saphir/TaskShader.cpp
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
 * https://github.com/EmeraudeEngine/emeraude-engine
 *
 * --- THIS IS AUTOMATICALLY GENERATED, DO NOT CHANGE ---
 */

#include "TaskShader.hpp"

namespace EmEn::Saphir
{
	TaskShader::TaskShader (std::string name, std::string GLSLVersion, std::string GLSLProfile, const std::array< uint32_t, 3 > & workgroupSize) noexcept
		: AbstractShader{std::move(name), std::move(GLSLVersion), std::move(GLSLProfile)},
		m_workgroupSize{workgroupSize}
	{
		this->setExtensionBehavior("GL_EXT_mesh_shader", "require");
	}

	bool
	TaskShader::onSourceCodeGeneration (Generator::Abstract & /*generator*/, std::stringstream & code, std::string & /*topInstructions*/, std::string & /*outputInstructions*/) noexcept
	{
		code <<
			"/* Task workgroup */" "\n"
			"layout(local_size_x = " << m_workgroupSize[0] << ", local_size_y = " << m_workgroupSize[1] << ", local_size_z = " << m_workgroupSize[2] << ") in;" "\n\n";

		if ( !m_taskPayload.empty() )
		{
			code << "/* Task payload (shared with the mesh stage) */" "\n" << m_taskPayload << "\n\n";
		}

		return true;
	}

	void
	TaskShader::onGetDeclarationStats (std::stringstream & output) const noexcept
	{
		output << "Task shader workgroup : " << m_workgroupSize[0] << " x " << m_workgroupSize[1] << " x " << m_workgroupSize[2] << "\n";
	}
}
