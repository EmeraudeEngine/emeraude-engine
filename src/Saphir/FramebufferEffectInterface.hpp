/*
 * src/Saphir/FramebufferEffectInterface.hpp
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

/* STL inclusions. */
#include <memory>
#include <set>

/* Forward declarations */
namespace EmEn::Saphir
{
	namespace Generator
	{
		class Abstract;
	}

	class VertexShader;
	class FragmentShader;
}

namespace EmEn::Saphir
{
	/**
	 * @brief The framebuffer effect interface.
	 */
	class FramebufferEffectInterface
	{
		public:

			/**
			 * @brief Copy constructor.
			 * @param copy A reference to the copied instance.
			 */
			FramebufferEffectInterface (const FramebufferEffectInterface & copy) noexcept = default;

			/**
			 * @brief Move constructor.
			 * @param copy A reference to the copied instance.
			 */
			FramebufferEffectInterface (FramebufferEffectInterface && copy) noexcept = default;

			/**
			 * @brief Copy assignment.
			 * @param copy A reference to the copied instance.
			 * @return FramebufferEffectInterface &
			 */
			FramebufferEffectInterface & operator= (const FramebufferEffectInterface & copy) noexcept = default;

			/**
			 * @brief Move assignment.
			 * @param copy A reference to the copied instance.
			 * @return FramebufferEffectInterface &
			 */
			FramebufferEffectInterface & operator= (FramebufferEffectInterface && copy) noexcept = default;
			
			/** 
			 * @brief Destructs the framebuffer effect interface.
			 */
			virtual ~FramebufferEffectInterface () = default;

		protected:

			/** 
			 * @brief Constructs a framebuffer effect interface.
			 */
			FramebufferEffectInterface () noexcept = default;
			
			/**
			 * @brief Says if this effect is overriding the way fragment are fetched from the color buffer.
			 * @note Override this method to return true for the final effect.
			 * @return bool
			 */
			[[nodiscard]]
			virtual
			bool
			overrideFragmentFetching () const noexcept
			{
				return false;
			}

			/**
			 * @brief Says this shader needs screen coordinates. Geometry texture coordinates attribute will be available.
			 * @note Override this method to return true for the final effect.
			 * @return bool
			 */
			[[nodiscard]]
			virtual
			bool
			requestScreenCoordinates () const noexcept
			{
				return false;
			}

			/**
			 * @brief Says this shader needs screen sizes. em_FrameSize will be available.
			 * @note Override this method to return true for the final effect.
			 * @return bool
			 */
			[[nodiscard]]
			virtual
			bool
			requestScreenSize () const noexcept
			{
				return false;
			}

			/**
			 * @brief Generates the code responsible for the fragment shader.
			 * @param generator A reference to the graphics shader generator.
			 * @param fragmentShader A reference to the fragment shader.
			 * @return bool
			 */
			[[nodiscard]]
			virtual bool generateFragmentShaderCode (Generator::Abstract & generator, FragmentShader & fragmentShader) const noexcept = 0;
	};

	using FramebufferEffectsList = std::set< std::shared_ptr< FramebufferEffectInterface > >;
}
