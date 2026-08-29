/*
 * src/Animations/PlaybackWrap.hpp
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

/* STL inclusions. */
#include <cstdint>

namespace EmEn::Animations
{
	/**
	 * @brief Playback mode for wrap behavior when time exceeds clip duration.
	 * @note Shared by every clip evaluator of the engine — the skeletal animator and the node
	 * animation component — so a caller states the wrap the same way whatever it drives.
	 */
	enum class EMEN_API PlaybackWrap : uint8_t
	{
		Once,	/**< Play once and stop at the last frame. */
		Loop,	/**< Loop back to the beginning. */
		PingPong /**< Alternate forward/backward. */
	};
}
