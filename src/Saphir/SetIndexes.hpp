/*
 * src/Saphir/SetIndexes.hpp
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
#include <cstdint>
#include <vector>
#include <string>
#include <sstream>
#include <algorithm>

namespace EmEn::Saphir
{
	/** @brief The set type. */
	enum class SetType
	{
		/* Everything used by the whole frame. */
		PerView,
		/* Everything used by the same light */
		PerLight,
		/* Everything used by the same model. */
		PerModel,
		/* Everything used by an object layer. */
		PerModelLayer
	};

	/**
	 * @brief The set indexes class. This ensures the order of set when generating the GLSL source code.
	 * @note This should be only used in the shader generation. The Vulkan set order is known by the rendering code.
	 */
	class SetIndexes final
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"SetIndexes"};

			/**
			 * @brief Constructs a set indexes.
			 */
			SetIndexes () noexcept = default;

			/**
			 * @brief Enables a set type.
			 * @param setType The type of set.
			 * @return void
			 */
			void
			enableSet (SetType setType) noexcept
			{
				if ( this->isSetEnabled(setType) )
				{
					return;
				}

				m_setTypes.emplace_back(setType);
			}

			/**
			 * @brief Returns whether a set is enabled.
			 * @param setType The type of set.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isSetEnabled (SetType setType) const noexcept
			{
				return std::ranges::any_of(m_setTypes, [setType] (const auto & currentDescriptorSetType) {
					return currentDescriptorSetType == setType;
				});
			}

			/**
			 * @brief Returns the index of a set.
			 * @param setType The type of set.
			 * @return uint32_t
			 */
			[[nodiscard]]
			uint32_t set (SetType setType) const noexcept;

		private:

			/**
			 * @brief STL streams printable object.
			 * @param out A reference to the stream output.
			 * @param obj A reference to the object to print.
			 * @return std::ostream &
			 */
			friend std::ostream & operator<< (std::ostream & out, const SetIndexes & obj);

			std::vector< SetType > m_setTypes;
	};

	inline
	std::ostream &
	operator<< (std::ostream & out, const SetIndexes & obj)
	{
		return out << SetIndexes::ClassId << " data :" "\n"
			"Set per view : " << obj.set(SetType::PerView) << "\n"
			"Set per light : " << obj.set(SetType::PerLight) << "\n"
			"Set per model layer : " << obj.set(SetType::PerModelLayer) << "\n";
	}

	/**
	 * @brief Stringifies the object.
	 * @param obj A reference to the object to print.
	 * @return std::string
	 */
	inline
	std::string
	to_string (const SetIndexes & obj) noexcept
	{
		std::stringstream output;

		output << obj;

		return output.str();
	}
}
