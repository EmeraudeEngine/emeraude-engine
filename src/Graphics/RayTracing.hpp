/*
 * src/Graphics/RayTracing.hpp
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

/* Engine configuration. */
#include "emeraude_export.hpp"

/* STL inclusions. */
#include <cstdint>
#include <memory>
#include <optional>

/* Local inclusions for inheritances. */
#include "ServiceInterface.hpp"

/* Local inclusions for usages. */
#include "Constants.hpp"
#include "RayTracingFrame.hpp"
#include "StaticVector.hpp"

/* Forward declarations. */
namespace EmEn
{
	namespace Graphics
	{
		class IrradianceProbeVolume;
		class Renderer;
		class SkinnedGeometryProcessor;
	}

	namespace Scenes
	{
		class LightSet;
		class SceneMetaData;
	}

	namespace Vulkan
	{
		class AccelerationStructureBuilder;
		class DescriptorSet;
		class DescriptorSetLayout;
		class ShaderStorageBufferObject;
	}
}

namespace EmEn::Graphics
{
	/**
	 * @brief The ray tracing of a session: everything the renderer owns to trace, as ONE sub-service.
	 * @details It groups what exists together or not at all:
	 * - the acceleration structure builder, shared by every geometry (BLAS) and every scene (TLAS);
	 * - the ray-tracing descriptor set layout — the set 0 of every traced pass — and one descriptor set per frame in
	 *   flight;
	 * - the skinned geometry processor (optional: without it skinned meshes are absent from the traced effects);
	 * - the irradiance probe volume (optional: without it the effects that read the radiance cache are not created).
	 *
	 * usable() IS the session's answer to "can anything be traced": the renderer starts the service when the device
	 * traces and the lighting lane may need it, and everything else asks this service — never a pointer of its own.
	 *
	 * A frame follows three steps, on the rendering thread:
	 * 1. a FrameGuard opens the frame: nothing is bound, whatever the previous frame or the previous scene left;
	 * 2. once the frame's acceleration structure build is recorded, bindFrame() writes this frame's descriptor set
	 *    from the scene being rendered and makes the frame's RayTracingFrame;
	 * 3. the traced passes receive it (IndirectPostProcessEffect::FrameContext::rayTracing); the guard ends it with
	 *    the frame.
	 * Nothing here keeps a pointer to an object a scene owns: the acceleration structure is read during bindFrame()
	 * and its handle written into the descriptor set, where the deferred destructor keeps it alive for the frames in
	 * flight.
	 * @extends EmEn::ServiceInterface A sub-service of the renderer.
	 */
	class EMEN_API RayTracing final : public ServiceInterface
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"RayTracingService"};

			/**
			 * @brief Bounds what a frame traces to that frame: nothing bound when it opens, nothing left when it
			 * closes — on every exit path of the frame function.
			 */
			class FrameGuard final
			{
				public:

					/**
					 * @brief Opens a frame.
					 * @param rayTracing A reference to the renderer's ray-tracing service (usable or not).
					 */
					explicit
					FrameGuard (RayTracing & rayTracing) noexcept
						: m_rayTracing{rayTracing}
					{
						m_rayTracing.m_frame.reset();
					}

					FrameGuard (const FrameGuard & copy) noexcept = delete;

					FrameGuard (FrameGuard && copy) noexcept = delete;

					FrameGuard & operator= (const FrameGuard & copy) noexcept = delete;

					FrameGuard & operator= (FrameGuard && copy) noexcept = delete;

					/** @brief Closes the frame. */
					~FrameGuard ()
					{
						m_rayTracing.m_frame.reset();
					}

				private:

					RayTracing & m_rayTracing;
			};

			/**
			 * @brief Constructs the ray-tracing service.
			 * @param renderer A reference to the graphics renderer (device, descriptor pool, frames in flight).
			 */
			explicit RayTracing (Renderer & renderer) noexcept;

			RayTracing (const RayTracing & copy) noexcept = delete;

			RayTracing (RayTracing && copy) noexcept = delete;

			RayTracing & operator= (const RayTracing & copy) noexcept = delete;

			RayTracing & operator= (RayTracing && copy) noexcept = delete;

			/** @brief Destructs the ray-tracing service. */
			~RayTracing () override;

			/**
			 * @brief Returns the acceleration structure builder, shared by every geometry (BLAS) and every scene (TLAS).
			 * @note Null when the service is not usable: no geometry builds a BLAS and no scene a TLAS, for the whole
			 * session (a BLAS is built when a geometry loads and cannot be gained afterwards).
			 * @return Vulkan::AccelerationStructureBuilder *
			 */
			[[nodiscard]]
			Vulkan::AccelerationStructureBuilder *
			accelerationStructureBuilder () const noexcept
			{
				return m_accelerationStructureBuilder.get();
			}

			/**
			 * @brief Returns the ray-tracing descriptor set layout: what a traced pass declares as its set 0 (binding 0
			 * the acceleration structure, 1 the mesh metadata, 2 the material data, 3 the lights, 4 the sub-geometry table).
			 * @note Session-constant. Null when the service is not usable.
			 * @return const std::shared_ptr< Vulkan::DescriptorSetLayout > &
			 */
			[[nodiscard]]
			const std::shared_ptr< Vulkan::DescriptorSetLayout > &
			setLayout () const noexcept
			{
				return m_setLayout;
			}

			/**
			 * @brief Returns the skinned geometry processor: the compute pass that skins vertices into the per-instance
			 * mirror buffers of the per-frame BLAS refit.
			 * @note Null when the service is not usable or the processor could not be initialized: skinned meshes are
			 * then absent from the traced effects.
			 * @return const SkinnedGeometryProcessor *
			 */
			[[nodiscard]]
			const SkinnedGeometryProcessor *
			skinnedGeometryProcessor () const noexcept
			{
				return m_skinnedGeometryProcessor.get();
			}

			/**
			 * @brief Returns the irradiance probe volume: the radiance cache the traced effects read at their hits.
			 * @note Null when the service is not usable or the volume could not be initialized: the effects that need
			 * it are then not created (the slot falls back to its screen-space occupant).
			 * @return IrradianceProbeVolume *
			 */
			[[nodiscard]]
			IrradianceProbeVolume *
			irradianceProbeVolume () const noexcept
			{
				return m_irradianceProbeVolume.get();
			}

			/**
			 * @brief Writes the descriptor set of the frame being rendered from its scene, and makes what the frame traces.
			 * @note RENDERING thread, once the frame's acceleration structure build is recorded. Only this frame's set is
			 * written: the others may be pending on the GPU.
			 * @param frameIndex The index of the frame in flight being rendered.
			 * @param sceneMetaData A reference to the ray-tracing meta data of the scene being rendered.
			 * @param lightSet A reference to that scene's light set.
			 * @return const RayTracingFrame * A null pointer when this frame does not trace: the service is not usable,
			 * the scene holds no created acceleration structure, or a binding could not be written (traced).
			 */
			const RayTracingFrame * bindFrame (uint32_t frameIndex, const Scenes::SceneMetaData & sceneMetaData, const Scenes::LightSet & lightSet) noexcept;

			/**
			 * @brief Returns what the frame being rendered traces.
			 * @note RENDERING thread. Null before bindFrame() succeeded in this frame, and outside a frame.
			 * @return const RayTracingFrame *
			 */
			[[nodiscard]]
			const RayTracingFrame *
			frame () const noexcept
			{
				return m_frame.has_value() ? &*m_frame : nullptr;
			}

		private:

			/** @copydoc EmEn::ServiceInterface::onInitialize() */
			bool onInitialize () noexcept override;

			/** @copydoc EmEn::ServiceInterface::onTerminate() */
			bool onTerminate () noexcept override;

			/**
			 * @brief Creates the descriptor set layout, the stand-in storage buffer and one descriptor set per frame in flight.
			 * @return bool
			 */
			[[nodiscard]]
			bool createDescriptorSets () noexcept;

			Renderer & m_renderer;
			/* NOTE: Declared in creation order: released in the reverse one (onTerminate() does it explicitly, while the
			 * device still exists). */
			std::unique_ptr< Vulkan::AccelerationStructureBuilder > m_accelerationStructureBuilder;
			std::shared_ptr< Vulkan::DescriptorSetLayout > m_setLayout;
			/** @brief What a storage binding the scene does not provide receives: the traced programs statically use
			 * every binding, so none may stay unwritten. Four zero vec4 read as an empty array. */
			std::unique_ptr< Vulkan::ShaderStorageBufferObject > m_standInSSBO;
			/** @brief One per frame in flight: a frame writes its own, the others may be pending on the GPU. */
			Base::StaticVector< std::unique_ptr< Vulkan::DescriptorSet >, MaxFramesInFlight > m_descriptorSets;
			std::unique_ptr< SkinnedGeometryProcessor > m_skinnedGeometryProcessor;
			std::unique_ptr< IrradianceProbeVolume > m_irradianceProbeVolume;
			/** @brief What the frame being rendered traces (FrameGuard, bindFrame()). */
			std::optional< RayTracingFrame > m_frame;
	};
}
