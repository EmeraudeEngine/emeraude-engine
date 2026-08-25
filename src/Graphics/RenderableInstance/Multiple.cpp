/*
 * src/Graphics/RenderableInstance/Multiple.cpp
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
 * https://github.com/EmeraudeEngine/emeraude-engine
 *
 * --- THIS IS AUTOMATICALLY GENERATED, DO NOT CHANGE ---
 */

#include "Multiple.hpp"

/* STL inclusions. */
#include <array>
#include <cstring>
#include <mutex>

/* Local inclusions. */
#include "Graphics/Renderer.hpp"
#include "Graphics/ViewMatricesInterface.hpp"
#include "Vulkan/CommandBuffer.hpp"
#include "Vulkan/PipelineLayout.hpp"
#include "Vulkan/TransferManager.hpp"

namespace EmEn::Graphics::RenderableInstance
{
	using namespace Base;
	using namespace Base::Math;
	using namespace Vulkan;

	Multiple::Multiple (const std::shared_ptr< Device > & device, const std::shared_ptr< Renderable::Abstract > & renderable, const std::vector< CartesianFrame< float > > & instanceLocations, uint32_t flagBits) noexcept
		: Abstract{renderable, flagBits},
		m_instanceCount{static_cast< uint32_t >(instanceLocations.size())},
		m_activeInstanceCount{m_instanceCount}
	{
		if ( m_instanceCount == 0 )
		{
			this->setBroken("The instance location list is empty !");

			return;
		}

		/* NOTE: Reserve the actual data place to speed up the local storage. */
		if ( this->renderable()->isSprite() )
		{
			m_localData.resize(m_instanceCount * SpriteVBOElementCount);
		}
		else
		{
			m_localData.resize(m_instanceCount * this->meshVBOElementCount());
		}

		if ( this->updateLocalData(instanceLocations, 0) )
		{
			/* Motion history: the initial update archived the pristine (zeroed) buffer into
			 * the previous-model slots. Re-seed them with the initial model matrices so the
			 * first rendered frame carries zero object velocity instead of a bogus one. */
			if ( this->isFlagEnabled(EnableInstanceMotionHistory) && !this->renderable()->isSprite() )
			{
				const auto elementCount = this->meshVBOElementCount();

				for ( uint32_t instanceIndex = 0; instanceIndex < m_instanceCount; instanceIndex++ )
				{
					const auto elementOffset = static_cast< size_t >(instanceIndex) * elementCount;

					std::memcpy(m_localData.data() + elementOffset + MeshVBOElementCount, m_localData.data() + elementOffset, 16UL * sizeof(float));
				}
			}

			/* Create a vertex buffer object to hold locations in video memory
			 * according to the size of local data. */
			if ( !this->createOnHardware(device) )
			{
				this->setBroken("Unable to create the model matrices VBO !");
			}
		}
		else
		{
			this->setBroken("Unable to write the local data !");
		}
	}

	Multiple::Multiple (const std::shared_ptr< Device > & device, const std::shared_ptr< Renderable::Abstract > & renderable, uint32_t instanceCount, uint32_t flagBits) noexcept
		: Abstract{renderable, flagBits},
		m_instanceCount{instanceCount}
	{
		if ( m_instanceCount == 0 )
		{
			this->setBroken("The location count is zero !");

			return;
		}

		/* NOTE: Reserve the actual data place to speed up the local storage. */
		if ( this->renderable()->isSprite() )
		{
			m_localData.resize(m_instanceCount * SpriteVBOElementCount);
		}
		else
		{
			m_localData.resize(m_instanceCount * this->meshVBOElementCount());
		}

		this->resetLocalData();

		/* Create a vertex buffer object to hold locations in video memory
		 * according to the size of local data. */
		if ( !this->createOnHardware(device) )
		{
			this->setBroken("Unable to create the model matrices VBO !");
		}
	}

	bool
	Multiple::updateLocalData (const CartesianFrame< float > & instanceLocation, uint32_t instanceIndex) noexcept
	{
		/* [VULKAN-CPU-SYNC] Protects local data (Logic Thread) */
		const std::lock_guard< std::mutex > lock{m_localDataAccess};

		/* Check against the local data. */
		if ( instanceIndex >= m_instanceCount )
		{
			TraceError{ClassId} << "Instance index out of bounds (" << instanceIndex << " >= " << m_instanceCount << ") !";

			return false;
		}

		if ( this->renderable()->isSprite() )
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
			size_t elementOffset = instanceIndex * this->meshVBOElementCount();

			/* Motion history: archive the current model matrix into the previous-model slot
			 * BEFORE overwriting it (one history step per logic update). */
			if ( this->isFlagEnabled(EnableInstanceMotionHistory) )
			{
				std::memcpy(m_localData.data() + elementOffset + MeshVBOElementCount, m_localData.data() + elementOffset, 16UL * sizeof(float));
			}

			/* Write model matrix for this instance. */
			const auto modelMatrix = instanceLocation.getModelMatrix();
			modelMatrix.copy(m_localData.data() + elementOffset);

			if ( !this->renderable()->isSprite() )
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
		/* [VULKAN-CPU-SYNC] Protects local data (Logic Thread) */
		const std::lock_guard< std::mutex > lock{m_localDataAccess};

		/* Check against the local data. */
		if ( const auto endOffset = instanceOffset + instanceLocations.size(); endOffset > m_instanceCount )
		{
			TraceError{ClassId} << "Instance range out of bounds (" << instanceOffset << " + " << instanceLocations.size() << "(=" << endOffset << ") > " << m_instanceCount << ") !";

			return false;
		}

		if ( this->renderable()->isSprite() )
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
			const auto elementCount = this->meshVBOElementCount();
			const bool motionHistory = this->isFlagEnabled(EnableInstanceMotionHistory);
			auto elementOffset = instanceOffset * elementCount;

			for ( const auto & instanceLocation : instanceLocations )
			{
				/* Motion history: archive the current model matrix into the previous-model
				 * slot BEFORE overwriting it (one history step per logic update). */
				if ( motionHistory )
				{
					std::memcpy(m_localData.data() + elementOffset + MeshVBOElementCount, m_localData.data() + elementOffset, 16UL * sizeof(float));
				}

				/* Write model matrix for this instance. */
				const auto modelMatrix = instanceLocation.getModelMatrix();
				modelMatrix.copy(m_localData.data() + elementOffset);

				/* Advance offset for the normal matrix (16 floats). */
				elementOffset += 4U * getAttributeSize(VertexAttributeType::ModelMatrixR0);

				/* Write normal matrix for this instance. */
				const auto normalModelMatrix = modelMatrix.inverse().transpose().toMatrix3();
				normalModelMatrix.copy(m_localData.data() + elementOffset);

				/* Advance offset for the next instance model matrix (9 floats),
				 * skipping the previous-model slot when motion history is enabled. */
				elementOffset += 3U * getAttributeSize(VertexAttributeType::NormalModelMatrixR0);

				if ( motionHistory )
				{
					elementOffset += 16U;
				}
			}
		}

		/* Mark GPU data out of date. */
		this->disableFlag(ArePositionsSynchronized);

		return true;
	}

	void
	Multiple::resetLocalData() noexcept
	{
		const auto limit = m_instanceCount;

		if ( this->renderable()->isSprite() )
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

			/* The offset in video memory */
			size_t elementOffset = 0;

			const bool motionHistory = this->isFlagEnabled(EnableInstanceMotionHistory);

			for ( size_t instanceIndex = 0; instanceIndex < limit; instanceIndex++ )
			{
				identity4.copy(m_localData.data() + elementOffset);

				/* Advance offset for the normal matrix (16 floats). */
				elementOffset += 4UL * getAttributeSize(VertexAttributeType::ModelMatrixR0);

				identity3.copy(m_localData.data() + elementOffset);

				/* Advance offset for the next instance model matrix (9 floats). */
				elementOffset += 3UL * getAttributeSize(VertexAttributeType::NormalModelMatrixR0);

				/* Motion history: the previous-model slot starts as identity too. */
				if ( motionHistory )
				{
					identity4.copy(m_localData.data() + elementOffset);

					elementOffset += 16UL;
				}
			}
		}

		this->disableFlag(ArePositionsSynchronized);
	}

	bool
	Multiple::createOnHardware(const std::shared_ptr< Device > & device) noexcept
	{
		/* [VULKAN-CPU-SYNC] Protects local data (Render Thread) */
		const std::lock_guard< std::mutex > lock{m_localDataAccess};

		if ( this->isModelMatricesCreated() )
		{
			return true;
		}

		const auto vertexElementCount = this->renderable()->isSprite() ? SpriteVBOElementCount : this->meshVBOElementCount();
		const auto vertexCount = static_cast< uint32_t >(m_localData.size() / vertexElementCount);

		m_vertexBufferObject = std::make_unique< VertexBufferObject >(device, vertexCount, vertexElementCount, true);
		m_vertexBufferObject->setIdentifier(ClassId, "MultipleInstance??", "VertexBufferObject");

		if ( !m_vertexBufferObject->createOnHardware() || !m_vertexBufferObject->writeData(m_localData) )
		{
			Tracer::error(ClassId, "Unable to create the vertex buffer object (VBO) !");

			m_vertexBufferObject.reset();

			return false;
		}

		this->enableFlag(ArePositionsSynchronized);

		return true;
	}

	bool
	Multiple::updateVideoMemory() noexcept
	{
		/* [VULKAN-CPU-SYNC] Protects local data (Render Thread) */
		const std::lock_guard< std::mutex > lock{m_localDataAccess};

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

		/* TODO: Try to use something always mapped! */
		if ( !m_vertexBufferObject->writeData(m_localData) )
		{
			Tracer::error(ClassId, "Unable to write data to the VBO.");

			return false;
		}

		/* Mark GPU data synchronized with local data. */
		this->enableFlag(ArePositionsSynchronized);

		return true;
	}

	void
	Multiple::pushMatricesForShadowCasting (const RenderPassContext & passContext, const PushConstantContext & pushContext, const CartesianFrame< float > * /*worldCoordinates*/) const noexcept
	{
		/* For cubemap rendering, View/Projection matrices are in UBO indexed by gl_ViewIndex.
		 * Model matrices are already in the VBO, so nothing to push! */
		if ( passContext.isCubemap )
		{
			/* No push constants needed for cubemap instancing. */
			return;
		}

		/* Classic 2D rendering (Model is in VBO).
		 * Layout: V|VP + jitter + frameIndex = 76 B, matching the instanced block declared by
		 * Saphir::Generator::Abstract::declareMatrixPushConstantBlock(). A shadow map view never
		 * jitters (only the main view calls setProjectionJitter()), so the jitter pushed here is
		 * always zero — but it MUST be pushed: the vertex shader reads it (a 2D shadow map is
		 * neither a cubemap nor a CSM target) and would otherwise offset gl_Position by
		 * uninitialized push-constant memory. */
		const auto & viewMatrix = passContext.viewMatrices->viewMatrix(passContext.readStateIndex, this->isUsingInfinityView(), 0);
		const auto & projectionJitter = passContext.viewMatrices->projectionJitter();

		std::array< float, Matrix4Alignment + 3 > buffer{};

		if ( pushContext.useBillboarding )
		{
			/* Push the view matrix (V) ONLY — the shader recomposes VP from the view UBO
			 * projection × V (the V + VP push block was above the 128 B minimum guarantee). */
			std::memcpy(buffer.data(), viewMatrix.data(), MatrixBytes);
		}
		else
		{
			/* Push the view projection matrix (VP). */
			const auto viewProjectionMatrix = passContext.viewMatrices->unjitteredProjectionMatrix(passContext.readStateIndex) * viewMatrix;

			std::memcpy(buffer.data(), viewProjectionMatrix.data(), MatrixBytes);
		}

		buffer[Matrix4Alignment] = projectionJitter.x();
		buffer[Matrix4Alignment + 1] = projectionJitter.y();
		buffer[Matrix4Alignment + 2] = static_cast< float >(this->frameIndexFor(pushContext.layerIndex));

		vkCmdPushConstants(
			passContext.commandBuffer->handle(),
			pushContext.pipelineLayout->handle(),
			pushContext.stageFlags,
			0,
			MatrixBytes + (3 * sizeof(float)),
			buffer.data()
		);
	}

	void
	Multiple::pushMatricesForRendering (const RenderPassContext & passContext, const PushConstantContext & pushContext, const CartesianFrame< float > * /*worldCoordinates*/) const noexcept
	{
		/* For cubemap rendering, View/Projection matrices are in UBO indexed by gl_ViewIndex.
		 * Model matrices are already in the VBO, so nothing to push! */
		if ( passContext.isCubemap )
		{
			/* No push constants needed for cubemap instancing. */
			return;
		}

		const auto handle = passContext.commandBuffer->handle();
		const auto layout = pushContext.pipelineLayout->handle();
		const auto flags = pushContext.stageFlags;
		const auto frameIndex = static_cast< float >(this->frameIndexFor(pushContext.layerIndex));

		/* Classic 2D rendering (Model is in VBO). */
		const auto & viewMatrix = passContext.viewMatrices->viewMatrix(passContext.readStateIndex, this->isUsingInfinityView(), 0);

		/* Layout: V|VP + jitter + frameIndex = 76 B. The pushed matrix is UNJITTERED — the
		 * sub-pixel TAA offset is applied to gl_Position by the vertex shader from the jitter
		 * member, keeping the velocity clip positions jitter-free. MUST match
		 * Saphir::Generator::Abstract::declareMatrixPushConstantBlock(). */
		const auto & projectionJitter = passContext.viewMatrices->projectionJitter();

		std::array< float, Matrix4Alignment + 3 > buffer{};

		if ( pushContext.useAdvancedMatrices || pushContext.useBillboarding )
		{
			/* Push the view matrix (V) ONLY — the shader recomposes VP from the view UBO
			 * projection × V (the V + VP + frameIndex push block was 132 B, above the 128 B
			 * Vulkan minimum guarantee for maxPushConstantsSize). */
			std::memcpy(buffer.data(), viewMatrix.data(), MatrixBytes);
		}
		else
		{
			/* Push the view projection matrix (VP). */
			const auto viewProjectionMatrix = passContext.viewMatrices->unjitteredProjectionMatrix(passContext.readStateIndex) * viewMatrix;

			std::memcpy(buffer.data(), viewProjectionMatrix.data(), MatrixBytes);
		}

		buffer[Matrix4Alignment] = projectionJitter.x();
		buffer[Matrix4Alignment + 1] = projectionJitter.y();
		buffer[Matrix4Alignment + 2] = frameIndex;

		vkCmdPushConstants(handle, layout, flags, 0, MatrixBytes + (3 * sizeof(float)), buffer.data());
	}

	void
	Multiple::bindInstanceModelLayer (const CommandBuffer & commandBuffer, uint32_t layerIndex, uint32_t LODLevel) const noexcept
	{
		/* Bind the geometry VBO and the optional IBO with the model matrix VBO. */
		commandBuffer.bind(*this->renderable()->geometry(LODLevel), *m_vertexBufferObject, layerIndex, 0);
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
