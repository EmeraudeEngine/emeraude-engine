/*
 * src/Graphics/Geometry/IndexedVertexResource.cpp
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

#include "IndexedVertexResource.hpp"

/* Local inclusions. */
#include "Libs/VertexFactory/ShapeGenerator.hpp"
#include "Libs/VertexFactory/FileIO.hpp"
#include "Vulkan/TransferManager.hpp"

namespace EmEn::Graphics::Geometry
{
	using namespace Libs;
	using namespace Libs::Math;
	using namespace Libs::VertexFactory;
	using namespace Libs::PixelFactory;
	using namespace Libs::VertexFactory;
	using namespace Vulkan;

	bool
	IndexedVertexResource::createOnHardware (TransferManager & transferManager) noexcept
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

		/* Create the vertex and the index buffers the local data. */
		std::vector< float > vertexAttributes;
		std::vector< uint32_t > indices;

		const auto vertexElementCount = m_localData.createIndexedVertexBuffer(
			vertexAttributes,
			indices,
			this->getNormalsFormat(),
			this->getPrimaryTextureCoordinatesFormat(),
			this->vertexColorEnabled() ? VertexColorType::RGBA : VertexColorType::None
		);

		if ( vertexAttributes.empty() || indices.empty() || vertexElementCount == 0 )
		{
			TraceError{ClassId} << "Unable to create the vertex buffer and the index buffer for geometry '" << this->name() << "' !";

			return false;
		}

		/* Create hardware buffers from local data. */
		return this->createVideoMemoryBuffers(transferManager, vertexAttributes, m_localData.vertexCount(), vertexElementCount, indices);
	}

	bool
	IndexedVertexResource::createVideoMemoryBuffers (TransferManager & transferManager, const std::vector< float > & vertexAttributes, uint32_t vertexCount, uint32_t vertexElementCount, const std::vector< uint32_t > & indices) noexcept
	{
		m_vertexBufferObject = std::make_unique< VertexBufferObject >(transferManager.device(), vertexCount, vertexElementCount, false);
		m_vertexBufferObject->setIdentifier(ClassId, this->name(), "VertexBufferObject");

		if ( !m_vertexBufferObject->createOnHardware() || !m_vertexBufferObject->transferData(transferManager, vertexAttributes) )
		{
			Tracer::error(ClassId, "Unable to create the vertex buffer object (VBO) !");

			m_vertexBufferObject.reset();

			return false;
		}

		m_indexBufferObject = std::make_unique< IndexBufferObject >(transferManager.device(), static_cast< uint32_t >(indices.size()));
		m_indexBufferObject->setIdentifier(ClassId, this->name(), "IndexBufferObject");

		if ( !m_indexBufferObject->createOnHardware() || !m_indexBufferObject->transferData(transferManager, indices) )
		{
			Tracer::error(ClassId, "Unable to get an index buffer object (IBO) !");

			m_indexBufferObject.reset();

			return false;
		}

		return true;
	}

	bool
	IndexedVertexResource::updateVideoMemory () noexcept
	{
		if ( !this->isCreated() )
		{
			Tracer::warning(ClassId, "No buffer in video memory to update !");

			return false;
		}

		Tracer::error(ClassId, "Updating geometry in video memory is not handled yet !");

		return false;
	}

	void
	IndexedVertexResource::destroyFromHardware (bool clearLocalData) noexcept
	{
		if ( m_vertexBufferObject != nullptr )
		{
			m_vertexBufferObject->destroyFromHardware();
			m_vertexBufferObject.reset();
		}

		if ( m_indexBufferObject != nullptr )
		{
			m_indexBufferObject->destroyFromHardware();
			m_indexBufferObject.reset();
		}

		if ( clearLocalData )
		{
			this->resetFlags();
			m_localData.clear();
			m_subGeometries.clear();
		}
	}

	bool
	IndexedVertexResource::load (Resources::AbstractServiceProvider & /*serviceProvider*/) noexcept
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
	IndexedVertexResource::load (Resources::AbstractServiceProvider & /*serviceProvider*/, const std::filesystem::path & filepath) noexcept
	{
		if ( !this->beginLoading() )
		{
			return false;
		}

		/* FIXME: Find a way to declare those flags outside de the loading function. */
		this->enableFlag(EnableTangentSpace);
		this->enableFlag(EnablePrimaryTextureCoordinates);

		ReadOptions options{};
		options.flipYAxis = true;
		options.requestNormal = this->isFlagEnabled(EnableNormal);
		options.requestTangentSpace = this->isFlagEnabled(EnableTangentSpace);
		options.requestTextureCoordinates = this->isFlagEnabled(EnablePrimaryTextureCoordinates) || this->isFlagEnabled(EnableSecondaryTextureCoordinates);
		options.requestVertexColor = this->isFlagEnabled(EnableVertexColor);

		if ( !VertexFactory::FileIO::read(filepath, m_localData, options) )
		{
			TraceError{ClassId} << "Unable to load geometry from '" << filepath << "' !";

			return this->setLoadSuccess(false);
		}

		return this->setLoadSuccess(true);
	}

	bool
	IndexedVertexResource::load (Resources::AbstractServiceProvider & /*serviceProvider*/, const Json::Value & /*data*/) noexcept
	{
		if ( !this->beginLoading() )
		{
			return false;
		}

		Tracer::warning(ClassId, "FIXME: This function is not yet available !");

		return this->setLoadSuccess(false);
	}

	bool
	IndexedVertexResource::load (const Shape< float > & shape) noexcept
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
