/*
 * src/Scenes/Manager.hpp
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
#include <array>
#include <cstddef>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>
#include <shared_mutex>

/* Local inclusions for inheritances. */
#include "ServiceInterface.hpp"
#include "Console/Controllable.hpp"

/* Local inclusions for usages. */
#include "DefinitionResource.hpp"
#include "Graphics/Renderable/AbstractBackground.hpp"
#include "Graphics/Renderable/SceneAreaInterface.hpp"
#include "Graphics/Renderable/SeaLevelInterface.hpp"
#include "Scene.hpp"

/* Forward declarations. */
namespace EmEn
{
	namespace Resources
	{
		class Manager;
	}

	namespace Graphics
	{
		class Renderer;
	}

	class PrimaryServices;
}

namespace EmEn::Scenes
{
	/**
	 * @brief Keeps scene targets for the console.
	 */
	class ConsoleMemory final
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"ConsoleMemory"};

			ConsoleMemory () = default;

			void
			target (const std::shared_ptr< Scene > & scene) noexcept
			{
				m_scene = scene;
			}

			void
			target (const std::shared_ptr< Node > & sceneNode) noexcept
			{
				m_sceneNode = sceneNode;
			}

			void
			target (const std::shared_ptr< StaticEntity > & staticEntity) noexcept
			{
				m_staticEntity = staticEntity;
			}

			void
			target (const std::shared_ptr< Component::Abstract > & entityComponent) noexcept
			{
				m_entityComponent = entityComponent;
			}

			[[nodiscard]]
			std::shared_ptr< Scene >
			scene () const noexcept
			{
				return m_scene.lock();
			}

			[[nodiscard]]
			std::shared_ptr< Node >
			sceneNode () const noexcept
			{
				return m_sceneNode.lock();
			}

			[[nodiscard]]
			std::shared_ptr< StaticEntity >
			staticEntity () const noexcept
			{
				return m_staticEntity.lock();
			}

			[[nodiscard]]
			std::shared_ptr< Component::Abstract >
			entityComponent () const noexcept
			{
				return m_entityComponent.lock();
			}

		private:

			std::weak_ptr< Scene > m_scene;
			std::weak_ptr< Node > m_sceneNode;
			std::weak_ptr< StaticEntity > m_staticEntity;
			std::weak_ptr< Component::Abstract > m_entityComponent;
	};

	/**
	 * @brief The scene manager service class.
	 * @extends EmEn::ServiceInterface This is a service.
	 * @extends EmEn::Console::Controllable The scene manager service is usable from the console.
	 */
	class Manager final : public ServiceInterface, public Console::Controllable
	{
		public:

			using SceneLoading = std::pair< std::shared_ptr< Scene >, std::shared_ptr< DefinitionResource > >;

			/** @brief Class identifier. */
			static constexpr auto ClassId{"SceneManagerService"};

			/** @brief Observable class unique identifier. */
			static const size_t ClassUID;

			static constexpr auto DefaultSceneBoundary{1000.0F};

			/** @brief Observable notification codes. */
			enum NotificationCode
			{
				/** @brief This event is fired when a new empty scene has been created. The scene smart pointer will be passed. */
				SceneCreated,
				/**
				 * @brief This event is fired when a scene is loaded from a file. The scene smart pointer will be passed.
				 * @note This event will come after a SceneCreated event. */
				SceneLoaded,
				/** @brief This event is fired when a scene has been destroyed or all scenes deleted. No data will be passed with it. */
				SceneDestroyed,
				/** @brief This event is fired when a scene become the active one. The scene smart pointer will be passed. */
				SceneActivated,
				/** @brief This event is fired when a scene is deactivated (not destroyed). The scene smart pointer will be passed. */
				SceneDeactivated,
				/* Enumeration boundary. */
				MaxEnum
			};

			/**
			 * @brief Constructs the scene manager.
			 * @param primaryServices A reference to primary services.
			 * @param resourceManager A reference to the resource manager.
			 * @param graphicsRenderer A reference to the graphics renderer.
			 * @param audioManager A reference to the audio manager.
			 */
			Manager (PrimaryServices & primaryServices, Resources::Manager & resourceManager, Graphics::Renderer & graphicsRenderer, Audio::Manager & audioManager) noexcept;

			/**
			 * @brief Copy constructor.
			 * @param copy A reference to the copied instance.
			 */
			Manager (const Manager & copy) noexcept = delete;

			/**
			 * @brief Move constructor.
			 * @param copy A reference to the copied instance.
			 */
			Manager (Manager && copy) noexcept = delete;

			/**
			 * @brief Copy assignment.
			 * @param copy A reference to the copied instance.
			 * @return Manager &
			 */
			Manager & operator= (const Manager & copy) noexcept = delete;

			/**
			 * @brief Move assignment.
			 * @param copy A reference to the copied instance.
			 * @return Manager &
			 */
			Manager & operator= (Manager && copy) noexcept = delete;

			/**
			 * @brief Destructs the console.
			 */
			~Manager () override = default;

			/** @copydoc EmEn::Libs::ObservableTrait::classUID() const */
			[[nodiscard]]
			size_t
			classUID () const noexcept override
			{
				return ClassUID;
			}

			/** @copydoc EmEn::Libs::ObservableTrait::is() const */
			[[nodiscard]]
			bool
			is (size_t classUID) const noexcept override
			{
				return classUID == ClassUID;
			}

			/** @copydoc EmEn::ServiceInterface::usable() */
			[[nodiscard]]
			bool
			usable () const noexcept override
			{
				return m_flags[ServiceInitialized];
			}

			/**
			 * @brief Returns whether a scene exists under the name.
			 * @param sceneName A reference to a string.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			hasSceneNamed (const std::string & sceneName) const noexcept
			{
				return m_scenes.contains(sceneName);
			}

			/**
			 * @brief Creates a new scene.
			 * @note Will return nullptr on existing scene name !
			 * @param sceneName A reference to a string to name it.
			 * @param boundary The distance in all directions to limit the area.
			 * @param background A reference to a background smart pointer. Default autogenerated.
			 * @param sceneArea A reference to a sceneArea smart pointer. Default autogenerated.
			 * @param seaLevel A reference to a seaLevel smart pointer. Default none.
			 * @return std::shared_ptr< Scene >
			 */
			[[nodiscard]]
			std::shared_ptr< Scene > newScene (const std::string & sceneName, float boundary, const std::shared_ptr< Graphics::Renderable::AbstractBackground > & background = nullptr, const std::shared_ptr< Graphics::Renderable::SceneAreaInterface > & sceneArea = nullptr, const std::shared_ptr< Graphics::Renderable::SeaLevelInterface > & seaLevel = nullptr) noexcept;

			/**
			 * @brief Loads a scene from a scene definition in the resource store.
			 * @note If no problem found, the method Manager::loadScene(const std::shared_ptr< DefinitionResource > &) will handle the loading.
			 * @param resourceName The name of the resource.
			 * @return std::pair< std::shared_ptr< Scene >, std::shared_ptr< DefinitionResource > >
			 */
			[[nodiscard]]
			SceneLoading loadScene (const std::string & resourceName) noexcept;

			/**
			 * @brief Loads a scene from an external scene definition file. This file will be added in the resource store.
			 * @note If no problem found, the method Manager::loadScene(const std::shared_ptr< DefinitionResource > &) will handle the loading.
			 * @param filepath A reference to a filesystem path.
			 * @return std::pair< std::shared_ptr< Scene >, std::shared_ptr< DefinitionResource > >
			 */
			[[nodiscard]]
			SceneLoading loadScene (const std::filesystem::path & filepath) noexcept;

			/**
			 * @brief Loads a scene from a JSON definition.
			 * @param sceneDefinition A scene definition object.
			 * @return std::pair< std::shared_ptr< Scene >, std::shared_ptr< DefinitionResource > >
			 */
			[[nodiscard]]
			SceneLoading loadScene (const std::shared_ptr< DefinitionResource > & sceneDefinition) noexcept;

			/**
			 * @brief Launches the process to refresh all scenes.
			 * @return void
			 */
			void refreshScenes () const noexcept;

			/**
			 * @brief Disables and delete a scene.
			 * @param sceneName The scene name.
			 * @return bool
			 */
			bool deleteScene (const std::string & sceneName) noexcept;

			/**
			 * @brief Disables active scene and clear all scene.
			 * @return void
			 */
			void deleteAllScenes () noexcept;

			/**
			 * @brief Sets a scene as active.
			 * @note This function will check the completeness of the scene with Scene::check().
			 * It will register every notifier with all concerned services.
			 * @param scene A reference to a smart pointer of the scene.
			 * @return bool
			 */
			bool enableScene (const std::shared_ptr< Scene > & scene) noexcept;

			/**
			 * @brief Disables the active scene.
			 * @return bool
			 */
			bool disableActiveScene () noexcept;

			/**
			 * @brief Creates a list of available scene names.
			 * @return std::vector< std::string >
			 */
			[[nodiscard]]
			std::vector< std::string > getSceneNames () const noexcept;

			/**
			 * @brief Returns a scene from its name.
			 * @param sceneName A std::string for the name of the scene.
			 * @return std::shared_ptr< Scene >
			 */
			[[nodiscard]]
			std::shared_ptr< Scene > getScene (const std::string & sceneName) noexcept;

			/**
			 * @brief Returns a scene from its name.
			 * @param sceneName A std::string for the name of the scene.
			 * @return std::shared_ptr< const Scene >
			 */
			[[nodiscard]]
			std::shared_ptr< const Scene > getScene (const std::string & sceneName) const noexcept;

			/**
			 * @brief Returns whether the manager has an active scene.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			hasActiveScene () const noexcept
			{
				return m_activeScene != nullptr;
			}

			/**
			 * @brief Returns the active scene.
			 * @return std::shared_ptr< Scene >
			 */
			[[nodiscard]]
			std::shared_ptr< Scene >
			activeScene () noexcept
			{
				return m_activeScene;
			}

			/**
			 * @brief Returns the active scene access shared mutex.
			 * @return std::shared_mutex &
			 */
			[[nodiscard]]
			std::shared_mutex &
			activeSceneAccess () noexcept
			{
				return m_activeSceneAccess;
			}

		private:

			/** @copydoc EmEn::ServiceInterface::onInitialize() */
			bool onInitialize () noexcept override;

			/** @copydoc EmEn::ServiceInterface::onTerminate() */
			bool onTerminate () noexcept override;

			/** @copydoc EmEn::Console::Controllable::onRegisterToConsole. */
			void onRegisterToConsole () noexcept override;

			/* Flag names */
			static constexpr auto ServiceInitialized{0UL};

			PrimaryServices & m_primaryServices;
			Resources::Manager & m_resourceManager;
			Graphics::Renderer & m_graphicsRenderer;
			Audio::Manager & m_audioManager;
			std::map< std::string, std::shared_ptr< Scene > > m_scenes;
			std::shared_ptr< Scene > m_activeScene;
			std::shared_mutex m_activeSceneAccess;
			ConsoleMemory m_consoleMemory;
			std::array< bool, 8 > m_flags{
				false/*ServiceInitialized*/,
				false/*UNUSED*/,
				false/*UNUSED*/,
				false/*UNUSED*/,
				false/*UNUSED*/,
				false/*UNUSED*/,
				false/*UNUSED*/,
				false/*UNUSED*/
			};
	};
}
