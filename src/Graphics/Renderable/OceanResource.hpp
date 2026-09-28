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
#include <memory>

/* Local inclusions for inheritances. */
#include "Abstract.hpp"
#include "Scenes/SeaLevelInterface.hpp"

/* Local inclusions for usages. */
#include "Graphics/Geometry/OceanSurfaceResource.hpp"

/* Forward declarations. */
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
	 * @note The sea level it answers is still the FLAT one: the waves reach the physics later (a CPU copy of the
	 * displacement, one frame late — item step 6).
	 * @extends EmEn::Graphics::Renderable::Abstract This class is a renderable object in the 3D world.
	 * @extends EmEn::Scenes::SeaLevelInterface This is a sea level.
	 */
	class EMEN_API OceanResource final : public Abstract, public Scenes::SeaLevelInterface
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
				: Abstract{serviceProvider, std::move(name), resourceFlags}
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

			/** @copydoc EmEn::Graphics::Renderable::Abstract::subGeometryCount() const */
			[[nodiscard]]
			uint32_t
			subGeometryCount () const noexcept override
			{
				return 1;
			}

			/** @copydoc EmEn::Graphics::Renderable::Abstract::layerCount() const */
			[[nodiscard]]
			uint32_t
			layerCount () const noexcept override
			{
				return 1;
			}

			/** @copydoc EmEn::Graphics::Renderable::Abstract::isOpaque(uint32_t) const */
			[[nodiscard]]
			bool
			isOpaque (uint32_t /*layerIndex*/) const noexcept override
			{
				return m_material != nullptr ? m_material->isOpaque() : true;
			}

			/** @copydoc EmEn::Graphics::Renderable::Abstract::requiresGrabPass(uint32_t) const */
			[[nodiscard]]
			bool
			requiresGrabPass (uint32_t /*layerIndex*/) const noexcept override
			{
				return m_material != nullptr ? m_material->requiresGrabPass() : false;
			}

			/** @copydoc EmEn::Graphics::Renderable::Abstract::geometry(uint32_t) const */
			[[nodiscard]]
			const Geometry::Interface *
			geometry (uint32_t /*LODLevel*/) const noexcept override
			{
				return m_geometry.get();
			}

			/** @copydoc EmEn::Graphics::Renderable::Abstract::material(uint32_t) const */
			[[nodiscard]]
			const Material::Interface *
			material (uint32_t /*layerIndex*/) const noexcept override
			{
				return m_material.get();
			}

			/** @copydoc EmEn::Graphics::Renderable::Abstract::layerRasterizationOptions(uint32_t) const */
			[[nodiscard]]
			const RasterizationOptions *
			layerRasterizationOptions (uint32_t /*layerIndex*/) const noexcept override
			{
				return nullptr;
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
			 * @param materialResource A reference to the water material.
			 * @return bool
			 */
			bool load (const Geometry::OceanSurfaceParameters & parameters, const std::shared_ptr< Material::Interface > & materialResource) noexcept;

			/** @copydoc EmEn::Scenes::SeaLevelInterface::getLevel() const */
			[[nodiscard]]
			float
			getLevel () const noexcept override
			{
				return m_waterLevel;
			}

			/** @copydoc EmEn::Scenes::SeaLevelInterface::getLevelAt(const Base::Math::Vector< 3, float > &) const */
			[[nodiscard]]
			float
			getLevelAt (const Base::Math::Vector< 3, float > & /*worldPosition*/) const noexcept override
			{
				return m_waterLevel;
			}

			/** @copydoc EmEn::Scenes::SeaLevelInterface::getLevelAt(float, float, float) const */
			[[nodiscard]]
			Base::Math::Vector< 3, float >
			getLevelAt (float positionX, float positionZ, float deltaY) const noexcept override
			{
				return {positionX, m_waterLevel + deltaY, positionZ};
			}

			/** @copydoc EmEn::Scenes::SeaLevelInterface::getNormalAt() const */
			[[nodiscard]]
			Base::Math::Vector< 3, float >
			getNormalAt (const Base::Math::Vector< 3, float > & /*worldPosition*/) const noexcept override
			{
				return {0.0F, 1.0F, 0.0F};
			}

			/** @copydoc EmEn::Scenes::SeaLevelInterface::isSubmerged() const */
			[[nodiscard]]
			bool
			isSubmerged (const Base::Math::Vector< 3, float > & worldPosition) const noexcept override
			{
				return worldPosition[Base::Math::Y] < m_waterLevel;
			}

			/** @copydoc EmEn::Scenes::SeaLevelInterface::getDepthAt() const */
			[[nodiscard]]
			float
			getDepthAt (const Base::Math::Vector< 3, float > & worldPosition) const noexcept override
			{
				return m_waterLevel - worldPosition[Base::Math::Y];
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
			bool onDependenciesLoaded () noexcept override;

			std::shared_ptr< Geometry::OceanSurfaceResource > m_geometry;
			std::shared_ptr< Material::Interface > m_material;
			float m_waterLevel{0.0F};
	};
}

/* Expose the resource manager as a convenient type. */
namespace EmEn::Resources
{
	using Oceans = Container< Graphics::Renderable::OceanResource >;
}
