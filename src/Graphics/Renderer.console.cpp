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
#include <iomanip>
#include <sstream>

/* Local inclusions. */
#include "FileSystem.hpp"
#include "IO/IO.hpp"
#include "PixelFactory/FileIO.hpp"
#include "MDI/BatchBuilder.hpp"
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
		this->bindCommand("screenshot", [this] (const Console::Arguments & /*arguments*/, Console::Outputs & outputs) {
			/* Gets the capture directory. */
			auto captureDirectory = m_primaryServices.fileSystem().userDataDirectory("captures");

			if ( !IO::writable(captureDirectory) )
			{
				outputs.emplace_back(Severity::Error, std::stringstream{} << "Unable to write in captures directory " << captureDirectory);

				return false;
			}

			std::array< PixelFactory::Pixmap< uint8_t >, 3 > images{};

			if ( !this->captureFramebuffer(images, false, false) || !images[0].isValid() )
			{
				outputs.emplace_back(Severity::Error, "Framebuffer capture failed !");

				return false;
			}

			std::stringstream filename;
			filename << std::chrono::duration_cast< std::chrono::seconds >(std::chrono::system_clock::now().time_since_epoch()).count() << ".png";

			const auto filepath = captureDirectory.append(filename.str());

			if ( !PixelFactory::FileIO::write(images[0], filepath) )
			{
				outputs.emplace_back(Severity::Error, std::stringstream{} << "Unable to write screenshot to " << filepath);

				return false;
			}

			outputs.emplace_back(Severity::Success, std::stringstream{} << "Screenshot saved: " << filepath);

			return true;
		}, "Captures the current framebuffer and saves it as a PNG.");

		this->bindCommand("testVideoFrameConverter", [this] (const Console::Arguments & /*arguments*/, Console::Outputs & outputs) {
			/* Self-test of the GPU BGRA->I420 converter (hardware video-encode path):
			 * converts a procedural pattern and compares byte-for-byte against the CPU
			 * reference running the same shared BT.709 integer math. */
			VideoFrameConverter converter{this->device(), this->shaderManager()};

			if ( !converter.create(1280U, 720U) )
			{
				outputs.emplace_back(Severity::Error, "Unable to create the video frame converter !");

				return false;
			}

			uint64_t mismatchedBytes = 0;

			if ( !converter.selfTest(mismatchedBytes) )
			{
				outputs.emplace_back(Severity::Error, std::stringstream{} << "GPU/CPU conversion mismatch (" << mismatchedBytes << " bytes differ) !");

				return false;
			}

			outputs.emplace_back(Severity::Success, "GPU BGRA->I420 conversion matches the CPU reference byte-for-byte (1280x720, BT.709 integer path).");

			return true;
		}, "Self-tests the GPU BGRA->I420 converter against the CPU reference (hardware video encode path).");

		this->bindCommand("testVideoEncoderH265", [this] (const Console::Arguments & /*arguments*/, Console::Outputs & outputs) {
			/* End-to-end hardware encode self-test: converts the procedural pattern once,
			 * then encodes 90 frames (3 GOPs) through the Vulkan Video session and writes
			 * an Annex-B .h265 elementary stream — decode it with ffprobe/ffplay. */
			if ( !this->device()->videoEncodeH265Enabled() )
			{
				outputs.emplace_back(Severity::Warning, "No H.265 hardware encode support on this device.");

				return false;
			}

			VideoFrameConverter converter{this->device(), this->shaderManager()};

			if ( !converter.create(2880U, 1620U) )
			{
				outputs.emplace_back(Severity::Error, "Unable to create the video frame converter !");

				return false;
			}

			uint64_t mismatchedBytes = 0;

			if ( !converter.selfTest(mismatchedBytes) )
			{
				outputs.emplace_back(Severity::Error, "The converter self-test failed !");

				return false;
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
				outputs.emplace_back(Severity::Error, "Unable to create the H.265 hardware encoder !");

				return false;
			}

			const auto captureDirectory = m_primaryServices.fileSystem().userDataDirectory("captures");
			const auto filepath = captureDirectory / "hw-encode-test.h265";

			std::ofstream stream{filepath, std::ios::binary | std::ios::trunc};

			if ( !stream.is_open() )
			{
				outputs.emplace_back(Severity::Error, std::stringstream{} << "Unable to open " << filepath << " !");

				return false;
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
					outputs.emplace_back(Severity::Error, std::stringstream{} << "Encode failed at frame " << frame << " !");

					return false;
				}

				stream.write(reinterpret_cast< const char * >(packet.data()), static_cast< std::streamsize >(packet.size()));

				totalBytes += packet.size();

				if ( wasIDR )
				{
					idrCount++;
				}
			}

			outputs.emplace_back(Severity::Success, std::stringstream{} << "90 frames hardware-encoded (" << totalBytes << " bytes, " << idrCount << " IDR) -> " << filepath);

			return true;
		}, "End-to-end hardware H.265 encode self-test: writes an Annex-B .h265 stream in the captures directory.");

		this->bindCommand("getGPUTimings", [this] (const Console::Arguments & arguments, Console::Outputs & outputs) {
			if ( m_GPUProfiler == nullptr )
			{
				outputs.emplace_back(Severity::Warning, "The GPU profiler is disabled. Set 'Core/Graphics/GPUProfiler/Enabled' to true and restart.");

				return true;
			}

			/* Optional argument: "reset" clears the accumulated statistics (averages, maxima). */
			if ( !arguments.empty() && arguments[0].asString() == "reset" )
			{
				m_GPUProfiler->resetStatistics();

				outputs.emplace_back(Severity::Success, "GPU timing statistics reset.");

				return true;
			}

			const auto timings = m_GPUProfiler->snapshot();

			if ( timings.empty() )
			{
				outputs.emplace_back(Severity::Info, "No GPU timings harvested yet (needs a few rendered frames).");

				return true;
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

			outputs.emplace_back(Severity::Info, table.str());

			return true;
		}, "Returns the per-pass GPU timings (timestamp queries). Optional arg: 'reset' to clear the statistics.");

		this->bindCommand("triggerRenderDocCapture", [this] (const Console::Arguments & arguments, Console::Outputs & outputs) {
			auto & renderDoc = m_vulkanInstance.renderDocCapture();

			if ( !renderDoc.isAvailable() )
			{
				outputs.emplace_back(Severity::Error, "RenderDoc is not available (launch the app under renderdoccmd to inject it).");

				return false;
			}

			/* Define WHERE captures are written. RenderDoc is never told otherwise, so without this
			 * the .rdc lands in an undefined default and the autonomous capture workflow produces
			 * nothing findable. Point it at the user-data RenderDoc directory. */
			auto captureDirectory = m_primaryServices.fileSystem().userDataDirectory("RenderDoc");

			if ( IO::writable(captureDirectory) )
			{
				renderDoc.setCaptureFilePath(captureDirectory.append("capture").string());
			}

			/* Optional first argument: number of consecutive frames to capture (default 1).
			 * Capturing past the first frame is how per-frame / state-tracking bugs are caught. */
			const auto frameCount = arguments.empty() ? 1U : static_cast< uint32_t >(std::max(1, arguments[0].asInteger()));

			if ( frameCount > 1U )
			{
				renderDoc.triggerMultiFrameCapture(frameCount);

				outputs.emplace_back(Severity::Success, std::stringstream{} << "RenderDoc: " << frameCount << " consecutive frame captures triggered.");
			}
			else
			{
				renderDoc.triggerCapture();

				outputs.emplace_back(Severity::Success, "RenderDoc: frame capture triggered (captured on the next present).");
			}

			return true;
		}, "Triggers a RenderDoc frame capture (requires launch under renderdoccmd). Optional arg: frame count.");

		this->bindCommand("getStatus", [this] (const Console::Arguments & /*arguments*/, Console::Outputs & outputs) {
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

			outputs.emplace_back(Severity::Info, status.str());

			return true;
		}, "Returns renderer statistics (FPS, frame time, resolution).");

		this->bindCommand("getMDIStats", [this] (const Console::Arguments & /*arguments*/, Console::Outputs & outputs) {
			std::stringstream json;

			if ( !m_MDIEnabled || m_MDIBatchBuilder == nullptr )
			{
				json << R"({"enabled":false,"reason":)"
					<< ( m_device == nullptr ? R"("no device")" : R"("setting disabled or hardware unsupported")" )
					<< "}";
				outputs.emplace_back(Severity::Info, json.str());

				return true;
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

			outputs.emplace_back(Severity::Info, json.str());

			return true;
		}, "Returns Multi-Draw Indirect statistics from the last frame as JSON (batched/fallback/skipped counts, batched ratio %).");
	}
}
