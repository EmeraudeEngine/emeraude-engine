/*
 * src/Saphir/VertexShader.hpp
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

/* STL inclusions. */
#include <sstream>
#include <string>
#include <utility>

/* Local inclusions for inheritances. */
#include "AbstractVertexStage.hpp"

namespace EmEn::Saphir
{
	/**
	 * @brief The vertex shader class: the per-vertex stage of the classic pipeline, fed by vertex attributes.
	 * @note Everything it synthesizes lives in AbstractVertexStage, shared with the mesh shader; this class
	 * only names the stage.
	 * @extends EmEn::Saphir::AbstractVertexStage The per-vertex stage feeding the rasterizer.
	 * @version 0.9.54
	 */
	class VertexShader final : public AbstractVertexStage
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"VertexShader"};

			/**
			 * @brief Constructs a vertex shader.
			 * @param name The name of the shader for identification [std::move].
			 * @param GLSLVersion A reference to a string [std::move].
			 * @param GLSLProfile A reference to a string [std::move].
			 */
			VertexShader (std::string name, std::string GLSLVersion, std::string GLSLProfile) noexcept
				: AbstractVertexStage{std::move(name), std::move(GLSLVersion), std::move(GLSLProfile)}
			{

			}

			/** @copydoc EmEn::Saphir::AbstractShader::type() */
			[[nodiscard]]
			ShaderType
			type () const noexcept override
			{
				return ShaderType::VertexShader;
			}

		private:

			/** @copydoc EmEn::Saphir::AbstractShader::onSourceCodeGeneration() */
			[[nodiscard]]
			bool onSourceCodeGeneration (Generator::Abstract & generator, std::stringstream & code, std::string & topInstructions, std::string & outputInstructions) noexcept override;
	};
}
