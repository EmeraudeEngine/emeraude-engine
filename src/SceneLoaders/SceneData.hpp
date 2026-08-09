/*
 * src/SceneLoaders/SceneData.hpp
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
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <unordered_set>
#include <vector>

/* Local inclusions for usages. */
#include "Math/CartesianFrame.hpp"
#include "PixelFactory/Color.hpp"

/* Forward declarations. */
namespace EmEn::Animations
{
	class SkeletonResource;
	class AnimationClipResource;
}

namespace EmEn::Graphics
{
	namespace Geometry
	{
		class Interface;
	}

	namespace Material
	{
		class Interface;
	}

	namespace Renderable
	{
		class Abstract;
	}
}

namespace EmEn::SceneLoaders
{
	/**
	 * @brief Format-agnostic description of a node in the loaded asset.
	 * @note Contains no Scene/Node/Entity types — purely data.
	 */
	struct EMEN_API NodeDescriptor
	{
		std::string name;
		Base::Math::CartesianFrame< float > localFrame;
		std::optional< size_t > meshIndex;
		std::optional< size_t > lightIndex;
		std::optional< size_t > cameraIndex;
		std::vector< size_t > childIndices;
	};

	/**
	 * @brief Type of a punctual light declared by an asset.
	 */
	enum class LightType : uint8_t
	{
		Directional,
		Point,
		Spot,
		/**
		 * @brief An environment dome: a whole-sky emitter carrying an image, not a position.
		 * @note This is what USD calls a DomeLight and glTF has no equivalent for. Its
		 * @a textureAssetPath is the sky itself — the engine turns it into a cubemap background
		 * and derives its ambient and IBL from the texels, so it must NOT be instantiated as a
		 * punctual emitter.
		 */
		Environment
	};

	/**
	 * @brief Describes a light declared by an asset, in PHOTOMETRIC units.
	 * @note The unit of @a intensity depends on @a type and follows the engine's photometric
	 * contract exactly, which is also the glTF `KHR_lights_punctual` contract:
	 * a directional light carries an ILLUMINANCE in lux, a point or spot light carries a
	 * LUMINOUS INTENSITY in candela. A loader MUST convert its source unit here, once, rather
	 * than leaving the consumer to guess — a descriptor whose unit depends on the producing
	 * format would defeat the whole point of a format-agnostic contract.
	 */
	struct EMEN_API LightDescriptor
	{
		std::string name;
		Base::PixelFactory::Color< float > color{1.0F, 1.0F, 1.0F, 1.0F};
		/**
		 * @brief Illuminance in lux (directional) or luminous intensity in candela (point, spot).
		 */
		float intensity{0.0F};
		/**
		 * @brief Culling distance beyond which the contribution is dropped, in engine units.
		 * @note `0.0F` means the asset declared no range. It is NOT a dimmer: the engine's
		 * falloff is carried by the inverse square, the radius is a culling window.
		 */
		float range{0.0F};
		/**
		 * @brief Spot cone angles in DEGREES, converted from whatever the source format uses.
		 * @note Ignored unless @a type is Spot.
		 */
		float innerConeAngle{0.0F};
		float outerConeAngle{0.0F};
		/**
		 * @brief Absolute path to the environment image (Environment type only).
		 * @note Resolved by the loader against the asset, so the consumer never has to know
		 * where the source file lived.
		 */
		std::string textureAssetPath;
		LightType type{LightType::Point};
	};

	/**
	 * @brief Describes a camera declared by an asset.
	 * @note Authored viewpoints are DATA: the consumer never instantiates them on its own
	 * (owner decision, 2026-08-08). A caller turns one into a `Component::Camera`, uses it as a
	 * benchmark viewpoint, or ignores it. The node carrying the camera holds its pose.
	 */
	struct EMEN_API CameraDescriptor
	{
		std::string name;
		/**
		 * @brief Vertical field of view in DEGREES (perspective only).
		 * @note The engine derives the focal length from this through the camera's own sensor
		 * height, so the framing stays a lens — never feed an angle to the renderer directly.
		 */
		float yFieldOfView{0.0F};
		/**
		 * @brief Orthographic half-extents (orthographic only).
		 */
		float xMagnification{0.0F};
		float yMagnification{0.0F};
		float distanceNear{0.1F};
		/**
		 * @brief `0.0F` means the asset asked for an infinite projection.
		 */
		float distanceFar{0.0F};
		/**
		 * @brief `0.0F` means the asset left it to the render target.
		 */
		float aspectRatio{0.0F};
		bool orthographic{false};
	};

	/**
	 * @brief Describes a loaded mesh with its geometry and materials.
	 */
	struct EMEN_API MeshDescriptor
	{
		std::shared_ptr< Graphics::Renderable::Abstract > renderable;
		std::shared_ptr< Graphics::Geometry::Interface > geometry;
		std::vector< std::shared_ptr< Graphics::Material::Interface > > materials;
		/**
		 * @brief Declares whether the consumer must put this mesh on the LIT path.
		 * @note Default true: a mesh coming from a lit format (glTF, FBX) expects the light
		 * set, the ambient pass and the environment IBL. A loader that bakes its own lighting
		 * into the vertex colors on unlit materials — the WAD materializer being the reference
		 * case — MUST set this to false: on the lit path the ambient/IBL term is scaled by the
		 * background luminance, so installing a sky would multiply the surfaces by the sky
		 * brightness and destroy the baked look.
		 */
		bool lightingEnabled{true};
	};

	/**
	 * @brief Describes many copies of ONE mesh, each with its own placement.
	 * @note This is not a USD notion. USD carries it as a `PointInstancer`, glTF as
	 * `EXT_mesh_gpu_instancing`, FBX as duplicated nodes a loader may choose to fold — the
	 * contract states the INTENT ("the same renderable, N times, here"), never the encoding,
	 * so a consumer serves every format through one path.
	 *
	 * @warning A set is a HINT about redundancy, not an instruction to draw. How the instances
	 * reach the GPU — one instanced entity, spatial cells, a culling compute pass — belongs to
	 * the consumer, because only the scene knows its own culling machinery. A loader that
	 * decided that here would be re-implementing the renderer.
	 *
	 * @note Frames are expressed in the SAME space as the meshes of the same SceneData, so
	 * whatever conversion the consumer applies to a mesh node applies here unchanged. A loader
	 * baking its own axis conversion into vertices MUST bake the very same one into these
	 * frames, or the vegetation lands mirrored while the ground looks right.
	 */
	struct EMEN_API InstanceSetDescriptor
	{
		std::string name;
		/**
		 * @brief The world placement of every copy: position, rotation and scale.
		 */
		std::vector< Base::Math::CartesianFrame< float > > instances;
		/**
		 * @brief Index into SceneData::meshes of the renderable every instance draws.
		 * @note The mesh it points at is NOT expected to appear in the node hierarchy: a
		 * prototype exists to be instanced, and drawing it once more at the asset's origin is a
		 * bug that looks like a stray object floating in the scene.
		 */
		size_t meshIndex{0};
	};

	/**
	 * @brief Format-agnostic result of loading a composite asset.
	 * @note All resources are already registered in engine containers.
	 * The node hierarchy is described via NodeDescriptors without any
	 * dependency on the Scenes/ subsystem.
	 */
	struct EMEN_API SceneData
	{
		/* Resources (already in engine containers). */
		std::vector< MeshDescriptor > meshes;
		std::vector< std::shared_ptr< Animations::SkeletonResource > > skeletons;
		std::vector< std::shared_ptr< Animations::AnimationClipResource > > animationClips;

		/* Scene description carried by the format, referenced by NodeDescriptor indices.
		 * Empty when the format cannot carry them, or when the loader does not read them —
		 * ask Interface::capabilities() to tell the two apart before loading. */
		std::vector< LightDescriptor > lights;
		std::vector< CameraDescriptor > cameras;
		std::vector< InstanceSetDescriptor > instanceSets;

		/* Node hierarchy (format-agnostic). */
		std::vector< NodeDescriptor > nodes;
		std::vector< size_t > rootNodeIndices;
		std::unordered_set< size_t > skinJointNodeIndices;

		/**
		 * @brief Checks if the asset contains exactly one mesh-bearing node.
		 * @note Structural and skeleton joint nodes are ignored.
		 * @return bool
		 */
		[[nodiscard]]
		bool
		isSingleMesh () const noexcept
		{
			size_t count = 0;

			for ( const auto & node : nodes )
			{
				if ( node.meshIndex.has_value() )
				{
					count++;

					if ( count > 1 )
					{
						return false;
					}
				}
			}

			return count == 1;
		}

		/**
		 * @brief Returns the index (into nodes[]) of the single mesh-bearing node.
		 * @warning Only valid when isSingleMesh() returns true.
		 * @return size_t
		 */
		[[nodiscard]]
		size_t
		singleMeshNodeIndex () const noexcept
		{
			for ( size_t i = 0; i < nodes.size(); ++i )
			{
				if ( nodes[i].meshIndex.has_value() )
				{
					return i;
				}
			}

			return 0;
		}
	};
}
