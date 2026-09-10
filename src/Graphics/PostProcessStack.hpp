/*
 * src/Graphics/PostProcessStack.hpp
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
#include <atomic>
#include <cstdint>
#include <memory>
#include <string_view>
#include <vector>

/* Local inclusions for inheritance. */
#include "Console/ControllableTrait.hpp"

/* Local inclusions for usages. */
#include "DirectPostProcessEffect.hpp"
#include "EffectSlot.hpp"

namespace EmEn::Graphics
{
	class IndirectPostProcessEffect;
	class Renderer;
}

namespace EmEn::Scenes
{
	class LightSet;
}

namespace EmEn::Scenes::Component
{
	class Camera;
}

namespace EmEn::Graphics::Effects::Camera
{
	class DepthOfField;
	class ToneMapping;
}

namespace EmEn::Graphics
{
	/**
	 * @brief The two implementation LANES of the lighting concepts.
	 * @note A lane is not a new taxonomy: an effect belongs to the ray-traced lane if and only if
	 * `IndirectPostProcessEffect::requiresRayTracing()` is true, and to the screen-space lane
	 * otherwise. Nothing else distinguishes them, and nothing else may — a second way of asking
	 * the same question is a second way of getting a different answer.
	 */
	enum class LightingLane : uint8_t
	{
		ScreenSpace,
		RayTracing
	};

	/**
	 * @brief Returns the name of a lighting lane, for the console and the traces.
	 * @param lane The lane.
	 * @return const char *
	 */
	[[nodiscard]]
	constexpr
	const char *
	to_cstring (LightingLane lane) noexcept
	{
		switch ( lane )
		{
			case LightingLane::ScreenSpace : return "ScreenSpace";
			case LightingLane::RayTracing : return "RayTracing";
		}

		return "Unknown";
	}

	/**
	 * @brief The frame's post-process chain, as a fixed sequence of CONCEPTS rather than a list.
	 * @note ⚠️⚠️ THE CHAIN ORDER IS A PROPERTY OF THIS CLASS, NOT OF THE CALL SEQUENCE. Effects
	 * are filed into the slot they declare (`IndirectPostProcessEffect::slot()`) and the chain is
	 * walked in `EffectSlot` order, so an application may add its effects in any order at all and
	 * still get the canonical one. This replaced an insertion-ordered vector in which twelve
	 * scenes each restated the order by hand — three of them differently, and a wrong order was
	 * silent: an ambient occlusion added before the indirect diffuse occluded the direct light
	 * and left the GI untouched, and a screen-space reflection added first reflected a world
	 * with no indirect light in it at all.
	 * @note A slot holds AS MANY alternatives as the application builds — several RTGI and
	 * several SSGI with different Parameters, all resident so a runtime switch can compare them
	 * on the very same framing — of which the stack keeps AT MOST ONE ENABLED. Enabling one is
	 * selecting it; its siblings are disabled mechanically (see disableSlotSiblings()).
	 * @note Owned by a Scene to provide per-scene post-processing configuration.
	 */
	class EMEN_API PostProcessStack final : public Console::ControllableTrait
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"PostProcessStack"};

			/** @brief The console identifier, i.e. the node name under the scene manager. */
			static constexpr auto ConsoleId{"PostProcess"};

			/** @brief The selection index meaning "this concept is switched off". */
			static constexpr int8_t NoOccupant{-1};

			/**
			 * @brief How many consecutive frames a selected occupant must fail to run before its
			 * slot falls back to the sibling lane.
			 * @note ⚠️ A grace period, not a debounce, and it exists because the fallback and the
			 * lazy residency otherwise defeat each other. `Renderer::isRayTracingReady()` is false
			 * for the first frames of EVERY scene — the TLAS is built asynchronously — and it
			 * cannot say whether it is "not yet" or "never" (it is a null/created test on the
			 * current TLAS, nothing more). Falling back immediately therefore materialized the
			 * WHOLE screen-space lane on every launch of a ray-traced scene, seconds before the
			 * traced one took over: measured on Sponza, all three screen-space effects came up
			 * "created" while the traced lane was the one running. Waiting a couple of seconds
			 * lets a warm-up resolve itself for free, and still rescues a scene that genuinely
			 * holds no ray traced geometry.
			 */
			static constexpr uint16_t FallbackGraceFrames{120};

			PostProcessStack () noexcept
				: ControllableTrait{ConsoleId}
			{
				/* ⚠️ NOT a default member initializer: std::atomic value-initializes to 0 in
				 * C++20, and 0 is a valid occupant index. Every slot starts with NO selection. */
				for ( auto & selected : m_selectedOccupant )
				{
					selected.store(NoOccupant, std::memory_order_relaxed);
				}

				for ( auto & effective : m_effectiveOccupant )
				{
					effective.store(NoOccupant, std::memory_order_relaxed);
				}
			}

			~PostProcessStack () noexcept override;

			/* ⚠️ Non-copiable AND non-movable. It used to be movable, and both reasons it can no
			 * longer be are structural rather than stylistic: it is a console object, whose
			 * parent holds a RAW back-pointer to it (moving a registered one dangles that entry),
			 * and it holds the per-slot selection as std::atomic. Nothing ever moved one — a
			 * stack lives behind the std::unique_ptr the scene owns. */
			PostProcessStack (const PostProcessStack &) = delete;

			PostProcessStack & operator= (const PostProcessStack &) = delete;

			PostProcessStack (PostProcessStack &&) noexcept = delete;

			PostProcessStack & operator= (PostProcessStack &&) noexcept = delete;

			/**
			 * @brief Files an effect into the slot it declares.
			 * @note ⚠️ The ORDER OF THE CALLS IS IRRELEVANT — the effect's own `slot()` decides
			 * where it runs. Adding several occupants to one slot is legal and is how a runtime
			 * A/B works (all resident, one enabled); the newcomer becomes the enabled one if it
			 * is enabled, which every effect is at construction.
			 * @warning A camera slot (DepthOfField, MotionBlur, Glare, ToneMapping) is REFUSED
			 * with a trace error: those belong to syncCameraEffects(), which owns their lifetime.
			 * @param effect A shared pointer to the effect.
			 * @return void
			 */
			void addEffect (const std::shared_ptr< IndirectPostProcessEffect >& effect) noexcept;

			/**
			 * @brief Disables every OTHER occupant of an effect's slot.
			 * @warning ⚠️ INTERNAL — called by IndirectPostProcessEffect::enable() to make the
			 * exclusivity of a concept mechanical. Never call it directly: enable the effect you
			 * want and the siblings follow.
			 * @param effect A reference to the effect being enabled.
			 * @return void
			 */
			void disableSlotSiblings (const IndirectPostProcessEffect & effect) const noexcept;

			/**
			 * @brief Returns the occupants of a slot, in their order of addition.
			 * @param slot The slot.
			 * @return const std::vector< std::shared_ptr< IndirectPostProcessEffect > > &
			 */
			[[nodiscard]]
			const std::vector< std::shared_ptr< IndirectPostProcessEffect > > &
			slotEffects (EffectSlot slot) const noexcept
			{
				return m_slots[static_cast< size_t >(slot)];
			}

			/**
			 * @brief Returns the ENABLED occupant of a slot, or nullptr.
			 * @param slot The slot.
			 * @return std::shared_ptr< IndirectPostProcessEffect >
			 */
			[[nodiscard]]
			std::shared_ptr< IndirectPostProcessEffect > enabledEffect (EffectSlot slot) const noexcept;

			/**
			 * @brief Installs the complete LIGHTING family: both lanes, one selected.
			 * @note This is the engine's own policy, and the reason it lives here rather than in
			 * an application: "which effects implement the indirect diffuse, the reflections, the
			 * ambient occlusion and the contact shadows, and which of them can this machine run"
			 * is a question about the engine, not about a game. Every consumer asking it by hand
			 * would answer it differently — which is the mistake the EffectSlot table already
			 * fixed for the chain ORDER.
			 *
			 * The screen-space lane is ALWAYS resident. The ray-traced lane is resident only when
			 * the two SESSION-CONSTANT conditions hold: the device supports ray tracing and the
			 * user setting allows it. ⚠️ Deliberately NOT `Renderer::isRayTracingReady()`, which
			 * is a per-frame answer (the TLAS is built asynchronously, and a scene may hold no ray
			 * traced geometry at all): residency is a lifetime decision, readiness is a frame
			 * decision, and confusing the two would either allocate a lane that can never run or
			 * refuse one that becomes runnable three frames later.
			 * @note Residency is not creation. An occupant that is merely resident holds NO GPU
			 * resources until it is selected — see @ref syncSlotSelection().
			 * @note ⚠️ The two lanes are NOT interchangeable renderings of one result, and that is
			 * why both stay resident instead of one being picked once and for all. Taking the
			 * indirect diffuse as the example: SSGI and RTGI share their whole integration
			 * contract — same G-buffer inputs, same demodulated signal, same denoiser, same
			 * additive combine — but not what they can GATHER. RTGI *owns* the indirect diffuse:
			 * its miss branch integrates the sky cubemap with real visibility, so the raster
			 * ambient pass drops its own diffuse IBL leg. SSGI has no sky term at all, so it only
			 * ADDS on-screen bounces to a raster ambient that stays whole. Switching lanes
			 * therefore changes what the frame MEANS, not merely how fast it was obtained — see
			 * `Graphics/AGENTS.md` § "Indirect-diffuse OWNERSHIP".
			 * @param renderer A reference to the graphics renderer.
			 * @return void
			 */
			void installLightingFamily (Renderer & renderer) noexcept;

			/**
			 * @brief Selects the occupant of a slot BY NAME, as an intent.
			 * @note ⚠️ MAIN THREAD (console). This does NOT touch the chain: it records what the
			 * owner wants, and @ref syncSlotSelection() applies it on the render thread at the
			 * next frame boundary. Calling `enable()` from here instead is what the console used
			 * to have to do, and it was a data race against the chain walk.
			 * @param slot The slot.
			 * @param label The occupant's label, as reported by IndirectPostProcessEffect::label().
			 * @return bool False when the slot holds no occupant carrying that label.
			 */
			bool selectOccupant (EffectSlot slot, std::string_view label) noexcept;

			/**
			 * @brief Selects NO occupant for a slot — the concept is switched off.
			 * @note ⚠️ MAIN THREAD (console). Same deferred contract as @ref selectOccupant().
			 * @param slot The slot.
			 * @return void
			 */
			void selectNoOccupant (EffectSlot slot) noexcept;

			/**
			 * @brief Selects a whole LANE across every lighting slot.
			 * @note ⚠️ MAIN THREAD (console). Same deferred contract as @ref selectOccupant().
			 * A lighting slot with no occupant in the requested lane is left EMPTY rather than
			 * kept on the other lane: asking for the screen-space lane and silently getting a
			 * ray-traced contact shadow would make an A/B measurement meaningless. Since Sep 2026
			 * every lighting slot has an occupant in both lanes, so this case no longer arises for
			 * the family the engine installs — it still can for a slot an application populates
			 * itself.
			 * @warning ⚠️ REFUSED, atomically, when the lane has no occupant in ANY lighting slot:
			 * nothing is changed and false is returned. Applying it would switch the whole
			 * lighting family OFF while reporting that the lane was selected — which is what it
			 * did until Sep 2026 on a session started with `LightingLane = "ScreenSpace"` (no
			 * acceleration structure is built then, so the traced lane is not resident at all).
			 * A partial lane is still applied: the slots that have an occupant get it, the others
			 * go off, which is the documented behaviour above.
			 * @param lane The lane.
			 * @return bool
			 */
			bool selectLightingLane (LightingLane lane) noexcept;

			/**
			 * @brief Returns the occupant the owner SELECTED for a slot, or nullptr.
			 * @note This is the INTENT, which is not always what runs: read @ref enabledEffect()
			 * for what the chain will actually record, and compare the two to see a fallback.
			 * @param slot The slot.
			 * @return std::shared_ptr< IndirectPostProcessEffect >
			 */
			[[nodiscard]]
			std::shared_ptr< IndirectPostProcessEffect > selectedOccupant (EffectSlot slot) const noexcept;

			/**
			 * @brief Returns the occupant the LAST SYNC actually put in the chain, or nullptr.
			 * @note Recorded by @ref syncSlotSelection() rather than re-derived on the spot, and
			 * that is the point: it reports what the render thread DID, not what the reader's
			 * thread would decide now, on a ray-tracing readiness that may already have moved.
			 * Differing from @ref selectedOccupant() is exactly what a fallback looks like.
			 * @param slot The slot.
			 * @return std::shared_ptr< IndirectPostProcessEffect >
			 */
			[[nodiscard]]
			std::shared_ptr< IndirectPostProcessEffect > effectiveOccupant (EffectSlot slot) const noexcept;

			/**
			 * @brief Returns whether an occupant could be recorded in the frame about to start.
			 * @note The gates that DISCRIMINATE BETWEEN SIBLINGS of one slot, and only those: ray
			 * tracing availability, the light set, and the creation-failure latch. The G-buffer
			 * gates the executor also applies (depth, normals, albedo, ...) are deliberately absent
			 * because they cannot discriminate: PostProcessStack aggregates its G-buffer
			 * requirements over EVERY resident effect, selected or not, so the scene target always
			 * carries the union of what both lanes need. That single fact is what makes a lane
			 * switch free — no target recreation, no pipeline reconfiguration.
			 * @param effect A reference to the effect.
			 * @param renderer A reference to the graphics renderer.
			 * @param lightSet A pointer to the scene's light set, nullptr when there is none.
			 * @return bool
			 */
			[[nodiscard]]
			static bool canOccupantRun (const IndirectPostProcessEffect & effect, const Renderer & renderer, const Scenes::LightSet * lightSet) noexcept;

			/**
			 * @brief Applies the pending selection, materializing what it needs.
			 * @note ⚠️ RENDER THREAD, once per frame, BEFORE syncSlotPairings() and before any
			 * recording. It is the single place where a chain effect is enabled or disabled, which
			 * is what makes the console switch safe: the console writes an intent from the main
			 * thread, this reads it here.
			 *
			 * For each slot it resolves the EFFECTIVE occupant — the selected one when it can run,
			 * otherwise the first sibling that can — creates it if it is not created yet (lazy
			 * residency: an alternative costs nothing until it is asked for), and enables it, which
			 * disables its siblings mechanically.
			 * @warning ⚠️ Do NOT call PostProcessStack::createAll() to cover this. That call creates
			 * what is selected AT THAT MOMENT; the whole point here is the occupant nobody selected
			 * yet.
			 * @param renderer A reference to the graphics renderer.
			 * @param lightSet A pointer to the scene's light set, nullptr when there is none.
			 * @return void
			 */
			void syncSlotSelection (Renderer & renderer, const Scenes::LightSet * lightSet) noexcept;

			/**
			 * @brief Removes an effect from the chain.
			 * @param effect A shared pointer to the effect.
			 * @return void
			 */
			void removeEffect (const std::shared_ptr< IndirectPostProcessEffect > & effect) noexcept;

			/**
			 * @brief Synchronizes the CAMERA-DRIVEN photographic effects with the active camera.
			 * @note Physical camera contract: the camera declares its photographic behaviour
			 * (enableDepthOfField()/enableHDR()); this call (de)materializes the matching effects
			 * at the END of the chain, in canonical order (DepthOfField, then VeilingGlare, then ToneMapping last).
			 * Scene effects (GI, AO, fog...) added by the application are left untouched.
			 * Called by the Renderer once per frame, on the render thread; removed effects are
			 * retired through the deferred destructor (frames-in-flight safety).
			 * @param camera The scene's active camera (nullptr = no photographic effects).
			 * @param renderer A reference to the graphics renderer.
			 * @return bool Whether the effect set changed (the pipeline must be reconfigured).
			 */
			[[nodiscard]]
			bool syncCameraEffects (const Scenes::Component::Camera * camera, Renderer & renderer) noexcept;

			/**
			 * @brief Wires the producer/consumer pairings BETWEEN slots, once per frame.
			 * @note RENDER THREAD, before the chain is recorded. Idempotent and cheap (two slot
			 * lookups): it is called unconditionally rather than on a change, because the state
			 * it mirrors — which occupant of a slot is enabled, and whether it is created —
			 * changes without going through `addEffect()`/`removeEffect()`.
			 *
			 * Today it wires ONE pairing, the ambient-occlusion lane
			 * (@ref IndirectPostProcessEffect::providesOcclusionLane): the enabled
			 * `IndirectDiffuse` occupant reduces the occlusion in its own trace loop, for free,
			 * and the enabled `AmbientOcclusion` occupant reads it instead of re-casting the
			 * same rays. ⚠️ It also DISARMS both sides when the pairing is not possible, which is
			 * what keeps the consumer's standalone path alive and forbids a stale lane pointer
			 * from outliving a frame.
			 *
			 * ⚠️ The VeilingGlare → ToneMapping pairing does NOT live here: those two are camera-owned,
			 * materialized together by syncCameraEffects(), and their pairing is baked into the
			 * tone mapping's pipeline variant at create() time rather than refreshed per frame.
			 * @return void
			 */
			void syncSlotPairings () const noexcept;

			/**
			 * @brief Returns the camera-materialized tone mapping effect, or nullptr.
			 * @note For readers of its metered values (the overlay panel): RENDER THREAD only,
			 * inside the frame scope — the instance is (de)materialized by syncCameraEffects()
			 * on that same thread, once per frame.
			 * @return std::shared_ptr< Effects::Camera::ToneMapping >
			 */
			[[nodiscard]]
			std::shared_ptr< Effects::Camera::ToneMapping > cameraToneMapping () const noexcept;

			/**
			 * @brief Returns the camera-materialized depth of field effect, or nullptr.
			 * @note Same RENDER THREAD / frame scope contract as cameraToneMapping(); the panel
			 * reads its metered focus distance through this.
			 * @return std::shared_ptr< Effects::Camera::DepthOfField >
			 */
			[[nodiscard]]
			std::shared_ptr< Effects::Camera::DepthOfField > cameraDepthOfField () const noexcept;

			/**
			 * @brief Appends a DISPLAY effect, compiled into the final fullscreen pass.
			 * @note Display effects (anti-aliasing, sharpening) are display-referred and
			 * single-pass by nature: instead of paying a render pass + render target each,
			 * they generate GLSL into the final post-process shader, BEFORE the camera's
			 * lens effects (film grain or scanlines must not be sharpened). At most ONE
			 * fetch-overriding effect (FXAA, FXAASharpen) per stack.
			 * @param effect A shared pointer to the display effect.
			 * @return void
			 */
			void
			addDisplayEffect (std::shared_ptr< DirectPostProcessEffect > effect) noexcept
			{
				if ( effect != nullptr )
				{
					m_displayEffects.emplace_back(std::move(effect));
				}
			}

			/**
			 * @brief Returns the display effect list (final-pass compiled effects).
			 * @return const DirectEffectList &
			 */
			[[nodiscard]]
			const DirectEffectList &
			displayEffects () const noexcept
			{
				return m_displayEffects;
			}

			/**
			 * @brief Returns whether the stack has any display effects.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			hasDisplayEffects () const noexcept
			{
				return !m_displayEffects.empty();
			}

			/**
			 * @brief Returns whether the stack holds ANY effect, indirect or direct.
			 * @note The question an owner asks before creating and installing a stack: an empty
			 * one is not worth a render target, and a stack holding only DISPLAY effects is not
			 * empty — those are folded into the final swap-chain shader and would be dropped by
			 * the narrower @ref hasEffects().
			 * @return bool
			 */
			[[nodiscard]]
			bool
			hasAnyEffect () const noexcept
			{
				return this->hasEffects() || this->hasDisplayEffects();
			}

			/**
			 * @brief Clears the entire effect chain.
			 * @return void
			 */
			void clearEffects () noexcept;

			/**
			 * @brief Returns the effect chain.
			 * @return const std::vector< std::shared_ptr< IndirectPostProcessEffect > > &
			 */
			[[nodiscard]]
			const std::vector< std::shared_ptr< IndirectPostProcessEffect > > &
			effects () const noexcept
			{
				return m_orderedEffects;
			}

			/**
			 * @brief Returns whether the stack has any effects.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			hasEffects () const noexcept
			{
				return !m_orderedEffects.empty();
			}

			/**
			 * @brief Returns whether an ENABLED scene-reflection provider (SSR, RTR) is in
			 * the chain.
			 * @note Read by the Renderer to suspend the continuous reflection probes while a
			 * traced reflection covers the same job (reflection cost ladder).
			 * @return bool
			 */
			[[nodiscard]]
			bool hasEnabledReflectionProvider () const noexcept;

			/**
			 * @brief Returns whether an ENABLED indirect-diffuse provider (RTGI) is in the chain.
			 * @note Read by the scene to hand the diffuse IBL leg over to that effect — see
			 * PostProcessEffect::providesIndirectDiffuse() for the ownership contract.
			 * @return bool
			 */
			[[nodiscard]]
			bool hasEnabledIndirectDiffuseProvider () const noexcept;

			/**
			 * @brief Returns whether an ENABLED effect sits in a pre-translucency slot.
			 * @note Read by the Renderer to decide whether the frame is cut in two around the
			 * TranslucentGB pass (see EffectSlot::isPreTranslucencySlot()). A frame with no such
			 * effect, or no translucent grab-pass object, runs the whole chain once, as before.
			 * @return bool
			 */
			[[nodiscard]]
			bool hasEnabledPreTranslucencyEffect () const noexcept;

			/**
			 * @brief Creates GPU resources for all effects.
			 * @param width The framebuffer width.
			 * @param height The framebuffer height.
			 * @return bool
			 */
			[[nodiscard]]
			bool createAll (uint32_t width, uint32_t height) const noexcept;

			/**
			 * @brief Destroys GPU resources for all effects.
			 * @return void
			 */
			void destroyAll () const noexcept;

			/**
			 * @brief Recreates GPU resources after a resize.
			 * @param width The new framebuffer width.
			 * @param height The new framebuffer height.
			 * @return bool
			 */
			[[nodiscard]]
			bool resizeAll (uint32_t width, uint32_t height) const noexcept;

			/**
			 * @brief Returns whether any effect in the stack requires HDR input.
			 * @return bool
			 */
			[[nodiscard]]
			bool requiresHDR () const noexcept;

			/**
			 * @brief Returns whether any effect in the stack requires depth input.
			 * @return bool
			 */
			[[nodiscard]]
			bool requiresDepth () const noexcept;

			/**
			 * @brief Returns whether any effect in the stack requires normals input.
			 * @return bool
			 */
			[[nodiscard]]
			bool requiresNormals () const noexcept;

			/**
			 * @brief Returns whether any effect in the stack requires material properties input.
			 * @return bool
			 */
			[[nodiscard]]
			bool requiresMaterialProperties () const noexcept;

			/**
			 * @brief Returns whether any effect in the stack requires albedo input.
			 * @note Albedo implies normals and material properties (fixed MRT order).
			 * @return bool
			 */
			[[nodiscard]]
			bool requiresAlbedo () const noexcept;

			/**
			 * @brief Returns whether any effect in the stack requires the velocity buffer (motion vectors).
			 * @note Velocity implies the full MRT chain before it (fixed MRT order).
			 * @return bool
			 */
			[[nodiscard]]
			bool requiresVelocity () const noexcept;

			/**
			 * @brief Returns whether any effect in the stack requires the scene light set.
			 * @return bool
			 */
			[[nodiscard]]
			bool requiresLightSet () const noexcept;

			/**
			 * @brief Returns whether any effect in the stack requires the sub-pixel projection jitter (TAA).
			 * @note The Renderer polls this once per rendered frame to drive the Halton jitter sequence.
			 * @return bool
			 */
			[[nodiscard]]
			bool requiresJitter () const noexcept;

		private:

			/** @copydoc EmEn::Console::ControllableTrait::onRegisterToConsole. */
			void onRegisterToConsole () noexcept override;

			/**
			 * @brief Materializes an occupant's GPU resources, latching a failure.
			 * @note ⚠️ RENDER THREAD — syncSlotSelection() only.
			 * @param effect A reference to the effect.
			 * @param renderer A reference to the graphics renderer.
			 * @return bool
			 */
			[[nodiscard]]
			static bool materializeOccupant (IndirectPostProcessEffect & effect, Renderer & renderer) noexcept;

			/**
			 * @brief Disables every occupant of a slot — the concept is switched off.
			 * @note ⚠️ RENDER THREAD — syncSlotSelection() only.
			 * @param slot The slot.
			 * @return void
			 */
			void disableSlotOccupants (EffectSlot slot) const noexcept;

			/**
			 * @brief Rebuilds the flat, slot-ordered view the chain executor walks.
			 * @note Rebuilt on every mutation rather than assembled on demand: it is read once
			 * per frame and mutated a handful of times per scene.
			 * @return void
			 */
			void rebuildOrderedEffects () noexcept;

			/** @brief The occupants of each concept, indexed by EffectSlot. */
			std::array< std::vector< std::shared_ptr< IndirectPostProcessEffect > >, EffectSlotCount > m_slots;
			/** @brief The occupant the OWNER selected per slot, as an index into m_slots, -1 for none.
			 * @note Atomic because the two ends sit on two threads: the console writes it on the
			 * main thread, syncSlotSelection() reads it on the render thread. An index rather than
			 * a pointer so the write stays a single lock-free store, and removeEffect() is the one
			 * place that has to repair it. */
			std::array< std::atomic< int8_t >, EffectSlotCount > m_selectedOccupant;
			/** @brief The occupant the last sync actually enabled per slot, -1 for none.
			 * @note Written by the render thread, read by the console on the main thread — the
			 * mirror image of m_selectedOccupant, and the only honest way to report a fallback. */
			std::array< std::atomic< int8_t >, EffectSlotCount > m_effectiveOccupant;
			/** @brief Consecutive frames the selected occupant of a slot could not run.
			 * @note RENDER THREAD only — read and written by syncSlotSelection() alone, hence a
			 * plain integer where its two neighbours are atomic. */
			std::array< uint16_t, EffectSlotCount > m_selectionStallFrames{};
			/** @brief The slot table flattened in EffectSlot order — THE chain order. */
			std::vector< std::shared_ptr< IndirectPostProcessEffect > > m_orderedEffects;
			/* Display effects (AA, sharpening): no GPU resources of their own, they are
			 * compiled into the final fullscreen pass shader before the camera lens effects. */
			DirectEffectList m_displayEffects;
			/* Camera-driven photographic effects (physical camera contract). Kept aside to
			 * distinguish them from the application/scene effects inside m_effects. */
			std::shared_ptr< IndirectPostProcessEffect > m_cameraDepthOfField;
			std::shared_ptr< IndirectPostProcessEffect > m_cameraMotionBlur;
			std::shared_ptr< IndirectPostProcessEffect > m_cameraGlare;
			std::shared_ptr< IndirectPostProcessEffect > m_cameraToneMapping;
	};
}
