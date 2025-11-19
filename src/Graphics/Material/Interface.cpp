/*
 * src/Graphics/Material/Interface.cpp
 * This file is part of Emeraude-Engine
 *
 * Copyright (C) 2010-2025 - Sébastien Léon Claude Christian Bémelmans "LondNoir" <londnoir@gmail.com>
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

#include "Interface.hpp"

/* Local inclusions. */
#include "Libs/FastJSON.hpp"
#include "Saphir/Declaration/UniformBlock.hpp"
#include "Graphics/Renderer.hpp"
#include "Helpers.hpp"
#include "Tracer.hpp"

namespace EmEn::Graphics::Material
{
	using namespace Libs;

	constexpr auto TracerTag{"MaterialInterface"};

	Renderer * Interface::s_graphicsRenderer{nullptr};

	bool
	Interface::onDependenciesLoaded () noexcept
	{
		if ( s_graphicsRenderer == nullptr )
		{
			Tracer::error(TracerTag, "The static renderer pointer is null !");

			return false;
		}

		if ( this->isCreated() )
		{
			TraceWarning{TracerTag} << "The material resource '" << this->name() << "' is already created !";

			return true;
		}

		if ( !this->create(*s_graphicsRenderer) )
		{
			TraceError{TracerTag} << "Unable to load the material resource '" << this->name() << "' into the GPU!";

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
		if ( auto sharedUniformBuffer = renderer.sharedUBOManager().getSharedUniformBuffer(identifier); sharedUniformBuffer != nullptr )
		{
			return sharedUniformBuffer;
		}

		const auto size = this->getUniformBlock(0, 0).bytes();

		return renderer.sharedUBOManager().createSharedUniformBuffer(identifier, size);
	}
}
