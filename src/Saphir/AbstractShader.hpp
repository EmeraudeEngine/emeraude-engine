/*
 * src/Saphir/AbstractShader.hpp
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
#include <filesystem>
#include <string>
#include <vector>

/* Local inclusions for inheritances. */
#include "NameableTrait.hpp"
#include "CodeGeneratorInterface.hpp"

/* Local inclusions for usages. */
#include "Declaration/Function.hpp"
#include "Declaration/PushConstantBlock.hpp"
#include "Declaration/Sampler.hpp"
#include "Declaration/ShaderStorageBlock.hpp"
#include "Declaration/SpecializationConstant.hpp"
#include "Declaration/Structure.hpp"
#include "Declaration/TexelBuffer.hpp"
#include "Declaration/UniformBlock.hpp"
#include "StaticVector.hpp"
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
	 * @extends EmEn::Base::NameableTrait
	 */
	class AbstractShader : public Base::NameableTrait, public CodeGeneratorInterface
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
			 * @note Emits a "#extension <extension> : <behavior>" preprocessor line, added to the shader header on the next generateSourceCode() call.
			 * @param extension The target extension name (e.g. "GL_EXT_buffer_reference").
			 * @param behavior The behavior keyword ("enable", "require", "warn" or "disable", see Keys::GLSL::Extension).
			 * @warning The default value of `behavior` is a trap: it is appended unconditionally to the generated line, and a null pointer passed to std::string::operator+= is undefined behavior. Every call site must supply `behavior` explicitly.
			 * @return void
			 */
			void setExtensionBehavior (const char * extension, const char * behavior = nullptr) noexcept;

			/**
			 * @brief Declare a specialization constant to be used in the shader.
			 * @note If a declaration with the same name already exists, it is silently kept as-is (a warning is traced) and this still returns true; the return value only distinguishes an invalid declaration (false) from an accepted-or-already-present one (true).
			 * @param declaration A reference to a SpecializationConstant.
			 * @return bool
			 */
			bool declare (const Declaration::SpecializationConstant & declaration) noexcept;

			/**
			 * @brief Declares a function to be used in the shader.
			 * @note Deduplicated by function name: a second declaration with the same name is silently kept as-is (a warning is traced) and this still returns true.
			 * @param declaration A reference to a ShaderFunction.
			 * @return bool
			 */
			bool declare (const Declaration::Function & declaration) noexcept;

			/**
			 * @brief Declare a structure to be used in the shader.
			 * @note Deduplicated by structure name: a second declaration with the same name is silently kept as-is (a warning is traced) and this still returns true.
			 * @param declaration A reference to a ShaderStructure.
			 * @return bool
			 */
			bool declare (const Declaration::Structure & declaration) noexcept;

			/**
			 * @brief Declares a uniform block to be used in the shader.
			 * @note Deduplicated by instance name (not block name): a second declaration with the same instance name is silently kept as-is (a warning is traced) and this still returns true.
			 * @param declaration A reference to a ShaderUniformBlock.
			 * @return bool
			 */
			bool declare (const Declaration::UniformBlock & declaration) noexcept;

			/**
			 * @brief Declares a shader storage block to be used in the shader.
			 * @note Deduplicated by instance name (not block name): a second declaration with the same instance name is silently kept as-is (a warning is traced) and this still returns true.
			 * @param declaration A reference to a shader storage block.
			 * @return bool
			 */
			bool declare (const Declaration::ShaderStorageBlock & declaration) noexcept;

			/**
			 * @brief Declares a sampler to be used in the shader.
			 * @note Deduplicated by name. For an unbounded (bindless) sampler array, a re-declaration is expected to be byte-identical (a fixed name always maps to the same set/binding/type) and is silently ignored with no warning, since several independent generators legitimately declare the same bindless array. A named, non-unbounded sampler re-declared under the same name still traces a warning, since that combination is more likely a genuine binding conflict. Either way this returns true.
			 * @param declaration A reference to a Sampler.
			 * @return bool
			 */
			bool declare (const Declaration::Sampler & declaration) noexcept;

			/**
			 * @brief Declares a texel buffer to be used in the shader (Vulkan only).
			 * @note Deduplicated by name: a second declaration with the same name is silently kept as-is (a warning is traced) and this still returns true.
			 * @param declaration A reference to a texel buffer.
			 * @return bool
			 */
			bool declare (const Declaration::TexelBuffer & declaration) noexcept;

			/**
			 * @brief Declares a push constant block to be used in the shader (Vulkan only).
			 * @note Deduplicated by name: a second declaration with the same name is silently kept as-is (a warning is traced) and this still returns true.
			 * @warning FIXME (see m_pushConstantBlocks): only a single push constant block is actually authorized per shader, but this method does not enforce that limit — it only rejects an invalid declaration or a name collision. Declaring more than one distinct push constant block currently succeeds silently, even though the storage backing it (Base::StaticVector<..., 4>) and the rest of the pipeline assume at most one.
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
			 * @warning FIXME: the backing storage allows up to 4 entries, but only a single push constant block is actually authorized per shader (see declare(const Declaration::PushConstantBlock &)); this accessor does not re-check that invariant, it only reflects whatever was declared.
			 * @return const Base::StaticVector< Declaration::PushConstantBlock, 4 > &
			 */
			[[nodiscard]]
			const Base::StaticVector< Declaration::PushConstantBlock, 4 > &
			pushConstantBlockDeclarations () const noexcept
			{
				return m_pushConstantBlocks;
			}

			/**
			 * @brief Generates and returns the shader source code in GLSL.
			 * @note Expects every declare() call and, for stages that need it, the specific setup (stage inputs/outputs, extensions) to have already been done. Internally writes the "#version"/extension header, delegates the stage-specific body to the pure virtual onSourceCodeGeneration() hook, appends the common declarations (specialization constants, functions, structures, uniform/storage blocks, samplers, texel buffers, push constant block), then assembles main() from the instructions collected through CodeGeneratorInterface. On success this replaces any previously generated or loaded source code and refreshes hash().
			 * @param generator A reference to the shader generator driving this generation pass.
			 * @return bool false if the stage-specific onSourceCodeGeneration() hook fails; the shader keeps whatever source code (if any) it had before the call.
			 */
			[[nodiscard]]
			bool generateSourceCode (Generator::Abstract & generator) noexcept;

			/**
			 * @brief Sets the source code for this shader.
			 * @note Alternative to generateSourceCode(): assigns raw GLSL directly (e.g. hand-written or externally produced), bypassing the declaration/generator pipeline entirely, and refreshes hash().
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
			 * @note The file extension must match this shader's stage (see getShaderFileExtension(type())), otherwise this fails without reading the file. On success refreshes hash().
			 * @param filepath A reference to a filesystem path.
			 * @return bool false if the extension does not match or the file could not be read.
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
			 * @note Kept in sync with sourceCode() by generateSourceCode(), setSourceCode() and loadSourceCode(); reads as 0 when no source code has been generated or loaded yet.
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
			 * @brief Builds a human-readable, multi-line summary of how many declarations of each kind (specialization constants, functions, structures, uniform/storage blocks, samplers, texel buffers, push constant block) this shader holds, followed by the stage-specific counts appended by onGetDeclarationStats().
			 * @return std::string
			 */
			[[nodiscard]]
			std::string getDeclarationStats () const noexcept;

			/**
			 * @brief Display with the tracer a successful shader generation.
			 * @note Does not check isGenerated(): calling this before generateSourceCode()/setSourceCode()/loadSourceCode() traces an empty GLSL body. Dumps the full generated GLSL source (via SourceCodeParser) followed by getDeclarationStats(), at Info level; intended to be called right after a successful generateSourceCode().
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
			 * @note Called once by generateSourceCode(), before onSourceCodeGeneration(); writes the "#version"/profile line (which must stay the very first line of the GLSL source), then one preprocessor line per extension registered via setExtensionBehavior(), then a comment naming the shader type and name. Not meant to be invoked directly by subclasses.
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
			generateDeclarations (std::stringstream & code, const Base::StaticVector< declaration_t, 4 > & declarations, const char * comment = nullptr) noexcept requires (std::is_base_of_v< Declaration::Interface, declaration_t >)
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
			 * @brief Hook implemented by each concrete shader stage to generate its stage-specific part of the source code.
			 * @note Invoked once by generateSourceCode(), after the "#version"/extension header and before the common declarations (specialization constants, functions, structures, blocks, samplers, ...) are appended. The override must write its own stage-specific declarations (e.g. stage inputs/outputs) directly to `code`, typically via the generateDeclarations() helper. `topInstructions` and `outputInstructions` are out-parameters: whatever the override assigns to them is passed as the "prepend" arguments to CodeGeneratorInterface::getCode(), i.e. prepended to the main() body before/after the instructions collected via addTopInstruction()/addInstruction()/addOutputInstruction(). Returning false aborts the whole generateSourceCode() call.
			 * @param generator A reference to the generator driving this generation pass.
			 * @param code A reference to the stream the stage-specific declarations must be written to.
			 * @param topInstructions A reference to a string of GLSL prepended before the main() top instructions.
			 * @param outputInstructions A reference to a string of GLSL prepended before the main() output instructions.
			 * @return bool false to abort code generation for this shader.
			 */
			[[nodiscard]]
			virtual bool onSourceCodeGeneration (Generator::Abstract & generator, std::stringstream & code, std::string & topInstructions, std::string & outputInstructions) noexcept = 0;

			/**
			 * @brief Hook implemented by each concrete shader stage to append its stage-specific declaration counts.
			 * @note Invoked once by getDeclarationStats(), after the common declaration counts have already been written to `output`; the override only needs to append its own stage-specific counts (e.g. stage inputs/outputs) to the same stream.
			 * @param output A reference to the string stream the stage-specific stats must be appended to.
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
			Base::StaticVector< Declaration::PushConstantBlock, 4 > m_pushConstantBlocks; /* (Vulkan only) FIXME: Only one is authorized ! */
	};
}
