/*
 * src/Scenes/SceneDataConsumer.cpp
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

#include "SceneDataConsumer.hpp"

/* STL inclusions. */
#include <numbers>

/* Local inclusions. */
#include "SceneLoaders/SceneData.hpp"
#include "Graphics/Renderable/Abstract.hpp"
#include "Math/Vector.hpp"
#include "Node.hpp"
#include "Scene.hpp"
#include "Scenes/Component/DirectionalLight.hpp"
#include "Scenes/Component/PointLight.hpp"
#include "Scenes/Component/SpotLight.hpp"
#include "Scenes/Component/Visual.hpp"
#include "StaticEntity.hpp"
#include "Tracer.hpp"

namespace EmEn::Scenes
{
	using namespace Base::Math;

	bool
	SceneDataConsumer::build (const SceneLoaders::SceneData & sceneData, Scene & scene, const std::shared_ptr< Node > & parentNode) noexcept
	{
		if ( sceneData.rootNodeIndices.empty() )
		{
			Tracer::warning(ClassId, "SceneData has no root nodes, nothing to build.");

			return true;
		}

		/* glTF is Y-up, engine is Y-down: 180° rotation around X.
		 * Build the root transform that will be applied to all children. */
		CartesianFrame< float > yUpToYDownFrame;
		yUpToYDownFrame.rotate(std::numbers::pi_v< float >, Vector< 3, float >::positiveX(), true);

		const bool useStaticEntities = (parentNode == nullptr);

		if ( useStaticEntities )
		{
			/* Static mode: create StaticEntity for each mesh node with world coordinates. */
			for ( const auto nodeIndex : sceneData.rootNodeIndices )
			{
				this->processNodeAsStatic(sceneData, nodeIndex, scene, yUpToYDownFrame);
			}
		}
		else
		{
			/* Node mode: build content under the caller-provided node.
			 * Apply the Y-up → Y-down coordinate conversion. */
			parentNode->rotate(std::numbers::pi_v< float >, Vector< 3, float >::positiveX(), TransformSpace::Local);

			if ( m_flattenHierarchy )
			{
				/* Flatten mode: skip all intermediate nodes, attach meshes directly. */
				bool firstMesh = true;

				for ( size_t nodeIndex = 0; nodeIndex < sceneData.nodes.size(); ++nodeIndex )
				{
					const auto & nodeDesc = sceneData.nodes[nodeIndex];

					if ( !nodeDesc.meshIndex.has_value() )
					{
						continue;
					}

					const auto meshIndex = nodeDesc.meshIndex.value();

					if ( meshIndex >= sceneData.meshes.size() || sceneData.meshes[meshIndex].renderable == nullptr )
					{
						continue;
					}

					if ( firstMesh )
					{
						parentNode->componentBuilder< Component::Visual >(nodeDesc.name + "/Visual")
							.setup([lightingEnabled = sceneData.meshes[meshIndex].lightingEnabled] (auto & visual) {
								visual.getRenderableInstance()->setLightingState(lightingEnabled);
							})
							.build(sceneData.meshes[meshIndex].renderable);

						firstMesh = false;
					}
					else
					{
						auto childNode = parentNode->createChild(nodeDesc.name);

						childNode->componentBuilder< Component::Visual >(nodeDesc.name + "/Visual")
							.setup([lightingEnabled = sceneData.meshes[meshIndex].lightingEnabled] (auto & visual) {
								visual.getRenderableInstance()->setLightingState(lightingEnabled);
							})
							.build(sceneData.meshes[meshIndex].renderable);
					}
				}
			}
			else
			{
				/* Default mode: build hierarchy with automatic identity flattening. */
				for ( const auto nodeIndex : sceneData.rootNodeIndices )
				{
					this->processNodeAsNode(sceneData, nodeIndex, parentNode);
				}
			}
		}

		return true;
	}

	template< typename entity_t >
	void
	SceneDataConsumer::attachLight (const SceneLoaders::SceneData & sceneData, const SceneLoaders::NodeDescriptor & nodeDescriptor, entity_t & entity) const noexcept
	{
		if ( !m_createLights || !nodeDescriptor.lightIndex.has_value() )
		{
			return;
		}

		const auto lightIndex = nodeDescriptor.lightIndex.value();

		if ( lightIndex >= sceneData.lights.size() )
		{
			return;
		}

		const auto & light = sceneData.lights[lightIndex];
		const auto componentName = nodeDescriptor.name + "/Light";

		/* The descriptor already carries the engine's own photometric units — lux for a
		 * directional light, candela for a point or a spot — so each setter takes the value
		 * as-is. Converting here would mean converting twice. */
		switch ( light.type )
		{
			case SceneLoaders::LightType::Directional :
				entity.template componentBuilder< Component::DirectionalLight >(componentName)
					.setup([&light] (auto & component) {
						component.setColor(light.color);
						component.setIlluminance(light.intensity);
					})
					.build();
				break;

			case SceneLoaders::LightType::Point :
				entity.template componentBuilder< Component::PointLight >(componentName)
					.setup([&light] (auto & component) {
						component.setColor(light.color);
						component.setIntensity(light.intensity);

						/* The range is a CULLING BOUND, not a dimmer. An asset that declares
						 * none leaves the engine default alone. */
						if ( light.range > 0.0F )
						{
							component.setRadius(light.range);
						}
					})
					.build();
				break;

			case SceneLoaders::LightType::Spot :
				entity.template componentBuilder< Component::SpotLight >(componentName)
					.setup([&light] (auto & component) {
						component.setColor(light.color);
						component.setIntensity(light.intensity);

						/* ⚠️ setRadius() is derived from the OUTER cone angle, so the angles
						 * MUST be set first — reversing these two lines silently yields a
						 * wrong culling radius. */
						component.setConeAngles(light.innerConeAngle, light.outerConeAngle);

						if ( light.range > 0.0F )
						{
							component.setRadius(light.range);
						}
					})
					.build();
				break;
		}
	}

	void
	SceneDataConsumer::processNodeAsStatic (const SceneLoaders::SceneData & sceneData, size_t nodeIndex, Scene & scene, const CartesianFrame< float > & parentWorldFrame) noexcept
	{
		if ( nodeIndex >= sceneData.nodes.size() )
		{
			return;
		}

		const auto & nodeDesc = sceneData.nodes[nodeIndex];

		/* Compute this node's world frame by accumulating transforms.
		 * NOTE: Extract scale from the combined matrix column lengths before
		 * CartesianFrame normalizes the direction vectors (which would lose scale). */
		const auto worldMatrix = parentWorldFrame.getModelMatrix() * nodeDesc.localFrame.getModelMatrix();
		const Vector< 3, float > worldScale{
			Vector< 3, float >{worldMatrix[M4x4Col0Row0], worldMatrix[M4x4Col0Row1], worldMatrix[M4x4Col0Row2]}.length(),
			Vector< 3, float >{worldMatrix[M4x4Col1Row0], worldMatrix[M4x4Col1Row1], worldMatrix[M4x4Col1Row2]}.length(),
			Vector< 3, float >{worldMatrix[M4x4Col2Row0], worldMatrix[M4x4Col2Row1], worldMatrix[M4x4Col2Row2]}.length()
		};
		const CartesianFrame< float > worldFrame{worldMatrix, worldScale};

		/* Create a StaticEntity when this node carries anything the scene must own. A light-only
		 * node has no mesh but still needs an entity to hold its emitter. */
		const bool hasMesh =
			nodeDesc.meshIndex.has_value() &&
			nodeDesc.meshIndex.value() < sceneData.meshes.size() &&
			sceneData.meshes[nodeDesc.meshIndex.value()].renderable != nullptr;
		const bool hasLight = m_createLights && nodeDesc.lightIndex.has_value();

		if ( hasMesh || hasLight )
		{
			auto staticEntity = scene.createStaticEntity(nodeDesc.name, worldFrame);

			if ( staticEntity != nullptr )
			{
				if ( hasMesh )
				{
					const auto meshIndex = nodeDesc.meshIndex.value();

					staticEntity->componentBuilder< Component::Visual >(nodeDesc.name + "/Visual")
						.setup([lightingEnabled = sceneData.meshes[meshIndex].lightingEnabled] (auto & visual) {
							visual.getRenderableInstance()->setLightingState(lightingEnabled);
						})
						.build(sceneData.meshes[meshIndex].renderable);
				}

				this->attachLight(sceneData, nodeDesc, *staticEntity);
			}
		}

		/* Recurse into children with accumulated world transform. */
		for ( const auto childIndex : nodeDesc.childIndices )
		{
			this->processNodeAsStatic(sceneData, childIndex, scene, worldFrame);
		}
	}

	void
	SceneDataConsumer::processNodeAsNode (const SceneLoaders::SceneData & sceneData, size_t nodeIndex, const std::shared_ptr< Node > & engineParent) noexcept
	{
		if ( nodeIndex >= sceneData.nodes.size() )
		{
			return;
		}

		const auto & nodeDesc = sceneData.nodes[nodeIndex];

		/* Skip skeleton joint nodes — their transforms are driven by
		 * the SkeletalAnimator, not by the scene node hierarchy.
		 * However, if the joint also carries a mesh, we must process it. */
		if ( sceneData.skinJointNodeIndices.contains(nodeIndex) && !nodeDesc.meshIndex.has_value() )
		{
			return;
		}

		const auto & frame = nodeDesc.localFrame;
		const bool hasTransform = (frame.getModelMatrix() != CartesianFrame< float >{}.getModelMatrix());
		const bool hasMesh = nodeDesc.meshIndex.has_value();
		/* A light-only node must survive flattening: dropping it would drop the emitter with it. */
		const bool hasLight = m_createLights && nodeDesc.lightIndex.has_value();

		/* Flatten the hierarchy when possible:
		 * - Identity transform + no mesh → skip this node, pass parent through.
		 * - Has mesh or has transform → need a node in the scene.
		 *   If the parent has no Visual yet, attach directly to it.
		 *   Otherwise, create a child node. */
		std::shared_ptr< Node > targetNode;

		if ( !hasMesh && !hasLight && !hasTransform )
		{
			/* Identity, no mesh, no light: flatten — skip this node entirely. */
			targetNode = engineParent;
		}
		else if ( hasMesh && !hasLight && !hasTransform && !engineParent->hasComponent() )
		{
			/* First mesh with identity transform: attach directly to the parent node. */
			const auto meshIndex = nodeDesc.meshIndex.value();

			if ( meshIndex < sceneData.meshes.size() && sceneData.meshes[meshIndex].renderable != nullptr )
			{
				engineParent->componentBuilder< Component::Visual >(nodeDesc.name + "/Visual")
					.setup([lightingEnabled = sceneData.meshes[meshIndex].lightingEnabled] (auto & visual) {
						visual.getRenderableInstance()->setLightingState(lightingEnabled);
					})
					.build(sceneData.meshes[meshIndex].renderable);
			}

			targetNode = engineParent;
		}
		else
		{
			/* Additional mesh or structural node with transform: create a child. */
			targetNode = engineParent->createChild(nodeDesc.name, frame);

			if ( hasMesh )
			{
				const auto meshIndex = nodeDesc.meshIndex.value();

				if ( meshIndex < sceneData.meshes.size() && sceneData.meshes[meshIndex].renderable != nullptr )
				{
					targetNode->componentBuilder< Component::Visual >(nodeDesc.name + "/Visual")
						.setup([lightingEnabled = sceneData.meshes[meshIndex].lightingEnabled] (auto & visual) {
							visual.getRenderableInstance()->setLightingState(lightingEnabled);
						})
						.build(sceneData.meshes[meshIndex].renderable);
				}
			}
		}

		/* Safe in every branch above: whenever targetNode was flattened onto the parent, the
		 * node was known to carry no light. */
		this->attachLight(sceneData, nodeDesc, *targetNode);

		/* Recurse into children. */
		for ( const auto childIndex : nodeDesc.childIndices )
		{
			this->processNodeAsNode(sceneData, childIndex, targetNode);
		}
	}
}
