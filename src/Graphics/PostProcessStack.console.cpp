/*
 * src/Graphics/PostProcessStack.console.cpp
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

#include "PostProcessStack.hpp"

/* STL inclusions. */
#include <algorithm>
#include <optional>
#include <sstream>
#include <string>

/* Local inclusions. */
#include "IndirectPostProcessEffect.hpp"
#include "Tracer.hpp"

namespace EmEn::Graphics
{
	using namespace Base;

	namespace
	{
		/**
		 * @brief Resolves a slot from its console name, case-sensitively.
		 * @note The names are `EffectSlot`'s own (to_cstring()), so the console vocabulary and
		 * the trace vocabulary can never drift apart.
		 * @param name The slot name.
		 * @return std::optional< EffectSlot >
		 */
		[[nodiscard]]
		std::optional< EffectSlot >
		parseSlot (const std::string & name) noexcept
		{
			for ( size_t index = 0; index < EffectSlotCount; ++index )
			{
				const auto slot = static_cast< EffectSlot >(index);

				if ( isChainSlot(slot) && name == to_cstring(slot) )
				{
					return slot;
				}
			}

			return std::nullopt;
		}

		/**
		 * @brief Returns the lane an effect belongs to, as a word.
		 * @param effect A reference to the effect.
		 * @return const char *
		 */
		[[nodiscard]]
		const char *
		laneName (const IndirectPostProcessEffect & effect) noexcept
		{
			return to_cstring(effect.requiresRayTracing() ? LightingLane::RayTracing : LightingLane::ScreenSpace);
		}
	}

	void
	PostProcessStack::onRegisterToConsole () noexcept
	{
		this->bindCommand("listEffects", [this] (const Console::Arguments & /*arguments*/, Console::Outputs & outputs) {
			auto any = false;

			for ( size_t index = 0; index < EffectSlotCount; ++index )
			{
				const auto slot = static_cast< EffectSlot >(index);

				if ( !isChainSlot(slot) || m_slots[index].empty() )
				{
					continue;
				}

				any = true;

				const auto selected = this->selectedOccupant(slot);
				const auto effective = this->effectiveOccupant(slot);

				/* ⚠️ The four photographic slots are NOT selectable and syncSlotSelection() skips
				 * them: their occupant is materialized by the camera, so both indices above stay
				 * empty for them. Reading the marks from those indices made the console report
				 * "ToneMapping: off" on a frame that was visibly tone mapped — a report that
				 * contradicts the image is worse than no report. Their state comes from the
				 * effect itself, and the slot says who owns it. */
				const auto cameraOwned = isCameraEffectSlot(slot);

				outputs.emplace_back(Severity::Info, std::stringstream{} << "[" << to_cstring(slot) << "]" << ( cameraOwned ? "  (camera-owned, not selectable)" : "" ));

				for ( const auto & occupant : m_slots[index] )
				{
					if ( occupant == nullptr )
					{
						continue;
					}

					/* '>' is what the owner asked for, '*' is what the last frame actually ran.
					 * They differ exactly when a fallback is active — the whole reason this
					 * listing prints two marks instead of one. */
					const auto * mark = "  ";

					if ( cameraOwned )
					{
						mark = occupant->isEnabled() && occupant->isCreated() ? " *" : "  ";
					}
					else if ( occupant == selected && occupant == effective )
					{
						mark = ">*";
					}
					else if ( occupant == selected )
					{
						mark = "> ";
					}
					else if ( occupant == effective )
					{
						mark = " *";
					}

					outputs.emplace_back(Severity::Info, std::stringstream{} <<
						"  " << mark << " " << occupant->label() <<
						" (" << laneName(*occupant) << ", " <<
						( occupant->isCreated() ? "created" : ( occupant->hasCreationFailed() ? "CREATION FAILED" : "not created" ) ) << ")"
					);
				}
			}

			if ( !any )
			{
				outputs.emplace_back(Severity::Warning, "The post-process stack holds no chain effect.");

				return false;
			}

			outputs.emplace_back(Severity::Info, "Legend: '>' selected by you, '*' running in the last frame.");

			return true;
		}, "List every slot of the chain, its occupants, which one is selected and which one actually runs.");

		this->bindCommand("getStatus", [this] (const Console::Arguments & /*arguments*/, Console::Outputs & outputs) {
			/* ⚠️ The lane header first, because "why is it in screen space?" is the question this
			 * command is actually asked. A ray-traced lane that is not RESIDENT was never filed
			 * into any slot, so the per-slot lines below cannot show it at all — the absence
			 * looks like a choice instead of an unavailability. */
			auto rayTracingLaneResident = false;

			for ( size_t index = 0; index < EffectSlotCount; ++index )
			{
				if ( !isLightingSlot(static_cast< EffectSlot >(index)) )
				{
					continue;
				}

				for ( const auto & occupant : m_slots[index] )
				{
					if ( occupant != nullptr && occupant->requiresRayTracing() )
					{
						rayTracingLaneResident = true;
					}
				}
			}

			if ( rayTracingLaneResident )
			{
				outputs.emplace_back(Severity::Info, "Lanes resident: ScreenSpace + RayTracing.");
			}
			else
			{
				outputs.emplace_back(Severity::Warning,
					"Lanes resident: ScreenSpace ONLY — the ray-traced lane is unavailable FOR THIS WHOLE SESSION: "
					"either this device cannot ray trace, or 'Core/Graphics/PostProcessing/LightingLane' was set to "
					"'ScreenSpace'/'None' at launch, in which case no acceleration structure was ever built and the "
					"geometries already loaded cannot gain one. setLightingMode(\"RayTracing\") cannot help; set the "
					"key to 'Auto' and relaunch.");
			}

			for ( size_t index = 0; index < EffectSlotCount; ++index )
			{
				const auto slot = static_cast< EffectSlot >(index);

				if ( !isChainSlot(slot) || m_slots[index].empty() )
				{
					continue;
				}

				/* Camera-owned slots carry no selection: report what the camera materialized. */
				if ( isCameraEffectSlot(slot) )
				{
					const auto & occupants = m_slots[index];

					const auto running = std::ranges::find_if(occupants, [] (const auto & occupant) {
						return occupant != nullptr && occupant->isEnabled() && occupant->isCreated();
					});

					outputs.emplace_back(Severity::Info, std::stringstream{} <<
						to_cstring(slot) << ": " << ( running != occupants.end() ? (*running)->label() : "off" ) << " (camera-owned)"
					);

					continue;
				}

				const auto selected = this->selectedOccupant(slot);
				const auto effective = this->effectiveOccupant(slot);

				if ( selected == effective )
				{
					outputs.emplace_back(Severity::Info, std::stringstream{} <<
						to_cstring(slot) << ": " << ( effective != nullptr ? effective->label() : "off" )
					);

					continue;
				}

				/* ⚠️ The fallback is SILENT at runtime on purpose — a trace here would be one
				 * line per slot per frame. This command is where it becomes visible, and the
				 * reason it can be trusted is that both sides are recorded by the render thread
				 * itself rather than re-derived here. */
				outputs.emplace_back(Severity::Warning, std::stringstream{} <<
					to_cstring(slot) << ": " << ( effective != nullptr ? effective->label() : "off" ) <<
					"  <- FALLBACK, you selected " << ( selected != nullptr ? selected->label() : "off" ) <<
					( selected != nullptr && selected->hasCreationFailed() ? " (its creation failed)" : " (it cannot run in this frame: ray tracing unavailable or not ready, or no main directional light)" )
				);
			}

			return true;
		}, "Report, per slot, what runs and whether it is what you selected.");

		this->bindCommand("select", [this] (const Console::Arguments & arguments, Console::Outputs & outputs) {
			if ( arguments.size() < 2 )
			{
				outputs.emplace_back(Severity::Error, "Usage: select(<slot>, <effect>). Use listEffects() for both vocabularies.");

				return false;
			}

			const auto slotName = arguments[0].asString();
			const auto slot = parseSlot(slotName);

			if ( !slot.has_value() )
			{
				outputs.emplace_back(Severity::Error, std::stringstream{} << "'" << slotName << "' is not a chain slot !");

				return false;
			}

			const auto effectName = arguments[1].asString();

			if ( !this->selectOccupant(slot.value(), effectName) )
			{
				outputs.emplace_back(Severity::Error, std::stringstream{} << "The '" << slotName << "' slot holds no occupant named '" << effectName << "' !");

				return false;
			}

			/* ⚠️ "Selected", not "enabled". The chain is walked by the RENDER thread and this
			 * command runs on the main one: the switch is applied by syncSlotSelection() at the
			 * next frame boundary, which is what keeps it from being a data race. */
			outputs.emplace_back(Severity::Success, std::stringstream{} << "'" << effectName << "' selected for the '" << slotName << "' slot (applied on the next frame).");

			return true;
		}, "Select the occupant of a slot. Arguments: slot name, effect name.");

		this->bindCommand("disable", [this] (const Console::Arguments & arguments, Console::Outputs & outputs) {
			if ( arguments.empty() )
			{
				outputs.emplace_back(Severity::Error, "Usage: disable(<slot>). Use listEffects() for the slot names.");

				return false;
			}

			const auto slotName = arguments[0].asString();
			const auto slot = parseSlot(slotName);

			if ( !slot.has_value() )
			{
				outputs.emplace_back(Severity::Error, std::stringstream{} << "'" << slotName << "' is not a chain slot !");

				return false;
			}

			this->selectNoOccupant(slot.value());

			outputs.emplace_back(Severity::Success, std::stringstream{} << "The '" << slotName << "' concept is switched off (applied on the next frame).");

			return true;
		}, "Switch a whole concept off. Argument: slot name.");

		this->bindCommand("setLightingMode", [this] (const Console::Arguments & arguments, Console::Outputs & outputs) {
			if ( arguments.empty() )
			{
				outputs.emplace_back(Severity::Error, "Usage: setLightingMode(\"ScreenSpace\") or setLightingMode(\"RayTracing\").");

				return false;
			}

			const auto modeName = arguments[0].asString();

			std::optional< LightingLane > lane;

			if ( modeName == to_cstring(LightingLane::ScreenSpace) )
			{
				lane = LightingLane::ScreenSpace;
			}
			else if ( modeName == to_cstring(LightingLane::RayTracing) )
			{
				lane = LightingLane::RayTracing;
			}

			if ( !lane.has_value() )
			{
				outputs.emplace_back(Severity::Error, std::stringstream{} << "'" << modeName << "' is not a lighting mode. Expected 'ScreenSpace' or 'RayTracing'.");

				return false;
			}

			if ( !this->selectLightingLane(lane.value()) )
			{
				outputs.emplace_back(Severity::Error, std::stringstream{} <<
					"No lighting slot has an occupant in the '" << modeName << "' lane, so nothing was changed. " <<
					( lane.value() == LightingLane::RayTracing
						? "The ray-traced lane is not resident this session: either this device cannot ray trace, or "
						  "'Core/Graphics/PostProcessing/LightingLane' was 'ScreenSpace'/'None' at launch, in which case no "
						  "acceleration structure was ever built. Set that key to 'Auto' and relaunch."
						: "The screen-space lane should always be resident — this is a wiring defect, not a configuration one." )
				);

				return false;
			}

			/* A lighting slot with no occupant in the requested lane is left EMPTY rather than
			 * kept on the other one, so the reader is told which ones went dark instead of
			 * discovering it on a capture. */
			for ( size_t index = 0; index < EffectSlotCount; ++index )
			{
				const auto slot = static_cast< EffectSlot >(index);

				if ( !isLightingSlot(slot) || m_slots[index].empty() )
				{
					continue;
				}

				if ( this->selectedOccupant(slot) == nullptr )
				{
					outputs.emplace_back(Severity::Warning, std::stringstream{} << "The '" << to_cstring(slot) << "' slot has no occupant in the '" << modeName << "' lane: it is now OFF.");
				}
			}

			outputs.emplace_back(Severity::Success, std::stringstream{} << "Lighting lane '" << modeName << "' selected (applied on the next frame).");

			return true;
		}, "Switch the whole lighting family to one lane. Argument: 'ScreenSpace' or 'RayTracing'.");
	}
}
