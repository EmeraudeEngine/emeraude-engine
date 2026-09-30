/*
 * src/Graphics/Geometry/HeightfieldDetailSurfaceResource.hpp
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
#include <memory>
#include <optional>
#include <string>

/* Local inclusions for inheritances. */
#include "VertexGridResource.hpp"

/* Local inclusions for usages. */
#include "CDLODTerrainResource.hpp"
#include "MeshShadingSurface.hpp"

namespace EmEn::Graphics::Geometry
{
	/**
	 * @brief The DETAIL WINDOW of a CDLOD terrain: a mesh-shading surface that stands on the terrain's height clipmap,
	 * around the camera of each pass, where the CDLOD leaves a hole (CDLODTerrainResource::enableDetailWindow()).
	 * @note Owner decisions 2026-09-28 (engine item mesh-shading-surface-on-heightfield): a window plus a hole, carried
	 * by a COMPANION renderable of Renderable::TerrainResource. The window is a pure function of the pass's camera
	 * (CDLODTerrainResource::detailWindowFor()), so this geometry and the CDLOD share no mutable state.
	 * @note It carries BOTH flags: EnableMeshShadingSurface (drawn by task + mesh stages) and EnableHeightfieldSurface
	 * (the program's PerModel set is the terrain's clipmap set, surfaceDescriptorSet()); the generator gives its mesh
	 * stage the heightfield BASE mode (AbstractVertexStage::enableHeightfieldBase()).
	 * @warning Only for a device with VK_EXT_mesh_shader: its vertex fallback would draw its placeholder grid, flat,
	 * without the terrain's heights. Renderable::TerrainResource creates it only there.
	 * @note Every member is defined in the .cpp (MSVC DLL export, see DisplacedGridResource).
	 * @extends EmEn::Graphics::Geometry::VertexGridResource A one-cell placeholder grid, never drawn.
	 */
	class EMEN_API HeightfieldDetailSurfaceResource final : public VertexGridResource
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"HeightfieldDetailSurfaceResource"};

			/**
			 * @brief Constructs a terrain detail window.
			 * @param serviceProvider A reference to the service provider.
			 * @param name The name of the resource [std::move].
			 * @param terrain The terrain it stands on (its clipmap set, its window function, its bounds).
			 */
			HeightfieldDetailSurfaceResource (Resources::AbstractServiceProvider & serviceProvider, const std::string & name, std::shared_ptr< const CDLODTerrainResource > terrain) noexcept;

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

			/**
			 * @brief Builds the placeholder grid (one cell over the terrain's extent). Call it once the terrain is loaded.
			 * @return bool
			 */
			bool loadFromTerrain () noexcept;

			/** @copydoc EmEn::Graphics::Geometry::Interface::boundingBox() */
			[[nodiscard]]
			const Base::Math::Space3D::AACuboid< float > & boundingBox () const noexcept override;

			/** @copydoc EmEn::Graphics::Geometry::Interface::boundingSphere() */
			[[nodiscard]]
			const Base::Math::Space3D::Sphere< float > & boundingSphere () const noexcept override;

			/** @copydoc EmEn::Graphics::Geometry::Interface::surfaceDescriptorSet() const */
			[[nodiscard]]
			const Vulkan::DescriptorSet * surfaceDescriptorSet () const noexcept override;

			/** @copydoc EmEn::Graphics::Geometry::Interface::markSurfaceDrawn() const */
			void markSurfaceDrawn (uint64_t frameCursor) const noexcept override;

			/** @copydoc EmEn::Graphics::Geometry::Interface::surfaceUpdatedFrame() const */
			[[nodiscard]]
			uint64_t surfaceUpdatedFrame () const noexcept override;

			/** @copydoc EmEn::Graphics::Geometry::Interface::meshShadingSurfaceFor() */
			[[nodiscard]]
			std::optional< MeshShadingSurface > meshShadingSurfaceFor (const Base::Math::Vector< 3, float > & cameraPosition) const noexcept override;

		private:

			std::shared_ptr< const CDLODTerrainResource > m_terrain;
	};
}
