/*
 * src/Scenes/Effect/VignettingLensEffect.cpp
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

#include "VignettingLensEffect.hpp"

/* STL inclusions. */
#include <cmath>

/* Local inclusions. */
#include "Resources/Manager.hpp"
#include "Graphics/ImageResource.hpp"
#include "Graphics/PostProcessor.hpp"
#include "Saphir/Generator/SceneRendering.hpp"
#include "Saphir/FragmentShader.hpp"
#include "Saphir/Code.hpp"
#include "Vulkan/Image.hpp"

namespace EmEn::Scenes
{
	using namespace EmEn::Libs;
	using namespace Saphir;
	using namespace Saphir::Keys;
	using namespace Graphics;

	VignettingLensEffect::VignettingLensEffect (float strength) noexcept
		: m_image(Resources::Manager::instance()->container< ImageResource >()->getResource("Miscellaneous/_vignetting", true))
		/*, m_texture(Image::Usage::Texture, true)*/,
		m_strength{strength}
	{
		//if ( !m_texture.initialize(m_image->data()) )
		//	Tracer::error(ClassId, "Unable to get Vignetting texture !");
	}

	bool
	VignettingLensEffect::generateFragmentShaderCode (Saphir::Generator::Abstract & generator, Saphir::FragmentShader & fragmentShader) const noexcept
	{
		//fragmentShader.declare(Declaration::Uniform{Declaration::VariableType::Sampler2D, VignettingTexture});
		//fragmentShader.declare(Declaration::Uniform{Declaration::VariableType::Float, Strength, "1.0"});

		fragmentShader.declare(Declaration::StageInput{generator.getNextShaderVariableLocation(), GLSL::FloatVector2, ShaderVariable::Primary2DTextureCoordinates, GLSL::Smooth});

		fragmentShader.addComment("Vignetting effect.");

		Code{fragmentShader} <<
			"cont vec4 cColor = texture(" << VignettingTexture << ", " << ShaderVariable::Primary2DTextureCoordinates << ");" << Line::End <<
			"vColor.rgb = clamp(((vColor.rgb - 0.5) * max(" << Strength << ", 0.0)) + 0.5, 0.0, 1.0);" << Line::End <<
			PostProcessor::Fragment << " = vec4(" << PostProcessor::Fragment << ".rgb * vColor.rgb, 1.0);";

		return true;
	}

	bool
	VignettingLensEffect::requestScreenCoordinates () const noexcept
	{
		return true;
	}

	void
	VignettingLensEffect::setStrength (float value) noexcept
	{
		m_strength = std::abs(value);
	}

	float
	VignettingLensEffect::strength () const noexcept
	{
		return m_strength;
	}
}
