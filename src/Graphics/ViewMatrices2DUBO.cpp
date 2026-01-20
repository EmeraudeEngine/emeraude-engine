/*
 * src/Graphics/ViewMatrices2DUBO.cpp
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

#include "ViewMatrices2DUBO.hpp"

/* STL inclusions. */
#include <cmath>
#include <cstring>

/* Local inclusions. */
#include "Graphics/Renderer.hpp"
#include "Tracer.hpp"

namespace EmEn::Graphics
{
	using namespace Libs;
	using namespace Libs::Math;
	using namespace Vulkan;

	void
	ViewMatrices2DUBO::updatePerspectiveViewProperties (float width, float height, float fov, float distance) noexcept
	{
		if ( width * height <= 0.0 )
		{
			TraceError{ClassId} << "The view size (" << width << " X " << height << ") is invalid!";

			return;
		}

		const auto aspectRatio = width / height;

		m_logicState.bufferData[ViewWidthOffset] = width;
		m_logicState.bufferData[ViewHeightOffset] = height;
		m_logicState.bufferData[ViewDistanceOffset] = distance;

		/* Formula : nearPlane = nearestObject / sqrt(1 + tan(fov/2)² · (aspectRatio² + 1)) */
		{
			const auto powA = std::pow(std::tan(Radian(fov) * 0.5F), 2.0F);
			const auto powB = std::pow(aspectRatio, 2.0F) + 1.0F;

			m_logicState.bufferData[ViewNearOffset] = 0.1F / std::sqrt(1.0F + powA * powB);
		}

		m_logicState.projection = Matrix< 4, float >::perspectiveProjection(fov, aspectRatio, m_logicState.bufferData[ViewNearOffset], m_logicState.bufferData[ViewDistanceOffset]);

		/*TraceDebug{ClassId} <<
			"Perspective projection:" "\n"
			"Size: " << width << " X " << height << "\n"
			"Distance: " << distance << "\n"
			"Field of view: " << fov << "\n"
			"Matrix: " << m_logicState.projection;*/

		std::memcpy(&m_logicState.bufferData[ProjectionMatrixOffset], m_logicState.projection.data(), Matrix4Alignment * sizeof(float));
	}

	void
	ViewMatrices2DUBO::updateOrthographicViewProperties (float width, float height, float nearDistance, float farDistance) noexcept
	{
		if ( width * height <= 0.0 )
		{
			TraceError{ClassId} << "The view size (" << width << " X " << height << ") is invalid!";

			return;
		}

		m_logicState.bufferData[ViewWidthOffset] = width;
		m_logicState.bufferData[ViewHeightOffset] = height;
		m_logicState.bufferData[ViewNearOffset] = nearDistance;
		m_logicState.bufferData[ViewDistanceOffset] = farDistance;

		/* NOTE: The farDistance parameter represents the TOTAL coverage size.
		 * We divide by 2 to get the half-size for the orthographic projection bounds.
		 * This makes coverageSize intuitive: coverageSize=100 means a 100x100 unit area. */
		const auto halfSide = (m_logicState.bufferData[ViewDistanceOffset] * 0.5F) * this->getAspectRatio();

		m_logicState.projection = Matrix< 4, float >::orthographicProjection(
			-halfSide, halfSide,
			-halfSide, halfSide,
			m_logicState.bufferData[ViewNearOffset], m_logicState.bufferData[ViewDistanceOffset]
		);

		/*TraceDebug{ClassId} <<
			"Orthographic projection:" "\n"
			"Size: " << width << " X " << height << "\n"
			"Near distance: " << nearDistance << "\n"
			"Far distance: " << farDistance << "\n"
			"Matrix: " << m_logicState.projection;*/

		std::memcpy(&m_logicState.bufferData[ProjectionMatrixOffset], m_logicState.projection.data(), Matrix4Alignment * sizeof(float));
	}

	void
	ViewMatrices2DUBO::updateViewCoordinates (const CartesianFrame< float > & coordinates, const Vector< 3, float > & velocity) noexcept
	{
		m_logicState.view = coordinates.getViewMatrix();
		m_logicState.infinityView = coordinates.getInfinityViewMatrix();
		m_logicState.position = coordinates.position();
		m_logicState.frustum.update(m_logicState.projection * m_logicState.view);

		/* FIXME: These data are not constantly updated on GPU. */
		m_logicState.bufferData[WorldPositionOffset + 0] = m_logicState.position.x();
		m_logicState.bufferData[WorldPositionOffset + 1] = m_logicState.position.y();
		m_logicState.bufferData[WorldPositionOffset + 2] = m_logicState.position.z();

		m_logicState.bufferData[VelocityVectorOffset + 0] = velocity.x();
		m_logicState.bufferData[VelocityVectorOffset + 1] = velocity.y();
		m_logicState.bufferData[VelocityVectorOffset + 2] = velocity.z();
	}

	void
	ViewMatrices2DUBO::updateAmbientLightProperties (const PixelFactory::Color< float > & color, float intensity) noexcept
	{
		m_logicState.bufferData[AmbientLightColorOffset+0] = color.red();
		m_logicState.bufferData[AmbientLightColorOffset+1] = color.green();
		m_logicState.bufferData[AmbientLightColorOffset+2] = color.blue();

		m_logicState.bufferData[AmbientLightIntensityOffset] = intensity;
	}

	bool
	ViewMatrices2DUBO::create (Renderer & renderer, const std::string & instanceID) noexcept
	{
		auto descriptorSetLayout = RenderTarget::Abstract::getDescriptorSetLayout(renderer.layoutManager());

		if ( descriptorSetLayout == nullptr )
		{
			return false;
		}

		m_uniformBufferObject = std::make_unique< UniformBufferObject >(renderer.device(), ViewUBOSize);
		m_uniformBufferObject->setIdentifier(ClassId, instanceID, "UniformBufferObject");

		if ( !m_uniformBufferObject->createOnHardware() )
		{
			Tracer::error(ClassId, "Unable to get an uniform buffer object for close view !");

			m_uniformBufferObject.reset();

			return false;
		}

		m_descriptorSet = std::make_unique< DescriptorSet >(renderer.descriptorPool(), descriptorSetLayout);
		m_descriptorSet->setIdentifier(ClassId, instanceID, "DescriptorSet");

		if ( !m_descriptorSet->create() )
		{
			m_descriptorSet.reset();

			Tracer::error(ClassId, "Unable to create the close view descriptor set !");

			return false;
		}

		if ( !m_descriptorSet->writeUniformBufferObject(0, *m_uniformBufferObject) )
		{
			Tracer::error(ClassId, "Unable to setup the close view descriptor set !");

			return false;
		}

		return true;
	}

	bool
	ViewMatrices2DUBO::updateVideoMemory (uint32_t readStateIndex) const noexcept
	{
		if constexpr ( IsDebug )
		{
			if ( readStateIndex >= m_renderState.size() )
			{
				Tracer::error(ClassId, "Index overflow !");

				return false;
			}

			if ( m_uniformBufferObject == nullptr )
			{
				Tracer::error(ClassId, "The view uniform buffer object is not initialized !");

				return false;
			}
		}

		/* [VULKAN-CPU-SYNC] Maybe useless */
		/* NOTE: Lock between updateVideoMemory() and destroy(). */
		const std::lock_guard< std::mutex > lock{m_GPUBufferAccessLock};

		auto * pointer = m_uniformBufferObject->mapMemoryAs< float >(0, VK_WHOLE_SIZE);

		if ( pointer == nullptr )
		{
			return false;
		}

		std::memcpy(pointer, m_renderState[readStateIndex].bufferData.data(), m_renderState[readStateIndex].bufferData.size() * sizeof(float));

		m_uniformBufferObject->unmapMemory(0, VK_WHOLE_SIZE);

		return true;
	}

	std::ostream &
	operator<< (std::ostream & out, const ViewMatrices2DUBO & obj)
	{
		out <<
			"2D View matrices data : " "\n"
			"World position " << obj.m_logicState.position << "\n"
			"Projection " << obj.m_logicState.projection <<
			"View " << obj.m_logicState.view <<
			"Infinity view " << obj.m_logicState.infinityView <<
			obj.m_logicState.frustum <<
			"Buffer data for GPU : " "\n";

		for ( size_t index = 0; index < obj.m_logicState.bufferData.size(); index += 4 )
		{
			out << '[' << obj.m_logicState.bufferData[index+0] << ", " << obj.m_logicState.bufferData[index+1] << ", " << obj.m_logicState.bufferData[index+2] << ", " << obj.m_logicState.bufferData[index+3] << "]" "\n";
		}

		return out;
	}

	std::string
	to_string (const ViewMatrices2DUBO & obj) noexcept
	{
		std::stringstream output;

		output << obj;

		return output.str();
	}
}
