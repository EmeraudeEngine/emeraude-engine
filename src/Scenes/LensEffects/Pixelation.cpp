/*
 * src/Scenes/LensEffects/Pixelation.cpp
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

#include "Pixelation.hpp"

/* Local inclusions. */
#include "Graphics/PostProcessor.hpp"
#include "Saphir/Generator/SceneRendering.hpp"
#include "Saphir/FragmentShader.hpp"
#include "Saphir/Code.hpp"
#include "Saphir/Keys.hpp"

namespace EmEn::Scenes::LensEffects
{
	using namespace Saphir;
	using namespace Saphir::Keys;
	using namespace Graphics;

	bool
	Pixelation::generateFragmentShaderCode (Generator::Abstract & /*generator*/, FragmentShader & fragmentShader) const noexcept
	{
		fragmentShader.addComment("Pixelation effect (low-resolution pixel blocks).");

		/* Snap UV coordinates to a grid defined by pixel size / screen resolution.
		 * Center sampling within each grid cell for correct block appearance. */
		Code{fragmentShader} <<
			"vec2 pixGrid = vec2(" << m_pixelSize << ") / " << PostProcessingPC(PushConstant::Component::FrameSize) << ";" << Line::End <<
			"vec2 pixUV = floor(" << ShaderVariable::Primary2DTextureCoordinates << " / pixGrid) * pixGrid + pixGrid * 0.5;" << Line::End <<
			PostProcessor::Fragment << " = texture(" << Uniform::PrimarySampler << ", pixUV);";

		return true;
	}

	void
	Pixelation::setPixelSize (float pixelSize) noexcept
	{
		if ( pixelSize <= 0.0F )
		{
			Tracer::warning(ClassId, "Pixel size must be > 0 !");

			return;
		}

		m_pixelSize = pixelSize;
	}
}
