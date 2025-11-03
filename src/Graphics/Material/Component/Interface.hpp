/*
 * src/Graphics/Material/Component/Interface.hpp
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
#include <cstdint>
#include <memory>
#include <string>

/* Local inclusions. */
#include "Saphir/Declaration/Sampler.hpp"

namespace EmEn
{
	namespace Vulkan
	{
		class TextureInterface;
	}

	namespace Graphics
	{
		namespace TextureResource
		{
			class Abstract;
		}

		class Renderer;
	}
}

namespace EmEn::Graphics::Material::Component
{
	/** @brief The component type enumeration. */
	enum class Type : uint8_t
	{
		Value,
		Color,
		Texture,
		None
	};

	/**
	 * @brief The component interface for material.
	 */
	class Interface
	{
		public:

			/**
			 * @brief Copy constructor.
			 * @param copy A reference to the copied instance.
			 */
			Interface (const Interface & copy) noexcept = default;

			/**
			 * @brief Move constructor.
			 * @param copy A reference to the copied instance.
			 */
			Interface (Interface && copy) noexcept = default;

			/**
			 * @brief Copy assignment.
			 * @param copy A reference to the copied instance.
			 * @return Interface &
			 */
			Interface & operator= (const Interface & copy) noexcept = default;

			/**
			 * @brief Move assignment.
			 * @param copy A reference to the copied instance.
			 * @return Interface &
			 */
			Interface & operator= (Interface && copy) noexcept = default;

			/**
			 * @brief Destructs the type interface.
			 */
			virtual ~Interface () = default;

			/**
			 * @brief Creates the underlying component resources.
			 * @param renderer A reference to the graphics renderer.
			 * @param binding A reference to a binding if requested.
			 * @return bool
			 */
			[[nodiscard]]
			virtual bool create (Renderer & renderer, uint32_t & binding) noexcept = 0;

			/**
			 * @brief Return the variable name.
			 * @return const std::string &
			 */
			[[nodiscard]]
			virtual const std::string & variableName () const noexcept = 0;

			/**
			 * @brief Returns the type of component.
			 * @return Type
			 */
			[[nodiscard]]
			virtual Type type () const noexcept = 0;

			/**
			 * @brief Returns whether this component is opaque.
			 * @return bool
			 */
			[[nodiscard]]
			virtual bool isOpaque () const noexcept = 0;

			/**
			 * @brief Returns the texture interface.
			 * @return std::shared_ptr< TextureResource::Abstract >
			 */
			[[nodiscard]]
			virtual std::shared_ptr< Vulkan::TextureInterface > texture () const noexcept = 0;

			/**
			 * @brief Returns the texture resource for the loading system.
			 * @note A texture is not necessarily loaded from the disk, and this can be nullptr.
			 * @return std::shared_ptr< TextureResource::Abstract >
			 */
			[[nodiscard]]
			virtual std::shared_ptr< TextureResource::Abstract > textureResource () const noexcept = 0;

			/**
			 * @brief Returns a sample declaration for the GLSL code.
			 * @warning Check for the component type being a texture before!
			 * @param materialSet The material descriptor set index.
			 * @return Saphir::Declaration::Sampler
			 */
			[[nodiscard]]
			virtual Saphir::Declaration::Sampler getSampler (uint32_t materialSet) const noexcept = 0;

		protected:

			/** 
			 * @brief Constructs a type interface.
			 */
			Interface () noexcept = default;
	};
}
