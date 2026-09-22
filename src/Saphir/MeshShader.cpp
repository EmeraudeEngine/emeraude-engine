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
#include <limits>
#include <vector>

/* Local inclusions. */
#include "Declaration/OutputBlock.hpp"
#include "Declaration/StageOutput.hpp"
#include "Declaration/Structure.hpp"
#include "Tracer.hpp"

namespace EmEn::Saphir
{
	using namespace Declaration;

	MeshShader::MeshShader (std::string name, std::string GLSLVersion, std::string GLSLProfile, MeshOutputTopology topology, uint32_t maxVertices, uint32_t maxPrimitives, const std::array< uint32_t, 3 > & workgroupSize) noexcept
		: AbstractVertexStage{std::move(name), std::move(GLSLVersion), std::move(GLSLProfile)},
		m_workgroupSize{workgroupSize},
		m_maxVertices{maxVertices},
		m_maxPrimitives{maxPrimitives},
		m_topology{topology}
	{
		this->setExtensionBehavior("GL_EXT_mesh_shader", "require");
		this->setPositionOutput(PositionOutput);
	}

	void
	MeshShader::setVertexSource (std::string countExpression, std::string prologue, std::set< Graphics::VertexAttributeType > providedAttributes) noexcept
	{
		m_vertexCountExpression = std::move(countExpression);
		m_vertexPrologue = std::move(prologue);
		m_providedAttributes = std::move(providedAttributes);
	}

	void
	MeshShader::setPrimitiveSource (std::string countExpression, std::string code) noexcept
	{
		m_primitiveCountExpression = std::move(countExpression);
		m_primitiveCode = std::move(code);
	}

	Key
	MeshShader::storeName (std::string name) noexcept
	{
		return m_generatedNames.emplace_back(std::move(name)).c_str();
	}

	bool
	MeshShader::onSourceCodeGeneration (Generator::Abstract & generator, std::stringstream & code, std::string & topInstructions, std::string & outputInstructions) noexcept
	{
		if ( m_maxVertices == 0 || m_maxPrimitives == 0 )
		{
			TraceError{ClassId} << "The mesh shader '" << this->name() << "' emits no vertex or no primitive !";

			return false;
		}

		if ( m_vertexCountExpression.empty() || m_primitiveCountExpression.empty() )
		{
			TraceError{ClassId} << "The mesh shader '" << this->name() << "' has no vertex or no primitive source !";

			return false;
		}

		/* The vertex shader modes that read what a mesh workgroup does not have: a per-draw BDA indexed by
		 * gl_DrawID, and per-instance attributes. */
		if ( this->isMDIEnabled() || this->isInstancingEnabled() || this->isBillBoardingEnabled() )
		{
			TraceError{ClassId} << "The mesh shader '" << this->name() << "' cannot run the MDI, instancing or billboard modes !";

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

		/* The shared per-vertex code: every synthetic variable the material, the lights and the shadows asked
		 * for, exactly as a vertex shader would generate it. */
		if ( !this->generatePerVertexCode(generator, code, topInstructions, outputInstructions) )
		{
			return false;
		}

		/* The attributes are PROVIDED by the vertex source, never fetched: one it does not provide would be an
		 * undeclared name in the GLSL — say which, here. */
		for ( const auto attribute : this->vertexAttributes() )
		{
			if ( !m_providedAttributes.contains(attribute) )
			{
				TraceError{ClassId} << "The mesh shader '" << this->name() << "' needs the vertex attribute '" << Graphics::to_cstring(attribute) << "', which its vertex source does not provide !";

				return false;
			}
		}

		const auto workgroupThreads = m_workgroupSize[0] * m_workgroupSize[1] * m_workgroupSize[2];

		std::stringstream frameTop;
		std::stringstream frameEnd;

		frameTop <<
			"\t" "SetMeshOutputsEXT(" << m_vertexCountExpression << ", " << m_primitiveCountExpression << ");" "\n\n"
			"\t" "for ( uint " << VertexIndex << " = gl_LocalInvocationIndex; " << VertexIndex << " < (" << m_vertexCountExpression << "); " << VertexIndex << " += " << workgroupThreads << "U )" "\n"
			"\t" "{" "\n" <<
			m_vertexPrologue << "\n"
			"\t" "vec4 " << PositionOutput << " = vec4(0.0, 0.0, 0.0, 1.0);" "\n";

		/* Per-vertex outputs: the synthesis writes locals of their canonical names, copied at the end of the
		 * iteration into arrays declared at the SAME locations, which is all the fragment stage links by. */
		std::vector< StageOutput > arrayOutputs;
		arrayOutputs.reserve(this->stageOutputs().size());

		for ( const auto & stageOutput : this->stageOutputs() )
		{
			const auto arrayName = this->storeName(std::string{stageOutput.name()} + "MS");

			arrayOutputs.emplace_back(stageOutput.location(), stageOutput.type(), arrayName, stageOutput.interpolation(), -1);

			frameTop << "\t" << stageOutput.type() << ' ' << stageOutput.name() << ";" "\n";
			frameEnd << "\t" << arrayName << '[' << VertexIndex << "] = " << stageOutput.name() << ";" "\n";
		}

		/* Output blocks (the light block of the light passes): a local of a structure with the same members,
		 * copied member by member into the block array. */
		std::vector< OutputBlock > arrayBlocks;
		arrayBlocks.reserve(this->outputBlocks().size());

		for ( const auto & outputBlock : this->outputBlocks() )
		{
			if ( !outputBlock.structureDeclaration().empty() )
			{
				TraceError{ClassId} << "The mesh shader '" << this->name() << "' cannot relay the structure members of the output block '" << outputBlock.name() << "' !";

				return false;
			}

			const auto instanceName = outputBlock.instanceName();
			const auto arrayInstance = this->storeName(instanceName + "MS");

			OutputBlock arrayBlock{outputBlock.name(), outputBlock.location(), arrayInstance, std::numeric_limits< uint32_t >::max()};
			Structure local{this->storeName(std::string{outputBlock.name()} + "Local")};

			for ( const auto & [memberName, member] : outputBlock.members() )
			{
				if ( member.arraySize() > 0 )
				{
					TraceError{ClassId} << "The mesh shader '" << this->name() << "' cannot relay the array member '" << memberName << "' of the output block '" << outputBlock.name() << "' !";

					return false;
				}

				if ( !arrayBlock.addMember(member.type(), member.name(), member.interpolation()) || !local.addMember(member.type(), member.name()) )
				{
					return false;
				}

				frameEnd << "\t" << arrayInstance << '[' << VertexIndex << "]." << member.name() << " = " << instanceName << '.' << member.name() << ";" "\n";
			}

			if ( !this->declare(local) )
			{
				return false;
			}

			frameTop << "\t" << local.name() << ' ' << instanceName << ";" "\n";

			arrayBlocks.emplace_back(arrayBlock);
		}

		frameTop << '\n';

		frameEnd <<
			"\t" "gl_MeshVerticesEXT[" << VertexIndex << "].gl_Position = " << PositionOutput << ";" "\n"
			"\t" "}" "\n\n"
			"\t" "for ( uint " << PrimitiveIndex << " = gl_LocalInvocationIndex; " << PrimitiveIndex << " < (" << m_primitiveCountExpression << "); " << PrimitiveIndex << " += " << workgroupThreads << "U )" "\n"
			"\t" "{" "\n" <<
			m_primitiveCode << "\n"
			"\t" "}" "\n";

		topInstructions.insert(0, frameTop.str());

		this->setMainEpilogue(frameEnd.str());

		generateDeclarations(code, arrayOutputs, "Stage outputs (per vertex, to the fragment stage)");
		generateDeclarations(code, arrayBlocks, "Output blocks (per vertex, to the fragment stage)");

		return true;
	}

	void
	MeshShader::onGetDeclarationStats (std::stringstream & output) const noexcept
	{
		output <<
			"Mesh shader output : " << m_maxVertices << " vertices, " << m_maxPrimitives << " primitives, workgroup " <<
			m_workgroupSize[0] << " x " << m_workgroupSize[1] << " x " << m_workgroupSize[2] << "\n";

		AbstractVertexStage::onGetDeclarationStats(output);
	}
}
