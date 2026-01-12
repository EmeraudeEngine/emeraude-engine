/*
 * src/Tool/ShowVulkanInformation.hpp
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

/* Local inclusions for inheritances. */
#include "ToolInterface.hpp"

namespace EmEn::Vulkan
{
	class Instance;
}

namespace EmEn::Tool
{
	/**
	 * @brief The vulkan information class.
	 * @extends EmEn::Tool::ToolInterface This is tool interface.
	 */
	class ShowVulkanInformation final : public ToolInterface
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"ShowVulkanInformation"};

			/**
			 * @brief Constructs the tool.
			 * @param arguments A reference to the arguments.
			 * @param vulkanInstance A reference to the vulkan instance.
			 */
			ShowVulkanInformation (const Arguments & arguments, const Vulkan::Instance & vulkanInstance) noexcept
				: m_arguments{arguments},
				m_vulkanInstance{vulkanInstance}
			{

			}

			/** @copydoc EmEn::Tool::ToolInterface::execute() */
			[[nodiscard]]
			bool execute () noexcept override;

		private:

			const Arguments & m_arguments;
			const Vulkan::Instance & m_vulkanInstance;
	};
}
