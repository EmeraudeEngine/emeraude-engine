/*
 * src/Scenes/Effect/FilmGrainLensEffect.hpp
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

namespace EmEn::Scenes
{
	/**
	 * @brief The film grain lens effect class;
	 * @extends EmEn::FramebufferEffectInterface This is a framebuffer effect.
	 */
	class FilmGrainLensEffect final : public Saphir::FramebufferEffectInterface
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"FilmGrainLensEffect"};

			/**
			 * @brief Constructs a film grain lens effect.
			 */
			FilmGrainLensEffect () noexcept = default;

			/** @copydoc EmEn::Saphir::FramebufferEffectInterface::generateFragmentShaderCode() */
			[[nodiscard]]
			bool generateFragmentShaderCode (Saphir::Generator::Abstract & generator, Saphir::FragmentShader & fragmentShader) const noexcept override;

		private:

			static constexpr auto RandomA{"em_RandomA"};
			static constexpr auto RandomB{"em_RandomB"};
			static constexpr auto RandomC{"em_RandomC"};

			/** @copydoc EmEn::Saphir::FramebufferEffectInterface::requestScreenSize() */
			[[nodiscard]]
			bool requestScreenSize () const noexcept override;
	};
}
