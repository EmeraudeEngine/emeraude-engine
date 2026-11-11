/*
 * src/Graphics/Material/Interface.cpp
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

#include "Interface.hpp"

/* STL inclusions. */
#include <optional>

/* Local inclusions. */
#include "BlobTrait.hpp"
#include "GPURTMaterialData.hpp"
#include "Graphics/Renderer.hpp"
#include "Graphics/SharedUBOManager.hpp"
#include "Helpers.hpp"
#include "Saphir/Declaration/UniformBlock.hpp"
#include "Tracer.hpp"

namespace EmEn::Graphics::Material
{
	using namespace Base;

	constexpr auto TracerTag{"MaterialInterface"};

	void
	Interface::exportRTMaterialData (GPURTMaterialData & outData) const noexcept
	{
		/* Default: gray dielectric with no textures. */
		outData = GPURTMaterialData{};
	}

	void
	Interface::collectRTTextures (std::vector< RTTextureSlot > & /*outSlots*/) const noexcept
	{
		/* Default: no textures. */
	}

	bool
	Interface::onDependenciesLoaded (const std::stop_token & stopToken) noexcept
	{
		if ( this->isCreated() )
		{
			TraceWarning{TracerTag} << "The material resource '" << this->name() << "' is already created !";

			return true;
		}

		/* Every dependency is loaded and nothing is on the GPU yet: the only window where a material
		 * may still change its flags after measuring a texture it depends on. */
		this->onBeforeCreation();

		if ( !this->create(this->serviceProvider().graphicsRenderer(), stopToken) )
		{
			/* NOTE: A creation the load's stop cut short is no error (the load ends abandoned, silently). */
			if ( !stopToken.stop_requested() )
			{
				TraceError{TracerTag} << "Unable to load the material resource '" << this->name() << "' into the GPU!";
			}

			return false;
		}

		this->enableFlag(IsCreated);

		return true;
	}

	void
	Interface::enableBlendingFromJson (const Json::Value & data) noexcept
	{
		const auto blendingMode = Material::getBlendingModeFromJSON(data);

		if ( blendingMode && blendingMode != BlendingMode::None )
		{
			this->enableBlending(blendingMode.value());
		}
	}

	std::shared_ptr< SharedUniformBuffer >
	Interface::getSharedUniformBuffer (Renderer & renderer, const std::string & identifier) const noexcept
	{
		/* NOTE: The identifier only encodes the material kind and its texture count, so distinct
		 * materials share it by design. As they are loaded concurrently from the resource thread pool,
		 * the buffer MUST be reached through a single atomic get-or-create: a get() then create()
		 * sequence loses the race for every material but one, and a material that cannot reach its
		 * shared uniform buffer fails to load entirely, removing its sub-meshes from the scene. */
		/* One region per frame in flight, each bound by its own descriptor set: before the renderer has its frames, a
		 * material would get one region and no set for the other frames (never drawn, silently). */
		const auto frameCount = renderer.framesInFlight();

		if ( frameCount == 0 )
		{
			TraceError{TracerTag} << "The material '" << this->name() << "' is created before the renderer has its frames in flight !";

			return nullptr;
		}

		return renderer.sharedUBOManager().getOrCreateSharedUniformBuffer(identifier, this->getUniformBlock(0, 0).bytes(), frameCount);
	}
}
