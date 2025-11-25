/*
 * src/Scenes/Scene.hpp
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

#pragma once

/* STL inclusions. */
#include <cstddef>
#include <cstdint>
#include <array>
#include <map>
#include <vector>
#include <set>
#include <string>
#include <any>
#include <memory>
#include <mutex>

/* Local inclusions for inheritances. */
#include "Libs/NameableTrait.hpp"
#include "Libs/ObserverTrait.hpp"
#include "Libs/Time/EventTrait.hpp"

/* Local inclusions for usages. */
#include "Audio/Ambience.hpp"
#include "Libs/Randomizer.hpp"
#include "Graphics/Renderable/AbstractBackground.hpp"
#include "Graphics/Renderable/SceneAreaInterface.hpp"
#include "Graphics/Renderable/SeaLevelInterface.hpp"
#include "Graphics/RenderTarget/ShadowMap.hpp"
#include "Graphics/RenderTarget/Texture.hpp"
#include "Graphics/RenderTarget/View.hpp"
#include "Physics/ConstraintSolver.hpp"
#include "Saphir/EffectInterface.hpp"
#include "Scenes/AVConsole/Manager.hpp"
#include "LightSet.hpp"
#include "OctreeSector.hpp"
#include "StaticEntity.hpp"
#include "Node.hpp"
#include "NodeController.hpp"
#include "RenderBatch.hpp"

/* Local inclusions for usages (unique_ptr members requiring complete types). */
#include "Scenes/Component/Visual.hpp"

/* Forward Declarations */
namespace EmEn
{
	namespace Input
	{
		class Manager;
	}

	namespace Graphics
	{
		class Renderer;
	}

	namespace Scenes::Component
	{
		class AbstractModifier;
	}
}

/* [PHYSICS-NEW-SYSTEM] Toggle between old position-based collision resolution and new impulse-based solver.
 * Set to 1 to enable the Sequential Impulse Solver (ConstraintSolver).
 * Set to 0 to use the legacy position correction system.
 * NOTE: When enabled, the old resolveCollisions() is automatically disabled for dynamic objects. */
#define ENABLE_NEW_PHYSICS_SYSTEM 0

namespace EmEn::Scenes
{
	/**
	 * @brief Configuration options for scene octree initialization.
	 *
	 * Controls the behavior of the dual octree system used for spatial partitioning.
	 * The rendering octree is used for frustum culling, while the physics octree
	 * is used for collision detection broad-phase.
	 *
	 * Default values are tuned for typical game scenes:
	 * - Rendering octree: larger sectors (256 entities) since culling is fast
	 * - Physics octree: smaller sectors (32 entities) for precise collision detection
	 *
	 * @see Scene::Scene() Constructor accepts these options.
	 * @see OctreeSector For the underlying spatial structure.
	 * @version 0.8.35
	 */
	struct SceneOctreeOptions
	{
		/**
		 * @brief Maximum entities per rendering octree sector before subdivision.
		 *
		 * When a sector contains more than this many renderable entities,
		 * it automatically subdivides into 8 child sectors.
		 * Higher values reduce octree depth but increase frustum culling cost per sector.
		 * Default: 256.
		 * @version 0.8.35
		 */
		size_t renderingOctreeAutoExpandAt{256};

		/**
		 * @brief Pre-allocate subdivision levels for rendering octree.
		 *
		 * Setting this > 0 pre-creates the octree hierarchy at initialization,
		 * avoiding runtime allocations when entities are first added.
		 * Default: 0 (no pre-allocation, octree grows on demand).
		 * @version 0.8.35
		 */
		size_t renderingOctreeReserve{0};

		/**
		 * @brief Maximum entities per physics octree sector before subdivision.
		 *
		 * Physics collision detection is O(n²) per sector, so smaller sectors
		 * dramatically improve performance. Keep this value low.
		 * Default: 32.
		 * @version 0.8.35
		 */
		size_t physicsOctreeAutoExpandAt{32};

		/**
		 * @brief Pre-allocate subdivision levels for physics octree.
		 *
		 * Pre-creating physics octree levels improves collision detection
		 * consistency by avoiding runtime rebalancing.
		 * Default: 3 (creates 8³ = 512 potential sectors).
		 * @version 0.8.35
		 */
		size_t physicsOctreeReserve{3};
	};

	/**
	 * @brief Unique non-owner list of render targets for faster access.
	 *
	 * Uses weak_ptr to avoid circular references with render targets.
	 * The owner_less comparator ensures proper ordering for weak_ptr in a set.
	 * Dead weak_ptr entries are cleaned lazily during iteration.
	 *
	 * @see forEachRenderToShadowMap() Iterates shadow map targets.
	 * @see forEachRenderToTexture() Iterates texture render targets.
	 * @see forEachRenderToView() Iterates view render targets.
	 * @version 0.8.35
	 */
	using RenderTargetAccessList = std::set< std::weak_ptr< Graphics::RenderTarget::Abstract >, std::owner_less<> >;

	/**
	 * @brief Unique non-owner list of scene modifiers for faster access.
	 *
	 * Modifiers are components that apply forces or effects to entities
	 * (gravity wells, wind zones, etc.). This list provides O(log n) access
	 * for applying modifiers during physics simulation.
	 *
	 * @see forEachModifiers() Iterates all active modifiers.
	 * @see Component::AbstractModifier For the modifier interface.
	 * @version 0.8.35
	 */
	using ModifierAccessList = std::set< std::weak_ptr< Component::AbstractModifier >, std::owner_less<> >;

	/**
	 * @brief Main container class that manages a complete 3D scene with entities, rendering, physics and audio.
	 *
	 * The Scene class is the central hub that orchestrates all aspects of a 3D environment:
	 *
	 * **Entity Management:**
	 * - Hierarchical Node tree for dynamic objects (parent-child relationships, physics simulation)
	 * - Flat StaticEntity map for optimized static geometry (no physics overhead)
	 * - Both entity types support Components (Visual, Light, Camera, SoundEmitter, etc.)
	 *
	 * **Spatial Optimization:**
	 * - Dual Octree system: one for rendering (frustum culling), one for physics (collision broad-phase)
	 * - Configurable octree subdivision via SceneOctreeOptions
	 * - Automatic entity placement and updates in octrees
	 *
	 * **Rendering Pipeline:**
	 * - Multiple render targets: Views (final output), Textures (RTT), ShadowMaps
	 * - Render batching with Z-sorting for correct transparency
	 * - Separate lists for opaque/translucent and lighted/unlighted objects
	 * - Double-buffered state for thread-safe rendering (logic and render threads)
	 *
	 * **Physics Integration:**
	 * - Collision detection via physics octree broad-phase
	 * - Support for both legacy position-correction and new impulse-based solver (ENABLE_NEW_PHYSICS_SYSTEM)
	 * - Scene boundary clipping (world cube limits)
	 * - Integration with SceneArea for ground collision
	 *
	 * **Audio-Visual Console (AVConsole):**
	 * - Manages Camera-to-VideoDevice and Microphone-to-AudioDevice connections
	 * - Automatic primary device assignment
	 *
	 * **Thread Safety:**
	 * - Mutex protection for octrees, entity lists, and render targets
	 * - Atomic render state index for lock-free state publishing
	 *
	 * @note Uses the Observer pattern to react to Node/StaticEntity/Component changes.
	 * @note [OBS][SHARED-OBSERVER] - Scene observes AVConsoleManager and root Node.
	 *
	 * @extends EmEn::Libs::NameableTrait A scene is a named object in the engine.
	 * @extends EmEn::Libs::Time::EventTrait A scene can have timed events.
	 * @extends EmEn::Libs::ObserverTrait The scene will observe the scene node tree and static entity list.
	 *
	 * @see Node For dynamic hierarchical entities with physics.
	 * @see StaticEntity For optimized static geometry.
	 * @see AVConsole::Manager For audio-video device routing.
	 * @see OctreeSector For spatial partitioning.
	 * @version 0.8.35
	 */
	class Scene final : public Libs::NameableTrait, public Libs::Time::EventTrait< uint32_t, std::milli >, public Libs::ObserverTrait
	{
		public:

			/**
			 * @brief Class identifier for logging and RTTI.
			 * @version 0.8.35
			 */
			static constexpr auto ClassId{"Scene"};

			/**
			 * @brief Constructs a scene with full configuration.
			 *
			 * Creates the scene infrastructure including:
			 * - Root node for the hierarchical entity tree
			 * - AVConsole manager for camera/microphone-to-device routing
			 * - Dual octree system (rendering + physics) based on boundary
			 * - Optional background, terrain (SceneArea), and water (SeaLevel)
			 *
			 * The scene starts in a non-initialized state and must be enabled
			 * via enable() before use. This allows deferred setup of cameras,
			 * microphones, and render targets.
			 *
			 * @param graphicsRenderer Reference to the graphics renderer for GPU resources.
			 * @param audioManager Reference to the audio manager for spatial audio.
			 * @param name Unique scene name (used for AVConsole identification).
			 * @param boundary Half-size of the cubic scene volume in meters.
			 *                 Scene spans from (-boundary, -boundary, -boundary) to
			 *                 (+boundary, +boundary, +boundary).
			 * @param background Optional skybox or procedural background. Default: nullptr.
			 * @param sceneArea Optional terrain/ground for ground collision. Default: nullptr.
			 * @param seaLevel Optional water surface. Default: nullptr.
			 * @param octreeOptions Octree tuning parameters. Default: SceneOctreeOptions{}.
			 *
			 * @note The scene observes the root node and AVConsoleManager for changes.
			 *
			 * @see enable() To activate the scene for rendering.
			 * @see SceneOctreeOptions For octree configuration.
			 * @version 0.8.35
			 */
			Scene (Graphics::Renderer & graphicsRenderer, Audio::Manager & audioManager, const std::string & name, float boundary, const std::shared_ptr< Graphics::Renderable::AbstractBackground > & background = nullptr, const std::shared_ptr< Graphics::Renderable::SceneAreaInterface > & sceneArea = nullptr, const std::shared_ptr< Graphics::Renderable::SeaLevelInterface > & seaLevel = nullptr, const SceneOctreeOptions & octreeOptions = {}) noexcept
				: NameableTrait{name},
				m_rootNode{std::make_shared< Node >(*this)},
				m_backgroundResource{background},
				m_sceneAreaResource{sceneArea},
				m_seaLevelResource{seaLevel},
				m_AVConsoleManager{name, graphicsRenderer, audioManager},
				m_boundary{boundary}
			{
				this->observe(&m_AVConsoleManager);
				this->observe(m_rootNode.get());

				this->buildOctrees(octreeOptions);
			}

			/**
			 * @brief Copy constructor (deleted).
			 *
			 * Scenes contain non-copyable resources (octrees, render targets, mutexes).
			 *
			 * @param copy Unused.
			 * @version 0.8.35
			 */
			Scene (const Scene & copy) noexcept = delete;

			/**
			 * @brief Move constructor (deleted).
			 *
			 * Scenes are observed by their children and cannot be safely moved.
			 *
			 * @param copy Unused.
			 * @version 0.8.35
			 */
			Scene (Scene && copy) noexcept = delete;

			/**
			 * @brief Copy assignment (deleted).
			 * @param copy Unused.
			 * @return Scene &
			 * @version 0.8.35
			 */
			Scene & operator= (const Scene & copy) noexcept = delete;

			/**
			 * @brief Move assignment (deleted).
			 * @param copy Unused.
			 * @return Scene &
			 * @version 0.8.35
			 */
			Scene & operator= (Scene && copy) noexcept = delete;

			/**
			 * @brief Destructs the scene and releases all resources.
			 *
			 * Cleanup order:
			 * 1. Scene setup data (effects, physics solver)
			 * 2. Octrees and fast-access structures
			 * 3. Managers (AVConsole, LightSet)
			 * 4. Content (background, sceneArea, seaLevel)
			 * 5. Entities (static entities, node tree)
			 *
			 * @warning All references to scene entities become invalid after destruction.
			 * @version 0.8.35
			 */
			~Scene () override;

			/**
			 * @brief Enables the scene and prepares it for rendering.
			 *
			 * Performs first-time initialization if not already done:
			 * 1. Registers visual components (background, sceneArea, seaLevel)
			 * 2. Creates default camera/microphone if missing
			 * 3. Connects SwapChain as primary video output
			 * 4. Auto-connects primary camera to video device
			 * 5. Initializes LightSet for shadow mapping
			 * 6. Sets up audio output (speaker) if audio is available
			 *
			 * Also registers the NodeController with the input manager for
			 * debug camera controls.
			 *
			 * @param inputManager Reference to register keyboard listeners.
			 * @param settings Core settings (currently unused, reserved for future).
			 * @return True if initialization succeeded, false on critical failure.
			 *
			 * @warning Returns false if camera or microphone creation fails.
			 *
			 * @see disable() To deactivate the scene.
			 * @version 0.8.35
			 */
			[[nodiscard]]
			bool enable (Input::Manager & inputManager, Settings & settings) noexcept;

			/**
			 * @brief Disables the scene and disconnects input handlers.
			 *
			 * Releases the NodeController from input management and stops
			 * any active debug controls. The scene remains initialized and
			 * can be re-enabled later.
			 *
			 * @param inputManager Reference to unregister keyboard listeners.
			 *
			 * @see enable() To reactivate the scene.
			 * @version 0.8.35
			 */
			void disable (Input::Manager & inputManager) noexcept;

			/**
			 * @brief Sets the scene boundary and rebuilds octrees.
			 *
			 * Changes the half-size of the cubic scene volume. Both rendering
			 * and physics octrees are rebuilt to accommodate the new size,
			 * preserving all existing entities.
			 *
			 * @param boundary New half-size in meters (absolute value used).
			 *
			 * @note This is an expensive operation - avoid calling frequently.
			 *
			 * @see boundary() To query current value.
			 * @see size() To get full scene width.
			 * @version 0.8.35
			 */
			void setBoundary (float boundary) noexcept
			{
				m_boundary = std::abs(boundary);

				this->rebuildRenderingOctree(true);

				this->rebuildPhysicsOctree(true);
			}

			/**
			 * @brief Returns the boundary (half-size) of the scene in one direction.
			 *
			 * The scene spans from -boundary to +boundary on each axis.
			 *
			 * @return Half-size of the scene cube in meters.
			 *
			 * @note To get the total size, multiply by 2 or use size().
			 *
			 * @see size() For full scene width.
			 * @see setBoundary() To modify.
			 * @version 0.8.35
			 */
			[[nodiscard]]
			float
			boundary () const noexcept
			{
				return m_boundary;
			}

			/**
			 * @brief Returns the full width of the scene cube.
			 *
			 * Equivalent to boundary() * 2.
			 *
			 * @return Total scene width in meters (same on all axes).
			 *
			 * @see boundary() For half-size.
			 * @version 0.8.35
			 */
			[[nodiscard]]
			float
			size () const noexcept
			{
				return m_boundary * 2;
			}

			/**
			 * @brief Returns the scene's random number generator.
			 *
			 * Provides deterministic random values for scene-specific logic.
			 * Use for procedural generation, particle systems, etc.
			 *
			 * @return Reference to the scene's float randomizer.
			 * @version 0.8.35
			 */
			[[nodiscard]]
			Libs::Randomizer< float > &
			randomizer () noexcept
			{
				return m_randomizer;
			}

			/**
			 * @brief Returns the scene execution time in microseconds.
			 *
			 * Accumulated time since scene initialization, updated each
			 * processLogics() call.
			 *
			 * @return Scene lifetime in microseconds (10⁻⁶ seconds).
			 *
			 * @see lifetimeMS() For milliseconds.
			 * @see cycle() For frame count.
			 * @version 0.8.35
			 */
			[[nodiscard]]
			uint64_t
			lifetimeUS () const noexcept
			{
				return m_lifetimeUS;
			}

			/**
			 * @brief Returns the scene execution time in milliseconds.
			 *
			 * Accumulated time since scene initialization, updated each
			 * processLogics() call.
			 *
			 * @return Scene lifetime in milliseconds (10⁻³ seconds).
			 *
			 * @see lifetimeUS() For higher precision.
			 * @see cycle() For frame count.
			 * @version 0.8.35
			 */
			[[nodiscard]]
			uint32_t
			lifetimeMS () const noexcept
			{
				return m_lifetimeMS;
			}

			/**
			 * @brief Returns the number of logic cycles executed.
			 *
			 * Incremented each processLogics() call. Useful for frame-based
			 * animations and time-dependent logic.
			 *
			 * @return Number of completed logic frames.
			 *
			 * @see lifetimeMS() For time-based queries.
			 * @version 0.8.35
			 */
			[[nodiscard]]
			size_t
			cycle () const noexcept
			{
				return m_cycle;
			}

			/**
			 * @brief Sets the scene's physical environment properties.
			 *
			 * Configures gravity, air density, and other physical constants
			 * that affect all entities in the scene.
			 *
			 * @param properties New environment properties (e.g., Earth, Moon, custom).
			 *
			 * @see physicalEnvironmentProperties() To query current values.
			 * @see Physics::EnvironmentPhysicalProperties::Earth() For Earth defaults.
			 * @version 0.8.35
			 */
			void
			setEnvironmentPhysicalProperties (const Physics::EnvironmentPhysicalProperties & properties) noexcept
			{
				m_environmentPhysicalProperties = properties;
			}

			/**
			 * @brief Creates a 2D shadow map render target for directional/spot lights.
			 *
			 * Shadow maps are depth-only textures rendered from a light's perspective.
			 * Use for directional lights (orthographic) or spot lights (perspective).
			 *
			 * @param name Unique name for the virtual video device (must not exist).
			 * @param resolution Shadow map size in pixels (width = height).
			 * @param viewDistance Maximum shadow casting distance in meters.
			 * @param isOrthographicProjection True for directional lights, false for spot lights.
			 * @return Shared pointer to the shadow map, or nullptr on failure.
			 *
			 * @todo Get the view distance value from settings.
			 *
			 * @see createRenderToCubicShadowMap() For point lights.
			 * @see LightSet For shadow map usage.
			 * @version 0.8.35
			 */
			[[nodiscard]]
			std::shared_ptr< Graphics::RenderTarget::ShadowMap< Graphics::ViewMatrices2DUBO > > createRenderToShadowMap (const std::string & name, uint32_t resolution, float viewDistance, bool isOrthographicProjection) noexcept;

			/**
			 * @brief Creates a cubic shadow map render target for point lights.
			 *
			 * Cubic shadow maps capture depth in all 6 directions, enabling
			 * omnidirectional shadow casting from point lights.
			 *
			 * @param name Unique name for the virtual video device (must not exist).
			 * @param resolution Cubemap face size in pixels.
			 * @param viewDistance Maximum shadow casting distance in meters.
			 * @param isOrthographicProjection Usually false for point lights.
			 * @return Shared pointer to the cubic shadow map, or nullptr on failure.
			 *
			 * @todo Get the view distance value from settings.
			 *
			 * @see createRenderToShadowMap() For directional/spot lights.
			 * @version 0.8.35
			 */
			[[nodiscard]]
			std::shared_ptr< Graphics::RenderTarget::ShadowMap< Graphics::ViewMatrices3DUBO > > createRenderToCubicShadowMap (const std::string & name, uint32_t resolution, float viewDistance, bool isOrthographicProjection) noexcept;

			/**
			 * @brief Creates a 2D render-to-texture target for off-screen rendering.
			 *
			 * Useful for mirrors, security cameras, portals, minimaps, etc.
			 * The texture can be sampled in shaders after rendering.
			 *
			 * @param name Unique name for the virtual video device (must not exist).
			 * @param width Texture width in pixels.
			 * @param height Texture height in pixels.
			 * @param colorCount Number of color attachments (1-4).
			 * @param viewDistance Maximum render distance in meters.
			 * @param isOrthographicProjection True for 2D/UI, false for 3D perspective.
			 * @return Shared pointer to the texture target, or nullptr on failure.
			 *
			 * @todo Get the view distance value from settings.
			 *
			 * @see createRenderToCubemap() For environment mapping.
			 * @version 0.8.35
			 */
			[[nodiscard]]
			std::shared_ptr< Graphics::RenderTarget::Texture< Graphics::ViewMatrices2DUBO > > createRenderToTexture2D (const std::string & name, uint32_t width, uint32_t height, uint32_t colorCount, float viewDistance, bool isOrthographicProjection) noexcept;

			/**
			 * @brief Creates a cubemap render-to-texture target for environment mapping.
			 *
			 * Renders the scene in all 6 directions for dynamic reflections,
			 * skybox generation, or environment probes.
			 *
			 * @param name Unique name for the virtual video device (must not exist).
			 * @param size Cubemap face size in pixels.
			 * @param colorCount Number of color attachments (1-4).
			 * @param viewDistance Maximum render distance in meters.
			 * @param isOrthographicProjection Usually false for cubemaps.
			 * @return Shared pointer to the cubemap target, or nullptr on failure.
			 *
			 * @todo Get the view distance value from settings.
			 *
			 * @see createRenderToTexture2D() For planar reflections.
			 * @version 0.8.35
			 */
			[[nodiscard]]
			std::shared_ptr< Graphics::RenderTarget::Texture< Graphics::ViewMatrices3DUBO > > createRenderToCubemap (const std::string & name, uint32_t size, uint32_t colorCount, float viewDistance, bool isOrthographicProjection) noexcept;

			/**
			 * @brief Creates a 2D view render target for final output.
			 *
			 * Views are high-quality render targets suitable for final display
			 * or post-processing input. Support configurable framebuffer precision.
			 *
			 * @param name Unique name for the virtual video device (must not exist).
			 * @param width View width in pixels.
			 * @param height View height in pixels.
			 * @param precisions Framebuffer bit depths (color, depth, stencil).
			 * @param viewDistance Maximum render distance in meters.
			 * @param isOrthographicProjection True for 2D/UI, false for 3D.
			 * @param primaryDevice True to set as the main output (receives camera feed).
			 * @return Shared pointer to the view target, or nullptr on failure.
			 *
			 * @see createRenderToCubicView() For VR/panoramic rendering.
			 * @version 0.8.35
			 */
			[[nodiscard]]
			std::shared_ptr< Graphics::RenderTarget::View< Graphics::ViewMatrices2DUBO > > createRenderToView (const std::string & name, uint32_t width, uint32_t height, const Graphics::FramebufferPrecisions & precisions, float viewDistance, bool isOrthographicProjection, bool primaryDevice = false) noexcept;

			/**
			 * @brief Creates a cubic view render target for VR or panoramic rendering.
			 *
			 * Renders to all 6 cubemap faces for omnidirectional viewing.
			 * Useful for VR environments or 360° captures.
			 *
			 * @param name Unique name for the virtual video device (must not exist).
			 * @param size Cubemap face size in pixels.
			 * @param precisions Framebuffer bit depths (color, depth, stencil).
			 * @param viewDistance Maximum render distance in meters.
			 * @param isOrthographicProjection Usually false for cubic views.
			 * @param primaryDevice True to set as the main output.
			 * @return Shared pointer to the cubic view target, or nullptr on failure.
			 *
			 * @see createRenderToView() For standard 2D views.
			 * @version 0.8.35
			 */
			[[nodiscard]]
			std::shared_ptr< Graphics::RenderTarget::View< Graphics::ViewMatrices3DUBO > > createRenderToCubicView (const std::string & name, uint32_t size, const Graphics::FramebufferPrecisions & precisions, float viewDistance, bool isOrthographicProjection, bool primaryDevice = false) noexcept;

			/**
			 * @brief Creates a static entity in the scene.
			 *
			 * StaticEntities are optimized for non-moving geometry:
			 * - Stored in a flat map (O(1) lookup by name)
			 * - No parent-child hierarchy overhead
			 * - No physics simulation (can still be collision deflectors)
			 * - World coordinates only (no local transforms)
			 *
			 * Use for buildings, terrain features, static props, etc.
			 * For moving objects, use Node via root()->createChild() instead.
			 *
			 * @note The entity is automatically observed by the Scene for component changes.
			 *
			 * @param name Unique identifier for the entity.
			 * @param coordinates Initial world position and orientation. Default: origin.
			 * @return Shared pointer to the new StaticEntity, or nullptr on failure.
			 *
			 * @see Node For dynamic hierarchical entities.
			 * @see removeStaticEntity() To remove a static entity.
			 * @version 0.8.35
			 */
			[[nodiscard]]
			std::shared_ptr< StaticEntity > createStaticEntity (const std::string & name, const Libs::Math::CartesianFrame< float > & coordinates = {}) noexcept;

			/**
			 * @brief Creates a static entity at a specific position with default orientation.
			 *
			 * Convenience overload that creates a CartesianFrame from position only.
			 *
			 * @param name Unique identifier for the entity.
			 * @param position World position for the entity.
			 * @return Shared pointer to the new StaticEntity, or nullptr on failure.
			 *
			 * @see createStaticEntity(const std::string&, const CartesianFrame&) For full transform.
			 * @version 0.8.35
			 */
			[[nodiscard]]
			std::shared_ptr< StaticEntity >
			createStaticEntity (const std::string & name, const Libs::Math::Vector< 3, float > & position) noexcept
			{
				return this->createStaticEntity(name, Libs::Math::CartesianFrame< float >{position});
			}

			/**
			 * @brief Removes a static entity from the scene by name.
			 *
			 * Performs cleanup:
			 * 1. Stops observing the entity
			 * 2. Removes from rendering octree
			 * 3. Removes from physics octree
			 * 4. Clears entity components
			 * 5. Erases from entity map
			 *
			 * @param name Name of the entity to remove.
			 * @return True if entity was found and removed, false if not found.
			 *
			 * @see createStaticEntity() To add entities.
			 * @see findStaticEntity() To check existence.
			 * @version 0.8.35
			 */
			bool removeStaticEntity (const std::string & name) noexcept;

			/**
			 * @brief Adds a global effect to the scene.
			 *
			 * Environment effects are applied scene-wide during rendering
			 * (fog, color grading, post-processing, etc.).
			 * Duplicate effects are silently ignored.
			 *
			 * @param effect Shared pointer to the effect interface.
			 *
			 * @bug Should use std::set instead of vector for O(1) lookup.
			 *
			 * @see isEnvironmentEffectPresent() To check before adding.
			 * @see clearEnvironmentEffects() To remove all effects.
			 * @version 0.8.35
			 */
			void
			addEnvironmentEffect (const std::shared_ptr< Saphir::EffectInterface > & effect) noexcept
			{
				/* We don't want to notify an effect twice. */
				// FIXME: Use a std::set so!
				if ( m_environmentEffects.contains(effect) )
				{
					return;
				}

				m_environmentEffects.emplace(effect);
			}

			/**
			 * @brief Checks if a global effect is already applied to the scene.
			 *
			 * @param effect Shared pointer to the effect to check.
			 * @return True if the effect is present, false otherwise.
			 *
			 * @see addEnvironmentEffect() To add new effects.
			 * @version 0.8.35
			 */
			[[nodiscard]]
			bool
			isEnvironmentEffectPresent (const std::shared_ptr< Saphir::EffectInterface > & effect) const noexcept
			{
				return m_environmentEffects.contains(effect);
			}

			/**
			 * @brief Removes all environment effects from the scene.
			 *
			 * @see addEnvironmentEffect() To add new effects.
			 * @see environmentEffects() To query current effects.
			 * @version 0.8.35
			 */
			void
			clearEnvironmentEffects () noexcept
			{
				m_environmentEffects.clear();
			}

			/**
			 * @brief Rebuilds the rendering octree with new boundary.
			 *
			 * Creates a new octree with the current boundary size and optionally
			 * transfers all existing entities to the new structure.
			 *
			 * @param keepElements True to preserve entities, false to clear.
			 * @return True on success, false if boundary is invalid (<=0).
			 *
			 * @note Thread-safe via mutex protection.
			 *
			 * @see rebuildPhysicsOctree() For physics octree.
			 * @see setBoundary() Triggers automatic rebuild.
			 * @version 0.8.35
			 */
			bool rebuildRenderingOctree (bool keepElements = true) noexcept;

			/**
			 * @brief Rebuilds the physics octree with new boundary.
			 *
			 * Creates a new octree with the current boundary size and optionally
			 * transfers all existing entities to the new structure.
			 *
			 * @param keepElements True to preserve entities, false to clear.
			 * @return True on success, false if boundary is invalid (<=0).
			 *
			 * @note Thread-safe via mutex protection.
			 *
			 * @see rebuildRenderingOctree() For rendering octree.
			 * @see setBoundary() Triggers automatic rebuild.
			 * @version 0.8.35
			 */
			bool rebuildPhysicsOctree (bool keepElements = true) noexcept;

			/**
			 * @brief Removes all child nodes from the root node.
			 *
			 * Destroys the entire node hierarchy except the root.
			 * Static entities are not affected.
			 *
			 * @note Thread-safe via mutex protection.
			 *
			 * @see root() To access the node tree.
			 * @version 0.8.35
			 */
			void resetNodeTree () const noexcept;

			/**
			 * @brief Main logic update called every frame by the Core engine.
			 *
			 * Performs the complete scene simulation step:
			 * 1. Updates scene lifetime counters (microseconds and milliseconds)
			 * 2. Updates NodeController (debug camera controls)
			 * 3. Processes StaticEntity logic (components, animations)
			 * 4. Processes Node tree logic via NodeCrawler traversal
			 * 5. Updates entity positions in octrees
			 * 6. Runs physics collision detection via octree broad-phase
			 * 7. Handles scene boundary clipping
			 * 8. Resolves collisions (position-based or impulse-based depending on ENABLE_NEW_PHYSICS_SYSTEM)
			 * 9. Cleans dead nodes from the tree
			 *
			 * @note This method is thread-safe and uses mutex protection for entity lists.
			 * @note Call publishStateForRendering() after this to make changes visible to the render thread.
			 *
			 * @param engineCycle The cycle number of the engine (for time-based logic).
			 * @version 0.8.35
			 */
			void processLogics (size_t engineCycle) noexcept;

			/**
			 * @brief Publishes the current simulation state for the render thread.
			 *
			 * Implements double-buffering for thread-safe rendering:
			 * - Copies StaticEntity states to the next buffer index
			 * - Copies Node tree states via NodeCrawler traversal
			 * - Copies render target view matrices
			 * - Atomically swaps the render state index
			 *
			 * This allows the logic thread to continue simulation while the render
			 * thread reads a consistent, stable snapshot of the scene.
			 *
			 * @note Must be called at the end of each logic frame, after processLogics().
			 * @note Uses atomic operations for the state index swap (lock-free).
			 * @version 0.8.35
			 */
			void publishStateForRendering () noexcept;

			/**
			 * @brief Updates GPU resources immediately before rendering.
			 *
			 * Synchronizes CPU-side data with GPU memory for:
			 * - Shadow maps (if enabled)
			 * - Render-to-texture targets (if enabled)
			 * - View matrices and uniforms
			 *
			 * @param shadowMapEnabled True to update shadow map resources.
			 * @param renderToTextureEnabled True to update texture render targets.
			 *
			 * @note Called by the Core engine between processLogics() and render().
			 *
			 * @see processLogics() For simulation updates.
			 * @see publishStateForRendering() For state synchronization.
			 * @version 0.8.35
			 */
			void updateVideoMemory (bool shadowMapEnabled, bool renderToTextureEnabled) const noexcept;

			/**
			 * @brief Performs the shadow map rendering pass.
			 *
			 * Renders the scene from a light's point of view to generate shadow maps.
			 * Only processes shadow-casting objects (sorted by distance).
			 *
			 * @note Skipped if LightSet is disabled.
			 * @note Uses the double-buffered render state (read-only access).
			 *
			 * @param renderTarget The shadow map render target (2D or cubemap).
			 * @param commandBuffer The Vulkan command buffer for recording draw calls.
			 * @version 0.8.35
			 */
			void castShadows (const std::shared_ptr< Graphics::RenderTarget::Abstract > & renderTarget, const Vulkan::CommandBuffer & commandBuffer) noexcept;

			/**
			 * @brief Main scene rendering pass.
			 *
			 * Renders all visible objects to the specified render target:
			 * 1. Populates render lists via frustum culling and Z-sorting
			 * 2. Renders opaque objects (front-to-back for early-Z optimization)
			 * 3. Renders opaque lighted objects with per-light passes
			 * 4. Renders translucent objects (back-to-front for correct blending)
			 * 5. Renders translucent lighted objects
			 *
			 * @note Uses the double-buffered render state (read-only access).
			 * @note Render lists are cleared and rebuilt each frame.
			 *
			 * @param renderTarget The destination (View, Texture, or SwapChain).
			 * @param commandBuffer The Vulkan command buffer for recording draw calls.
			 * @version 0.8.35
			 */
			void render (const std::shared_ptr< Graphics::RenderTarget::Abstract > & renderTarget, const Vulkan::CommandBuffer & commandBuffer) noexcept;

			/**
			 * @brief Returns the audio-video console manager (const).
			 *
			 * The AVConsole routes cameras to video devices and microphones
			 * to audio devices.
			 *
			 * @return Const reference to the AVConsole manager.
			 *
			 * @see AVConsole::Manager For device routing API.
			 * @version 0.8.35
			 */
			[[nodiscard]]
			const AVConsole::Manager &
			AVConsoleManager () const noexcept
			{
				return m_AVConsoleManager;
			}

			/**
			 * @brief Returns the audio-video console manager (mutable).
			 *
			 * Use to add/remove video/audio devices and configure routing.
			 *
			 * @return Mutable reference to the AVConsole manager.
			 *
			 * @see AVConsole::Manager For device routing API.
			 * @version 0.8.35
			 */
			[[nodiscard]]
			AVConsole::Manager &
			AVConsoleManager () noexcept
			{
				return m_AVConsoleManager;
			}

			/**
			 * @brief Returns the engine context (const).
			 *
			 * Provides access to core engine services: Graphics::Renderer
			 * and Audio::Manager.
			 *
			 * @return Const reference to the engine context.
			 * @version 0.8.35
			 */
			[[nodiscard]]
			const EngineContext &
			engineContext () const noexcept
			{
				return m_AVConsoleManager.engineContext();
			}

			/**
			 * @brief Returns the engine context (mutable).
			 *
			 * Provides access to core engine services for resource creation
			 * and configuration.
			 *
			 * @return Mutable reference to the engine context.
			 * @version 0.8.35
			 */
			[[nodiscard]]
			EngineContext &
			engineContext () noexcept
			{
				return m_AVConsoleManager.engineContext();
			}

			/**
			 * @brief Returns the scene's light management system (const).
			 *
			 * LightSet manages all lights in the scene, including ambient light,
			 * shadow-casting lights, and their GPU resources.
			 *
			 * @return Const reference to the light set.
			 *
			 * @see LightSet For light management API.
			 * @version 0.8.35
			 */
			[[nodiscard]]
			const LightSet &
			lightSet () const noexcept
			{
				return m_lightSet;
			}

			/**
			 * @brief Returns the scene's light management system (mutable).
			 *
			 * Use to add/remove lights, configure ambient lighting, and
			 * enable/disable shadow casting.
			 *
			 * @return Mutable reference to the light set.
			 *
			 * @see LightSet For light management API.
			 * @version 0.8.35
			 */
			[[nodiscard]]
			LightSet &
			lightSet () noexcept
			{
				return m_lightSet;
			}

			/**
			 * @brief Returns the debug node controller (const).
			 *
			 * NodeController provides keyboard-driven camera control for
			 * debugging and scene exploration.
			 *
			 * @return Const reference to the node controller.
			 *
			 * @note This is a debug utility.
			 *
			 * @see NodeController For control API.
			 * @version 0.8.35
			 */
			[[nodiscard]]
			const NodeController &
			nodeController () const noexcept
			{
				return m_nodeController;
			}

			/**
			 * @brief Returns the debug node controller (mutable).
			 *
			 * Use to attach/detach nodes and configure control parameters.
			 *
			 * @return Mutable reference to the node controller.
			 *
			 * @note This is a debug utility.
			 * @bug Should not be a persistent instance here.
			 *
			 * @see NodeController For control API.
			 * @version 0.8.35
			 */
			[[nodiscard]]
			NodeController & nodeController () noexcept
			{
				return m_nodeController;
			}

			/**
			 * @brief Returns the physical environment properties (const).
			 *
			 * Environment properties include gravity, air density, and other
			 * constants that affect physics simulation.
			 *
			 * @return Const reference to environment properties.
			 *
			 * @see setEnvironmentPhysicalProperties() To modify.
			 * @see Physics::EnvironmentPhysicalProperties For property details.
			 * @version 0.8.35
			 */
			[[nodiscard]]
			const Physics::EnvironmentPhysicalProperties &
			physicalEnvironmentProperties () const noexcept
			{
				return m_environmentPhysicalProperties;
			}

			/**
			 * @brief Returns the physical environment properties (mutable).
			 *
			 * Allows direct modification of individual properties.
			 *
			 * @return Mutable reference to environment properties.
			 *
			 * @see setEnvironmentPhysicalProperties() For bulk assignment.
			 * @version 0.8.35
			 */
			[[nodiscard]]
			Physics::EnvironmentPhysicalProperties &
			physicalEnvironmentProperties () noexcept
			{
				return m_environmentPhysicalProperties;
			}

			/**
			 * @brief Executes a function with thread-safe access to all shadow maps.
			 *
			 * Holds the shadow map mutex while executing the callback.
			 * Use for batch operations on all shadow maps.
			 *
			 * @tparam function_t Callable with signature: void(const RenderTargetAccessList&)
			 * @param processRenderTargets Callback receiving the shadow map list.
			 *
			 * @see forEachRenderToShadowMap() For per-target iteration.
			 * @version 0.8.35
			 */
			template< typename function_t >
			void
			withRenderToShadowMaps (function_t && processRenderTargets) const noexcept requires (std::is_invocable_v< function_t, const RenderTargetAccessList & >)
			{
				const std::lock_guard< std::mutex > lock{m_renderToShadowMapAccess};

				processRenderTargets(m_renderToShadowMaps);
			}

			/**
			 * @brief Executes a function with thread-safe access to all texture targets.
			 *
			 * Holds the texture target mutex while executing the callback.
			 * Use for batch operations on all render-to-texture targets.
			 *
			 * @tparam function_t Callable with signature: void(const RenderTargetAccessList&)
			 * @param processRenderTargets Callback receiving the texture target list.
			 *
			 * @see forEachRenderToTexture() For per-target iteration.
			 * @version 0.8.35
			 */
			template< typename function_t >
			void
			withRenderToTextures (function_t && processRenderTargets) const noexcept requires (std::is_invocable_v< function_t, const RenderTargetAccessList & >)
			{
				const std::lock_guard< std::mutex > lock{m_renderToTextureAccess};

				processRenderTargets(m_renderToTextures);
			}

			/**
			 * @brief Executes a function with thread-safe access to all view targets.
			 *
			 * Holds the view target mutex while executing the callback.
			 * Use for batch operations on all render-to-view targets.
			 *
			 * @tparam function_t Callable with signature: void(const RenderTargetAccessList&)
			 * @param processRenderTargets Callback receiving the view target list.
			 *
			 * @see forEachRenderToView() For per-target iteration.
			 * @version 0.8.35
			 */
			template< typename function_t >
			void
			withRenderToViews (function_t && processRenderTargets) const noexcept requires (std::is_invocable_v< function_t, const RenderTargetAccessList & >)
			{
				const std::lock_guard< std::mutex > lock{m_renderToViewAccess};

				processRenderTargets(m_renderToViews);
			}

			/**
			 * @brief Iterates all shadow maps with thread-safe per-target callback.
			 *
			 * Automatically locks the weak_ptr and skips expired targets
			 * (with debug warning).
			 *
			 * @tparam function_t Callable with signature: void(const shared_ptr<RenderTarget::Abstract>&)
			 * @param processRenderTarget Callback receiving each valid shadow map.
			 *
			 * @see withRenderToShadowMaps() For batch access.
			 * @version 0.8.35
			 */
			template< typename function_t >
			void
			forEachRenderToShadowMap (function_t && processRenderTarget) const noexcept requires (std::is_invocable_v< function_t, const std::shared_ptr< Graphics::RenderTarget::Abstract > & >)
			{
				const std::lock_guard< std::mutex > lock{m_renderToShadowMapAccess};

				for ( const auto & renderTargetWeak : m_renderToShadowMaps )
				{
					if ( const auto renderTarget = renderTargetWeak.lock() )
					{
						processRenderTarget(renderTarget);
					}
					else
					{
						Tracer::debug(ClassId, "Dead RenderTarget in the scene shadow map list!");
					}
				}
			}

			/**
			 * @brief Iterates all texture targets with thread-safe per-target callback.
			 *
			 * Automatically locks the weak_ptr and skips expired targets
			 * (with debug warning).
			 *
			 * @tparam function_t Callable with signature: void(const shared_ptr<RenderTarget::Abstract>&)
			 * @param processRenderTarget Callback receiving each valid texture target.
			 *
			 * @todo Pass the render target as a reference instead of shared_ptr.
			 *
			 * @see withRenderToTextures() For batch access.
			 * @version 0.8.35
			 */
			template< typename function_t >
			void
			forEachRenderToTexture (function_t && processRenderTarget) const noexcept requires (std::is_invocable_v< function_t, const std::shared_ptr< Graphics::RenderTarget::Abstract > & >)
			{
				const std::lock_guard< std::mutex > lock{m_renderToTextureAccess};

				for ( const auto & renderTargetWeak : m_renderToTextures )
				{
					if ( const auto renderTarget = renderTargetWeak.lock() )
					{
						processRenderTarget(renderTarget);
					}
					else
					{
						Tracer::debug(ClassId, "Dead RenderTarget in the scene texture list!");
					}
				}
			}

			/**
			 * @brief Iterates all view targets with thread-safe per-target callback.
			 *
			 * Automatically locks the weak_ptr and skips expired targets
			 * (with debug warning).
			 *
			 * @tparam function_t Callable with signature: void(const shared_ptr<RenderTarget::Abstract>&)
			 * @param processRenderTarget Callback receiving each valid view target.
			 *
			 * @todo Pass the render target as a reference instead of shared_ptr.
			 *
			 * @see withRenderToViews() For batch access.
			 * @version 0.8.35
			 */
			template< typename function_t >
			void
			forEachRenderToView (function_t && processRenderTarget) const noexcept requires (std::is_invocable_v< function_t, const std::shared_ptr< Graphics::RenderTarget::Abstract > & >)
			{
				const std::lock_guard< std::mutex > lock{m_renderToViewAccess};

				for ( const auto & renderTargetWeak : m_renderToViews )
				{
					if ( const auto renderTarget = renderTargetWeak.lock() )
					{
						processRenderTarget(renderTarget);
					}
					else
					{
						Tracer::debug(ClassId, "Dead RenderTarget in the scene view list!");
					}
				}
			}

			/**
			 * @brief Sets the scene background (skybox, procedural sky, etc.).
			 *
			 * The background is rendered behind all other scene content.
			 * Changes take effect immediately after registerSceneVisualComponents().
			 *
			 * @param background Shared pointer to the background, or nullptr to clear.
			 *
			 * @todo Pass the render target as a reference instead of shared_ptr.
			 *
			 * @see background() To query current background.
			 * @see Graphics::Renderable::AbstractBackground For background interface.
			 * @version 0.8.35
			 */
			void
			setBackground (const std::shared_ptr< Graphics::Renderable::AbstractBackground > & background) noexcept
			{
				m_backgroundResource = background;

				this->registerSceneVisualComponents();
			}

			/**
			 * @brief Returns the current scene background.
			 *
			 * @return Shared pointer to the background, or nullptr if none set.
			 *
			 * @see setBackground() To modify.
			 * @version 0.8.35
			 */
			[[nodiscard]]
			std::shared_ptr< Graphics::Renderable::AbstractBackground >
			background () const noexcept
			{
				return m_backgroundResource;
			}

			/**
			 * @brief Sets the scene terrain (ground/landscape).
			 *
			 * SceneArea provides ground collision and height queries for
			 * entities and physics simulation.
			 *
			 * @param sceneArea Shared pointer to the terrain, or nullptr to clear.
			 *
			 * @see sceneArea() To query current terrain.
			 * @see Graphics::Renderable::SceneAreaInterface For terrain interface.
			 * @version 0.8.35
			 */
			void
			setSceneArea (const std::shared_ptr< Graphics::Renderable::SceneAreaInterface > & sceneArea) noexcept
			{
				m_sceneAreaResource = sceneArea;

				this->registerSceneVisualComponents();
			}

			/**
			 * @brief Returns the current scene terrain.
			 *
			 * @return Shared pointer to the terrain, or nullptr if none set.
			 *
			 * @see setSceneArea() To modify.
			 * @version 0.8.35
			 */
			[[nodiscard]]
			std::shared_ptr< Graphics::Renderable::SceneAreaInterface >
			sceneArea () const noexcept
			{
				return m_sceneAreaResource;
			}

			/**
			 * @brief Sets the scene water level surface.
			 *
			 * SeaLevel provides water plane rendering with reflections
			 * and refraction effects.
			 *
			 * @param seaLevel Shared pointer to the water surface, or nullptr to clear.
			 *
			 * @see seaLevel() To query current water level.
			 * @see Graphics::Renderable::SeaLevelInterface For water interface.
			 * @version 0.8.35
			 */
			void
			setSeaLevel (const std::shared_ptr< Graphics::Renderable::SeaLevelInterface > & seaLevel) noexcept
			{
				m_seaLevelResource = seaLevel;

				this->registerSceneVisualComponents();
			}

			/**
			 * @brief Returns the current water level surface.
			 *
			 * @return Shared pointer to the water surface, or nullptr if none set.
			 *
			 * @see setSeaLevel() To modify.
			 * @version 0.8.35
			 */
			[[nodiscard]]
			std::shared_ptr< Graphics::Renderable::SeaLevelInterface >
			seaLevel () const noexcept
			{
				return m_seaLevelResource;
			}

			/**
			 * @brief Returns the root node of the scene hierarchy.
			 *
			 * All dynamic entities (Nodes) are children of this root.
			 * Use root()->createChild() to add new entities.
			 *
			 * @return Shared pointer to the root node (never null).
			 *
			 * @see Node For entity manipulation.
			 * @see findNode() To search by name.
			 * @version 0.8.35
			 */
			[[nodiscard]]
			std::shared_ptr< Node >
			root () const noexcept
			{
				return m_rootNode;
			}

			/**
			 * @brief Searches for a node by name in the hierarchy.
			 *
			 * Performs depth-first search starting from root.
			 * Returns the first match found.
			 *
			 * @param nodeName Name to search for (case-sensitive).
			 * @return Shared pointer to the node, or nullptr if not found.
			 *
			 * @see root() To traverse manually.
			 * @version 0.8.35
			 */
			[[nodiscard]]
			std::shared_ptr< Node > findNode (const std::string & nodeName) const noexcept;

			/**
			 * @brief Iterates all static entities with thread-safe callback.
			 *
			 * Holds the static entity mutex during iteration.
			 *
			 * @tparam function_t Callable with signature: void(const StaticEntity&)
			 * @param processStaticEntity Callback receiving each entity by reference.
			 *
			 * @see findStaticEntity() For lookup by name.
			 * @version 0.8.35
			 */
			template< typename function_t >
			void
			forEachStaticEntities (function_t && processStaticEntity) const noexcept requires (std::is_invocable_v< function_t, const StaticEntity & >)
			{
				const std::lock_guard< std::mutex > lock{m_staticEntitiesAccess};

				for ( const auto & entity : m_staticEntities | std::views::values )
				{
					processStaticEntity(*entity);
				}
			}

			/**
			 * @brief Finds a static entity by name.
			 *
			 * O(log n) lookup in the entity map.
			 *
			 * @param staticEntityName Name to search for (case-sensitive).
			 * @return Shared pointer to the entity, or nullptr if not found.
			 *
			 * @note Always check for nullptr return!
			 *
			 * @see createStaticEntity() To add new entities.
			 * @see forEachStaticEntities() For iteration.
			 * @version 0.8.35
			 */
			[[nodiscard]]
			std::shared_ptr< StaticEntity >
			findStaticEntity (const std::string & staticEntityName) const noexcept
			{
				const auto staticEntityIt = m_staticEntities.find(staticEntityName);

				if ( staticEntityIt == m_staticEntities.end() )
				{
					return nullptr;
				}

				return staticEntityIt->second;
			}

			/**
			 * @brief Iterates all scene modifiers (force fields, wind zones, etc.).
			 *
			 * Automatically locks weak_ptr and skips expired modifiers.
			 *
			 * @tparam function_t Callable with signature: void(const AbstractModifier&)
			 * @param processModifier Callback receiving each valid modifier.
			 *
			 * @see Component::AbstractModifier For modifier interface.
			 * @version 0.8.35
			 */
			template< typename function_t >
			void
			forEachModifiers (function_t && processModifier) const noexcept requires (std::is_invocable_v< function_t, const Component::AbstractModifier & >)
			{
				for ( const auto & modifierWeak : m_modifiers )
				{
					if ( const auto modifier = modifierWeak.lock() )
					{
						processModifier(*modifier);
					}
					else
					{
						Tracer::debug(ClassId, "Dead modifier in the scene modifier list!");
					}
				}
			}

			/**
			 * @brief Returns the list of active environment effects.
			 *
			 * Environment effects are global post-processing or rendering
			 * modifications (fog, color grading, etc.).
			 *
			 * @return Const reference to the effects list.
			 *
			 * @see addEnvironmentEffect() To add effects.
			 * @see clearEnvironmentEffects() To remove all.
			 * @version 0.8.35
			 */
			[[nodiscard]]
			const Saphir::EffectsList &
			environmentEffects () const noexcept
			{
				return m_environmentEffects;
			}

			/**
			 * @brief Returns the audio ambience for this scene.
			 *
			 * Creates the ambience on first access (lazy initialization).
			 * Use this to configure ambience parameters before loading sounds.
			 *
			 * @return Reference to the scene's audio ambience.
			 *
			 * @see hasAmbience() To check if ambience exists without creating it.
			 * @see loadAmbience() To load sounds from a JSON file.
			 * @see Audio::Ambience For ambience configuration API.
			 * @version 0.8.35
			 */
			[[nodiscard]]
			Audio::Ambience & ambience () noexcept;

			/**
			 * @brief Checks if an audio ambience has been created.
			 *
			 * Returns true only if ambience() was called or loadAmbience() succeeded.
			 * Does not create the ambience object.
			 *
			 * @return True if ambience exists, false otherwise.
			 *
			 * @see ambience() To access or create the ambience.
			 * @version 0.8.35
			 */
			[[nodiscard]]
			bool
			hasAmbience () const noexcept
			{
				return m_ambience != nullptr;
			}

			/**
			 * @brief Loads an ambience sound set from a JSON file.
			 *
			 * Creates the ambience if it doesn't exist, then loads sounds
			 * from the specified JSON file. The ambience is not automatically
			 * started after loading.
			 *
			 * @param resourceManager Reference to the resource manager for loading sounds.
			 * @param filepath Path to the JSON ambience definition file.
			 * @return True if loading succeeded, false on failure.
			 *
			 * @see startAmbience() To begin playback after loading.
			 * @see Audio::Ambience::loadSoundSet() For JSON format details.
			 * @version 0.8.35
			 */
			[[nodiscard]]
			bool loadAmbience (Resources::Manager & resourceManager, const std::filesystem::path & filepath) noexcept;

			/**
			 * @brief Starts playing the audio ambience.
			 *
			 * Does nothing if no ambience exists or if already playing.
			 *
			 * @see stopAmbience() To stop playback.
			 * @see loadAmbience() To load sounds first.
			 * @version 0.8.35
			 */
			void startAmbience () const noexcept;

			/**
			 * @brief Stops the audio ambience playback.
			 *
			 * Does nothing if no ambience exists or if not playing.
			 *
			 * @see startAmbience() To resume playback.
			 * @version 0.8.35
			 */
			void stopAmbience () const noexcept;

			/**
			 * @brief Resets the audio ambience to its initial state.
			 *
			 * Stops playback and clears all loaded sounds.
			 * Does nothing if no ambience exists.
			 *
			 * @see loadAmbience() To load new sounds after reset.
			 * @version 0.8.35
			 */
			void resetAmbience () const noexcept;

			/**
			 * @brief Checks if a world position is inside the scene boundary.
			 *
			 * Tests against the cubic boundary defined by setBoundary().
			 *
			 * @param worldPosition Position to test in world coordinates.
			 * @return True if position is within bounds, false otherwise.
			 *
			 * @see boundary() For boundary value.
			 * @version 0.8.35
			 */
			[[nodiscard]]
			bool contains (const Libs::Math::Vector< 3, float > & worldPosition) const noexcept;

			/**
			 * @brief Generates a random position within the scene boundary.
			 *
			 * Uses quickRandom() for fast pseudo-random distribution.
			 * Position is uniformly distributed within the cubic boundary.
			 *
			 * @return Random 3D position within [-boundary, +boundary] on all axes.
			 *
			 * @see boundary() For scene extent.
			 * @see randomizer() For deterministic random.
			 * @version 0.8.35
			 */
			Libs::Math::Vector< 3, float >
			getRandomPosition () const noexcept
			{
				return {
					Libs::Utility::quickRandom(-m_boundary, m_boundary),
					Libs::Utility::quickRandom(-m_boundary, m_boundary),
					Libs::Utility::quickRandom(-m_boundary, m_boundary)
				};
			}

			/**
			 * @brief Returns node tree statistics.
			 *
			 * @return Array of [nodeCount, maxDepth].
			 *
			 * @note Debug utility for scene analysis.
			 *
			 * @see getNodeSystemStatistics() For detailed string report.
			 * @version 0.8.35
			 */
			[[nodiscard]]
			std::array< size_t, 2 > getNodeStatistics () const noexcept;

			/**
			 * @brief Displays an orientation compass at scene origin.
			 *
			 * Creates a visual compass helper showing XYZ axes.
			 * Useful for debugging coordinate systems.
			 *
			 * @param resourceManager Resource manager for loading compass mesh.
			 * @return True if compass was created, false on failure.
			 *
			 * @note This is a debug utility.
			 *
			 * @see disableCompassDisplay() To remove.
			 * @see compassDisplayEnabled() To check state.
			 * @version 0.8.35
			 */
			bool enableCompassDisplay (Resources::Manager & resourceManager) noexcept;

			/**
			 * @brief Removes the orientation compass.
			 *
			 * @note This is a debug utility.
			 *
			 * @see enableCompassDisplay() To show again.
			 * @version 0.8.35
			 */
			void disableCompassDisplay () noexcept;

			/**
			 * @brief Checks if the compass is currently displayed.
			 *
			 * @return True if compass is visible, false otherwise.
			 *
			 * @note This is a debug utility.
			 * @version 0.8.35
			 */
			[[nodiscard]]
			bool compassDisplayEnabled () const noexcept;

			/**
			 * @brief Toggles compass visibility.
			 *
			 * @param resourceManager Resource manager for loading compass mesh.
			 * @return True if compass is now visible, false otherwise.
			 *
			 * @note This is a debug utility.
			 * @version 0.8.35
			 */
			bool toggleCompassDisplay (Resources::Manager & resourceManager) noexcept;

			/**
			 * @brief Displays a ground zero reference plane at Y=0.
			 *
			 * Creates a visual grid at the origin for debugging.
			 *
			 * @param resourceManager Resource manager for loading plane mesh.
			 * @return True if plane was created, false on failure.
			 *
			 * @note This is a debug utility.
			 *
			 * @see disableGroundZeroDisplay() To remove.
			 * @version 0.8.35
			 */
			bool enableGroundZeroDisplay (Resources::Manager & resourceManager) noexcept;

			/**
			 * @brief Removes the ground zero reference plane.
			 *
			 * @note This is a debug utility.
			 *
			 * @see enableGroundZeroDisplay() To show again.
			 * @version 0.8.35
			 */
			void disableGroundZeroDisplay () noexcept;

			/**
			 * @brief Checks if ground zero plane is displayed.
			 *
			 * @return True if visible, false otherwise.
			 *
			 * @note This is a debug utility.
			 * @version 0.8.35
			 */
			[[nodiscard]]
			bool groundZeroDisplayEnabled () const noexcept;

			/**
			 * @brief Toggles ground zero plane visibility.
			 *
			 * @param resourceManager Resource manager for loading plane mesh.
			 *
			 * @note This is a debug utility.
			 * @version 0.8.35
			 */
			void toggleGroundZeroDisplay (Resources::Manager & resourceManager) noexcept;

			/**
			 * @brief Displays scene boundary visualization planes.
			 *
			 * Shows translucent planes at the scene boundary limits.
			 *
			 * @param resourceManager Resource manager for loading plane meshes.
			 * @return True if planes were created, false on failure.
			 *
			 * @note This is a debug utility.
			 *
			 * @see boundary() For boundary extent.
			 * @see disableBoundaryPlanesDisplay() To remove.
			 * @version 0.8.35
			 */
			bool enableBoundaryPlanesDisplay (Resources::Manager & resourceManager) noexcept;

			/**
			 * @brief Removes the boundary visualization planes.
			 *
			 * @note This is a debug utility.
			 *
			 * @see enableBoundaryPlanesDisplay() To show again.
			 * @version 0.8.35
			 */
			void disableBoundaryPlanesDisplay () noexcept;

			/**
			 * @brief Checks if boundary planes are displayed.
			 *
			 * @return True if visible, false otherwise.
			 *
			 * @note This is a debug utility.
			 * @version 0.8.35
			 */
			[[nodiscard]]
			bool boundaryPlanesDisplayEnabled () const noexcept;

			/**
			 * @brief Toggles boundary planes visibility.
			 *
			 * @param resourceManager Resource manager for loading plane meshes.
			 *
			 * @note This is a debug utility.
			 * @version 0.8.35
			 */
			void toggleBoundaryPlanesDisplay (Resources::Manager & resourceManager) noexcept;

			/**
			 * @brief Rebuilds GPU resources for all renderable instances.
			 *
			 * Forces re-preparation of all instances for their render targets.
			 * Useful after GPU resource invalidation.
			 *
			 * @return True if all instances refreshed successfully.
			 * @version 0.8.35
			 */
			bool refreshRenderableInstances () const noexcept;

			/**
			 * @brief Applies all scene modifiers to a node.
			 *
			 * Iterates through m_modifiers and applies each valid modifier's
			 * effect to the given node (gravity, wind, etc.).
			 *
			 * @param node Node to apply modifiers to.
			 *
			 * @see forEachModifiers() For modifier iteration.
			 * @version 0.8.35
			 */
			void applyModifiers (Node & node) const noexcept;

			/**
			 * @brief Returns detailed node system statistics as a formatted string.
			 *
			 * Includes node count, tree depth, and optionally the full hierarchy.
			 *
			 * @param showTree True to include hierarchical tree visualization.
			 * @return Formatted statistics string.
			 *
			 * @note Debug utility for scene analysis.
			 *
			 * @see getNodeStatistics() For raw statistics.
			 * @version 0.8.35
			 */
			[[nodiscard]]
			std::string getNodeSystemStatistics (bool showTree = false) const noexcept;

			/**
			 * @brief Returns detailed static entity statistics as a formatted string.
			 *
			 * Includes entity count and optionally the full entity list.
			 *
			 * @param showTree True to include entity list visualization.
			 * @return Formatted statistics string.
			 *
			 * @note Debug utility for scene analysis.
			 * @version 0.8.35
			 */
			[[nodiscard]]
			std::string getStaticEntitySystemStatistics (bool showTree = false) const noexcept;

			/**
			 * @brief Returns detailed octree sector statistics as a formatted string.
			 *
			 * Includes sector counts for both rendering and physics octrees,
			 * and optionally the full sector hierarchy.
			 *
			 * @param showTree True to include octree visualization.
			 * @return Formatted statistics string.
			 *
			 * @note Debug utility for scene analysis.
			 * @version 0.8.35
			 */
			[[nodiscard]]
			std::string getSectorSystemStatistics (bool showTree = false) const noexcept;

		private:

			/** @copydoc EmEn::Libs::ObserverTrait::onNotification() */
			[[nodiscard]]
			bool onNotification (const Libs::ObservableTrait * observable, int notificationCode, const std::any & data) noexcept override;

			/**
			 * @brief Check if a renderable instance is ready for shadow casting.
			 * @param renderTarget A reference to the render target smart-pointer.
			 * @param renderableInstance A reference to the renderable instance smart-pointer.
			 * @return bool. True to ignore this renderable instance.
			 */
			[[nodiscard]]
			bool checkRenderableInstanceForShadowCasting (const std::shared_ptr< Graphics::RenderTarget::Abstract > & renderTarget, const std::shared_ptr< Graphics::RenderableInstance::Abstract > & renderableInstance) const noexcept;

			/**
			 * @brief Updates the shadow casting render list from the point of view of a light to prepare only the useful data to make a render with it.
			 * @param renderTarget A reference to the render target smart pointer.
			 * @param readStateIndex The render state valid index to read data.
			 * @return bool
			 */
			bool populateShadowCastingRenderList (const std::shared_ptr< Graphics::RenderTarget::Abstract > & renderTarget, uint32_t readStateIndex) noexcept;

			/**
			 * @brief Check if a renderable instance is ready for rendering.
			 * @param renderTarget A reference to the render target smart-pointer.
			 * @param renderableInstance A reference to the renderable instance smart-pointer.
			 * @return bool. True to ignore this renderable instance.
			 */
			[[nodiscard]]
			bool checkRenderableInstanceForRendering (const std::shared_ptr< Graphics::RenderTarget::Abstract > & renderTarget, const std::shared_ptr< Graphics::RenderableInstance::Abstract > & renderableInstance) const noexcept;

			/**
			 * @brief Updates the render lists from a point of view of a camera to prepare only the useful data to make a render with it.
			 * @param renderTarget A reference to the render target smart pointer.
			 * @param readStateIndex The render state valid index to read data.
			 * @return bool
			 */
			bool populateRenderLists (const std::shared_ptr< Graphics::RenderTarget::Abstract > & renderTarget, uint32_t readStateIndex) noexcept;

			/**
			 * @brief Inserts a renderable instance in the render batch list for shadow casting.
			 * @param renderableInstance A reference to a renderable instance.
			 * @param worldCoordinates A pointer to a cartesian frame. A 'nullptr' means origin.
			 * @param distance The distance from the camera.
			 * @return void
			 */
			void insertIntoShadowCastingRenderList (const std::shared_ptr< Graphics::RenderableInstance::Abstract > & renderableInstance, const Libs::Math::CartesianFrame< float > * worldCoordinates, float distance) noexcept;

			/**
			 * @brief Inserts a renderable instance in render lists.
			 * @param renderableInstance A reference to a renderable instance.
			 * @param worldCoordinates A pointer to a cartesian frame. A 'nullptr' means origin.
			 * @param distance The distance from the camera.
			 * @return void
			 */
			void insertIntoRenderLists (const std::shared_ptr< Graphics::RenderableInstance::Abstract > & renderableInstance, const Libs::Math::CartesianFrame< float > * worldCoordinates, float distance) noexcept;

			/**
			 * @brief Renders a list of objects Z-sorted that uses lighting.
			 * @param renderTarget A reference to the render target smart pointer.
			 * @param readStateIndex The render state valid index to read data.
			 * @param commandBuffer A reference to the command buffer.
			 * @param renderBatches A reference to a render batch.
			 * @return void
			 */
			void renderLightedSelection (const std::shared_ptr< Graphics::RenderTarget::Abstract > & renderTarget, uint32_t readStateIndex, const Vulkan::CommandBuffer & commandBuffer, const RenderBatch::List & renderBatches) const noexcept;

			/**
			 * @brief Loops over each renderable instance of the scene
			 * @param function A reference to a function.
			 * @return void
			 */
			void forEachRenderableInstance (const std::function< bool (const std::shared_ptr< Graphics::RenderableInstance::Abstract > & renderableInstance) > & function) const noexcept;

			/**
			 * @brief Checks a notification from the audio video console manager.
			 * @param notificationCode The notification code from AVConsole::Console::NotificationCode enum.
			 * @param data A reference to the notification payload.
			 * @return void
			 */
			void checkAVConsoleNotification (int notificationCode, const std::any & data) noexcept;

			/**
			 * @brief Checks a notification from a scene node.
			 * @param notificationCode The notification code from the Node::NotificationCode enum.
			 * @param data A reference to the notification payload.
			 * @return bool
			 */
			[[nodiscard]]
			bool checkRootNodeNotification (int notificationCode, const std::any & data) noexcept;

			/**
			 * @brief Checks a notification from a scene entity.
			 * @param notificationCode The notification code from the AbstractEntity::NotificationCode enum.
			 * @param data A reference to the notification payload.
			 * @return bool
			 */
			bool checkEntityNotification (int notificationCode, const std::any & data) noexcept;

			/**
			 * @brief Initializes the base component (camera, microphone) of the scene.
			 * @note This is a subpart of the initialize method.
			 * @return bool
			 */
			[[nodiscard]]
			bool initializeBaseComponents () const noexcept;

			/**
			 * @brief Suspends all entities and ambience when scene is disabled.
			 *
			 * Releases pooled resources (audio sources, etc.) from all entities.
			 * Called by disable() when the scene manager switches to another scene.
			 *
			 * @see wakeupAllEntities() To restore after suspension.
			 * @version 0.8.35
			 */
			void suspendAllEntities () noexcept;

			/**
			 * @brief Wakes up all entities and ambience when scene is re-enabled.
			 *
			 * Reacquires pooled resources and restores state for all entities.
			 * Called by enable() when the scene manager activates this scene.
			 *
			 * @see suspendAllEntities() To release resources.
			 * @version 0.8.35
			 */
			void wakeupAllEntities () noexcept;

			/**
			 * @brief Saves scene global visual components.
			 * @return void
			 */
			void registerSceneVisualComponents () noexcept;

			/**
			 * @brief Builds the scene octrees.
			 * @param octreeOptions A reference to an octree options struct.
			 * @return bool
			 */
			bool buildOctrees (const SceneOctreeOptions & octreeOptions) noexcept;

			/**
			 * @brief Destroys the scene octrees.
			 * @return void
			 */
			void destroyOctrees () noexcept;

			/**
			 * @brief Checks the entity location inside all octrees.
			 * @param entity A reference to an entity smart pointer.
			 * @return void
			 */
			void checkEntityLocationInOctrees (const std::shared_ptr< AbstractEntity > & entity) const noexcept;

			/**
			 * @brief Executes collision tests within an octree sector.
			 *
			 * Performs narrow-phase collision detection between all entities
			 * in the sector and recursively processes subsectors.
			 *
			 * @param sector Reference to the current sector being tested.
			 * @param manifolds Output vector for detected collision contact manifolds.
			 *
			 * @note This is a recursive method that descends into subsectors.
			 * @version 0.8.35
			 */
			void sectorCollisionTest (const OctreeSector< AbstractEntity, true > & sector, std::vector< Physics::ContactManifold > & manifolds) noexcept;

			/**
			 * @brief [PHYSICS-NEW-SYSTEM] Clips entity against scene boundaries using sphere collision.
			 *
			 * Uses the entity's bounding sphere to test against the cubic scene
			 * boundaries and generates contact manifolds for boundary collisions.
			 *
			 * @param entity Entity to test for boundary clipping.
			 * @param manifolds Output vector for boundary collision manifolds.
			 *
			 * @see clipWithBoundingBox() For AABB-based clipping.
			 * @version 0.8.35
			 */
			void clipWithBoundingSphere (const std::shared_ptr< AbstractEntity > & entity, std::vector< Physics::ContactManifold > & manifolds) const noexcept;

			/**
			 * @brief [PHYSICS-NEW-SYSTEM] Clips entity against scene boundaries using AABB collision.
			 *
			 * Uses the entity's axis-aligned bounding box to test against the
			 * cubic scene boundaries and generates contact manifolds.
			 *
			 * @param entity Entity to test for boundary clipping.
			 * @param manifolds Output vector for boundary collision manifolds.
			 *
			 * @see clipWithBoundingSphere() For sphere-based clipping.
			 * @version 0.8.35
			 */
			void clipWithBoundingBox (const std::shared_ptr< AbstractEntity > & entity, std::vector< Physics::ContactManifold > & manifolds) const noexcept;

			/**
			 * @brief Initializes a render target with all scene renderable instances.
			 *
			 * Prepares GPU pipelines and resources for each renderable instance
			 * that will be rendered to this target.
			 *
			 * @param renderTarget Target to initialize instances for.
			 * @version 0.8.35
			 */
			void initializeRenderTarget (const std::shared_ptr< Graphics::RenderTarget::Abstract > & renderTarget) const noexcept;

			/**
			 * @brief Determines which render passes apply to a renderable instance.
			 *
			 * Analyzes instance properties (transparency, lighting, etc.) to
			 * select appropriate render passes.
			 *
			 * @param renderableInstance Instance to analyze.
			 * @return Vector of applicable render pass types.
			 * @version 0.8.35
			 */
			[[nodiscard]]
			Libs::StaticVector< Graphics::RenderPassType, Graphics::MaxPassCount > prepareRenderPassTypes (const Graphics::RenderableInstance::Abstract & renderableInstance) const noexcept;

			/**
			 * @brief Prepares a renderable instance for shadow map rendering.
			 *
			 * Creates shadow-casting pipelines and descriptors for the instance.
			 *
			 * @param renderableInstance Instance to prepare.
			 * @param renderTarget Shadow map target.
			 * @return True if ready or deferred, false on permanent failure.
			 *
			 * @note Returns true even if preparation is deferred.
			 * @version 0.8.35
			 */
			[[nodiscard]]
			bool getRenderableInstanceReadyForShadowCasting (const std::shared_ptr< Graphics::RenderableInstance::Abstract > & renderableInstance, const std::shared_ptr< Graphics::RenderTarget::Abstract > & renderTarget) const noexcept;

			/**
			 * @brief Prepares a renderable instance for standard rendering.
			 *
			 * Creates rendering pipelines and descriptors for the instance
			 * on the specified render target.
			 *
			 * @param renderableInstance Instance to prepare.
			 * @param renderTarget Destination render target.
			 * @return True if ready or deferred, false on permanent failure.
			 *
			 * @note Returns true even if preparation is deferred.
			 * @version 0.8.35
			 */
			[[nodiscard]]
			bool getRenderableInstanceReadyForRendering (const std::shared_ptr< Graphics::RenderableInstance::Abstract > & renderableInstance, const std::shared_ptr< Graphics::RenderTarget::Abstract > & renderTarget) const noexcept;

			/** @brief Debug entity name prefix for compass display. @version 0.8.35 */
			static constexpr auto CompassDisplay{"+Compass"};
			/** @brief Debug entity name prefix for ground zero plane. @version 0.8.35 */
			static constexpr auto GroundZeroPlaneDisplay{"+GroundZeroPlane"};
			/** @brief Debug entity name prefix for boundary planes. @version 0.8.35 */
			static constexpr auto BoundaryPlanesDisplay{"+BoundaryPlane"};

			/** @brief Render list index for opaque objects (no lighting). @version 0.8.35 */
			static constexpr auto Opaque{0UL};
			/** @brief Render list index for translucent objects (no lighting). @version 0.8.35 */
			static constexpr auto Translucent{1UL};
			/** @brief Render list index for opaque objects with lighting. @version 0.8.35 */
			static constexpr auto OpaqueLighted{2UL};
			/** @brief Render list index for translucent objects with lighting. @version 0.8.35 */
			static constexpr auto TranslucentLighted{3UL};
			/** @brief Render list index for shadow-casting objects. @version 0.8.35 */
			static constexpr auto Shadows{4UL};

			/* ============================================================
			 * Scene Content - Core entity storage
			 * ============================================================ */

			/** @brief Root of the dynamic node hierarchy tree. Never null. @version 0.8.35 */
			std::shared_ptr< Node > m_rootNode;
			/** @brief Map of static entities by name. O(log n) lookup. @version 0.8.35 */
			std::map< std::string , std::shared_ptr< StaticEntity > > m_staticEntities;
			/** @brief Scene background (skybox, procedural sky). May be null. @version 0.8.35 */
			std::shared_ptr< Graphics::Renderable::AbstractBackground > m_backgroundResource;
			/** @brief Scene terrain/ground. May be null. @version 0.8.35 */
			std::shared_ptr< Graphics::Renderable::SceneAreaInterface > m_sceneAreaResource;
			/** @brief Scene water surface. May be null. @version 0.8.35 */
			std::shared_ptr< Graphics::Renderable::SeaLevelInterface > m_seaLevelResource;

			/* ============================================================
			 * Managers - Core scene subsystems
			 * ============================================================ */

			/** @brief Audio-video console for camera/microphone routing. @version 0.8.35 */
			AVConsole::Manager m_AVConsoleManager;
			/** @brief Light management system for the scene. @version 0.8.35 */
			LightSet m_lightSet;
			/** @brief Render lists indexed by render category (Opaque, Translucent, etc.). @version 0.8.35 */
			std::array< RenderBatch::List, 5 > m_renderLists{};
			/** @brief Debug camera controller. @bug Should not be persistent. @version 0.8.35 */
			NodeController m_nodeController;

			/* ============================================================
			 * Fast Access Structures - Spatial partitioning and caches
			 * ============================================================ */

			/** @brief Octree for rendering frustum culling. @note Uses shared_ptr due to enable_shared_from_this. @version 0.8.35 */
			std::shared_ptr< OctreeSector< AbstractEntity, false > > m_renderingOctree;
			/** @brief Octree for physics broad-phase collision. @note Uses shared_ptr due to enable_shared_from_this. @version 0.8.35 */
			std::shared_ptr< OctreeSector< AbstractEntity, true > > m_physicsOctree;
			/** @brief Visual components for background/terrain/water. @bug Should be refactored. @version 0.8.35 */
			std::array< std::unique_ptr< Component::Visual >, 3 > m_sceneVisualComponents{nullptr, nullptr, nullptr};
			/** @brief Weak references to shadow map render targets. @version 0.8.35 */
			RenderTargetAccessList m_renderToShadowMaps;
			/** @brief Weak references to texture render targets. @version 0.8.35 */
			RenderTargetAccessList m_renderToTextures;
			/** @brief Weak references to view render targets. @version 0.8.35 */
			RenderTargetAccessList m_renderToViews;
			/** @brief Weak references to scene modifiers (force fields, etc.). @version 0.8.35 */
			ModifierAccessList m_modifiers;

			/* ============================================================
			 * Scene Configuration - Physics, effects, timing
			 * ============================================================ */

			/** @brief Global post-processing and rendering effects. @version 0.8.35 */
			Saphir::EffectsList m_environmentEffects;
			/** @brief Audio ambience for background sounds. Lazy-initialized. @version 0.8.35 */
			std::unique_ptr< Audio::Ambience > m_ambience;
			/** @brief Physical environment (gravity, air density). Default: Earth. @version 0.8.35 */
			Physics::EnvironmentPhysicalProperties m_environmentPhysicalProperties{Physics::EnvironmentPhysicalProperties::Earth()};
			/** @brief [PHYSICS-NEW-SYSTEM] Sequential impulse constraint solver. @version 0.8.35 */
			Physics::ConstraintSolver m_constraintSolver{8, 3};
			/** @brief Scene-local random number generator. @version 0.8.35 */
			Libs::Randomizer< float > m_randomizer;
			/** @brief Half-size of cubic scene boundary in meters. @version 0.8.35 */
			float m_boundary{0};
			/** @brief Accumulated scene runtime in microseconds. @version 0.8.35 */
			uint64_t m_lifetimeUS{0};
			/** @brief Accumulated scene runtime in milliseconds. @version 0.8.35 */
			uint32_t m_lifetimeMS{0};
			/** @brief Number of logic cycles executed. @version 0.8.35 */
			size_t m_cycle{0};

			/* ============================================================
			 * Thread Synchronization - Mutexes and atomic state
			 * ============================================================ */

			/** @brief Double-buffer index for thread-safe render state. Atomic for lock-free swap. @version 0.8.35 */
			std::atomic_uint32_t m_renderStateIndex;
			/** @brief Mutex protecting node tree operations. @version 0.8.35 */
			mutable std::mutex m_sceneNodesAccess;
			/** @brief Mutex protecting static entity map. @version 0.8.35 */
			mutable std::mutex m_staticEntitiesAccess;
			/** @brief Mutex protecting rendering octree. @version 0.8.35 */
			mutable std::mutex m_renderingOctreeAccess;
			/** @brief Mutex protecting physics octree. @version 0.8.35 */
			mutable std::mutex m_physicsOctreeAccess;
			/** @brief Mutex protecting shadow map list. @version 0.8.35 */
			mutable std::mutex m_renderToShadowMapAccess;
			/** @brief Mutex protecting texture target list. @version 0.8.35 */
			mutable std::mutex m_renderToTextureAccess;
			/** @brief Mutex protecting view target list. @version 0.8.35 */
			mutable std::mutex m_renderToViewAccess;
			/** @brief Mutex for double-buffer state copy operation. @version 0.8.35 */
			mutable std::mutex m_stateCopyLock;
			/** @brief True after first enable() call succeeds. @version 0.8.35 */
			bool m_initialized{false};
	};
}
