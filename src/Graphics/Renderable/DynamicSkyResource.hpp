/*
 * src/Graphics/Renderable/DynamicSkyResource.hpp
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
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <utility>

/* Local inclusions for inheritances. */
#include "AbstractBackground.hpp"

/* Local inclusions for usages. */
#include "Hash/FNV1a.hpp"
#include "Resources/ResourceTrait.hpp"
#include "Resources/Types.hpp"

/* Forward declarations. */
namespace EmEn::Resources
{
	template< typename resource_t >
	class Container;
}

namespace EmEn::Graphics
{
	struct AtmosphereParameters;
	class SkyAtmosphere;
	class StarCatalogueResource;
}

namespace EmEn::Graphics::Geometry
{
	class Interface;
}

namespace EmEn::Graphics::Material
{
	class Interface;
	class SkyAtmosphereResource;
}

namespace Json
{
	class Value;
}

namespace EmEn::Graphics::Renderable
{
	/**
	 * @brief A PHYSICAL SKY background: a planet's atmosphere (Graphics::SkyAtmosphere) drawn on the skybox cube by
	 * Material::SkyAtmosphereResource, lit by the scene's star.
	 * @note The renderer refreshes the atmosphere's sky-view table every frame, before the scene is drawn, from the
	 * scene's sky state (Scenes::Scene::skyStateForRendering(), written by a Scenes::Component::CelestialClock) and the
	 * camera's altitude; without a clock, a default afternoon sun lights it.
	 * @note JSON: "Planet" (a preset name, AtmosphereParameters::preset(), default "Earth"), "Atmosphere" (an object whose
	 * keys override the preset: "GroundRadius", "TopRadius" and the scale heights in km, "RayleighScattering",
	 * "MieScattering", "MieAbsorption", "AbsorptionExtinction" per km as [r, g, b], "MiePhaseG" as [r, g, b] or one
	 * number, "AbsorptionLayerWidth" in km, "GroundAlbedo" as [r, g, b]), the material's artistic
	 * keys (Material::SkyAtmosphereResource: "Tint", "Intensity", "Saturation", "StarDisc", "LimbDarkening"), the moons'
	 * "Moons", the planets' "PlanetSurfaces", the star catalogue "StarCatalogue" and the background's photometry (AbstractBackground::parsePhotometry()).
	 * @extends EmEn::Graphics::Renderable::AbstractBackground The is a specialized background.
	 */
	class EMEN_API DynamicSkyResource final : public AbstractBackground
	{
		friend class Resources::Container< DynamicSkyResource >;

		using ResourceTrait::load;

		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"DynamicSkyResource"};

			/** @brief Defines the resource dependency complexity. */
			static constexpr auto Complexity{Resources::DepComplexity::Complex};

			/* JSON keys. */
			/** @brief A physical sky is ABSOLUTE radiance: its luminance, the scale applied to its environment cubemap, is 1
			 * (SkyEnvironment, owner decision 2026-10-25). */
			static constexpr auto AbsoluteLuminance{1.0F};

			static constexpr auto JKPlanet{"Planet"};
			static constexpr auto JKAtmosphere{"Atmosphere"};
			static constexpr auto JKGroundRadius{"GroundRadius"};
			static constexpr auto JKTopRadius{"TopRadius"};
			static constexpr auto JKRayleighScattering{"RayleighScattering"};
			static constexpr auto JKRayleighScaleHeight{"RayleighScaleHeight"};
			static constexpr auto JKMieScattering{"MieScattering"};
			static constexpr auto JKMieAbsorption{"MieAbsorption"};
			static constexpr auto JKMieScaleHeight{"MieScaleHeight"};
			static constexpr auto JKMiePhaseG{"MiePhaseG"};
			static constexpr auto JKAbsorptionExtinction{"AbsorptionExtinction"};
			static constexpr auto JKAbsorptionLayerWidth{"AbsorptionLayerWidth"};
			static constexpr auto JKGroundAlbedo{"GroundAlbedo"};
			/** @brief The moons' appearance, an array indexed like the scene's clock declares its moons: each an object
			 * with an optional "Surface", a store texture (an equirectangular reflectance map, sRGB, 0° longitude at its
			 * centre, east to the right — Material::SkyAtmosphereResource::setMoonSurface()). */
			static constexpr auto JKMoons{"Moons"};
			static constexpr auto JKSurface{"Surface"};
			/** @brief The star catalogue the chain's star field draws (a Graphics::StarCatalogueResource of the store
			 * `StarCatalogues`, e.g. "YaleBSC5"; graphics doc 42 § "The stars"). A dependency: the sky is loaded with it. */
			static constexpr auto JKStarCatalogue{"StarCatalogue"};
			/** @brief The planets' surfaces, an object { "<planet name>": "<store texture>" } (equirectangular, planetocentric,
			 * 0° east longitude at the centre — Material::SkyAtmosphereResource::setPlanetSurface()). ONE planet in lot S3c
			 * (graphics doc 42 § "The planets"): more is refused. */
			static constexpr auto JKPlanetSurfaces{"PlanetSurfaces"};

			/**
			 * @brief Constructs a dynamic skybox resource.
			 * @param serviceProvider A reference to the service provider.
			 * @param name The name of the resource [std::move].
			 * @param resourceFlags The resource flag bits. Default none.
			 */
			DynamicSkyResource (Resources::AbstractServiceProvider & serviceProvider, std::string name, uint32_t resourceFlags = 0) noexcept
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

			/**
			 * @brief Loads a sky from atmosphere parameters, the material at its defaults.
			 * @param parameters A reference to the parameters of the planet's atmosphere.
			 * @return bool
			 */
			bool load (const AtmosphereParameters & parameters) noexcept;

			/** @copydoc EmEn::Resources::ResourceTrait::memoryOccupied() const noexcept */
			[[nodiscard]]
			size_t
			memoryOccupied () const noexcept override
			{
				return sizeof(*this);
			}

			/**
			 * @brief Returns the atmosphere drawn by this sky: the renderer records its sky-view pass.
			 * @return std::shared_ptr< SkyAtmosphere >
			 */
			[[nodiscard]]
			std::shared_ptr< SkyAtmosphere >
			atmosphere () const noexcept
			{
				return m_atmosphere;
			}

			/**
			 * @brief Returns the material drawing this sky: its artistic controls.
			 * @return std::shared_ptr< Material::SkyAtmosphereResource >
			 */
			[[nodiscard]]
			std::shared_ptr< Material::SkyAtmosphereResource >
			skyMaterial () const noexcept
			{
				return m_skyMaterial;
			}

			/**
			 * @brief Returns the star catalogue this sky names ("StarCatalogue"), or nullptr.
			 * @note Set while the sky loads, before its status publishes it; the catalogue is a dependency, loaded with it.
			 * @return std::shared_ptr< const StarCatalogueResource >
			 */
			[[nodiscard]]
			std::shared_ptr< const StarCatalogueResource >
			starCatalogue () const noexcept
			{
				return m_starCatalogue;
			}

		private:

			/**
			 * @brief Builds the atmosphere, the geometry and the material.
			 * @param parameters A reference to the parameters of the planet's atmosphere.
			 * @param data A pointer to the JSON data holding the material's artistic keys, or nullptr.
			 * @return bool
			 */
			bool build (const AtmosphereParameters & parameters, const Json::Value * data) noexcept;

			/** @copydoc EmEn::Resources::ResourceTrait::onDependenciesLoaded() */
			[[nodiscard]]
			bool onDependenciesLoaded () noexcept override;

			/**
			 * @brief Requests the star catalogue the manifest names ("StarCatalogue"), a dependency of the sky.
			 * @param data A reference to the manifest (an object).
			 * @return bool False for a malformed key or a catalogue the store does not hold; true without the key.
			 */
			[[nodiscard]]
			bool setStarCatalogue (const Json::Value & data) noexcept;

			/**
			 * @brief setGeometry
			 * @param geometry A reference to a geometry smart pointer.
			 * @return bool
			 */
			bool setGeometry (const std::shared_ptr< Geometry::Interface > & geometry) noexcept;

			/**
			 * @brief setMaterial
			 * @param material A reference to a material smart pointer.
			 * @return bool
			 */
			bool setMaterial (const std::shared_ptr< Material::Interface > & material) noexcept;

			std::shared_ptr< Geometry::Interface > m_geometry;
			std::shared_ptr< Material::Interface > m_material;
			std::shared_ptr< Material::SkyAtmosphereResource > m_skyMaterial;
			std::shared_ptr< SkyAtmosphere > m_atmosphere;
			std::shared_ptr< StarCatalogueResource > m_starCatalogue;
	};
}

/* Expose the resource manager as a convenient type. */
namespace EmEn::Resources
{
	using DynamicSkies = Container< Graphics::Renderable::DynamicSkyResource >;
}
