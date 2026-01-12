/*
 * src/Scenes/Effect/GrayscaleLensEffect.hpp
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

/* Local inclusions for inheritance */
#include "Saphir/FramebufferEffectInterface.hpp"

/* Local inclusions for usages. */
#include "Libs/PixelFactory/Color.hpp"

namespace EmEn::Scenes
{
	/**
	 * @brief The gray scale lens effect class;
	 * @extends EmEn::FramebufferEffectInterface This is a framebuffer effect.
	 */
	class GrayscaleLensEffect final : public Saphir::FramebufferEffectInterface
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"GrayscaleLensEffect"};

			/** @brief Grayscale mode enumeration. */
			enum class Mode
			{
				/* Gray = (Red + Green + Blue) / 3 */
				Average,
				/* Gray = 0.2989 * Red + 0.5866 * Green + 0.1145 * Blue */
				LumaRec601_1,
				/* Gray = 0.2126 * Red + 0.7152 * Green + 0.0722 * Blue */
				LumaRec709,
				/* Gray = 0.2220 * Red + 0.7067 * Green + 0.0713 * Blue */
				LumaITU,
				/* Gray = ( max(Red, Green, Blue) + min(Red, Green, Blue) ) / 2 */
				Desaturation,
				/* Gray = min(Red, Green, Blue) OR Gray = max(Red, Green, Blue) */
				Decomposition,
				/* Gray = Red OR Gray = Green OR ... */
				SingleChannel,
				ShadesScale,
				EightBits
			};

			/** @brief Channel options enumeration. */
			enum class Option
			{
				None,
				/* Options for mode SingleChannel */
				RedChannel,
				GreenChannel,
				BlueChannel,
				AlphaChannel,
				/* Options for mode decomposition */
				Min,
				Max
			};

			/**
			 * @brief Constructs a grayscale lens effect.
			 * @param mode The mode of grayscale calculation. Default average.
			 * @param option The option for the grayscale calculation mode. Default none.
			 */
			explicit GrayscaleLensEffect (Mode mode = Mode::Average, Option option = Option::None) noexcept;

			/** @copydoc EmEn::Saphir::FramebufferEffectInterface::generateFragmentShaderCode() */
			[[nodiscard]]
			bool generateFragmentShaderCode (Saphir::Generator::Abstract & generator, Saphir::FragmentShader & fragmentShader) const noexcept override;

			/**
			 * @brief Sets the grayscale calculation mode.
			 * @param mode The mode.
			 * @return void
			 */
			void setMode (Mode mode) noexcept;

			/**
			 * @brief Returns the grayscale calculation mode.
			 * @return Mode
			 */
			[[nodiscard]]
			Mode mode () const noexcept;

			/**
			 * @brief Sets the option for the grayscale calculation mode.
			 * @param option Option.
			 * @return void
			 */
			void setOption (Option option) noexcept;

			/**
			 * @brief Returns the options for the grayscale calculation mode.
			 * @return Option
			 */
			[[nodiscard]]
			Option option () const noexcept;

			/**
			 * @brif Sets the scale level.
			 * @param levels The number of levels.
			 * @return void
			 */
			void setScaleLevel (unsigned int levels) noexcept;

			/**
			 * @brief Returns the number of gray levels.
			 * @return unsigned int
			 */
			[[nodiscard]]
			unsigned int scaleLevel () const noexcept;

			/**
			 * @brief Sets the taint used for the colored scale.
			 * @param color A reference to a color
			 * @return void
			 */
			void setTaint (const Libs::PixelFactory::Color< float > & color) noexcept;

			/**
			 * @brief Returns the color.
			 * @return const Libraries::PixelFactory::Color< float > &
			 */
			[[nodiscard]]
			const Libs::PixelFactory::Color< float > & taint () const noexcept;

		private:

			static constexpr auto Taint{"em_Taint"};
			static constexpr auto ScaleLevel{"em_ScaleLevel"};

			Libs::PixelFactory::Color< float > m_taint = Libs::PixelFactory::White;
			Mode m_mode{Mode::Average};
			Option m_option{Option::None};
			unsigned int m_scaleLevel{16};
	};
}
