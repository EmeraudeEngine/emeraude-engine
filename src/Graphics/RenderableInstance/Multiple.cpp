/*
 * src/Graphics/RenderableInstance/Multiple.cpp
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

#include "Multiple.hpp"

/* STL inclusions. */
#include <array>
#include <cstdint>
#include <cstring>
#include <memory>
#include <mutex>
#include <vector>

/* Local inclusions. */
#include "Abstract.hpp"
#include "Graphics/Renderable/Interface.hpp"
#include "Graphics/Types.hpp"
#include "Graphics/ViewMatricesInterface.hpp"
#include "Libs/Math/CartesianFrame.hpp"
#include "Libs/Math/Matrix.hpp"
#include "Tracer.hpp"
#include "Saphir/Program.hpp"
#include "Vulkan/CommandBuffer.hpp"
#include "Vulkan/PipelineLayout.hpp"
#include "Vulkan/TransferManager.hpp"
#include "Vulkan/Types.hpp"

namespace EmEn::Graphics::RenderableInstance
{
	using namespace EmEn::Libs;
	using namespace EmEn::Libs::Math;
	using namespace EmEn::Vulkan;

	Multiple::Multiple (const std::shared_ptr< Renderable::Interface > & renderable, const std::vector< CartesianFrame< float > > & instanceLocations, uint32_t flagBits) noexcept
		: Abstract(renderable, EnableInstancing | flagBits),
		m_instanceCount(static_cast< uint32_t >(instanceLocations.size())),
		m_activeInstanceCount(m_instanceCount)
	{
		if ( m_instanceCount == 0 )
		{
			this->setBroken("The instance location list is empty !");

			return;
		}

		/* NOTE: Reserve the actual data place to speed up the local storage. */
		if ( this->isFacingCamera() )
		{
			m_localData.resize(m_instanceCount * SpriteVBOElementCount);
		}
		else
		{
			m_localData.resize(m_instanceCount * MeshVBOElementCount);
		}

		if ( this->updateLocalData(instanceLocations, 0) )
		{
			/* Create a vertex buffer object to hold locations in video memory
			 * according to the size of local data. */
			if ( this->createModelMatrices() )
			{
				this->observe(renderable.get());
			}
			else
			{
				this->setBroken("Unable to create the model matrices VBO !");
			}
		}
		else
		{
			this->setBroken("Unable to write the local data !");
		}
	}

	Multiple::Multiple (const std::shared_ptr< Renderable::Interface > & renderable, uint32_t instanceCount, uint32_t flagBits) noexcept
		: Abstract(renderable, EnableInstancing | flagBits),
		m_instanceCount(instanceCount)
	{
		if ( m_instanceCount == 0 )
		{
			this->setBroken("The location count is zero !");

			return;
		}

		/* NOTE: Reserve the actual data place to speed up the local storage. */
		if ( this->isFacingCamera() )
		{
			m_localData.resize(m_instanceCount * SpriteVBOElementCount);
		}
		else
		{
			m_localData.resize(m_instanceCount * MeshVBOElementCount);
		}

		this->resetLocalData();

		/* Create a vertex buffer object to hold locations in video memory
		 * according to the size of local data. */
		if ( this->createModelMatrices() )
		{
			this->observe(renderable.get());
		}
		else
		{
			this->setBroken("Unable to create the model matrices VBO !");
		}
	}

	bool
	Multiple::updateLocalData (const CartesianFrame< float > & instanceLocation, uint32_t instanceIndex) noexcept
	{
		/* Check against the local data. */
		if ( instanceIndex >= m_instanceCount )
		{
			TraceError{ClassId} << "Instance index out of bounds (" << instanceIndex << " >= " << m_instanceCount << ") !";

			return false;
		}

		if ( this->isFacingCamera() )
		{
			/* Starting offset to write vectors */
			size_t elementOffset = instanceIndex * SpriteVBOElementCount;

			/* Position */
			{
				const auto & position = instanceLocation.position();

				m_localData[elementOffset++] = position[X];
				m_localData[elementOffset++] = position[Y];
				m_localData[elementOffset++] = position[Z];
			}

			/* Scaling */
			{
				const auto & scaling = instanceLocation.scalingFactor();

				m_localData[elementOffset++] = scaling[X];
				m_localData[elementOffset++] = scaling[Y];
				m_localData[elementOffset++] = scaling[Z];
			}
		}
		else
		{
			/* Starting offset to write matrices */
			size_t elementOffset = instanceIndex * MeshVBOElementCount;

			/* Write model matrix for this instance. */
			const auto modelMatrix = instanceLocation.getModelMatrix();
			modelMatrix.copy(m_localData.data() + elementOffset);

			if ( !this->isFacingCamera() )
			{
				/* Advance offset for the normal matrix (16 floats). */
				elementOffset += 4UL * getAttributeSize(VertexAttributeType::ModelMatrixR0);

				/* Write normal matrix for this instance. */
				const auto normalModelMatrix = modelMatrix.inverse().transpose().toMatrix3();
				normalModelMatrix.copy(m_localData.data() + elementOffset);
			}
		}

		/* Mark GPU data out of date. */
		this->disableFlag(ArePositionsSynchronized);

		return true;
	}

	bool
	Multiple::updateLocalData (const std::vector< CartesianFrame< float > > & instanceLocations, uint32_t instanceOffset) noexcept
	{
		/* Check against the local data. */
		const auto endOffset = instanceOffset + instanceLocations.size();

		if ( endOffset > m_instanceCount )
		{
			TraceError{ClassId} << "Instance range out of bounds (" << instanceOffset << " + " << instanceLocations.size() << "(=" << endOffset << ") > " << m_instanceCount << ") !";

			return false;
		}

		if ( this->isFacingCamera() )
		{
			/* Starting offset to write vectors */
			auto elementOffset = instanceOffset * SpriteVBOElementCount;

			for ( const auto & instanceLocation : instanceLocations )
			{
				/* Position */
				{
					const auto & position = instanceLocation.position();

					m_localData[elementOffset++] = position[X];
					m_localData[elementOffset++] = position[Y];
					m_localData[elementOffset++] = position[Z];
				}

				/* Scaling */
				{
					const auto & scaling = instanceLocation.scalingFactor();

					m_localData[elementOffset++] = scaling[X];
					m_localData[elementOffset++] = scaling[Y];
					m_localData[elementOffset++] = scaling[Z];
				}
			}
		}
		else
		{
			/* Starting offset to write matrices */
			auto elementOffset = instanceOffset * MeshVBOElementCount;

			for ( const auto & instanceLocation : instanceLocations )
			{
				/* Write model matrix for this instance. */
				const auto modelMatrix = instanceLocation.getModelMatrix();
				modelMatrix.copy(m_localData.data() + elementOffset);

				/* Advance offset for the normal matrix (16 floats). */
				elementOffset += 4U * getAttributeSize(VertexAttributeType::ModelMatrixR0);

				/* Write normal matrix for this instance. */
				const auto normalModelMatrix = modelMatrix.inverse().transpose().toMatrix3();
				normalModelMatrix.copy(m_localData.data() + elementOffset);

				/* Advance offset for the next instance model matrix (9 floats). */
				elementOffset += 3U * getAttributeSize(VertexAttributeType::NormalModelMatrixR0);
			}
		}

		/* Mark GPU data out of date. */
		this->disableFlag(ArePositionsSynchronized);

		return true;
	}

	void
	Multiple::resetLocalData () noexcept
	{
		const auto limit = m_instanceCount;

		if ( this->isFacingCamera() )
		{
			for ( size_t instanceIndex = 0; instanceIndex < limit; instanceIndex++ )
			{
				auto offset = instanceIndex * SpriteVBOElementCount;

				/* Position */
				m_localData[offset++] = 0.0F;
				m_localData[offset++] = 0.0F;
				m_localData[offset++] = 0.0F;

				/* Scaling */
				m_localData[offset++] = 1.0F;
				m_localData[offset++] = 1.0F;
				m_localData[offset++] = 1.0F;
			}
		}
		else
		{
			constexpr auto identity3 = Matrix< 3, float >::identity();
			constexpr auto identity4 = Matrix< 4, float >::identity();

			/* Starting offset in video memory */
			size_t elementOffset = 0;

			for ( size_t instanceIndex = 0; instanceIndex < limit; instanceIndex++ )
			{
				identity4.copy(m_localData.data() + elementOffset);

				/* Advance offset for the normal matrix (16 floats). */
				elementOffset += 4UL * getAttributeSize(VertexAttributeType::ModelMatrixR0);

				identity3.copy(m_localData.data() + elementOffset);

				/* Advance offset for the next instance model matrix (9 floats). */
				elementOffset += 3UL * getAttributeSize(VertexAttributeType::NormalModelMatrixR0);
			}
		}

		this->disableFlag(ArePositionsSynchronized);
	}

	bool
	Multiple::createModelMatrices () noexcept
	{
		const std::lock_guard< std::mutex > lock{m_GPUMemoryAccess};

		if ( this->isModelMatricesCreated() )
		{
			return true;
		}

		auto * transferManager = TransferManager::instance(GPUWorkType::Graphics);

		const auto vertexElementCount = this->isFacingCamera() ? SpriteVBOElementCount : MeshVBOElementCount;
		const auto vertexCount = static_cast< uint32_t >(m_localData.size() / vertexElementCount);

		m_vertexBufferObject = std::make_unique< VertexBufferObject >(transferManager->device(), vertexCount, vertexElementCount);
		m_vertexBufferObject->setIdentifier("Multiple-ModelVBO-VertexBufferObject");

		if ( !m_vertexBufferObject->create(*transferManager, m_localData) )
		{
			Tracer::error(ClassId, "Unable to create the vertex buffer object (VBO) !");

			m_vertexBufferObject.reset();

			return false;
		}

		this->enableFlag(ArePositionsSynchronized);

		return true;
	}

	bool
	Multiple::updateVideoMemory () noexcept
	{
		const std::lock_guard< std::mutex > lock{m_GPUMemoryAccess};

		if constexpr ( IsDebug )
		{
			if ( !this->isModelMatricesCreated() )
			{
				Tracer::error(ClassId, "Trying to map an uninitialized VBO.");

				return false;
			}
		}

		if ( this->isFlagEnabled(ArePositionsSynchronized) )
		{
			return true;
		}

		auto * transferManager = TransferManager::instance(GPUWorkType::Graphics);

		if ( !m_vertexBufferObject->writeData(*transferManager, m_localData) )
		{
			Tracer::error(ClassId, "Unable to write data to the VBO.");

			return false;
		}

		/* Mark GPU data synchronized with local data. */
		this->enableFlag(ArePositionsSynchronized);

		return true;
	}

	void
	Multiple::pushMatrices (const CommandBuffer & commandBuffer, const PipelineLayout & pipelineLayout, const ViewMatricesInterface & viewMatrices, const Saphir::Program & program) const noexcept
	{
		constexpr uint32_t MatrixBytes{Matrix4Alignment * sizeof(float)};

		const VkShaderStageFlags stageFlags = program.hasGeometryShader() ?
			VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_GEOMETRY_BIT :
			VK_SHADER_STAGE_VERTEX_BIT;

		const auto & viewMatrix = viewMatrices.viewMatrix(this->isUsingInfinityView(), 0);
		const auto viewProjectionMatrix = viewMatrices.projectionMatrix() * viewMatrix;

		/* [VULKAN-PUSH-CONSTANT:4] Push camera related matrices. */
		if ( program.wasAdvancedMatricesEnabled() || program.wasBillBoardingEnabled() )
		{
			if constexpr ( MergePushConstants )
			{
				/* NOTE: Create a single buffer for 2x mat4x4. */
				std::array< float, 32 > buffer{};
				std::memcpy(buffer.data(), viewMatrix.data(), MatrixBytes);
				std::memcpy(&buffer[Matrix4Alignment], viewProjectionMatrix.data(), MatrixBytes);

				/* NOTE: Push the view matrix (V) and the view projection matrix (VP). */
				vkCmdPushConstants(
					commandBuffer.handle(),
					pipelineLayout.handle(),
					stageFlags,
					0,
					MatrixBytes * 2,
					buffer.data()
				);
			}
			else
			{
				/* NOTE: Push the view matrix (V). */
				vkCmdPushConstants(
					commandBuffer.handle(),
					pipelineLayout.handle(),
					stageFlags,
					0,
					MatrixBytes,
					viewMatrix.data()
				);

				/* NOTE: Push the view projection matrix (VP). */
				vkCmdPushConstants(
					commandBuffer.handle(),
					pipelineLayout.handle(),
					stageFlags,
					MatrixBytes,
					MatrixBytes,
					viewProjectionMatrix.data()
				);
			}
		}
		else
		{
			/* NOTE: Push the view projection matrix (VP). */
			vkCmdPushConstants(
				commandBuffer.handle(),
				pipelineLayout.handle(),
				stageFlags,
				0,
				MatrixBytes,
				viewProjectionMatrix.data()
			);
		}
	}

	void
	Multiple::bindInstanceModelLayer (const CommandBuffer & commandBuffer, uint32_t layerIndex) const noexcept
	{
		/*  Bind the geometry VBO and the optional IBO with the model matrix VBO. */
		commandBuffer.bind(*this->renderable()->geometry(), *m_vertexBufferObject, layerIndex);
	}

	bool
	Multiple::coordinatesToModelMatrices (const std::vector< CartesianFrame< float > > & coordinates, std::vector< Matrix< 4, float > > & modelMatrices, bool strict) noexcept
	{
		if ( coordinates.empty() )
		{
			return false;
		}

		if ( strict && coordinates.size() != modelMatrices.size() )
		{
			return false;
		}

		const size_t limit = std::min(coordinates.size(), modelMatrices.size());

		for ( size_t index = 0; index < limit; index++ )
		{
			modelMatrices[index] = coordinates[index].getModelMatrix();
		}

		return true;
	}
}
