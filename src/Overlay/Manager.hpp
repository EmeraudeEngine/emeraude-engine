/*
 * src/Overlay/Manager.hpp
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
#include "emeraude_config.hpp" // IMGUI_ENABLED
#include "emeraude_export.hpp"

/* STL inclusions. */
#include <cstdint>
#include <cstddef>
#include <atomic>
#include <chrono>
#include <array>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>


/* Local inclusions for inheritances. */
#include "ServiceInterface.hpp"
#include "ObserverTrait.hpp"
#include "ObservableTrait.hpp"
#include "Input/KeyboardListenerInterface.hpp"
#include "Input/PointerListenerInterface.hpp"

/* Local inclusions for usages. */
#include "Input/Manager.hpp"
#include "FramebufferProperties.hpp"

/* Forward declarations. */
namespace EmEn
{
	namespace Graphics::RenderTarget
	{
		class Abstract;
	}
	namespace Vulkan
	{
		class LayoutManager;
		class DescriptorSetLayout;
		class CommandBuffer;
		class DescriptorPool;
	}

	namespace Resources
	{
		class Manager;
	}

	namespace Saphir
	{
		class Program;
	}

	namespace Graphics::Geometry
	{
		class IndexedVertexResource;
	}

	namespace Overlay
	{
		class UIScreen;
#ifdef IMGUI_ENABLED
		class ImGUIScreen;
#endif
	}
}
namespace EmEn::Saphir
{
	class Program;
}

namespace EmEn::Overlay
{
	/**
	 * @brief The overlay manager service class.
	 * @note [OBS][STATIC-OBSERVER][STATIC-OBSERVABLE]
	 * @extends EmEn::ServiceInterface This is a service.
	 * @extends EmEn::Base::ObserverTrait This service observe screens.
	 * @extends EmEn::Base::ObservableTrait This service is observable.
	 * @extends EmEn::Input::KeyboardListenerInterface The manager needs to listen to the keyboard.
	 * @extends EmEn::Input::PointerListenerInterface The manager needs to listen to the pointer.
	 */
	class EMEN_LEAN_API Manager final : public ServiceInterface, public Base::ObserverTrait, public Base::ObservableTrait, public Input::KeyboardListenerInterface, public Input::PointerListenerInterface
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"OverlayManagerService"};

			enum NotificationCode : std::uint8_t
			{
				UIScreenCreated,
				UIScreenDestroying,
				UIScreenDestroyed,
				OverlayResized,
				RedrawRequested, ///< The overlay changed visually (content, layout, visibility, order) and must be re-rendered.
				OverlayScaleChanged, ///< The HiDPI content scale changed (cross-monitor move / fractional-scale change). Payload: std::array< float, 2 >{scaleX, scaleY}. Fires in addition to OverlayResized.
				MaxEnum
			};

			/**
			 * @brief Constructs an overlay manager.
			 * @param primaryServices A reference to primary services.
			 * @param resourceManager A reference to the resource manager.
			 */
			Manager (PrimaryServices & primaryServices, Resources::Manager & resourceManager) noexcept;

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
			 * @brief Returns the reference to the resource manager.
			 * @return Resources::Manager &
			 */
			[[nodiscard]]
			Resources::Manager &
			resourceManager () const noexcept
			{
				return m_resourceManager;
			}

			/**
			 * @brief Changes the master control state of overlaying.
			 * @param inputManager A reference to the input manager.
			 * @param state The state.
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
			 * @brief Requests a re-render of the overlay.
			 * @details Emits the RedrawRequested notification so an observer (typically the engine
			 * Core) can wake its on-demand rendering thread. Called internally on every visual
			 * mutation: surface content/geometry/visibility/stack-order (via the requester propagated
			 * to screens and surfaces) and screen lifecycle/visibility. Harmless in continuous
			 * rendering (observers simply ignore it). May also be called by application code that
			 * changes the overlay in a way not covered by the built-in triggers.
			 * @note Never emitted under m_physicalRepresentationUpdateMutex (Ave Robustus II, CP.31/32; owner decision
			 * 2026-10-13; docs/caution-points.md § "Overlay lock order"): a request made on the thread running
			 * a representation pass (updateVideoMemory(), onWindowResized() — the surfaces' requester fires there) is
			 * recorded, and the pass emits ONE RedrawRequested after releasing the lock. A request from any other thread is
			 * emitted at once.
			 */
			void
			requestRedraw () noexcept
			{
				if ( m_representationPassThread.load() == std::this_thread::get_id() )
				{
					m_redrawDeferred = true;

					return;
				}

				this->notify(RedrawRequested);
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
			 * @note The screen notifications are emitted with no lock of the manager held.
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
			 */
			void
			disableInputExclusiveScreen () noexcept
			{
				const std::scoped_lock lock{m_screensAccess};

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
				const std::scoped_lock lock{m_screensAccess};

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
				const std::scoped_lock lock{m_screensAccess};

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
			void updateVideoMemory () noexcept;

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
			 * @note Main thread only (Core::onWindowChanged()). OverlayResized (the framebuffer size) and, on an overlay
			 * scale transition, OverlayScaleChanged (the overlay scale, the forced one included) are emitted AFTER
			 * m_physicalRepresentationUpdateMutex is released: an observer may call back into the overlay manager.
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

			/** @copydoc EmEn::Base::ObserverTrait::onNotification() */
			[[nodiscard]]
			bool onNotification (const ObservableTrait * observable, int notificationCode, const Base::Any & data) noexcept override;

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
			 * @brief Fetches framebuffer properties.
			 */
			void updateFramebufferProperties () noexcept;

			/**
			 * @brief Marks the calling thread as the one running a representation pass, for its scope.
			 * @details Declared right AFTER the m_physicalRepresentationUpdateMutex guard, so it ends before the lock is
			 * released. requestRedraw() on that thread defers instead of emitting; flushDeferredRedraw() emits after the
			 * lock. Non-owning: the manager outlives every pass it runs.
			 */
			class RepresentationPass final
			{
				public:

					/**
					 * @brief Marks the calling thread.
					 * @param passThread A reference to the manager's pass-thread slot.
					 */
					explicit
					RepresentationPass (std::atomic< std::thread::id > & passThread) noexcept
						: m_passThread{&passThread}
					{
						m_passThread->store(std::this_thread::get_id());
					}

					RepresentationPass (const RepresentationPass & copy) noexcept = delete;
					RepresentationPass (RepresentationPass && copy) noexcept = delete;
					RepresentationPass & operator= (const RepresentationPass & copy) noexcept = delete;
					RepresentationPass & operator= (RepresentationPass && copy) noexcept = delete;

					/**
					 * @brief Clears the mark.
					 */
					~RepresentationPass ()
					{
						m_passThread->store(std::thread::id{});
					}

				private:

					std::atomic< std::thread::id > * m_passThread;
			};

			/**
			 * @brief Emits the RedrawRequested deferred by a representation pass, once.
			 * @pre m_physicalRepresentationUpdateMutex is NOT held by the caller.
			 */
			void
			flushDeferredRedraw () noexcept
			{
				if ( m_redrawDeferred.exchange(false) )
				{
					this->notify(RedrawRequested);
				}
			}

			/** @brief What an input dispatch walks: a copy taken under m_screensAccess. */
			struct InputTargets
			{
				std::vector< std::shared_ptr< UIScreen > > screens;
				std::shared_ptr< UIScreen > exclusiveScreen;
			};

			/**
			 * @brief Copies the screens and the input exclusive screen under m_screensAccess.
			 * @details The input listeners walk this copy, never m_screens: the list changes from any thread
			 * (createScreen(), destroyScreen()), and a screen's input handler may call back into the manager.
			 * @return InputTargets
			 */
			[[nodiscard]]
			InputTargets inputTargets () const noexcept;

			/**
			 * @brief Finds a screen by name under m_screensAccess; the caller acts on it with the lock released.
			 * @param name A reference to the screen name.
			 * @return std::shared_ptr< UIScreen > nullptr when no screen has that name.
			 */
			[[nodiscard]]
			std::shared_ptr< UIScreen > findScreen (const std::string & name) const noexcept;

			/**
			 * @brief Removes a named screen from the list, only if it is still this very screen.
			 * @param name A reference to the screen name.
			 * @param screen A reference to the screen expected under that name.
			 * @return bool False when another caller removed or replaced it meanwhile.
			 */
			[[nodiscard]]
			bool eraseScreen (const std::string & name, const std::shared_ptr< UIScreen > & screen) noexcept;

			/**
			 * @brief Copies the screen pointers into m_screenSnapshot under m_screensAccess.
			 * @note The render-thread passes (updateVideoMemory(), onWindowResized(), dumpUploadStatistics()) iterate this
			 * copy: iterating m_screens itself raced with createScreen() / destroyScreen() on the main thread (a rehash or
			 * an erase during the iteration), and holding m_screensAccess during the updates would deadlock an observer
			 * that touches the screen list. The caller holds m_physicalRepresentationUpdateMutex.
			 */
			void snapshotScreens () noexcept;

			/** @brief Number of shader programs (2 alpha modes × 2 pixel formats = 4). */
			static constexpr size_t ProgramCount = 4;

			/**
			 * @brief Generates the overlay shader program.
			 * @param premultipliedAlpha Whether to use premultiplied alpha blending.
			 * @param isBGRASurface Whether the source pixels are in BGRA format.
			 * @return std::shared_ptr< Saphir::Program >
			 */
			[[nodiscard]]
			std::shared_ptr< Saphir::Program > generateShaderProgram (bool premultipliedAlpha, bool isBGRASurface) const noexcept;

#ifdef IMGUI_ENABLED

			/**
			* @brief Initializes the ImGUI library.
			* @return bool
			*/
			[[nodiscard]]
			bool initImGUI () noexcept;

			/**
			* @brief Releases the ImGUI library.
			*/
			void releaseImGUI () noexcept;

#endif

			PrimaryServices & m_primaryServices;
			Resources::Manager & m_resourceManager;
			FramebufferProperties m_framebufferProperties;
			std::shared_ptr< Graphics::Geometry::IndexedVertexResource > m_surfaceGeometry;
			std::array< std::shared_ptr< Saphir::Program >, ProgramCount > m_programs;
			std::unordered_map< std::string, std::shared_ptr< UIScreen > > m_screens;
			/* The screens being processed by a render-thread pass (snapshotScreens()), reused: no allocation per frame. */
			std::vector< std::shared_ptr< UIScreen > > m_screenSnapshot;
			std::shared_ptr< UIScreen > m_inputExclusiveScreen;
#ifdef IMGUI_ENABLED
			/* NOTE: These own the UTF-8 bytes that ImGuiIO::IniFilename / ::LogFilename point
			 * at. ImGui keeps the raw 'const char *' and dereferences it much later — up to
			 * ImGui::DestroyContext(), which saves the .ini — so the buffer MUST outlive the
			 * ImGui context. Never hand it a temporary, and never store a std::filesystem::path
			 * here: path::string() is ANSI-encoded on Windows whereas ImGui expects UTF-8. */
			std::string m_iniFilepath;
			std::string m_logFilepath;
			std::shared_ptr< Vulkan::DescriptorPool > m_ImGUIDescriptorPool;
			std::unordered_map< std::string, std::shared_ptr< ImGUIScreen > > m_ImGUIScreens;
#endif
			/* NOTE: Lock order: m_physicalRepresentationUpdateMutex THEN m_screensAccess (or both at once). */
			mutable std::mutex m_physicalRepresentationUpdateMutex;
			mutable std::mutex m_screensAccess;
			/* The overlay's main thread (the service is constructed there): the only one allowed to create, destroy and
			 * clear the screens, and their surfaces (UIScreen::isOwnerThread()). */
			std::thread::id m_ownerThread{std::this_thread::get_id()};
			/* The thread running a representation pass (none outside one), and a redraw requested during it. */
			std::atomic< std::thread::id > m_representationPassThread;
			std::atomic_bool m_redrawDeferred{false};
			bool m_enabled{false};
#ifdef IMGUI_ENABLED
			/** @brief False in a window-less run: ImGUI's platform backend needs a GLFW window. */
			bool m_ImGUIInitialized{false};
			/** @brief Whether an ImGUI window is under the pointer (io.WantCaptureMouse), latched by the render thread at
			 * each ImGUI frame (mutable: the const render pass latches it), read by the input thread: such a press or wheel
			 * event is consumed, it must not reach the scene or the editor behind the window (ImGUI reads the event through
			 * its own GLFW callbacks). */
			mutable std::atomic_bool m_ImGUICapturesPointer{false};
#endif

			/* NOTE: Diagnostic only, render thread. @see Surface::UploadStatistics and the setting
			 * Core/Video/Overlay/EnableUploadStatistics (read once at initialization). */
			bool m_uploadStatisticsEnabled{false};
			std::chrono::steady_clock::time_point m_lastUploadStatisticsDump;

			/**
			 * @brief Dumps and clears the per-surface GPU upload statistics, at most once per second.
			 * @note Render thread only, called at the end of updateVideoMemory().
			 */
			void dumpUploadStatistics () noexcept;
	};
}
