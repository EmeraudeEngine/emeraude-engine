/*
 * src/Graphics/EffectSlot.hpp
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
#include <cstddef>
#include <cstdint>

namespace EmEn::Graphics
{
	/**
	 * @brief The ordered CONCEPTS a framebuffer post-process effect can occupy.
	 * @note ⚠️⚠️ THE DECLARATION ORDER OF THIS ENUM IS THE RENDER ORDER OF THE CHAIN, and it is
	 * the ONLY thing that decides it. `PostProcessStack` stores its effects per slot and walks
	 * the slots in this order, so the order in which an application ADDS its effects has no
	 * effect whatsoever. This replaces an insertion-ordered vector where the chain order was a
	 * property of the call sequence — twelve scenes each restating it by hand, three of them
	 * differently, and no way to be wrong loudly.
	 * @note A slot is a CONCEPT, not a class: `RTR` and `SSR` are two implementations of
	 * `Reflections` and can never both be live, which the stack now enforces instead of
	 * documenting.
	 */
	enum class EffectSlot : uint8_t
	{
		/**
		 * @brief NOT a chain member: a reusable COMPONENT owned by an effect.
		 * @note `GIDenoiser`, `DenoisePass` and `CombinePass` inherit the effect base for its
		 * pipeline helpers (descriptor layouts, per-frame sets, fullscreen pipelines) while
		 * being owned BY an effect, never filed into a stack. They answer this so the base can
		 * keep `slot()` pure virtual — an effect that forgets to state where it belongs must not
		 * compile — and `PostProcessStack::addEffect()` refuses this slot loudly.
		 */
		Internal,

		/* ---- Scene effects (linear HDR), declared by the application ---- */

		/**
		 * @brief Indirect diffuse light: RTGI or SSGI.
		 * @note FIRST, and this is a correctness requirement rather than a preference: the
		 * reflection slot below SAMPLES THE CHAIN COLOUR to fetch what a reflected ray sees
		 * (`readsChainColorUpstream()`), so the indirect diffuse must already be composited or
		 * every screen-space reflection shows an unlit world. Since the indirect-diffuse
		 * OWNERSHIP contract (Aug 2026) an enabled RTGI also switches the raster's own ambient
		 * IBL leg off, so a reflection sampled before it would carry no sky light at all.
		 */
		IndirectDiffuse,

		/**
		 * @brief Scene reflections: RTR or SSR.
		 * @note Reads the chain colour — everything that lights the surfaces it reflects must
		 * come before it.
		 */
		Reflections,

		/**
		 * @brief Ambient occlusion: RTAO or SSAO.
		 * @note LAST of the three indirect terms, and the position is load-bearing in BOTH
		 * directions. The members of a combine group emit their snippets into ONE generated pass
		 * in slot order, and this one is a global multiply (`em_Color.rgb *= ao`): everything
		 * BEFORE it is attenuated, everything after is not.
		 * ⚠️ Placed before the indirect diffuse it occluded the DIRECT light and left the GI it
		 * exists to occlude untouched. Placed before the reflections it stopped attenuating them,
		 * and traced reflections came out at full strength inside creases and occluded corners —
		 * big bright patches, owner-reported on Sponza the day the slots landed. Here it catches
		 * the direct light, the indirect diffuse AND the reflections, which is what a global
		 * occlusion multiplier is for (same choice as UE4).
		 */
		AmbientOcclusion,

		/** @brief Fine-detail depth-derived shadowing (ContactShadows). */
		ContactShadows,

		/** @brief Participating medium (AtmosphericFog). */
		Fog,

		/**
		 * @brief Light shafts (VolumetricLight).
		 * @note Order-insensitive by construction: its combine snippet is a pure add and its
		 * passes never sample the chain — the shafts are built from the depth occlusion mask and
		 * the light alone. Its place here is a convention, not a constraint.
		 */
		VolumetricLight,

		/** @brief Lens flare from bright sources. Reads the chain colour for its bright pass. */
		LensFlare,

		/**
		 * @brief Application-defined effects with no engine concept.
		 * @note The ONLY slot that holds several effects at once, in their order of addition. An
		 * effect landing here is an extension point, not a mistake — but a concept the engine
		 * knows must NEVER be filed here, or the guarantees above stop applying to it.
		 */
		Custom,

		/**
		 * @brief Temporal anti-aliasing (TAA).
		 * @note Closes the scene phase: it resolves the fully composited HDR image against the
		 * previous frame, so anything that adds light must already have run.
		 */
		TemporalAA,

		/* ---- Photographic effects (HDR resolve), materialized by the ACTIVE CAMERA ---- */

		/**
		 * @brief Optical defocus (DepthOfField).
		 * @warning ⚠️ The four slots below belong to `PostProcessStack::syncCameraEffects()`,
		 * which mirrors the camera's own switches. An application NEVER adds them by hand: the
		 * stack owns their lifetime and rebuilds them when the camera changes.
		 */
		DepthOfField,

		/** @brief The smear of the exposure duration (MotionBlur), on the image the optics formed. */
		MotionBlur,

		/** @brief Veiling glare scattered INSIDE the lens (Bloom), before the sensor responds. */
		Glare,

		/** @brief The sensor response, HDR to display-referred (ToneMapping). Closes the chain. */
		ToneMapping,

		/* ---- Post-tone-mapping framebuffer effects (LDR, display-referred) ---- */

		/**
		 * @brief Framebuffer effects that MUST see display-referred values.
		 * @note Reserved: the LDR effects in service today (FXAA, Sharpen, FXAASharpen) are
		 * DIRECT effects and live in the stack's separate display list. A framebuffer effect
		 * that needs LDR input belongs here — running antialiasing or sharpening on linear HDR
		 * produces severe posterization and halo streaks (observed live on Sponza, Jul 2026).
		 */
		PostToneMapping
	};

	/** @brief The number of slots, hence the size of the stack's slot table. */
	constexpr auto EffectSlotCount{static_cast< size_t >(EffectSlot::PostToneMapping) + 1UL};

	/**
	 * @brief Returns whether a slot runs BEFORE the translucent grab-pass objects are drawn.
	 * @note ⚠️ THE FRAME IS CUT IN TWO around the TranslucentGB pass when the frame contains such
	 * objects (Renderer::renderFrameWithInternal). The indirect DIFFUSE must be composited into
	 * the scene colour BEFORE the material grab pass copies it, or nothing seen THROUGH a glass
	 * ever receives it — and since the indirect-diffuse ownership contract switched the raster's
	 * own IBL leg off under RTGI, "nothing" meant nothing at all (measured on the watch dial,
	 * Aug 2026). ONLY the indirect diffuse moves forward: the ambient occlusion must stay after
	 * the reflections (its snippet is a global multiply — placed before them it stopped
	 * attenuating them, the owner-reported bright patches), and the reflections stay after the
	 * translucent pass so a water surface keeps its SSR/RTR through its G-buffer footprint.
	 * @param slot The slot.
	 * @return bool
	 */
	[[nodiscard]]
	constexpr
	bool
	isPreTranslucencySlot (EffectSlot slot) noexcept
	{
		return slot == EffectSlot::IndirectDiffuse;
	}

	/**
	 * @brief Returns whether a slot may be filed into a stack at all.
	 * @param slot The slot.
	 * @return bool
	 */
	[[nodiscard]]
	constexpr
	bool
	isChainSlot (EffectSlot slot) noexcept
	{
		return slot != EffectSlot::Internal;
	}

	/**
	 * @brief Returns whether a slot belongs to the camera's photographic chain.
	 * @note Those slots are filled by PostProcessStack::syncCameraEffects() alone.
	 * @param slot The slot.
	 * @return bool
	 */
	[[nodiscard]]
	constexpr
	bool
	isCameraEffectSlot (EffectSlot slot) noexcept
	{
		return slot >= EffectSlot::DepthOfField && slot <= EffectSlot::ToneMapping;
	}

	/**
	 * @brief Returns whether a slot may hold SEVERAL effects at once.
	 * @note Every other slot holds alternatives of ONE concept, of which the stack keeps at most
	 * one enabled.
	 * @param slot The slot.
	 * @return bool
	 */
	[[nodiscard]]
	constexpr
	bool
	isMultiOccupantSlot (EffectSlot slot) noexcept
	{
		return slot == EffectSlot::Custom;
	}

	/**
	 * @brief Returns the name of a slot, for traces and profiling scopes.
	 * @param slot The slot.
	 * @return const char *
	 */
	[[nodiscard]]
	constexpr
	const char *
	to_cstring (EffectSlot slot) noexcept
	{
		switch ( slot )
		{
			case EffectSlot::Internal : return "Internal";
			case EffectSlot::IndirectDiffuse : return "IndirectDiffuse";
			case EffectSlot::AmbientOcclusion : return "AmbientOcclusion";
			case EffectSlot::Reflections : return "Reflections";
			case EffectSlot::ContactShadows : return "ContactShadows";
			case EffectSlot::Fog : return "Fog";
			case EffectSlot::VolumetricLight : return "VolumetricLight";
			case EffectSlot::LensFlare : return "LensFlare";
			case EffectSlot::Custom : return "Custom";
			case EffectSlot::TemporalAA : return "TemporalAA";
			case EffectSlot::DepthOfField : return "DepthOfField";
			case EffectSlot::MotionBlur : return "MotionBlur";
			case EffectSlot::Glare : return "Glare";
			case EffectSlot::ToneMapping : return "ToneMapping";
			case EffectSlot::PostToneMapping : return "PostToneMapping";
		}

		return "Unknown";
	}
}
