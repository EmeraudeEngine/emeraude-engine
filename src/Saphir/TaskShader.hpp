/*
 * src/Saphir/TaskShader.hpp
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
#include <array>
#include <cstdint>
#include <string>

/* Local inclusions for inheritances. */
#include "AbstractShader.hpp"

namespace EmEn::Saphir
{
	/**
	 * @brief The task (amplification) shader of a mesh-shading program (VK_EXT_mesh_shader, OPTIONAL).
	 * @details It runs before the mesh stage, one workgroup per dispatched group, and launches mesh
	 * workgroups with EmitMeshTasksEXT() — typically after culling a cluster. What it hands the mesh
	 * stage is the TASK PAYLOAD, a `taskPayloadSharedEXT` variable declared identically in both stages
	 * (setTaskPayload()).
	 * @warning Only usable when Vulkan::Device::meshShadersEnabled() and the device reports the task stage
	 * (PhysicalDevice::meshShaderFeatures().taskShader). A consumer keeps a classic path otherwise.
	 * @extends EmEn::Saphir::AbstractShader
	 */
	class TaskShader final : public AbstractShader
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"TaskShader"};

			using AbstractShader::declare;

			/**
			 * @brief Constructs a task shader.
			 * @param name The shader name.
			 * @param GLSLVersion The GLSL version.
			 * @param GLSLProfile The GLSL profile.
			 * @param workgroupSize The local workgroup size (X, Y, Z).
			 */
			TaskShader (std::string name, std::string GLSLVersion, std::string GLSLProfile, const std::array< uint32_t, 3 > & workgroupSize) noexcept;

			/** @copydoc EmEn::Saphir::AbstractShader::type() const */
			[[nodiscard]]
			ShaderType
			type () const noexcept override
			{
				return ShaderType::TaskShader;
			}

			/**
			 * @brief Sets the task payload declaration, e.g. "taskPayloadSharedEXT struct { uint ids[32]; } payload;".
			 * @note The mesh shader of the same program must declare the SAME text (MeshShader::setTaskPayload()).
			 * @param declaration The GLSL declaration, emitted verbatim at global scope.
			 * @return void
			 */
			void
			setTaskPayload (std::string declaration) noexcept
			{
				m_taskPayload = std::move(declaration);
			}

			/**
			 * @brief Returns the local workgroup size.
			 * @return const std::array< uint32_t, 3 > &
			 */
			[[nodiscard]]
			const std::array< uint32_t, 3 > &
			workgroupSize () const noexcept
			{
				return m_workgroupSize;
			}

		private:

			/** @copydoc EmEn::Saphir::AbstractShader::onSourceCodeGeneration() */
			[[nodiscard]]
			bool onSourceCodeGeneration (Generator::Abstract & generator, std::stringstream & code, std::string & topInstructions, std::string & outputInstructions) noexcept override;

			/** @copydoc EmEn::Saphir::AbstractShader::onGetDeclarationStats() */
			void onGetDeclarationStats (std::stringstream & output) const noexcept override;

			std::string m_taskPayload;
			std::array< uint32_t, 3 > m_workgroupSize{1, 1, 1};
	};
}
