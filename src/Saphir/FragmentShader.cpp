/*
 * src/Saphir/FragmentShader.cpp
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

#include "FragmentShader.hpp"

/* STL inclusions. */
#include <algorithm>
#include <array>
#include <cstring>

/* Local inclusions. */
#include "GeometryShader.hpp"
#include "MeshShader.hpp"
#include "Graphics/Geometry/HeightfieldSurface.hpp"
#include "TesselationEvaluationShader.hpp"
#include "Tracer.hpp"
#include "VertexShader.hpp"

namespace
{
	/** @brief A frame variable the heightfield rebuilds per pixel: its canonical name, the name the
	 * interpolated vertex value is received under, and the GLSL rebuilding it from the pixel's
	 * T/B/N (object space) and the flat rotations of the vertex stage. */
	struct HeightfieldFrameOverride
	{
		const char * canonical;
		const char * received;
		const char * type;
		const char * definition;
	};

	/* NOTE: Each definition is the vertex stage's own formula (VertexShader::synthesize*()), applied
	 * to the pixel's vectors: world vectors = model matrix · v (not normalized), view vectors =
	 * normalize(normal matrix · v), and TangentToWorldMatrix = normal matrix · mat3(T, B, N). */
	constexpr std::array< HeightfieldFrameOverride, 9 > HeightfieldFrameOverrides{{
		{EmEn::Saphir::Keys::ShaderVariable::NormalWorldSpace, "hfvNormalWorldSpace", EmEn::Saphir::Keys::GLSL::FloatVector3, "hfPixelToWorld * hfPixelNormal"},
		{EmEn::Saphir::Keys::ShaderVariable::TangentWorldSpace, "hfvTangentWorldSpace", EmEn::Saphir::Keys::GLSL::FloatVector3, "hfPixelToWorld * hfPixelTangent"},
		{EmEn::Saphir::Keys::ShaderVariable::BinormalWorldSpace, "hfvBinormalWorldSpace", EmEn::Saphir::Keys::GLSL::FloatVector3, "hfPixelToWorld * hfPixelBinormal"},
		{EmEn::Saphir::Keys::ShaderVariable::NormalViewSpace, "hfvNormalViewSpace", EmEn::Saphir::Keys::GLSL::FloatVector3, "normalize(hfPixelToView * hfPixelNormal)"},
		{EmEn::Saphir::Keys::ShaderVariable::TangentViewSpace, "hfvTangentViewSpace", EmEn::Saphir::Keys::GLSL::FloatVector3, "normalize(hfPixelToView * hfPixelTangent)"},
		{EmEn::Saphir::Keys::ShaderVariable::BinormalViewSpace, "hfvBinormalViewSpace", EmEn::Saphir::Keys::GLSL::FloatVector3, "normalize(hfPixelToView * hfPixelBinormal)"},
		{EmEn::Saphir::Keys::ShaderVariable::WorldTBNMatrix, "hfvWorldTBNMatrix", EmEn::Saphir::Keys::GLSL::Matrix3, "mat3(normalize(hfPixelToWorld * hfPixelTangent), normalize(hfPixelToWorld * hfPixelBinormal), normalize(hfPixelToWorld * hfPixelNormal))"},
		{EmEn::Saphir::Keys::ShaderVariable::ViewTBNMatrix, "hfvViewTBNMatrix", EmEn::Saphir::Keys::GLSL::Matrix3, "transpose(mat3(normalize(hfPixelToView * hfPixelTangent), normalize(hfPixelToView * hfPixelBinormal), normalize(hfPixelToView * hfPixelNormal)))"},
		{EmEn::Saphir::Keys::ShaderVariable::TangentToWorldMatrix, "hfvTangentToWorldMatrix", EmEn::Saphir::Keys::GLSL::Matrix3, "hfPixelToView * mat3(hfPixelTangent, hfPixelBinormal, hfPixelNormal)"}
	}};
}

namespace EmEn::Saphir
{
	using namespace Base;
	using namespace Saphir::Declaration;
	using namespace Saphir::Keys;
	using namespace Graphics;

	bool
	FragmentShader::declare (const StageInput & declaration) noexcept
	{
		if ( !declaration.isValid() )
		{
			Tracer::error(ClassId, "The stage input is invalid for code generation !");

			return false;
		}

		if ( std::ranges::any_of(m_stageInputs, [&declaration] (const auto & existing) {return existing.name() == declaration.name();}) )
		{
			TraceWarning{ClassId} << "A stage input declaration named '" << declaration.name() << "' already exists !";

			return true;
		}

		m_stageInputs.emplace_back(declaration);

		return true;
	}

	bool
	FragmentShader::declare (const InputBlock & declaration) noexcept
	{
		if ( !declaration.isValid() )
		{
			Tracer::error(ClassId, "The input block is invalid for code generation !");

			return false;
		}

		if ( std::ranges::any_of(m_inputBlocks, [&declaration] (const auto & existing) {return existing.instanceName() == declaration.instanceName();}) )
		{
			TraceWarning{ClassId} << "An input block declaration named '" << declaration.name() << "' already exists !";

			return true;
		}

		m_inputBlocks.emplace_back(declaration);

		return true;
	}

	bool
	FragmentShader::declare(const OutputFragment & declaration) noexcept
	{
		if ( !declaration.isValid() )
		{
			Tracer::error(ClassId, "The output fragment is invalid for code generation !");

			return false;
		}

		if ( std::ranges::any_of(m_outputFragments, [&declaration] (const auto & existing) {return existing.name() == declaration.name();}) )
		{
			TraceWarning{ClassId} << "An output fragment declaration named '" << declaration.name() << "' already exists !";

			return true;
		}

		m_outputFragments.emplace_back(declaration);

		return true;
	}

	bool
	FragmentShader::connectFromPreviousShader (const VertexShader & vertexShader) noexcept
	{
		if ( !vertexShader.isGenerated() )
		{
			TraceError{ClassId} << "The vertex shader '" << vertexShader.name() << "' is not generated !";

			return false;
		}

		if ( vertexShader.stageOutputs().empty() && vertexShader.outputBlocks().empty() )
		{
			/* NOTE : This can only have gl_Position. */
			//TraceWarning{ClassId} << "The vertex shader '" << vertexShader.name() << "' has no output declaration !";

			return true;
		}

		for ( const auto & stageOutput : vertexShader.stageOutputs() )
		{
			if ( m_heightfieldPixelFrameEnabled )
			{
				const auto * override = std::ranges::find_if(HeightfieldFrameOverrides, [&stageOutput] (const auto & entry) {
					return std::strcmp(entry.canonical, stageOutput.name()) == 0;
				});

				if ( override != HeightfieldFrameOverrides.end() )
				{
					/* Same location, same type: the interpolated value is still received (the interface
					 * must match), under a name nothing reads. */
					this->declare(StageInput{stageOutput.location(), stageOutput.type(), override->received, stageOutput.interpolation(), stageOutput.arraySize()});

					m_heightfieldOverrides.emplace_back(override->canonical);

					continue;
				}
			}

			this->declare(StageInput{stageOutput});
		}

		for ( const auto & outputBlock : vertexShader.outputBlocks() )
		{
			this->declare(InputBlock{outputBlock});
		}

		return true;
	}

	bool
	FragmentShader::connectFromPreviousShader (const TesselationEvaluationShader & tesselationEvaluationShader) noexcept
	{
		if ( !tesselationEvaluationShader.isGenerated() )
		{
			TraceError{ClassId} << "The tesselation evaluation shader '" << tesselationEvaluationShader.name() << "' is not generated !";

			return false;
		}

		if ( tesselationEvaluationShader.stageOutputs().empty() && tesselationEvaluationShader.outputBlocks().empty() )
		{
			TraceWarning{ClassId} << "The tesselation evaluation shader '" << tesselationEvaluationShader.name() << "' has no output declaration !";

			return false;
		}

		for ( const auto & stageOutput : tesselationEvaluationShader.stageOutputs() )
		{
			this->declare(StageInput{stageOutput});
		}

		for ( const auto & outputBlock : tesselationEvaluationShader.outputBlocks() )
		{
			this->declare(InputBlock{outputBlock});
		}

		return true;
	}

	bool
	FragmentShader::connectFromPreviousShader (const MeshShader & meshShader) noexcept
	{
		if ( !meshShader.isGenerated() )
		{
			TraceError{ClassId} << "The mesh shader '" << meshShader.name() << "' is not generated !";

			return false;
		}

		for ( const auto & stageOutput : meshShader.stageOutputs() )
		{
			/* The array is the mesh stage's (one element per vertex or primitive); the fragment stage
			 * receives one interpolated value. */
			this->declare(StageInput{stageOutput.location(), stageOutput.type(), stageOutput.name(), stageOutput.interpolation(), 0});
		}

		return true;
	}

	bool
	FragmentShader::connectFromPreviousShader (const GeometryShader & geometryShader) noexcept
	{
		if ( !geometryShader.isGenerated() )
		{
			TraceError{ClassId} << "The geometry shader '" << geometryShader.name() << "' is not generated !";

			return false;
		}

		if ( geometryShader.stageOutputs().empty() && geometryShader.outputBlocks().empty() )
		{
			TraceWarning{ClassId} << "The geometry shader '" << geometryShader.name() << "' has no output declaration !";

			return false;
		}

		for ( const auto & stageOutput : geometryShader.stageOutputs() )
		{
			this->declare(StageInput{stageOutput});
		}

		for ( const auto & outputBlock : geometryShader.outputBlocks() )
		{
			this->declare(InputBlock{outputBlock});
		}

		return true;
	}

	bool
	FragmentShader::onSourceCodeGeneration (Generator::Abstract & /*generator*/, std::stringstream & code, std::string & topInstructions, std::string & /*outputInstructions*/) noexcept
	{
		/* The surface frame of THIS pixel, first thing in main(): every canonical frame variable the
		 * vertex stage handed over is defined here, from the normal clipmap. */
		if ( m_heightfieldPixelFrameEnabled )
		{
			std::string prelude =
				"\t" "/* Heightfield surface: the frame of this pixel, from the normal clipmap. */" "\n"
				"\t" "const vec3 hfPixelNormal = hfPixelNormalAt(" + std::string{Geometry::HeightfieldSurface::PixelPositionVarying} + ");" "\n"
				"\t" "const vec3 hfPixelTangent = normalize(vec3(hfPixelNormal.y, -hfPixelNormal.x, 0.0));" "\n"
				"\t" "const vec3 hfPixelBinormal = cross(hfPixelNormal, hfPixelTangent);" "\n";

			for ( const auto * canonical : m_heightfieldOverrides )
			{
				const auto * override = std::ranges::find_if(HeightfieldFrameOverrides, [canonical] (const auto & entry) {
					return std::strcmp(entry.canonical, canonical) == 0;
				});

				prelude += "\t" "const " + std::string{override->type} + " " + canonical + " = " + override->definition + ";" "\n";
			}

			topInstructions.insert(0, prelude + "\n");
		}

		/* Specific input shader code declarations. */
		AbstractShader::generateDeclarations(code, m_stageInputs, "Stage inputs (From previous stage)");
		AbstractShader::generateDeclarations(code, m_inputBlocks, "Input blocks (From previous stage)");

		/* Specific output shader code declarations. */
		AbstractShader::generateDeclarations(code, m_outputFragments, "Output fragments (Fragment shader only)");

		return true;
	}

	void
	FragmentShader::onGetDeclarationStats (std::stringstream & output) const noexcept
	{
		output <<
		 	"Fragment shader input declarations : " "\n"
			" - Stage input : " << m_stageInputs.size() << "\n"
			" - Input block : " << m_inputBlocks.size() << "\n"

			"Vertex shader output declarations : " "\n"
			" - Output fragment (FS) : " << m_outputFragments.size() << "\n";
	}

	Function
	FragmentShader::generateToSRGBColorFunction () noexcept
	{
		Function function{"toSRGBColor", GLSL::FloatVector4};
		function.addInParameter(GLSL::FloatVector4, "linearRGB", true);

		/* NOTE: Only convert RGB components, alpha channel is always linear. */
		function.addInstruction(
			"\t" "bvec3 cutoff = lessThan(linearRGB.rgb, vec3(0.0031308));" "\n"
			"\t" "vec3 higher = vec3(1.055) * pow(linearRGB.rgb, vec3(1.0 / 2.4)) - vec3(0.055);" "\n"
			"\t" "vec3 lower = linearRGB.rgb * vec3(12.92);" "\n\n"

			"\t" "return vec4(mix(higher, lower, cutoff), linearRGB.a);" "\n"
		);

		return function;
	}

	Function
	FragmentShader::generateToLinearColorFunction () noexcept
	{
		Function function{"toLinearColor", GLSL::FloatVector4};
		function.addInParameter(GLSL::FloatVector4, "sRGB", true);

		/* NOTE: Only convert RGB components, alpha channel is always linear. */
		function.addInstruction(
			"\t" "bvec3 cutoff = lessThan(sRGB.rgb, vec3(0.04045));" "\n"
			"\t" "vec3 higher = pow((sRGB.rgb + vec3(0.055)) / vec3(1.055), vec3(2.4));" "\n"
			"\t" "vec3 lower = sRGB.rgb / vec3(12.92);" "\n\n"

			"\t" "return vec4(mix(higher, lower, cutoff), sRGB.a);" "\n"
		);

		return function;
	}
}
