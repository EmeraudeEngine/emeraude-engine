/*
 * src/Graphics/Renderable/OceanResource.hpp
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
#include "Graphics/Geometry/OceanSurfaceResource.hpp"
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
	 * @brief An animated sea: FFT spectral waves on a camera-following CDLOD plane (Geometry::OceanSurfaceResource).
	 * @note Engine item ocean-fft-surface, owner decisions 2026-09-28: a new renderable, BasicSeaResource staying the lake
	 * (its animated normal map); each demo picks one as its sea level.
	 * @note The sea level it answers is still the FLAT one, the family level's (AbstractSea::getLevelAt()): the waves reach
	 * the physics later (a CPU copy of the displacement, one frame late — item step 6).
	 * @extends EmEn::Graphics::Renderable::AbstractSea The family level of the seas: the level, the material and its water.
	 */
	class EMEN_API OceanResource final : public AbstractSea
	{
		friend class Resources::Container< OceanResource >;

		using ResourceTrait::load;

		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"OceanResource"};

			/** @brief Defines the resource dependency complexity. */
			static constexpr auto Complexity{Resources::DepComplexity::Complex};

			/**
			 * @brief Constructs an ocean.
			 * @param serviceProvider A reference to the service provider.
			 * @param name The name of the resource [std::move].
			 * @param resourceFlags The resource flag bits. Default none.
			 */
			OceanResource (Resources::AbstractServiceProvider & serviceProvider, std::string name, uint32_t resourceFlags = 0) noexcept
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
				return m_geometry != nullptr ? m_geometry->boundingBox() : NullBoundingBox;
			}

			/** @copydoc EmEn::Graphics::Renderable::Abstract::boundingSphere() const */
			[[nodiscard]]
			const Base::Math::Space3D::Sphere< float > &
			boundingSphere () const noexcept override
			{
				return m_geometry != nullptr ? m_geometry->boundingSphere() : NullBoundingSphere;
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
			 * @brief Loads an ocean.
			 * @param parameters The surface and sea-state knobs.
			 * @param materialResource A reference to the water material. The water it declares
			 * (Material::Interface::waterVolume()) is the ocean's: drawn on both sides, with an underwater view.
			 * @return bool
			 */
			bool load (const Geometry::OceanSurfaceParameters & parameters, const std::shared_ptr< Material::Interface > & materialResource) noexcept;

			/**
			 * @copydoc EmEn::Scenes::SeaLevelInterface::adaptToGround()
			 * @note The ocean reads the ground's height at the centre of each cell of its shelter map
			 * (Geometry::OceanSurfaceParameters::shelterMapSide cells a side over the scene's square), tells which water
			 * the open sea reaches (base VertexFactory::buildShelterMap(): the sea is beyond the square's border, a
			 * passage narrower than shelterMinimumPassage keeps it out) and hands the map to its surface.
			 */
			void adaptToGround (const Scenes::GroundLevelInterface & ground, float boundary) noexcept override;

			/**
			 * @copydoc EmEn::Scenes::SeaLevelInterface::shelterSettings() const
			 * @note The surface's live tuning (Geometry::OceanSurfaceResource::shelterTuning()); nothing before the
			 * ocean is loaded.
			 */
			[[nodiscard]]
			std::optional< Scenes::SeaShelterSettings > shelterSettings () const noexcept override;

			/**
			 * @copydoc EmEn::Scenes::SeaLevelInterface::setShelterSettings()
			 * @note Refused before the ocean is loaded.
			 */
			[[nodiscard]]
			bool setShelterSettings (const Scenes::SeaShelterSettings & settings) noexcept override;

			/**
			 * @copydoc EmEn::Scenes::SeaLevelInterface::surfaceWaves() const
			 * @note The FFT cascades of the surface, once it has been drawn.
			 */
			[[nodiscard]]
			const SeaSurfaceWaves *
			surfaceWaves () const noexcept override
			{
				return m_geometry != nullptr ? m_geometry->surfaceWaves() : nullptr;
			}

			/** @copydoc EmEn::Scenes::SeaLevelInterface::updateVisibility() */
			void
			updateVisibility (const Base::Math::Vector< 3, float > & /*worldPosition*/) noexcept override
			{
				/* NOTE: The surface follows the camera of each pass (Geometry::OceanSurfaceResource::prepareAdaptiveRendering()). */
			}

		private:

			/** @copydoc EmEn::Resources::ResourceTrait::onDependenciesLoaded() */
			[[nodiscard]]
			bool onDependenciesLoaded (const std::stop_token & stopToken) noexcept override;

			std::shared_ptr< Geometry::OceanSurfaceResource > m_geometry;
	};
}

/* Expose the resource manager as a convenient type. */
namespace EmEn::Resources
{
	using Oceans = Container< Graphics::Renderable::OceanResource >;
}
