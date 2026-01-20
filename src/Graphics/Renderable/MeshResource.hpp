/*
 * src/Graphics/Renderable/MeshResource.hpp
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
#include <vector>
#include <memory>

/* Local inclusions for inheritances. */
#include "Abstract.hpp"

/* Local inclusions for usages. */
#include "Resources/Container.hpp"

/* Forward declarations. */
namespace EmEn::Resources
{
	class Manager;
}

namespace EmEn::Graphics::Renderable
{
	/**
	 * @brief The mesh layer class.
	 */
	class MeshLayer final : public Libs::NameableTrait
	{
		public:

			/**
			 * @brief Constructs a mesh layer.
			 * @param layerName A string for the layer name [std::move].
			 * @param material A reference to a material for this layer.
			 * @param options A reference to rasterization options.
			 * @param renderableFlags The renderable level flags.
			 */
			MeshLayer (std::string layerName, const std::shared_ptr< Material::Interface > & material, const RasterizationOptions & options, uint32_t renderableFlags) noexcept
				: NameableTrait{std::move(layerName)},
				m_material{material},
				m_rasterizationOptions{options},
				m_renderableFlags{renderableFlags}
			{

			}

			/**
			 * @brief Returns the material resource of the layer.
			 * @return shared_ptr< Material::Interface >
			 */
			[[nodiscard]]
			std::shared_ptr< Material::Interface >
			material () const noexcept
			{
				return m_material;
			}

			/**
			 * @brief Returns the rasterization options for this layer.
			 * @return const RasterizationOptions &
			 */
			[[nodiscard]]
			const RasterizationOptions &
			rasterizationOptions () const noexcept
			{
				return m_rasterizationOptions;
			}

			/**
			 * @brief Returns renderable level flags.
			 * @return uint32_t
			 */
			[[nodiscard]]
			uint32_t
			flags () const noexcept
			{
				return m_renderableFlags;
			}

		private:

			std::shared_ptr< Material::Interface > m_material;
			RasterizationOptions m_rasterizationOptions;
			uint32_t m_renderableFlags;
	};

	/**
	 * @brief This class provides a high-level object to describe a physical object in the 3D world.
	 * @extends EmEn::Graphics::Renderable::Abstract Adds the ability to be rendered in the 3D world.
	 */
	class MeshResource final : public Abstract
	{
		friend class Resources::Container< MeshResource >;

		using ResourceTrait::load;

		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"MeshResource"};

			/** @brief Defines the resource dependency complexity. */
			static constexpr auto Complexity{Resources::DepComplexity::Complex};

			/**
			 * @brief Construct a mesh resource.
			 * @param name A string for the resource name.
			 * @param renderableFlags The resource flag bits. Default none.
			 */
			explicit
			MeshResource (std::string name, uint32_t renderableFlags = 0) noexcept
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

			/** @copydoc EmEn::Graphics::Renderable::Abstract::subGeometryCount() const */
			[[nodiscard]]
			uint32_t
			subGeometryCount () const noexcept override
			{
				if ( m_geometry == nullptr )
				{
					return 0;
				}

				return m_geometry->subGeometryCount();
			}

			/** @copydoc EmEn::Graphics::Renderable::Abstract::layerCount() const */
			[[nodiscard]]
			uint32_t
			layerCount () const noexcept override
			{
				return static_cast< uint32_t >(m_layers.size());
			}

			/** @copydoc EmEn::Graphics::Renderable::Abstract::isOpaque() const */
			[[nodiscard]]
			bool isOpaque (uint32_t layerIndex) const noexcept override;

			/** @copydoc EmEn::Graphics::Renderable::Abstract::geometry() const */
			[[nodiscard]]
			const Geometry::Interface *
			geometry () const noexcept override
			{
				return m_geometry.get();
			}

			/** @copydoc EmEn::Graphics::Renderable::Abstract::material() const */
			[[nodiscard]]
			const Material::Interface * material (uint32_t layerIndex) const noexcept override;

			/** @copydoc EmEn::Graphics::Renderable::Abstract::layerRasterizationOptions() const */
			[[nodiscard]]
			const RasterizationOptions * layerRasterizationOptions (uint32_t layerIndex) const noexcept override;

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
			 * @brief Loads a mesh resource from a geometry and a material. This will produce a single layer mesh.
			 * @param geometry A reference to a geometry resource smart pointer.
			 * @param material A reference to a material resource smart pointer.
			 * @param rasterizationOptions A reference to rasterization options. Defaults.
			 * @return bool
			 */
			bool load (const std::shared_ptr< Geometry::Interface > & geometry, const std::shared_ptr< Material::Interface > & material, const RasterizationOptions & rasterizationOptions = {}) noexcept;

			/**
			 * @brief Loads a mesh resource from a geometry and a materials list. This will produce a mesh with multiple layers.
			 * @param geometry A reference to a geometry resource smart pointer.
			 * @param materialList A reference to a list of a material resource smart pointer.
			 * @param rasterizationOptions A reference to a list of rasterization options. Defaults.
			 * @return bool
			 */
			bool load (const std::shared_ptr< Geometry::Interface > & geometry, const std::vector< std::shared_ptr< Material::Interface > > & materialList, const std::vector< RasterizationOptions > & rasterizationOptions = {}) noexcept;

			/**
			 * @brief Gives a hint for the mesh size. This is not effective by itself, you can use it to scale a scene node.
			 * @return float
			 */
			[[nodiscard]]
			float baseSize () const noexcept;

			/**
			 * @brief Creates a unique mesh or returns the existing one with the same parameters.
			 * The resource name will be based on sub-resource names.
			 * @param serviceProvider A reference to the resource manager through a service provider.
			 * @param geometryResource A reference to a geometry resource smart pointer.
			 * @param materialResource A reference to a material resource smart pointer.
			 * @param resourceName A string. Default auto generated name.
			 * @return std::shared_ptr< MeshResource >
			 */
			[[nodiscard]]
			static std::shared_ptr< MeshResource > getOrCreate (Resources::AbstractServiceProvider & serviceProvider, const std::shared_ptr< Geometry::Interface > & geometryResource, const std::shared_ptr< Material::Interface > & materialResource, std::string resourceName = {}) noexcept;

			/**
			 * @brief Creates a unique mesh or returns the existing one with the same parameters.
			 * The resource name will be based on sub-resource names.
			 * @param serviceProvider A reference to the resource manager through a service provider.
			 * @param geometryResource A reference to a geometry resource smart pointer.
			 * @param materialResources A reference to a material resource smart pointer list.
			 * @param resourceName A string. Default auto generated name.
			 * @return std::shared_ptr< MeshResource >
			 */
			[[nodiscard]]
			static std::shared_ptr< MeshResource > getOrCreate (Resources::AbstractServiceProvider & serviceProvider, const std::shared_ptr< Geometry::Interface > & geometryResource, const std::vector< std::shared_ptr< Material::Interface > > & materialResources, std::string resourceName = {}) noexcept;

			/**
			 * @brief Parses a JSON stream to get the material information.
			 * @note This method is public to allow SimpleMeshResource to reuse it.
			 * @param serviceProvider A reference to the resource manager through a service provider.
			 * @param data A reference to a JSON node.
			 * @return std::shared_ptr< Material::Interface >
			 */
			static std::shared_ptr< Material::Interface > parseLayer (Resources::AbstractServiceProvider & serviceProvider, const Json::Value & data) noexcept;

			/**
			 * @brief Parses a JSON stream to get the mesh options.
			 * @note This method is public to allow SimpleMeshResource to reuse it.
			 * @param data A reference to a JSON node.
			 * @return RasterizationOptions
			 */
			static RasterizationOptions parseLayerOptions (const Json::Value & data) noexcept;

			/* JSON keys (public for shared use with SimpleMeshResource). */
			static constexpr auto LayersKey{"Layers"};
			static constexpr auto GeometryTypeKey{"GeometryType"};
			static constexpr auto GeometryNameKey{"GeometryName"};
			static constexpr auto MaterialTypeKey{"MaterialType"};
			static constexpr auto MaterialNameKey{"MaterialName"};
			static constexpr auto BaseSizeKey{"BaseSize"};
			static constexpr auto EnableDoubleSidedFaceKey{"EnableDoubleSidedFace"};
			static constexpr auto DrawingModeKey{"DrawingMode"};

		private:

			/**
			 * @brief Sets the geometry resource.
			 * @param geometry A reference to a geometry resource smart pointer.
			 * @return bool
			 */
			bool setGeometry (const std::shared_ptr< Geometry::Interface > & geometry) noexcept;

			/**
			 * @brief Sets the material resource.
			 * @param material A reference to a material resource smart pointer.
			 * @param options A reference to rasterization options.
			 * @param flags The renderable level flags.
			 * @return bool
			 */
			bool
			setMaterial (const std::shared_ptr< Material::Interface > & material, const RasterizationOptions & options, uint32_t flags) noexcept
			{
				m_layers.clear();

				return this->addMaterial(material, options, flags);
			}

			/**
			 * @brief Adds a layer with a material and rasterization options.
			 * @param material A reference to a material resource smart pointer.
			 * @param options A reference to rasterization options.
			 * @param flags The renderable level flags.
			 * @return bool
			 */
			bool addMaterial (const std::shared_ptr< Material::Interface > & material, const RasterizationOptions & options, uint32_t flags) noexcept;

			/**
			 * @brief Parses a JSON stream to get the geometry information.
			 * @param serviceProvider A reference to the resource manager through a service provider.
			 * @param data A reference to a JSON node.
			 * @return std::shared_ptr< Geometry::Interface >
			 */
			std::shared_ptr< Geometry::Interface > parseGeometry (Resources::AbstractServiceProvider & serviceProvider, const Json::Value & data) noexcept;

			/* Flag names. */
			static constexpr auto IsReadyToSetupGPU{0UL};
			static constexpr auto IsBroken{1UL};

			std::shared_ptr< Geometry::Interface > m_geometry;
			std::vector< MeshLayer > m_layers;
			float m_baseSize{1.0F};
	};
}

/* Expose the resource manager as a convenient type. */
namespace EmEn::Resources
{
	using Meshes = Container< Graphics::Renderable::MeshResource >;
}
