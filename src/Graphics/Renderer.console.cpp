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
#include <algorithm>
#include <vector>
#include <tuple>
#include <optional>
#include <map>
#include <charconv>
#include <array>
#include <chrono>
#include <cmath>
#include <iomanip>
#include <sstream>
#include <string>
#include <string_view>

/* Local inclusions. */
#include "FastJSON.hpp"
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

	namespace
	{
		/**
		 * @brief Names the usage flags of a buffer or an image (the bits that tell resources apart).
		 * @param image Whether the flags are image usage flags.
		 * @param flags The flags.
		 * @return std::string
		 */
		[[nodiscard]]
		std::string
		usageNames (bool image, uint64_t flags) noexcept
		{
			struct Bit
			{
				uint64_t mask;
				const char * name;
			};

			static constexpr std::array< Bit, 10 > BufferBits{{
				{.mask = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, .name = "VERTEX"},
				{.mask = VK_BUFFER_USAGE_INDEX_BUFFER_BIT, .name = "INDEX"},
				{.mask = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, .name = "UNIFORM"},
				{.mask = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, .name = "STORAGE"},
				{.mask = VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT, .name = "INDIRECT"},
				{.mask = VK_BUFFER_USAGE_TRANSFER_SRC_BIT, .name = "TRANSFER_SRC"},
				{.mask = VK_BUFFER_USAGE_TRANSFER_DST_BIT, .name = "TRANSFER_DST"},
				{.mask = VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, .name = "DEVICE_ADDRESS"},
				{.mask = VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR, .name = "AS_STORAGE"},
				{.mask = VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR, .name = "AS_INPUT"}
			}};

			static constexpr std::array< Bit, 7 > ImageBits{{
				{.mask = VK_IMAGE_USAGE_SAMPLED_BIT, .name = "SAMPLED"},
				{.mask = VK_IMAGE_USAGE_STORAGE_BIT, .name = "STORAGE"},
				{.mask = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, .name = "COLOR_ATTACHMENT"},
				{.mask = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, .name = "DEPTH_STENCIL"},
				{.mask = VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT, .name = "INPUT_ATTACHMENT"},
				{.mask = VK_IMAGE_USAGE_TRANSFER_SRC_BIT, .name = "TRANSFER_SRC"},
				{.mask = VK_IMAGE_USAGE_TRANSFER_DST_BIT, .name = "TRANSFER_DST"}
			}};

			std::string names;

			const auto append = [&names, flags] (const auto & bits) {
				for ( const auto & bit : bits )
				{
					if ( (flags & bit.mask) != 0 )
					{
						if ( !names.empty() )
						{
							names += '|';
						}

						names += bit.name;
					}
				}
			};

			if ( image )
			{
				append(ImageBits);
			}
			else
			{
				append(BufferBits);
			}

			return names.empty() ? std::string{"NONE"} : names;
		}

		/** @brief Allocations of one heap, kind and usage. */
		struct AllocationGroup
		{
			uint64_t bytes{0};
			uint64_t largest{0};
			size_t count{0};
		};

		/**
		 * @brief Walks VMA's detailed statistics and sums the allocations by (heap, kind, usage).
		 * @note Any object carrying "Type" (a string other than FREE), "Size" and "Usage" is an allocation; the memory
		 * type comes from the enclosing "Type <n>" key of the pools. Defensive: an unexpected shape is skipped.
		 * @param node The JSON node.
		 * @param memoryType The memory type of the enclosing pool, when known.
		 * @param device The device (memory type → heap).
		 * @param groups The groups [in/out].
		 * @param depth The recursion depth (bounded).
		 * @return void
		 */
		void
		collectAllocations (const Json::Value & node, std::optional< uint32_t > memoryType, const Vulkan::Device & device, std::map< std::tuple< uint32_t, std::string, std::string >, AllocationGroup > & groups, int depth) noexcept
		{
			if ( depth > 16 )
			{
				return;
			}

			if ( node.isArray() )
			{
				for ( const auto & child : node )
				{
					collectAllocations(child, memoryType, device, groups, depth + 1);
				}

				return;
			}

			if ( !node.isObject() )
			{
				return;
			}

			const auto type = FastJSON::getValue< std::string >(node, "Type");
			const auto size = FastJSON::getValue< uint64_t >(node, "Size");
			const auto usage = FastJSON::getValue< uint64_t >(node, "Usage");

			if ( type && size && usage && memoryType )
			{
				if ( *type != "FREE" )
				{
					const auto isImage = type->starts_with("IMAGE");
					const auto heap = device.memoryTypeHeapIndex(*memoryType).value_or(UINT32_MAX);
					auto & group = groups[{heap, isImage ? std::string{"IMAGE"} : *type, usageNames(isImage, *usage)}];

					group.bytes += *size;
					group.largest = std::max(group.largest, *size);
					++group.count;
				}

				return;
			}

			for ( const auto & key : node.getMemberNames() )
			{
				auto childType = memoryType;

				/* "Type 3": the pools of memory type 3. */
				if ( key.starts_with("Type ") )
				{
					uint32_t index = 0;

					if ( const auto [end, error] = std::from_chars(key.data() + 5, key.data() + key.size(), index); error == std::errc{} && end == key.data() + key.size() )
					{
						childType = index;
					}
				}

				collectAllocations(node[key], childType, device, groups, depth + 1);
			}
		}
	}

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

					/* The label column shrinks by the indent; a nesting deeper than 20 levels gets no padding, not a wrapped width. */
					table << std::left << std::setw(static_cast< int >(40U - (std::min(timing.depth, 20U) * 2U))) << timing.label
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

		this->bindCommand("setPostProcessorEnabled", "Allows or forbids post-processing (the master switch, PostProcessor::enable()). Forbidden, every frame takes the DIRECT swap-chain path: no scene target, no exposure nor tone mapping (a photometric frame clips to white) — a diagnostic of that path, not an A/B of the effects (use PostProcess.bypassSceneEffects for that).",
			{
				{"enabled", "1 (true) allows post-processing (the default), 0 (false) forbids it."}
			},
			[this] (bool enabled) {
				/* Read by the render thread at its next frame. */
				m_postProcessor.enable(enabled);

				return Console::CommandResult::success(enabled ?
					"Post-processing allowed from the next frame (taken when the active scene has effects to run)." :
					"Post-processing forbidden from the next frame: every frame takes the direct swap-chain path.");
			}, Console::CommandHint::Idempotent);

		this->bindCommand("setDeferredPunctualLights", "Switches the deferred resolve of the unshadowed, unprojected point and spot lights on or off (DeferredLightResolve; the settings key Core/Graphics/DeferredPunctualLights/Enabled at launch). Off, every light gets its forward pass per batch again — the A/B of the two paths, same frame, same pose.",
			{
				{"enabled", "1 (true) resolves them deferred (the default), 0 (false) restores the forward passes."}
			},
			[this] (bool enabled) {
				/* Read by the render thread at its next frame: the snapshot is taken per frame. */
				this->enableDeferredPunctualLights(enabled);

				return Console::CommandResult::success(enabled ?
					"Deferred punctual lights ON from the next frame (the GPU profiler shows the 'DeferredLights' scope when a light is resolved)." :
					"Deferred punctual lights OFF from the next frame: every point and spot light is drawn by forward passes.");
			}, Console::CommandHint::Idempotent);

		this->bindCommand("getDeferredLightStatistics", "Returns what the last frame did with the unshadowed, unprojected point and spot lights (DeferredLightResolve): how many were eligible, how many missed the camera frustum (drawn by nobody), how many the resolve shaded (at most DeferredLightResolve::MaxLights = 1024, the closest to the camera, tile-culled), and how many visible ones were left to the forward passes.", [this] () {
			const auto statistics = m_deferredLightResolve.statistics();

			std::stringstream message;
			message <<
				"Deferred punctual lights (last prepared frame): " << statistics.eligible << " eligible, " <<
				statistics.invisible << " outside the frustum, " <<
				statistics.resolved << " resolved (max " << DeferredLightResolve::MaxLights << "), " <<
				statistics.forward << " left to the forward passes." <<
				( m_deferredPunctualLightsEnabled.load(std::memory_order_relaxed) ? "" : " The resolve is switched OFF: these counts are from before." );

			return Console::CommandResult::success(message.str());
		});

		this->bindCommand("setDeferredLightTileCulling", "Switches the tile culling of the deferred resolve on or off (DeferredLightResolve, 16 x 16 tiles). Off, every tile holds every resolved light: the per-pixel loop of before. The exactness A/B — both frames must be bit-identical, only the DeferredLights time differs.",
			{
				{"enabled", "1 (true) culls per tile (the default), 0 (false) gives every tile every light."}
			},
			[this] (bool enabled) {
				m_deferredLightResolve.enableTileCulling(enabled);

				return Console::CommandResult::success(enabled ?
					"Deferred light tile culling ON from the next frame." :
					"Deferred light tile culling OFF from the next frame: every tile holds every resolved light.");
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
				{"frameCount", "Number of consecutive frames to capture (1 to 100): capturing past the first frame catches per-frame / state-tracking bugs.", 1}
			},
			[this] (int32_t requestedFrameCount) {
				/* NOTE: Owner ruling (2026-10-01): longer than 100 frames is not a debugging session. */
				constexpr int32_t MaxFrameCount{100};

				if ( requestedFrameCount < 1 || requestedFrameCount > MaxFrameCount )
				{
					return Console::CommandResult::error("triggerRenderDocCapture(): frameCount must be from 1 to " + std::to_string(MaxFrameCount) + ".");
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

		this->bindCommand("getGPUMemory", "Returns the GPU memory as JSON: each heap's budget and usage (VK_EXT_memory_budget), then the allocations grouped by heap, kind (buffer / image) and usage flags, largest first (at most 'top' groups).",
			{
				{"top", "How many groups to return (default 20)."}
			},
			[this] (std::optional< int > top) {
				if ( m_device == nullptr )
				{
					return Console::CommandResult::error("No device.");
				}

				Json::Value heaps{Json::arrayValue};

				for ( const auto & heap : m_device->memoryBudgets() )
				{
					Json::Value entry{Json::objectValue};
					entry["heap"] = heap.heapIndex;
					entry["deviceLocal"] = heap.deviceLocal;
					entry["sizeMiB"] = static_cast< double >(heap.size) / 1048576.0;
					entry["budgetMiB"] = static_cast< double >(heap.budget) / 1048576.0;
					entry["usageMiB"] = static_cast< double >(heap.usage) / 1048576.0;
					entry["allocatedMiB"] = static_cast< double >(heap.allocationBytes) / 1048576.0;
					entry["reservedMiB"] = static_cast< double >(heap.blockBytes) / 1048576.0;
					entry["atBudget"] = static_cast< double >(heap.usage) >= 0.98 * static_cast< double >(heap.budget);

					heaps.append(std::move(entry));
				}

				const auto limit = static_cast< size_t >(std::clamp(top.value_or(20), 1, 1000));

				Json::Value report{Json::objectValue};
				report["heaps"] = std::move(heaps);
				report["groups"] = this->gpuAllocationGroups(limit);

				return Console::CommandResult::json(FastJSON::stringify(report));
			}, Console::CommandHint::ReadOnly);

		this->bindCommand("writeGPUMemoryReport", "Writes VMA's detailed statistics (every block and allocation) as JSON into the captures directory and answers its path.", [this] () {
			if ( m_device == nullptr )
			{
				return Console::CommandResult::error("No device.");
			}

			const auto json = m_device->memoryDetailedStatisticsJSON();

			if ( json.empty() )
			{
				return Console::CommandResult::error("No memory allocator.");
			}

			const auto seconds = std::chrono::duration_cast< std::chrono::seconds >(std::chrono::system_clock::now().time_since_epoch()).count();
			const auto filepath = m_primaryServices.fileSystem().userDataDirectory("captures") / ("gpu-memory-" + std::to_string(seconds) + ".json");

			if ( !IO::filePutContents(filepath, json, false, true) )
			{
				return Console::CommandResult::error("Unable to write " + IO::toU8String(filepath) + " !");
			}

			return Console::CommandResult::success(IO::toU8String(filepath));
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

	Json::Value
	Renderer::gpuAllocationGroups (size_t top) const noexcept
	{
		Json::Value list{Json::arrayValue};

		if ( m_device == nullptr )
		{
			return list;
		}

		std::map< std::tuple< uint32_t, std::string, std::string >, AllocationGroup > groups;

		if ( const auto statistics = FastJSON::getRootFromString(m_device->memoryDetailedStatisticsJSON(), 32, true); statistics )
		{
			collectAllocations(*statistics, std::nullopt, *m_device, groups, 0);
		}

		std::vector< std::pair< std::tuple< uint32_t, std::string, std::string >, AllocationGroup > > sorted{groups.begin(), groups.end()};

		std::ranges::sort(sorted, std::ranges::greater{}, [] (const auto & entry) {
			return entry.second.bytes;
		});

		for ( size_t index = 0; index < sorted.size() && index < top; ++index )
		{
			const auto & [key, group] = sorted[index];

			Json::Value entry{Json::objectValue};
			entry["heap"] = std::get< 0 >(key);
			entry["kind"] = std::get< 1 >(key);
			entry["usage"] = std::get< 2 >(key);
			entry["count"] = static_cast< Json::UInt64 >(group.count);
			entry["MiB"] = static_cast< double >(group.bytes) / 1048576.0;
			entry["largestMiB"] = static_cast< double >(group.largest) / 1048576.0;

			list.append(std::move(entry));
		}

		return list;
	}
}
