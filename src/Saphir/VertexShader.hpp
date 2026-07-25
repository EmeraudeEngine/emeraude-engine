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
#include <cstdint>
#include <set>
#include <string>
#include <utility>
#include <vector>

/* Local inclusions for inheritances. */
#include "AbstractShader.hpp"

/* Local inclusions for usages. */
#include "Declaration/InputAttribute.hpp"
#include "Declaration/OutputBlock.hpp"
#include "Declaration/StageOutput.hpp"
#include "Graphics/Types.hpp"
#include "Types.hpp"

namespace EmEn::Saphir
{
	/** @brief Defines the scope of a synthesized variable. */
	enum class VariableScope : uint8_t
	{
		/** @brief The variable is only used in the current shader. */
		Local,
		/** @brief The variable is only an output for the next stage. */
		ToNextStage,
		/** @brief The variable is used in the shader and an output for the next stage. */
		Both
	};

	/**
	 * @brief The vertex shader class.
	 * @extends EmEn::Saphir::AbstractShader The base class of every shader type.
	 */
	class VertexShader final : public AbstractShader
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"VertexShader"};

			/** @brief Extends declaration from base class. */
			using AbstractShader::declare;

			/**
			 * @brief Constructs a vertex shader.
			 * @param name The name of the shader for identification [std::move].
			 * @param GLSLVersion A reference to a string [std::move].
			 * @param GLSLProfile A reference to a string [std::move].
			 */
			VertexShader (std::string name, std::string GLSLVersion, std::string GLSLProfile) noexcept
				: AbstractShader{std::move(name), std::move(GLSLVersion), std::move(GLSLProfile)}
			{

			}

			/** @copydoc EmEn::Saphir::AbstractShader::type() */
			[[nodiscard]]
			ShaderType
			type () const noexcept override
			{
				return ShaderType::VertexShader;
			}

			/**
			 * @brief Declares a vertex attribute to be used in a vertex shader.
			 * @note Re-declaring an already-present attribute is a silent no-op: the
			 * name, location and GLSL type are all derived from the VertexAttributeType,
			 * so a duplicate is byte-identical and harmless. Composable shader generators
			 * (the synthesize and TBN helpers) declare the attributes they consume independently
			 * and rely on this de-duplication.
			 * @param declaration A reference to a shader input attribute.
			 * @return bool
			 */
			bool declare (const Declaration::InputAttribute & declaration) noexcept;

			/**
			 * @brief Declares a stage output variable to be used in the shader.
			 * @param declaration A reference to a ShaderStageOutput.
			 * @return bool
			 */
			bool declare (const Declaration::StageOutput & declaration) noexcept;

			/**
			 * @brief Declares an output block to be used in the shader.
			 * @param declaration A reference to an OutputBlock.
			 * @return bool
			 */
			bool declare (const Declaration::OutputBlock & declaration) noexcept;

			/**
			 * @brief Returns the list of input attribute declarations.
			 * @return const std::vector< Declaration::InputAttribute > &
			 */
			[[nodiscard]]
			const std::vector< Declaration::InputAttribute > &
			inputAttributes () const noexcept
			{
				return m_inputAttributes;
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
			 * @brief Requests to synthesize an instruction in the vertex shader.
			 * @warning This function is recursive for the variable "PositionTextureSpace".
			 * @param variableName The variable name to synthesize. This should be a key from Keys::ShaderVariables namespace.
			 * @param scope Set the variable scope in the vertex shader. Default VariableScope::ToNextStage.
			 * @return bool
			 */
			bool requestSynthesizeInstruction (const char * variableName, VariableScope scope = VariableScope::ToNextStage) noexcept;

			/**
			 * @brief Returns a list of requested vertex attributes in the shader.
			 * @return const std::set< Graphics::VertexAttributeType > &
			 */
			[[nodiscard]]
			const std::set< Graphics::VertexAttributeType > &
			vertexAttributes () const noexcept
			{
				return m_vertexAttributes;
			}

			/**
			 * @brief Sets the vertex shader uses instancing.
			 * @return void
			 */
			void
			enableInstancing () noexcept
			{
				m_instancingEnabled = true;
			}

			/**
			 * @brief Returns whether the vertex shader is using instancing.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isInstancingEnabled () const noexcept
			{
				return m_instancingEnabled;
			}

			/**
			 * @brief Enables the need of advanced matrices (for lighting or reflection).
			 * @return void
			 */
			void
			enableAdvancedMatrices () noexcept
			{
				m_advancedMatricesEnabled = true;
			}

			/**
			 * @brief Returns whether the advances matrices are enabled.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isAdvancedMatricesEnabled () const noexcept
			{
				return m_advancedMatricesEnabled;
			}

			/**
			 * @brief Enables bill boarding render.
			 * @return void
			 */
			void
			enableBillBoarding () noexcept
			{
				m_billBoardingEnabled = true;
			}

			/**
			 * @brief Returns whether the vertex shader is rendering bill boards.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isBillBoardingEnabled () const noexcept
			{
				return m_billBoardingEnabled;
			}

			/**
			 * @brief Enables cubemap rendering mode (multiview with gl_ViewIndex).
			 * @return void
			 */
			void
			enableCubemapMode () noexcept
			{
				m_cubemapModeEnabled = true;
			}

			/**
			 * @brief Returns whether the vertex shader is in cubemap rendering mode.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isCubemapModeEnabled () const noexcept
			{
				return m_cubemapModeEnabled;
			}

			/**
			 * @brief Enables CSM (Cascaded Shadow Map) rendering mode.
			 * @note CSM mode uses multiview with gl_ViewIndex to select the cascade view-projection matrix.
			 * @return void
			 */
			void
			enableCSMMode () noexcept
			{
				m_csmModeEnabled = true;
			}

			/**
			 * @brief Returns whether the vertex shader is in CSM rendering mode.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isCSMModeEnabled () const noexcept
			{
				return m_csmModeEnabled;
			}

			/**
			 * @brief Enables Multi-Draw Indirect mode.
			 * @return void
			 */
			void
			enableMDI () noexcept
			{
				m_MDIEnabled = true;

				/* Register extensions early so generateHeaders() emits them
				 * before the BDA struct declaration in onSourceCodeGeneration(). */
				this->setExtensionBehavior("GL_EXT_buffer_reference", "enable");
				this->setExtensionBehavior("GL_EXT_buffer_reference2", "enable");
				this->setExtensionBehavior("GL_ARB_gpu_shader_int64", "enable");
				this->setExtensionBehavior("GL_ARB_shader_draw_parameters", "enable");
			}

			/**
			 * @brief Returns whether MDI mode is enabled.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isMDIEnabled () const noexcept
			{
				return m_MDIEnabled;
			}

			/**
			 * @brief Enables the InstanceTransforms SSBO as the model matrix source.
			 * @note The model matrix is read from the scene InstanceTransforms SSBO entry
			 * indexed by gl_InstanceIndex (== the firstInstance draw parameter — this path
			 * always draws with instanceCount = 1, so no shaderDrawParameters feature is
			 * required, contrary to gl_BaseInstance).
			 * @return void
			 */
			void
			enableInstanceTransforms () noexcept
			{
				m_instanceTransformsEnabled = true;
			}

			/**
			 * @brief Returns whether the InstanceTransforms SSBO is the model matrix source.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isInstanceTransformsEnabled () const noexcept
			{
				return m_instanceTransformsEnabled;
			}

			/**
			 * @brief Enables the infinity-view mode: the velocity outputs read the previous
			 * INFINITY view-projection from the InstanceTransforms header.
			 * @note The infinity view drops the camera translation. Mixing it with the regular
			 * previous view-projection yields a velocity wrong by that translation, which does
			 * NOT cancel on a static camera (it is a structural, not a temporal, mismatch).
			 * @return void
			 */
			void
			enableInfinityView () noexcept
			{
				m_infinityViewEnabled = true;
			}

			/**
			 * @brief Returns whether the infinity-view mode is enabled.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isInfinityViewEnabled () const noexcept
			{
				return m_infinityViewEnabled;
			}

			/**
			 * @brief Returns whether the matrix push constant block carries the TAA sub-pixel
			 * projection jitter, i.e. whether gl_Position must be offset by it in this shader.
			 * @note MUST mirror Generator::Abstract::declareMatrixPushConstantBlock(): the
			 * ProjectionJitter member exists in the instanced and InstanceTransforms blocks only.
			 * Cubemap/CSM targets are excluded because nothing is pushed for them (their
			 * view-projection comes from the view UBO indexed by gl_ViewIndex), and MDI plus the
			 * push-constant-only fallbacks are excluded because they keep the jitter baked in
			 * their CPU-computed matrices — none of them outputs a velocity.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isProjectionJitterPushed () const noexcept
			{
				if ( m_MDIEnabled || m_cubemapModeEnabled || m_csmModeEnabled )
				{
					return false;
				}

				return m_instancingEnabled || m_instanceTransformsEnabled;
			}

			/**
			 * @brief Enables the instanced motion history mode: the per-instance VBO carries
			 * the previous model matrix (+4 vec4 attribute slots after the normal matrix).
			 * @note Affects the vertex buffer format stride even when the shader does not
			 * consume the attribute (jumped over).
			 * @return void
			 */
			void
			enableInstanceMotionHistory () noexcept
			{
				m_instanceMotionHistoryEnabled = true;
			}

			/**
			 * @brief Returns whether the instanced motion history mode is enabled.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isInstanceMotionHistoryEnabled () const noexcept
			{
				return m_instanceMotionHistoryEnabled;
			}

			/**
			 * @brief Enables skeletal skinning in this vertex shader.
			 * @return void
			 */
			void
			enableSkinning () noexcept
			{
				m_skinningEnabled = true;
			}

			/**
			 * @brief Returns whether skeletal skinning is enabled.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isSkinningEnabled () const noexcept
			{
				return m_skinningEnabled;
			}

		private:

			/** @copydoc EmEn::Saphir::AbstractShader::onSourceCodeGeneration() */
			[[nodiscard]]
			bool onSourceCodeGeneration (Generator::Abstract & generator, std::stringstream & code, std::string & topInstructions, std::string & outputInstructions) noexcept override;

			/** @copydoc EmEn::Saphir::AbstractShader::onGetDeclarationStats() */
			void onGetDeclarationStats (std::stringstream & output) const noexcept override;

			/**
			 * @brief Returns whether a variable preparation has already been asked.
			 * @param preparation The name of the variable.
			 * @return bool
			 */
			[[nodiscard]]
			bool preparationAlreadyDone (const char * preparation) const noexcept;

			/**
			 * @brief Creates a local variable for sprite model matrix with VBO.
			 * @return bool
			 */
			[[nodiscard]]
			bool prepareSpriteModelMatrix () noexcept;

			[[nodiscard]]
			bool prepareMDIModelMatrix () noexcept;

			/**
			 * @brief Creates a local variable for the model matrix read from the InstanceTransforms SSBO.
			 * @note Indexed by gl_InstanceIndex (instanceCount is always 1 on this path).
			 * @return bool
			 */
			[[nodiscard]]
			bool prepareInstanceModelMatrix () noexcept;

		public:

			/**
			 * @brief Synthesizes the current/previous clip-space position outputs for the velocity pass.
			 * @note The previous view-projection matrix comes from the InstanceTransforms SSBO header;
			 * the previous model matrix comes from the SSBO entry (non-instanced), from the
			 * PreviousModelMatrix attribute (instanced with motion history) or falls back to the
			 * current model matrix (instanced without history, billboards — camera-only velocity;
			 * skinned meshes use the CURRENT pose until double skinning exists).
			 * Requires the InstanceTransforms SSBO GLSL block to be declared. Not available for MDI.
			 * @param generator A reference to the shader generator (output locations).
			 * @param emitted Set to true when the outputs were emitted, false when unavailable.
			 * @return bool False on a declaration error only.
			 */
			[[nodiscard]]
			bool synthesizeVelocityClipPositions (Generator::Abstract & generator, bool & emitted) noexcept;

		private:

			/**
			 * @brief Creates a local variable for modelView matrix with VBO.
			 * @return bool
			 */
			[[nodiscard]]
			bool prepareModelViewMatrix () noexcept;

			/**
			 * @brief Creates a local variable for normal matrix with VBO.
			 * @return bool
			 */
			[[nodiscard]]
			bool prepareNormalMatrix () noexcept;

			/**
			 * @brief Creates a local variable for modelViewProjection matrix with VBO.
			 * @return bool
			 */
			[[nodiscard]]
			bool prepareModelViewProjectionMatrix () noexcept;

			/**
			 * @brief Synthesizes the vertex position in world space in the vertex shader.
			 * @note gl_Position = gl_modelMatrix * gl_Vertex;
			 * @param generator A reference to the shader generator.
			 * @param topInstructions Every instruction that should be on the top of the main() function.
			 * @param outputInstructions Every instruction that should be at the end of the main() function.
			 * @param scope Set the variable scope in the vertex shader.
			 * @param asGLStandardPosition Synthesize as gl_Position for specific shaders.
			 * @return bool
			 */
			bool synthesizeVertexPositionInWorldSpace (Generator::Abstract & generator, std::string & topInstructions, std::string & outputInstructions, VariableScope scope, bool asGLStandardPosition = false) noexcept;

			/**
			 * @brief Synthesizes the vertex position in view space in the vertex shader.
			 * @note gl_Position = gl_modelViewMatrix * gl_Vertex;
			 * @param generator A reference to the shader generator.
			 * @param topInstructions Every instruction that should be on the top of the main() function.
			 * @param outputInstructions Every instruction that should be at the end of the main() function.
			 * @param scope Set the variable scope in the vertex shader.
			 * @return bool
			 */
			bool synthesizeVertexPositionInViewSpace (Generator::Abstract & generator, std::string & topInstructions, std::string & outputInstructions, VariableScope scope) noexcept;

			/**
			 * @brief Synthesizes the vertex position in screen space in the vertex shader.
			 * @note gl_Position = gl_modelViewProjectionMatrix * gl_Vertex;
			 * @param outputInstructions Every instruction that should be at the end of the main() function.
			 * @return bool
			 */
			bool synthesizeVertexPositionInScreenSpace (std::string & outputInstructions) noexcept;

			/**
			 * @brief Synthesizes the vertex position in texture space in the vertex shader.
			 * @note gl_Position = gl_modelViewProjectionMatrix * gl_Vertex;
			 * @param generator A reference to the shader generator.
			 * @param topInstructions Every instruction that should be on the top of the main() function.
			 * @param outputInstructions Every instruction that should be at the end of the main() function.
			 * @param scope Set the variable scope in the vertex shader.
			 * @return bool
			 */
			bool synthesizeVertexPositionInTextureSpace (Generator::Abstract & generator, std::string & topInstructions, std::string & outputInstructions, VariableScope scope) noexcept;

			/**
			 * @brief Synthesizes the vertex color in the vertex shader.
			 * @note This will not generate a local variable.
			 * @param generator A reference to the shader generator.
			 * @param outputInstructions Every instruction that should be at the end of the main() function.
			 * @return bool
			 */
			bool synthesizeVertexColor (Generator::Abstract & generator, std::string & outputInstructions) noexcept;

			/**
			 * @brief Synthesizes the vertex texture coordinates in the vertex shader.
			 * @note This will not generate a local variable.
			 * @param generator A reference to the shader generator.
			 * @param outputInstructions Every instruction that should be at the end of the main() function.
			 * @param TCVariableName The name of the texture coordinates variable.
			 * @return bool
			 */
			bool synthesizeVertexTextureCoordinates (Generator::Abstract & generator, std::string & outputInstructions, const char * TCVariableName) noexcept;

			/**
			 * @brief Synthesizes the vertex tangent, bi-normal or normal vector in world space in the vertex shader.
			 * @param generator A reference to the shader generator.
			 * @param topInstructions Every instruction that should be on the top of the main() function.
			 * @param outputInstructions Every instruction that should be at the end of the main() function.
			 * @param vectorType The vector type to generate.
			 * @param scope Set the variable scope in the vertex shader.
			 * @return bool
			 */
			bool synthesizeVertexVectorInWorldSpace (Generator::Abstract & generator, std::string & topInstructions, std::string & outputInstructions, Graphics::VertexAttributeType vectorType, VariableScope scope) noexcept;

			/**
			 * @brief Synthesizes the vertex tangent, bi-normal or normal vector in view space variable in the vertex shader.
			 * @param generator A reference to the shader generator.
			 * @param topInstructions Every instruction that should be on the top of the main() function.
			 * @param outputInstructions Every instruction that should be at the end of the main() function.
			 * @param vectorType The vector type to generate.
			 * @param scope Set the variable scope in the vertex shader.
			 * @return bool
			 */
			bool synthesizeVertexVectorInViewSpace (Generator::Abstract & generator, std::string & topInstructions, std::string & outputInstructions, Graphics::VertexAttributeType vectorType, VariableScope scope) noexcept;

			/**
			 * @brief synthesizeWorldTBNMatrix
			 * @param generator A reference to the shader generator.
			 * @param topInstructions Every instruction that should be on the top of the main() function.
			 * @param outputInstructions Every instruction that should be at the end of the main() function.
			 * @param scope Set the variable scope in the vertex shader.
			 * @return bool
			 */
			bool synthesizeWorldTBNMatrix (Generator::Abstract & generator, std::string & topInstructions, std::string & outputInstructions, VariableScope scope) noexcept;

			/**
			 * @brief synthesizeViewTBNMatrix
			 * @param generator A reference to the shader generator.
			 * @param topInstructions Every instruction that should be on the top of the main() function.
			 * @param outputInstructions Every instruction that should be at the end of the main() function.
			 * @param scope Set the variable scope in the vertex shader.
			 * @return bool
			 */
			bool synthesizeViewTBNMatrix (Generator::Abstract & generator, std::string & topInstructions, std::string & outputInstructions, VariableScope scope) noexcept;

			/**
			 * @brief Synthesizes the matrix to transform tangent-space normals to world space.
			 * @param generator A reference to the shader generator.
			 * @param topInstructions Every instruction that should be on the top of the main() function.
			 * @param outputInstructions Every instruction that should be at the end of the main() function.
			 * @param scope Set the variable scope in the vertex shader.
			 * @return bool
			 */
			bool synthesizeTangentToWorldMatrix (Generator::Abstract & generator, std::string & topInstructions, std::string & outputInstructions, VariableScope scope) noexcept;

			/**
			 * @brief Synthesizes instructions before generating the final source code.
			 * @param generator A reference to the shader generator.
			 * @param topInstructions Every instruction that should be on the top of the main() function.
			 * @param outputInstructions Every instruction that should be at the end of the main() function.
			 * @return bool
			 */
			bool synthesizeRequestInstructions (Generator::Abstract & generator, std::string & topInstructions, std::string & outputInstructions) noexcept;

			/**
			 * @brief Generates final unique instructions.
			 * @param generator A reference to the shader generator.
			 * @param topInstructions A reference to the top instructions string.
			 * @param outputInstructions  A reference to the output instructions string.
			 * @return bool
			 */
			[[nodiscard]]
			bool generateMainUniqueInstructions (Generator::Abstract & generator, std::string & topInstructions, std::string & outputInstructions) noexcept;

			/**
			 * @brief Checks whether a synthetic variable is allowed.
			 * @param variableName A pointer to a C-string for the variable name.
			 * @return bool
			 */
			[[nodiscard]]
			static bool isSyntheticVariableAllowed (const char * variableName) noexcept;

			[[nodiscard]]
			static Declaration::Function generateComputeUpwardVectorFunction () noexcept;

			[[nodiscard]]
			static Declaration::Function generateGetBillBoardModelMatrixFunction () noexcept;

			std::vector< std::pair< const char *, std::string > > m_uniquePreparations; // ie normalMatrix for VBO
			std::vector< std::pair< const char *, VariableScope > > m_requests;
			std::vector< Declaration::InputAttribute > m_inputAttributes;
			std::vector< Declaration::StageOutput > m_stageOutputs;
			std::vector< Declaration::OutputBlock > m_outputBlocks;
			std::set< Graphics::VertexAttributeType > m_vertexAttributes;
			bool m_instancingEnabled{false};
			bool m_advancedMatricesEnabled{false};
			bool m_billBoardingEnabled{false};
			bool m_cubemapModeEnabled{false};
			bool m_csmModeEnabled{false};
			bool m_MDIEnabled{false};
			bool m_skinningEnabled{false};
			bool m_instanceTransformsEnabled{false};
			bool m_infinityViewEnabled{false};
			bool m_instanceMotionHistoryEnabled{false};
			/** @brief Whether the velocity outputs need the previous skinned position (double skinning). */
			bool m_previousSkinningRequired{false};
	};
}
