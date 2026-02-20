/*
 * src/Scenes/LensEffects/Dithering.cpp
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

#include "Dithering.hpp"

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
	Dithering::generateFragmentShaderCode (Generator::Abstract & /*generator*/, FragmentShader & fragmentShader) const noexcept
	{
		fragmentShader.addComment("Ordered Bayer dithering effect.");

		switch ( m_matrixSize )
		{
			case 2:
			{
				/* 2x2 Bayer matrix. */
				Code{fragmentShader} <<
					"int ditX = int(mod(gl_FragCoord.x, 2.0));" << Line::End <<
					"int ditY = int(mod(gl_FragCoord.y, 2.0));" << Line::Blank <<

					"const float bayer2[4] = float[4](" << Line::End <<
					"    0.0 / 4.0, 2.0 / 4.0," << Line::End <<
					"    3.0 / 4.0, 1.0 / 4.0" << Line::End <<
					");" << Line::Blank <<

					"float ditThreshold = bayer2[ditY * 2 + ditX];" << Line::End <<
					PostProcessor::Fragment << ".rgb += (ditThreshold - 0.5) * " << m_intensity << ";";
				break;
			}

			case 8:
			{
				/* 8x8 Bayer matrix. */
				Code{fragmentShader} <<
					"int ditX = int(mod(gl_FragCoord.x, 8.0));" << Line::End <<
					"int ditY = int(mod(gl_FragCoord.y, 8.0));" << Line::Blank <<

					"const float bayer8[64] = float[64](" << Line::End <<
					"     0.0/64.0, 32.0/64.0,  8.0/64.0, 40.0/64.0,  2.0/64.0, 34.0/64.0, 10.0/64.0, 42.0/64.0," << Line::End <<
					"    48.0/64.0, 16.0/64.0, 56.0/64.0, 24.0/64.0, 50.0/64.0, 18.0/64.0, 58.0/64.0, 26.0/64.0," << Line::End <<
					"    12.0/64.0, 44.0/64.0,  4.0/64.0, 36.0/64.0, 14.0/64.0, 46.0/64.0,  6.0/64.0, 38.0/64.0," << Line::End <<
					"    60.0/64.0, 28.0/64.0, 52.0/64.0, 20.0/64.0, 62.0/64.0, 30.0/64.0, 54.0/64.0, 22.0/64.0," << Line::End <<
					"     3.0/64.0, 35.0/64.0, 11.0/64.0, 43.0/64.0,  1.0/64.0, 33.0/64.0,  9.0/64.0, 41.0/64.0," << Line::End <<
					"    51.0/64.0, 19.0/64.0, 59.0/64.0, 27.0/64.0, 49.0/64.0, 17.0/64.0, 57.0/64.0, 25.0/64.0," << Line::End <<
					"    15.0/64.0, 47.0/64.0,  7.0/64.0, 39.0/64.0, 13.0/64.0, 45.0/64.0,  5.0/64.0, 37.0/64.0," << Line::End <<
					"    63.0/64.0, 31.0/64.0, 55.0/64.0, 23.0/64.0, 61.0/64.0, 29.0/64.0, 53.0/64.0, 21.0/64.0" << Line::End <<
					");" << Line::Blank <<

					"float ditThreshold = bayer8[ditY * 8 + ditX];" << Line::End <<
					PostProcessor::Fragment << ".rgb += (ditThreshold - 0.5) * " << m_intensity << ";";
				break;
			}

			default:
			{
				/* 4x4 Bayer matrix (default). */
				Code{fragmentShader} <<
					"int ditX = int(mod(gl_FragCoord.x, 4.0));" << Line::End <<
					"int ditY = int(mod(gl_FragCoord.y, 4.0));" << Line::Blank <<

					"const float bayer4[16] = float[16](" << Line::End <<
					"     0.0/16.0,  8.0/16.0,  2.0/16.0, 10.0/16.0," << Line::End <<
					"    12.0/16.0,  4.0/16.0, 14.0/16.0,  6.0/16.0," << Line::End <<
					"     3.0/16.0, 11.0/16.0,  1.0/16.0,  9.0/16.0," << Line::End <<
					"    15.0/16.0,  7.0/16.0, 13.0/16.0,  5.0/16.0" << Line::End <<
					");" << Line::Blank <<

					"float ditThreshold = bayer4[ditY * 4 + ditX];" << Line::End <<
					PostProcessor::Fragment << ".rgb += (ditThreshold - 0.5) * " << m_intensity << ";";
				break;
			}
		}

		return true;
	}

	void
	Dithering::setMatrixSize (int size) noexcept
	{
		if ( size != 2 && size != 4 && size != 8 )
		{
			Tracer::warning(ClassId, "Matrix size must be 2, 4, or 8 !");

			return;
		}

		m_matrixSize = size;
	}
}
