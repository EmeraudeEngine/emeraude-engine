/*
 * src/Physics/Vehicle.hpp
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
#include <cstdint>

/* Local inclusions for usages. */
#include "Math/Base.hpp"
#include "Math/PiecewiseLinear.hpp"
#include "Math/Vector.hpp"
#include "StaticVector.hpp"

/*
 * A wheeled vehicle (physics overhaul bonus phase, owner decisions 15): its settings and its state between two physics
 * steps — the wheels, the engine, the gearbox, the differentials, the driver's inputs. The scene casts the wheels and the
 * solver solves them (SoftStepSolver::Wheel); this class runs the drive train around them. Model and default values:
 * Jolt's VehicleConstraint / WheeledVehicleController (J. Rouwé, MIT) — no code taken. Design:
 * docs/subsystems/physics/19-wheeled-vehicle.md.
 */

namespace EmEn::Physics
{
	/** @brief A curve of the vehicle: a torque over the RPM, a friction over a slip. */
	using VehicleCurve = Base::Math::PiecewiseLinear< float, 16 >;

	/**
	 * @brief One wheel, in the chassis' local space (the node's frame).
	 */
	struct WheelSettings final
	{
		/** The top of the suspension travel. */
		Base::Math::Vector< 3, float > attachment;
		/** Down along the suspension (unit). */
		Base::Math::Vector< 3, float > suspensionDirection{0.0F, -1.0F, 0.0F};
		/** The rolling direction when not steered (unit, perpendicular to the suspension). */
		Base::Math::Vector< 3, float > forward{0.0F, 0.0F, -1.0F};
		/** Friction coefficient over the slip ratio (Jolt's default: 1.2 at 0.06, 1.0 beyond 0.2). */
		VehicleCurve longitudinalFriction;
		/** Friction coefficient over the slip angle in degrees (Jolt's default: 1.2 at 3°, 1.0 beyond 20°). */
		VehicleCurve lateralFriction;
		float radius{0.3F};
		float width{0.1F};
		float suspensionMinLength{0.3F};
		float suspensionMaxLength{0.5F};
		/** The suspension's natural frequency (Hz) and damping ratio. */
		float suspensionFrequency{1.5F};
		float suspensionDamping{0.5F};
		/** The largest steering angle (radians, 0 for a wheel that does not steer). */
		float maxSteerAngle{0.0F};
		float maxBrakeTorque{1500.0F};
		float maxHandBrakeTorque{0.0F};
		/** The wheel's inertia about its axle (kg·m²) and its angular damping (1 / s). */
		float inertia{0.9F};
		float angularDamping{0.2F};
	};

	/** @brief The engine. */
	struct EngineSettings final
	{
		/** The torque over the RPM, normalized: x in [0, 1] from minRPM to maxRPM, y a fraction of maxTorque. */
		VehicleCurve normalizedTorque;
		float maxTorque{500.0F};
		float minRPM{1000.0F};
		float maxRPM{6000.0F};
		/** Its inertia (kg·m²) and its angular damping (1 / s), when the clutch frees it. */
		float inertia{0.5F};
		float angularDamping{0.2F};
	};

	/** @brief The automatic gearbox. */
	struct GearboxSettings final
	{
		Base::StaticVector< float, 8 > forwardRatios;
		Base::StaticVector< float, 2 > reverseRatios;
		/** How long a shift keeps the clutch open (s), then how long it takes to close it (s). */
		float switchTime{0.5F};
		float clutchReleaseTime{0.3F};
		/** The least time between two shifts (s). */
		float switchLatency{0.5F};
		float shiftUpRPM{4000.0F};
		float shiftDownRPM{2000.0F};
	};

	/** @brief A differential between two wheels. */
	struct DifferentialSettings final
	{
		uint32_t leftWheel{0};
		uint32_t rightWheel{1};
		/** The final drive ratio. */
		float ratio{3.42F};
		/** The share of its torque to the left wheel. */
		float leftRightSplit{0.5F};
		/** The faster wheel above this ratio of the slower one gives its torque to it (≥ 1). */
		float limitedSlipRatio{1.4F};
		/** Its share of the engine's torque. */
		float engineTorqueRatio{1.0F};
	};

	/**
	 * @brief A whole vehicle.
	 */
	struct EMEN_API VehicleSettings final
	{
		Base::StaticVector< WheelSettings, 8 > wheels;
		Base::StaticVector< DifferentialSettings, 4 > differentials;
		EngineSettings engine;
		GearboxSettings gearbox;
		/**
		 * @brief The steepest contact a wheel accepts, radians in [0, π/2]: a contact whose normal deviates more than this
		 * from the wheel's suspension axis (its "up") is ignored. Jolt's VehicleCollisionTester::mMaxSlopeAngle, 80° by
		 * default. An overturned or capsized car's wheels then touch nothing (2026-10-03: on its roof, a car reported its
		 * four wheels in contact — their casts start inside the ground, which they took for a road).
		 */
		float maxSlopeAngle{Base::Math::Radian(80.0F)};

		/**
		 * @brief Fills the curves and the gear ratios with Jolt's defaults (a road car).
		 */
		void useDefaultCurvesAndRatios () noexcept;

		/**
		 * @brief Checks the settings: at least one wheel, every value finite, radius, travel, frequencies and inertias
		 * positive, unit directions, a ratio for every gear, the differentials' wheels existing, the slope angle in
		 * [0, π/2].
		 * @return bool
		 */
		[[nodiscard]]
		bool isValid () const noexcept;
	};

	/**
	 * @brief A vehicle's state between two physics steps, and its drive train.
	 */
	class EMEN_API VehicleController final
	{
		public:

			/** @brief The state of one wheel. */
			struct WheelState final
			{
				/** Where it touches (world), its normal (towards the chassis), written by the wheel cast. */
				Base::Math::Vector< 3, float > contactPoint;
				Base::Math::Vector< 3, float > contactNormal{0.0F, 1.0F, 0.0F};
				/** Its spin about its axle (rad / s, positive rolling forward), and its rotation angle (rad). */
				float angularVelocity{0.0F};
				float rotationAngle{0.0F};
				/** Its steering angle (rad), right-handed about the suspension's up: POSITIVE turns it LEFT (a right input gives a negative angle). */
				float steerAngle{0.0F};
				/** The suspension's length (the maximum when in the air). */
				float suspensionLength{0.0F};
				/** This step's drive and brake torques (N·m). */
				float driveTorque{0.0F};
				float brakeTorque{0.0F};
				/** The last step's slips and impulses (what the solver gave). */
				float slipRatio{0.0F};
				float slipAngle{0.0F};
				float suspensionImpulse{0.0F};
				float longitudinalImpulse{0.0F};
				float lateralImpulse{0.0F};
				bool contact{false};
			};

			/**
			 * @brief Constructs a vehicle with no wheel (setup() makes it one).
			 */
			VehicleController () noexcept = default;

			/**
			 * @brief Takes settings.
			 * @param settings A reference to the settings.
			 * @return bool False (nothing changed) for invalid settings (VehicleSettings::isValid()).
			 */
			[[nodiscard]]
			bool setup (const VehicleSettings & settings) noexcept;

			/**
			 * @brief Sets the driver's inputs.
			 * @param forward The throttle, −1 to 1 (negative: reverse); clamped.
			 * @param right The steering, −1 (left) to 1 (right); clamped.
			 * @param brake 0 to 1; clamped.
			 * @param handBrake 0 to 1; clamped.
			 * @return bool False (nothing changed) for a non-finite value.
			 */
			[[nodiscard]]
			bool setInput (float forward, float right, float brake, float handBrake) noexcept;

			/**
			 * @brief The drive train before the solver: the steering, the gear, the engine RPM and torque, the wheels'
			 * drive and brake torques.
			 * @param forwardSpeed The chassis' speed along its forward direction (m/s).
			 * @param deltaTime The step (s), > 0.
			 */
			void prepareStep (float forwardSpeed, float deltaTime) noexcept;

			/**
			 * @brief After the solver: the wheels' rotation angles, the gearbox's timers.
			 * @param deltaTime The step (s), > 0.
			 */
			void finishStep (float deltaTime) noexcept;

			/** @brief Returns whether it has settings (a wheel at least). */
			[[nodiscard]]
			bool
			isReady () const noexcept
			{
				return !m_settings.wheels.empty();
			}

			/** @brief Returns the settings. */
			[[nodiscard]]
			const VehicleSettings &
			settings () const noexcept
			{
				return m_settings;
			}

			/** @brief Returns the wheels' state (as many as wheels). */
			[[nodiscard]]
			Base::StaticVector< WheelState, 8 > &
			wheels () noexcept
			{
				return m_wheels;
			}

			/** @brief Returns the wheels' state. */
			[[nodiscard]]
			const Base::StaticVector< WheelState, 8 > &
			wheels () const noexcept
			{
				return m_wheels;
			}

			/** @brief Returns the engine's RPM. */
			[[nodiscard]]
			float
			engineRPM () const noexcept
			{
				return m_engineRPM;
			}

			/** @brief Returns the gear: 0 neutral, 1… forward, −1… reverse. */
			[[nodiscard]]
			int32_t
			gear () const noexcept
			{
				return m_gear;
			}

			/** @brief Returns how much the clutch transmits (0 to 1). */
			[[nodiscard]]
			float
			clutch () const noexcept
			{
				return m_clutch;
			}

			/** @brief Returns the throttle (−1 to 1). */
			[[nodiscard]]
			float
			forwardInput () const noexcept
			{
				return m_forward;
			}

			/**
			 * @brief Returns whether the driver's inputs need the chassis awake: a throttle, or a steering the wheels
			 * have not reached yet (a parked car steered shows its wheels turned).
			 * @return bool
			 */
			[[nodiscard]]
			bool wantsToMove () const noexcept;

		private:

			/** @brief The ratio of the current gear (0 in neutral). */
			[[nodiscard]]
			float gearRatio () const noexcept;

			/**
			 * @brief Returns the steering angle the input asks of a wheel: right-handed about the suspension's up, so a
			 * RIGHT input (positive) gives a NEGATIVE angle (the forward −Z turns towards +X, the right).
			 * @param wheelIndex The wheel's index (< the wheels' count).
			 * @return float
			 */
			[[nodiscard]]
			float targetSteerAngle (size_t wheelIndex) const noexcept;

			/** @brief Starts a shift to a gear. */
			void shiftTo (int32_t gear) noexcept;

			VehicleSettings m_settings;
			Base::StaticVector< WheelState, 8 > m_wheels;
			float m_forward{0.0F};
			float m_right{0.0F};
			float m_brake{0.0F};
			float m_handBrake{0.0F};
			float m_engineRPM{0.0F};
			float m_clutch{1.0F};
			/** The time left with the clutch open, then closing (s), and the time since the last shift (s). */
			float m_switchTimeLeft{0.0F};
			float m_clutchReleaseLeft{0.0F};
			float m_sinceShift{0.0F};
			int32_t m_gear{0};
	};
}
