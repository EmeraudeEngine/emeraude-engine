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
#include <ranges>

/* Local inclusions. */
#include "Effects/Framebuffer/DepthOfField.hpp"
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
	}

	void
	PostProcessStack::addEffect (std::shared_ptr< IndirectPostProcessEffect > effect) noexcept
	{
		if ( effect != nullptr )
		{
			m_effects.emplace_back(std::move(effect));
		}
	}

	void
	PostProcessStack::removeEffect (const std::shared_ptr< IndirectPostProcessEffect > & effect) noexcept
	{
		std::erase(m_effects, effect);
	}

	void
	PostProcessStack::clearEffects () noexcept
	{
		m_effects.clear();
	}

	bool
	PostProcessStack::syncCameraEffects (const Scenes::Component::Camera * camera, Renderer & renderer) noexcept
	{
		const bool wantDepthOfField = camera != nullptr && camera->isDepthOfFieldEnabled();
		const bool wantHDR = camera != nullptr && camera->isHDREnabled();

		const bool hasDepthOfField = m_cameraDepthOfField != nullptr;
		const bool hasHDR = m_cameraToneMapping != nullptr;

		if ( wantDepthOfField == hasDepthOfField && wantHDR == hasHDR )
		{
			return false;
		}

		const auto mainRenderTarget = renderer.mainRenderTarget();

		if ( mainRenderTarget == nullptr )
		{
			return false;
		}

		const auto & extent = mainRenderTarget->extent();

		/* Detach the current camera effects: they are re-appended below in canonical
		 * order (DepthOfField, then ToneMapping LAST — HDR resolve closes the chain). */
		if ( m_cameraDepthOfField != nullptr )
		{
			std::erase(m_effects, m_cameraDepthOfField);
		}

		if ( m_cameraToneMapping != nullptr )
		{
			std::erase(m_effects, m_cameraToneMapping);
		}

		/* Depth of field materialization. */
		if ( wantDepthOfField && m_cameraDepthOfField == nullptr )
		{
			auto effect = std::make_shared< Effects::Framebuffer::DepthOfField >(renderer);

			if ( effect->create(extent.width, extent.height) )
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

		/* HDR (tone mapping) materialization. */
		if ( wantHDR && m_cameraToneMapping == nullptr )
		{
			auto effect = std::make_shared< Effects::Framebuffer::ToneMapping >(renderer);

			if ( effect->create(extent.width, extent.height) )
			{
				m_cameraToneMapping = std::move(effect);
			}
			else
			{
				TraceError{ClassId} << "Failed to materialize the camera tone mapping effect !";
			}
		}
		else if ( !wantHDR && m_cameraToneMapping != nullptr )
		{
			renderer.deferredDestructor().retireAction([effect = std::move(m_cameraToneMapping)] () {
				effect->destroy();
			});

			m_cameraToneMapping.reset();
		}

		/* Re-insert the surviving camera effects after the scene (HDR) effects but BEFORE
		 * the first post-tonemap (LDR) effect: antialiasing/sharpening operate on
		 * display-referred values and misbehave on linear HDR input. */
		auto insertIt = std::find_if(m_effects.begin(), m_effects.end(), [] (const auto & effect) {
			return effect != nullptr && effect->runsAfterToneMapping();
		});

		if ( m_cameraDepthOfField != nullptr )
		{
			insertIt = std::next(m_effects.insert(insertIt, m_cameraDepthOfField));
		}

		if ( m_cameraToneMapping != nullptr )
		{
			m_effects.insert(insertIt, m_cameraToneMapping);
		}

		return true;
	}

	bool
	PostProcessStack::createAll (uint32_t width, uint32_t height) const noexcept
	{
		for ( const auto & effect : m_effects )
		{
			if ( effect == nullptr )
			{
				continue;
			}

			if ( !effect->create(width, height) )
			{
				TraceError{ClassId} << "Failed to create effect in the post-process stack !";

				return false;
			}
		}

		return true;
	}

	void
	PostProcessStack::destroyAll () const noexcept
	{
		for ( auto & effect : m_effects )
		{
			if ( effect != nullptr )
			{
				effect->destroy();
			}
		}
	}

	bool
	PostProcessStack::resizeAll (uint32_t width, uint32_t height) const noexcept
	{
		for ( const auto & effect : m_effects )
		{
			if ( effect == nullptr )
			{
				continue;
			}

			if ( !effect->resize(width, height) )
			{
				TraceError{ClassId} << "Failed to resize effect in the post-process stack !";

				return false;
			}
		}

		return true;
	}

	bool
	PostProcessStack::requiresHDR () const noexcept
	{
		return std::ranges::any_of(m_effects, [] (const auto & effect) {
			return effect != nullptr && effect->isEnabled() && effect->requiresHDR();
		});
	}

	bool
	PostProcessStack::requiresDepth () const noexcept
	{
		return std::ranges::any_of(m_effects, [] (const auto & effect) {
			return effect != nullptr && effect->isEnabled() && effect->requiresDepth();
		});
	}

	bool
	PostProcessStack::requiresNormals () const noexcept
	{
		return std::ranges::any_of(m_effects, [] (const auto & effect) {
			return effect != nullptr && effect->isEnabled() && effect->requiresNormals();
		});
	}

	bool
	PostProcessStack::requiresMaterialProperties () const noexcept
	{
		return std::ranges::any_of(m_effects, [] (const auto & effect) {
			return effect != nullptr && effect->isEnabled() && effect->requiresMaterialProperties();
		});
	}

	bool
	PostProcessStack::requiresAlbedo () const noexcept
	{
		return std::ranges::any_of(m_effects, [] (const auto & effect) {
			return effect != nullptr && effect->isEnabled() && effect->requiresAlbedo();
		});
	}

	bool
	PostProcessStack::requiresVelocity () const noexcept
	{
		return std::ranges::any_of(m_effects, [] (const auto & effect) {
			return effect != nullptr && effect->isEnabled() && effect->requiresVelocity();
		});
	}

	bool
	PostProcessStack::requiresLightSet () const noexcept
	{
		return std::ranges::any_of(m_effects, [] (const auto & effect) {
			return effect != nullptr && effect->isEnabled() && effect->requiresLightSet();
		});
	}
}
