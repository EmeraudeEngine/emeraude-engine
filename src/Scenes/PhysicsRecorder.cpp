/*
 * src/Scenes/PhysicsRecorder.cpp
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

#include "PhysicsRecorder.hpp"

/* STL inclusions. */
#include <cmath>
#include <iomanip>
#include <sstream>
#include <utility>

/* Local inclusions. */
#include "IO/IO.hpp"
#include "json/json.h"
#include "Node.hpp"
#include "Scene.hpp"

namespace EmEn::Scenes
{
	using namespace Base;
	using namespace Base::Math;

	namespace
	{
		/**
		 * @brief Writes a vector as a JSON array (a non-finite component as null, the others with 9 digits).
		 * @param output A reference to the stream.
		 * @param vector A reference to the vector.
		 */
		void
		writeVector (std::ostream & output, const Vector< 3, float > & vector) noexcept
		{
			output << '[';

			for ( size_t index = 0; index < 3; ++index )
			{
				if ( index > 0 )
				{
					output << ',';
				}

				if ( std::isfinite(vector[index]) )
				{
					output << std::setprecision(9) << vector[index];
				}
				else
				{
					output << "null";
				}
			}

			output << ']';
		}

		/**
		 * @brief Returns the name of a grounded source, as getNodePhysics() writes it.
		 * @param source The source.
		 * @return const char *
		 */
		[[nodiscard]]
		const char *
		groundedSourceName (Physics::GroundedSource source) noexcept
		{
			switch ( source )
			{
				case Physics::GroundedSource::Ground :
					return "Ground";

				case Physics::GroundedSource::Boundary :
					return "Boundary";

				case Physics::GroundedSource::Entity :
					return "Entity";

				case Physics::GroundedSource::None :
					break;
			}

			return "None";
		}
	}

	const char *
	PhysicsRecorder::stateName (State state) noexcept
	{
		switch ( state )
		{
			case State::Idle :
				return "Idle";

			case State::Waiting :
				return "Waiting";

			case State::Recording :
				return "Recording";

			case State::Complete :
				return "Complete";
		}

		return "Idle";
	}

	bool
	PhysicsRecorder::start (std::vector< std::string > nodeNames, size_t firstCycle, size_t cycleCount, std::string & error) noexcept
	{
		if ( nodeNames.empty() || nodeNames.size() > MaxNodes )
		{
			error = "Give 1 to " + std::to_string(MaxNodes) + " node names (got " + std::to_string(nodeNames.size()) + ").";

			return false;
		}

		if ( cycleCount == 0 || cycleCount > MaxCycles )
		{
			error = "Give a cycle count from 1 to " + std::to_string(MaxCycles) + " (got " + std::to_string(cycleCount) + ").";

			return false;
		}

		for ( const auto & name : nodeNames )
		{
			if ( name.empty() )
			{
				error = "A node name is empty.";

				return false;
			}
		}

		const std::scoped_lock lock{m_access};

		m_nodeNames = std::move(nodeNames);
		m_nodes.clear();
		m_samples.clear();
		/* Every sample is reserved here: sample() never allocates on the logic thread. */
		m_samples.reserve(m_nodeNames.size() * cycleCount);
		m_firstCycle = firstCycle;
		m_cycleCount = cycleCount;
		m_recordedCycles = 0;
		m_state = State::Waiting;
		m_armed.store(true, std::memory_order_release);

		return true;
	}

	void
	PhysicsRecorder::stop () noexcept
	{
		const std::scoped_lock lock{m_access};

		if ( m_state == State::Waiting || m_state == State::Recording )
		{
			m_state = m_recordedCycles > 0 ? State::Complete : State::Idle;
		}

		m_armed.store(false, std::memory_order_release);
	}

	void
	PhysicsRecorder::sample (const Scene & scene) noexcept
	{
		if ( !m_armed.load(std::memory_order_acquire) )
		{
			return;
		}

		const std::scoped_lock lock{m_access};

		if ( m_state != State::Waiting && m_state != State::Recording )
		{
			return;
		}

		const size_t cycle = scene.cycle();

		if ( m_state == State::Waiting )
		{
			if ( cycle < m_firstCycle )
			{
				return;
			}

			/* The names are resolved once, when the recording begins: a weak pointer per name (expired = missing). */
			m_nodes.clear();
			m_nodes.reserve(m_nodeNames.size());

			for ( const auto & name : m_nodeNames )
			{
				m_nodes.emplace_back(scene.root()->findChild(name));
			}

			m_firstCycle = cycle;
			m_state = State::Recording;
		}

		for ( size_t index = 0; index < m_nodes.size(); ++index )
		{
			Sample sample;
			sample.cycle = cycle;
			sample.node = static_cast< uint32_t >(index);

			if ( const auto node = m_nodes[index].lock() )
			{
				const auto coordinates = node->getWorldCoordinates();

				sample.position = coordinates.position();
				sample.upward = coordinates.upwardVector();
				sample.backward = coordinates.backwardVector();
				sample.linearVelocity = node->linearVelocity();
				sample.angularVelocity = node->angularVelocity();
				sample.groundedSource = node->groundedSource();
				sample.found = true;
				sample.simulationPaused = node->isSimulationPaused();
				sample.grounded = node->isGrounded();
			}

			/* Within the capacity reserved by start(): nodes × cycles, and the range closes below. */
			m_samples.push_back(sample);
		}

		++m_recordedCycles;

		if ( m_recordedCycles >= m_cycleCount )
		{
			m_state = State::Complete;
			m_armed.store(false, std::memory_order_release);
		}
	}

	std::string
	PhysicsRecorder::status () const noexcept
	{
		const std::scoped_lock lock{m_access};

		std::stringstream output;

		output << R"({"state":")" << stateName(m_state) << R"(","firstCycle":)" << m_firstCycle << R"(,"cycleCount":)" << m_cycleCount << R"(,"recordedCycles":)" << m_recordedCycles << R"(,"nodes":)" << m_nodeNames.size() << '}';

		return output.str();
	}

	bool
	PhysicsRecorder::write (const std::filesystem::path & filepath, std::string & error) noexcept
	{
		std::stringstream output;

		{
			const std::scoped_lock lock{m_access};

			if ( m_state != State::Complete || m_samples.empty() )
			{
				error = std::string{"Nothing to write: the recorder is "} + stateName(m_state) + " with " + std::to_string(m_recordedCycles) + " recorded cycle(s).";

				return false;
			}

			output << R"({"firstCycle":)" << m_firstCycle << R"(,"cycleCount":)" << m_recordedCycles << R"(,"nodes":{)";

			for ( size_t nodeIndex = 0; nodeIndex < m_nodeNames.size(); ++nodeIndex )
			{
				output << (nodeIndex > 0 ? "," : "") << Json::valueToQuotedString(m_nodeNames[nodeIndex].c_str(), m_nodeNames[nodeIndex].size()) << ":[";

				bool first = true;

				for ( const auto & sample : m_samples )
				{
					if ( sample.node != nodeIndex )
					{
						continue;
					}

					output << (first ? "" : ",") << R"({"sceneCycle":)" << sample.cycle << R"(,"found":)" << (sample.found ? "true" : "false");

					if ( sample.found )
					{
						output << R"(,"worldPosition":)";
						writeVector(output, sample.position);
						output << R"(,"worldUpward":)";
						writeVector(output, sample.upward);
						output << R"(,"worldBackward":)";
						writeVector(output, sample.backward);
						output << R"(,"linearVelocity":)";
						writeVector(output, sample.linearVelocity);
						output << R"(,"angularVelocity":)";
						writeVector(output, sample.angularVelocity);
						output << R"(,"simulationPaused":)" << (sample.simulationPaused ? "true" : "false");
						output << R"(,"grounded":)" << (sample.grounded ? "true" : "false");
						output << R"(,"groundedSource":")" << groundedSourceName(sample.groundedSource) << '"';
					}

					output << '}';

					first = false;
				}

				output << ']';
			}

			output << "}}\n";

			/* Released: a recording is written once. */
			m_samples.clear();
			m_samples.shrink_to_fit();
			m_state = State::Idle;
		}

		/* The file is written outside the lock: the logic thread never waits on the disk. */
		if ( !IO::filePutContents(filepath, output.str(), false, true) )
		{
			error = "Unable to write '" + filepath.string() + "'.";

			return false;
		}

		return true;
	}
}
