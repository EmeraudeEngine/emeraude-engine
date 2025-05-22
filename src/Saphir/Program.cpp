/*
 * src/Saphir/Program.cpp
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

#include "Program.hpp"

/* Local inclusions. */
#include "Graphics/Geometry/Interface.hpp"
#include "Tracer.hpp"

namespace EmEn::Saphir
{
	using namespace EmEn::Libs;
	using namespace Graphics;
	
	Program::Program (const std::string & name, std::string GLSLVersion, std::string GLSLProfile) noexcept
		: NameableTrait(name), m_GLSLVersion(std::move(GLSLVersion)), m_GLSLProfile(std::move(GLSLProfile))
	{

	}

	bool
	Program::isComplete () const noexcept
	{
		/* NOTE: The vertex shader is mandatory! */
		if ( m_vertexShader == nullptr || !m_vertexShader->isGenerated() )
		{
			return false;
		}

		/* NOTE: The tesselation shader is optional.
		 * But if a control shader is present, the evaluation must be too. */
		if ( m_tesselationControlShader != nullptr )
		{
			if ( !m_tesselationControlShader->isGenerated() )
			{
				return false;
			}

			if ( m_tesselationEvaluationShader == nullptr || !m_tesselationEvaluationShader->isGenerated() )
			{
				return false;
			}
		}

		/* NOTE: The geometry shader is optional. */
		if ( m_geometryShader != nullptr && !m_geometryShader->isGenerated() )
		{
			return false;
		}

		/* NOTE: The fragment shader is mandatory! */
		if ( m_fragmentShader == nullptr || !m_fragmentShader->isGenerated() )
		{
			return false;
		}

		return true;
	}

	bool
	Program::useTesselation () const noexcept
	{
		if ( m_tesselationControlShader == nullptr )
		{
			return false;
		}

		if ( m_tesselationEvaluationShader == nullptr )
		{
			TraceDebug{ClassId} << "A tesselation control shader was present on program '" << this->name() << "' but not the evaluation shader !";

			return false;
		}

		return true;
	}

	bool
	Program::wasInstancingEnabled () const noexcept
	{
		if ( m_vertexShader == nullptr )
		{
			Tracer::error(ClassId, "The vertex shader is uninitialized ! Unable to know if instancing was enabled ...");

			return false;
		}

		return m_vertexShader->isInstancingEnabled();
	}

	bool
	Program::wasAdvancedMatricesEnabled () const noexcept
	{
		if ( m_vertexShader == nullptr )
		{
			Tracer::error(ClassId, "The vertex shader is uninitialized ! Unable to know if advanced matrices were enabled ...");

			return false;
		}

		return m_vertexShader->isAdvancedMatricesEnabled();
	}

	bool
	Program::wasBillBoardingEnabled () const noexcept
	{
		if ( m_vertexShader == nullptr )
		{
			Tracer::error(ClassId, "The vertex shader is uninitialized ! Unable to know if bill boarding was enabled ...");

			return false;
		}

		return m_vertexShader->isBillBoardingEnabled();
	}

	VertexShader *
	Program::initVertexShader (const std::string & name, bool enableInstancing, bool enableAdvancedMatrices, bool enableBillBoarding) noexcept
	{
		if ( m_vertexShader != nullptr )
		{
			Tracer::error(ClassId, "The vertex shader is already initialized !");

			return nullptr;
		}

		m_vertexShader = std::make_unique< VertexShader >(name, m_GLSLVersion, m_GLSLProfile);

		if ( enableInstancing )
		{
			m_vertexShader->enableInstancing();
		}

		if ( enableAdvancedMatrices )
		{
			m_vertexShader->enableAdvancedMatrices();
		}

		if ( enableBillBoarding )
		{
			TraceInfo{ClassId} << "The vertex shader '" << name << "' declared for bill-boards rendering ...";

			m_vertexShader->enableBillBoarding();
		}

		return m_vertexShader.get();
	}

	TesselationControlShader *
	Program::initTesselationControlShader (const std::string & name) noexcept
	{
		if ( m_tesselationControlShader != nullptr )
		{
			Tracer::error(ClassId, "The tesselation control shader is already initialized !");

			return nullptr;
		}

		m_tesselationControlShader = std::make_unique< TesselationControlShader >(name, m_GLSLVersion, m_GLSLProfile);

		return m_tesselationControlShader.get();
	}

	TesselationEvaluationShader *
	Program::initTesselationEvaluationShader (const std::string & name) noexcept
	{
		if ( m_tesselationEvaluationShader != nullptr )
		{
			Tracer::error(ClassId, "The tesselation evaluation shader is already initialized !");

			return nullptr;
		}

		m_tesselationEvaluationShader = std::make_unique< TesselationEvaluationShader >(name, m_GLSLVersion, m_GLSLProfile);

		return m_tesselationEvaluationShader.get();
	}

	GeometryShader *
	Program::initGeometryShader (const std::string & name, const Declaration::InputPrimitive & inputPrimitive, const Declaration::OutputPrimitive & outputPrimitive) noexcept
	{
		if ( m_geometryShader != nullptr )
		{
			Tracer::error(ClassId, "The geometry shader is already initialized !");

			return nullptr;
		}

		m_geometryShader = std::make_unique< GeometryShader >(name, inputPrimitive, outputPrimitive, m_GLSLVersion, m_GLSLProfile);

		return m_geometryShader.get();
	}

	FragmentShader *
	Program::initFragmentShader (const std::string & name) noexcept
	{
		if ( m_fragmentShader != nullptr )
		{
			Tracer::error(ClassId, "The fragment shader is already initialized !");

			return nullptr;
		}

		m_fragmentShader = std::make_unique< FragmentShader >(name, m_GLSLVersion, m_GLSLProfile);

		return m_fragmentShader.get();
	}

	ShaderType
	Program::lastShaderStageType () const noexcept
	{
		if ( m_fragmentShader != nullptr && m_fragmentShader->isGenerated() )
		{
			return ShaderType::FragmentShader;
		}

		if ( m_geometryShader != nullptr && m_geometryShader->isGenerated() )
		{
			return ShaderType::GeometryShader;
		}

		if ( m_tesselationEvaluationShader != nullptr && m_tesselationEvaluationShader->isGenerated() )
		{
			return ShaderType::TesselationEvaluationShader;
		}

		if ( m_tesselationControlShader != nullptr && m_tesselationControlShader->isGenerated() )
		{
			return ShaderType::TesselationControlShader;
		}

		if ( m_vertexShader != nullptr && m_vertexShader->isGenerated() )
		{
			return ShaderType::VertexShader;
		}

		return ShaderType::Undefined;
	}

	const VertexShader *
	Program::vertexShader () const noexcept
	{
		if ( m_vertexShader == nullptr )
		{
			Tracer::warning(ClassId, "The vertex shader is not set !");

			return nullptr;
		}

		return m_vertexShader.get();
	}

	VertexShader *
	Program::vertexShader () noexcept
	{
		if ( m_vertexShader == nullptr )
		{
			Tracer::warning(ClassId, "The vertex shader is not set !");

			return nullptr;
		}

		return m_vertexShader.get();
	}

	const TesselationControlShader *
	Program::tesselationControlShader () const noexcept
	{
		if ( m_tesselationControlShader == nullptr )
		{
			Tracer::warning(ClassId, "The tesselation control shader is not set !");

			return nullptr;
		}

		return m_tesselationControlShader.get();
	}

	TesselationControlShader *
	Program::tesselationControlShader () noexcept
	{
		if ( m_tesselationControlShader == nullptr )
		{
			Tracer::warning(ClassId, "The tesselation control shader is not set !");

			return nullptr;
		}

		return m_tesselationControlShader.get();
	}

	const TesselationEvaluationShader *
	Program::tesselationEvaluationShader () const noexcept
	{
		if ( m_tesselationEvaluationShader == nullptr )
		{
			Tracer::warning(ClassId, "The tesselation evaluation shader is not set !");

			return nullptr;
		}

		return m_tesselationEvaluationShader.get();
	}

	TesselationEvaluationShader *
	Program::tesselationEvaluationShader () noexcept
	{
		if ( m_tesselationEvaluationShader == nullptr )
		{
			Tracer::warning(ClassId, "The tesselation evaluation shader is not set !");

			return nullptr;
		}

		return m_tesselationEvaluationShader.get();
	}

	const GeometryShader *
	Program::geometryShader () const noexcept
	{
		if ( m_geometryShader == nullptr )
		{
			Tracer::warning(ClassId, "The geometry shader is not set !");

			return nullptr;
		}

		return m_geometryShader.get();
	}

	GeometryShader *
	Program::geometryShader () noexcept
	{
		if ( m_geometryShader == nullptr )
		{
			Tracer::warning(ClassId, "The geometry shader is not set !");

			return nullptr;
		}

		return m_geometryShader.get();
	}

	const FragmentShader *
	Program::fragmentShader () const noexcept
	{
		if ( m_fragmentShader == nullptr )
		{
			Tracer::warning(ClassId, "The fragment shader is not set !");

			return nullptr;
		}

		return m_fragmentShader.get();
	}

	FragmentShader *
	Program::fragmentShader () noexcept
	{
		if ( m_fragmentShader == nullptr )
		{
			Tracer::warning(ClassId, "The fragment shader is not set !");

			return nullptr;
		}

		return m_fragmentShader.get();
	}

	std::vector< AbstractShader * >
	Program::getShaderList () const noexcept
	{
		std::vector< AbstractShader * > list{};
		list.reserve(5);;

		if ( m_vertexShader != nullptr )
		{
			list.emplace_back(m_vertexShader.get());
		}

		if ( m_tesselationControlShader != nullptr )
		{
			list.emplace_back(m_tesselationControlShader.get());
		}

		if ( m_tesselationEvaluationShader != nullptr )
		{
			list.emplace_back(m_tesselationEvaluationShader.get());
		}

		if ( m_geometryShader != nullptr )
		{
			list.emplace_back(m_geometryShader.get());
		}

		if ( m_fragmentShader != nullptr )
		{
			list.emplace_back(m_fragmentShader.get());
		}

		return list;
	}

	bool
	Program::createVertexBufferFormat (VertexBufferFormatManager & vertexBufferFormatManager, const Geometry::Interface * geometry) noexcept
	{
		if ( !this->hasVertexShader() )
		{
			Tracer::error(ClassId, "There is no vertex shader to build the vertex buffer format !");

			return false;
		}

		if ( geometry == nullptr )
		{
			Tracer::error(ClassId, "There is no geometry to build the vertex buffer format !");

			return false;
		}

		/* Prepare the vertex buffer format. */
		m_vertexBufferFormat = vertexBufferFormatManager.getVertexBufferFormat(*m_vertexShader, *geometry);

		if ( m_vertexBufferFormat == nullptr )
		{
			TraceError{ClassId} << "Unable to get a vertex buffer format for geometry '" << geometry->name() << "' !";

			return false;
		}

		return true;
	}

	bool
	Program::createVertexBufferFormat (VertexBufferFormatManager & vertexBufferFormatManager, Topology topology, uint32_t geometryFlagBits) noexcept
	{
		if ( !this->hasVertexShader() )
		{
			Tracer::error(ClassId, "There is no vertex shader to build the vertex buffer format !");

			return false;
		}

		/* Prepare the vertex buffer format. */
		m_vertexBufferFormat = vertexBufferFormatManager.getVertexBufferFormat(*m_vertexShader, topology, geometryFlagBits);

		if ( m_vertexBufferFormat == nullptr )
		{
			Tracer::error(ClassId, "Unable to get a vertex buffer format !");

			return false;
		}

		return true;
	}
}
