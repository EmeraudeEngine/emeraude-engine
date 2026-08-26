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
 * https://github.com/EmeraudeEngine/emeraude-engine
 *
 * --- THIS IS AUTOMATICALLY GENERATED, DO NOT CHANGE ---
 */

#include "ViewMatricesCascadedUBO.hpp"

#include <string>

#include <algorithm>

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
	using namespace Base;
	using namespace Base::Math;
	using namespace Vulkan;

	ViewMatricesCascadedUBO::~ViewMatricesCascadedUBO () = default;

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

		/* ⚠️ std::abs() is a NO-OP today and load-bearing tomorrow. This recovers the FOV from the
		 * matrix and SceneRenderTarget::setViewDistance() feeds it straight back into
		 * perspectiveProjection(), which rebuilds that same matrix. The Y-up migration makes
		 * [Col1Row1] NEGATIVE; without abs() the recovered FOV is negative, cotan(fov/2) is
		 * negative, and -a comes out POSITIVE again — the flip would silently undo itself on the
		 * first view-distance change, on every resize path. Do not remove. */
		return std::atan(1.0F / std::abs(m_logicState.projection[M4x4Col1Row1])) * 2.0F * Rad2Deg;
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
	ViewMatricesCascadedUBO::updateOrthographicViewProperties (float width, float height, float /*nearDistance*/, float /*farDistance*/) noexcept
	{
		if ( width * height <= 0.0 )
		{
			TraceError{ClassId} << "The view size (" << width << " X " << height << ") is invalid!";

			return;
		}

		m_logicState.bufferData[ViewWidthOffset] = width;
		m_logicState.bufferData[ViewHeightOffset] = height;

		/* ⚠️⚠️ A CASCADED VIEW HAS NO SINGLE VIEW RANGE. Its near/far and its projections belong to
		 * updateFromMainCameraFrustum(), which refits them to the MAIN CAMERA on every logic tick.
		 * The only caller that reaches here is the light connexion
		 * (AbstractLightEmitter::onOutputDeviceConnected → ShadowMap::updateViewRangesProperties), and a
		 * directional light hands it DUMMIES: getFovOrNear() is a hard-coded 0.0 and getDistanceOrFar()
		 * returns a hard-coded 1.0 in CSM mode, saying so in its own comment.
		 * Accepting them poisoned the state twice:
		 *   - computeSplitDistances(0.0, 1.0) evaluates near * pow(far / near, p) = 0 * pow(+inf, p),
		 *     i.e. 0 * inf = NaN, in EVERY split slot — uploaded as-is on the first frames, where the
		 *     shader's `viewDepth < splitDistances[i]` is false against a NaN and every fragment falls
		 *     through to the last cascade;
		 *   - m_logicState.projection became a 1 × 1 box over [0, 1] and stayed there for the process
		 *     lifetime, because updateFromMainCameraFrustum() never writes it, leaving mainFrustum()
		 *     (built from it in updateViewCoordinates()) permanently garbage.
		 * The size IS meaningful and is kept: it is the shadow map extent, and getAspectRatio() reads it.
		 * The range is dropped on purpose — do not "restore" it. */
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
	ViewMatricesCascadedUBO::updateAmbientLightProperties (const PixelFactory::Color< float > & color, float intensity, float environmentLuminance) noexcept
	{
		m_logicState.bufferData[AmbientLightColorOffset + 0] = color.red();
		m_logicState.bufferData[AmbientLightColorOffset + 1] = color.green();
		m_logicState.bufferData[AmbientLightColorOffset + 2] = color.blue();
		m_logicState.bufferData[AmbientLightColorOffset + 3] = 1.0F;

		m_logicState.bufferData[AmbientLightIntensityOffset] = intensity;
		m_logicState.bufferData[EnvironmentLuminanceOffset] = environmentLuminance;
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
	ViewMatricesCascadedUBO::computeCascadeProjection (size_t /*cascadeIndex*/, const Vector< 3, float > & lightDirection, const std::array< Vector< 3, float >, 8 > & cascadeCorners) const noexcept
	{
		/* ⚠️⚠️ STABILITY IS THE POINT OF THIS FUNCTION, not tightness.
		 * A cascade refit to the camera frustum every logic tick with a light-space AABB and no
		 * snapping makes every shadow edge re-quantise on every tick: the edges crawl and shimmer
		 * while the camera moves and freeze the instant it stops. Two independent causes, one fix:
		 *   - an AABB of a rotating frustum slice is NOT rotation-invariant, so the world size of a
		 *     texel changed as the camera merely turned;
		 *   - nothing aligned the light-space origin to the texel grid, so the whole grid slid
		 *     continuously under the geometry.
		 * The remedy is the standard one: fit each slice with its BOUNDING SPHERE — whose radius is
		 * invariant under camera rotation and constant for a fixed FOV and split — then round the
		 * light-space centre down to a whole texel.
		 * References (technique only, nothing integrated):
		 *   - Michal Valient, "Stable Rendering of Cascaded Shadow Maps", ShaderX6 (2008) — the
		 *     origin of the sphere fit plus texel snapping.
		 *   - Microsoft, "Cascaded Shadow Maps" D3D sample —
		 *     https://learn.microsoft.com/en-us/windows/win32/dxtecharts/cascaded-shadow-maps
		 *     documents fit-to-scene vs fit-to-cascade and the same snapping trick.
		 * Cost, stated honestly: a sphere wastes map area against a tight AABB — up to ~1 - 2/π per
		 * axis in the worst case. It is bought back with cascadeScale, and an edge that holds still
		 * is worth more than texels spent on an edge that crawls. */

		/* Step 1: the slice centroid — computed here since the very first revision, and never used
		 * until now, while a comment three lines below claimed the light camera was positioned. */
		Vector< 3, float > frustumCenter{0.0F, 0.0F, 0.0F};

		for ( const auto & corner : cascadeCorners )
		{
			frustumCenter += corner;
		}

		frustumCenter /= 8.0F;

		/* Step 2: the enclosing sphere. Its radius depends only on the slice geometry, so it does
		 * not change when the camera turns — which is exactly what the AABB failed to guarantee. */
		float radius = 0.0F;

		for ( const auto & corner : cascadeCorners )
		{
			radius = std::max(radius, Vector< 3, float >::distance(corner, frustumCenter));
		}

		/* Quantise the radius so floating-point wobble in the split maths cannot make the texel size
		 * breathe from one tick to the next — a moving texel size defeats the snapping below. */
		radius = std::ceil(radius * 16.0F) / 16.0F;

		if ( radius <= 0.0F )
		{
			return Matrix< 4, float >{};
		}

		/* Step 3: snap the centre to a whole texel, in the light's own frame.
		 * The rotation-only frame is what defines the texel grid; snapping in it and only then
		 * translating is what keeps the grid still while the camera moves. */
		CartesianFrame< float > rotationFrame;
		rotationFrame.setBackwardVector(-lightDirection);

		const auto lightRotation = rotationFrame.getViewMatrix();

		const auto resolution = m_logicState.bufferData[ViewWidthOffset];

		if ( resolution > 0.0F )
		{
			const auto worldUnitsPerTexel = (2.0F * radius) / resolution;

			const auto centerLightSpace = lightRotation * Vector< 4, float >(frustumCenter.x(), frustumCenter.y(), frustumCenter.z(), 1.0F);

			const auto snappedLightSpace = Vector< 4, float >{
				std::floor(centerLightSpace.x() / worldUnitsPerTexel) * worldUnitsPerTexel,
				std::floor(centerLightSpace.y() / worldUnitsPerTexel) * worldUnitsPerTexel,
				centerLightSpace.z(),
				1.0F
			};

			const auto snappedWorld = lightRotation.inverse() * snappedLightSpace;

			frustumCenter = Vector< 3, float >{snappedWorld.x(), snappedWorld.y(), snappedWorld.z()};
		}
		else
		{
			Tracer::error(ClassId, "The cascaded view has no resolution ! Texel snapping is disabled and the shadow edges WILL crawl.");
		}

		/* Step 4: anchor the light camera on the snapped centre, pushed back far enough that the
		 * whole sphere sits in front of it, plus a margin for casters standing BETWEEN the light and
		 * the slice. The margin is a fraction of the cascade rather than a fixed number of metres:
		 * the previous hard-coded 100 m was roughly five times too large for cascade 0 and far too
		 * small to mean anything for cascade 3, and it made the depth range breathe with the fit.
		 * With margin = radius the depth range is exactly 3 * radius, a pure function of the split. */
		const auto casterMargin = radius;

		CartesianFrame< float > cascadeFrame;
		cascadeFrame.setBackwardVector(-lightDirection);
		cascadeFrame.setPosition(frustumCenter - lightDirection * (radius + casterMargin));

		const auto lightView = cascadeFrame.getViewMatrix();

		/* ⚠️ orthographicProjection() takes positive DISTANCES from the camera, not view-space Z
		 * coordinates — mixing the two once put whole cascades outside [0,1], clipping every caster
		 * out of the map and failing the sampling guard, so no shadow at all. Anchoring the camera
		 * ourselves makes the range trivial and removes that trap entirely: the near plane is the
		 * camera itself, the far plane is the far side of the sphere plus the margin. */
		const auto lightProjection = Matrix< 4, float >::orthographicProjection(
			-radius, radius,
			-radius, radius,
			0.0F, (2.0F * radius) + casterMargin
		);

		return lightProjection * lightView;
	}

	bool
	ViewMatricesCascadedUBO::create (Renderer & renderer, const std::string & instanceID, uint32_t frameCount) noexcept
	{
		auto descriptorSetLayout = RenderTarget::Abstract::getDescriptorSetLayout(renderer.layoutManager());

		if ( descriptorSetLayout == nullptr )
		{
			return false;
		}

		const auto regionCount = std::max(1U, frameCount);

		m_uniformBufferObjects.reserve(regionCount);
		m_descriptorSets.reserve(regionCount);

		for ( uint32_t frameIndex = 0; frameIndex < regionCount; ++frameIndex )
		{
			const auto regionID = instanceID + "Frame" + std::to_string(frameIndex);

			auto uniformBufferObject = std::make_unique< UniformBufferObject >(renderer.device(), ViewUBOSize);
			uniformBufferObject->setIdentifier(ClassId, regionID, "UniformBufferObject");

			if ( !uniformBufferObject->createOnHardware() )
			{
				TraceError{ClassId} << "Unable to get an uniform buffer object for cascaded view region #" << frameIndex << " !";

				m_uniformBufferObjects.clear();
				m_descriptorSets.clear();

				return false;
			}

			auto descriptorSet = std::make_unique< DescriptorSet >(renderer.descriptorPool(), descriptorSetLayout);
			descriptorSet->setIdentifier(ClassId, regionID, "DescriptorSet");

			if ( !descriptorSet->create() )
			{
				TraceError{ClassId} << "Unable to create the cascaded view descriptor set for region #" << frameIndex << " !";

				m_uniformBufferObjects.clear();
				m_descriptorSets.clear();

				return false;
			}

			if ( !descriptorSet->writeUniformBufferObject(0, *uniformBufferObject) )
			{
				TraceError{ClassId} << "Unable to setup the cascaded view descriptor set for region #" << frameIndex << " !";

				m_uniformBufferObjects.clear();
				m_descriptorSets.clear();

				return false;
			}

			m_uniformBufferObjects.emplace_back(std::move(uniformBufferObject));
			m_descriptorSets.emplace_back(std::move(descriptorSet));
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
	ViewMatricesCascadedUBO::updateVideoMemory (uint32_t readStateIndex, uint32_t frameIndex) const noexcept
	{
		if ( readStateIndex >= m_renderState.size() || frameIndex >= m_uniformBufferObjects.size() )
		{
			Tracer::error(ClassId, "Index overflow !");

			return false;
		}

		/* ⚠️ Published BEFORE any early return: descriptorSet() reads it for every bind of this
		 * frame, and a stale value would address a region another frame-in-flight is still using. */
		m_currentFrameRegion = frameIndex;

		/* [VULKAN-CPU-SYNC] Maybe useless */
		/* NOTE: Lock between updateVideoMemory() and destroy(). */
		const std::lock_guard< std::mutex > lock{m_GPUBufferAccessLock};

		auto & uniformBufferObject = m_uniformBufferObjects[frameIndex];

		auto * pointer = uniformBufferObject->mapMemoryAs< float >(0, VK_WHOLE_SIZE);

		if ( pointer == nullptr )
		{
			return false;
		}

		std::memcpy(pointer, m_renderState[readStateIndex].bufferData.data(), m_renderState[readStateIndex].bufferData.size() * sizeof(float));

		uniformBufferObject->unmapMemory(0, VK_WHOLE_SIZE);

		return true;
	}

	void
	ViewMatricesCascadedUBO::destroy () noexcept
	{
		/* [VULKAN-CPU-SYNC] Maybe useless */
		/* NOTE: Lock between updateVideoMemory() and destroy(). */
		const std::lock_guard< std::mutex > lock{m_GPUBufferAccessLock};

		m_descriptorSets.clear();
		m_uniformBufferObjects.clear();
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
