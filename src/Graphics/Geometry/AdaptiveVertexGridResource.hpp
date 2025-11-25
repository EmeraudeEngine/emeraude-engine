/*
 * src/Graphics/Geometry/AdaptiveVertexGridResource.hpp
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
#include "Interface.hpp"

/* Local inclusions for usages. */
#include "Libs/Math/CartesianFrame.hpp"
#include "Resources/Container.hpp"
#include "Graphics/ImageResource.hpp"

namespace EmEn::Graphics::Geometry
{
	/**
	 * @brief Defines a geometry using a VBO and an IBO to produce a grid with LOD adapted from the point of view.
	 * @extends EmEn::Graphics::Geometry::Interface The common base for all geometry types.
	 */
	class AdaptiveVertexGridResource final : public Interface
	{
		friend class Resources::Container< AdaptiveVertexGridResource >;

		using ResourceTrait::load;

		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"AdaptiveVertexGridResource"};

			/** @brief Defines the resource dependency complexity. */
			static constexpr auto Complexity{Resources::DepComplexity::One};

			/**
			 * @brief Constructs an adaptive grid geometry resource.
			 * @param name A reference to a string for the resource name.
			 * @param geometryFlagBits The geometry resource flag bits, See EmEn::Graphics::Geometry::GeometryFlagBits. Default EnablePrimitiveRestart.
			 */
			explicit
			AdaptiveVertexGridResource (const std::string & name, uint32_t geometryFlagBits = EnablePrimitiveRestart) noexcept
				: Interface{name, geometryFlagBits}
			{

			}

			/**
			 * @brief Copy constructor.
			 * @param copy A reference to the copied instance.
			 */
			AdaptiveVertexGridResource (const AdaptiveVertexGridResource & copy) noexcept = delete;

			/**
			 * @brief Move constructor.
			 * @param copy A reference to the copied instance.
			 */
			AdaptiveVertexGridResource (AdaptiveVertexGridResource && copy) noexcept = delete;

			/**
			 * @brief Copy assignment.
			 * @param copy A reference to the copied instance.
			 */
			AdaptiveVertexGridResource & operator= (const AdaptiveVertexGridResource & copy) noexcept = delete;

			/**
			 * @brief Move assignment.
			 * @param copy A reference to the copied instance.
			 */
			AdaptiveVertexGridResource & operator= (AdaptiveVertexGridResource && copy) noexcept = delete;

			/**
			 * @brief Destructs the adaptive grid geometry resource.
			 */
			~AdaptiveVertexGridResource () override
			{
				this->destroyFromHardware(true);
			}

			/**
			 * @brief Returns the unique identifier for this class [Thread-safe].
			 * @return size_t
			 */
			static
			size_t
			getClassUID () noexcept
			{
				static const size_t classUID = EmEn::Libs::Hash::FNV1a(ClassId);

				return classUID;
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

			/** @copydoc EmEn::Graphics::Geometry::Interface::isCreated() */
			[[nodiscard]]
			bool
			isCreated () const noexcept override
			{
				if ( m_vertexBufferObject == nullptr || !m_vertexBufferObject->isCreated() )
				{
					return false;
				}

				if ( m_indexBufferObject == nullptr || !m_indexBufferObject->isCreated() )
				{
					return false;
				}

				return true;
			}

			/** @copydoc EmEn::Graphics::Geometry::Interface::topology() */
			[[nodiscard]]
			Topology
			topology () const noexcept override
			{
				return Topology::TriangleStrip;
			}

			/** @copydoc EmEn::Graphics::Geometry::Interface::subGeometryCount() */
			[[nodiscard]]
			uint32_t
			subGeometryCount () const noexcept override
			{
				return 1;
			}

			/** @copydoc EmEn::Graphics::Geometry::Interface::subGeometryRange() */
			[[nodiscard]]
			std::array< uint32_t, 2 >
			subGeometryRange (uint32_t /*subGeometryIndex*/ = 0) const noexcept override
			{
				return {0, m_indexBufferObject->indexCount()};
			}

			/** @copydoc EmEn::Graphics::Geometry::Interface::boundingBox() */
			[[nodiscard]]
			const Libs::Math::Space3D::AACuboid< float > &
			boundingBox () const noexcept override
			{
				return m_boundingBox;
			}

			/** @copydoc EmEn::Graphics::Geometry::Interface::boundingSphere() */
			[[nodiscard]]
			const Libs::Math::Space3D::Sphere< float > &
			boundingSphere () const noexcept override
			{
				return m_boundingSphere;
			}

			/** @copydoc EmEn::Graphics::Geometry::Interface::vertexBufferObject() */
			[[nodiscard]]
			const Vulkan::VertexBufferObject *
			vertexBufferObject () const noexcept override
			{
				return m_vertexBufferObject.get();
			}

			/** @copydoc EmEn::Graphics::Geometry::Interface::indexBufferObject() */
			[[nodiscard]]
			const Vulkan::IndexBufferObject *
			indexBufferObject () const noexcept override
			{
				return m_indexBufferObject.get();
			}

			/** @copydoc EmEn::Graphics::Geometry::Interface::useIndexBuffer() */
			[[nodiscard]]
			bool
			useIndexBuffer () const noexcept override
			{
				if constexpr ( IsDebug )
				{
					return m_indexBufferObject != nullptr;
				}

				return true;
			}

			/** @copydoc EmEn::Graphics::Geometry::Interface::createOnHardware() noexcept */
			bool createOnHardware (Vulkan::TransferManager & transferManager) noexcept override;

			/** @copydoc EmEn::Graphics::Geometry::Interface::updateVideoMemory() noexcept */
			bool updateVideoMemory () noexcept override;

			/** @copydoc EmEn::Graphics::Geometry::Interface::destroyFromHardware(bool) noexcept */
			void destroyFromHardware (bool clearLocalData) noexcept override;

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
				// TODO ...
				return 0;
			}

			/**
			 * @brief Prepares a grid ready to go on video memory with create() method from a position and a number of quads (lesser the whole grid).
			 * @param baseGrid
			 * @param quadCount
			 * @param position
			 * @return bool
			 */
			bool load (const Libs::VertexFactory::Grid< float > & baseGrid, uint32_t quadCount, const Libs::Math::Vector< 3, float > & position) noexcept;

			/**
			 * @brief This will rewrite the vertices buffer from the grid and the new position.
			 * @param baseGrid
			 * @param position
			 * @return bool
			 */
			bool updateLocalData (const Libs::VertexFactory::Grid< float > & baseGrid, const Libs::Math::Vector< 3, float > & position) noexcept;

			/**
			 * @brief This will modify the render from the position of the camera using LOD with the existing data.
			 * @param view
			 */
			void updateVisibility (const Libs::Math::CartesianFrame< float > & view) noexcept;

			/**
			 * @brief Gets the minimal distance to fire an processLogics.
			 * @return float
			 */
			[[nodiscard]]
			float
			getMinimalUpdateDistance () const noexcept
			{
				return m_minimalUpdateDistance;
			}

			/**
			 * @brief Enables the use of vertex color.
			 * @param vertexColorMap A reference to an image smart pointer.
			 */
			void enableVertexColor (const std::shared_ptr< ImageResource > & vertexColorMap) noexcept;

			/**
			 * @brief Returns whether the use of vertex color is enabled or not.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isVertexColorEnabled () const noexcept
			{
				return m_vertexColorMap != nullptr;
			}

		private:

			/**
			 * @brief Returns The first offset from base grid to build the adaptive grid.
			 * @param baseGrid A reference to the base grid.
			 * @param position A reference to a vector for the position.
			 * @return uint32_t
			 */
			[[nodiscard]]
			uint32_t findStartingOffset (const Libs::VertexFactory::Grid< float > & baseGrid, const Libs::Math::Vector< 3, float > & position) const noexcept;

			/**
			 * @brief Generates the indices buffer once for all.
			 * @return bool
			 */
			bool generateIndicesBuffer () noexcept;

			/**
			 * @brief Adds vertex to vertex attributes buffer.
			 * @param grid A reference to a grid.
			 * @param index The index of the current point in the grid
			 * @return void
			 */
			void addGridPointToVertexAttributes (const Libs::VertexFactory::Grid< float > & grid, uint32_t index) noexcept;

			static constexpr auto DefaultMinimalUpdateDistance{1024.0F};

			std::unique_ptr< Vulkan::VertexBufferObject > m_vertexBufferObject;
			std::unique_ptr< Vulkan::IndexBufferObject > m_indexBufferObject;
			std::vector< float > m_localData;
			std::vector< uint32_t > m_indices;
			Libs::Math::Space3D::AACuboid< float > m_boundingBox;
			Libs::Math::Space3D::Sphere< float > m_boundingSphere;
			float m_minimalUpdateDistance{DefaultMinimalUpdateDistance};
			/* Contains the number of quads in on dimension. Grids are always square. */
			uint32_t m_squareQuadCount{0};
			uint32_t m_quadCount{0};
			/* NOTE: m_squareQuads + 1 */
			uint32_t m_squarePointCount{0};
			uint32_t m_pointCount{0};
			std::shared_ptr< ImageResource > m_vertexColorMap{};
	};
}

/* Expose the resource manager as a convenient type. */
namespace EmEn::Resources
{
	using AdaptiveVertexGridGeometries = Container< Graphics::Geometry::AdaptiveVertexGridResource >;
}
