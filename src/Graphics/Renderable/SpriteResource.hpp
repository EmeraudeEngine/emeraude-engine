/*
 * src/Graphics/Renderable/SpriteResource.hpp
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
#include <memory>
#include <mutex>

/* Local inclusions for inheritances. */
#include "Abstract.hpp"

/* Local inclusions for usages. */
#include "Resources/Container.hpp"

namespace EmEn::Graphics::Renderable
{
	/**
	 * @brief This class provides a high-level object to describe a sprite (2D) in the 3D world.
	 * @note The animation is limited to 120 frames.
	 * @extends EmEn::Graphics::Renderable::Interface Adds the ability to be rendered in the 3D world.
	 */
	class SpriteResource final : public Abstract
	{
		friend class Resources::Container< SpriteResource >;

		using ResourceTrait::load;

		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"SpriteResource"};

			/** @brief Defines the resource dependency complexity. */
			static constexpr auto Complexity{Resources::DepComplexity::Complex};

			/**
			 * @brief Construct a Sprite resource.
			 * @param name A string for the resource name [std::move].
			 * @param renderableFlags The resource flag bits. Default none.
			 */
			explicit
			SpriteResource (std::string name, uint32_t renderableFlags = 0) noexcept
				: Abstract{std::move(name), IsSprite | renderableFlags}
			{

			}

			/**
			 * @brief Returns the unique identifier for this class [Thread-safe].
			 * @return size_t
			 */
			static
			size_t
			getClassUID () noexcept
			{
				return Libs::Hash::FNV1a(ClassId);
			}

			/** @copydoc EmEn::Libs::ObservableTrait::classUID() const */
			[[nodiscard]]
			size_t
			classUID () const noexcept override
			{
				return getClassUID();
			}

			/** @copydoc EmEn::Libs::ObservableTrait::is() const */
			[[nodiscard]]
			bool
			is (size_t classUID) const noexcept override
			{
				return classUID == getClassUID();
			}

			/** @copydoc EmEn::Graphics::Renderable::Interface::layerCount() const */
			[[nodiscard]]
			uint32_t
			layerCount () const noexcept override
			{
				return 1;
			}

			/** @copydoc EmEn::Graphics::Renderable::Interface::isOpaque() const */
			[[nodiscard]]
			bool
			isOpaque (uint32_t /*layerIndex*/) const noexcept override
			{
				if ( m_material != nullptr )
				{
					return m_material->isOpaque();
				}

				return true;
			}

			/** @copydoc EmEn::Graphics::Renderable::Interface::geometry() const */
			[[nodiscard]]
			const Geometry::Interface *
			geometry () const noexcept override
			{
				return m_geometry.get();
			}

			/** @copydoc EmEn::Graphics::Renderable::Interface::material() const */
			[[nodiscard]]
			const Material::Interface *
			material (uint32_t /*layerIndex*/) const noexcept override
			{
				return m_material.get();
			}

			/** @copydoc EmEn::Graphics::Renderable::Interface::layerRasterizationOptions() const */
			[[nodiscard]]
			const RasterizationOptions *
			layerRasterizationOptions (uint32_t /*layerIndex*/) const noexcept override
			{
				return nullptr;
			}

			/** @copydoc EmEn::Graphics::Renderable::Interface::boundingBox() const */
			[[nodiscard]]
			const Libs::Math::Space3D::AACuboid< float > &
			boundingBox () const noexcept override
			{
				if ( m_geometry == nullptr )
				{
					return NullBoundingBox;
				}

				return m_geometry->boundingBox();
			}

			/** @copydoc EmEn::Graphics::Renderable::Interface::boundingSphere() const */
			[[nodiscard]]
			const Libs::Math::Space3D::Sphere< float > &
			boundingSphere () const noexcept override
			{
				if ( m_geometry == nullptr )
				{
					return NullBoundingSphere;
				}

				return m_geometry->boundingSphere();
			}

			/** @copydoc EmEn::Resources::ResourceTrait::classLabel() const */
			[[nodiscard]]
			const char *
			classLabel () const noexcept override
			{
				return ClassId;
			}

			/** @copydoc EmEn::Resources::ResourceTrait::load(Resources::ServiceProvider &) */
			bool load (Resources::AbstractServiceProvider & serviceProvider) noexcept override;

			/** @copydoc EmEn::Resources::ResourceTrait::load(Resources::Manager &, const Json::Value &) */
			bool load (Resources::AbstractServiceProvider & serviceProvider, const Json::Value & data) noexcept override;

			/** @copydoc EmEn::Resources::ResourceTrait::memoryOccupied() const noexcept */
			[[nodiscard]]
			size_t
			memoryOccupied () const noexcept override
			{
				return sizeof(*this);
			}

			/**
			 * @brief Loads a sprite resource from a material.
			 * @param serviceProvider A reference to the resource manager through a service provider.
			 * @param material A reference to a material resource smart pointer.
			 * @param centerAtBottom Set the sprite center to the bottom of the quad. Default false.
			 * @param flip Flip the sprite picture. Default false.
			 * @param rasterizationOptions A reference to rasterization options. Defaults.
			 * @return bool
			 */
			bool load (Resources::AbstractServiceProvider & serviceProvider, const std::shared_ptr< Material::Interface > & material, bool centerAtBottom = false, bool flip = false, const RasterizationOptions & rasterizationOptions = {}) noexcept;

			/**
			 * @brief Sets the site of the sprite.
			 * @param value
			 */
			void
			setSize (float value) noexcept
			{
				m_size = std::abs(value);
			}

			/**
			 * @brief Returns the size of the sprite.
			 * @return float
			 */
			[[nodiscard]]
			float
			size () const noexcept
			{
				return m_size;
			}

			/**
			 * @brief Returns the number of frames from the material.
			 * @note Will return 1 if no material is associated.
			 * @return size_t
			 */
			[[nodiscard]]
			size_t
			frameCount () const noexcept
			{
				if ( m_material == nullptr )
				{
					Tracer::warning(ClassId, "Material is not yet loaded ! Unable to get the Sprite frame count.");

					return 1;
				}

				return m_material->frameCount();
			}

			/**
			 * @brief Returns the duration in milliseconds from the material.
			 * @note Will return 0 if no material is associated.
			 * @return uint32_t
			 */
			[[nodiscard]]
			uint32_t
			duration () const noexcept
			{
				if ( m_material == nullptr )
				{
					Tracer::warning(ClassId, "Material is not yet loaded ! Unable to get the Sprite duration.");

					return 0;
				}

				return m_material->duration();
			}

		private:

			/** @copydoc EmEn::Resources::ResourceTrait::onDependenciesLoaded() */
			[[nodiscard]]
			bool onDependenciesLoaded () noexcept override;

			/**
			 * @brief Prepares the geometry resource for the sprite.
			 * @note This geometry resource will be shared between all sprites.
			 * @param serviceProvider A reference to the resource manager through a service provider.
			 * @param isAnimated Set texture coordinates to 3D if so.
			 * @param centerAtBottom Set the geometry center at the bottom for specific sprites.
			 * @param flip Flip the UV on X axis.
			 * @return bool
			 */
			bool prepareGeometry (Resources::AbstractServiceProvider & serviceProvider, bool isAnimated, bool centerAtBottom, bool flip) noexcept;

			/**
			 * @brief Attaches the material resource.
			 * @param materialResource A reference to a material resource smart pointer.
			 * @return bool
			 */
			bool setMaterial (const std::shared_ptr< Material::Interface > & materialResource) noexcept;

			/* JSON key. */
			static constexpr auto JKSizeKey{"Size"};
			static constexpr auto JKCenterAtBottomKey{"CenterAtBottom"};
			static constexpr auto JKFlipKey{"Flip"};

			static inline std::mutex s_lockGeometryLoading;

			std::shared_ptr< Geometry::Interface > m_geometry;
			std::shared_ptr< Material::Interface > m_material;
			float m_size{1.0F};
	};
}

/* Expose the resource manager as a convenient type. */
namespace EmEn::Resources
{
	using Sprites = Container< Graphics::Renderable::SpriteResource >;
}
