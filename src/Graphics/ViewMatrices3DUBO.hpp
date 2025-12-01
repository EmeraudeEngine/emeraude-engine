/*
 * src/Graphics/ViewMatrices3DUBO.hpp
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
	/**
	 * @brief Specialization of view matrices for cubemap rendering.
	 * @extends EmEn::Graphics::ViewMatricesInterface
	 */
	class ViewMatrices3DUBO final : public ViewMatricesInterface
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"ViewMatrices3DUBO"};

			/**
			 * @brief Creates 3D view matrices.
			 */
			ViewMatrices3DUBO () noexcept = default;

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
			viewMatrix (bool infinity, size_t index) const noexcept override
			{
				if ( index >= CubemapFaceCount )
				{
					Tracer::error(ClassId, "Index overflow !");

					index = 0;
				}

				return infinity ? m_logicState.infinityViews[index] : m_logicState.views[index];
			}

			/** @copydoc EmEn::Graphics::ViewMatricesInterface::viewMatrix(uint32_t, bool, size_t) const */
			[[nodiscard]]
			const Libs::Math::Matrix< 4, float > &
			viewMatrix (uint32_t readStateIndex, bool infinity, size_t index) const noexcept override
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
			frustum (size_t index) const noexcept override
			{
				if ( index >= CubemapFaceCount )
				{
					Tracer::error(ClassId, "Index overflow !");

					index = 0;
				}

				return m_logicState.frustums[index];
			}

			/** @copydoc EmEn::Graphics::ViewMatricesInterface::frustum(uint32_t, size_t) const */
			[[nodiscard]]
			const Frustum &
			frustum (uint32_t readStateIndex, size_t index) const noexcept override
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

			/** @copydoc EmEn::Graphics::ViewMatricesInterface::getAspectRatio() */
			[[nodiscard]]
			float
			getAspectRatio () const noexcept override
			{
				return 1.0F;
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
				const std::lock_guard< std::mutex > lock{m_memoryAccess};

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

		private:

			/**
			 * @brief STL streams printable object.
			 * @param out A reference to the stream output.
			 * @param obj A reference to the object to print.
			 * @return std::ostream &
			 */
			friend std::ostream & operator<< (std::ostream & out, const ViewMatrices3DUBO & obj);

			/** @brief Total number of elements in the UBO buffer. */
			static constexpr auto ViewUBOElementCount = (6 * Matrix4Alignment) + Matrix4Alignment + (5 * VectorAlignment);
			/** @brief Total size in bytes of the UBO buffer. */
			static constexpr auto ViewUBOSize = ViewUBOElementCount * sizeof(float);

			/* @brief Jump offset over the 6 view matrices for the cubemap. */
			static constexpr auto ViewMatricesJumpOffset{6 + 16UL};

			/** @brief Offset of the projection matrix in the buffer. */
			static constexpr auto ProjectionMatrixOffset{ViewMatricesJumpOffset + 0UL};
			/** @brief Offset of the world position in the buffer. */
			static constexpr auto WorldPositionOffset{ViewMatricesJumpOffset + 16UL};
			/** @brief Offset of the velocity vector in the buffer. */
			static constexpr auto VelocityVectorOffset{ViewMatricesJumpOffset + 20UL};
			/** @brief Offset of the view properties in the buffer. */
			static constexpr auto ViewPropertiesOffset{ViewMatricesJumpOffset + 24UL};
			/** @brief Offset of the view width in the buffer. */
			static constexpr auto ViewWidthOffset{ViewMatricesJumpOffset + 24UL};
			/** @brief Offset of the view height in the buffer. */
			static constexpr auto ViewHeightOffset{ViewMatricesJumpOffset + 25UL};
			/** @brief Offset of the near plane distance in the buffer. */
			static constexpr auto ViewNearOffset{ViewMatricesJumpOffset + 26UL};
			/** @brief Offset of the far plane distance in the buffer. */
			static constexpr auto ViewDistanceOffset{ViewMatricesJumpOffset + 27UL};
			/** @brief Offset of the ambient light color in the buffer. */
			static constexpr auto AmbientLightColorOffset{ViewMatricesJumpOffset + 28UL};
			/** @brief Offset of the ambient light intensity in the buffer. */
			static constexpr auto AmbientLightIntensityOffset{ViewMatricesJumpOffset + 32UL};

			/** @brief Orientation matrices for the 6 faces of a standard cubemap. */
			static const std::array< Libs::Math::Matrix< 4, float >, CubemapFaceCount > CubemapOrientation;
			/** @brief Orientation matrices for the 6 faces of a shadow cubemap. */
			static const std::array< Libs::Math::Matrix< 4, float >, CubemapFaceCount > ShadowCubemapOrientation;

			/**
			 * @brief Internal state structure holding view matrices and related data for all 6 cubemap faces.
			 */
			struct DataState
			{
				Libs::Math::Matrix< 4, float > projection;                               /**< Projection matrix for 3D cubemap. */
				std::array< Libs::Math::Matrix< 4, float >, CubemapFaceCount > views{}; /**< View matrices for each cubemap face. */
				std::array< Libs::Math::Matrix< 4, float >, CubemapFaceCount > infinityViews{}; /**< View matrices for infinite distance (skybox). */
				Libs::Math::Vector< 3, float > position;                                  /**< Camera position in world space. */
				std::array< Frustum, CubemapFaceCount > frustums{};                      /**< Frustums for each cubemap face. */
				std::array< float, ViewUBOElementCount > bufferData{
					/* View matrix #1. */
					1.0F, 0.0F, 0.0F, 0.0F,
					0.0F, 1.0F, 0.0F, 0.0F,
					0.0F, 0.0F, 1.0F, 0.0F,
					0.0F, 0.0F, 0.0F, 1.0F,
					/* View matrix #2. */
					1.0F, 0.0F, 0.0F, 0.0F,
					0.0F, 1.0F, 0.0F, 0.0F,
					0.0F, 0.0F, 1.0F, 0.0F,
					0.0F, 0.0F, 0.0F, 1.0F,
					/* View matrix #3. */
					1.0F, 0.0F, 0.0F, 0.0F,
					0.0F, 1.0F, 0.0F, 0.0F,
					0.0F, 0.0F, 1.0F, 0.0F,
					0.0F, 0.0F, 0.0F, 1.0F,
					/* View matrix #4. */
					1.0F, 0.0F, 0.0F, 0.0F,
					0.0F, 1.0F, 0.0F, 0.0F,
					0.0F, 0.0F, 1.0F, 0.0F,
					0.0F, 0.0F, 0.0F, 1.0F,
					/* View matrix #5. */
					1.0F, 0.0F, 0.0F, 0.0F,
					0.0F, 1.0F, 0.0F, 0.0F,
					0.0F, 0.0F, 1.0F, 0.0F,
					0.0F, 0.0F, 0.0F, 1.0F,
					/* View matrix #6. */
					1.0F, 0.0F, 0.0F, 0.0F,
					0.0F, 1.0F, 0.0F, 0.0F,
					0.0F, 0.0F, 1.0F, 0.0F,
					0.0F, 0.0F, 0.0F, 1.0F,
					/* Projection matrix. */
					1.0F, 0.0F, 0.0F, 0.0F,
					0.0F, 1.0F, 0.0F, 0.0F,
					0.0F, 0.0F, 1.0F, 0.0F,
					0.0F, 0.0F, 0.0F, 1.0F,
					/* World position. */
					0.0F, 0.0F, 0.0F, 1.0F,
					/* Velocity vector. */
					0.0F, 0.0F, 0.0F, 0.0F,
					/* View properties. */
					1.0F, 1.0F, 1.0F, 1.0F,
					/* Light ambient color. */
					0.0F, 0.0F, 0.0F, 1.0F,
					/* Light ambient intensity. */
					0.00F, 0.0F, 0.0F, 0.0F
				};
			};

			DataState m_logicState;                                              /**< Current logic state (write). */
			std::array< DataState, 2 > m_renderState;                           /**< Double-buffered render states (read). */
			std::unique_ptr< Vulkan::UniformBufferObject > m_uniformBufferObject; /**< Vulkan UBO for GPU memory. */
			std::unique_ptr< Vulkan::DescriptorSet > m_descriptorSet;           /**< Vulkan descriptor set. */
			mutable std::mutex m_memoryAccess;                                   /**< Mutex for GPU memory access synchronization. */
	};

	/**
	 * @brief Stringifies the object.
	 * @param obj A reference to the object to print.
	 * @return std::string
	 */
	std::string to_string (const ViewMatrices3DUBO & obj) noexcept;
}
