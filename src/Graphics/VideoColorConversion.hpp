/*
 * src/Graphics/VideoColorConversion.hpp
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

/* STL inclusions. */
#include <cstdint>
#include <string>

namespace EmEn::Graphics::VideoColor
{
	/* BT.709 limited-range RGB->YCbCr coefficients (Q8 fixed-point).
	 * SINGLE SOURCE OF TRUTH for every conversion path: the CPU scalar/SIMD
	 * converters (Recorder.cpp software VP9 fallback) and the GPU compute
	 * converter (VideoFrameConverter, hardware H.265 path — the GLSL consumes
	 * these values through glslDefines()). HD players assume BT.709; BT.601
	 * coefficients on HD content shift hues on playback.
	 *   Y = ((YCoefR*R + YCoefG*G + YCoefB*B + 128) >> 8) + 16
	 *   U = ((UCoefR*R + UCoefG*G + UCoefB*B + 128) >> 8) + 128
	 *   V = ((VCoefR*R + VCoefG*G + VCoefB*B + 128) >> 8) + 128 */
	constexpr int32_t YCoefR{47};
	constexpr int32_t YCoefG{157};
	constexpr int32_t YCoefB{16};
	constexpr int32_t UCoefR{-26};
	constexpr int32_t UCoefG{-87};
	constexpr int32_t UCoefB{112};
	constexpr int32_t VCoefR{112};
	constexpr int32_t VCoefG{-102};
	constexpr int32_t VCoefB{-10};

	/**
	 * @brief Returns the coefficient set as a GLSL define block, so compute
	 * shaders share the exact integer math of the CPU converters (bit-exact
	 * parity — the GPU output is validated byte-for-byte against the CPU one).
	 * @return std::string
	 */
	[[nodiscard]]
	inline std::string
	glslDefines () noexcept
	{
		return
			"#define Y_COEF_R " + std::to_string(YCoefR) + "\n"
			"#define Y_COEF_G " + std::to_string(YCoefG) + "\n"
			"#define Y_COEF_B " + std::to_string(YCoefB) + "\n"
			"#define U_COEF_R " + std::to_string(UCoefR) + "\n"
			"#define U_COEF_G " + std::to_string(UCoefG) + "\n"
			"#define U_COEF_B " + std::to_string(UCoefB) + "\n"
			"#define V_COEF_R " + std::to_string(VCoefR) + "\n"
			"#define V_COEF_G " + std::to_string(VCoefG) + "\n"
			"#define V_COEF_B " + std::to_string(VCoefB) + "\n";
	}
}
