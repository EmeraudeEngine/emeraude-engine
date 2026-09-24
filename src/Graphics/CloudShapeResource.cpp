/*
 * src/Graphics/CloudShapeResource.cpp
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

#include "CloudShapeResource.hpp"

/* STL inclusions. */
#include <algorithm>
#include <cmath>
#include <limits>
#include <sstream>

/* Local inclusions. */
#include "Algorithms/WorleyNoise.hpp"
#include "FastJSON.hpp"
#include "Graphics/Renderer.hpp"
#include "Tracer.hpp"
#include "Vulkan/Image.hpp"
#include "Vulkan/ImageView.hpp"
#include "Vulkan/MemoryRegion.hpp"

namespace
{
	using namespace EmEn;
	using namespace EmEn::Base::Math;

	/** @brief One sphere of the growth. */
	struct Puff
	{
		Vector< 3, float > center;
		float radius;
	};

	/**
	 * @brief A deterministic stream of numbers in [0, 1), one integer hash per draw.
	 * @note ⚠️ Not std::uniform_real_distribution: its algorithm is implementation-defined, so the
	 * same seed would grow a different cloud on MSVC and on libstdc++.
	 */
	class RandomStream final
	{
		public:

			explicit
			RandomStream (uint32_t seed) noexcept
				: m_state{hash(seed ^ 0x9E3779B9U)}
			{

			}

			[[nodiscard]]
			float
			next () noexcept
			{
				m_state = hash(m_state + 0x6D2B79F5U);

				return static_cast< float >(m_state >> 8U) * (1.0F / 16777216.0F);
			}

			[[nodiscard]]
			float
			range (float minimum, float maximum) noexcept
			{
				return minimum + (maximum - minimum) * this->next();
			}

		private:

			/* The "lowbias32" hash, C. Wellons, public domain — the same one WorleyNoise uses. */
			[[nodiscard]]
			static
			constexpr
			uint32_t
			hash (uint32_t value) noexcept
			{
				value ^= value >> 16U;
				value *= 0x7FEB352DU;
				value ^= value >> 15U;
				value *= 0x846CA68BU;
				value ^= value >> 16U;

				return value;
			}

			uint32_t m_state;
	};

	/**
	 * @brief Polynomial smooth minimum, I. Quilez, *Smooth minimum* (2013).
	 * @note Always <= min(a, b), so the union it builds stays a CONSERVATIVE distance: the skip
	 * channel derived from it can never step over a surface.
	 */
	[[nodiscard]]
	float
	smoothMinimum (float a, float b, float blend) noexcept
	{
		const auto h = std::max(blend - std::abs(a - b), 0.0F) / blend;

		return std::min(a, b) - h * h * blend * 0.25F;
	}

	/** @brief The voxel counts of a parameter set, after clamping. */
	struct GridDimensions
	{
		uint32_t width;
		uint32_t height;
		uint32_t depth;
	};

	[[nodiscard]]
	GridDimensions
	gridDimensions (const Graphics::CloudShapeResource::Parameters & parameters) noexcept
	{
		const auto width = std::clamp(parameters.resolution, 16U, 256U);
		const auto heightRatio = std::clamp(parameters.heightRatio, 0.1F, 1.0F);
		const auto depthRatio = std::clamp(parameters.depthRatio, 0.1F, 1.0F);

		return {
			width,
			std::max(8U, static_cast< uint32_t >(std::lround(static_cast< float >(width) * heightRatio))),
			std::max(8U, static_cast< uint32_t >(std::lround(static_cast< float >(width) * depthRatio)))
		};
	}

	/** @brief The width of the blend between two puffs, in normalised units. */
	constexpr auto PuffBlend{0.08F};
	/** @brief Children seeded per parent at each generation. */
	constexpr uint32_t ChildrenPerParent{3};
	/** @brief Attempts to place a child inside the box before the parent gives up on it. */
	constexpr uint32_t PlacementAttempts{6};
	/** @brief Octaves of the billow noise. */
	constexpr uint32_t BillowOctaves{3};
	/** @brief Billow cells per normalised unit at the first octave. */
	constexpr auto BillowFrequency{3.0F};
	/** @brief The period of the billow lattice, in cells: far beyond one shape, so nothing repeats in it. */
	constexpr uint32_t BillowPeriod{64};
}

namespace EmEn::Graphics
{
	using namespace Base;
	using namespace Base::Math;
	using namespace Vulkan;

	constexpr auto TracerTag{"CloudShapeResource"};

	Vector< 3, float >
	CloudShapeResource::proportionsOf (const Parameters & parameters) noexcept
	{
		const auto dimensions = gridDimensions(parameters);
		const auto voxelSize = 2.0F / static_cast< float >(dimensions.width);

		return {
			1.0F,
			static_cast< float >(dimensions.height) * voxelSize * 0.5F,
			static_cast< float >(dimensions.depth) * voxelSize * 0.5F
		};
	}

	std::string
	CloudShapeResource::resourceName (const Parameters & parameters) noexcept
	{
		std::stringstream name;

		name << "CloudShape"
			"+s" << parameters.seed <<
			"+r" << parameters.resolution <<
			"+h" << parameters.heightRatio <<
			"+d" << parameters.depthRatio <<
			"+p" << parameters.puffCount <<
			"+b" << parameters.billowStrength <<
			"+f" << parameters.flatBase <<
			"+e" << parameters.edgeSoftness;

		return name.str();
	}

	bool
	CloudShapeResource::load () noexcept
	{
		return this->load(Parameters{});
	}

	bool
	CloudShapeResource::load (const Json::Value & data) noexcept
	{
		Parameters parameters;

		if ( const auto value = FastJSON::getValue< uint32_t >(data, "Seed") )
		{
			parameters.seed = *value;
		}

		if ( const auto value = FastJSON::getValue< uint32_t >(data, "Resolution") )
		{
			parameters.resolution = *value;
		}

		if ( const auto value = FastJSON::getValue< float >(data, "HeightRatio") )
		{
			parameters.heightRatio = *value;
		}

		if ( const auto value = FastJSON::getValue< float >(data, "DepthRatio") )
		{
			parameters.depthRatio = *value;
		}

		if ( const auto value = FastJSON::getValue< uint32_t >(data, "PuffCount") )
		{
			parameters.puffCount = *value;
		}

		if ( const auto value = FastJSON::getValue< float >(data, "BillowStrength") )
		{
			parameters.billowStrength = *value;
		}

		if ( const auto value = FastJSON::getValue< float >(data, "FlatBase") )
		{
			parameters.flatBase = *value;
		}

		if ( const auto value = FastJSON::getValue< float >(data, "EdgeSoftness") )
		{
			parameters.edgeSoftness = *value;
		}

		return this->load(parameters);
	}

	bool
	CloudShapeResource::load (const Parameters & parameters) noexcept
	{
		if ( !this->beginLoading() )
		{
			return false;
		}

		m_parameters = parameters;

		if ( !this->generate() )
		{
			TraceError{TracerTag} << "Unable to grow the cloud shape '" << this->name() << "' !";

			return this->setLoadSuccess(false);
		}

		return this->setLoadSuccess(true);
	}

	bool
	CloudShapeResource::generate () noexcept
	{
		/* ---- Sanitise. The resource name is built from the parameters AS GIVEN, the grid from these. ---- */
		m_parameters.resolution = std::clamp(m_parameters.resolution, 16U, 256U);
		m_parameters.heightRatio = std::clamp(m_parameters.heightRatio, 0.1F, 1.0F);
		m_parameters.depthRatio = std::clamp(m_parameters.depthRatio, 0.1F, 1.0F);
		m_parameters.puffCount = std::clamp(m_parameters.puffCount, 1U, 256U);
		m_parameters.billowStrength = std::clamp(m_parameters.billowStrength, 0.0F, 1.0F);
		m_parameters.flatBase = std::clamp(m_parameters.flatBase, 0.0F, 1.0F);

		const auto dimensions = gridDimensions(m_parameters);

		m_width = dimensions.width;
		m_height = dimensions.height;
		m_depth = dimensions.depth;

		/* Cubic voxels: the box follows the voxel counts, not the requested ratios. */
		const auto voxelSize = 2.0F / static_cast< float >(m_width);

		m_proportions = proportionsOf(m_parameters);

		m_parameters.edgeSoftness = std::clamp(m_parameters.edgeSoftness, voxelSize, 0.5F);

		const auto halfX = m_proportions[X];
		const auto halfY = m_proportions[Y];
		const auto halfZ = m_proportions[Z];

		/* ---- The growth (Bouthors & Neyret 2004). Every puff keeps clear of the box by a margin
		 * that absorbs the density ramp and the outward billow, so no density reaches a face of
		 * the grid: the renderer clips its march to the box and would cut the cloud flat there. ---- */
		RandomStream random{m_parameters.seed};

		/* ⚠️⚠️ THE CORE IS A DOME, NOT A ROW OF SPHERES. The first two versions built the whole
		 * cloud out of spheres, and a sphere whose radius is bounded by the box HEIGHT cannot fill a
		 * box twice as wide as it is tall: the grown clouds used 5-8 % of their voxels and read 3 to
		 * 5 times smaller than their box (measured on `forest`, 2026-09-24: a 6.4 m cloud in a ~25 m
		 * box, with a row of puffs and again with a taller "tower" of puffs). The mass is now ONE
		 * ellipsoid filling the box, cut flat at the base and narrower there — the shape of a
		 * cumulus — and the puffs only BUD on its upper surface. A half-ellipsoid fills pi/6 of its
		 * bounding box. */
		const auto provisionalMargin = 2.0F * voxelSize + m_parameters.edgeSoftness;
		const auto usableHeight = std::max(2.0F * (halfY - provisionalMargin), 4.0F * voxelSize);

		/* The dome keeps 18 % of the usable height for the buds that rise over it. flatBase moves its
		 * centre UP from the base plane: 1 = the equator is the base (widest flat base), 0 = the
		 * centre half a semi-axis over it (a rounded belly narrowing to a small base). */
		const auto semiAxisY = 0.82F * usableHeight / (1.5F - 0.5F * m_parameters.flatBase);
		const auto billowAmplitude = m_parameters.billowStrength * semiAxisY * 0.25F;
		const auto margin = provisionalMargin + billowAmplitude;
		const auto baseLevel = -halfY + margin;

		const Vector< 3, float > coreCenter{0.0F, baseLevel + 0.5F * (1.0F - m_parameters.flatBase) * semiAxisY, 0.0F};
		const Vector< 3, float > coreAxes{
			std::max(0.84F * (halfX - margin), 2.0F * voxelSize),
			semiAxisY,
			std::max(0.84F * (halfZ - margin), 2.0F * voxelSize)
		};
		const auto coreMinimumAxis = std::min({coreAxes[X], coreAxes[Y], coreAxes[Z]});

		const auto fitsInBox = [halfX, halfY, halfZ, margin] (const Puff & puff) {
			return
				std::abs(puff.center[X]) + puff.radius <= halfX - margin &&
				std::abs(puff.center[Z]) + puff.radius <= halfZ - margin &&
				puff.center[Y] + puff.radius <= halfY - margin &&
				puff.center[Y] - puff.radius >= -halfY + margin;
		};

		std::vector< Puff > puffs;
		puffs.reserve(m_parameters.puffCount);

		/* The first generation BUDS on the dome: seeded on its surface, over the upper hemisphere
		 * mostly (a cumulus bulges up and out, never down), half sunk into it. */
		const auto firstGeneration = std::max(m_parameters.puffCount / 2U, 1U);

		for ( uint32_t index = 0; index < firstGeneration; ++index )
		{
			for ( uint32_t attempt = 0; attempt < PlacementAttempts; ++attempt )
			{
				auto direction = Vector< 3, float >{random.range(-1.0F, 1.0F), random.range(-0.1F, 1.0F), random.range(-1.0F, 1.0F)};
				direction.normalize();

				Puff candidate{
					.center = coreCenter + Vector< 3, float >{direction[X] * coreAxes[X], direction[Y] * coreAxes[Y], direction[Z] * coreAxes[Z]} * 0.9F,
					.radius = semiAxisY * random.range(0.2F, 0.34F)
				};

				while ( !fitsInBox(candidate) && candidate.radius > 2.0F * voxelSize )
				{
					candidate.radius *= 0.9F;
				}

				if ( fitsInBox(candidate) )
				{
					puffs.emplace_back(candidate);

					break;
				}
			}
		}

		/* The next generations: children on the UPPER surface of their parents, smaller each time. */
		size_t generationBegin = 0;

		while ( puffs.size() < m_parameters.puffCount && !puffs.empty() )
		{
			const auto generationEnd = puffs.size();

			for ( auto parentIndex = generationBegin; parentIndex < generationEnd && puffs.size() < m_parameters.puffCount; ++parentIndex )
			{
				const auto parent = puffs[parentIndex];

				for ( uint32_t child = 0; child < ChildrenPerParent && puffs.size() < m_parameters.puffCount; ++child )
				{
					for ( uint32_t attempt = 0; attempt < PlacementAttempts; ++attempt )
					{
						/* A direction biased upward: the cumulus grows by convection. */
						auto direction = Vector< 3, float >{random.range(-1.0F, 1.0F), random.range(0.2F, 1.0F), random.range(-1.0F, 1.0F)};
						direction.normalize();

						/* Seeded ON the parent's surface, half sunk into it: a bulge, not a satellite. */
						const Puff candidate{
							.center = parent.center + direction * (parent.radius * 0.85F),
							.radius = parent.radius * random.range(0.42F, 0.6F)
						};

						if ( candidate.radius >= 2.0F * voxelSize && fitsInBox(candidate) )
						{
							puffs.emplace_back(candidate);

							break;
						}
					}
				}
			}

			/* No room left anywhere: the box is full at this resolution. */
			if ( puffs.size() == generationEnd )
			{
				break;
			}

			generationBegin = generationEnd;
		}

		/* ---- The voxels. ---- */
		const Algorithms::WorleyNoise< float > billows{m_parameters.seed, BillowPeriod};
		/* Outside this band around the surface the density is known without the noise. */
		const auto bandOutside = billowAmplitude + m_parameters.edgeSoftness;
		const auto bandInside = billowAmplitude + m_parameters.edgeSoftness;

		m_voxels.assign(static_cast< size_t >(m_width) * m_height * m_depth * 2, 0);

		size_t occupiedVoxels = 0;

		for ( uint32_t zIndex = 0; zIndex < m_depth; ++zIndex )
		{
			const auto z = -halfZ + (static_cast< float >(zIndex) + 0.5F) * voxelSize;

			for ( uint32_t yIndex = 0; yIndex < m_height; ++yIndex )
			{
				const auto y = -halfY + (static_cast< float >(yIndex) + 0.5F) * voxelSize;

				for ( uint32_t xIndex = 0; xIndex < m_width; ++xIndex )
				{
					const auto x = -halfX + (static_cast< float >(xIndex) + 0.5F) * voxelSize;
					const Vector< 3, float > position{x, y, z};

					/* The dome. The density follows I. Quilez's ellipsoid distance (*Ellipsoid SDF*, 2019),
					 * a good approximation near the surface; the SKIP channel needs a true lower bound
					 * instead, and (|p/r| - 1) * min(r) is one — scaling by the smallest axis only ever
					 * shrinks a distance. */
					const auto relative = position - coreCenter;
					const Vector< 3, float > scaled{relative[X] / coreAxes[X], relative[Y] / coreAxes[Y], relative[Z] / coreAxes[Z]};
					const Vector< 3, float > scaledTwice{scaled[X] / coreAxes[X], scaled[Y] / coreAxes[Y], scaled[Z] / coreAxes[Z]};
					const auto k0 = scaled.length();
					const auto k1 = std::max(scaledTwice.length(), 1.0e-6F);

					auto distance = k0 * (k0 - 1.0F) / k1;
					auto lowerBound = (k0 - 1.0F) * coreMinimumAxis;

					/* The smooth union of the buds. smin stays within blend/4 of min, hence the bound. */
					for ( const auto & puff : puffs )
					{
						const auto puffDistance = Vector< 3, float >::distance(position, puff.center) - puff.radius;

						distance = smoothMinimum(distance, puffDistance, PuffBlend);
						lowerBound = std::min(lowerBound, puffDistance);
					}

					lowerBound -= PuffBlend * 0.25F;

					/* Intersected with the half-space above the base: the condensation level. */
					distance = std::max(distance, baseLevel - y);
					lowerBound = std::max(lowerBound, baseLevel - y);

					float density;

					if ( distance > bandOutside )
					{
						density = 0.0F;
					}
					else if ( distance < -bandInside )
					{
						density = 1.0F;
					}
					else
					{
						/* The billows push the surface OUT where the inverted cellular noise peaks. */
						const auto bulge = billows.generateBillows(x * BillowFrequency, y * BillowFrequency, z * BillowFrequency, BillowOctaves);
						const auto displaced = distance - billowAmplitude * (bulge - 0.5F) * 2.0F;

						density = std::clamp(-displaced / m_parameters.edgeSoftness, 0.0F, 1.0F);
					}

					/* The skip distance: how far the nearest density can be, assuming the billow pushes it
					 * out by its full amplitude. Rounded DOWN so the march never steps over a surface. */
					const auto safeDistance = std::clamp((lowerBound - billowAmplitude) / MaxSkipDistance, 0.0F, 1.0F);

					const auto voxelIndex = ((static_cast< size_t >(zIndex) * m_height + yIndex) * m_width + xIndex) * 2;

					m_voxels[voxelIndex] = static_cast< uint8_t >(std::lround(density * 255.0F));
					m_voxels[voxelIndex + 1] = static_cast< uint8_t >(std::floor(safeDistance * 255.0F));

					if ( m_voxels[voxelIndex] > 0 )
					{
						++occupiedVoxels;
					}
				}
			}
		}

		m_occupancy = static_cast< float >(occupiedVoxels) / static_cast< float >(m_width * m_height * m_depth);

		TraceInfo{TracerTag} << "Cloud shape '" << this->name() << "' grown: a dome and " << puffs.size() << " buds, " << m_width << "x" << m_height << "x" << m_depth << " voxels, " << (m_occupancy * 100.0F) << "% occupied.";

		return occupiedVoxels > 0;
	}

	bool
	CloudShapeResource::onDependenciesLoaded () noexcept
	{
		if ( !this->isCreated() && !this->createOnHardware(this->serviceProvider().graphicsRenderer()) )
		{
			TraceError{TracerTag} << "Unable to create the cloud shape '" << this->name() << "' on the GPU !";

			return false;
		}

		return true;
	}

	bool
	CloudShapeResource::isCreated () const noexcept
	{
		return m_image != nullptr && m_image->isCreated() &&
			m_imageView != nullptr && m_imageView->isCreated() &&
			m_sampler != nullptr && m_sampler->isCreated();
	}

	bool
	CloudShapeResource::createOnHardware (Renderer & renderer) noexcept
	{
		if ( m_voxels.empty() )
		{
			Tracer::error(TracerTag, "No voxel grid to upload !");

			return false;
		}

		/* The FULL chain: the light march reads a coarser level (a cheaper, smoother density), and a
		 * distant cloud is marched with steps longer than a voxel. */
		const auto mipLevels = static_cast< uint32_t >(std::floor(std::log2(static_cast< float >(std::max({m_width, m_height, m_depth}))))) + 1U;

		m_image = std::make_shared< Image >(
			renderer.device(),
			VK_IMAGE_TYPE_3D,
			VK_FORMAT_R8G8_UNORM,
			VkExtent3D{m_width, m_height, m_depth},
			VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
			0,
			mipLevels
		);
		m_image->setIdentifier(ClassId, this->name(), "Image");

		if ( !m_image->createOnHardware() )
		{
			Tracer::error(TracerTag, "Unable to create the 3D image !");

			m_image.reset();

			return false;
		}

		/* ⚠️ The mips are generated by the upload's blit chain — which filtered ONE SLICE of a 3D image
		 * until this resource needed it (ImageTransferOperation::finalizeForGPU()). */
		if ( !m_image->writeData(renderer.transferManager(), MemoryRegion{m_voxels.data(), m_voxels.size()}) )
		{
			Tracer::error(TracerTag, "Unable to upload the voxel grid !");

			m_image->destroyFromHardware();
			m_image.reset();

			return false;
		}

		m_imageView = std::make_shared< ImageView >(
			m_image,
			VK_IMAGE_VIEW_TYPE_3D,
			VkImageSubresourceRange{
				.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
				.baseMipLevel = 0,
				.levelCount = m_image->createInfo().mipLevels,
				.baseArrayLayer = 0,
				.layerCount = 1
			}
		);
		m_imageView->setIdentifier(ClassId, this->name(), "ImageView");

		if ( !m_imageView->createOnHardware() )
		{
			Tracer::error(TracerTag, "Unable to create the 3D image view !");

			return false;
		}

		/* ⚠️ CLAMP, never the REPEAT of the generic Texture3D sampler: a shape is a finite object, and a
		 * wrapped fetch at a face of the box would read the density of the opposite face. The grid keeps
		 * its faces empty anyway (the growth margin), which makes the clamp exact. */
		m_sampler = renderer.getSampler("CloudShape", [] (Settings & /*settings*/, VkSamplerCreateInfo & createInfo) {
			createInfo.magFilter = VK_FILTER_LINEAR;
			createInfo.minFilter = VK_FILTER_LINEAR;
			createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
			createInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
			createInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
			createInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
			createInfo.mipLodBias = 0.0F;
			createInfo.anisotropyEnable = VK_FALSE;
			createInfo.maxAnisotropy = 1.0F;
			createInfo.compareEnable = VK_FALSE;
			createInfo.minLod = 0.0F;
			createInfo.maxLod = VK_LOD_CLAMP_NONE;
		});

		if ( m_sampler == nullptr )
		{
			Tracer::error(TracerTag, "Unable to get the cloud shape sampler !");

			return false;
		}

		return true;
	}

	void
	CloudShapeResource::destroyFromHardware () noexcept
	{
		if ( m_imageView != nullptr )
		{
			m_imageView->destroyFromHardware();
			m_imageView.reset();
		}

		if ( m_image != nullptr )
		{
			m_image->destroyFromHardware();
			m_image.reset();
		}

		/* NOTE: The sampler belongs to the renderer's shared cache — only release our reference. */
		m_sampler.reset();
	}
}
