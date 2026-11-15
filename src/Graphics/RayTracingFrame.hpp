/*
 * src/Graphics/RayTracingFrame.hpp
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

/* Forward declarations. */
namespace EmEn::Vulkan
{
	class DescriptorSet;
}

namespace EmEn::Graphics
{
	/**
	 * @brief What ONE frame traces: the ray-tracing descriptor set this frame wrote for the acceleration structure it
	 * built (binding 0) and the scene's storage buffers (1: mesh metadata, 2: material data, 3: lights, 4: sub-geometry
	 * table), and the number of lights that buffer holds for this frame.
	 * @note Holding one is the proof that the frame can trace: it is made by RayTracing::bindFrame() once every binding
	 * is written, and it does not outlive its frame (RayTracing::FrameGuard). The descriptor set is a
	 * REFERENCE on purpose: nothing downstream tests it.
	 */
	struct RayTracingFrame final
	{
		const Vulkan::DescriptorSet & descriptorSet;
		uint32_t lightCount;
	};
}
