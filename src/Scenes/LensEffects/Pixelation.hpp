/*
 * src/Scenes/LensEffects/Pixelation.hpp
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

/* Local inclusions for inheritances. */
#include "Saphir/FramebufferEffectInterface.hpp"

namespace EmEn::Scenes::LensEffects
{
	/**
	 * @brief The pixelation lens effect class.
	 * @extends EmEn::Saphir::FramebufferEffectInterface This is a framebuffer effect.
	 *
	 * Simulates the blocky pixels of low-resolution displays by snapping
	 * UV coordinates to a grid before sampling, producing visible pixel blocks.
	 */
	class Pixelation final : public Saphir::FramebufferEffectInterface
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"Pixelation"};

			/**
			 * @brief Constructs a pixelation lens effect.
			 * @param pixelSize The size of each "big pixel" in screen pixels. Default 4.0.
			 */
			explicit
			Pixelation (float pixelSize = 4.0F) noexcept
				: m_pixelSize{pixelSize > 0.0F ? pixelSize : 4.0F}
			{

			}

			/** @copydoc EmEn::Saphir::FramebufferEffectInterface::generateFragmentShaderCode() */
			[[nodiscard]]
			bool generateFragmentShaderCode (Saphir::Generator::Abstract & generator, Saphir::FragmentShader & fragmentShader) const noexcept override;

			/**
			 * @brief Sets the pixel block size in screen pixels.
			 * @param pixelSize Value > 0.
			 * @return void
			 */
			void setPixelSize (float pixelSize) noexcept;

			/**
			 * @brief Returns the pixel block size.
			 * @return float
			 */
			[[nodiscard]]
			float
			pixelSize () const noexcept
			{
				return m_pixelSize;
			}

		private:

			/** @copydoc EmEn::Saphir::FramebufferEffectInterface::overrideFragmentFetching() */
			[[nodiscard]]
			bool
			overrideFragmentFetching () const noexcept override
			{
				return true;
			}

			/** @copydoc EmEn::Saphir::FramebufferEffectInterface::requestScreenSize() */
			[[nodiscard]]
			bool
			requestScreenSize () const noexcept override
			{
				return true;
			}

			float m_pixelSize{4.0F};
	};
}
