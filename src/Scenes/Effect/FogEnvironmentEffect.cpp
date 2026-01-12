/*
 * src/Scenes/Effect/FogEnvironmentEffect.cpp
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

#include "FogEnvironmentEffect.hpp"

/* Local inclusions. */
#include "Saphir/Code.hpp"
#include "Saphir/FragmentShader.hpp"
#include "Saphir/Generator/SceneRendering.hpp"
#include "Saphir/VertexShader.hpp"
#include "Tracer.hpp"

namespace EmEn::Scenes
{
	using namespace Libs;
	using namespace Saphir;
	using namespace Saphir::Keys;

	bool
	FogEnvironmentEffect::generateVertexShaderCode (Generator::Abstract & generator, VertexShader & vertexShader) const noexcept
	{
		/* Fog per vertex */
		if ( !m_highQuality )
		{
			vertexShader.declare(Declaration::StageOutput{generator.getNextShaderVariableLocation(), GLSL::Float, FogVertexDistance});

			vertexShader.addComment("Fog effect.");

			Code{vertexShader} << FogVertexDistance << " = length(" << Attribute::Position << ");";
		}

		return true;
	}

	bool
	FogEnvironmentEffect::generateFragmentShaderCode (Generator::Abstract & /*generator*/, FragmentShader & fragmentShader) const noexcept
	{
		//constexpr auto FogFactor{"fogFactor"};

		/* Adds color vector */
		if ( !m_useBackgroundColor )
		{
			const auto defaultValue = (std::stringstream{} << "vec4(" << m_color.red() << ", " << m_color.green() << ", " << m_color.blue() << ", " << m_color.alpha() << ")").str();

			//fragmentShader.declare(Declaration::Uniform{Declaration::VariableType::FloatVector4, FogColor, defaultValue.c_str()});
		}

		Declaration::Function function("get_fog_factor", GLSL::Float);
		function.addInParameter(GLSL::Float, "z", true);

		/* Adds fog function to program */
		switch ( m_mode )
		{
			/* Formula : fogFactor = (end - z) / (end - execute) */
			case FogMode::Linear :
				//fragmentShader.declare(Declaration::Uniform{Declaration::VariableType::Float, FogStart, "0.0"});
				//fragmentShader.declare(Declaration::Uniform{Declaration::VariableType::Float, FogEnd, "1.0"});

				Code{function} << "return clamp((" << FogEnd << " - z) / (" << FogEnd << " - " << FogStart << "), 0.0, 1.0);";
				break;

				/* Formula : fogFactor = e [exp] -(density * z) */
			case FogMode::Exponential :
				//fragmentShader.declare(Declaration::Uniform{Declaration::VariableType::Float, FogDensity, "1.0"});

				Code{function} << "return clamp(exp(-(z * " << FogDensity << ")), 0.0, 1.0);";
				break;

				/* Formula : fogFactor = e [exp] -(density * z)² */
			case FogMode::Exponential2 :
				//fragmentShader.declare(Declaration::Uniform{Declaration::VariableType::Float, FogDensity, "1.0"});

				Code{function} <<
					"float factor = z * " << FogDensity << ";" << Line::End <<
					"return clamp(exp(-(factor * factor)), 0.0, 1.0);";
				break;

			case FogMode::Exponential2Alt :
				//fragmentShader.declare(Declaration::Uniform{Declaration::VariableType::Float, FogDensity, "1.0"});

				Code{function} << "return clamp(exp2(-fogFactor * fogFactor * z * z * 1.442695), 0.0, 1.0);";
				break;
		}

		fragmentShader.declare(function);

		fragmentShader.addComment("Fog effect.");

		/* Fog per pixel */
		if ( m_highQuality )
		{
			Code{fragmentShader} << "float fogFactor = " << function.callCode("gl_FragCoord.z / gl_FragCoord.w") << ';';
		}
		/* Fog per vertex */
		else
		{
			Code{fragmentShader} << "float fogFactor = " << function.callCode(FogVertexDistance) << ';';
		}

		/* Final process */
		if ( m_useBackgroundColor )
		{
			Code{fragmentShader} << ShaderVariable::OutputFragment << " = vec4(" << ShaderVariable::OutputFragment << ".rgb, mix(0.0, 1.0, fogFactor));";
		}
		else
		{
			Code{fragmentShader} << ShaderVariable::OutputFragment << " = mix(" << FogColor << ", " << ShaderVariable::OutputFragment << ", fogFactor);";
		}

		return true;
	}

	FogEnvironmentEffect::FogEnvironmentEffect (FogMode mode, bool highQuality) noexcept
		: m_mode(mode), m_highQuality(highQuality)
	{

	}

	void
	FogEnvironmentEffect::setDensity (float density) noexcept
	{
		if ( density < 0.0F )
		{
			Tracer::warning(ClassId, "Fog intensity must be positive !");

			return;
		}

		if ( m_mode == FogMode::Linear )
		{
			Tracer::warning(ClassId, "Changing density doesn't affect linear fog.");
		}

		m_density = density;
	}

	void
	FogEnvironmentEffect::setLinearRange (float start, float end) noexcept
	{
		if ( m_mode != FogMode::Linear )
		{
			Tracer::warning(ClassId, "Changing range doesn't affect exponential fog.");
		}

		m_linearStart = std::min(start, end);
		m_linearEnd = std::max(start, end);
	}

	void
	FogEnvironmentEffect::setColor (const PixelFactory::Color< float > & color) noexcept
	{
		m_color = color;
		m_useBackgroundColor = false;
	}

	void
	FogEnvironmentEffect::useBackgroundColor () noexcept
	{
		m_useBackgroundColor = true;
	}

	FogEnvironmentEffect::FogMode
	FogEnvironmentEffect::mode () const noexcept
	{
		return m_mode;
	}

	float
	FogEnvironmentEffect::density () const noexcept
	{
		return m_density;
	}

	float
	FogEnvironmentEffect::linearStart () const noexcept
	{
		return m_linearStart;
	}

	float
	FogEnvironmentEffect::linearEnd () const noexcept
	{
		return m_linearEnd;
	}

	const PixelFactory::Color< float > &
	FogEnvironmentEffect::color () const noexcept
	{
		return m_color;
	}
}
