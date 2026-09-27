/*
 * src/Scenes/Component/AnimationConsoleAdapters.cpp
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

#include "ConsoleAdapter.hpp"

/* STL inclusions. */
#include <cmath>
#include <optional>

/* Third-party inclusions. */
#include "json/json.h"

/* Local inclusions. */
#include "Constants.hpp"
#include "FastJSON.hpp"
#include "Scenes/AbstractEntity.hpp"
#include "NodeAnimation.hpp"
#include "ParticlesEmitter.hpp"

namespace EmEn::Scenes::Component
{
	using namespace Base;

	namespace
	{
		/**
		 * @brief Returns the console name of a playback wrap mode.
		 * @param wrap The wrap mode.
		 * @return const char *
		 */
		[[nodiscard]]
		const char *
		wrapName (Animations::PlaybackWrap wrap) noexcept
		{
			switch ( wrap )
			{
				case Animations::PlaybackWrap::Once :
					return "Once";

				case Animations::PlaybackWrap::Loop :
					return "Loop";

				case Animations::PlaybackWrap::PingPong :
					return "PingPong";
			}

			return "Loop";
		}

		/**
		 * @brief Parses the console name of a playback wrap mode.
		 * @param name The name ("Once", "Loop" or "PingPong").
		 * @return std::optional< Animations::PlaybackWrap >
		 */
		[[nodiscard]]
		std::optional< Animations::PlaybackWrap >
		parseWrap (const std::string & name) noexcept
		{
			if ( name == "Once" )
			{
				return Animations::PlaybackWrap::Once;
			}

			if ( name == "Loop" )
			{
				return Animations::PlaybackWrap::Loop;
			}

			if ( name == "PingPong" )
			{
				return Animations::PlaybackWrap::PingPong;
			}

			return std::nullopt;
		}

		/**
		 * @brief Returns the full state of a node animation as JSON.
		 * @param animation The node animation.
		 * @return std::string
		 */
		[[nodiscard]]
		std::string
		stateOf (const NodeAnimation & animation) noexcept
		{
			Json::Value state{Json::objectValue};
			state["name"] = animation.name();
			state["entity"] = animation.parentEntity().name();
			state["type"] = NodeAnimation::ClassId;
			state["playing"] = animation.isPlaying();
			state["activeClip"] = animation.activeClipName();
			state["wrap"] = wrapName(animation.wrap());
			state["speed"] = static_cast< double >(animation.speed());

			Json::Value clips{Json::arrayValue};

			for ( const auto & clipName : animation.clipNames() )
			{
				clips.append(clipName);
			}

			state["clips"] = std::move(clips);

			return FastJSON::stringify(state);
		}

		/**
		 * @brief Returns the full state of a particles emitter as JSON.
		 * @param emitter The particles emitter.
		 * @return std::string
		 */
		[[nodiscard]]
		std::string
		stateOf (const ParticlesEmitter & emitter) noexcept
		{
			const auto & physics = emitter.particlePhysicalProperties();

			Json::Value state{Json::objectValue};
			state["name"] = emitter.name();
			state["entity"] = emitter.parentEntity().name();
			state["type"] = ParticlesEmitter::ClassId;
			state["emitting"] = emitter.isEmitting();
			state["logicCyclesPerSecond"] = WorldPhysicsUpdateFrequency< int >;
			state["particleLimit"] = static_cast< Json::UInt64 >(emitter.particleLimit());
			state["particlesPerCycle"] = emitter.particleGeneratedPerCycle();
			state["minimumLifetimeCycles"] = emitter.minimumParticleLifetime();
			state["maximumLifetimeCycles"] = emitter.maximumParticleLifetime();
			state["minimumSize"] = static_cast< double >(emitter.minimumParticlesSize());
			state["maximumSize"] = static_cast< double >(emitter.maximumParticlesSize());
			state["sizeDeltaPerCycle"] = static_cast< double >(emitter.particleSizeUpdateFactor());
			state["spreadingRadius"] = static_cast< double >(emitter.spreadingRadius());
			state["chaos"] = static_cast< double >(emitter.chaos());

			Json::Value particle{Json::objectValue};
			particle["mass"] = static_cast< double >(physics.mass());
			particle["surface"] = static_cast< double >(physics.surface());
			particle["dragCoefficient"] = static_cast< double >(physics.dragCoefficient());
			particle["angularDragCoefficient"] = static_cast< double >(physics.angularDragCoefficient());
			particle["bounciness"] = static_cast< double >(physics.bounciness());
			particle["stickiness"] = static_cast< double >(physics.stickiness());
			state["particlePhysics"] = std::move(particle);

			return FastJSON::stringify(state);
		}

		/**
		 * @brief The answer of a setter: the confirmation, then the component's NEW state as JSON.
		 * @tparam component_t The component type.
		 * @param component The component, after the change.
		 * @param message The confirmation.
		 * @return Console::CommandResult
		 */
		template< typename component_t >
		[[nodiscard]]
		Console::CommandResult
		changed (const component_t & component, std::string message) noexcept
		{
			return changedState(std::move(message), stateOf(component));
		}

		/** @brief `Core.SceneManagerService.NodeAnimation.*`. */
		class NodeAnimationConsoleAdapter final : public ConsoleAdapter< NodeAnimation >
		{
			public:

				explicit
				NodeAnimationConsoleAdapter (const Manager & sceneManager) noexcept
					: ConsoleAdapter{sceneManager}
				{

				}

			private:

				void
				onRegisterToConsole () noexcept override
				{
					const Console::Parameter entity{entityParameter("the node animation")};
					const Console::Parameter component{componentParameter(NodeAnimation::ClassId)};

					this->bindCommand("getState", "Returns the state of a node animation as JSON: its clips, the active one, whether it plays, its wrap mode and speed.",
						{entity, component},
						[this] (const std::string & entityName, const std::string & componentName) {
							return this->act(entityName, componentName, [] (NodeAnimation & animation) {
								return Console::CommandResult::json(stateOf(animation));
							});
						}, Console::CommandHint::ReadOnly);

					this->bindCommand("play", "Plays a clip of a node animation from its start; its first frame is applied at once.",
						{
							entity,
							component,
							{"clip", "The clip name (getState lists them)."},
							{"wrap", "Once (stops on the last frame), Loop or PingPong.", "Loop"}
						},
						[this] (const std::string & entityName, const std::string & componentName, const std::string & clip, const std::string & wrap) {
							const auto parsedWrap = parseWrap(wrap);

							if ( !parsedWrap.has_value() )
							{
								return Console::CommandResult::error("Unknown wrap mode '" + wrap + "': Once, Loop or PingPong.");
							}

							return this->act(entityName, componentName, [&clip, &parsedWrap] (NodeAnimation & animation) {
								if ( !animation.play(clip, *parsedWrap) )
								{
									return Console::CommandResult::error("Node animation '" + animation.name() + "' has no clip named '" + clip + "' (getState lists them).");
								}

								return changed(animation, "Node animation '" + animation.name() + "' plays '" + clip + "'.");
							});
						}, Console::CommandHint::Idempotent);

					this->bindCommand("stop", "Stops a node animation and puts every node it drives back on its rest frame.",
						{entity, component},
						[this] (const std::string & entityName, const std::string & componentName) {
							return this->act(entityName, componentName, [] (NodeAnimation & animation) {
								animation.stop();

								return changed(animation, "Node animation '" + animation.name() + "' stopped (rest frame restored).");
							});
						}, Console::CommandHint::Idempotent);

					this->bindCommand("setSpeed", "Sets the playback speed of a node animation.",
						{
							entity,
							component,
							{"speed", "The speed multiplier, 0 or more (1 = normal, 0.5 = half, 0 = frozen on the current frame)."}
						},
						[this] (const std::string & entityName, const std::string & componentName, float speed) {
							/* NOTE: a negative speed would run the clip time below 0, which the wrap modes do not handle. */
							if ( !std::isfinite(speed) || speed < 0.0F )
							{
								return Console::CommandResult::error("The speed must be a finite number, 0 or more.");
							}

							return this->act(entityName, componentName, [speed] (NodeAnimation & animation) {
								animation.setSpeed(speed);

								return changed(animation, "Node animation '" + animation.name() + "' speed set to " + std::to_string(speed) + ".");
							});
						}, Console::CommandHint::Idempotent);
				}
		};

		/** @brief `Core.SceneManagerService.ParticlesEmitter.*`. */
		class ParticlesEmitterConsoleAdapter final : public ConsoleAdapter< ParticlesEmitter >
		{
			public:

				explicit
				ParticlesEmitterConsoleAdapter (const Manager & sceneManager) noexcept
					: ConsoleAdapter{sceneManager}
				{

				}

			private:

				void
				onRegisterToConsole () noexcept override
				{
					const Console::Parameter entity{entityParameter("the particles emitter")};
					const Console::Parameter component{componentParameter(ParticlesEmitter::ClassId)};

					this->bindCommand("getState", "Returns the state of a particles emitter as JSON: emitting, particle limit, particles per logic cycle, lifetime (logic cycles), size range, size delta per cycle, spreading radius, chaos and the physical properties of one particle.",
						{entity, component},
						[this] (const std::string & entityName, const std::string & componentName) {
							return this->act(entityName, componentName, [] (ParticlesEmitter & emitter) {
								return Console::CommandResult::json(stateOf(emitter));
							});
						}, Console::CommandHint::ReadOnly);

					/* NOTE: ParticlesEmitter::start(duration) is not exposed with its timeout: the timer is armed in
					 * microseconds where the declaration documents milliseconds (engine item particles-emitter-timeout-unit). */
					this->bindCommand("start", "Starts the emission of a particles emitter.",
						{entity, component},
						[this] (const std::string & entityName, const std::string & componentName) {
							return this->act(entityName, componentName, [] (ParticlesEmitter & emitter) {
								emitter.start();

								return changed(emitter, "Particles emitter '" + emitter.name() + "' emitting.");
							});
						}, Console::CommandHint::Idempotent);

					this->bindCommand("stop", "Stops the emission of a particles emitter; the particles already emitted live out their lifetime.",
						{entity, component},
						[this] (const std::string & entityName, const std::string & componentName) {
							return this->act(entityName, componentName, [] (ParticlesEmitter & emitter) {
								emitter.stop();

								return changed(emitter, "Particles emitter '" + emitter.name() + "' stopped.");
							});
						}, Console::CommandHint::Idempotent);

					this->bindCommand("setSpawnRate", "Sets how many particles a particles emitter creates per logic cycle (60 per second), while below its particle limit.",
						{
							entity,
							component,
							{"particlesPerCycle", "Particles per logic cycle, 0 up to the emitter's particle limit (getState)."}
						},
						[this] (const std::string & entityName, const std::string & componentName, int32_t particlesPerCycle) {
							if ( particlesPerCycle < 0 )
							{
								return Console::CommandResult::error("The particle count per cycle must be 0 or more.");
							}

							return this->act(entityName, componentName, [particlesPerCycle] (ParticlesEmitter & emitter) {
								/* ⚠️ The setter CLAMPS to the limit: refuse instead of applying a different value. */
								if ( static_cast< size_t >(particlesPerCycle) > emitter.particleLimit() )
								{
									return Console::CommandResult::error("Particles emitter '" + emitter.name() + "' holds at most " + std::to_string(emitter.particleLimit()) + " particles: the limit is set at creation.");
								}

								emitter.setParticleGeneratedPerCycle(static_cast< uint32_t >(particlesPerCycle));

								return changed(emitter, "Particles emitter '" + emitter.name() + "' emits " + std::to_string(particlesPerCycle) + " particles per cycle.");
							});
						}, Console::CommandHint::Idempotent);

					this->bindCommand("setLifetime", "Sets the lifetime of the NEW particles of an emitter, in logic cycles (60 per second): fixed, or drawn in a range.",
						{
							entity,
							component,
							{"minimum", "The (minimum) lifetime, in logic cycles, 1 or more."},
							{"maximum", "The maximum lifetime, in logic cycles, the minimum or more; omit it for a fixed lifetime."}
						},
						[this] (const std::string & entityName, const std::string & componentName, int32_t minimum, std::optional< int32_t > maximum) {
							if ( minimum < 1 || ( maximum.has_value() && *maximum < minimum ) )
							{
								return Console::CommandResult::error("The lifetime must satisfy 1 <= minimum <= maximum (logic cycles).");
							}

							return this->act(entityName, componentName, [minimum, maximum] (ParticlesEmitter & emitter) {
								if ( maximum.has_value() && *maximum > minimum )
								{
									emitter.setParticleLifetime(static_cast< uint32_t >(minimum), static_cast< uint32_t >(*maximum));
								}
								else
								{
									emitter.setParticleLifetime(static_cast< uint32_t >(minimum));
								}

								return changed(emitter, "Particles emitter '" + emitter.name() + "' lifetime set.");
							});
						}, Console::CommandHint::Idempotent);

					this->bindCommand("setSize", "Sets the size of the NEW particles of an emitter: fixed, or drawn in a range.",
						{
							entity,
							component,
							{"minimum", "The (minimum) size, more than 0."},
							{"maximum", "The maximum size, the minimum or more; omit it for a fixed size."}
						},
						[this] (const std::string & entityName, const std::string & componentName, float minimum, std::optional< float > maximum) {
							if ( !std::isfinite(minimum) || minimum <= 0.0F || ( maximum.has_value() && ( !std::isfinite(*maximum) || *maximum < minimum ) ) )
							{
								return Console::CommandResult::error("The size must satisfy 0 < minimum <= maximum.");
							}

							return this->act(entityName, componentName, [minimum, maximum] (ParticlesEmitter & emitter) {
								if ( maximum.has_value() )
								{
									emitter.setParticleSize(minimum, *maximum);
								}
								else
								{
									emitter.setParticleSize(minimum);
								}

								return changed(emitter, "Particles emitter '" + emitter.name() + "' size set.");
							});
						}, Console::CommandHint::Idempotent);

					this->bindCommand("setSizeDelta", "Sets how much a particle of an emitter grows (or shrinks, negative) every logic cycle of its life.",
						{
							entity,
							component,
							{"delta", "The size change per logic cycle, positive or negative."}
						},
						[this] (const std::string & entityName, const std::string & componentName, float delta) {
							if ( !std::isfinite(delta) )
							{
								return Console::CommandResult::error("The size delta must be a finite number.");
							}

							return this->act(entityName, componentName, [delta] (ParticlesEmitter & emitter) {
								emitter.setParticleSizeDeltaPerCycle(delta);

								return changed(emitter, "Particles emitter '" + emitter.name() + "' size delta set to " + std::to_string(delta) + " per cycle.");
							});
						}, Console::CommandHint::Idempotent);

					this->bindCommand("setSpreadingRadius", "Sets the radius, around the emitter, inside which new particles appear.",
						{
							entity,
							component,
							{"radius", "The spreading radius, in metres, 0 or more (0 = every particle starts at the emitter)."}
						},
						[this] (const std::string & entityName, const std::string & componentName, float radius) {
							if ( !std::isfinite(radius) || radius < 0.0F )
							{
								return Console::CommandResult::error("The spreading radius must be a finite number of metres, 0 or more.");
							}

							return this->act(entityName, componentName, [radius] (ParticlesEmitter & emitter) {
								emitter.setSpreadingRadius(radius);

								return changed(emitter, "Particles emitter '" + emitter.name() + "' spreading radius set to " + std::to_string(radius) + " m.");
							});
						}, Console::CommandHint::Idempotent);

					this->bindCommand("setChaos", "Sets the chaos of a particles emitter: the magnitude of the random disturbance applied to its particles.",
						{
							entity,
							component,
							{"magnitude", "The chaos magnitude, 0 or more (0 = none)."}
						},
						[this] (const std::string & entityName, const std::string & componentName, float magnitude) {
							if ( !std::isfinite(magnitude) || magnitude < 0.0F )
							{
								return Console::CommandResult::error("The chaos magnitude must be a finite number, 0 or more.");
							}

							return this->act(entityName, componentName, [magnitude] (ParticlesEmitter & emitter) {
								emitter.setChaos(magnitude);

								return changed(emitter, "Particles emitter '" + emitter.name() + "' chaos set to " + std::to_string(magnitude) + ".");
							});
						}, Console::CommandHint::Idempotent);
				}
		};
	}

	void
	appendAnimationConsoleAdapters (const Manager & sceneManager, std::vector< std::unique_ptr< Console::ControllableTrait > > & adapters) noexcept
	{
		adapters.emplace_back(std::make_unique< NodeAnimationConsoleAdapter >(sceneManager));
		adapters.emplace_back(std::make_unique< ParticlesEmitterConsoleAdapter >(sceneManager));
	}
}
