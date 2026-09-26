/*
 * src/Graphics/OverflowCensus.cpp
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

#include "OverflowCensus.hpp"

/* STL inclusions. */
#include <algorithm>
#include <bit>
#include <cstring>
#include <optional>
#include <string>
#include <string_view>

/* Local inclusions. */
#include "IrradianceProbeVolume.hpp"
#include "Renderer.hpp"
#include "Saphir/ShaderManager.hpp"
#include "Tracer.hpp"
#include "Vulkan/Buffer.hpp"
#include "Vulkan/CommandBuffer.hpp"
#include "Vulkan/ComputePipeline.hpp"
#include "Vulkan/DescriptorPool.hpp"
#include "Vulkan/DescriptorSet.hpp"
#include "Vulkan/DescriptorSetLayout.hpp"
#include "Vulkan/Device.hpp"
#include "Vulkan/GPUProfiler.hpp"
#include "Vulkan/Image.hpp"
#include "Vulkan/ImageView.hpp"
#include "Vulkan/MemoryRegion.hpp"
#include "Vulkan/PhysicalDevice.hpp"
#include "Vulkan/PipelineLayout.hpp"
#include "Vulkan/Sampler.hpp"
#include "Vulkan/ShaderModule.hpp"
#include "Vulkan/Sync/BufferMemoryBarrier.hpp"
#include "Vulkan/Sync/MemoryBarrier.hpp"
#include "Vulkan/TextureInterface.hpp"
#include "Vulkan/TransferManager.hpp"

namespace
{
	using namespace EmEn;

	/* The census compute shader, one source for both variants (a plain 2D image, or the 2D-array probe atlas with
	 * EM_CENSUS_ARRAY defined after the version line). 16×16 invocations per workgroup, one per texel; the counts are
	 * reduced in shared memory, then ONE atomic per workgroup and per class lands in the channel's region.
	 *
	 * ⚠️ INTEGER-ONLY classification of the IEEE bits of the worst RGB channel: no isnan()/isinf() and no float
	 * comparison, which a fast-math compiler may fold away (Metal does, see ToneMapping's plausibility window). With
	 * the sign bit cleared, the IEEE order of the bits IS the numeric order of the magnitudes, so one max() of three
	 * uints finds the worst channel and the classes are plain integer thresholds: above 0x7F800000 is a NaN (any
	 * payload, any sign), 0x7F800000 is an infinity, and 0x477FE000 (65 504.0, the largest finite binary16) up to it
	 * is the ceiling. Alpha is never read: nothing the census counts carries radiance there.
	 *
	 * ⚠️ No early return before either barrier(): both must be reached in uniform control flow, so an invocation outside
	 * the image or on an atlas tile border simply tests nothing. */
	constexpr auto CensusComputeShaderBody = R"GLSL(
layout(local_size_x = 16, local_size_y = 16, local_size_z = 1) in;

#ifdef EM_CENSUS_ARRAY
layout(set = 0, binding = 0) uniform sampler2DArray sourceImage;
#else
layout(set = 0, binding = 0) uniform sampler2D sourceImage;
#endif

/* THIS channel's region of the slot buffer: the descriptor range IS the channel, disjoint from every other
 * dispatch of the batch, so the dispatches need no barrier between them. */
layout(set = 0, binding = 1, std430) buffer ChannelCounters
{
	uint tested;
	uint nanTexels;
	uint infTexels;
	uint ceilingTexels;
	uint peakBits;
} counters;

layout(push_constant) uniform PushConstants
{
	uint width;
	uint height;
	/* Texels per atlas tile side, border included; 0 for a plain image. */
	uint tile;
	uint border;
} pc;

const uint InfBits = 0x7F800000u;
const uint HalfMaxBits = 0x477FE000u;

shared uint s_tested;
shared uint s_nan;
shared uint s_inf;
shared uint s_ceiling;
shared uint s_peak;

void
main ()
{
	if ( gl_LocalInvocationIndex == 0u )
	{
		s_tested = 0u;
		s_nan = 0u;
		s_inf = 0u;
		s_ceiling = 0u;
		s_peak = 0u;
	}

	memoryBarrierShared();
	barrier();

	uvec2 p = gl_GlobalInvocationID.xy;
	bool inside = p.x < pc.width && p.y < pc.height;

	/* Probe atlas: the interior texels only — a border repeats the octahedral wrap. */
	if ( inside && pc.tile != 0u )
	{
		uvec2 t = p % pc.tile;

		inside = all(greaterThanEqual(t, uvec2(pc.border))) && all(lessThan(t, uvec2(pc.tile - pc.border)));
	}

	if ( inside )
	{
#ifdef EM_CENSUS_ARRAY
		vec3 c = texelFetch(sourceImage, ivec3(ivec2(p), int(gl_GlobalInvocationID.z)), 0).rgb;
#else
		vec3 c = texelFetch(sourceImage, ivec2(p), 0).rgb;
#endif

		/* |c| as bits: the IEEE order of the bits is the numeric order of the magnitudes. */
		uvec3 m = floatBitsToUint(c) & uvec3(0x7FFFFFFFu);
		uint worst = max(m.r, max(m.g, m.b));

		atomicAdd(s_tested, 1u);

		if ( worst > InfBits )
		{
			/* Any NaN channel wins. */
			atomicAdd(s_nan, 1u);
		}
		else if ( worst == InfBits )
		{
			atomicAdd(s_inf, 1u);
		}
		else if ( worst >= HalfMaxBits )
		{
			/* Finite, at the binary16 ceiling: an overflow a GPU rounded to the largest finite value. */
			atomicAdd(s_ceiling, 1u);
		}
		else
		{
			atomicMax(s_peak, worst);
		}
	}

	memoryBarrierShared();
	barrier();

	if ( gl_LocalInvocationIndex == 0u )
	{
		atomicAdd(counters.tested, s_tested);

		if ( s_nan != 0u )
		{
			atomicAdd(counters.nanTexels, s_nan);
		}

		if ( s_inf != 0u )
		{
			atomicAdd(counters.infTexels, s_inf);
		}

		if ( s_ceiling != 0u )
		{
			atomicAdd(counters.ceilingTexels, s_ceiling);
		}

		atomicMax(counters.peakBits, s_peak);
	}
}
)GLSL";

	/** @brief The workgroup side of the census shader. */
	constexpr uint32_t WorkgroupSide{16};

	/** @brief Bytes of one channel region: five uints, rounded up (the stride then honours the storage alignment). */
	constexpr VkDeviceSize RegionBytes{32};

	/** @brief The GPU counters of one region, in the order of the GLSL block. */
	enum CounterIndex : size_t
	{
		CounterTested = 0,
		CounterNaN = 1,
		CounterInf = 2,
		CounterCeiling = 3,
		CounterPeakBits = 4,
		CounterCount = 5
	};

	static_assert(sizeof(uint32_t) * CounterCount <= RegionBytes, "A census region must hold the five counters.");

	/**
	 * @brief Copies a channel name, truncated and zero-terminated.
	 * @param destination The name to write.
	 * @param source The source string.
	 * @return void
	 */
	void
	copyName (Graphics::OverflowCensusChannelName & destination, std::string_view source) noexcept
	{
		destination.fill('\0');

		const auto length = std::min(source.size(), destination.size() - 1);

		std::memcpy(destination.data(), source.data(), length);
	}

	/**
	 * @brief Returns whether a format is a floating-point colour format, the only kind a census count means anything on.
	 * @param format The format.
	 * @return bool
	 */
	[[nodiscard]]
	bool
	isFloatColourFormat (VkFormat format) noexcept
	{
		switch ( format )
		{
			case VK_FORMAT_R16G16B16A16_SFLOAT :
			case VK_FORMAT_R16G16B16_SFLOAT :
			case VK_FORMAT_R32G32B32A32_SFLOAT :
			case VK_FORMAT_R32G32B32_SFLOAT :
			case VK_FORMAT_B10G11R11_UFLOAT_PACK32 :
				return true;

			default :
				return false;
		}
	}

	/**
	 * @brief Encodes a small positive integer as a binary16, exactly (1 to 2048).
	 * @param value The integer.
	 * @return uint16_t
	 */
	[[nodiscard]]
	constexpr
	uint16_t
	halfFromInteger (uint32_t value) noexcept
	{
		uint32_t exponent = 0;

		while ( (value >> (exponent + 1U)) != 0U )
		{
			++exponent;
		}

		const uint32_t mantissa = ((value << 10U) >> exponent) & 0x3FFU;

		return static_cast< uint16_t >(((exponent + 15U) << 10U) | mantissa);
	}

	static_assert(halfFromInteger(1) == 0x3C00U, "1.0 is 0x3C00 in binary16.");
	static_assert(halfFromInteger(255) == 0x5BF8U, "255.0 is 0x5BF8 in binary16.");

	/** @brief The self-test image, RGBA16F, as raw half bit patterns. */
	using SelfTestTexels = std::array< uint16_t, static_cast< size_t >(Graphics::OverflowCensus::SelfTestExtent) * Graphics::OverflowCensus::SelfTestExtent * 4 >;

	/**
	 * @brief Builds the self-test image.
	 * @note Row-major texel index i, raw half bits; every channel not listed is 1.0 (0x3C00). The expected census of
	 * this image is the OverflowCensus::SelfTestExpected* tuple — 256 tested, 21 NaN, 12 Inf, 8 ceiling, peak
	 * 65 472 — and the reason for each pattern is the trap it tests:
	 *  -   0-9   R = 0x7E00 (qNaN)                              -> NaN
	 *  -  10-14  G = 0xFE00 (-qNaN)                             -> NaN (the sign is cleared)
	 *  -  15-19  B = 0x7C01 (NaN, smallest payload)             -> NaN (a payload, not only the canonical one)
	 *  -  20     R = 0x7E00 and G = 0x7C00 (+Inf)               -> NaN, counted ONCE (precedence)
	 *  -  21-28  R = 0x7C00 (+Inf)                              -> Inf
	 *  -  29-32  B = 0xFC00 (-Inf)                              -> Inf (by magnitude)
	 *  -  33-38  G = 0x7BFF (65 504)                            -> ceiling
	 *  -  39-40  R = 0xFBFF (-65 504)                           -> ceiling (by magnitude)
	 *  -  41-46  A = 0x7E00, RGB = 1.0                          -> nothing (alpha is never counted)
	 *  -  47     R = 0x7BFE (65 472)                            -> nothing, and it is the peak
	 *  -  48-50  R = 0x0001 (subnormal)                         -> nothing, flushed to zero or not
	 *  -  51-255 R = G = B = i                                  -> nothing
	 * @return SelfTestTexels
	 */
	[[nodiscard]]
	SelfTestTexels
	buildSelfTestTexels () noexcept
	{
		SelfTestTexels texels{};
		texels.fill(0x3C00U);

		const auto set = [&texels] (size_t texel, size_t channel, uint16_t bits) {
			texels[(texel * 4) + channel] = bits;
		};

		constexpr size_t R{0};
		constexpr size_t G{1};
		constexpr size_t B{2};
		constexpr size_t A{3};

		for ( size_t texel = 0; texel <= 9; ++texel )
		{
			set(texel, R, 0x7E00U);
		}

		for ( size_t texel = 10; texel <= 14; ++texel )
		{
			set(texel, G, 0xFE00U);
		}

		for ( size_t texel = 15; texel <= 19; ++texel )
		{
			set(texel, B, 0x7C01U);
		}

		set(20, R, 0x7E00U);
		set(20, G, 0x7C00U);

		for ( size_t texel = 21; texel <= 28; ++texel )
		{
			set(texel, R, 0x7C00U);
		}

		for ( size_t texel = 29; texel <= 32; ++texel )
		{
			set(texel, B, 0xFC00U);
		}

		for ( size_t texel = 33; texel <= 38; ++texel )
		{
			set(texel, G, 0x7BFFU);
		}

		for ( size_t texel = 39; texel <= 40; ++texel )
		{
			set(texel, R, 0xFBFFU);
		}

		for ( size_t texel = 41; texel <= 46; ++texel )
		{
			set(texel, A, 0x7E00U);
		}

		set(47, R, 0x7BFEU);

		for ( size_t texel = 48; texel <= 50; ++texel )
		{
			set(texel, R, 0x0001U);
		}

		for ( size_t texel = 51; texel < Graphics::OverflowCensus::SelfTestExpectedTested; ++texel )
		{
			const auto bits = halfFromInteger(static_cast< uint32_t >(texel));

			set(texel, R, bits);
			set(texel, G, bits);
			set(texel, B, bits);
		}

		return texels;
	}
}

namespace EmEn::Graphics
{
	using namespace Base;
	using namespace Vulkan;
	using namespace Saphir;

	OverflowCensus::OverflowCensus () noexcept = default;

	OverflowCensus::~OverflowCensus () noexcept
	{
		this->destroy();
	}

	bool
	OverflowCensus::create (Renderer & renderer) noexcept
	{
		if ( this->available() )
		{
			return true;
		}

		const auto frameCount = renderer.framesInFlight();

		if ( frameCount == 0 )
		{
			TraceError{ClassId} << "The renderer has no frame in flight yet: the census cannot size its per-frame ring !";

			return false;
		}

		m_device = renderer.device();

		if ( m_device == nullptr )
		{
			TraceError{ClassId} << "No device !";

			return false;
		}

		/* One region for the serial header, then one per channel, each at the storage offset alignment: a
		 * descriptor's range must start on it, and each channel's descriptor covers ONLY its own region. */
		const auto alignment = std::max< VkDeviceSize >(m_device->physicalDevice()->propertiesVK10().limits.minStorageBufferOffsetAlignment, 1);

		m_stride = ((RegionBytes + alignment - 1) / alignment) * alignment;

		if ( !this->createPipelines(renderer) || !this->createSlots(frameCount) || !this->createSelfTestImage(renderer) )
		{
			TraceWarning{ClassId} << "The overflow census is UNAVAILABLE for this session (see above).";

			this->destroy();

			return false;
		}

		m_available.store(true, std::memory_order_release);

		TraceSuccess{ClassId} << "Overflow census ready: " << frameCount << " frame slots, " << MaxChannels << " channels of " << m_stride << " bytes each, " << ( this->armed() ? "ARMED" : "disarmed" ) << ".";

		return true;
	}

	bool
	OverflowCensus::createPipelines (Renderer & renderer) noexcept
	{
		m_descriptorSetLayout = std::make_shared< DescriptorSetLayout >(m_device, "OverflowCensus");
		m_descriptorSetLayout->setIdentifier(ClassId, "Main", "DescriptorSetLayout");

		if ( !m_descriptorSetLayout->declareCombinedImageSampler(0, VK_SHADER_STAGE_COMPUTE_BIT) || !m_descriptorSetLayout->declareStorageBuffer(1, VK_SHADER_STAGE_COMPUTE_BIT) )
		{
			TraceError{ClassId} << "Unable to declare the descriptor set layout bindings !";

			return false;
		}

		if ( !m_descriptorSetLayout->createOnHardware() )
		{
			TraceError{ClassId} << "Unable to create the descriptor set layout !";

			return false;
		}

		VkPushConstantRange pushConstantRange{};
		pushConstantRange.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
		pushConstantRange.offset = 0;
		pushConstantRange.size = sizeof(PushConstants);

		m_pipelineLayout = std::make_shared< PipelineLayout >(
			m_device, "OverflowCensusPipelineLayout",
			StaticVector< std::shared_ptr< DescriptorSetLayout >, 6 >{m_descriptorSetLayout},
			StaticVector< VkPushConstantRange, 4 >{pushConstantRange}
		);

		if ( !m_pipelineLayout->createOnHardware() )
		{
			TraceError{ClassId} << "Unable to create the pipeline layout !";

			return false;
		}

		auto & shaderManager = renderer.shaderManager();

		const auto buildPipeline = [&] (std::unique_ptr< ComputePipeline > & pipeline, const char * name, const std::string & source) {
			const auto shaderModule = shaderManager.getShaderModuleFromSourceCode(m_device, name, ShaderType::ComputeShader, source);

			if ( shaderModule == nullptr )
			{
				TraceError{ClassId} << "Failed to compile the compute shader '" << name << "' !";

				return false;
			}

			pipeline = std::make_unique< ComputePipeline >(m_pipelineLayout);
			pipeline->setShaderModule(shaderModule->handle());

			if ( !pipeline->createOnHardware() )
			{
				TraceError{ClassId} << "Unable to create the compute pipeline '" << name << "' !";

				pipeline.reset();

				return false;
			}

			return true;
		};

		const std::string source2D = std::string{"#version 450\n"} + CensusComputeShaderBody;
		const std::string sourceArray = std::string{"#version 450\n#define EM_CENSUS_ARRAY\n"} + CensusComputeShaderBody;

		if ( !buildPipeline(m_pipeline2D, "OverflowCensus2D_CS", source2D) || !buildPipeline(m_pipelineArray, "OverflowCensusArray_CS", sourceArray) )
		{
			return false;
		}

		/* texelFetch() ignores the filter; nearest and clamped all the same, and no anisotropy. */
		m_sampler = renderer.getSampler("OverflowCensus", [] (Settings & /*settings*/, VkSamplerCreateInfo & createInfo) {
			createInfo.magFilter = VK_FILTER_NEAREST;
			createInfo.minFilter = VK_FILTER_NEAREST;
			createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
			createInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
			createInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
			createInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
			createInfo.anisotropyEnable = VK_FALSE;
			createInfo.compareEnable = VK_FALSE;
			createInfo.minLod = 0.0F;
			createInfo.maxLod = 0.0F;
		});

		if ( m_sampler == nullptr )
		{
			TraceError{ClassId} << "Unable to get the census sampler !";

			return false;
		}

		return true;
	}

	bool
	OverflowCensus::createSlots (uint32_t frameCount) noexcept
	{
		/* Own pool: one set per (frame, channel). ⚠️ FREE_DESCRIPTOR_SET_BIT is mandatory, Vulkan::DescriptorSet frees
		 * its set individually (see src/Vulkan/AGENTS.md). */
		const uint32_t setCount = frameCount * MaxChannels;

		const std::vector< VkDescriptorPoolSize > poolSizes{
			{.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, .descriptorCount = setCount},
			{.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, .descriptorCount = setCount}
		};

		m_descriptorPool = std::make_shared< DescriptorPool >(m_device, poolSizes, setCount, VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT);
		m_descriptorPool->setIdentifier(ClassId, "Main", "DescriptorPool");

		if ( !m_descriptorPool->createOnHardware() )
		{
			TraceError{ClassId} << "Unable to create the descriptor pool !";

			return false;
		}

		const VkDeviceSize bytes = m_stride * (1 + MaxChannels);

		m_slots.clear();
		m_slots.resize(frameCount);

		for ( uint32_t frameIndex = 0; frameIndex < frameCount; ++frameIndex )
		{
			auto & slot = m_slots[frameIndex];
			const auto suffix = "-F" + std::to_string(frameIndex);

			slot.counters = std::make_unique< Buffer >(m_device, static_cast< VkBufferCreateFlags >(0), bytes, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, false);
			slot.counters->setIdentifier(ClassId, "Counters" + suffix, "Buffer");

			if ( !slot.counters->createOnHardware() )
			{
				TraceError{ClassId} << "Unable to create the counter buffer of frame " << frameIndex << " !";

				return false;
			}

			/* Persistently mapped, host-cached: read once per harvest, after the slot's fence and the copy's
			 * TRANSFER -> HOST barrier (a fence alone does not make device writes visible to the host). */
			slot.readback = std::make_unique< Buffer >(m_device, static_cast< VkBufferCreateFlags >(0), bytes, VK_BUFFER_USAGE_TRANSFER_DST_BIT, true);
			slot.readback->setHostReadable(true);
			slot.readback->setIdentifier(ClassId, "Readback" + suffix, "Buffer");

			if ( !slot.readback->createOnHardware() )
			{
				TraceError{ClassId} << "Unable to create the readback buffer of frame " << frameIndex << " !";

				return false;
			}

			slot.mappedPointer = slot.readback->mapMemoryAs< uint8_t >();

			if ( slot.mappedPointer == nullptr )
			{
				TraceError{ClassId} << "Unable to map the readback buffer of frame " << frameIndex << " !";

				return false;
			}

			for ( uint32_t channel = 0; channel < MaxChannels; ++channel )
			{
				auto descriptorSet = std::make_unique< DescriptorSet >(m_descriptorPool, m_descriptorSetLayout);
				descriptorSet->setIdentifier(ClassId, "Channel" + std::to_string(channel) + suffix, "DescriptorSet");

				if ( !descriptorSet->create() )
				{
					TraceError{ClassId} << "Unable to allocate the descriptor set of channel " << channel << ", frame " << frameIndex << " !";

					return false;
				}

				/* Written ONCE: the storage binding of set c is region c + 1, and only it. Binding 0 (the image)
				 * is written per frame, when the channel table is known. */
				VkDescriptorBufferInfo bufferInfo{};
				bufferInfo.buffer = slot.counters->handle();
				bufferInfo.offset = m_stride * (channel + 1);
				bufferInfo.range = m_stride;

				if ( !descriptorSet->writeStorageBuffer(1, bufferInfo) )
				{
					TraceError{ClassId} << "Unable to write the counter region of channel " << channel << ", frame " << frameIndex << " !";

					return false;
				}

				slot.sets[channel] = std::move(descriptorSet);
			}
		}

		return true;
	}

	bool
	OverflowCensus::createSelfTestImage (Renderer & renderer) noexcept
	{
		const auto texels = buildSelfTestTexels();

		m_selfTestImage = std::make_shared< Image >(
			m_device,
			VK_IMAGE_TYPE_2D,
			VK_FORMAT_R16G16B16A16_SFLOAT,
			VkExtent3D{SelfTestExtent, SelfTestExtent, 1},
			VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT
		);
		m_selfTestImage->setIdentifier(ClassId, "SelfTest", "Image");

		if ( !m_selfTestImage->createOnHardware() )
		{
			TraceError{ClassId} << "Unable to create the self-test image !";

			return false;
		}

		/* A buffer-to-image copy: the raw bit patterns reach the image untouched, where a clear or a draw would go
		 * through a float conversion that may canonicalize the NaN payloads. It leaves the image in
		 * SHADER_READ_ONLY_OPTIMAL. */
		if ( !m_selfTestImage->writeData(renderer.transferManager(), MemoryRegion{texels.data(), sizeof(texels)}) )
		{
			TraceError{ClassId} << "Unable to upload the self-test image !";

			return false;
		}

		/* ⚠️ The transfer manager submits on a queue of its own and returns without waiting, and nothing orders that
		 * queue with the frames (Vulkan::Device::getGraphicsQueue() rotates). Once, at creation, before any frame. */
		m_device->waitIdle("OverflowCensus::createSelfTestImage()");

		m_selfTestView = std::make_shared< ImageView >(m_selfTestImage, VK_IMAGE_VIEW_TYPE_2D, VkImageSubresourceRange{VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1});
		m_selfTestView->setIdentifier(ClassId, "SelfTest", "ImageView");

		if ( !m_selfTestView->createOnHardware() )
		{
			TraceError{ClassId} << "Unable to create the self-test image view !";

			return false;
		}

		return true;
	}

	void
	OverflowCensus::destroy () noexcept
	{
		m_available.store(false, std::memory_order_release);

		for ( auto & slot : m_slots )
		{
			if ( slot.readback != nullptr && slot.mappedPointer != nullptr )
			{
				slot.readback->unmapMemory();
			}
		}

		m_slots.clear();
		m_selfTestView.reset();
		m_selfTestImage.reset();
		m_sampler.reset();
		m_pipelineArray.reset();
		m_pipeline2D.reset();
		m_pipelineLayout.reset();
		m_descriptorPool.reset();
		m_descriptorSetLayout.reset();
		m_device.reset();
		m_notedTargets.clear();
		m_preparedSerial = 0;
		m_frameSelfTestGeneration = 0;
		m_frameArmed = false;
	}

	void
	OverflowCensus::prepareFrame (uint64_t frameSerial) noexcept
	{
		/* The second half of a cut frame: keep what the first half noted. */
		if ( frameSerial == m_preparedSerial )
		{
			return;
		}

		m_preparedSerial = frameSerial;
		m_notedTargets.clear();

		if ( !this->available() )
		{
			m_frameArmed = false;
			m_frameSelfTestGeneration = 0;

			return;
		}

		m_frameArmed = m_armed.load(std::memory_order_acquire);

		/* A self-test request rides every batch until its result is published: a batch recorded but never executed
		 * (a discarded frame) must not swallow it. */
		const auto requested = m_selfTestRequested.load(std::memory_order_acquire);

		m_frameSelfTestGeneration = requested > m_selfTestPublished.load(std::memory_order_acquire) ? requested : 0;

		m_lastPreparedSerial.store(frameSerial, std::memory_order_release);
	}

	void
	OverflowCensus::noteRadianceTargets (std::span< const IndirectPostProcessEffect::RadianceTarget > targets) noexcept
	{
		for ( const auto & target : targets )
		{
			if ( target.texture == nullptr || target.name == nullptr )
			{
				continue;
			}

			if ( m_notedTargets.full() )
			{
				if ( !m_droppedChannelReported )
				{
					TraceWarning{ClassId} << "More radiance targets than census channels: '" << target.name << "' and the next ones are NOT counted (reported once).";

					m_droppedChannelReported = true;
				}

				return;
			}

			NotedTarget noted{};
			copyName(noted.name, target.name);
			noted.texture = target.texture;

			m_notedTargets.emplace_back(noted);
		}
	}

	void
	OverflowCensus::recordBatch (const CommandBuffer & commandBuffer, uint32_t frameIndex, const TextureInterface & sceneColour, const TextureInterface & toneMapInput, bool toneMapped, const IrradianceProbeVolume * volume, GPUProfiler * profiler) noexcept
	{
		if ( !this->frameArmed() || !this->available() || frameIndex >= m_slots.size() )
		{
			return;
		}

		auto & slot = m_slots[frameIndex];

		/* A slot is harvested at every fence wait of its frame index, before it can be recorded again: still pending
		 * here, its batch was never read. */
		if ( slot.pending )
		{
			slot.pending = false;

			const std::lock_guard< std::mutex > lock{m_statisticsAccess};

			m_window.staleSlots++;
		}

		/* 1. The channel table, in the fixed order: the chain colour, the declared traces, the probe atlas, the
		 * self-test (whose room is reserved). */
		struct Source
		{
			OverflowCensusChannelName name{};
			const TextureInterface * texture{nullptr};
			const ImageView * view{nullptr};
			VkImageLayout layout{VK_IMAGE_LAYOUT_UNDEFINED};
			PushConstants constants{};
			uint32_t layers{1};
			uint32_t expectedTexels{0};
			bool array{false};
			bool selfTest{false};
		};

		StaticVector< Source, MaxChannels > sources;

		const bool selfTest = m_frameSelfTestGeneration != 0;
		const size_t capacity = MaxChannels - ( selfTest ? 1U : 0U );

		const auto addTexture = [&] (std::string_view name, const TextureInterface & texture) {
			if ( sources.size() >= capacity )
			{
				if ( !m_droppedChannelReported )
				{
					TraceWarning{ClassId} << "No census channel left for '" << name << "': it is NOT counted (reported once).";

					m_droppedChannelReported = true;
				}

				return;
			}

			const auto image = texture.image();

			if ( !texture.isCreated() || image == nullptr || texture.imageView() == nullptr || texture.sampler() == nullptr )
			{
				return;
			}

			/* A 2D float colour image only: an 8-bit target cannot overflow, and a count on it would read as clean. */
			if ( texture.type() != TextureType::Texture2D || !isFloatColourFormat(image->createInfo().format) )
			{
				if ( !m_skippedChannelReported )
				{
					TraceWarning{ClassId} << "The '" << name << "' channel is not a 2D floating-point colour image: it is NOT counted (reported once).";

					m_skippedChannelReported = true;
				}

				return;
			}

			const auto & extent = image->createInfo().extent;

			Source source{};
			copyName(source.name, name);
			source.texture = &texture;
			source.constants = {.width = extent.width, .height = extent.height, .tile = 0, .border = 0};
			source.expectedTexels = extent.width * extent.height;

			sources.emplace_back(source);
		};

		if ( m_frameArmed )
		{
			addTexture("SceneColour", sceneColour);
			addTexture("ToneMapInput", toneMapInput);

			for ( const auto & noted : m_notedTargets )
			{
				addTexture(noted.name.data(), *noted.texture);
			}

			if ( volume != nullptr && sources.size() < capacity )
			{
				if ( const auto atlas = volume->irradianceCensusView(); atlas.has_value() && atlas->view != nullptr && atlas->tile > 2 * atlas->border )
				{
					const auto interior = atlas->tile - (2 * atlas->border);

					Source source{};
					copyName(source.name, "ProbeIrradiance");
					source.view = atlas->view;
					source.layout = VK_IMAGE_LAYOUT_GENERAL;
					source.constants = {.width = atlas->width, .height = atlas->height, .tile = atlas->tile, .border = atlas->border};
					source.layers = atlas->layers;
					source.expectedTexels = (atlas->width / atlas->tile) * (atlas->height / atlas->tile) * atlas->layers * interior * interior;
					source.array = true;

					sources.emplace_back(source);
				}
			}
		}

		if ( selfTest && m_selfTestView != nullptr )
		{
			Source source{};
			copyName(source.name, "SelfTest");
			source.view = m_selfTestView.get();
			source.layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			source.constants = {.width = SelfTestExtent, .height = SelfTestExtent, .tile = 0, .border = 0};
			source.expectedTexels = SelfTestExpectedTested;
			source.selfTest = true;

			sources.emplace_back(source);
		}

		if ( sources.empty() )
		{
			return;
		}

		const GPUProfiler::ScopedZone profilingZone{profiler, commandBuffer, "OverflowCensus"};

		/* 2. Reset: the channel regions to zero, the header to the frame serial. Disjoint ranges, no barrier between. */
		commandBuffer.fill(*slot.counters, m_stride, VK_WHOLE_SIZE, 0U);
		commandBuffer.update(*slot.counters, 0, sizeof(m_preparedSerial), &m_preparedSerial);

		/* 3. Every counted image is complete and visible to the compute stage, and so is the reset:
		 *  - the intermediate targets' render passes (their outbound dependency targets FRAGMENT only), hence
		 *    COLOR_ATTACHMENT_OUTPUT for the writes and FRAGMENT to chain the final-layout transition;
		 *  - the grab-pass copy (TRANSFER), the probe atlas (COMPUTE), the reset (TRANSFER). */
		{
			const Sync::MemoryBarrier barrier{
				VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT | VK_ACCESS_SHADER_WRITE_BIT,
				VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT
			};

			commandBuffer.pipelineBarrier(
				barrier,
				VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_TRANSFER_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
				VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT
			);
		}

		/* 4. One dispatch per channel, each into its own region (its own set). */
		slot.channelCount = 0;
		slot.selfTestChannel = MaxChannels;

		const ComputePipeline * boundPipeline = nullptr;

		for ( const auto & source : sources )
		{
			const auto index = slot.channelCount;
			const auto & descriptorSet = *slot.sets[index];

			/* A chain image through its own interface (its tracked layout and sampler, the tone mapper's idiom); the
			 * atlas and the self-test in their explicit layout. */
			const bool written = source.texture != nullptr ?
				descriptorSet.writeCombinedImageSampler(0, *source.texture) :
				descriptorSet.writeCombinedImageSampler(0, *source.view, *m_sampler, source.layout);

			if ( !written )
			{
				if ( !m_skippedChannelReported )
				{
					TraceError{ClassId} << "Unable to bind the '" << source.name.data() << "' channel: it is NOT counted (reported once).";

					m_skippedChannelReported = true;
				}

				continue;
			}

			const auto & pipeline = source.array ? *m_pipelineArray : *m_pipeline2D;

			if ( boundPipeline != &pipeline )
			{
				commandBuffer.bind(pipeline);

				boundPipeline = &pipeline;
			}

			commandBuffer.bind(descriptorSet, *m_pipelineLayout, VK_PIPELINE_BIND_POINT_COMPUTE, 0);

			vkCmdPushConstants(commandBuffer.handle(), m_pipelineLayout->handle(), VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(PushConstants), &source.constants);

			commandBuffer.dispatch(
				(source.constants.width + WorkgroupSide - 1) / WorkgroupSide,
				(source.constants.height + WorkgroupSide - 1) / WorkgroupSide,
				source.layers
			);

			slot.channels[index] = {.name = source.name, .expectedTexels = source.expectedTexels};

			if ( source.selfTest )
			{
				slot.selfTestChannel = index;
			}

			slot.channelCount++;
		}

		/* 5. The counters (the atomics, and the header the reset wrote) to the copy; and the next WRITERS of every
		 * counted image wait for these reads: the intermediate targets' inbound dependency (src FRAGMENT), the grab
		 * pre-barrier (src COLOR_ATTACHMENT_OUTPUT | FRAGMENT, then TRANSFER), the next probe update (src FRAGMENT |
		 * COMPUTE). */
		{
			const Sync::MemoryBarrier barrier{VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT};

			commandBuffer.pipelineBarrier(
				barrier,
				VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_TRANSFER_BIT,
				VK_PIPELINE_STAGE_TRANSFER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
			);
		}

		/* 6. The header and the regions used, to the host. ⚠️ A fence wait alone does not make device writes visible to
		 * the host: the TRANSFER -> HOST barrier does. */
		commandBuffer.copy(*slot.counters, *slot.readback, 0, 0, m_stride * (1 + slot.channelCount));

		commandBuffer.pipelineBarrier(
			Sync::BufferMemoryBarrier{*slot.readback, VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_HOST_READ_BIT},
			VK_PIPELINE_STAGE_TRANSFER_BIT,
			VK_PIPELINE_STAGE_HOST_BIT
		);

		slot.expectedSerial = m_preparedSerial;
		slot.selfTestGeneration = slot.selfTestChannel < MaxChannels ? m_frameSelfTestGeneration : 0;
		slot.toneMapped = toneMapped;
		slot.countsFrame = m_frameArmed;
		slot.pending = slot.channelCount > 0;
	}

	void
	OverflowCensus::harvest (uint32_t frameIndex) noexcept
	{
		if ( frameIndex >= m_slots.size() )
		{
			return;
		}

		auto & slot = m_slots[frameIndex];

		if ( !slot.pending || slot.mappedPointer == nullptr )
		{
			return;
		}

		slot.pending = false;

		uint64_t header = 0;
		std::memcpy(&header, slot.mappedPointer, sizeof(header));

		/* The batch was recorded but never executed (a frame discarded after its recording): the buffer holds an older
		 * frame's counts. Dropped, never counted as this frame. */
		if ( header != slot.expectedSerial )
		{
			const std::lock_guard< std::mutex > lock{m_statisticsAccess};

			m_window.staleSlots++;

			return;
		}

		OverflowCensusReport report{};
		report.frameSerial = slot.expectedSerial;
		report.toneMapped = slot.toneMapped;
		report.valid = true;

		std::optional< OverflowCensusSelfTest > selfTest;
		const OverflowCensusChannel * coverageDefect = nullptr;

		for ( uint32_t index = 0; index < slot.channelCount; ++index )
		{
			std::array< uint32_t, CounterCount > counters{};
			std::memcpy(counters.data(), slot.mappedPointer + (m_stride * (index + 1)), sizeof(counters));

			OverflowCensusChannel channel{};
			channel.name = slot.channels[index].name;
			channel.expectedTexels = slot.channels[index].expectedTexels;
			channel.tested = counters[CounterTested];
			channel.nanTexels = counters[CounterNaN];
			channel.infTexels = counters[CounterInf];
			channel.ceilingTexels = counters[CounterCeiling];
			channel.peakFinite = std::bit_cast< float >(counters[CounterPeakBits]);
			channel.invalid = channel.tested != channel.expectedTexels;

			if ( index == slot.selfTestChannel )
			{
				selfTest = OverflowCensusSelfTest{
					.measured = channel,
					.frameSerial = slot.expectedSerial,
					.ran = true,
					.passed =
						!channel.invalid &&
						channel.tested == SelfTestExpectedTested &&
						channel.nanTexels == SelfTestExpectedNaN &&
						channel.infTexels == SelfTestExpectedInf &&
						channel.ceilingTexels == SelfTestExpectedCeiling &&
						counters[CounterPeakBits] == SelfTestExpectedPeakBits
				};

				continue;
			}

			report.channels[report.channelCount] = channel;

			if ( channel.invalid && coverageDefect == nullptr )
			{
				coverageDefect = &report.channels[report.channelCount];
			}

			report.channelCount++;
		}

		{
			const std::lock_guard< std::mutex > lock{m_statisticsAccess};

			/* A batch recorded for the self-test alone (census disarmed) is not a counted frame. */
			if ( slot.countsFrame )
			{
				m_latest = report;

				if ( report.frameSerial > m_window.startsAfterFrame )
				{
					this->accumulateWindow(report);
				}
			}

			if ( selfTest.has_value() )
			{
				m_selfTest = selfTest.value();

				if ( slot.selfTestGeneration > m_selfTestPublished.load(std::memory_order_relaxed) )
				{
					m_selfTestPublished.store(slot.selfTestGeneration, std::memory_order_release);
				}
			}
		}

		if ( selfTest.has_value() )
		{
			m_selfTestCompletion.notify_all();

			const auto & measured = selfTest->measured;

			if ( selfTest->passed )
			{
				TraceSuccess{ClassId} << "Self-test PASSED on frame " << selfTest->frameSerial << ": tested " << measured.tested << ", NaN " << measured.nanTexels << ", Inf " << measured.infTexels << ", ceiling " << measured.ceilingTexels << ", peak " << measured.peakFinite << ".";
			}
			else
			{
				TraceError{ClassId} <<
					"Self-test FAILED on frame " << selfTest->frameSerial << ": expected tested " << SelfTestExpectedTested << ", NaN " << SelfTestExpectedNaN << ", Inf " << SelfTestExpectedInf << ", ceiling " << SelfTestExpectedCeiling << ", peak 65472; "
					"measured tested " << measured.tested << ", NaN " << measured.nanTexels << ", Inf " << measured.infTexels << ", ceiling " << measured.ceilingTexels << ", peak " << measured.peakFinite << ". "
					"The census is BLIND on this machine: no count it reports here can be trusted.";
			}
		}

		/* tested != expected is a census defect (a mask or a dispatch extent), never a property of the scene. */
		if ( slot.countsFrame )
		{
			if ( coverageDefect != nullptr )
			{
				if ( !m_coverageDefectReported )
				{
					TraceError{ClassId} << "Channel '" << coverageDefect->name.data() << "' tested " << coverageDefect->tested << " texels where " << coverageDefect->expectedTexels << " were expected: its counts cannot be trusted (reported until a clean frame).";

					m_coverageDefectReported = true;
				}
			}
			else
			{
				m_coverageDefectReported = false;
			}
		}
	}

	void
	OverflowCensus::accumulateWindow (const OverflowCensusReport & report) noexcept
	{
		auto & window = m_window;

		if ( window.framesCounted == 0 )
		{
			window.firstFrame = report.frameSerial;
		}

		window.lastFrame = report.frameSerial;
		window.framesCounted++;

		bool frameOverflows = false;

		for ( uint32_t index = 0; index < report.channelCount; ++index )
		{
			const auto & channel = report.channels[index];

			/* An untrustworthy channel stays out of the statistics. */
			if ( channel.invalid )
			{
				continue;
			}

			OverflowCensusWindowChannel * entry = nullptr;

			for ( uint32_t slotIndex = 0; slotIndex < window.channelCount; ++slotIndex )
			{
				if ( window.channels[slotIndex].name == channel.name )
				{
					entry = &window.channels[slotIndex];

					break;
				}
			}

			if ( entry == nullptr )
			{
				if ( window.channelCount >= window.channels.size() )
				{
					continue;
				}

				entry = &window.channels[window.channelCount++];
				entry->name = channel.name;
			}

			const auto overflow = channel.nanTexels + channel.infTexels + channel.ceilingTexels;

			entry->framesCounted++;

			if ( overflow > 0 )
			{
				entry->framesOverflowing++;

				frameOverflows = true;
			}

			if ( overflow > entry->maxOverflow )
			{
				entry->maxOverflow = overflow;
				entry->maxOverflowFrame = report.frameSerial;
			}

			entry->maxPeakFinite = std::max(entry->maxPeakFinite, channel.peakFinite);
		}

		if ( frameOverflows )
		{
			window.framesWithOverflow++;
		}
	}

	uint64_t
	OverflowCensus::resetWindow () noexcept
	{
		const auto startsAfter = m_lastPreparedSerial.load(std::memory_order_acquire);

		const std::lock_guard< std::mutex > lock{m_statisticsAccess};

		m_window = {};
		m_window.startsAfterFrame = startsAfter;

		return startsAfter;
	}

	OverflowCensusSnapshot
	OverflowCensus::snapshot () const noexcept
	{
		OverflowCensusSnapshot snapshot{};
		snapshot.available = this->available();
		snapshot.armed = this->armed();

		const std::lock_guard< std::mutex > lock{m_statisticsAccess};

		snapshot.latest = m_latest;
		snapshot.window = m_window;
		snapshot.selfTest = m_selfTest;

		return snapshot;
	}

	OverflowCensusSelfTest
	OverflowCensus::runSelfTest (std::chrono::milliseconds timeout) noexcept
	{
		if ( !this->available() )
		{
			return {};
		}

		const auto generation = m_selfTestRequestCounter.fetch_add(1, std::memory_order_acq_rel) + 1;

		m_selfTestRequested.store(generation, std::memory_order_release);

		std::unique_lock< std::mutex > lock{m_statisticsAccess};

		const auto answered = m_selfTestCompletion.wait_for(lock, timeout, [this, generation] {
			return m_selfTestPublished.load(std::memory_order_acquire) >= generation;
		});

		if ( !answered )
		{
			/* Withdrawn: no frame counted it in time. A batch that did record it still publishes, harmlessly. */
			auto expected = generation;

			m_selfTestRequested.compare_exchange_strong(expected, 0, std::memory_order_acq_rel);

			return {};
		}

		return m_selfTest;
	}
}
