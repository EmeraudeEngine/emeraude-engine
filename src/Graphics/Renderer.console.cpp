/*
 * src/Graphics/Renderer.console.cpp
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

#include "Renderer.hpp"

/* STL inclusions. */
#include <chrono>
#include <cmath>
#include <iomanip>
#include <sstream>
#include <string>
#include <string_view>

/* Local inclusions. */
#include "FileSystem.hpp"
#include "IO/IO.hpp"
#include "PixelFactory/FileIO.hpp"
#include "MDI/BatchBuilder.hpp"
#include "OceanWaves.hpp"
#include "OverflowCensus.hpp"
#include "PrimaryServices.hpp"
#include "VideoFrameConverter.hpp"
#include "Vulkan/Instance.hpp"
#include "Vulkan/SwapChain.hpp"
#include "Vulkan/VideoEncoderH265.hpp"

namespace EmEn::Graphics
{
	using namespace Base;

	void
	Renderer::onRegisterToConsole () noexcept
	{
		this->bindCommand("screenshot", "Captures the next presented frame (UI included) and saves it as <unix seconds>.png in the captures directory.", [this] () {
			/* The next presented frame, copied inside that frame (FrameCapture): exactly what reaches the screen. */
			const auto result = this->captureFrames(1, false, std::chrono::seconds{5});

			if ( !result.success || result.files.empty() )
			{
				return Console::CommandResult::error("Screenshot failed: " + result.error);
			}

			std::stringstream message;
			message << "Screenshot saved: " << result.files.front();

			/* An IMAGE result: the MCP channel shows it (reduced), the text console prints this message. */
			return Console::CommandResult::image(result.files.front(), "image/png", message.str());
		});

		this->bindCommand("temporalCapture", "DEV: captures N consecutive presented frames as <unix seconds>-<n>.png, plus <unix seconds>.json with the per-frame metadata (jitter, camera, exposure, timing).",
			{
				{"frameCount", "Number of consecutive frames to capture (at least 1).", static_cast< int32_t >(FrameCapture::DefaultTemporalFrameCount)}
			},
			[this] (int32_t requestedFrameCount) {
				/* DEV tool (owner, 2026-09-23): N CONSECUTIVE presented frames, to see shimmer and temporal artefacts on a
				 * still camera. The frames pay one GPU copy each and nothing on the CPU until the last one is done. */
				if ( requestedFrameCount < 1 )
				{
					return Console::CommandResult::error("temporalCapture(): frameCount must be at least 1.");
				}

				const auto frameCount = static_cast< uint32_t >(requestedFrameCount);

				/* The frames themselves, then one PNG per frame on the thread pool: generous, a slow machine runs at 20 fps. */
				const auto timeout = std::chrono::seconds{10} + std::chrono::milliseconds{500} * frameCount;
				const auto result = this->captureFrames(frameCount, true, std::chrono::duration_cast< std::chrono::milliseconds >(timeout));

				if ( !result.success )
				{
					return Console::CommandResult::error("Temporal capture failed: " + result.error);
				}

				std::stringstream message;
				message << "Temporal capture of " << frameCount << " consecutive frames saved:";

				for ( const auto & file : result.files )
				{
					message << "\n  " << file.string();
				}

				return Console::CommandResult::success(message.str());
			});

		this->bindCommand("testVideoFrameConverter", "Self-tests the GPU BGRA->I420 converter against the CPU reference (hardware video encode path).", [this] () {
			/* Self-test of the GPU BGRA->I420 converter (hardware video-encode path):
			 * converts a procedural pattern and compares byte-for-byte against the CPU
			 * reference running the same shared BT.709 integer math. */
			VideoFrameConverter converter{this->device(), this->shaderManager()};

			if ( !converter.create(1280U, 720U) )
			{
				return Console::CommandResult::error("Unable to create the video frame converter !");
			}

			uint64_t mismatchedBytes = 0;

			if ( !converter.selfTest(mismatchedBytes) )
			{
				return Console::CommandResult::error("GPU/CPU conversion mismatch (" + std::to_string(mismatchedBytes) + " bytes differ) !");
			}

			return Console::CommandResult::success("GPU BGRA->I420 conversion matches the CPU reference byte-for-byte (1280x720, BT.709 integer path).");
		});

		this->bindCommand("testOceanWaves", "Self-tests the GPU FFT ocean waves (JONSWAP spectrum, 3 cascades of 256²) against a CPU reference FFT of the same spectrum.", [this] () {
			/* Engine item ocean-fft-surface, step 1: the GPU evolution + Stockham FFT + resolve, read back and compared
			 * field by field with a double-precision Cooley-Tukey reference. */
			OceanWaves waves{this->device(), this->shaderManager()};

			if ( !waves.create(OceanWaveParameters{}) )
			{
				return Console::CommandResult::error("Unable to create the ocean wave generator !");
			}

			std::string report;

			if ( !waves.selfTest(3.7F, report) )
			{
				return Console::CommandResult::error("GPU/CPU ocean mismatch: " + report);
			}

			return Console::CommandResult::success("GPU ocean waves match the CPU reference: " + report);
		});

		this->bindCommand("testVideoEncoderH265", "End-to-end hardware H.265 encode self-test: writes an Annex-B .h265 stream in the captures directory.", [this] () {
			/* End-to-end hardware encode self-test: converts the procedural pattern once,
			 * then encodes 90 frames (3 GOPs) through the Vulkan Video session and writes
			 * an Annex-B .h265 elementary stream — decode it with ffprobe/ffplay. */
			if ( !this->device()->videoEncodeH265Enabled() )
			{
				return Console::CommandResult::error("No H.265 hardware encode support on this device.");
			}

			VideoFrameConverter converter{this->device(), this->shaderManager()};

			if ( !converter.create(2880U, 1620U) )
			{
				return Console::CommandResult::error("Unable to create the video frame converter !");
			}

			uint64_t mismatchedBytes = 0;

			if ( !converter.selfTest(mismatchedBytes) )
			{
				return Console::CommandResult::error("The converter self-test failed !");
			}

			Vulkan::VideoEncoderH265 encoder{this->device()};
			Vulkan::VideoEncoderH265::Settings settings{};
			settings.width = 2880;
			settings.height = 1620;
			settings.frameRate = 30;
			settings.averageBitrateKbps = 8000;
			settings.maximumBitrateKbps = 12000;
			settings.idrPeriod = 1; /* TEMP: all-intra bench. */

			if ( !encoder.create(settings) )
			{
				return Console::CommandResult::error("Unable to create the H.265 hardware encoder !");
			}

			const auto captureDirectory = m_primaryServices.fileSystem().userDataDirectory("captures");
			const auto filepath = captureDirectory / "hw-encode-test.h265";

			std::ofstream stream{filepath, std::ios::binary | std::ios::trunc};

			if ( !stream.is_open() )
			{
				std::stringstream message;
				message << "Unable to open " << filepath << " !";

				return Console::CommandResult::error(message.str());
			}

			const auto & header = encoder.headerBytes();
			stream.write(reinterpret_cast< const char * >(header.data()), static_cast< std::streamsize >(header.size()));

			uint64_t totalBytes = header.size();
			uint32_t idrCount = 0;
			std::vector< uint8_t > packet;

			for ( uint32_t frame = 0; frame < 90; frame++ )
			{
				bool wasIDR = false;

				if ( !encoder.encodeFrame(*converter.lumaImage(), *converter.chromaImage(), packet, wasIDR) )
				{
					return Console::CommandResult::error("Encode failed at frame " + std::to_string(frame) + " !");
				}

				stream.write(reinterpret_cast< const char * >(packet.data()), static_cast< std::streamsize >(packet.size()));

				totalBytes += packet.size();

				if ( wasIDR )
				{
					idrCount++;
				}
			}

			std::stringstream message;
			message << "90 frames hardware-encoded (" << totalBytes << " bytes, " << idrCount << " IDR) -> " << filepath;

			return Console::CommandResult::success(message.str());
		});

		this->bindCommand("getGPUTimings", "Returns the per-pass GPU timings (timestamp queries), or clears their statistics.",
			{
				{"action", "'reset' clears the accumulated statistics (averages, maxima) instead of reading them."}
			},
			[this] (const std::optional< std::string > & action) {
				if ( action.has_value() && *action != "reset" )
				{
					return Console::CommandResult::error("getGPUTimings(): unknown action '" + *action + "' (only 'reset').");
				}

				if ( m_GPUProfiler == nullptr )
				{
					return Console::CommandResult::warning("The GPU profiler is disabled. Set 'Core/Graphics/GPUProfiler/Enabled' to true and restart.");
				}

				if ( action.has_value() )
				{
					m_GPUProfiler->resetStatistics();

					return Console::CommandResult::success("GPU timing statistics reset.");
				}

				const auto timings = m_GPUProfiler->snapshot();

				if ( timings.empty() )
				{
					return Console::CommandResult::info("No GPU timings harvested yet (needs a few rendered frames).");
				}

				/* Display order = command stream order; the depth column indents nested scopes.
				 * The averages are ~60-frame rolling values (see GPUProfiler::AverageAlpha). */
				std::stringstream table;
				table << "GPU timings (ms):" "\n";
				table << std::fixed << std::setprecision(3);

				for ( const auto & timing : timings )
				{
					table << "  ";

					for ( uint32_t level = 0; level < timing.depth; level++ )
					{
						table << "  ";
					}

					table << std::left << std::setw(static_cast< int >(40 - timing.depth * 2)) << timing.label
						<< " last " << std::setw(8) << timing.lastMS
						<< " avg " << std::setw(8) << timing.averageMS
						<< " max " << std::setw(8) << timing.maximumMS
						<< " samples " << timing.sampleCount << "\n";
				}

				return Console::CommandResult::info(table.str());
			});

		/* ---- The overflow census (scene-colour pre-exposure, step B1a). ---- */

		this->bindCommand("setOverflowCensus", "Arms or disarms the overflow census: per-frame NaN / Inf / fp16-ceiling texel counts of the scene-radiance images.",
			{
				{"armed", "1 (true) arms the census from the next rendered frame, 0 (false) disarms it."}
			},
			[this] (bool armed) {
				auto * census = m_postProcessor.overflowCensus();

				if ( census == nullptr || !census->available() )
				{
					return Console::CommandResult::error("The overflow census is unavailable: not created yet, or its creation failed (see the log).");
				}

				/* Latched by the render thread at the next rendered frame, like any console switch. */
				census->setArmed(armed);

				if ( armed )
				{
					return Console::CommandResult::success("Overflow census available and ARMED from the next rendered frame (the HDR chain only). Read it with getFrameDiagnostics() or Core.SceneManagerService.PostProcess.getStatus(); testOverflowCensus() must have answered PASS on this machine for a count to mean anything.");
				}

				return Console::CommandResult::success("Overflow census disarmed from the next rendered frame (its last counts and its window are kept).");
			}, Console::CommandHint::Idempotent);

		this->bindCommand("resetOverflowCensus", "Opens a new statistics window of the overflow census (maxima, frames with an overflow), from the next rendered frame.", [this] () {
			auto * census = m_postProcessor.overflowCensus();

			if ( census == nullptr || !census->available() )
			{
				return Console::CommandResult::error("The overflow census is unavailable: not created yet, or its creation failed (see the log).");
			}

			const auto startsAfter = census->resetWindow();

			std::stringstream message;
			message <<
				"Overflow census window reset: it holds the frames rendered after frame " << startsAfter << "." <<
				( census->armed() ? "" : " The census is disarmed: nothing is counted until setOverflowCensus(1)." );

			return Console::CommandResult::success(message.str());
		});

		this->bindCommand("testOverflowCensus", "Positive control of the overflow census: counts a 16x16 image of raw half bit patterns in the next frame (blocks at most 3 s) and answers PASS or FAIL with the expected and measured tuples.", [this] () {
			auto * census = m_postProcessor.overflowCensus();

			if ( census == nullptr || !census->available() )
			{
				return Console::CommandResult::error("FAIL: the overflow census is unavailable (not created yet, or its creation failed: see the log).");
			}

			/* Blocks this (main) thread until a frame counted the self-test image: the render thread produces it. */
			const auto result = census->runSelfTest(std::chrono::seconds{3});

			if ( !result.ran )
			{
				return Console::CommandResult::error("FAIL: no frame counted the self-test image within 3 s. A scene must be rendering through the HDR post-process chain (the census counts nothing else).");
			}

			const auto & measured = result.measured;

			std::stringstream message;
			message << std::setprecision(9);

			if ( result.passed )
			{
				message << "PASS (frame " << result.frameSerial << "): tested " << measured.tested << ", NaN " << measured.nanTexels << ", Inf " << measured.infTexels << ", ceiling " << measured.ceilingTexels << ", peak " << measured.peakFinite << ". The census sees every class on this machine.";

				return Console::CommandResult::success(message.str());
			}

			message <<
				"FAIL (frame " << result.frameSerial << "): expected tested " << OverflowCensus::SelfTestExpectedTested << ", NaN " << OverflowCensus::SelfTestExpectedNaN << ", Inf " << OverflowCensus::SelfTestExpectedInf << ", ceiling " << OverflowCensus::SelfTestExpectedCeiling << ", peak 65472; "
				"measured tested " << measured.tested << ", NaN " << measured.nanTexels << ", Inf " << measured.infTexels << ", ceiling " << measured.ceilingTexels << ", peak " << measured.peakFinite << ". "
				"The census is BLIND on this machine: take no baseline here.";

			return Console::CommandResult::error(message.str());
		});

		this->bindCommand("getFrameDiagnostics", "Returns the diagnostics of the last rendered frame as JSON: the tone mapper metering and the overflow census (last counted frame, statistics window, self-test).", [this] () {
			const auto diagnostics = this->frameDiagnostics();

			/* JSON has no NaN nor infinity: a non-finite float is written as null. */
			const auto number = [] (float value) {
				if ( !std::isfinite(value) )
				{
					return std::string{"null"};
				}

				std::stringstream text;
				text << std::setprecision(9) << value;

				return text.str();
			};

			/* The names are engine identifiers; escape what JSON requires anyway. */
			const auto name = [] (const OverflowCensusChannelName & value) {
				std::string text{"\""};

				for ( const char character : std::string_view{value.data()} )
				{
					if ( character == '"' || character == '\\' )
					{
						text += '\\';
					}

					text += character;
				}

				return text + "\"";
			};

			const auto boolean = [] (bool value) {
				return value ? "true" : "false";
			};

			const auto & metering = diagnostics.metering;
			const auto & census = diagnostics.census;
			const auto & latest = census.latest;
			const auto & window = census.window;
			const auto & selfTest = census.selfTest;

			std::stringstream json;

			json << R"({"renderedFrame":)" << diagnostics.renderedFrame << ",";

			json << R"("metering":{)" <<
				R"("toneMapper":)" << boolean(metering.toneMapper) << "," <<
				R"("auto":)" << boolean(metering.autoExposure) << "," <<
				R"("meteredLuminance":)" << number(metering.meteredLuminance) << "," <<
				R"("meteredSensitivity":)" << number(metering.meteredSensitivity) << "," <<
				R"("meteredShutterSpeed":)" << number(metering.meteredShutterSpeed) << "," <<
				R"("rejected":)" << metering.rejectedCount << "},";

			json << R"("census":{)" <<
				R"("available":)" << boolean(census.available) << "," <<
				R"("armed":)" << boolean(census.armed) << "," <<
				R"("valid":)" << boolean(latest.valid) << "," <<
				R"("frame":)" << latest.frameSerial << "," <<
				R"("toneMapped":)" << boolean(latest.toneMapped) << "," <<
				R"("channels":[)";

			for ( uint32_t index = 0; index < latest.channelCount; ++index )
			{
				const auto & channel = latest.channels[index];

				json << ( index > 0 ? "," : "" ) << "{" <<
					R"("name":)" << name(channel.name) << "," <<
					R"("tested":)" << channel.tested << "," <<
					R"("expected":)" << channel.expectedTexels << "," <<
					R"("nan":)" << channel.nanTexels << "," <<
					R"("inf":)" << channel.infTexels << "," <<
					R"("ceiling":)" << channel.ceilingTexels << "," <<
					R"("overflow":)" << (static_cast< uint64_t >(channel.nanTexels) + channel.infTexels + channel.ceilingTexels) << "," <<
					R"("peakFinite":)" << number(channel.peakFinite) << "," <<
					R"("invalid":)" << boolean(channel.invalid) << "}";
			}

			json << "]," << R"("window":{)" <<
				R"("startsAfterFrame":)" << window.startsAfterFrame << "," <<
				R"("firstFrame":)" << window.firstFrame << "," <<
				R"("lastFrame":)" << window.lastFrame << "," <<
				R"("frames":)" << window.framesCounted << "," <<
				R"("framesWithOverflow":)" << window.framesWithOverflow << "," <<
				R"("staleSlots":)" << window.staleSlots << "," <<
				R"("channels":[)";

			for ( uint32_t index = 0; index < window.channelCount; ++index )
			{
				const auto & entry = window.channels[index];

				json << ( index > 0 ? "," : "" ) << "{" <<
					R"("name":)" << name(entry.name) << "," <<
					R"("framesCounted":)" << entry.framesCounted << "," <<
					R"("framesOverflowing":)" << entry.framesOverflowing << "," <<
					R"("maxOverflow":)" << entry.maxOverflow << "," <<
					R"("maxOverflowFrame":)" << entry.maxOverflowFrame << "," <<
					R"("maxPeakFinite":)" << number(entry.maxPeakFinite) << "}";
			}

			json << "]}," << R"("selfTest":{)" <<
				R"("ran":)" << boolean(selfTest.ran) << "," <<
				R"("passed":)" << boolean(selfTest.passed) << "," <<
				R"("frame":)" << selfTest.frameSerial << "," <<
				R"("tested":)" << selfTest.measured.tested << "," <<
				R"("nan":)" << selfTest.measured.nanTexels << "," <<
				R"("inf":)" << selfTest.measured.infTexels << "," <<
				R"("ceiling":)" << selfTest.measured.ceilingTexels << "," <<
				R"("peakFinite":)" << number(selfTest.measured.peakFinite) << "}}}";

			return Console::CommandResult::json(json.str());
		}, Console::CommandHint::ReadOnly);

		this->bindCommand("triggerRenderDocCapture", "Triggers a RenderDoc frame capture (requires launch under renderdoccmd).",
			{
				{"frameCount", "Number of consecutive frames to capture (at least 1): capturing past the first frame catches per-frame / state-tracking bugs.", 1}
			},
			[this] (int32_t requestedFrameCount) {
				if ( requestedFrameCount < 1 )
				{
					return Console::CommandResult::error("triggerRenderDocCapture(): frameCount must be at least 1.");
				}

				auto & renderDoc = m_vulkanInstance.renderDocCapture();

				if ( !renderDoc.isAvailable() )
				{
					return Console::CommandResult::error("RenderDoc is not available (launch the app under renderdoccmd to inject it).");
				}

				/* Define WHERE captures are written. RenderDoc is never told otherwise, so without this
				 * the .rdc lands in an undefined default and the autonomous capture workflow produces
				 * nothing findable. Point it at the user-data RenderDoc directory. */
				auto captureDirectory = m_primaryServices.fileSystem().userDataDirectory("RenderDoc");

				if ( IO::writable(captureDirectory) )
				{
					renderDoc.setCaptureFilePath(captureDirectory.append("capture").string());
				}

				const auto frameCount = static_cast< uint32_t >(requestedFrameCount);

				if ( frameCount > 1U )
				{
					renderDoc.triggerMultiFrameCapture(frameCount);

					return Console::CommandResult::success("RenderDoc: " + std::to_string(frameCount) + " consecutive frame captures triggered.");
				}

				renderDoc.triggerCapture();

				return Console::CommandResult::success("RenderDoc: frame capture triggered (captured on the next present).");
			});

		this->bindCommand("getStatus", "Returns renderer statistics (FPS, frame time, resolution).", [this] () {
			const auto & stats = this->statistics();

			std::stringstream status;
			status << "Renderer status:" "\n";
			status << "  FPS: " << stats.executionsPerSecond() << " (avg: " << stats.averageExecutionsPerSecond() << ")" "\n";
			status << "  Frame time: " << stats.duration() << " ms (avg: " << stats.averageDuration() << " ms)" "\n";
			status << "  Frames in flight: " << this->framesInFlight() << "\n";

			if ( m_swapChain != nullptr )
			{
				const auto extent = m_swapChain->extent();
				status << "  Resolution: " << extent.width << "x" << extent.height << "\n";
			}

			return Console::CommandResult::info(status.str());
		}, Console::CommandHint::ReadOnly);

		this->bindCommand("getMDIStats", "Returns Multi-Draw Indirect statistics from the last frame as JSON (batched/fallback/skipped counts, batched ratio %).", [this] () {
			std::stringstream json;

			if ( !m_MDIEnabled || m_MDIBatchBuilder == nullptr )
			{
				json << R"({"enabled":false,"reason":)"
					<< ( m_device == nullptr ? R"("no device")" : R"("setting disabled or hardware unsupported")" )
					<< "}";

				return Console::CommandResult::json(json.str());
			}

			const auto * builder = m_MDIBatchBuilder.get();
			const auto batched = builder->totalDrawsBatched();
			const auto fallback = builder->totalFallbackDraws();
			const auto skipped = builder->skippedCount();
			const auto total = batched + fallback;
			const double batchedRatio = total > 0 ? 100.0 * static_cast< double >(batched) / static_cast< double >(total) : 0.0;

			json << R"({"enabled":true,)"
				<< R"("ready":)" << ( builder->isReady() ? "true" : "false" ) << ","
				<< R"("totalDrawsBatched":)" << batched << ","
				<< R"("totalFallbackDraws":)" << fallback << ","
				<< R"("skippedCount":)" << skipped << ","
				<< R"("totalDraws":)" << total << ","
				<< R"("batchedRatio":)" << std::fixed << std::setprecision(2) << batchedRatio
				<< "}";

			return Console::CommandResult::json(json.str());
		}, Console::CommandHint::ReadOnly);
	}
}
