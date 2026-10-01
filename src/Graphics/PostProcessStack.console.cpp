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
#include <cstddef>
#include <iomanip>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>

/* Local inclusions. */
#include "IndirectPostProcessEffect.hpp"
#include "SettingKeys.hpp"
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

		/**
		 * @brief Returns a census channel name as a view.
		 * @param name The copied, zero-terminated name.
		 * @return std::string_view
		 */
		[[nodiscard]]
		std::string_view
		channelName (const OverflowCensusChannelName & name) noexcept
		{
			return {name.data()};
		}

		/**
		 * @brief Appends the "Metering:" line of a frame's diagnostics.
		 * @param metering The tone mapper metering of the frame.
		 * @param outputs The console outputs.
		 * @return void
		 */
		void
		appendMetering (const MeteringReport & metering, Console::Outputs & outputs) noexcept
		{
			if ( !metering.toneMapper )
			{
				outputs.emplace_back(Severity::Info, "Metering: no tone mapper in this chain (nothing is metered, the chain is not tone mapped).");

				return;
			}

			const auto rejected = std::to_string(metering.rejectedCount) + " measurement(s) rejected since the tone mapper was created (counted only with a camera)";

			if ( !metering.autoExposure )
			{
				outputs.emplace_back(Severity::Info, "Metering: manual — the camera's APEX triad exposes the frame, nothing is metered — " + rejected + ".");

				return;
			}

			if ( metering.meteredLuminance <= 0.0F )
			{
				outputs.emplace_back(Severity::Info, "Metering: auto — no measurement read back yet — " + rejected + ".");

				return;
			}

			outputs.emplace_back(Severity::Info, std::stringstream{} << std::fixed << std::setprecision(1) <<
				"Metering: auto — scene average " << metering.meteredLuminance << " nits — ISO " << std::setprecision(0) << metering.meteredSensitivity <<
				" at 1/" << ( metering.meteredShutterSpeed > 0.0F ? 1.0F / metering.meteredShutterSpeed : 0.0F ) << " s (aperture priority: the shutter shortens once the ISO sits at its floor) — " << rejected << "."
			);
		}

		/**
		 * @brief Returns the window part of a census channel line.
		 * @param entry The channel's window entry, or nullptr.
		 * @return std::string
		 */
		[[nodiscard]]
		std::string
		windowSummary (const OverflowCensusWindowChannel * entry) noexcept
		{
			std::stringstream text;

			if ( entry == nullptr )
			{
				text << "| window: not counted";
			}
			else if ( entry->maxOverflow > 0 )
			{
				text << "| window max " << entry->maxOverflow << " (frame " << entry->maxOverflowFrame << "), " << entry->framesOverflowing << "/" << entry->framesCounted << " frames";
			}
			else
			{
				text << "| window max 0 over " << entry->framesCounted << " frames, peak " << std::fixed << std::setprecision(1) << entry->maxPeakFinite;
			}

			return text.str();
		}

		/**
		 * @brief Appends the "Overflow census:" block of a frame's diagnostics.
		 * @param diagnostics The frame's diagnostics.
		 * @param outputs The console outputs.
		 * @return void
		 */
		void
		appendOverflowCensus (const FrameDiagnostics & diagnostics, Console::Outputs & outputs) noexcept
		{
			const auto & census = diagnostics.census;

			if ( !census.available )
			{
				outputs.emplace_back(Severity::Warning, "Overflow census: unavailable (creation failed, see the log).");

				return;
			}

			const auto & latest = census.latest;
			const auto & window = census.window;

			if ( !census.armed )
			{
				outputs.emplace_back(Severity::Info, "Overflow census: disarmed — Core.RendererService.setOverflowCensus(1) (or 'Core/Graphics/PostProcessing/OverflowCensus/Enabled' at launch).");
			}
			else if ( !latest.valid )
			{
				outputs.emplace_back(Severity::Info, std::stringstream{} << "Overflow census: ARMED — no frame counted yet (rendered " << diagnostics.renderedFrame << "; only the HDR chain is counted).");
			}
			else
			{
				std::stringstream header;
				header << "Overflow census: ARMED — last counted frame " << latest.frameSerial << " (rendered " << diagnostics.renderedFrame << ")";

				if ( window.framesCounted > 0 )
				{
					header << "; window from frame " << window.firstFrame << ": " << window.framesCounted << " frames counted, " << window.framesWithOverflow << " with an overflow, " << window.staleSlots << " stale";
				}
				else
				{
					header << "; window opened after frame " << window.startsAfterFrame << ", nothing counted in it yet, " << window.staleSlots << " stale";
				}

				if ( !latest.toneMapped )
				{
					header << " — NO tone mapper ran: ToneMapInput is the chain output";
				}

				outputs.emplace_back(Severity::Info, header.str());

				const auto findWindowChannel = [&window] (const OverflowCensusChannelName & name) -> const OverflowCensusWindowChannel * {
					for ( uint32_t index = 0; index < window.channelCount; ++index )
					{
						if ( window.channels[index].name == name )
						{
							return &window.channels[index];
						}
					}

					return nullptr;
				};

				for ( uint32_t index = 0; index < latest.channelCount; ++index )
				{
					const auto & channel = latest.channels[index];

					std::stringstream line;
					line <<
						"  " << std::left << std::setw(16) << channelName(channel.name) << " " <<
						channel.tested << "/" << channel.expectedTexels << " texels  NaN " << channel.nanTexels << "  Inf " << channel.infTexels << "  ceiling " << channel.ceilingTexels <<
						"  peak " << std::fixed << std::setprecision(1) << channel.peakFinite << "  " << windowSummary(findWindowChannel(channel.name));

					if ( channel.invalid )
					{
						line << "  <- INVALID: the census did not test the whole image, these counts cannot be trusted";
					}

					outputs.emplace_back(channel.invalid ? Severity::Warning : Severity::Info, line.str());
				}

				/* A channel of the window absent from the last counted frame: its effect did not run in that frame. */
				for ( uint32_t index = 0; index < window.channelCount; ++index )
				{
					const auto & entry = window.channels[index];

					const auto counted = std::any_of(latest.channels.begin(), latest.channels.begin() + static_cast< std::ptrdiff_t >(latest.channelCount), [&entry] (const OverflowCensusChannel & channel) {
						return channel.name == entry.name;
					});

					if ( !counted )
					{
						outputs.emplace_back(Severity::Info, std::stringstream{} << "  " << std::left << std::setw(16) << channelName(entry.name) << " not counted in that frame (the effect did not run)  " << windowSummary(&entry));
					}
				}
			}

			/* The positive control: a count means something only on a machine where it passed. */
			if ( !census.selfTest.ran )
			{
				outputs.emplace_back(Severity::Info, "Overflow census: a clean count is meaningful only after Core.RendererService.testOverflowCensus() answered PASS on this machine.");
			}
			else if ( census.selfTest.passed )
			{
				outputs.emplace_back(Severity::Info, std::stringstream{} << "Overflow census: self-test PASSED on frame " << census.selfTest.frameSerial << " this session.");
			}
			else
			{
				outputs.emplace_back(Severity::Warning, std::stringstream{} << "Overflow census: self-test FAILED on frame " << census.selfTest.frameSerial << " — the census is BLIND on this machine, no count it reports can be trusted.");
			}
		}
	}

	void
	PostProcessStack::onRegisterToConsole () noexcept
	{
		this->bindCommand("listEffects", "List every slot of the chain, its occupants, which one is selected and which one actually runs.", [this] () {
			Console::Outputs outputs;
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

					const char * state = "not created";

					if ( occupant->isCreated() )
					{
						state = "created";
					}
					else if ( occupant->hasCreationFailed() )
					{
						state = "CREATION FAILED";
					}

					outputs.emplace_back(Severity::Info, std::stringstream{} <<
						"  " << mark << " " << occupant->label() <<
						" (" << laneName(*occupant) << ", " << state << ")"
					);
				}
			}

			if ( !any )
			{
				outputs.emplace_back(Severity::Warning, "The post-process stack holds no chain effect.");

				return Console::CommandResult::fromOutputs(std::move(outputs), false);
			}

			outputs.emplace_back(Severity::Info, "Legend: '>' selected by you, '*' running in the last frame.");

			return Console::CommandResult::fromOutputs(std::move(outputs), true);
		}, Console::CommandHint::ReadOnly);

		this->bindCommand("getStatus", "Report, per slot, what runs and whether it is what you selected; then the tone mapper metering and the overflow census of the last frame.", [this] () {
			Console::Outputs outputs;

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

			/* The lane the family stands on, as recorded by the whole-family switches. */
			if ( const auto lane = this->selectedLightingLane(); lane.has_value() )
			{
				outputs.emplace_back(Severity::Info, std::stringstream{} << "Lane selected: " << to_cstring(lane.value()) << ".");
			}
			else
			{
				outputs.emplace_back(Severity::Info, "Lane selected: none — the lighting family is off (setLightingMode(\"ScreenSpace\"|\"RayTracing\") brings it back, concepts as they were).");
			}

			/* The bypass overrides every scene slot below: say so BEFORE them, or each "off" reads as a
			 * fallback. */
			const auto sceneEffectsBypassed = this->isSceneEffectsBypassed();

			if ( sceneEffectsBypassed )
			{
				outputs.emplace_back(Severity::Warning, "Scene effects: BYPASSED — every scene slot is off, the camera chain (exposure, tone mapping) still runs; bypassSceneEffects(0) brings back exactly what ran before.");
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

				if ( sceneEffectsBypassed && isSceneEffectSlot(slot) )
				{
					outputs.emplace_back(Severity::Info, std::stringstream{} <<
						to_cstring(slot) << ": off  (bypassed; selected " << ( selected != nullptr ? selected->label() : "off" ) << ")"
					);

					continue;
				}

				if ( selected == effective )
				{
					/* A lighting concept switched OFF by its gate says so: "off" alone reads as a
					 * fallback or a wiring defect, when it is the owner's setting being honoured
					 * across the lane switches. */
					const auto gatedOff = isLightingSlot(slot) && !this->isConceptEnabled(slot);

					outputs.emplace_back(Severity::Info, std::stringstream{} <<
						to_cstring(slot) << ": " << ( effective != nullptr ? effective->label() : "off" ) <<
						( gatedOff ? "  (concept switched off — a lane switch leaves it off; select(<slot>, <effect>) turns it back on)" : "" )
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

			/* The frame diagnostics the render thread published with this stack (a copy: nothing here reads
			 * render-thread state). */
			if ( const auto diagnostics = this->frameDiagnostics(); diagnostics.renderedFrame == 0 )
			{
				outputs.emplace_back(Severity::Info, "Metering / overflow census: no frame rendered with this stack yet.");
			}
			else
			{
				appendMetering(diagnostics.metering, outputs);
				appendOverflowCensus(diagnostics, outputs);
			}

			return Console::CommandResult::fromOutputs(std::move(outputs), true);
		}, Console::CommandHint::ReadOnly);

		this->bindCommand("select", "Select the occupant of a slot (applied on the next frame). A concept switched off is turned back on for the session.",
			{
				{"slot", "The slot name, e.g. 'IndirectDiffuse' (listEffects() lists them)."},
				{"effect", "The occupant to run in that slot, e.g. 'RTGI' (listEffects() lists them)."}
			},
			[this] (const std::string & slotName, const std::string & effectName) {
				const auto slot = parseSlot(slotName);

				if ( !slot.has_value() )
				{
					return Console::CommandResult::error("'" + slotName + "' is not a chain slot !");
				}

				if ( !this->selectOccupant(slot.value(), effectName) )
				{
					return Console::CommandResult::error("The '" + slotName + "' slot holds no occupant named '" + effectName + "' !");
				}

				/* ⚠️ "Selected", not "enabled". The chain is walked by the RENDER thread and this
				 * command runs on the main one: the switch is applied by syncSlotSelection() at the
				 * next frame boundary, which is what keeps it from being a data race. */
				return Console::CommandResult::success("'" + effectName + "' selected for the '" + slotName + "' slot (applied on the next frame).");
			}, Console::CommandHint::Idempotent);

		this->bindCommand("disable", "Switch a whole concept off, for the session — a lane switch leaves it off.",
			{
				{"slot", "The slot name, e.g. 'Reflections' (listEffects() lists them)."}
			},
			[this] (const std::string & slotName) {
				const auto slot = parseSlot(slotName);

				if ( !slot.has_value() )
				{
					return Console::CommandResult::error("'" + slotName + "' is not a chain slot !");
				}

				this->selectNoOccupant(slot.value());

				return Console::CommandResult::success("The '" + slotName + "' concept is switched off (applied on the next frame). It stays off across lane switches; select(" + slotName + ", <effect>) turns it back on.");
			}, Console::CommandHint::Idempotent);

		this->bindCommand("bypassSceneEffects", "Switch every SCENE effect off or back on, keeping the camera's exposure and tone mapping — the no-effect A/B. Selections, concept gates and lane are kept.",
			{
				{"bypassed", "1 (true) switches every scene effect off, 0 (false) brings them back exactly as selected."}
			},
			[this] (bool bypassed) {
				this->bypassSceneEffects(bypassed);

				return Console::CommandResult::success(bypassed ?
					"Scene effects BYPASSED (applied on the next frame): lighting family, clouds, light shafts, fog, custom effects and TAA are off; the camera chain keeps running. Nothing selected was changed." :
					"Scene effects running again (applied on the next frame), exactly as selected before the bypass."
				);
			}, Console::CommandHint::Idempotent);

		this->bindCommand("setLightingMode", "Switch the whole lighting family to one lane, or off. A concept switched off stays off.",
			{
				{"mode", "'ScreenSpace', 'RayTracing' (refused when the traced lane is not resident this session) or 'None' (family off, concept switches kept)."}
			},
			[this] (const std::string & modeName) {
				Console::Outputs outputs;

				/* "None" = the whole family off, the concept gates kept — the live twin of the
				 * `LightingLane = "None"` setting, and the one-command control a "no indirect
				 * lighting" capture needs. */
				if ( modeName == GraphicsPPLightingLaneNone )
				{
					this->selectNoLightingLane();

					return Console::CommandResult::success("Lighting family switched OFF (applied on the next frame). The concepts keep their switches: the next lane selection brings back exactly those that were on.");
				}

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
					return Console::CommandResult::error("'" + modeName + "' is not a lighting mode. Expected 'ScreenSpace', 'RayTracing' or 'None'.");
				}

				if ( !this->selectLightingLane(lane.value()) )
				{
					std::stringstream message;
					message <<
						"No lighting slot has an occupant in the '" << modeName << "' lane, so nothing was changed. " <<
						( lane.value() == LightingLane::RayTracing
							? "The ray-traced lane is not resident this session: either this device cannot ray trace, or "
							  "'Core/Graphics/PostProcessing/LightingLane' was 'ScreenSpace'/'None' at launch, in which case no "
							  "acceleration structure was ever built. Set that key to 'Auto' and relaunch."
							: "The screen-space lane should always be resident — this is a wiring defect, not a configuration one." );

					return Console::CommandResult::error(message.str());
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

					if ( this->selectedOccupant(slot) != nullptr )
					{
						continue;
					}

					/* Two reasons for an empty slot, and they must not be confused: the owner's gate
					 * (honoured, expected) or a lane with no occupant there (a wiring gap, worth a
					 * warning). */
					if ( !this->isConceptEnabled(slot) )
					{
						outputs.emplace_back(Severity::Info, std::stringstream{} << "The '" << to_cstring(slot) << "' concept stays OFF: it is switched off (setting or disable()); select(" << to_cstring(slot) << ", <effect>) turns it back on.");
					}
					else
					{
						outputs.emplace_back(Severity::Warning, std::stringstream{} << "The '" << to_cstring(slot) << "' slot has no occupant in the '" << modeName << "' lane: it is now OFF.");
					}
				}

				outputs.emplace_back(Severity::Success, std::stringstream{} << "Lighting lane '" << modeName << "' selected (applied on the next frame).");

				return Console::CommandResult::fromOutputs(std::move(outputs), true);
			}, Console::CommandHint::Idempotent);
	}
}
