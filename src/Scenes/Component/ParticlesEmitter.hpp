/*
 * src/Scenes/Component/ParticlesEmitter.hpp
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
 * https://github.com/londnoir/emeraude-engine
 *
 * --- THIS IS AUTOMATICALLY GENERATED, DO NOT CHANGE ---
 */

#pragma once

/* STL inclusions. */
#include <memory>
#include <string>
#include <vector>
#include <functional>

/* Local inclusions for inheritances. */
#include "Abstract.hpp"
#include "Libs/ObserverTrait.hpp"

/* Local inclusions for usages. */
#include "Graphics/RenderableInstance/Multiple.hpp"
#include "Graphics/Renderer.hpp"
#include "Libs/Time/TimedEvent.hpp"
#include "Physics/Particle.hpp"

namespace EmEn::Scenes::Component
{
	/**
	 * @brief A class that build a particle generator with a mesh or sprite instance.
	 * @note [OBS][SHARED-OBSERVER]
	 * @extends EmEn::Scenes::Component::Abstract The base class for each entity component.
	 * @extends EmEn::Libs::ObserverTrait This class must dispatch modifications from a renderable instance to the entity.
	 */
	class ParticlesEmitter final : public Abstract, public Libs::ObserverTrait
	{
		public:

			/** @brief Animatable Interface key. */
			enum AnimationID : uint8_t
			{
				EmittingState,
				ParticlesGeneratePerCycle,
				ParticlesGenerateRate,
				ParticleLifetime,
				ParticleSize,
				ParticleSizeDeltaPerCycle,
				SpreadingRadius,
				ChaosMagnitude
			};

			/** @brief Class identifier. */
			static constexpr auto ClassId{"ParticlesEmitter"};

			/**
			 * @brief Construct a ParticlesEmitter resource.
			 * @param componentName A reference to a string.
			 * @param parentEntity A reference to the parent entity.
			 * @param renderable The rendered object by this particle emitter.
			 * @param instanceCount A fixed instance count.
			 */
			ParticlesEmitter (const std::string & componentName, const AbstractEntity & parentEntity, const std::shared_ptr< Graphics::Renderable::Abstract > & renderable, uint32_t instanceCount) noexcept
				: Abstract{componentName, parentEntity},
				m_renderableInterface{renderable},
				m_renderableInstance{std::make_shared< Graphics::RenderableInstance::Multiple >(this->engineContext().graphicsRenderer.device(), renderable, instanceCount, Graphics::RenderableInstance::None)},
				m_particleLimit{instanceCount}
			{
				/* NOTE: Prepare local data at the fixed size. */
				m_particles.resize(m_particleLimit);

				this->observe(renderable.get());
			}

			/** @copydoc EmEn::Scenes::Component::Abstract::getRenderableInstance() const */
			[[nodiscard]]
			std::shared_ptr< Graphics::RenderableInstance::Abstract >
			getRenderableInstance () const noexcept override
			{
				return m_renderableInstance;
			}

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

			/** @copydoc EmEn::Scenes::Component::Abstract::move() */
			void
			move (const Libs::Math::CartesianFrame< float > & /*worldCoordinates*/) noexcept override
			{
				// FIXME: Check the particle coordinates
			}

			/** @copydoc EmEn::Scenes::Component::Abstract::processLogics() */
			void processLogics (const Scene & scene) noexcept override;

			/** @copydoc EmEn::Scenes::Component::Abstract::shouldBeRemoved() */
			[[nodiscard]]
			bool
			shouldBeRemoved () const noexcept override
			{
				return m_renderableInstance->isBroken();
			}

			/**
			 * @brief Returns the particle limit.
			 * @return size_t
			 */
			[[nodiscard]]
			size_t
			particleLimit () const noexcept
			{
				return m_particleLimit;
			}

			/**
			 * @brief Sets the physical properties basis for each particle.
			 * @note This is different from the physical properties of the component.
			 * @param mass The mass in kilograms.
			 * @param surface The average surface in square meters.
			 * @param dragCoefficient The drag coefficient.
			 * @param angularDrag The angular drag coefficient.
			 * @param bounciness A bounciness scalar of the object when hitting something. Default 50%.
			 * @param stickiness A stickiness scalar of the object when hitting something. Default 50%.
			 * @return void
			 */
			void
			setParticlePhysicalProperties (float mass, float surface, float dragCoefficient, float angularDrag, float bounciness = 0.5F, float stickiness = 0.5F) noexcept
			{
				m_particlePhysicalProperties.setProperties(mass, surface, dragCoefficient, angularDrag, bounciness, stickiness, {});
			}

			/**
			 * @brief Returns the particle physical properties.
			 * @return const Physics::PhysicalObjectProperties &
			 */
			[[nodiscard]]
			const Physics::BodyPhysicalProperties &
			particlePhysicalProperties () const noexcept
			{
				return m_particlePhysicalProperties;
			}

			/**
			 * @brief Sets how many particles will be generated par logics cycle.
			 * @param count The particles count if the limit is not reached.
			 * @return void
			 */
			void
			setParticleGeneratedPerCycle (uint32_t count) noexcept
			{
				m_particleGeneratedPerCycle = std::min< uint32_t >(count, m_particleLimit);
			}

			/**
			 * @brief Sets the particle rate generation per engine logics update.
			 * @note This is the ratio of the max particles. 1 means the whole limit at once.
			 * @param rate A value between 0 and 1.
			 * @return void
			 */
			void
			setParticleGenerationRate (float rate) noexcept
			{
				m_particleGeneratedPerCycle = static_cast< uint32_t >(std::round(static_cast< float >(m_particleLimit) * std::abs(rate)));
			}

			/**
			 * @brief Returns the particle rate generation per engine logics update.
			 * @return uint32_t
			 */
			[[nodiscard]]
			uint32_t
			particleGeneratedPerCycle () const noexcept
			{
				return m_particleGeneratedPerCycle;
			}

			/**
			 * @brief Sets the initial particle lifetime in engine cycles.
			 * @param lifetime The initial lifetime of a new particle.
			 * @return void
			 */
			void setParticleLifetime (uint32_t lifetime) noexcept;

			/**
			 * @brief Sets the initial particle randomized lifetime in engine cycles.
			 * @param minimumLifetime The initial minimum lifetime of a new particle.
			 * @param maximumLifetime The initial maximum lifetime of a new particle.
			 * @return void
			 */
			void setParticleLifetime (uint32_t minimumLifetime, uint32_t maximumLifetime) noexcept;

			/**
			 * @brief Returns the initial particle minimum lifetime in engine cycles.
			 * @return uint32_t
			 */
			[[nodiscard]]
			uint32_t
			minimumParticleLifetime () const noexcept
			{
				return m_minimumParticleLifetime;
			}

			/**
			 * @brief Returns the initial particle maximum lifetime in engine cycles.
			 * @return uint32_t
			 */
			[[nodiscard]]
			uint32_t
			maximumParticleLifetime () const noexcept
			{
				return m_maximumParticleLifetime;
			}

			/**
			 * @brief Sets the initial particle size.
			 * @param size The initial size of a new particle.
			 * @return void
			 */
			void
			setParticleSize (float size) noexcept
			{
				m_minimumParticleSize = size;
				m_maximumParticleSize = size;
			}

			/**
			 * @brief Sets the initial particle randomized lifetime in engine cycles.
			 * @param minimumSize The initial minimum size of a new particle.
			 * @param maximumSize The initial maximum size of a new particle.
			 * @return void
			 */
			void
			setParticleSize (float minimumSize, float maximumSize) noexcept
			{
				if ( minimumSize < maximumSize )
				{
					m_minimumParticleSize = minimumSize;
					m_maximumParticleSize = maximumSize;
				}
				else
				{
					m_minimumParticleSize = maximumSize;
					m_maximumParticleSize = minimumSize;
				}
			}

			/**
			 * @brief Returns the initial particle minimum size.
			 * @return float
			 */
			[[nodiscard]]
			float
			minimumParticlesSize () const noexcept
			{
				return m_minimumParticleSize;
			}

			/**
			 * @brief Returns the initial particle maximum size.
			 * @return float
			 */
			[[nodiscard]]
			float
			maximumParticlesSize () const noexcept
			{
				return m_maximumParticleSize;
			}

			/**
			 * @brief Updates the particle size per cycle update.
			 * @param delta A positive or negative value to vary the size.
			 * @return void
			 */
			void
			setParticleSizeDeltaPerCycle (float delta) noexcept
			{
				m_particleSizeDeltaPerCycle = delta;
			}

			/**
			 * @brief Returns the particle size processLogics factor.
			 * @return float
			 */
			[[nodiscard]]
			float
			particleSizeUpdateFactor () const noexcept
			{
				return m_particleSizeDeltaPerCycle;
			}

			/**
			 * @brief Sets a spreading radius for particle generation.
			 * @param radius The size of the radius.
			 * @return void
			 */
			void
			setSpreadingRadius (float radius) noexcept
			{
				m_spreadingRadius = std::abs(radius);
			}

			/**
			 * @brief Returns the spreading radius of the particle generation.
			 * @return float
			 */
			[[nodiscard]]
			float
			spreadingRadius () const noexcept
			{
				return m_spreadingRadius;
			}

			/**
			 * @brief Sets a chaos parameter.
			 * @param magnitude The chaos magnitude.
			 * @return void
			 */
			void
			setChaos (float magnitude) noexcept
			{
				m_chaosMagnitude = magnitude;
			}

			/**
			 * @brief Returns the chaos magnitude.
			 * @return float
			 */
			[[nodiscard]]
			float
			chaos () const noexcept
			{
				return m_chaosMagnitude;
			}

			/**
			 * @brief Enables particles to move with the emitter.
			 * @return void
			 */
			void
			constraintParticlesToEmitterSpace () noexcept
			{
				this->enableFlag(ConstrainedToEmitterSpace);
			}

			/**
			 * @brief Overrides the default physics behavior for particles.
			 * @param function A reference to a function.
			 * @return void
			 */
			void
			setCustomPhysicsSimulationFunction (const std::function< bool (Physics::Particle &) > & function) noexcept
			{
				m_customPhysicsSimulationSimpleFunction = function;

				m_physicsSimulationFunction = PhysicsSimulationFunction::Simple;
			}

			/**
			 * @brief Overrides the default physics behavior for particles.
			 * @param function A reference to a function.
			 * @return void
			 */
			void
			setCustomPhysicsSimulationFunction (const std::function< bool (const Physics::EnvironmentPhysicalProperties &, const Physics::BodyPhysicalProperties &, const Libs::Math::CartesianFrame< float > &, Physics::Particle &) > & function) noexcept
			{
				m_customPhysicsSimulationFullFunction = function;

				m_physicsSimulationFunction = PhysicsSimulationFunction::Full;
			}

			/**
			 * @brief Starts the particle emission.
			 * @param duration A timeout in milliseconds to stop the emission automatically. Default none.
			 */
			void start (uint32_t duration = 0) noexcept;

			/** @brief Stops the particle emission. */
			void
			stop () noexcept
			{
				this->disableFlag(IsEmitting);
			}

			/**
			 * @brief Returns the state of particle emission.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isEmitting () const noexcept
			{
				return this->isFlagEnabled(IsEmitting);
			}

		private:

			/** @copydoc EmEn::Scenes::Component::Abstract::onSuspend() */
			void onSuspend () noexcept override { }

			/** @copydoc EmEn::Scenes::Component::Abstract::onWakeup() */
			void onWakeup () noexcept override { }

			/** @copydoc EmEn::Animations::AnimatableInterface::playAnimation() */
			bool playAnimation (uint8_t animationID, const Libs::Variant & value, size_t cycle) noexcept override;

			/** @copydoc EmEn::Libs::ObserverTrait::onNotification() */
			[[nodiscard]]
			bool onNotification (const ObservableTrait * observable, int notificationCode, const std::any & data) noexcept override;

			/**
			 * @brief Returns the lifetime for a new particle.
			 * @return uint32_t
			 */
			[[nodiscard]]
			uint32_t
			getParticleLifetime () const noexcept
			{
				if ( m_minimumParticleLifetime >= m_maximumParticleLifetime )
				{
					return m_minimumParticleLifetime;
				}

				return Libs::Utility::quickRandom(m_minimumParticleLifetime, m_maximumParticleLifetime);
			}

			/**
			 * @brief Returns the size for a new particle.
			 * @return float
			 */
			[[nodiscard]]
			float
			getParticleSize () const noexcept
			{
				if ( m_minimumParticleSize >= m_maximumParticleSize )
				{
					return m_minimumParticleSize;
				}

				return Libs::Utility::quickRandom(m_minimumParticleSize, m_maximumParticleSize);
			}

			/**
			 * @brief STL streams printable object.
			 * @param out A reference to the stream output.
			 * @param obj A reference to the object to print.
			 * @return std::ostream &
			 */
			friend std::ostream & operator<< (std::ostream & out, const ParticlesEmitter & obj);

			/* Flag names */
			static constexpr auto IsEmitting{UnusedFlag + 0UL};
			static constexpr auto GravityEnabled{UnusedFlag + 1UL};
			static constexpr auto SphereCollisionEnabled{UnusedFlag + 2UL};
			static constexpr auto IsSimulationPaused{UnusedFlag + 3UL};
			static constexpr auto IsCollisionDisabled{UnusedFlag + 4UL};
			static constexpr auto ConstrainedToEmitterSpace{UnusedFlag + 5UL};

			enum class PhysicsSimulationFunction : uint8_t
			{
				Default,
				Simple,
				Full
			};

			std::weak_ptr< Graphics::Renderable::Abstract > m_renderableInterface;
			std::shared_ptr< Graphics::RenderableInstance::Multiple > m_renderableInstance;
			Physics::BodyPhysicalProperties m_particlePhysicalProperties;
			std::vector< Physics::Particle > m_particles;
			std::unique_ptr< Libs::Time::TimedEvent< uint64_t, std::micro > > m_timedEvent;
			std::function< bool (Physics::Particle &) > m_customPhysicsSimulationSimpleFunction;
			std::function< bool (const Physics::EnvironmentPhysicalProperties &, const Physics::BodyPhysicalProperties &, const Libs::Math::CartesianFrame< float > &, Physics::Particle & particle) > m_customPhysicsSimulationFullFunction;
			uint32_t m_particleLimit;
			uint32_t m_particleGeneratedPerCycle{1};
			uint32_t m_minimumParticleLifetime{1};
			uint32_t m_maximumParticleLifetime{1};
			float m_minimumParticleSize{1.0F};
			float m_maximumParticleSize{1.0F};
			float m_particleSizeDeltaPerCycle{0.0F};
			float m_spreadingRadius{1.0F};
			float m_chaosMagnitude{0.0F};
			PhysicsSimulationFunction m_physicsSimulationFunction{PhysicsSimulationFunction::Default};
	};

	inline
	std::ostream &
	operator<< (std::ostream & out, const ParticlesEmitter & obj)
	{
		return out <<
			"Particles Generator data :\n"
			"Enabled : " << ( obj.isEmitting() ? "yes" : "no" ) << "\n"
			"Particles generated per cycle : " << obj.m_particleGeneratedPerCycle << "\n"
			"Particles limit : " << obj.m_particleLimit << "\n"
			"Particles min life : " << obj.m_minimumParticleLifetime << "\n"
			"Particles max life : " << obj.m_maximumParticleLifetime << "\n"
			"Particles min size : " << obj.m_minimumParticleSize << "\n"
			"Particles max size : " << obj.m_maximumParticleSize << "\n"
			"Particles size processLogics factor : " << obj.m_particleSizeDeltaPerCycle << "\n"
			"Spreading : " << obj.m_spreadingRadius << "\n"
			"Chaos magnitude : " << obj.m_chaosMagnitude << "\n"
			"Living particles : " << obj.m_particles.size() << '\n';
	}

	/**
	 * @brief Stringifies the object.
	 * @param obj A reference to the object to print.
	 * @return std::string
	 */
	inline
	std::string
	to_string (const ParticlesEmitter & obj) noexcept
	{
		std::stringstream output;

		output << obj;

		return output.str();
	}
}
