/*
 * src/Graphics/Renderable/MeshLayer.hpp
 * This file is part of Emeraude-Engine
 *
 * Copyright (C) 2010-2025 - Sébastien Léon Claude Christian Bémelmans "LondNoir" <londnoir@gmail.com>
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
#include <string>
#include <memory>

/* Local inclusions for inheritances. */
#include "Libs/NameableTrait.hpp"

/* Local inclusions for usages. */
#include "Graphics/Material/Interface.hpp"
#include "Graphics/RasterizationOptions.hpp"

namespace EmEn::Graphics::Renderable
{
	/**
	 * @brief The MeshLayer class
	 */
	class MeshLayer final : public Libs::NameableTrait
	{
		public:

			/**
			 * @brief Constructs a mesh layer.
			 * @param name The name of the layer.
			 * @param material A reference to a material for this layer.
			 * @param options A reference to a rasterization options.
			 * @param flags The renderable level flags.
			 */
			MeshLayer (const std::string & name, const std::shared_ptr< Material::Interface > & material, const RasterizationOptions & options, int flags) noexcept;

			/**
			 * @brief Returns the material resource of the layer.
			 * @return shared_ptr< Material::Interface >
			 */
			[[nodiscard]]
			std::shared_ptr< Material::Interface >
			material () const noexcept
			{
				return m_material;
			}

			/**
			 * @brief Returns the rasterization options for this layer.
			 * @return const RasterizationOptions &
			 */
			[[nodiscard]]
			const RasterizationOptions &
			rasterizationOptions () const noexcept
			{
				return m_rasterizationOptions;
			}

			/**
			 * @brief Returns renderable level flags.
			 * @return int
			 */
			[[nodiscard]]
			int
			flags () const noexcept
			{
				return m_renderableFlags;
			}

		private:

			std::shared_ptr< Material::Interface > m_material;
			RasterizationOptions m_rasterizationOptions;
			int m_renderableFlags;
	};
}
