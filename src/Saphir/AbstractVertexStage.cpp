/*
 * src/Saphir/AbstractVertexStage.cpp
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

#include "AbstractVertexStage.hpp"

/* STL inclusions. */
#include <algorithm>
#include <array>
#include <cstring>
#include <ranges>
#include <set>
#include <sstream>
#include <string>
#include <vector>

/* Local inclusions. */
#include "AbstractShader.hpp"
#include "Code.hpp"
#include "Declaration/InputAttribute.hpp"
#include "Declaration/OutputBlock.hpp"
#include "Declaration/StageOutput.hpp"
#include "Generator/Abstract.hpp"
#include "Graphics/Geometry/HeightfieldSurface.hpp"
#include "Graphics/Types.hpp"
#include "Keys.hpp"
#include "Tracer.hpp"
#include "Types.hpp"

namespace EmEn::Saphir
{
	using namespace Base;
	using namespace Saphir::Declaration;
	using namespace Saphir::Keys;
	using namespace Graphics;

	Function
	AbstractVertexStage::generateComputeUpwardVectorFunction () noexcept
	{
		std::stringstream functionCode;

		functionCode <<
			// Special case when backward is equal to downward (0, 1, 0).
			"\t" "if ( backward == vec3(0.0, 1.0, 0.0) )" "\n"
			"\t" "{" "\n"
			"\t\t" "return vec3(0.0, 0.0, 1.0);" "\n"
			"\t" "}" "\n\n"

			// Special case when backward is equal to upward (0, -1, 0).
			"\t" "if ( backward == vec3(0.0, -1.0, 0.0) )" "\n"
			"\t" "{" "\n"
			"\t\t" "return vec3(0.0, 0.0, -1.0);" "\n"
			"\t" "}" "\n\n"

			"\t" "vec3 downward;" "\n\n"

			// Compute temporal downward vector based on the backward
			// vector watch out when look up/down at 90 degree for
			// example, backward vector is on the Y axis.
			"\t" "if ( abs(backward.x) < 0.0 && abs(backward.z) < 0.0 )" "\n"
			"\t" "{" "\n"
			// If backward vector is pointing on +Y axis.
			"\t\t" "downward = backward.y > 0.0 ? vec3(0.0, 0.0, -1.0) : vec3(0.0, 0.0, 1.0);" "\n"
			"\t" "}" "\n"
			"\t" "else" "\n"
			"\t" "{" "\n"
			// In general, downward vector is straight down.
			"\t\t" "downward = vec3(0.0, 1.0, 0.0);" "\n"
			"\t" "}" "\n\n"

			// Re-calculate the orthonormal downward vector with right vector.
			"\t" "const vec3 right = normalize(cross(downward, backward));" "\n\n"

			"\t" "return cross(backward, right);" "\n";

		Function function{"computeYAxis", GLSL::FloatVector3};
		function.addInParameter(GLSL::FloatVector3, "backward");
		function.addInstruction(functionCode.str());

		return function;
	}

	Function
	AbstractVertexStage::generateGetBillBoardModelMatrixFunction () noexcept
	{
		std::stringstream functionCode;

		functionCode <<
			"\t" "const vec3 backward = normalize(cameraPosition - modelPosition);" "\n"
			"\t" "const vec3 downward = computeYAxis(backward);" "\n"
			"\t" "const vec3 right = cross(downward, backward);" "\n"
			"\t" "const mat4 scalingMatrix = mat4(" "\n"
			"\t\t" "vec4(modelScaling.x, 0.0, 0.0, 0.0), " "\n"
			"\t\t" "vec4(0.0, modelScaling.y, 0.0, 0.0), " "\n"
			"\t\t" "vec4(0.0, 0.0, modelScaling.z, 0.0), " "\n"
			"\t\t" "vec4(0.0, 0.0, 0.0, 1.0)" "\n" // TODO: Try to make it work with uniform scaling 1.0/UScale
			"\t" ");" "\n\n"

			"\t" "return mat4(vec4(right, 0.0), vec4(downward.xyz, 0.0), vec4(backward, 0.0), vec4(modelPosition, 1.0)) * scalingMatrix;" "\n";

		Function function{"getBillBoardModelMatrix", GLSL::Matrix4};
		function.addInParameter(GLSL::ConstFloatVector3, "cameraPosition");
		function.addInParameter(GLSL::ConstFloatVector3, "modelPosition");
		function.addInParameter(GLSL::ConstFloatVector3, "modelScaling");
		function.addInstruction(functionCode.str());

		return function;
	}

	bool
	AbstractVertexStage::declare (const InputAttribute & declaration) noexcept
	{
		if ( !declaration.isValid() )
		{
			Tracer::error(ClassId, "The input attribute is invalid for code generation !");

			return false;
		}

		if ( !this->isInstancingEnabled() && declaration.isModelMatricesVBOAttribute() )
		{
			TraceError{ClassId} << "This is invalid to add a matrix attribute with non instancing vertex shader !";

			return false;
		}

		/* Silent de-duplication: the attribute name/location/type are all derived
		 * from the VertexAttributeType, so a re-declaration is byte-identical and
		 * safely ignored. Composable generators declare what they consume without
		 * coordinating and rely on this — no warning, no cost beyond the lookup. */
		if ( std::ranges::any_of(m_inputAttributes, [&declaration] (const auto & existing) {return existing.name() == declaration.name();}) )
		{
			return true;
		}

		m_inputAttributes.emplace_back(declaration);

		m_vertexAttributes.emplace(static_cast< VertexAttributeType >(declaration.location()));

		return true;
	}

	bool
	AbstractVertexStage::declare (const StageOutput & declaration) noexcept
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
	AbstractVertexStage::declare (const OutputBlock & declaration) noexcept
	{
		if ( !declaration.isValid() )
		{
			Tracer::error(ClassId, "The output block is invalid for code generation !");

			return false;
		}

		if ( std::ranges::any_of(m_outputBlocks, [&declaration] (const auto & existing) {return existing.instanceName() == declaration.instanceName();}) )
		{
			TraceWarning{ClassId} << "An output block declaration named '" << declaration.name() << "' already exists !";

			return true;
		}

		m_outputBlocks.emplace_back(declaration);

		return true;
	}

	bool
	AbstractVertexStage::preparationAlreadyDone (const char * preparation) const noexcept
	{
		/*return std::ranges::any_of(m_uniquePreparations, [preparation] (const auto & pair) {
			return std::strcmp(pair.first, preparation) == 0;
		});*/

		/* TODO: Check if the code above is right ! */
		for ( const auto & variableType : std::ranges::views::keys(m_uniquePreparations) )
		{
			if ( std::strcmp(variableType, preparation) == 0 )
			{
				return true;
			}
		}

		return false;
	}

	bool
	AbstractVertexStage::prepareSpriteModelMatrix () noexcept
	{
		if ( this->preparationAlreadyDone(ShaderVariable::SpriteModelMatrix) )
		{
			return true;
		}

		if ( !this->isBillBoardingEnabled() )
		{
			Tracer::error(ClassId, "The billBoarding is not enabled !");

			return false;
		}

		std::stringstream code;

		if ( !this->declare(InputAttribute{VertexAttributeType::ModelPosition}) )
		{
			return false;
		}

		if ( !this->declare(InputAttribute{VertexAttributeType::ModelScaling}) )
		{
			return false;
		}

		this->declare(AbstractVertexStage::generateComputeUpwardVectorFunction());
		this->declare(AbstractVertexStage::generateGetBillBoardModelMatrixFunction());

		/* NOTE: In cubemap mode, the view matrix comes from the UBO indexed by gl_ViewIndex,
		 * not from the push constant. */
		const auto viewMatrixSource = this->isCubemapModeEnabled() ?
			ViewUB(Keys::UniformBlock::Component::ViewMatrix, true) :
			MatrixPC(PushConstant::Component::ViewMatrix);

		/* TODO: Find a way to get the camera world position directly (View UBO is not constantly updated for now) */
		code <<
			"\t" "const mat4 InvView = inverse(" << viewMatrixSource << ");" "\n"
			"\t" "const mat4 " << ShaderVariable::SpriteModelMatrix << " = getBillBoardModelMatrix(InvView[3].xyz, " << Attribute::ModelPosition << ", " << Attribute::ModelScaling << ");" "\n\n";

		m_uniquePreparations.emplace_back(ShaderVariable::SpriteModelMatrix, code.str());

		return true;
	}

	bool
	AbstractVertexStage::prepareMDIModelMatrix () noexcept
	{
		if ( this->preparationAlreadyDone(ShaderVariable::MDIModelMatrix) )
		{
			return true;
		}

		/* NOTE: Extensions are registered in onSourceCodeGeneration() to ensure
		 * they appear before the PerDrawDataRef declaration in the generated GLSL. */

		std::stringstream code;

		code <<
			"\t" "const uint64_t perDrawAddr = packUint2x32(uvec2(" << MatrixPC(PushConstant::Component::PerDrawAddrLo) << ", " << MatrixPC(PushConstant::Component::PerDrawAddrHi) << "));" "\n"
			"\t" "const mat4 " << ShaderVariable::MDIModelMatrix << " = mat4(PerDrawDataRef(perDrawAddr)[gl_DrawID].modelMatrix);" "\n\n";

		m_uniquePreparations.emplace_back(ShaderVariable::MDIModelMatrix, code.str());

		return true;
	}

	bool
	AbstractVertexStage::prepareInstanceModelMatrix () noexcept
	{
		if ( this->preparationAlreadyDone(ShaderVariable::InstanceModelMatrix) )
		{
			return true;
		}

		std::stringstream code;

		/* NOTE: The InstanceTransforms SSBO interleaves {model, previousModel} matrices
		 * (stride 2). The slot is encoded in the firstInstance draw parameter: with
		 * instanceCount always 1 on this path, gl_InstanceIndex == firstInstance — no
		 * shaderDrawParameters feature required (contrary to gl_BaseInstance). */
		code << "\t" "const mat4 " << ShaderVariable::InstanceModelMatrix << " = ubInstanceTransforms.instanceMatrices[" << m_instanceIndexExpression << " * 2];" "\n\n";

		m_uniquePreparations.emplace_back(ShaderVariable::InstanceModelMatrix, code.str());

		return true;
	}

	bool
	AbstractVertexStage::synthesizeVelocityClipPositions (Generator::Abstract & generator, bool & emitted) noexcept
	{
		emitted = false;

		/* NOTE: MDI has no previous matrix source; cubemap/CSM targets never carry a
		 * velocity attachment. The fragment shader writes a zero velocity instead. */
		if ( this->isMDIEnabled() || this->isCubemapModeEnabled() || this->isCSMModeEnabled() )
		{
			return true;
		}

		/* Previous model matrix source — resolved FIRST: the push-constant fallback path
		 * has no source and must bail out before any declaration/preparation happens. */
		std::string previousModelMatrix;

		if ( this->isInstancingEnabled() )
		{
			if ( this->isBillBoardingEnabled() )
			{
				/* NOTE: Billboards face the CURRENT camera — the previous sprite matrix is
				 * unknown, fall back to the current one (camera-only velocity, assumed limit). */
				if ( !this->prepareSpriteModelMatrix() )
				{
					return false;
				}

				previousModelMatrix = ShaderVariable::SpriteModelMatrix;
			}
			else if ( this->isInstanceMotionHistoryEnabled() )
			{
				if ( !this->declare(InputAttribute{VertexAttributeType::PreviousModelMatrixR0}) )
				{
					return false;
				}

				previousModelMatrix = Attribute::PreviousModelMatrix;
			}
			else
			{
				/* NOTE: Instanced without motion history: static instances, the current
				 * model matrix IS the previous one (camera-only velocity for movers). */
				if ( !this->declare(InputAttribute{VertexAttributeType::ModelMatrixR0}) )
				{
					return false;
				}

				previousModelMatrix = Attribute::ModelMatrix;
			}
		}
		else if ( this->isInstanceTransformsEnabled() )
		{
			/* Non-instanced: the previous model matrix lives in the SSBO entry (odd index). */
			previousModelMatrix = "ubInstanceTransforms.instanceMatrices[" + std::string{m_instanceIndexExpression} + " * 2 + 1]";
		}
		else
		{
			/* NOTE: Push-constant fallback path (instance transforms unavailable):
			 * no previous matrix source. */
			return true;
		}

		if ( !this->declare(StageOutput{generator.getNextShaderVariableLocation(), GLSL::FloatVector4, ShaderVariable::ClipPositionCurrent, GLSL::Smooth}) )
		{
			return false;
		}

		if ( !this->declare(StageOutput{generator.getNextShaderVariableLocation(), GLSL::FloatVector4, ShaderVariable::ClipPositionPrevious, GLSL::Smooth}) )
		{
			return false;
		}

		/* Current clip position: the same MVP as gl_Position (recomputed to stay
		 * independent from the output instruction ordering). */
		if ( !this->prepareModelViewProjectionMatrix() )
		{
			return false;
		}

		/* Double skinning: the previous clip position uses the PREVIOUS pose (the skinning
		 * SSBO interleaves {current, previous} bone matrices) — limb motion produces real
		 * velocity, not just the transform delta. */
		const auto posExpr = this->vertexPositionExpression();
		const auto prevPosExpr = this->previousVertexPositionExpression();

		if ( m_skinningEnabled )
		{
			m_previousSkinningRequired = true;
		}

		/* NOTE: The wind needs its previous displacement for the SAME reason skinning needs its
		 * previous pose: without it a swaying vertex reports zero velocity and smears under TAA. */
		if ( m_vegetationWindEnabled )
		{
			m_previousWindRequired = true;
		}

		Code{*this, Location::Output} << ShaderVariable::ClipPositionCurrent << " = " << ShaderVariable::ModelViewProjectionMatrix << " * vec4(" << posExpr << ", 1.0);";
		/* The INFINITY view drops the camera translation: an object rendered with it (the sky
		 * background) must have its previous clip position built from the previous INFINITY
		 * view-projection. Mixing the two is a STRUCTURAL mismatch — it does not cancel on a
		 * static camera, and produced a smooth NDC-position-like velocity gradient of up to
		 * ~0.3 NDC over the whole sky (measured 2026-07-25 with a boolean velocity probe). */
		const auto * previousViewProjection = m_infinityViewEnabled ?
			"ubInstanceTransforms.previousViewProjectionInfinity" :
			"ubInstanceTransforms.previousViewProjection";

		Code{*this, Location::Output} << ShaderVariable::ClipPositionPrevious << " = " << previousViewProjection << " * " << previousModelMatrix << " * vec4(" << prevPosExpr << ", 1.0);";

		/* NOTE: Both clip positions are jitter-free BY CONSTRUCTION — no matrix ever carries
		 * the TAA sub-pixel jitter (neither the view UBO projection, nor the pushed view
		 * projection, nor the SSBO header). The jitter is a per-draw push constant applied to
		 * gl_Position alone (synthesizeVertexPositionInScreenSpace), so nothing has to be
		 * subtracted back here. Do NOT reintroduce a jitter term: the previous version
		 * subtracted a jitter read per-frame from the SSBO while the raster used the jitter
		 * baked in the SINGLE-buffered view UBO, and that mismatch produced a constant
		 * ~1-2 px bogus velocity on a static camera. */

		emitted = true;

		return true;
	}

	bool
	AbstractVertexStage::prepareModelViewMatrix () noexcept
	{
		if ( this->preparationAlreadyDone(ShaderVariable::ModelViewMatrix) )
		{
			return true;
		}

		std::stringstream code;

		/* NOTE: In cubemap mode, the view matrix comes from the UBO indexed by gl_ViewIndex,
		 * not from the push constant. */
		if ( this->isCubemapModeEnabled() )
		{
			if ( this->isInstancingEnabled() )
			{
				if ( this->isBillBoardingEnabled() )
				{
					if ( !this->prepareSpriteModelMatrix() )
					{
						return false;
					}

					code << "\t" "const mat4 " << ShaderVariable::ModelViewMatrix << " = "
						<< ViewUB(Keys::UniformBlock::Component::ViewMatrix, true) << " * "
						<< ShaderVariable::SpriteModelMatrix << ";" "\n";
				}
				else
				{
					if ( !this->declare(InputAttribute{VertexAttributeType::ModelMatrixR0}) )
					{
						return false;
					}

					code << "\t" "const mat4 " << ShaderVariable::ModelViewMatrix << " = "
						<< ViewUB(Keys::UniformBlock::Component::ViewMatrix, true) << " * "
						<< Attribute::ModelMatrix << ";" "\n";
				}
			}
			else
			{
				code << "\t" "const mat4 " << ShaderVariable::ModelViewMatrix << " = "
					<< ViewUB(Keys::UniformBlock::Component::ViewMatrix, true) << " * "
					<< MatrixPC(PushConstant::Component::ModelMatrix) << ";" "\n";
			}
		}
		else if ( this->isInstancingEnabled() )
		{
			if ( this->isBillBoardingEnabled() )
			{
				if ( !this->prepareSpriteModelMatrix() )
				{
					return false;
				}

				code << "\t" "const mat4 " << ShaderVariable::ModelViewMatrix << " = " << MatrixPC(PushConstant::Component::ViewMatrix) << " * " << ShaderVariable::SpriteModelMatrix << ";" "\n";
			}
			else
			{
				if ( !this->declare(InputAttribute{VertexAttributeType::ModelMatrixR0}) )
				{
					return false;
				}

				code << "\t" "const mat4 " << ShaderVariable::ModelViewMatrix << " = " << MatrixPC(PushConstant::Component::ViewMatrix) << " * " << Attribute::ModelMatrix << ";" "\n";
			}
		}
		else if ( this->isMDIEnabled() )
		{
			if ( !this->prepareMDIModelMatrix() )
			{
				return false;
			}

			code << "\t" "const mat4 " << ShaderVariable::ModelViewMatrix << " = "
				<< ViewUB(Keys::UniformBlock::Component::ViewMatrix, false) << " * "
				<< ShaderVariable::MDIModelMatrix << ";" "\n";
		}
		else if ( this->isInstanceTransformsEnabled() )
		{
			/* NOTE: Advanced path on the InstanceTransforms SSBO: the view matrix is
			 * pushed, the model matrix comes from the per-instance entry. */
			if ( !this->prepareInstanceModelMatrix() )
			{
				return false;
			}

			code << "\t" "const mat4 " << ShaderVariable::ModelViewMatrix << " = " << MatrixPC(PushConstant::Component::ViewMatrix) << " * " << ShaderVariable::InstanceModelMatrix << ";" "\n";
		}
		else
		{
			/* NOTE: For unique sprite (this->isBillBoardingEnabled()), the model matrix is already oriented to the camera. */
			code << "\t" "const mat4 " << ShaderVariable::ModelViewMatrix << " = " << MatrixPC(PushConstant::Component::ViewMatrix) << " * " << MatrixPC(PushConstant::Component::ModelMatrix) << ";" "\n";
		}

		m_uniquePreparations.emplace_back(ShaderVariable::ModelViewMatrix, code.str());

		return true;
	}

	bool
	AbstractVertexStage::prepareNormalMatrix () noexcept
	{
		if ( this->preparationAlreadyDone(ShaderVariable::NormalMatrix) )
		{
			return true;
		}

		std::stringstream code{};

		if ( this->isMDIEnabled() )
		{
			/* MDI pushes only the combined view-projection matrix (BDA per-draw model matrix +
			 * VP), so there is no separate view matrix available. The MDI vertex shader works
			 * entirely in world space (world-space position and tangent-to-world outputs), so the
			 * normal matrix is derived from the world model matrix, not from a model-view matrix. */
			if ( !this->prepareMDIModelMatrix() )
			{
				return false;
			}

			code << "\t" "const mat3 " << ShaderVariable::NormalMatrix << " = transpose(mat3(inverse(" << ShaderVariable::MDIModelMatrix << ")));" "\n";
		}
		else
		{
			if ( !this->prepareModelViewMatrix() )
			{
				return false;
			}

			code << "\t" "const mat3 " << ShaderVariable::NormalMatrix << " = transpose(mat3(inverse(" << ShaderVariable::ModelViewMatrix << ")));" "\n";
		}

		m_uniquePreparations.emplace_back(ShaderVariable::NormalMatrix, code.str());

		return true;
	}

	bool
	AbstractVertexStage::prepareModelViewProjectionMatrix () noexcept
	{
		if ( this->preparationAlreadyDone(ShaderVariable::ModelViewProjectionMatrix) )
		{
			return true;
		}

		std::stringstream code;

		/* NOTE: CSM (Cascaded Shadow Map) mode uses multiview rendering with gl_ViewIndex
		 * to select the correct cascade view-projection matrix from the UBO.
		 * The CSM UBO has mat4[4] cascadeViewProjectionMatrices at offset 0. */
		if ( this->isCSMModeEnabled() )
		{
			if ( this->isInstancingEnabled() )
			{
				if ( this->isBillBoardingEnabled() )
				{
					if ( !this->prepareSpriteModelMatrix() )
					{
						return false;
					}

					code << "\t" "const mat4 " << ShaderVariable::ModelViewProjectionMatrix << " = "
						<< Keys::UniformBlock::View << "." << Keys::UniformBlock::Component::CascadeViewProjectionMatrices << "[gl_ViewIndex] * "
						<< ShaderVariable::SpriteModelMatrix << ";" "\n";
				}
				else
				{
					if ( !this->declare(InputAttribute{VertexAttributeType::ModelMatrixR0}) )
					{
						return false;
					}

					code << "\t" "const mat4 " << ShaderVariable::ModelViewProjectionMatrix << " = "
						<< Keys::UniformBlock::View << "." << Keys::UniformBlock::Component::CascadeViewProjectionMatrices << "[gl_ViewIndex] * "
						<< Attribute::ModelMatrix << ";" "\n";
				}
			}
			else
			{
				code << "\t" "const mat4 " << ShaderVariable::ModelViewProjectionMatrix << " = "
					<< Keys::UniformBlock::View << "." << Keys::UniformBlock::Component::CascadeViewProjectionMatrices << "[gl_ViewIndex] * "
					<< MatrixPC(PushConstant::Component::ModelMatrix) << ";" "\n";
			}
		}
		/* NOTE: Cubemap mode uses multiview rendering with gl_ViewIndex to select the correct view matrix from UBO.
		 * The projection matrix is shared (not per-face), so we use ViewUB(..., false) for projection
		 * but ViewUB(..., true) for the view matrix to get instance[gl_ViewIndex].viewMatrix. */
		else if ( this->isCubemapModeEnabled() )
		{
			if ( this->isInstancingEnabled() )
			{
				if ( this->isBillBoardingEnabled() )
				{
					if ( !this->prepareSpriteModelMatrix() )
					{
						return false;
					}

					code << "\t" "const mat4 " << ShaderVariable::ModelViewProjectionMatrix << " = "
						<< ViewUB(Keys::UniformBlock::Component::ProjectionMatrix, false) << " * "
						<< ViewUB(Keys::UniformBlock::Component::ViewMatrix, true) << " * "
						<< ShaderVariable::SpriteModelMatrix << ";" "\n";
				}
				else
				{
					if ( !this->declare(InputAttribute{VertexAttributeType::ModelMatrixR0}) )
					{
						return false;
					}

					code << "\t" "const mat4 " << ShaderVariable::ModelViewProjectionMatrix << " = "
						<< ViewUB(Keys::UniformBlock::Component::ProjectionMatrix, false) << " * "
						<< ViewUB(Keys::UniformBlock::Component::ViewMatrix, true) << " * "
						<< Attribute::ModelMatrix << ";" "\n";
				}
			}
			else
			{
				code << "\t" "const mat4 " << ShaderVariable::ModelViewProjectionMatrix << " = "
					<< ViewUB(Keys::UniformBlock::Component::ProjectionMatrix, false) << " * "
					<< ViewUB(Keys::UniformBlock::Component::ViewMatrix, true) << " * "
					<< MatrixPC(PushConstant::Component::ModelMatrix) << ";" "\n";
			}
		}
		else if ( this->isInstancingEnabled() )
		{
			if ( this->isBillBoardingEnabled() )
			{
				if ( !this->prepareSpriteModelMatrix() )
				{
					return false;
				}

				/* NOTE: VP is recomposed from the view UBO projection × the pushed view
				 * matrix — pushing V + VP + frameIndex was 132 B, above the 128 B Vulkan
				 * minimum guarantee for maxPushConstantsSize. */
				code << "\t" "const mat4 " << ShaderVariable::ModelViewProjectionMatrix << " = " << ViewUB(Keys::UniformBlock::Component::ProjectionMatrix, false) << " * " << MatrixPC(PushConstant::Component::ViewMatrix) << " * " << ShaderVariable::SpriteModelMatrix << ";" "\n";
			}
			else
			{
				if ( !this->declare(InputAttribute{VertexAttributeType::ModelMatrixR0}) )
				{
					return false;
				}

				if ( this->isAdvancedMatricesEnabled() )
				{
					/* NOTE: Advanced instanced pushes V only (see the billboard note). */
					code << "\t" "const mat4 " << ShaderVariable::ModelViewProjectionMatrix << " = " << ViewUB(Keys::UniformBlock::Component::ProjectionMatrix, false) << " * " << MatrixPC(PushConstant::Component::ViewMatrix) << " * " << Attribute::ModelMatrix << ";" "\n";
				}
				else
				{
					code << "\t" "const mat4 " << ShaderVariable::ModelViewProjectionMatrix << " = " << MatrixPC(PushConstant::Component::ViewProjectionMatrix) << " * " << Attribute::ModelMatrix << ";" "\n";
				}
			}
		}
		else if ( this->isMDIEnabled() )
		{
			if ( !this->prepareMDIModelMatrix() )
			{
				return false;
			}

			code << "\t" "const mat4 " << ShaderVariable::ModelViewProjectionMatrix << " = " << MatrixPC(PushConstant::Component::ViewProjectionMatrix) << " * " << ShaderVariable::MDIModelMatrix << ";" "\n";
		}
		else if ( this->isInstanceTransformsEnabled() && !this->isAdvancedMatricesEnabled() )
		{
			/* NOTE: Classic path on the InstanceTransforms SSBO: the view-projection matrix
			 * is pushed (MDI precedent), the model matrix comes from the per-instance entry. */
			if ( !this->prepareInstanceModelMatrix() )
			{
				return false;
			}

			code << "\t" "const mat4 " << ShaderVariable::ModelViewProjectionMatrix << " = " << MatrixPC(PushConstant::Component::ViewProjectionMatrix) << " * " << ShaderVariable::InstanceModelMatrix << ";" "\n";
		}
		else if ( this->isInstanceTransformsEnabled() )
		{
			/* NOTE: Advanced path on the InstanceTransforms SSBO: the projection comes
			 * from the view UBO, the view matrix is pushed, the model matrix comes from
			 * the per-instance entry. */
			if ( !this->prepareInstanceModelMatrix() )
			{
				return false;
			}

			code << "\t" "const mat4 " << ShaderVariable::ModelViewProjectionMatrix << " = " << ViewUB(Keys::UniformBlock::Component::ProjectionMatrix, false) << " * " << MatrixPC(PushConstant::Component::ViewMatrix) << " * " << ShaderVariable::InstanceModelMatrix << ";" "\n";
		}
		else
		{
			/* NOTE: For unique sprite (this->isBillBoardingEnabled()), the model matrix is already oriented to the camera. */
			code << "\t" "const mat4 " << ShaderVariable::ModelViewProjectionMatrix << " = " << ViewUB(Keys::UniformBlock::Component::ProjectionMatrix, false) << " * " << MatrixPC(PushConstant::Component::ViewMatrix) << " * " << MatrixPC(PushConstant::Component::ModelMatrix) << ";" "\n";
		}

		m_uniquePreparations.emplace_back(ShaderVariable::ModelViewProjectionMatrix, code.str());

		return true;
	}

	bool
	AbstractVertexStage::synthesizeVertexPositionInWorldSpace (Generator::Abstract & generator, std::string & topInstructions, std::string & outputInstructions, VariableScope scope, bool asGLStandardPosition) noexcept
	{
		if ( !this->declare(InputAttribute{VertexAttributeType::Position}) )
		{
			return false;
		}

		std::stringstream code{};

		code << '\t';

		if ( asGLStandardPosition )
		{
			/* NOTE: gl_Position receives a WORLD-space position here (a later stage is expected to
			 * project it). Reachable only through ShaderVariable::GLPositionWorldSpace, which NO
			 * generator requests today — verified 2026-07-25. Should it ever be used for a real
			 * render pass, the TAA sub-pixel jitter would have to be applied by whichever stage
			 * produces the clip position (see isProjectionJitterPushed()). */
			code << m_positionOutput << " = ";
		}
		else
		{
			if ( scope == VariableScope::Local )
			{
				code << "const vec4 ";
			}
			else if ( !this->declare(StageOutput{generator.getNextShaderVariableLocation(), GLSL::FloatVector4, ShaderVariable::PositionWorldSpace, GLSL::Smooth}) )
			{
				return false;
			}

			code << ShaderVariable::PositionWorldSpace << " = ";
		}

		if ( this->isMDIEnabled() )
		{
			if ( !this->prepareMDIModelMatrix() )
			{
				return false;
			}

			const auto posExpr = this->vertexPositionExpression();

			code << ShaderVariable::MDIModelMatrix << " * vec4(" << posExpr << ", 1.0);" "\n";
		}
		else if ( this->isInstancingEnabled() )
		{
			if ( !this->declare(InputAttribute{VertexAttributeType::ModelMatrixR0}) )
			{
				return false;
			}

			const auto posExpr = this->vertexPositionExpression();

			code << Attribute::ModelMatrix << " * vec4(" << posExpr << ", 1.0);" "\n";
		}
		else if ( this->isInstanceTransformsEnabled() && !this->isCubemapModeEnabled() && !this->isCSMModeEnabled() )
		{
			if ( !this->prepareInstanceModelMatrix() )
			{
				return false;
			}

			const auto posExpr = this->vertexPositionExpression();

			code << ShaderVariable::InstanceModelMatrix << " * vec4(" << posExpr << ", 1.0);" "\n";
		}
		else
		{
			const auto posExpr = this->vertexPositionExpression();

			code << MatrixPC(PushConstant::Component::ModelMatrix) << " * vec4(" << posExpr << ", 1.0);" "\n";
		}

		if ( scope != VariableScope::ToNextStage )
		{
			topInstructions.append(code.str());
		}
		else
		{
			outputInstructions.append(code.str());
		}

		return true;
	}

	bool
	AbstractVertexStage::synthesizeModelScale (Generator::Abstract & generator, std::string & outputInstructions) noexcept
	{
		/* NOTE: The four branches MIRROR synthesizeVertexPositionInWorldSpace() — same paths, same
		 * order, same prepare*() calls. They are duplicated rather than factored because the
		 * position path also weaves in skinning and the gl_Position variant; if a fifth model
		 * matrix path ever appears, BOTH have to learn it. */
		if ( !this->declare(StageOutput{generator.getNextShaderVariableLocation(), GLSL::FloatVector3, ShaderVariable::ModelScale, GLSL::Flat}) )
		{
			return false;
		}

		std::string matrixExpression{};

		if ( this->isMDIEnabled() )
		{
			if ( !this->prepareMDIModelMatrix() )
			{
				return false;
			}

			matrixExpression = ShaderVariable::MDIModelMatrix;
		}
		else if ( this->isInstancingEnabled() )
		{
			if ( !this->declare(InputAttribute{VertexAttributeType::ModelMatrixR0}) )
			{
				return false;
			}

			matrixExpression = Attribute::ModelMatrix;
		}
		else if ( this->isInstanceTransformsEnabled() && !this->isCubemapModeEnabled() && !this->isCSMModeEnabled() )
		{
			if ( !this->prepareInstanceModelMatrix() )
			{
				return false;
			}

			matrixExpression = ShaderVariable::InstanceModelMatrix;
		}
		else
		{
			matrixExpression = MatrixPC(PushConstant::Component::ModelMatrix);
		}

		std::stringstream code{};

		code << '\t' << ShaderVariable::ModelScale << " = vec3("
			"length(" << matrixExpression << "[0].xyz), "
			"length(" << matrixExpression << "[1].xyz), "
			"length(" << matrixExpression << "[2].xyz));" "\n";

		outputInstructions.append(code.str());

		return true;
	}

	bool
	AbstractVertexStage::synthesizeRestPositionInModelSpace (Generator::Abstract & generator, std::string & outputInstructions) noexcept
	{
		if ( !this->declare(InputAttribute{VertexAttributeType::Position}) )
		{
			return false;
		}

		if ( !this->declare(StageOutput{generator.getNextShaderVariableLocation(), GLSL::FloatVector3, ShaderVariable::RestPositionModelSpace, GLSL::Smooth}) )
		{
			return false;
		}

		std::stringstream code{};

		code << '\t' << ShaderVariable::RestPositionModelSpace << " = " << Attribute::Position << ";" "\n";

		outputInstructions.append(code.str());

		return true;
	}

	bool
	AbstractVertexStage::synthesizeVertexPositionInViewSpace (Generator::Abstract & generator, std::string & topInstructions, std::string & outputInstructions, VariableScope scope) noexcept
	{
		if ( !this->declare(InputAttribute{VertexAttributeType::Position}) )
		{
			return false;
		}

		std::stringstream code{};

		code << '\t';

		if ( scope == VariableScope::Local )
		{
			code << "const vec4 ";
		}
		else if ( !this->declare(StageOutput{generator.getNextShaderVariableLocation(), GLSL::FloatVector4, ShaderVariable::PositionViewSpace, GLSL::Smooth}) )
		{
			return false;
		}

		if ( !this->prepareModelViewMatrix() )
		{
			return false;
		}

		const auto posExpr = this->vertexPositionExpression();

		code << ShaderVariable::PositionViewSpace << " = " << ShaderVariable::ModelViewMatrix << " * vec4(" << posExpr << ", 1.0);" "\n";

		if ( scope != VariableScope::ToNextStage )
		{
			topInstructions.append(code.str());
		}
		else
		{
			outputInstructions.append(code.str());
		}

		return true;
	}

	bool
	AbstractVertexStage::synthesizeVertexPositionInScreenSpace (std::string & outputInstructions) noexcept
	{
		if ( !this->declare(InputAttribute{VertexAttributeType::Position}) )
		{
			return false;
		}

		std::string MVPMatrix;

		/* NOTE: When rendering to a cubemap or CSM, we MUST always prepare the ModelViewProjectionMatrix
		 * because the push constant contains only the Model matrix, and Projection/View come from the UBO
		 * indexed by gl_ViewIndex. Without this, the shader would try to read a non-existent MVP from push constants. */
		if ( this->isMDIEnabled() || this->isInstancingEnabled() || this->isAdvancedMatricesEnabled() || this->isCubemapModeEnabled() || this->isCSMModeEnabled() || this->isInstanceTransformsEnabled() )
		{
			if ( !this->prepareModelViewProjectionMatrix() )
			{
				return false;
			}

			MVPMatrix = ShaderVariable::ModelViewProjectionMatrix;
		}
		else
		{
			MVPMatrix = MatrixPC(PushConstant::Component::ModelViewProjectionMatrix);
		}

		const auto posExpr = this->vertexPositionExpression();

		outputInstructions += "\t" + std::string{m_positionOutput} + " = ";
		outputInstructions += MVPMatrix;
		outputInstructions += " * vec4(";
		outputInstructions += posExpr;
		outputInstructions += ", 1.0);" "\n";

		/* ⚠️ Infinity view (the sky background, its ONLY user): the clip depth is pinned to the
		 * far plane, so the sky can never be clipped away by a short far distance. The projection
		 * maps near to 0 and far to 1 (Vulkan range, not reversed), hence z = w. This is the
		 * standard skybox depth trick (`gl_Position = clipPosition.xyww` in the Khronos glTF
		 * Sample Viewer's skybox.vert).
		 *
		 * WHY, and it is not a micro-optimisation: the sky is a 512 m cuboid centred on the
		 * camera, so its faces sit 256 m away and its corners 443 m. ANY scene whose camera far
		 * distance fell under that lost its sky ENTIRELY — silently, with no error, no warning and
		 * no broken-renderable trace. Measured on the +ModelViewer, whose far is derived from the
		 * model size (`max(100, radius * 20)`): every asset under ~22 m of radius rendered against
		 * a bit-exact (0,0,0) void, which also made every reflective, transmissive and clearcoat
		 * material unjudgeable. Pinning the depth removes the whole class of bug and makes the
		 * geometric size of the sky irrelevant.
		 *
		 * Safe by construction: the background is the only renderable carrying the infinity view
		 * (Scene::registerSceneVisualComponents), and it is drawn with the depth test AND the depth
		 * write disabled, so pinning its depth can neither fail a comparison nor pollute the
		 * depth buffer. The velocity clip positions are synthesized independently of gl_Position
		 * (synthesizeVelocityClipPositions), so the motion vectors are untouched. */
		if ( this->isInfinityViewEnabled() )
		{
			outputInstructions += "\t" + std::string{m_positionOutput} + ".z = " + m_positionOutput + ".w;" "\n";
		}

		/* TAA sub-pixel jitter: applied HERE and nowhere else. Offsetting the clip position by
		 * jitter * W is exactly an NDC translation after the perspective division. Keeping it
		 * out of every matrix is what makes the velocity outputs jitter-free and, above all,
		 * keeps the frame-varying value out of the SINGLE-buffered view UBO (a push constant is
		 * recorded per draw, hence per-frame AND per-render-target by construction: only the
		 * main view sets a jitter, shadow maps / RTT / cubemaps push zero). */
		if ( this->isProjectionJitterPushed() )
		{
			outputInstructions += "\t" + std::string{m_positionOutput} + ".xy += ";
			outputInstructions += MatrixPC(PushConstant::Component::ProjectionJitter);
			outputInstructions += " * " + std::string{m_positionOutput} + ".w;" "\n";
		}

		return true;
	}

	bool
	AbstractVertexStage::synthesizeVertexPositionInTextureSpace (Generator::Abstract & generator, std::string & topInstructions, std::string & outputInstructions, VariableScope scope) noexcept
	{
		topInstructions.append((std::stringstream{} <<
			"\t" "const float positionTextureX = dot(-" << ShaderVariable::PositionViewSpace << ".xyz, " << ShaderVariable::TangentViewSpace << ");" "\n" <<
			"\t" "const float positionTextureY = dot(-" << ShaderVariable::PositionViewSpace << ".xyz, " << ShaderVariable::BinormalViewSpace << ");" "\n" <<
			"\t" "const float positionTextureZ = dot(-" << ShaderVariable::PositionViewSpace << ".xyz, " << ShaderVariable::NormalViewSpace << ");" "\n\n"
		).str());

		/* FIXME: Rework this to avoid code duplication. */
		if ( scope == VariableScope::Local )
		{
			topInstructions.append((std::stringstream{} << "	const vec4 " << ShaderVariable::PositionTextureSpace << " = vec4(positionTextureX, positionTextureY, positionTextureZ, 1.0);" "\n").str());
		}
		else
		{
			if ( !this->declare(StageOutput{generator.getNextShaderVariableLocation(), GLSL::FloatVector4, ShaderVariable::PositionTextureSpace, GLSL::Smooth}) )
			{
				return false;
			}

			const auto subCode = (std::stringstream{} << '\t' << ShaderVariable::PositionTextureSpace << " = vec4(positionTextureX, positionTextureY, positionTextureZ, 1.0);" "\n").str();

			if ( scope != VariableScope::ToNextStage )
			{
				topInstructions.append(subCode);
			}
			else
			{
				outputInstructions.append(subCode);
			}
		}

		return true;
	}

	bool
	AbstractVertexStage::synthesizeVertexColor (Generator::Abstract & generator, std::string & outputInstructions) noexcept
	{
		if ( !this->declare(StageOutput{generator.getNextShaderVariableLocation(), GLSL::FloatVector4, ShaderVariable::PrimaryVertexColor, GLSL::Smooth}) )
		{
			return false;
		}

		if ( !this->declare(InputAttribute{VertexAttributeType::VertexColor}) )
		{
			return false;
		}

		outputInstructions += '\t';
		outputInstructions += ShaderVariable::PrimaryVertexColor;
		outputInstructions += " = ";
		outputInstructions += Attribute::Color;
		outputInstructions += ";\n";

		return true;
	}

	bool
	AbstractVertexStage::synthesizeVertexTextureCoordinates (Generator::Abstract & generator, std::string & outputInstructions, const char * TCVariableName) noexcept
	{
		if ( std::strcmp(TCVariableName, ShaderVariable::Primary2DTextureCoordinates) == 0 )
		{
			/* A heightfield has no UV attribute: its coordinates are the world XZ, scaled. */
			if ( m_heightfieldSurfaceEnabled )
			{
				m_heightfieldTextureCoordinatesRequested = true;
			}
			else if ( !this->declare(InputAttribute{VertexAttributeType::Primary2DTextureCoordinates}) )
			{
				return false;
			}

			if ( !this->declare(StageOutput{generator.getNextShaderVariableLocation(), GLSL::FloatVector2, TCVariableName, GLSL::Smooth}) )
			{
				return false;
			}

			outputInstructions.append((std::stringstream{} <<
				'\t' << TCVariableName << " = " << (m_heightfieldSurfaceEnabled ? "hfTextureCoordinates" : Attribute::Primary2DTextureCoordinates) << ";" "\n"
			).str());
		}
		else if ( std::strcmp(TCVariableName, ShaderVariable::Primary3DTextureCoordinates) == 0 )
		{
			if ( !this->declare(InputAttribute{VertexAttributeType::Primary3DTextureCoordinates}) )
			{
				return false;
			}

			if ( !this->declare(StageOutput{generator.getNextShaderVariableLocation(), GLSL::FloatVector3, TCVariableName, GLSL::Smooth}) )
			{
				return false;
			}

			outputInstructions.append((std::stringstream{} <<
				'\t' << TCVariableName << " = " << Attribute::Primary3DTextureCoordinates << ";" "\n"
			).str());
		}
		else if ( std::strcmp(TCVariableName, ShaderVariable::Secondary2DTextureCoordinates) == 0 )
		{
			if ( !this->declare(InputAttribute{VertexAttributeType::Secondary2DTextureCoordinates}) )
			{
				return false;
			}

			if ( !this->declare(StageOutput{generator.getNextShaderVariableLocation(), GLSL::FloatVector2, TCVariableName, GLSL::Smooth}) )
			{
				return false;
			}

			outputInstructions.append((std::stringstream{} <<
				'\t' << TCVariableName << " = " << Attribute::Secondary2DTextureCoordinates << ";" "\n"
			).str());
		}
		else if ( std::strcmp(TCVariableName, ShaderVariable::Secondary3DTextureCoordinates) == 0 )
		{
			if ( !this->declare(InputAttribute{VertexAttributeType::Secondary3DTextureCoordinates}) )
			{
				return false;
			}

			if ( !this->declare(StageOutput{generator.getNextShaderVariableLocation(), GLSL::FloatVector3, TCVariableName, GLSL::Smooth}) )
			{
				return false;
			}

			outputInstructions.append((std::stringstream{} <<
				'\t' << TCVariableName << " = " << Attribute::Secondary3DTextureCoordinates << ";" "\n"
			).str());
		}
		else
		{
			TraceError{ClassId} << "Texture coordinates variable '" << TCVariableName << "' is not handled !";

			return false;
		}

		return true;
	}

	bool
	AbstractVertexStage::synthesizeVertexVectorInWorldSpace (Generator::Abstract & generator, std::string & topInstructions, std::string & outputInstructions, VertexAttributeType vectorType, VariableScope scope) noexcept
	{
		const char * attributeName = nullptr;
		const char * vectorName = nullptr;

		switch ( vectorType )
		{
			case VertexAttributeType::Tangent :
				attributeName = this->vertexFrameExpression(VertexAttributeType::Tangent);
				vectorName = ShaderVariable::TangentWorldSpace;
				break;

			case VertexAttributeType::Binormal :
				attributeName = this->vertexFrameExpression(VertexAttributeType::Binormal);
				vectorName = ShaderVariable::BinormalWorldSpace;
				break;

			case VertexAttributeType::Normal :
				attributeName = this->vertexFrameExpression(VertexAttributeType::Normal);
				vectorName = ShaderVariable::NormalWorldSpace;
				break;

			/* NOTE: Ignorable "Unreachable code" warning. */
			default :
				Tracer::error(ClassId, "The synthetic vertex vector in world space can only be the tangent, the bi-normal or the normal !");

				return false;
		}

		if ( !this->declareFrameAttribute(vectorType) )
		{
			return false;
		}

		std::stringstream code{};

		code << '\t';

		if ( scope == VariableScope::Local )
		{
			code << "const vec3 ";
		}
		else if ( !this->declare(StageOutput{generator.getNextShaderVariableLocation(), GLSL::FloatVector3, vectorName, GLSL::Smooth}) )
		{
			return false;
		}

		std::string modelMatrix;

		if ( !this->resolveWorldModelMatrix(modelMatrix) )
		{
			return false;
		}

		/* NOTE: w=0.0 because normals are direction vectors, not points.
		 * Using w=1.0 would incorrectly apply the model matrix translation to the normal. */
		code << vectorName << " = (" << modelMatrix << " * vec4(" << attributeName << ", 0.0)).xyz;" "\n";

		if ( scope != VariableScope::ToNextStage )
		{
			topInstructions.append(code.str());
		}
		else
		{
			outputInstructions.append(code.str());
		}

		return true;
	}

	bool
	AbstractVertexStage::synthesizeVertexVectorInViewSpace (Generator::Abstract & generator, std::string & topInstructions, std::string & outputInstructions, VertexAttributeType vectorType, VariableScope scope) noexcept
	{
		const char * attributeName = nullptr;
		const char * vectorName = nullptr;

		switch ( vectorType )
		{
			case VertexAttributeType::Tangent :
				attributeName = this->vertexFrameExpression(VertexAttributeType::Tangent);
				vectorName = ShaderVariable::TangentViewSpace;
				break;

			case VertexAttributeType::Binormal :
				attributeName = this->vertexFrameExpression(VertexAttributeType::Binormal);
				vectorName = ShaderVariable::BinormalViewSpace;
				break;

			case VertexAttributeType::Normal :
				attributeName = this->vertexFrameExpression(VertexAttributeType::Normal);
				vectorName = ShaderVariable::NormalViewSpace;
				break;

			/* NOTE: Ignorable "Unreachable code" warning. */
			default :
				Tracer::error(ClassId, "The synthetic vertex vector in view space can only be the tangent, the bi-normal or the normal !");

				return false;
		}

		if ( !this->declareFrameAttribute(vectorType) )
		{
			return false;
		}

		std::stringstream code{};

		code << '\t';

		if ( scope == VariableScope::Local )
		{
			code << "const vec3 ";
		}
		else if ( !this->declare(StageOutput{generator.getNextShaderVariableLocation(), GLSL::FloatVector3, vectorName, GLSL::Smooth}) )
		{
			return false;
		}

		if ( !this->prepareNormalMatrix() )
		{
			return false;
		}

		code << vectorName << " = normalize(" << ShaderVariable::NormalMatrix << " * " << attributeName << ");" "\n";

		if ( scope != VariableScope::ToNextStage )
		{
			topInstructions.append(code.str());
		}
		else
		{
			outputInstructions.append(code.str());
		}

		return true;
	}

	bool
	AbstractVertexStage::synthesizeWorldTBNMatrix (Generator::Abstract & generator, std::string & topInstructions, std::string & outputInstructions, VariableScope scope) noexcept
	{
		if ( !this->declareFrameAttribute(VertexAttributeType::Tangent) )
		{
			return false;
		}

		if ( !this->declareFrameAttribute(VertexAttributeType::Binormal) )
		{
			return false;
		}

		if ( !this->declareFrameAttribute(VertexAttributeType::Normal) )
		{
			return false;
		}

		if ( !this->declare(StageOutput{generator.getNextShaderVariableLocation(3), GLSL::Matrix3, ShaderVariable::WorldTBNMatrix, GLSL::Smooth}) )
		{
			return false;
		}

		/* NOTE: The one resolution of the world model matrix, shared with the world-space vectors. This
		 * copy used to have no MDI branch, so an MDI program asking for the world TBN read a push
		 * constant member that its block does not declare. */
		std::string modelMatrix;

		if ( !this->resolveWorldModelMatrix(modelMatrix) )
		{
			return false;
		}

		const auto tanExpr = this->vertexFrameExpression(VertexAttributeType::Tangent);
		const auto binExpr = this->vertexFrameExpression(VertexAttributeType::Binormal);
		const auto norExpr = this->vertexFrameExpression(VertexAttributeType::Normal);

		topInstructions.append((std::stringstream{} <<
			"	const vec3 worldT = normalize((" << modelMatrix << " * vec4(" << tanExpr << ", 0.0)).xyz);" "\n"
			"	const vec3 worldB = normalize((" << modelMatrix << " * vec4(" << binExpr << ", 0.0)).xyz);" "\n"
			"	const vec3 worldN = normalize((" << modelMatrix << " * vec4(" << norExpr << ", 0.0)).xyz);" "\n"
		).str());

		const auto matrixCode = (std::stringstream{} <<
			'\t' << ShaderVariable::WorldTBNMatrix << " = mat3(worldT, worldB, worldN);" "\n"
		).str();

		if ( scope != VariableScope::ToNextStage )
		{
			topInstructions.append(matrixCode);
		}
		else
		{
			outputInstructions.append(matrixCode);
		}

		return true;
	}

	bool
	AbstractVertexStage::synthesizeViewTBNMatrix (Generator::Abstract & generator, std::string & topInstructions, std::string & outputInstructions, VariableScope scope) noexcept
	{
		if ( !this->declareFrameAttribute(VertexAttributeType::Tangent) )
		{
			return false;
		}

		if ( !this->declareFrameAttribute(VertexAttributeType::Binormal) )
		{
			return false;
		}

		if ( !this->declareFrameAttribute(VertexAttributeType::Normal) )
		{
			return false;
		}

		if ( !this->declare(StageOutput{generator.getNextShaderVariableLocation(3), GLSL::Matrix3, ShaderVariable::ViewTBNMatrix, GLSL::Smooth}) )
		{
			return false;
		}

		if ( !this->prepareNormalMatrix() )
		{
			return false;
		}

		{
			const auto tanExpr = this->vertexFrameExpression(VertexAttributeType::Tangent);
			const auto binExpr = this->vertexFrameExpression(VertexAttributeType::Binormal);
			const auto norExpr = this->vertexFrameExpression(VertexAttributeType::Normal);

			topInstructions.append((std::stringstream{} <<
				"	const vec3 viewT = normalize(" << ShaderVariable::NormalMatrix << " * " << tanExpr << ");" "\n"
				"	const vec3 viewB = normalize(" << ShaderVariable::NormalMatrix << " * " << binExpr << ");" "\n"
				"	const vec3 viewN = normalize(" << ShaderVariable::NormalMatrix << " * " << norExpr << ");" "\n"
			).str());
		}

		const auto matrixCode = (std::stringstream{} << '\t' << ShaderVariable::ViewTBNMatrix << " = transpose(mat3(viewT, viewB, viewN));" "\n").str();

		if ( scope != VariableScope::ToNextStage )
		{
			topInstructions.append(matrixCode);
		}
		else
		{
			outputInstructions.append(matrixCode);
		}

		return true;
	}

	bool
	AbstractVertexStage::synthesizeTangentToWorldMatrix (Generator::Abstract & generator, std::string & topInstructions, std::string & outputInstructions, VariableScope scope) noexcept
	{
		if ( !this->declareFrameAttribute(VertexAttributeType::Tangent) )
		{
			return false;
		}

		if ( !this->declareFrameAttribute(VertexAttributeType::Binormal) )
		{
			return false;
		}

		if ( !this->declareFrameAttribute(VertexAttributeType::Normal) )
		{
			return false;
		}

		if ( !this->declare(StageOutput{generator.getNextShaderVariableLocation(3), GLSL::Matrix3, ShaderVariable::TangentToWorldMatrix, GLSL::Smooth}) )
		{
			return false;
		}

		if ( !this->prepareNormalMatrix() )
		{
			return false;
		}

		const auto tanExpr = this->vertexFrameExpression(VertexAttributeType::Tangent);
		const auto binExpr = this->vertexFrameExpression(VertexAttributeType::Binormal);
		const auto norExpr = this->vertexFrameExpression(VertexAttributeType::Normal);

		const auto matrixCode = (std::stringstream{} <<
			'\t' << ShaderVariable::TangentToWorldMatrix << " = " << ShaderVariable::NormalMatrix << " * mat3(" << tanExpr << ", " << binExpr << ", " << norExpr << ");" "\n"
		).str();

		if ( scope != VariableScope::ToNextStage )
		{
			topInstructions.append(matrixCode);
		}
		else
		{
			outputInstructions.append(matrixCode);
		}

		return true;
	}

	bool
	AbstractVertexStage::synthesizeRequestInstructions (Generator::Abstract & generator, std::string & topInstructions, std::string & outputInstructions) noexcept
	{
		for ( const auto & [variableType, variableScope] : m_requests )
		{
			if ( std::strcmp(variableType, ShaderVariable::PositionScreenSpace) == 0 )
			{
				if ( !this->synthesizeVertexPositionInScreenSpace(outputInstructions) )
				{
					return false;
				}

				continue;
			}

			if ( std::strcmp(variableType, ShaderVariable::PositionWorldSpace) == 0 )
			{
				if ( !this->synthesizeVertexPositionInWorldSpace(generator, topInstructions, outputInstructions, variableScope, false) )
				{
					return false;
				}

				continue;
			}

			if ( std::strcmp(variableType, ShaderVariable::GLPositionWorldSpace) == 0 )
			{
				if ( !this->synthesizeVertexPositionInWorldSpace(generator, topInstructions, outputInstructions, variableScope, true) )
				{
					return false;
				}

				continue;
			}

			if ( std::strcmp(variableType, ShaderVariable::RestPositionModelSpace) == 0 )
			{
				if ( !this->synthesizeRestPositionInModelSpace(generator, outputInstructions) )
				{
					return false;
				}

				continue;
			}

			if ( std::strcmp(variableType, ShaderVariable::ModelScale) == 0 )
			{
				if ( !this->synthesizeModelScale(generator, outputInstructions) )
				{
					return false;
				}

				continue;
			}

			if ( std::strcmp(variableType, ShaderVariable::PositionViewSpace) == 0 )
			{
				if ( !this->synthesizeVertexPositionInViewSpace(generator, topInstructions, outputInstructions, variableScope) )
				{
					return false;
				}

				continue;
			}

			if ( std::strcmp(variableType, ShaderVariable::PositionTextureSpace) == 0 )
			{
				if ( !this->synthesizeVertexPositionInTextureSpace(generator, topInstructions, outputInstructions, variableScope) )
				{
					return false;
				}

				continue;
			}

			if ( std::strcmp(variableType, ShaderVariable::PrimaryVertexColor) == 0 )
			{
				if ( !this->synthesizeVertexColor(generator, outputInstructions) )
				{
					return false;
				}

				continue;
			}

			/* Texture coordinates flavors. */
			{
				constexpr std::array< const char *, 4 > textureCoordinates{
					ShaderVariable::Primary2DTextureCoordinates,
					ShaderVariable::Primary3DTextureCoordinates,
					ShaderVariable::Secondary2DTextureCoordinates,
					ShaderVariable::Secondary3DTextureCoordinates
				};

				bool found = false;

				for ( const auto & shaderVariable : textureCoordinates )
				{
					if ( std::strcmp(variableType, shaderVariable) != 0 )
					{
						continue;
					}

					if ( !this->synthesizeVertexTextureCoordinates(generator, outputInstructions, shaderVariable) )
					{
						return false;
					}

					found = true;
				}

				if ( found )
				{
					continue;
				}
			}

			if ( std::strcmp(variableType, ShaderVariable::TangentWorldSpace) == 0 )
			{
				if ( !this->synthesizeVertexVectorInWorldSpace(generator, topInstructions, outputInstructions, VertexAttributeType::Tangent, variableScope) )
				{
					return false;
				}

				continue;
			}

			if ( std::strcmp(variableType, ShaderVariable::TangentViewSpace) == 0 )
			{
				if ( !this->synthesizeVertexVectorInViewSpace(generator, topInstructions, outputInstructions, VertexAttributeType::Tangent, variableScope) )
				{
					return false;
				}

				continue;
			}

			if ( std::strcmp(variableType, ShaderVariable::BinormalWorldSpace) == 0 )
			{
				if ( !this->synthesizeVertexVectorInWorldSpace(generator, topInstructions, outputInstructions, VertexAttributeType::Binormal, variableScope) )
				{
					return false;
				}

				continue;
			}

			if ( std::strcmp(variableType, ShaderVariable::BinormalViewSpace) == 0 )
			{
				if ( !this->synthesizeVertexVectorInViewSpace(generator, topInstructions, outputInstructions, VertexAttributeType::Binormal, variableScope) )
				{
					return false;
				}

				continue;
			}

			if ( std::strcmp(variableType, ShaderVariable::NormalWorldSpace) == 0 )
			{
				if ( !this->synthesizeVertexVectorInWorldSpace(generator, topInstructions, outputInstructions, VertexAttributeType::Normal, variableScope) )
				{
					return false;
				}

				continue;
			}

			if ( std::strcmp(variableType, ShaderVariable::NormalViewSpace) == 0 )
			{
				if ( !this->synthesizeVertexVectorInViewSpace(generator, topInstructions, outputInstructions, VertexAttributeType::Normal, variableScope) )
				{
					return false;
				}

				continue;
			}

			if ( std::strcmp(variableType, ShaderVariable::WorldTBNMatrix) == 0 )
			{
				if ( !this->synthesizeWorldTBNMatrix(generator, topInstructions, outputInstructions, variableScope) )
				{
					return false;
				}

				continue;
			}

			if ( std::strcmp(variableType, ShaderVariable::ViewTBNMatrix) == 0 )
			{
				if ( !this->synthesizeViewTBNMatrix(generator, topInstructions, outputInstructions, variableScope) )
				{
					return false;
				}

				continue;
			}


			if ( std::strcmp(variableType, ShaderVariable::TangentToWorldMatrix) == 0 )
			{
				if ( !this->synthesizeTangentToWorldMatrix(generator, topInstructions, outputInstructions, variableScope) )
				{
					return false;
				}

				continue;
			}
		}

		return true;
	}

	const char *
	AbstractVertexStage::vertexFrameExpression (VertexAttributeType vectorType) const noexcept
	{
		switch ( vectorType )
		{
			case VertexAttributeType::Tangent :
				if ( m_heightfieldSurfaceEnabled )
				{
					return "hfTangent";
				}

				return m_skinningEnabled ? "skinnedTangent" : Attribute::Tangent;

			case VertexAttributeType::Binormal :
				if ( m_heightfieldSurfaceEnabled )
				{
					return "hfBinormal";
				}

				return m_skinningEnabled ? "skinnedBinormal" : Attribute::Binormal;

			case VertexAttributeType::Normal :
				if ( m_heightfieldSurfaceEnabled )
				{
					return "hfNormal";
				}

				return m_skinningEnabled ? "skinnedNormal" : Attribute::Normal;

			default :
				return nullptr;
		}
	}

	bool
	AbstractVertexStage::declareFrameAttribute (VertexAttributeType vectorType) noexcept
	{
		if ( m_heightfieldSurfaceEnabled )
		{
			m_heightfieldFrameRequested = true;

			return true;
		}

		return this->declare(InputAttribute{vectorType});
	}

	bool
	AbstractVertexStage::resolveWorldModelMatrix (std::string & expression) noexcept
	{
		if ( this->isMDIEnabled() )
		{
			if ( !this->prepareMDIModelMatrix() )
			{
				return false;
			}

			expression = ShaderVariable::MDIModelMatrix;
		}
		else if ( this->isInstancingEnabled() )
		{
			if ( !this->declare(InputAttribute{VertexAttributeType::ModelMatrixR0}) )
			{
				return false;
			}

			expression = Attribute::ModelMatrix;
		}
		else if ( this->isInstanceTransformsEnabled() && !this->isCubemapModeEnabled() && !this->isCSMModeEnabled() )
		{
			if ( !this->prepareInstanceModelMatrix() )
			{
				return false;
			}

			expression = ShaderVariable::InstanceModelMatrix;
		}
		else
		{
			expression = MatrixPC(PushConstant::Component::ModelMatrix);
		}

		return true;
	}

	std::string
	AbstractVertexStage::generateHeightfieldSurfaceCode () const noexcept
	{
		using namespace Graphics::Geometry;

		const std::string surface{HeightfieldSurface::UniformBlockInstance};
		const std::string node{MatrixPC(PushConstant::Component::HeightfieldNode)};
		const std::string camera{MatrixPC(PushConstant::Component::HeightfieldCamera)};

		/* Strugar's CDLOD vertex program (JGT 2009, § 3.3), on a clipmap instead of one heightmap:
		 * - the patch point g (integers 0..G) is placed in the node: flat = origin + g · step;
		 * - the morph factor grows from 0 to 1 over the last part of the node's range, measured from
		 *   the camera the levels were SELECTED for (pushed per pass) to the unmorphed vertex;
		 * - the odd coordinates slide by one cell toward the even ones, so at 1 the patch IS the next
		 *   level's patch (degenerate triangles fill the rest);
		 * - the height blends from the node's clip level to the next one with the same factor: a fully
		 *   morphed vertex reads exactly what the coarser neighbour reads, and no seam is left to stitch. */
		std::string code =
			"\t" "/* Heightfield surface: CDLOD patch on the height clipmap. */" "\n"
			"\t" "const vec4 hfNode = " + node + ";" "\n"
			"\t" "const vec3 hfEye = " + camera + ".xyz;" "\n"
			"\t" "const float hfStep = hfNode.z / " + surface + ".grid.w;" "\n"
			"\t" "const int hfLastLevel = int(" + surface + ".grid.z) - 1;" "\n"
			"\t" "const int hfLOD = int(hfNode.w);" "\n"
			"\t" "const int hfFineLevel = min(hfLOD, hfLastLevel);" "\n"
			"\t" "const int hfCoarseLevel = min(hfLOD + 1, hfLastLevel);" "\n"
			"\t" "const vec2 hfGrid = " + std::string{Attribute::Position} + ".xz;" "\n"
			"\t" "const vec2 hfFlat = hfNode.xy + (hfGrid * hfStep);" "\n"
			"\t" "const vec4 hfRange = " + surface + ".levelsOfDetail[hfLOD];" "\n"
			"\t" "const float hfMorph = clamp((distance(hfEye, vec3(hfFlat.x, hfHeight(hfFlat, hfFineLevel), hfFlat.y)) - hfRange.x) * hfRange.y, 0.0, 1.0);" "\n"
			"\t" "const vec2 hfMorphed = hfNode.xy + ((hfGrid - (mod(hfGrid, 2.0) * hfMorph)) * hfStep);" "\n"
			"\t" "const vec3 hfPosition = vec3(hfMorphed.x, mix(hfHeight(hfMorphed, hfFineLevel), hfHeight(hfMorphed, hfCoarseLevel), hfMorph), hfMorphed.y);" "\n";

		/* The frame is a function of the normal (Khronos convention, like the grid it replaces:
		 * T = dP/du along +X, B = N x T). The shadow pass asks for none of it. */
		if ( m_heightfieldFrameRequested )
		{
			code +=
				"\t" "const vec3 hfNormal = normalize(mix(hfNormalAt(hfMorphed, hfFineLevel), hfNormalAt(hfMorphed, hfCoarseLevel), hfMorph));" "\n"
				"\t" "const vec3 hfTangent = normalize(vec3(hfNormal.y, -hfNormal.x, 0.0));" "\n"
				"\t" "const vec3 hfBinormal = cross(hfNormal, hfTangent);" "\n";
		}

		if ( m_heightfieldTextureCoordinatesRequested )
		{
			code += "\t" "const vec2 hfTextureCoordinates = (hfMorphed * " + surface + ".textureCoordinates.xy) + " + surface + ".textureCoordinates.zw;" "\n";
		}

		code += "\n";

		return code;
	}

	bool
	AbstractVertexStage::declareHeightfieldPixelFrameOutputs (Generator::Abstract & generator, std::string & outputInstructions) noexcept
	{
		using namespace Graphics::Geometry;

		if ( !this->declare(StageOutput{generator.getNextShaderVariableLocation(), GLSL::FloatVector2, HeightfieldSurface::PixelPositionVarying, GLSL::Smooth}) )
		{
			return false;
		}

		if ( !this->declare(StageOutput{generator.getNextShaderVariableLocation(3), GLSL::Matrix3, HeightfieldSurface::PixelToWorldVarying, GLSL::Flat}) )
		{
			return false;
		}

		if ( !this->declare(StageOutput{generator.getNextShaderVariableLocation(3), GLSL::Matrix3, HeightfieldSurface::PixelToViewVarying, GLSL::Flat}) )
		{
			return false;
		}

		std::string modelMatrix;

		if ( !this->resolveWorldModelMatrix(modelMatrix) || !this->prepareNormalMatrix() )
		{
			return false;
		}

		/* The same two rotations the vertex stage applies to a surface vector
		 * (synthesizeVertexVectorInWorldSpace() / synthesizeVertexVectorInViewSpace()), handed flat so
		 * the fragment stage applies them to the vector of ITS pixel. */
		outputInstructions.append((std::stringstream{} <<
			'\t' << HeightfieldSurface::PixelPositionVarying << " = hfPosition.xz;" "\n" <<
			'\t' << HeightfieldSurface::PixelToWorldVarying << " = mat3(" << modelMatrix << ");" "\n" <<
			'\t' << HeightfieldSurface::PixelToViewVarying << " = " << ShaderVariable::NormalMatrix << ";" "\n"
		).str());

		return true;
	}

	const char *
	AbstractVertexStage::vertexPositionExpression () const noexcept
	{
		/* A heightfield is its own displacement stage: the patch point placed and lifted. */
		if ( m_heightfieldSurfaceEnabled )
		{
			return "hfPosition";
		}

		/* The chain is skinning -> wind -> consumers: the wind displaces what the skinning
		 * produced, never the raw attribute. */
		if ( m_vegetationWindEnabled )
		{
			return "windPosition";
		}

		if ( m_skinningEnabled )
		{
			return "skinnedPosition";
		}

		return Attribute::Position;
	}

	const char *
	AbstractVertexStage::previousVertexPositionExpression () const noexcept
	{
		/* The ground does not move: a vertex's previous position is its current one. The geomorph does
		 * slide a vertex as the camera moves, by a fraction of a cell over the morph range — a motion
		 * the velocity buffer deliberately ignores rather than tracking the previous camera. */
		if ( m_heightfieldSurfaceEnabled )
		{
			return "hfPosition";
		}

		if ( m_vegetationWindEnabled )
		{
			return "previousWindPosition";
		}

		if ( m_skinningEnabled )
		{
			return "previousSkinnedPosition";
		}

		return Attribute::Position;
	}

	std::string
	AbstractVertexStage::generateVegetationWindCode (const char * baseExpression) const noexcept
	{
		const std::string base{baseExpression};
		const std::string color{Attribute::Color};

		/* The wind state rides in the instance-transforms SSBO header, beside the previous
		 * view-projection: per-frame state, and its PREVIOUS value sits where the motion-vector
		 * pass already reads its own.
		 * ⚠️ CONTINUITY is the contract (owner, 2026-09-23: the wind tore the trees apart): every term is a
		 * continuous function over the tree, so two vertices at a junction move together. R and G are
		 * continuous along the skeleton (TreeSkinner), B is shared by a whole limb, the spatial phase is a
		 * smooth function of the position, and the flutter is 0 at every petiole (card V = 1). */
		std::string code =
			"\t" "/* Vegetation wind. R trunk bend, G branch bend (cumulative from the trunk), B limb phase. */" "\n"
			"\t" "vec3 windDirection = ubInstanceTransforms.windDirectionStrength.xyz;" "\n"
			"\t" "float windAmplitude = ubInstanceTransforms.windDirectionStrength.w * (1.0 + ubInstanceTransforms.windTimes.z);" "\n"
			"\t" "float windSpatial = dot(" + base + ".xz, vec2(0.27, 0.19));" "\n"
			"\t" "float windLimbPhase = " + color + ".b * 6.2831853;" "\n";

		if ( m_vegetationFlutterEnabled )
		{
			code +=
				"\t" "float windFlutterWeight = 1.0 - " + std::string{Attribute::Primary2DTextureCoordinates} + ".y;" "\n"
				"\t" "float windFlutterPhase = dot(" + base + ", vec3(5.1, 3.7, 4.3));" "\n";
		}

		const auto offsetCode = [&color, this] (const std::string & target, const std::string & timeExpression) {
			std::string offset =
				"\t" "{" "\n"
				"\t\t" "float windTime = " + timeExpression + ";" "\n"
				"\t\t" "float trunkWave = sin(windTime * 0.9 + windSpatial);" "\n"
				"\t\t" "float branchWave = sin(windTime * 2.7 + windSpatial + windLimbPhase);" "\n"
				"\t\t" + target + " += windDirection * (" + color + ".r * trunkWave + " + color + ".g * branchWave * 0.45) * windAmplitude;" "\n";

			if ( m_vegetationFlutterEnabled )
			{
				offset += "\t\t" + target + ".y += windFlutterWeight * sin(windTime * 9.0 + windFlutterPhase) * windAmplitude * 0.08;" "\n";
			}

			return offset + "\t" "}" "\n";
		};

		/* Two scales, not one sine: the trunk carries everything slowly, the branch beats faster
		 * over it, the leaf faster still. A single global sine reads as a breathing blob. */
		code += "\t" "vec3 windPosition = " + base + ";" "\n";
		code += offsetCode("windPosition", "ubInstanceTransforms.windTimes.x");

		if ( m_previousWindRequired )
		{
			code += "\t" "vec3 previousWindPosition = " + base + ";" "\n";
			code += offsetCode("previousWindPosition", "ubInstanceTransforms.windTimes.y");
		}

		code += "\n";

		return code;
	}

	bool
	AbstractVertexStage::generateMainUniqueInstructions (Generator::Abstract & generator, std::string & topInstructions, std::string & outputInstructions) noexcept
	{
		std::string tempTopInstructions{};

		if ( !this->synthesizeRequestInstructions(generator, tempTopInstructions, outputInstructions) )
		{
			return false;
		}

		if ( !m_uniquePreparations.empty() )
		{
			for ( const auto & instruction : std::ranges::views::values(m_uniquePreparations) )
			{
				topInstructions += instruction;
			}

			topInstructions += '\n';
		}

		topInstructions += tempTopInstructions;

		return true;
	}

	bool
	AbstractVertexStage::isSyntheticVariableAllowed (const char * variableName) noexcept
	{
		constexpr std::array< const char *, 21 > variables{
			ShaderVariable::PositionScreenSpace,
			ShaderVariable::PositionWorldSpace,
			ShaderVariable::GLPositionWorldSpace,
			ShaderVariable::PositionViewSpace,
			ShaderVariable::PositionTextureSpace,
			ShaderVariable::PrimaryVertexColor,
			ShaderVariable::Primary2DTextureCoordinates,
			ShaderVariable::Primary3DTextureCoordinates,
			ShaderVariable::Secondary2DTextureCoordinates,
			ShaderVariable::Secondary3DTextureCoordinates,
			ShaderVariable::TangentWorldSpace,
			ShaderVariable::TangentViewSpace,
			ShaderVariable::BinormalWorldSpace,
			ShaderVariable::BinormalViewSpace,
			ShaderVariable::NormalWorldSpace,
			ShaderVariable::NormalViewSpace,
			ShaderVariable::ModelScale,
			ShaderVariable::RestPositionModelSpace,
			ShaderVariable::WorldTBNMatrix,
			ShaderVariable::ViewTBNMatrix,
			ShaderVariable::TangentToWorldMatrix
		};

		return std::ranges::any_of(variables, [variableName] (const auto & name) {
			return std::strcmp(name, variableName) == 0;
		});
	}

	bool
	AbstractVertexStage::requestSynthesizeInstruction (const char * variableName, VariableScope scope) noexcept
	{
		if ( !AbstractVertexStage::isSyntheticVariableAllowed(variableName) )
		{
			TraceError{ClassId} << "Unable to synthesize '" << variableName << "' variable for " << to_string(this->type()) << " '" << this->name() << "' !";

			return false;
		}

		auto requestFound = false;

		for ( auto & [requestName, requestScope] : m_requests )
		{
			if ( std::strcmp(requestName, variableName) == 0  )
			{
				/* NOTE: The variable is present, but needs to override the scope. */
				if ( requestScope != scope )
				{
					requestScope = VariableScope::Both;
				}

				requestFound = true;
			}
		}

		if ( !requestFound )
		{
			/* NOTE: Prerequisite for special shader variables.
			 * NOTE²: Ignorable Clang-Tidy warning for "Function XXX is within a recursive call chain". */
			if ( std::strcmp(variableName, ShaderVariable::PositionTextureSpace) == 0 )
			{
				if ( !this->requestSynthesizeInstruction(ShaderVariable::PositionViewSpace, VariableScope::Local) )
				{
					return false;
				}

				if ( !this->requestSynthesizeInstruction(ShaderVariable::TangentViewSpace, VariableScope::Local) )
				{
					return false;
				}

				if ( !this->requestSynthesizeInstruction(ShaderVariable::BinormalViewSpace, VariableScope::Local) )
				{
					return false;
				}

				if ( !this->requestSynthesizeInstruction(ShaderVariable::NormalViewSpace, VariableScope::Local) )
				{
					return false;
				}
			}

			m_requests.emplace_back(variableName, scope);
		}

		return true;
	}

	bool
	AbstractVertexStage::generatePerVertexCode (Generator::Abstract & generator, std::stringstream & code, std::string & topInstructions, std::string & outputInstructions) noexcept
	{
		/* ⚠️ BEFORE the unique instructions: these outputs prepare the normal matrix and the model
		 * matrix, and generateMainUniqueInstructions() is what emits every preparation — asked after
		 * it, they would name variables nothing declares. */
		if ( m_heightfieldSurfaceEnabled && m_heightfieldPixelFrameEnabled && !this->declareHeightfieldPixelFrameOutputs(generator, outputInstructions) )
		{
			return false;
		}

		/* NOTE: This will add some declarations and populate m_vertexAttributes. */
		if ( !this->generateMainUniqueInstructions(generator, topInstructions, outputInstructions) )
		{
			return false;
		}

		/* Heightfield surface: the patch point is placed and displaced at the top of main(), before any
		 * synthesis reads hfPosition or its frame. It is its own displacement stage: a heightfield is
		 * never skinned nor blown by the wind. */
		if ( m_heightfieldSurfaceEnabled )
		{
			if ( m_skinningEnabled || m_vegetationWindEnabled )
			{
				Tracer::error(ClassId, "A heightfield surface cannot be skinned or displaced by the wind !");

				return false;
			}

			if ( !this->declare(InputAttribute{VertexAttributeType::Position}) )
			{
				return false;
			}

			topInstructions.insert(0, this->generateHeightfieldSurfaceCode());
		}

		/* Skeletal skinning: compute skinned position/normal/tangent/binormal at the top of main().
		 * This runs AFTER generateMainUniqueInstructions so m_vertexAttributes is fully populated.
		 * The code is PREPENDED to topInstructions so it executes before any synthesis code. */
		size_t displacementPrefixLength = 0;

		if ( m_skinningEnabled )
		{
			const bool hasNormal = m_vertexAttributes.contains(Graphics::VertexAttributeType::Normal);
			const bool hasTangent = m_vertexAttributes.contains(Graphics::VertexAttributeType::Tangent);
			const bool hasBinormal = m_vertexAttributes.contains(Graphics::VertexAttributeType::Binormal);
			const bool needsSkinMatrix3 = hasNormal || hasTangent || hasBinormal;

			/* NOTE: The skinning SSBO interleaves {current, previous} bone matrices
			 * (stride 2) — see RenderableInstance::Abstract::updateSkinningMatrices(). */
			std::string skinCode =
				"\t" "/* Skeletal skinning. */" "\n"
				"\t" "ivec4 boneIdx = ivec4(" + std::string{Keys::Attribute::BoneInfluence} + ") * 2;" "\n"
				"\t" "mat4 skinMatrix = " + std::string{Keys::Attribute::BoneWeight} + ".x * ubSkinningMatrices.bones[boneIdx.x]" "\n"
				"\t" "			   + " + std::string{Keys::Attribute::BoneWeight} + ".y * ubSkinningMatrices.bones[boneIdx.y]" "\n"
				"\t" "			   + " + std::string{Keys::Attribute::BoneWeight} + ".z * ubSkinningMatrices.bones[boneIdx.z]" "\n"
				"\t" "			   + " + std::string{Keys::Attribute::BoneWeight} + ".w * ubSkinningMatrices.bones[boneIdx.w];" "\n"
				"\t" "vec3 skinnedPosition = (skinMatrix * vec4(" + std::string{Keys::Attribute::Position} + ", 1.0)).xyz;" "\n";

			/* Double skinning (velocity): blend the PREVIOUS pose matrices (odd slots). */
			if ( m_previousSkinningRequired )
			{
				skinCode +=
					"\t" "mat4 prevSkinMatrix = " + std::string{Keys::Attribute::BoneWeight} + ".x * ubSkinningMatrices.bones[boneIdx.x + 1]" "\n"
					"\t" "				   + " + std::string{Keys::Attribute::BoneWeight} + ".y * ubSkinningMatrices.bones[boneIdx.y + 1]" "\n"
					"\t" "				   + " + std::string{Keys::Attribute::BoneWeight} + ".z * ubSkinningMatrices.bones[boneIdx.z + 1]" "\n"
					"\t" "				   + " + std::string{Keys::Attribute::BoneWeight} + ".w * ubSkinningMatrices.bones[boneIdx.w + 1];" "\n"
					"\t" "vec3 previousSkinnedPosition = (prevSkinMatrix * vec4(" + std::string{Keys::Attribute::Position} + ", 1.0)).xyz;" "\n";
			}

			if ( needsSkinMatrix3 )
			{
				skinCode += "\t" "mat3 skinMatrix3 = mat3(skinMatrix);" "\n";
			}

			if ( hasNormal )
			{
				skinCode += "\t" "vec3 skinnedNormal = normalize(skinMatrix3 * " + std::string{Keys::Attribute::Normal} + ");" "\n";
			}

			if ( hasTangent )
			{
				skinCode += "\t" "vec3 skinnedTangent = normalize(skinMatrix3 * " + std::string{Keys::Attribute::Tangent} + ");" "\n";
			}

			if ( hasBinormal )
			{
				skinCode += "\t" "vec3 skinnedBinormal = normalize(skinMatrix3 * " + std::string{Keys::Attribute::Binormal} + ");" "\n";
			}

			skinCode += "\n";

			/* Prepend: skinning must execute before any other topInstructions. */
			topInstructions.insert(0, skinCode);

			displacementPrefixLength = skinCode.size();
		}

		/* The wind goes AFTER the skinning prefix and before everything else: it displaces the
		 * skinned position, never the raw attribute. Inserting it at 0 would run it first and
		 * read a variable that does not exist yet. */
		if ( m_vegetationWindEnabled )
		{
			/* The four channels are what the wind reads; a consumer that never asked for the colour
			 * would otherwise leave the attribute undeclared and the shader would not compile. */
			if ( !this->declare(InputAttribute{VertexAttributeType::VertexColor}) )
			{
				return false;
			}

			/* The flutter weight is the leaf card's V. */
			if ( m_vegetationFlutterEnabled && !this->declare(InputAttribute{VertexAttributeType::Primary2DTextureCoordinates}) )
			{
				return false;
			}

			const auto * base = m_skinningEnabled ? "skinnedPosition" : Attribute::Position;

			topInstructions.insert(displacementPrefixLength, this->generateVegetationWindCode(base));
		}

		/* MDI: Declare the buffer_reference struct for per-draw SSBO access via BDA.
		 * Extensions are registered in enableMDI() so they appear in generateHeaders(). */
		if ( m_MDIEnabled )
		{
			code <<
				"\n" "/* MDI per-draw data (BDA buffer reference). */" "\n"
				"layout(buffer_reference, std430) readonly buffer PerDrawDataRef" "\n"
				"{" "\n"
				"\t" "mat4 modelMatrix;" "\n"
				"\t" "uint frameIndex;" "\n"
				"\t" "uint _padding[3];" "\n"
				"};" "\n\n";
		}

		return true;
	}

	void
	AbstractVertexStage::onGetDeclarationStats (std::stringstream & output) const noexcept
	{
		output <<
		 	"Vertex shader input declarations : " "\n"
			" - Input attribute : " << m_inputAttributes.size() << "\n"

			"Vertex shader output declarations : " "\n"
			" - Output block : " <<  m_outputBlocks.size() << "\n"
			" - Stage input : " << m_stageOutputs.size() << "\n";
	}
}
