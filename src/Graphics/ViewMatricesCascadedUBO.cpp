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
		/* Update cascade count in buffer if different from default. */
		m_logicState.bufferData[CascadeCountOffset] = static_cast< float >(m_cascadeCount);
	}

	const Matrix< 4, float > &
	ViewMatricesCascadedUBO::projectionMatrix (uint32_t readStateIndex) const noexcept
	{
		if constexpr ( IsDebug )
		{
			if ( readStateIndex >= m_renderState.size() )
			{
				Tracer::error(ClassId, "Index overflow !");

				return m_logicState.projection;
			}
		}

		return m_renderState[readStateIndex].projection;
	}

	const Matrix< 4, float > &
	ViewMatricesCascadedUBO::viewMatrix (bool infinity, size_t /*index*/) const noexcept
	{
		return infinity ? m_logicState.infinityView : m_logicState.view;
	}

	const Matrix< 4, float > &
	ViewMatricesCascadedUBO::viewMatrix (uint32_t readStateIndex, bool infinity, size_t /*index*/) const noexcept
	{
		if constexpr ( IsDebug )
		{
			if ( readStateIndex >= m_renderState.size() )
			{
				Tracer::error(ClassId, "Index overflow !");

				return infinity ? m_logicState.infinityView : m_logicState.view;
			}
		}

		return infinity ? m_renderState[readStateIndex].infinityView : m_renderState[readStateIndex].view;
	}

	const Vector< 3, float > &
	ViewMatricesCascadedUBO::position (uint32_t readStateIndex) const noexcept
	{
		if constexpr ( IsDebug )
		{
			if ( readStateIndex >= m_renderState.size() )
			{
				Tracer::error(ClassId, "Index overflow !");

				return m_logicState.position;
			}
		}

		return m_renderState[readStateIndex].position;
	}

	/**
	 * @brief Returns the main frustum for a specific cascade.
	 * @return const Frustum &
	 */
	[[nodiscard]]
	const Frustum &
	ViewMatricesCascadedUBO::mainFrustum () const noexcept
	{
		return m_logicState.mainFrustum;
	}

	/**
	 * @brief Returns the main frustum for a specific cascade with render state.
	 * @param readStateIndex The render state index.
	 * @return const Frustum &
	 */
	[[nodiscard]]
	const Frustum &
	ViewMatricesCascadedUBO::mainFrustum (uint32_t readStateIndex) const noexcept
	{
		if constexpr ( IsDebug )
		{
			if ( readStateIndex >= m_renderState.size() )
			{
				Tracer::error(ClassId, "Index overflow !");

				return m_logicState.cascadeFrustums[0];
			}
		}

		return m_renderState[readStateIndex].mainFrustum;
	}

	const Frustum &
	ViewMatricesCascadedUBO::frustum (size_t index) const noexcept
	{
		if ( index >= m_cascadeCount )
		{
			Tracer::error(ClassId, "Cascade index overflow !");

			index = 0;
		}

		return m_logicState.cascadeFrustums[index];
	}

	const Frustum &
	ViewMatricesCascadedUBO::frustum (uint32_t readStateIndex, size_t index) const noexcept
	{
		if constexpr ( IsDebug )
		{
			if ( readStateIndex >= m_renderState.size() )
			{
				Tracer::error(ClassId, "Index overflow !");

				return m_logicState.cascadeFrustums[0];
			}
		}

		if ( index >= m_cascadeCount )
		{
			Tracer::error(ClassId, "Cascade index overflow !");

			index = 0;
		}

		return m_renderState[readStateIndex].cascadeFrustums[index];
	}

	float
	ViewMatricesCascadedUBO::getAspectRatio () const noexcept
	{
		if ( m_logicState.bufferData[ViewWidthOffset] * m_logicState.bufferData[ViewHeightOffset] <= 0.0F )
		{
			Tracer::error(ClassId, "View properties for width and height are invalid ! Unable to compute the aspect ratio.");

			return 1.0F;
		}

		return m_logicState.bufferData[ViewWidthOffset] / m_logicState.bufferData[ViewHeightOffset];
	}

	float
	ViewMatricesCascadedUBO::fieldOfView () const noexcept
	{
		constexpr auto Rad2Deg = HalfRevolution< float > / std::numbers::pi_v< float >;

		return std::atan(1.0F / m_logicState.projection[M4x4Col1Row1]) * 2.0F * Rad2Deg;
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
		m_logicState.bufferData[FarPlaneOffset] = distance;

		/* Formula : nearPlane = nearestObject / sqrt(1 + tan(fov/2)² · (aspectRatio² + 1)) */
		{
			const auto powA = std::pow(std::tan(Radian(fov) * 0.5F), 2.0F);
			const auto powB = std::pow(aspectRatio, 2.0F) + 1.0F;

			m_logicState.bufferData[NearPlaneOffset] = 0.1F / std::sqrt(1.0F + powA * powB);
		}

		m_logicState.projection = Matrix< 4, float >::perspectiveProjection(fov, aspectRatio, m_logicState.bufferData[NearPlaneOffset], m_logicState.bufferData[FarPlaneOffset]);

		/* Recompute split distances when view properties change. */
		this->computeSplitDistances(m_logicState.bufferData[NearPlaneOffset], distance);
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
		m_logicState.bufferData[NearPlaneOffset] = nearDistance;
		m_logicState.bufferData[FarPlaneOffset] = farDistance;

		/* NOTE: Compute the side AFTER updating ViewDistanceOffset to use the new farDistance value.
		 * The side represents half the width/height of the orthographic frustum, scaled by aspect ratio. */
		const auto side = m_logicState.bufferData[FarPlaneOffset] * this->getAspectRatio();

		m_logicState.projection = Matrix< 4, float >::orthographicProjection(
			-side, side,
			-side, side,
			m_logicState.bufferData[NearPlaneOffset], m_logicState.bufferData[FarPlaneOffset]
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
		m_logicState.mainFrustum.update(m_logicState.projection * m_logicState.view);

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
	ViewMatricesCascadedUBO::setCascadeCount (uint32_t count) noexcept
	{
		m_cascadeCount = std::clamp(count, 1U, MaxCascadeCount);
		m_logicState.bufferData[CascadeCountOffset] = static_cast< float >(m_cascadeCount);
	}

	void
	ViewMatricesCascadedUBO::setLambda (float value) noexcept
	{
		m_lambda = std::clamp(value, 0.0F, 1.0F);
	}

	float
	ViewMatricesCascadedUBO::splitDistance (size_t cascadeIndex) const noexcept
	{
		if ( cascadeIndex >= m_cascadeCount )
		{
			return m_logicState.bufferData[FarPlaneOffset];
		}

		return m_logicState.bufferData[CascadeSplitDistancesOffset + cascadeIndex];
	}

	const Matrix< 4, float > &
	ViewMatricesCascadedUBO::cascadeViewProjectionMatrix (size_t cascadeIndex) const noexcept
	{
		if ( cascadeIndex >= m_cascadeCount )
		{
			Tracer::error(ClassId, "Cascade index overflow !");

			cascadeIndex = 0;
		}

		return m_logicState.cascadeViewProjections[cascadeIndex];
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
	ViewMatricesCascadedUBO::updateFromMainCameraFrustum (const Vector< 3, float > & lightDirection, const std::array< Vector< 3, float >, 8 > & cameraFrustumCorners, float nearPlane, float farPlane) noexcept
	{
		/* Store the camera's near/far planes for viewDistance() and frustum culling.
		 * CSM derives its coverage from the camera frustum. */
		m_logicState.bufferData[NearPlaneOffset] = nearPlane;
		m_logicState.bufferData[FarPlaneOffset] = farPlane;

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
			for ( size_t index = 0; index < 4; ++index )
			{
				const auto nearCorner = cameraFrustumCorners[index];
				const auto farCorner = cameraFrustumCorners[index + 4];

				/* Cascade near corners. */
				cascadeCorners[index] = nearCorner + (farCorner - nearCorner) * nearRatio;
				/* Cascade far corners. */
				cascadeCorners[index + 4] = nearCorner + (farCorner - nearCorner) * farRatio;
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
	ViewMatricesCascadedUBO::computeCascadeProjection (size_t /*cascadeIndex*/, const Vector< 3, float > & lightDirection, const std::array< Vector< 3, float >, 8 > & cascadeCorners) noexcept
	{
		/* Step 1: Compute the center of the frustum slice (centroid of 8 corners). */
		Vector< 3, float > frustumCenter{0.0F, 0.0F, 0.0F};

		for ( const auto & corner : cascadeCorners )
		{
			frustumCenter += corner;
		}

		frustumCenter /= 8.0F;

		/* Step 2: Build light view matrix centered on frustum. */
		CartesianFrame< float > cascadeFrame;
		cascadeFrame.setBackwardVector(-lightDirection);

		/* Position light camera far enough behind to see the whole frustum. */
		const auto lightView = cascadeFrame.getViewMatrix();

		/* Step 3: Transform corners to light space and find AABB. */
		float minX = std::numeric_limits< float >::max();
		float maxX = std::numeric_limits< float >::lowest();
		float minY = std::numeric_limits< float >::max();
		float maxY = std::numeric_limits< float >::lowest();
		float minZ = std::numeric_limits< float >::max();
		float maxZ = std::numeric_limits< float >::lowest();

		for ( const auto & corner : cascadeCorners )
		{
			const auto lightSpaceCorner = lightView * Vector< 4, float >(corner.x(), corner.y(), corner.z(), 1.0F);

			minX = std::min(minX, lightSpaceCorner.x());
			maxX = std::max(maxX, lightSpaceCorner.x());
			minY = std::min(minY, lightSpaceCorner.y());
			maxY = std::max(maxY, lightSpaceCorner.y());
			minZ = std::min(minZ, lightSpaceCorner.z());
			maxZ = std::max(maxZ, lightSpaceCorner.z());
		}

		/* Step 4: Build tight orthographic projection from AABB.
		 * Add margin for shadow casters behind the frustum. */
		constexpr float zMargin = 100.0F;

		const auto lightProjection = Matrix< 4, float >::orthographicProjection(
			minX, maxX,
			minY, maxY,
			minZ - zMargin, maxZ + zMargin
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

	void
	ViewMatricesCascadedUBO::publishStateForRendering (uint32_t writeStateIndex) noexcept
	{
		if constexpr ( IsDebug )
		{
			if ( writeStateIndex >= m_renderState.size() )
			{
				Tracer::error(ClassId, "Index overflow !");

				return;
			}
		}

		m_renderState[writeStateIndex] = m_logicState;
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

	void
	ViewMatricesCascadedUBO::destroy () noexcept
	{
		/* [VULKAN-CPU-SYNC] Maybe useless */
		/* NOTE: Lock between updateVideoMemory() and destroy(). */
		const std::lock_guard< std::mutex > lock{m_GPUBufferAccessLock};

		m_descriptorSet.reset();
		m_uniformBufferObject.reset();
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