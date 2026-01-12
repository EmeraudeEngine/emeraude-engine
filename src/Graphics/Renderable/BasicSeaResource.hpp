/*
 * src/Graphics/Renderable/BasicSeaResource.hpp
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
#include <memory>

/* Local inclusions for inheritances. */
#include "Abstract.hpp"
#include "Scenes/SeaLevelInterface.hpp"

/* Local inclusions for usages. */
#include "Resources/Container.hpp"
#include "Graphics/Geometry/VertexGridResource.hpp"

namespace EmEn::Graphics::Renderable
{
	/**
	 * @brief The basic sea resource class.
	 * @extends EmEn::Graphics::Renderable::Abstract This class is a renderable object in the 3D world.
	 * @extends EmEn::Scenes::SeaLevelInterface This is a sea level.
	 */
	class BasicSeaResource : public Abstract, public Scenes::SeaLevelInterface
	{
		friend class Resources::Container< BasicSeaResource >;

		using ResourceTrait::load;

		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"BasicSeaResource"};

			/** @brief Defines the resource dependency complexity. */
			static constexpr auto Complexity{Resources::DepComplexity::Complex};

			static constexpr auto CellSize{100.0F};
			static constexpr auto DefaultSize{1024.0F};
			static constexpr auto DefaultDivision{16};

			/**
			 * @brief Constructs a water level resource.
			 * @param name A reference to a string for the resource name.
			 * @param renderableFlags The resource flag bits. Default none.
			 */
			explicit
			BasicSeaResource (std::string name, uint32_t renderableFlags = 0) noexcept
				: Abstract{std::move(name), renderableFlags}
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
				if ( m_material == nullptr )
				{
					return true;
				}

				return m_material->isOpaque();
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
				if ( m_geometry == nullptr )
				{
					return NullBoundingBox;
				}

				return m_geometry->boundingBox();
			}

			/** @copydoc EmEn::Graphics::Renderable::Abstract::boundingSphere() const */
			[[nodiscard]]
			const Libs::Math::Space3D::Sphere< float > &
			boundingSphere () const noexcept override
			{
				if ( m_geometry == nullptr )
				{
					return NullBoundingSphere;
				}

				return m_geometry->boundingSphere();
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

			/** @copydoc EmEn::Resources::ResourceTrait::load(Resources::Manager &, const Json::Value &) */
			bool load (Resources::AbstractServiceProvider & serviceProvider, const Json::Value & data) noexcept override;

			/** @copydoc EmEn::Resources::ResourceTrait::memoryOccupied() const noexcept */
			[[nodiscard]]
			size_t
			memoryOccupied () const noexcept override
			{
				return sizeof(*this);
			}

			/**
			 * @brief Loads a water level from geometry and material resources.
			 * @param geometryResource A reference to a geometry resource smart pointer.
			 * @param materialResource A reference to a material resource smart pointer.
			 * @param waterLevel The height of the water surface. Default 0.0F.
			 * @return bool
			 */
			bool load (const std::shared_ptr< Geometry::VertexGridResource > & geometryResource, const std::shared_ptr< Material::Interface > & materialResource, float waterLevel = 0.0F) noexcept;

			/**
			 * @brief Loads a water level by using parameters to generate the water plane.
			 * @param gridSize The size of the whole size of one dimension of the grid.
			 * @param gridDivision How many cells in one dimension.
			 * @param materialResource A reference to a material smart pointer.
			 * @param waterLevel The height of the water surface. Default 0.0F.
			 * @param UVMultiplier Texture coordinates multiplier. Default 1.
			 * @return bool
			 */
			bool load (float gridSize, uint32_t gridDivision, const std::shared_ptr< Material::Interface > & materialResource, float waterLevel = 0.0F, float UVMultiplier = 1.0F) noexcept;

			/** @copydoc EmEn::Scenes::SeaLevelInterface::getLevel() const */
			[[nodiscard]]
			float
			getLevel () const noexcept override
			{
				return m_waterLevel;
			}

			/** @copydoc EmEn::Scenes::SeaLevelInterface::getLevelAt(const Libs::Math::Vector< 3, float > &) const */
			[[nodiscard]]
			float
			getLevelAt (const Libs::Math::Vector< 3, float > & /*worldPosition*/) const noexcept override
			{
				return m_waterLevel;
			}

			/** @copydoc EmEn::Scenes::SeaLevelInterface::getLevelAt(float, float, float) const */
			[[nodiscard]]
			Libs::Math::Vector< 3, float >
			getLevelAt (float positionX, float positionZ, float deltaY) const noexcept override
			{
				return {positionX, m_waterLevel + deltaY, positionZ};
			}

			/** @copydoc EmEn::Scenes::SeaLevelInterface::getNormalAt() const */
			[[nodiscard]]
			Libs::Math::Vector< 3, float >
			getNormalAt (const Libs::Math::Vector< 3, float > & /*worldPosition*/) const noexcept override
			{
				return {0.0F, 1.0F, 0.0F};
			}

			/** @copydoc EmEn::Scenes::SeaLevelInterface::isSubmerged() const */
			[[nodiscard]]
			bool
			isSubmerged (const Libs::Math::Vector< 3, float > & worldPosition) const noexcept override
			{
				return worldPosition[Libs::Math::Y] < m_waterLevel;
			}

			/** @copydoc EmEn::Scenes::SeaLevelInterface::getDepthAt() const */
			[[nodiscard]]
			float
			getDepthAt (const Libs::Math::Vector< 3, float > & worldPosition) const noexcept override
			{
				return m_waterLevel - worldPosition[Libs::Math::Y];
			}

			/** @copydoc EmEn::Scenes::SeaLevelInterface::updateVisibility() */
			void
			updateVisibility (const Libs::Math::Vector< 3, float > & /*worldPosition*/) noexcept override
			{
				/* NOTE: Nothing to do for a simple flat water plane. */
			}

			/**
			 * @brief Sets the water level height.
			 * @param waterLevel The new water level.
			 */
			void
			setWaterLevel (float waterLevel) noexcept
			{
				m_waterLevel = waterLevel;
			}

		private:

			/**
			 * @brief Attaches the geometry resource.
			 * @param geometryResource A reference to a geometry resource smart pointer.
			 * @return bool
			 */
			bool setGeometry (const std::shared_ptr< Geometry::VertexGridResource > & geometryResource) noexcept;

			/**
			 * @brief Attaches the material resource.
			 * @param materialResource A reference to a material resource smart pointer.
			 * @return bool
			 */
			bool setMaterial (const std::shared_ptr< Material::Interface > & materialResource) noexcept;

			std::shared_ptr< Geometry::VertexGridResource > m_geometry;
			std::shared_ptr< Material::Interface > m_material;
			float m_waterLevel{0.0F};
	};
}

/* Expose the resource manager as a convenient type. */
namespace EmEn::Resources
{
	using BasicSeas = Container< Graphics::Renderable::BasicSeaResource >;
}
