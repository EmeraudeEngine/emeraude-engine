/*
 * src/Graphics/ViewMatricesCascadedUBO.hpp
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

#pragma once

/* Project configuration. */
#include "emeraude_config.hpp"

/* STL inclusions. */
#include <cstddef>
#include <array>
#include <memory>
#include <mutex>

/* Local inclusions for inheritances. */
#include "ViewMatricesInterface.hpp"

/* Local inclusions for usages. */
#include "Vulkan/UniformBufferObject.hpp"
#include "Graphics/Types.hpp"
#include "Tracer.hpp"

namespace EmEn::Graphics
{
	/** @brief Maximum number of shadow map cascades supported. */
	static constexpr uint32_t MaxCascadeCount{4};
	/** @brief Default lambda value for cascade split calculation (0.5 = balanced log/linear). */
	static constexpr float DefaultCascadeLambda{0.5F};

	/**
	 * @brief Specialization of view matrices for cascaded shadow map rendering.
	 * @extends EmEn::Graphics::ViewMatricesInterface
	 * @note This class manages multiple view-projection matrices for Cascaded Shadow Maps (CSM).
	 *       Each cascade covers a different depth range of the camera frustum, providing
	 *       higher shadow resolution near the camera and lower resolution far away.
	 */
	class ViewMatricesCascadedUBO final : public ViewMatricesInterface
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"ViewMatricesCascadedUBO"};

			/**
			 * @brief Creates cascaded view matrices.
			 * @param cascadeCount The number of cascades (1-4). Default is 4.
			 * @param lambda The split factor (0 = linear, 1 = logarithmic, 0.5 = balanced). Default is 0.5.
			 */
			explicit ViewMatricesCascadedUBO (uint32_t cascadeCount = MaxCascadeCount, float lambda = DefaultCascadeLambda) noexcept;

			/** @copydoc EmEn::Graphics::ViewMatricesInterface::projectionMatrix() const */
			[[nodiscard]]
			const Libs::Math::Matrix< 4, float > &
			projectionMatrix () const noexcept override
			{
				return m_logicState.projection;
			}

			/** @copydoc EmEn::Graphics::ViewMatricesInterface::projectionMatrix(uint32_t) const */
			[[nodiscard]]
			const Libs::Math::Matrix< 4, float > &
			projectionMatrix (uint32_t readStateIndex) const noexcept override
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

			/** @copydoc EmEn::Graphics::ViewMatricesInterface::viewMatrix(bool, size_t) const */
			[[nodiscard]]
			const Libs::Math::Matrix< 4, float > &
			viewMatrix (bool infinity, size_t /*index*/) const noexcept override
			{
				return infinity ? m_logicState.infinityView : m_logicState.view;
			}

			/** @copydoc EmEn::Graphics::ViewMatricesInterface::viewMatrix(uint32_t, bool, size_t) const */
			[[nodiscard]]
			const Libs::Math::Matrix< 4, float > &
			viewMatrix (uint32_t readStateIndex, bool infinity, size_t /*index*/) const noexcept override
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

			/** @copydoc EmEn::Graphics::ViewMatricesInterface::position() const */
			[[nodiscard]]
			const Libs::Math::Vector< 3, float > &
			position () const noexcept override
			{
				return m_logicState.position;
			}

			/** @copydoc EmEn::Graphics::ViewMatricesInterface::position(uint32_t) const */
			[[nodiscard]]
			const Libs::Math::Vector< 3, float > &
			position (uint32_t readStateIndex) const noexcept override
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

			/** @copydoc EmEn::Graphics::ViewMatricesInterface::frustum(size_t) const */
			[[nodiscard]]
			const Frustum &
			frustum (size_t /*index*/) const noexcept override
			{
				/* NOTE: Returns the main frustum, not per-cascade. Use cascadeFrustum() for cascade-specific frustums. */
				return m_logicState.frustum;
			}

			/** @copydoc EmEn::Graphics::ViewMatricesInterface::frustum(uint32_t, size_t) const */
			[[nodiscard]]
			const Frustum &
			frustum (uint32_t readStateIndex, size_t /*index*/) const noexcept override
			{
				if constexpr ( IsDebug )
				{
					if ( readStateIndex >= m_renderState.size() )
					{
						Tracer::error(ClassId, "Index overflow !");

						return m_logicState.frustum;
					}
				}

				return m_renderState[readStateIndex].frustum;
			}

			/**
			 * @brief Returns the frustum for a specific cascade.
			 * @param cascadeIndex The cascade index (0 to cascadeCount-1).
			 * @return const Frustum &
			 */
			[[nodiscard]]
			const Frustum &
			cascadeFrustum (size_t cascadeIndex) const noexcept
			{
				if ( cascadeIndex >= m_cascadeCount )
				{
					Tracer::error(ClassId, "Cascade index overflow !");

					cascadeIndex = 0;
				}

				return m_logicState.cascadeFrustums[cascadeIndex];
			}

			/**
			 * @brief Returns the frustum for a specific cascade with render state.
			 * @param readStateIndex The render state index.
			 * @param cascadeIndex The cascade index (0 to cascadeCount-1).
			 * @return const Frustum &
			 */
			[[nodiscard]]
			const Frustum &
			cascadeFrustum (uint32_t readStateIndex, size_t cascadeIndex) const noexcept
			{
				if constexpr ( IsDebug )
				{
					if ( readStateIndex >= m_renderState.size() )
					{
						Tracer::error(ClassId, "Index overflow !");

						return m_logicState.cascadeFrustums[0];
					}
				}

				if ( cascadeIndex >= m_cascadeCount )
				{
					Tracer::error(ClassId, "Cascade index overflow !");

					cascadeIndex = 0;
				}

				return m_renderState[readStateIndex].cascadeFrustums[cascadeIndex];
			}

			/** @copydoc EmEn::Graphics::ViewMatricesInterface::getAspectRatio() */
			[[nodiscard]]
			float
			getAspectRatio () const noexcept override
			{
				if ( m_logicState.bufferData[ViewWidthOffset] * m_logicState.bufferData[ViewHeightOffset] <= 0.0F )
				{
					Tracer::error(ClassId, "View properties for width and height are invalid ! Unable to compute the aspect ratio.");

					return 1.0F;
				}

				return m_logicState.bufferData[ViewWidthOffset] / m_logicState.bufferData[ViewHeightOffset];
			}

			/** @copydoc EmEn::Graphics::ViewMatricesInterface::fieldOfView() */
			[[nodiscard]]
			float
			fieldOfView () const noexcept override
			{
				using namespace Libs::Math;

				constexpr auto Rad2Deg = HalfRevolution< float > / std::numbers::pi_v< float >;

				return std::atan(1.0F / m_logicState.projection[M4x4Col1Row1]) * 2.0F * Rad2Deg;
			}

			/** @copydoc EmEn::Graphics::ViewMatricesInterface::updatePerspectiveViewProperties() */
			void updatePerspectiveViewProperties (float width, float height, float fov, float distance) noexcept override;

			/** @copydoc EmEn::Graphics::ViewMatricesInterface::updateOrthographicViewProperties() */
			void updateOrthographicViewProperties (float width, float height, float nearDistance, float farDistance) noexcept override;

			/** @copydoc EmEn::Graphics::ViewMatricesInterface::updateViewCoordinates() */
			void updateViewCoordinates (const Libs::Math::CartesianFrame< float > & coordinates, const Libs::Math::Vector< 3, float > & velocity) noexcept override;

			/** @copydoc EmEn::Graphics::ViewMatricesInterface::updateAmbientLightProperties() */
			void updateAmbientLightProperties (const Libs::PixelFactory::Color< float > & color, float intensity) noexcept override;

			/** @copydoc EmEn::Graphics::ViewMatricesInterface::create() */
			bool create (Renderer & renderer, const std::string & instanceID) noexcept override;

			/** @copydoc EmEn::Graphics::ViewMatricesInterface::publishStateForRendering(uint32_t) */
			void
			publishStateForRendering (uint32_t writeStateIndex) noexcept override
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

			/** @copydoc EmEn::Graphics::ViewMatricesInterface::updateVideoMemory(uint32_t) const */
			bool updateVideoMemory (uint32_t readStateIndex) const noexcept override;

			/** @copydoc EmEn::Graphics::ViewMatricesInterface::destroy() */
			void
			destroy () noexcept override
			{
				/* [VULKAN-CPU-SYNC] Maybe useless */
				/* NOTE: Lock between updateVideoMemory() and destroy(). */
				const std::lock_guard< std::mutex > lock{m_GPUBufferAccessLock};

				m_descriptorSet.reset();
				m_uniformBufferObject.reset();
			}

			/** @copydoc EmEn::Graphics::ViewMatricesInterface::descriptorSet() */
			[[nodiscard]]
			const Vulkan::DescriptorSet *
			descriptorSet () const noexcept override
			{
				return m_descriptorSet.get();
			}

			/**
			 * @brief Returns the number of cascades.
			 * @return uint32_t
			 */
			[[nodiscard]]
			uint32_t
			cascadeCount () const noexcept
			{
				return m_cascadeCount;
			}

			/**
			 * @brief Sets the number of cascades.
			 * @param count The cascade count (1-4).
			 * @return void
			 */
			void
			setCascadeCount (uint32_t count) noexcept
			{
				m_cascadeCount = std::clamp(count, 1U, MaxCascadeCount);
				m_logicState.bufferData[CascadeCountOffset] = static_cast< float >(m_cascadeCount);
			}

			/**
			 * @brief Returns the lambda value for cascade split calculation.
			 * @return float
			 */
			[[nodiscard]]
			float
			lambda () const noexcept
			{
				return m_lambda;
			}

			/**
			 * @brief Sets the lambda value for cascade split calculation.
			 * @param value The lambda value (0 = linear, 1 = logarithmic).
			 * @return void
			 */
			void
			setLambda (float value) noexcept
			{
				m_lambda = std::clamp(value, 0.0F, 1.0F);
			}

			/**
			 * @brief Returns the split distance for a specific cascade.
			 * @param cascadeIndex The cascade index (0 to cascadeCount-1).
			 * @return float
			 */
			[[nodiscard]]
			float
			splitDistance (size_t cascadeIndex) const noexcept
			{
				if ( cascadeIndex >= m_cascadeCount )
				{
					return m_logicState.bufferData[ViewDistanceOffset];
				}

				return m_logicState.bufferData[CascadeSplitDistancesOffset + cascadeIndex];
			}

			/**
			 * @brief Returns the view-projection matrix for a specific cascade.
			 * @param cascadeIndex The cascade index (0 to cascadeCount-1).
			 * @return const Libs::Math::Matrix< 4, float > &
			 */
			[[nodiscard]]
			const Libs::Math::Matrix< 4, float > &
			cascadeViewProjectionMatrix (size_t cascadeIndex) const noexcept
			{
				if ( cascadeIndex >= m_cascadeCount )
				{
					Tracer::error(ClassId, "Cascade index overflow !");

					cascadeIndex = 0;
				}

				return m_logicState.cascadeViewProjections[cascadeIndex];
			}

			/**
			 * @brief Updates all cascade matrices based on the light direction and camera frustum.
			 * @note This should be called after updateViewCoordinates() when used for directional light shadows.
			 * @param lightDirection The normalized light direction vector.
			 * @param cameraFrustumCorners Array of 8 corners of the camera frustum in world space.
			 * @param nearPlane The camera near plane distance.
			 * @param farPlane The camera far plane distance.
			 * @return void
			 */
			void updateCascades (
				const Libs::Math::Vector< 3, float > & lightDirection,
				const std::array< Libs::Math::Vector< 3, float >, 8 > & cameraFrustumCorners,
				float nearPlane,
				float farPlane
			) noexcept;

			/**
			 * @brief Computes the split distances using the practical split scheme.
			 * @note Uses a blend of logarithmic and linear split based on lambda value.
			 * @param nearPlane The camera near plane distance.
			 * @param farPlane The camera far plane distance.
			 * @return void
			 */
			void computeSplitDistances (float nearPlane, float farPlane) noexcept;

		private:

			/**
			 * @brief STL streams printable object.
			 * @param out A reference to the stream output.
			 * @param obj A reference to the object to print.
			 * @return std::ostream &
			 */
			friend std::ostream & operator<< (std::ostream & out, const ViewMatricesCascadedUBO & obj);

			/**
			 * @brief Computes a tight-fit orthographic projection for a cascade.
			 * @param cascadeIndex The cascade index.
			 * @param lightDirection The light direction.
			 * @param cascadeCorners The 8 corners of the cascade frustum portion.
			 * @return Libs::Math::Matrix< 4, float >
			 */
			[[nodiscard]]
			Libs::Math::Matrix< 4, float > computeCascadeProjection (
				size_t cascadeIndex,
				const Libs::Math::Vector< 3, float > & lightDirection,
				const std::array< Libs::Math::Vector< 3, float >, 8 > & cascadeCorners
			) noexcept;

			/*
			 * UBO Layout (std140):
			 * Offset  Size    Content
			 * 0       256     mat4[4] cascadeViewProjectionMatrices
			 * 256     16      vec4 cascadeSplitDistances
			 * 272     16      vec4 (cascadeCount, shadowBias, reserved, reserved)
			 * 288     16      vec4 worldPosition
			 * 304     16      vec4 velocity
			 * 320     16      vec4 viewProperties (width, height, near, far)
			 * 336     16      vec4 ambientLightColor
			 * 352     16      vec4 (ambientLightIntensity, padding...)
			 * Total: 368 bytes (std140 aligned)
			 */

			/** @brief Total number of elements in the UBO buffer. */
			static constexpr auto ViewUBOElementCount = (MaxCascadeCount * Matrix4Alignment) + (7 * VectorAlignment);
			/** @brief Total size in bytes of the UBO buffer. */
			static constexpr auto ViewUBOSize = ViewUBOElementCount * sizeof(float);

			/* Cascade view-projection matrices offset (4 matrices * 16 floats each). */
			static constexpr auto CascadeMatricesJumpOffset{MaxCascadeCount * 16UL};

			/** @brief Offset of the cascade split distances in the buffer. */
			static constexpr auto CascadeSplitDistancesOffset{CascadeMatricesJumpOffset + 0UL};
			/** @brief Offset of the cascade count in the buffer. */
			static constexpr auto CascadeCountOffset{CascadeMatricesJumpOffset + 4UL};
			/** @brief Offset of the shadow bias in the buffer. */
			static constexpr auto ShadowBiasOffset{CascadeMatricesJumpOffset + 5UL};
			/** @brief Offset of the world position in the buffer. */
			static constexpr auto WorldPositionOffset{CascadeMatricesJumpOffset + 8UL};
			/** @brief Offset of the velocity vector in the buffer. */
			static constexpr auto VelocityVectorOffset{CascadeMatricesJumpOffset + 12UL};
			/** @brief Offset of the view properties in the buffer. */
			static constexpr auto ViewPropertiesOffset{CascadeMatricesJumpOffset + 16UL};
			/** @brief Offset of the view width in the buffer. */
			static constexpr auto ViewWidthOffset{CascadeMatricesJumpOffset + 16UL};
			/** @brief Offset of the view height in the buffer. */
			static constexpr auto ViewHeightOffset{CascadeMatricesJumpOffset + 17UL};
			/** @brief Offset of the near plane distance in the buffer. */
			static constexpr auto ViewNearOffset{CascadeMatricesJumpOffset + 18UL};
			/** @brief Offset of the far plane distance in the buffer. */
			static constexpr auto ViewDistanceOffset{CascadeMatricesJumpOffset + 19UL};
			/** @brief Offset of the ambient light color in the buffer. */
			static constexpr auto AmbientLightColorOffset{CascadeMatricesJumpOffset + 20UL};
			/** @brief Offset of the ambient light intensity in the buffer. */
			static constexpr auto AmbientLightIntensityOffset{CascadeMatricesJumpOffset + 24UL};

			/**
			 * @brief Internal state structure holding view matrices and cascade data.
			 */
			struct DataState
			{
				Libs::Math::Matrix< 4, float > projection;								   /**< Main projection matrix. */
				Libs::Math::Matrix< 4, float > view;									   /**< Main view matrix. */
				Libs::Math::Matrix< 4, float > infinityView;							   /**< View matrix for infinite distance. */
				std::array< Libs::Math::Matrix< 4, float >, MaxCascadeCount > cascadeViewProjections{}; /**< View-projection matrices per cascade. */
				Libs::Math::Vector< 3, float > position;								   /**< Camera/light position in world space. */
				Frustum frustum;														   /**< Main frustum for culling. */
				std::array< Frustum, MaxCascadeCount > cascadeFrustums{};				   /**< Per-cascade frustums for culling. */
				std::array< float, ViewUBOElementCount > bufferData{};					   /**< GPU buffer data. */
			};

			DataState m_logicState;											  /**< Current logic state (write). */
			std::array< DataState, 2 > m_renderState;						   /**< Double-buffered render states (read). */
			std::unique_ptr< Vulkan::UniformBufferObject > m_uniformBufferObject; /**< Vulkan UBO for GPU memory. */
			std::unique_ptr< Vulkan::DescriptorSet > m_descriptorSet;		   /**< Vulkan descriptor set. */
			mutable std::mutex m_GPUBufferAccessLock;							/**< Mutex for GPU buffer access synchronization. */
			uint32_t m_cascadeCount{MaxCascadeCount};						   /**< Number of active cascades. */
			float m_lambda{DefaultCascadeLambda};							   /**< Split calculation blend factor. */
	};

	/**
	 * @brief Stringifies the object.
	 * @param obj A reference to the object to print.
	 * @return std::string
	 */
	std::string to_string (const ViewMatricesCascadedUBO & obj) noexcept;
}
