/*
 * src/Audio/EffectSlot.hpp
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
#include <memory>

/* Local inclusions for inheritances. */
#include "AbstractObject.hpp"

/* Local inclusions for usages. */
#include "Effects/Abstract.hpp"
#include "Filters/Abstract.hpp"

namespace EmEn::Audio
{
	/**
	 * @brief The EffectSlot class.
	 * @extends EmEn::Audio::AbstractObject
	 */
	class EffectSlot final : public AbstractObject
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"EffectSlot"};

			/**
			 * @brief Constructs an effect slot.
			 */
			EffectSlot () noexcept;

			/**
			 * @brief Copy constructor.
			 * @param copy A reference to the copied instance.
			 */
			EffectSlot (const EffectSlot & copy) noexcept = default;

			/**
			 * @brief Move constructor.
			 * @param copy A reference to the copied instance.
			 */
			EffectSlot (EffectSlot && copy) noexcept = default;

			/**
			 * @brief Copy assignment.
			 * @param copy A reference to the copied instance.
			 */
			EffectSlot & operator= (const EffectSlot & copy) noexcept = default;

			/**
			 * @brief Move assignment.
			 * @param copy A reference to the copied instance.
			 */
			EffectSlot & operator= (EffectSlot && copy) noexcept = default;

			/**
			 * @brief Destructs the effect slot.
			 */
			~EffectSlot () override;

			/** @copydoc EmEn::Engine::Audio::AbstractObject::isCreated() */
			[[nodiscard]]
			bool isCreated () const noexcept override;

			/**
			 * @brief Adds an effect to the slot.
			 * @param effect A reference to an effect smart pointer.
			 * @return bool
			 */
			bool setEffect (const std::shared_ptr< Effects::Abstract > & effect) noexcept;

			/**
			 * @brief Adds a filter to the slot.
			 * @param filter A reference to a filter smart pointer.
			 * @return bool
			 */
			bool setFilter (const std::shared_ptr< Filters::Abstract > & filter) noexcept;

			/**
			 * @brief ???
			 * @FIXME Check this method and use smart pointer.
			 * @param source A pointer to a source.
			 * @return bool
			 */
			bool enable (const AbstractObject * source) const noexcept;

			/**
			 * @brief ???
			 * @FIXME Check this method and use smart pointer.
			 * @param source A pointer to a source.
			 */
			void disable (const AbstractObject * source) const noexcept;

			/** @brief Removes all effects from the source. */
			void clearEffect () noexcept;

			/** @brief Removes all filters from the source. */
			void
			clearFilter () noexcept
			{
				m_filter.reset();
			}

		private:

			std::shared_ptr< const Effects::Abstract > m_effect;
			std::shared_ptr< const Filters::Abstract > m_filter;
	};
}
