/*
 * src/Graphics/Renderable/SkyBoxResource.hpp
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

#pragma once

/* Project configuration. */
#include "emeraude_export.hpp"

/* STL inclusions. */
#include <memory>

/* Local inclusions for inheritances. */
#include "AbstractBackground.hpp"

/* Forward declarations. */
namespace EmEn::Resources
{
	template< typename resource_t >
	class Container;
}

namespace EmEn::Graphics::Renderable
{
	/**
	 * @brief The skybox resource class.
	 * @extends EmEn::Graphics::Renderable::AbstractBackground This is a specialized background.
	 */
	class EMEN_API SkyBoxResource final : public AbstractBackground
	{
		friend class Resources::Container< SkyBoxResource >;

		using ResourceTrait::load;

		public:

			/**
			 * @brief Default sky luminance, in nits (cd/m²).
			 * @note An overcast sky sits around 8000 nits and a clear blue sky away from the sun
			 * in the same range, which is what makes a sky readable next to a 100000 lx sun. The
			 * cubemap is a normalized LDR gradient; this is the physical scale applied to it.
			 */
			/**
			 * @brief Reference sky LUMINANCES, in nits (cd/m²) — what a sky actually measures.
			 * @note A sky is an emitter, so it is described by a luminance, and its value spans
			 * SEVEN orders of magnitude between noon and midnight: that range is precisely what a
			 * scene's exposure has to sit inside. Authoring a night scene under a daylight sky
			 * leaves the metering with fifteen stops to reconcile — the sky clips to white and the
			 * ground crushes to black (observed on the Citadel demo, Jul 2026).
			 * @note ⚠️ Because the cubemap is LDR ([0,1] — the image pipeline has no HDR format),
			 * ONE scalar has to stand for the whole sky, moon and stars included. A real moonlit sky
			 * measures 0.001-0.01 cd/m² while the moon disc itself measures ~2500: no single number
			 * describes both, so `MoonlitNightSkyLuminance` deliberately represents the sky AS SEEN
			 * with its moon glow — which lands near late twilight — rather than the empty sky. This
			 * compromise disappears the day a real HDR sky format exists (see TODO.md).
			 */
			static constexpr auto DaylightSkyLuminance{8000.0F};   /* Clear day, away from the sun. */
			static constexpr auto OvercastSkyLuminance{2000.0F};   /* Heavy cloud cover. */
			static constexpr auto TwilightSkyLuminance{10.0F};     /* Civil dusk, sun just set. */
			static constexpr auto MoonlitNightSkyLuminance{1.0F};  /* Full moon visible (see note). */

			/** @brief Luminance used when a sky declares none — a clear day. */
			static constexpr auto DefaultSkyLuminance{DaylightSkyLuminance};

			/** @brief Class identifier. */
			static constexpr auto ClassId{"SkyBoxResource"};

			/** @brief Defines the resource dependency complexity. */
			static constexpr auto Complexity{Resources::DepComplexity::Complex};

			/**
			 * @brief Constructs a skybox resource.
			 * @param serviceProvider A reference to the service provider.
			 * @param name The name of the resource [std::move].
			 * @param resourceFlags The resource flag bits. Default none.
			 */
			SkyBoxResource (Resources::AbstractServiceProvider & serviceProvider, std::string name, uint32_t resourceFlags = 0) noexcept
				: AbstractBackground{serviceProvider, std::move(name), resourceFlags}
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
				return Base::Hash::FNV1a(ClassId);
			}

			/** @copydoc EmEn::Base::ObservableTrait::classUID() const */
			[[nodiscard]]
			size_t
			classUID () const noexcept override
			{
				return getClassUID();
			}

			/** @copydoc EmEn::Base::ObservableTrait::is() const */
			[[nodiscard]]
			bool
			is (size_t classUID) const noexcept override
			{
				return classUID == getClassUID();
			}

			/** @copydoc EmEn::Graphics::Renderable::Abstract::subGeometryCount() const */
			[[nodiscard]]
			uint32_t
			subGeometryCount () const noexcept override
			{
				return 1;
			}

			/** @copydoc EmEn::Graphics::Renderable::Abstract::layerCount() const */
			[[nodiscard]]
			uint32_t
			layerCount () const noexcept override
			{
				return 1;
			}

			/** @copydoc EmEn::Graphics::Renderable::Abstract::isOpaque(uint32_t) const */
			[[nodiscard]]
			bool
			isOpaque (uint32_t /*layerIndex*/) const noexcept override
			{
				return true;
			}

			/** @copydoc EmEn::Graphics::Renderable::Abstract::requiresGrabPass(uint32_t) const */
			[[nodiscard]]
			bool
			requiresGrabPass (uint32_t /*layerIndex*/) const noexcept override
			{
				return false;
			}

			/** @copydoc EmEn::Graphics::Renderable::Abstract::geometry(uint32_t) const */
			[[nodiscard]]
			const Geometry::Interface *
			geometry (uint32_t /*LODLevel*/) const noexcept override
			{
				return m_geometry.get();
			}

			/** @copydoc EmEn::Graphics::Renderable::Abstract::material(uint32_t) const */
			[[nodiscard]]
			const Material::Interface *
			material (uint32_t /*layerIndex*/) const noexcept override
			{
				return m_material.get();
			}

			/** @copydoc EmEn::Graphics::Renderable::Abstract::layerRasterizationOptions(uint32_t) const */
			[[nodiscard]]
			const RasterizationOptions *
			layerRasterizationOptions (uint32_t /*layerIndex*/) const noexcept override
			{
				return nullptr;
			}

			/** @copydoc EmEn::Resources::ResourceTrait::classLabel() const */
			[[nodiscard]]
			const char *
			classLabel () const noexcept override
			{
				return ClassId;
			}

			/** @copydoc EmEn::Resources::ResourceTrait::load() */
			bool load () noexcept override;

			/** @copydoc EmEn::Resources::ResourceTrait::load(const Json::Value &) */
			bool load (const Json::Value & data) noexcept override;

			/** @copydoc EmEn::Resources::ResourceTrait::memoryOccupied() const noexcept */
			[[nodiscard]]
			size_t
			memoryOccupied () const noexcept override
			{
				return sizeof(*this);
			}

			/**
			 * @brief Loads a skybox with a material resource.
			 * @param material A reference to a material smart pointer.
			 * @return bool
			 */
			bool load (const std::shared_ptr< Material::Interface > & material) noexcept;

			/** @copydoc EmEn::Graphics::Renderable::AbstractBackground::environmentCubemap() */
			[[nodiscard]]
			std::shared_ptr< TextureResource::TextureCubemap >
			environmentCubemap () const noexcept override
			{
				return m_environmentCubemap;
			}

			/**
			 * @brief Sets the cubemap the scene derives its environment lighting from.
			 * @note ⚠️ Only the name-based load paths set this on their own. A skybox built from a
			 * MATERIAL — the path a loader takes when it produces the cubemap itself, as USD's
			 * DomeLight does — has no way to declare its IBL source otherwise, and the scene
			 * silently keeps `+DefaultTextureCubemap`: the sky renders correctly while every
			 * surface is lit by the wrong environment, with no error anywhere.
			 * @param cubemap A reference to the cubemap texture the sky is made of.
			 */
			void
			setEnvironmentCubemap (const std::shared_ptr< TextureResource::TextureCubemap > & cubemap) noexcept
			{
				m_environmentCubemap = cubemap;
			}

		private:

			/** @copydoc EmEn::Resources::ResourceTrait::onDependenciesLoaded() */
			[[nodiscard]]
			bool onDependenciesLoaded () noexcept override;

			/**
			 * @brief Sets the geometry resource.
			 * @param geometry A reference to a geometry resource smart pointer.
			 * @return bool
			 */
			bool setGeometry (const std::shared_ptr< Geometry::Interface > & geometry) noexcept;

			/**
			 * @brief Sets the material resource.
			 * @param material A reference to a material resource smart pointer.
			 * @return bool
			 */
			bool setMaterial (const std::shared_ptr< Material::Interface > & material) noexcept;

			std::shared_ptr< Geometry::Interface > m_geometry;
			std::shared_ptr< Material::Interface > m_material;
			std::shared_ptr< TextureResource::TextureCubemap > m_environmentCubemap;
	};
}

/* Expose the resource manager as a convenient type. */
namespace EmEn::Resources
{
	using SkyBoxes = Container< Graphics::Renderable::SkyBoxResource >;
}
