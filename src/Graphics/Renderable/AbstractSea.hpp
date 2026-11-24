/*
 * src/Graphics/Renderable/AbstractSea.hpp
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
#include <cstdint>
#include <memory>
#include <string>
#include <utility>

/* Local inclusions for inheritances. */
#include "Abstract.hpp"
#include "Scenes/SeaLevelInterface.hpp"

/* Local inclusions for usages. */
#include "Graphics/Types.hpp"
#include "Math/Vector.hpp"

/* Forward declarations. */
namespace EmEn::Resources
{
	class AbstractServiceProvider;
}

namespace EmEn::Graphics::Renderable
{
	/**
	 * @brief The family level of the seas: a renderable water surface that is the sea level of a scene.
	 * @note A family with a level (owner decision 2026-11-23, engine item underwater-view), on the model of
	 * AbstractBackground. This level holds what every sea has: its level, its surface material and the answers that
	 * follow from them. Each sea keeps its geometry: BasicSeaResource (a flat grid, the lake) and OceanResource (FFT
	 * waves on a camera-following plane).
	 * @note The WATER of a sea is the one its surface material declares (Material::Interface::waterVolume(), owner
	 * decision 2026-11-24): one statement, read by the surface's shading and — through waterMedium() — by the
	 * underwater view, so that above and below agree by construction. A sea whose material declares its water is
	 * drawn on BOTH sides — it exists from below; a sea whose material declares none is drawn as before, its upper
	 * side alone.
	 * @extends EmEn::Graphics::Renderable::Abstract This class is a renderable object in the 3D world.
	 * @extends EmEn::Scenes::SeaLevelInterface This is a sea level.
	 */
	class EMEN_API AbstractSea : public Abstract, public Scenes::SeaLevelInterface
	{
		public:

			/** @copydoc EmEn::Graphics::Renderable::Abstract::subGeometryCount() const */
			[[nodiscard]]
			uint32_t
			subGeometryCount () const noexcept final
			{
				return 1;
			}

			/** @copydoc EmEn::Graphics::Renderable::Abstract::layerCount() const */
			[[nodiscard]]
			uint32_t
			layerCount () const noexcept final
			{
				return 1;
			}

			/** @copydoc EmEn::Graphics::Renderable::Abstract::isOpaque(uint32_t) const */
			[[nodiscard]]
			bool
			isOpaque (uint32_t /*layerIndex*/) const noexcept final
			{
				return m_material != nullptr ? m_material->isOpaque() : true;
			}

			/** @copydoc EmEn::Graphics::Renderable::Abstract::requiresGrabPass(uint32_t) const */
			[[nodiscard]]
			bool
			requiresGrabPass (uint32_t /*layerIndex*/) const noexcept final
			{
				return m_material != nullptr ? m_material->requiresGrabPass() : false;
			}

			/** @copydoc EmEn::Graphics::Renderable::Abstract::material(uint32_t) const */
			[[nodiscard]]
			const Material::Interface *
			material (uint32_t /*layerIndex*/) const noexcept final
			{
				return m_material.get();
			}

			/**
			 * @copydoc EmEn::Graphics::Renderable::Abstract::layerRasterizationOptions(uint32_t) const
			 * @note Both sides when the sea's material declares its water (waterMedium()), the default options (the
			 * upper side alone) otherwise.
			 */
			[[nodiscard]]
			const RasterizationOptions *
			layerRasterizationOptions (uint32_t /*layerIndex*/) const noexcept final
			{
				return this->waterMedium() != nullptr ? &m_bothSides : nullptr;
			}

			/** @copydoc EmEn::Scenes::SeaLevelInterface::getLevel() const */
			[[nodiscard]]
			float
			getLevel () const noexcept final
			{
				return m_waterLevel;
			}

			/**
			 * @copydoc EmEn::Scenes::SeaLevelInterface::getLevelAt(const Base::Math::Vector< 3, float > &) const
			 * @note The level at rest. A sea whose surface moves answers its own (isSubmerged() and getDepthAt() follow).
			 */
			[[nodiscard]]
			float
			getLevelAt (const Base::Math::Vector< 3, float > & /*worldPosition*/) const noexcept override
			{
				return m_waterLevel;
			}

			/** @copydoc EmEn::Scenes::SeaLevelInterface::getLevelAt(float, float, float) const */
			[[nodiscard]]
			Base::Math::Vector< 3, float >
			getLevelAt (float positionX, float positionZ, float deltaY) const noexcept final
			{
				return {positionX, this->getLevelAt(Base::Math::Vector< 3, float >{positionX, m_waterLevel, positionZ}) + deltaY, positionZ};
			}

			/**
			 * @copydoc EmEn::Scenes::SeaLevelInterface::getNormalAt() const
			 * @note The normal at rest. A sea whose surface moves answers its own.
			 */
			[[nodiscard]]
			Base::Math::Vector< 3, float >
			getNormalAt (const Base::Math::Vector< 3, float > & /*worldPosition*/) const noexcept override
			{
				return {0.0F, 1.0F, 0.0F};
			}

			/** @copydoc EmEn::Scenes::SeaLevelInterface::isSubmerged() const */
			[[nodiscard]]
			bool
			isSubmerged (const Base::Math::Vector< 3, float > & worldPosition) const noexcept final
			{
				return worldPosition[Base::Math::Y] < this->getLevelAt(worldPosition);
			}

			/** @copydoc EmEn::Scenes::SeaLevelInterface::getDepthAt() const */
			[[nodiscard]]
			float
			getDepthAt (const Base::Math::Vector< 3, float > & worldPosition) const noexcept final
			{
				return this->getLevelAt(worldPosition) - worldPosition[Base::Math::Y];
			}

			/**
			 * @copydoc EmEn::Scenes::SeaLevelInterface::waterMedium() const
			 * @note The water the sea's surface material declares (Material::Interface::waterVolume()), answered once
			 * the sea is ready for instantiation: its material — a dependency — is then loaded, and what it declares
			 * no longer changes. Before that, null: the scene has no underwater view yet.
			 * @note Called from the render thread every frame: isReadyForInstantiation() is the acquire side of the
			 * publication, the material's statement was written before its release side.
			 */
			[[nodiscard]]
			const Scenes::WaterMedium *
			waterMedium () const noexcept final
			{
				return this->isReadyForInstantiation() && m_material != nullptr ? m_material->waterVolume() : nullptr;
			}

			/**
			 * @copydoc EmEn::Scenes::SeaLevelInterface::adaptToGround()
			 * @note A surface at rest has nothing to adapt. A sea whose surface moves answers its own.
			 */
			void
			adaptToGround (const Scenes::GroundLevelInterface & /*ground*/, float /*boundary*/) noexcept override
			{

			}

			/**
			 * @copydoc EmEn::Scenes::SeaLevelInterface::surfaceWaves() const
			 * @note A surface at rest. A sea whose surface moves answers its own.
			 */
			[[nodiscard]]
			const SeaSurfaceWaves *
			surfaceWaves () const noexcept override
			{
				return nullptr;
			}

		protected:

			/**
			 * @brief Constructs the family level of a sea.
			 * @param serviceProvider A reference to the service provider.
			 * @param name The name of the resource [std::move].
			 * @param resourceFlags The resource flag bits.
			 */
			AbstractSea (Resources::AbstractServiceProvider & serviceProvider, std::string name, uint32_t resourceFlags) noexcept
				: Abstract{serviceProvider, std::move(name), resourceFlags}
			{

			}

			/**
			 * @brief Takes the surface material of the sea and declares it as a dependency of the resource.
			 * @note To call while the resource loads (between beginLoading() and setLoadSuccess()).
			 * @param materialResource A reference to a material smart pointer.
			 * @return bool False when the material is null or cannot become a dependency: the load must fail.
			 */
			[[nodiscard]]
			bool setSeaMaterial (const std::shared_ptr< Material::Interface > & materialResource) noexcept;

			/**
			 * @brief States the level of the sea at rest.
			 * @note ⚠️ The answer only: the geometry of a sea is placed at its level by the sea itself, when it loads.
			 * @param waterLevel The world-space Y, in m.
			 */
			void
			setSeaLevel (float waterLevel) noexcept
			{
				m_waterLevel = waterLevel;
			}

		private:

			std::shared_ptr< Material::Interface > m_material;
			RasterizationOptions m_bothSides{PolygonMode::Fill, CullingMode::None};
			float m_waterLevel{0.0F};
	};
}
