/*
 * src/Scenes/Scene.lighting.cpp
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

#include "Scene.hpp"

/* Local inclusions. */
#include "Graphics/Compute/IBLBaker.hpp"
#include "Graphics/IBLTexture.hpp"

namespace EmEn::Scenes
{
	using namespace Base;
	using namespace Base::Math;
	using namespace Graphics;

	bool
	Scene::applyBackgroundLighting (const BackgroundLightingOptions & options) noexcept
	{
		if ( m_backgroundResource == nullptr )
		{
			Tracer::warning(ClassId, "There is no background to derive the lighting from !");

			return false;
		}

		/* ⚠️ Enable the light set NOW, not at application time: LightSet::initialize() runs
		 * once at scene enabling and skips a disabled set — a deferred enable would leave the
		 * whole scene without light buffers, hence without a single lighted render program. */
		m_lightSet.enable();

		/* ⚠️ NEVER apply from here: this entry point runs on the CALLER's thread (a console
		 * TCP thread, an input event callback, a demo constructor...) while the application
		 * creates entities and lights — logic-thread work. The request is polled and honored
		 * by Scene::processLogics() as soon as the background resource is loaded. */
		m_backgroundLightingOptions = options;
		m_backgroundLightingRequested = true;

		return true;
	}

	void
	Scene::applyBackgroundLightingNow () noexcept
	{
		/* The entity holding a derived directional light sits toward the celestial body: the
		 * component default behavior shines along -normalize(position), i.e. from the body to
		 * the scene. The magnitude itself is irrelevant to the light. */
		constexpr auto StarEntityDistance{1000.0F};

		const auto & background = *m_backgroundResource;
		const auto & options = m_backgroundLightingOptions;

		m_backgroundLightingRequested = false;

		m_lightSet.enable();

		/* RE-APPLICATION (background switch): the stars of the previous sky must go away,
		 * or every switch would stack directional lights. */
		for ( const auto & entityName : m_backgroundStarEntities )
		{
			this->removeStaticEntity(entityName);
		}

		m_backgroundStarEntities.clear();

		/* The applyAmbient contract gates the IBL diffuse: when the sky drives the ambient,
		 * the baked irradiance cubemap replaces the flat scalar term in the ambient pass
		 * (refreshAmbientLightProperties pushes a ZERO intensity to the view UBOs — a
		 * directional E(n) and a flat scalar would double-count the same sky). The LightSet
		 * still records the photometric values: post-process effects read them dynamically. */
		m_IBLAmbientEnabled = options.applyAmbient;

		if ( options.applyAmbient )
		{
			m_lightSet.setAmbientLightColor(background.averageColor());
			m_lightSet.setAmbientLightIntensity(background.ambientIlluminance());
		}

		if ( options.applyStars )
		{
			uint32_t starIndex = 0;

			for ( const auto & star : background.stars() )
			{
				const auto entityName = background.name() + CelestialBody::typeName(star.type()) + std::to_string(starIndex++);

				CartesianFrame< float > coordinates;
				coordinates.setPosition(star.direction() * StarEntityDistance);

				const auto entity = this->createStaticEntity(entityName, coordinates);

				if ( entity == nullptr )
				{
					TraceError{ClassId} << "Unable to create the static entity '" << entityName << "' for a background star !";

					continue;
				}

				const auto setup = [&star] (auto & light) {
					light.setColor(star.color());
					/* The celestial body illuminance is in lux, the directional light unit. */
					light.setIlluminance(star.illuminance());
				};

				std::shared_ptr< Component::DirectionalLight > component;

				if ( options.shadowMapResolution == 0 )
				{
					component = entity->componentBuilder< Component::DirectionalLight >(entityName).setup(setup).build();
				}
				else if ( options.cascadeCount > 0 )
				{
					component = entity->componentBuilder< Component::DirectionalLight >(entityName).setup(setup).build(options.shadowMapResolution, options.cascadeCount, options.cascadeLambda, options.cascadeScale);
				}
				else
				{
					component = entity->componentBuilder< Component::DirectionalLight >(entityName).setup(setup).build(options.shadowMapResolution, options.shadowCoverage);
				}

				if ( component == nullptr )
				{
					TraceError{ClassId} << "Unable to create the directional light '" << entityName << "' from a background star !";
				}
				else
				{
					m_backgroundStarEntities.emplace_back(entityName);
				}
			}
		}

		this->refreshAmbientLightProperties();

		/* The lighting derivation IS scene content for a probe: the sun and the ambient
		 * just arrived (possibly SECONDS after scene build — this waits on the async
		 * background resource load), so everything a once-probe baked before this point
		 * was captured UNLIT (measured: pitch-black floor in the reflexion-debug once-probe
		 * when the bake won the race against the BlueSky load). Same contract as
		 * setBackground(): re-bake the on-demand targets (deferred flag, thread-safe). */
		this->signalOnDemandRenderTargets();
	}

	void
	Scene::refreshAmbientLightProperties () const noexcept
	{
		const auto & color = m_lightSet.ambientLightColor();
		/* When the sky drives the ambient (applyAmbient), the ambient pass reads the baked
		 * irradiance cubemap instead — push a zero scalar so the two never double-count.
		 * The LightSet keeps the photometric values for the effects reading it directly. */
		const auto intensity = m_IBLAmbientEnabled ? 0.0F : m_lightSet.ambientLightIntensity();

		/* NOTE: 1.0 is the neutral IBL scale when no background declares a luminance. */
		const auto environmentLuminance = m_backgroundResource != nullptr ? m_backgroundResource->luminance() : 1.0F;

		if ( const auto renderTarget = m_AVConsoleManager.graphicsRenderer().mainRenderTarget(); renderTarget != nullptr )
		{
			renderTarget->viewMatrices().updateAmbientLightProperties(color, intensity, environmentLuminance);
		}

		const auto updateTarget = [&color, intensity, environmentLuminance] (const std::shared_ptr< RenderTarget::Abstract > & renderTarget) {
			renderTarget->viewMatrices().updateAmbientLightProperties(color, intensity, environmentLuminance);
		};

		this->forEachRenderToView(updateTarget);
		this->forEachRenderToTexture(updateTarget);
	}

	void
	Scene::updateCSMCascades (const std::shared_ptr< RenderTarget::Abstract > & mainRenderTarget) const noexcept
	{
		{
			const std::scoped_lock lock{m_renderToShadowMapAccess};

			if ( mainRenderTarget == nullptr || m_renderToShadowMaps.empty() )
			{
				return;
			}
		}

		/* Get frustum corners from the View's matrices (which come from the connected camera). */
		const auto & viewMatrices = mainRenderTarget->viewMatrices();
		const std::array< Vector< 3, float >, 8 > frustumCorners = viewMatrices.getFrustumCornersWorld();

		/* Get near and far planes from the view matrices. */
		const float nearPlane = viewMatrices.nearPlane();
		const float farPlane = viewMatrices.farPlane();

		/* Update all CSM-enabled directional lights with the camera frustum.
		 * NOTE: We iterate through lights because they know their direction. */
		for ( const auto & light : m_lightSet.directionalLights() )
		{
			if ( light->usesCSM() && light->isShadowCastingEnabled() )
			{
				light->updateCascades(frustumCorners, nearPlane, farPlane);
			}
		}
	}

	void
	Scene::updateEnvironmentIBL () noexcept
	{
		/* The bindless set is the mutex-protected source of truth for the adopted
		 * environment cubemap (setBackground can run on any thread and the late adoption
		 * of an async-loaded cubemap happens on the render thread). The engine default
		 * black cubemap is never baked: the reserved IBL slots already park on it. */
		const auto source = m_bindlessTextureSet.environmentCubemap();

		if ( source != nullptr && source->isCreated() && source.get() != m_IBLBakedSource
			&& source.get() != static_cast< const Vulkan::TextureInterface * >(m_graphicsRenderer.getDefaultTextureCubemap().get()) )
		{
			auto & irradiance = m_IBLIrradiance[m_IBLWriteIndex];
			auto & prefiltered = m_IBLPrefiltered[m_IBLWriteIndex];

			if ( irradiance == nullptr )
			{
				irradiance = std::make_shared< IBLTexture >(IBLTexture::Role::IrradianceCubemap);
			}

			if ( prefiltered == nullptr )
			{
				prefiltered = std::make_shared< IBLTexture >(IBLTexture::Role::PrefilteredCubemap);
			}

			/* NOTE: Whatever happens below, do not retry every logic tick on the same source. */
			m_IBLBakedSource = source.get();

			if ( irradiance->create(m_graphicsRenderer) && prefiltered->create(m_graphicsRenderer) )
			{
				if ( m_graphicsRenderer.IBLBaker().bakeEnvironment(*source, *irradiance, *prefiltered) )
				{
					/* Publish the prefiltered environment (UPDATE_AFTER_BIND hot-swap at
					 * the manager's next sync) and flip the pair: frames in flight keep
					 * sampling the previous bake untouched. The irradiance publication is
					 * decided below, by the applyAmbient contract. */
					m_bindlessTextureSet.setPrefilteredCubemap(prefiltered);
					m_IBLBakedIrradiance = irradiance;
					m_IBLWriteIndex = (m_IBLWriteIndex + 1) % 2;

					/* The environment IBL is scene LIGHT for every material (ambient-pass
					 * irradiance, prefiltered reflections): a once-probe baked before this
					 * point captured a darker world — re-bake it (deferred flag, thread-safe). */
					this->signalOnDemandRenderTargets();
				}
				else
				{
					TraceError{ClassId} << "Unable to bake the environment IBL from '" << m_environmentCubemap->name() << "' !";
				}
			}
			else
			{
				TraceError{ClassId} << "Unable to create the environment IBL textures !";
			}
		}

		/* The irradiance SLOT publication follows the applyAmbient contract, even between
		 * bakes (a scene can derive its lighting from the sky after the bake happened).
		 * Unpublished, the slot parks on the default black cubemap: the ambient-pass IBL
		 * diffuse term contributes nothing and the scalar ambient stands alone — that is
		 * the anti-double-count contract of the manually-lit scenes (RTGI demos). */
		const std::shared_ptr< Vulkan::TextureInterface > desiredIrradiance = m_IBLAmbientEnabled ? m_IBLBakedIrradiance : nullptr;

		if ( desiredIrradiance != m_IBLPublishedIrradiance )
		{
			m_bindlessTextureSet.setIrradianceCubemap(desiredIrradiance);

			m_IBLPublishedIrradiance = desiredIrradiance;
		}
	}
}
