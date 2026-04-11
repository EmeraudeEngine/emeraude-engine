/*
 * src/Graphics/Geometry/RawVertexResource.hpp
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
#include <cstdint>
#include <memory>
#include <span>

/* Local inclusions for inheritances. */
#include "Interface.hpp"

/* Local inclusions for usages. */
#include "RawGeometryOptions.hpp"
#include "Resources/Container.hpp"

namespace EmEn::Graphics::Geometry
{
	/**
	 * @brief Defines a geometry using a VBO from raw buffer data.
	 * @note Unlike VertexResource, this class does not use Shape as an intermediate.
	 * Data is uploaded directly to the GPU during load() via std::span, accepting
	 * std::vector, std::array, C arrays, or raw {pointer, size} pairs.
	 * The caller retains ownership of its data. No local CPU copy is kept.
	 * @extends EmEn::Graphics::Geometry::Interface The common base for all geometry types.
	 */
	class RawVertexResource final : public Interface
	{
		friend class Resources::Container< RawVertexResource >;

		using ResourceTrait::load;

		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"RawVertexResource"};

			/** @brief Defines the resource dependency complexity. */
			static constexpr auto Complexity{Resources::DepComplexity::One};

			/**
			 * @brief Constructs a raw vertex geometry resource.
			 * @param serviceProvider A reference to the service provider.
			 * @param name The name of the resource [std::move].
			 * @param resourceFlags The geometry resource flag bits. See EmEn::Graphics::Geometry::GeometryFlagBits. Default none.
			 */
			RawVertexResource (Resources::AbstractServiceProvider & serviceProvider, const std::string & name, uint32_t resourceFlags = 0) noexcept
				: Interface{serviceProvider, name, resourceFlags}
			{

			}

			/** @brief Copy constructor. */
			RawVertexResource (const RawVertexResource & copy) noexcept = delete;

			/** @brief Move constructor. */
			RawVertexResource (RawVertexResource && copy) noexcept = delete;

			/** @brief Copy assignment. */
			RawVertexResource & operator= (const RawVertexResource & copy) noexcept = delete;

			/** @brief Move assignment. */
			RawVertexResource & operator= (RawVertexResource && copy) noexcept = delete;

			/** @brief Destructs the raw vertex geometry resource. */
			~RawVertexResource () override
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

			/** @copydoc EmEn::Graphics::Geometry::Interface::isCreated() */
			[[nodiscard]]
			bool
			isCreated () const noexcept override
			{
				return !static_cast< bool >(m_vertexBufferObject == nullptr || !m_vertexBufferObject->isCreated());
			}

			/** @copydoc EmEn::Graphics::Geometry::Interface::topology() */
			[[nodiscard]]
			Topology
			topology () const noexcept override
			{
				return m_topology;
			}

			/** @copydoc EmEn::Graphics::Geometry::Interface::subGeometryCount() */
			[[nodiscard]]
			uint32_t
			subGeometryCount () const noexcept override
			{
				return 1;
			}

			/** @copydoc EmEn::Graphics::Geometry::Interface::subGeometryRange(uint32_t) const */
			[[nodiscard]]
			std::array< uint32_t, 2 >
			subGeometryRange ([[maybe_unused]] uint32_t subGeometryIndex) const noexcept override
			{
				if ( m_vertexBufferObject != nullptr )
				{
					return {0, m_vertexBufferObject->vertexCount()};
				}

				return {0, 0};
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
				return nullptr;
			}

			/** @copydoc EmEn::Graphics::Geometry::Interface::useIndexBuffer() */
			[[nodiscard]]
			bool
			useIndexBuffer () const noexcept override
			{
				return false;
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

			/** @copydoc EmEn::Resources::ResourceTrait::load() */
			bool load () noexcept override;

			/** @copydoc EmEn::Resources::ResourceTrait::load(const std::filesystem::path &) */
			bool load (const std::filesystem::path & filepath) noexcept override;

			/** @copydoc EmEn::Resources::ResourceTrait::load(const Json::Value &) */
			bool load (const Json::Value & data) noexcept override;

			/** @copydoc EmEn::Resources::ResourceTrait::memoryOccupied() const noexcept */
			[[nodiscard]]
			size_t
			memoryOccupied () const noexcept override
			{
				return 0;
			}

			/**
			 * @brief Loads geometry from a pre-interleaved vertex buffer.
			 * @note Accepts std::vector, std::array, C arrays, or raw {pointer, size} pairs.
			 * vertexCount is deduced from vertexAttributes.size() / vertexElementCount.
			 * @param vertexAttributes The interleaved vertex attribute data.
			 * @param vertexElementCount The number of float elements per vertex (stride). Must be >= 3.
			 * @param options The raw geometry options. Default TriangleList with auto-computed bounds.
			 * @return bool
			 */
			bool load (std::span< const float > vertexAttributes, uint32_t vertexElementCount, const RawGeometryOptions & options = {}) noexcept;

			/**
			 * @brief Loads geometry from separate attribute arrays, interleaved by the engine.
			 * @note Accepts std::vector, std::array, C arrays, or raw {pointer, size} pairs.
			 * vertexCount is deduced from positions.size() / 3.
			 * Geometry flags (EnableNormal, etc.) are set automatically from non-empty spans.
			 * Strides: positions=3, normals=3, textureCoordinates=2, colors=4 (RGBA).
			 * @param positions Position data (3 floats per vertex). Required.
			 * @param normals Normal data (3 floats per vertex). Empty if absent.
			 * @param textureCoordinates UV data (2 floats per vertex). Empty if absent.
			 * @param colors RGBA color data (4 floats per vertex). Empty if absent.
			 * @param options The raw geometry options. Default TriangleList with auto-computed bounds.
			 * @return bool
			 */
			bool load (std::span< const float > positions, std::span< const float > normals = {}, std::span< const float > textureCoordinates = {}, std::span< const float > colors = {}, const RawGeometryOptions & options = {}) noexcept;

		private:

			/**
			 * @brief Uploads pre-interleaved vertex data to the GPU.
			 * @note Must be called after beginLoading(). Does NOT call beginLoading/setLoadSuccess.
			 * @return bool
			 */
			[[nodiscard]]
			bool uploadToGPU (const float * vertexAttributes, uint32_t vertexCount, uint32_t vertexElementCount, const RawGeometryOptions & options) noexcept;

			/** @brief Computes bounding volumes from explicit min/max points. */
			static void computeBoundingVolumes (Libs::Math::Space3D::AACuboid< float > & boundingBox, Libs::Math::Space3D::Sphere< float > & boundingSphere, const Libs::Math::Space3D::Point< float > & minimum, const Libs::Math::Space3D::Point< float > & maximum) noexcept;

			/** @brief Auto-computes bounding volumes from position data with given stride. */
			static void computeBoundingVolumes (Libs::Math::Space3D::AACuboid< float > & boundingBox, Libs::Math::Space3D::Sphere< float > & boundingSphere, const float * positions, uint32_t vertexCount, uint32_t stride) noexcept;

			std::unique_ptr< Vulkan::VertexBufferObject > m_vertexBufferObject;
			Libs::Math::Space3D::AACuboid< float > m_boundingBox;
			Libs::Math::Space3D::Sphere< float > m_boundingSphere;
			Topology m_topology{Topology::TriangleList};
	};
}

/* Expose the resource manager as a convenient type. */
namespace EmEn::Resources
{
	using RawVertexGeometries = Container< Graphics::Geometry::RawVertexResource >;
}
