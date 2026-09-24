/*
 * src/Scenes/Manager.hpp
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
#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <cstddef>
#include <filesystem>
#include <map>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <string>
#include <utility>
#include <vector>

/* Local inclusions for inheritances. */
#include "ServiceInterface.hpp"
#include "Console/ControllableTrait.hpp"

/* Local inclusions for usages. */
#include "DefinitionResource.hpp"
#include "Editor/Manager.hpp"
#include "Graphics/Renderable/AbstractBackground.hpp"
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
	class Notifier;
}

namespace EmEn::Scenes::Loaders
{
	class Interface;
}

namespace EmEn::Scenes
{
	/**
	 * @brief Keeps scene targets for the console.
	 */
	class EMEN_API ConsoleMemory final
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
	 * @note [OBS][STATIC-OBSERVABLE]
	 * @extends EmEn::ServiceInterface This is a service.
	 * @extends EmEn::Base::ObservableTrait This service is observable.
	 * @extends EmEn::Console::ControllableTrait The scene manager service is usable from the console.
	 */
	class EMEN_LEAN_API Manager final : public ServiceInterface, public Base::ObservableTrait, public Console::ControllableTrait
	{
		public:

			using SceneLoading = std::pair< std::shared_ptr< Scene >, std::shared_ptr< DefinitionResource > >;

			/** @brief Class identifier. */
			static constexpr auto ClassId{"SceneManagerService"};

			static constexpr auto DefaultSceneBoundary{1000.0F};

			/** @brief Observable notification codes. */
			enum NotificationCode : std::uint8_t
			{
				/** @brief This event is fired when a new empty scene has been created. The scene smart pointer will be passed. */
				SceneCreated,
				/**
				 * @brief This event is fired when a scene is loaded from a file. The scene smart pointer will be passed.
				 * @note This event will come after a SceneCreated event. */
				SceneLoaded,
				/** @brief This event is fired when a scene has been destroyed or all scenes deleted. No data will be passed with it. */
				SceneDestroyed,
				/** @brief This event is fired when a scene becomes the active one. The scene smart pointer will be passed. */
				SceneEnabled,
				/** @brief This event is fired when a scene is disabled (not destroyed). The scene smart pointer will be passed. */
				SceneDisabled,
				/* Enumeration boundary. */
				MaxEnum
			};

			/**
			 * @brief Constructs the scene manager.
			 * @param primaryServices A reference to primary services.
			 * @param resourceManager A reference to the resource manager.
			 * @param inputManager A reference to the input manager.
			 * @param notifier A reference to the engine notifier.
			 */
			Manager (PrimaryServices & primaryServices, Resources::Manager & resourceManager, Input::Manager & inputManager, Notifier & notifier) noexcept
				: ServiceInterface{ClassId},
				ControllableTrait{ClassId},
				m_primaryServices{primaryServices},
				m_resourceManager{resourceManager},
				m_inputManager{inputManager},
				m_notifier{notifier}
			{

			}

			/**
			 * @brief Returns the unique identifier for this class [Thread-safe].
			 * @return size_t
			 */
			static
			size_t
			getClassUID () noexcept
			{
				return Base::Hash::FNV1a(ClassId);
			}

			/** @copydoc EmEn::Base::ObservableTrait::classUID() const */
			[[nodiscard]]
			size_t
			classUID () const noexcept override
			{
				return getClassUID();
			}

			/** @copydoc EmEn::Base::ObservableTrait::is() const */
			[[nodiscard]]
			bool
			is (size_t classUID) const noexcept override
			{
				return classUID == getClassUID();
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
			 * @note Will return nullptr on an existing scene with the same name!
			 * @param sceneName A reference to a string to name it.
			 * @param boundary The distance in all directions to limit the area.
			 * @param background A reference to a background smart pointer. Default autogenerated.
			 * @param groundLevel A reference to a ground interface smart pointer. Default autogenerated.
			 * @param seaLevel A reference to a sea level interface smart pointer. Default none.
			 * @return std::shared_ptr< Scene >
			 */
			[[nodiscard]]
			std::shared_ptr< Scene > newScene (const std::string & sceneName, float boundary, const std::shared_ptr< Graphics::Renderable::AbstractBackground > & background = nullptr, const std::shared_ptr< GroundLevelInterface > & groundLevel = nullptr, const std::shared_ptr< SeaLevelInterface > & seaLevel = nullptr) noexcept;

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
			 * @brief Loads a scene from a JSON string, builds it, and enables it.
			 * @param jsonString The JSON scene description.
			 * @param outputs Console outputs for feedback.
			 * @return bool
			 */
			bool loadSceneFromJson (const std::string & jsonString, Console::Outputs & outputs) noexcept;

			/**
			 * @brief Creates the scene loader able to read a composite asset file.
			 * @details Single dispatch point for external composite assets (glTF, FBX, USD, WAD, ...) :
			 * selects the loader by the file extension, case-insensitively, through
			 * Loaders::Interface::supportsExtension(). Loaders are cheap per-load objects,
			 * a new instance is returned on every call.
			 * @param filepath A reference to a filesystem path.
			 * @return std::unique_ptr< Loaders::Interface > The loader, or nullptr when no loader handles this file type.
			 */
			[[nodiscard]]
			std::unique_ptr< Loaders::Interface > createSceneLoader (const std::filesystem::path & filepath) const noexcept;

			/**
			 * @brief Disables and delete a scene.
			 * @param sceneName The scene name.
			 * @return bool
			 */
			bool deleteScene (const std::string & sceneName) noexcept;

			/**
			 * @brief Sets a scene as active.
			 * @note This function will check the completeness of the scene with Scene::check().
			 * It will register every notifier with all concerned services.
			 * @param scene A reference to a smart pointer of the scene.
			 * @return bool
			 */
			bool enableScene (const std::shared_ptr< Scene > & scene) noexcept;

			/**
			 * @brief Disables the active scene if one exists.
			 * @return bool Returns false if any scene was active.
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
			 * @brief Returns whether a scene is currently active [Thread-safe].
			 * @note Cheap alternative to withSharedActiveScene() when only the presence of an
			 * active scene is needed (e.g. the on-demand rendering gate in Core::renderingTask()).
			 * @return bool
			 */
			[[nodiscard]]
			bool
			hasActiveScene () const noexcept
			{
				this->waitForAnnouncedExclusiveAccesses();

				const std::shared_lock lock{m_activeSceneSharedAccess};

				return m_activeScene != nullptr;
			}

			/**
			 * @brief Executes a function on the active scene with thread-safe shared access.
			 * @note ⚠️ WRITER PREFERENCE (2026-09-24): a reader first waits for every ANNOUNCED exclusive
			 * access to finish (waitForAnnouncedExclusiveAccesses()). The render loop takes this access
			 * for a whole frame and again microseconds later; on MSVC `std::shared_mutex` is an SRWLOCK,
			 * neither fair nor FIFO, so it stole the lock back from the woken writer frame after frame —
			 * a scene deletion (the shutdown) waited 24 s to over 3 minutes on Windows with validation ON.
			 * @warning Never call it (or hasActiveScene()) from inside a shared or exclusive section on
			 * the same thread: a re-entrant acquisition was already undefined behaviour, and it is now a
			 * deadlock on every OS as soon as a writer is announced. The render loop exposes its scene to
			 * the overlay instead (Core::m_frameScene).
			 * @tparam function_t The type of function. Signature: void (const std::shared_ptr< Scene > &)
			 * @param processActiveScene A function to process the active scene.
			 * @param abortOnNullScene Do not trigger the function if there is no active scene.
			 * @return void
			 */
			template< typename function_t >
			void
			withSharedActiveScene (function_t && processActiveScene, bool abortOnNullScene) const noexcept requires (std::is_invocable_v< function_t, const std::shared_ptr< Scene > & >)
			{
				this->waitForAnnouncedExclusiveAccesses();

				const std::shared_lock lock{m_activeSceneSharedAccess};

				if ( abortOnNullScene && m_activeScene == nullptr )
				{
					return;
				}

				processActiveScene(m_activeScene);
			}

			/**
			 * @brief Executes a function on the active scene with thread-safe exclusive access.
			 * @tparam function_t The type of function. Signature: void (const std::shared_ptr< Scene > &)
			 * @param processActiveScene A function to process the active scene.
			 * @param abortOnNullScene Do not trigger the function if there is no active scene.
			 * @return void
			 */
			template< typename function_t >
			void
			withExclusiveActiveScene (function_t && processActiveScene, bool abortOnNullScene) const noexcept requires (std::is_invocable_v< function_t, const std::shared_ptr< Scene > & >)
			{
				/* Announced BEFORE queuing on the lock, withdrawn AFTER releasing it (reverse destruction order). */
				const ExclusiveAccessAnnouncement announcement{*this};
				const std::unique_lock lock{m_activeSceneSharedAccess};

				if ( abortOnNullScene && m_activeScene == nullptr )
				{
					return;
				}

				processActiveScene(m_activeScene);
			}

			/**
			 * @brief Toggles the scene editor mode on the active scene.
			 * @note The editor works in physical framebuffer pixels and reads the main
			 * render target extent itself; no viewport dimensions are needed.
			 * @return void
			 */
			void toggleEditorMode () noexcept;

			/**
			 * @brief Returns the scene editor manager.
			 * @return Editor::Manager &
			 */
			[[nodiscard]]
			Editor::Manager &
			editorManager () noexcept
			{
				return m_editorManager;
			}

			/**
			 * @brief Returns the scene editor manager.
			 * @return const Editor::Manager &
			 */
			[[nodiscard]]
			const Editor::Manager &
			editorManager () const noexcept
			{
				return m_editorManager;
			}

		private:

			/**
			 * @brief Announces an exclusive access to the active scene for its whole lifetime (writer preference).
			 * @note Construct it BEFORE the std::unique_lock on m_activeSceneSharedAccess, so that it is
			 * destroyed AFTER the lock is released: new readers wait from the moment the writer queues
			 * until it is done.
			 */
			class ExclusiveAccessAnnouncement final
			{
				public:

					/**
					 * @brief Announces the exclusive access.
					 * @param manager A reference to the scene manager.
					 */
					explicit
					ExclusiveAccessAnnouncement (const Manager & manager) noexcept
						: m_manager{manager}
					{
						m_manager.announceExclusiveAccess();
					}

					/**
					 * @brief Withdraws the announcement, and wakes the readers when it was the last one.
					 */
					~ExclusiveAccessAnnouncement ()
					{
						m_manager.withdrawExclusiveAccess();
					}

					ExclusiveAccessAnnouncement (const ExclusiveAccessAnnouncement & copy) noexcept = delete;
					ExclusiveAccessAnnouncement (ExclusiveAccessAnnouncement && copy) noexcept = delete;
					ExclusiveAccessAnnouncement & operator= (const ExclusiveAccessAnnouncement & copy) noexcept = delete;
					ExclusiveAccessAnnouncement & operator= (ExclusiveAccessAnnouncement && copy) noexcept = delete;

				private:

					const Manager & m_manager;
			};

			/**
			 * @brief Counts one more announced exclusive access.
			 * @return void
			 */
			void announceExclusiveAccess () const noexcept;

			/**
			 * @brief Counts one announced exclusive access less, and wakes the waiting readers at zero.
			 * @return void
			 */
			void withdrawExclusiveAccess () const noexcept;

			/**
			 * @brief Blocks a READER while an exclusive access is announced. Lock-free when none is.
			 * @return void
			 */
			void waitForAnnouncedExclusiveAccesses () const noexcept;

			/** @copydoc EmEn::ServiceInterface::onInitialize() */
			bool onInitialize () noexcept override;

			/** @copydoc EmEn::ServiceInterface::onTerminate() */
			bool onTerminate () noexcept override;

			/** @copydoc EmEn::Console::ControllableTrait::onRegisterToConsole. */
			void onRegisterToConsole () noexcept override;

			PrimaryServices & m_primaryServices;
			Resources::Manager & m_resourceManager;
			Input::Manager & m_inputManager;
			Notifier & m_notifier;
			std::map< std::string, std::shared_ptr< Scene > > m_scenes;
			std::shared_ptr< Scene > m_activeScene;
			ConsoleMemory m_consoleMemory;
			Editor::Manager m_editorManager{m_inputManager, m_resourceManager, m_notifier}; ///< Scene editor mode (picking, gizmo).
			/** @brief Handle the thread-safe access to the member 'm_scenes', when creating, adding, moving a scene. */
			mutable std::mutex m_sceneListAccess;
			/** @brief Handle a shared thread-safe access to the member 'm_activeScene', when manipulating the content of a scene. 'Readers' can share the access, while 'Writers' can have an exclusive lock.  */
			mutable std::shared_mutex m_activeSceneSharedAccess;
			/** @brief The writer-preference gate of 'm_activeSceneSharedAccess': the exclusive accesses announced and not
			 * finished, and what the readers wait on (see withSharedActiveScene()). */
			mutable std::atomic< uint32_t > m_announcedExclusiveAccesses{0};
			mutable std::mutex m_exclusiveAnnouncementAccess;
			mutable std::condition_variable m_exclusiveAccessesWithdrawn;
	};
}
