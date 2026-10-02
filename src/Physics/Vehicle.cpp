/*
 * src/Physics/Vehicle.cpp
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

#include "Vehicle.hpp"

/* STL inclusions. */
#include <algorithm>
#include <cmath>
#include <numbers>

namespace EmEn::Physics
{
	using namespace Base::Math;

	namespace
	{
		/** @brief rad/s → RPM. */
		constexpr float RadiansPerSecondToRPM{60.0F / (2.0F * std::numbers::pi_v< float >)};

		/** @brief Under this speed (m/s) the vehicle counts as stopped (a reverse may be engaged). */
		constexpr float VehicleStoppedSpeed{1.0F};

		/** @brief A unit vector within this of length 1. */
		constexpr float VehicleUnitTolerance{1.0e-3F};

		[[nodiscard]]
		bool
		isFiniteVector (const Vector< 3, float > & vector) noexcept
		{
			return std::isfinite(vector[X]) && std::isfinite(vector[Y]) && std::isfinite(vector[Z]);
		}

		[[nodiscard]]
		bool
		isUnit (const Vector< 3, float > & vector) noexcept
		{
			return isFiniteVector(vector) && std::abs(vector.length() - 1.0F) <= VehicleUnitTolerance;
		}

		[[nodiscard]]
		bool
		isNonNegative (float value) noexcept
		{
			return std::isfinite(value) && value >= 0.0F;
		}

		[[nodiscard]]
		bool
		isPositive (float value) noexcept
		{
			return std::isfinite(value) && value > 0.0F;
		}
	}

	void
	VehicleSettings::useDefaultCurvesAndRatios () noexcept
	{
		/* Jolt's defaults (WheelSettingsWV, VehicleEngineSettings, VehicleTransmissionSettings; MIT): data, not code. */
		for ( auto & wheel : wheels )
		{
			wheel.longitudinalFriction.clear();
			static_cast< void >(wheel.longitudinalFriction.addPoint(0.0F, 0.0F));
			static_cast< void >(wheel.longitudinalFriction.addPoint(0.06F, 1.2F));
			static_cast< void >(wheel.longitudinalFriction.addPoint(0.2F, 1.0F));

			wheel.lateralFriction.clear();
			static_cast< void >(wheel.lateralFriction.addPoint(0.0F, 0.0F));
			static_cast< void >(wheel.lateralFriction.addPoint(3.0F, 1.2F));
			static_cast< void >(wheel.lateralFriction.addPoint(20.0F, 1.0F));
		}

		engine.normalizedTorque.clear();
		static_cast< void >(engine.normalizedTorque.addPoint(0.0F, 0.8F));
		static_cast< void >(engine.normalizedTorque.addPoint(0.66F, 1.0F));
		static_cast< void >(engine.normalizedTorque.addPoint(1.0F, 0.8F));

		gearbox.forwardRatios.clear();

		for ( const auto ratio : {2.66F, 1.78F, 1.3F, 1.0F, 0.74F} )
		{
			gearbox.forwardRatios.push_back(ratio);
		}

		gearbox.reverseRatios.clear();
		gearbox.reverseRatios.push_back(-2.90F);
	}

	bool
	VehicleSettings::isValid () const noexcept
	{
		if ( wheels.empty() || gearbox.forwardRatios.empty() || gearbox.reverseRatios.empty() )
		{
			return false;
		}

		for ( const auto & wheel : wheels )
		{
			if ( !isFiniteVector(wheel.attachment) || !isUnit(wheel.suspensionDirection) || !isUnit(wheel.forward) ||
				std::abs(Vector< 3, float >::dotProduct(wheel.suspensionDirection, wheel.forward)) > VehicleUnitTolerance ||
				!isPositive(wheel.radius) || !isNonNegative(wheel.width) || !isNonNegative(wheel.suspensionMinLength) ||
				!std::isfinite(wheel.suspensionMaxLength) || wheel.suspensionMaxLength < wheel.suspensionMinLength ||
				!isPositive(wheel.suspensionFrequency) || !isNonNegative(wheel.suspensionDamping) ||
				!isNonNegative(wheel.maxSteerAngle) || !isNonNegative(wheel.maxBrakeTorque) || !isNonNegative(wheel.maxHandBrakeTorque) ||
				!isPositive(wheel.inertia) || !isNonNegative(wheel.angularDamping) ||
				wheel.longitudinalFriction.empty() || wheel.lateralFriction.empty() )
			{
				return false;
			}
		}

		if ( engine.normalizedTorque.empty() || !isNonNegative(engine.maxTorque) || !isPositive(engine.minRPM) ||
			!std::isfinite(engine.maxRPM) || !(engine.maxRPM > engine.minRPM) || !isPositive(engine.inertia) || !isNonNegative(engine.angularDamping) )
		{
			return false;
		}

		const auto validRatio = [] (float ratio) {
			return std::isfinite(ratio) && ratio != 0.0F;
		};

		if ( !std::ranges::all_of(gearbox.forwardRatios, validRatio) || !std::ranges::all_of(gearbox.reverseRatios, validRatio) ||
			!isNonNegative(gearbox.switchTime) || !isNonNegative(gearbox.clutchReleaseTime) || !isNonNegative(gearbox.switchLatency) ||
			!isPositive(gearbox.shiftDownRPM) || !std::isfinite(gearbox.shiftUpRPM) || !(gearbox.shiftUpRPM > gearbox.shiftDownRPM) )
		{
			return false;
		}

		return std::ranges::all_of(differentials, [this] (const DifferentialSettings & differential) {
			return differential.leftWheel < wheels.size() && differential.rightWheel < wheels.size() &&
				std::isfinite(differential.ratio) && differential.ratio != 0.0F &&
				std::isfinite(differential.leftRightSplit) && differential.leftRightSplit >= 0.0F && differential.leftRightSplit <= 1.0F &&
				std::isfinite(differential.limitedSlipRatio) && differential.limitedSlipRatio >= 1.0F &&
				isNonNegative(differential.engineTorqueRatio);
		});
	}

	bool
	VehicleController::setup (const VehicleSettings & settings) noexcept
	{
		if ( !settings.isValid() )
		{
			return false;
		}

		m_settings = settings;
		m_wheels.clear();

		for ( const auto & wheel : m_settings.wheels )
		{
			WheelState state;

			state.suspensionLength = wheel.suspensionMaxLength;
			m_wheels.push_back(state);
		}

		m_engineRPM = m_settings.engine.minRPM;
		m_gear = 0;
		m_clutch = 1.0F;
		m_switchTimeLeft = 0.0F;
		m_clutchReleaseLeft = 0.0F;
		m_sinceShift = m_settings.gearbox.switchLatency;

		return true;
	}

	bool
	VehicleController::setInput (float forward, float right, float brake, float handBrake) noexcept
	{
		if ( !std::isfinite(forward) || !std::isfinite(right) || !std::isfinite(brake) || !std::isfinite(handBrake) )
		{
			return false;
		}

		m_forward = std::clamp(forward, -1.0F, 1.0F);
		m_right = std::clamp(right, -1.0F, 1.0F);
		m_brake = std::clamp(brake, 0.0F, 1.0F);
		m_handBrake = std::clamp(handBrake, 0.0F, 1.0F);

		return true;
	}

	float
	VehicleController::gearRatio () const noexcept
	{
		const auto & gearbox = m_settings.gearbox;

		if ( m_gear > 0 && static_cast< size_t >(m_gear) <= gearbox.forwardRatios.size() )
		{
			return gearbox.forwardRatios[static_cast< size_t >(m_gear - 1)];
		}

		if ( m_gear < 0 && static_cast< size_t >(-m_gear) <= gearbox.reverseRatios.size() )
		{
			return gearbox.reverseRatios[static_cast< size_t >(-m_gear - 1)];
		}

		return 0.0F;
	}

	void
	VehicleController::shiftTo (int32_t gear) noexcept
	{
		if ( gear == m_gear )
		{
			return;
		}

		m_gear = gear;
		m_switchTimeLeft = m_settings.gearbox.switchTime;
		m_clutchReleaseLeft = m_settings.gearbox.clutchReleaseTime;
		m_sinceShift = 0.0F;
	}

	float
	VehicleController::targetSteerAngle (size_t wheelIndex) const noexcept
	{
		return -m_right * m_settings.wheels[wheelIndex].maxSteerAngle;
	}

	bool
	VehicleController::wantsToMove () const noexcept
	{
		if ( !this->isReady() )
		{
			return false;
		}

		if ( m_forward != 0.0F )
		{
			return true;
		}

		for ( size_t index = 0; index < m_wheels.size(); ++index )
		{
			if ( m_wheels[index].steerAngle != this->targetSteerAngle(index) )
			{
				return true;
			}
		}

		return false;
	}

	void
	VehicleController::prepareStep (float forwardSpeed, float deltaTime) noexcept
	{
		if ( !this->isReady() || !(deltaTime > 0.0F) || !std::isfinite(forwardSpeed) )
		{
			return;
		}

		const auto & settings = m_settings;
		const auto & engine = settings.engine;
		const auto & gearbox = settings.gearbox;

		/* The steering. */
		for ( size_t index = 0; index < m_wheels.size(); ++index )
		{
			m_wheels[index].steerAngle = this->targetSteerAngle(index);
		}

		/* The driven wheels' spin seen from the gearbox (through the differentials, weighted by their engine share). */
		float drivenSpin = 0.0F;
		float drivenWeight = 0.0F;

		for ( const auto & differential : settings.differentials )
		{
			const auto spin = (m_wheels[differential.leftWheel].angularVelocity + m_wheels[differential.rightWheel].angularVelocity) * 0.5F;

			drivenSpin += spin * differential.ratio * differential.engineTorqueRatio;
			drivenWeight += differential.engineTorqueRatio;
		}

		if ( drivenWeight > 0.0F )
		{
			drivenSpin /= drivenWeight;
		}

		/* The automatic gearbox: a forward or a reverse gear for the throttle's sign (a reverse only nearly stopped),
		 * then up / down by the RPM the GROUND speed gives through the gear (the output shaft's: a spinning wheel must not
		 * shift up — it did, then the open clutch let it slow, and the gearbox cycled 1-2-1), only with the clutch closed,
		 * never twice within the latency. */
		m_sinceShift += deltaTime;

		float drivenRadius = 0.0F;
		float drivenRatio = 0.0F;

		for ( const auto & differential : settings.differentials )
		{
			drivenRadius += (settings.wheels[differential.leftWheel].radius + settings.wheels[differential.rightWheel].radius) * 0.5F * differential.engineTorqueRatio;
			drivenRatio += differential.ratio * differential.engineTorqueRatio;
		}

		if ( drivenWeight > 0.0F )
		{
			drivenRadius /= drivenWeight;
			drivenRatio /= drivenWeight;
		}

		if ( m_switchTimeLeft <= 0.0F && m_clutchReleaseLeft <= 0.0F && m_sinceShift >= gearbox.switchLatency )
		{
			if ( m_forward > 0.0F && m_gear <= 0 && forwardSpeed > -VehicleStoppedSpeed )
			{
				this->shiftTo(1);
			}
			else if ( m_forward < 0.0F && m_gear >= 0 && forwardSpeed < VehicleStoppedSpeed )
			{
				this->shiftTo(-1);
			}
			else if ( m_gear > 0 && drivenRadius > 0.0F )
			{
				const auto rpm = std::abs(forwardSpeed / drivenRadius * drivenRatio * this->gearRatio()) * RadiansPerSecondToRPM;

				if ( rpm > gearbox.shiftUpRPM && static_cast< size_t >(m_gear) < gearbox.forwardRatios.size() )
				{
					this->shiftTo(m_gear + 1);
				}
				else if ( rpm < gearbox.shiftDownRPM && m_gear > 1 )
				{
					this->shiftTo(m_gear - 1);
				}
			}
		}

		/* The clutch: open while shifting, then closing over the release time. */
		if ( m_switchTimeLeft > 0.0F )
		{
			m_switchTimeLeft -= deltaTime;
			m_clutch = 0.0F;
		}
		else if ( m_clutchReleaseLeft > 0.0F )
		{
			m_clutchReleaseLeft -= deltaTime;
			m_clutch = gearbox.clutchReleaseTime > 0.0F ? std::clamp(1.0F - (m_clutchReleaseLeft / gearbox.clutchReleaseTime), 0.0F, 1.0F) : 1.0F;
		}
		else
		{
			m_clutch = 1.0F;
		}

		/* The engine: the throttle in the gear's direction (backwards in reverse). */
		const auto ratio = this->gearRatio();
		const float throttle = (m_gear > 0 && m_forward > 0.0F) || (m_gear < 0 && m_forward < 0.0F) ? std::abs(m_forward) : 0.0F;
		const auto rpmRange = engine.maxRPM - engine.minRPM;
		const auto torqueAt = [&engine, rpmRange] (float rpm) {
			return engine.maxTorque * engine.normalizedTorque.value((rpm - engine.minRPM) / rpmRange);
		};

		/* Free: it revs by its inertia. Engaged: it follows the wheels (never under its idle RPM). */
		const auto freeSpin = m_engineRPM / RadiansPerSecondToRPM;
		const auto freeTorque = (throttle * torqueAt(m_engineRPM)) - (engine.angularDamping * engine.inertia * freeSpin);
		const auto freeRPM = std::clamp((freeSpin + (freeTorque / engine.inertia * deltaTime)) * RadiansPerSecondToRPM, engine.minRPM, engine.maxRPM);
		const auto wheelRPM = std::abs(drivenSpin * ratio) * RadiansPerSecondToRPM;
		const auto coupledRPM = std::clamp(wheelRPM, engine.minRPM, engine.maxRPM);

		m_engineRPM = ratio != 0.0F ? freeRPM + ((coupledRPM - freeRPM) * m_clutch) : freeRPM;

		/* The rev limiter: no torque at the top. */
		const auto engineTorque = m_engineRPM >= engine.maxRPM ? 0.0F : throttle * torqueAt(m_engineRPM);
		const auto gearboxTorque = engineTorque * ratio * m_clutch;

		/* The wheels: no drive torque but through a differential; the brakes. */
		for ( auto & wheel : m_wheels )
		{
			wheel.driveTorque = 0.0F;
		}

		for ( const auto & differential : settings.differentials )
		{
			const auto torque = gearboxTorque * differential.ratio * differential.engineTorqueRatio;
			const auto spinLeft = std::abs(m_wheels[differential.leftWheel].angularVelocity);
			const auto spinRight = std::abs(m_wheels[differential.rightWheel].angularVelocity);
			auto leftShare = differential.leftRightSplit;

			/* The limited slip: the faster wheel beyond the ratio gives its torque to the slower one. */
			if ( spinLeft > differential.limitedSlipRatio * spinRight && spinLeft > 0.0F )
			{
				leftShare *= differential.limitedSlipRatio * spinRight / spinLeft;
			}
			else if ( spinRight > differential.limitedSlipRatio * spinLeft && spinRight > 0.0F )
			{
				leftShare = 1.0F - ((1.0F - leftShare) * differential.limitedSlipRatio * spinLeft / spinRight);
			}

			m_wheels[differential.leftWheel].driveTorque += torque * leftShare;
			m_wheels[differential.rightWheel].driveTorque += torque * (1.0F - leftShare);
		}

		for ( size_t index = 0; index < m_wheels.size(); ++index )
		{
			const auto & wheelSettings = settings.wheels[index];

			m_wheels[index].brakeTorque = (m_brake * wheelSettings.maxBrakeTorque) + (m_handBrake * wheelSettings.maxHandBrakeTorque);
		}
	}

	void
	VehicleController::finishStep (float deltaTime) noexcept
	{
		if ( !(deltaTime > 0.0F) )
		{
			return;
		}

		constexpr auto FullTurn = 2.0F * std::numbers::pi_v< float >;

		for ( auto & wheel : m_wheels )
		{
			wheel.rotationAngle = std::fmod(wheel.rotationAngle + (wheel.angularVelocity * deltaTime), FullTurn);
		}
	}
}
