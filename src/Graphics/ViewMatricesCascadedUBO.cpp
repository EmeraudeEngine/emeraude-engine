/*
 * src/Graphics/ViewMatricesCascadedUBO.cpp
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

#include "ViewMatricesCascadedUBO.hpp"

/* STL inclusions. */
#include <cmath>
#include <cstring>
#include <limits>

/* Local inclusions. */
#include "Graphics/Renderer.hpp"
#include "Graphics/RenderTarget/Abstract.hpp"
#include "Tracer.hpp"

namespace EmEn::Graphics
{
	using namespace Libs;
	using namespace Libs::Math;
	using namespace Vulkan;

	ViewMatricesCascadedUBO::ViewMatricesCascadedUBO (uint32_t cascadeCount, float lambda) noexcept
		: m_cascadeCount{std::clamp(cascadeCount, 1U, MaxCascadeCount)},
		m_lambda{std::clamp(lambda, 0.0F, 1.0F)}
	{
		/* Initialize buffer data with identity matrices for cascades. */
		for ( size_t cascade = 0; cascade < MaxCascadeCount; ++cascade )
		{
			const auto offset = cascade * 16;
			m_logicState.bufferData[offset + 0] = 1.0F;
			m_logicState.bufferData[offset + 5] = 1.0F;
			m_logicState.bufferData[offset + 10] = 1.0F;
			m_logicState.bufferData[offset + 15] = 1.0F;

			m_logicState.cascadeViewProjections[cascade] = Matrix< 4, float >::identity();
		}

		/* Initialize cascade count in buffer. */
		m_logicState.bufferData[CascadeCountOffset] = static_cast< float >(m_cascadeCount);
		m_logicState.bufferData[ShadowBiasOffset] = 0.005F; /* Default shadow bias. */

		/* Initialize view properties. */
		m_logicState.bufferData[ViewWidthOffset] = 1.0F;
		m_logicState.bufferData[ViewHeightOffset] = 1.0F;
		m_logicState.bufferData[ViewNearOffset] = 0.1F;
		m_logicState.bufferData[ViewDistanceOffset] = 1000.0F;
	}

	void
	ViewMatricesCascadedUBO::updatePerspectiveViewProperties (float width, float height, float fov, float distance) noexcept
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

		/* Recompute split distances when view properties change. */
		this->computeSplitDistances(m_logicState.bufferData[ViewNearOffset], distance);
	}

	void
	ViewMatricesCascadedUBO::updateOrthographicViewProperties (float width, float height, float nearDistance, float farDistance) noexcept
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

		/* NOTE: Compute the side AFTER updating ViewDistanceOffset to use the new farDistance value.
		 * The side represents half the width/height of the orthographic frustum, scaled by aspect ratio. */
		const auto side = m_logicState.bufferData[ViewDistanceOffset] * this->getAspectRatio();

		m_logicState.projection = Matrix< 4, float >::orthographicProjection(
			-side, side,
			-side, side,
			m_logicState.bufferData[ViewNearOffset], m_logicState.bufferData[ViewDistanceOffset]
		);

		/* Recompute split distances when view properties change. */
		this->computeSplitDistances(nearDistance, farDistance);
	}

	void
	ViewMatricesCascadedUBO::updateViewCoordinates (const CartesianFrame< float > & coordinates, const Vector< 3, float > & velocity) noexcept
	{
		m_logicState.view = coordinates.getViewMatrix();
		m_logicState.infinityView = coordinates.getInfinityViewMatrix();
		m_logicState.position = coordinates.position();
		m_logicState.frustum.update(m_logicState.projection * m_logicState.view);

		m_logicState.bufferData[WorldPositionOffset + 0] = m_logicState.position.x();
		m_logicState.bufferData[WorldPositionOffset + 1] = m_logicState.position.y();
		m_logicState.bufferData[WorldPositionOffset + 2] = m_logicState.position.z();
		m_logicState.bufferData[WorldPositionOffset + 3] = 1.0F;

		m_logicState.bufferData[VelocityVectorOffset + 0] = velocity.x();
		m_logicState.bufferData[VelocityVectorOffset + 1] = velocity.y();
		m_logicState.bufferData[VelocityVectorOffset + 2] = velocity.z();
		m_logicState.bufferData[VelocityVectorOffset + 3] = 0.0F;
	}

	void
	ViewMatricesCascadedUBO::updateAmbientLightProperties (const PixelFactory::Color< float > & color, float intensity) noexcept
	{
		m_logicState.bufferData[AmbientLightColorOffset + 0] = color.red();
		m_logicState.bufferData[AmbientLightColorOffset + 1] = color.green();
		m_logicState.bufferData[AmbientLightColorOffset + 2] = color.blue();
		m_logicState.bufferData[AmbientLightColorOffset + 3] = 1.0F;

		m_logicState.bufferData[AmbientLightIntensityOffset] = intensity;
	}

	void
	ViewMatricesCascadedUBO::computeSplitDistances (float nearPlane, float farPlane) noexcept
	{
		/* Practical split scheme: blend between logarithmic and linear splits.
		 * Formula: splitDistance[i] = lambda * log + (1 - lambda) * linear
		 * Where:
		 *   log = near * pow(far/near, p)
		 *   linear = near + (far - near) * p
		 *   p = (i + 1) / cascadeCount */

		for ( uint32_t i = 0; i < m_cascadeCount; ++i )
		{
			const auto p = static_cast< float >(i + 1) / static_cast< float >(m_cascadeCount);

			const auto logSplit = nearPlane * std::pow(farPlane / nearPlane, p);
			const auto linearSplit = nearPlane + (farPlane - nearPlane) * p;

			m_logicState.bufferData[CascadeSplitDistancesOffset + i] = m_lambda * logSplit + (1.0F - m_lambda) * linearSplit;
		}

		/* Fill remaining slots with far plane distance. */
		for ( uint32_t i = m_cascadeCount; i < MaxCascadeCount; ++i )
		{
			m_logicState.bufferData[CascadeSplitDistancesOffset + i] = farPlane;
		}
	}

	void
	ViewMatricesCascadedUBO::updateCascades (
		const Vector< 3, float > & lightDirection,
		const std::array< Vector< 3, float >, 8 > & cameraFrustumCorners,
		float nearPlane,
		float farPlane
	) noexcept
	{
		/* Recompute split distances. */
		this->computeSplitDistances(nearPlane, farPlane);

		/* For each cascade, compute the tight-fit orthographic projection. */
		float lastSplitDist = nearPlane;

		for ( uint32_t cascade = 0; cascade < m_cascadeCount; ++cascade )
		{
			const float splitDist = m_logicState.bufferData[CascadeSplitDistancesOffset + cascade];

			/* Compute the frustum corners for this cascade. */
			std::array< Vector< 3, float >, 8 > cascadeCorners{};

			/* Interpolate between near and far corners based on split distances. */
			const float nearRatio = (lastSplitDist - nearPlane) / (farPlane - nearPlane);
			const float farRatio = (splitDist - nearPlane) / (farPlane - nearPlane);

			/* Near frustum corners (indices 0-3), Far frustum corners (indices 4-7). */
			for ( size_t i = 0; i < 4; ++i )
			{
				const auto nearCorner = cameraFrustumCorners[i];
				const auto farCorner = cameraFrustumCorners[i + 4];

				/* Cascade near corners. */
				cascadeCorners[i] = nearCorner + (farCorner - nearCorner) * nearRatio;
				/* Cascade far corners. */
				cascadeCorners[i + 4] = nearCorner + (farCorner - nearCorner) * farRatio;
			}

			/* Compute tight-fit projection for this cascade. */
			m_logicState.cascadeViewProjections[cascade] = this->computeCascadeProjection(cascade, lightDirection, cascadeCorners);

			/* Update cascade frustum. */
			m_logicState.cascadeFrustums[cascade].update(m_logicState.cascadeViewProjections[cascade]);

			/* Copy matrix to buffer. */
			std::memcpy(&m_logicState.bufferData[cascade * 16], m_logicState.cascadeViewProjections[cascade].data(), 16 * sizeof(float));

			lastSplitDist = splitDist;
		}
	}

	Matrix< 4, float >
	ViewMatricesCascadedUBO::computeCascadeProjection (
		size_t /*cascadeIndex*/,
		const Vector< 3, float > & lightDirection,
		const std::array< Vector< 3, float >, 8 > & cascadeCorners
	) noexcept
	{
		/* Use CartesianFrame directly - same as classic shadow map.
		 * This guarantees identical matrix convention. */
		CartesianFrame< float > lightFrame;

		/* Set the backward vector to -lightDirection (same as classic shadow map).
		 * This makes the camera look in the light direction. */
		lightFrame.setBackwardVector(-lightDirection);

		/* Get the view matrix (rotation only, no translation yet). */
		const auto lightRotation = lightFrame.getViewMatrix();

		/* Transform all corners to light-aligned space and find bounding box. */
		float minX = std::numeric_limits< float >::max();
		float maxX = std::numeric_limits< float >::lowest();
		float minY = std::numeric_limits< float >::max();
		float maxY = std::numeric_limits< float >::lowest();
		float minZ = std::numeric_limits< float >::max();
		float maxZ = std::numeric_limits< float >::lowest();

		for ( const auto & corner : cascadeCorners )
		{
			const auto lightSpaceCorner = lightRotation * Vector< 4, float >{corner.x(), corner.y(), corner.z(), 1.0F};

			minX = std::min(minX, lightSpaceCorner.x());
			maxX = std::max(maxX, lightSpaceCorner.x());
			minY = std::min(minY, lightSpaceCorner.y());
			maxY = std::max(maxY, lightSpaceCorner.y());
			minZ = std::min(minZ, lightSpaceCorner.z());
			maxZ = std::max(maxZ, lightSpaceCorner.z());
		}

		/* Extend Z range to include shadow casters behind the camera frustum.
		 * We extend the "back" (minZ) significantly to catch objects that cast shadows
		 * into the view frustum but are themselves outside it. */
		constexpr float zBackExtension = 500.0F;
		constexpr float zFrontExtension = 10.0F;

		minZ -= zBackExtension;
		maxZ += zFrontExtension;

		/* Compute the center of the bounding box in light space. */
		const float centerX = (minX + maxX) * 0.5F;
		const float centerY = (minY + maxY) * 0.5F;
		const float centerZ = (minZ + maxZ) * 0.5F;

		/* Transform center back to world space to get camera position.
		 * For orthonormal matrices, inverse = transpose. */
		const Vector< 3, float > lightSpaceCenter{centerX, centerY, centerZ};
		auto lightRotationInverse = lightRotation; /* Copy the matrix. */
		lightRotationInverse.transpose(); /* Transpose in place. */
		const auto worldCenter = lightRotationInverse * Vector< 4, float >{lightSpaceCenter.x(), lightSpaceCenter.y(), lightSpaceCenter.z(), 1.0F};

		/* Create a new CartesianFrame with the correct position and orientation. */
		CartesianFrame< float > cascadeFrame;
		cascadeFrame.setPosition(Vector< 3, float >{worldCenter.x(), worldCenter.y(), worldCenter.z()});
		cascadeFrame.setBackwardVector(-lightDirection);

		/* Get the complete view matrix from CartesianFrame (includes translation). */
		const auto lightView = cascadeFrame.getViewMatrix();

		/* Create orthographic projection centered at origin with half-extents. */
		const float halfWidth = (maxX - minX) * 0.5F;
		const float halfHeight = (maxY - minY) * 0.5F;
		const float halfDepth = (maxZ - minZ) * 0.5F;

		const auto lightProjection = Matrix< 4, float >::orthographicProjection(
			-halfWidth, halfWidth,
			-halfHeight, halfHeight,
			-halfDepth, halfDepth
		);

		return lightProjection * lightView;
	}

	bool
	ViewMatricesCascadedUBO::create (Renderer & renderer, const std::string & instanceID) noexcept
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
			Tracer::error(ClassId, "Unable to get an uniform buffer object for cascaded view !");

			m_uniformBufferObject.reset();

			return false;
		}

		m_descriptorSet = std::make_unique< DescriptorSet >(renderer.descriptorPool(), descriptorSetLayout);
		m_descriptorSet->setIdentifier(ClassId, instanceID, "DescriptorSet");

		if ( !m_descriptorSet->create() )
		{
			m_descriptorSet.reset();

			Tracer::error(ClassId, "Unable to create the cascaded view descriptor set !");

			return false;
		}

		if ( !m_descriptorSet->writeUniformBufferObject(0, *m_uniformBufferObject) )
		{
			Tracer::error(ClassId, "Unable to setup the cascaded view descriptor set !");

			return false;
		}

		return true;
	}

	bool
	ViewMatricesCascadedUBO::updateVideoMemory (uint32_t readStateIndex) const noexcept
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
				Tracer::error(ClassId, "The cascaded view uniform buffer object is not initialized !");

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
	operator<< (std::ostream & out, const ViewMatricesCascadedUBO & obj)
	{
		out <<
			"Cascaded View matrices data : " "\n"
			"Cascade count: " << obj.m_cascadeCount << "\n"
			"Lambda: " << obj.m_lambda << "\n"
			"World position " << obj.m_logicState.position << "\n"
			"Projection " << obj.m_logicState.projection <<
			"View " << obj.m_logicState.view <<
			"Infinity view " << obj.m_logicState.infinityView <<
			"Split distances: [";

		for ( uint32_t i = 0; i < obj.m_cascadeCount; ++i )
		{
			if ( i > 0 )
			{
				out << ", ";
			}

			out << obj.m_logicState.bufferData[ViewMatricesCascadedUBO::CascadeSplitDistancesOffset + i];
		}

		out << "]" "\n";

		for ( uint32_t i = 0; i < obj.m_cascadeCount; ++i )
		{
			out << "Cascade " << i << " VP matrix: " << obj.m_logicState.cascadeViewProjections[i];
		}

		out << "Buffer data for GPU : " "\n";

		for ( size_t index = 0; index < obj.m_logicState.bufferData.size(); index += 4 )
		{
			out << '[' << obj.m_logicState.bufferData[index + 0] << ", " << obj.m_logicState.bufferData[index + 1] << ", " << obj.m_logicState.bufferData[index + 2] << ", " << obj.m_logicState.bufferData[index + 3] << "]" "\n";
		}

		return out;
	}

	std::string
	to_string (const ViewMatricesCascadedUBO & obj) noexcept
	{
		std::stringstream output;

		output << obj;

		return output.str();
	}
}