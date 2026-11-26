/*
 * src/Audio/Filters/Abstract.hpp
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

#pragma once

/* Project configuration. */
#include "emeraude_export.hpp"

/* Local inclusions for inheritances. */
#include "Audio/AbstractObject.hpp"

namespace EmEn::Audio::Filters
{
	/**
	 * @brief What a filter does to the three bands OpenAL's filters know: the whole signal, its low frequencies
	 * (a shelf at 250 Hz) and its high frequencies (a shelf at 5 kHz). 1 leaves a band as it is.
	 * @note Two filters in a row multiply their responses: how a source combines its user's direct filter with the
	 * listener's medium (Source::setListenerMedium()), OpenAL giving a source one direct filter only.
	 */
	struct Response final
	{
		float gain{1.0F};
		float gainLF{1.0F};
		float gainHF{1.0F};
	};

	/**
	 * @brief The base class for all OpenAL filters.
	 * @extends EmEn::Audio::AbstractObject This is a base OpenAL object.
	 */
	class EMEN_API Abstract : public AbstractObject
	{
		public:

			/**
			 * @brief Destructs the filter.
			 */
			~Abstract () override;

			/**
			 * @brief Copy constructor.
			 * @param copy A reference to the copied instance.
			 */
			Abstract (const Abstract & copy) noexcept = delete;

			/**
			 * @brief Move constructor.
			 * @param copy A reference to the copied instance.
			 */
			Abstract (Abstract && copy) noexcept = delete;

			/**
			 * @brief Copy assignment.
			 * @param copy A reference to the copied instance.
			 */
			Abstract & operator= (const Abstract & copy) noexcept = delete;

			/**
			 * @brief Move assignment.
			 * @param copy A reference to the copied instance.
			 */
			Abstract & operator= (Abstract && copy) noexcept = delete;

			/** @copydoc EmEn::Audio::AbstractObject::isCreated() */
			[[nodiscard]]
			bool isCreated () const noexcept final;

			/**
			 * @brief Reset all properties to default value.
			 */
			virtual void resetProperties () noexcept = 0;

			/**
			 * @brief Returns what this filter does to each band.
			 * @return Response
			 */
			[[nodiscard]]
			virtual Response response () const noexcept = 0;

		protected:

			/**
			 * @brief Constructs a filter.
			 */
			Abstract () noexcept;
	};
}
