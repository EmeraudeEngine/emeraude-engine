/*
 * src/Saphir/GeometryShader.hpp
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

/* Local inclusions for inheritances. */
#include "AbstractShader.hpp"

/* Local inclusions for usages. */
#include "Declaration/InputBlock.hpp"
#include "Declaration/InputPrimitive.hpp"
#include "Declaration/OutputBlock.hpp"
#include "Declaration/OutputPrimitive.hpp"
#include "Declaration/StageInput.hpp"
#include "Declaration/StageOutput.hpp"

/* Forward declarations. */
namespace EmEn::Saphir
{
	class VertexShader;
	class TesselationEvaluationShader;
}

namespace EmEn::Saphir
{
	/**
	 * @brief The geometry shader class.
	 * @extends EmEn::Saphir::AbstractShader The base class of every shader type.
	 */
	class GeometryShader final : public AbstractShader
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"GeometryShader"};

			/** @brief Extends declaration from base class. */
			using AbstractShader::declare;

			/**
			 * @brief Constructs a geometry shader.
			 * @param name The name of the shader for identification [std::move].
			 * @param inputPrimitive An input primitive declaration [std::move].
			 * @param outputPrimitive An output primitive declaration [std::move].
			 * @param GLSLVersion The GLSL version in use [std::move].
			 * @param GLSLProfile The GLSL profile in use [std::move].
			 */
			GeometryShader (std::string name, Declaration::InputPrimitive inputPrimitive, Declaration::OutputPrimitive outputPrimitive, std::string GLSLVersion, std::string GLSLProfile) noexcept
				: AbstractShader(std::move(name), std::move(GLSLVersion), std::move(GLSLProfile)),
				m_inputPrimitive(std::move(inputPrimitive)),
				m_outputPrimitive(std::move(outputPrimitive))
			{

			}

			/** @copydoc EmEn::Saphir::AbstractShader::type() */
			[[nodiscard]]
			ShaderType
			type () const noexcept override
			{
				return ShaderType::GeometryShader;
			}

			/**
			 * @brief Declares a stage input variable to be used in the shader.
			 * @param declaration A reference to a ShaderStageInput.
			 * @return bool
			 */
			bool declare (const Declaration::StageInput & declaration) noexcept;

			/**
			 * @brief Declares an input block to be used in the shader.
			 * @param declaration A reference to a InputBlock.
			 * @return bool
			 */
			bool declare (const Declaration::InputBlock & declaration) noexcept;

			/**
			 * @brief Declares a stage output variable to be used in the shader.
			 * @param declaration A reference to a ShaderStageOutput.
			 * @return bool
			 */
			bool declare (const Declaration::StageOutput & declaration) noexcept;

			/**
			 * @brief Declares an output block to be used in the shader.
			 * @param declaration A reference to a OutputBlock.
			 * @return bool
			 */
			bool declare (const Declaration::OutputBlock & declaration) noexcept;

			/**
			 * @brief Returns the input primitive declaration.
			 * @return const Declaration::InputPrimitive &
			 */
			[[nodiscard]]
			const Declaration::InputPrimitive &
			inputPrimitive () const noexcept
			{
				return m_inputPrimitive;
			}

			/**
			 * @brief Returns the output primitive declaration.
			 * @return const Declaration::OutputPrimitive &
			 */
			[[nodiscard]]
			const Declaration::OutputPrimitive &
			outputPrimitive () const noexcept
			{
				return m_outputPrimitive;
			}

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
			 * @brief Returns the list of stage output declarations.
			 * @return const std::vector< Declaration::StageOutput > &
			 */
			[[nodiscard]]
			const std::vector< Declaration::StageOutput > &
			stageOutputs () const noexcept
			{
				return m_stageOutputs;
			}

			/**
			 * @brief Returns the list of output block declarations.
			 * @return const std::vector< Declaration::OutputBlock > &
			 */
			[[nodiscard]]
			const std::vector< Declaration::OutputBlock > &
			outputBlocks () const noexcept
			{
				return m_outputBlocks;
			}

			/**
			 * @brief Copies output from a vertex shader to this fragment shader.
			 * @param vertexShader A reference to a vertex shader.
			 * @return bool
			 */
			[[nodiscard]]
			bool connectFromPreviousShader (const VertexShader & vertexShader) noexcept;

			/**
			 * @brief Copies output from a tesselation evaluation shader to this fragment shader.
			 * @param tesselationEvaluationShader A reference to a tesselation evaluation shader.
			 * @return bool
			 */
			[[nodiscard]]
			bool connectFromPreviousShader (const TesselationEvaluationShader & tesselationEvaluationShader) noexcept;

		private:

			/** @copydoc EmEn::Saphir::AbstractShader::onSourceCodeGeneration() */
			[[nodiscard]]
			bool onSourceCodeGeneration (Generator::Abstract & generator, std::stringstream & code, std::string & topInstructions, std::string & outputInstructions) noexcept override;

			/** @copydoc EmEn::Saphir::AbstractShader::onGetDeclarationStats() */
			void onGetDeclarationStats (std::stringstream & output) const noexcept override;

			Declaration::InputPrimitive m_inputPrimitive;
			Declaration::OutputPrimitive m_outputPrimitive;
			std::vector< Declaration::StageInput > m_stageInputs;
			std::vector< Declaration::InputBlock > m_inputBlocks;
			std::vector< Declaration::StageOutput > m_stageOutputs;
			std::vector< Declaration::OutputBlock > m_outputBlocks;
	};
}
