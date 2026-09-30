/*
 * src/Scenes/Component/LineLight.cpp
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

#include "LineLight.hpp"

/* STL inclusions. */
#include <cmath>
#include <limits>
#include <numbers>

/* Local inclusions. */
#include "Math/CurveTessellation.hpp"
#include "Math/Space3D/Collisions/PointSphere.hpp"
#include "Saphir/LightGenerator.hpp"
#include "Scenes/Scene.hpp"
#include "Tracer.hpp"

namespace EmEn::Scenes::Component
{
	using namespace Base;
	using namespace Base::Math;
	using namespace Graphics;
	using namespace Saphir;

	namespace
	{
		/**
		 * @brief Returns the distance from a point to a polyline stored in a light block (world space).
		 * @param block The block (points at PointsOffset, count at PointCountOffset).
		 * @param pointsOffset The float offset of the first point.
		 * @param count The number of points.
		 * @param position The point.
		 * @return float
		 */
		[[nodiscard]]
		float
		distanceToBlockPolyline (const float * block, size_t pointsOffset, uint32_t count, const Vector< 3, float > & position) noexcept
		{
			const auto point = [block, pointsOffset] (uint32_t index) noexcept {
				const auto * p = block + pointsOffset + (4 * static_cast< size_t >(index));

				return Vector< 3, float >{p[0], p[1], p[2]};
			};

			if ( count == 0 )
			{
				return std::numeric_limits< float >::max();
			}

			float best = (position - point(0)).length();

			for ( uint32_t index = 0; index + 1 < count; ++index )
			{
				best = std::min(best, CurveTessellation::distanceToSegment(position, point(index), point(index + 1)));
			}

			return best;
		}
	}

	LineLight::LineLight (const std::string & componentName, const AbstractEntity & parentEntity) noexcept
		: AbstractLightEmitter{componentName, parentEntity, 0}
	{
		const auto & chromaticity = this->emissionChromaticity();

		m_buffer[ColorOffset + 0] = chromaticity[X];
		m_buffer[ColorOffset + 1] = chromaticity[Y];
		m_buffer[ColorOffset + 2] = chromaticity[Z];
		m_buffer[ColorOffset + 3] = 1.0F;
		m_buffer[TubeRadiusOffset] = DefaultTubeRadius;
		m_buffer[RadiusOffset] = DefaultRadius;

		this->setIntensity(1000.0F);

		const std::array< Vector< 3, float >, 2 > segment{Vector< 3, float >{0.0F, 0.0F, 0.0F}, Vector< 3, float >{1.0F, 0.0F, 0.0F}};

		this->setPolyline(std::span< const Vector< 3, float > >{segment});
	}

	bool
	LineLight::playAnimation (uint8_t animationID, const Variant & value, size_t /*cycle*/) noexcept
	{
		switch ( animationID )
		{
			case EmittingState :
				this->enable(value.asBool());
				return true;

			case Color :
				this->setColor(value.asColor());
				return true;

			case Intensity :
				this->setIntensity(value.asFloat());
				return true;

			case Radius :
				this->setRadius(value.asFloat());
				return true;

			default:
				return false;
		}
	}

	void
	LineLight::processLogics (const Scene & scene) noexcept
	{
		if ( !this->isEnabled() )
		{
			return;
		}

		this->updateAnimations(scene.cycle());
	}

	void
	LineLight::move (const CartesianFrame< float > & worldCoordinates) noexcept
	{
		/* No early return on a disabled light (PointLight::move()): a light tracks its frame whether it emits or not. */
		this->updateWorldPoints(worldCoordinates);
	}

	void
	LineLight::setPolyline (std::span< const Vector< 3, float > > points) noexcept
	{
		if ( !this->acceptsFinite("setPolyline", points) )
		{
			return;
		}

		m_localPoints.assign(points.begin(), points.end());

		/* More points than the block carries: resampled by arc length, the ends kept. */
		if ( m_localPoints.size() > MaxPoints )
		{
			std::vector< float > arcLengths(m_localPoints.size(), 0.0F);

			for ( size_t index = 1; index < m_localPoints.size(); ++index )
			{
				arcLengths[index] = arcLengths[index - 1] + (m_localPoints[index] - m_localPoints[index - 1]).length();
			}

			const auto total = arcLengths.back();
			std::vector< Vector< 3, float > > resampled;
			resampled.reserve(MaxPoints);

			size_t cursor = 0;

			for ( uint32_t sample = 0; sample < MaxPoints; ++sample )
			{
				const auto target = total * static_cast< float >(sample) / static_cast< float >(MaxPoints - 1);

				while ( cursor + 2 < arcLengths.size() && arcLengths[cursor + 1] < target )
				{
					++cursor;
				}

				const auto span = arcLengths[cursor + 1] - arcLengths[cursor];
				const auto t = span > 0.0F ? std::clamp((target - arcLengths[cursor]) / span, 0.0F, 1.0F) : 0.0F;

				resampled.emplace_back(m_localPoints[cursor] + (m_localPoints[cursor + 1] - m_localPoints[cursor]) * t);
			}

			resampled.back() = m_localPoints.back();
			m_localPoints = std::move(resampled);
		}

		this->updateWorldPoints(this->getWorldCoordinates());
	}

	void
	LineLight::updateWorldPoints (const CartesianFrame< float > & worldCoordinates) noexcept
	{
		const auto model = worldCoordinates.getModelMatrix();
		const auto count = static_cast< uint32_t >(std::min< size_t >(m_localPoints.size(), MaxPoints));

		for ( uint32_t index = 0; index < MaxPoints; ++index )
		{
			/* The unused tail repeats the last point (never read: the shader stops at the count); no point at all, zeros. */
			Vector< 3, float > local{};

			if ( !m_localPoints.empty() )
			{
				local = index < count ? m_localPoints[index] : m_localPoints.back();
			}
			const auto world = model * Vector< 4, float >{local[X], local[Y], local[Z], 1.0F};

			const auto slot = PointsOffset + (4 * static_cast< size_t >(index));

			m_buffer[slot + 0] = world[X];
			m_buffer[slot + 1] = world[Y];
			m_buffer[slot + 2] = world[Z];
			m_buffer[slot + 3] = 1.0F;
		}

		m_buffer[PointCountOffset] = static_cast< float >(count);

		this->requestVideoMemoryUpdate();
	}

	void
	LineLight::setLuminousFluxPerMetre (float lumensPerMetre) noexcept
	{
		if ( !this->acceptsFinite("setLuminousFluxPerMetre", lumensPerMetre) )
		{
			return;
		}

		/* A Lambertian tube: exitance π L over 2π R square metres per metre. */
		const auto perimeter = 2.0F * std::numbers::pi_v< float > * this->tubeRadius();

		this->setIntensity(std::max(0.0F, lumensPerMetre) / (std::numbers::pi_v< float > * perimeter));
	}

	float
	LineLight::luminousFluxPerMetre () const noexcept
	{
		return std::numbers::pi_v< float > * this->intensity() * 2.0F * std::numbers::pi_v< float > * this->tubeRadius();
	}

	void
	LineLight::setTubeRadius (float radius) noexcept
	{
		if ( !this->acceptsFinite("setTubeRadius", radius) )
		{
			return;
		}

		m_buffer[TubeRadiusOffset] = std::max(radius, 1.0e-5F);

		this->requestVideoMemoryUpdate();
	}

	void
	LineLight::setRadius (float radius) noexcept
	{
		if ( !this->acceptsFinite("setRadius", radius) )
		{
			return;
		}

		m_buffer[RadiusOffset] = std::abs(radius);

		this->requestVideoMemoryUpdate();
	}

	void
	LineLight::onColorChange (const Vector< 3, float > & chromaticity) noexcept
	{
		m_buffer[ColorOffset + 0] = chromaticity[X];
		m_buffer[ColorOffset + 1] = chromaticity[Y];
		m_buffer[ColorOffset + 2] = chromaticity[Z];
	}

	void
	LineLight::onIntensityChange (float intensity) noexcept
	{
		m_buffer[IntensityOffset] = intensity;
	}

	bool
	LineLight::touch (const Vector< 3, float > & position) const noexcept
	{
		/* A null reach means unbounded (AbstractLightEmitter::DefaultRadius). */
		if ( this->radius() <= 0.0F )
		{
			return true;
		}

		return distanceToBlockPolyline(m_buffer.data(), PointsOffset, static_cast< uint32_t >(m_buffer[PointCountOffset]), position) <= this->radius();
	}

	bool
	LineLight::touch (const Space3D::Sphere< float > & target, uint32_t readStateIndex) const noexcept
	{
		/* ⚠️ Render thread: the published block of the latched slot only (PointLight::touch()). */
		const auto & block = this->publishedBlock(readStateIndex);

		if ( block[RadiusOffset] <= 0.0F )
		{
			return true;
		}

		return distanceToBlockPolyline(block.data(), PointsOffset, static_cast< uint32_t >(block[PointCountOffset]), target.position()) <= block[RadiusOffset] + target.radius();
	}

	bool
	LineLight::createOnHardware (Scene & scene) noexcept
	{
		if ( this->isCreated() )
		{
			TraceWarning{ClassId} << "The line light '" << this->name() << "' is already created !";

			return true;
		}

		/* The line lights' shared buffer builds its descriptor sets with the LTC tables at binding 1 (LightSet). */
		if ( !this->addToSharedUniformBuffer(scene.lightSet().lineLightBuffer()) )
		{
			Tracer::error(ClassId, "Unable to create the line light shared uniform buffer !");

			return false;
		}

		this->updateWorldPoints(this->getWorldCoordinates());

		return this->primeVideoMemory();
	}

	void
	LineLight::destroyFromHardware (Scene & /*scene*/) noexcept
	{
		this->removeFromSharedUniformBuffer();
	}

	Saphir::Declaration::UniformBlock
	LineLight::getUniformBlock (uint32_t set, uint32_t binding, bool /*useShadow*/, bool /*useColorProjection*/) const noexcept
	{
		return LightGenerator::getUniformBlock(set, binding, LightType::Line, false, false);
	}

	LineLight::PublishedLine
	LineLight::publishedLine (uint32_t readStateIndex) const noexcept
	{
		const auto & block = this->publishedBlock(readStateIndex);

		PublishedLine line;
		line.color = {block[ColorOffset + 0], block[ColorOffset + 1], block[ColorOffset + 2]};
		line.luminance = block[IntensityOffset];
		line.reach = block[RadiusOffset];
		line.tubeRadius = block[TubeRadiusOffset];
		line.pointCount = std::min(static_cast< uint32_t >(block[PointCountOffset]), MaxPoints);

		for ( uint32_t index = 0; index < line.pointCount; ++index )
		{
			const auto * point = block.data() + PointsOffset + (4 * static_cast< size_t >(index));

			line.points[index] = {point[0], point[1], point[2]};
		}

		return line;
	}

	void
	LineLight::writeUniformBlock (float * destination) noexcept
	{
		std::copy_n(m_buffer.data(), m_buffer.size(), destination);
	}
}
