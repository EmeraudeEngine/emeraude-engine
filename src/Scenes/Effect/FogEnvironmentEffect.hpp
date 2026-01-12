/*
 * src/Scenes/Effect/FogEnvironmentEffect.hpp
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
#include "Saphir/EffectInterface.hpp"

/* Local inclusions for usages. */
#include "Libs/PixelFactory/Color.hpp"

namespace EmEn::Scenes
{
	/**
	 * @brief The fog environment effect.
	 * @extends EmEn::Saphir::EffectInterface This is a shader effect.
	 */
	class FogEnvironmentEffect final : public Saphir::EffectInterface
	{
		public:

			/** @brief The fog calculation mode enumeration. */
			enum class FogMode
			{
				Linear,
				Exponential,
				Exponential2,
				Exponential2Alt
			};

			/** @brief Class identifier. */
			static constexpr auto ClassId{"FogEnvironmentEffect"};

			/**
			 * @brief Constructs the fog environment effect.
			 * @param mode The fog algorithm. Default FogMode::Exponential.
			 * @param highQuality Sets the fog per fragment. Default true.
			 */
			FogEnvironmentEffect (FogMode mode = FogMode::Exponential, bool highQuality = true) noexcept;

			/** @copydoc EmEn::Saphir::EffectInterface::FogEnvironmentEffect::generateVertexShaderCode() */
			[[nodiscard]]
			bool generateVertexShaderCode (Saphir::Generator::Abstract & generator, Saphir::VertexShader & vertexShader) const noexcept override;

			/** @copydoc EmEn::Saphir::EffectInterface::FogEnvironmentEffect::generateFragmentShaderCode() */
			[[nodiscard]]
			bool generateFragmentShaderCode (Saphir::Generator::Abstract & generator, Saphir::FragmentShader & fragmentShader) const noexcept override;

			/**
			 * @brief Sets the fog density.
			 * @param density The value.
			 * @return void
			 */
			void setDensity (float density) noexcept;

			/**
			 * @brief Sets the linear range from no density to full density.
			 * @note Only useful with the linear calculation mode.
			 * @param start A value in Z axis.
			 * @param end A value in Z axis.
			 * @return void
			 */
			void setLinearRange (float start, float end) noexcept;

			/**
			 * @brief Sets the color of the fog.
			 * @param color A reference to a color.
			 * @return void
			 */
			void setColor (const Libs::PixelFactory::Color< float > & color) noexcept;

			/**
			 * @brief Tries to use the background color of the scene.
			 * @return void
			 */
			void useBackgroundColor () noexcept;

			/**
			 * @brief Returns the fog calculation mode.
			 * @return FogMode
			 */
			[[nodiscard]]
			FogMode mode () const noexcept;

			/**
			 * @brief Returns the fog density.
			 * @return float
			 */
			[[nodiscard]]
			float density () const noexcept;

			/**
			 * @brief Returns the linear mode starting point.
			 * @return float
			 */
			[[nodiscard]]
			float linearStart () const noexcept;

			/**
			 * @brief Returns the linear mode ending point.
			 * @return float
			 */
			[[nodiscard]]
			float linearEnd () const noexcept;

			/**
			 *
			 * @return const Libraries::PixelFactory::Color< float > &
			 */
			[[nodiscard]]
			const Libs::PixelFactory::Color< float > & color () const noexcept;

		private:

			static constexpr auto FogDensity = "em_FogDensity";
			static constexpr auto FogStart = "em_FogStart";
			static constexpr auto FogEnd = "em_FogEnd";
			static constexpr auto FogColor = "em_FogColor";

			static constexpr auto FogVertexDistance = "em_FogVertexDistance";

			FogMode m_mode;
			/* Only for linear mode. */
			float m_linearStart{0.0F};
			float m_linearEnd{1.0F};
			/* Only for exponential mode */
			float m_density{1.0F};
			/* Fog color */
			Libs::PixelFactory::Color< float > m_color{};
			/* Per vertex or pixel */
			bool m_highQuality{true};
			bool m_useBackgroundColor{false};
	};
}
