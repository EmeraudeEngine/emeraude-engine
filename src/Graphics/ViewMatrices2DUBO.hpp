/*
 * src/Graphics/ViewMatrices2DUBO.hpp
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
	 * @brief Specialization of view matrices for 2D surface rendering.
	 * @extends EmEn::Graphics::ViewMatricesInterface
	 */
	class ViewMatrices2DUBO final : public ViewMatricesInterface
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"ViewMatrices2DUBO"};

			/**
			 * @brief Creates 2D view matrices.
			 */
			ViewMatrices2DUBO () noexcept = default;

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

		private:

			/**
			 * @brief STL streams printable object.
			 * @param out A reference to the stream output.
			 * @param obj A reference to the object to print.
			 * @return std::ostream &
			 */
			friend std::ostream & operator<< (std::ostream & out, const ViewMatrices2DUBO & obj);

			static constexpr auto ViewUBOElementCount = Matrix4Alignment + (5 * VectorAlignment);
			static constexpr auto ViewUBOSize = ViewUBOElementCount * sizeof(float);

			static constexpr auto ProjectionMatrixOffset{0UL};
			static constexpr auto WorldPositionOffset{16UL};
			static constexpr auto VelocityVectorOffset{20UL};
			static constexpr auto ViewPropertiesOffset{24UL};
			static constexpr auto ViewWidthOffset{24UL};
			static constexpr auto ViewHeightOffset{25UL};
			static constexpr auto ViewNearOffset{26UL};
			static constexpr auto ViewDistanceOffset{27UL};
			static constexpr auto AmbientLightColorOffset{28UL};
			static constexpr auto AmbientLightIntensityOffset{32UL};

			struct DataState
			{
				Libs::Math::Matrix< 4, float > projection;
				Libs::Math::Matrix< 4, float > view;
				Libs::Math::Matrix< 4, float > infinityView;
				Libs::Math::Vector< 3, float > position;
				Frustum frustum;
				std::array< float, ViewUBOElementCount > bufferData{
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
					0.0F, 0.0F, 0.0F, 0.0F
				};
			};

			DataState m_logicState;
			std::array< DataState, 2 > m_renderState;
			std::unique_ptr< Vulkan::UniformBufferObject > m_uniformBufferObject;
			std::unique_ptr< Vulkan::DescriptorSet > m_descriptorSet;
			mutable std::mutex m_GPUBufferAccessLock;
	};

	inline
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

	/**
	 * @brief Stringifies the object.
	 * @param obj A reference to the object to print.
	 * @return std::string
	 */
	inline
	std::string
	to_string (const ViewMatrices2DUBO & obj) noexcept
	{
		std::stringstream output;

		output << obj;

		return output.str();
	}
}
