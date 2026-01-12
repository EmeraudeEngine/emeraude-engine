/*
 * src/Scenes/Effect/ChromaticAberrationLensEffect.hpp
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

/* Local inclusions for usages. */
#include "Libs/Math/Vector.hpp"

namespace EmEn::Scenes
{
	/**
	 * @brief The chromatic aberration lens effect class.
	 * @extends EmEn::FramebufferEffectInterface This is a framebuffer effect.
	 */
	class ChromaticAberrationLensEffect final : public Saphir::FramebufferEffectInterface
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"ChromaticAberrationLensEffect"};

			/**
			 * @brief Constructs a chromatic aberration lens effect
			 * @param xScale Difference on X.
			 * @param yScale Difference on Y.
			 */
			ChromaticAberrationLensEffect (float xScale, float yScale) noexcept;

			/** @copydoc EmEn::Saphir::FramebufferEffectInterface::generateFragmentShaderCode() */
			[[nodiscard]]
			bool generateFragmentShaderCode (Saphir::Generator::Abstract & generator, Saphir::FragmentShader & fragmentShader) const noexcept override;

		private:

			/** @copydoc EmEn::Saphir::FramebufferEffectInterface::overrideFragmentFetching() */
			[[nodiscard]]
			bool overrideFragmentFetching () const noexcept override;

			static constexpr auto RedOffset{"em_RedOffset"};
			static constexpr auto GreenOffset{"em_GreenOffset"};
			static constexpr auto BlueOffset{"em_BlueOffset"};

			mutable uint32_t m_samples{0};
			Libs::Math::Vector< 2, float > m_redOffset{};
			Libs::Math::Vector< 2, float > m_greenOffset{};
			Libs::Math::Vector< 2, float > m_blueOffset{};
	};
}
