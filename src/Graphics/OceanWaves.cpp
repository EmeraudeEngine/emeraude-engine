/*
 * src/Graphics/OceanWaves.cpp
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

#include "OceanWaves.hpp"

/* STL inclusions. */
#include <algorithm>
#include <bit>
#include <cmath>
#include <cstring>
#include <numbers>
#include <sstream>
#include <string>

/* Local inclusions. */
#include "Saphir/ShaderManager.hpp"
#include "Tracer.hpp"
#include "Vulkan/Buffer.hpp"
#include "Vulkan/CommandBuffer.hpp"
#include "Vulkan/CommandPool.hpp"
#include "Vulkan/ComputePipeline.hpp"
#include "Vulkan/DescriptorPool.hpp"
#include "Vulkan/DescriptorSet.hpp"
#include "Vulkan/DescriptorSetLayout.hpp"
#include "Vulkan/Device.hpp"
#include "Vulkan/Image.hpp"
#include "Vulkan/ImageView.hpp"
#include "Vulkan/PipelineLayout.hpp"
#include "Vulkan/Queue.hpp"
#include "Vulkan/ShaderModule.hpp"

namespace EmEn::Graphics
{
	using namespace Base;

	namespace
	{
		/** @brief The push constants every pass reads (64 bytes). */
		struct PushConstants
		{
			std::array< float, 4 > cascadeSizes{};
			std::array< float, 4 > whitecapCascades{};
			float time{0.0F};
			float gravity{9.81F};
			float choppiness{1.0F};
			int32_t vertical{0};
			float deltaTime{0.0F};
			float whitecapThreshold{0.78F};
			float whitecapSharpness{10.0F};
			float whitecapLifetime{2.5F};
		};

		static_assert(sizeof(PushConstants) == 64);

		/** @brief One cascade layer's extent, and the range of every cascade layer. */
		constexpr VkExtent3D CascadeExtent{OceanWaves::Resolution, OceanWaves::Resolution, 1};
		constexpr VkImageSubresourceRange AllCascades{VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, OceanWaves::CascadeCount};

		/* The block every shader declares (same layout as PushConstants). */
		constexpr auto PushConstantBlock = R"GLSL(
layout(push_constant) uniform Ocean
{
	vec4 cascadeSizes;
	vec4 whitecapCascades;
	float time;
	float gravity;
	float choppiness;
	int vertical;
	float deltaTime;
	float whitecapThreshold;
	float whitecapSharpness;
	float whitecapLifetime;
} pc;
)GLSL";

		/* The eight real fields of Tessendorf's choppy surface, packed two by two (A + iB) into two RGBA32F images:
		 * field0 = (Dx + i h, Dz + i ∂h/∂x), field1 = (∂h/∂z + i ∂Dx/∂x, ∂Dz/∂z + i ∂Dx/∂z). */
		constexpr auto EvolveShaderBody = R"GLSL(
layout(local_size_x = 16, local_size_y = 16, local_size_z = 1) in;

layout(set = 0, binding = 0, rg32f) uniform readonly image2DArray spectrum;
layout(set = 0, binding = 1, rgba32f) uniform writeonly image2DArray field0;
layout(set = 0, binding = 2, rgba32f) uniform writeonly image2DArray field1;

vec2 cmul (vec2 a, vec2 b) { return vec2(a.x * b.x - a.y * b.y, a.x * b.y + a.y * b.x); }
vec2 cconj (vec2 a) { return vec2(a.x, -a.y); }
/* A + iB for two complex spectra A and B of real signals. */
vec2 cpair (vec2 a, vec2 b) { return vec2(a.x - b.y, a.y + b.x); }

void main ()
{
	const ivec3 id = ivec3(gl_GlobalInvocationID.xyz);

	if ( id.x >= N || id.y >= N )
	{
		return;
	}

	/* The FFT's native order: index n is the wave number n below N/2, n - N above. */
	const int n = id.x < N / 2 ? id.x : id.x - N;
	const int m = id.y < N / 2 ? id.y : id.y - N;
	const vec2 k = vec2(float(n), float(m)) * (6.28318530717958647692 / pc.cascadeSizes[id.z]);
	const float kl = length(k);

	/* h(k, t) = h0(k) e^(iwt) + conj(h0(-k)) e^(-iwt), deep water w = sqrt(g k) (Tessendorf, eq. 43). */
	const vec2 h0 = imageLoad(spectrum, id).xy;
	const vec2 h0Mirror = cconj(imageLoad(spectrum, ivec3((N - id.x) % N, (N - id.y) % N, id.z)).xy);
	const float phase = sqrt(pc.gravity * kl) * pc.time;
	const vec2 e = vec2(cos(phase), sin(phase));
	const vec2 h = cmul(h0, e) + cmul(h0Mirror, cconj(e));

	const vec2 hx = cmul(h, vec2(0.0, k.x));
	const vec2 hz = cmul(h, vec2(0.0, k.y));
	vec2 dx = vec2(0.0);
	vec2 dz = vec2(0.0);
	vec2 dxx = vec2(0.0);
	vec2 dzz = vec2(0.0);
	vec2 dxz = vec2(0.0);

	if ( kl > 1.0e-6 )
	{
		/* D = -i k / |k| h (Tessendorf, eq. 44), and its derivatives i k D. */
		dx = cmul(h, vec2(0.0, -k.x / kl));
		dz = cmul(h, vec2(0.0, -k.y / kl));
		dxx = h * (k.x * k.x / kl);
		dzz = h * (k.y * k.y / kl);
		dxz = h * (k.x * k.y / kl);
	}

	imageStore(field0, id, vec4(cpair(dx, h), cpair(dz, hx)));
	imageStore(field1, id, vec4(cpair(hz, dxx), cpair(dzz, dxz)));
}
)GLSL";

		/* One inverse FFT of N points per workgroup (a row, or a column when pc.vertical != 0), in place, entirely in
		 * shared memory: radix-2 Stockham auto-sort, N / 2 butterflies per stage, one per thread, both images at once.
		 * The inverse transform takes e^(+i...) and no 1 / N: the spectrum amplitudes are Tessendorf's, summed. */
		constexpr auto FFTShaderBody = R"GLSL(
layout(local_size_x = N / 2, local_size_y = 1, local_size_z = 1) in;

layout(set = 0, binding = 1, rgba32f) uniform image2DArray field0;
layout(set = 0, binding = 2, rgba32f) uniform image2DArray field1;

shared vec4 pingA[N];
shared vec4 pingB[N];
shared vec4 pongA[N];
shared vec4 pongB[N];

vec4 cmul2 (vec4 a, vec2 w) { return vec4(a.x * w.x - a.y * w.y, a.x * w.y + a.y * w.x, a.z * w.x - a.w * w.y, a.z * w.y + a.w * w.x); }

ivec3 texel (uint index)
{
	return pc.vertical != 0 ? ivec3(gl_WorkGroupID.x, index, gl_WorkGroupID.z) : ivec3(index, gl_WorkGroupID.x, gl_WorkGroupID.z);
}

void main ()
{
	const uint j = gl_LocalInvocationID.x;

	pingA[j] = imageLoad(field0, texel(j));
	pingB[j] = imageLoad(field1, texel(j));
	pingA[j + N / 2] = imageLoad(field0, texel(j + N / 2));
	pingB[j + N / 2] = imageLoad(field1, texel(j + N / 2));

	barrier();

	bool fromPing = true;

	for ( uint span = 1u; span < uint(N); span <<= 1u )
	{
		const uint k = j & (span - 1u);
		const float angle = 3.14159265358979323846 * float(k) / float(span);
		const vec2 w = vec2(cos(angle), sin(angle));
		const uint destination = (j / span) * 2u * span + k;

		vec4 a0;
		vec4 b0;
		vec4 a1;
		vec4 b1;

		if ( fromPing )
		{
			a0 = pingA[j];
			b0 = cmul2(pingA[j + N / 2], w);
			a1 = pingB[j];
			b1 = cmul2(pingB[j + N / 2], w);
			pongA[destination] = a0 + b0;
			pongA[destination + span] = a0 - b0;
			pongB[destination] = a1 + b1;
			pongB[destination + span] = a1 - b1;
		}
		else
		{
			a0 = pongA[j];
			b0 = cmul2(pongA[j + N / 2], w);
			a1 = pongB[j];
			b1 = cmul2(pongB[j + N / 2], w);
			pingA[destination] = a0 + b0;
			pingA[destination + span] = a0 - b0;
			pingB[destination] = a1 + b1;
			pingB[destination + span] = a1 - b1;
		}

		fromPing = !fromPing;

		barrier();
	}

	/* log2(N) is even for N = 256: the result is back in the ping buffers. */
	imageStore(field0, texel(j), fromPing ? pingA[j] : pongA[j]);
	imageStore(field1, texel(j), fromPing ? pingB[j] : pongB[j]);
	imageStore(field0, texel(j + N / 2), fromPing ? pingA[j + N / 2] : pongA[j + N / 2]);
	imageStore(field1, texel(j + N / 2), fromPing ? pingB[j + N / 2] : pongB[j + N / 2]);
}
)GLSL";

		/* The real fields out of the packed transforms, the choppiness applied; then the whitecaps, accumulated.
		 * FOAM_FORMAT is r16f, or rgba16f where the device cannot write R16F (defined by create()). */
		constexpr auto ResolveShaderBody = R"GLSL(
layout(local_size_x = 16, local_size_y = 16, local_size_z = 1) in;

layout(set = 0, binding = 1, rgba32f) uniform readonly image2DArray field0;
layout(set = 0, binding = 2, rgba32f) uniform readonly image2DArray field1;
layout(set = 0, binding = 3, rgba16f) uniform writeonly image2DArray displacement;
layout(set = 0, binding = 4, rgba16f) uniform writeonly image2DArray slopes;
layout(set = 0, binding = 5, FOAM_FORMAT) uniform image2DArray foam;

void main ()
{
	const ivec3 id = ivec3(gl_GlobalInvocationID.xyz);

	if ( id.x >= N || id.y >= N )
	{
		return;
	}

	const vec4 f0 = imageLoad(field0, id);
	const vec4 f1 = imageLoad(field1, id);

	const vec4 displaced = vec4(pc.choppiness * f0.x, f0.y, pc.choppiness * f0.z, pc.choppiness * f1.w);
	const vec4 slope = vec4(f0.w, f1.x, pc.choppiness * f1.y, pc.choppiness * f1.z);

	imageStore(displacement, id, displaced);
	imageStore(slopes, id, slope);

	/* The Jacobian of the horizontal displacement (Tessendorf § 4.4.3): below 1 the surface is compressed, below 0 the
	 * crest folds over itself. Foam is born under the threshold and decays exponentially — max(), so a crest that keeps
	 * breaking holds its foam at the birth value and the frame rate never changes the look. */
	const float jacobian = (1.0 + slope.z) * (1.0 + slope.w) - displaced.w * displaced.w;
	const float birth = clamp((pc.whitecapThreshold - jacobian) * pc.whitecapSharpness, 0.0, 1.0) * pc.whitecapCascades[id.z];
	const float left = imageLoad(foam, id).r * exp(-pc.deltaTime / max(pc.whitecapLifetime, 0.001));

	imageStore(foam, id, vec4(max(left, birth), 0.0, 0.0, 0.0));
}
)GLSL";

		/** @brief PCG hash (M. O'Neill), the deterministic source of the spectrum's draws. */
		constexpr uint32_t
		pcgHash (uint32_t value) noexcept
		{
			const uint32_t state = (value * 747796405U) + 2891336453U;
			const uint32_t word = ((state >> ((state >> 28U) + 4U)) ^ state) * 277803737U;

			return (word >> 22U) ^ word;
		}

		/** @brief Two independent standard Gaussians for one spectrum texel (Box-Muller). */
		std::complex< double >
		gaussianPair (uint32_t seed, uint32_t cascade, uint32_t index) noexcept
		{
			const auto key = pcgHash(seed ^ pcgHash((cascade * 0x9E3779B9U) ^ pcgHash(index)));
			const auto first = (static_cast< double >(pcgHash(key)) + 1.0) / 4294967297.0;
			const auto second = static_cast< double >(pcgHash(key ^ 0x68E31DA4U)) / 4294967296.0;
			const auto radius = std::sqrt(-2.0 * std::log(first));
			const auto angle = 2.0 * std::numbers::pi * second;

			return {radius * std::cos(angle), radius * std::sin(angle)};
		}

		/** @brief JONSWAP frequency spectrum S(ω) (Hasselmann et al. 1973, as Horvath 2015 § 5.1.3). */
		double
		jonswap (double omega, double windSpeed, double fetch, double gravity) noexcept
		{
			const auto alpha = 0.076 * std::pow((windSpeed * windSpeed) / (fetch * gravity), 0.22);
			const auto peak = 22.0 * std::cbrt((gravity * gravity) / (windSpeed * fetch));
			const auto sigma = omega <= peak ? 0.07 : 0.09;
			const auto r = std::exp(-((omega - peak) * (omega - peak)) / (2.0 * sigma * sigma * peak * peak));

			return ((alpha * gravity * gravity) / std::pow(omega, 5.0)) * std::exp(-1.25 * std::pow(peak / omega, 4.0)) * std::pow(3.3, r);
		}

		/** @brief Donelan-Banner directional spreading D(ω, θ) (Horvath 2015 § 5.2.4), normalised over θ in [-π, π]. */
		double
		donelanBanner (double omega, double theta, double windSpeed, double fetch, double gravity) noexcept
		{
			const auto peak = 22.0 * std::cbrt((gravity * gravity) / (windSpeed * fetch));
			const auto ratio = omega / peak;
			double beta = 0.0;

			if ( ratio < 0.95 )
			{
				beta = 2.61 * std::pow(ratio, 1.3);
			}
			else if ( ratio < 1.6 )
			{
				beta = 2.28 * std::pow(ratio, -1.3);
			}
			else
			{
				beta = std::pow(10.0, -0.4 + (0.8393 * std::exp(-0.567 * std::log(ratio * ratio))));
			}

			const auto sech = 1.0 / std::cosh(beta * theta);

			return (beta / (2.0 * std::tanh(beta * std::numbers::pi))) * sech * sech;
		}

		/** @brief In-place radix-2 Cooley-Tukey inverse DFT (e^(+i...), no 1 / N), double precision: the CPU reference. */
		void
		inverseFFT (std::vector< std::complex< double > > & data) noexcept
		{
			const auto count = data.size();

			for ( size_t index = 1, reversed = 0; index < count; ++index )
			{
				size_t bit = count >> 1U;

				for ( ; (reversed & bit) != 0; bit >>= 1U )
				{
					reversed ^= bit;
				}

				reversed ^= bit;

				if ( index < reversed )
				{
					std::swap(data[index], data[reversed]);
				}
			}

			for ( size_t length = 2; length <= count; length <<= 1U )
			{
				const auto angle = 2.0 * std::numbers::pi / static_cast< double >(length);
				const std::complex< double > step{std::cos(angle), std::sin(angle)};

				for ( size_t start = 0; start < count; start += length )
				{
					std::complex< double > twiddle{1.0, 0.0};

					for ( size_t offset = 0; offset < length / 2; ++offset )
					{
						const auto even = data[start + offset];
						const auto odd = data[start + offset + (length / 2)] * twiddle;

						data[start + offset] = even + odd;
						data[start + offset + (length / 2)] = even - odd;
						twiddle *= step;
					}
				}
			}
		}

		/** @brief IEEE half to float. */
		float
		halfToFloat (uint16_t half) noexcept
		{
			const uint32_t sign = static_cast< uint32_t >(half & 0x8000U) << 16U;
			uint32_t exponent = (half >> 10U) & 0x1FU;
			uint32_t mantissa = half & 0x3FFU;
			uint32_t bits = 0;

			if ( exponent == 0 )
			{
				if ( mantissa != 0 )
				{
					/* Subnormal: normalise. */
					exponent = 127 - 15 + 1;

					while ( (mantissa & 0x400U) == 0 )
					{
						mantissa <<= 1U;
						--exponent;
					}

					mantissa &= 0x3FFU;
					bits = sign | (exponent << 23U) | (mantissa << 13U);
				}
				else
				{
					bits = sign;
				}
			}
			else if ( exponent == 0x1FU )
			{
				bits = sign | 0x7F800000U | (mantissa << 13U);
			}
			else
			{
				bits = sign | ((exponent + 127 - 15) << 23U) | (mantissa << 13U);
			}

			return std::bit_cast< float >(bits);
		}
	}

	OceanWaves::OceanWaves (std::shared_ptr< Vulkan::Device > device, Saphir::ShaderManager & shaderManager) noexcept
		: m_device{std::move(device)},
		m_shaderManager{&shaderManager}
	{

	}

	OceanWaves::~OceanWaves ()
	{
		this->destroy();
	}

	void
	OceanWaves::buildInitialSpectrum (const OceanWaveParameters & parameters, uint32_t cascade, std::vector< std::complex< float > > & spectrum) noexcept
	{
		constexpr auto N = static_cast< int32_t >(Resolution);

		spectrum.assign(static_cast< size_t >(N) * N, {0.0F, 0.0F});

		if ( cascade >= CascadeCount )
		{
			return;
		}

		const auto size = static_cast< double >(parameters.cascadeSizes[cascade]);
		const auto deltaK = 2.0 * std::numbers::pi / size;
		const auto gravity = static_cast< double >(parameters.gravity);
		const auto windSpeed = std::max(static_cast< double >(parameters.windSpeed), 0.1);
		const auto fetch = std::max(static_cast< double >(parameters.fetch), 1.0);

		/* Each cascade holds its own band of wave numbers, so no wave is counted twice: from the previous cascade's
		 * upper bound to 6 fundamentals of the next (smaller) tile; the last one up to its Nyquist. */
		const auto bandTop = [&parameters] (uint32_t index) {
			return index + 1 < CascadeCount ? 6.0 * 2.0 * std::numbers::pi / static_cast< double >(parameters.cascadeSizes[index + 1]) : 1.0e30;
		};
		const auto lowest = cascade == 0 ? 0.0 : bandTop(cascade - 1);
		const auto highest = bandTop(cascade);

		for ( int32_t z = 0; z < N; ++z )
		{
			for ( int32_t x = 0; x < N; ++x )
			{
				const auto n = x < N / 2 ? x : x - N;
				const auto m = z < N / 2 ? z : z - N;
				const auto kx = static_cast< double >(n) * deltaK;
				const auto kz = static_cast< double >(m) * deltaK;
				const auto k = std::sqrt((kx * kx) + (kz * kz));

				/* ⚠️ The Nyquist row and column (index N/2) are their own mirror in the FFT: an ODD operator (i kx, kx / |k|)
				 * breaks the Hermitian symmetry there, the field is no longer real, and its imaginary part leaks into its
				 * partner of the two-by-two packing (measured: Dz, dDx/dx, dDz/dz 1-3 % off before this cut). */
				if ( n == -N / 2 || m == -N / 2 || k <= 0.0 || k < lowest || k >= highest )
				{
					continue;
				}

				/* S(k) = S(ω) D(ω, θ) dω/dk / k in the wave-number plane, deep water ω = sqrt(g k). */
				const auto omega = std::sqrt(gravity * k);
				const auto dOmegaDk = gravity / (2.0 * omega);
				auto theta = std::atan2(kz, kx) - static_cast< double >(parameters.windDirection);
				theta = std::remainder(theta, 2.0 * std::numbers::pi);

				const auto density = jonswap(omega, windSpeed, fetch, gravity) * donelanBanner(omega, theta, windSpeed, fetch, gravity) * dOmegaDk / k;
				/* E|h(k, t)|² = S(k) Δk²: h(k, t) sums h₀(k) and conj(h₀(−k)), two independent draws, and a complex draw of
				 * unit Gaussians already carries E|ξ|² = 2 — hence the quarter. Half of it doubled the variance (a Hs of 3.1 m
				 * for 10 m/s over 100 km). */
				const auto amplitude = std::sqrt(density * deltaK * deltaK * 0.25);
				const auto draw = gaussianPair(parameters.seed, cascade, static_cast< uint32_t >((z * N) + x));

				spectrum[(static_cast< size_t >(z) * static_cast< size_t >(N)) + static_cast< size_t >(x)] = {static_cast< float >(draw.real() * amplitude), static_cast< float >(draw.imag() * amplitude)};
			}
		}
	}

	bool
	OceanWaves::create (const OceanWaveParameters & parameters) noexcept
	{
		this->destroy();

		m_parameters = parameters;

		/* The foam is one channel: R16F where the device can write it from a compute shader and filter it (an extended
		 * storage format), RGBA16F otherwise (core for both) — MoltenVK included either way. */
		VkFormat foamFormat = VK_FORMAT_R16G16B16A16_SFLOAT;
		{
			const auto properties = m_device->physicalDevice()->getFormatProperties(VK_FORMAT_R16_SFLOAT);
			constexpr VkFormatFeatureFlags Needs = VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT | VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT;

			if ( (properties.optimalTilingFeatures & Needs) == Needs && m_device->physicalDevice()->featuresVK10().shaderStorageImageExtendedFormats == VK_TRUE )
			{
				foamFormat = VK_FORMAT_R16_SFLOAT;
			}
		}

		const auto createImage = [this] (std::shared_ptr< Vulkan::Image > & image, std::shared_ptr< Vulkan::ImageView > & view, VkFormat format, VkImageUsageFlags usage, const char * label) {
			image = std::make_shared< Vulkan::Image >(m_device, VK_IMAGE_TYPE_2D, format, CascadeExtent, usage, 0, 1, CascadeCount);
			image->setIdentifier(ClassId, label, "Image");

			if ( !image->createOnHardware() )
			{
				return false;
			}

			view = std::make_shared< Vulkan::ImageView >(image, VK_IMAGE_VIEW_TYPE_2D_ARRAY, AllCascades);
			view->setIdentifier(ClassId, label, "ImageView");

			return view->createOnHardware();
		};

		if (
			!createImage(m_spectrumImage, m_spectrumView, VK_FORMAT_R32G32_SFLOAT, VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, "Spectrum") ||
			!createImage(m_fieldImages[0], m_fieldViews[0], VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_USAGE_STORAGE_BIT, "Field0") ||
			!createImage(m_fieldImages[1], m_fieldViews[1], VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_USAGE_STORAGE_BIT, "Field1") ||
			!createImage(m_displacementImage, m_displacementView, VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT, "Displacement") ||
			!createImage(m_slopeImage, m_slopeView, VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT, "Slopes") ||
			!createImage(m_foamImage, m_foamView, foamFormat, VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, "Foam") ||
			!createImage(m_previousDisplacementImage, m_previousDisplacementView, VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, "PreviousDisplacement")
		)
		{
			Tracer::error(ClassId, "Unable to create the ocean images !");

			this->destroy();

			return false;
		}

		/* One set for every pass: each shader declares the bindings it uses. */
		m_descriptorSetLayout = std::make_shared< Vulkan::DescriptorSetLayout >(m_device, "OceanWavesDSLayout");

		for ( uint32_t binding = 0; binding < 6; ++binding )
		{
			m_descriptorSetLayout->declare(VkDescriptorSetLayoutBinding{binding, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1, VK_SHADER_STAGE_COMPUTE_BIT, nullptr});
		}

		if ( !m_descriptorSetLayout->createOnHardware() )
		{
			Tracer::error(ClassId, "Unable to create the descriptor set layout !");

			this->destroy();

			return false;
		}

		m_pipelineLayout = std::make_shared< Vulkan::PipelineLayout >(
			m_device, "OceanWavesPipelineLayout",
			StaticVector< std::shared_ptr< Vulkan::DescriptorSetLayout >, 6 >{m_descriptorSetLayout},
			StaticVector< VkPushConstantRange, 4 >{VkPushConstantRange{VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(PushConstants)}}
		);

		if ( !m_pipelineLayout->createOnHardware() )
		{
			Tracer::error(ClassId, "Unable to create the pipeline layout !");

			this->destroy();

			return false;
		}

		const std::string header = std::string{"#version 450\n#define N "} + std::to_string(Resolution) + "\n#define FOAM_FORMAT " + (foamFormat == VK_FORMAT_R16_SFLOAT ? "r16f" : "rgba16f") + "\n" + PushConstantBlock;

		const auto createPipeline = [this, &header] (std::unique_ptr< Vulkan::ComputePipeline > & pipeline, const char * name, const char * body) {
			const auto shaderModule = m_shaderManager->getShaderModuleFromSourceCode(m_device, name, Saphir::ShaderType::ComputeShader, header + body);

			if ( shaderModule == nullptr )
			{
				TraceError{ClassId} << "Unable to compile the compute shader '" << name << "' !";

				return false;
			}

			pipeline = std::make_unique< Vulkan::ComputePipeline >(m_pipelineLayout);
			pipeline->setShaderModule(shaderModule->handle());

			return pipeline->createOnHardware();
		};

		if (
			!createPipeline(m_evolvePipeline, "OceanWavesEvolve_CS", EvolveShaderBody) ||
			!createPipeline(m_fftPipeline, "OceanWavesFFT_CS", FFTShaderBody) ||
			!createPipeline(m_resolvePipeline, "OceanWavesResolve_CS", ResolveShaderBody)
		)
		{
			Tracer::error(ClassId, "Unable to create the ocean compute pipelines !");

			this->destroy();

			return false;
		}

		/* NOTE: FREE_DESCRIPTOR_SET_BIT is mandatory, Vulkan::DescriptorSet frees its set individually. */
		m_descriptorPool = std::make_shared< Vulkan::DescriptorPool >(m_device, std::vector< VkDescriptorPoolSize >{{VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 6}}, 1, VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT);

		if ( !m_descriptorPool->createOnHardware() )
		{
			Tracer::error(ClassId, "Unable to create the descriptor pool !");

			this->destroy();

			return false;
		}

		m_descriptorSet = std::make_unique< Vulkan::DescriptorSet >(m_descriptorPool, m_descriptorSetLayout);

		if (
			!m_descriptorSet->create() ||
			!m_descriptorSet->writeStorageImage(0, *m_spectrumView) ||
			!m_descriptorSet->writeStorageImage(1, *m_fieldViews[0]) ||
			!m_descriptorSet->writeStorageImage(2, *m_fieldViews[1]) ||
			!m_descriptorSet->writeStorageImage(3, *m_displacementView) ||
			!m_descriptorSet->writeStorageImage(4, *m_slopeView) ||
			!m_descriptorSet->writeStorageImage(5, *m_foamView)
		)
		{
			Tracer::error(ClassId, "Unable to create the descriptor set !");

			this->destroy();

			return false;
		}

		if ( !this->uploadSpectrum() )
		{
			Tracer::error(ClassId, "Unable to upload the initial spectrum !");

			this->destroy();

			return false;
		}

		return true;
	}

	void
	OceanWaves::destroy () noexcept
	{
		m_descriptorSet.reset();
		m_descriptorPool = nullptr;
		m_resolvePipeline.reset();
		m_fftPipeline.reset();
		m_evolvePipeline.reset();
		m_pipelineLayout.reset();
		m_descriptorSetLayout.reset();
		m_previousDisplacementView.reset();
		m_foamView.reset();
		m_slopeView.reset();
		m_displacementView.reset();
		m_fieldViews[1].reset();
		m_fieldViews[0].reset();
		m_spectrumView.reset();
		m_previousDisplacementImage.reset();
		m_foamImage.reset();
		m_slopeImage.reset();
		m_displacementImage.reset();
		m_fieldImages[1].reset();
		m_fieldImages[0].reset();
		m_spectrumImage.reset();
		m_previousDisplacementValid = false;
	}

	bool
	OceanWaves::uploadSpectrum () noexcept
	{
		constexpr size_t TexelsPerLayer = static_cast< size_t >(Resolution) * Resolution;

		std::vector< float > data(TexelsPerLayer * CascadeCount * 2);
		std::vector< std::complex< float > > spectrum;

		for ( uint32_t cascade = 0; cascade < CascadeCount; ++cascade )
		{
			buildInitialSpectrum(m_parameters, cascade, spectrum);

			std::memcpy(data.data() + (TexelsPerLayer * cascade * 2), spectrum.data(), TexelsPerLayer * sizeof(std::complex< float >));
		}

		auto staging = std::make_unique< Vulkan::Buffer >(m_device, static_cast< VkBufferCreateFlags >(0), data.size() * sizeof(float), VK_BUFFER_USAGE_TRANSFER_SRC_BIT, true);

		if ( !staging->createOnHardware() || !staging->writeData(data) )
		{
			return false;
		}

		const auto commandPool = std::make_shared< Vulkan::CommandPool >(m_device, m_device->getGraphicsFamilyIndex(), true, true, false);

		if ( !commandPool->createOnHardware() )
		{
			return false;
		}

		const auto commandBuffer = std::make_unique< Vulkan::CommandBuffer >(commandPool, true);

		if ( !commandBuffer->isCreated() || !commandBuffer->begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT) )
		{
			return false;
		}

		const auto handle = commandBuffer->handle();

		/* Every image to GENERAL (the storage layout), the spectrum through TRANSFER_DST for its upload, the foam cleared
		 * (a transfer, in GENERAL) — the resolve pass reads the previous foam before it writes. */
		{
			std::array< VkImageMemoryBarrier, 7 > barriers{};
			const std::array< Vulkan::Image *, 7 > images{m_spectrumImage.get(), m_fieldImages[0].get(), m_fieldImages[1].get(), m_displacementImage.get(), m_slopeImage.get(), m_foamImage.get(), m_previousDisplacementImage.get()};

			for ( size_t index = 0; index < barriers.size(); ++index )
			{
				barriers[index].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
				barriers[index].srcAccessMask = 0;
				barriers[index].dstAccessMask = index == 0 || index >= 5 ? VK_ACCESS_TRANSFER_WRITE_BIT : VK_ACCESS_SHADER_WRITE_BIT;
				barriers[index].oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
				barriers[index].newLayout = index == 0 ? VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL : VK_IMAGE_LAYOUT_GENERAL;
				barriers[index].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
				barriers[index].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
				barriers[index].image = images[index]->handle();
				barriers[index].subresourceRange = AllCascades;
			}

			vkCmdPipelineBarrier(handle, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0, nullptr, 0, nullptr, static_cast< uint32_t >(barriers.size()), barriers.data());
		}

		{
			VkBufferImageCopy region{};
			region.imageSubresource = {.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .mipLevel = 0, .baseArrayLayer = 0, .layerCount = CascadeCount};
			region.imageExtent = {.width = Resolution, .height = Resolution, .depth = 1};

			vkCmdCopyBufferToImage(handle, staging->handle(), m_spectrumImage->handle(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

			constexpr VkClearColorValue NoFoam{};

			vkCmdClearColorImage(handle, m_foamImage->handle(), VK_IMAGE_LAYOUT_GENERAL, &NoFoam, 1, &AllCascades);
		}

		{
			std::array< VkImageMemoryBarrier, 2 > barriers{};

			for ( auto & barrier : barriers )
			{
				barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
				barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
				barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
				barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
				barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
				barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
				barrier.subresourceRange = AllCascades;
			}

			barriers[0].oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			barriers[0].image = m_spectrumImage->handle();
			barriers[1].oldLayout = VK_IMAGE_LAYOUT_GENERAL;
			barriers[1].image = m_foamImage->handle();

			vkCmdPipelineBarrier(handle, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0, nullptr, 0, nullptr, static_cast< uint32_t >(barriers.size()), barriers.data());
		}

		if ( !commandBuffer->end() )
		{
			return false;
		}

		auto * queue = m_device->getGraphicsQueue(Vulkan::QueuePriority::High);

		if ( queue == nullptr || !queue->submit(*commandBuffer) || !queue->waitIdle() )
		{
			return false;
		}

		for ( const auto & image : {m_spectrumImage, m_fieldImages[0], m_fieldImages[1], m_displacementImage, m_slopeImage, m_foamImage, m_previousDisplacementImage} )
		{
			image->setCurrentImageLayout(VK_IMAGE_LAYOUT_GENERAL);
		}

		return true;
	}

	void
	OceanWaves::recordUpdate (const Vulkan::CommandBuffer & commandBuffer, float time, float deltaTime) noexcept
	{
		const auto handle = commandBuffer.handle();
		const auto descriptorSet = m_descriptorSet->handle();

		PushConstants constants;
		constants.cascadeSizes = {m_parameters.cascadeSizes[0], m_parameters.cascadeSizes[1], m_parameters.cascadeSizes[2], 1.0F};
		constants.time = time;
		constants.gravity = m_parameters.gravity;
		constants.choppiness = m_parameters.choppiness;
		constants.deltaTime = std::max(deltaTime, 0.0F);
		constants.whitecapThreshold = m_parameters.whitecapThreshold;
		constants.whitecapSharpness = m_parameters.whitecapSharpness;
		constants.whitecapLifetime = m_parameters.whitecapLifetime;

		/* A cascade births whitecaps when its band's LONGEST wave reaches the minimum wavelength. buildInitialSpectrum()
		 * starts the band of cascade c > 0 at 6 fundamentals of its own tile: its longest wave is the tile / 6. */
		for ( uint32_t cascade = 0; cascade < CascadeCount; ++cascade )
		{
			const auto longest = cascade == 0 ? m_parameters.cascadeSizes[0] : m_parameters.cascadeSizes[cascade] / 6.0F;

			constants.whitecapCascades[cascade] = longest >= m_parameters.whitecapMinimumWavelength ? 1.0F : 0.0F;
		}

		/* Every pass reads what the previous one wrote; the first one overwrites the fields the previous frame's
		 * resolve and the vertex/fragment stages read. */
		const auto computeToCompute = [handle] (VkPipelineStageFlags sourceStages, VkAccessFlags sourceAccess) {
			VkMemoryBarrier barrier{};
			barrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
			barrier.srcAccessMask = sourceAccess;
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;

			vkCmdPipelineBarrier(handle, sourceStages, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 1, &barrier, 0, nullptr, 0, nullptr);
		};

		computeToCompute(VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_TRANSFER_READ_BIT);

		/* The displacement the previous frames were drawn with becomes the PREVIOUS displacement (the velocity of the
		 * displaced surface). The previous frame's vertex stages are done with both images (the barrier above); the
		 * copy must be done before the resolve overwrites the displacement, and its result visible to this frame's
		 * vertex stages. The first update has no displacement yet: it copies its own result after the resolve. */
		const auto copyDisplacement = [this, handle] (VkPipelineStageFlags sourceStages, VkAccessFlags sourceAccess) {
			VkMemoryBarrier before{};
			before.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
			before.srcAccessMask = sourceAccess;
			before.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;

			vkCmdPipelineBarrier(handle, sourceStages, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 1, &before, 0, nullptr, 0, nullptr);

			VkImageCopy region{};
			region.srcSubresource = {.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .mipLevel = 0, .baseArrayLayer = 0, .layerCount = CascadeCount};
			region.dstSubresource = {.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .mipLevel = 0, .baseArrayLayer = 0, .layerCount = CascadeCount};
			region.extent = CascadeExtent;

			vkCmdCopyImage(handle, m_displacementImage->handle(), VK_IMAGE_LAYOUT_GENERAL, m_previousDisplacementImage->handle(), VK_IMAGE_LAYOUT_GENERAL, 1, &region);

			VkMemoryBarrier after{};
			after.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
			after.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			after.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;

			vkCmdPipelineBarrier(handle, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 1, &after, 0, nullptr, 0, nullptr);
		};

		if ( m_previousDisplacementValid )
		{
			copyDisplacement(VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_ACCESS_SHADER_WRITE_BIT);
		}

		vkCmdBindDescriptorSets(handle, VK_PIPELINE_BIND_POINT_COMPUTE, m_pipelineLayout->handle(), 0, 1, &descriptorSet, 0, nullptr);

		/* 1. h(k, t) and the eight fields. */
		vkCmdBindPipeline(handle, VK_PIPELINE_BIND_POINT_COMPUTE, m_evolvePipeline->handle());
		vkCmdPushConstants(handle, m_pipelineLayout->handle(), VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(constants), &constants);
		vkCmdDispatch(handle, Resolution / 16, Resolution / 16, CascadeCount);

		computeToCompute(VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_ACCESS_SHADER_WRITE_BIT);

		/* 2. The rows, then the columns: one workgroup per line. */
		vkCmdBindPipeline(handle, VK_PIPELINE_BIND_POINT_COMPUTE, m_fftPipeline->handle());

		for ( const int32_t vertical : {0, 1} )
		{
			constants.vertical = vertical;

			vkCmdPushConstants(handle, m_pipelineLayout->handle(), VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(constants), &constants);
			vkCmdDispatch(handle, Resolution, 1, CascadeCount);

			computeToCompute(VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_ACCESS_SHADER_WRITE_BIT);
		}

		/* 3. The real fields, the choppiness applied, and the whitecaps. */
		vkCmdBindPipeline(handle, VK_PIPELINE_BIND_POINT_COMPUTE, m_resolvePipeline->handle());
		vkCmdPushConstants(handle, m_pipelineLayout->handle(), VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(constants), &constants);
		vkCmdDispatch(handle, Resolution / 16, Resolution / 16, CascadeCount);

		if ( !m_previousDisplacementValid )
		{
			copyDisplacement(VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_ACCESS_SHADER_WRITE_BIT);

			m_previousDisplacementValid = true;
		}

		/* The outputs, visible to whoever samples or copies them. */
		{
			VkMemoryBarrier barrier{};
			barrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
			barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_TRANSFER_READ_BIT;

			vkCmdPipelineBarrier(handle, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 1, &barrier, 0, nullptr, 0, nullptr);
		}
	}

	bool
	OceanWaves::readback (std::vector< float > & displacement, std::vector< float > & slopes) const noexcept
	{
		constexpr VkDeviceSize LayerBytes = static_cast< VkDeviceSize >(Resolution) * Resolution * 4 * sizeof(uint16_t);
		constexpr VkDeviceSize ImageBytes = LayerBytes * CascadeCount;

		auto staging = std::make_unique< Vulkan::Buffer >(m_device, static_cast< VkBufferCreateFlags >(0), ImageBytes * 2, VK_BUFFER_USAGE_TRANSFER_DST_BIT, true);
		staging->setHostReadable(true);

		if ( !staging->createOnHardware() )
		{
			return false;
		}

		const auto commandPool = std::make_shared< Vulkan::CommandPool >(m_device, m_device->getGraphicsFamilyIndex(), true, true, false);

		if ( !commandPool->createOnHardware() )
		{
			return false;
		}

		const auto commandBuffer = std::make_unique< Vulkan::CommandBuffer >(commandPool, true);

		if ( !commandBuffer->isCreated() || !commandBuffer->begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT) )
		{
			return false;
		}

		VkBufferImageCopy region{};
		region.imageSubresource = {.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .mipLevel = 0, .baseArrayLayer = 0, .layerCount = CascadeCount};
		region.imageExtent = {.width = Resolution, .height = Resolution, .depth = 1};

		vkCmdCopyImageToBuffer(commandBuffer->handle(), m_displacementImage->handle(), VK_IMAGE_LAYOUT_GENERAL, staging->handle(), 1, &region);

		region.bufferOffset = ImageBytes;

		vkCmdCopyImageToBuffer(commandBuffer->handle(), m_slopeImage->handle(), VK_IMAGE_LAYOUT_GENERAL, staging->handle(), 1, &region);

		if ( !commandBuffer->end() )
		{
			return false;
		}

		auto * queue = m_device->getGraphicsQueue(Vulkan::QueuePriority::High);

		if ( queue == nullptr || !queue->submit(*commandBuffer) || !queue->waitIdle() )
		{
			return false;
		}

		const auto * mapped = staging->mapMemoryAs< uint16_t >();

		if ( mapped == nullptr )
		{
			return false;
		}

		const auto values = static_cast< size_t >(ImageBytes / sizeof(uint16_t));

		displacement.resize(values);
		slopes.resize(values);

		for ( size_t index = 0; index < values; ++index )
		{
			displacement[index] = halfToFloat(mapped[index]);
			slopes[index] = halfToFloat(mapped[values + index]);
		}

		staging->unmapMemory();

		return true;
	}

	bool
	OceanWaves::selfTest (float time, std::string & report) noexcept
	{
		if ( m_descriptorSet == nullptr )
		{
			report = "The generator is not created.";

			return false;
		}

		/* The GPU path, one-shot. */
		{
			const auto commandPool = std::make_shared< Vulkan::CommandPool >(m_device, m_device->getGraphicsFamilyIndex(), true, true, false);

			if ( !commandPool->createOnHardware() )
			{
				report = "Unable to create a command pool.";

				return false;
			}

			const auto commandBuffer = std::make_unique< Vulkan::CommandBuffer >(commandPool, true);

			if ( !commandBuffer->isCreated() || !commandBuffer->begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT) )
			{
				report = "Unable to record a command buffer.";

				return false;
			}

			this->recordUpdate(*commandBuffer, time, 0.0F);

			auto * queue = m_device->getGraphicsQueue(Vulkan::QueuePriority::High);

			if ( !commandBuffer->end() || queue == nullptr || !queue->submit(*commandBuffer) || !queue->waitIdle() )
			{
				report = "Unable to run the GPU update.";

				return false;
			}
		}

		std::vector< float > gpuDisplacement;
		std::vector< float > gpuSlopes;

		if ( !this->readback(gpuDisplacement, gpuSlopes) )
		{
			report = "Unable to read the outputs back.";

			return false;
		}

		/* The CPU reference: each of the eight fields on its own, double precision. */
		constexpr auto N = static_cast< size_t >(Resolution);
		constexpr size_t FieldCount = 8;
		constexpr std::array< const char *, FieldCount > Names{"Dx", "h", "Dz", "dDx/dz", "dh/dx", "dh/dz", "dDx/dx", "dDz/dz"};

		std::array< double, FieldCount > maxError{};
		std::array< double, FieldCount > peak{};
		std::array< double, CascadeCount > rmsHeight{};
		std::vector< std::complex< float > > spectrum;
		std::vector< std::complex< double > > field(N * N);
		std::vector< std::complex< double > > line(N);
		const auto lambda = static_cast< double >(m_parameters.choppiness);
		const auto gravity = static_cast< double >(m_parameters.gravity);

		for ( uint32_t cascade = 0; cascade < CascadeCount; ++cascade )
		{
			buildInitialSpectrum(m_parameters, cascade, spectrum);

			const auto deltaK = 2.0 * std::numbers::pi / static_cast< double >(m_parameters.cascadeSizes[cascade]);

			for ( size_t fieldIndex = 0; fieldIndex < FieldCount; ++fieldIndex )
			{
				for ( size_t z = 0; z < N; ++z )
				{
					for ( size_t x = 0; x < N; ++x )
					{
						const auto n = static_cast< int64_t >(x) - (x < N / 2 ? 0 : static_cast< int64_t >(N));
						const auto m = static_cast< int64_t >(z) - (z < N / 2 ? 0 : static_cast< int64_t >(N));
						const auto kx = static_cast< double >(n) * deltaK;
						const auto kz = static_cast< double >(m) * deltaK;
						const auto kl = std::sqrt((kx * kx) + (kz * kz));
						const auto mirror = (((N - z) % N) * N) + ((N - x) % N);
						const std::complex< double > h0{static_cast< double >(spectrum[(z * N) + x].real()), static_cast< double >(spectrum[(z * N) + x].imag())};
						const std::complex< double > h0Mirror = std::conj(std::complex< double >{static_cast< double >(spectrum[mirror].real()), static_cast< double >(spectrum[mirror].imag())});
						const auto phase = std::sqrt(gravity * kl) * static_cast< double >(time);
						const std::complex< double > e{std::cos(phase), std::sin(phase)};
						const auto h = (h0 * e) + (h0Mirror * std::conj(e));
						const std::complex< double > i{0.0, 1.0};
						std::complex< double > value{0.0, 0.0};

						switch ( fieldIndex )
						{
							case 0 : value = kl > 1.0e-6 ? -i * (kx / kl) * h * lambda : 0.0; break;
							case 1 : value = h; break;
							case 2 : value = kl > 1.0e-6 ? -i * (kz / kl) * h * lambda : 0.0; break;
							case 3 : value = kl > 1.0e-6 ? (kx * kz / kl) * h * lambda : 0.0; break;
							case 4 : value = i * kx * h; break;
							case 5 : value = i * kz * h; break;
							case 6 : value = kl > 1.0e-6 ? (kx * kx / kl) * h * lambda : 0.0; break;
							default : value = kl > 1.0e-6 ? (kz * kz / kl) * h * lambda : 0.0; break;
						}

						field[(z * N) + x] = value;
					}
				}

				/* 2D inverse FFT: the rows, then the columns. */
				for ( size_t z = 0; z < N; ++z )
				{
					std::copy_n(field.begin() + static_cast< std::ptrdiff_t >(z * N), N, line.begin());
					inverseFFT(line);
					std::copy_n(line.begin(), N, field.begin() + static_cast< std::ptrdiff_t >(z * N));
				}

				for ( size_t x = 0; x < N; ++x )
				{
					for ( size_t z = 0; z < N; ++z )
					{
						line[z] = field[(z * N) + x];
					}

					inverseFFT(line);

					for ( size_t z = 0; z < N; ++z )
					{
						field[(z * N) + x] = line[z];
					}
				}

				/* The GPU layout: displacement = (Dx, h, Dz, dDx/dz), slopes = (dh/dx, dh/dz, dDx/dx, dDz/dz). */
				const auto & gpu = fieldIndex < 4 ? gpuDisplacement : gpuSlopes;
				const auto channel = fieldIndex % 4;
				double sumSquares = 0.0;

				for ( size_t texel = 0; texel < N * N; ++texel )
				{
					const auto reference = field[texel].real();
					const auto measured = static_cast< double >(gpu[(((cascade * N * N) + texel) * 4) + channel]);

					maxError[fieldIndex] = std::max(maxError[fieldIndex], std::abs(measured - reference));
					peak[fieldIndex] = std::max(peak[fieldIndex], std::abs(reference));
					sumSquares += reference * reference;
				}

				if ( fieldIndex == 1 )
				{
					rmsHeight[cascade] = std::sqrt(sumSquares / static_cast< double >(N * N));
				}
			}
		}

		/* Half floats keep 11 significant bits: 2^-11 of the peak, twice for the FFT's float32 rounding. */
		bool passed = true;
		std::stringstream stream;

		for ( size_t fieldIndex = 0; fieldIndex < FieldCount; ++fieldIndex )
		{
			const auto tolerance = std::max(peak[fieldIndex] * 2.0e-3, 1.0e-6);

			if ( maxError[fieldIndex] > tolerance )
			{
				passed = false;
			}

			stream << Names[fieldIndex] << " err " << maxError[fieldIndex] << " / peak " << peak[fieldIndex] << "; ";
		}

		stream << "RMS height per cascade: " << rmsHeight[0] << ", " << rmsHeight[1] << ", " << rmsHeight[2] << " m (Hs ~ 4 sigma of the sum). ";

		/* The whitecap tuning data: each cascade's Jacobian (the foam is born below the threshold). */
		std::vector< double > jacobians(N * N);

		for ( uint32_t cascade = 0; cascade < CascadeCount; ++cascade )
		{
			size_t below = 0;

			for ( size_t texel = 0; texel < N * N; ++texel )
			{
				const auto base = ((cascade * N * N) + texel) * 4;

				jacobians[texel] = ((1.0 + static_cast< double >(gpuSlopes[base + 2])) * (1.0 + static_cast< double >(gpuSlopes[base + 3]))) - static_cast< double >(gpuDisplacement[base + 3] * gpuDisplacement[base + 3]);

				if ( jacobians[texel] < static_cast< double >(m_parameters.whitecapThreshold) )
				{
					++below;
				}
			}

			std::ranges::sort(jacobians);

			stream << "J cascade " << cascade << ": min " << jacobians.front() << ", 1 % " << jacobians[(N * N) / 100] << ", 5 % " << jacobians[(N * N) / 20] << ", " << (100.0 * static_cast< double >(below) / static_cast< double >(N * N)) << " % below " << m_parameters.whitecapThreshold << "; ";
		}

		report = stream.str();

		return passed;
	}
}
