/*
 * src/Graphics/ViewMatrices3DUBO.cpp
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

#include "ViewMatrices3DUBO.hpp"

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

	const std::array< Matrix< 4, float >, CubemapFaceIndexes.size() > ViewMatrices3DUBO::CubemapOrientation{
		Matrix< 4, float >::lookAt(Vector< 3, float >{0.0F, 0.0F, 0.0F}, Vector< 3, float >{ 1.0F,  0.0F,  0.0F}, Vector< 3, float >{ 0.0F, -1.0F,  0.0F}), // X+
		Matrix< 4, float >::lookAt(Vector< 3, float >{0.0F, 0.0F, 0.0F}, Vector< 3, float >{-1.0F,  0.0F,  0.0F}, Vector< 3, float >{ 0.0F, -1.0F,  0.0F}), // X-
		Matrix< 4, float >::lookAt(Vector< 3, float >{0.0F, 0.0F, 0.0F}, Vector< 3, float >{ 0.0F, -1.0F,  0.0F}, Vector< 3, float >{ 0.0F,  0.0F, -1.0F}), // Y+
		Matrix< 4, float >::lookAt(Vector< 3, float >{0.0F, 0.0F, 0.0F}, Vector< 3, float >{ 0.0F,  1.0F,  0.0F}, Vector< 3, float >{ 0.0F,  0.0F,  1.0F}), // Y-
		Matrix< 4, float >::lookAt(Vector< 3, float >{0.0F, 0.0F, 0.0F}, Vector< 3, float >{ 0.0F,  0.0F,  1.0F}, Vector< 3, float >{ 0.0F, -1.0F,  0.0F}), // Z+
		Matrix< 4, float >::lookAt(Vector< 3, float >{0.0F, 0.0F, 0.0F}, Vector< 3, float >{ 0.0F,  0.0F, -1.0F}, Vector< 3, float >{ 0.0F, -1.0F,  0.0F}) // Z-
	};

	const std::array< Matrix< 4, float >, CubemapFaceIndexes.size() > ViewMatrices3DUBO::ShadowCubemapOrientation{
		Matrix< 4, float >::lookAt(Vector< 3, float >{0.0F, 0.0F, 0.0F}, Vector< 3, float >{-1.0F,  0.0F,  0.0F}, Vector< 3, float >{ 0.0F,  1.0F,  0.0F}), // X+
		Matrix< 4, float >::lookAt(Vector< 3, float >{0.0F, 0.0F, 0.0F}, Vector< 3, float >{ 1.0F,  0.0F,  0.0F}, Vector< 3, float >{ 0.0F,  1.0F,  0.0F}), // X-
		Matrix< 4, float >::lookAt(Vector< 3, float >{0.0F, 0.0F, 0.0F}, Vector< 3, float >{ 0.0F,  1.0F,  0.0F}, Vector< 3, float >{ 0.0F,  0.0F,  1.0F}), // Y+
		Matrix< 4, float >::lookAt(Vector< 3, float >{0.0F, 0.0F, 0.0F}, Vector< 3, float >{ 0.0F, -1.0F,  0.0F}, Vector< 3, float >{ 0.0F,  0.0F, -1.0F}), // Y-
		Matrix< 4, float >::lookAt(Vector< 3, float >{0.0F, 0.0F, 0.0F}, Vector< 3, float >{ 0.0F,  0.0F, -1.0F}, Vector< 3, float >{ 0.0F,  1.0F,  0.0F}), // Z+
		Matrix< 4, float >::lookAt(Vector< 3, float >{0.0F, 0.0F, 0.0F}, Vector< 3, float >{ 0.0F,  0.0F,  1.0F}, Vector< 3, float >{ 0.0F,  1.0F,  0.0F}) // Z-
	};

	void
	ViewMatrices3DUBO::updatePerspectiveViewProperties (float width, float height, float distance, float /*fov*/) noexcept
	{
		const auto powA = std::pow(std::tan(Radian(QuartRevolution< float >) * 0.5F), 2.0F);

		m_bufferData[ViewWidthOffset] = width;
		m_bufferData[ViewHeightOffset] = height;
		m_bufferData[ViewDistanceOffset] = distance;
		m_bufferData[ViewNearOffset] = 0.1F / std::sqrt(1.0F + powA * 2.0F);

		m_projection = Matrix< 4, float >::perspectiveProjection(QuartRevolution< float >, 1.0F, m_bufferData[ViewNearOffset], m_bufferData[ViewDistanceOffset]);

		std::memcpy(&m_bufferData[ProjectionMatrixOffset], m_projection.data(), Matrix4Alignment * sizeof(float));

		this->updateVideoMemory();
	}

	void
	ViewMatrices3DUBO::updateOrthographicViewProperties (float width, float height, float distance, float near) noexcept
	{
		m_bufferData[ViewWidthOffset] = width;
		m_bufferData[ViewHeightOffset] = height;
		m_bufferData[ViewNearOffset] = near;
		m_bufferData[ViewDistanceOffset] = distance;

		m_projection = Matrix< 4, float >::orthographicProjection(
			-m_bufferData[ViewDistanceOffset], m_bufferData[ViewDistanceOffset],
			-m_bufferData[ViewDistanceOffset], m_bufferData[ViewDistanceOffset],
			m_bufferData[ViewNearOffset], m_bufferData[ViewDistanceOffset]
		);

		std::memcpy(&m_bufferData[ProjectionMatrixOffset], m_projection.data(), Matrix4Alignment * sizeof(float));

		this->updateVideoMemory();
	}

	void
	ViewMatrices3DUBO::updateViewCoordinates (const CartesianFrame< float > & coordinates, const Vector< 3, float > & velocity) noexcept
	{
		m_position = coordinates.position();

		for ( auto face : CubemapFaceIndexes )
		{
			const auto faceIndex = static_cast< size_t >(face);

			m_views.at(faceIndex) = CubemapOrientation.at(faceIndex) * Matrix< 4, float >::translation(-m_position);
			m_infinityViews.at(faceIndex) = CubemapOrientation.at(faceIndex) * Matrix< 4, float >::translation(-m_position);
			m_frustums.at(faceIndex).update(m_projection * m_views.at(faceIndex));
		}

		/* FIXME: These data is not constantly updated on GPU. */
		m_bufferData[WorldPositionOffset + 0] = m_position.x();
		m_bufferData[WorldPositionOffset + 1] = m_position.y();
		m_bufferData[WorldPositionOffset + 2] = m_position.z();

		m_bufferData[VelocityVectorOffset + 0] = velocity.x();
		m_bufferData[VelocityVectorOffset + 1] = velocity.y();
		m_bufferData[VelocityVectorOffset + 2] = velocity.z();
	}

	void
	ViewMatrices3DUBO::updateAmbientLightProperties (const PixelFactory::Color< float > & color, float intensity) noexcept
	{
		m_bufferData[AmbientLightColorOffset+0] = color.red();
		m_bufferData[AmbientLightColorOffset+1] = color.green();
		m_bufferData[AmbientLightColorOffset+2] = color.blue();

		m_bufferData[AmbientLightIntensityOffset] = intensity;

		this->updateVideoMemory();
	}

	bool
	ViewMatrices3DUBO::create (Renderer & renderer, const std::string & instanceID) noexcept
	{
		const auto descriptorSetLayout = this->getDescriptorSetLayout();

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

		m_descriptorSet = std::make_unique< DescriptorSet >(renderer.descriptorPool(), this->getDescriptorSetLayout());
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
	ViewMatrices3DUBO::updateVideoMemory () const noexcept
	{
		if constexpr ( IsDebug )
		{
			if ( m_uniformBufferObject == nullptr )
			{
				Tracer::error(ClassId, "The uniform buffer object is uninitialized !");

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
	operator<< (std::ostream & out, const ViewMatrices3DUBO & /*obj*/)
	{
		return out << "NOT YET";
	}

	std::string
	to_string (const ViewMatrices3DUBO & obj) noexcept
	{
		std::stringstream output;

		output << obj;

		return output.str();
	}
}
