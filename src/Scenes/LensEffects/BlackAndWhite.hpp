/*
 * src/Scenes/LensEffects/BlackAndWhite.hpp
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

/* Local inclusions for inheritance. */
#include "Saphir/FramebufferEffectInterface.hpp"

/* Local inclusions for usages. */
#include "Libs/PixelFactory/Color.hpp"
#include "Libs/Math/Vector.hpp"

namespace EmEn::Scenes::LensEffects
{
	/**
	 * @brief The black and white lens effect class.
	 * @extends EmEn::Saphir::FramebufferEffectInterface This is a framebuffer effect.
	 */
	class BlackAndWhite final : public Saphir::FramebufferEffectInterface
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"BlackAndWhite"};

			/**
			 * @brief Luminance calculation mode enumeration.
			 */
			enum class Mode
			{
				/** @brief Luma Rec.709 : 0.2126R + 0.7152G + 0.0722B (HD/sRGB standard). */
				LumaRec709,
				/** @brief Luma Rec.601 : 0.2989R + 0.5866G + 0.1145B (SD standard). */
				LumaRec601,
				/** @brief Average : (R+G+B)/3. */
				Average,
				/** @brief Desaturation : (min+max)/2 (Photoshop-like). */
				Desaturation,
				/** @brief Lightness : min(R,G,B). */
				Lightness,
				/** @brief Value : max(R,G,B). */
				Value,
				/** @brief Custom weights : dot(rgb, weights) with user-defined vec3. */
				CustomWeights
			};

			/**
			 * @brief Constructs a black and white lens effect.
			 * @param mode The luminance calculation mode. Default LumaRec709.
			 */
			explicit
			BlackAndWhite (Mode mode = Mode::LumaRec709) noexcept
				: m_mode{mode}
			{

			}

			/** @copydoc EmEn::Saphir::FramebufferEffectInterface::generateFragmentShaderCode() */
			[[nodiscard]]
			bool generateFragmentShaderCode (Saphir::Generator::Abstract & generator, Saphir::FragmentShader & fragmentShader) const noexcept override;

			/**
			 * @brief Sets the luminance calculation mode.
			 * @param mode The mode.
			 * @return void
			 */
			void
			setMode (Mode mode) noexcept
			{
				m_mode = mode;
			}

			/**
			 * @brief Returns the luminance calculation mode.
			 * @return Mode
			 */
			[[nodiscard]]
			Mode
			mode () const noexcept
			{
				return m_mode;
			}

			/**
			 * @brief Sets custom RGB channel weights for CustomWeights mode.
			 * @param weights A vec3 of RGB weights.
			 * @return void
			 */
			void
			setChannelWeights (const Libs::Math::Vector< 3, float > & weights) noexcept
			{
				m_channelWeights = weights;
			}

			/**
			 * @brief Returns the custom RGB channel weights.
			 * @return const Libs::Math::Vector< 3, float > &
			 */
			[[nodiscard]]
			const Libs::Math::Vector< 3, float > & 
			channelWeights () const noexcept
			{
				return m_channelWeights;
			}

			/**
			 * @brief Sets the brightness offset.
			 * @param brightness Value in range [-1, 1].
			 * @return void
			 */
			void 
			setBrightness (float brightness) noexcept
			{
				m_brightness = std::clamp(brightness, -1.0F, 1.0F);
			}

			/**
			 * @brief Returns the brightness offset.
			 * @return float
			 */
			[[nodiscard]]
			float 
			brightness () const noexcept
			{
				return m_brightness;
			}

			/**
			 * @brief Sets the contrast multiplier.
			 * @param contrast Value >= 0.
			 * @return void
			 */
			void setContrast (float contrast) noexcept;

			/**
			 * @brief Returns the contrast multiplier.
			 * @return float
			 */
			[[nodiscard]]
			float 
			contrast () const noexcept
			{
				return m_contrast;
			}

			/**
			 * @brief Sets the gamma correction value.
			 * @param gamma Value > 0.
			 * @return void
			 */
			void setGamma (float gamma) noexcept;

			/**
			 * @brief Returns the gamma correction value.
			 * @return float
			 */
			[[nodiscard]]
			float 
			gamma () const noexcept
			{
				return m_gamma;
			}

			/**
			 * @brief Sets the tint color applied to the B&W output.
			 * @param color A reference to a color (e.g. sepia, cyanotype, selenium).
			 * @return void
			 */
			void 
			setTint (const Libs::PixelFactory::Color< float > & color) noexcept
			{
				m_tint = color;
			}

			/**
			 * @brief Returns the tint color.
			 * @return const Libs::PixelFactory::Color< float > &
			 */
			[[nodiscard]]
			const Libs::PixelFactory::Color< float > & 
			tint () const noexcept
			{
				return m_tint;
			}

			/**
			 * @brief Sets the posterize level count.
			 * @param levels 0 = disabled, >0 = number of discrete levels.
			 * @return void
			 */
			void 
			setLevels (uint32_t levels) noexcept
			{
				m_levels = levels;
			}

			/**
			 * @brief Returns the posterize level count.
			 * @return uint32_t
			 */
			[[nodiscard]]
			uint32_t 
			levels () const noexcept
			{
				return m_levels;
			}

		private:

			Mode m_mode{Mode::LumaRec709};
			Libs::Math::Vector< 3, float > m_channelWeights{0.2126F, 0.7152F, 0.0722F};
			float m_brightness{0.0F};
			float m_contrast{1.0F};
			float m_gamma{1.0F};
			Libs::PixelFactory::Color< float > m_tint{Libs::PixelFactory::White};
			uint32_t m_levels{0};
	};
}
