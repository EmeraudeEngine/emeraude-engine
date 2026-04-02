/*
 * src/AssetLoaders/FBXLoader.hpp
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
#include "Interface.hpp"

/* Local inclusions for usages. */
#include "Resources/Manager.hpp"

namespace EmEn::AssetLoaders
{
	/**
	 * @brief FBX format loader stub.
	 * @note Not yet implemented — proves the AssetLoaders interface architecture.
	 */
	class FBXLoader final : public Interface
	{
		public:

			static constexpr auto ClassId{"AssetLoaders::FBXLoader"};

			/**
			 * @brief Constructs the FBX loader.
			 * @param resources A reference to the engine resource manager.
			 */
			explicit
			FBXLoader (Resources::Manager & resources) noexcept
				: m_resources{resources}
			{

			}

			/** @copydoc EmEn::AssetLoaders::Interface::load() */
			[[nodiscard]]
			bool load (const std::filesystem::path & filepath, AssetData & output) noexcept override;

			/** @copydoc EmEn::AssetLoaders::Interface::supportsExtension() */
			[[nodiscard]]
			bool
			supportsExtension (std::string_view extension) const noexcept override
			{
				return extension == ".fbx";
			}

		private:

			Resources::Manager & m_resources;
	};
}
