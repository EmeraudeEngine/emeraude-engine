/*
 * src/Graphics/Renderable/TerrainResource.hpp
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
#include <memory>

/* Local inclusions for inheritances. */
#include "Abstract.hpp"
#include "Scenes/GroundInterface.hpp"

/* Local inclusions for usages. */
#include "Resources/Container.hpp"
#include "Graphics/Geometry/AdaptiveVertexGridResource.hpp"
#include "Graphics/Geometry/VertexGridResource.hpp"

namespace EmEn::Graphics::Renderable
{
	/**
	 * @brief The terrain resource class.
	 * @extends EmEn::Graphics::Renderable::Abstract This class is a renderable object in the 3D world.
	 * @extends EmEn::Scenes::GroundInterface This is the scene ground.
	 */
	class TerrainResource final : public Abstract, public Scenes::GroundInterface
	{
		friend class Resources::Container< TerrainResource >;

		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"TerrainResource"};

			/** @brief Defines the resource dependency complexity. */
			static constexpr auto Complexity{Resources::DepComplexity::Complex};

			/* JSON key. */
			static constexpr auto HeightMapKey{"HeightMap"};
				static constexpr auto ImageNameKey{"ImageName"};
				static constexpr auto InverseKey{"Inverse"};
			static constexpr auto MaterialTypeKey{"MaterialType"};
			static constexpr auto MaterialNameKey{"MaterialName"};
			static constexpr auto PerlinNoiseKey{"PerlinNoise"};
			static constexpr auto VertexColorKey{"VertexColor"};

			static constexpr auto DefaultSize{4096.0F};
			static constexpr auto DefaultDivision{4096};

			/**
			 * @brief Constructs a terrain resource.
			 * @param name A reference to a string for the resource name.
			 * @param renderableFlags The resource flag bits. Default none.
			 */
			explicit
			TerrainResource (std::string name, uint32_t renderableFlags = 0) noexcept
				: Abstract{std::move(name), renderableFlags},
				  m_geometry{std::make_unique< Geometry::AdaptiveVertexGridResource >(this->name() + "AdaptiveGrid")}
			{

			}

			/**
			 * @brief Returns the unique identifier for this class [Thread-safe].
			 * @return size_t
			 */
			static
			size_t
			getClassUID () noexcept
			{
				return Libs::Hash::FNV1a(ClassId);
			}

			/** @copydoc EmEn::Libs::ObservableTrait::classUID() const */
			[[nodiscard]]
			size_t
			classUID () const noexcept override
			{
				return getClassUID();
			}

			/** @copydoc EmEn::Libs::ObservableTrait::is() const */
			[[nodiscard]]
			bool
			is (size_t classUID) const noexcept override
			{
				return classUID == getClassUID();
			}

			/** @copydoc EmEn::Graphics::Renderable::Abstract::layerCount() const */
			[[nodiscard]]
			uint32_t
			layerCount () const noexcept override
			{
				return 1;
			}

			/** @copydoc EmEn::Graphics::Renderable::Abstract::isOpaque() const */
			[[nodiscard]]
			bool
			isOpaque (uint32_t /*layerIndex*/) const noexcept override
			{
				if ( m_material != nullptr )
				{
					return m_material->isOpaque();
				}

				return true;
			}

			/** @copydoc EmEn::Graphics::Renderable::Abstract::geometry() const */
			[[nodiscard]]
			const Geometry::Interface *
			geometry () const noexcept override
			{
				return m_geometry.get();
			}

			/** @copydoc EmEn::Graphics::Renderable::Abstract::material() const */
			[[nodiscard]]
			const Material::Interface *
			material (uint32_t /*layerIndex*/) const noexcept override
			{
				return m_material.get();
			}

			/** @copydoc EmEn::Graphics::Renderable::Abstract::layerRasterizationOptions() const */
			[[nodiscard]]
			const RasterizationOptions *
			layerRasterizationOptions (uint32_t /*layerIndex*/) const noexcept override
			{
				return nullptr;
			}

			/** @copydoc EmEn::Graphics::Renderable::Abstract::boundingBox() const */
			[[nodiscard]]
			const Libs::Math::Space3D::AACuboid< float > &
			boundingBox () const noexcept override
			{
				return m_localData.boundingBox();
			}

			/** @copydoc EmEn::Graphics::Renderable::Abstract::boundingSphere() const */
			[[nodiscard]]
			const Libs::Math::Space3D::Sphere< float > &
			boundingSphere () const noexcept override
			{
				return m_localData.boundingSphere();
			}

			/** @copydoc EmEn::Resources::ResourceTrait::classLabel() const */
			[[nodiscard]]
			const char *
			classLabel () const noexcept override
			{
				return ClassId;
			}

			/** @copydoc EmEn::Resources::ResourceTrait::load(Resources::ServiceProvider &) */
			bool load (Resources::AbstractServiceProvider & serviceProvider) noexcept override;

			/** @copydoc EmEn::Resources::ResourceTrait::load(Resources::ServiceProvider &, const std::filesystem::path &) */
			bool load (Resources::AbstractServiceProvider & serviceProvider, const std::filesystem::path & filepath) noexcept override;

			/** @copydoc EmEn::Resources::ResourceTrait::load(Resources::Manager &, const Json::Value &) */
			bool load (Resources::AbstractServiceProvider & serviceProvider, const Json::Value & data) noexcept override;

			/** @copydoc EmEn::Resources::ResourceTrait::memoryOccupied() const noexcept */
			[[nodiscard]]
			size_t
			memoryOccupied () const noexcept override
			{
				// TODO ...
				return 0;
			}

			/** @copydoc EmEn::Scenes::GroundInterface::getLevelAt(const Libs::Math::Vector< 3, float > &) const */
			[[nodiscard]]
			float
			getLevelAt (const Libs::Math::Vector< 3, float > & worldPosition) const noexcept override
			{
				return m_localData.getHeightAt(worldPosition[Libs::Math::X], worldPosition[Libs::Math::Z]);
			}

			/** @copydoc EmEn::Scenes::GroundInterface::getLevelAt(float, float, float) const */
			[[nodiscard]]
			Libs::Math::Vector< 3, float >
			getLevelAt (float positionX, float positionZ, float deltaY) const noexcept override
			{
				return {positionX, m_localData.getHeightAt(positionX, positionZ) + deltaY, positionZ};
			}

			/** @copydoc EmEn::Scenes::GroundInterface::getNormalAt() const */
			[[nodiscard]]
			Libs::Math::Vector< 3, float >
			getNormalAt (const Libs::Math::Vector< 3, float > & worldPosition) const noexcept override
			{
				return m_localData.getNormalAt(worldPosition[Libs::Math::X], worldPosition[Libs::Math::Z]);
			}

			/**
			 * @brief Loads a parametric terrain with a material.
			 * @param size
			 * @param division
			 * @param material A pointer to a material resource.
			 * @return bool
			 */
			bool load (float size, uint32_t division, const std::shared_ptr< Material::Interface > & material) noexcept;

		private:

			/**
			 * @brief Prepares all about geometry of the terrain.
			 * @param size The size of the terrain.
			 * @param division The number of division.
			 * @return bool
			 */
			bool prepareGeometry (float size, uint32_t division) noexcept;

			/**
			 * @brief Sets a grid geometry.
			 * @param geometryResource A reference to a material smart pointer.
			 * @return bool
			 */
			bool setGeometry (const std::shared_ptr< Geometry::AdaptiveVertexGridResource > & geometryResource) noexcept;

			/**
			 * @brief Sets a material.
			 * @param materialResource A reference to a material smart pointer.
			 * @return bool
			 */
			bool setMaterial (const std::shared_ptr< Material::Interface > & materialResource) noexcept;

			/**
			 * @brief updateActiveGeometryProcess
			 */
			void updateActiveGeometryProcess () noexcept;

			/* Contains the graphical sub-data. */
			std::shared_ptr< Geometry::AdaptiveVertexGridResource > m_geometry;
			std::shared_ptr< Geometry::VertexGridResource > m_farGeometry;
			std::shared_ptr< Material::Interface > m_material;
			/* Contains the whole data. */
			Libs::VertexFactory::Grid< float > m_localData{};
			Libs::Math::Vector< 3, float > m_lastUpdatePosition{};
			bool m_updatingActiveGeometryProcess = false;
	};
}

/* Expose the resource manager as a convenient type. */
namespace EmEn::Resources
{
	using Terrains = Container< Graphics::Renderable::TerrainResource >;
}
