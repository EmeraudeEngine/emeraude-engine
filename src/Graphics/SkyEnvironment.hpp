/*
 * src/Graphics/SkyEnvironment.hpp
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
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>

/* Local inclusions for usages. */
#include "Constants.hpp"
#include "Graphics/Compute/IBLBaker.hpp"
#include "Math/Vector.hpp"
#include "Mutex.hpp"
#include "Vulkan/DescriptorSet.hpp"

/* Forward declarations. */
namespace EmEn::Vulkan
{
	class CommandBuffer;
	class ComputePipeline;
	class DescriptorPool;
	class DescriptorSetLayout;
	class PipelineLayout;
}

namespace EmEn::Graphics
{
	class IBLTexture;
	class Renderer;
	class SkyAtmosphere;
}

namespace EmEn::Graphics
{
	/**
	 * @brief The environment of a physical sky for the image-based lighting: the sky captured into a small cubemap, and
	 * its irradiance and GGX-prefiltered bakes, AMORTISED one stage per frame and published as a complete set.
	 * @details Owner decisions (2026-10-25; graphics doc 42 § "The environment"):
	 * - the source is a COMPUTE capture of this frame's sky-view table (AtmosphereGLSL::SkyViewLookup, the planet's
	 *   ground included), 64² with its full mip chain — never a render of the sky material;
	 * - ABSOLUTE radiance (nits): the star's illuminance applied, the sky's artistic grading applied (what lights is what
	 *   is seen); the dynamic sky then declares a luminance of 1 to the scene;
	 * - the star's DISC is not captured (it lights through its directional light); its Mie halo is (it is sky);
	 * - one stage per frame: the capture and its mips, the irradiance, then one prefilter mip per frame — 2 + 6 frames a
	 *   cycle; a cycle starts when the star moved more than TriggerAngle, its illuminance or the grading changed, or
	 *   nothing was baked yet;
	 * - a RING of MaxFramesInFlight + 1 sets, allocated once: a cycle writes the set published RingSize − 1 cycles ago,
	 *   which no frame in flight can still read.
	 * @note RENDER THREAD: recordStage() and onSubmitted() (the sky's command buffer, Renderer::renderSkyAtmosphere()).
	 * Any thread: takePublication(), under a mutex — the scene takes it on the logic thread (Scene::updateEnvironmentIBL()).
	 * @note References: Unity's time-sliced reflection probes (one face or one mip per frame); Unreal's sky light "Real
	 * Time Capture".
	 */
	class EMEN_API SkyEnvironment final
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"SkyEnvironment"};

			/** @brief The sets of the ring: a frame in flight never reads the set being written. */
			static constexpr size_t RingSize{MaxFramesInFlight + 1};

			/** @brief The star's motion, in degrees, that starts a new cycle (owner decision 2026-10-25: 0.25°). */
			static constexpr float TriggerAngle{0.25F};

			/** @brief The relative change of the star's illuminance (or of the grading) that starts a new cycle. */
			static constexpr float TriggerRatio{0.01F};

			/**
			 * @brief The sky's artistic grading, applied to the capture as to the drawn sky.
			 */
			struct Grading
			{
				/** @brief The tint (linear RGB). */
				Base::Math::Vector< 3, float > tint{1.0F, 1.0F, 1.0F};
				/** @brief The intensity. */
				float intensity{1.0F};
				/** @brief The saturation around the luminance (1 = neutral). */
				float saturation{1.0F};
			};

			/**
			 * @brief What a cycle is captured from: the star and the grading of the frame that starts it.
			 */
			struct Inputs
			{
				/** @brief The unit world vector toward the star. */
				Base::Math::Vector< 3, float > sunDirection{0.0F, 1.0F, 0.0F};
				/** @brief The star's illuminance above the atmosphere × its colour, in lux. */
				Base::Math::Vector< 3, float > sunIlluminance{0.0F, 0.0F, 0.0F};
				/** @brief The sky's artistic grading. */
				Grading grading{};
			};

			/**
			 * @brief A complete set, ready for the scene's IBL slots.
			 */
			struct Publication
			{
				/** @brief The captured sky (the scene's environment slot: refraction, RTGI misses, the probe volume). */
				std::shared_ptr< IBLTexture > environment;
				/** @brief Its irradiance (the ambient's diffuse IBL). */
				std::shared_ptr< IBLTexture > irradiance;
				/** @brief Its GGX-prefiltered chain (the reflections). */
				std::shared_ptr< IBLTexture > prefiltered;
				/** @brief The unit vector toward the star when the set was captured (world, Y up). */
				Base::Math::Vector< 3, float > sunDirection{0.0F, 1.0F, 0.0F};
			};

			/**
			 * @brief Constructs the environment of a sky (nothing on the GPU before the first recordStage()).
			 * @param name The sky's name, for the labels and the traces.
			 */
			explicit SkyEnvironment (std::string name) noexcept;

			SkyEnvironment (const SkyEnvironment &) = delete;

			SkyEnvironment & operator= (const SkyEnvironment &) = delete;

			SkyEnvironment (SkyEnvironment &&) = delete;

			SkyEnvironment & operator= (SkyEnvironment &&) = delete;

			/**
			 * @brief Destructs the environment.
			 * @note The owner (SkyAtmosphere) outlives every frame that recorded with it (Renderer's atmospheres in flight).
			 */
			~SkyEnvironment ();

			/**
			 * @brief Records this frame's stage of the amortised bake, or nothing when no cycle runs.
			 * @note RENDER THREAD, in the sky's command buffer, AFTER the sky-view pass and its barrier: the capture reads
			 * this frame's table. The first call creates the GPU objects; a failure is traced once and the environment
			 * stays idle (the scene keeps its previous IBL).
			 * @param renderer A reference to the renderer.
			 * @param commandBuffer A reference to the recording command buffer.
			 * @param atmosphere A reference to the sky's atmosphere (its tables, created).
			 * @param inputs The frame's star and grading.
			 */
			void recordStage (Renderer & renderer, const Vulkan::CommandBuffer & commandBuffer, const SkyAtmosphere & atmosphere, const Inputs & inputs) noexcept;

			/**
			 * @brief Closes the command buffer recordStage() wrote into. Submitted: a set completed in it is released (no
			 * frame can bind it before its bake is on the queue). Not submitted: the stages recorded in it never run —
			 * the cycle is dropped and starts over.
			 * @note RENDER THREAD, after the submission attempt.
			 * @param submitted Whether the command buffer was submitted.
			 */
			void onSubmitted (bool submitted) noexcept;

			/**
			 * @brief Takes the last completed set, if a new one is available.
			 * @note Any thread (the scene: the logic thread).
			 * @return std::optional< Publication >
			 */
			[[nodiscard]]
			std::optional< Publication > takePublication () noexcept;

		private:

			/** @brief The stage the next frame records. */
			enum class Stage : uint8_t
			{
				Idle,
				Capture,
				Irradiance,
				Prefilter
			};

			/** @brief One set of the ring: the capture and its two bakes, with their descriptor sets. */
			struct RingSet
			{
				std::shared_ptr< IBLTexture > capture;
				std::shared_ptr< IBLTexture > irradiance;
				std::shared_ptr< IBLTexture > prefiltered;
				std::unique_ptr< Compute::IBLBaker::EnvironmentBindings > bindings;
				std::unique_ptr< Vulkan::DescriptorSet > captureSet;
			};

			/**
			 * @brief Creates the capture pipeline and the ring.
			 * @param renderer A reference to the renderer.
			 * @param atmosphere A reference to the sky's atmosphere.
			 * @return bool
			 */
			[[nodiscard]]
			bool create (Renderer & renderer, const SkyAtmosphere & atmosphere) noexcept;

			/**
			 * @brief Returns whether a new cycle must start.
			 * @param inputs The frame's star (its direction normalized) and grading.
			 * @return bool
			 */
			[[nodiscard]]
			bool cycleNeeded (const Inputs & inputs) const noexcept;

			/**
			 * @brief Records the capture of the sky into a set's cubemap and the blit cascade of its mips.
			 * @param commandBuffer A reference to the recording command buffer.
			 * @param set A reference to the set.
			 * @param grading The grading.
			 */
			void recordCapture (const Vulkan::CommandBuffer & commandBuffer, const RingSet & set, const Grading & grading) const noexcept;

			std::string m_name;
			std::array< RingSet, RingSize > m_ring{};
			std::shared_ptr< Vulkan::DescriptorSetLayout > m_captureLayout;
			std::shared_ptr< Vulkan::PipelineLayout > m_pipelineLayout;
			std::unique_ptr< Vulkan::ComputePipeline > m_capturePipeline;
			std::shared_ptr< Vulkan::DescriptorPool > m_descriptorPool;
			/** @brief Render thread only: the set completed in the command buffer being recorded. */
			std::optional< Publication > m_completed;
			/** @brief The set released after its submission, until the scene takes it (m_publicationAccess). */
			std::optional< Publication > m_publication;
			/** @brief The inputs the running (or last) cycle was captured from. */
			Inputs m_cycle{};
			size_t m_ringIndex{0};
			uint32_t m_prefilterMip{0};
			Stage m_stage{Stage::Idle};
			mutable Base::Mutex m_publicationAccess;
			bool m_created{false};
			bool m_failed{false};
			bool m_baked{false};
			bool m_restart{false};
	};
}
