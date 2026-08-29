/*
 * src/Graphics/ViewMatrices3DUBO.cpp
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

#include "ViewMatrices3DUBO.hpp"

#include <string>

#include <algorithm>

/* STL inclusions. */
#include <cmath>
#include <cstring>

/* Local inclusions. */
#include "Graphics/Renderer.hpp"
#include "Tracer.hpp"

namespace EmEn::Graphics
{
	using namespace Base;
	using namespace Base::Math;
	using namespace Vulkan;

	ViewMatrices3DUBO::ViewMatrices3DUBO () noexcept = default;

	ViewMatrices3DUBO::~ViewMatrices3DUBO () = default;

	/* Face capture orientations — the camera for each face of a cubemap RENDER TARGET.
	 *
	 * ⚠️⚠️ THE THREE PIECES BELOW ARE ONE MECHANISM. Change one alone and the cubemap breaks:
	 *   1. this table: each face looks along ITS OWN axis, with NEGATED up vectors
	 *      (sides -Y, the +Y pole +Z, the -Y pole -Z);
	 *   2. updatePerspectiveViewProperties(): the Y-up projection flip is UNDONE for cube faces;
	 *   3. GraphicsPipeline::configureRasterizationState(..., mirroredViewport = isCubemap()):
	 *      the front face is inverted.
	 *
	 * WHY, and it is not negotiable: the cube-face convention is LEFT-handed — a face wants
	 * `right x up = +look` (see the (dx,dy,dz) tables in CubemapResource / IBLBaker) — while a camera
	 * is right-handed and gives `right x up = -look`. MEASURED: with honest look/up vectors, all six
	 * faces come out with EXACTLY the opposite right vector. No choice of look/up can repair that,
	 * because changing `up` only ROTATES a face (up -> -up gives right -> -right AND up -> -up,
	 * i.e. a 180 degree turn) and never MIRRORS it. Handedness is invariant under that reparameter-
	 * isation, so the fix has to be a reflection — items 2 and 3 above. Undoing the projection Y flip
	 * supplies the second mirror (mirror + mirror = a 180 degree rotation, orientation-preserving),
	 * and the negated ups in this table cancel that rotation. Item 3 pays for the winding the
	 * reflection reverses.
	 *
	 * ⚠️ Before the Y-up flip this table was ALSO half-migrated: the Y+ row looked at -Y, the Y- row
	 * at +Y, and both Z rows carried a "(direction swapped)" comment. Those compensated the Y-down
	 * world and are gone.
	 *
	 * THE THREE SYMPTOMS, in the order they were peeled off in `reflexion-debug --demo-options 0,3,0`
	 * (mode 3 = CameraOnce, a probe baked once — the only mode where left/right is READABLE, because
	 * a reflected sky has no landmark):
	 *   - wrong face axes  -> GROUND at the top of the sphere, SKY at the bottom, faces not joining;
	 *   - missing mirror   -> palm TRUNK left but its CROWN crossed to the right (the crown lands on
	 *                         the +Y pole face, whose mirror axis is X: a GLOBAL mirror would have
	 *                         moved the whole tree, a per-face one cuts it in two);
	 *   - missing item 3   -> geometry correctly PLACED but the cubemap mostly BLACK (culling).
	 * Correct: palm whole on the left, dragon on the right, continuous horizon, no black.
	 *
	 * ⚠️⚠️ This mechanism feeds EVERY cubemap render target — reflection probes AND point-light
	 * shadow cubemaps. Do not tune it against one of them alone. `light-and-shadow-debug` measured
	 * 3329 differing pixels out of 4665600 across the change: shadows are unaffected.
	 *
	 * ⚠️⚠️ THAT LAST SENTENCE IS TRUE AND IT MISLED US FOR WEEKS. It says this change did not move
	 * the shadows; it does NOT say the shadows were CORRECT — and they were not. Only the RENDER
	 * is shared with the probes. The point-light shadow LOOKUP lives in
	 * `LightGenerator::generate3DShadowMapCode()` and is shared with nothing, and it still carried
	 * an X-only negation from 0.8.5, i.e. a Y-DOWN-era compensation: on `global-illumination` the
	 * walking paladin cast his shadow on the CEILING. Fixed Aug 2026 by using the plain
	 * light-to-fragment direction. Lesson: validating the cubemap on probes proves nothing about
	 * shadows, and a measurement that only shows "X did not change Y" is not a verification that
	 * Y is right. */
	const std::array< Matrix< 4, float >, CubemapFaceIndexes.size() > ViewMatrices3DUBO::CubemapOrientation{
		Matrix< 4, float >::lookAt(Vector< 3, float >{0.0F, 0.0F, 0.0F}, Vector< 3, float >{ 1.0F,  0.0F,  0.0F}, Vector< 3, float >{ 0.0F, -1.0F,  0.0F}), // X+
		Matrix< 4, float >::lookAt(Vector< 3, float >{0.0F, 0.0F, 0.0F}, Vector< 3, float >{-1.0F,  0.0F,  0.0F}, Vector< 3, float >{ 0.0F, -1.0F,  0.0F}), // X-
		Matrix< 4, float >::lookAt(Vector< 3, float >{0.0F, 0.0F, 0.0F}, Vector< 3, float >{ 0.0F,  1.0F,  0.0F}, Vector< 3, float >{ 0.0F,  0.0F,  1.0F}), // Y+
		Matrix< 4, float >::lookAt(Vector< 3, float >{0.0F, 0.0F, 0.0F}, Vector< 3, float >{ 0.0F, -1.0F,  0.0F}, Vector< 3, float >{ 0.0F,  0.0F, -1.0F}), // Y-
		Matrix< 4, float >::lookAt(Vector< 3, float >{0.0F, 0.0F, 0.0F}, Vector< 3, float >{ 0.0F,  0.0F,  1.0F}, Vector< 3, float >{ 0.0F, -1.0F,  0.0F}), // Z+
		Matrix< 4, float >::lookAt(Vector< 3, float >{0.0F, 0.0F, 0.0F}, Vector< 3, float >{ 0.0F,  0.0F, -1.0F}, Vector< 3, float >{ 0.0F, -1.0F,  0.0F}) // Z-
	};

	const Matrix< 4, float > &
	ViewMatrices3DUBO::projectionMatrix (uint32_t readStateIndex) const noexcept
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
	ViewMatrices3DUBO::viewMatrix (bool infinity, size_t index) const noexcept
	{
		if ( index >= CubemapFaceCount )
		{
			Tracer::error(ClassId, "Index overflow !");

			index = 0;
		}

		return infinity ? m_logicState.infinityViews[index] : m_logicState.views[index];
	}

	const Matrix< 4, float > &
	ViewMatrices3DUBO::viewMatrix (uint32_t readStateIndex, bool infinity, size_t index) const noexcept
	{
		if constexpr ( IsDebug )
		{
			if ( index >= CubemapFaceCount )
			{
				Tracer::error(ClassId, "Index overflow !");

				index = 0;
			}

			if ( readStateIndex >= m_renderState.size() )
			{
				Tracer::error(ClassId, "Index overflow !");

				return infinity ? m_logicState.infinityViews[index] : m_logicState.views[index];
			}
		}

		return infinity ? m_renderState[readStateIndex].infinityViews[index] : m_renderState[readStateIndex].views[index];
	}

	const Vector< 3, float > &
	ViewMatrices3DUBO::position (uint32_t readStateIndex) const noexcept
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

	const Frustum &
	ViewMatrices3DUBO::frustum (size_t index) const noexcept
	{
		if ( index >= CubemapFaceCount )
		{
			Tracer::error(ClassId, "Index overflow !");

			index = 0;
		}

		return m_logicState.frustums[index];
	}

	const Frustum &
	ViewMatrices3DUBO::frustum (uint32_t readStateIndex, size_t index) const noexcept
	{
		if constexpr ( IsDebug )
		{
			if ( index >= CubemapFaceCount )
			{
				Tracer::error(ClassId, "Index overflow !");

				index = 0;
			}

			if ( readStateIndex >= m_renderState.size() )
			{
				Tracer::error(ClassId, "Index overflow !");

				return m_logicState.frustums[index];
			}
		}

		return m_renderState[readStateIndex].frustums[index];
	}

	float
	ViewMatrices3DUBO::fieldOfView () const noexcept
	{
		using namespace Base::Math;

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
	ViewMatrices3DUBO::updatePerspectiveViewProperties (float width, float height, float fov, float distance) noexcept
	{
		if ( width * height <= 0.0 )
		{
			TraceError{ClassId} << "The view size (" << width << " X " << height << ") is invalid!";

			return;
		}

		m_logicState.bufferData[ViewWidthOffset] = width;
		m_logicState.bufferData[ViewHeightOffset] = height;
		m_logicState.bufferData[FarPlaneOffset] = distance;
		/* A cube face is square: aspectRatio 1 makes the bracket 2, which is what the hand-written
		 * `powA * 2.0F` was. */
		m_logicState.bufferData[NearPlaneOffset] = ViewMatricesInterface::computeNearPlane(m_nearestObjectDistance, QuartRevolution< float >, 1.0F);

		m_logicState.projection = Matrix< 4, float >::perspectiveProjection(QuartRevolution< float >, 1.0F, m_logicState.bufferData[NearPlaneOffset], m_logicState.bufferData[FarPlaneOffset]);

		/* ⚠️ Undo the Y-up projection flip — for CUBE FACES ONLY, and never in isolation.
		 * This is piece 2 of the three-part mechanism documented on CubemapOrientation above: it
		 * supplies the mirror the left-handed cube-face convention needs, the negated up vectors in
		 * that table cancel the resulting 180 degree rotation, and the front-face inversion in
		 * GraphicsPipeline pays for the reversed winding. Remove any one of the three and the
		 * captured cubemap is mirrored, upside down or black. */
		m_logicState.projection[M4x4Col1Row1] = -m_logicState.projection[M4x4Col1Row1];


		/*TraceDebug{ClassId} <<
			"Perspective projection:" "\n"
			"Size: " << width << " X " << height << "\n"
			"Distance: " << distance << "\n"
			"Field of view: " << fov << "\n"
			"Matrix: " << m_logicState.projection;*/

		std::memcpy(&m_logicState.bufferData[ProjectionMatrixOffset], m_logicState.projection.data(), Matrix4Alignment * sizeof(float));
	}

	void
	ViewMatrices3DUBO::updateOrthographicViewProperties (float width, float height, float nearDistance, float farDistance) noexcept
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

		m_logicState.projection = Matrix< 4, float >::orthographicProjection(
			-m_logicState.bufferData[FarPlaneOffset], m_logicState.bufferData[FarPlaneOffset],
			-m_logicState.bufferData[FarPlaneOffset], m_logicState.bufferData[FarPlaneOffset],
			m_logicState.bufferData[NearPlaneOffset], m_logicState.bufferData[FarPlaneOffset]
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
	ViewMatrices3DUBO::updateViewCoordinates (const CartesianFrame< float > & coordinates, const Vector< 3, float > & velocity) noexcept
	{
		m_logicState.position = coordinates.position();

		for ( auto face : CubemapFaceIndexes )
		{
			const auto faceIndex = static_cast< size_t >(face);

			m_logicState.views.at(faceIndex) = CubemapOrientation.at(faceIndex) * Matrix< 4, float >::translation(-m_logicState.position);
			m_logicState.infinityViews.at(faceIndex) = CubemapOrientation.at(faceIndex) * Matrix< 4, float >::translation(-m_logicState.position);
			m_logicState.frustums.at(faceIndex).update(m_logicState.projection * m_logicState.views.at(faceIndex));

			/* Copy view matrix to buffer data for GPU upload. */
			std::memcpy(&m_logicState.bufferData[faceIndex * Matrix4Alignment], m_logicState.views.at(faceIndex).data(), Matrix4Alignment * sizeof(float));
		}

		m_logicState.bufferData[WorldPositionOffset + 0] = m_logicState.position.x();
		m_logicState.bufferData[WorldPositionOffset + 1] = m_logicState.position.y();
		m_logicState.bufferData[WorldPositionOffset + 2] = m_logicState.position.z();

		m_logicState.bufferData[VelocityVectorOffset + 0] = velocity.x();
		m_logicState.bufferData[VelocityVectorOffset + 1] = velocity.y();
		m_logicState.bufferData[VelocityVectorOffset + 2] = velocity.z();
	}

	void
	ViewMatrices3DUBO::updateAmbientLightProperties (const PixelFactory::Color< float > & color, float intensity, float environmentLuminance, float IBLDiffuseWeight) noexcept
	{
		m_logicState.bufferData[AmbientLightColorOffset+0] = color.red();
		m_logicState.bufferData[AmbientLightColorOffset+1] = color.green();
		m_logicState.bufferData[AmbientLightColorOffset+2] = color.blue();

		m_logicState.bufferData[AmbientLightIntensityOffset] = intensity;
		m_logicState.bufferData[EnvironmentLuminanceOffset] = environmentLuminance;
		m_logicState.bufferData[IBLDiffuseWeightOffset] = IBLDiffuseWeight;
	}

	bool
	ViewMatrices3DUBO::create (Renderer & renderer, const std::string & instanceID, uint32_t frameCount) noexcept
	{
		const auto descriptorSetLayout = RenderTarget::Abstract::getDescriptorSetLayout(renderer.layoutManager());

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
				TraceError{ClassId} << "Unable to get an uniform buffer object for the cubemap view region #" << frameIndex << " !";

				m_uniformBufferObjects.clear();
				m_descriptorSets.clear();

				return false;
			}

			auto descriptorSet = std::make_unique< DescriptorSet >(renderer.descriptorPool(), descriptorSetLayout);
			descriptorSet->setIdentifier(ClassId, regionID, "DescriptorSet");

			if ( !descriptorSet->create() || !descriptorSet->writeUniformBufferObject(0, *uniformBufferObject) )
			{
				TraceError{ClassId} << "Unable to set up the cubemap view descriptor set for region #" << frameIndex << " !";

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
	ViewMatrices3DUBO::publishStateForRendering (uint32_t writeStateIndex) noexcept
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
	ViewMatrices3DUBO::updateVideoMemory (uint32_t readStateIndex, uint32_t frameIndex) const noexcept
	{
		if constexpr ( IsDebug )
		{
			if ( readStateIndex >= m_renderState.size() )
			{
				Tracer::error(ClassId, "Index overflow !");

				return false;
			}

		}

		/* [VULKAN-CPU-SYNC] Maybe useless */
		/* NOTE: Lock between updateVideoMemory() and destroy(). */
		const std::lock_guard< std::mutex > lock{m_memoryAccess};

		if ( frameIndex >= m_uniformBufferObjects.size() )
		{
			TraceError{ClassId} << "Frame region overflow: asked for region #" << frameIndex << " but only " << m_uniformBufferObjects.size() << " exist !";

			return false;
		}

		/* ⚠️ Published BEFORE the mapping: every bind of this frame reads it. */
		m_currentFrameRegion = frameIndex;

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
	ViewMatrices3DUBO::destroy () noexcept
	{
		/* [VULKAN-CPU-SYNC] Maybe useless */
		/* NOTE: Lock between updateVideoMemory() and destroy(). */
		const std::lock_guard< std::mutex > lock{m_memoryAccess};

		m_descriptorSets.clear();
		m_uniformBufferObjects.clear();
	}

	std::ostream &
	operator<< (std::ostream & out, const ViewMatrices3DUBO & obj)
	{
		out <<
			"3D View matrices data : " "\n"
			"World position " << obj.m_logicState.position << "\n"
			"Projection " << obj.m_logicState.projection;

		for ( uint32_t viewIndex = 0; viewIndex < CubemapFaceCount; ++viewIndex )
		{
			out << "Face #" << viewIndex << "\n"
				"\t" "View " << obj.m_logicState.views[viewIndex] <<
				"\t" "Infinity view " << obj.m_logicState.infinityViews[viewIndex] <<
				"\t" << obj.m_logicState.frustums[viewIndex];
		}

		out << "Buffer data for GPU : " "\n";

		for ( size_t index = 0; index < obj.m_logicState.bufferData.size(); index += 4 )
		{
			out << '[' << obj.m_logicState.bufferData[index+0] << ", " << obj.m_logicState.bufferData[index+1] << ", " << obj.m_logicState.bufferData[index+2] << ", " << obj.m_logicState.bufferData[index+3] << "]" "\n";
		}

		return out;
	}

	std::string
	to_string (const ViewMatrices3DUBO & obj) noexcept
	{
		std::stringstream output;

		output << obj;

		return output.str();
	}
}
