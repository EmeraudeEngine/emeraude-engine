/*
 * src/Graphics/RasterizationOptions.hpp
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

/* Local inclusions for usages. */
#include "Types.hpp"

namespace EmEn::Graphics
{
	/**
	 * @brief Defines options to rasterize a renderable instance.
	 * @note These options are dynamic to gain control over the rendering of multiple instance of the same renderable object.
	 */
	class RasterizationOptions final
	{
		public:

			/**
			 * @brief Constructs defaults rasterization options.
			 */
			RasterizationOptions () noexcept = default;

			/**
			 * @brief Constructs rasterization options.
			 * @param polygonMode The polygon rasterization mode.
			 * @param cullingMode The triangle culling mode.
			 * @param triangleClockwise Sets the triangle winding in clockwise mode. Default false.
			 */
			RasterizationOptions (PolygonMode polygonMode, CullingMode cullingMode, bool triangleClockwise = false) noexcept
				: m_polygonMode{polygonMode},
				m_cullingMode{cullingMode},
				m_triangleClockwise{triangleClockwise}
			{

			}

			/**
			 * @brief Sets how triangles will be rasterized on screen.
			 * @note This affect only triangle primitive.
			 * @param polygonMode Mode of drawing the triangles.
			 * @return void
			 */
			void
			setPolygonMode (PolygonMode polygonMode) noexcept
			{
				m_polygonMode = polygonMode;
			}

			/**
			 * @brief Returns how triangles will be rasterized on screen.
			 * @note The initial option is 'PolygonMode::Fill'.
			 * @return PolygonMode
			 */
			[[nodiscard]]
			PolygonMode
			polygonMode () const noexcept
			{
				return m_polygonMode;
			}

			/**
			* @brief Sets the discard mode for triangles from rasterization.
			* @param cullingMode The culling mode.
			* @return void
			*/
			void
			setCullingMode (CullingMode cullingMode) noexcept
			{
				m_cullingMode = cullingMode;
			}

			/**
			* @brief Returns the discard mode for triangles from rasterization.
			* @note The initial option is 'CullingMode::Back'.
			* @return CullingMode
			*/
			[[nodiscard]]
			CullingMode
			cullingMode () const noexcept
			{
				return m_cullingMode;
			}

			/**
			 * @brief Sets the triangle winding to use clockwise policy.
			 * @param state The state
			 * @return void
			 */
			void
			setTriangleClockwise (bool state) noexcept
			{
				m_triangleClockwise = state;
			}

			/**
			 * @brief Returns whether the triangle winding follow the clockwise policy.
			 * @note The initial option is 'false' (Counter clockwise).
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isTriangleClockwise () const noexcept
			{
				return m_triangleClockwise;
			}

			/**
			 * @brief Sets the depth bias (polygon offset) parameters.
			 * @note Automatically enables depth bias when called.
			 * @param factor The slope factor (depthBiasSlopeFactor).
			 * @param units The constant factor (depthBiasConstantFactor).
			 * @param clamp The maximum depth bias (depthBiasClamp). Default 0.0f.
			 * @return void
			 */
			void
			setDepthBias (float factor, float units, float clamp = 0.0F) noexcept
			{
				m_depthBiasEnabled = true;
				m_depthBiasSlopeFactor = factor;
				m_depthBiasConstantFactor = units;
				m_depthBiasClamp = clamp;
			}

			/**
			 * @brief Manually enable or disable depth bias.
			 * @param state The state.
			 */
			void
			enableDepthBias (bool state) noexcept
			{
				m_depthBiasEnabled = state;
			}

			/**
			 * @brief Returns whether depth bias is enabled.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isDepthBiasEnabled () const noexcept
			{
				return m_depthBiasEnabled;
			}

			/**
			 * @brief Returns the depth bias slope factor.
			 * @return float
			 */
			[[nodiscard]]
			float
			depthBiasSlopeFactor () const noexcept
			{
				return m_depthBiasSlopeFactor;
			}

			/**
			 * @brief Returns the depth bias constant factor.
			 * @return float
			 */
			[[nodiscard]]
			float
			depthBiasConstantFactor () const noexcept
			{
				return m_depthBiasConstantFactor;
			}

			/**
			 * @brief Returns the depth bias clamp.
			 * @return float
			 */
			[[nodiscard]]
			float
			depthBiasClamp () const noexcept
			{
				return m_depthBiasClamp;
			}

			// Deprecated / Alias for backwards compatibility if needed, but updated to use new members
			[[deprecated("Use setDepthBias(slope, constant) instead")]]
			void
			setPolygonOffsetParameters (float factor, float units) noexcept
			{
				setDepthBias(factor, units, 0.0F);
			}

			[[deprecated("Use depthBiasSlopeFactor() instead")]]
			[[nodiscard]]
			float
			polygonOffsetFactor () const noexcept
			{
				return m_depthBiasSlopeFactor;
			}

			[[deprecated("Use depthBiasConstantFactor() instead")]]
			[[nodiscard]]
			float
			polygonOffsetUnits () const noexcept
			{
				return m_depthBiasConstantFactor;
			}

		private:

			PolygonMode m_polygonMode{PolygonMode::Fill};
			CullingMode m_cullingMode{CullingMode::Back};
			
			bool m_depthBiasEnabled{false};
			float m_depthBiasSlopeFactor{0.0F};    // Was factor
			float m_depthBiasConstantFactor{0.0F}; // Was units
			float m_depthBiasClamp{0.0F};

			bool m_triangleClockwise{false};
	};
}
