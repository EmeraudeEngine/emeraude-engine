/*
 * src/Scenes/CloudSet.cpp
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

#include "CloudSet.hpp"

/* STL inclusions. */
#include <algorithm>

/* Local inclusions. */
#include "Component/CloudVolume.hpp"
#include "Component/DirectionalLight.hpp"
#include "Graphics/CloudShadowMap.hpp"
#include "Graphics/Effects/Atmosphere/VolumetricClouds.hpp"
#include "Graphics/Renderer.hpp"
#include "BindlessTextureSet.hpp"
#include "PrimaryServices.hpp"
#include "SettingKeys.hpp"
#include "Tracer.hpp"

namespace EmEn::Scenes
{
	constexpr auto TracerTag{"CloudSet"};

	CloudSet::CloudSet () noexcept = default;

	CloudSet::~CloudSet ()
	{
		if ( m_shadowMap != nullptr )
		{
			if ( m_shadowMapBindlessSet != nullptr )
			{
				m_shadowMapBindlessSet->unregisterTexture2D(m_shadowMap->target().get());
			}

			m_shadowMap->destroy();
		}
	}

	void
	CloudSet::recordShadowMap (const Vulkan::CommandBuffer & commandBuffer, Graphics::Renderer & renderer, BindlessTextureSet & bindlessTextureSet, const Component::DirectionalLight & sun, uint32_t readStateIndex) noexcept
	{
		if ( this->empty() )
		{
			return;
		}

		/* The clouds as the view march reads them: the map sizes itself and searches along the light on them. */
		std::array< Graphics::CloudBlock, Graphics::MaxCloudVolumes > blocks{};

		const auto census = Graphics::Effects::Atmosphere::VolumetricClouds::gatherClouds(*this, readStateIndex, 0.0F, blocks);

		/* The mean HORIZONTAL width of the drawn clouds: a shadow is as wide as the cloud seen from above. */
		const auto automaticCoverage = [&blocks, &census] (float minimum) noexcept {
			if ( census.drawn == 0 )
			{
				return minimum;
			}

			auto widthSum = 0.0F;

			for ( uint32_t index = 0; index < census.drawn; ++index )
			{
				widthSum += 2.0F * std::max(blocks[index].axisX[3], blocks[index].axisZ[3]);
			}

			return std::max(minimum, AutomaticCoverageFactor * widthSum / static_cast< float >(census.drawn));
		};

		/* ⚠️ The map is created once the first cloud is DRAWN (its coverage is measured on the clouds), but once
		 * it exists it is recorded every frame, drawn clouds or not: an empty record clears it, a skipped one would
		 * leave last frame's shadows under a sun that still casts. */
		if ( !m_shadowMapResolved && census.drawn > 0 )
		{
			m_shadowMapResolved = true;

			auto & settings = renderer.primaryServices().settings();

			if ( !settings.getOrSetDefault< bool >(GraphicsPPCloudsShadowsEnabledKey, DefaultGraphicsPPCloudsShadowsEnabled) )
			{
				TraceInfo{TracerTag} << "'" << GraphicsPPCloudsShadowsEnabledKey << "' is false: the clouds cast no shadow.";

				return;
			}

			const auto resolution = std::clamp(settings.getOrSetDefault< uint32_t >(GraphicsPPCloudsShadowResolutionKey, DefaultGraphicsPPCloudsShadowResolution), 64U, 4096U);

			m_shadowMapMinimumCoverage = std::max(settings.getOrSetDefault< float >(GraphicsPPCloudsShadowCoverageKey, DefaultGraphicsPPCloudsShadowCoverage), 16.0F);
			m_shadowMapCoverageCloudCount = census.drawn;

			const auto coverage = automaticCoverage(m_shadowMapMinimumCoverage);

			auto shadowMap = std::make_unique< Graphics::CloudShadowMap >(renderer, resolution, coverage);

			if ( !shadowMap->create(resolution, resolution) )
			{
				TraceError{TracerTag} << "Unable to create the clouds' shadow map: the clouds cast no shadow.";

				return;
			}

			const auto index = bindlessTextureSet.registerTexture2D(shadowMap->target());

			if ( index == UINT32_MAX )
			{
				TraceError{TracerTag} << "The bindless 2D array is full: the clouds cast no shadow.";

				shadowMap->destroy();

				return;
			}

			m_shadowMap = std::move(shadowMap);
			m_shadowMapBindlessSet = &bindlessTextureSet;
			m_shadowMapCoverage.store(coverage, std::memory_order_release);
			m_shadowMapResolution.store(resolution, std::memory_order_release);
			m_shadowMapBindlessIndex.store(index, std::memory_order_release);
		}

		if ( m_shadowMap == nullptr )
		{
			return;
		}

		/* Clouds still growing on the thread pool join the census one by one: the coverage follows the set, not
		 * the frame. The sun reads the new value at its next update, the map at the frame that publishes it. */
		if ( census.drawn > 0 && census.drawn != m_shadowMapCoverageCloudCount )
		{
			m_shadowMapCoverageCloudCount = census.drawn;

			const auto coverage = automaticCoverage(m_shadowMapMinimumCoverage);

			if ( coverage != m_shadowMapCoverage.load(std::memory_order_acquire) )
			{
				m_shadowMap->setCoverage(coverage);
				m_shadowMapCoverage.store(coverage, std::memory_order_release);

				TraceInfo{TracerTag} << "The clouds' shadow map now covers " << coverage << " m for " << census.drawn << " clouds (" << (coverage / static_cast< float >(m_shadowMapResolution.load(std::memory_order_acquire))) << " m per texel).";
			}
		}

		/* ⚠️ Only once the sun's PUBLISHED block reads this map: before that (the first ticks) its
		 * matrix is not the map's, and no lit shader samples the map anyway. */
		if ( sun.cloudShadowIndex(readStateIndex) != m_shadowMapBindlessIndex.load(std::memory_order_acquire) )
		{
			return;
		}

		const auto & worldToMap = sun.cloudShadowMatrix(readStateIndex);

		/* How far along the light the clouds sit from the camera's plane: the third ROW of the matrix is
		 * depth = light . P - camera depth (column-major, elements 2, 6, 10, 14); a box reaches |light . axis| x
		 * its half extent on each of its axes. */
		const auto * matrix = worldToMap.data();
		auto depthRange = 0.0F;

		for ( uint32_t index = 0; index < census.drawn; ++index )
		{
			const auto & cloud = blocks[index];
			const auto depth = matrix[2] * cloud.centerAndExtinction[0] + matrix[6] * cloud.centerAndExtinction[1] + matrix[10] * cloud.centerAndExtinction[2] + matrix[14];
			const auto reach =
				std::abs(matrix[2] * cloud.axisX[0] + matrix[6] * cloud.axisX[1] + matrix[10] * cloud.axisX[2]) * cloud.axisX[3] +
				std::abs(matrix[2] * cloud.axisY[0] + matrix[6] * cloud.axisY[1] + matrix[10] * cloud.axisY[2]) * cloud.axisY[3] +
				std::abs(matrix[2] * cloud.axisZ[0] + matrix[6] * cloud.axisZ[1] + matrix[10] * cloud.axisZ[2]) * cloud.axisZ[3];

			depthRange = std::max(depthRange, std::abs(depth) + reach);
		}

		static_cast< void >(m_shadowMap->record(commandBuffer, blocks, census.drawn, worldToMap, depthRange));
	}

	void
	CloudSet::add (Scene & scene, const std::shared_ptr< Component::CloudVolume > & cloud) noexcept
	{
		if ( cloud == nullptr )
		{
			return;
		}

		/* Outside the lock: the registration may observe the shape resource, which takes its own. */
		cloud->createOnHardware(scene);

		const std::lock_guard< std::mutex > lock{m_access};

		if ( std::ranges::find(m_clouds, cloud) != m_clouds.end() )
		{
			return;
		}

		m_clouds.emplace_back(cloud);
		m_count.store(m_clouds.size(), std::memory_order_release);
	}

	void
	CloudSet::remove (const std::shared_ptr< Component::CloudVolume > & cloud) noexcept
	{
		if ( cloud == nullptr )
		{
			return;
		}

		{
			const std::lock_guard< std::mutex > lock{m_access};

			std::erase(m_clouds, cloud);
			m_count.store(m_clouds.size(), std::memory_order_release);
		}

		cloud->destroyFromHardware();
	}

	void
	CloudSet::clear () noexcept
	{
		std::vector< std::shared_ptr< Component::CloudVolume > > clouds;

		{
			const std::lock_guard< std::mutex > lock{m_access};

			clouds.swap(m_clouds);
			m_count.store(0, std::memory_order_release);
		}

		for ( const auto & cloud : clouds )
		{
			cloud->destroyFromHardware();
		}
	}
}
