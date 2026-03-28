/*
 * src/Graphics/Renderable/SimpleMeshResource.hpp
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
#include <future>
#include <memory>
#include <mutex>

/* Local inclusions for inheritances. */
#include "Abstract.hpp"
#include "SkeletalDataTrait.hpp"

/* Local inclusions for usages. */
#include "Graphics/Geometry/IndexedVertexResource.hpp"
#include "Resources/Container.hpp"

namespace EmEn::Graphics::Renderable
{
	/**
	 * @brief Simple mesh renderable with only one layer.
	 * @extends EmEn::Graphics::Renderable::Abstract
	 */
	class SimpleMeshResource final : public Abstract, public SkeletalDataTrait
	{
		friend class Resources::Container< SimpleMeshResource >;

		using ResourceTrait::load;

		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"SimpleMeshResource"};

			/** @brief Defines the resource dependency complexity. */
			static constexpr auto Complexity{Resources::DepComplexity::Complex};

			/**
			 * @brief Constructs a simple mesh resource.
			 * @param serviceProvider A reference to the service provider.
			 * @param name The name of the resource [std::move].
			 * @param resourceFlags The resource flag bits. Default none.
			 */
			SimpleMeshResource (Resources::AbstractServiceProvider & serviceProvider, std::string name, uint32_t resourceFlags = 0) noexcept
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
			const Geometry::Interface * geometry (uint32_t LODIndex) const noexcept override;

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
				return &m_rasterizationOptions;
			}

			/** @copydoc EmEn::Graphics::Renderable::Abstract::boundingBox() const */
			[[nodiscard]]
			const Libs::Math::Space3D::AACuboid< float > &
			boundingBox () const noexcept override
			{
				return !m_geometry.empty() && m_geometry[0] != nullptr ?
					m_geometry[0]->boundingBox() :
					NullBoundingBox;
			}

			/** @copydoc EmEn::Graphics::Renderable::Abstract::boundingSphere() const */
			[[nodiscard]]
			const Libs::Math::Space3D::Sphere< float > &
			boundingSphere () const noexcept override
			{
				return !m_geometry.empty() && m_geometry[0] != nullptr ?
					m_geometry[0]->boundingSphere() :
					NullBoundingSphere;
			}

			/**
			 * @brief Loads a simple mesh.
			 * @param geometry A reference to a geometry smart pointer.
			 * @param material A reference to a material smart pointer.
			 * @param rasterizationOptions Rasterization options. Default none.
			 * @return bool
			 */
			bool load (const std::shared_ptr< Geometry::Interface > & geometry, const std::shared_ptr< Material::Interface > & material = nullptr, const RasterizationOptions & rasterizationOptions = {}) noexcept;

		private:

			/** @copydoc EmEn::Resources::ResourceTrait::onDependenciesLoaded() */
			[[nodiscard]]
			bool onDependenciesLoaded () noexcept override;

			/**
			 * @brief Attaches the geometry resource.
			 * @param geometryResource A reference to a geometry resource smart pointer.
			 * @return bool
			 */
			bool setGeometry (const std::shared_ptr< Geometry::Interface > & geometryResource) noexcept;

			/**
			 * @brief Attaches the material resource.
			 * @param materialResource A reference to a material resource smart pointer.
			 * @return bool
			 */
			bool setMaterial (const std::shared_ptr< Material::Interface > & materialResource) noexcept;

			/**
			 * @brief Generates a single LOD level from the source geometry via mesh decimation.
			 * @param sourceGeometry The LOD 0 indexed geometry with local data.
			 * @param LODLevel The target LOD level (1-3).
			 * @param ratio The decimation ratio (0.0 = max reduction, 1.0 = no reduction).
			 * @return void
			 */
			void generateLODLevel (const std::shared_ptr< Geometry::IndexedVertexResource > & sourceGeometry, uint32_t LODLevel, float ratio) noexcept;

			Libs::StaticVector< std::shared_ptr< Geometry::Interface >, MaxLODLevels > m_geometry;
			std::shared_ptr< Material::Interface > m_material;
			RasterizationOptions m_rasterizationOptions{};
			std::vector< std::future< void > > m_lodFutures;
			mutable std::mutex m_geometryMutex;
	};
}

/* Expose the resource manager as a convenient type. */
namespace EmEn::Resources
{
	using SimpleMeshes = Container< Graphics::Renderable::SimpleMeshResource >;
}
