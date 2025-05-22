/*
 * src/Saphir/ShaderManager.cpp
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

#include "ShaderManager.hpp"

/* STL inclusions. */
#include <string>
#include <algorithm>

/* Third-party inclusions. */
#ifdef EMERAUDE_USE_SYSTEM_LIBS
#include <glslang/SPIRV/GlslangToSpv.h>
#else
#include "SPIRV/GlslangToSpv.h"
#endif

/* Local inclusions. */
#include "Libs/SourceCodeParser.hpp"
#include "Libs/IO/IO.hpp"
#include "Vulkan/ShaderModule.hpp"
#include "AbstractShader.hpp"
#include "PrimaryServices.hpp"
#include "SettingKeys.hpp"

namespace EmEn::Saphir
{
	using namespace EmEn::Libs;
	using namespace Vulkan;

	const size_t ShaderManager::ClassUID{getClassUID(ClassId)};
	std::array< ShaderManager *, 2 > ShaderManager::s_instances{nullptr, nullptr};

	ShaderManager::ShaderManager (PrimaryServices & primaryServices, GPUWorkType type) noexcept
		: ServiceInterface(ClassId),
		m_primaryServices(primaryServices)
	{
		if ( s_instances.at(static_cast< size_t >(type)) != nullptr )
		{
			std::cerr << __PRETTY_FUNCTION__ << ", constructor called twice !" "\n";

			std::terminate();
		}

		s_instances.at(static_cast< size_t >(type)) = this;
	}

	ShaderManager::~ShaderManager ()
	{
		for ( auto & pointer : s_instances )
		{
			if ( pointer == this )
			{
				pointer = nullptr;

				break;
			}
		}
	}

	bool
	ShaderManager::onInitialize () noexcept
	{
		/* Read core settings. */
		{
			auto & settings = m_primaryServices.settings();

			m_flags[ShowInformation] = settings.get< bool >(VkShowInformationKey, DefaultVkShowInformation);
			m_flags[ShowSourceCode] = settings.get< bool >(ShowSourceCodeKey, DefaultShowSourceCode);
			m_flags[SourceCodeCacheEnabled] = settings.get< bool >(SourceCodeCacheEnabledKey, DefaultSourceCodeCacheEnabled);
			m_flags[BinaryCacheEnabled] = settings.get< bool >(BinaryCacheEnabledKey, DefaultBinaryCacheEnabled);
		}

		/* Shader source cache directory. */
		m_shadersSourcesDirectory = m_primaryServices.fileSystem().cacheDirectory(ShaderSourcesDirectoryName);

		if ( !IO::createDirectory(m_shadersSourcesDirectory) )
		{
			TraceError{ClassId} << "Unable to create '" << m_shadersSourcesDirectory << "' directory !";

			return false;
		}

		/* Shader binaries cache directory. */
		m_shadersBinariesDirectory = m_primaryServices.fileSystem().cacheDirectory(ShaderBinariesDirectoryName);

		if ( !IO::createDirectory(m_shadersBinariesDirectory) )
		{
			TraceError{ClassId} << "Unable to create '" << m_shadersBinariesDirectory << "' directory !";

			return false;
		}

		/* Checks shader cache. */
		if ( m_primaryServices.arguments().get("--clear-shader-cache").isPresent() )
		{
			this->clearCache();
		}
		else
		{
			this->readCache();
		}

		if ( m_flags[ShowInformation] )
		{
			TraceInfo{ClassId} << "GLSLang GLSL version supported : " << glslang::GetGlslVersionString();
		}

		if ( !glslang::InitializeProcess() )
		{
			Tracer::error(ClassId, "Unable to initialize GLSLang process !");

			return false;
		}

		m_builtInResource.maxLights = 32;
		m_builtInResource.maxClipPlanes = 6;
		m_builtInResource.maxTextureUnits = 32;
		m_builtInResource.maxTextureCoords = 32;
		m_builtInResource.maxVertexAttribs = 64;
		m_builtInResource.maxVertexUniformComponents = 4096;
		m_builtInResource.maxVaryingFloats = 64;
		m_builtInResource.maxVertexTextureImageUnits = 32;
		m_builtInResource.maxCombinedTextureImageUnits = 80;
		m_builtInResource.maxTextureImageUnits = 32;
		m_builtInResource.maxFragmentUniformComponents = 4096;
		m_builtInResource.maxDrawBuffers = 32;
		m_builtInResource.maxVertexUniformVectors = 128;
		m_builtInResource.maxVaryingVectors = 8;
		m_builtInResource.maxFragmentUniformVectors = 16;
		m_builtInResource.maxVertexOutputVectors = 16;
		m_builtInResource.maxFragmentInputVectors = 15;
		m_builtInResource.minProgramTexelOffset = -8;
		m_builtInResource.maxProgramTexelOffset = 7;
		m_builtInResource.maxClipDistances = 8;
		m_builtInResource.maxComputeWorkGroupCountX = 65535;
		m_builtInResource.maxComputeWorkGroupCountY = 65535;
		m_builtInResource.maxComputeWorkGroupCountZ = 65535;
		m_builtInResource.maxComputeWorkGroupSizeX = 1024;
		m_builtInResource.maxComputeWorkGroupSizeY = 1024;
		m_builtInResource.maxComputeWorkGroupSizeZ = 64;
		m_builtInResource.maxComputeUniformComponents = 1024;
		m_builtInResource.maxComputeTextureImageUnits = 16;
		m_builtInResource.maxComputeImageUniforms = 8;
		m_builtInResource.maxComputeAtomicCounters = 8;
		m_builtInResource.maxComputeAtomicCounterBuffers = 1;
		m_builtInResource.maxVaryingComponents = 60;
		m_builtInResource.maxVertexOutputComponents = 64;
		m_builtInResource.maxGeometryInputComponents = 64;
		m_builtInResource.maxGeometryOutputComponents = 128;
		m_builtInResource.maxFragmentInputComponents = 128;
		m_builtInResource.maxImageUnits = 8;
		m_builtInResource.maxCombinedImageUnitsAndFragmentOutputs = 8;
		m_builtInResource.maxCombinedShaderOutputResources = 8;
		m_builtInResource.maxImageSamples = 0;
		m_builtInResource.maxVertexImageUniforms = 0;
		m_builtInResource.maxTessControlImageUniforms = 0;
		m_builtInResource.maxTessEvaluationImageUniforms = 0;
		m_builtInResource.maxGeometryImageUniforms = 0;
		m_builtInResource.maxFragmentImageUniforms = 8;
		m_builtInResource.maxCombinedImageUniforms = 8;
		m_builtInResource.maxGeometryTextureImageUnits = 16;
		m_builtInResource.maxGeometryOutputVertices = 256;
		m_builtInResource.maxGeometryTotalOutputComponents = 1024;
		m_builtInResource.maxGeometryUniformComponents = 1024;
		m_builtInResource.maxGeometryVaryingComponents = 64;
		m_builtInResource.maxTessControlInputComponents = 128;
		m_builtInResource.maxTessControlOutputComponents = 128;
		m_builtInResource.maxTessControlTextureImageUnits = 16;
		m_builtInResource.maxTessControlUniformComponents = 1024;
		m_builtInResource.maxTessControlTotalOutputComponents = 4096;
		m_builtInResource.maxTessEvaluationInputComponents = 128;
		m_builtInResource.maxTessEvaluationOutputComponents = 128;
		m_builtInResource.maxTessEvaluationTextureImageUnits = 16;
		m_builtInResource.maxTessEvaluationUniformComponents = 1024;
		m_builtInResource.maxTessPatchComponents = 120;
		m_builtInResource.maxPatchVertices = 32;
		m_builtInResource.maxTessGenLevel = 64;
		m_builtInResource.maxViewports = 16;
		m_builtInResource.maxVertexAtomicCounters = 0;
		m_builtInResource.maxTessControlAtomicCounters = 0;
		m_builtInResource.maxTessEvaluationAtomicCounters = 0;
		m_builtInResource.maxGeometryAtomicCounters = 0;
		m_builtInResource.maxFragmentAtomicCounters = 8;
		m_builtInResource.maxCombinedAtomicCounters = 8;
		m_builtInResource.maxAtomicCounterBindings = 1;
		m_builtInResource.maxVertexAtomicCounterBuffers = 0;
		m_builtInResource.maxTessControlAtomicCounterBuffers = 0;
		m_builtInResource.maxTessEvaluationAtomicCounterBuffers = 0;
		m_builtInResource.maxGeometryAtomicCounterBuffers = 0;
		m_builtInResource.maxFragmentAtomicCounterBuffers = 1;
		m_builtInResource.maxCombinedAtomicCounterBuffers = 1;
		m_builtInResource.maxAtomicCounterBufferSize = 16384;
		m_builtInResource.maxTransformFeedbackBuffers = 4;
		m_builtInResource.maxTransformFeedbackInterleavedComponents = 64;
		m_builtInResource.maxCullDistances = 8;
		m_builtInResource.maxCombinedClipAndCullDistances = 8;
		m_builtInResource.maxSamples = 4;

		m_builtInResource.limits.nonInductiveForLoops = true;
		m_builtInResource.limits.whileLoops = true;
		m_builtInResource.limits.doWhileLoops = true;
		m_builtInResource.limits.generalUniformIndexing = true;
		m_builtInResource.limits.generalAttributeMatrixVectorIndexing = true;
		m_builtInResource.limits.generalVaryingIndexing = true;
		m_builtInResource.limits.generalSamplerIndexing = true;
		m_builtInResource.limits.generalVariableIndexing = true;
		m_builtInResource.limits.generalConstantMatrixVectorIndexing = true;

		//m_includer.pushExternalLocalDirectory("");

		m_flags[ServiceInitialized] = true;

		return true;
	}

	bool
	ShaderManager::onTerminate () noexcept
	{
		if ( m_flags[ServiceInitialized] )
		{
			m_flags[ServiceInitialized] = false;

			glslang::FinalizeProcess();
		}

		m_cachedShaderBinaries.clear();
		m_cachedShaderSourceCodes.clear();
		m_shaderModules.clear();

		return true;
	}

	bool
	ShaderManager::cacheShaderSourceCode (const AbstractShader & shader) const noexcept
	{
		if ( !m_flags[SourceCodeCacheEnabled] )
		{
			return true;
		}

		const auto cacheFilepath = this->generateShaderSourceCacheFilepath(shader);

		if ( cacheFilepath.empty() )
		{
			TraceError{ClassId} << "Unable to get a proper source cache path for shader '" << shader.name() << "' !";

			return false;
		}

		if ( !shader.writeSourceCode(cacheFilepath) )
		{
			TraceError{ClassId} << "Unable to write the source cache file '" << cacheFilepath << "' for shader '" << shader.name() << "' !";

			return false;
		}

		return true;
	}

	bool
	ShaderManager::cacheShaderBinary (const AbstractShader & shader, const std::vector< uint32_t > & binaryCode) const noexcept
	{
		if ( !m_flags[BinaryCacheEnabled] )
		{
			return true;
		}

		const auto cacheFilepath = this->generateShaderBinaryCacheFilepath(shader);

		if ( cacheFilepath.empty() )
		{
			TraceError{ClassId} << "Unable to get a proper binary cache path for shader '" << shader.name() << "' !";

			return false;
		}

		if ( !IO::filePutContents(cacheFilepath, binaryCode) )
		{
			TraceError{ClassId} << "Unable to write the shader binary code to file '" << cacheFilepath << "' for shader '" << shader.name() << "' !";

			return false;
		}

		return true;
	}

	bool
	ShaderManager::checkBinaryFromCache (const AbstractShader & shader, std::vector< uint32_t > & binaryCode) noexcept
	{
		if ( !m_flags[BinaryCacheEnabled] )
		{
			return false;
		}

		const auto binaryIt = m_cachedShaderBinaries.find(shader.hash());

		if ( binaryIt == m_cachedShaderBinaries.cend() )
		{
			return false;
		}

		if ( !IO::fileGetContents(binaryIt->second, binaryCode) )
		{
			TraceError{ClassId} << "Unable to read the shader binary code from file '" << binaryIt->second << "' !";

			return false;
		}

		if ( binaryCode.empty() )
		{
			TraceError{ClassId} << "The shader binary code from file '" << binaryIt->second << "' is empty !";

			return false;
		}

		return true;
	}

	std::shared_ptr< ShaderModule >
	ShaderManager::getShaderModuleFromGeneratedShader (const std::shared_ptr< Device > & device, const AbstractShader & shader) noexcept
	{
		if ( !this->usable() )
		{
			Tracer::error(ClassId, "There is no device to load create the shader module !");

			return {};
		}

		/* The shader must have a source code before anything else.
		 * The hash depend on it. */
		if ( !shader.isGenerated() )
		{
			TraceError{ClassId} <<
				"The shader '" << shader.name() << "' is empty ! "
				"Generate it first.";

			return {};
		}

		if ( m_flags[ShowSourceCode] )
		{
			TraceInfo{ClassId} << "\n"
				"****** START OF LOADED GLSL SHADER CODE ******" "\n" <<
				SourceCodeParser::parse(shader.sourceCode()) <<
				"******  END OF LOADED GLSL SHADER CODE  ******" "\n";
		}

		const auto shaderHash = shader.hash();

		/* Checks in loaded shader list with the hash. */
		const auto shaderIt = m_shaderModules.find(shaderHash);

		if ( shaderIt != m_shaderModules.cend() )
		{
			return shaderIt->second;
		}

		std::vector< uint32_t > binaryCode;

		/* Checks in cached binaries to prevent a compilation. */
		if ( this->checkBinaryFromCache(shader, binaryCode) )
		{
			const auto bytes = binaryCode.size() * sizeof(uint32_t);

			TraceSuccess{ClassId} << "The shader '" << shader.name() << "' (" << bytes << " bytes) loaded from binary cache !";
		}
		/* If not, we compile it. */
		else
		{
			/* Write the source code to the cache. */
			if ( !this->cacheShaderSourceCode(shader) )
			{
				TraceWarning{ClassId} << "Unable to write the source code of shader '" << shader.name() << "' to the cache !";
			}

			if ( !this->compile(shader, binaryCode) )
			{
				TraceError{ClassId} << "Unable to compile shader '" << shader.name() << "' !";

				return {};
			}

			if ( !this->cacheShaderBinary(shader, binaryCode) )
			{
				TraceWarning{ClassId} << "Unable to write the binary code of shader '" << shader.name() << "' to the cache !";
			}
		}

		auto shaderModule = std::make_shared< ShaderModule >(device, ShaderManager::vkShaderType(shader.type()), binaryCode);
		shaderModule->setIdentifier(ClassId, shader.name(), "ShaderModule");

		if ( !shaderModule->createOnHardware() )
		{
			Tracer::error(ClassId, "Unable to create a shader module !");

			return {};
		}

		/* Save a copy into loaded shaders with the associated vulkan shader module. */
		const auto [newShader, success] = m_shaderModules.emplace(shaderHash, shaderModule);

		return newShader->second;
	}

	std::shared_ptr< ShaderModule >
	ShaderManager::getShaderModuleFromSourceCode (const std::shared_ptr< Device > & device, const std::string & shaderName, ShaderType shaderType, const std::string & sourceCode) noexcept
	{
		if ( !this->usable() )
		{
			Tracer::error(ClassId, "There is no device to load create the shader module !");

			return {};
		}

		// TODO: This version do not use cache or save.

		std::vector< uint32_t > binaryCode;

		if ( !this->compile(shaderName, shaderType, sourceCode, binaryCode) )
		{
			TraceError{ClassId} << "Unable to compile shader '" << shaderName << "' !";

			return {};
		}

		auto shaderModule = std::make_shared< ShaderModule >(device, ShaderManager::vkShaderType(shaderType), binaryCode);
		shaderModule->setIdentifier(ClassId, shaderName, "ShaderModule");

		if ( !shaderModule->createOnHardware() )
		{
			Tracer::error(ClassId, "Unable to create a shader module !");

			return {};
		}

		return shaderModule;
	}

	std::vector< std::shared_ptr< ShaderModule > >
	ShaderManager::getShaderModules (const std::shared_ptr< Device > & device, const std::shared_ptr< Program > & program) noexcept
	{
		std::vector< std::shared_ptr< ShaderModule > > shaderModules;
		shaderModules.reserve(program->getShaderList().size());

		for ( const auto * shader : program->getShaderList() )
		{
			const auto shaderModule = this->getShaderModuleFromGeneratedShader(device, *shader);

			if ( shaderModule == nullptr )
			{
				TraceError{ClassId} << "Unable to create the shader module from the shader '" << shader->name() << "' source code !";

				return {};
			}

			shaderModules.emplace_back(shaderModule);
		}

		return shaderModules;
	}

	void
	ShaderManager::readCache () noexcept
	{
		for ( const auto & filepath : IO::directoryEntries(m_shadersSourcesDirectory) )
		{
			const auto extension = IO::getFileExtension(filepath);
			bool isShaderFile = false;

			for ( const auto & allowedExtension : ShaderFileExtensions )
			{
				if ( extension == allowedExtension )
				{
					isShaderFile = true;
					break;
				}
			}

			if ( !isShaderFile )
			{
				continue;
			}

			auto hash = ShaderManager::extractHashFromFilepath(filepath);

			if ( hash == 0 )
			{
				TraceError{ClassId} << "The hash from shader source file '" << filepath << "' is invalid !";

				continue;
			}

			m_cachedShaderSourceCodes.emplace(hash, filepath);
		}

		for ( const auto & filepath : IO::directoryEntries(m_shadersBinariesDirectory) )
		{
			if ( IO::getFileExtension(filepath) != "bin" )
			{
				continue;
			}

			auto hash = ShaderManager::extractHashFromFilepath(filepath);

			if ( hash == 0 )
			{
				TraceError{ClassId} << "The hash from shader binary file '" << filepath << "' is invalid !";

				continue;
			}

			m_cachedShaderBinaries.emplace(hash, filepath);
		}
	}

	void
	ShaderManager::clearCache () noexcept
	{
		for ( const auto & filepath : IO::directoryEntries(m_shadersSourcesDirectory) )
		{
			const auto extension = IO::getFileExtension(filepath);
			bool isShaderFile = false;

			for ( const auto & allowedExtension : ShaderFileExtensions )
			{
				if ( extension == allowedExtension )
				{
					isShaderFile = true;
					break;
				}
			}

			if ( !isShaderFile )
			{
				continue;
			}

			if ( !IO::eraseFile(filepath) )
			{
				TraceError{ClassId} << "Unable to erase '" << filepath << "' !";
			}
		}

		m_cachedShaderSourceCodes.clear();

		for ( const auto & filepath : IO::directoryEntries(m_shadersBinariesDirectory) )
		{
			if ( IO::getFileExtension(filepath) != "bin" )
			{
				continue;
			}

			if ( !IO::eraseFile(filepath) )
			{
				TraceError{ClassId} << "Unable to erase '" << filepath << "' !";
			}
		}

		m_cachedShaderBinaries.clear();
	}

	std::filesystem::path
	ShaderManager::generateShaderSourceCacheFilepath (const AbstractShader & shader) const noexcept
	{
		std::stringstream filename;
		filename << shader.name() << '_' << shader.hash() << '.' << getShaderFileExtension(shader.type());

		auto filepath = m_shadersSourcesDirectory;
		filepath.append(filename.str());

		return filepath;
	}

	std::filesystem::path
	ShaderManager::generateShaderBinaryCacheFilepath (const AbstractShader & shader) const noexcept
	{
		std::stringstream filename;
		filename << shader.name() << '_' << shader.hash() << ".bin";

		auto filepath = m_shadersBinariesDirectory;
		filepath.append(filename.str());

		return filepath;
	}

	size_t
	ShaderManager::extractHashFromFilepath (const std::filesystem::path & filepath) noexcept
	{
		const auto tmpA = String::explode(filepath.filename().string(), '_');

		if ( tmpA.size() != 2 )
		{
			return 0;
		}

		const auto tmpB = String::explode(tmpA[1], '.');

		if ( tmpB.size() != 2 )
		{
			return 0;
		}

		return std::stoull(tmpB[0]);
	}

	EShLanguage
	ShaderManager::GLSLangShaderType (ShaderType shaderType) noexcept
	{
		switch ( shaderType )
		{
			case ShaderType::VertexShader :
				return EShLangVertex;

			case ShaderType::TesselationControlShader :
				return EShLangTessControl;

			case ShaderType::TesselationEvaluationShader :
				return EShLangTessEvaluation;

			case ShaderType::GeometryShader :
				return EShLangGeometry;

			case ShaderType::FragmentShader :
				return EShLangFragment;

			case ShaderType::ComputeShader :
				return EShLangCompute;

			default:
				Tracer::error(ClassId, "Unknown shader type !");

				return EShLangCount;
		}
	}

	VkShaderStageFlagBits
	ShaderManager::vkShaderType (ShaderType shaderType) noexcept
	{
		switch ( shaderType )
		{
			case ShaderType::VertexShader :
				return VK_SHADER_STAGE_VERTEX_BIT;

			case ShaderType::TesselationControlShader :
				return VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;

			case ShaderType::TesselationEvaluationShader :
				return VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;

			case ShaderType::GeometryShader :
				return VK_SHADER_STAGE_GEOMETRY_BIT;

			case ShaderType::FragmentShader :
				return VK_SHADER_STAGE_FRAGMENT_BIT;

			case ShaderType::ComputeShader :
				return VK_SHADER_STAGE_COMPUTE_BIT;

			default:
				return static_cast< VkShaderStageFlagBits >(0);
		}
	}

	bool
	ShaderManager::compile (const AbstractShader & shader, std::vector< uint32_t > & binaryCode) noexcept
	{
		/* NOTE: The shader must have a source code before a compilation can occur. */
		if ( !shader.isGenerated() )
		{
			TraceError{ClassId} << "The shader '" << shader.name() << "' has an empty source code !";

			return false;
		}

		return this->compile(shader.name(), shader.type(), shader.sourceCode(), binaryCode);
	}

	bool
	ShaderManager::compile (const std::string & shaderName, ShaderType type, const std::string & sourceCode, std::vector< uint32_t > & binaryCode) noexcept
	{
		/* NOTE: The shader must have a source code before a compilation can occur. */
		if ( sourceCode.empty() )
		{
			TraceError{ClassId} << "The source code is empty !";

			return false;
		}

		const std::string shaderIdentifier = ShaderManager::getShaderIdentificationString(type, shaderName);

		/* NOTE: Convert shader shaderType to GLSLang shaderType. */
		const auto shaderType = ShaderManager::GLSLangShaderType(type);
		const auto * sourceCodeCString = sourceCode.c_str();

		glslang::TShader glslShader{shaderType};
		glslShader.setStrings(&sourceCodeCString, 1);
		glslShader.setEnvInput(glslang::EShSourceGlsl, shaderType, glslang::EShClientVulkan, m_defaultVersion);
#if IS_MACOS
		glslShader.setEnvClient(glslang::EShClientVulkan, glslang::EShTargetVulkan_1_2);
		glslShader.setEnvTarget(glslang::EShTargetSpv, glslang::EShTargetSpv_1_5);
#else
		glslShader.setEnvClient(glslang::EShClientVulkan, glslang::EShTargetVulkan_1_3);
		glslShader.setEnvTarget(glslang::EShTargetSpv, glslang::EShTargetSpv_1_6);
#endif

		/* NOTE: Preprocess the source code. */
		std::string preprocessedSource;

		if ( !glslShader.preprocess(&m_builtInResource, m_defaultVersion, m_profile, m_flags[ForceDefaultVersionAndProfile], m_flags[ForwardCompatible], m_messageFilter, &preprocessedSource, m_includer) )
		{
			this->printCompilationErrors(shaderIdentifier, preprocessedSource, glslShader.getInfoLog());

			return false;
		}

		preprocessedSource.erase(std::ranges::unique(preprocessedSource, [] (char chrA, char chrB) {
			return chrA == '\n' && chrB == '\n';
		}).begin(),preprocessedSource.end());

		if ( m_flags[ShowSourceCode] )
		{
			TraceInfo{ClassId} << "\n"
				"****** START OF PRE-PROCESSED GLSL SHADER CODE " << shaderIdentifier << " ******" "\n" <<
				SourceCodeParser::parse(preprocessedSource) <<
				"****** END OF PRE-PROCESSED GLSL SHADER CODE " << shaderIdentifier << " ******" "\n";
		}

		/* NOTE: Parse the final source code. */
		const auto * c_string = preprocessedSource.c_str();

		glslShader.setStrings(&c_string, 1);

		if ( !glslShader.parse(&m_builtInResource, m_defaultVersion, m_profile, m_flags[ForceDefaultVersionAndProfile], m_flags[ForwardCompatible], m_messageFilter, m_includer) )
		{
			this->printCompilationErrors(shaderIdentifier, preprocessedSource, glslShader.getInfoLog());

			return false;
		}

		/* NOTE: Link the shader. */
		glslang::TProgram program;
		program.addShader(&glslShader);

		if ( !program.link(m_messageFilter) )
		{
			this->printCompilationErrors(shaderIdentifier, preprocessedSource, glslShader.getInfoLog());

			return false;
		}

		/* NOTE: Retrieve the binary data. */
		spv::SpvBuildLogger logger;
		glslang::SpvOptions spvOptions{
			.generateDebugInfo = false,
			.stripDebugInfo = false,
			.disableOptimizer = true,
			.optimizeSize = false,
			.disassemble = false,
			.validate = false,
			.emitNonSemanticShaderDebugInfo = false,
			.emitNonSemanticShaderDebugSource = false,
			//.compileOnly = false,
			//.optimizerAllowExpandedIDBound = false
		};

		GlslangToSpv(*program.getIntermediate(shaderType), binaryCode, &logger, &spvOptions);

		const auto messages = logger.getAllMessages();

		if ( !messages.empty() )
		{
			TraceInfo{ClassId} << "GLSL to SPIR-V messages : " << messages;
		}

		this->notify(ShaderCompilationSucceed, shaderIdentifier);

		return true;
	}

	void
	ShaderManager::printCompilationErrors (const std::string & shaderIdentifier, const std::string & sourceCode, const char * log) noexcept
	{
		SourceCodeParser parser{sourceCode};

		for ( const auto & error : String::explode(log, '\n') )
		{
			if ( std::ranges::count(error, ':') > 1 )
			{
				const auto chunks = String::explode(error, ':');

				const auto line = std::stoi(chunks[2]);
				const auto column = std::stoi(chunks[1]);

				parser.notice(line, column, error);
			}
			else
			{
				parser.notice(error);
			}
		}

		const std::string annotatedSourceCode = parser.getParsedSourceCode();

		TraceError{ClassId} << "\n"
			"****** START OF ERRONEOUS GLSL SHADER CODE " << shaderIdentifier << " ******" "\n" <<
			annotatedSourceCode <<
			"****** END OF ERRONEOUS GLSL SHADER CODE " << shaderIdentifier << " ******" "\n";

		this->notify(ShaderCompilationFailed, std::pair< std::string, std::string >(shaderIdentifier, annotatedSourceCode));
	}
}
