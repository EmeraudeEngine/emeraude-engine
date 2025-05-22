/*
 * src/Scenes/Component/ParticlesEmitter.cpp
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

#include "ParticlesEmitter.hpp"

/* Local inclusions. */
#include "Scenes/Scene.hpp"

namespace EmEn::Scenes::Component
{
	using namespace EmEn::Libs;
	using namespace EmEn::Libs::Math;
	using namespace Animations;
	using namespace Physics;
	using namespace Graphics;

	ParticlesEmitter::ParticlesEmitter (const std::string & name, const AbstractEntity & parentEntity, const std::shared_ptr< Renderable::Interface > & renderable, uint32_t instanceCount) noexcept
		: Abstract(name, parentEntity),
		m_renderableInstance(std::make_shared< RenderableInstance::Multiple >(renderable, instanceCount, renderable->isSprite() ? RenderableInstance::FacingCamera : RenderableInstance::None)),
		m_particleLimit(instanceCount)
	{
		/* NOTE: Prepare local data at the fixed size. */
		m_particles.resize(m_particleLimit);

		this->observe(m_renderableInstance.get());
	}

	void
	ParticlesEmitter::processLogics (const Scene & scene) noexcept
	{
		if ( m_renderableInstance->isBroken() )
		{
			return;
		}

		if ( m_renderableInstance->isAnimated() )
		{
			m_renderableInstance->updateFrameIndex(scene.lifetimeMS() - this->parentEntity().birthTime());
		}

		this->updateAnimations(scene.cycle());

		const auto & envProperties = scene.physicalEnvironmentProperties();
		const auto emitterLocation = this->getWorldCoordinates();
		const auto emitterMatrix = emitterLocation.getModelMatrix();

		/* Particle parameters. */
		uint32_t activeInstanceCount = 0;
		uint32_t newParticles = this->isEmitting() ? m_particleGeneratedPerCycle : 0;

		for ( auto & particle : m_particles )
		{
			bool newLocation = false;

			/* If the particle is dead, we plan a possible reset. */
			if ( particle.isDead() )
			{
				/* Replacing by a fresh new one. */
				if ( newParticles > 0 )
				{
					if ( this->isFlagEnabled(ConstrainedToEmitterSpace) )
					{
						particle.initialize(this->getParticleLifetime(), this->getParticleSize(), m_spreadingRadius);
					}
					else
					{
						particle.initialize(this->getParticleLifetime(), this->getParticleSize(), m_spreadingRadius, emitterLocation);
					}

					newParticles--;

					newLocation = true;
				}
				else
				{
					continue;
				}
			}

			/* Update the particle physics. */
			switch ( m_physicsSimulationFunction )
			{
				case PhysicsSimulationFunction::Default :
					if ( this->isFlagEnabled(ConstrainedToEmitterSpace) )
					{
						const auto coordinates = CartesianFrame< float >{emitterMatrix * particle.localCoordinates().getModelMatrix()};

						newLocation = particle.updateSimulation(envProperties, m_particlePhysicalProperties, coordinates, scene.modifiers());
					}
					else
					{
						newLocation = particle.updateSimulation(envProperties, m_particlePhysicalProperties, particle.localCoordinates(), scene.modifiers());
					}
					break;

				case PhysicsSimulationFunction::Simple :
					newLocation = m_customPhysicsSimulationSimpleFunction(particle);
					break;

				case PhysicsSimulationFunction::Full :
					if ( this->isFlagEnabled(ConstrainedToEmitterSpace) )
					{
						const auto coordinates = CartesianFrame< float >{emitterMatrix * particle.localCoordinates().getModelMatrix()};

						newLocation = m_customPhysicsSimulationFullFunction(envProperties, m_particlePhysicalProperties, coordinates, particle);
					}
					else
					{
						newLocation = m_customPhysicsSimulationFullFunction(envProperties, m_particlePhysicalProperties, particle.localCoordinates(), particle);
					}
					break;
			}

			/* Update the buffer if requested. */
			if ( newLocation )
			{
				/* Update the particle. */
				particle.update(particle.linearVelocity(), m_particleSizeDeltaPerCycle, m_chaosMagnitude);

				/* NOTE: This is the updated particle location. */
				const auto particleCoordinates = particle.localCoordinates();
				const auto modelMatrix = this->isFlagEnabled(ConstrainedToEmitterSpace) ?
					emitterMatrix * particleCoordinates.getModelMatrix() :
					particleCoordinates.getModelMatrix();
				const auto scalingVector = emitterLocation.scalingFactor() * particleCoordinates.scalingFactor();

				if ( !m_renderableInstance->updateLocalData(CartesianFrame< float >{modelMatrix, scalingVector}, activeInstanceCount) )
				{
					TraceError{ClassId} << "Failed to update particle local data for instance #" << activeInstanceCount;
				}
			}

			activeInstanceCount++;
		}

		m_renderableInstance->setActiveInstanceCount(activeInstanceCount);
		m_renderableInstance->updateVideoMemory();
	}

	bool
	ParticlesEmitter::playAnimation (uint8_t animationID, const Variant & value, size_t /*cycle*/) noexcept
	{
		switch ( animationID )
		{
			case EmittingState :
				this->setFlag(IsEmitting, value.asBool());
				return true;

			case ParticlesGeneratePerCycle :
				this->setParticleGeneratedPerCycle(value.asUnsignedInteger32());
				return true;

			case ParticlesGenerateRate :
				this->setParticleGenerationRate(value.asFloat());
				return true;

			case ParticleLifetime :
				this->setParticleLifetime(value.asUnsignedInteger32());
				return true;

			case ParticleSize :
				this->setParticleSize(value.asFloat());
				return true;

			case ParticleSizeDeltaPerCycle :
				this->setParticleSizeDeltaPerCycle(value.asFloat());
				return true;

			case SpreadingRadius :
				this->setSpreadingRadius(value.asFloat());
				return true;

			case ChaosMagnitude :
				this->setChaos(value.asFloat());
				return true;

			default:
				return false;
		}
	}

	bool
	ParticlesEmitter::onNotification (const ObservableTrait * observable, int notificationCode, const std::any & /*data*/) noexcept
	{
		if ( observable == m_renderableInstance.get() )
		{
			switch ( notificationCode )
			{
				case RenderableInstance::Abstract::ReadyToSetupOnGPU :
					this->notify(ComponentContentModified);
					break;

				default:
					break;
			}

			return true;
		}

		/* NOTE: Auto-forget */
		return false;
	}

	void
	ParticlesEmitter::setParticleLifetime (uint32_t lifetime) noexcept
	{
		if ( lifetime < 1 )
		{
			Tracer::error(ClassId, "The life parameter must at least be 1 !");

			return;
		}

		m_minimumParticleLifetime = lifetime;
		m_maximumParticleLifetime = lifetime;
	}

	void
	ParticlesEmitter::setParticleLifetime (uint32_t minimumLifetime, uint32_t maximumLifetime) noexcept
	{
		if ( minimumLifetime < 1 || maximumLifetime < 1 )
		{
			Tracer::error(ClassId, "The minimum/maximum lifetime parameter must at least be 1 !");

			return;
		}

		if ( minimumLifetime < maximumLifetime )
		{
			m_minimumParticleLifetime = minimumLifetime;
			m_maximumParticleLifetime = maximumLifetime;
		}
		else
		{
			m_minimumParticleLifetime = maximumLifetime;
			m_maximumParticleLifetime = minimumLifetime;
		}
	}

	void
	ParticlesEmitter::setParticleSize (float size) noexcept
	{
		m_minimumParticleSize = size;
		m_maximumParticleSize = size;
	}

	void
	ParticlesEmitter::setParticleSize (float minimumSize, float maximumSize) noexcept
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

	void
	ParticlesEmitter::start (uint32_t duration) noexcept
	{
		this->enableFlag(IsEmitting);

		if ( duration > 0 )
		{
			m_timedEvent = std::make_unique< Time::TimedEvent< uint64_t, std::micro > >(
				[&] (Time::TimerID /*timerID*/) {
					this->stop();

					return true;
				},
				duration,
				true
			);
			m_timedEvent->start();
		}
	}

	void
	ParticlesEmitter::stop () noexcept
	{
		this->disableFlag(IsEmitting);
	}

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

	std::string
	to_string (const ParticlesEmitter & obj) noexcept
	{
		std::stringstream output;

		output << obj;

		return output.str();
	}
}
