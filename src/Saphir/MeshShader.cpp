/*
 * src/Saphir/MeshShader.cpp
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

#include "MeshShader.hpp"

/* STL inclusions. */
#include <algorithm>

/* Local inclusions. */
#include "Tracer.hpp"

namespace EmEn::Saphir
{
	using namespace Declaration;

	MeshShader::MeshShader (std::string name, std::string GLSLVersion, std::string GLSLProfile, MeshOutputTopology topology, uint32_t maxVertices, uint32_t maxPrimitives, const std::array< uint32_t, 3 > & workgroupSize) noexcept
		: AbstractShader{std::move(name), std::move(GLSLVersion), std::move(GLSLProfile)},
		m_workgroupSize{workgroupSize},
		m_maxVertices{maxVertices},
		m_maxPrimitives{maxPrimitives},
		m_topology{topology}
	{
		this->setExtensionBehavior("GL_EXT_mesh_shader", "require");
	}

	bool
	MeshShader::declare (const StageOutput & declaration) noexcept
	{
		if ( !declaration.isValid() )
		{
			Tracer::error(ClassId, "The stage output is invalid for code generation !");

			return false;
		}

		if ( std::ranges::any_of(m_stageOutputs, [&declaration] (const auto & existing) {return existing.name() == declaration.name();}) )
		{
			TraceWarning{ClassId} << "A stage output declaration named '" << declaration.name() << "' already exists !";

			return true;
		}

		m_stageOutputs.emplace_back(declaration);

		return true;
	}

	bool
	MeshShader::onSourceCodeGeneration (Generator::Abstract & /*generator*/, std::stringstream & code, std::string & /*topInstructions*/, std::string & /*outputInstructions*/) noexcept
	{
		if ( m_maxVertices == 0 || m_maxPrimitives == 0 )
		{
			TraceError{ClassId} << "The mesh shader '" << this->name() << "' emits no vertex or no primitive !";

			return false;
		}

		const char * topology = "triangles";

		switch ( m_topology )
		{
			case MeshOutputTopology::Points :
				topology = "points";
				break;

			case MeshOutputTopology::Lines :
				topology = "lines";
				break;

			case MeshOutputTopology::Triangles :
				topology = "triangles";
				break;
		}

		code <<
			"/* Mesh workgroup and output */" "\n"
			"layout(local_size_x = " << m_workgroupSize[0] << ", local_size_y = " << m_workgroupSize[1] << ", local_size_z = " << m_workgroupSize[2] << ") in;" "\n"
			"layout(" << topology << ", max_vertices = " << m_maxVertices << ", max_primitives = " << m_maxPrimitives << ") out;" "\n\n";

		if ( !m_taskPayload.empty() )
		{
			code << "/* Task payload (from the task stage) */" "\n" << m_taskPayload << "\n\n";
		}

		AbstractShader::generateDeclarations(code, m_stageOutputs, "Stage outputs (To next stage)");

		return true;
	}

	void
	MeshShader::onGetDeclarationStats (std::stringstream & output) const noexcept
	{
		output <<
			"Mesh shader output : " << m_maxVertices << " vertices, " << m_maxPrimitives << " primitives, workgroup " <<
			m_workgroupSize[0] << " x " << m_workgroupSize[1] << " x " << m_workgroupSize[2] << "\n"
			" - Stage output : " << m_stageOutputs.size() << "\n";
	}
}
