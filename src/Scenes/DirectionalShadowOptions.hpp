/*
 * src/Scenes/DirectionalShadowOptions.hpp
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

#include "emeraude_export.hpp"

/* STL inclusions. */
#include <cstdint>
#include <memory>
#include <string>

/* Local inclusions for usages. */
#include "Component/DirectionalLight.hpp"

namespace EmEn::Scenes
{
	/**
	 * @brief Shadow-mapping policy for a directional light the engine DERIVES from data.
	 * @note Two paths derive directional lights from data instead of a demo building them by
	 * hand: the celestial bodies of a background manifest (Scene::applyBackgroundLighting())
	 * and the lights an asset declares (SceneDataConsumer::setDirectionalLightShadows()).
	 * Neither source can say how its light should be shadowed — a shadow map is a RUNTIME
	 * BUDGET decision, never photometry — so both take this one struct: one notion, one type,
	 * one dispatch (build()). Default: no shadow mapping.
	 * @see EmEn::Scenes::BackgroundLightingOptions
	 * @see EmEn::Scenes::SceneDataConsumer::setDirectionalLightShadows()
	 */
	struct EMEN_API DirectionalShadowOptions
	{
		/** @brief Shadow map resolution. 0 = no shadow mapping. */
		uint32_t shadowMapResolution{0};

		/** @brief Cascade count. 0 = classic shadow map; 1-4 = cascaded shadow mapping (CSM). */
		uint32_t cascadeCount{0};

		/** @brief Classic shadow map coverage — a HALF-extent — in world units. Ignored with CSM. */
		float shadowCoverage{100.0F};

		/** @brief CSM split factor (0 = linear, 1 = logarithmic). Ignored with a classic map. */
		float cascadeLambda{0.5F};

		/**
		 * @brief CSM coverage zoom, feeding DirectionalLight's csmScale. Ignored with a classic map.
		 * @note 1 = the cascades span the WHOLE camera frustum; 4 = a quarter of it, and so on.
		 * ⚠️ This is the knob that decides whether CSM shadows are VISIBLE at all, and leaving it at
		 * 1 is almost never right: the covered depth is the camera's view distance divided by this
		 * value, split across at most 4 cascades. On a 500 m view distance at scale 1 the last
		 * cascade spans hundreds of metres, so an ordinary caster is sub-texel — the map holds its
		 * shadow, the screen shows nothing, and the mode looks broken while it is merely
		 * mis-budgeted. Pick it from the scene: covered depth = viewDistance / cascadeScale should
		 * be about the distance at which shadows still matter to the eye.
		 */
		float cascadeScale{1.0F};

		/**
		 * @brief Builds a directional light component on an entity under this policy.
		 * @note THE single dispatch between the three DirectionalLight constructors (no shadow,
		 * classic map, CSM). The two derivation paths call it, so a policy means the same thing
		 * whether the light comes from a sky manifest or from an asset.
		 * @tparam entity_t The entity type owning the component (Node or StaticEntity).
		 * @tparam setup_t A callable taking a Component::DirectionalLight & (color, illuminance, ...).
		 * @param entity A reference to the entity that will own the light component.
		 * @param componentName The component name.
		 * @param setup The setup function, run after construction and before linking.
		 * @return std::shared_ptr< Component::DirectionalLight > Null when the entity is full.
		 */
		template< typename entity_t, typename setup_t >
		[[nodiscard]]
		std::shared_ptr< Component::DirectionalLight >
		build (entity_t & entity, const std::string & componentName, const setup_t & setup) const noexcept
		{
			if ( shadowMapResolution == 0 )
			{
				return entity.template componentBuilder< Component::DirectionalLight >(componentName).setup(setup).build();
			}

			if ( cascadeCount > 0 )
			{
				return entity.template componentBuilder< Component::DirectionalLight >(componentName).setup(setup).build(shadowMapResolution, cascadeCount, cascadeLambda, cascadeScale);
			}

			return entity.template componentBuilder< Component::DirectionalLight >(componentName).setup(setup).build(shadowMapResolution, shadowCoverage);
		}
	};
}
