/*
 * src/Scenes/Scene.debug.cpp
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
#include "Graphics/Geometry/ResourceGenerator.hpp"
#include "Graphics/Material/StandardResource.hpp"
#include "Graphics/RenderTarget/Abstract.hpp"
#include "Graphics/Renderable/MeshResource.hpp"
#include "Graphics/Renderer.hpp"
#include "Graphics/ViewMatricesInterface.hpp"
#include "PixelFactory/Color.hpp"
#include "Resources/Manager.hpp"
#include "Tracer.hpp"
#include "Vulkan/CommandBuffer.hpp"

namespace EmEn::Scenes
{
	using namespace Base;
	using namespace Base::Math;
	using namespace Base::PixelFactory;
	using namespace Graphics;

	constexpr auto AxisCount{6U};

	namespace
	{
		struct debugAxis
		{
			const char * label = nullptr;
			Vector< 3, float > position;
			Color< float > color;
		};
	}

	/* NOTE: The compass landmark table now lives in Debug::Compass — the compass is no longer
	 * built out of scene entities. Only the boundary planes still use this local table. */

	/** @brief Debug entity name prefix for ground zero plane. */
	constexpr auto GroundZeroPlaneDisplay{"+GroundZeroPlane"};

	/** @brief Luminance carried by the debug grids, in nits.
	 * @note Unlike the compass, these planes are drawn INSIDE the scene pass (they need depth
	 * occlusion), so the camera exposure applies to them. They are unlit and emissive: without an
	 * absolute luminance their raw [0,1] vertex colors would be crushed to black by the photometric
	 * exposure. This value sits in the overcast-sky range — readable against any usual exposure
	 * without blowing the tone mapping out. */
	constexpr auto DebugGridLuminance{2000.0F};

	/** @brief Debug entity name prefix for boundary planes. */
	constexpr std::array< debugAxis , AxisCount > BoundaryPlanes{{
		{
			.label = "+BoundaryPlane0",
			.position = Vector< 3, float >::positiveX(),
			.color = Red
		},
		{
			.label = "+BoundaryPlane1",
			.position = Vector< 3, float >::positiveY(),
			.color = Green
		},
		{
			.label = "+BoundaryPlane2",
			.position = Vector< 3, float >::positiveZ(),
			.color = Blue
		},
		{
			.label = "+BoundaryPlane3",
			.position = Vector< 3, float >::negativeX(),
			.color = Cyan
		},
		{
			.label = "+BoundaryPlane4",
			.position = Vector< 3, float >::negativeY(),
			.color = Magenta
		},
		{
			.label = "+BoundaryPlane5",
			.position = Vector< 3, float >::negativeZ(),
			.color = Yellow
		}
	}};

	void
	Scene::enableCompassDisplay (Resources::Manager & resources) noexcept
	{
		if ( this->compassDisplayEnabled() )
		{
			return;
		}

		/* ⚠️ The compass is NOT built as a scene entity any more. As scene content it was drawn in
		 * the HDR pass, where `ToneMapping` multiplies everything by the camera exposure — an
		 * exposure calibrated for thousands of nits turned the spheres black even though they were
		 * already unlit. A reference whose colors depend on the camera settings measures nothing.
		 * It is now a standalone visual recorded after the post-process chain: see Debug::Compass
		 * and Scene::renderDebugOverlay(). */
		auto & renderer = resources.graphicsRenderer();

		if ( !m_compass.create(renderer, resources, renderer.mainRenderTarget()) )
		{
			Tracer::error(ClassId, "Unable to create the debug compass !");
		}
	}

	void
	Scene::disableCompassDisplay () noexcept
	{
		m_compass.destroy();
	}

	bool
	Scene::compassDisplayEnabled () const noexcept
	{
		return m_compass.isCreated();
	}

	void
	Scene::renderDebugOverlay (const Vulkan::CommandBuffer & commandBuffer) const noexcept
	{
		if ( !m_compass.isCreated() )
		{
			return;
		}

		/* NOTE: The view matrices of the main camera, resolved the same way the scene editor
		 * resolves them for its gizmos: the first render-to-view target. */
		const Graphics::ViewMatricesInterface * viewMatrices = nullptr;

		this->forEachRenderToView([&viewMatrices] (const auto & renderTarget) {
			if ( viewMatrices == nullptr )
			{
				viewMatrices = &renderTarget->viewMatrices();
			}
		});

		if ( viewMatrices == nullptr )
		{
			return;
		}

		m_compass.render(commandBuffer, *viewMatrices);
	}

	bool
	Scene::toggleCompassDisplay (Resources::Manager & resources) noexcept
	{
		if ( this->compassDisplayEnabled() )
		{
			this->disableCompassDisplay();

			return false;
		}

		this->enableCompassDisplay(resources);

		return true;
	}

	void
	Scene::enableGroundZeroDisplay (Resources::Manager & resources) noexcept
	{
		if ( this->groundZeroDisplayEnabled() )
		{
			return;
		}

		const auto planeSize = m_boundary * Double< float >;
		const auto planeDivision = static_cast< uint32_t >(m_boundary / 100.0F);

		const auto mesh = resources.container< Renderable::MeshResource >()
			->getOrCreateResource(GroundZeroPlaneDisplay, [&resources, planeSize, planeDivision] (auto & meshResource) {
				Geometry::ResourceGenerator generator{resources, Geometry::EnableNormal | Geometry::EnableVertexColor | Geometry::EnablePrimitiveRestart};
				generator.parameters().setVertexColorGenMode(VertexColorGenMode::UseGlobalColor);
				generator.parameters().setGlobalVertexColor(White);

				const auto geometry = generator.surface(planeSize, planeDivision, GroundZeroPlaneDisplay);

				const auto material = resources.container< Material::StandardResource >()
					->getOrCreateResource("+DebugSceneMaterial", [] (auto & materialResource) {
						/* NOTE: A measurement reference must never depend on the lighting of the scene
						 * it measures: unlit, carrying its own luminance, with the vertex colors as the
						 * only signal. */
						materialResource.enableVertexColor();
						materialResource.enableUnlit();
						materialResource.setAutoIlluminationComponent(1.0F);
						materialResource.setEmissiveStrength(DebugGridLuminance);

						return materialResource.setManualLoadSuccess(true);
					});

				return meshResource.load(geometry, material, {PolygonMode::Line, CullingMode::None});
			});

		const auto meshInstance = this->createStaticEntity(GroundZeroPlaneDisplay)
			->componentBuilder< Component::Visual >(GroundZeroPlaneDisplay)
			.setup([] (auto & component) {
				component.getRenderableInstance()->disableDepthTest(true);
			})
			.build(mesh);
	}

	void
	Scene::disableGroundZeroDisplay () noexcept
	{
		this->removeStaticEntity(GroundZeroPlaneDisplay);
	}

	bool
	Scene::groundZeroDisplayEnabled () const noexcept
	{
		return m_staticEntities.contains(GroundZeroPlaneDisplay);
	}

	void
	Scene::toggleGroundZeroDisplay (Resources::Manager & resources) noexcept
	{
		if ( this->groundZeroDisplayEnabled() )
		{
			this->disableGroundZeroDisplay();
		}
		else
		{
			this->enableGroundZeroDisplay(resources);
		}
	}

	void
	Scene::enableBoundaryPlanesDisplay (Resources::Manager & resources) noexcept
	{
		if ( this->boundaryPlanesDisplayEnabled() )
		{
			return;
		}

		const auto planeSize = m_boundary * Double< float >;
		const auto planeDivision = static_cast< uint32_t >(m_boundary / 100.0F);

		for ( const auto & plane : BoundaryPlanes )
		{
			const auto mesh = resources.container< Renderable::MeshResource >()
				->getOrCreateResource(plane.label, [&resources, &plane, planeSize, planeDivision] (auto & meshResource) {
					Geometry::ResourceGenerator generator{resources, Geometry::EnableNormal | Geometry::EnableVertexColor | Geometry::EnablePrimitiveRestart};
					generator.parameters().setVertexColorGenMode(VertexColorGenMode::UseGlobalColor);
					generator.parameters().setGlobalVertexColor(plane.color);

					const auto geometryResource = generator.surface(planeSize, planeDivision, plane.label);

					const auto material = resources.container< Material::StandardResource >()
						->getOrCreateResource("+DebugSceneMaterial", [] (auto & materialResource) {
							/* NOTE: Same shared debug material as the ground-zero plane: unlit and
							 * emissive, so the exposure of the scene pass cannot crush the reference. */
							materialResource.enableVertexColor();
							materialResource.enableUnlit();
							materialResource.setAutoIlluminationComponent(1.0F);
							materialResource.setEmissiveStrength(DebugGridLuminance);

							return materialResource.setManualLoadSuccess(true);
						});

					return meshResource.load(geometryResource, material, {PolygonMode::Line, CullingMode::None});
				});

			const auto entity = this->createStaticEntity(plane.label, plane.position * m_boundary);

			if ( plane.position[X] != 0.0F )
			{
				entity->roll(Radian(QuartRevolution< float >), TransformSpace::Local);
			}

			if ( plane.position[Z] != 0.0F )
			{
				entity->pitch(Radian(QuartRevolution< float >), TransformSpace::Local);
			}

			const auto meshInstance = entity
				->componentBuilder< Component::Visual >(plane.label)
				.setup([] (auto & component) {
					component.getRenderableInstance()->disableDepthTest(true);
				})
				.build(mesh);
		}
	}

	void
	Scene::disableBoundaryPlanesDisplay () noexcept
	{
		for ( const auto & plane : BoundaryPlanes )
		{
			this->removeStaticEntity(plane.label);
		}
	}

	bool
	Scene::boundaryPlanesDisplayEnabled () const noexcept
	{
		return m_staticEntities.contains(BoundaryPlanes[0].label);
	}

	void
	Scene::toggleBoundaryPlanesDisplay (Resources::Manager & resourceManager) noexcept
	{
		if ( this->boundaryPlanesDisplayEnabled() )
		{
			this->disableGroundZeroDisplay();
		}
		else
		{
			this->enableGroundZeroDisplay(resourceManager);
		}
	}
}
