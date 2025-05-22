/*
 * src/Graphics/Renderable/Interface.hpp
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

/* Local inclusions for inheritances. */
#include "Resources/ResourceTrait.hpp"

/* Local inclusions for usages. */
#include "Libs/Math/Space3D/AACuboid.hpp"
#include "Libs/Math/Space3D/Sphere.hpp"
#include "Graphics/Geometry/Interface.hpp"
#include "Graphics/Material/Interface.hpp"
#include "Graphics/RasterizationOptions.hpp"

namespace EmEn::Graphics::Renderable
{
	/** @brief Renderable interface flag bits. */
	enum RenderableFlagBits : uint32_t
	{
		None = 0U,
		/** @brief This flag is set when the geometry is fully usable by the GPU,
		 * thus ready to make mesh, sprite, things, ... as instances. */
		IsReadyForInstantiation = 1U << 0,
		/** @brief This flag tells that the renderable has a skeletal animation available. */
		HasSkeletalAnimation = 1U << 1,
		/** @brief This flag tells the system this renderable uses a single quad which should always face the camera. */
		IsSprite = 1U << 2
	};

	/**
	 * @brief Defines a contract to render an object in the 3D world.
	 * @note This holds only what to draw.
	 * @extends EmEn::Resources::ResourceTrait Every renderable is a resource.
	 */
	class Interface : public Resources::ResourceTrait
	{
		public:

			static constexpr Libs::Math::Space3D::AACuboid< float > NullBoundingBox{};
			static constexpr Libs::Math::Space3D::Sphere< float > NullBoundingSphere{};

			/**
			 * @brief Copy constructor.
			 * @param copy A reference to the copied instance.
			 */
			Interface (const Interface & copy) noexcept = delete;

			/**
			 * @brief Move constructor.
			 * @param copy A reference to the copied instance.
			 */
			Interface (Interface && copy) noexcept = delete;

			/**
			 * @brief Copy assignment.
			 * @param copy A reference to the copied instance.
			 * @return Interface &
			 */
			Interface & operator= (const Interface & copy) noexcept = delete;

			/**
			 * @brief Move assignment.
			 * @param copy A reference to the copied instance.
			 * @return Interface &
			 */
			Interface & operator= (Interface && copy) noexcept = delete;

			/**
			 * @brief Destructs the renderable object.
			 */
			~Interface () override = default;

			/**
			 * @brief Returns whether the renderable is ready to prepare an instance on GPU for rendering.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isReadyForInstantiation () const noexcept
			{
				return this->isFlagEnabled(IsReadyForInstantiation);
			}

			/**
			 * @brief Returns whether the renderable has a skeletal animation.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			hasSkeletalAnimation () const noexcept
			{
				return this->isFlagEnabled(HasSkeletalAnimation);
			}

			/**
			 * @brief Returns whether the renderable is a sprite to differentiate it from a regular 3D mesh.
			 * @note This mainly means the renderable should always face the camera by providing a model matrix without initial rotation.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isSprite () const noexcept
			{
				return this->isFlagEnabled(IsSprite);
			}

			/**
			 * @brief Returns the number of layouts to render the whole object.
			 * @return uint32_t
			 */
			[[nodiscard]]
			virtual uint32_t layerCount () const noexcept = 0;

			/**
			 * @brief Returns whether the renderable is opaque to get the way to order it with the render lists.
			 * @param layerIndex The index of the layer.
			 * @return bool
			 */
			[[nodiscard]]
			virtual bool isOpaque (uint32_t layerIndex) const noexcept = 0;

			/**
			 * @brief Returns the geometry of the renderable.
			 * @return const Geometry::Interface *
			 */
			[[nodiscard]]
			virtual const Geometry::Interface * geometry () const noexcept = 0;

			/**
			 * @brief Returns the material of the renderable.
			 * @note This can be nullptr.
			 * @param layerIndex The index of the layer.
			 * @return const Material::Interface *
			 */
			[[nodiscard]]
			virtual const Material::Interface * material (uint32_t layerIndex) const noexcept = 0;

			/**
			 * @brief Returns the rasterization options for the renderable layer.
			 * @warning Can return nullptr.
			 * @param layerIndex The layer index.
			 * @return const RasterizationOptions *
			 */
			[[nodiscard]]
			virtual const RasterizationOptions * layerRasterizationOptions (uint32_t layerIndex) const noexcept = 0;

			/**
			 * @brief Returns the bounding box surrounding the renderable.
			 * @return const Libs::Math::Space3D::AACuboid< float > &
			 */
			[[nodiscard]]
			virtual const Libs::Math::Space3D::AACuboid< float > & boundingBox () const noexcept = 0;

			/**
			 * @brief Returns the bounding sphere surrounding the renderable.
			 * @return const Libs::Math::Space3D::Sphere< float > &
			 */
			[[nodiscard]]
			virtual const Libs::Math::Space3D::Sphere< float > & boundingSphere () const noexcept = 0;

		protected:

			/**
			 * @brief Constructs a renderable object.
			 * @param resourceName A string for the resource name [std::move].
			 * @param resourceFlags The resource flag bits.
			 */
			explicit
			Interface (std::string resourceName, uint32_t resourceFlags) noexcept
				: ResourceTrait{std::move(resourceName), resourceFlags}
			{

			}

			/**
			 * @brief Sets the renderable ready to prepare an instance on GPU.
			 * @param state The state.
			 * @return void
			 */
			void
			setReadyForInstantiation (bool state) noexcept
			{
				if ( state )
				{
					this->enableFlag(IsReadyForInstantiation);
				}
				else
				{
					this->disableFlag(IsReadyForInstantiation);
				}
			}

		private:

			/** @copydoc EmEn::Resources::ResourceTrait::onDependenciesLoaded() */
			[[nodiscard]]
			bool onDependenciesLoaded () noexcept override;
	};
}
