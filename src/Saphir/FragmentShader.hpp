/*
 * src/Saphir/FragmentShader.hpp
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

#pragma once

/* Local inclusions for inheritances. */
#include "AbstractShader.hpp"

/* Local inclusions for usages. */
#include "Declaration/InputBlock.hpp"
#include "Declaration/OutputFragment.hpp"
#include "Declaration/StageInput.hpp"

/* Forward declarations. */
namespace EmEn::Saphir
{
	class VertexShader;
	class TesselationEvaluationShader;
	class GeometryShader;
	class MeshShader;
	class AbstractVertexStage;
}

namespace EmEn::Saphir
{
	/**
	 * @brief The fragment shader class.
	 * @extends EmEn::Saphir::AbstractShader The base class of every shader type.
	 */
	class FragmentShader final : public AbstractShader
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"FragmentShader"};

			/** @brief Extends declaration from base class. */
			using AbstractShader::declare;

			/**
			 * @brief Constructs a fragment shader.
			 * @param name The name of the shader for identification [std::move].
			 * @param GLSLVersion A reference to a string [std::move].
			 * @param GLSLProfile A reference to a string [std::move].
			 */
			FragmentShader (std::string name, std::string GLSLVersion, std::string GLSLProfile) noexcept
				: AbstractShader{std::move(name), std::move(GLSLVersion), std::move(GLSLProfile)}
			{

			}

			/** @copydoc EmEn::Saphir::AbstractShader::type() */
			[[nodiscard]]
			ShaderType
			type () const noexcept override
			{
				return ShaderType::FragmentShader;
			}

			/**
			 * @brief Declares a stage input variable to be used in the shader.
			 * @note If a stage input with the same name is already declared, this is not an error: the call is
			 * a no-op and still returns true.
			 * @param declaration A reference to a ShaderStageInput.
			 * @return bool False only if the declaration itself is invalid.
			 */
			bool declare (const Declaration::StageInput & declaration) noexcept;

			/**
			 * @brief Declares an input block to be used in the shader.
			 * @note If an input block with the same instance name is already declared, this is not an error: the
			 * call is a no-op and still returns true.
			 * @param declaration A reference to a InputBlock.
			 * @return bool False only if the declaration itself is invalid.
			 */
			bool declare (const Declaration::InputBlock & declaration) noexcept;

			/**
			 * @brief Declares a fragment output to be used in a fragment shader.
			 * @note If an output fragment with the same name is already declared, this is not an error: the call
			 * is a no-op and still returns true.
			 * @param declaration A reference to a OutputBlock.
			 * @return bool False only if the declaration itself is invalid.
			 */
			bool declare (const Declaration::OutputFragment & declaration) noexcept;

			/**
			 * @brief Returns the list of stage input declarations.
			 * @return const std::vector< Declaration::StageInput > &
			 */
			[[nodiscard]]
			const std::vector< Declaration::StageInput > &
			stageInputs () const noexcept
			{
				return m_stageInputs;
			}

			/**
			 * @brief Returns the list of input block declarations.
			 * @return const std::vector< Declaration::InputBlock > &
			 */
			[[nodiscard]]
			const std::vector< Declaration::InputBlock > &
			inputBlocks () const noexcept
			{
				return m_inputBlocks;
			}

			/**
			 * @brief Returns the list of output fragment declarations.
			 * @return const std::vector< Declaration::OutputFragment > &
			 */
			[[nodiscard]]
			const std::vector< Declaration::OutputFragment > &
			outputFragments () const noexcept
			{
				return m_outputFragments;
			}

			/**
			 * @brief Declares the default fragment output.
			 * @return bool
			 */
			bool
			declareDefaultOutputFragment () noexcept
			{
				return this->declare(Declaration::OutputFragment{0, Keys::GLSL::FloatVector4, Keys::ShaderVariable::OutputFragment});
			}

			/**
			 * @brief Sets the number of samples used by the frame buffer for a pixel.
			 * @param samples
			 * @return void
			 */
			void
			setSamples (uint32_t samples) noexcept
			{
				m_samples = samples;
			}

			/**
			 * @brief Returns the number of samples used by the frame buffer for a pixel.
			 * @return uint32_t
			 */
			[[nodiscard]]
			uint32_t
			samples () const noexcept
			{
				return m_samples;
			}

			/**
			 * @brief Rebuilds the SURFACE FRAME per pixel from the heightfield's normal clipmap instead of
			 * interpolating the vertex stage's.
			 * @note Every frame variable the vertex stage hands over (normal, tangent, binormal and the three
			 * TBN matrices, in world or view space) is received under another name, and main() opens by
			 * defining the canonical name from the normal of THIS pixel — so the material code, which reads
			 * the canonical names, is untouched. Vulkan matches stage interfaces by LOCATION, never by name,
			 * which is what makes the rename free. The normal comes from the finest clip level that covers
			 * the pixel and is not finer than its footprint (Graphics::Geometry::HeightfieldSurface): a
			 * distant ridge is lit by its filtered normal, not by the 1 m one of the vertex that happens to
			 * be there. Requires VertexShader::enableHeightfieldPixelFrame() and the surface set declared
			 * in this stage (Generator::declareHeightfieldSurface()).
			 * @return void
			 */
			void
			enableHeightfieldPixelFrame () noexcept
			{
				m_heightfieldPixelFrameEnabled = true;
			}

			/**
			 * @brief Copies output from a vertex shader to this fragment shader.
			 * @warning The vertex shader must already have its source code generated (@c isGenerated() must be
			 * true), otherwise this fails. A vertex shader with no stage output and no output block (i.e. it only
			 * writes @c gl_Position) is a valid, non-error case and this method simply returns true without
			 * declaring anything.
			 * @param vertexShader A reference to a vertex shader.
			 * @return bool False if the vertex shader is not generated yet.
			 */
			[[nodiscard]]
			bool connectFromPreviousShader (const VertexShader & vertexShader) noexcept;

			/**
			 * @brief Copies output from a tesselation evaluation shader to this fragment shader.
			 * @warning The tesselation evaluation shader must already have its source code generated
			 * (@c isGenerated() must be true), otherwise this fails. Unlike the vertex shader overload, a
			 * tesselation evaluation shader with no stage output and no output block is treated as an error here.
			 * @param tesselationEvaluationShader A reference to a tesselation evaluation shader.
			 * @return bool False if the shader is not generated yet, or has no output declaration at all.
			 */
			[[nodiscard]]
			bool connectFromPreviousShader (const TesselationEvaluationShader & tesselationEvaluationShader) noexcept;

			/**
			 * @brief Copies the outputs of a mesh shader to this fragment shader.
			 * @note A mesh output is an ARRAY indexed by vertex (or by primitive); the matching fragment input
			 * is the plain variable, same location, same qualifier (perprimitiveEXT included).
			 * @param meshShader A reference to a generated mesh shader.
			 * @return bool False if the mesh shader is not generated yet.
			 */
			[[nodiscard]]
			bool connectFromPreviousShader (const MeshShader & meshShader) noexcept;

			/**
			 * @brief Copies output from a geometry shader to this fragment shader.
			 * @warning The geometry shader must already have its source code generated (@c isGenerated() must be
			 * true), otherwise this fails. Unlike the vertex shader overload, a geometry shader with no stage
			 * output and no output block is treated as an error here.
			 * @param geometryShader A reference to a geometry shader.
			 * @return bool False if the shader is not generated yet, or has no output declaration at all.
			 */
			[[nodiscard]]
			bool connectFromPreviousShader (const GeometryShader & geometryShader) noexcept;

			/**
			 * @brief Generates the GLSL function declaration converting a linear RGB color to sRGB.
			 * @note Only the RGB components are converted; the alpha channel is passed through unchanged, as it
			 * is never gamma-encoded.
			 * @return Declaration::Function The generated function, named "toSRGBColor", taking a vec4 and
			 * returning a vec4.
			 */
			[[nodiscard]]
			static Declaration::Function generateToSRGBColorFunction () noexcept;

			/**
			 * @brief Generates the GLSL function declaration converting an sRGB color to linear RGB.
			 * @note Only the RGB components are converted; the alpha channel is passed through unchanged, as it
			 * is never gamma-encoded.
			 * @return Declaration::Function The generated function, named "toLinearColor", taking a vec4 and
			 * returning a vec4.
			 */
			[[nodiscard]]
			static Declaration::Function generateToLinearColorFunction () noexcept;

		private:

			/**
			 * @brief Receives the outputs of a per-vertex stage (vertex or mesh shader): what both overloads share.
			 * @param perVertexStage A reference to the per-vertex stage.
			 * @return bool
			 */
			[[nodiscard]]
			bool connectFromPerVertexStage (const AbstractVertexStage & perVertexStage) noexcept;

			/** @copydoc EmEn::Saphir::AbstractShader::onSourceCodeGeneration() */
			[[nodiscard]]
			bool onSourceCodeGeneration (Generator::Abstract & generator, std::stringstream & code, std::string & topInstructions, std::string & outputInstructions) noexcept override;

			/** @copydoc EmEn::Saphir::AbstractShader::onGetDeclarationStats() */
			void onGetDeclarationStats (std::stringstream & output) const noexcept override;

			std::vector< Declaration::StageInput > m_stageInputs;
			std::vector< Declaration::InputBlock > m_inputBlocks;
			std::vector< Declaration::OutputFragment > m_outputFragments;
			std::vector< const char * > m_heightfieldOverrides; ///< Canonical frame variables received renamed (enableHeightfieldPixelFrame()).
			uint32_t m_samples{1};
			bool m_heightfieldPixelFrameEnabled{false};
	};
}
