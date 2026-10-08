/*
 * src/Graphics/Geometry/PulledVertexResource.hpp
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
#include <array>
#include <atomic>
#include <cstdint>
#include <string>

/* Local inclusions for inheritances. */
#include "Interface.hpp"

/* Forward declarations. */
namespace EmEn::Resources
{
	template< typename resource_t >
	class Container;
}

namespace EmEn::Graphics::Geometry
{
	/**
	 * @brief A geometry WITHOUT a vertex buffer: its vertex stage builds every vertex from gl_VertexIndex, reading its
	 * data elsewhere ("vertex pulling" — a Scenes::Component::Path reads its points from the scene's per-frame SSBO).
	 * @note No vertex attribute is declared, nothing is bound (Vulkan requires a buffer only for a binding an
	 * attribute reads), and the draw count is subGeometryRange(0)[1]: the classic draw path
	 * (Vulkan::CommandBuffer::drawWithFirstInstance()) already draws that range.
	 * @note The vertex count is a CAPACITY that only grows (setVertexCapacity()): the render thread reads it when it
	 * records the draw while the logic thread may be publishing more points; the vertex stage collapses every vertex
	 * past the frame's own point count. It never shrinks, so a draw can never be shorter than the data it pulls.
	 * @note No ray tracing (no vertex data to build a BLAS from), no shadow casting (the depth-only programs expect
	 * attributes): the owner disables both on the instance.
	 * @extends EmEn::Graphics::Geometry::Interface
	 */
	class EMEN_API PulledVertexResource final : public Interface
	{
		friend class Resources::Container< PulledVertexResource >;

		using ResourceTrait::load;

		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"PulledVertexResource"};

			/** @brief Defines the resource dependency complexity. */
			static constexpr auto Complexity{Resources::DepComplexity::None};

			/**
			 * @brief Constructs a pulled-vertex geometry.
			 * @param serviceProvider A reference to the service provider.
			 * @param name A string for the resource name.
			 * @param resourceFlags The resource flag bits. Default none.
			 */
			PulledVertexResource (Resources::AbstractServiceProvider & serviceProvider, const std::string & name, uint32_t resourceFlags = 0) noexcept
				: Interface{serviceProvider, name, resourceFlags}
			{

			}

			/** @brief Deleted copy and move. */
			PulledVertexResource (const PulledVertexResource & copy) noexcept = delete;
			PulledVertexResource (PulledVertexResource && copy) noexcept = delete;
			PulledVertexResource & operator= (const PulledVertexResource & copy) noexcept = delete;
			PulledVertexResource & operator= (PulledVertexResource && copy) noexcept = delete;

			/** @brief Destructs the geometry. */
			~PulledVertexResource () override = default;

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

			/** @copydoc EmEn::Graphics::Geometry::Interface::isCreated() */
			[[nodiscard]]
			bool
			isCreated () const noexcept override
			{
				return m_created;
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

			/**
			 * @copydoc EmEn::Graphics::Geometry::Interface::subGeometryRange()
			 * @note {0, the vertex capacity}.
			 */
			[[nodiscard]]
			std::array< uint32_t, 2 >
			subGeometryRange (uint32_t /*subGeometryIndex*/) const noexcept override
			{
				return {0, m_vertexCapacity.load(std::memory_order_acquire)};
			}

			/** @copydoc EmEn::Graphics::Geometry::Interface::boundingBox() */
			[[nodiscard]]
			const Base::Math::Space3D::AACuboid< float > &
			boundingBox () const noexcept override
			{
				return m_boundingBox;
			}

			/** @copydoc EmEn::Graphics::Geometry::Interface::boundingSphere() */
			[[nodiscard]]
			const Base::Math::Space3D::Sphere< float > &
			boundingSphere () const noexcept override
			{
				return m_boundingSphere;
			}

			/**
			 * @copydoc EmEn::Graphics::Geometry::Interface::vertexBufferObject()
			 * @note Always nullptr: nothing is bound.
			 */
			[[nodiscard]]
			const Vulkan::VertexBufferObject *
			vertexBufferObject () const noexcept override
			{
				return nullptr;
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

			/**
			 * @copydoc EmEn::Graphics::Geometry::Interface::createOnHardware()
			 * @note Nothing to upload.
			 */
			bool
			createOnHardware (Vulkan::TransferManager & /*transferManager*/) noexcept override
			{
				m_created = true;

				return true;
			}

			/** @copydoc EmEn::Graphics::Geometry::Interface::updateVideoMemory() */
			bool
			updateVideoMemory () noexcept override
			{
				return true;
			}

			/** @copydoc EmEn::Graphics::Geometry::Interface::destroyFromHardware() */
			void
			destroyFromHardware (bool /*clearLocalData*/) noexcept override
			{
				m_created = false;
			}

			/** @copydoc EmEn::Resources::ResourceTrait::classLabel() const */
			[[nodiscard]]
			const char *
			classLabel () const noexcept override
			{
				return ClassId;
			}

			/**
			 * @copydoc EmEn::Resources::ResourceTrait::load()
			 * @note A triangle strip of no vertex yet: setVertexCapacity() sizes it.
			 */
			bool load () noexcept override;

			/**
			 * @copydoc EmEn::Resources::ResourceTrait::load(const Json::Value &)
			 * @note Not loadable from data: refused.
			 */
			bool load (const Json::Value & data) noexcept override;

			/** @copydoc EmEn::Resources::ResourceTrait::memoryOccupied() const */
			[[nodiscard]]
			size_t
			memoryOccupied () const noexcept override
			{
				return sizeof(*this);
			}

			/**
			 * @brief Loads the geometry with a topology and an initial vertex capacity.
			 * @param topology The topology the vertex stage builds (a ribbon: TriangleStrip).
			 * @param vertexCapacity The initial capacity.
			 * @return bool
			 */
			bool load (Topology topology, uint32_t vertexCapacity) noexcept;

			/**
			 * @brief Grows the vertex capacity to at least this many vertices (never shrinks: see the class note).
			 * @note Any thread.
			 * @param vertexCount The number of vertices the next data will need.
			 */
			void
			setVertexCapacity (uint32_t vertexCount) noexcept
			{
				auto current = m_vertexCapacity.load(std::memory_order_relaxed);

				while ( vertexCount > current && !m_vertexCapacity.compare_exchange_weak(current, vertexCount, std::memory_order_release, std::memory_order_relaxed) )
				{
					/* current was reloaded by the failed exchange. */
				}
			}

			/**
			 * @brief Sets the local bounds (the renderable's culling volume; the component keeps them up to date).
			 * @param boundingBox A reference to the box.
			 * @param boundingSphere A reference to the sphere.
			 */
			void
			setBounds (const Base::Math::Space3D::AACuboid< float > & boundingBox, const Base::Math::Space3D::Sphere< float > & boundingSphere) noexcept
			{
				m_boundingBox = boundingBox;
				m_boundingSphere = boundingSphere;
			}

		private:

			Base::Math::Space3D::AACuboid< float > m_boundingBox;
			Base::Math::Space3D::Sphere< float > m_boundingSphere;
			std::atomic< uint32_t > m_vertexCapacity{0};
			Topology m_topology{Topology::TriangleStrip};
			bool m_created{false};
	};
}

/* Expose the resource manager as a convenient type. */
namespace EmEn::Resources
{
	using PulledVertexGeometries = Container< Graphics::Geometry::PulledVertexResource >;
}
