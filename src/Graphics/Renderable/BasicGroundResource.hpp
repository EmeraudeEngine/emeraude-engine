/*
 * src/Graphics/Renderable/BasicGroundResource.hpp
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
#include "Scenes/GroundLevelInterface.hpp"

/* Local inclusions for usages. */
#include "Resources/Container.hpp"
#include "Graphics/Geometry/VertexGridResource.hpp"

namespace EmEn::Graphics::Renderable
{
	/**
	 * @brief The basic ground resource class.
	 * @extends EmEn::Graphics::Renderable::Abstract This class is a renderable object in the 3D world.
	 * @extends EmEn::Scenes::GroundLevelInterface This is the scene ground.
	 */
	class BasicGroundResource final : public Abstract, public Scenes::GroundLevelInterface
	{
		friend class Resources::Container< BasicGroundResource >;

		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"BasicGroundResource"};

			static constexpr auto DefaultSize{1024.0F};
			static constexpr auto DefaultDivision{16};
			static constexpr uint32_t DefaultGeometryFlags{Geometry::EnableTangentSpace | Geometry::EnableVertexColor | Geometry::EnablePrimaryTextureCoordinates | Geometry::EnablePrimitiveRestart};

			/* JSON key. */
			static constexpr auto SizeKey{"Size"};
			static constexpr auto DivisionKey{"Division"};
			static constexpr auto HeightMapKey{"HeightMap"};
				static constexpr auto ImageNameKey{"ImageName"};
				static constexpr auto ScaleKey{"Scale"};
				static constexpr auto InverseKey{"Inverse"};
			static constexpr auto MaterialTypeKey{"MaterialType"};
			static constexpr auto MaterialNameKey{"MaterialName"};
			static constexpr auto UVMultiplierKey{"UVMultiplier"};

			/** @brief Defines the resource dependency complexity. */
			static constexpr auto Complexity{Resources::DepComplexity::Complex};

			/**
			 * @brief Constructs a basic ground resource.
			 * @param name A string for the basic ground name [std::move].
			 * @param renderableFlags The resource flag bits. Default none.
			 */
			explicit
			BasicGroundResource (std::string name, uint32_t renderableFlags = 0) noexcept
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

			/** @copydoc EmEn::Graphics::Renderable::Abstract::layerCount() const */
			[[nodiscard]]
			uint32_t layerCount () const noexcept override
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
				return &m_rasterizationOptions;
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

			/** @copydoc EmEn::Scenes::GroundLevelInterface::getLevelAt(const Libs::Math::Vector< 3, float > &) const */
			[[nodiscard]]
			float getLevelAt (const Libs::Math::Vector< 3, float > & worldPosition) const noexcept override;

			/** @copydoc EmEn::Scenes::GroundLevelInterface::getLevelAt(float, float, float) const */
			[[nodiscard]]
			Libs::Math::Vector< 3, float > getLevelAt (float positionX, float positionZ, float deltaY) const noexcept override;

			/** @copydoc EmEn::Scenes::GroundLevelInterface::getNormalAt() const */
			[[nodiscard]]
			Libs::Math::Vector< 3, float > getNormalAt (const Libs::Math::Vector< 3, float > & worldPosition) const noexcept override;

			/** @copydoc EmEn::Scenes::GroundLevelInterface::updateVisibility() */
			void
			updateVisibility (const Libs::Math::Vector< 3, float > & worldPosition) noexcept override
			{
				/* NOTE: Nothing to do ... */
			}

			/**
			 * @brief Loads a basic ground by providing a vertex grid geometry for the ground and a material to paint it.
			 * @param vertexGridResource A reference to a vertex grid resource smart-pointer.
			 * @param materialResource A reference to a material resource smart-pointer.
			 * @param rasterizationOptions Rasterization options. Default none.
			 * @return bool
			 */
			bool load (const std::shared_ptr< Geometry::VertexGridResource > & vertexGridResource, const std::shared_ptr< Material::Interface > & materialResource, const RasterizationOptions & rasterizationOptions = {}) noexcept;

			/**
			 * @brief Loads a basic ground by using parameters to generate the ground and a material to paint it.
			 * @param gridSize The size of the whole size of one dimension of the grid. I.e., If the size is 1024, the grid will be from +512 to -512.
			 * @param gridDivision How many cells in one dimension.
			 * @param materialResource A reference to a material smart pointer.
			 * @param rasterizationOptions Rasterization options. Default none.
			 * @param UVMultiplier Texture coordinates multiplier. Default 1.
			 * @return bool
			 */
			bool load (float gridSize, uint32_t gridDivision, const std::shared_ptr< Material::Interface > & materialResource, const RasterizationOptions & rasterizationOptions = {}, float UVMultiplier = 1.0F) noexcept;

			/**
			 * @brief Loads a basic ground by using parameters to generate the ground with a displacement map and a material to paint it.
			 * @tparam pixmapData_t The type used within the pixmap.
			 * @param gridSize The size of the whole size of one dimension of the grid. I.e., If the size is 1024, the grid will be from +512 to -512.
			 * @param gridDivision How many cells in one dimension.
			 * @param displacementMap A pixmap to use as a displacement map.
			 * @param displacementFactor Factor of displacement.
			 * @param materialResource A reference to a material smart pointer.
			 * @param rasterizationOptions Rasterization options. Default none.
			 * @param UVMultiplier Texture coordinates multiplier. Default 1.
			 * @return bool
			 */
			template< typename pixmapData_t >
			bool
			load (float gridSize, uint32_t gridDivision, const Libs::PixelFactory::Pixmap< pixmapData_t > & displacementMap, float displacementFactor, const std::shared_ptr< Material::Interface > & materialResource, const RasterizationOptions & rasterizationOptions = {}, float UVMultiplier = 1.0F) noexcept requires (std::is_arithmetic_v< pixmapData_t >)
			{
				const auto geometryResource = std::make_shared< Geometry::VertexGridResource >(this->name() + "GridGeometryDisplaced");

				if ( !geometryResource->load(gridSize, gridDivision, UVMultiplier) )
				{
					Tracer::error(ClassId, "Unable to generate a basic ground geometry !");

					return false;
				}

				geometryResource->localData().applyDisplacementMapping(displacementMap, displacementFactor);

				return this->load(geometryResource, materialResource, rasterizationOptions);
			}

			/**
			 * @brief Loads a basic ground by using parameters to generate the ground with diamond square and a material to paint it.
			 * @param gridSize The size of the whole size of one dimension of the grid. I.e., If the size is 1024, the grid will be from +512 to -512.
			 * @param gridDivision How many cells in one dimension.
			 * @param materialResource A reference to a material smart pointer.
			 * @param noise A reference to a struct.
			 * @param rasterizationOptions Rasterization options. Default none.
			 * @param UVMultiplier Texture coordinates multiplier. Default 1.
			 * @param shiftHeight Apply a shift on each final height. Default none.
			 * @return bool
			 */
			bool loadDiamondSquare (float gridSize, uint32_t gridDivision, const std::shared_ptr< Material::Interface > & materialResource, const Libs::VertexFactory::DiamondSquareParams< float > & noise, const RasterizationOptions & rasterizationOptions = {}, float UVMultiplier = 1.0F, float shiftHeight = 0.0F) noexcept;

			/**
			 * @brief Loads a basic ground by using parameters to generate the ground with perlin noise and a material to paint it.
			 * @param gridSize The size of the whole size of one dimension of the grid. I.e., If the size is 1024, the grid will be from +512 to -512.
			 * @param gridDivision How many cells in one dimension.
			 * @param materialResource A reference to a material smart pointer.
			 * @param noise A reference to a struct.
			 * @param rasterizationOptions Rasterization options. Default none.
			 * @param UVMultiplier Texture coordinates multiplier. Default 1.
			 * @param shiftHeight Apply a shift on each final height. Default none.
			 * @return bool
			 */
			bool loadPerlinNoise (float gridSize, uint32_t gridDivision, const std::shared_ptr< Material::Interface > & materialResource, const Libs::VertexFactory::PerlinNoiseParams< float > & noise, const RasterizationOptions & rasterizationOptions = {}, float UVMultiplier = 1.0F, float shiftHeight = 0.0F) noexcept;

		private:

			/**
			 * @brief Sets the geometry resource.
			 * @param geometryResource A reference to a geometry resource smart pointer.
			 * @return bool
			 */
			bool setGeometry (const std::shared_ptr< Geometry::VertexGridResource > & geometryResource) noexcept;

			/**
			 * @brief Sets the material resource.
			 * @param materialResource A reference to a material resource smart pointer.
			 * @return bool
			 */
			bool setMaterial (const std::shared_ptr< Material::Interface > & materialResource) noexcept;

			std::shared_ptr< Geometry::VertexGridResource > m_geometry;
			std::shared_ptr< Material::Interface > m_material;
			RasterizationOptions m_rasterizationOptions{};
	};
}

/* Expose the resource manager as a convenient type. */
namespace EmEn::Resources
{
	using BasicGrounds = Container< Graphics::Renderable::BasicGroundResource >;
}
