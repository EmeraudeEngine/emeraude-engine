/*
 * src/Graphics/Material/SkyAtmosphereResource.cpp
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

#include "SkyAtmosphereResource.hpp"

/* STL inclusions. */
#include <algorithm>
#include <initializer_list>
#include <iterator>
#include <optional>
#include <span>
#include <string>

/* Third-party inclusions. */
#include "volk.h"

/* Local inclusions. */
#include "BlobTrait.hpp"
#include "FastJSON.hpp"
#include "Graphics/Effects/Shared/AtmosphereGLSL.hpp"
#include "Graphics/ImageResource.hpp"
#include "Graphics/Material/Interface.hpp"
#include "Graphics/MoonSurfacePhotometry.hpp"
#include "Graphics/Renderer.hpp"
#include "Graphics/SharedUniformBuffer.hpp"
#include "Graphics/SkyAtmosphere.hpp"
#include "Graphics/StorageTexture2D.hpp"
#include "Graphics/TextureResource/Texture2D.hpp"
#include "Saphir/AbstractShader.hpp"
#include "Saphir/AbstractVertexStage.hpp"
#include "Saphir/Code.hpp"
#include "Saphir/Declaration/Function.hpp"
#include "Saphir/Declaration/Sampler.hpp"
#include "Saphir/Declaration/StageOutput.hpp"
#include "Saphir/Declaration/Types.hpp"
#include "Saphir/Declaration/UniformBlock.hpp"
#include "Saphir/FragmentShader.hpp"
#include "Saphir/Generator/Abstract.hpp"
#include "Saphir/Keys.hpp"
#include "Saphir/Program.hpp"
#include "Saphir/SetIndexes.hpp"
#include "Tracer.hpp"
#include "Vulkan/DescriptorSet.hpp"
#include "Vulkan/DescriptorSetLayout.hpp"
#include "Vulkan/LayoutManager.hpp"

namespace EmEn::Graphics::Material
{
	using namespace Base;
	using namespace Base::PixelFactory;
	using namespace Saphir;
	using namespace Saphir::Keys;
	using namespace Vulkan;

	namespace
	{
		/* The descriptor set's bindings (the material UBO first, as every material). */
		constexpr uint32_t MaterialBinding{0};
		constexpr uint32_t AtmosphereBinding{1};
		constexpr uint32_t SkyViewBinding{2};
		constexpr uint32_t TransmittanceBinding{3};
		constexpr uint32_t FrameBinding{4};
		/* The moons' surfaces: one binding per moon, from 5 (an unused one holds the transmittance table, never read). */
		constexpr uint32_t MoonSurfaceBinding{5};
		/* The textured planet's surface, after the moons' (the transmittance table when there is none, never read). */
		constexpr uint32_t PlanetSurfaceBinding{MoonSurfaceBinding + SkyAtmosphereResource::MaxMoons};
		/* The most planets the frame carries, and where they start in the frame texture. */
		constexpr uint32_t MaxPlanets{SkyFrameInputs::MaxPlanets};

		/* GLSL names. */
		constexpr auto SkyDirection{"skyDirection"};
		constexpr auto SkyViewLut{"skyViewLut"};
		constexpr auto TransmittanceLut{"skyTransmittanceLut"};
		constexpr auto FrameData{"skyFrameData"};
		constexpr auto PlanetSurface{"skyPlanetSurfaceMap"};
		/**
		 * @brief A moon's names in the generated shader.
		 */
		struct MoonShaderNames
		{
			/** @brief Its surface's sampler. Static storage: a Saphir declaration keeps the name's pointer (Keys::Key). */
			const char * surface;
			/** @brief Its lane in the SkyMoonSurfaces vector. */
			char lane;
		};

		constexpr std::array< MoonShaderNames, SkyAtmosphereResource::MaxMoons > MoonNames{{
			{.surface = "skyMoonSurface0", .lane = 'x'},
			{.surface = "skyMoonSurface1", .lane = 'y'},
			{.surface = "skyMoonSurface2", .lane = 'z'},
			{.surface = "skyMoonSurface3", .lane = 'w'}
		}};
		constexpr auto SkyRadiance{"skyRadiance"};

		/* The ceiling of the star's disc in the PRE-EXPOSED scene colour: under the fp16 maximum (65 504) with the sky
		 * added, so a noon sun never stores an infinity a blur would spread. The disc stays far above any display
		 * white: it saturates, it does not dim. */
		constexpr auto StarDiscCeiling{32768.0F};

		/**
		 * @brief Declares one shared atmosphere function in a Saphir shader.
		 * @param shader A reference to the shader.
		 * @param source A reference to the function source.
		 * @return bool
		 */
		bool
		declareAtmosphereFunction (AbstractShader & shader, const AtmosphereGLSL::FunctionSource & source) noexcept
		{
			Declaration::Function function{source.name, source.returnType};

			uint32_t index = 0;

			for ( const auto & parameter : source.parameters )
			{
				if ( index == source.parameterCount )
				{
					break;
				}

				function.addInParameter(parameter.type, parameter.name);

				++index;
			}

			Code{function, Location::Output} << source.body;

			return shader.declare(function);
		}

		/**
		 * @brief Measures a moon's surface: its near-side mean and its photometry table (MoonSurfacePhotometry).
		 * @param material The sky material's name, for the traces.
		 * @param index The moon's index.
		 * @param surface A reference to the surface texture, loaded (its pixels are leased while measured).
		 * @return std::shared_ptr< const MoonSurfacePhotometry > Null (traced) when the surface has no measurable pixels:
		 * the moon is then drawn with a uniform albedo.
		 */
		[[nodiscard]]
		std::shared_ptr< const MoonSurfacePhotometry >
		measureMoonSurface (const std::string & material, uint32_t index, TextureResource::Texture2D & surface) noexcept
		{
			const auto image = surface.localData();

			if ( image == nullptr )
			{
				TraceError{SkyAtmosphereResource::ClassId} << "The surface '" << surface.name() << "' of moon #" << index << " of sky material '" << material << "' has no pixels to measure (a block-compressed source): the moon is drawn with a uniform albedo !";

				return nullptr;
			}

			/* The pixels stay resident while measured (reloaded if a release pass took them). */
			const auto lease = image->acquireLocalData();

			if ( !lease.isValid() )
			{
				TraceError{SkyAtmosphereResource::ClassId} << "The pixels of the surface '" << surface.name() << "' of moon #" << index << " of sky material '" << material << "' could not be read: the moon is drawn with a uniform albedo !";

				return nullptr;
			}

			const auto photometry = MoonSurfacePhotometry::measure(image->data(), surface.isSRGB());

			if ( !photometry.has_value() )
			{
				TraceError{SkyAtmosphereResource::ClassId} << "The surface '" << surface.name() << "' of moon #" << index << " of sky material '" << material << "' has no measurable near side (empty or black): the moon is drawn with a uniform albedo !";

				return nullptr;
			}

			return std::make_shared< const MoonSurfacePhotometry >(photometry.value());
		}

		/**
		 * @brief Measures a planet's surface: its luminance over the whole sphere (MoonSurfacePhotometry::sphereMean()).
		 * @param material The sky material's name, for the traces.
		 * @param planetIndex The planet's index.
		 * @param surface A reference to the surface texture, loaded (its pixels are leased while measured).
		 * @return std::optional< float > Nothing (traced) when the surface has no measurable pixels: the planet keeps a
		 * uniform albedo.
		 */
		[[nodiscard]]
		std::optional< float >
		measurePlanetSurface (const std::string & material, uint32_t planetIndex, TextureResource::Texture2D & surface) noexcept
		{
			const auto image = surface.localData();

			if ( image == nullptr )
			{
				TraceError{SkyAtmosphereResource::ClassId} << "The surface '" << surface.name() << "' of planet #" << planetIndex << " of sky material '" << material << "' has no pixels to measure: the planet is drawn with a uniform albedo !";

				return std::nullopt;
			}

			const auto lease = image->acquireLocalData();

			if ( !lease.isValid() )
			{
				TraceError{SkyAtmosphereResource::ClassId} << "The pixels of the surface '" << surface.name() << "' of planet #" << planetIndex << " of sky material '" << material << "' could not be read: the planet is drawn with a uniform albedo !";

				return std::nullopt;
			}

			const auto mean = MoonSurfacePhotometry::sphereMean(image->data(), surface.isSRGB());

			if ( !mean.has_value() )
			{
				TraceError{SkyAtmosphereResource::ClassId} << "The surface '" << surface.name() << "' of planet #" << planetIndex << " of sky material '" << material << "' is black: the planet is drawn with a uniform albedo !";
			}

			return mean;
		}
	}

	SkyAtmosphereResource::SkyAtmosphereResource (Resources::AbstractServiceProvider & serviceProvider, const std::string & name, uint32_t resourceFlags) noexcept
		: Interface{serviceProvider, name, resourceFlags}
	{
		/* A sky emits: what this material writes IS the radiance. */
		this->enableFlag(UnlitEnabled);
	}

	bool
	SkyAtmosphereResource::load () noexcept
	{
		if ( !this->beginLoading() )
		{
			return false;
		}

		/* The defaults: the physical sky, its star drawn. */
		return this->setLoadSuccess(true);
	}

	bool
	SkyAtmosphereResource::load (const Json::Value & data) noexcept
	{
		if ( !this->beginLoading() )
		{
			return false;
		}

		this->setGrading(
			FastJSON::getValue< Color< float > >(data, JKTint).value_or(White),
			FastJSON::getValue< float >(data, JKIntensity).value_or(1.0F)
		);
		this->setSaturation(FastJSON::getValue< float >(data, JKSaturation).value_or(1.0F));
		this->enableStarDisc(FastJSON::getValue< bool >(data, JKStarDisc).value_or(true));
		this->setLimbDarkening(FastJSON::getValue< float >(data, JKLimbDarkening).value_or(DefaultLimbDarkening));

		return this->setLoadSuccess(true);
	}

	bool
	SkyAtmosphereResource::setAtmosphere (const std::shared_ptr< SkyAtmosphere > & atmosphere) noexcept
	{
		if ( this->isCreated() )
		{
			TraceError{ClassId} << "The sky material '" << this->name() << "' is already created: its atmosphere can't change !";

			return false;
		}

		m_atmosphere = atmosphere;

		return true;
	}

	bool
	SkyAtmosphereResource::setMoonSurface (uint32_t index, const std::shared_ptr< TextureResource::Texture2D > & texture) noexcept
	{
		if ( index >= MaxMoons )
		{
			TraceError{ClassId} << "The sky material '" << this->name() << "' draws " << MaxMoons << " moons: no surface for moon #" << index << " !";

			return false;
		}

		if ( texture == nullptr )
		{
			TraceError{ClassId} << "The sky material '" << this->name() << "' refused a null surface for moon #" << index << " !";

			return false;
		}

		if ( this->isCreated() )
		{
			TraceError{ClassId} << "The sky material '" << this->name() << "' is already created: the surface of moon #" << index << " can't change !";

			return false;
		}

		*std::next(m_moonSurfaces.begin(), index) = texture;

		return this->addDependency(texture);
	}

	bool
	SkyAtmosphereResource::setPlanetSurface (uint32_t planetIndex, const std::shared_ptr< TextureResource::Texture2D > & texture) noexcept
	{
		if ( planetIndex >= MaxPlanets )
		{
			TraceError{ClassId} << "The sky material '" << this->name() << "' draws " << MaxPlanets << " planets: no surface for planet #" << planetIndex << " !";

			return false;
		}

		if ( texture == nullptr )
		{
			TraceError{ClassId} << "The sky material '" << this->name() << "' refused a null surface for planet #" << planetIndex << " !";

			return false;
		}

		if ( this->isCreated() )
		{
			TraceError{ClassId} << "The sky material '" << this->name() << "' is already created: the surface of planet #" << planetIndex << " can't change !";

			return false;
		}

		if ( m_planetSurface != nullptr )
		{
			TraceError{ClassId} << "The sky material '" << this->name() << "' already textures planet #" << m_planetSurfaceIndex << ": one textured planet per sky !";

			return false;
		}

		m_planetSurface = texture;
		m_planetSurfaceIndex = planetIndex;

		return this->addDependency(texture);
	}

	void
	SkyAtmosphereResource::onBeforeCreation () noexcept
	{
		/* The textured planet: its sphere mean divided out (the magnitude already gives its light). */
		m_properties[PlanetSurfaceOffset] = 0.0F;
		m_properties[PlanetSurfaceOffset + 1] = static_cast< float >(m_planetSurfaceIndex);

		if ( m_planetSurface != nullptr )
		{
			if ( const auto mean = measurePlanetSurface(this->name(), m_planetSurfaceIndex, *m_planetSurface) )
			{
				m_properties[PlanetSurfaceOffset] = 1.0F / mean.value();
			}
		}

		/* The moons' surfaces: their near side's projected mean divided out, their photometry kept for the moons' light
		 * (MoonSurfacePhotometry). Every index below is < MaxMoons, the size of the three arrays. */
		const auto scales = std::span{m_properties}.subspan< MoonSurfacesOffset, MaxMoons >();

		for ( uint32_t index = 0; index < MaxMoons; ++index )
		{
			const auto & surface = *std::next(m_moonSurfaces.cbegin(), index);
			auto & photometry = *std::next(m_moonPhotometry.begin(), index);

			photometry = surface != nullptr ? measureMoonSurface(this->name(), index, *surface) : nullptr;
			scales[index] = photometry != nullptr ? 1.0F / photometry->nearSideMean() : 0.0F;
		}
	}

	bool
	SkyAtmosphereResource::create (Renderer & renderer) noexcept
	{
		/* The renderer flushes the look changes made after creation. */
		m_renderer = &renderer;

		if ( m_atmosphere == nullptr )
		{
			m_atmosphere = std::make_shared< SkyAtmosphere >(this->name(), AtmosphereParameters::earth());
		}

		/* Idempotent: the background that owns the atmosphere may have created it already. */
		if ( !m_atmosphere->createOnHardware(renderer) )
		{
			TraceError{ClassId} << "Unable to create the atmosphere of sky material '" << this->name() << "' !";

			return false;
		}

		const auto identifier = this->getSharedUniformBufferIdentifier();

		if ( !this->createElementInSharedBuffer(renderer, identifier) )
		{
			TraceError{ClassId} << "Unable to create the data inside the shared uniform buffer '" << identifier << "' for sky material '" << this->name() << "' !";

			return false;
		}

		if ( !this->createDescriptorSetLayout(renderer.layoutManager(), identifier) )
		{
			TraceError{ClassId} << "Unable to create the descriptor set layout for sky material '" << this->name() << "' !";

			return false;
		}

		if ( !this->createDescriptorSet(renderer, *m_sharedUniformBuffer->uniformBufferObject(m_sharedUBOIndex)) )
		{
			TraceError{ClassId} << "Unable to create the descriptor set for sky material '" << this->name() << "' !";

			return false;
		}

		if ( !this->updateVideoMemory() )
		{
			TraceError{ClassId} << "Unable to upload the initial data of sky material '" << this->name() << "' !";

			return false;
		}

		return true;
	}

	void
	SkyAtmosphereResource::destroy () noexcept
	{
		m_sharedUBOSeat.reset();

		m_descriptorSet.reset();
		m_descriptorSetLayout.reset();
		m_sharedUniformBuffer.reset();
		m_sharedUBOIndex = 0;
		m_renderer = nullptr;
		/* NOTE: The atmosphere is kept: it is the background's, and a re-creation binds the same tables. */
	}

	bool
	SkyAtmosphereResource::createElementInSharedBuffer (Renderer & renderer, const std::string & identifier) noexcept
	{
		m_sharedUniformBuffer = this->getSharedUniformBuffer(renderer, identifier);

		if ( m_sharedUniformBuffer == nullptr )
		{
			Tracer::error(ClassId, "Unable to get the shared uniform buffer !");

			return false;
		}

		m_sharedUBOSeat = m_sharedUniformBuffer->addElement(this, m_sharedUBOIndex);

		if ( !m_sharedUBOSeat.active() )
		{
			Tracer::error(ClassId, "Unable to add the sky material to the shared uniform buffer !");

			return false;
		}

		return true;
	}

	bool
	SkyAtmosphereResource::createDescriptorSetLayout (LayoutManager & layoutManager, const std::string & identifier) noexcept
	{
		m_descriptorSetLayout = layoutManager.getDescriptorSetLayout(identifier);

		if ( m_descriptorSetLayout != nullptr )
		{
			return true;
		}

		auto newLayout = layoutManager.prepareNewDescriptorSetLayout(identifier);
		newLayout->setIdentifier(ClassId, identifier, "DescriptorSetLayout");

		newLayout->declareUniformBuffer(MaterialBinding, VK_SHADER_STAGE_FRAGMENT_BIT);
		newLayout->declareUniformBuffer(AtmosphereBinding, VK_SHADER_STAGE_FRAGMENT_BIT);
		newLayout->declareCombinedImageSampler(SkyViewBinding, VK_SHADER_STAGE_FRAGMENT_BIT);
		newLayout->declareCombinedImageSampler(TransmittanceBinding, VK_SHADER_STAGE_FRAGMENT_BIT);
		newLayout->declareCombinedImageSampler(FrameBinding, VK_SHADER_STAGE_FRAGMENT_BIT);

		for ( uint32_t index = 0; index < MaxMoons; ++index )
		{
			newLayout->declareCombinedImageSampler(MoonSurfaceBinding + index, VK_SHADER_STAGE_FRAGMENT_BIT);
		}

		newLayout->declareCombinedImageSampler(PlanetSurfaceBinding, VK_SHADER_STAGE_FRAGMENT_BIT);

		if ( !layoutManager.createDescriptorSetLayout(newLayout) )
		{
			return false;
		}

		/* NOTE: Another thread may have registered the same layout first: hold the canonical one. */
		m_descriptorSetLayout = layoutManager.getDescriptorSetLayout(identifier);

		return m_descriptorSetLayout != nullptr;
	}

	bool
	SkyAtmosphereResource::createDescriptorSet (Renderer & renderer, const UniformBufferObject & /*uniformBufferObject*/) noexcept
	{
		m_descriptorSet = std::make_unique< DescriptorSet >(renderer.descriptorPool(), m_descriptorSetLayout);
		m_descriptorSet->setIdentifier(ClassId, this->name(), "DescriptorSet");

		if ( !m_descriptorSet->create() )
		{
			TraceError{ClassId} << "Unable to create the descriptor set for sky material '" << this->name() << "' !";

			return false;
		}

		if ( !m_descriptorSet->writeUniformBuffer(MaterialBinding, m_sharedUniformBuffer->getDescriptorInfoForElement(m_sharedUBOIndex)) )
		{
			TraceError{ClassId} << "Unable to write the uniform buffer to the descriptor set of sky material '" << this->name() << "' !";

			return false;
		}

		if ( !m_descriptorSet->writeUniformBufferObject(AtmosphereBinding, *m_atmosphere->parametersBuffer()) )
		{
			TraceError{ClassId} << "Unable to write the atmosphere parameters to the descriptor set of sky material '" << this->name() << "' !";

			return false;
		}

		/* NOTE: The tables' layouts are written explicitly: the atmosphere moves them with its own barriers, the
		 * image's tracked layout does not follow. Between the passes that write them, they are shader-read-only. */
		const auto writeTable = [this] (uint32_t binding, const StorageTexture2D & table) {
			return m_descriptorSet->writeCombinedImageSampler(binding, *table.imageView(), *table.sampler(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		};

		if ( !writeTable(SkyViewBinding, m_atmosphere->skyView()) ||
			 !writeTable(TransmittanceBinding, m_atmosphere->transmittance()) ||
			 !writeTable(FrameBinding, m_atmosphere->frameData()) )
		{
			TraceError{ClassId} << "Unable to write the atmosphere tables to the descriptor set of sky material '" << this->name() << "' !";

			return false;
		}

		const auto scales = std::span{m_properties}.subspan< MoonSurfacesOffset, MaxMoons >();

		for ( uint32_t index = 0; index < MaxMoons; ++index )
		{
			const auto & surface = *std::next(m_moonSurfaces.cbegin(), index);
			const auto written = surface != nullptr && scales[index] > 0.0F ?
				m_descriptorSet->writeCombinedImageSampler(MoonSurfaceBinding + index, *surface) :
				writeTable(MoonSurfaceBinding + index, m_atmosphere->transmittance());

			if ( !written )
			{
				TraceError{ClassId} << "Unable to write the surface of moon #" << index << " to the descriptor set of sky material '" << this->name() << "' !";

				return false;
			}
		}

		const auto planetWritten = m_planetSurface != nullptr && m_properties[PlanetSurfaceOffset] > 0.0F ?
			m_descriptorSet->writeCombinedImageSampler(PlanetSurfaceBinding, *m_planetSurface) :
			writeTable(PlanetSurfaceBinding, m_atmosphere->transmittance());

		if ( !planetWritten )
		{
			TraceError{ClassId} << "Unable to write the planet's surface to the descriptor set of sky material '" << this->name() << "' !";

			return false;
		}

		return true;
	}

	bool
	SkyAtmosphereResource::updateVideoMemory () noexcept
	{
		if ( m_sharedUniformBuffer == nullptr )
		{
			return false;
		}

		/* Lowered BEFORE the properties are read (see BeamResource::updateVideoMemory()). */
		m_videoMemoryUpdated.store(false, std::memory_order_release);

		return m_sharedUniformBuffer->writeElementData(m_sharedUBOIndex, 0, m_properties.data());
	}

	void
	SkyAtmosphereResource::markVideoMemoryDirty () noexcept
	{
		/* One registration per flush: the flag is cleared by updateVideoMemory(). Before creation, create() uploads. */
		if ( m_renderer == nullptr || !this->isCreated() || m_videoMemoryUpdated.exchange(true, std::memory_order_acq_rel) )
		{
			return;
		}

		m_renderer->requestMaterialVideoMemoryUpdate(std::static_pointer_cast< Interface >(this->weak_from_this().lock()));
	}

	uint32_t
	SkyAtmosphereResource::UBOAlignment () const noexcept
	{
		return m_sharedUniformBuffer->blockAlignedSize();
	}

	uint32_t
	SkyAtmosphereResource::UBOOffset () const noexcept
	{
		/* LOCAL offset within the element's bank (see StandardResource::UBOOffset()). */
		return static_cast< uint32_t >(m_sharedUniformBuffer->getByteOffsetForElement(m_sharedUBOIndex, 0));
	}

	bool
	SkyAtmosphereResource::samplesTexture (const TextureInterface * texture) const noexcept
	{
		if ( texture == nullptr || m_atmosphere == nullptr )
		{
			return false;
		}

		return texture == &m_atmosphere->skyView() || texture == &m_atmosphere->transmittance() || texture == &m_atmosphere->frameData();
	}

	Declaration::UniformBlock
	SkyAtmosphereResource::getUniformBlock (uint32_t set, uint32_t binding) const noexcept
	{
		Declaration::UniformBlock block{set, binding, Declaration::MemoryLayout::Std140, UniformBlock::Type::SkyAtmosphereMaterial, UniformBlock::Material};
		block.addMember(Declaration::VariableType::FloatVector4, UniformBlock::Component::SkyGrading);
		block.addMember(Declaration::VariableType::FloatVector4, UniformBlock::Component::SkyStar);
		block.addMember(Declaration::VariableType::FloatVector4, UniformBlock::Component::SkyMoonSurfaces);
		block.addMember(Declaration::VariableType::FloatVector4, UniformBlock::Component::SkyPlanetSurface);

		return block;
	}

	bool
	SkyAtmosphereResource::generateVertexShaderCode (Generator::Abstract & generator, AbstractVertexStage & vertexShader) const noexcept
	{
		if ( !this->isCreated() )
		{
			TraceError{ClassId} << "The sky material '" << this->name() << "' is not created ! It can't generate a vertex shader source code.";

			return false;
		}

		/* The skybox cube is centred on the camera and never rotated: its vertex position IS the world direction of
		 * the sky (see Material::checkPrimaryTextureCoordinates()). Normalized per fragment, not here: interpolating a
		 * unit vector across a face shortens it. */
		if ( !vertexShader.declare(Declaration::StageOutput{generator.getNextShaderVariableLocation(), GLSL::FloatVector3, SkyDirection, GLSL::Smooth}) )
		{
			return false;
		}

		Code{vertexShader, Location::Output} << SkyDirection << " = " << Attribute::Position << ";";

		return true;
	}

	bool
	SkyAtmosphereResource::generateFragmentShaderCode (Generator::Abstract & generator, LightGenerator & /*lightGenerator*/, FragmentShader & fragmentShader) const noexcept
	{
		if ( !this->isCreated() )
		{
			TraceError{ClassId} << "The sky material '" << this->name() << "' is not created ! It can't generate a fragment shader source code.";

			return false;
		}

		if ( !generator.declareMaterialUniformBlock(*this, fragmentShader, MaterialBinding) )
		{
			return false;
		}

		/* The pre-exposure caps the star's disc (StarDiscCeiling). */
		if ( !generator.declareViewUniformBlock(fragmentShader) )
		{
			return false;
		}

		const uint32_t materialSet = generator.shaderProgram()->setIndex(SetType::PerModelLayer);

		/* The atmosphere: the same block and functions the tables are built with (Effects/Shared/AtmosphereGLSL.hpp). */
		{
			Declaration::UniformBlock block{materialSet, AtmosphereBinding, Declaration::MemoryLayout::Std140, AtmosphereGLSL::BlockType, AtmosphereGLSL::BlockInstance};

			for ( const auto * member : AtmosphereGLSL::BlockMembers )
			{
				block.addMember(Declaration::VariableType::FloatVector4, member);
			}

			if ( !fragmentShader.declare(block) )
			{
				return false;
			}
		}

		if ( !fragmentShader.declare(Declaration::Sampler{materialSet, SkyViewBinding, GLSL::Sampler2D, SkyViewLut}) ||
			 !fragmentShader.declare(Declaration::Sampler{materialSet, TransmittanceBinding, GLSL::Sampler2D, TransmittanceLut}) ||
			 !fragmentShader.declare(Declaration::Sampler{materialSet, FrameBinding, GLSL::Sampler2D, FrameData}) )
		{
			return false;
		}

		for ( uint32_t index = 0; const auto & names : MoonNames )
		{
			if ( !fragmentShader.declare(Declaration::Sampler{materialSet, MoonSurfaceBinding + index, GLSL::Sampler2D, names.surface}) )
			{
				return false;
			}

			++index;
		}

		if ( !fragmentShader.declare(Declaration::Sampler{materialSet, PlanetSurfaceBinding, GLSL::Sampler2D, PlanetSurface}) )
		{
			return false;
		}

		for ( const auto * function : {&AtmosphereGLSL::FromUnitToSubUv, &AtmosphereGLSL::RaySphereNearest, &AtmosphereGLSL::TransmittanceParametersToUv, &AtmosphereGLSL::SkyViewParametersToUv, &AtmosphereGLSL::SkyViewLookup, &AtmosphereGLSL::Grade, &AtmosphereGLSL::LunarReflectance} )
		{
			if ( !declareAtmosphereFunction(fragmentShader, *function) )
			{
				TraceError{ClassId} << "Unable to declare the atmosphere function '" << function->name << "' in the sky material '" << this->name() << "' !";

				return false;
			}
		}

		const auto grading = MaterialUB(UniformBlock::Component::SkyGrading);
		const auto star = MaterialUB(UniformBlock::Component::SkyStar);

		/* The frame texture (SkyAtmosphere::recordFrame()): texel 0 (unit vector toward the star, cos of its angular
		 * radius), texel 1 (its illuminance above the atmosphere × its colour in lux, the camera's distance to the
		 * planet's centre in km). The sky-view table holds radiance PER LUX of that illuminance. */
		Code{fragmentShader, Location::Top} <<
			"const vec3 skyDir = normalize(" << SkyDirection << ");" << Line::End <<
			"const vec4 skyFrame0 = texelFetch(" << FrameData << ", ivec2(0, 0), 0);" << Line::End <<
			"const vec4 skyFrame1 = texelFetch(" << FrameData << ", ivec2(1, 0), 0);" << Line::End <<
			"const vec3 skyStarDir = skyFrame0.xyz;" << Line::End <<
			"const float skyViewHeight = skyFrame1.w;" << Line::End <<
			/* The table's lookup, shared with the environment capture (AtmosphereGLSL::SkyViewLookup). */
			"const vec3 skyLookup = atmSkyViewLookup(skyDir, skyStarDir, skyViewHeight);" << Line::End <<
			"const bool skyHitsGround = skyLookup.z > 0.5;" << Line::End <<
			"vec3 " << SkyRadiance << " = textureLod(" << SkyViewLut << ", skyLookup.xy, 0.0).rgb * skyFrame1.rgb;" << Line::End <<
			/* The star's disc: its illuminance spread over its solid angle (the mean radiance), shaped by the linear
			 * limb darkening normalized to that mean, seen through the atmosphere. Antialiased over one pixel. */
			/* ⚠️ The derivative is taken BEFORE the branch: fwidth() in non-uniform control flow is undefined. */
			"const float skyCosRadius = skyFrame0.w;" << Line::End <<
			"const float skyCosAngle = dot(skyDir, skyStarDir);" << Line::End <<
			"const float skyEdge = max(fwidth(skyCosAngle), 1e-7);" << Line::End <<
			"vec3 skySunDisc = vec3(0.0);" << Line::End <<
			"if ( " << star << ".y > 0.5 && !skyHitsGround ) {" << Line::End <<
			"	const float skyCoverage = smoothstep(skyCosRadius - skyEdge, skyCosRadius + skyEdge, skyCosAngle);" << Line::End <<
			"	if ( skyCoverage > 0.0 ) {" << Line::End <<
			"		const float skySolidAngle = 2.0 * 3.14159265358979 * max(1.0 - skyCosRadius, 1e-9);" << Line::End <<
			"		const float skyRadius = acos(clamp(skyCosRadius, -1.0, 1.0));" << Line::End <<
			"		const float skyRadial = clamp(acos(clamp(skyCosAngle, -1.0, 1.0)) / max(skyRadius, 1e-6), 0.0, 1.0);" << Line::End <<
			"		const float skyMu = sqrt(max(1.0 - (skyRadial * skyRadial), 0.0));" << Line::End <<
			"		const float skyLimb = (1.0 - " << star << ".z * (1.0 - skyMu)) / (1.0 - " << star << ".z / 3.0);" << Line::End <<
			"		const vec3 skyTransmittance = textureLod(" << TransmittanceLut << ", atmTransmittanceParametersToUv(skyViewHeight, skyDir.y), 0.0).rgb;" << Line::End <<
			"		vec3 skyDisc = skyFrame1.rgb / skySolidAngle * skyLimb * skyTransmittance;" << Line::End <<
			"		const float skyPeak = max(max(skyDisc.r, skyDisc.g), skyDisc.b) * " << ViewUB(UniformBlock::Component::PreExposure, false) << ";" << Line::End <<
			"		if ( skyPeak > " << StarDiscCeiling << " ) { skyDisc *= " << StarDiscCeiling << " / skyPeak; }" << Line::End <<
			"		skySunDisc = skyDisc * skyCoverage;" << Line::End <<
			"	}" << Line::End <<
			"}" << Line::End <<
			/* The MOONS (graphics doc 42 § "The moons"): frame texels 4 + 3i (direction, cos of the angular radius), 5 + 3i
			 * (radiance per unit reflectance × colour, earthshine), 6 + 3i (the orbit's pole). Each disc is a sphere lit by the
			 * star with the lunar reflectance — the one its light is the integral of — plus the planet's light from the
			 * observer's side, behind the atmosphere (× the transmittance) and in front of the star (it covers its disc). */
			"const int skyMoonCount = int(clamp(texelFetch(" << FrameData << ", ivec2(2, 0), 0).x, 0.0, " << MaxMoons << ".0));" << Line::End <<
			"const float skyEarthshineReflectance = atmLunarReflectance(1.0, 1.0, 0.0);" << Line::End <<
			/* The view direction's screen derivatives, in uniform control flow, once for every moon: the maps' gradients are
			 * derived from them analytically, inside each disc only (the rest of the sky pays nothing for the moons). */
			"const vec3 skyDirDx = dFdx(skyDir);" << Line::End <<
			"const vec3 skyDirDy = dFdy(skyDir);" << Line::End <<
			"float skyMoonCover = 0.0;" << Line::End <<
			"vec3 skyMoonLight = vec3(0.0);";

		for ( uint32_t index = 0; const auto & names : MoonNames )
		{
			const auto moon = std::string{"skyMoon"} + std::to_string(index);
			const auto texel = [index] (uint32_t offset) {
				return std::to_string(4 + (3 * index) + offset);
			};
			const auto surfaceScale = MaterialUB(UniformBlock::Component::SkyMoonSurfaces) + "." + names.lane;

			/* ⚠️ The only implicit derivative, fwidth() of the edge, is taken in the branch on the moon count, which is
			 * UNIFORM: implicit derivatives in non-uniform control flow are undefined. Everything else waits for a covered
			 * pixel; the map's gradients come from skyDirDx / skyDirDy by the chain rule (sphere hit → normal → longitude,
			 * latitude), as exact as dFdx / dFdy of the coordinates were. */
			Code{fragmentShader, Location::Top} <<
				"if ( skyMoonCount > " << index << " ) {" << Line::End <<
				"	const vec4 " << moon << "Centre = texelFetch(" << FrameData << ", ivec2(" << texel(0) << ", 0), 0);" << Line::End <<
				"	const float " << moon << "CosAngle = dot(skyDir, " << moon << "Centre.xyz);" << Line::End <<
				"	const float " << moon << "Edge = max(fwidth(" << moon << "CosAngle), 1e-7);" << Line::End <<
				"	const float " << moon << "Coverage = skyHitsGround ? 0.0 : smoothstep(" << moon << "Centre.w - " << moon << "Edge, " << moon << "Centre.w + " << moon << "Edge, " << moon << "CosAngle);" << Line::End <<
				"	if ( " << moon << "Coverage > 0.0 ) {" << Line::End <<
				"		const vec4 " << moon << "Light = texelFetch(" << FrameData << ", ivec2(" << texel(1) << ", 0), 0);" << Line::End <<
				"		const vec4 " << moon << "Pole = texelFetch(" << FrameData << ", ivec2(" << texel(2) << ", 0), 0);" << Line::End <<
				/* The sphere at a unit distance, of radius sin r: the ray's nearest hit t (the limb's in the antialiased band),
				 * the point p from the centre, the unit normal n. */
				"		const float " << moon << "Half = sqrt(max((" << moon << "CosAngle * " << moon << "CosAngle) - (" << moon << "Centre.w * " << moon << "Centre.w), 0.0));" << Line::End <<
				"		const float " << moon << "Hit = " << moon << "CosAngle - " << moon << "Half;" << Line::End <<
				"		const vec3 " << moon << "Point = (skyDir * " << moon << "Hit) - " << moon << "Centre.xyz;" << Line::End <<
				"		const float " << moon << "Radius = max(length(" << moon << "Point), 1e-7);" << Line::End <<
				"		const vec3 " << moon << "Normal = " << moon << "Point / " << moon << "Radius;" << Line::End <<
				/* The face toward the planet, north along the orbit's pole (perpendicular to the moon's direction by construction;
				 * any perpendicular if degenerate): 0° longitude at the map's centre, east to the right. */
				"		const vec3 " << moon << "Toward = -" << moon << "Centre.xyz;" << Line::End <<
				"		vec3 " << moon << "North = " << moon << "Pole.xyz - (dot(" << moon << "Pole.xyz, " << moon << "Toward) * " << moon << "Toward);" << Line::End <<
				"		if ( dot(" << moon << "North, " << moon << "North) < 1e-8 ) {" << Line::End <<
				"			" << moon << "North = abs(" << moon << "Toward.y) < 0.9 ? vec3(0.0, 1.0, 0.0) : vec3(1.0, 0.0, 0.0);" << Line::End <<
				"			" << moon << "North -= dot(" << moon << "North, " << moon << "Toward) * " << moon << "Toward;" << Line::End <<
				"		}" << Line::End <<
				"		" << moon << "North = normalize(" << moon << "North);" << Line::End <<
				"		const vec3 " << moon << "East = cross(" << moon << "North, " << moon << "Toward);" << Line::End <<
				"		const vec3 " << moon << "Local = vec3(dot(" << moon << "Normal, " << moon << "Toward), dot(" << moon << "Normal, " << moon << "East), dot(" << moon << "Normal, " << moon << "North));" << Line::End <<
				"		const vec2 " << moon << "Uv = vec2(0.5 + (atan(" << moon << "Local.y, " << moon << "Local.x) / 6.28318531), 0.5 - (asin(clamp(" << moon << "Local.z, -1.0, 1.0)) / 3.14159265));" << Line::End <<
				/* d(hit) from |p| = R: dt = -t (p · dd) / (p · d); p · d → 0 at the limb (an infinite footprint, clamped). Then
				 * dn = (dp - n (n · dp)) / R, du = (x dy - y dx) / (2π (x² + y²)), dv = -dz / (π √(1 - z²)). */
				"		const float " << moon << "Facing = min(dot(" << moon << "Point, skyDir), -1e-3 * " << moon << "Radius);" << Line::End <<
				"		mat2 " << moon << "Gradients;" << Line::End <<
				"		for ( int skyAxis = 0; skyAxis < 2; ++skyAxis ) {" << Line::End <<
				"			const vec3 skyDd = skyAxis == 0 ? skyDirDx : skyDirDy;" << Line::End <<
				"			const vec3 skyDp = (skyDir * (-" << moon << "Hit * dot(" << moon << "Point, skyDd) / " << moon << "Facing)) + (skyDd * " << moon << "Hit);" << Line::End <<
				"			const vec3 skyDn = (skyDp - (" << moon << "Normal * dot(" << moon << "Normal, skyDp))) / " << moon << "Radius;" << Line::End <<
				"			const vec3 skyDl = vec3(dot(skyDn, " << moon << "Toward), dot(skyDn, " << moon << "East), dot(skyDn, " << moon << "North));" << Line::End <<
				"			" << moon << "Gradients[skyAxis] = vec2(((" << moon << "Local.x * skyDl.y) - (" << moon << "Local.y * skyDl.x)) / (6.28318531 * max(dot(" << moon << "Local.xy, " << moon << "Local.xy), 1e-8)), -skyDl.z / (3.14159265 * sqrt(max(1.0 - (" << moon << "Local.z * " << moon << "Local.z), 1e-8))));" << Line::End <<
				"		}" << Line::End <<
				/* The phase angle, star–moon–observer: the star at infinity. */
				"		const float " << moon << "Phase = acos(clamp(dot(skyStarDir, " << moon << "Toward), -1.0, 1.0));" << Line::End <<
				"		const float " << moon << "CosEmission = max(dot(" << moon << "Normal, -skyDir), 1e-4);" << Line::End <<
				"		float " << moon << "Reflectance = atmLunarReflectance(dot(" << moon << "Normal, skyStarDir), " << moon << "CosEmission, " << moon << "Phase) + (" << moon << "Light.w * skyEarthshineReflectance);" << Line::End <<
				"		if ( " << surfaceScale << " > 0.0 ) {" << Line::End <<
				"			" << moon << "Reflectance *= dot(textureGrad(" << names.surface << ", " << moon << "Uv, " << moon << "Gradients[0], " << moon << "Gradients[1]).rgb, vec3(0.2126, 0.7152, 0.0722)) * " << surfaceScale << ";" << Line::End <<
				"		}" << Line::End <<
				"		const vec3 " << moon << "Transmittance = textureLod(" << TransmittanceLut << ", atmTransmittanceParametersToUv(skyViewHeight, skyDir.y), 0.0).rgb;" << Line::End <<
				"		skyMoonLight += " << moon << "Light.rgb * " << moon << "Reflectance * " << moon << "Transmittance * " << moon << "Coverage;" << Line::End <<
				"		skyMoonCover = max(skyMoonCover, " << moon << "Coverage);" << Line::End <<
				"	}" << Line::End <<
				"}";

			++index;
		}

		/* The PLANETS (graphics doc 42 § "The planets"): frame texels PlanetTexels + 5j — the direction and the SINE of the
		 * angular radius (a float's cos of a few arc seconds is 1: the disc is tested on the perpendicular offset), K × the
		 * colour and the scattering, the pole and K, the prime meridian, the direction toward its star. Each disc is a sphere seen from afar (orthographic:
		 * the normal from the offset), Lambert's or Lommel-Seeliger's radiance K f(μ0, μ) whose integral is the magnitude's
		 * illuminance, through the atmosphere, behind the moons. Under 1.5 px the star pass draws it as a point; from 1.5 to
		 * 3 px the two cross-fade (the same weight on both sides: the light is conserved). The textured planet reads its map
		 * by its planetocentric east longitude and latitude, the gradients by the chain rule from skyDirDx / skyDirDy. */
		const auto planetSurface = MaterialUB(UniformBlock::Component::SkyPlanetSurface);

		Code{fragmentShader, Location::Top} <<
			"const int skyPlanetCount = int(clamp(texelFetch(" << FrameData << ", ivec2(2, 0), 0).y, 0.0, " << MaxPlanets << ".0));" << Line::End <<
			"const float skyPixelAngle = max(max(length(skyDirDx), length(skyDirDy)), 1e-12);" << Line::End <<
			"vec3 skyPlanetLight = vec3(0.0);" << Line::End <<
			"float skyPlanetCover = 0.0;" << Line::End <<
			"for ( int skyPlanet = 0; skyPlanet < skyPlanetCount; ++skyPlanet ) {" << Line::End <<
			"	const int skyTexel = " << SkyAtmosphere::PlanetTexels << " + (" << SkyAtmosphere::TexelsPerPlanet << " * skyPlanet);" << Line::End <<
			"	const vec4 skyPlanetCentre = texelFetch(" << FrameData << ", ivec2(skyTexel, 0), 0);" << Line::End <<
			"	const float skyPlanetSin = max(skyPlanetCentre.w, 1e-12);" << Line::End <<
			"	const float skyPlanetWeight = smoothstep(1.5, 3.0, 2.0 * skyPlanetSin / skyPixelAngle);" << Line::End <<
			"	const vec3 skyOffset = skyDir - (skyPlanetCentre.xyz * dot(skyDir, skyPlanetCentre.xyz));" << Line::End <<
			"	const float skyRho = length(skyOffset) / skyPlanetSin;" << Line::End <<
			/* One pixel of the disc's radius, for the antialiased edge. */
			"	const float skyRhoEdge = skyPixelAngle / skyPlanetSin;" << Line::End <<
			"	if ( skyPlanetWeight <= 0.0 || skyHitsGround || skyRho >= 1.0 + skyRhoEdge || dot(skyDir, skyPlanetCentre.xyz) <= 0.0 ) {" << Line::End <<
			"		continue;" << Line::End <<
			"	}" << Line::End <<
			"	const vec4 skyPlanetLightK = texelFetch(" << FrameData << ", ivec2(skyTexel + 1, 0), 0);" << Line::End <<
			"	const vec4 skyPlanetPole = texelFetch(" << FrameData << ", ivec2(skyTexel + 2, 0), 0);" << Line::End <<
			"	const vec4 skyPlanetMeridian = texelFetch(" << FrameData << ", ivec2(skyTexel + 3, 0), 0);" << Line::End <<
			/* ⚠️ The light from the PLANET toward its star, not the observer's star direction (Mars at 37° of phase angle
			 * was drawn as a crescent, 2026-10-26). */
			"	const vec3 skyPlanetLightDir = texelFetch(" << FrameData << ", ivec2(skyTexel + 4, 0), 0).xyz;" << Line::End <<
			"	const float skyDepth = sqrt(max(1.0 - min(skyRho * skyRho, 1.0), 0.0));" << Line::End <<
			"	const vec3 skyNormal = normalize((skyOffset / skyPlanetSin) - (skyPlanetCentre.xyz * skyDepth));" << Line::End <<
			"	const float skyMu0 = max(dot(skyNormal, skyPlanetLightDir), 0.0);" << Line::End <<
			"	const float skyMu = max(dot(skyNormal, -skyDir), 1e-4);" << Line::End <<
			"	const float skyShape = skyPlanetLightK.w > 0.5 ? skyMu0 / (skyMu0 + skyMu) : skyMu0;" << Line::End <<
			"	vec3 skyPlanetRadiance = skyPlanetLightK.rgb * skyShape;" << Line::End <<
			"	if ( " << planetSurface << ".x > 0.0 && skyPlanet == int(" << planetSurface << ".y + 0.5) ) {" << Line::End <<
			"		const vec3 skyEast = cross(skyPlanetPole.xyz, skyPlanetMeridian.xyz);" << Line::End <<
			"		const vec3 skyLocal = vec3(dot(skyNormal, skyPlanetMeridian.xyz), dot(skyNormal, skyEast), dot(skyNormal, skyPlanetPole.xyz));" << Line::End <<
			"		const vec2 skyUv = vec2(0.5 + (atan(skyLocal.y, skyLocal.x) / 6.28318531), 0.5 - (asin(clamp(skyLocal.z, -1.0, 1.0)) / 3.14159265));" << Line::End <<
			/* dn = do / sin r − c d(depth), d(depth) = −(o · do) / (sin² r depth); then the moons' du, dv. */
			"		mat2 skyGradients;" << Line::End <<
			"		for ( int skyAxis = 0; skyAxis < 2; ++skyAxis ) {" << Line::End <<
			"			const vec3 skyDd = skyAxis == 0 ? skyDirDx : skyDirDy;" << Line::End <<
			"			const vec3 skyDo = skyDd - (skyPlanetCentre.xyz * dot(skyDd, skyPlanetCentre.xyz));" << Line::End <<
			"			const float skyDdepth = -dot(skyOffset, skyDo) / (skyPlanetSin * skyPlanetSin * max(skyDepth, 1e-3));" << Line::End <<
			"			const vec3 skyDn = (skyDo / skyPlanetSin) - (skyPlanetCentre.xyz * skyDdepth);" << Line::End <<
			"			const vec3 skyDl = vec3(dot(skyDn, skyPlanetMeridian.xyz), dot(skyDn, skyEast), dot(skyDn, skyPlanetPole.xyz));" << Line::End <<
			"			skyGradients[skyAxis] = vec2(((skyLocal.x * skyDl.y) - (skyLocal.y * skyDl.x)) / (6.28318531 * max(dot(skyLocal.xy, skyLocal.xy), 1e-8)), -skyDl.z / (3.14159265 * sqrt(max(1.0 - (skyLocal.z * skyLocal.z), 1e-8))));" << Line::End <<
			"		}" << Line::End <<
			/* The map's own colour, its sphere mean luminance divided out: K × f × the relative albedo. */
			"		skyPlanetRadiance = skyPlanetPole.w * skyShape * textureGrad(" << PlanetSurface << ", skyUv, skyGradients[0], skyGradients[1]).rgb * " << planetSurface << ".x;" << Line::End <<
			"	}" << Line::End <<
			"	const float skyPlanetEdge = 1.0 - smoothstep(1.0 - skyRhoEdge, 1.0 + skyRhoEdge, skyRho);" << Line::End <<
			"	const vec3 skyPlanetTransmittance = textureLod(" << TransmittanceLut << ", atmTransmittanceParametersToUv(skyViewHeight, skyDir.y), 0.0).rgb;" << Line::End <<
			/* The antialiased edge, a ramp of ±1 px, adds π e² / 3 to a disc of radius 1 (e in its radius): divided out, a
			 * 1.5-px disc keeps the light of its point. */
			"	skyPlanetLight += skyPlanetRadiance * skyPlanetTransmittance * skyPlanetEdge * skyPlanetWeight / (1.0 + (skyRhoEdge * skyRhoEdge / 3.0));" << Line::End <<
			"	skyPlanetCover = max(skyPlanetCover, skyPlanetEdge * skyPlanetWeight);" << Line::End <<
			"}";

		Code{fragmentShader, Location::Top} <<
			SkyRadiance << " += (skySunDisc * (1.0 - max(skyMoonCover, skyPlanetCover))) + (skyPlanetLight * (1.0 - skyMoonCover)) + skyMoonLight;" << Line::End <<
			/* The artistic grading, shared with the environment capture (AtmosphereGLSL::Grade). */
			SkyRadiance << " = atmGrade(" << SkyRadiance << ", " << grading << ", " << star << ".x);";

		return true;
	}

	std::string
	SkyAtmosphereResource::reactiveMaskExpression () const noexcept
	{
		/* Both covers are declared at the top of the generated main (generateFragmentShaderCode()). */
		return "max(skyMoonCover, skyPlanetCover)";
	}

	std::string
	SkyAtmosphereResource::fragmentColor () const noexcept
	{
		return std::string{"vec4("} + SkyRadiance + ", 1.0)";
	}

	void
	SkyAtmosphereResource::enableBlending (BlendingMode mode) noexcept
	{
		TraceWarning{ClassId} << "A sky is opaque: blending '" << to_cstring(mode) << "' ignored !";
	}

	void
	SkyAtmosphereResource::setGrading (const Color< float > & tint, float intensity) noexcept
	{
		m_properties[GradingOffset] = std::max(0.0F, tint.red());
		m_properties[GradingOffset + 1] = std::max(0.0F, tint.green());
		m_properties[GradingOffset + 2] = std::max(0.0F, tint.blue());
		m_properties[GradingOffset + 3] = std::max(0.0F, intensity);

		this->markVideoMemoryDirty();
	}

	void
	SkyAtmosphereResource::setSaturation (float saturation) noexcept
	{
		m_properties[StarOffset] = std::max(0.0F, saturation);

		this->markVideoMemoryDirty();
	}

	void
	SkyAtmosphereResource::enableStarDisc (bool state) noexcept
	{
		m_properties[StarOffset + 1] = state ? 1.0F : 0.0F;

		this->markVideoMemoryDirty();
	}

	void
	SkyAtmosphereResource::setLimbDarkening (float coefficient) noexcept
	{
		m_properties[StarOffset + 2] = std::clamp(coefficient, 0.0F, 1.0F);

		this->markVideoMemoryDirty();
	}
}
