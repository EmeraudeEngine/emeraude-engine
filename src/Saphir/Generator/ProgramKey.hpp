/*
 * src/Saphir/Generator/ProgramKey.hpp
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

/* Local inclusions for usages. */
#include "ExactKey.hpp"

namespace EmEn::Saphir::Generator
{
	/**
	 * @brief The EXACT identity of a generated shader program, the key of the renderer's program cache
	 * (Graphics::Renderer::findCachedProgram()): the generator's class identifier first, then every input that changes
	 * the generated GLSL (Base::ExactKey: the bytes of every input, compared exactly; the hash only places it).
	 * @note ⚠️ Until 2026-10-31 that cache was keyed by a bare size_t, a hash of these same inputs with no equality
	 * behind it: two programs whose hashes met shared one entry, and the first one generated was served for both — a
	 * spot-shadow program (sampler2D) drawn for the sun's cascaded pass (a 2D-array view), intermittently, since the hash
	 * folds in a render pass handle (an address, different every run). docs/caution-points.md.
	 */
	using ProgramKey = Base::ExactKey;
}
