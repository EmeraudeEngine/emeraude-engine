/*
 * src/Saphir/MeshShader.hpp
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
#include <vector>

/* Local inclusions for inheritances. */
#include "AbstractShader.hpp"

/* Local inclusions for usages. */
#include "Declaration/StageOutput.hpp"

namespace EmEn::Saphir
{
	/** @brief The primitive a mesh shader emits. */
	enum class MeshOutputTopology : uint8_t
	{
		Points,
		Lines,
		Triangles
	};

	/**
	 * @brief The mesh shader of a mesh-shading program (VK_EXT_mesh_shader, OPTIONAL).
	 * @details It replaces the vertex stage (and the input assembly): each workgroup calls
	 * SetMeshOutputsEXT(vertexCount, primitiveCount), writes `gl_MeshVerticesEXT[i].gl_Position` and the
	 * primitive indices (`gl_PrimitiveTriangleIndicesEXT` for triangles), and its per-vertex outputs are
	 * ARRAYS indexed by the vertex (declare them with an array size of -1: `out vec3 name[]`). The fragment
	 * stage receives them as plain inputs (FragmentShader::connectFromPreviousShader(const MeshShader &)).
	 * @warning Only usable when Vulkan::Device::meshShadersEnabled(). Stay under the device's ceilings:
	 * PhysicalDevice::meshShaderProperties() (maxMeshOutputVertices, maxMeshOutputPrimitives,
	 * maxMeshWorkGroupSize...). A consumer keeps a classic vertex path otherwise (MoltenVK: never, 2026-09-22).
	 * @extends EmEn::Saphir::AbstractShader
	 */
	class MeshShader final : public AbstractShader
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"MeshShader"};

			using AbstractShader::declare;

			/**
			 * @brief Constructs a mesh shader.
			 * @param name The shader name.
			 * @param GLSLVersion The GLSL version.
			 * @param GLSLProfile The GLSL profile.
			 * @param topology The emitted primitive.
			 * @param maxVertices The most vertices a workgroup emits.
			 * @param maxPrimitives The most primitives a workgroup emits.
			 * @param workgroupSize The local workgroup size (X, Y, Z).
			 */
			MeshShader (std::string name, std::string GLSLVersion, std::string GLSLProfile, MeshOutputTopology topology, uint32_t maxVertices, uint32_t maxPrimitives, const std::array< uint32_t, 3 > & workgroupSize) noexcept;

			/** @copydoc EmEn::Saphir::AbstractShader::type() const */
			[[nodiscard]]
			ShaderType
			type () const noexcept override
			{
				return ShaderType::MeshShader;
			}

			/**
			 * @brief Declares an output to the fragment stage: per vertex (an array of size -1), or per
			 * primitive (interpolation "perprimitiveEXT", also an array).
			 * @param declaration A reference to a stage output.
			 * @return bool
			 */
			bool declare (const Declaration::StageOutput & declaration) noexcept;

			/**
			 * @brief Sets the task payload declaration read by this stage (same text as TaskShader::setTaskPayload()).
			 * @param declaration The GLSL declaration, emitted verbatim at global scope.
			 * @return void
			 */
			void
			setTaskPayload (std::string declaration) noexcept
			{
				m_taskPayload = std::move(declaration);
			}

			/**
			 * @brief Returns the stage outputs.
			 * @return const std::vector< Declaration::StageOutput > &
			 */
			[[nodiscard]]
			const std::vector< Declaration::StageOutput > &
			stageOutputs () const noexcept
			{
				return m_stageOutputs;
			}

			/**
			 * @brief Returns the emitted primitive.
			 * @return MeshOutputTopology
			 */
			[[nodiscard]]
			MeshOutputTopology
			topology () const noexcept
			{
				return m_topology;
			}

			/**
			 * @brief Returns the most vertices a workgroup emits.
			 * @return uint32_t
			 */
			[[nodiscard]]
			uint32_t
			maxVertices () const noexcept
			{
				return m_maxVertices;
			}

			/**
			 * @brief Returns the most primitives a workgroup emits.
			 * @return uint32_t
			 */
			[[nodiscard]]
			uint32_t
			maxPrimitives () const noexcept
			{
				return m_maxPrimitives;
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

			std::vector< Declaration::StageOutput > m_stageOutputs;
			std::string m_taskPayload;
			std::array< uint32_t, 3 > m_workgroupSize{32, 1, 1};
			uint32_t m_maxVertices{0};
			uint32_t m_maxPrimitives{0};
			MeshOutputTopology m_topology{MeshOutputTopology::Triangles};
	};
}
