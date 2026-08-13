/*
 * src/Saphir/ShaderManager.cpp
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

#include "ShaderManager.hpp"

/* Project configuration. */
#include "emeraude_platform.hpp"

/* STL inclusions. */
#include <algorithm>
#include <charconv>
#include <cstring>
#include <filesystem>
#include <ranges>
#include <string>

/* Third-party inclusions. */
#include <glslang/Public/ShaderLang.h>
#include <glslang/SPIRV/GlslangToSpv.h>

/* Local inclusions. */
#include "AbstractShader.hpp"
#include "Arguments.hpp"
#include "FileSystem.hpp"
#include "IO/IO.hpp"
#include "Program.hpp"
#include "SourceCodeParser.hpp"
#include "TokenFormatter.hpp"
#include "PrimaryServices.hpp"
#include "SettingKeys.hpp"
#include "Settings.hpp"
#include "Vulkan/ShaderModule.hpp"

namespace EmEn::Saphir
{
	using namespace Base;
	using namespace Vulkan;

	/**
	 * @brief The GLSLang compilation context, kept out of ShaderManager.hpp so that
	 * <glslang/Public/ShaderLang.h> does not leak into every consumer of the manager.
	 */
	struct ShaderManager::GLSLangContext
	{
		TBuiltInResource builtInResource{};
		/* Saphir GENERATES its GLSL: no source it hands to glslang ever carries an #include
		 * directive, and no include directory is ever registered. glslang still requires an
		 * Includer for preprocess()/parse(), so its own no-op one is the right answer — and it
		 * FAILS an include instead of silently resolving it against an empty search stack,
		 * which is what we want if a directive ever appears by accident.
		 * ⚠️ The day hand-written GLSL sources become a thing (see TODO.md), this is where a
		 * real includer plugs in — written against an actual specification, not copied. */
		glslang::TShader::ForbidIncluder includer;
		EProfile profile{ECoreProfile}; // ENoProfile
		int defaultVersion{100};
		EShMessages messageFilter{static_cast< EShMessages >(EShMsgDefault | EShMsgSpvRules | EShMsgVulkanRules | EShMsgDebugInfo)};
		bool forceDefaultVersionAndProfile{false};
		bool forwardCompatible{false};
	};

	ShaderManager::ShaderManager (PrimaryServices & primaryServices) noexcept
		: ServiceInterface{ClassId},
		m_primaryServices{primaryServices},
		m_glslang{std::make_unique< GLSLangContext >()}
	{

	}

	ShaderManager::~ShaderManager () = default;

	/**
	 * @brief Converts a Saphir shader type to its GLSLang counterpart.
	 * @note File-local: its EShLanguage return type used to drag the glslang public
	 * header into ShaderManager.hpp.
	 * @param shaderType The Saphir shader type.
	 * @return EShLanguage
	 */
	[[nodiscard]]
	static
	EShLanguage
	toGLSLangShaderType (ShaderType shaderType) noexcept
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
				Tracer::error(ShaderManager::ClassId, "Unknown shader type !");

				return EShLangCount;
		}
	}

	bool
	ShaderManager::onInitialize () noexcept
	{
		/* Read core settings. */
		{
			const auto & arguments = m_primaryServices.arguments();
			auto & settings = m_primaryServices.settings();

			m_showInformation = settings.getOrSetDefault< bool >(VideoShowInformationKey, DefaultVideoShowInformation) ||
				arguments.isSwitchPresent("--show-all-infos") ||
				arguments.isSwitchPresent("--show-video-infos");
			m_showSourceCode = settings.getOrSetDefault< bool >(ShowSourceCodeKey, DefaultShowSourceCode);
			m_sourceCodeDumpEnabled = settings.getOrSetDefault< bool >(SourceCodeDumpEnabledKey, DefaultSourceCodeDumpEnabled);
			m_binaryCacheEnabled = settings.getOrSetDefault< bool >(BinaryCacheEnabledKey, DefaultBinaryCacheEnabled);
		}

		/* Shader source DUMP directory (see the class documentation: this is an inspection dump,
		 * not a cache). Only created when the feature is on — a disabled debug facility must
		 * never be able to bring the renderer down, which is what a hard failure here used to do. */
		if ( m_sourceCodeDumpEnabled )
		{
			m_generatedShadersDirectory = m_primaryServices.fileSystem().cacheDirectory(GeneratedShadersDirectoryName);

			if ( !IO::createDirectory(m_generatedShadersDirectory) )
			{
				TraceWarning{ClassId} << "Unable to create '" << m_generatedShadersDirectory << "' directory ! The generated source dump is disabled.";

				m_generatedShadersDirectory.clear();
				m_sourceCodeDumpEnabled = false;
			}
		}

		/* Shader binaries cache directory. */
		if ( m_binaryCacheEnabled )
		{
			m_shadersBinariesDirectory = m_primaryServices.fileSystem().cacheDirectory(ShaderBinariesDirectoryName);

			if ( !IO::createDirectory(m_shadersBinariesDirectory) )
			{
				TraceWarning{ClassId} << "Unable to create '" << m_shadersBinariesDirectory << "' directory ! The binary cache is disabled.";

				m_shadersBinariesDirectory.clear();
				m_binaryCacheEnabled = false;
			}
		}

		/* Checks shader cache. */
		if ( m_primaryServices.arguments().isSwitchPresent("--clear-renderer-cache") )
		{
			this->clearCache();
		}
		else if ( m_binaryCacheEnabled )
		{
			/* NOTE: Only the BINARY cache is ever read back; the source dump index was scanned
			 * for nothing (nothing ever reads a dumped source — see dumpShaderSourceCode()). */
			this->readBinaryCache();
		}

		if ( m_showInformation )
		{
			TraceInfo{ClassId} << "GLSLang GLSL version supported : " << glslang::GetGlslVersionString();
		}

		if ( !glslang::InitializeProcess() )
		{
			Tracer::error(ClassId, "Unable to initialize GLSLang process !");

			return false;
		}

		auto & builtInResource = m_glslang->builtInResource;
		builtInResource.maxLights = 32;
		builtInResource.maxClipPlanes = 6;
		builtInResource.maxTextureUnits = 32;
		builtInResource.maxTextureCoords = 32;
		builtInResource.maxVertexAttribs = 64;
		builtInResource.maxVertexUniformComponents = 4096;
		builtInResource.maxVaryingFloats = 64;
		builtInResource.maxVertexTextureImageUnits = 32;
		builtInResource.maxCombinedTextureImageUnits = 80;
		builtInResource.maxTextureImageUnits = 32;
		builtInResource.maxFragmentUniformComponents = 4096;
		builtInResource.maxDrawBuffers = 32;
		builtInResource.maxVertexUniformVectors = 128;
		builtInResource.maxVaryingVectors = 8;
		builtInResource.maxFragmentUniformVectors = 16;
		builtInResource.maxVertexOutputVectors = 16;
		builtInResource.maxFragmentInputVectors = 15;
		builtInResource.minProgramTexelOffset = -8;
		builtInResource.maxProgramTexelOffset = 7;
		builtInResource.maxClipDistances = 8;
		builtInResource.maxComputeWorkGroupCountX = 65535;
		builtInResource.maxComputeWorkGroupCountY = 65535;
		builtInResource.maxComputeWorkGroupCountZ = 65535;
		builtInResource.maxComputeWorkGroupSizeX = 1024;
		builtInResource.maxComputeWorkGroupSizeY = 1024;
		builtInResource.maxComputeWorkGroupSizeZ = 64;
		builtInResource.maxComputeUniformComponents = 1024;
		builtInResource.maxComputeTextureImageUnits = 16;
		builtInResource.maxComputeImageUniforms = 8;
		builtInResource.maxComputeAtomicCounters = 8;
		builtInResource.maxComputeAtomicCounterBuffers = 1;
		builtInResource.maxVaryingComponents = 60;
		builtInResource.maxVertexOutputComponents = 64;
		builtInResource.maxGeometryInputComponents = 64;
		builtInResource.maxGeometryOutputComponents = 128;
		builtInResource.maxFragmentInputComponents = 128;
		builtInResource.maxImageUnits = 8;
		builtInResource.maxCombinedImageUnitsAndFragmentOutputs = 8;
		builtInResource.maxCombinedShaderOutputResources = 8;
		builtInResource.maxImageSamples = 0;
		builtInResource.maxVertexImageUniforms = 0;
		builtInResource.maxTessControlImageUniforms = 0;
		builtInResource.maxTessEvaluationImageUniforms = 0;
		builtInResource.maxGeometryImageUniforms = 0;
		builtInResource.maxFragmentImageUniforms = 8;
		builtInResource.maxCombinedImageUniforms = 8;
		builtInResource.maxGeometryTextureImageUnits = 16;
		builtInResource.maxGeometryOutputVertices = 256;
		builtInResource.maxGeometryTotalOutputComponents = 1024;
		builtInResource.maxGeometryUniformComponents = 1024;
		builtInResource.maxGeometryVaryingComponents = 64;
		builtInResource.maxTessControlInputComponents = 128;
		builtInResource.maxTessControlOutputComponents = 128;
		builtInResource.maxTessControlTextureImageUnits = 16;
		builtInResource.maxTessControlUniformComponents = 1024;
		builtInResource.maxTessControlTotalOutputComponents = 4096;
		builtInResource.maxTessEvaluationInputComponents = 128;
		builtInResource.maxTessEvaluationOutputComponents = 128;
		builtInResource.maxTessEvaluationTextureImageUnits = 16;
		builtInResource.maxTessEvaluationUniformComponents = 1024;
		builtInResource.maxTessPatchComponents = 120;
		builtInResource.maxPatchVertices = 32;
		builtInResource.maxTessGenLevel = 64;
		builtInResource.maxViewports = 16;
		builtInResource.maxVertexAtomicCounters = 0;
		builtInResource.maxTessControlAtomicCounters = 0;
		builtInResource.maxTessEvaluationAtomicCounters = 0;
		builtInResource.maxGeometryAtomicCounters = 0;
		builtInResource.maxFragmentAtomicCounters = 8;
		builtInResource.maxCombinedAtomicCounters = 8;
		builtInResource.maxAtomicCounterBindings = 1;
		builtInResource.maxVertexAtomicCounterBuffers = 0;
		builtInResource.maxTessControlAtomicCounterBuffers = 0;
		builtInResource.maxTessEvaluationAtomicCounterBuffers = 0;
		builtInResource.maxGeometryAtomicCounterBuffers = 0;
		builtInResource.maxFragmentAtomicCounterBuffers = 1;
		builtInResource.maxCombinedAtomicCounterBuffers = 1;
		builtInResource.maxAtomicCounterBufferSize = 16384;
		builtInResource.maxTransformFeedbackBuffers = 4;
		builtInResource.maxTransformFeedbackInterleavedComponents = 64;
		builtInResource.maxCullDistances = 8;
		builtInResource.maxCombinedClipAndCullDistances = 8;
		builtInResource.maxSamples = 4;

		builtInResource.limits.nonInductiveForLoops = true;
		builtInResource.limits.whileLoops = true;
		builtInResource.limits.doWhileLoops = true;
		builtInResource.limits.generalUniformIndexing = true;
		builtInResource.limits.generalAttributeMatrixVectorIndexing = true;
		builtInResource.limits.generalVaryingIndexing = true;
		builtInResource.limits.generalSamplerIndexing = true;
		builtInResource.limits.generalVariableIndexing = true;
		builtInResource.limits.generalConstantMatrixVectorIndexing = true;

		return true;
	}

	bool
	ShaderManager::onTerminate () noexcept
	{
		glslang::FinalizeProcess();

		m_cachedShaderBinaries.clear();
		m_shaderModules.clear();

		return true;
	}

	bool
	ShaderManager::dumpShaderSourceCode (const AbstractShader & shader, const char * generatorClassId) const noexcept
	{
		if ( !m_sourceCodeDumpEnabled )
		{
			return true;
		}

		const auto dumpFilepath = this->generateShaderDumpFilepath(shader, generatorClassId);

		if ( dumpFilepath.empty() )
		{
			TraceError{ClassId} << "Unable to get a proper source dump path for shader '" << shader.name() << "' !";

			return false;
		}

		if ( !shader.writeSourceCode(dumpFilepath) )
		{
			TraceError{ClassId} << "Unable to write the source dump file '" << dumpFilepath << "' for shader '" << shader.name() << "' !";

			return false;
		}

		return true;
	}

	namespace
	{
		/* Application wrapper around the SPIR-V blob. The filename says WHICH shader
		 * (name + source hash); this header says whether the blob is still VALID — i.e. whether
		 * the toolchain that produced it is the one running now. Without it a stale blob survives
		 * a glslang upgrade or a target-environment change and is handed to vkCreateShaderModule
		 * unchecked. Same discipline as the pipeline cache (see src/Vulkan/AGENTS.md). */
		struct ShaderBinaryFileHeader
		{
			uint32_t magic;
			uint32_t formatVersion;
			uint64_t sourceHash;
			uint64_t dataSize;
			uint64_t dataHash;
			uint64_t toolchainHash;
			uint32_t shaderType;
			uint32_t reserved;
		};

		constexpr uint32_t ShaderBinaryMagic{0x45534243U}; /* "ESBC" */
		constexpr uint32_t ShaderBinaryFormatVersion{1};
		constexpr uint32_t SpirVMagicWord{0x07230203U};

		[[nodiscard]]
		uint64_t
		hashBytes (const void * data, size_t size) noexcept
		{
			const auto * bytes = static_cast< const uint8_t * >(data);

			uint64_t hash = 14695981039346656037ULL;

			for ( size_t index = 0; index < size; ++index )
			{
				hash ^= static_cast< uint64_t >(bytes[index]);
				hash *= 1099511628211ULL;
			}

			return hash;
		}

		/* Everything that changes the bytes glslang produces for identical GLSL: the compiler
		 * itself, the client/target environment pair, and the engine version (a change in how the
		 * source is assembled invalidates nothing here, but a change in the compile SETUP does). */
		[[nodiscard]]
		uint64_t
		toolchainIdentity () noexcept
		{
			std::string identity{glslang::GetGlslVersionString()};
			identity += '|';
			identity += std::to_string(glslang::GetSpirvGeneratorVersion());
			identity += '|';

			if constexpr ( IsMacOS )
			{
				identity += "vk1.2-spv1.5";
			}
			else
			{
				identity += "vk1.3-spv1.6";
			}

			identity += '|';
			identity += VersionString;

			return hashBytes(identity.data(), identity.size());
		}
	}

	bool
	ShaderManager::cacheShaderBinary (const AbstractShader & shader, const std::vector< uint32_t > & binaryCode) const noexcept
	{
		if ( !m_binaryCacheEnabled )
		{
			return true;
		}

		const auto cacheFilepath = this->generateShaderBinaryCacheFilepath(shader);

		if ( cacheFilepath.empty() )
		{
			TraceError{ClassId} << "Unable to get a proper binary cache path for shader '" << shader.name() << "' !";

			return false;
		}

		const auto blobSize = binaryCode.size() * sizeof(uint32_t);

		ShaderBinaryFileHeader header{};
		header.magic = ShaderBinaryMagic;
		header.formatVersion = ShaderBinaryFormatVersion;
		header.sourceHash = shader.hash();
		header.dataSize = blobSize;
		header.dataHash = hashBytes(binaryCode.data(), blobSize);
		header.toolchainHash = toolchainIdentity();
		header.shaderType = static_cast< uint32_t >(shader.type());
		header.reserved = 0;

		std::vector< uint8_t > fileContent(sizeof(header) + blobSize);
		std::memcpy(fileContent.data(), &header, sizeof(header));
		std::memcpy(fileContent.data() + sizeof(header), binaryCode.data(), blobSize);

		/* ⚠️ Written aside then renamed: a SIGKILL — the documented shutdown fallback — must not
		 * be able to leave a truncated blob behind for the next launch to load. */
		auto temporaryPath = cacheFilepath;
		temporaryPath += ".tmp";

		if ( !IO::filePutContents(temporaryPath, fileContent) )
		{
			TraceError{ClassId} << "Unable to write the shader binary code to file '" << temporaryPath << "' for shader '" << shader.name() << "' !";

			return false;
		}

		std::error_code error;
		std::filesystem::rename(temporaryPath, cacheFilepath, error);

		if ( error )
		{
			TraceError{ClassId} << "Unable to commit the shader binary cache file '" << cacheFilepath << "' : " << error.message() << " !";

			IO::eraseFile(temporaryPath);

			return false;
		}

		return true;
	}

	bool
	ShaderManager::checkBinaryFromCache (const AbstractShader & shader, std::vector< uint32_t > & binaryCode) noexcept
	{
		if ( !m_binaryCacheEnabled )
		{
			return false;
		}

		const auto binaryIt = m_cachedShaderBinaries.find(shader.hash());

		if ( binaryIt == m_cachedShaderBinaries.cend() )
		{
			return false;
		}

		std::vector< uint8_t > fileContent;

		if ( !IO::fileGetContents(binaryIt->second, fileContent) )
		{
			TraceError{ClassId} << "Unable to read the shader binary code from file '" << binaryIt->second << "' !";

			return false;
		}

		if ( fileContent.size() <= sizeof(ShaderBinaryFileHeader) )
		{
			TraceWarning{ClassId} << "The shader binary file '" << binaryIt->second << "' is truncated ! Discarding it.";

			IO::eraseFile(binaryIt->second);

			return false;
		}

		ShaderBinaryFileHeader header{};
		std::memcpy(&header, fileContent.data(), sizeof(header));

		const auto blobSize = fileContent.size() - sizeof(header);
		const auto * blob = fileContent.data() + sizeof(header);

		/* ⚠️ EVERY field is checked before a single byte can reach vkCreateShaderModule. A SPIR-V
		 * blob is only valid for the exact source AND the exact toolchain that produced it; the
		 * toolchain hash is what makes a glslang upgrade invalidate the file instead of silently
		 * feeding the driver stale bytes. */
		const auto rejected =
			header.magic != ShaderBinaryMagic ||
			header.formatVersion != ShaderBinaryFormatVersion ||
			header.sourceHash != shader.hash() ||
			header.shaderType != static_cast< uint32_t >(shader.type()) ||
			header.dataSize != blobSize ||
			header.toolchainHash != toolchainIdentity() ||
			blobSize < sizeof(uint32_t) ||
			blobSize % sizeof(uint32_t) != 0 ||
			header.dataHash != hashBytes(blob, blobSize);

		if ( rejected )
		{
			TraceInfo{ClassId} << "The cached binary of shader '" << shader.name() << "' is stale or corrupt ! Recompiling.";

			IO::eraseFile(binaryIt->second);

			return false;
		}

		binaryCode.resize(blobSize / sizeof(uint32_t));
		std::memcpy(binaryCode.data(), blob, blobSize);

		/* Last sanity check, on the SPIR-V itself: a valid module always starts with its magic. */
		if ( binaryCode.front() != SpirVMagicWord )
		{
			TraceWarning{ClassId} << "The cached binary of shader '" << shader.name() << "' is not SPIR-V ! Discarding it.";

			IO::eraseFile(binaryIt->second);
			binaryCode.clear();

			return false;
		}

		return true;
	}

	std::shared_ptr< ShaderModule >
	ShaderManager::getShaderModuleFromGeneratedShader (const std::shared_ptr< Device > & device, const AbstractShader & shader, const char * generatorClassId) noexcept
	{
		if ( !this->usable() )
		{
			Tracer::error(ClassId, "There is no device to load create the shader module !");

			return {};
		}

		/* The shader must have a source code before anything else.
		 * The hash depends on it. */
		if ( !shader.isGenerated() )
		{
			TraceError{ClassId} <<
				"The shader '" << shader.name() << "' is empty ! "
				"Generate it first.";

			return {};
		}

		const auto shaderHash = shader.hash();

		/* Checks in a loaded shader list with the hash. */
		if ( const auto shaderIt = m_shaderModules.find(shaderHash); shaderIt != m_shaderModules.cend() )
		{
			return shaderIt->second;
		}

		/* Dump the generated source for inspection. ⚠️ Done BEFORE the binary-cache check on
		 * purpose: this is not a cache, it is the window onto what the generators produced, and
		 * hanging it off the compile path meant a binary cache hit silently stopped producing it. */
		if ( !this->dumpShaderSourceCode(shader, generatorClassId) )
		{
			TraceWarning{ClassId} << "Unable to dump the generated source of shader '" << shader.name() << "' !";
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

	StaticVector< std::shared_ptr< ShaderModule >, 5 >
	ShaderManager::getShaderModules (const std::shared_ptr< Device > & device, const std::shared_ptr< Program > & program, const char * generatorClassId) noexcept
	{
		StaticVector< std::shared_ptr< ShaderModule >, 5 > shaderModules;

		for ( const auto * shader : program->getShaderList() )
		{
			const auto shaderModule = this->getShaderModuleFromGeneratedShader(device, *shader, generatorClassId);

			if ( shaderModule == nullptr )
			{
				TraceError{ClassId} << "Unable to create the shader module from the shader '" << shader->name() << "' source code !";

				return {};
			}

			/* NOTE: Apply specialization constants from the Program to the fragment shader.
			 * This is used for features like shadow mapping which can be enabled/disabled via spec constants. */
			if ( shader->type() == ShaderType::FragmentShader && program->hasSpecializationConstants() )
			{
				for ( const auto & [constantId, value] : program->fragmentSpecializationConstantsBool() )
				{
					shaderModule->setSpecializationConstant(constantId, value);
				}

				if ( !shaderModule->rebuildPipelineShaderStageCreateInfo() )
				{
					TraceError{ClassId} << "Unable to rebuild pipeline shader stage create info for shader '" << shader->name() << "' !";

					return {};
				}
			}

			shaderModules.emplace_back(shaderModule);
		}

		return shaderModules;
	}

	void
	ShaderManager::readBinaryCache () noexcept
	{
		/* NOTE: Only the BINARY cache is ever read back. The source directory is an inspection
		 * DUMP that nothing reloads, so indexing it served no purpose; worse, this function only
		 * runs when the binary cache is on, and the dump is off by default, which meant scanning
		 * an EMPTY path and logging an IO error on every single startup. */
		if ( m_shadersBinariesDirectory.empty() )
		{
			return;
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
		/* NOTE: A disabled facility leaves its directory path EMPTY, and --clear-renderer-cache runs
		 * whatever the settings say, so both loops must be guarded. */
		for ( const auto & filepath : m_generatedShadersDirectory.empty() ? std::vector< std::filesystem::path >{} : IO::directoryEntries(m_generatedShadersDirectory) )
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

		for ( const auto & filepath : m_shadersBinariesDirectory.empty() ? std::vector< std::filesystem::path >{} : IO::directoryEntries(m_shadersBinariesDirectory) )
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
	ShaderManager::generateShaderDumpFilepath (const AbstractShader & shader, const char * generatorClassId) const noexcept
	{
		const auto & name = shader.name();
		const auto hashStr = std::to_string(shader.hash());
		const auto * extension = getShaderFileExtension(shader.type());

		std::string filename;
		filename.reserve(name.size() + hashStr.size() + 16);
		filename += name;
		filename += '_';
		filename += hashStr;
		filename += '.';
		filename += extension;

		auto filepath = m_generatedShadersDirectory;

		/* One sub-directory per generator, so the dump can be inspected by category instead of
		 * being one flat pile. Created lazily: a generator that never runs leaves no directory. */
		if ( generatorClassId != nullptr )
		{
			filepath.append(generatorClassId);

			if ( !IO::createDirectory(filepath) )
			{
				TraceWarning{ClassId} << "Unable to create the generator sub-directory '" << filepath << "' ! Dumping at the dump root instead.";

				filepath = m_generatedShadersDirectory;
			}
		}

		filepath.append(filename);

		return filepath;
	}

	std::filesystem::path
	ShaderManager::generateShaderBinaryCacheFilepath (const AbstractShader & shader) const noexcept
	{
		const auto & name = shader.name();
		const auto hashStr = std::to_string(shader.hash());

		std::string filename;
		filename.reserve(name.size() + hashStr.size() + 6);
		filename += name;
		filename += '_';
		filename += hashStr;
		filename += ".bin";

		auto filepath = m_shadersBinariesDirectory;
		filepath.append(filename);

		return filepath;
	}

	size_t
	ShaderManager::extractHashFromFilepath (const std::filesystem::path & filepath) noexcept
	{
		/* NOTE: Equivalent to String::explode(filename, '_').size() == 2, i.e. the filename
		 * must contain exactly one '_' (separating the shader name from "<hash>.<ext>"),
		 * done here without allocating the intermediate std::vector< std::string >. */
		const auto filename = filepath.filename().string();

		const auto underscorePos = filename.find('_');

		if ( underscorePos == std::string::npos || underscorePos != filename.rfind('_') )
		{
			return 0;
		}

		/* NOTE: Equivalent to String::explode(tmpA[1], '.').size() == 2, i.e. exactly one '.'
		 * after the underscore (separating "<hash>" from "<ext>"). */
		const auto dotPos = filename.find('.', underscorePos + 1);

		if ( dotPos == std::string::npos || dotPos != filename.rfind('.') )
		{
			return 0;
		}

		/* ⚠️ NOT std::stoull: the cascade builds with -fno-exceptions (EMERAUDE_DISABLE_EXCEPTIONS,
		 * default ON), so a stray file in this user-writable directory — anything shaped like
		 * "name_notanumber.vert" — would throw and terminate the process AT STARTUP. from_chars
		 * reports the failure instead, and an unparsable entry is simply ignored. */
		const auto * first = filename.data() + underscorePos + 1;
		const auto * last = filename.data() + dotPos;

		size_t hash = 0;

		if ( const auto [ptr, error] = std::from_chars(first, last, hash); error != std::errc{} || ptr != last )
		{
			return 0;
		}

		return hash;
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

		auto shaderIdentifier = TokenFormatter::toUpperSpaced(to_cstring(type));
		shaderIdentifier += " (";
		shaderIdentifier += shaderName;
		shaderIdentifier += ')';

		/* NOTE: Convert shader shaderType to GLSLang shaderType. */
		const auto shaderType = toGLSLangShaderType(type);
		const auto * sourceCodeCString = sourceCode.c_str();

		glslang::TShader glslShader{shaderType};
		glslShader.setStrings(&sourceCodeCString, 1);
		glslShader.setEnvInput(glslang::EShSourceGlsl, shaderType, glslang::EShClientVulkan, m_glslang->defaultVersion);
		/* [VULKAN-API-SETUP] GLSL/SPIR-V version for Vulkan. */
		if constexpr ( IsMacOS )
		{
			/* NOTE: macOS don't support the Vulkan API.
			 * MoltenVK is used to translate commands to Metal API, some features can be unsupported. */
			glslShader.setEnvClient(glslang::EShClientVulkan, glslang::EShTargetVulkan_1_2);
			glslShader.setEnvTarget(glslang::EShTargetSpv, glslang::EShTargetSpv_1_5);
		}
		else
		{
			glslShader.setEnvClient(glslang::EShClientVulkan, glslang::EShTargetVulkan_1_3);
			glslShader.setEnvTarget(glslang::EShTargetSpv, glslang::EShTargetSpv_1_6);
		}

		/* NOTE: Preprocess the source code. */
		std::string preprocessedSource;

		if ( !glslShader.preprocess(&m_glslang->builtInResource, m_glslang->defaultVersion, m_glslang->profile, m_glslang->forceDefaultVersionAndProfile, m_glslang->forwardCompatible, m_glslang->messageFilter, &preprocessedSource, m_glslang->includer) )
		{
			this->printCompilationErrors(shaderIdentifier, preprocessedSource, glslShader.getInfoLog());

			return false;
		}

		preprocessedSource.erase(std::ranges::unique(preprocessedSource, [] (char chrA, char chrB) {
			return chrA == '\n' && chrB == '\n';
		}).begin(),preprocessedSource.end());

		if ( this->showSourceCode() )
		{
			/* NOTE: Show the pre-processed version of the source code by GLSlang.
			 * This can be helpful when trying to understand a shader compilation error. */
			TraceDebug{ClassId} << "\n"
				"/****** START OF PRE-PROCESSED GLSL " << shaderIdentifier << " CODE ******/" "\n" <<
				SourceCodeParser::parse(preprocessedSource, 0, true) <<
				"/****** END OF PRE-PROCESSED GLSL " << shaderIdentifier << " CODE ******/" "\n";
		}

		/* NOTE: Parse the final source code. */
		const auto * c_string = preprocessedSource.c_str();

		glslShader.setStrings(&c_string, 1);

		if ( !glslShader.parse(&m_glslang->builtInResource, m_glslang->defaultVersion, m_glslang->profile, m_glslang->forceDefaultVersionAndProfile, m_glslang->forwardCompatible, m_glslang->messageFilter, m_glslang->includer) )
		{
			this->printCompilationErrors(shaderIdentifier, preprocessedSource, glslShader.getInfoLog());

			return false;
		}

		/* NOTE: Link the shader. */
		glslang::TProgram program;
		program.addShader(&glslShader);

		if ( !program.link(m_glslang->messageFilter) )
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

		if ( const auto messages = logger.getAllMessages(); !messages.empty() )
		{
			TraceInfo{ClassId} << "GLSL to SPIR-V messages : " << messages;
		}

		this->notify(ShaderCompilationSucceed, shaderIdentifier);

		return true;
	}

	void
	ShaderManager::printCompilationErrors (const std::string & shaderIdentifier, const std::string & sourceCode, const char * log) noexcept
	{
		SourceCodeParser parser{sourceCode, 5, false};

		for ( const auto & error : String::explode(log, '\n') )
		{
			if ( std::ranges::count(error, ':') > 1 )
			{
				const auto chunks = String::explode(error, ':');

				int line = 0;
				int column = 0;

				const auto & lineStr = chunks[2];
				const auto & colStr = chunks[1];

				auto [ptrL, ecL] = std::from_chars(lineStr.data(), lineStr.data() + lineStr.size(), line);
				auto [ptrC, ecC] = std::from_chars(colStr.data(), colStr.data() + colStr.size(), column);

				if ( ecL == std::errc{} && ecC == std::errc{} )
				{
					parser.annotate(line, column, error);
				}
				else
				{
					parser.annotate(error);
				}
			}
			else
			{
				parser.annotate(error);
			}
		}

		const std::string annotatedSourceCode = parser.getParsedSourceCode();

		TraceError{ClassId} << "\n"
			"/****** START OF ERRONEOUS GLSL " << shaderIdentifier << " CODE ******/" "\n" <<
			annotatedSourceCode <<
			"/****** END OF ERRONEOUS GLSL " << shaderIdentifier << " CODE ******/" "\n";

		this->notify(ShaderCompilationFailed, std::pair< std::string, std::string >(shaderIdentifier, annotatedSourceCode));
	}
}
