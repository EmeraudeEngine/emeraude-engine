/*
 * src/Graphics/PostProcessor.hpp
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
#include "Saphir/FramebufferEffectInterface.hpp"
#include "Geometry/IndexedVertexResource.hpp"

namespace EmEn::Graphics
{
	/**
	 * @brief The PostProcessor class
	 */
	class PostProcessor final
	{
		friend class View;

		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"PostProcessor"};

			/* GLSL variables. */
			static constexpr auto Fragment = "em_Fragment";

			/**
			 * @brief Construct an offscreen framebuffer for post-rendering effects.
			 * @param width The framebuffer width.
			 * @param height The framebuffer height.
			 * @param colorBufferBits The desired color precision.
			 * @param depthBufferBits The desired depth precision.
			 * @param stencilBufferBits The desired stencil precision.
			 * @param samples The desired multisample multiplier.
			 */
			PostProcessor (unsigned int width, unsigned int height, unsigned int colorBufferBits, unsigned int depthBufferBits, unsigned int stencilBufferBits, unsigned int samples) noexcept;

			/**
			 * @brief Returns whether the post-processor is usable.
			 * @return bool
			 */
			[[nodiscard]]
			bool usable () const noexcept;

			/**
			 * @brief Sets a list of post-effect.
			 * @param effectsList The list of shader effects.
			 * @return void
			 */
			void setEffectsList (const Saphir::FramebufferEffectsList & effectsList) noexcept;

			/**
			 * @brief Clears every effect.
			 * @return void
			 */
			void clearEffects () noexcept;

			/**
			 * @brief Prepares the post-processor for drawing to the off-screen framebuffer.
			 * @return void
			 */
			void begin () noexcept;

			/**
			 * @brief Release post-processor to let draw the off-screen framebuffer to view framebuffer.
			 * @return void
			 */
			void end () noexcept;

			/**
			 * @brief Render the off-screen buffer with effects to the default framebuffer.
			 * @param region The viewport area to blit/copies the render.
			 * @return void
			 */
			void render (const Libs::Math::Space2D::AARectangle< uint32_t > & region) const noexcept;

			/**
			 * @brief Returns a pointer to the framebuffer responsible for the post-processor.
			 * @return const Framebuffer *
			 */
			//[[nodiscard]]
			//const Framebuffer * framebuffer () const noexcept;

			/**
			 * @brief Returns whether the framebuffer is using multisample.
			 * @return bool
			 */
			[[nodiscard]]
			bool isMultisamplingEnabled () const noexcept;

			/**
			 * @brief Sets the background color for the framebuffer.
			 * @param color A reference to a color.
			 * @return void
			 */
			void setBackgroundColor (const Libs::PixelFactory::Color< float > & color) noexcept;

		private:

			/**
			 * @brief Resizes the framebuffer.
			 * @param width The new width.
			 * @param height The new height.
			 * @return bool
			 */
			bool resize (unsigned int width, unsigned int height) noexcept;

			/**
			 * @brief
			 * @return bool
			 */
			bool loadGeometry () noexcept;

			/**
			 * @brief Load the post-processor shading program.
			 * @return bool
			 */
			bool loadProgram () noexcept;

			/**
			 * @brief
			 * @param width
			 * @param height
			 * @param colorBufferBits
			 * @param depthBufferBits
			 * @param stencilBufferBits
			 * @param samples
			 * @return bool
			 */
			bool buildFramebuffer (unsigned int width, unsigned int height, unsigned int colorBufferBits, unsigned int depthBufferBits, unsigned int stencilBufferBits, unsigned int samples) noexcept;

			/* Flag names. */
			static constexpr auto ServiceInitialized{0UL};
			static constexpr auto UpdateRequested{1UL};

			//Framebuffer m_framebuffer{};
			//std::shared_ptr< Vulkan::Texture2D > m_colorBuffer{};
			//RenderBufferObject m_extraBuffer{};
			std::unique_ptr< Geometry::IndexedVertexResource > m_geometry{};
			Saphir::FramebufferEffectsList m_effectsList{};
			//std::shared_ptr< Program > m_program{};
			//RasterizationMode m_rasterizationMode{};
			std::array< bool, 8 > m_flags{
				false/*ServiceInitialized*/,
				false/*UpdateRequested*/,
				false/*UNUSED*/,
				false/*UNUSED*/,
				false/*UNUSED*/,
				false/*UNUSED*/,
				false/*UNUSED*/,
				false/*UNUSED*/
			};
	};
}
