/*
 * src/Scenes/Component/LineLight.hpp
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
#include <algorithm>
#include <array>
#include <cstring>
#include <span>
#include <vector>

/* Local inclusions for inheritances. */
#include "AbstractLightEmitter.hpp"

/* Local inclusions for usages. */
#include "Math/Vector.hpp"
#include "Saphir/LightGenerator.hpp"

namespace EmEn::Scenes::Component
{
	/**
	 * @brief A LINE light: a tube of emitting surface along a polyline — a neon tube, a laser, an electric arc (owner
	 * decisions 2026-09-30: a true linear light, Linearly Transformed Cosines, no shadow in the first pass).
	 * @note Photometry: a LUMINANCE in nits (cd/m², the emissive contract of Graphics/Photometry.hpp) of a Lambertian
	 * tube of radius tubeRadius(). A metre of it emits π × L × 2πR lumens (setLuminousFluxPerMetre()). Its falloff is
	 * the integral itself; the reach (setRadius(), 0 = unbounded) only windows it, like the point light's.
	 * @note Evaluated per fragment in a forward pass of its own (RenderPassType::LineLightPass): the base lobe by the LTC
	 * integrals over the polyline's segments (Heitz, Dupuy, Hill, Neubelt 2016; Heitz & Hill 2017), the secondary lobes
	 * (clear coat, sheen, subsurface) through the point of the polyline closest to the fragment.
	 * @note The polyline is in the ENTITY's space, at most Saphir::LineLightMaxPoints points (a longer one is resampled
	 * by arc length, its ends kept); the light publishes it in world space.
	 * @extends EmEn::Scenes::Component::AbstractLightEmitter This is a light.
	 */
	class EMEN_API LineLight final : public AbstractLightEmitter
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"LineLight"};

			/** @brief The most points of the polyline (8 segments). */
			static constexpr uint32_t MaxPoints{Saphir::LineLightMaxPoints};

			/** @brief The default radius of the emitting tube, in metres (a thin tube). */
			static constexpr float DefaultTubeRadius{0.01F};

			/**
			 * @brief Constructs a line light: a 1 m segment along +X, 1000 nits, unbounded reach.
			 * @param componentName A reference to a string.
			 * @param parentEntity A reference to the parent entity.
			 */
			LineLight (const std::string & componentName, const AbstractEntity & parentEntity) noexcept;

			LineLight (const LineLight & copy) noexcept = delete;
			LineLight (LineLight && copy) noexcept = delete;
			LineLight & operator= (const LineLight & copy) noexcept = delete;
			LineLight & operator= (LineLight && copy) noexcept = delete;

			/**
			 * @brief Destructs the line light.
			 */
			~LineLight () override = default;

			/** @copydoc EmEn::Scenes::Component::Abstract::getComponentType() */
			[[nodiscard]]
			const char *
			getComponentType () const noexcept override
			{
				return ClassId;
			}

			/** @copydoc EmEn::Scenes::Component::Abstract::isComponent() */
			[[nodiscard]]
			bool
			isComponent (const char * classID) const noexcept override
			{
				return strcmp(ClassId, classID) == 0;
			}

			/** @copydoc EmEn::Scenes::Component::Abstract::processLogics() */
			void processLogics (const Scene & scene) noexcept override;

			/** @copydoc EmEn::Scenes::Component::Abstract::move() */
			void move (const Base::Math::CartesianFrame< float > & worldCoordinates) noexcept override;

			/** @copydoc EmEn::Scenes::Component::Abstract::shouldBeRemoved() */
			[[nodiscard]]
			bool
			shouldBeRemoved () const noexcept override
			{
				return false;
			}

			/** @copydoc EmEn::Scenes::Component::AbstractLightEmitter::touch(const Base::Math::Vector< 3, float > &) const */
			[[nodiscard]]
			bool touch (const Base::Math::Vector< 3, float > & position) const noexcept override;

			/** @copydoc EmEn::Scenes::Component::AbstractLightEmitter::touch(const Base::Math::Space3D::Sphere< float > &, uint32_t) const */
			[[nodiscard]]
			bool touch (const Base::Math::Space3D::Sphere< float > & target, uint32_t readStateIndex) const noexcept override;

			/** @copydoc EmEn::Scenes::Component::AbstractLightEmitter::createOnHardware() */
			[[nodiscard]]
			bool createOnHardware (Scene & scene) noexcept override;

			/** @copydoc EmEn::Scenes::Component::AbstractLightEmitter::destroyFromHardware() */
			void destroyFromHardware (Scene & scene) noexcept override;

			/** @copydoc EmEn::Scenes::Component::AbstractLightEmitter::shadowMap() */
			[[nodiscard]]
			std::shared_ptr< Graphics::RenderTarget::Abstract >
			shadowMap () const noexcept override
			{
				return nullptr;
			}

			/** @copydoc EmEn::Scenes::Component::AbstractLightEmitter::getUniformBlock() */
			[[nodiscard]]
			Saphir::Declaration::UniformBlock getUniformBlock (uint32_t set, uint32_t binding, bool useShadow, bool useColorProjection) const noexcept override;

			/**
			 * @copydoc EmEn::Scenes::Component::AbstractLightEmitter::setPCFRadius()
			 * @note A line light casts no shadow: ignored.
			 */
			void
			setPCFRadius (float /*radius*/) noexcept override
			{

			}

			/** @copydoc EmEn::Scenes::Component::AbstractLightEmitter::PCFRadius() */
			[[nodiscard]]
			float
			PCFRadius () const noexcept override
			{
				return 0.0F;
			}

			/**
			 * @copydoc EmEn::Scenes::Component::AbstractLightEmitter::setShadowBias()
			 * @note A line light casts no shadow: ignored.
			 */
			void
			setShadowBias (float /*bias*/) noexcept override
			{

			}

			/** @copydoc EmEn::Scenes::Component::AbstractLightEmitter::shadowBias() */
			[[nodiscard]]
			float
			shadowBias () const noexcept override
			{
				return 0.0F;
			}

			/**
			 * @brief Sets the polyline the light emits along, in the entity's space.
			 * @param points The points (2 at least to emit; more than MaxPoints are resampled by arc length, the ends kept).
			 * @return void
			 */
			void setPolyline (std::span< const Base::Math::Vector< 3, float > > points) noexcept;

			/**
			 * @brief Returns the polyline, in the entity's space (after the resampling).
			 * @return const std::vector< Base::Math::Vector< 3, float > > &
			 */
			[[nodiscard]]
			const std::vector< Base::Math::Vector< 3, float > > &
			polyline () const noexcept
			{
				return m_localPoints;
			}

			/**
			 * @brief Sets the luminance of the tube, in nits (cd/m²) — the light's intensity.
			 * @param nits The luminance, 0 or more.
			 * @return void
			 */
			void
			setLuminance (float nits) noexcept
			{
				if ( !this->acceptsFinite("setLuminance", nits) )
				{
					return;
				}

				this->setIntensity(std::max(0.0F, nits));
			}

			/**
			 * @brief Sets the luminance from the luminous flux a metre of the tube emits (π × L × 2πR lumens per metre).
			 * @param lumensPerMetre The flux per metre, 0 or more.
			 * @return void
			 */
			void setLuminousFluxPerMetre (float lumensPerMetre) noexcept;

			/**
			 * @brief Returns the luminous flux a metre of the tube emits, in lumens.
			 * @return float
			 */
			[[nodiscard]]
			float luminousFluxPerMetre () const noexcept;

			/**
			 * @brief Sets the radius of the emitting tube.
			 * @param radius The radius in metres, above 0.
			 * @return void
			 */
			void setTubeRadius (float radius) noexcept;

			/**
			 * @brief Returns the radius of the emitting tube, in metres.
			 * @return float
			 */
			[[nodiscard]]
			float
			tubeRadius () const noexcept
			{
				return m_buffer[TubeRadiusOffset];
			}

			/**
			 * @brief Sets the reach: the distance from the polyline beyond which the light is windowed to 0 (0 = unbounded).
			 * @param radius The reach in metres, 0 or more.
			 * @return void
			 */
			void setRadius (float radius) noexcept;

			/**
			 * @brief Returns the reach, in metres (0 = unbounded).
			 * @return float
			 */
			[[nodiscard]]
			float
			radius () const noexcept
			{
				return m_buffer[RadiusOffset];
			}

			/** @brief What the render thread reads of a line light: its published block, world space. */
			struct PublishedLine
			{
				std::array< Base::Math::Vector< 3, float >, MaxPoints > points{};
				Base::Math::Vector< 3, float > color;
				uint32_t pointCount{0};
				float luminance{0.0F};
				float tubeRadius{0.0F};
				float reach{0.0F};
			};

			/**
			 * @brief Returns the line as published for a render state slot (the traced effects pack it per segment).
			 * @note Render thread: the published block only, never the parent entity (a retired light).
			 * @param readStateIndex The frame's read slot.
			 * @return PublishedLine
			 */
			[[nodiscard]]
			PublishedLine publishedLine (uint32_t readStateIndex) const noexcept;

		private:

			/** @copydoc EmEn::Animations::AnimatableInterface::playAnimation() */
			bool playAnimation (uint8_t animationID, const Base::Variant & value, size_t cycle) noexcept override;

			/** @copydoc EmEn::Scenes::Component::AbstractLightEmitter::createShadowDescriptorSet() */
			bool
			createShadowDescriptorSet (Scene & /*scene*/) noexcept override
			{
				return false;
			}

			/** @copydoc EmEn::Scenes::Component::AbstractLightEmitter::getFovOrNear() */
			[[nodiscard]]
			float
			getFovOrNear () const noexcept override
			{
				return 90.0F;
			}

			/** @copydoc EmEn::Scenes::Component::AbstractLightEmitter::getDistanceOrFar() */
			[[nodiscard]]
			float
			getDistanceOrFar () const noexcept override
			{
				return this->radius() > 0.0F ? this->radius() : DefaultGraphicsShadowMappingViewDistance;
			}

			/** @copydoc EmEn::Scenes::Component::AbstractLightEmitter::isOrthographicProjection() */
			[[nodiscard]]
			bool
			isOrthographicProjection () const noexcept override
			{
				return false;
			}

			/** @copydoc EmEn::Scenes::Component::AbstractLightEmitter::writeUniformBlock(float *) */
			void writeUniformBlock (float * destination) noexcept override;

			/** @copydoc EmEn::Scenes::Component::AbstractLightEmitter::onColorChange() */
			void onColorChange (const Base::Math::Vector< 3, float > & chromaticity) noexcept override;

			/** @copydoc EmEn::Scenes::Component::AbstractLightEmitter::onIntensityChange() */
			void onIntensityChange (float intensity) noexcept override;

			/**
			 * @brief Writes the polyline in world space into the block, from the entity's frame.
			 * @param worldCoordinates The entity's world frame.
			 * @return void
			 */
			void updateWorldPoints (const Base::Math::CartesianFrame< float > & worldCoordinates) noexcept;

			/* Uniform block (std140, Saphir::LightGenerator::getUniformBlock(LightType::Line)):
			 * vec4 Color: floats 0-3; float Intensity (nits): 4; float Radius (reach): 5; float TubeRadius: 6;
			 * float PointCount: 7; vec4 Points[MaxPoints]: 8 + 4 i (xyz world, w unused). */
			static constexpr auto ColorOffset{0UL};
			static constexpr auto IntensityOffset{4UL};
			static constexpr auto RadiusOffset{5UL};
			static constexpr auto TubeRadiusOffset{6UL};
			static constexpr auto PointCountOffset{7UL};
			static constexpr auto PointsOffset{8UL};

			std::vector< Base::Math::Vector< 3, float > > m_localPoints;
			std::array< float, PointsOffset + (4 * static_cast< size_t >(MaxPoints)) > m_buffer{};
	};
}
