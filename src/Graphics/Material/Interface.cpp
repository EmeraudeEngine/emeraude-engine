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
		auto sharedUniformBuffer = renderer.sharedUBOManager().getSharedUniformBuffer(identifier);

		if ( sharedUniformBuffer != nullptr )
		{
			return sharedUniformBuffer;
		}

		const auto size = this->getUniformBlock(0, 0).bytes();

		return renderer.sharedUBOManager().createSharedUniformBuffer(identifier, size);
	}

	bool
	Interface::onDependenciesLoaded () noexcept
	{
		if ( s_graphicsRenderer == nullptr )
		{
			TraceError{TracerTag} << "The static renderer pointer is null !";

			return false;
		}

		if ( !this->isCreated() && !this->createOnHardware(*s_graphicsRenderer) )
		{
			TraceError{TracerTag} << "Unable to load material resource (" << this->classLabel() << ") '" << this->name() << "' !";

			return false;
		}

		this->onMaterialLoaded();

		return true;
	}
}
