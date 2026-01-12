/*
 * src/Graphics/VolumetricImageResource.cpp
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
 * https://github.com/londnoir/emeraude-engine
 *
 * --- THIS IS AUTOMATICALLY GENERATED, DO NOT CHANGE ---
 */

#include "VolumetricImageResource.hpp"

/* Local inclusions. */
#include "Resources/Manager.hpp"
#include "Tracer.hpp"

namespace EmEn::Graphics
{
	using namespace Libs;

	bool
	VolumetricImageResource::load (Resources::AbstractServiceProvider & /*serviceProvider*/) noexcept
	{
		if ( !this->beginLoading() )
		{
			return false;
		}

		/* Generate a default 32x32x32 RGBA volume with gradient colors. */
		constexpr uint32_t DefaultSize{32};
		m_width = DefaultSize;
		m_height = DefaultSize;
		m_depth = DefaultSize;
		m_colorCount = 4; /* RGBA */

		m_data.resize(static_cast< size_t >(m_width) * m_height * m_depth * m_colorCount);

		for ( uint32_t zIndex = 0; zIndex < m_depth; zIndex++ )
		{
			for ( uint32_t yIndex = 0; yIndex < m_height; yIndex++ )
			{
				for ( uint32_t xIndex = 0; xIndex < m_width; xIndex++ )
				{
					const auto index = ((static_cast< size_t >(zIndex) * m_height + yIndex) * m_width + xIndex) * m_colorCount;

					m_data[index] = static_cast< uint8_t >(xIndex * 255 / (m_width - 1));
					m_data[index + 1] = static_cast< uint8_t >(yIndex * 255 / (m_height - 1));
					m_data[index + 2] = static_cast< uint8_t >(zIndex * 255 / (m_depth - 1));
					m_data[index + 3] = 255;
				}
			}
		}

		return this->setLoadSuccess(true);
	}

	bool
	VolumetricImageResource::load (Resources::AbstractServiceProvider & /*serviceProvider*/, const std::filesystem::path & filepath) noexcept
	{
		if ( !this->beginLoading() )
		{
			return false;
		}

		/* TODO: Implement file loading for volumetric data formats (e.g., .raw, .vox, etc.) */
		TraceWarning{ClassId} << "Loading volumetric data from file '" << filepath << "' is not yet implemented !";

		return this->setLoadSuccess(false);
	}

	bool
	VolumetricImageResource::load (Resources::AbstractServiceProvider & /*serviceProvider*/, const Json::Value & /*data*/) noexcept
	{
		if ( !this->beginLoading() )
		{
			return false;
		}

		Tracer::error(ClassId, "This method can't be used !");

		return this->setLoadSuccess(false);
	}

	bool
	VolumetricImageResource::isGrayScale () const noexcept
	{
		if ( m_data.empty() || m_colorCount < 3 )
		{
			return m_colorCount == 1;
		}

		/* Check if all voxels have equal R, G, B values. */
		for ( size_t index = 0; index < m_data.size(); index += m_colorCount )
		{
			if ( m_data[index] != m_data[index + 1] || m_data[index] != m_data[index + 2] )
			{
				return false;
			}
		}

		return true;
	}

	PixelFactory::Color< float >
	VolumetricImageResource::averageColor () const noexcept
	{
		if ( m_data.empty() || m_colorCount == 0 )
		{
			return PixelFactory::Black;
		}

		const size_t voxelCount = m_data.size() / m_colorCount;

		if ( voxelCount == 0 )
		{
			return PixelFactory::Black;
		}

		uint64_t sumR = 0;
		uint64_t sumG = 0;
		uint64_t sumB = 0;
		uint64_t sumA = 0;

		for ( size_t index = 0; index < m_data.size(); index += m_colorCount )
		{
			sumR += m_data[index];

			if ( m_colorCount >= 2 )
			{
				sumG += m_data[index + 1];
			}

			if ( m_colorCount >= 3 )
			{
				sumB += m_data[index + 2];
			}

			if ( m_colorCount >= 4 )
			{
				sumA += m_data[index + 3];
			}
		}

		const auto avgR = static_cast< float >(sumR) / static_cast< float >(voxelCount * 255);
		const auto avgG = m_colorCount >= 2 ? static_cast< float >(sumG) / static_cast< float >(voxelCount * 255) : avgR;
		const auto avgB = m_colorCount >= 3 ? static_cast< float >(sumB) / static_cast< float >(voxelCount * 255) : avgR;
		const auto avgA = m_colorCount >= 4 ? static_cast< float >(sumA) / static_cast< float >(voxelCount * 255) : 1.0F;

		return {avgR, avgG, avgB, avgA};
	}
}
