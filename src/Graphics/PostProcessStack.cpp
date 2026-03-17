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
 * https://github.com/londnoir/emeraude-engine
 *
 * --- THIS IS AUTOMATICALLY GENERATED, DO NOT CHANGE ---
 */

#include "PostProcessStack.hpp"

/* STL inclusions. */
#include <algorithm>

/* Local inclusions. */
#include "IndirectPostProcessEffect.hpp"
#include "Tracer.hpp"

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
	PostProcessStack::requiresLightSet () const noexcept
	{
		return std::ranges::any_of(m_effects, [] (const auto & effect) {
			return effect != nullptr && effect->isEnabled() && effect->requiresLightSet();
		});
	}
}
