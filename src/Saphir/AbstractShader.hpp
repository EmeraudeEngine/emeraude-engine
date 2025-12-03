/*
 * src/Saphir/AbstractShader.hpp
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
#include <string>
#include <vector>
#include <filesystem>

/* Local inclusions for inheritances. */
#include "Libs/NameableTrait.hpp"
#include "CodeGeneratorInterface.hpp"

/* Local inclusions for usages. */
#include "Libs/StaticVector.hpp"
#include "Declaration/Function.hpp"
#include "Declaration/PushConstantBlock.hpp"
#include "Declaration/Sampler.hpp"
#include "Declaration/ShaderStorageBlock.hpp"
#include "Declaration/SpecializationConstant.hpp"
#include "Declaration/Structure.hpp"
#include "Declaration/TexelBuffer.hpp"
#include "Declaration/UniformBlock.hpp"
#include "Types.hpp"

/* Forward declarations. */
namespace EmEn::Saphir::Generator
{
	class Abstract;
}

namespace EmEn::Saphir
{
	/**
	 * @brief Defines a shader that can be filled with source code and compiled. Or directly filled with binary code.
	 * @extends EmEn::Libs::NameableTrait
	 */
	class AbstractShader : public Libs::NameableTrait, public CodeGeneratorInterface
	{
		public:

			/**
			 * @brief Copy constructor.
			 * @param copy A reference to the copied instance.
			 */
			AbstractShader (const AbstractShader & copy) noexcept = delete;

			/**
			 * @brief Move constructor.
			 * @param copy A reference to the copied instance.
			 */
			AbstractShader (AbstractShader && copy) noexcept = delete;

			/**
			 * @brief Copy assignment.
			 * @param copy A reference to the copied instance.
			 * @return AbstractShader &
			 */
			AbstractShader & operator= (const AbstractShader & copy) noexcept = delete;

			/**
			 * @brief Move assignment.
			 * @param copy A reference to the copied instance.
			 * @return AbstractShader &
			 */
			AbstractShader & operator= (AbstractShader && copy) noexcept = delete;

			/**
			 * @brief Destructs an abstract shader.
			 */
			~AbstractShader () override = default;

			/**
			 * @brief Sets a GLSL extension behavior.
			 * @param extension The target extension.
			 * @param behavior The behavior.
			 * @return void
			 */
			void setExtensionBehavior (const char * extension, const char * behavior = nullptr) noexcept;

			/**
			 * @brief Declare a specialization constant to be used in the shader.
			 * @param declaration A reference to a SpecializationConstant.
			 * @return bool
			 */
			bool declare (const Declaration::SpecializationConstant & declaration) noexcept;

			/**
			 * @brief Declares a function to be used in the shader.
			 * @param declaration A reference to a ShaderFunction.
			 * @return bool
			 */
			bool declare (const Declaration::Function & declaration) noexcept;

			/**
			 * @brief Declare a structure to be used in the shader.
			 * @param declaration A reference to a ShaderStructure.
			 * @return bool
			 */
			bool declare (const Declaration::Structure & declaration) noexcept;

			/**
			 * @brief Declares a uniform block to be used in the shader.
			 * @param declaration A reference to a ShaderUniformBlock.
			 * @return bool
			 */
			bool declare (const Declaration::UniformBlock & declaration) noexcept;

			/**
			 * @brief Declares a shader storage block to be used in the shader.
			 * @param declaration A reference to a shader storage block.
			 * @return bool
			 */
			bool declare (const Declaration::ShaderStorageBlock & declaration) noexcept;

			/**
			 * @brief Declares a sampler to be used in the shader.
			 * @param declaration A reference to a Sampler.
			 * @return bool
			 */
			bool declare (const Declaration::Sampler & declaration) noexcept;

			/**
			 * @brief Declares a texel buffer to be used in the shader (Vulkan only).
			 * @param declaration A reference to a texel buffer.
			 * @return bool
			 */
			bool declare (const Declaration::TexelBuffer & declaration) noexcept;

			/**
			 * @brief Declares a push constant block to be used in the shader (Vulkan only).
			 * @param declaration A reference to a push constant block.
			 * @return bool
			 */
			bool declare (const Declaration::PushConstantBlock & declaration) noexcept;

			/**
			 * @brief Returns the list of specialization constant declarations.
			 * @return const std::vector< Declaration::SpecializationConstant > &
			 */
			[[nodiscard]]
			const std::vector< Declaration::SpecializationConstant > &
			specializationConstantDeclarations () const noexcept
			{
				return m_specializationConstants;
			}

			/**
			 * @brief Returns the list of function declarations.
			 * @return const std::vector< Declaration::Function > &
			 */
			[[nodiscard]]
			const std::vector< Declaration::Function > &
			functionDeclarations () const noexcept
			{
				return m_functions;
			}

			/**
			 * @brief Returns the list of structure declarations.
			 * @return const std::vector< Declaration::Structure > &
			 */
			[[nodiscard]]
			const std::vector< Declaration::Structure > &
			structureDeclarations () const noexcept
			{
				return m_structures;
			}

			/**
			 * @brief Returns the list of uniform block declarations.
			 * @return const std::vector< Declaration::UniformBlock > &
			 */
			[[nodiscard]]
			const std::vector< Declaration::UniformBlock > &
			uniformBlockDeclarations () const noexcept
			{
				return m_uniformBlocks;
			}

			/**
			 * @brief Returns the list of shader storage block declarations.
			 * @return const std::vector< Declaration::ShaderStorageBlock > &
			 */
			[[nodiscard]]
			const std::vector< Declaration::ShaderStorageBlock > &
			shaderStorageBlockDeclarations () const noexcept
			{
				return m_shaderStorageBlocks;
			}

			/**
			 * @brief Returns the list of sample declarations.
			 * @return const std::vector< Declaration::Sampler > &
			 */
			[[nodiscard]]
			const std::vector< Declaration::Sampler > &
			samplerDeclarations () const noexcept
			{
				return m_samplers;
			}

			/**
			 * @brief Returns the list of texel buffer declarations.
			 * @return const std::vector< Declaration::TexelBuffer > &
			 */
			[[nodiscard]]
			const std::vector< Declaration::TexelBuffer > &
			texelBufferDeclarations () const noexcept
			{
				return m_texelBuffers;
			}

			/**
			 * @brief Returns the list of push constant block declarations.
			 * @return const Libs::StaticVector< Declaration::PushConstantBlock, 4 > &
			 */
			[[nodiscard]]
			const Libs::StaticVector< Declaration::PushConstantBlock, 4 > &
			pushConstantBlockDeclarations () const noexcept
			{
				return m_pushConstantBlocks;
			}

			/**
			 * @brief Generates and returns the shader source code in GLSL.
			 * @param generator A reference to the shader generator.
			 * @return bool
			 */
			[[nodiscard]]
			bool generateSourceCode (Generator::Abstract & generator) noexcept;

			/**
			 * @brief Sets the source code for this shader.
			 * @param sourceCode The source code.
			 * @return void
			 */
			void
			setSourceCode (const std::string & sourceCode) noexcept
			{
				m_sourceCode = sourceCode;

				this->generateHash();
			}

			/**
			 * @brief Loads source code from a file.
			 * @param filepath A reference to a filesystem path.
			 * @return bool
			 */
			bool loadSourceCode (const std::filesystem::path & filepath) noexcept;

			/**
			 * @brief Writes the source code to a file.
			 * @param filepath A reference to a filesystem path.
			 * @return bool
			 */
			[[nodiscard]]
			bool writeSourceCode (const std::filesystem::path & filepath) const noexcept;

			/**
			 * @brief Returns the source code.
			 * @return const std::string &
			 */
			[[nodiscard]]
			const std::string &
			sourceCode () const noexcept
			{
				return m_sourceCode;
			}

			/**
			 * @brief Returns the hash of the source code.
			 * @return size_t
			 */
			[[nodiscard]]
			size_t
			hash () const noexcept
			{
				return m_sourceCodeHash;
			}

			/**
			 * @brief Returns whether a source code is present.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isGenerated () const noexcept
			{
				return !m_sourceCode.empty();
			}

			/**
			 * @brief Returns the number of declarations type in a string.
			 * @return std:string;
			 */
			[[nodiscard]]
			std::string getDeclarationStats () const noexcept;

			/**
			 * @brief Display with the tracer a successful shader generation.
			 * @return void
			 */
			void traceSuccessfulGeneration () const noexcept;

			/**
			 * @brief Returns the shader type.
			 * @return ShaderType
			 */
			[[nodiscard]]
			virtual ShaderType type () const noexcept = 0;

		protected:

			/**
			 * @brief Constructs an abstract shader.
			 * @param name The name of the shader for identification [std::move].
			 * @param GLSLVersion The GLSL version in use [std::move].
			 * @param GLSLProfile The GLSL profile in use [std::move].
			 */
			AbstractShader (std::string name, std::string GLSLVersion, std::string GLSLProfile) noexcept
				: NameableTrait{std::move(name)},
				m_GLSLVersion{std::move(GLSLVersion)},
				m_GLSLProfile{std::move(GLSLProfile)}
			{

			}

			/**
			 * @brief Generates the shader file header.
			 * @param code A reference to a stream.
			 * @return void
			 */
			void generateHeaders (std::stringstream & code) const noexcept;

			/**
			 * @brief Generates shader declarations.
			 * @tparam declaration_t The type of declaration. This should be derived from Declaration::Interface.
			 * @param code A reference to a stream.
			 * @param declarations A reference to a list of declaration.
			 * @param comment A section comment. Default none.
			 * @return void
			 */
			template< typename declaration_t >
			static
			void
			generateDeclarations (std::stringstream & code, const std::vector< declaration_t > & declarations, const char * comment = nullptr) noexcept requires (std::is_base_of_v< Declaration::Interface, declaration_t >)
			{
				if ( declarations.empty() )
				{
					return;
				}

				if ( comment != nullptr )
				{
					code << "/* " << comment << " */" "\n";
				}

				for ( const auto & declaration : declarations )
				{
					code << declaration.sourceCode();
				}

				code << '\n';
			}

			/**
			 * @brief Generates shader declarations.
			 * @tparam declaration_t The type of declaration. This should be derived from Declaration::Interface.
			 * @param code A reference to a stream.
			 * @param declarations A reference to a list of declaration.
			 * @param comment A section comment. Default none.
			 * @return void
			 */
			template< typename declaration_t >
			static
			void
			generateDeclarations (std::stringstream & code, const Libs::StaticVector< declaration_t, 4 > & declarations, const char * comment = nullptr) noexcept requires (std::is_base_of_v< Declaration::Interface, declaration_t >)
			{
				if ( declarations.empty() )
				{
					return;
				}

				if ( comment != nullptr )
				{
					code << "/* " << comment << " */" "\n";
				}

				for ( const auto & declaration : declarations )
				{
					code << declaration.sourceCode();
				}

				code << '\n';
			}

			/**
			 * @brief Called from the child class for generating the source code.
			 * @param generator A reference to the generator.
			 * @param code A reference to string.
			 * @param topInstructions A reference to a string for the main top instructions.
			 * @param outputInstructions A reference to a string for the main output instructions.
			 * @return bool
			 */
			[[nodiscard]]
			virtual bool onSourceCodeGeneration (Generator::Abstract & generator, std::stringstream & code, std::string & topInstructions, std::string & outputInstructions) noexcept = 0;

			/**
			 * @brief Called from the child class for generating declaration stats.
			 * @param output A reference to a string stream.
			 * @return void
			 */
			virtual void onGetDeclarationStats (std::stringstream & output) const noexcept = 0;

		private:

			/**
			 * @brief Generates a hash from the source code.
			 * @return void
			 */
			virtual void generateHash () noexcept;

			std::string m_GLSLVersion;
			std::string m_GLSLProfile;
			std::string m_sourceCode;
			size_t m_sourceCodeHash{0};
			std::vector< std::string > m_headers;
			std::vector< Declaration::SpecializationConstant > m_specializationConstants; /* Special case before compilation */
			std::vector< Declaration::Function > m_functions;
			std::vector< Declaration::Structure > m_structures;
			std::vector< Declaration::UniformBlock > m_uniformBlocks; /* UBO */
			std::vector< Declaration::ShaderStorageBlock > m_shaderStorageBlocks; /* SSBO */
			std::vector< Declaration::Sampler > m_samplers;
			std::vector< Declaration::TexelBuffer > m_texelBuffers; /* (Vulkan only) */
			Libs::StaticVector< Declaration::PushConstantBlock, 4 > m_pushConstantBlocks; /* (Vulkan only) FIXME: Only one is authorized ! */
	};
}
