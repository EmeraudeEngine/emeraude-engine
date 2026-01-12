/*
 * src/Libs/PixelFactory/Gradient.hpp
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

#pragma once

/* STL inclusions. */
#include <map>

/* Local inclusions. */
#include "Libs/Math/Base.hpp"
#include "Color.hpp"

namespace EmEn::Libs::PixelFactory
{
	/**
	 * @brief The gradient pixmap class.
	 * @tparam scale_data_t
	 * @tparam color_data_t
	 */
	template< typename scale_data_t = float, typename color_data_t = float >
	requires (std::is_floating_point_v< scale_data_t >, std::is_floating_point_v< color_data_t >)
	class Gradient final
	{
		public:

			/**
			 * @brief Constructs a default gradient.
			 */
			Gradient () noexcept = default;

			/**
			 * @brief Constructs a gradient from two colors.
			 * @param colorAtZero A reference to a color for the execute point.
			 * @param colorAtOne A reference to a color for the end point.
			 */
			Gradient (const Color< color_data_t > & colorAtZero, const Color< color_data_t > & colorAtOne) noexcept
			{
				this->addColorAt(static_cast< scale_data_t >(0), colorAtZero);
				this->addColorAt(static_cast< scale_data_t >(1), colorAtOne);
			}

			/**
			 * @brief Adds a new color to the existing gradient.
			 * @param position A scalar value to place the color in the gradient.
			 * @param color A reference to a color.
			 */
			void
			addColorAt (scale_data_t position, const Color< color_data_t > & color) noexcept
			{
				if ( position < 0 )
				{
					position = 0;
				}
				else if ( position > 1 )
				{
					position = 1;
				}

				m_points.emplace(position, color);
			}

			[[nodiscard]]
			Color< color_data_t >
			colorAt (scale_data_t position) const noexcept
			{
				if ( m_points.size() < 2 )
				{
					return Black;
				}

				position = Math::clampToUnit(position);

				auto itStart = m_points.cend();
				auto itEnd = m_points.cend();

				for ( auto pointIt = m_points.cbegin(); pointIt != m_points.cend(); ++pointIt )
				{
					if ( position > pointIt->first )
					{
						itStart = pointIt;
					}

					if ( itEnd == m_points.cend() && position <= pointIt->first )
					{
						itEnd = pointIt;

						break;
					}
				}

				if ( itStart != m_points.cend() && itEnd != m_points.cend() )
				{
					const auto normalizedStart = position - itStart->first;
					const auto normalizedEnd = itEnd->first - itStart->first;

					const auto factor = Math::clampToUnit(static_cast< scale_data_t >(normalizedStart) / static_cast< scale_data_t >(normalizedEnd));

					/* Interpolate the targeted value. */
					return Color< color_data_t >::linearInterpolation(
						itStart->second,
						itEnd->second,
						factor
					);
				}

				if ( itStart == m_points.cend() )
				{
					/* Return the first key frame value. */
					return m_points.begin()->second;
				}

				/* Return the last key frame value. */
				return m_points.rbegin()->second;
			}

		private:

			std::map< scale_data_t, Color< color_data_t > > m_points;
	};
}
