/*
 * src/Graphics/Compute/IBLBaker.hpp
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
#include <memory>

namespace EmEn::Vulkan
{
	class Device;
}

namespace EmEn::Saphir
{
	class ShaderManager;
}

namespace EmEn::Graphics
{
	class IBLTexture;
}

namespace EmEn::Graphics::Compute
{
	/**
	 * @brief GPU generator for the image-based lighting (IBL) assets.
	 * @note Lot 1 of the IBL work: bakes the split-sum BRDF LUT (Karis 2013) once at boot.
	 * The irradiance and prefiltered environment passes will land here next.
	 * All work is submitted on the GRAPHICS queue: the produced images are sampled by
	 * fragment shaders on that same queue, which avoids a queue-family ownership transfer
	 * on an EXCLUSIVE image (graphics queues always support compute dispatches).
	 */
	class EMEN_API IBLBaker final
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"GraphicsComputeIBLBaker"};

			/**
			 * @brief Constructs an IBL baker.
			 * @param device The Vulkan device.
			 * @param shaderManager The shader compilation manager.
			 */
			IBLBaker (const std::shared_ptr< Vulkan::Device > & device, Saphir::ShaderManager & shaderManager) noexcept
				: m_device{device},
				m_shaderManager{&shaderManager}
			{

			}

			/** @brief Non-copyable. */
			IBLBaker (const IBLBaker &) = delete;

			IBLBaker & operator= (const IBLBaker &) = delete;

			IBLBaker (IBLBaker &&) = delete;

			IBLBaker & operator= (IBLBaker &&) = delete;

			/** @brief Destructor. */
			~IBLBaker () = default;

			/**
			 * @brief Bakes the split-sum BRDF LUT into the texture (blocking, one-shot).
			 * @note The texture image must be created (IBLTexture::create()) with the BRDFLut
			 * role. On success the image is left in SHADER_READ_ONLY_OPTIMAL layout, ready to
			 * be published to the bindless table.
			 * @param lut A reference to the destination texture.
			 * @return bool True on success.
			 */
			[[nodiscard]]
			bool generateBRDFLut (IBLTexture & lut) const noexcept;

		private:

			std::shared_ptr< Vulkan::Device > m_device;
			Saphir::ShaderManager * m_shaderManager;
	};
}
