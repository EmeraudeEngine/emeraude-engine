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
#include "Libs/ObservableTrait.hpp"
#include "Libs/ObserverTrait.hpp"
#include "Libs/Time/EventTrait.hpp"

/* Local inclusions for usages. */
#include "Libs/Randomizer.hpp"
#include "Graphics/Renderable/AbstractBackground.hpp"
#include "Graphics/Renderable/SceneAreaInterface.hpp"
#include "Graphics/Renderable/SeaLevelInterface.hpp"
#include "Graphics/RenderTarget/ShadowMap.hpp"
#include "Graphics/RenderTarget/Texture.hpp"
#include "Graphics/RenderTarget/View.hpp"
#include "Audio/SoundEnvironmentProperties.hpp"
#include "Saphir/EffectInterface.hpp"
#include "AVConsole/Manager.hpp"
#include "LightSet.hpp"
#include "OctreeSector.hpp"
#include "StaticEntity.hpp"
#include "Node.hpp"
#include "NodeController.hpp"
#include "RenderBatch.hpp"

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
}

namespace EmEn::Scenes
{
	/** @brief Structure to configure the scene octree initialization. */
	struct SceneOctreeOptions
	{
		size_t renderingOctreeAutoExpandAt{256};
		size_t renderingOctreeReserve{0};
		size_t physicsOctreeAutoExpandAt{32};
		size_t physicsOctreeReserve{3};
	};

	/** @brief Unique non-owner list of render targets for faster access. */
	using RenderTargetAccessList = std::set< std::weak_ptr< Graphics::RenderTarget::Abstract >, std::owner_less<> >;
	/** @brief Unique non-owner list of scene modifiers for faster access. */
	using ModifierAccessList = std::set< std::weak_ptr< Component::AbstractModifier >, std::owner_less<> >;

	/**
	 * @brief Class that describe a whole scene through a node structure.
	 * @note [OBS][SHARED-OBSERVER]
	 * @extends EmEn::Libs::NameableTrait A scene is a named object in the engine.
	 * @extends EmEn::Libs::Time::EventTrait A scene can have timed events.
	 * @extends EmEn::Libs::ObserverTrait The scene will observe the scene node tree and static entity list.
	 * @extends EmEn::Libs::ObservableTrait Scene will notify its content change.
	 */
	class Scene final : public Libs::NameableTrait, public Libs::Time::EventTrait< uint32_t, std::milli >, public Libs::ObserverTrait
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"Scene"};

			/**
			 * @brief Constructs a scene.
			 * @param graphicsRenderer A reference to the graphics renderer.
			 * @param audioManager A reference to the audio manager.
			 * @param name A reference to a string to name it.
			 * @param boundary The distance in all directions to limit the area.
			 * @param background A reference to a background smart pointer. Default none.
			 * @param sceneArea A reference to a sceneArea smart pointer. Default none.
			 * @param seaLevel A reference to a seaLevel smart pointer. Default none.
			 * @param octreeOptions A reference to an option struct. Default octree options.
			 */
			Scene (Graphics::Renderer & graphicsRenderer, Audio::Manager & audioManager, const std::string & name, float boundary, const std::shared_ptr< Graphics::Renderable::AbstractBackground > & background = nullptr, const std::shared_ptr< Graphics::Renderable::SceneAreaInterface > & sceneArea = nullptr, const std::shared_ptr< Graphics::Renderable::SeaLevelInterface > & seaLevel = nullptr, const SceneOctreeOptions & octreeOptions = {}) noexcept
				: NameableTrait{name},
				m_rootNode{std::make_shared< Node >()},
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
			 * @brief Copy constructor.
			 * @param copy A reference to the copied instance.
			 */
			Scene (const Scene & copy) noexcept = delete;

			/**
			 * @brief Move constructor.
			 * @param copy A reference to the copied instance.
			 */
			Scene (Scene && copy) noexcept = delete;

			/**
			 * @brief Copy assignment.
			 * @param copy A reference to the copied instance.
			 * @return Scene &
			 */
			Scene & operator= (const Scene & copy) noexcept = delete;

			/**
			 * @brief Move assignment.
			 * @param copy A reference to the copied instance.
			 * @return Scene &
			 */
			Scene & operator= (Scene && copy) noexcept = delete;

			/**
			 * @brief Destructs the scene.
			 */
			~Scene () override;

			/**
			 * @brief Enables the scene and get it ready for playing.
			 * @param inputManager A reference to the input manager.
			 * @param settings A reference to core settings.
			 * @return bool
			 */
			[[nodiscard]]
			bool enable (Input::Manager & inputManager, Settings & settings) noexcept;

			/**
			 * @brief Disable the scene.
			 * @param inputManager A reference to the input manager.
			 * @return void
			 */
			void disable (Input::Manager & inputManager) noexcept;

			/**
			 * @brief Sets the scene boundary.
			 * @param boundary A value from the center of the scene to the limit of the scene.
			 * @return void
			 */
			void setBoundary (float boundary) noexcept
			{
				m_boundary = std::abs(boundary);

				this->rebuildRenderingOctree(true);

				this->rebuildPhysicsOctree(true);
			}

			/**
			 * @brief Returns the boundary in one direction.
			 * @note To get the total size of an axis, you need to multiply it by two or use SceneAreaInterface::size().
			 * @return float
			 */
			[[nodiscard]]
			float
			boundary () const noexcept
			{
				return m_boundary;
			}

			/**
			 * @brief Returns the square size of the area.
			 * @return float
			 */
			[[nodiscard]]
			float
			size () const noexcept
			{
				return m_boundary * 2;
			}

			/**
			 * @brief Returns the float randomizer from the scene.
			 * @return Libs::Randomizer< float > &
			 */
			[[nodiscard]]
			Libs::Randomizer< float > &
			randomizer () noexcept
			{
				return m_randomizer;
			}

			/**
			 * @brief Returns the execution time of the scene in microseconds.
			 * @return uint64_t
			 */
			[[nodiscard]]
			uint64_t
			lifetimeUS () const noexcept
			{
				return m_lifetimeUS;
			}

			/**
			 * @brief Returns the execution time of the scene in milliseconds.
			 * @return uint32_t
			 */
			[[nodiscard]]
			uint32_t
			lifetimeMS () const noexcept
			{
				return m_lifetimeMS;
			}

			/**
			 * @brief Returns the number of cycles executed by the scene.
			 * @return size_t
			 */
			[[nodiscard]]
			size_t
			cycle () const noexcept
			{
				return m_cycle;
			}

			/**
			 * @brief Sets the scene physical environment properties.
			 * @param properties A reference to a physical environment properties.
			 * @return void
			 */
			void
			setPhysicalEnvironmentProperties (const Physics::PhysicalEnvironmentProperties & properties) noexcept
			{
				m_physicalEnvironmentProperties = properties;
			}

			/**
			 * @brief Creates a render to a shadow map (Texture2D) device.
			 * @param name A reference to a string to name the virtual video device.
			 * @param resolution The resolution of the shadow map.
			 * @param isOrthographicProjection Set an orthographic projection instead of perspective.
			 * @return std::shared_ptr< Graphics::RenderTarget::ShadowMap< Graphics::ViewMatrices2DUBO > >
			 */
			[[nodiscard]]
			std::shared_ptr< Graphics::RenderTarget::ShadowMap< Graphics::ViewMatrices2DUBO > > createRenderToShadowMap (const std::string & name, uint32_t resolution, bool isOrthographicProjection) noexcept;

			/**
			 * @brief Creates a render to a cubic shadow map (Cubemap) device.
			 * @param name A reference to a string to name the virtual video device.
			 * @param resolution The resolution of the shadow map.
			 * @param isOrthographicProjection Set an orthographic projection instead of perspective.
			 * @return std::shared_ptr< Graphics::RenderTarget::ShadowMap< Graphics::ViewMatrices3DUBO > >
			 */
			[[nodiscard]]
			std::shared_ptr< Graphics::RenderTarget::ShadowMap< Graphics::ViewMatrices3DUBO > > createRenderToCubicShadowMap (const std::string & name, uint32_t resolution, bool isOrthographicProjection) noexcept;

			/**
			 * @brief Creates a render to a texture 2D device.
			 * @param name A reference to a string to name the virtual video device.
			 * @param width The width of the surface.
			 * @param height The height of the surface.
			 * @param colorCount The number of color channels desired for the texture2Ds.
			 * @param isOrthographicProjection Set an orthographic projection instead of perspective.
			 * @return std::shared_ptr< Graphics::RenderTarget::Texture< Graphics::ViewMatrices2DUBO > >
			 */
			[[nodiscard]]
			std::shared_ptr< Graphics::RenderTarget::Texture< Graphics::ViewMatrices2DUBO > > createRenderToTexture2D (const std::string & name, uint32_t width, uint32_t height, uint32_t colorCount, bool isOrthographicProjection) noexcept;

			/**
			 * @brief Creates a render to cubemap device.
			 * @param name A reference to a string to name the virtual video device.
			 * @param size The size of the cubemap.
			 * @param colorCount The number of color channels desired for the texture2Ds.
			 * @param isOrthographicProjection Set an orthographic projection instead of perspective.
			 * @return std::shared_ptr< Graphics::RenderTarget::Texture::Texture< Graphics::ViewMatrices3DUBO > >
			 */
			[[nodiscard]]
			std::shared_ptr< Graphics::RenderTarget::Texture< Graphics::ViewMatrices3DUBO > > createRenderToCubemap (const std::string & name, uint32_t size, uint32_t colorCount, bool isOrthographicProjection) noexcept;

			/**
			 * @brief Creates a render-to-view (Texture 2D) device.
			 * @param name A reference to a string to name the virtual video device.
			 * @param width The width of the surface.
			 * @param height The height of the surface.
			 * @param precisions A reference to a framebuffer precisions structure.
			 * @param isOrthographicProjection Set an orthographic projection instead of perspective.
			 * @param primaryDevice Set the device as the primary output. Default false.
			 * @return std::shared_ptr< Graphics::RenderTarget::View< Graphics::ViewMatrices2DUBO > >
			 */
			[[nodiscard]]
			std::shared_ptr< Graphics::RenderTarget::View< Graphics::ViewMatrices2DUBO > > createRenderToView (const std::string & name, uint32_t width, uint32_t height, const Graphics::FramebufferPrecisions & precisions, bool isOrthographicProjection, bool primaryDevice = false) noexcept;

			/**
			 * @brief Creates a render to cubic view (Cubemap) device.
			 * @param name A reference to a string to name the virtual video device.
			 * @param size The size of the cubemap.
			 * @param precisions A reference to a framebuffer precisions structure.
			 * @param isOrthographicProjection Set an orthographic projection instead of perspective.
			 * @param primaryDevice Set the device as the primary output. Default false.
			 * @return std::shared_ptr< Graphics::RenderTarget::View< Graphics::ViewMatrices3DUBO > >
			 */
			[[nodiscard]]
			std::shared_ptr< Graphics::RenderTarget::View< Graphics::ViewMatrices3DUBO > > createRenderToCubicView (const std::string & name, uint32_t size, const Graphics::FramebufferPrecisions & precisions, bool isOrthographicProjection, bool primaryDevice = false) noexcept;

			/**
			 * @brief Creates a static entity in the scene.
			 * @param name The name of the entity.
			 * @param coordinates A reference to coordinates for the initial location of the entity. Default origin.
			 * @return std::shared_ptr< StaticEntity >
			 */
			[[nodiscard]]
			std::shared_ptr< StaticEntity > createStaticEntity (const std::string & name, const Libs::Math::CartesianFrame< float > & coordinates = {}) noexcept;

			/**
			 * @brief Creates a static entity in the scene using only a position.
			 * @param name The name of the entity.
			 * @param position A reference to a vector.
			 * @return std::shared_ptr< StaticEntity >
			 */
			[[nodiscard]]
			std::shared_ptr< StaticEntity >
			createStaticEntity (const std::string & name, const Libs::Math::Vector< 3, float > & position) noexcept
			{
				return this->createStaticEntity(name, Libs::Math::CartesianFrame< float >{position});
			}

			/**
			 * @brief Removes a static entity from the scene.
			 * @param name A reference to a string for the entity name.
			 * @return bool
			 */
			bool removeStaticEntity (const std::string & name) noexcept;

			/**
			 * @brief Adds a global effect to the scene.
			 * @param effect A reference to an effect interface smart pointer.
			 * @return void
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
			 * @brief Returns whether a global effect is already present on the scene.
			 * @param effect A reference to an effect interface smart pointer.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isEnvironmentEffectPresent (const std::shared_ptr< Saphir::EffectInterface > & effect) const noexcept
			{
				return m_environmentEffects.contains(effect);
			}

			/**
			 * @brief Clears every effect from the scene.
			 * @return void
			 */
			void
			clearEnvironmentEffects () noexcept
			{
				m_environmentEffects.clear();
			}

			/**
			 * @brief Rebuilds the scene rendering octree.
			 * @param keepElements Keep elements from the previous octrees. Default true.
			 * @return void
			 */
			bool rebuildRenderingOctree (bool keepElements = true) noexcept;

			/**
			 * @brief Rebuilds the physics rendering octree.
			 * @param keepElements Keep elements from the previous octrees. Default true.
			 * @return void
			 */
			bool rebuildPhysicsOctree (bool keepElements = true) noexcept;

			/**
			 * @brief Removes all nodes.
			 * @return void
			 */
			void resetNodeTree () const noexcept;

			/**
			 * @brief The Core calls this method every logic cycle.
			 * @param engineCycle The cycle number of the engine.
			 * @return void
			 */
			void processLogics (size_t engineCycle) noexcept;

			/**
			 * @brief Copies local data for a stable render for all scene elements.
			 * @note This must be done at the end of the logic loop.
			 * @return void
			 */
			void publishStateForRendering () noexcept;

			/**
			 * @brief Updates the video memory just before rendering.
			 * @note This should only update things which must be ready for rendering.
			 * @param shadowMapEnabled The render to shadow map state.
			 * @param renderToTextureEnabled The render to texture state.
			 * @return void
			 */
			void updateVideoMemory (bool shadowMapEnabled, bool renderToTextureEnabled) const noexcept;

			/**
			 * @brief Performs a shadow casting pass of the scene.
			 * @param renderTarget A reference to the render target smart pointer.
			 * @param commandBuffer A reference to a command buffer.
			 * @return void
			 */
			void castShadows (const std::shared_ptr< Graphics::RenderTarget::Abstract > & renderTarget, const Vulkan::CommandBuffer & commandBuffer) noexcept;

			/**
			 * @brief Renders the scene to a render target.
			 * @param renderTarget A reference to the render target smart pointer.
			 * @param commandBuffer A reference to a command buffer.
			 * @return void
			 */
			void render (const std::shared_ptr< Graphics::RenderTarget::Abstract > & renderTarget, const Vulkan::CommandBuffer & commandBuffer) noexcept;

			/**
			 * @brief Returns the audio video console manager.
			 * @return const AVConsole::Manager &
			 */
			[[nodiscard]]
			const AVConsole::Manager &
			AVConsoleManager () const noexcept
			{
				return m_AVConsoleManager;
			}

			/**
			 * @brief Returns the audio video console manager.
			 * @return AVConsole::Manager &
			 */
			[[nodiscard]]
			AVConsole::Manager &
			AVConsoleManager () noexcept
			{
				return m_AVConsoleManager;
			}

			/**
			 * @brief Returns the light set of the scene.
			 * @return const LightSet &
			 */
			[[nodiscard]]
			const LightSet &
			lightSet () const noexcept
			{
				return m_lightSet;
			}

			/**
			 * @brief Returns the light set of the scene.
			 * @return LightSet &
			 */
			[[nodiscard]]
			LightSet &
			lightSet () noexcept
			{
				return m_lightSet;
			}

			/**
			 * @brief Returns the node controller.
			 * @return const NodeController &
			 */
			[[nodiscard]]
			const NodeController &
			nodeController () const noexcept
			{
				return m_nodeController;
			}

			/**
			 * @brief Returns the node controller.
			 * @return NodeController &
			 */
			[[nodiscard]]
			NodeController & nodeController () noexcept
			{
				return m_nodeController;
			}

			/**
			 * @brief Returns the scene physical environment properties.
			 * @return const Physics::PhysicalEnvironmentProperties &
			 */
			[[nodiscard]]
			const Physics::PhysicalEnvironmentProperties &
			physicalEnvironmentProperties () const noexcept
			{
				return m_physicalEnvironmentProperties;
			}

			/**
			 * @brief Returns the scene physical environment properties.
			 * @return Physics::PhysicalEnvironmentProperties &
			 */
			[[nodiscard]]
			Physics::PhysicalEnvironmentProperties &
			physicalEnvironmentProperties () noexcept
			{
				return m_physicalEnvironmentProperties;
			}

			/**
			 * @brief Executes a function on render to shadow maps with thread-safe access.
			 * @tparam function_t The type of function. Signature: void (const RenderTargetAccessList &)
			 * @param processRenderTargets
			 * @return void
			 */
			template< typename function_t >
			void
			withRenderToShadowMaps (function_t && processRenderTargets) const noexcept requires (std::is_invocable_v< function_t, const RenderTargetAccessList & >)
			{
				const std::lock_guard< std::mutex > lock{m_renderToShadowMapAccess};

				processRenderTargets(m_renderToShadowMaps);
			}

			/**
			 * @brief Executes a function on render to textures with thread-safe access.
			 * @tparam function_t The type of function. Signature: void (const RenderTargetAccessList &)
			 * @param processRenderTargets
			 * @return void
			 */
			template< typename function_t >
			void
			withRenderToTextures (function_t && processRenderTargets) const noexcept requires (std::is_invocable_v< function_t, const RenderTargetAccessList & >)
			{
				const std::lock_guard< std::mutex > lock{m_renderToTextureAccess};

				processRenderTargets(m_renderToTextures);
			}

			/**
			 * @brief Executes a function on render to views with thread-safe access.
			 * @tparam function_t The type of function. Signature: void (const RenderTargetAccessList &)
			 * @param processRenderTargets
			 * @return void
			 */
			template< typename function_t >
			void
			withRenderToViews (function_t && processRenderTargets) const noexcept requires (std::is_invocable_v< function_t, const RenderTargetAccessList & >)
			{
				const std::lock_guard< std::mutex > lock{m_renderToViewAccess};

				processRenderTargets(m_renderToViews);
			}

			/**
			 * @brief Executes a function per render to a shadow map with thread-safe access.
			 * @tparam function_t The type of function. Signature: void (const std::shared_ptr< Graphics::RenderTarget::Abstract > &)
			 * @param processRenderTarget
			 * @return void
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
			 * @brief Executes a function per render to texture with thread-safe access.
			 * @todo Pass the render target as a reference !
			 * @tparam function_t The type of function. Signature: void (const std::shared_ptr< Graphics::RenderTarget::Abstract > &)
			 * @param processRenderTarget
			 * @return void
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
			 * @brief Executes a function per render to view with thread-safe access.
			 * @todo Pass the render target as a reference !
			 * @tparam function_t The type of function. Signature: void (const std::shared_ptr< Graphics::RenderTarget::Abstract > &)
			 * @param processRenderTarget
			 * @return void
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
			 * @brief Sets the scene background.
			 * @todo Pass the render target as a reference !
			 * @param background A reference to a background smart pointer.
			 * @return void
			 */
			void
			setBackground (const std::shared_ptr< Graphics::Renderable::AbstractBackground > & background) noexcept
			{
				m_backgroundResource = background;

				this->registerSceneVisualComponents();
			}

			/**
			 * @brief Returns the current background of the scene.
			 * @return std::shared_ptr< Graphics::Renderable::AbstractBackground >
			 */
			[[nodiscard]]
			std::shared_ptr< Graphics::Renderable::AbstractBackground >
			background () const noexcept
			{
				return m_backgroundResource;
			}

			/**
			 * @brief Sets the scene sceneArea.
			 * @param sceneArea A reference to a sceneArea smart pointer.
			 * @return void
			 */
			void
			setSceneArea (const std::shared_ptr< Graphics::Renderable::SceneAreaInterface > & sceneArea) noexcept
			{
				m_sceneAreaResource = sceneArea;

				this->registerSceneVisualComponents();
			}

			/**
			 * @brief Returns the current scene area.
			 * @return std::shared_ptr< Graphics::Renderable::SceneAreaInterface >
			 */
			[[nodiscard]]
			std::shared_ptr< Graphics::Renderable::SceneAreaInterface >
			sceneArea () const noexcept
			{
				return m_sceneAreaResource;
			}

			/**
			 * @brief Sets the sea level for the scene.
			 * @param seaLevel A reference to a background smart pointer.
			 * @return void
			 */
			void
			setSeaLevel (const std::shared_ptr< Graphics::Renderable::SeaLevelInterface > & seaLevel) noexcept
			{
				m_seaLevelResource = seaLevel;

				this->registerSceneVisualComponents();
			}

			/**
			 * @brief Returns the current water level.
			 * @return std::shared_ptr< Graphics::Renderable::SeaLevelInterface >
			 */
			[[nodiscard]]
			std::shared_ptr< Graphics::Renderable::SeaLevelInterface >
			seaLevel () const noexcept
			{
				return m_seaLevelResource;
			}

			/**
			 * @brief Returns the root scene node from the scene.
			 * @return std::shared_ptr< Node >
			 */
			[[nodiscard]]
			std::shared_ptr< Node >
			root () const noexcept
			{
				return m_rootNode;
			}

			/**
			 * @brief Searches from the top of the node tree the first named node.
			 * @param nodeName A reference to a string.
			 * @return std::shared_ptr< Node >
			 */
			[[nodiscard]]
			std::shared_ptr< Node > findNode (const std::string & nodeName) const noexcept;

			/**
			 * @brief Executes a function per static entity with thread-safe access.
			 * @tparam function_t The type of function. Signature: void (const StaticEntity &)
			 * @param processStaticEntity
			 * @return void
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
			 * @brief Tries to find a static entity by its name.
			 * @note Check for a nullptr return!
			 * @param staticEntityName A reference to a string.
			 * @return std::shared_ptr< StaticEntity >
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
			 * @brief Executes a function per modifier present in the scene.
			 * @tparam function_t The type of function. Signature: void (const Component::AbstractModifier &)
			 * @param processModifier
			 * @return void
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
			 * @brief Returns the list of active global effects in the scene.
			 * @return const Saphir::EffectsList &
			 */
			[[nodiscard]]
			const Saphir::EffectsList &
			environmentEffects () const noexcept
			{
				return m_environmentEffects;
			}

			/**
			 * @brief Checks if a position is inside the scene area.
			 * @param worldPosition An absolute position.
			 * @return bool
			 */
			[[nodiscard]]
			bool contains (const Libs::Math::Vector< 3, float > & worldPosition) const noexcept;

			/**
			 * @brief Returns a random position within the scene area.
			 * @return Libs::Math::Vector< 3, float >
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
			 * @brief Returns node count and depth.
			 * @return std::array< size_t, 2 >
			 */
			[[nodiscard]]
			std::array< size_t, 2 > getNodeStatistics () const noexcept;

			/**
			 * @brief Shows a compass.
			 * @note This a debug utility.
			 * @param resourceManager A reference to the resource manager.
			 * @return bool
			 */
			bool enableCompassDisplay (Resources::Manager & resourceManager) noexcept;

			/**
			 * @brief Removes the scene compass.
			 * @note This a debug utility.
			 * @return void
			 */
			void disableCompassDisplay () noexcept;

			/**
			 * @brief Returns whether the scene compass is displayed.
			 * @note This a debug utility.
			 * @return bool
			 */
			[[nodiscard]]
			bool compassDisplayEnabled () const noexcept;

			/**
			 * @brief Toggles the display of the scene compass.
			 * @note This a debug utility.
			 * @param resourceManager A reference to the resource manager.
			 * @return bool
			 */
			bool toggleCompassDisplay (Resources::Manager & resourceManager) noexcept;

			/**
			 * @brief Shows the ground zero of the scene.
			 * @note This a debug utility.
			 * @param resourceManager A reference to the resource manager.
			 * @return bool
			 */
			bool enableGroundZeroDisplay (Resources::Manager & resourceManager) noexcept;

			/**
			 * @brief Removes the ground zero of the scene.
			 * @note This a debug utility.
			 * @return void
			 */
			void disableGroundZeroDisplay () noexcept;

			/**
			 * @brief Returns whether the ground zero is displayed.
			 * @note This a debug utility.
			 * @return bool
			 */
			[[nodiscard]]
			bool groundZeroDisplayEnabled () const noexcept;

			/**
			 * @brief Toggles the display of ground zero.
			 * @note This a debug utility.
			 * @param resourceManager A reference to the resource manager.
			 * @return void
			 */
			void toggleGroundZeroDisplay (Resources::Manager & resourceManager) noexcept;

			/**
			 * @brief Shows the scene boundary planes.
			 * @note This a debug utility.
			 * @param resourceManager A reference to the resource manager.
			 * @return bool
			 */
			bool enableBoundaryPlanesDisplay (Resources::Manager & resourceManager) noexcept;

			/**
			 * @brief Removes the scene boundary planes.
			 * @note This a debug utility.
			 * @return void
			 */
			void disableBoundaryPlanesDisplay () noexcept;

			/**
			 * @brief Returns whether the scene boundary planes are displayed.
			 * @note This a debug utility.
			 * @return bool
			 */
			[[nodiscard]]
			bool boundaryPlanesDisplayEnabled () const noexcept;

			/**
			 * @brief Toggles the display of scene boundary planes.
			 * @note This a debug utility.
			 * @param resourceManager A reference to the resource manager.
			 * @return void
			 */
			void toggleBoundaryPlanesDisplay (Resources::Manager & resourceManager) noexcept;

			/**
			 * @brief Refreshes all renderable instances used in the scene.
			 * @return bool
			 */
			bool refreshRenderableInstances () const noexcept;

			/**
			 * @brief Applies scene modifiers on a node.
			 * @param node A reference to a node.
			 * @return void
			 */
			void applyModifiers (Node & node) const noexcept;

			/**
			 * @brief Returns a string with the node system statistics.
			 * @note Debug purpose.
			 * @param showTree Enable the scene node tree. Default false.
			 * @return std::string
			 */
			[[nodiscard]]
			std::string getNodeSystemStatistics (bool showTree = false) const noexcept;

			/**
			 * @brief Returns a string with the static entity system statistics.
			 * @note Debug purpose.
			 * @param showTree Enable the static entity tree. Default false.
			 * @return std::string
			 */
			[[nodiscard]]
			std::string getStaticEntitySystemStatistics (bool showTree = false) const noexcept;

			/**
			 * @brief Returns a string with the sector system statistics.
			 * @note Debug purpose.
			 * @param showTree Enable the sector tree. Default false.
			 * @return std::string
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
			bool
			checkRenderableInstanceForShadowCasting (const std::shared_ptr< Graphics::RenderTarget::Abstract > & renderTarget, const std::shared_ptr< Graphics::RenderableInstance::Abstract > & renderableInstance) const noexcept
			{
				/* Check whether the renderable instance is ready for shadow casting. */
				if ( renderableInstance->isReadyToCastShadows(renderTarget) )
				{
					return false; // Render
				}

				/* If it still unloaded. */
				if ( !renderableInstance->renderable()->isReadyForInstantiation() )
				{
					return true; // Continue
				}

				if ( this->getRenderableInstanceReadyForShadowCasting(renderableInstance, renderTarget) )
				{
					return false; // Render
				}

				/* If the object cannot be loaded, mark it as broken! */
				renderableInstance->setBroken("Unable to get ready for shadow casting !");

				return true; // Continue
			}

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
			bool
			checkRenderableInstanceForRendering (const std::shared_ptr< Graphics::RenderTarget::Abstract > & renderTarget, const std::shared_ptr< Graphics::RenderableInstance::Abstract > & renderableInstance) const noexcept
			{
				/* Check whether the renderable instance is ready for shadow casting. */
				if ( renderableInstance->isReadyToRender(renderTarget) )
				{
					return false; // Render
				}

				/* If it still unloaded. */
				if ( !renderableInstance->renderable()->isReadyForInstantiation() )
				{
					return true; // Continue
				}

				if ( this->getRenderableInstanceReadyForRendering(renderableInstance, renderTarget) )
				{
					return false; // Render
				}

				/* If the object cannot be loaded, mark it as broken! */
				renderableInstance->setBroken("Unable to get ready for rendering !");

				return true; // Continue
			}

			/**
			 * @brief Updates the render lists from a point of view of a camera to prepare only the useful data to make a render with it.
			 * @param renderTarget A reference to the render target smart pointer.
			 * @param readStateIndex The render state valid index to read data.
			 * @return bool
			 */
			bool populateRenderLists (const std::shared_ptr< Graphics::RenderTarget::Abstract > & renderTarget, uint32_t readStateIndex) noexcept;

			/**
			 * @brief Inserts a renderable instance in the render batch list for shadow casting.
			 * @param renderTarget A reference to the render target smart pointer.
			 * @param renderableInstance A reference to a renderable instance.
			 * @param worldCoordinates A pointer to a cartesian frame. A 'nullptr' means origin.
			 * @param distance The distance from the camera.
			 * @return void
			 */
			void insertIntoShadowCastingRenderList (const std::shared_ptr< Graphics::RenderTarget::Abstract > & renderTarget, const std::shared_ptr< Graphics::RenderableInstance::Abstract > & renderableInstance, const Libs::Math::CartesianFrame< float > * worldCoordinates, float distance) noexcept;

			/**
			 * @brief Inserts a renderable instance in render lists.
			 * @param renderTarget A reference to the render target smart pointer.
			 * @param renderableInstance A reference to a renderable instance.
			 * @param worldCoordinates A pointer to a cartesian frame. A 'nullptr' means origin.
			 * @param distance The distance from the camera.
			 * @return void
			 */
			void insertIntoRenderLists (const std::shared_ptr< Graphics::RenderTarget::Abstract > & renderTarget, const std::shared_ptr< Graphics::RenderableInstance::Abstract > & renderableInstance, const Libs::Math::CartesianFrame< float > * worldCoordinates, float distance) noexcept;

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
			 * @brief Executes a collision test between a scene at a sector.
			 * @note This is a recursive method with subsector.
			 * @param sector A reference to the current sector tested.
			 * @return void
			 */
			void sectorCollisionTest (const OctreeSector< AbstractEntity, true > & sector) noexcept;

			/**
			 * @brief Checks if a scene node is clipping with the scene area boundaries.
			 * @param entity A reference to an entity smart pointer.
			 * @return void
			 */
			void clipWithBoundingSphere (const std::shared_ptr< AbstractEntity > & entity) const noexcept;

			/**
			 * @brief Checks if a scene node is clipping with the scene area boundaries.
			 * @param entity A reference to an entity smart pointer.
			 * @return void
			 */
			void clipWithBoundingBox (const std::shared_ptr< AbstractEntity > & entity) const noexcept;

			/**
			 * @brief Initializes a render target with renderable instances.
			 * @param renderTarget A reference to a render target smart pointer.
			 * @return void
			 */
			void initializeRenderTarget (const std::shared_ptr< Graphics::RenderTarget::Abstract > & renderTarget) const noexcept;

			/**
			 * @brief Prepare the render passes, according to the scene.
			 * @param renderableInstance A reference to the renderable instance.
			 * @return std::vector< RenderPassType >
			 */
			[[nodiscard]]
			std::vector< Graphics::RenderPassType > prepareRenderPassTypes (const Graphics::RenderableInstance::Abstract & renderableInstance) const noexcept;

			/**
			 * @brief Prepares a renderable instance for a shadow map.
			 * @note This function returns false only if the instance cannot be prepared. 'True' can postpone the preparation.
			 * @param renderableInstance A reference to the renderable instance smart pointer.
			 * @param renderTarget A reference to a render target smart pointer where the renderable instance must get ready.
			 * @return bool
			 */
			[[nodiscard]]
			bool getRenderableInstanceReadyForShadowCasting (const std::shared_ptr< Graphics::RenderableInstance::Abstract > & renderableInstance, const std::shared_ptr< Graphics::RenderTarget::Abstract > & renderTarget) const noexcept;

			/**
			 * @brief Prepares a renderable instance for a specific rendering.
			 * @note This function returns false only if the instance cannot be prepared. 'True' can postpone the preparation.
			 * @param renderableInstance A reference to the renderable instance smart pointer.
			 * @param renderTarget A reference to a render target smart pointer where the renderable instance must get ready.
			 * @return bool
			 */
			[[nodiscard]]
			bool getRenderableInstanceReadyForRendering (const std::shared_ptr< Graphics::RenderableInstance::Abstract > & renderableInstance, const std::shared_ptr< Graphics::RenderTarget::Abstract > & renderTarget) const noexcept;

			static constexpr auto CompassDisplay{"+Compass"};
			static constexpr auto GroundZeroPlaneDisplay{"+GroundZeroPlane"};
			static constexpr auto BoundaryPlanesDisplay{"+BoundaryPlane"};

			/* Render list types. */
			static constexpr auto Opaque{0UL};
			static constexpr auto Translucent{1UL};
			static constexpr auto OpaqueLighted{2UL};
			static constexpr auto TranslucentLighted{3UL};
			static constexpr auto Shadows{4UL};

			/* NOTE: Real scene content holder. */
			std::shared_ptr< Node > m_rootNode;
			std::map< std::string , std::shared_ptr< StaticEntity > > m_staticEntities;
			std::shared_ptr< Graphics::Renderable::AbstractBackground > m_backgroundResource;
			std::shared_ptr< Graphics::Renderable::SceneAreaInterface > m_sceneAreaResource;
			std::shared_ptr< Graphics::Renderable::SeaLevelInterface > m_seaLevelResource;

			/* NOTE: Managers deeply linked to the scene content. */
			AVConsole::Manager m_AVConsoleManager;
			LightSet m_lightSet;
			std::array< RenderBatch::List, 5 > m_renderLists{};
			NodeController m_nodeController; /* FIXME: This shouldn't be a persistent instance here. This is a debug thing. */

			/* NOTE: Structures for faster access to specific data. */
			std::shared_ptr< OctreeSector< AbstractEntity, false > > m_renderingOctree; // NOTE: Should be std::unique_ptr, but can't because of the std::enable_shared_from_this usage.
			std::shared_ptr< OctreeSector< AbstractEntity, true > > m_physicsOctree; // NOTE: Should be std::unique_ptr, but can't because of the std::enable_shared_from_this usage.
			std::array< std::unique_ptr< Component::Visual >, 3 > m_sceneVisualComponents{nullptr, nullptr, nullptr}; // FIXME: Remove this!
			RenderTargetAccessList m_renderToShadowMaps;
			RenderTargetAccessList m_renderToTextures;
			RenderTargetAccessList m_renderToViews;
			ModifierAccessList m_modifiers;

			/* NOTE: Scene setup data. */
			Saphir::EffectsList m_environmentEffects;
			Physics::PhysicalEnvironmentProperties m_physicalEnvironmentProperties{Physics::PhysicalEnvironmentProperties::Earth()};
			Audio::SoundEnvironmentProperties m_soundEnvironmentProperties;
			Libs::Randomizer< float > m_randomizer; /* NOTE: Keep a randomizer active for this scene. */
			float m_boundary{0};
			uint64_t m_lifetimeUS{0};
			uint32_t m_lifetimeMS{0};
			size_t m_cycle{0};
			std::atomic_uint32_t m_renderStateIndex;
			mutable std::mutex m_sceneNodesAccess;
			mutable std::mutex m_staticEntitiesAccess;
			mutable std::mutex m_renderingOctreeAccess;
			mutable std::mutex m_physicsOctreeAccess;
			mutable std::mutex m_renderToShadowMapAccess;
			mutable std::mutex m_renderToTextureAccess;
			mutable std::mutex m_renderToViewAccess;
			mutable std::mutex m_stateCopyLock;
			bool m_initialized{false};
	};
}
