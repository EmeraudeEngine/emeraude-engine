/*
 * tools/stress/ResourceJobsStress.cpp
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

/* STL inclusions. */
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <iostream>
#include <memory>
#include <string>

/* Local inclusions. */
#include "Core.hpp"
#include "Graphics/Geometry/IndexedVertexResource.hpp"
#include "Graphics/Material/StandardResource.hpp"
#include "Graphics/Renderable/MeshResource.hpp"
#include "PlatformManager.hpp"
#include "Resources/Container.hpp"
#include "SettingKeys.hpp"
#include "VertexFactory/ShapeGenerator.hpp"

/*
 * The ThreadSanitizer proof of the owned resource jobs (Ave Robustus II, phase P2, 2026-10-15; built with
 * EMERAUDE_ENABLE_TSAN only). A window-less, CEF-less engine creates meshes without pause — the geometry and the mesh
 * through the containers' asynchronous loads, each mesh starting its automatic LOD job — and drops them again: the main
 * thread unloads the unused resources, so meshes die with their LOD job queued or running. Released CPU copies are asked
 * back (a local-data reload job, a GPU readback). The run then stops in the middle of it: Core::terminate() stops the
 * resource manager's jobs while loads, LODs and reloads are in flight. The verdict is TSan's: run it with
 * TSAN_OPTIONS=halt_on_error=1 and a settings file enabling the CPU-copy release (procedure: projet-alpha
 * docs/unit-testing.md § ThreadSanitizer).
 */
namespace
{
	using namespace EmEn;

	/* How long the creations run, the pause between two of them, and how many meshes stay alive. */
	constexpr std::chrono::seconds RunDuration{20};
	constexpr std::chrono::milliseconds CreationPeriod{50};
	constexpr std::chrono::milliseconds UnloadPeriod{400};
	constexpr size_t KeptMeshCount{6};

	class ResourceJobsStress final : public Core
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"ResourceJobsStress"};

			ResourceJobsStress (int argc, char * * argv) noexcept
				: Core{argc, argv, "EmeraudeResourceJobsStress", {1, 0, 0}, "EmeraudeEngine", "emeraude-engine.org"}
			{

			}

			ResourceJobsStress (const ResourceJobsStress & copy) noexcept = delete;
			ResourceJobsStress (ResourceJobsStress && copy) noexcept = delete;
			ResourceJobsStress & operator= (const ResourceJobsStress & copy) noexcept = delete;
			ResourceJobsStress & operator= (ResourceJobsStress && copy) noexcept = delete;

			~ResourceJobsStress () override = default;

			[[nodiscard]]
			size_t
			classUID () const noexcept override
			{
				return Base::Hash::FNV1a(ClassId);
			}

			[[nodiscard]]
			bool
			is (size_t classUID) const noexcept override
			{
				return classUID == Base::Hash::FNV1a(ClassId);
			}

			/** @brief Whether the run created meshes (a refused start is a failed run). */
			[[nodiscard]]
			bool
			created () const noexcept
			{
				return m_creations > 0;
			}

		private:

			bool
			onCoreStarted (const Arguments & /*arguments*/, Settings & settings) noexcept override
			{
				/* NOTE: Read by each mesh at its load: every mesh below starts an automatic LOD job. */
				settings.set(GraphicsLODEnableAutomaticGenerationKey, true);

				m_material = this->resourceManager().container< Graphics::Material::StandardResource >()->getDefaultResource();

				if ( m_material == nullptr )
				{
					TraceError{ClassId} << "No default material !";

					return false;
				}

				m_start = std::chrono::steady_clock::now();
				m_lastCreation = m_start;
				m_lastUnload = m_start;

				TraceInfo{ClassId} << "Creating meshes with automatic LODs for " << RunDuration.count() << " s.";

				return true;
			}

			void
			onCoreMainLoopCycle () noexcept override
			{
				if ( m_stopping )
				{
					return;
				}

				const auto now = std::chrono::steady_clock::now();

				if ( now - m_start >= RunDuration )
				{
					/* NOTE: In the middle of the work: loads, LOD jobs and reloads are in flight. */
					m_stopping = true;

					TraceInfo{ClassId} << m_creations << " meshes created, " << m_unloaded << " resources unloaded, " << m_reloadRequests << " local-data requests; stopping with " << m_meshes.size() << " meshes alive.";

					this->stop();

					return;
				}

				if ( now - m_lastCreation >= CreationPeriod )
				{
					m_lastCreation = now;

					this->createMesh();
				}

				if ( now - m_lastUnload >= UnloadPeriod )
				{
					m_lastUnload = now;

					/* NOTE: Main thread: the dropped meshes die here, their LOD job queued or running. */
					m_unloaded += this->resourceManager().unloadUnusedResources();

					/* A released CPU copy comes back on a worker (ResourceTrait::requestLocalData()). */
					for ( const auto & geometry : m_geometries )
					{
						static_cast< void >(geometry->requestLocalData());

						++m_reloadRequests;
					}
				}
			}

			bool
			onBeforeCoreStop () noexcept override
			{
				/* NOTE: The services are still up: the application drops its GPU resources here, so Core::terminate()
				 * unloads them before the renderer goes. Held past it, they would be destroyed by ~Core() after the device
				 * (a VMA allocator gone: a crash). */
				m_meshes.clear();
				m_geometries.clear();
				m_material.reset();

				return true;
			}

			void
			onCoreProcessLogics (size_t /*engineCycle*/) noexcept override
			{
				/* NOTE: The logic thread runs the engine's own resource work (the CPU-copy release pass scheduling). */
			}

			/** @brief Creates one geometry and one mesh, asynchronously, and drops the oldest ones. */
			void
			createMesh () noexcept
			{
				const auto index = m_creations++;
				/* NOTE: ~6 k to ~25 k triangles: three or four LOD levels each, a few hundred ms of decimation under TSan. */
				const auto slices = static_cast< uint32_t >(56U + ((index % 4U) * 24U));
				const auto name = std::string{"ResourceJobsStress"} + std::to_string(index);

				auto geometry = this->resourceManager().container< Graphics::Geometry::IndexedVertexResource >()->getOrCreateResource(name + "Geometry", [slices] (Graphics::Geometry::IndexedVertexResource & geometryResource) {
					return geometryResource.load(Base::VertexFactory::ShapeGenerator::generateSphere< float, uint32_t >(1.0F, slices, slices / 2U));
				});

				auto mesh = this->resourceManager().container< Graphics::Renderable::MeshResource >()->getOrCreateResource(name, [geometry, material = m_material] (Graphics::Renderable::MeshResource & meshResource) {
					return meshResource.load(geometry, material);
				});

				m_geometries.emplace_back(std::move(geometry));
				m_meshes.emplace_back(std::move(mesh));

				while ( m_meshes.size() > KeptMeshCount )
				{
					m_meshes.pop_front();
					m_geometries.pop_front();
				}
			}

			std::shared_ptr< Graphics::Material::Interface > m_material;
			std::deque< std::shared_ptr< Graphics::Geometry::IndexedVertexResource > > m_geometries{};
			std::deque< std::shared_ptr< Graphics::Renderable::MeshResource > > m_meshes{};
			std::chrono::steady_clock::time_point m_start;
			std::chrono::steady_clock::time_point m_lastCreation;
			std::chrono::steady_clock::time_point m_lastUnload;
			uint64_t m_creations{0};
			uint64_t m_unloaded{0};
			uint64_t m_reloadRequests{0};
			bool m_stopping{false};
	};
}

int
main (int argc, char * * argv)
{
	/* NOTE: Before any thread, as every consumer's main() (PlatformManager::prepareVulkanLoaderEnvironment()). */
	static_cast< void >(EmEn::PlatformManager::prepareVulkanLoaderEnvironment());

	ResourceJobsStress stress{argc, argv};

	const auto exitCode = stress.run();

	if ( !stress.created() )
	{
		std::cerr << "[ResourceJobsStress] No mesh was created: the run proves nothing !" "\n";

		return 1;
	}

	return exitCode;
}
