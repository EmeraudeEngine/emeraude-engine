/*
 * src/Graphics/ViewMatrices2DUBO.cpp
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

#include "ViewMatrices2DUBO.hpp"

/* STL inclusions. */
#include <cmath>
#include <cstring>
#include <ostream>

/* Local inclusions. */
#include "Graphics/Renderer.hpp"
#include "Tracer.hpp"

namespace EmEn::Graphics
{
	using namespace EmEn::Libs;
	using namespace EmEn::Libs::Math;
	using namespace Vulkan;

	void
	ViewMatrices2DUBO::updatePerspectiveViewProperties (float width, float height, float distance, float fov) noexcept
	{
		if ( width * height <= 0.0 )
		{
			TraceError{ClassId} << "The view size is invalid ! Width: " << width << ", height: " << height << ", distance: " << distance << ", FOV: " << fov;

			return;
		}

		const auto aspectRatio = width / height;

		m_bufferData[ViewWidthOffset] = width;
		m_bufferData[ViewHeightOffset] = height;
		m_bufferData[ViewDistanceOffset] = distance;

		/* Formula : nearPlane = nearestObject / sqrt(1 + tan(fov/2)² · (aspectRatio² + 1)) */
		{
			const auto powA = std::pow(std::tan(Radian(fov) * 0.5F), 2.0F);
			const auto powB = std::pow(aspectRatio, 2.0F) + 1.0F;

			m_bufferData[ViewNearOffset] = 0.1F / std::sqrt(1.0F + powA * powB);
		}

		m_projection = Matrix< 4, float >::perspectiveProjection(fov, aspectRatio, m_bufferData[ViewNearOffset], m_bufferData[ViewDistanceOffset]);

		std::memcpy(&m_bufferData[ProjectionMatrixOffset], m_projection.data(), Matrix4Alignment * sizeof(float));

		this->updateVideoMemory();
	}

	void
	ViewMatrices2DUBO::updateOrthographicViewProperties (float width, float height, float distance, float near) noexcept
	{
		if ( width * height <= 0.0 )
		{
			TraceError{ClassId} << "The view size is invalid ! Width: " << width << ", height: " << height << ", distance: " << distance << ", near: " << near;

			return;
		}

		const auto side = m_bufferData[ViewDistanceOffset] * this->getAspectRatio();

		m_bufferData[ViewWidthOffset] = width;
		m_bufferData[ViewHeightOffset] = height;
		m_bufferData[ViewNearOffset] = near;
		m_bufferData[ViewDistanceOffset] = distance;

		m_projection = Matrix< 4, float >::orthographicProjection(
			-side, side,
			-m_bufferData[ViewDistanceOffset], m_bufferData[ViewDistanceOffset],
			m_bufferData[ViewNearOffset], m_bufferData[ViewDistanceOffset]
		);

		std::memcpy(&m_bufferData[ProjectionMatrixOffset], m_projection.data(), Matrix4Alignment * sizeof(float));

		this->updateVideoMemory();
	}

	void
	ViewMatrices2DUBO::updateViewCoordinates (const CartesianFrame< float > & coordinates, const Vector< 3, float > & velocity) noexcept
	{
		m_view = coordinates.getViewMatrix();
		m_infinityView = coordinates.getInfinityViewMatrix();
		m_position = coordinates.position();
		m_frustum.update(m_projection * m_view);

		/* FIXME: These data is not constantly updated on GPU. */
		m_bufferData[WorldPositionOffset + 0] = m_position.x();
		m_bufferData[WorldPositionOffset + 1] = m_position.y();
		m_bufferData[WorldPositionOffset + 2] = m_position.z();

		m_bufferData[VelocityVectorOffset + 0] = velocity.x();
		m_bufferData[VelocityVectorOffset + 1] = velocity.y();
		m_bufferData[VelocityVectorOffset + 2] = velocity.z();
	}

	void
	ViewMatrices2DUBO::updateAmbientLightProperties (const PixelFactory::Color< float > & color, float intensity) noexcept
	{
		m_bufferData[AmbientLightColorOffset+0] = color.red();
		m_bufferData[AmbientLightColorOffset+1] = color.green();
		m_bufferData[AmbientLightColorOffset+2] = color.blue();

		m_bufferData[AmbientLightIntensityOffset] = intensity;

		this->updateVideoMemory();
	}

	bool
	ViewMatrices2DUBO::create (Renderer & renderer, const std::string & instanceID) noexcept
	{
		auto descriptorSetLayout = this->getDescriptorSetLayout();

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

		/* Initial video memory update. */
		return this->updateVideoMemory();
	}

	bool
	ViewMatrices2DUBO::updateVideoMemory () const noexcept
	{
		if constexpr ( IsDebug )
		{
			if ( m_uniformBufferObject == nullptr )
			{
				Tracer::error(ClassId, "The view uniform buffer object is not initialized !");

				return false;
			}
		}

		/* NOTE: Lock between updateVideoMemory() and destroy(). */
		const std::lock_guard< std::mutex > lockGuard{m_GPUBufferAccessLock};

		auto * pointer = m_uniformBufferObject->mapMemory< float >();

		if ( pointer == nullptr )
		{
			return false;
		}

		std::memcpy(pointer, m_bufferData.data(), m_bufferData.size() * sizeof(float));

		m_uniformBufferObject->unmapMemory();

		return true;
	}

	std::ostream &
	operator<< (std::ostream & out, const ViewMatrices2DUBO & obj)
	{
		out <<
			"2D View matrices data : " "\n"
			"World position " << obj.m_position << "\n"
			"Projection " << obj.m_projection <<
			"View " << obj.m_view <<
			"Infinity view " << obj.m_infinityView <<
			obj.m_frustum <<
			"Buffer data for GPU : " "\n";

		for ( size_t index = 0; index < obj.m_bufferData.size(); index += 4 )
		{
			out << '[' << obj.m_bufferData[index+0] << ", " << obj.m_bufferData[index+1] << ", " << obj.m_bufferData[index+2] << ", " << obj.m_bufferData[index+3] << "]" "\n";
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
