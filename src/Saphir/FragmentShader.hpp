/*
 * src/Saphir/FragmentShader.hpp
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
#include "Declaration/OutputFragment.hpp"
#include "Declaration/StageInput.hpp"

/* Forward declarations. */
namespace EmEn::Saphir
{
	class VertexShader;
	class TesselationEvaluationShader;
	class GeometryShader;
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
			 * @brief Declares a fragment output to be used in a fragment shader.
			 * @param declaration A reference to a OutputBlock.
			 * @return bool
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

			/**
			 * @brief Copies output from a geometry shader to this fragment shader.
			 * @param geometryShader A reference to a geometry shader.
			 * @return bool
			 */
			[[nodiscard]]
			bool connectFromPreviousShader (const GeometryShader & geometryShader) noexcept;

			[[nodiscard]]
			static Declaration::Function generateToSRGBColorFunction () noexcept;

			[[nodiscard]]
			static Declaration::Function generateToLinearColorFunction () noexcept;

		private:

			/** @copydoc EmEn::Saphir::AbstractShader::onSourceCodeGeneration() */
			[[nodiscard]]
			bool onSourceCodeGeneration (Generator::Abstract & generator, std::stringstream & code, std::string & topInstructions, std::string & outputInstructions) noexcept override;

			/** @copydoc EmEn::Saphir::AbstractShader::onGetDeclarationStats() */
			void onGetDeclarationStats (std::stringstream & output) const noexcept override;

			std::vector< Declaration::StageInput > m_stageInputs;
			std::vector< Declaration::InputBlock > m_inputBlocks;
			std::vector< Declaration::OutputFragment > m_outputFragments;
			uint32_t m_samples{1};
	};
}
