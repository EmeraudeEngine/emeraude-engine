/*
 * src/Graphics/Geometry/DisplacedGridResource.hpp
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
#include <string>

/* Local inclusions for inheritances. */
#include "VertexGridResource.hpp"

/* Local inclusions for usages. */
#include "MeshShadingSurface.hpp"

namespace EmEn::Graphics::Geometry
{
	/**
	 * @brief A flat grid that is a MESH-SHADING SURFACE: on a device with VK_EXT_mesh_shader, it is re-tessellated
	 * near the camera and displaced by its material's height map every frame (Geometry::MeshShadingSurface);
	 * elsewhere, and for the ray tracing and the physics, it is its ordinary flat grid.
	 * @note Owner decision 2026-09-22: a dedicated GEOMETRY (not a renderable), so any renderable carries it — the
	 * ground (Renderable::BasicGroundResource::load(vertexGridResource, …)) first.
	 * @warning The grid must be centred on its origin (no world or UV offset), which is what every
	 * VertexGridResource::load() builds.
	 * @note Every member is defined in the .cpp: an EMEN_API class defined entirely inline in a header that no engine
	 * translation unit includes is never exported by the MSVC DLL, and a consumer then fails to link (LNK2019 on the
	 * __imp_ symbols) while GCC and Clang, which emit inline members on the consumer side, link fine.
	 * @extends EmEn::Graphics::Geometry::VertexGridResource The flat grid it falls back to.
	 */
	class EMEN_API DisplacedGridResource final : public VertexGridResource
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"DisplacedGridResource"};

			/**
			 * @brief Constructs a displaced grid.
			 * @param serviceProvider A reference to the service provider.
			 * @param name The name of the resource [std::move].
			 * @param tileSize The side of one mesh-shading tile, in metres. Default 1 m.
			 * @param resourceFlags The geometry resource flag bits. Default none.
			 */
			DisplacedGridResource (Resources::AbstractServiceProvider & serviceProvider, const std::string & name, float tileSize = 1.0F, uint32_t resourceFlags = 0) noexcept;

			/**
			 * @brief Returns the unique identifier for this class [Thread-safe].
			 * @return size_t
			 */
			static
			size_t
			getClassUID () noexcept
			{
				return Base::Hash::FNV1a(ClassId);
			}

			/** @copydoc EmEn::Base::ObservableTrait::classUID() const */
			[[nodiscard]]
			size_t classUID () const noexcept override;

			/** @copydoc EmEn::Base::ObservableTrait::is() const */
			[[nodiscard]]
			bool is (size_t classUID) const noexcept override;

			/** @copydoc EmEn::Resources::ResourceTrait::classLabel() const */
			[[nodiscard]]
			const char * classLabel () const noexcept override;

			/** @copydoc EmEn::Graphics::Geometry::Interface::meshShadingSurface() */
			[[nodiscard]]
			const MeshShadingSurface * meshShadingSurface () const noexcept override;

		private:

			/** @brief The tiling, refreshed from the grid on each query (the grid may be reloaded). */
			mutable MeshShadingSurface m_surface{};
			float m_tileSize;
	};
}
