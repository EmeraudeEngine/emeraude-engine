/*
 * src/Animations/Sequence.cpp
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

#include "Sequence.hpp"

/* STL inclusions. */
#include <cmath>
#include <ostream>
#include <tuple>
#include <utility>

/* Local inclusions. */
#include "Libs/Math/CartesianFrame.hpp"
#include "Libs/PixelFactory/Color.hpp"
#include "Constants.hpp"
#include "Tracer.hpp"

namespace EmEn::Animations
{
	using namespace EmEn::Libs;
	using namespace EmEn::Libs::Math;

	Variant
	Sequence::getLinearInterpolation (const KeyFrames::const_iterator & itStart, const KeyFrames::const_iterator & itEnd, float factor) noexcept
	{
		if ( factor <= 0.0F )
		{
			return itStart->second.value();
		}

		if ( factor >= 1.0F )
		{
			return itEnd->second.value();
		}

		switch ( itStart->second.value().type() )
		{
			case Variant::Type::Integer8 :
				return Variant{static_cast< int8_t >(linearInterpolation< float >(
					itStart->second.value().asInteger8(),
					itEnd->second.value().asInteger8(),
					factor
				))};

			case Variant::Type::UnsignedInteger8 :
				return Variant{static_cast< uint8_t >(linearInterpolation< float >(
					itStart->second.value().asUnsignedInteger8(),
					itEnd->second.value().asUnsignedInteger8(),
					factor
				))};

			case Variant::Type::Integer16 :
				return Variant{static_cast< int16_t >(linearInterpolation< float >(
					itStart->second.value().asInteger16(),
					itEnd->second.value().asInteger16(),
					factor
				))};

			case Variant::Type::UnsignedInteger16 :
				return Variant{static_cast< uint16_t >(linearInterpolation< float >(
					itStart->second.value().asUnsignedInteger16(),
					itEnd->second.value().asUnsignedInteger16(),
					factor
				))};

			case Variant::Type::Integer32 :
				return Variant{static_cast< int32_t >(linearInterpolation< double >(
					itStart->second.value().asInteger32(),
					itEnd->second.value().asInteger32(),
					factor
				))};

			case Variant::Type::UnsignedInteger32 :
				return Variant{static_cast< uint32_t >(linearInterpolation< double >(
					itStart->second.value().asUnsignedInteger32(),
					itEnd->second.value().asUnsignedInteger32(),
					factor
				))};

			case Variant::Type::Integer64 :
				return Variant{static_cast< int64_t >(linearInterpolation< long double >(
					itStart->second.value().asInteger64(),
					itEnd->second.value().asInteger64(),
					factor
				))};

			case Variant::Type::UnsignedInteger64 :
				return Variant{static_cast< uint64_t >(linearInterpolation< long double >(
					itStart->second.value().asUnsignedInteger64(),
					itEnd->second.value().asUnsignedInteger64(),
					factor
				))};

			case Variant::Type::Float :
				return Variant{linearInterpolation(
					itStart->second.value().asFloat(),
					itEnd->second.value().asFloat(),
					factor
				)};

			case Variant::Type::Double :
				return Variant{linearInterpolation(
					itStart->second.value().asDouble(),
					itEnd->second.value().asDouble(),
					factor
				)};

			case Variant::Type::LongDouble :
				return Variant{linearInterpolation(
					itStart->second.value().asLongDouble(),
					itEnd->second.value().asLongDouble(),
					factor
				)};

			case Variant::Type::Boolean :
			{
				const auto operandA = static_cast< float >(itStart->second.value().asBool());
				const auto operandB = static_cast< float >(itEnd->second.value().asBool());

				if ( linearInterpolation(operandA, operandB, factor) > 0.5F )
				{
					return Variant{true};
				}
			}
				return Variant{false};

			case Variant::Type::Vector2Float :
				return Variant{linearInterpolation(
					itStart->second.value().asVector2Float(),
					itEnd->second.value().asVector2Float(),
					factor
				)};

			case Variant::Type::Vector3Float :
				return Variant{linearInterpolation(
					itStart->second.value().asVector3Float(),
					itEnd->second.value().asVector3Float(),
					factor
				)};

			case Variant::Type::Vector4Float :
				return Variant{linearInterpolation(
					itStart->second.value().asVector4Float(),
					itEnd->second.value().asVector4Float(),
					factor
				)};

			case Variant::Type::Matrix2Float :
				return Variant{linearInterpolation(
					itStart->second.value().asMatrix2Float(),
					itEnd->second.value().asMatrix2Float(),
					factor
				)};

			case Variant::Type::Matrix3Float :
				return Variant{linearInterpolation(
					itStart->second.value().asMatrix3Float(),
					itEnd->second.value().asMatrix3Float(),
					factor
				)};

			case Variant::Type::Matrix4Float :
				return Variant{linearInterpolation(
					itStart->second.value().asMatrix4Float(),
					itEnd->second.value().asMatrix4Float(),
					factor
				)};

			case Variant::Type::CartesianFrameFloat :
				return Variant{CartesianFrame< float >::linearInterpolation(
					itStart->second.value().asCartesianFrameFloat(),
					itEnd->second.value().asCartesianFrameFloat(),
					factor
				)};

			case Variant::Type::Color :
				return Variant{PixelFactory::Color< float >::linearInterpolation(
					itStart->second.value().asColor(),
					itEnd->second.value().asColor(),
					factor
				)};

			case Variant::Type::Null :
				return {};
		}

		return {};
	}

	Variant
	Sequence::getCosineInterpolation (const KeyFrames::const_iterator & itStart, const KeyFrames::const_iterator & itEnd, float factor) noexcept
	{
		if ( factor <= 0.0F )
		{
			return itStart->second.value();
		}

		if ( factor >= 1.0F )
		{
			return itEnd->second.value();
		}

		switch ( itStart->second.value().type() )
		{
			case Variant::Type::Integer8 :
				return Variant{static_cast< int8_t >(cosineInterpolation< float >(
					itStart->second.value().asInteger8(),
					itEnd->second.value().asInteger8(),
					factor
				))};

			case Variant::Type::UnsignedInteger8 :
				return Variant{static_cast< uint8_t >(cosineInterpolation< float >(
					itStart->second.value().asUnsignedInteger8(),
					itEnd->second.value().asUnsignedInteger8(),
					factor
				))};

			case Variant::Type::Integer16 :
				return Variant{static_cast< int16_t >(cosineInterpolation< float >(
					itStart->second.value().asInteger16(),
					itEnd->second.value().asInteger16(),
					factor
				))};

			case Variant::Type::UnsignedInteger16 :
				return Variant{static_cast< uint16_t >(cosineInterpolation< float >(
					itStart->second.value().asUnsignedInteger16(),
					itEnd->second.value().asUnsignedInteger16(),
					factor
				))};

			case Variant::Type::Integer32 :
				return Variant{static_cast< int32_t >(cosineInterpolation< double > (
					itStart->second.value().asInteger32(),
					itEnd->second.value().asInteger32(),
					factor
				))};

			case Variant::Type::UnsignedInteger32 :
				return Variant{static_cast< uint32_t >(cosineInterpolation< double >(
					itStart->second.value().asUnsignedInteger32(),
					itEnd->second.value().asUnsignedInteger32(),
					factor
				))};

			case Variant::Type::Integer64 :
				return Variant{static_cast< int64_t >(cosineInterpolation< long double >(
					itStart->second.value().asInteger64(),
					itEnd->second.value().asInteger64(),
					factor
				))};

			case Variant::Type::UnsignedInteger64 :
				return Variant{static_cast< uint64_t >(cosineInterpolation< long double >(
					itStart->second.value().asUnsignedInteger64(),
					itEnd->second.value().asUnsignedInteger64(),
					factor
				))};

			case Variant::Type::Float :
				return Variant{cosineInterpolation(
					itStart->second.value().asFloat(),
					itEnd->second.value().asFloat(),
					factor
				)};

			case Variant::Type::Double :
				return Variant{cosineInterpolation(
					itStart->second.value().asDouble(),
					itEnd->second.value().asDouble(),
					  factor
				  )};

			case Variant::Type::LongDouble :
				return Variant{cosineInterpolation(
					itStart->second.value().asLongDouble(),
					itEnd->second.value().asLongDouble(),
					factor
				)};

			case Variant::Type::Boolean :
			{
				const auto operandA = static_cast< float >(itStart->second.value().asBool());
				const auto operandB = static_cast< float >(itEnd->second.value().asBool());

				if ( cosineInterpolation(operandA, operandB, factor) > 0.5F)
				{
					return Variant{true};
				}
			}
				return Variant{false};

			case Variant::Type::Vector2Float :
				return Variant{cosineInterpolation(
					itStart->second.value().asVector2Float(),
					itEnd->second.value().asVector2Float(),
					factor
				)};

			case Variant::Type::Vector3Float :
				return Variant{cosineInterpolation(
					itStart->second.value().asVector3Float(),
					itEnd->second.value().asVector3Float(),
					factor
				)};

			case Variant::Type::Vector4Float :
				return Variant{cosineInterpolation(
					itStart->second.value().asVector4Float(),
					itEnd->second.value().asVector4Float(),
					factor
				)};

			case Variant::Type::Matrix2Float :
				return Variant{cosineInterpolation(
					itStart->second.value().asMatrix2Float(),
					itEnd->second.value().asMatrix2Float(),
					factor
				)};

			case Variant::Type::Matrix3Float :
				return Variant{cosineInterpolation(
					itStart->second.value().asMatrix3Float(),
					itEnd->second.value().asMatrix3Float(),
					factor
				)};

			case Variant::Type::Matrix4Float :
				return Variant{cosineInterpolation(
					itStart->second.value().asMatrix4Float(),
					itEnd->second.value().asMatrix4Float(),
					factor
				)};

			case Variant::Type::CartesianFrameFloat :
				return Variant{CartesianFrame< float >::cosineInterpolation(
					itStart->second.value().asCartesianFrameFloat(),
					itEnd->second.value().asCartesianFrameFloat(),
					factor
				)};

			case Variant::Type::Color :
				return Variant{PixelFactory::Color< float >::cosineInterpolation(
					itStart->second.value().asColor(),
					itEnd->second.value().asColor(),
					factor
				)};

			case Variant::Type::Null :
				return {};
		}

		return {};
	}

	Variant
	Sequence::getCubicInterpolation (const KeyFrames::const_iterator & /*itStart*/, const KeyFrames::const_iterator & /*itEnd*/, float /*factor*/) noexcept
	{
		/* FIXME: Create the method. */
		return {};
	}

	void
	Sequence::addKeyFrame (uint32_t timeCode, const Variant & value, InterpolationType interpolation) noexcept
	{
		if ( timeCode > m_duration )
		{
			TraceWarning{ClassId} << "The time code at " << timeCode << "ms is out of the timeline (" << m_duration << "ms) !";

			return;
		}

		m_keyFrames.emplace(
			std::piecewise_construct,
			std::forward_as_tuple(timeCode),
			std::forward_as_tuple(value, interpolation)
		);
	}

	void
	Sequence::addKeyFrame (float position, const Variant & value, InterpolationType interpolation) noexcept
	{
		const auto timePoint = static_cast< float >(m_duration) * clampToUnit(position);

		this->addKeyFrame(static_cast< uint32_t >(std::round(timePoint)), value, interpolation);
	}

	void
	Sequence::setCurrentTime (uint32_t timeCode) noexcept
	{
		if ( timeCode > m_duration )
		{
			TraceWarning{ClassId} << "The time code at " << timeCode << "ms is out of the timeline (" << m_duration << "ms) !";

			return;
		}

		m_elapsedTime = timeCode;
	}

	void
	Sequence::setCurrentTime (float position) noexcept
	{
		const auto timePoint = static_cast< float >(m_duration) * clampToUnit(position);

		this->setCurrentTime(static_cast< uint32_t >(std::round(timePoint)));
	}

	bool
	Sequence::play () noexcept
	{
		if ( m_keyFrames.size() < 2 )
		{
			Tracer::warning(ClassId, "Incomplete animation !");

			return false;
		}

		m_isPlaying = true;

		return true;
	}

	bool
	Sequence::pause () noexcept
	{
		if ( m_isPlaying )
		{
			m_isPlaying = false;

			return true;
		}

		return false;
	}

	Variant
	Sequence::getNextValue () noexcept
	{
		if ( !m_isPlaying )
		{
			return {};
		}

		Variant result;
		auto itStart = m_keyFrames.cend();
		auto itEnd = m_keyFrames.cend();

		for ( auto frameIt = m_keyFrames.cbegin(); frameIt != m_keyFrames.cend(); ++frameIt )
		{
			if ( m_elapsedTime > frameIt->first )
			{
				itStart = frameIt;
			}

			if ( itEnd == m_keyFrames.cend() && m_elapsedTime <= frameIt->first )
			{
				itEnd = frameIt;

				break;
			}
		}

		if ( itStart != m_keyFrames.cend() && itEnd != m_keyFrames.cend() )
		{
			const auto normalizedStart = m_elapsedTime - itStart->first;
			const auto normalizedEnd = itEnd->first - itStart->first;

			auto factor = clampToUnit(static_cast< float >(normalizedStart) / static_cast< float >(normalizedEnd));

			if ( !Utility::isZero(m_noise) )
			{
				factor += Utility::quickRandom(-m_noise, m_noise);
			}

			/* Interpolate the targeted value. */
			switch ( itStart->second.interpolation() )
			{
				case InterpolationType::Linear :
					result = getLinearInterpolation(itStart, itEnd, factor);
					break;

				case InterpolationType::Cosine :
					result = getCosineInterpolation(itStart, itEnd, factor);
					break;

				case InterpolationType::Cubic :
					result = getCubicInterpolation(itStart, itEnd, factor);
					break;
			}
		}
		else if ( itStart == m_keyFrames.cend() )
		{
			/* Return the first key frame value. */
			result = m_keyFrames.cbegin()->second.value();
		}
		else
		{
			/* Return the last key frame value. */
			result = m_keyFrames.rbegin()->second.value();
		}

		this->advanceTime();

		return result;
	}

	void
	Sequence::advanceTime () noexcept
	{
		m_elapsedTime += EngineUpdateCycleDurationMS< uint32_t >;

		/* Checks for the end of the cycle. */
		if ( m_elapsedTime < m_duration )
		{
			return;
		}

		if ( m_repeat < 0 )
		{
			/* NOTE: A loop animation just restarts the time. */
			m_elapsedTime = 0;
		}
		else
		{
			if ( m_repeat > 0 )
			{
				/* NOTE: Restart the time for a new repetition. */
				m_repeat--;

				m_elapsedTime = 0;
			}
			else
			{
				/* NOTE: The sequence has reached the end. */
				m_isPlaying = false;

				m_elapsedTime = m_duration;
			}
		}
	}
}
