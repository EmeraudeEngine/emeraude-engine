/*
 * src/Saphir/Program.hpp
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

/* STL inclusions. */
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

/* Local inclusions for inheritances. */
#include "Libs/NameableTrait.hpp"

/* Local inclusions for usages. */
#include "Graphics/VertexBufferFormatManager.hpp"
#include "Vulkan/GraphicsPipeline.hpp"
#include "VertexShader.hpp"
#include "TesselationControlShader.hpp"
#include "TesselationEvaluationShader.hpp"
#include "GeometryShader.hpp"
#include "FragmentShader.hpp"
#include "SetIndexes.hpp"
#include "Types.hpp"

namespace EmEn::Saphir
{
	/**
	 * @brief The program class.
	 * @note This will contain all the necessary shaders to build a program like OpenGL.
	 * @extends EmEn::Libs::NameableTrait This is a nameable class.
	 */
	class Program final : public Libs::NameableTrait
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"Program"};

			/** 
			 * @brief Constructs a graphics shader container.
			 * @param name A reference to a string.
			 * @param GLSLVersion The GLSL version in use [std::move].
			 * @param GLSLProfile The GLSL profile in use [std::move].
			 */
			Program (const std::string & name, std::string GLSLVersion, std::string GLSLProfile) noexcept;

			/**
			 * @brief Returns whether the program has shader source codes.
			 * @return bool
			 */
			[[nodiscard]]
			bool isComplete () const noexcept;

			/**
			 * @brief Returns whether the program shaders are compiled.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isCompiled () const noexcept
			{
				return m_graphicsPipeline != nullptr;
			}

			/**
			 * @brief Returns whether the program use tesselation technics.
			 * @return bool
			 */
			[[nodiscard]]
			bool useTesselation () const noexcept;

			/**
			 * @brief Returns whether the program needed instancing.
			 * @return
			 */
			[[nodiscard]]
			bool wasInstancingEnabled () const noexcept;

			/**
			 * @brief Returns whether the rendering needed more specific matrices.
			 * @return bool
			 */
			[[nodiscard]]
			bool wasAdvancedMatricesEnabled () const noexcept;

			/**
			 * @brief Returns whether the rendering of bill boards was enabled.
			 * @return bool
			 */
			[[nodiscard]]
			bool wasBillBoardingEnabled () const noexcept;

			/**
			 * @brief Initializes the vertex shader and returns it.
			 * @param name A reference to a string.
			 * @param enableInstancing Enable instancing for the vertex shader.
			 * @param enableAdvancedMatrices Enable the advanced matrices for the vertex shader.
			 * @param enableBillBoarding Enable the render of bill boards instead of classic geometry.
			 * @return VertexShader *
			 */
			[[nodiscard]]
			VertexShader * initVertexShader (const std::string & name, bool enableInstancing, bool enableAdvancedMatrices, bool enableBillBoarding) noexcept;

			/**
			 * @brief Initializes the tesselation control shader and returns it.
			 * @param name A reference to a string.
			 * @return TesselationControlShader *
			 */
			[[nodiscard]]
			TesselationControlShader * initTesselationControlShader (const std::string & name) noexcept;

			/**
			 * @brief Initializes the tesselation evaluation shader and returns it.
			 * @param name A reference to a string.
			 * @return TesselationEvaluationShader *
			 */
			[[nodiscard]]
			TesselationEvaluationShader * initTesselationEvaluationShader (const std::string & name) noexcept;

			/**
			 * @brief Initializes the geometry shader and returns it.
			 * @param name A reference to a string.
			 * @param inputPrimitive A reference to an input primitive declaration.
			 * @param outputPrimitive A reference to an output primitive declaration.
			 * @return GeometryShader *
			 */
			[[nodiscard]]
			GeometryShader * initGeometryShader (const std::string & name, const Declaration::InputPrimitive & inputPrimitive, const Declaration::OutputPrimitive & outputPrimitive) noexcept;

			/**
			 * @brief Initializes the fragment shader.
			 * @param name A reference to a string.
			 * @return FragmentShader *
			 */
			[[nodiscard]]
			FragmentShader * initFragmentShader (const std::string & name) noexcept;

			/**
			 * @brief Returns the last shader generated type.
			 * @return ShaderType
			 */
			[[nodiscard]]
			ShaderType lastShaderStageType () const noexcept;

			/**
			 * @brief Returns the set indexes structure.
			 * @return const SetIndexes &
			 */
			[[nodiscard]]
			const SetIndexes &
			setIndexes () const noexcept
			{
				return m_setIndexes;
			}

			/**
			 * @brief Returns the set indexes structure.
			 * @return const SetIndexes &
			 */
			[[nodiscard]]
			SetIndexes &
			setIndexes () noexcept
			{
				return m_setIndexes;
			}

			/**
			 * @brief Returns a specific set index.
			 * @param setType The set type.
			 * @return uint32_t
			 */
			[[nodiscard]]
			uint32_t
			setIndex (SetType setType) const noexcept
			{
				return m_setIndexes.set(setType);
			}

			/**
			 * @brief Returns whether a vertex shader is present in the program.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			hasVertexShader () const noexcept
			{
				return m_vertexShader != nullptr;
			}

			/**
			 * @brief Returns whether a tesselation control shader is present in the program.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			hasTesselationControlShader () const noexcept
			{
				return m_tesselationControlShader != nullptr;
			}

			/**
			 * @brief Returns whether a tesselation evaluation shader is present in the program.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			hasTesselationEvaluationShader () const noexcept
			{
				return m_tesselationEvaluationShader != nullptr;
			}

			/**
			 * @brief Returns whether a geometry shader is present in the program.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			hasGeometryShader () const noexcept
			{
				return m_geometryShader != nullptr;
			}

			/**
			 * @brief Returns whether a fragment shader is present in the program.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			hasFragmentShader () const noexcept
			{
				return m_fragmentShader != nullptr;
			}

			/**
			 * @brief Returns the vertex shader.
			 * @return const VertexShader *
			 */
			[[nodiscard]]
			const VertexShader * vertexShader () const noexcept;

			/**
			 * @brief Returns the vertex shader.
			 * @return VertexShader *
			 */
			[[nodiscard]]
			VertexShader * vertexShader () noexcept;

			/**
			 * @brief Returns the tesselation control shader.
			 * @return const TesselationControlShader *
			 */
			[[nodiscard]]
			const TesselationControlShader * tesselationControlShader () const noexcept;

			/**
			 * @brief Returns the tesselation control shader.
			 * @return TesselationControlShader *
			 */
			[[nodiscard]]
			TesselationControlShader * tesselationControlShader () noexcept;

			/**
			 * @brief Returns the tesselation evaluation shader.
			 * @return const TesselationEvaluationShader *
			 */
			[[nodiscard]]
			const TesselationEvaluationShader * tesselationEvaluationShader () const noexcept;

			/**
			 * @brief Returns the tesselation evaluation shader.
			 * @return TesselationEvaluationShader *
			 */
			[[nodiscard]]
			TesselationEvaluationShader * tesselationEvaluationShader () noexcept;

			/**
			 * @brief Returns the geometry shader.
			 * @return const GeometryShader *
			 */
			[[nodiscard]]
			const GeometryShader * geometryShader () const noexcept;

			/**
			 * @brief Returns the geometry shader.
			 * @return GeometryShader *
			 */
			[[nodiscard]]
			GeometryShader * geometryShader () noexcept;

			/**
			 * @brief Returns the fragment shader.
			 * @return const FragmentShader *
			 */
			[[nodiscard]]
			const FragmentShader * fragmentShader () const noexcept;

			/**
			 * @brief Returns the fragment shader.
			 * @return FragmentShader *
			 */
			[[nodiscard]]
			FragmentShader * fragmentShader () noexcept;

			/**
			 * @brief Returns the list of all shader pointers.
			 * @return std::vector< AbstractShader * >
			 */
			[[nodiscard]]
			std::vector< AbstractShader * > getShaderList () const noexcept;

			/**
			 * @brief Creates a vertex buffer format.
			 * @param vertexBufferFormatManager A reference to the vertex buffer format manager.
			 * @param geometry A pointer to a geometry interface.
			 * @return void
			 */
			[[nodiscard]]
			bool createVertexBufferFormat (Graphics::VertexBufferFormatManager & vertexBufferFormatManager, const Graphics::Geometry::Interface * geometry) noexcept;

			/**
			 * @brief Creates a vertex buffer format.
			 * @param vertexBufferFormatManager A reference to the vertex buffer format manager.
			 * @param topology The geometry topology.
			 * @param geometryFlagBits The geometry flags.
			 * @return void
			 */
			[[nodiscard]]
			bool createVertexBufferFormat (Graphics::VertexBufferFormatManager & vertexBufferFormatManager, Graphics::Topology topology, uint32_t geometryFlagBits) noexcept;

			/**
			 * @brief Returns the vertex buffer format.
			 * @return std::shared_ptr< Graphics::VertexBufferFormat >
			 */
			[[nodiscard]]
			std::shared_ptr< Graphics::VertexBufferFormat >
			vertexBufferFormat () const noexcept
			{
				return m_vertexBufferFormat;
			}

			/**
			 * @brief Sets the pipeline layout.
			 * @param pipelineLayout A reference to a pipeline layout smart pointer.
			 * @return void
			 */
			void
			setPipelineLayout (const std::shared_ptr< Vulkan::PipelineLayout > & pipelineLayout) noexcept
			{
				m_pipelineLayout = pipelineLayout;
			}

			/**
			 * @brief Returns the pipeline layout corresponding to this program.
			 * @return std::shared_ptr< Vulkan::PipelineLayout >
			 */
			[[nodiscard]]
			std::shared_ptr< Vulkan::PipelineLayout >
			pipelineLayout () const noexcept
			{
				return m_pipelineLayout;
			}

			/**
			 * @brief Sets the graphics pipeline.
			 * @param graphicsPipeline A reference to a graphics pipeline smart pointer.
			 * @return void
			 */
			void
			setGraphicsPipeline (const std::shared_ptr< Vulkan::GraphicsPipeline > & graphicsPipeline) noexcept
			{
				m_graphicsPipeline = graphicsPipeline;
			}

			/**
			 * @brief Returns the graphics pipeline corresponding to this program.
			 * @return std::shared_ptr< Vulkan::GraphicsPipeline >
			 */
			[[nodiscard]]
			std::shared_ptr< Vulkan::GraphicsPipeline >
			graphicsPipeline () const noexcept
			{
				return m_graphicsPipeline;
			}

		private:

			std::string m_GLSLVersion;
			std::string m_GLSLProfile;
			SetIndexes m_setIndexes;
			std::unique_ptr< VertexShader > m_vertexShader;
			std::unique_ptr< TesselationControlShader > m_tesselationControlShader;
			std::unique_ptr< TesselationEvaluationShader > m_tesselationEvaluationShader;
			std::unique_ptr< GeometryShader > m_geometryShader;
			std::unique_ptr< FragmentShader > m_fragmentShader;
			std::shared_ptr< Graphics::VertexBufferFormat > m_vertexBufferFormat;
			std::shared_ptr< Vulkan::PipelineLayout > m_pipelineLayout;
			std::shared_ptr< Vulkan::GraphicsPipeline > m_graphicsPipeline;
	};
}
