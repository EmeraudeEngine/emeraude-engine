/*
 * src/Graphics/Renderable/AbstractBackground.hpp
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

/* Local inclusions for inheritances. */
#include "Abstract.hpp"

/* STL inclusions. */
#include <vector>

/* Local inclusions for usages. */
#include "Graphics/CelestialBody.hpp"
#include "Graphics/Geometry/IndexedVertexResource.hpp"
#include "Graphics/Photometry.hpp"
#include "PixelFactory/Color.hpp"

namespace EmEn::Graphics::TextureResource
{
	class TextureCubemap;
}

namespace EmEn::Graphics::Renderable
{
	/**
	 * @brief An abstract class to define the far background of a scene.
	 * @extends EmEn::Graphics::Renderable::Abstract This class is a renderable object in the 3D world.
	 */
	class EMEN_LEAN_API AbstractBackground : public Abstract
	{
		public:

			/**
			 * @brief Default background luminance, in nits (cd/m²).
			 * @note An overcast sky sits around 8000 nits, and a clear blue sky away from the sun
			 * in the same range — which is what makes a sky readable next to a 100000 lx sun.
			 */
			static constexpr auto DefaultLuminance{8000.0F};

			/**
			 * @brief Copy constructor.
			 * @param copy A reference to the copied instance.
			 */
			AbstractBackground (const AbstractBackground & copy) noexcept = delete;

			/**
			 * @brief Move constructor.
			 * @param copy A reference to the copied instance.
			 */
			AbstractBackground (AbstractBackground && copy) noexcept = delete;

			/**
			 * @brief Copy assignment.
			 * @param copy A reference to the copied instance.
			 * @return AbstractBackground &
			 */
			AbstractBackground & operator= (const AbstractBackground & copy) noexcept = delete;

			/**
			 * @brief Move assignment.
			 * @param copy A reference to the copied instance.
			 * @return AbstractBackground &
			 */
			AbstractBackground & operator= (AbstractBackground && copy) noexcept = delete;

			/**
			 * @brief Destructs the renderable background.
			 */
			~AbstractBackground () override = default;

			/** @copydoc EmEn::Graphics::Renderable::Abstract::boundingBox() */
			[[nodiscard]]
			const Base::Math::Space3D::AACuboid< float > &
			boundingBox () const noexcept final
			{
				return NullBoundingBox;
			}

			/** @copydoc EmEn::Graphics::Renderable::Abstract::boundingSphere() */
			[[nodiscard]]
			const Base::Math::Space3D::Sphere< float > &
			boundingSphere () const noexcept final
			{
				return NullBoundingSphere;
			}

			/**
			 * @brief Sets the LUMINANCE of the background, in nits (cd/m²).
			 * @note A background is a self-illuminating surface: this is the physical scale
			 * applied to its normalized [0,1] source, both when it is DRAWN (the skybox material's
			 * emissive strength) and when it is REFLECTED (the IBL contribution of every material
			 * sampling the environment cubemap). An overcast sky is ~8000 nits.
			 * @param nits The luminance, in candela per square meter.
			 * @return void
			 */
			void
			setLuminance (float nits) noexcept
			{
				m_luminance = std::max(0.0F, nits);
			}

			/**
			 * @brief Returns the luminance of the background, in nits.
			 * @return float
			 */
			[[nodiscard]]
			float
			luminance () const noexcept
			{
				return m_luminance;
			}

			/**
			 * @brief Sets the average color to represent the background.
			 * @param color A reference to a color.
			 * @return void
			 */
			void
			setAverageColor (const Base::PixelFactory::Color< float > & color) noexcept
			{
				m_averageColor = color;
			}

			/**
			 * @brief Sets the illuminance the background pours on the scene, in lux.
			 * @note This is the value the LightSet ambient light receives when the scene derives
			 * its lighting from the background. References: open shade under a clear sky
			 * 20000 lx, overcast 5000 lx, moonlit night ~1 lx.
			 * @param lux The ambient illuminance, in lux.
			 * @return void
			 */
			void
			setAmbientIlluminance (float lux) noexcept
			{
				m_ambientIlluminance = std::max(0.0F, lux);
			}

			/**
			 * @brief Returns the illuminance the background pours on the scene, in lux.
			 * @note When the manifest does not declare it, it is derived from the luminance and
			 * the illuminance factor MEASURED on the actual source (`E = L x factor`, see
			 * setAmbientIlluminanceFactor()); pi — the uniform dome — until a loader measures.
			 * @return float
			 */
			[[nodiscard]]
			float
			ambientIlluminance () const noexcept
			{
				return m_ambientIlluminance < 0.0F ? m_luminance * m_ambientIlluminanceFactor : m_ambientIlluminance;
			}

			/**
			 * @brief Adds a celestial body (a sun, a moon, ...) to the background description.
			 * @param celestialBody A reference to a celestial body.
			 * @return void
			 */
			void
			addStar (const CelestialBody & celestialBody) noexcept
			{
				m_stars.emplace_back(celestialBody);
			}

			/**
			 * @brief Returns the celestial bodies identified in the background.
			 * @note Can be empty: a background without an identifiable light point (overcast sky,
			 * nebula, cave dome) only provides ambiance. The first one is meant to become the
			 * scene main directional light.
			 * @return const std::vector< CelestialBody > &
			 */
			[[nodiscard]]
			const std::vector< CelestialBody > &
			stars () const noexcept
			{
				return m_stars;
			}

			/**
			 * @brief Returns the average color of the background.
			 * @return const Libraries::PixelFactory::Color< float > &
			 */
			[[nodiscard]]
			const Base::PixelFactory::Color< float > &
			averageColor () const noexcept
			{
				return m_averageColor;
			}

			/**
			 * @brief Returns the environment cubemap texture for IBL (Image-Based Lighting).
			 * @note Override in derived classes that provide a cubemap (e.g., SkyBoxResource).
			 * @return Shared pointer to the cubemap texture, or nullptr if not available.
			 */
			[[nodiscard]]
			virtual std::shared_ptr< TextureResource::TextureCubemap >
			environmentCubemap () const noexcept
			{
				return nullptr;
			}

			/**
			 * @brief Creates and/or returns a skybox (cube) geometry.
			 * @param serviceProvider A reference to the resource manager through a service provider.
			 * @return shared_ptr< Geometry::IndexedVertexResource >
			 */
			[[nodiscard]]
			static std::shared_ptr< Geometry::IndexedVertexResource > getSkyBoxGeometry (Resources::AbstractServiceProvider & serviceProvider) noexcept;

			/**
			 * @brief Creates and/or returns a sky dome (sphere) geometry.
			 * @param serviceProvider A reference to the resource manager through a service provider.
			 * @return shared_ptr< Geometry::IndexedVertexResource >
			 */
			[[nodiscard]]
			static std::shared_ptr< Geometry::IndexedVertexResource > getSkyDomeGeometry (Resources::AbstractServiceProvider & serviceProvider) noexcept;

		protected:

			/**
			 * @brief Constructs an abstract renderable background.
			 * @param serviceProvider A reference to the service provider.
			 * @param name The name of the resource [std::move].
			 * @param resourceFlags The resource flag bits. Default none.
			 */
			AbstractBackground (Resources::AbstractServiceProvider & serviceProvider, std::string name, uint32_t resourceFlags) noexcept
				: Abstract{serviceProvider, std::move(name), resourceFlags}
			{

			}

			/**
			 * @brief Sets the factor deriving the default ambient illuminance from the luminance.
			 * @note Loaders holding the actual pixels call this with the MEASURED upper-hemisphere
			 * integral (owner decision, Jul 2026: the uniform-dome pi over-lit every sky whose
			 * dome is partly dark — see CubemapResource::hemisphereIlluminanceFactor()). An
			 * explicit "AmbientIlluminance" manifest key bypasses the derivation entirely.
			 * @param factor The illuminance factor (pi = uniform dome).
			 * @return void
			 */
			void
			setAmbientIlluminanceFactor (float factor) noexcept
			{
				m_ambientIlluminanceFactor = std::max(1e-4F, factor);
			}

			/**
			 * @brief Parses the photometric part of a background manifest — the SINGLE parsing
			 * point shared by every background type (sky box, dynamic sky, color background).
			 * @note Reads the optional keys "Luminance" (nits, defaults to a clear day),
			 * "AverageColor" (sRGB, defaults to the loaded source average), "AmbientIlluminance"
			 * (lux, defaults to pi * luminance) and "Stars" (array of celestial bodies, each with
			 * "Direction" and "Illuminance" required, "Type", "Temperature"/"Color",
			 * "AngularDiameter" and "InTexture" optional).
			 * @param data A reference to the JSON data.
			 * @return bool
			 */
			bool parsePhotometry (const Json::Value & data) noexcept;

		private:

			static constexpr auto TracerTag{"AbstractBackground"};

			static constexpr auto SkyBoxGeometryName{"SkyBoxGeometry"};
			static constexpr auto SkyDomeGeometryName{"SkyDomeGeometry"};

			/* FIXME: Set the correct size. */
			static constexpr auto SkySize{512.0F};

			std::vector< CelestialBody > m_stars;
			Base::PixelFactory::Color< float > m_averageColor{10.0F / 256.0F, 24.0F / 256.0F, 43.0F / 256.0F, 1.0F};
			float m_luminance{DefaultLuminance}; /**< Physical luminance of the background, in nits. */
			float m_ambientIlluminance{-1.0F}; /**< Ambient illuminance, in lux. Negative means "derive from the luminance". */
			float m_ambientIlluminanceFactor{Photometry::UniformDomeIlluminanceFactor}; /**< Derivation factor, measured by loaders. */
	};
}
