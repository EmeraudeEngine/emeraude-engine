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
 * https://github.com/londnoir/emeraude-engine
 *
 * --- THIS IS AUTOMATICALLY GENERATED, DO NOT CHANGE ---
 */

#include "Renderer.hpp"

/* STL inclusions. */
#include <chrono>
#include <sstream>

/* Local inclusions. */
#include "Libs/PixelFactory/FileIO.hpp"
#include "Libs/IO/IO.hpp"
#include "FileSystem.hpp"
#include "PrimaryServices.hpp"
#include "Vulkan/SwapChain.hpp"

namespace EmEn::Graphics
{
	using namespace Libs;

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
	}
}