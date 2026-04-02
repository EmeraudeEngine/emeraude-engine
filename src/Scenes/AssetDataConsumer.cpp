/*
 * src/Scenes/AssetDataConsumer.cpp
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

#include "AssetDataConsumer.hpp"

/* STL inclusions. */
#include <numbers>

/* Local inclusions. */
#include "AssetLoaders/AssetData.hpp"
#include "Graphics/Renderable/Abstract.hpp"
#include "Libs/Math/Vector.hpp"
#include "Node.hpp"
#include "Scene.hpp"
#include "Scenes/Component/Visual.hpp"
#include "Tracer.hpp"

namespace EmEn::Scenes
{
	using namespace Libs::Math;

	bool
	AssetDataConsumer::build (const AssetLoaders::AssetData & assetData, Scene & scene, const std::shared_ptr< Node > & parentNode) noexcept
	{
		if ( assetData.rootNodeIndices.empty() )
		{
			Tracer::warning(ClassId, "AssetData has no root nodes, nothing to build.");

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
			for ( const auto nodeIndex : assetData.rootNodeIndices )
			{
				this->processNodeAsStatic(assetData, nodeIndex, scene, yUpToYDownFrame);
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

				for ( size_t nodeIndex = 0; nodeIndex < assetData.nodes.size(); ++nodeIndex )
				{
					const auto & nodeDesc = assetData.nodes[nodeIndex];

					if ( !nodeDesc.meshIndex.has_value() )
					{
						continue;
					}

					const auto meshIndex = nodeDesc.meshIndex.value();

					if ( meshIndex >= assetData.meshes.size() || assetData.meshes[meshIndex].renderable == nullptr )
					{
						continue;
					}

					if ( firstMesh )
					{
						parentNode->componentBuilder< Component::Visual >(nodeDesc.name + "/Visual")
							.setup([] (auto & visual) {
								visual.getRenderableInstance()->enableLighting();
							})
							.build(assetData.meshes[meshIndex].renderable);

						firstMesh = false;
					}
					else
					{
						auto childNode = parentNode->createChild(nodeDesc.name);

						childNode->componentBuilder< Component::Visual >(nodeDesc.name + "/Visual")
							.setup([] (auto & visual) {
								visual.getRenderableInstance()->enableLighting();
							})
							.build(assetData.meshes[meshIndex].renderable);
					}
				}
			}
			else
			{
				/* Default mode: build hierarchy with automatic identity flattening. */
				for ( const auto nodeIndex : assetData.rootNodeIndices )
				{
					this->processNodeAsNode(assetData, nodeIndex, parentNode);
				}
			}
		}

		return true;
	}

	void
	AssetDataConsumer::processNodeAsStatic (const AssetLoaders::AssetData & assetData, size_t nodeIndex, Scene & scene, const CartesianFrame< float > & parentWorldFrame) noexcept
	{
		if ( nodeIndex >= assetData.nodes.size() )
		{
			return;
		}

		const auto & nodeDesc = assetData.nodes[nodeIndex];

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

		/* Create a StaticEntity if this node has a mesh. */
		if ( nodeDesc.meshIndex.has_value() )
		{
			const auto meshIndex = nodeDesc.meshIndex.value();

			if ( meshIndex < assetData.meshes.size() && assetData.meshes[meshIndex].renderable != nullptr )
			{
				auto staticEntity = scene.createStaticEntity(nodeDesc.name, worldFrame);

				if ( staticEntity != nullptr )
				{
					staticEntity->componentBuilder< Component::Visual >(nodeDesc.name + "/Visual")
						.setup([] (auto & visual) {
							visual.getRenderableInstance()->enableLighting();
						})
						.build(assetData.meshes[meshIndex].renderable);
				}
			}
		}

		/* Recurse into children with accumulated world transform. */
		for ( const auto childIndex : nodeDesc.childIndices )
		{
			this->processNodeAsStatic(assetData, childIndex, scene, worldFrame);
		}
	}

	void
	AssetDataConsumer::processNodeAsNode (const AssetLoaders::AssetData & assetData, size_t nodeIndex, const std::shared_ptr< Node > & engineParent) noexcept
	{
		if ( nodeIndex >= assetData.nodes.size() )
		{
			return;
		}

		const auto & nodeDesc = assetData.nodes[nodeIndex];

		/* Skip skeleton joint nodes — their transforms are driven by
		 * the SkeletalAnimator, not by the scene node hierarchy.
		 * However, if the joint also carries a mesh, we must process it. */
		if ( assetData.skinJointNodeIndices.contains(nodeIndex) && !nodeDesc.meshIndex.has_value() )
		{
			return;
		}

		const auto & frame = nodeDesc.localFrame;
		const bool hasTransform = (frame.getModelMatrix() != CartesianFrame< float >{}.getModelMatrix());
		const bool hasMesh = nodeDesc.meshIndex.has_value();

		/* Flatten the hierarchy when possible:
		 * - Identity transform + no mesh → skip this node, pass parent through.
		 * - Has mesh or has transform → need a node in the scene.
		 *   If the parent has no Visual yet, attach directly to it.
		 *   Otherwise, create a child node. */
		std::shared_ptr< Node > targetNode;

		if ( !hasMesh && !hasTransform )
		{
			/* Identity, no mesh: flatten — skip this node entirely. */
			targetNode = engineParent;
		}
		else if ( hasMesh && !hasTransform && !engineParent->hasComponent() )
		{
			/* First mesh with identity transform: attach directly to the parent node. */
			const auto meshIndex = nodeDesc.meshIndex.value();

			if ( meshIndex < assetData.meshes.size() && assetData.meshes[meshIndex].renderable != nullptr )
			{
				engineParent->componentBuilder< Component::Visual >(nodeDesc.name + "/Visual")
					.setup([] (auto & visual) {
						visual.getRenderableInstance()->enableLighting();
					})
					.build(assetData.meshes[meshIndex].renderable);
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

				if ( meshIndex < assetData.meshes.size() && assetData.meshes[meshIndex].renderable != nullptr )
				{
					targetNode->componentBuilder< Component::Visual >(nodeDesc.name + "/Visual")
						.setup([] (auto & visual) {
							visual.getRenderableInstance()->enableLighting();
						})
						.build(assetData.meshes[meshIndex].renderable);
				}
			}
		}

		/* Recurse into children. */
		for ( const auto childIndex : nodeDesc.childIndices )
		{
			this->processNodeAsNode(assetData, childIndex, targetNode);
		}
	}
}
