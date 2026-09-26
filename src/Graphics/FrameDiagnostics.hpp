/*
 * src/Graphics/FrameDiagnostics.hpp
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
#include <array>
#include <cstdint>

/*
 * The per-frame DIAGNOSTICS the render thread publishes for the console (scene-colour pre-exposure,
 * step B1, 2026-09-26): the overflow census and the tone mapper's metering.
 *
 * ⚠️ PLAIN DATA ONLY — no function body, no export macro, no pointer. They are composed ONCE per
 * rendered frame on the render thread (Renderer::publishFrameDiagnostics()) and COPIED BY VALUE into
 * two boards behind a mutex (the renderer's and the active stack's), so a reader on the main thread
 * never touches render-thread state. A name is therefore a copied array, never a pointer into an
 * effect that the camera sync may retire the next frame. And a class holding only inline bodies
 * under EMEN_API is the MSVC LNK2019 trap: none are needed here.
 */
namespace EmEn::Graphics
{
	/** @brief The most channels one census frame counts (OverflowCensus::MaxChannels is this value). */
	constexpr uint32_t OverflowCensusMaxChannels{8};

	/** @brief A census channel name, copied, zero-terminated. */
	using OverflowCensusChannelName = std::array< char, 24 >;

	/**
	 * @brief One image, counted in one frame.
	 * @note "Overflow" is `nanTexels + infTexels + ceilingTexels`: a texel whose worst RGB channel is
	 * NaN, infinite, or finite at/above 65 504, the largest finite binary16. Alpha is never counted.
	 */
	struct OverflowCensusChannel
	{
		OverflowCensusChannelName name{};
		/** @brief The texels the dispatch had to test (the image extent, the atlas interior). */
		uint32_t expectedTexels{0};
		/** @brief The texels the GPU actually tested. Differing from expectedTexels is a census defect. */
		uint32_t tested{0};
		uint32_t nanTexels{0};
		uint32_t infTexels{0};
		/** @brief Finite texels at/above the binary16 ceiling: an overflow rounded to the largest finite value instead of +Inf. */
		uint32_t ceilingTexels{0};
		/** @brief The largest finite magnitude below the ceiling (the worst RGB channel), 0 when none. */
		float peakFinite{0.0F};
		/** @brief tested != expectedTexels: the counts of this channel cannot be trusted. */
		bool invalid{false};
	};

	/**
	 * @brief Every channel counted in one frame.
	 */
	struct OverflowCensusReport
	{
		std::array< OverflowCensusChannel, OverflowCensusMaxChannels > channels{};
		/** @brief The rendered-frame serial the counts belong to (Renderer::renderedFrameSerial()). */
		uint64_t frameSerial{0};
		uint32_t channelCount{0};
		/** @brief Whether the ToneMapInput channel is the tone mapper's input (false: the chain output, no tone mapper ran). */
		bool toneMapped{false};
		/** @brief Whether at least one frame was counted since the census exists. */
		bool valid{false};
	};

	/**
	 * @brief One channel over the statistics window.
	 */
	struct OverflowCensusWindowChannel
	{
		OverflowCensusChannelName name{};
		/** @brief The frame of the largest overflow count, 0 when the channel never overflowed. */
		uint64_t maxOverflowFrame{0};
		/** @brief The frames in which the channel was counted. */
		uint32_t framesCounted{0};
		uint32_t framesOverflowing{0};
		uint32_t maxOverflow{0};
		float maxPeakFinite{0.0F};
	};

	/**
	 * @brief The statistics window, opened at the census creation or by Core.RendererService.resetOverflowCensus().
	 */
	struct OverflowCensusWindow
	{
		std::array< OverflowCensusWindowChannel, OverflowCensusMaxChannels > channels{};
		/** @brief Frames at or before this serial belong to the previous window, even when harvested after the reset. */
		uint64_t startsAfterFrame{0};
		uint64_t firstFrame{0};
		uint64_t lastFrame{0};
		uint64_t framesCounted{0};
		uint64_t framesWithOverflow{0};
		/** @brief Slots recorded but never executed (their serial header did not match): dropped, not counted. */
		uint64_t staleSlots{0};
		uint32_t channelCount{0};
	};

	/**
	 * @brief The outcome of the census positive control (Core.RendererService.testOverflowCensus()).
	 */
	struct OverflowCensusSelfTest
	{
		OverflowCensusChannel measured{};
		uint64_t frameSerial{0};
		/** @brief Whether a frame counted the self-test image. */
		bool ran{false};
		/** @brief Whether the measured tuple is exactly the expected one. */
		bool passed{false};
	};

	/**
	 * @brief What the census can tell at one moment, copied out of its lock.
	 */
	struct OverflowCensusSnapshot
	{
		OverflowCensusReport latest{};
		OverflowCensusWindow window{};
		/** @brief The last positive control of the session (ran = false when none). */
		OverflowCensusSelfTest selfTest{};
		/** @brief Whether the census exists and its GPU resources were created. */
		bool available{false};
		bool armed{false};
	};

	/**
	 * @brief The camera tone mapper's metering, as the render thread reads it.
	 */
	struct MeteringReport
	{
		/** @brief The metered scene average luminance in nits, 0 until a measurement completed. */
		float meteredLuminance{0.0F};
		/** @brief The ISO the auto-exposure landed on, 0 until a measurement completed or in manual exposure. */
		float meteredSensitivity{0.0F};
		/** @brief Measurements the adaptation pass rejected since the tone mapper was created (counted only with a camera). */
		uint32_t rejectedCount{0};
		/** @brief Whether the camera tone mapping exists, is enabled and created. */
		bool toneMapper{false};
		/** @brief Whether the tone mapper meters the frame (auto exposure with its luminance chain). */
		bool autoExposure{false};
	};

	/**
	 * @brief Everything the render thread publishes about one rendered frame.
	 */
	struct FrameDiagnostics
	{
		OverflowCensusSnapshot census{};
		MeteringReport metering{};
		/** @brief The rendered-frame serial of the publication (Renderer::renderedFrameSerial()). */
		uint64_t renderedFrame{0};
	};
}
