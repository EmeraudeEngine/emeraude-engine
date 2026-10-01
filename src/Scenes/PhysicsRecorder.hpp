/*
 * src/Scenes/PhysicsRecorder.hpp
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

/* STL inclusions. */
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

/* Project configuration. */
#include "emeraude_export.hpp"

/* Local inclusions for usages. */
#include "Math/Vector.hpp"
#include "Physics/MovableTrait.hpp"

namespace EmEn::Scenes
{
	class Node;
	class Scene;

	/**
	 * @brief Records the physics state of chosen root nodes on EVERY logic cycle of a cycle range, on the logic thread.
	 * @note A measurement tool (physics overhaul, engine docs/physics-overhaul.md § 6). Polling `getNodePhysics()` from
	 * the console samples a node about every 9 cycles, with a phase that changes per launch, and can label a sample
	 * one cycle off (the console reads while the logic thread ticks). The recorder samples inside the tick, right after
	 * the collisions are resolved, so two runs cover the same cycles and every sample is consistent.
	 * @note Threads: start(), stop(), status() and write() run on the console thread; sample() on the logic thread. One
	 * mutex guards the state; nothing is notified under it. The memory is reserved by start(), so sample() never
	 * allocates; the file is written by write(), never by the logic thread.
	 */
	class EMEN_API PhysicsRecorder final
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"PhysicsRecorder"};

			/** @brief The most nodes one recording follows. */
			static constexpr size_t MaxNodes{64};

			/** @brief The longest recording: 10 minutes at 60 Hz. */
			static constexpr size_t MaxCycles{36000};

			/**
			 * @brief Constructs an idle recorder.
			 */
			PhysicsRecorder () noexcept = default;

			/**
			 * @brief Starts a recording, replacing any previous one.
			 * @param nodeNames The names of the ROOT nodes to follow (1 to MaxNodes). A name not found when the recording
			 * begins is recorded as missing.
			 * @param firstCycle The first scene cycle to record. A cycle already past starts the recording at once.
			 * @param cycleCount The number of cycles to record (1 to MaxCycles).
			 * @param error A reference to a string receiving the reason of a refusal.
			 * @return bool False when the parameters are refused.
			 */
			[[nodiscard]]
			bool start (std::vector< std::string > nodeNames, size_t firstCycle, size_t cycleCount, std::string & error) noexcept;

			/**
			 * @brief Stops the recording; the cycles recorded so far stay available to write().
			 * @return void
			 */
			void stop () noexcept;

			/**
			 * @brief Records one cycle of the scene when it falls in the range. Called by Scene::processLogics() after the
			 * collisions are resolved.
			 * @param scene A reference to the scene.
			 * @return void
			 */
			void sample (const Scene & scene) noexcept;

			/**
			 * @brief Returns the state as a JSON object: state, first cycle, cycle count, recorded cycles, nodes.
			 * @return std::string
			 */
			[[nodiscard]]
			std::string status () const noexcept;

			/**
			 * @brief Writes the recorded cycles to a JSON file and releases them.
			 * @param filepath A reference to the file path.
			 * @param error A reference to a string receiving the reason of a failure.
			 * @return bool False when nothing is recorded or the file cannot be written.
			 */
			[[nodiscard]]
			bool write (const std::filesystem::path & filepath, std::string & error) noexcept;

		private:

			/** @brief The state of a recording. */
			enum class State : uint8_t
			{
				Idle,
				Waiting,
				Recording,
				Complete
			};

			/** @brief One node on one cycle. */
			struct Sample final
			{
				size_t cycle{0};
				Base::Math::Vector< 3, float > position;
				Base::Math::Vector< 3, float > upward;
				Base::Math::Vector< 3, float > backward;
				Base::Math::Vector< 3, float > linearVelocity;
				Base::Math::Vector< 3, float > angularVelocity;
				uint32_t node{0};
				Physics::GroundedSource groundedSource{Physics::GroundedSource::None};
				bool found{false};
				bool simulationPaused{false};
				bool grounded{false};
			};

			/**
			 * @brief Returns the name of a state.
			 * @param state The state.
			 * @return const char *
			 */
			[[nodiscard]]
			static const char * stateName (State state) noexcept;

			mutable std::mutex m_access;
			/** Set while a recording is waiting or running: an idle sample() returns without taking the lock. */
			std::atomic< bool > m_armed{false};
			std::vector< std::string > m_nodeNames;
			std::vector< std::weak_ptr< Node > > m_nodes;
			std::vector< Sample > m_samples;
			size_t m_firstCycle{0};
			size_t m_cycleCount{0};
			size_t m_recordedCycles{0};
			State m_state{State::Idle};
	};
}
