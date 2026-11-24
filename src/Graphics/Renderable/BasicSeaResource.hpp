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
 * https://github.com/EmeraudeEngine/emeraude-engine
 *
 * --- THIS IS AUTOMATICALLY GENERATED, DO NOT CHANGE ---
 */

#pragma once

/* Project configuration. */
#include "emeraude_export.hpp"

/* STL inclusions. */
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <utility>

/* Local inclusions for inheritances. */
#include "AbstractSea.hpp"

/* Local inclusions for usages. */
#include "Graphics/Geometry/VertexGridResource.hpp"
#include "Hash/FNV1a.hpp"
#include "Resources/ResourceTrait.hpp"
#include "Resources/Types.hpp"

/* Forward declarations. */
namespace EmEn::Graphics::Material
{
	class Interface;
}

namespace EmEn::Resources
{
	template< typename resource_t >
	class Container;
}

namespace EmEn::Graphics::Renderable
{
	/**
	 * @brief The basic sea resource class.
	 * @note A flat grid at the sea level: the lake.
	 * @extends EmEn::Graphics::Renderable::AbstractSea The family level of the seas: the level, the material and its water.
	 */
	class EMEN_API BasicSeaResource final : public AbstractSea
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
			 * @param serviceProvider A reference to the service provider.
			 * @param name The name of the resource [std::move].
			 * @param resourceFlags The resource flag bits. Default none.
			 */
			BasicSeaResource (Resources::AbstractServiceProvider & serviceProvider, std::string name, uint32_t resourceFlags = 0) noexcept
				: AbstractSea{serviceProvider, std::move(name), resourceFlags}
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
				return Base::Hash::FNV1a(ClassId);
			}

			/** @copydoc EmEn::Base::ObservableTrait::classUID() const */
			[[nodiscard]]
			size_t
			classUID () const noexcept override
			{
				return getClassUID();
			}

			/** @copydoc EmEn::Base::ObservableTrait::is() const */
			[[nodiscard]]
			bool
			is (size_t classUID) const noexcept override
			{
				return classUID == getClassUID();
			}

			/** @copydoc EmEn::Graphics::Renderable::Abstract::geometry(uint32_t) const */
			[[nodiscard]]
			const Geometry::Interface *
			geometry (uint32_t /*LODLevel*/) const noexcept override
			{
				return m_geometry.get();
			}

			/** @copydoc EmEn::Graphics::Renderable::Abstract::boundingBox() const */
			[[nodiscard]]
			const Base::Math::Space3D::AACuboid< float > &
			boundingBox () const noexcept override
			{
				return m_geometry != nullptr ?
					m_geometry->boundingBox() :
					NullBoundingBox;
			}

			/** @copydoc EmEn::Graphics::Renderable::Abstract::boundingSphere() const */
			[[nodiscard]]
			const Base::Math::Space3D::Sphere< float > &
			boundingSphere () const noexcept override
			{
				return m_geometry != nullptr ?
					m_geometry->boundingSphere() :
					NullBoundingSphere;
			}

			/** @copydoc EmEn::Resources::ResourceTrait::classLabel() const */
			[[nodiscard]]
			const char *
			classLabel () const noexcept override
			{
				return ClassId;
			}

			/** @copydoc EmEn::Resources::ResourceTrait::load() */
			bool load () noexcept override;

			/** @copydoc EmEn::Resources::ResourceTrait::load(const Json::Value &) */
			bool load (const Json::Value & data) noexcept override;

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
			 * @param materialResource A reference to a material resource smart pointer. The water it declares
			 * (Material::Interface::waterVolume()) is the sea's: drawn on both sides, with an underwater view.
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

			/** @copydoc EmEn::Scenes::SeaLevelInterface::updateVisibility() */
			void
			updateVisibility (const Base::Math::Vector< 3, float > & /*worldPosition*/) noexcept override
			{
				/* NOTE: Nothing to do for a simple flat water plane. */
			}

		private:

			/** @copydoc EmEn::Resources::ResourceTrait::onDependenciesLoaded() */
			[[nodiscard]]
			bool onDependenciesLoaded (const std::stop_token & stopToken) noexcept override;

			/**
			 * @brief Attaches the geometry resource.
			 * @param geometryResource A reference to a geometry resource smart pointer.
			 * @return bool
			 */
			bool setGeometry (const std::shared_ptr< Geometry::VertexGridResource > & geometryResource) noexcept;

			std::shared_ptr< Geometry::VertexGridResource > m_geometry;
	};
}

/* Expose the resource manager as a convenient type. */
namespace EmEn::Resources
{
	using BasicSeas = Container< Graphics::Renderable::BasicSeaResource >;
}
