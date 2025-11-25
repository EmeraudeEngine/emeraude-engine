/*
 * src/Overlay/Manager.hpp
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
#include <array>
#include <unordered_map>
#include <vector>
#include <memory>
#include <string>
#include <any>

/* Local inclusions for inheritances. */
#include "ServiceInterface.hpp"
#include "Libs/ObservableTrait.hpp"
#include "Input/KeyboardListenerInterface.hpp"
#include "Input/PointerListenerInterface.hpp"
#include "Libs/ObserverTrait.hpp"

/* Local inclusions for usages. */
#include "Input/Manager.hpp"
#include "FramebufferProperties.hpp"
#include "UIScreen.hpp"
#ifdef IMGUI_ENABLED
#include "imgui.h"
#include "ImGUIScreen.hpp"
#endif

/* Forward declarations. */
namespace EmEn::Graphics::Geometry
{
	class IndexedVertexResource;
}

namespace EmEn::Overlay
{
	/**
	 * @brief The overlay manager service class.
	 * @note [OBS][STATIC-OBSERVER][STATIC-OBSERVABLE]
	 * @extends EmEn::ServiceInterface This is a service.
	 * @extends EmEn::Libs::ObservableTrait This service is observable.
	 * @extends EmEn::Input::KeyboardListenerInterface The manager needs to listen to the keyboard.
	 * @extends EmEn::Input::PointerListenerInterface The manager needs to listen to the pointer.
	 * @extends EmEn::Libs::ObservableTrait This service observer.
	 */
	class Manager final : public ServiceInterface, public Libs::ObservableTrait, public Input::KeyboardListenerInterface, public Input::PointerListenerInterface, public Libs::ObserverTrait
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"OverlayManagerService"};

			enum NotificationCode
			{
				UIScreenCreated,
				UIScreenDestroying,
				UIScreenDestroyed,
				OverlayResized,
				MaxEnum
			};

			/**
			 * @brief Constructs an overlay manager.
			 * @param primaryServices A reference to primary services.
			 * @param window A reference to the window.
			 * @param graphicsRenderer A reference to the graphics renderer.
			 */
			Manager (PrimaryServices & primaryServices, Window & window, Graphics::Renderer & graphicsRenderer) noexcept
				: ServiceInterface{ClassId},
				KeyboardListenerInterface{false, true},
				PointerListenerInterface{false, false, true},
				m_primaryServices{primaryServices},
				m_window{window},
				m_graphicsRenderer{graphicsRenderer}
			{
				this->observe(&m_window);
			}

			/**
			 * @brief Returns the unique identifier for this class [Thread-safe].
			 * @return size_t
			 */
			static
			size_t
			getClassUID () noexcept
			{
				static const size_t classUID = EmEn::Libs::Hash::FNV1a(ClassId);

				return classUID;
			}

			/** @copydoc EmEn::Libs::ObservableTrait::classUID() const */
			[[nodiscard]]
			size_t
			classUID () const noexcept override
			{
				return getClassUID();
			}

			/** @copydoc EmEn::Libs::ObservableTrait::is() const */
			[[nodiscard]]
			bool
			is (size_t classUID) const noexcept override
			{
				return classUID == getClassUID();
			}

			/** @copydoc EmEn::Input::KeyboardListenerInterface::onKeyPress() */
			bool onKeyPress (int32_t key, int32_t scancode, int32_t modifiers, bool repeat) noexcept override;

			/** @copydoc EmEn::Input::KeyboardListenerInterface::onKeyRelease() */
			bool onKeyRelease (int32_t key, int32_t scancode, int32_t modifiers) noexcept override;

			/** @copydoc EmEn::Input::KeyboardListenerInterface::onCharacterType() */
			bool onCharacterType (uint32_t unicode) noexcept override;

			/** @copydoc EmEn::Input::PointerListenerInterface::onPointerMove() */
			bool onPointerMove (float positionX, float positionY) noexcept override;

			/** @copydoc EmEn::Input::PointerListenerInterface::onButtonPress() */
			bool onButtonPress (float positionX, float positionY, int32_t buttonNumber, int32_t modifiers) noexcept override;

			/** @copydoc EmEn::Input::PointerListenerInterface::onButtonRelease() */
			bool onButtonRelease (float positionX, float positionY, int32_t buttonNumber, int32_t modifiers) noexcept override;

			/** @copydoc EmEn::Input::PointerListenerInterface::onMouseWheel() */
			bool onMouseWheel (float positionX, float positionY, float xOffset, float yOffset, int32_t modifiers = 0) noexcept override;

			/**
			 * @brief Returns the reference to the primary services.
			 * @return PrimaryServices &
			 */
			[[nodiscard]]
			PrimaryServices &
			primaryServices () const noexcept
			{
				return m_primaryServices;
			}

			/**
			 * @brief Changes the master control state of overlaying.
			 * @param inputManager A reference to the input manager.
			 * @param state The state.
			 * @return void
			 */
			void enable (Input::Manager & inputManager, bool state) noexcept;

			/**
			 * @brief Returns whether the overlaying is enabled.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isEnabled () const noexcept
			{
				return m_enabled;
			}

			/**
			 * @brief Creates a new screen.
			 * @param name A reference to a string.
			 * @param enableKeyboardListener Enables the keyboard listener at creation.
			 * @param enablePointerListener Enables the pointer listener at creation.
			 * @return std::shared_ptr< UIScreen >
			 */
			[[nodiscard]]
			std::shared_ptr< UIScreen > createScreen (const std::string & name, bool enableKeyboardListener, bool enablePointerListener) noexcept;

#ifdef IMGUI_ENABLED
			/**
			 * @brief Creates an ImGUI screen.
			 * @param name A reference to a string.
			 * @param drawFunction A reference to a function.
			 * @return std::shared_ptr< ImGUIScreen >
			 */
			[[nodiscard]]
			std::shared_ptr< ImGUIScreen > createImGUIScreen (const std::string & name, const std::function< void () > & drawFunction) noexcept;
#endif

			/**
			 * @brief Destroys a named screen.
			 * @param name A reference to a string.
			 * @return bool
			 */
			bool destroyScreen (const std::string & name) noexcept;

			/**
			 * @brief Deletes all screens.
			 * @return void
			 */
			void clearScreens () noexcept;

			/**
			 * @brief Gets the named screen, and if it's found, it will be active on top.
			 * @param name A reference to a string.
			 * @return bool
			 */
			bool enableScreen (const std::string & name) noexcept;

			/**
			 * @brief Toggle screens activity.
			 * @param name A reference to a string.
			 * @return bool
			 */
			bool toggleScreen (const std::string & name) noexcept;

			/**
			 * @brief Disables a named active screen.
			 * @param name A reference to a string.
			 * @return bool
			 */
			bool disableScreen (const std::string & name) noexcept;

			/**
			 * @brief Disables all active screens.
			 * @return void
			 */
			void disableAllScreens () noexcept;

			/**
			 * @brief Makes a named screen on top and eventually disable all other screens.
			 * @param screenName A reference to a string.
			 * @return bool
			 */
			bool bringScreenOnTop (const std::string & screenName) noexcept;

			/**
			 * @brief Returns a list of screen names.
			 * @return std::vector< std::string >
			 */
			[[nodiscard]]
			std::vector< std::string > screensNameList () const noexcept;

			/**
			 * @brief Returns a list of active screen names.
			 * @return std::vector< std::string >
			 */
			[[nodiscard]]
			std::vector< std::string > activeScreensNameList () const noexcept;

			/**
			 * @brief Returns a named screen.
			 * @param screenName A reference to a string.
			 * @return std::shared_ptr< const Screen >
			 */
			[[nodiscard]]
			std::shared_ptr< const UIScreen > screen (const std::string & screenName) const noexcept;

			/**
			 * @brief Returns a named screen.
			 * @param screenName A reference to a string.
			 * @return std::shared_ptr< Screen >
			 */
			std::shared_ptr< UIScreen > screen (const std::string & screenName) noexcept;

			/**
			 * @brief Draws active screens over the 3D render.
			 * @param renderTarget A reference to a render target smart pointer.
			 * @param commandBuffer A reference to a command buffer.
			 * @return void
			 */
			void render (const std::shared_ptr< Graphics::RenderTarget::Abstract > & renderTarget, const Vulkan::CommandBuffer & commandBuffer) const noexcept;

			/**
			 * @brief Returns the framebuffer properties used to build the UI.
			 * @return const FramebufferProperties &
			 */
			[[nodiscard]]
			const FramebufferProperties &
			framebufferProperties () const noexcept
			{
				return m_framebufferProperties;
			}

			/**
			 * @brief Returns the surface geometry.
			 * @return std::shared_ptr< const Graphics::Geometry::IndexedVertexResource >
			 */
			[[nodiscard]]
			std::shared_ptr< const Graphics::Geometry::IndexedVertexResource >
			surfaceGeometry () const noexcept
			{
				return m_surfaceGeometry;
			}

			/**
			 * @brief Sets an exclusive screen to receive inputs.
			 * @param name A reference to a string.
			 * @return bool
			 */
			[[nodiscard]]
			bool setInputExclusiveScreen (const std::string & name) noexcept;

			/**
			 * @brief Disables a previous input exclusive screen.
			 * @return void
			 */
			void
			disableInputExclusiveScreen () noexcept
			{
				m_inputExclusiveScreen.reset();
			}

			/**
			 * @bries Returns whether an input exclusive screen has been set.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isInputExclusiveScreenEnabled () const noexcept
			{
				return m_inputExclusiveScreen != nullptr;
			}

			/**
			 * @brief Returns the screen set as input exclusive.
			 * @warning This can be nullptr.
			 * @return std::shared_ptr< UIScreen >
			 */
			[[nodiscard]]
			std::shared_ptr< UIScreen >
			inputExclusiveScreen () const noexcept
			{
				return m_inputExclusiveScreen;
			}

			/**
			 * @brief Processes pending surface updates for the current frame.
			 * @details This method is called every frame from the render loop. It handles:
			 * - Local surface changes (content updates, manual resize via setSize()/setGeometry())
			 * - GPU memory uploads for surfaces with outdated content
			 * @note Only processes VISIBLE screens for performance. Hidden screens are skipped
			 * and will be processed when they become visible again.
			 * @note This method is thread-safe and may skip processing if a window resize is in progress.
			 * @note On failure, the affected screen is automatically disabled (visibility set to false).
			 */
			void processFrameUpdates () noexcept;

			/**
			 * @brief Handles window resize by updating all overlay resources.
			 * @details This method is called when the window is resized. It performs:
			 * - Updates the shared FramebufferProperties with the new window dimensions
			 * - Recreates the overlay graphics pipeline for the new resolution
			 * - Forces ALL surfaces (visible or not) to recalculate their pixel dimensions
			 * - Notifies observers via OverlayResized notification
			 * @note Unlike processFrameUpdates(), this processes ALL screens regardless of visibility
			 * to ensure consistency when screens are shown later.
			 * @note Surfaces are NOT automatically swapped after resize. The back buffer is prepared
			 * with the new size, but the front buffer continues rendering until the application
			 * explicitly calls swapFramebuffers(). This allows asynchronous renderers (e.g., CEF)
			 * to prepare new content at the correct size before swapping.
			 * @return bool True if resize succeeded, false on critical failure.
			 */
			[[nodiscard]]
			bool onWindowResized () noexcept;

			/**
			 * @brief Gets or creates the descriptor set layout for this surface.
			 * @note The descriptor set layout is common for all the overlay.
			 * @warning Can be nullptr!
			 * @param layoutManager A reference to the layout manager.
			 * @return std::shared_ptr< Vulkan::DescriptorSetLayout >
			 */
			[[nodiscard]]
			static std::shared_ptr< Vulkan::DescriptorSetLayout > getDescriptorSetLayout (Vulkan::LayoutManager & layoutManager) noexcept;

		private:

			/** @copydoc EmEn::ServiceInterface::onInitialize() */
			bool onInitialize () noexcept override;

			/** @copydoc EmEn::ServiceInterface::onTerminate() */
			bool onTerminate () noexcept override;

			/** @copydoc EmEn::Libs::ObserverTrait::onNotification() */
			[[nodiscard]]
			bool onNotification (const ObservableTrait * observable, int notificationCode, const std::any & data) noexcept override;

			/**
			 * @brief Fetches framebuffer properties.
			 * @return void
			 */
			void updateFramebufferProperties () noexcept;

			/**
			 * @brief Generates the overlay shader program.
			 * @return bool
			 */
			[[nodiscard]]
			bool generateShaderProgram () noexcept;

#ifdef IMGUI_ENABLED

			/**
			* @brief Initializes the ImGUI library.
			* @return bool
			*/
			[[nodiscard]]
			bool initImGUI () noexcept;

			/**
			* @brief Releases the ImGUI library.
			* @return void
			*/
			void releaseImGUI () noexcept;

#endif

			PrimaryServices & m_primaryServices;
			Window & m_window;
			Graphics::Renderer & m_graphicsRenderer;
			FramebufferProperties m_framebufferProperties;
			std::shared_ptr< Graphics::Geometry::IndexedVertexResource > m_surfaceGeometry;
			std::shared_ptr< Saphir::Program > m_program;
			std::unordered_map< std::string, std::shared_ptr< UIScreen > > m_screens;
			std::shared_ptr< UIScreen > m_inputExclusiveScreen;
#ifdef IMGUI_ENABLED
			std::string m_iniFilepath;
			std::string m_logFilepath;
			std::shared_ptr< Vulkan::DescriptorPool > m_ImGUIDescriptorPool;
			std::unordered_map< std::string, std::shared_ptr< ImGUIScreen > > m_ImGUIScreens;
#endif
			mutable std::mutex m_physicalRepresentationUpdateMutex;
			mutable std::mutex m_screensAccess;
			bool m_enabled{false};
	};
}
