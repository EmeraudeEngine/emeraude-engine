/*
 * src/Scenes/Component/AudioConsoleAdapters.cpp
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

/* Third-party inclusions. */
#include "json/json.h"

/* Local inclusions. */
#include "FastJSON.hpp"
#include "Scenes/AbstractEntity.hpp"
#include "SoundEmitter.hpp"

namespace EmEn::Scenes::Component
{
	using namespace Base;

	namespace
	{
		/**
		 * @brief Returns the full state of a sound emitter as JSON.
		 * @param emitter The sound emitter.
		 * @return std::string
		 */
		[[nodiscard]]
		std::string
		stateOf (const SoundEmitter & emitter) noexcept
		{
			Json::Value state{Json::objectValue};
			state["name"] = emitter.name();
			state["entity"] = emitter.parentEntity().name();
			state["type"] = SoundEmitter::ClassId;
			state["playing"] = emitter.isPlaying();
			state["hasSource"] = emitter.hasSource();
			state["gain"] = static_cast< double >(emitter.gain());
			state["velocityDistortion"] = emitter.velocityDistortionEnabled();

			if ( const auto & sound = emitter.attachedSound(); sound != nullptr )
			{
				state["sound"] = sound->name();
				state["loop"] = emitter.isLooping();
			}
			else
			{
				state["sound"] = Json::Value{Json::nullValue};
			}

			return FastJSON::stringify(state);
		}

		/**
		 * @brief The answer of a setter: the confirmation, then the emitter's NEW state as JSON.
		 * @param emitter The sound emitter, after the change.
		 * @param message The confirmation.
		 * @return Console::CommandResult
		 */
		[[nodiscard]]
		Console::CommandResult
		changed (const SoundEmitter & emitter, std::string message) noexcept
		{
			return changedState(std::move(message), stateOf(emitter));
		}

		/** @brief `Core.SceneManagerService.SoundEmitter.*` (drives the attached sound; loading one is not a console matter yet). */
		class SoundEmitterConsoleAdapter final : public ConsoleAdapter< SoundEmitter >
		{
			public:

				explicit
				SoundEmitterConsoleAdapter (const Manager & sceneManager) noexcept
					: ConsoleAdapter{sceneManager}
				{

				}

			private:

				/**
				 * @brief Binds a transport command acting on the emitter's audio source, refused when it has none.
				 * @param name The command name.
				 * @param description The command description.
				 * @param action The transport action.
				 * @param done The confirmation suffix.
				 * @return void
				 */
				void
				bindTransport (const std::string & name, const std::string & description, void (SoundEmitter::* action)() const noexcept, const std::string & done) noexcept
				{
					this->bindCommand(name, description,
						{entityParameter("the sound emitter"), componentParameter(SoundEmitter::ClassId)},
						[this, action, done] (const std::string & entityName, const std::string & componentName) {
							return this->act(entityName, componentName, [action, &done] (SoundEmitter & emitter) {
								/* NOTE: without a source, SoundEmitter's transport silently does nothing. */
								if ( !emitter.hasSource() )
								{
									return Console::CommandResult::error("Sound emitter '" + emitter.name() + "' has no audio source: nothing is playing or paused on it (replay starts its attached sound).");
								}

								(emitter.*action)();

								return changed(emitter, "Sound emitter '" + emitter.name() + "' " + done + ".");
							});
						}, Console::CommandHint::Idempotent);
				}

				void
				onRegisterToConsole () noexcept override
				{
					const Console::Parameter entity{entityParameter("the sound emitter")};
					const Console::Parameter component{componentParameter(SoundEmitter::ClassId)};

					this->bindCommand("getState", "Returns the state of a sound emitter as JSON: whether it plays, its gain, its attached sound (resource name, loop) and velocity distortion (Doppler).",
						{entity, component},
						[this] (const std::string & entityName, const std::string & componentName) {
							return this->act(entityName, componentName, [] (SoundEmitter & emitter) {
								return Console::CommandResult::json(stateOf(emitter));
							});
						}, Console::CommandHint::ReadOnly);

					this->bindCommand("setGain", "Sets the gain of a sound emitter (applied at once to a playing sound).",
						{
							entity,
							component,
							{"gain", "The gain, 0 or more (1 = the sound as recorded)."}
						},
						[this] (const std::string & entityName, const std::string & componentName, float gain) {
							if ( !std::isfinite(gain) || gain < 0.0F )
							{
								return Console::CommandResult::error("The gain must be a finite number, 0 or more.");
							}

							return this->act(entityName, componentName, [gain] (SoundEmitter & emitter) {
								emitter.setGain(gain);

								return changed(emitter, "Sound emitter '" + emitter.name() + "' gain set to " + std::to_string(gain) + ".");
							});
						}, Console::CommandHint::Idempotent);

					this->bindCommand("setVelocityDistortion", "Switches the Doppler shift of a sound emitter (the pitch following the entity's velocity) on or off.",
						{
							entity,
							component,
							{"enabled", "1 (true) shifts the pitch with the velocity, 0 (false) keeps it."}
						},
						[this] (const std::string & entityName, const std::string & componentName, bool enabled) {
							return this->act(entityName, componentName, [enabled] (SoundEmitter & emitter) {
								emitter.enableVelocityDistortion(enabled);

								return changed(emitter, std::string{"Sound emitter '"} + emitter.name() + ( enabled ? "' Doppler on." : "' Doppler off." ));
							});
						}, Console::CommandHint::Idempotent);

					this->bindCommand("replay", "Plays the attached sound of a sound emitter from its start (rewinds it if it is playing).",
						{entity, component},
						[this] (const std::string & entityName, const std::string & componentName) {
							return this->act(entityName, componentName, [] (SoundEmitter & emitter) {
								if ( emitter.attachedSound() == nullptr )
								{
									return Console::CommandResult::error("Sound emitter '" + emitter.name() + "' has no attached sound to play.");
								}

								emitter.replay();

								return changed(emitter, "Sound emitter '" + emitter.name() + "' replays '" + emitter.attachedSound()->name() + "'.");
							});
						});

					this->bindCommand("stop", "Stops a sound emitter (the attached sound stays attached: replay plays it again).",
						{entity, component},
						[this] (const std::string & entityName, const std::string & componentName) {
							return this->act(entityName, componentName, [] (SoundEmitter & emitter) {
								emitter.stop();

								return changed(emitter, "Sound emitter '" + emitter.name() + "' stopped.");
							});
						}, Console::CommandHint::Idempotent);

					this->bindTransport("pause", "Pauses the sound playing on a sound emitter.", &SoundEmitter::pause, "paused");
					this->bindTransport("resume", "Resumes the paused sound of a sound emitter.", &SoundEmitter::resume, "resumed");
					this->bindTransport("rewind", "Rewinds the sound of a sound emitter to its start.", &SoundEmitter::rewind, "rewound");
				}
		};
	}

	void
	appendAudioConsoleAdapters (const Manager & sceneManager, std::vector< std::unique_ptr< Console::ControllableTrait > > & adapters) noexcept
	{
		adapters.emplace_back(std::make_unique< SoundEmitterConsoleAdapter >(sceneManager));
	}
}
