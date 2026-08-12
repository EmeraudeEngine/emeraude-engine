/*
 * src/Saphir/Program.hpp
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
#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <vector>

/* Local inclusions for inheritances. */
#include "NameableTrait.hpp"

/* Local inclusions for usages. */
#include "FragmentShader.hpp"
#include "GeometryShader.hpp"
#include "Graphics/VertexBufferFormatManager.hpp"
#include "SetIndexes.hpp"
#include "TesselationControlShader.hpp"
#include "TesselationEvaluationShader.hpp"
#include "Types.hpp"
#include "VertexShader.hpp"
#include "Vulkan/GraphicsPipeline.hpp"

namespace EmEn::Saphir
{
	/**
	 * @brief The program class.
	 * @note This will contain all the necessary shaders to build a program like OpenGL.
	 * @extends EmEn::Base::NameableTrait This is a nameable class.
	 */
	class Program final : public Base::NameableTrait
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"Program"};

			/** 
			 * @brief Constructs a graphics shader container.
			 * @param name A string [std::move].
			 * @param GLSLVersion The GLSL version in use [std::move].
			 * @param GLSLProfile The GLSL profile in use [std::move].
			 */
			Program (std::string name, std::string GLSLVersion, std::string GLSLProfile) noexcept
				: NameableTrait{std::move(name)},
				m_GLSLVersion{std::move(GLSLVersion)},
				m_GLSLProfile{std::move(GLSLProfile)}
			{

			}

			/**
			 * @brief Returns whether the program has all the shader source codes it needs, generated and consistent.
			 * @note The vertex shader is mandatory. The tesselation control and evaluation shaders are optional,
			 * but if a control shader is present, the evaluation shader must be present too (and vice versa is not checked).
			 * The geometry and fragment shaders are optional.
			 * @return bool
			 */
			[[nodiscard]]
			bool isComplete () const noexcept;

			/**
			 * @brief Returns whether the program shaders are compiled.
			 * @note This reflects whether setGraphicsPipeline() has been called with a valid pipeline, not that
			 * this class performed the compilation itself; the actual GLSL-to-SPIR-V compilation and graphics
			 * pipeline creation happen elsewhere (Generator::Abstract::createGraphicsPipeline()).
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isCompiled () const noexcept
			{
				return m_graphicsPipeline != nullptr;
			}

			/**
			 * @brief Returns whether the program uses tesselation technics.
			 * @note Requires both the control AND the evaluation shader to be present; a control shader alone
			 * (evaluation missing) is treated as a mismatched setup and returns false.
			 * @return bool
			 */
			[[nodiscard]]
			bool useTesselation () const noexcept;

			/**
			 * @brief Returns whether the program needed instancing.
			 * @return bool
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
			 * @brief Returns whether MDI (Multi-Draw Indirect) was enabled for this program.
			 * @note Unlike wasInstancingEnabled(), wasAdvancedMatricesEnabled() and wasBillBoardingEnabled(), this
			 * returns false silently (no error trace) if the vertex shader has not been initialized yet.
			 * @return bool
			 */
			[[nodiscard]]
			bool wasMDIEnabled () const noexcept;

			/**
			 * @brief Returns whether the vertex shader reads its model matrix from the InstanceTransforms SSBO.
			 * @note Returns false silently (no error trace) if the vertex shader has not been initialized yet.
			 * @return bool
			 */
			[[nodiscard]]
			bool wasInstanceTransformsEnabled () const noexcept;

			/**
			 * @brief Initializes the vertex shader and returns it.
			 * @note Calling this a second time on the same Program is a no-op that logs an error and returns
			 * nullptr; the vertex shader, once initialized, cannot be re-initialized or replaced.
			 * @param name A reference to a string.
			 * @param enableInstancing Enable instancing for the vertex shader.
			 * @param enableAdvancedMatrices Enable the advanced matrices for the vertex shader.
			 * @param enableBillBoarding Enable the render of bill boards instead of classic geometry.
			 * @param enableCubemapMode Enable cubemap multiview rendering mode.
			 * @param enableMDI Enable Multi-Draw Indirect mode.
			 * @return VertexShader *
			 */
			[[nodiscard]]
			VertexShader * initVertexShader (const std::string & name, bool enableInstancing, bool enableAdvancedMatrices, bool enableBillBoarding, bool enableCubemapMode = false, bool enableMDI = false) noexcept;

			/**
			 * @brief Initializes the tesselation control shader and returns it.
			 * @note Calling this a second time on the same Program is a no-op that logs an error and returns
			 * nullptr. If used, the tesselation evaluation shader must also be initialized for the program to
			 * be considered complete (see isComplete()).
			 * @param name A reference to a string.
			 * @return TesselationControlShader *
			 */
			[[nodiscard]]
			TesselationControlShader * initTesselationControlShader (const std::string & name) noexcept;

			/**
			 * @brief Initializes the tesselation evaluation shader and returns it.
			 * @note Calling this a second time on the same Program is a no-op that logs an error and returns
			 * nullptr.
			 * @param name A reference to a string.
			 * @return TesselationEvaluationShader *
			 */
			[[nodiscard]]
			TesselationEvaluationShader * initTesselationEvaluationShader (const std::string & name) noexcept;

			/**
			 * @brief Initializes the geometry shader and returns it.
			 * @note Calling this a second time on the same Program is a no-op that logs an error and returns
			 * nullptr.
			 * @param name A reference to a string.
			 * @param inputPrimitive A reference to an input primitive declaration.
			 * @param outputPrimitive A reference to an output primitive declaration.
			 * @return GeometryShader *
			 */
			[[nodiscard]]
			GeometryShader * initGeometryShader (const std::string & name, const Declaration::InputPrimitive & inputPrimitive, const Declaration::OutputPrimitive & outputPrimitive) noexcept;

			/**
			 * @brief Initializes the fragment shader.
			 * @note Calling this a second time on the same Program is a no-op that logs an error and returns
			 * nullptr. The fragment shader is optional (e.g. depth-only passes have none).
			 * @param name A reference to a string.
			 * @return FragmentShader *
			 */
			[[nodiscard]]
			FragmentShader * initFragmentShader (const std::string & name) noexcept;

			/**
			 * @brief Returns the type of the last (most downstream) shader stage that has been generated.
			 * @note Checks stages in pipeline order from fragment down to vertex and returns the first one found
			 * generated; returns ShaderType::Undefined if no stage has been generated yet.
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
			 * @note Logs a warning and returns nullptr if hasVertexShader() is false (not initialized yet).
			 * @return const VertexShader *
			 */
			[[nodiscard]]
			const VertexShader * vertexShader () const noexcept;

			/**
			 * @brief Returns the vertex shader.
			 * @note Logs a warning and returns nullptr if hasVertexShader() is false (not initialized yet).
			 * @return VertexShader *
			 */
			[[nodiscard]]
			VertexShader * vertexShader () noexcept;

			/**
			 * @brief Returns the tesselation control shader.
			 * @note Logs a warning and returns nullptr if hasTesselationControlShader() is false (not initialized yet).
			 * @return const TesselationControlShader *
			 */
			[[nodiscard]]
			const TesselationControlShader * tesselationControlShader () const noexcept;

			/**
			 * @brief Returns the tesselation control shader.
			 * @note Logs a warning and returns nullptr if hasTesselationControlShader() is false (not initialized yet).
			 * @return TesselationControlShader *
			 */
			[[nodiscard]]
			TesselationControlShader * tesselationControlShader () noexcept;

			/**
			 * @brief Returns the tesselation evaluation shader.
			 * @note Logs a warning and returns nullptr if hasTesselationEvaluationShader() is false (not initialized yet).
			 * @return const TesselationEvaluationShader *
			 */
			[[nodiscard]]
			const TesselationEvaluationShader * tesselationEvaluationShader () const noexcept;

			/**
			 * @brief Returns the tesselation evaluation shader.
			 * @note Logs a warning and returns nullptr if hasTesselationEvaluationShader() is false (not initialized yet).
			 * @return TesselationEvaluationShader *
			 */
			[[nodiscard]]
			TesselationEvaluationShader * tesselationEvaluationShader () noexcept;

			/**
			 * @brief Returns the geometry shader.
			 * @note Logs a warning and returns nullptr if hasGeometryShader() is false (not initialized yet).
			 * @return const GeometryShader *
			 */
			[[nodiscard]]
			const GeometryShader * geometryShader () const noexcept;

			/**
			 * @brief Returns the geometry shader.
			 * @note Logs a warning and returns nullptr if hasGeometryShader() is false (not initialized yet).
			 * @return GeometryShader *
			 */
			[[nodiscard]]
			GeometryShader * geometryShader () noexcept;

			/**
			 * @brief Returns the fragment shader.
			 * @note Logs a warning and returns nullptr if hasFragmentShader() is false (not initialized yet).
			 * @return const FragmentShader *
			 */
			[[nodiscard]]
			const FragmentShader * fragmentShader () const noexcept;

			/**
			 * @brief Returns the fragment shader.
			 * @note Logs a warning and returns nullptr if hasFragmentShader() is false (not initialized yet).
			 * @return FragmentShader *
			 */
			[[nodiscard]]
			FragmentShader * fragmentShader () noexcept;

			/**
			 * @brief Returns the list of the shader pointers currently held by this program.
			 * @note Only the stages that have been initialized are included (no nullptr entries), in pipeline
			 * stage order: vertex, tesselation control, tesselation evaluation, geometry, fragment.
			 * @return std::vector< AbstractShader * >
			 */
			[[nodiscard]]
			std::vector< AbstractShader * > getShaderList () const noexcept;

			/**
			 * @brief Creates the vertex buffer format matching this program's vertex shader and a given geometry.
			 * @note Requires the vertex shader to be initialized first (hasVertexShader()). On success, the
			 * result is stored and retrievable through vertexBufferFormat().
			 * @param vertexBufferFormatManager A reference to the vertex buffer format manager.
			 * @param geometry A pointer to a geometry interface.
			 * @return bool True if the format was created (or retrieved from cache), false otherwise.
			 */
			[[nodiscard]]
			bool createVertexBufferFormat (Graphics::VertexBufferFormatManager & vertexBufferFormatManager, const Graphics::Geometry::Interface * geometry) noexcept;

			/**
			 * @brief Creates the vertex buffer format matching this program's vertex shader for a raw topology, without a geometry instance.
			 * @note Requires the vertex shader to be initialized first (hasVertexShader()). On success, the
			 * result is stored and retrievable through vertexBufferFormat().
			 * @param vertexBufferFormatManager A reference to the vertex buffer format manager.
			 * @param topology The geometry topology.
			 * @param geometryFlagBits The geometry flags.
			 * @return bool True if the format was created (or retrieved from cache), false otherwise.
			 */
			[[nodiscard]]
			bool createVertexBufferFormat (Graphics::VertexBufferFormatManager & vertexBufferFormatManager, Graphics::Topology topology, uint32_t geometryFlagBits) noexcept;

			/**
			 * @brief Returns the vertex buffer format.
			 * @note Null until createVertexBufferFormat() has been called successfully.
			 * @return const std::shared_ptr< Graphics::VertexBufferFormat > &
			 */
			[[nodiscard]]
			const std::shared_ptr< Graphics::VertexBufferFormat > &
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
			 * @note Null until setPipelineLayout() has been called.
			 * @return const std::shared_ptr< Vulkan::PipelineLayout > &
			 */
			[[nodiscard]]
			const std::shared_ptr< Vulkan::PipelineLayout > &
			pipelineLayout () const noexcept
			{
				return m_pipelineLayout;
			}

			/**
			 * @brief Sets the graphics pipeline.
			 * @note Assigning a non-null pipeline here is what makes isCompiled() return true.
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
			 * @note Null until setGraphicsPipeline() has been called (see isCompiled()).
			 * @return const std::shared_ptr< Vulkan::GraphicsPipeline > &
			 */
			[[nodiscard]]
			const std::shared_ptr< Vulkan::GraphicsPipeline > &
			graphicsPipeline () const noexcept
			{
				return m_graphicsPipeline;
			}

			/**
			 * @brief Sets a boolean specialization constant for the fragment shader.
			 * @note Must be called before shader compilation (createGraphicsPipeline).
			 * @param constantId The constant ID as declared in the shader (layout(constant_id = X)).
			 * @param value The boolean value for the constant.
			 * @return void
			 */
			void
			setFragmentSpecializationConstant (uint32_t constantId, bool value) noexcept
			{
				m_fragmentSpecConstantsBool.emplace(constantId, value);
			}

			/**
			 * @brief Returns the fragment shader boolean specialization constants.
			 * @return const std::map< uint32_t, bool > &
			 */
			[[nodiscard]]
			const std::map< uint32_t, bool > &
			fragmentSpecializationConstantsBool () const noexcept
			{
				return m_fragmentSpecConstantsBool;
			}

			/**
			 * @brief Returns whether the program has specialization constants defined.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			hasSpecializationConstants () const noexcept
			{
				return !m_fragmentSpecConstantsBool.empty();
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
			std::map< uint32_t, bool > m_fragmentSpecConstantsBool; // FIXME: Use a cheaper structure here.
	};
}
