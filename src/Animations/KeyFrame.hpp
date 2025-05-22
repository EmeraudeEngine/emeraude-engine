/*
 * src/Animations/KeyFrame.hpp
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

#pragma once

/* STL inclusions. */
#include <map>

/* Local inclusions for usages. */
#include "Libs/Math/Base.hpp"
#include "Libs/Variant.hpp"

namespace EmEn::Animations
{
	/**
	 * @brief The Keyframe definition.
	 */
	class KeyFrame final
	{
		public:

			/**
			 * @brief Construct a new key with a valued type.
			 * @param value The value at this keyframe [std::move].
			 * @param interpolation The type of interpolation.
			 */
			KeyFrame (Libs::Variant value, Libs::Math::InterpolationType interpolation) noexcept
				: m_value{std::move(value)},
				m_interpolation{interpolation}
			{

			}

			/**
			 * @brief Returns the value.
			 * @return const Libraries::Variant &
			 */
			[[nodiscard]]
			const Libs::Variant &
			value () const noexcept
			{
				return m_value;
			}

			/**
			 * @brief Returns the interpolation type.
			 * @return const Libs::Math::InterpolationType &
			 */
			[[nodiscard]]
			const Libs::Math::InterpolationType &
			interpolation () const noexcept
			{
				return m_interpolation;
			}

		private:

			Libs::Variant m_value;
			Libs::Math::InterpolationType m_interpolation;
	};

	/**
	 * @brief KeyFrame list using a std::map.
	 */
	using KeyFrames = std::map< uint32_t, KeyFrame >;
}
