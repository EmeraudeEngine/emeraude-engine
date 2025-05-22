/*
 * src/Saphir/AbstractShader.cpp
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

#include "AbstractShader.hpp"

/* STL inclusions. */
#include <algorithm>
#include <functional>

/* Local inclusions. */
#include "Libs/IO/IO.hpp"
#include "Generator/Abstract.hpp"
#include "Keys.hpp"
#include "Tracer.hpp"

namespace EmEn::Saphir
{
	using namespace EmEn::Libs;
	using namespace Declaration;
	
	constexpr auto TracerTag{"Shader"};

	AbstractShader::AbstractShader (const std::string & name, std::string GLSLVersion, std::string GLSLProfile) noexcept
		: NameableTrait(name), m_GLSLVersion(std::move(GLSLVersion)), m_GLSLProfile(std::move(GLSLProfile))
	{

	}

	void
	AbstractShader::setExtensionBehavior (const char * extension, const char * behavior) noexcept
	{
		std::stringstream headers;
		headers << Keys::GLSL::Define::Extension << ' ' << extension << " : " << behavior;

		m_headers.emplace_back(headers.str());
	}

	bool
	AbstractShader::declare (const SpecializationConstant & declaration) noexcept
	{
		if ( !declaration.isValid() )
		{
			Tracer::error(TracerTag, "The specialization constant is invalid for code generation !");

			return false;
		}

		if ( std::ranges::any_of(m_specializationConstants, [declaration] (const auto & existing) {return existing.name() == declaration.name();}) )
		{
			TraceWarning{TracerTag} << "A specialization constant declaration named '" << declaration.name() << "' already exists !";

			return true;
		}

		m_specializationConstants.emplace_back(declaration);

		return true;
	}

	bool
	AbstractShader::declare (const Function & declaration) noexcept
	{
		if ( !declaration.isValid() )
		{
			Tracer::error(TracerTag, "The function is invalid for code generation !");

			return false;
		}

		if ( std::ranges::any_of(m_functions, [declaration] (const auto & existing) {return existing.name() == declaration.name();}) )
		{
			TraceWarning{TracerTag} << "A function declaration named '" << declaration.name() << "' already exists !";

			return true;
		}

		m_functions.emplace_back(declaration);

		return true;
	}

	bool
	AbstractShader::declare (const Structure & declaration) noexcept
	{
		if ( !declaration.isValid() )
		{
			Tracer::error(TracerTag, "The structure is invalid for code generation !");

			return false;
		}

		if ( std::ranges::any_of(m_structures, [declaration] (const auto & existing) {return existing.name() == declaration.name();}) )
		{
			TraceWarning{TracerTag} << "A structure declaration named '" << declaration.name() << "' already exists !";

			return true;
		}

		m_structures.emplace_back(declaration);

		return true;
	}

	bool
	AbstractShader::declare (const UniformBlock & declaration) noexcept
	{
		if ( !declaration.isValid() )
		{
			Tracer::error(TracerTag, "The uniform block is invalid for code generation !");

			return false;
		}

		if ( std::ranges::any_of(m_uniformBlocks, [declaration] (const auto & existing) {return existing.instanceName() == declaration.instanceName();}) )
		{
			TraceWarning{TracerTag} << "An uniform block declaration named '" << declaration.name() << "' already exists !";

			return true;
		}

		m_uniformBlocks.emplace_back(declaration);

		return true;
	}

	bool
	AbstractShader::declare (const ShaderStorageBlock & declaration) noexcept
	{
		if ( !declaration.isValid() )
		{
			Tracer::error(TracerTag, "The shader storage block is invalid for code generation !");

			return false;
		}

		if ( std::ranges::any_of(m_shaderStorageBlocks, [declaration] (const auto & existing) {return existing.instanceName() == declaration.instanceName();}) )
		{
			TraceWarning{TracerTag} << "A shader storage block declaration named '" << declaration.name() << "' already exists !";

			return true;
		}

		m_shaderStorageBlocks.emplace_back(declaration);

		return true;
	}

	bool
	AbstractShader::declare (const Sampler & declaration) noexcept
	{
		if ( !declaration.isValid() )
		{
			Tracer::error(TracerTag, "The sampler declaration is invalid for code generation !");

			return false;
		}

		if ( std::ranges::any_of(m_samplers, [declaration] (const auto & existing) {return existing.name() == declaration.name();}) )
		{
			TraceWarning{TracerTag} << "A sampler declaration named '" << declaration.name() << "' already exists !";

			return true;
		}

		m_samplers.emplace_back(declaration);

		return true;
	}

	bool
	AbstractShader::declare (const TexelBuffer & declaration) noexcept
	{
		if ( !declaration.isValid() )
		{
			Tracer::error(TracerTag, "The texel buffer is invalid for code generation !");

			return false;
		}

		if ( std::ranges::any_of(m_texelBuffers, [declaration] (const auto & existing) {return existing.name() == declaration.name();}) )
		{
			TraceWarning{TracerTag} << "A texel buffer declaration named '" << declaration.name() << "' already exists !";

			return true;
		}

		m_texelBuffers.emplace_back(declaration);

		return true;
	}

	bool
	AbstractShader::declare (const PushConstantBlock & declaration) noexcept
	{
		if ( !declaration.isValid() )
		{
			Tracer::error(TracerTag, "The push constant is invalid for code generation !");

			return false;
		}

		if ( std::ranges::any_of(m_pushConstantBlocks, [declaration] (const auto & existing) {return existing.name() == declaration.name();}) )
		{
			TraceWarning{TracerTag} << "A push constant declaration named '" << declaration.name() << "' already exists !";

			return true;
		}

		m_pushConstantBlocks.emplace_back(declaration);

		return true;
	}

	bool
	AbstractShader::generateSourceCode (Generator::Abstract & generator) noexcept
	{
		std::stringstream code{};
		std::string topInstructions{};
		std::string outputInstructions{};

		this->generateHeaders(code);

		if ( !this->onSourceCodeGeneration(generator, code, topInstructions, outputInstructions) )
		{
			return false;
		}

		/* Common shader code declarations. */
		AbstractShader::generateDeclarations(code, this->specializationConstantDeclarations(), "Specialization constants");
		AbstractShader::generateDeclarations(code, this->functionDeclarations(), "Functions");
		AbstractShader::generateDeclarations(code, this->structureDeclarations(), "Structures");
		AbstractShader::generateDeclarations(code, this->uniformBlockDeclarations(), "Uniform blocks (OpenGL/Vulkan UBO)");
		AbstractShader::generateDeclarations(code, this->shaderStorageBlockDeclarations(), "Shader storage blocks (OpenGL/Vulkan SSBO)");
		AbstractShader::generateDeclarations(code, this->samplerDeclarations(), "Samplers");
		AbstractShader::generateDeclarations(code, this->texelBufferDeclarations(), "Texel buffers");
		AbstractShader::generateDeclarations(code, this->pushConstantBlockDeclarations(), "Push constant block (Vulkan)");

		/* NOTE: Generate the main() function code. */
		code <<
			 "void main ()" "\n"
			 "{" "\n" <<
			 this->getCode(topInstructions, outputInstructions) <<
			 "}" "\n";

		m_sourceCode = code.str();

		this->generateHash();

		return true;
	}

	std::string
	AbstractShader::getDeclarationStats () const noexcept
	{
		std::stringstream output;

		output <<
			"Common shader declarations : " "\n"
			" - Specialization constant : " << m_specializationConstants.size() << "\n"
			" - Function : " << m_functions.size() << "\n"
			" - Structure : " << m_structures.size() << "\n"
			" - Uniform block : " << m_uniformBlocks.size() << "\n"
			" - Shader storage block : " << m_shaderStorageBlocks.size() << "\n"
			" - Sampler : " << m_samplers.size() << "\n"
			" - Texel : " << m_texelBuffers.size() << "\n"
			" - Push constant block (<= 1) : " << m_pushConstantBlocks.size() << "\n";

		this->onGetDeclarationStats(output);

		return output.str();
	}

	void
	AbstractShader::generateHeaders (std::stringstream & code) const noexcept
	{
		/* NOTE: This must be the first line of the source code.
		 * This is the version and the profile of the current shader. */
		code << Keys::GLSL::Define::Version << ' ' << m_GLSLVersion << ' ' << m_GLSLProfile << '\n';

		/* Adds defines and other pre-processor instructions. */
		if ( !m_headers.empty() )
		{
			for ( const auto & line : m_headers )
			{
				code << line << '\n';
			}
		}

		code << '\n';

//#if IS_EMBEDDED_PLATFORM
//		code << Keys::GLSL::Precision << ' ' << Keys::GLSL::Medium << ' ' << Keys::GLSL::Float << ';' << "\n\n";
//#endif

		code << "/* " << to_string(this->type()) << " : " << this->name() << " */" "\n\n";
	}

	bool
	AbstractShader::loadSourceCode (const std::filesystem::path & filepath) noexcept
	{
		const auto * extension = getShaderFileExtension(this->type());

		if ( IO::getFileExtension(filepath) != extension )
		{
			TraceError{TracerTag} << "The shader source file extension must be '" << extension << "' !";

			return false;
		}

		if ( !IO::fileGetContents(filepath, m_sourceCode) )
		{
			TraceError{TracerTag} << "Unable to read the shader source code from file '" << filepath << "' !";

			return false;
		}

		this->generateHash();

		return true;
	}

	bool
	AbstractShader::writeSourceCode (const std::filesystem::path & filepath) const noexcept
	{
		if ( !IO::filePutContents(filepath, m_sourceCode) )
		{
			TraceError{TracerTag} << "Unable to write the shader source code to file '" << filepath << "' !";

			return false;
		}

		return true;
	}

	void
	AbstractShader::generateHash () noexcept
	{
		if ( m_sourceCode.empty() )
		{
			m_sourceCodeHash = 0;
		}
		else
		{
			const std::hash< std::string > hashString{};

			m_sourceCodeHash = hashString(m_sourceCode);
		}
	}
}
