/*
 * src/Graphics/Geometry/VertexResource.cpp
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

#include "VertexResource.hpp"

/* Local inclusions. */
#include "Libs/VertexFactory/ShapeGenerator.hpp"
#include "Libs/VertexFactory/FileIO.hpp"
#include "Vulkan/TransferManager.hpp"

/* Defining the resource manager class id. */
template<>
const char * const EmEn::Resources::Container< EmEn::Graphics::Geometry::VertexResource >::ClassId{"VertexContainer"};

/* Defining the resource manager ClassUID. */
template<>
const size_t EmEn::Resources::Container< EmEn::Graphics::Geometry::VertexResource >::ClassUID{getClassUID(ClassId)};

namespace EmEn::Graphics::Geometry
{
	using namespace EmEn::Libs;
	using namespace EmEn::Libs::Math;
	using namespace EmEn::Libs::VertexFactory;
	using namespace EmEn::Libs::PixelFactory;
	using namespace EmEn::Vulkan;

	const size_t VertexResource::ClassUID{getClassUID(ClassId)};

	bool
	VertexResource::createOnHardware (TransferManager & transferManager) noexcept
	{
		if ( this->isCreated() )
		{
			Tracer::warning(ClassId, "The buffers are already in video memory ! Use processLogics() instead.");

			return true;
		}

		/* Checking local data ... */
		if ( !m_localData.isValid() )
		{
			TraceError{ClassId} <<
				"Resource '" << this->name() << "' has invalid local data ! "
				"Loading into video memory cancelled.";

			return false;
		}

		if ( !Interface::buildSubGeometries(m_subGeometries, m_localData) )
		{
			TraceError{ClassId} << "Resource '" << this->name() << "' fails to build sub-geometries !";

			return false;
		}

		/* Create the vertex buffer local data. */
		std::vector< float > vertexAttributes;

		const auto vertexElementCount = m_localData.createVertexBuffer(
			vertexAttributes,
			this->getNormalsFormat(),
			this->getPrimaryTextureCoordinatesFormat(),
			this->vertexColorEnabled() ? VertexColorType::RGBA : VertexColorType::None
		);

		TraceDebug{ClassId} <<
			"Buffer statistics." "\n"
			"Vertex count : " << m_localData.vertexCount() << "\n"
			"Vertex buffer (VBO) size : " << vertexAttributes.size() << "\n"
			"Vertex element count : " << vertexElementCount;

		if ( vertexAttributes.empty() || vertexElementCount == 0 )
		{
			Tracer::error(ClassId, "Buffers creation fails !");

			return false;
		}

		/* Create hardware buffers from local data. */
		return this->createVideoMemoryBuffers(transferManager, vertexAttributes, m_localData.vertexCount(), vertexElementCount);
	}

	bool
	VertexResource::createVideoMemoryBuffers (TransferManager & transferManager, const std::vector< float > & vertexAttributes, uint32_t vertexCount, uint32_t vertexElementCount) noexcept
	{
		m_vertexBufferObject = std::make_unique< VertexBufferObject >(transferManager.device(), vertexCount, vertexElementCount);
		m_vertexBufferObject->setIdentifier(this->name() + "-VBO-VertexBufferObject");

		if ( !m_vertexBufferObject->create(transferManager, vertexAttributes) )
		{
			Tracer::error(ClassId, "Unable to create the vertex buffer object (VBO) !");

			m_vertexBufferObject.reset();

			return false;
		}

		return true;
	}

	bool
	VertexResource::updateVideoMemory () noexcept
	{
		if ( !this->isCreated() )
		{
			Tracer::warning(ClassId, "No buffer in video memory to update !");

			return false;
		}

		std::vector< float > vertexAttributes{};

		m_localData.createVertexBuffer(
			vertexAttributes,
			this->getNormalsFormat(),
			this->getPrimaryTextureCoordinatesFormat(),
			this->vertexColorEnabled() ? VertexColorType::RGBA : VertexColorType::None
		);

		Tracer::error(ClassId, "Updating geometry in video memory is not handled yet !");

		// TODO

		return false;
	}

	void
	VertexResource::destroyFromHardware (bool clearLocalData) noexcept
	{
		if ( m_vertexBufferObject != nullptr )
		{
			m_vertexBufferObject->destroyFromHardware();
			m_vertexBufferObject.reset();
		}

		if ( clearLocalData )
		{
			this->resetFlags();
			m_localData.clear();
			m_subGeometries.clear();
		}
	}

	bool
	VertexResource::load () noexcept
	{
		if ( !this->beginLoading() )
		{
			return false;
		}

		ShapeBuilderOptions< float > options{};
		options.enableGlobalVertexColor(Red);

		m_localData = ShapeGenerator::generateCuboid(1.0F, 1.0F, 1.0F, options);

		return this->setLoadSuccess(true);
	}

	bool
	VertexResource::load (const std::filesystem::path & filepath) noexcept
	{
		if ( !this->beginLoading() )
		{
			return false;
		}

		if ( !VertexFactory::FileIO::read(filepath, m_localData) )
		{
			TraceError{ClassId} << "Unable to load geometry from '" << filepath << "' !";

			return this->setLoadSuccess(false);
		}

		/* FIXME: Should set that at reading step. */
		m_localData.flipYAxis();

		return this->setLoadSuccess(true);
	}

	bool
	VertexResource::load (const Json::Value & /*data*/) noexcept
	{
		if ( !this->beginLoading() )
		{
			return false;
		}

		Tracer::warning(ClassId, "FIXME: This function is not available yet !");

		return this->setLoadSuccess(false);
	}

	bool
	VertexResource::load (const Shape< float > & shape) noexcept
	{
		if ( !this->beginLoading() )
		{
			return false;
		}

		if ( !shape.isValid() )
		{
			Tracer::error(ClassId, "The base geometry is not usable ! Abort loading ...");

			return this->setLoadSuccess(false);
		}

		m_localData = shape;

		return this->setLoadSuccess(true);
	}
}
