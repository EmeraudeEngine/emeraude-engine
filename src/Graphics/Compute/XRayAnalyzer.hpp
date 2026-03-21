/*
 * src/Graphics/Compute/XRayAnalyzer.hpp
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

/* STL inclusions. */
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>

/* Local inclusions. */
#include "Libs/VertexFactory/Shape.hpp"
#include "Libs/Math/CartesianFrame.hpp"
#include "Libs/PixelFactory/Pixmap.hpp"

/* Forward declarations. */
namespace EmEn::Vulkan
{
	class Device;
}

namespace EmEn::Saphir
{
	class ShaderManager;
}

namespace EmEn::Graphics::Compute
{
	/**
	 * @brief GPU-accelerated X-Ray cross-section analyzer using Vulkan compute shaders.
	 * @note Produces 2D slice images (like CT scans) at given depths through shapes.
	 * White = inside a shape, black = outside.
	 */
	class XRayAnalyzer final
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"GraphicsComputeXRayAnalyzer"};

			/**
			 * @brief Constructs an X-Ray analyzer with Vulkan compute support.
			 * @param device The Vulkan device.
			 * @param shaderManager The shader compilation manager.
			 */
			XRayAnalyzer (const std::shared_ptr< Vulkan::Device > & device, Saphir::ShaderManager & shaderManager) noexcept;

			/** @brief Destructor. */
			~XRayAnalyzer () noexcept;

			/** @brief Non-copyable. */
			XRayAnalyzer (const XRayAnalyzer &) = delete;
			XRayAnalyzer & operator= (const XRayAnalyzer &) = delete;
			XRayAnalyzer (XRayAnalyzer &&) = delete;
			XRayAnalyzer & operator= (XRayAnalyzer &&) = delete;

			/**
			 * @brief Adds a shape to the scene.
			 * @param shape The shape to add (must remain valid).
			 * @param frame The spatial position/orientation. Default identity.
			 */
			void addShape (const Libs::VertexFactory::Shape< float > & shape, const Libs::Math::CartesianFrame< float > & frame = {}) noexcept;

			/**
			 * @brief Sets the viewpoint.
			 * @param viewpoint The scan direction and position.
			 */
			void setViewpoint (const Libs::Math::CartesianFrame< float > & viewpoint) noexcept;

			/**
			 * @brief Prepares the GPU resources (uploads triangles, builds pipeline).
			 * @param resolution The output image resolution (square).
			 * @return bool True if preparation succeeded.
			 */
			[[nodiscard]]
			bool prepare (uint32_t resolution) noexcept;

			/**
			 * @brief Scans a single slice at the given depth.
			 * @param depth Normalized depth [0.0 = front, 1.0 = back].
			 * @return Libs::PixelFactory::Pixmap< uint8_t > The cross-section image.
			 */
			[[nodiscard]]
			Libs::PixelFactory::Pixmap< uint8_t > scan (float depth) noexcept;

			/**
			 * @brief Scans all slices using single-pass ray casting on GPU.
			 * @param sliceCount Number of slices.
			 * @param callback Called for each completed slice.
			 */
			void scanAll (uint32_t sliceCount, const std::function< void (uint32_t, const Libs::PixelFactory::Pixmap< uint8_t > &) > & callback) noexcept;

		private:

			struct Impl;
			std::unique_ptr< Impl > m_impl;
	};
}
