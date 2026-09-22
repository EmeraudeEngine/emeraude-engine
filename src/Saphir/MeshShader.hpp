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
#include <list>
#include <set>
#include <sstream>
#include <string>

/* Local inclusions for inheritances. */
#include "AbstractVertexStage.hpp"

/* Local inclusions for usages. */
#include "Graphics/Types.hpp"

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
	 * @brief The mesh shader of a mesh-shading program (VK_EXT_mesh_shader, OPTIONAL): the per-vertex stage
	 * that builds its own geometry.
	 * @details It IS an AbstractVertexStage: the material, light and shadow generators address it exactly as
	 * they address a vertex shader, and every synthetic variable they request is produced by the same code.
	 * What differs is the frame around that code, generated here:
	 * - the workgroup calls SetMeshOutputsEXT(vertexCount, primitiveCount), then loops over its vertices
	 *   (`msVertexIndex`, strided by the workgroup size);
	 * - each iteration runs the VERTEX SOURCE's prologue, which must define as locals the vertex attributes
	 *   the synthesis reads, under their attribute names (`vaPosition`, `vaNormal`...) — the attributes a
	 *   vertex shader would declare `in` are provided, not fetched;
	 * - then the synthesized per-vertex code, whose outputs are locals of their canonical names, copied at
	 *   the end of the iteration into per-vertex arrays at the SAME locations (Vulkan links stages by location:
	 *   the fragment stage is connected exactly as from a vertex shader);
	 * - the clip position is a local (`msPosition`) copied into `gl_MeshVerticesEXT[i].gl_Position`;
	 * - then the PRIMITIVE SOURCE runs once per primitive (`msPrimitiveIndex`).
	 * @warning Only usable when Vulkan::Device::meshShadersEnabled(). Stay under the device's ceilings:
	 * PhysicalDevice::meshShaderProperties() (maxMeshOutputVertices, maxMeshOutputPrimitives,
	 * maxMeshWorkGroupSize...). A consumer keeps a classic vertex path otherwise (MoltenVK: never, 2026-09-22).
	 * Not supported here: vertex attributes the source does not provide (reported), MDI, instancing
	 * attributes, per-primitive outputs.
	 * @extends EmEn::Saphir::AbstractVertexStage The per-vertex stage feeding the rasterizer.
	 */
	class MeshShader final : public AbstractVertexStage
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"MeshShader"};

			/** @brief The loop variable of the per-vertex iteration, readable by the vertex source's prologue. */
			static constexpr auto VertexIndex{"msVertexIndex"};
			/** @brief The loop variable of the per-primitive iteration, readable by the primitive source. */
			static constexpr auto PrimitiveIndex{"msPrimitiveIndex"};
			/** @brief The local receiving the clip-space position (AbstractVertexStage::setPositionOutput()). */
			static constexpr auto PositionOutput{"msPosition"};

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
			 * @brief Sets what the workgroup's vertices are.
			 * @param countExpression A GLSL expression, uniform across the workgroup: the vertex count.
			 * @param prologue GLSL run first in each vertex iteration (VertexIndex is defined): it must define every
			 * attribute of @p providedAttributes as a local of its attribute name.
			 * @param providedAttributes The vertex attributes the prologue defines. Generation fails, naming the
			 * attribute, when the synthesis needs one that is not in this set.
			 * @return void
			 */
			void setVertexSource (std::string countExpression, std::string prologue, std::set< Graphics::VertexAttributeType > providedAttributes) noexcept;

			/**
			 * @brief Sets what the workgroup's primitives are.
			 * @param countExpression A GLSL expression, uniform across the workgroup: the primitive count.
			 * @param code GLSL run once per primitive (PrimitiveIndex is defined), writing the primitive indices
			 * (`gl_PrimitiveTriangleIndicesEXT[msPrimitiveIndex]` for triangles).
			 * @return void
			 */
			void setPrimitiveSource (std::string countExpression, std::string code) noexcept;

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

			/**
			 * @brief Keeps a generated name alive for the declarations that reference it by Key.
			 * @param name The name [std::move].
			 * @return Key
			 */
			[[nodiscard]]
			Key storeName (std::string name) noexcept;

			/** @brief Generated names referenced by Key (a list: an element never moves). */
			std::list< std::string > m_generatedNames;
			std::set< Graphics::VertexAttributeType > m_providedAttributes;
			std::string m_taskPayload;
			std::string m_vertexCountExpression;
			std::string m_vertexPrologue;
			std::string m_primitiveCountExpression;
			std::string m_primitiveCode;
			std::array< uint32_t, 3 > m_workgroupSize{32, 1, 1};
			uint32_t m_maxVertices{0};
			uint32_t m_maxPrimitives{0};
			MeshOutputTopology m_topology{MeshOutputTopology::Triangles};
	};
}
