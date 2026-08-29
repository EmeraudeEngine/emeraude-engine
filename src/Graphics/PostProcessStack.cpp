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

/* Local inclusions. */
#include "Effects/Framebuffer/Bloom.hpp"
#include "Effects/Framebuffer/DepthOfField.hpp"
#include "Effects/Framebuffer/MotionBlur.hpp"
#include "Effects/Framebuffer/ToneMapping.hpp"
#include "IndirectPostProcessEffect.hpp"
#include "Renderer.hpp"
#include "Scenes/Component/Camera.hpp"
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
	PostProcessStack::addEffect (std::shared_ptr< IndirectPostProcessEffect > effect) noexcept
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

		auto & occupants = m_slots[static_cast< size_t >(effect->slot())];

		if ( std::erase(occupants, effect) > 0 )
		{
			effect->setOwnerStack(nullptr);

			this->rebuildOrderedEffects();
		}
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

		this->rebuildOrderedEffects();
	}

	void
	PostProcessStack::disableSlotSiblings (const IndirectPostProcessEffect & effect) noexcept
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

	bool
	PostProcessStack::syncCameraEffects (const Scenes::Component::Camera * camera, Renderer & renderer) noexcept
	{
		const bool wantDepthOfField = camera != nullptr && camera->isDepthOfFieldEnabled();
		const bool wantMotionBlur = camera != nullptr && camera->isMotionBlurEnabled();
		const bool wantBloom = camera != nullptr && camera->isBloomEnabled();
		const bool wantHDR = camera != nullptr && camera->isHDREnabled();

		const bool hasDepthOfField = m_cameraDepthOfField != nullptr;
		const bool hasMotionBlur = m_cameraMotionBlur != nullptr;
		const bool hasBloom = m_cameraBloom != nullptr;
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
			auto effect = std::make_shared< Effects::Framebuffer::DepthOfField >(renderer);

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
			renderer.deferredDestructor().retireAction([effect = std::move(m_cameraDepthOfField)] () {
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
			auto effect = std::make_shared< Effects::Framebuffer::MotionBlur >(renderer);

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
			renderer.deferredDestructor().retireAction([effect = std::move(m_cameraMotionBlur)] () {
				effect->destroy();
			});

			m_cameraMotionBlur.reset();
		}

		/* Lens glare materialization. Veiling glare is scattering INSIDE the lens, so it applies
		 * to the image the optics have already formed — after the defocus, before the sensor. */
		if ( wantBloom && m_cameraBloom == nullptr )
		{
			auto effect = std::make_shared< Effects::Framebuffer::Bloom >(renderer, Effects::Framebuffer::Bloom::Parameters{
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
				m_cameraBloom = std::move(effect);
			}
			else
			{
				TraceError{ClassId} << "Failed to materialize the camera bloom effect !";
			}
		}
		else if ( !wantBloom && m_cameraBloom != nullptr )
		{
			renderer.deferredDestructor().retireAction([effect = std::move(m_cameraBloom)] () {
				effect->destroy();
			});

			m_cameraBloom.reset();
		}

		/* HDR (tone mapping) materialization. The tone mapping OWNS the bloom application
		 * (folded composite): its pipeline variant is baked at create() time, so a bloom
		 * (de)materialization above forces the tone mapping to rebuild with/without the
		 * glare sampler. Rare event (a camera toggling its glare), and the auto-exposure
		 * re-adapts within a second. */
		const bool bloomPresenceChanged = wantBloom != hasBloom;

		if ( m_cameraToneMapping != nullptr && ( !wantHDR || bloomPresenceChanged ) )
		{
			renderer.deferredDestructor().retireAction([effect = std::move(m_cameraToneMapping)] () {
				effect->destroy();
			});

			m_cameraToneMapping.reset();

			/* The glare loses its consumer: restore the bloom's own composite pass. */
			if ( !wantHDR && m_cameraBloom != nullptr )
			{
				std::static_pointer_cast< Effects::Framebuffer::Bloom >(m_cameraBloom)->setCompositeBypassed(false);
			}
		}

		if ( wantHDR && m_cameraToneMapping == nullptr )
		{
			auto effect = std::make_shared< Effects::Framebuffer::ToneMapping >(renderer);

			/* Pair the camera glare with its consumer: the tone mapping samples the bloom
			 * chain directly and the bloom skips its own full-res composite pass. */
			if ( m_cameraBloom != nullptr )
			{
				auto bloom = std::static_pointer_cast< Effects::Framebuffer::Bloom >(m_cameraBloom);

				effect->setBloomSource(bloom);
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
		publish(EffectSlot::Glare, m_cameraBloom);
		publish(EffectSlot::ToneMapping, m_cameraToneMapping);

		this->rebuildOrderedEffects();

		return true;
	}

	std::shared_ptr< Effects::Framebuffer::ToneMapping >
	PostProcessStack::cameraToneMapping () const noexcept
	{
		/* NOTE: m_cameraToneMapping is only ever assigned a ToneMapping (materialized above). */
		return std::static_pointer_cast< Effects::Framebuffer::ToneMapping >(m_cameraToneMapping);
	}

	std::shared_ptr< Effects::Framebuffer::DepthOfField >
	PostProcessStack::cameraDepthOfField () const noexcept
	{
		/* NOTE: m_cameraDepthOfField is only ever assigned a DepthOfField (materialized above). */
		return std::static_pointer_cast< Effects::Framebuffer::DepthOfField >(m_cameraDepthOfField);
	}

	bool
	PostProcessStack::createAll (uint32_t width, uint32_t height) const noexcept
	{
		auto success = true;

		for ( const auto & effect : m_orderedEffects )
		{
			if ( effect == nullptr )
			{
				continue;
			}

			const auto created = effect->create(width, height);

			/* ⚠️ The flag is what keeps a failed effect OUT of the recording. The loop no longer
			 * returns on the first failure either: stopping there left the untouched effects
			 * un-created too, and they were recorded exactly like the one that failed. */
			effect->setCreatedFlag(created);

			if ( !created )
			{
				TraceError{ClassId} << "Failed to create the '" << effect->label() << "' effect of the post-process stack !";

				success = false;
			}
		}

		return success;
	}

	void
	PostProcessStack::destroyAll () const noexcept
	{
		for ( const auto & effect : m_orderedEffects )
		{
			if ( effect != nullptr )
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

			/* ⚠️ resize() DESTROYS then creates: a failure here leaves the effect in the chain
			 * with its resources already gone, which is the second way an un-created effect used
			 * to reach the recording. */
			const auto resized = effect->resize(width, height);

			effect->setCreatedFlag(resized);

			if ( !resized )
			{
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
