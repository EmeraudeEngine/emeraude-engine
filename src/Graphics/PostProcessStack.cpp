/*
 * src/Graphics/PostProcessStack.cpp
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
#include <array>
#include <string>
#include <utility>

/* Local inclusions. */
#include "Effects/Camera/VeilingGlare.hpp"
#include "Effects/Camera/DepthOfField.hpp"
#include "Effects/Camera/MotionBlur.hpp"
#include "Effects/Camera/ToneMapping.hpp"
#include "Effects/Lighting/RTContactShadows.hpp"
#include "Effects/Lighting/SSContactShadows.hpp"
#include "Effects/Lighting/RTAO.hpp"
#include "Effects/Lighting/RTGI.hpp"
#include "Effects/Lighting/RTR.hpp"
#include "Effects/Lighting/SSAO.hpp"
#include "Effects/Lighting/SSGI.hpp"
#include "Effects/Lighting/SSR.hpp"
#include "IndirectPostProcessEffect.hpp"
#include "PrimaryServices.hpp"
#include "Renderer.hpp"
#include "Scenes/Component/Camera.hpp"
#include "Scenes/LightSet.hpp"
#include "SettingKeys.hpp"
#include "Tracer.hpp"
#include "Vulkan/SwapChain.hpp"

namespace EmEn::Graphics
{
	PostProcessStack::~PostProcessStack () noexcept
	{
		this->destroyAll();

		/* ⚠️ The effects may outlive this stack — an application keeps shared_ptr copies to
		 * toggle them — so their back-pointer must die WITH the stack, not after it. */
		for ( auto & slot : m_slots )
		{
			for ( const auto & effect : slot )
			{
				if ( effect != nullptr )
				{
					effect->setOwnerStack(nullptr);
				}
			}
		}
	}

	void
	PostProcessStack::addEffect (const std::shared_ptr< IndirectPostProcessEffect > & effect) noexcept
	{
		if ( effect == nullptr )
		{
			return;
		}

		const auto slot = effect->slot();

		/* The photographic chain belongs to the camera: syncCameraEffects() materializes those
		 * four effects from the camera's own switches and owns their lifetime. An application
		 * adding one by hand would have it destroyed under its feet at the next camera change. */
		if ( !isChainSlot(slot) )
		{
			TraceError{ClassId} <<
				"The effect '" << effect->label() << "' declares EffectSlot::Internal: it is a "
				"component owned by an effect, it does not belong to a chain !";

			return;
		}

		if ( isCameraEffectSlot(slot) )
		{
			TraceError{ClassId} <<
				"The '" << to_cstring(slot) << "' slot belongs to the active camera: "
				"declare it on the camera (enableDepthOfField(), enableHDR(), ...), never on the stack !";

			return;
		}

		auto & occupants = m_slots[static_cast< size_t >(slot)];

		if ( std::ranges::find(occupants, effect) != occupants.end() )
		{
			return;
		}

		effect->setOwnerStack(this);

		occupants.emplace_back(effect);

		/* Several occupants are LEGAL and are how a runtime A/B works — but only one of a
		 * concept may run. Every effect is enabled at construction, so the newcomer selects
		 * itself, exactly as a later enable() would. */
		if ( effect->isEnabled() && !isMultiOccupantSlot(slot) )
		{
			this->disableSlotSiblings(*effect);

			/* The SELECTION follows, and it must: it is what syncSlotSelection() reads every
			 * frame on the render thread. Left behind, the very next sync would revert this
			 * effect to whatever was selected before it existed. */
			m_selectedOccupant[static_cast< size_t >(slot)].store(static_cast< int8_t >(occupants.size() - 1), std::memory_order_relaxed);
		}

		this->rebuildOrderedEffects();
	}

	void
	PostProcessStack::removeEffect (const std::shared_ptr< IndirectPostProcessEffect > & effect) noexcept
	{
		if ( effect == nullptr )
		{
			return;
		}

		const auto slot = effect->slot();

		auto & occupants = m_slots[static_cast< size_t >(slot)];

		const auto removedIt = std::ranges::find(occupants, effect);

		if ( removedIt == occupants.end() )
		{
			return;
		}

		const auto removedIndex = static_cast< int8_t >(std::distance(occupants.begin(), removedIt));

		occupants.erase(removedIt);

		effect->setOwnerStack(nullptr);

		/* ⚠️ The selection is an INDEX into this very vector, so an erase invalidates it: the
		 * indices past the hole all shift down by one, and removing the selected occupant leaves
		 * the slot selecting a stranger. This is the one place that has to repair it. */
		auto & selected = m_selectedOccupant[static_cast< size_t >(slot)];

		const auto current = selected.load(std::memory_order_relaxed);

		if ( current == removedIndex )
		{
			selected.store(NoOccupant, std::memory_order_relaxed);
		}
		else if ( current > removedIndex )
		{
			selected.store(static_cast< int8_t >(current - 1), std::memory_order_relaxed);
		}

		this->rebuildOrderedEffects();
	}

	void
	PostProcessStack::clearEffects () noexcept
	{
		for ( auto & slot : m_slots )
		{
			for ( const auto & effect : slot )
			{
				if ( effect != nullptr )
				{
					effect->setOwnerStack(nullptr);
				}
			}

			slot.clear();
		}

		m_displayEffects.clear();

		for ( auto & selected : m_selectedOccupant )
		{
			selected.store(NoOccupant, std::memory_order_relaxed);
		}

		this->rebuildOrderedEffects();
	}

	void
	PostProcessStack::disableSlotSiblings (const IndirectPostProcessEffect & effect) const noexcept
	{
		const auto slot = effect.slot();

		if ( isMultiOccupantSlot(slot) )
		{
			return;
		}

		for ( const auto & occupant : m_slots[static_cast< size_t >(slot)] )
		{
			if ( occupant != nullptr && occupant.get() != &effect )
			{
				/* ⚠️ The FLAG, not enable(): going through enable() would ask this very method
				 * to disable the siblings of the effect being disabled — infinite recursion. */
				occupant->setEnabledFlag(false);
			}
		}
	}

	std::shared_ptr< IndirectPostProcessEffect >
	PostProcessStack::enabledEffect (EffectSlot slot) const noexcept
	{
		for ( const auto & occupant : m_slots[static_cast< size_t >(slot)] )
		{
			if ( occupant != nullptr && occupant->isEnabled() )
			{
				return occupant;
			}
		}

		return nullptr;
	}

	std::shared_ptr< IndirectPostProcessEffect >
	PostProcessStack::selectedOccupant (EffectSlot slot) const noexcept
	{
		const auto index = m_selectedOccupant[static_cast< size_t >(slot)].load(std::memory_order_relaxed);

		if ( index < 0 )
		{
			return nullptr;
		}

		const auto & occupants = m_slots[static_cast< size_t >(slot)];

		if ( static_cast< size_t >(index) >= occupants.size() )
		{
			return nullptr;
		}

		return occupants[static_cast< size_t >(index)];
	}

	std::shared_ptr< IndirectPostProcessEffect >
	PostProcessStack::effectiveOccupant (EffectSlot slot) const noexcept
	{
		const auto index = m_effectiveOccupant[static_cast< size_t >(slot)].load(std::memory_order_relaxed);

		if ( index < 0 )
		{
			return nullptr;
		}

		const auto & occupants = m_slots[static_cast< size_t >(slot)];

		if ( static_cast< size_t >(index) >= occupants.size() )
		{
			return nullptr;
		}

		return occupants[static_cast< size_t >(index)];
	}

	bool
	PostProcessStack::selectOccupant (EffectSlot slot, std::string_view label) noexcept
	{
		if ( !isChainSlot(slot) )
		{
			return false;
		}

		const auto & occupants = m_slots[static_cast< size_t >(slot)];

		for ( size_t index = 0; index < occupants.size(); ++index )
		{
			if ( occupants[index] == nullptr || label != occupants[index]->label() )
			{
				continue;
			}

			m_selectedOccupant[static_cast< size_t >(slot)].store(static_cast< int8_t >(index), std::memory_order_relaxed);

			return true;
		}

		return false;
	}

	void
	PostProcessStack::selectNoOccupant (EffectSlot slot) noexcept
	{
		if ( !isChainSlot(slot) )
		{
			return;
		}

		m_selectedOccupant[static_cast< size_t >(slot)].store(NoOccupant, std::memory_order_relaxed);
	}

	bool
	PostProcessStack::selectLightingLane (LightingLane lane) noexcept
	{
		const auto wantRayTracing = lane == LightingLane::RayTracing;

		/* ⚠️ Resolve EVERYTHING before writing anything. A lane that is nowhere resident must
		 * leave the selection untouched: applying it slot by slot would switch the whole family
		 * off on the way to discovering there was nothing to select. */
		std::array< int8_t, EffectSlotCount > resolved{};

		resolved.fill(NoOccupant);

		auto found = false;

		for ( size_t index = 0; index < EffectSlotCount; ++index )
		{
			const auto slot = static_cast< EffectSlot >(index);

			if ( !isLightingSlot(slot) )
			{
				continue;
			}

			const auto & occupants = m_slots[index];

			/* ⚠️ A lighting slot with no occupant in the requested lane is left EMPTY, never
			 * kept on the other one. Asking for the screen-space lane and silently getting a
			 * ray-traced contact shadow would turn an A/B measurement into a comparison of two
			 * things that are not what they say they are. */
			auto selected = NoOccupant;

			for ( size_t occupantIndex = 0; occupantIndex < occupants.size(); ++occupantIndex )
			{
				if ( occupants[occupantIndex] == nullptr || occupants[occupantIndex]->requiresRayTracing() != wantRayTracing )
				{
					continue;
				}

				selected = static_cast< int8_t >(occupantIndex);

				break;
			}

			resolved[index] = selected;

			if ( selected != NoOccupant )
			{
				found = true;
			}
		}

		if ( !found )
		{
			return false;
		}

		for ( size_t index = 0; index < EffectSlotCount; ++index )
		{
			if ( isLightingSlot(static_cast< EffectSlot >(index)) )
			{
				m_selectedOccupant[index].store(resolved[index], std::memory_order_relaxed);
			}
		}

		return true;
	}

	void
	PostProcessStack::installLightingFamily (Renderer & renderer) noexcept
	{
		auto & settings = renderer.primaryServices().settings();

		/* The screen-space lane is unconditional: it is what every device can run, and what the
		 * ray-traced lane falls back to. */
		this->addEffect(std::make_shared< Effects::Lighting::SSGI >(renderer));
		this->addEffect(std::make_shared< Effects::Lighting::SSR >(renderer));
		this->addEffect(std::make_shared< Effects::Lighting::SSAO >(renderer));
		this->addEffect(std::make_shared< Effects::Lighting::SSContactShadows >(renderer));

		/* ⚠️ The SESSION-CONSTANT condition, and only it. Renderer::isRayTracingReady() has no
		 * business here: it answers a question about THIS FRAME (the TLAS is built
		 * asynchronously, and a scene may hold no ray traced geometry), so consulting it at
		 * install time would refuse a lane that becomes runnable three frames later — and it is
		 * already consulted, every frame, by canOccupantRun().
		 * ⚠️⚠️ The question asked is "does an acceleration structure builder EXIST", not "does the
		 * device support ray tracing". They differ: the Renderer only creates the builder when the
		 * requested lane may need it ("Auto" or "RayTracing"), and without it no geometry ever
		 * built a BLAS — so on a perfectly capable device with the lane set to "ScreenSpace" there
		 * is nothing to trace against, for the whole session. Filing the traced lane anyway would
		 * offer a `setLightingMode("RayTracing")` that quietly renders nothing. One pointer, one
		 * truth. */
		const auto rayTracingLaneAvailable = renderer.accelerationStructureBuilder() != nullptr;

		if ( rayTracingLaneAvailable )
		{
			this->addEffect(std::make_shared< Effects::Lighting::RTGI >(renderer));
			this->addEffect(std::make_shared< Effects::Lighting::RTR >(renderer));
			this->addEffect(std::make_shared< Effects::Lighting::RTAO >(renderer));
			this->addEffect(std::make_shared< Effects::Lighting::RTContactShadows >(renderer));
		}

		/* ⚠️ EXPLICIT, although the call order above would already leave the ray-traced lane
		 * selected (addEffect() selects the newcomer). "Correct because of the order the calls
		 * happen to be in" is exactly the property the EffectSlot table was built to destroy for
		 * the chain order; it has no more business deciding the default lane. */
		const auto requestedLane = settings.getOrSetDefault< std::string >(GraphicsPPLightingLaneKey, DefaultGraphicsPPLightingLane);

		auto lane = LightingLane::RayTracing;
		auto wantsNoLane = false;

		/* "Auto" resolves to the ray-traced lane and lets the availability check below quietly
		 * downgrade it — it is a policy, not a request, so an unavailable lane is not a problem
		 * worth a warning. Naming a lane explicitly IS a request. */
		if ( requestedLane == to_cstring(LightingLane::ScreenSpace) )
		{
			lane = LightingLane::ScreenSpace;
		}
		else if ( requestedLane == GraphicsPPLightingLaneNone )
		{
			wantsNoLane = true;
		}
		else if ( requestedLane != to_cstring(LightingLane::RayTracing) && requestedLane != GraphicsPPLightingLaneAuto )
		{
			TraceWarning{ClassId} <<
				"'" << GraphicsPPLightingLaneKey << "' holds '" << requestedLane << "', which is not a lighting lane. "
				"Expected 'Auto', 'RayTracing', 'ScreenSpace' or 'None' — behaving as 'Auto'.";
		}

		/* ⚠️⚠️ An unavailable lane must SAY SO. This used to fall back to screen space in silence,
		 * and the owner hit it on the first settings reset (2026-09-10): a second key,
		 * `Core/Graphics/RayTracing/Enabled`, defaulted to FALSE and silently dominated this one,
		 * so the file read "RayTracing" and the frame was screen space with nothing to explain it.
		 * That key is GONE; the only remaining reason is that the device cannot ray trace.
		 * A silent fallback on an EXPLICIT request is a lie by omission; a fallback on a request
		 * nobody made ("Auto") is not — which is why this only fires when the file actually names
		 * the traced lane. */
		if ( lane == LightingLane::RayTracing && !rayTracingLaneAvailable )
		{
			if ( requestedLane == to_cstring(LightingLane::RayTracing) )
			{
				TraceWarning{ClassId} <<
					"'" << GraphicsPPLightingLaneKey << "' asks for the ray-traced lighting lane, but no acceleration structure builder exists: " <<
					( renderer.device()->rayTracingEnabled()
						? "the builder was not created at renderer initialization (see Renderer::onInitialize)"
						: "this device has no ray tracing support (no VK_KHR_acceleration_structure / VK_KHR_ray_query)" ) <<
					". The lighting family starts on the SCREEN-SPACE lane. Set the key to 'Auto' to follow what this machine can actually do.";
			}

			lane = LightingLane::ScreenSpace;
		}

		static_cast< void >(this->selectLightingLane(lane));

		if ( wantsNoLane )
		{
			for ( size_t index = 0; index < EffectSlotCount; ++index )
			{
				const auto slot = static_cast< EffectSlot >(index);

				if ( isLightingSlot(slot) )
				{
					this->selectNoOccupant(slot);
				}
			}

			return;
		}

		/* The per-CONCEPT switches, applied AFTER the lane: turning a concept off must survive
		 * the lane choice, and a later setLightingMode() from the console deliberately brings it
		 * back — the settings decide how the scene STARTS, the console owns the session. */
		const std::array< std::pair< EffectSlot, bool >, 4 > conceptSwitches{{
			{EffectSlot::ContactShadows, settings.getOrSetDefault< bool >(GraphicsPPContactShadowsEnabledKey, DefaultGraphicsPPContactShadowsEnabled)},
			{EffectSlot::IndirectDiffuse, settings.getOrSetDefault< bool >(GraphicsPPIndirectDiffuseEnabledKey, DefaultGraphicsPPIndirectDiffuseEnabled)},
			{EffectSlot::Reflections, settings.getOrSetDefault< bool >(GraphicsPPReflectionsEnabledKey, DefaultGraphicsPPReflectionsEnabled)},
			{EffectSlot::AmbientOcclusion, settings.getOrSetDefault< bool >(GraphicsPPAmbientOcclusionEnabledKey, DefaultGraphicsPPAmbientOcclusionEnabled)}
		}};

		for ( const auto & [slot, enabled] : conceptSwitches )
		{
			if ( !enabled )
			{
				this->selectNoOccupant(slot);
			}
		}
	}

	bool
	PostProcessStack::canOccupantRun (const IndirectPostProcessEffect & effect, const Renderer & renderer, const Scenes::LightSet * lightSet) noexcept
	{
		/* An effect whose creation failed will not start working on its own — and the slot has an
		 * alternative that might. */
		if ( effect.hasCreationFailed() )
		{
			return false;
		}

		/* ⚠️ THE SAME THREE CONDITIONS the executor skips a ray traced effect on
		 * (PostProcessor::executeIndirectPostProcessEffects). They must stay identical: a
		 * fallback that disagrees with the recorder either leaves the slot dark while claiming
		 * otherwise, or enables two occupants of one concept. */
		if ( effect.requiresRayTracing() && (!renderer.device()->rayTracingEnabled() || !renderer.isRayTracingReady()) )
		{
			return false;
		}

		if ( effect.requiresLightSet() && (lightSet == nullptr || lightSet->mainDirectionalLight() == nullptr) )
		{
			return false;
		}

		return true;
	}

	bool
	PostProcessStack::materializeOccupant (IndirectPostProcessEffect & effect, Renderer & renderer) noexcept
	{
		if ( effect.isCreated() )
		{
			return true;
		}

		const auto renderTarget = renderer.mainRenderTarget();

		if ( renderTarget == nullptr )
		{
			return false;
		}

		const auto & extent = renderTarget->extent();

		const auto created = effect.create(extent.width, extent.height);

		effect.setCreatedFlag(created);

		if ( !created )
		{
			/* ⚠️ LATCHED. Without it this runs again on the very next frame, and every frame
			 * after that: an allocation storm plus one trace line per frame, for a failure that
			 * will not resolve itself. Cleared only by a resize, where the extent — a plausible
			 * cause of the failure — has actually changed. */
			effect.setCreationFailedFlag();

			TraceError{ClassId} << "Failed to materialize the '" << effect.label() << "' effect selected for the '" << to_cstring(effect.slot()) << "' slot !";
		}

		return created;
	}

	void
	PostProcessStack::syncSlotSelection (Renderer & renderer, const Scenes::LightSet * lightSet) noexcept
	{
		for ( size_t index = 0; index < EffectSlotCount; ++index )
		{
			const auto slot = static_cast< EffectSlot >(index);

			/* The Custom slot holds several effects at once, all of them running: there is no
			 * selection to resolve, and no sibling to fall back to. */
			if ( !isChainSlot(slot) || isCameraEffectSlot(slot) || isMultiOccupantSlot(slot) )
			{
				continue;
			}

			const auto & occupants = m_slots[index];

			if ( occupants.empty() )
			{
				continue;
			}

			const auto selected = this->selectedOccupant(slot);

			/* The effective occupant: what the owner asked for when it can run, otherwise the
			 * first sibling that can — the automatic lane fallback. */
			std::shared_ptr< IndirectPostProcessEffect > effective;

			if ( selected != nullptr && canOccupantRun(*selected, renderer, lightSet) )
			{
				effective = selected;

				m_selectionStallFrames[index] = 0;
			}
			else if ( selected != nullptr )
			{
				/* ⚠️ THE GRACE PERIOD, and it is what keeps the fallback from eating the lazy
				 * residency alive. The selected occupant being unable to run is the NORMAL state
				 * for the first frames of a ray-traced scene (the TLAS is built asynchronously),
				 * so falling back on the first frame materialized the entire screen-space lane on
				 * every launch — the exact allocation the residency exists to avoid. Below the
				 * threshold the slot keeps whatever it had, which for a fresh scene is the
				 * selected occupant sitting enabled-but-skipped: a couple of dark frames, the
				 * behaviour that predates this whole mechanism. */
				if ( m_selectionStallFrames[index] < FallbackGraceFrames )
				{
					++m_selectionStallFrames[index];

					continue;
				}

				for ( const auto & occupant : occupants )
				{
					if ( occupant != nullptr && occupant != selected && canOccupantRun(*occupant, renderer, lightSet) )
					{
						effective = occupant;

						break;
					}
				}
			}
			else
			{
				m_selectionStallFrames[index] = 0;
			}

			/* What the console will report as "running". ⚠️ It is stored on the UNCHANGED path
			 * too, not only when the selection moves: recorded after the early-out below, a slot
			 * whose selection was already correct on the very first frame — TAA, and every slot
			 * of a scene that never switches lane — reached the store never once and reported
			 * "nothing runs here" for the whole session while running perfectly. Measured on
			 * Sponza: listEffects() marked TAA as selected and not running. */
			const auto recordEffective = [this, index, &occupants] (const std::shared_ptr< IndirectPostProcessEffect > & occupant) {
				m_effectiveOccupant[index].store(
					occupant == nullptr ? NoOccupant : static_cast< int8_t >(std::distance(occupants.begin(), std::ranges::find(occupants, occupant))),
					std::memory_order_relaxed
				);
			};

			/* Nothing changes: the common case, every frame, once the scene has settled. */
			if ( effective == this->enabledEffect(slot) )
			{
				recordEffective(effective);

				continue;
			}

			if ( effective == nullptr )
			{
				this->disableSlotOccupants(slot);

				recordEffective(nullptr);

				continue;
			}

			/* ⚠️ Materialize BEFORE enabling. Enabled and un-created is the one combination the
			 * executor cannot record — it gates on both, so the slot would simply go dark, and
			 * the sibling that could have run stays disabled.
			 * ⚠️ And nothing is recorded on failure: the report must keep saying what the LAST
			 * good frame ran, not name an occupant that just failed to allocate. The latch
			 * materializeOccupant() sets makes the next frame fall back to the sibling. */
			if ( !materializeOccupant(*effective, renderer) )
			{
				continue;
			}

			/* The single site where a chain effect is enabled. enable() disables the siblings on
			 * its own, so the exclusivity of the concept holds by construction. */
			effective->enable(true);

			recordEffective(effective);
		}
	}

	void
	PostProcessStack::disableSlotOccupants (EffectSlot slot) const noexcept
	{
		for ( const auto & occupant : m_slots[static_cast< size_t >(slot)] )
		{
			if ( occupant != nullptr )
			{
				/* ⚠️ The FLAG, not enable(false): the same recursion guard disableSlotSiblings()
				 * documents. */
				occupant->setEnabledFlag(false);
			}
		}
	}

	void
	PostProcessStack::rebuildOrderedEffects () noexcept
	{
		m_orderedEffects.clear();

		/* THE chain order: the slot table flattened in EffectSlot declaration order. */
		for ( const auto & slot : m_slots )
		{
			for ( const auto & effect : slot )
			{
				if ( effect != nullptr )
				{
					m_orderedEffects.emplace_back(effect);
				}
			}
		}
	}

	bool
	PostProcessStack::hasEnabledReflectionProvider () const noexcept
	{
		return std::ranges::any_of(m_orderedEffects, [] (const auto & effect) {
			return effect != nullptr && effect->isEnabled() && effect->providesReflections();
		});
	}

	bool
	PostProcessStack::hasEnabledPreTranslucencyEffect () const noexcept
	{
		return std::ranges::any_of(m_orderedEffects, [] (const auto & effect) {
			return effect != nullptr && effect->isEnabled() && isPreTranslucencySlot(effect->slot());
		});
	}

	bool
	PostProcessStack::hasEnabledIndirectDiffuseProvider () const noexcept
	{
		return std::ranges::any_of(m_orderedEffects, [] (const auto & effect) {
			return effect != nullptr && effect->isEnabled() && effect->providesIndirectDiffuse();
		});
	}

	void
	PostProcessStack::syncSlotPairings () const noexcept
	{
		/* ---- The ambient-occlusion lane ----
		 * The producer is the enabled indirect-diffuse occupant, IF it can publish a lane; the
		 * consumer is the enabled ambient-occlusion occupant, IF it can read one. Both must hold
		 * their GPU resources: an un-created producer has no lane texture, and an un-created
		 * consumer is not recorded at all. */
		const auto producer = this->enabledEffect(EffectSlot::IndirectDiffuse);
		const auto consumer = this->enabledEffect(EffectSlot::AmbientOcclusion);

		const bool pairable =
			producer != nullptr && producer->isCreated() && producer->providesOcclusionLane() &&
			consumer != nullptr && consumer->isCreated() && consumer->consumesOcclusionLane();

		if ( pairable )
		{
			/* Bidirectional: the producer cannot reduce an occlusion without the consumer's
			 * range, and the consumer cannot read a lane it has not been handed. */
			producer->setOcclusionLaneEnabled(true, consumer->occlusionMaxDistance());

			/* ⚠️ Asked AFTER arming: an implementation may only expose its lane once armed. */
			consumer->setOcclusionLaneSource(producer->occlusionLaneTexture());

			return;
		}

		/* Not pairable: disarm BOTH sides every frame. The producer stops paying for a lane
		 * nobody reads, and — the load-bearing half — the consumer drops a pointer that would
		 * otherwise survive the removal of the producer that owns the texture. */
		if ( producer != nullptr && producer->providesOcclusionLane() )
		{
			producer->setOcclusionLaneEnabled(false, 0.0F);
		}

		if ( consumer != nullptr && consumer->consumesOcclusionLane() )
		{
			consumer->setOcclusionLaneSource(nullptr);
		}
	}

	bool
	PostProcessStack::syncCameraEffects (const Scenes::Component::Camera * camera, Renderer & renderer) noexcept
	{
		/* ⚠️ The camera REQUESTS, the user DISPOSES. These two effects are expensive (7 and 4
		 * passes) and intrusive, so a settings refusal wins over what the scene asked for and the
		 * effect is never materialized — `Core/Graphics/PostProcessing/{DepthOfField,MotionBlur}/Enabled`.
		 * Enforced HERE because this is the only place either effect comes into existence: the
		 * motion-blur refusal used to live in projet-alpha's Player at spawn time, so any demo
		 * calling Camera::enableMotionBlur() straight on the camera bypassed it entirely. */
		const bool wantDepthOfField = camera != nullptr && camera->isDepthOfFieldEnabled() && renderer.isDepthOfFieldAllowed();
		const bool wantMotionBlur = camera != nullptr && camera->isMotionBlurEnabled() && renderer.isMotionBlurAllowed();
		const bool wantBloom = camera != nullptr && camera->isBloomEnabled();
		const bool wantHDR = camera != nullptr && camera->isHDREnabled();

		const bool hasDepthOfField = m_cameraDepthOfField != nullptr;
		const bool hasMotionBlur = m_cameraMotionBlur != nullptr;
		const bool hasBloom = m_cameraGlare != nullptr;
		const bool hasHDR = m_cameraToneMapping != nullptr;

		if ( wantDepthOfField == hasDepthOfField && wantMotionBlur == hasMotionBlur && wantBloom == hasBloom && wantHDR == hasHDR )
		{
			return false;
		}

		const auto mainRenderTarget = renderer.mainRenderTarget();

		if ( mainRenderTarget == nullptr )
		{
			return false;
		}

		const auto & extent = mainRenderTarget->extent();

		/* NOTE: nothing to detach any more. The four photographic effects OWN their slots
		 * (DepthOfField, MotionBlur, Glare, ToneMapping, in that order by construction — optics,
		 * then exposure duration, then the glare scattered in the lens, then the sensor
		 * response), and the assignment at the end of this method is the whole placement. The
		 * erase-then-find_if-then-insert dance this replaced existed only because the chain was
		 * a flat vector in which their position had to be recomputed from the neighbours. */

		/* Depth of field materialization. */
		if ( wantDepthOfField && m_cameraDepthOfField == nullptr )
		{
			auto effect = std::make_shared< Effects::Camera::DepthOfField >(renderer);

			const auto created = effect->create(extent.width, extent.height);

			/* ⚠️ The photographic effects are created HERE, never by createAll(): their flag has
			 * to be raised on this path too, or the executor's "is it created" gate would skip
			 * the tone mapping and leave the frame in linear HDR. */
			effect->setCreatedFlag(created);

			if ( created )
			{
				m_cameraDepthOfField = std::move(effect);
			}
			else
			{
				TraceError{ClassId} << "Failed to materialize the camera depth of field effect !";
			}
		}
		else if ( !wantDepthOfField && m_cameraDepthOfField != nullptr )
		{
			/* Retire GPU resources once every in-flight frame is done with them. */
			renderer.deferredDestructor().retireAction([effect = std::move(m_cameraDepthOfField)] {
				effect->destroy();
			});

			m_cameraDepthOfField.reset();
		}

		/* Motion blur materialization. The smear happens DURING the exposure, on the image the
		 * optics have already formed — hence after the defocus — and before the glare scatters it
		 * and the sensor responds. Its length comes from the camera's shutter speed, so there is
		 * nothing photographic to pass here; the quality knobs are read from the settings by the
		 * effect itself. */
		if ( wantMotionBlur && m_cameraMotionBlur == nullptr )
		{
			auto effect = std::make_shared< Effects::Camera::MotionBlur >(renderer);

			const auto created = effect->create(extent.width, extent.height);

			/* ⚠️ The photographic effects are created HERE, never by createAll(): their flag has
			 * to be raised on this path too, or the executor's "is it created" gate would skip
			 * the tone mapping and leave the frame in linear HDR. */
			effect->setCreatedFlag(created);

			if ( created )
			{
				m_cameraMotionBlur = std::move(effect);
			}
			else
			{
				TraceError{ClassId} << "Failed to materialize the camera motion blur effect !";
			}
		}
		else if ( !wantMotionBlur && m_cameraMotionBlur != nullptr )
		{
			renderer.deferredDestructor().retireAction([effect = std::move(m_cameraMotionBlur)] {
				effect->destroy();
			});

			m_cameraMotionBlur.reset();
		}

		/* Lens glare materialization. Veiling glare is scattering INSIDE the lens, so it applies
		 * to the image the optics have already formed — after the defocus, before the sensor. */
		if ( wantBloom && m_cameraGlare == nullptr )
		{
			auto effect = std::make_shared< Effects::Camera::VeilingGlare >(renderer, Effects::Camera::VeilingGlare::Parameters{
				.threshold = camera->bloomThreshold(),
				.intensity = camera->bloomIntensity()
			});

			const auto created = effect->create(extent.width, extent.height);

			/* ⚠️ The photographic effects are created HERE, never by createAll(): their flag has
			 * to be raised on this path too, or the executor's "is it created" gate would skip
			 * the tone mapping and leave the frame in linear HDR. */
			effect->setCreatedFlag(created);

			if ( created )
			{
				m_cameraGlare = std::move(effect);
			}
			else
			{
				TraceError{ClassId} << "Failed to materialize the camera bloom effect !";
			}
		}
		else if ( !wantBloom && m_cameraGlare != nullptr )
		{
			renderer.deferredDestructor().retireAction([effect = std::move(m_cameraGlare)] {
				effect->destroy();
			});

			m_cameraGlare.reset();
		}

		/* HDR (tone mapping) materialization. The tone mapping OWNS the bloom application
		 * (folded composite): its pipeline variant is baked at create() time, so a bloom
		 * (de)materialization above forces the tone mapping to rebuild with/without the
		 * glare sampler. Rare event (a camera toggling its glare), and the auto-exposure
		 * re-adapts within a second. */
		const bool bloomPresenceChanged = wantBloom != hasBloom;

		if ( m_cameraToneMapping != nullptr && ( !wantHDR || bloomPresenceChanged ) )
		{
			renderer.deferredDestructor().retireAction([effect = std::move(m_cameraToneMapping)] {
				effect->destroy();
			});

			m_cameraToneMapping.reset();

			/* The glare loses its consumer: restore the bloom's own composite pass. */
			if ( !wantHDR && m_cameraGlare != nullptr )
			{
				std::static_pointer_cast< Effects::Camera::VeilingGlare >(m_cameraGlare)->setCompositeBypassed(false);
			}
		}

		if ( wantHDR && m_cameraToneMapping == nullptr )
		{
			auto effect = std::make_shared< Effects::Camera::ToneMapping >(renderer);

			/* Pair the camera glare with its consumer: the tone mapping samples the bloom
			 * chain directly and the bloom skips its own full-res composite pass. */
			if ( m_cameraGlare != nullptr )
			{
				auto bloom = std::static_pointer_cast< Effects::Camera::VeilingGlare >(m_cameraGlare);

				effect->setGlareSource(bloom);
				bloom->setCompositeBypassed(true);
			}

			const auto created = effect->create(extent.width, extent.height);

			/* ⚠️ The photographic effects are created HERE, never by createAll(): their flag has
			 * to be raised on this path too, or the executor's "is it created" gate would skip
			 * the tone mapping and leave the frame in linear HDR. */
			effect->setCreatedFlag(created);

			if ( created )
			{
				m_cameraToneMapping = std::move(effect);
			}
			else
			{
				TraceError{ClassId} << "Failed to materialize the camera tone mapping effect !";
			}
		}

		/* Publish the surviving photographic effects into their own slots. Their place in the
		 * chain — after every scene effect, before anything display-referred — is the slot
		 * order itself, so there is no position left to compute. */
		const auto publish = [this] (EffectSlot slot, const std::shared_ptr< IndirectPostProcessEffect > & effect) {
			auto & occupants = m_slots[static_cast< size_t >(slot)];

			for ( const auto & previous : occupants )
			{
				if ( previous != nullptr && previous != effect )
				{
					previous->setOwnerStack(nullptr);
				}
			}

			occupants.clear();

			if ( effect != nullptr )
			{
				effect->setOwnerStack(this);

				occupants.emplace_back(effect);
			}
		};

		publish(EffectSlot::DepthOfField, m_cameraDepthOfField);
		publish(EffectSlot::MotionBlur, m_cameraMotionBlur);
		publish(EffectSlot::Glare, m_cameraGlare);
		publish(EffectSlot::ToneMapping, m_cameraToneMapping);

		this->rebuildOrderedEffects();

		return true;
	}

	std::shared_ptr< Effects::Camera::ToneMapping >
	PostProcessStack::cameraToneMapping () const noexcept
	{
		/* NOTE: m_cameraToneMapping is only ever assigned a ToneMapping (materialized above). */
		return std::static_pointer_cast< Effects::Camera::ToneMapping >(m_cameraToneMapping);
	}

	std::shared_ptr< Effects::Camera::DepthOfField >
	PostProcessStack::cameraDepthOfField () const noexcept
	{
		/* NOTE: m_cameraDepthOfField is only ever assigned a DepthOfField (materialized above). */
		return std::static_pointer_cast< Effects::Camera::DepthOfField >(m_cameraDepthOfField);
	}

	bool
	PostProcessStack::createAll (uint32_t width, uint32_t height) const noexcept
	{
		auto success = true;

		/* ⚠️ SELECTED occupants only, and this is the whole of the lazy residency: a slot holds
		 * both lanes of its concept, and the one nobody asked for must not allocate a render
		 * target, a pipeline or a descriptor set. Creating everything here — which this method
		 * did until Sep 2026 — defeated the residency at scene load, before a single frame had
		 * run. The alternative is materialized by syncSlotSelection() the frame it is selected. */
		for ( size_t index = 0; index < EffectSlotCount; ++index )
		{
			const auto slot = static_cast< EffectSlot >(index);

			if ( !isChainSlot(slot) )
			{
				continue;
			}

			for ( const auto & effect : m_slots[index] )
			{
				if ( effect == nullptr )
				{
					continue;
				}

				/* The Custom slot is the only multi-occupant one: everything filed there runs,
				 * so everything filed there is created. */
				if ( !isMultiOccupantSlot(slot) && effect != this->selectedOccupant(slot) )
				{
					continue;
				}

				const auto created = effect->create(width, height);

				/* ⚠️ The flag is what keeps a failed effect OUT of the recording. The loop no
				 * longer returns on the first failure either: stopping there left the untouched
				 * effects un-created too, and they were recorded exactly like the one that
				 * failed. */
				effect->setCreatedFlag(created);

				if ( !created )
				{
					/* Latched, so the per-frame sync does not retry it forever, and so the slot
					 * falls back to the sibling lane instead of going dark. */
					effect->setCreationFailedFlag();

					TraceError{ClassId} << "Failed to create the '" << effect->label() << "' effect of the post-process stack !";

					success = false;
				}
			}
		}

		return success;
	}

	void
	PostProcessStack::destroyAll () const noexcept
	{
		for ( const auto & effect : m_orderedEffects )
		{
			/* ⚠️ isCreated() is not a micro-optimisation here: since the lazy residency a stack
			 * routinely holds occupants that were NEVER created, and an effect's destroy()
			 * releases members it only ever assigned in create(). */
			if ( effect != nullptr && effect->isCreated() )
			{
				effect->destroy();
				effect->setCreatedFlag(false);
			}
		}
	}

	bool
	PostProcessStack::resizeAll (uint32_t width, uint32_t height) const noexcept
	{
		auto success = true;

		for ( const auto & effect : m_orderedEffects )
		{
			if ( effect == nullptr )
			{
				continue;
			}

			/* A new extent is new information: a creation that failed at the previous size gets
			 * a fresh chance, on this occupant and on the alternative nobody has selected yet. */
			effect->clearCreationFailure();

			/* ⚠️⚠️ An occupant that was never created must NOT be resized, and this is the
			 * sharpest trap of the lazy residency: resize() DESTROYS then CREATES, and the line
			 * below raises the created flag on its result — so a resident alternative nobody
			 * ever selected would quietly MATERIALIZE ITSELF at the first window resize, and the
			 * whole laziness would evaporate on a gesture that has nothing to do with it. */
			if ( !effect->isCreated() )
			{
				continue;
			}

			/* ⚠️ resize() DESTROYS then creates: a failure here leaves the effect in the chain
			 * with its resources already gone, which is the second way an un-created effect used
			 * to reach the recording. */
			const auto resized = effect->resize(width, height);

			effect->setCreatedFlag(resized);

			if ( !resized )
			{
				/* Latched like a failed creation: the effect has no resources any more, and
				 * retrying the same size every frame would only repeat the failure. */
				effect->setCreationFailedFlag();

				TraceError{ClassId} << "Failed to resize the '" << effect->label() << "' effect of the post-process stack !";

				success = false;
			}
		}

		return success;
	}

	bool
	PostProcessStack::requiresHDR () const noexcept
	{
		return std::ranges::any_of(m_orderedEffects, [] (const auto & effect) {
			return effect != nullptr && effect->requiresHDR();
		});
	}

	bool
	PostProcessStack::requiresDepth () const noexcept
	{
		return std::ranges::any_of(m_orderedEffects, [] (const auto & effect) {
			return effect != nullptr && effect->requiresDepth();
		});
	}

	bool
	PostProcessStack::requiresNormals () const noexcept
	{
		return std::ranges::any_of(m_orderedEffects, [] (const auto & effect) {
			return effect != nullptr && effect->requiresNormals();
		});
	}

	bool
	PostProcessStack::requiresMaterialProperties () const noexcept
	{
		return std::ranges::any_of(m_orderedEffects, [] (const auto & effect) {
			return effect != nullptr && effect->requiresMaterialProperties();
		});
	}

	bool
	PostProcessStack::requiresAlbedo () const noexcept
	{
		return std::ranges::any_of(m_orderedEffects, [] (const auto & effect) {
			return effect != nullptr && effect->requiresAlbedo();
		});
	}

	bool
	PostProcessStack::requiresVelocity () const noexcept
	{
		return std::ranges::any_of(m_orderedEffects, [] (const auto & effect) {
			return effect != nullptr && effect->requiresVelocity();
		});
	}

	bool
	PostProcessStack::requiresLightSet () const noexcept
	{
		return std::ranges::any_of(m_orderedEffects, [] (const auto & effect) {
			return effect != nullptr && effect->requiresLightSet();
		});
	}

	bool
	PostProcessStack::requiresJitter () const noexcept
	{
		return std::ranges::any_of(m_orderedEffects, [] (const auto & effect) {
			return effect != nullptr && effect->requiresJitter();
		});
	}
}
