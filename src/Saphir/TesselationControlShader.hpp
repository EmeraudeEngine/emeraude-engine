/*
 * src/Saphir/TesselationControlShader.hpp
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
}

namespace EmEn::Saphir
{
	/**
	 * @brief The TesselationControlShader class.
	 * @extends EmEn::Saphir::AbstractShader The base class of every shader type.
	 */
	class TesselationControlShader final : public AbstractShader
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"TesselationControlShader"};

			/** @brief Extends declaration from base class. */
			using AbstractShader::declare;

			/**
			 * @brief Constructs a tesselation control shader.
			 * @param name The name of the shader for identification [std::move].
			 * @param GLSLVersion A reference to a string [std::move].
			 * @param GLSLProfile A reference to a string [std::move].
			 */
			TesselationControlShader (std::string name, std::string GLSLVersion, std::string GLSLProfile) noexcept
				: AbstractShader{std::move(name), std::move(GLSLVersion), std::move(GLSLProfile)}
			{

			}

			/** @copydoc EmEn::Saphir::AbstractShader::type() */
			[[nodiscard]]
			ShaderType
			type () const noexcept override
			{
				return ShaderType::TesselationControlShader;
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
			 * @brief Declares an input primitive to be used in a geometry shader.
			 * @param declaration A reference to a ShaderInputPrimitive.
			 * @return bool
			 */
			bool declare (const Declaration::InputPrimitive & declaration) noexcept;

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
			 * @brief Declares an output primitive to be used in the shader.
			 * @param declaration A reference to a ShaderOutputPrimitive.
			 * @return bool
			 */
			bool declare (const Declaration::OutputPrimitive & declaration) noexcept;

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
			 * @brief Returns the list of input primitive declarations.
			 * @return const std::vector< Declaration::InputPrimitive > &
			 */
			[[nodiscard]]
			const std::vector< Declaration::InputPrimitive > &
			inputPrimitives () const noexcept
			{
				return m_inputPrimitives;
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
			 * @brief Returns the list of output primitive declarations.
			 * @return const std::vector< Declaration::OutputPrimitive > &
			 */
			[[nodiscard]]
			const std::vector< Declaration::OutputPrimitive > &
			outputPrimitives () const noexcept
			{
				return m_outputPrimitives;
			}

			/**
			 * @brief Copies output from a vertex shader to this fragment shader.
			 * @param vertexShader A reference to a vertex shader.
			 * @return bool
			 */
			[[nodiscard]]
			bool connectFromPreviousShader (const VertexShader & vertexShader) noexcept;

		private:

			/** @copydoc EmEn::Saphir::AbstractShader::onSourceCodeGeneration() */
			[[nodiscard]]
			bool onSourceCodeGeneration (Generator::Abstract & generator, std::stringstream & code, std::string & topInstructions, std::string & outputInstructions) noexcept override;

			/** @copydoc EmEn::Saphir::AbstractShader::onGetDeclarationStats() */
			void onGetDeclarationStats (std::stringstream & output) const noexcept override;

			/* Tesselation control shader inputs. */
			std::vector< Declaration::StageInput > m_stageInputs{};
			std::vector< Declaration::InputBlock > m_inputBlocks{};
			std::vector< Declaration::InputPrimitive > m_inputPrimitives{};
			/* Tesselation control shader outputs. */
			std::vector< Declaration::StageOutput > m_stageOutputs{};
			std::vector< Declaration::OutputBlock > m_outputBlocks{};
			std::vector< Declaration::OutputPrimitive > m_outputPrimitives{};
	};
}
