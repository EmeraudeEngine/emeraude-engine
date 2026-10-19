/*
 * src/Overlay/Manager.cpp
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

#include "Manager.hpp"

/* Project configuration. */
#include "emeraude_config.hpp"

/* STL inclusions. */
#include <algorithm>
#include <array>
#include <cmath>
#include <iterator>
#include <mutex>
#include <ranges>
#include <string>
#include <utility>
#include <vector>

/* Third-party inclusions. */
#ifdef IMGUI_ENABLED
#include "imgui.h"
#include "backends/imgui_impl_vulkan.h"
#include "backends/imgui_impl_glfw.h"

#include "IO/IO.hpp"
#include "Vulkan/Device.hpp"
#include "Vulkan/Instance.hpp"
#include "Vulkan/PhysicalDevice.hpp"
#include "Vulkan/Queue.hpp"
#include "Vulkan/DescriptorPool.hpp"
#include "Vulkan/Framebuffer.hpp"
#include "ImGUIScreen.hpp"
#endif

/* Local inclusions. */
#include "Graphics/Geometry/IndexedVertexResource.hpp"
#include "Graphics/Renderer.hpp"
#include "VertexFactory/ShapeGenerator.hpp"
#include "FileSystem.hpp"
#include "PrimaryServices.hpp"
#include "Resources/Manager.hpp"
#include "Saphir/Generator/OverlayRendering.hpp"
#include "Settings.hpp"
#include "SettingKeys.hpp"
#include "UIScreen.hpp"
#include "Vulkan/CommandBuffer.hpp"
#include "Vulkan/DescriptorSetLayout.hpp"
#include "Window.hpp"

namespace EmEn::Overlay
{
	using namespace Base;
	using namespace Base::Math;
	using namespace Base::VertexFactory;
	using namespace Saphir;
	using namespace Graphics;
	using namespace Vulkan;

	Manager::Manager (PrimaryServices & primaryServices, Resources::Manager & resourceManager) noexcept
		: ServiceInterface{ClassId},
		KeyboardListenerInterface{false, true},
		PointerListenerInterface{false, false, true},
		m_primaryServices{primaryServices},
		m_resourceManager{resourceManager}
	{
		m_redrawLink->manager.store(this);

		this->observe(&m_resourceManager.graphicsRenderer().window());
	}

	Manager::~Manager ()
	{
		this->severRedrawLink();
	}

	void
	Manager::severRedrawLink () noexcept
	{
		m_redrawLink->manager.store(nullptr);

		for ( auto inFlight = m_redrawLink->requestsInFlight.load(); inFlight != 0; inFlight = m_redrawLink->requestsInFlight.load() )
		{
			m_redrawLink->requestsInFlight.wait(inFlight);
		}
	}

	std::shared_ptr< Program >
	Manager::generateShaderProgram (bool premultipliedAlpha, bool isBGRASurface) const noexcept
	{
		auto & renderer = m_resourceManager.graphicsRenderer();

		Generator::OverlayRendering generator{renderer.mainRenderTarget(), m_surfaceGeometry, premultipliedAlpha, isBGRASurface};

		/* Use the post-process framebuffer for pipeline creation (single-sample overlay). */
		if ( const auto * overlayFB = renderer.overlayFramebuffer(); overlayFB != nullptr )
		{
			generator.setPipelineFramebuffer(overlayFB);
		}

		if ( !generator.generateShaderProgram(renderer) )
		{
			Tracer::error(ClassId, "Unable to generate the overlay manager shader program !");

			return nullptr;
		}

		return generator.shaderProgram();
	}

	void
	Manager::enable (Input::Manager & inputManager, bool state) noexcept
	{
		if ( !this->usable() )
		{
			Tracer::warning(ClassId, "The overlay manager is not available !");

			return;
		}

		if ( state )
		{
			m_keyboardRegistration = inputManager.addKeyboardListener(this);

			/* ⚠️⚠️ PRIORITY. The overlay is drawn ON TOP of the scene, so it must be offered the
			 * pointer first — input follows the visual stacking. Without it any scene registered
			 * afterwards is served before the interface and eats its clicks: the application menu
			 * went dead as soon as a model viewer existed, because that viewer gives the scene's
			 * orbit controller a node to drive and the controller then answers `true` instead of
			 * declining. It stayed unnoticed because a demo scene's controller has no node and
			 * declines, so the menu worked there. */
			m_pointerRegistration = inputManager.addPointerListener(this, true);
		}
		else
		{
			m_keyboardRegistration.reset();
			m_pointerRegistration.reset();
		}

		m_enabled = state;

		/* NOTE: Enabling/disabling the whole overlay changes what is rendered. */
		this->requestRedraw();
	}

	bool
	Manager::onInitialize () noexcept
	{
		m_surfaceGeometry = std::make_shared< Geometry::IndexedVertexResource >(m_resourceManager, "OverlayQuad", Geometry::EnablePrimaryTextureCoordinates);

		if ( !m_surfaceGeometry->load(ShapeGenerator::generateScreenQuad()) )
		{
			TraceError{ClassId} << "Unable to generate a geometry for UI surfaces !";

			m_surfaceGeometry.reset();

			return false;
		}

		/* Generate program variants for all combinations of alpha modes and pixel formats.
		 * Index layout:
		 *   0 = RGBA + standard alpha
		 *   1 = RGBA + premultiplied alpha
		 *   2 = BGRA + standard alpha
		 *   3 = BGRA + premultiplied alpha */
		for ( size_t index = 0; index < ProgramCount; ++index )
		{
			const bool premultipliedAlpha = (index & 0x1) != 0;
			const bool isBGRASurface = (index & 0x2) != 0;

			m_programs[index] = this->generateShaderProgram(premultipliedAlpha, isBGRASurface);

			if ( m_programs[index] == nullptr )
			{
				Tracer::error(ClassId, "Unable to generate an overlay manager shader program !");

				return false;
			}
		}

#ifdef IMGUI_ENABLED
		/* NOTE: ImGUI's platform backend IS a GLFW window (inputs, cursor, display size). A window-less run
		 * (--window-less) has none: initialising it there dereferenced a null GLFWwindow and killed the process
		 * without a line in the log (SIGSEGV in glfwSetWindowFocusCallback, found 2026-09-22 on the Intel iGPU
		 * bench). The ImGUI screens still exist; they are simply never drawn. */
		if ( m_resourceManager.graphicsRenderer().window().isWindowLessMode() )
		{
			TraceInfo{ClassId} << "Window-less mode: the ImGUI library is not initialized, ImGUI screens are not drawn.";
		}
		else if ( this->initImGUI() )
		{
			m_ImGUIInitialized = true;

			TraceSuccess{ClassId} << "ImGUI library initialized !";
		}
		else
		{
			TraceError{ClassId} << "Unable to initialize ImGUI library !";

			return false;
		}
#endif

		/* NOTE: Diagnostic, read once. @see Surface::UploadStatistics */
		m_uploadStatisticsEnabled = m_primaryServices.settings().getOrSetDefault< bool >(OverlayUploadStatisticsKey, DefaultOverlayUploadStatistics);
		m_lastUploadStatisticsDump = std::chrono::steady_clock::now();

		if ( m_uploadStatisticsEnabled )
		{
			TraceInfo{ClassId} << "Overlay GPU upload statistics enabled (" << OverlayUploadStatisticsKey << ").";
		}

		return true;
	}

	bool
	Manager::onTerminate () noexcept
	{
#ifdef IMGUI_ENABLED
		if ( m_ImGUIInitialized )
		{
			TraceInfo{ClassId} << "Releasing ImGUI library ...";

			this->releaseImGUI();

			m_ImGUIInitialized = false;
		}
#endif

		this->forget(&m_resourceManager.graphicsRenderer().window());

		/* NOTE: A screen or surface kept elsewhere (the notifier, the application) must not reach a terminated overlay. */
		this->severRedrawLink();

		/* NOTE: The input manager must not serve a terminated overlay (enable(false) is never called at the end). */
		m_keyboardRegistration.reset();
		m_pointerRegistration.reset();
		m_enabled = false;

		/* NOTE: Taken out under the lock, destroyed outside it: a screen going away may call back into this manager. */
		std::unordered_map< std::string, std::shared_ptr< UIScreen > > screens;

		{
			const Base::MutexLock lock{m_screensAccess};

			screens = std::exchange(m_screens, {});
		}

		screens.clear();

		for ( auto & program : m_programs )
		{
			program.reset();
		}

		m_surfaceGeometry.reset();

		return true;
	}

	std::shared_ptr< UIScreen >
	Manager::createScreen (const std::string & name, bool enableKeyboardListener, bool enablePointerListener) noexcept
	{
		if ( std::this_thread::get_id() != m_ownerThread )
		{
			TraceError{ClassId} << "Manager::createScreen() refused: the screens are created and destroyed on the overlay's main thread only.";

			return nullptr;
		}

		std::shared_ptr< UIScreen > screen;

		{
			const Base::MutexLock lock{m_screensAccess};

			if constexpr ( IsDebug )
			{
				if ( !m_framebufferProperties.isValid() )
				{
					TraceError{ClassId} << "The screen size are not initialized !";

					return nullptr;
				}
			}

			if ( m_screens.contains(name) )
			{
				TraceError{ClassId} << "An UI screen named '" << name << "' already exists !";

				return nullptr;
			}

			screen = std::make_shared< UIScreen >(
				name,
				m_framebufferProperties,
				m_resourceManager.graphicsRenderer(),
				enableKeyboardListener,
				enablePointerListener,
				m_ownerThread
			);

			/* NOTE: Propagate the redraw requester so this screen and every surface it creates can
			 * wake the on-demand rendering thread on any visual mutation. No-op in continuous rendering. */
			screen->setRedrawRequester([weakLink = std::weak_ptr< RedrawLink >{m_redrawLink}] {
				const auto redrawLink = weakLink.lock();

				if ( redrawLink == nullptr )
				{
					return;
				}

				/* NOTE: Counted in flight BEFORE the manager is read: severRedrawLink() then waits for this request. */
				redrawLink->requestsInFlight.fetch_add(1);

				if ( auto * manager = redrawLink->manager.load(); manager != nullptr )
				{
					manager->requestRedraw();
				}

				if ( redrawLink->requestsInFlight.fetch_sub(1) == 1 )
				{
					redrawLink->requestsInFlight.notify_all();
				}
			});

			m_screens[name] = screen;
		}

		/* NOTE: Notified with m_screensAccess released (Ave Robustus II, CP.31/32): an observer that calls back into the
		 * manager (a representation pass takes the lock in snapshotScreens()) self-deadlocked here — found by the overlay
		 * stress, 2026-10-13. */
		this->notify(UIScreenCreated, screen);

		/* NOTE: A new screen changes the composited image. */
		this->requestRedraw();

		return screen;
	}

#ifdef IMGUI_ENABLED
	std::shared_ptr< ImGUIScreen >
	Manager::createImGUIScreen (const std::string & name, const std::function< void () > & drawFunction) noexcept
	{
		/* NOTE: render() reads m_ImGUIScreens under this mutex on the render thread. */
		const Base::MutexLock lock{m_screensAccess};

		if ( m_ImGUIScreens.contains(name) )
		{
			TraceError{ClassId} << "An ImGUI screen named '" << name << "' already exists !";

			return nullptr;
		}

		auto screen = std::make_shared< ImGUIScreen >(name, drawFunction);

		m_ImGUIScreens[name] = screen;

		return screen;
	}
#endif

	bool
	Manager::destroyScreen (const std::string & name) noexcept
	{
		if ( std::this_thread::get_id() != m_ownerThread )
		{
			TraceError{ClassId} << "Manager::destroyScreen() refused: the screens are created and destroyed on the overlay's main thread only.";

			return false;
		}

		/* NOTE: The notifications are emitted with m_screensAccess released (see createScreen()); UIScreenDestroying still
		 * comes BEFORE the screen leaves the list, UIScreenDestroyed after. */
		std::shared_ptr< UIScreen > screen;

		{
			const Base::MutexLock lock{m_screensAccess};

			const auto screenIt = m_screens.find(name);

			if ( screenIt == m_screens.end() )
			{
				TraceError{ClassId} << "Unable to find '" << name << "' UI screen to erase it !";

				return false;
			}

			screen = screenIt->second;
		}

		this->notify(UIScreenDestroying, screen);

		if ( !this->eraseScreen(name, screen) )
		{
			TraceWarning{ClassId} << "The UI screen '" << name << "' was destroyed by another caller meanwhile.";

			return false;
		}

		this->notify(UIScreenDestroyed, name);

		/* NOTE: Removing a screen changes the composited image. */
		this->requestRedraw();

		return true;
	}

	Manager::InputTargets
	Manager::inputTargets () const noexcept
	{
		const Base::MutexLock lock{m_screensAccess};

		InputTargets targets;
		targets.screens.reserve(m_screens.size());

		for ( const auto & screen : m_screens | std::views::values )
		{
			targets.screens.push_back(screen);
		}

		targets.exclusiveScreen = m_inputExclusiveScreen;

		return targets;
	}

	std::shared_ptr< UIScreen >
	Manager::findScreen (const std::string & name) const noexcept
	{
		const Base::MutexLock lock{m_screensAccess};

		const auto screenIt = m_screens.find(name);

		return screenIt == m_screens.cend() ? nullptr : screenIt->second;
	}

	bool
	Manager::eraseScreen (const std::string & name, const std::shared_ptr< UIScreen > & screen) noexcept
	{
		const Base::MutexLock lock{m_screensAccess};

		const auto screenIt = m_screens.find(name);

		/* NOTE: Only this very screen: another caller may have destroyed it, or replaced it under the same name, while the
		 * lock was released. */
		if ( screenIt == m_screens.end() || screenIt->second != screen )
		{
			return false;
		}

		m_screens.erase(screenIt);

		return true;
	}

	void
	Manager::clearScreens () noexcept
	{
		if ( std::this_thread::get_id() != m_ownerThread )
		{
			TraceError{ClassId} << "Manager::clearScreens() refused: the screens are created and destroyed on the overlay's main thread only.";

			return;
		}

		/* NOTE: A copy of the list, then one screen at a time with m_screensAccess released around the notifications (see
		 * createScreen() and destroyScreen()). A cold path: the copy allocates once. */
		std::vector< std::pair< std::string, std::shared_ptr< UIScreen > > > screens;

		{
			const Base::MutexLock lock{m_screensAccess};

			screens.assign(m_screens.begin(), m_screens.end());
		}

		for ( const auto & [name, screen] : screens )
		{
			this->notify(UIScreenDestroying, screen);

			if ( this->eraseScreen(name, screen) )
			{
				this->notify(UIScreenDestroyed, name);
			}
		}

		/* NOTE: The overlay screen set changed: request a redraw (on-demand rendering). */
		this->requestRedraw();
	}

	bool
	Manager::enableScreen (const std::string & name) noexcept
	{
		/* NOTE: Found under m_screensAccess, acted on with it released: the visibility change requests a redraw. */
		const auto screen = this->findScreen(name);

		if ( screen == nullptr )
		{
			TraceWarning{ClassId} << "Unable to find the UI screen '" << name << "' to activate it !";

			return false;
		}

		screen->setVisibility(true);

		return true;
	}

	bool
	Manager::toggleScreen (const std::string & name) noexcept
	{
		/* NOTE: Found under m_screensAccess, acted on with it released: the visibility change requests a redraw. */
		const auto screen = this->findScreen(name);

		if ( screen == nullptr )
		{
			TraceWarning{ClassId} << "Unable to find the UI screen '" << name << "' to toggle it !";

			return false;
		}

		screen->setVisibility(!screen->isVisible());

		return true;
	}

	bool
	Manager::disableScreen (const std::string & name) noexcept
	{
		/* NOTE: Found under m_screensAccess, acted on with it released: the visibility change requests a redraw. */
		const auto screen = this->findScreen(name);

		if ( screen == nullptr )
		{
			TraceWarning{ClassId} << "Unable to find the UI screen '" << name << "' to disable it !";

			return false;
		}

		screen->setVisibility(false);

		return true;
	}

	void
	Manager::disableAllScreens () noexcept
	{
		/* NOTE: A copy, acted on with m_screensAccess released (each change requests a redraw). */
		const auto targets = this->inputTargets();

		for ( const auto & screen : targets.screens )
		{
			screen->setVisibility(false);
		}
	}

	bool
	Manager::bringScreenOnTop (const std::string & screenName) noexcept
	{
		if ( this->findScreen(screenName) == nullptr )
		{
			TraceWarning{ClassId} << "Unable to find the UI screen '" << screenName << "' to bring it on top !";

			return false;
		}

		// TODO : Re-order the map.

		return true;
	}

	std::vector< std::string >
	Manager::screensNameList () const noexcept
	{
		const Base::MutexLock lock{m_screensAccess};

		std::vector< std::string > screenNames;
		screenNames.reserve(m_screens.size());

		std::ranges::transform(m_screens, std::back_inserter(screenNames), [] (const auto & screenIt) {
			return screenIt.first;
		});

		return screenNames;
	}

	std::vector< std::string >
	Manager::activeScreensNameList () const noexcept
	{
		const Base::MutexLock lock{m_screensAccess};

		std::vector< std::string > screenNames;
		screenNames.reserve(m_screens.size());

		for ( const auto & [screenName, screen] : m_screens )
		{
			if ( screen->isVisible() )
			{
				screenNames.emplace_back(screenName);
			}
		}

		return screenNames;
	}

	std::shared_ptr< const UIScreen >
	Manager::screen (const std::string & screenName) const noexcept
	{
		auto screen = this->findScreen(screenName);

		if ( screen == nullptr )
		{
			TraceWarning{ClassId} << "There is no screen named '" << screenName << "' !";
		}

		return screen;
	}

	std::shared_ptr< UIScreen >
	Manager::screen (const std::string & screenName) noexcept
	{
		auto screen = this->findScreen(screenName);

		if ( screen == nullptr )
		{
			TraceWarning{ClassId} << "There is no screen named '" << screenName << "' !";
		}

		return screen;
	}

	void
	Manager::updateFramebufferProperties () noexcept
	{
		const auto & windowState = m_resourceManager.graphicsRenderer().window().state();

		if ( auto & settings = m_primaryServices.settings(); settings.getOrSetDefault< bool >(OverlayForceScaleKey, DefaultOverlayForceScale) )
		{
			m_framebufferProperties.updateProperties(
				windowState.framebufferWidth,
				windowState.framebufferHeight,
				settings.getOrSetDefault< float >(OverlayScaleXKey, DefaultOverlayScale),
				settings.getOrSetDefault< float >(OverlayScaleYKey, DefaultOverlayScale)
			);
		}
		else
		{
			m_framebufferProperties.updateProperties(
				windowState.framebufferWidth,
				windowState.framebufferHeight,
				windowState.contentXScale,
				windowState.contentYScale
			);
		}
	}

	void
	Manager::updateVideoMemory () noexcept
	{
		{
			/* NOTE: This can collide with the window resize event from Manager::onWindowResized() in another thread. */
			const Base::MutexLock lock{m_physicalRepresentationUpdateMutex};
			const RepresentationPass pass{m_representationPassThread};

			if ( this->isEnabled() )
			{
				this->snapshotScreens();

				/* NOTE: Only process VISIBLE screens for performance.
				 * Hidden screens will be processed when they become visible again. */
				for ( const auto & screen : m_screenSnapshot )
				{
					if ( screen->empty() || !screen->isVisible() )
					{
						continue;
					}

					if ( !screen->processSurfaceUpdates(false) )
					{
						TraceError{ClassId} << "Failed to process screen '" << screen->name() << "' updates!";
					}
				}

				this->dumpUploadStatistics();

				/* NOTE: Released now, so a screen destroyed meanwhile does not outlive this pass. */
				m_screenSnapshot.clear();
			}
		}

		/* NOTE: The redraw requests of the surfaces processed above, emitted once, the lock released. */
		this->flushDeferredRedraw();
	}

	void
	Manager::snapshotScreens () noexcept
	{
		const Base::MutexLock lock{m_screensAccess};

		m_screenSnapshot.clear();

		for ( const auto & screen : m_screens | std::views::values )
		{
			m_screenSnapshot.push_back(screen);
		}
	}

	void
	Manager::dumpUploadStatistics () noexcept
	{
		if ( !m_uploadStatisticsEnabled )
		{
			return;
		}

		const auto now = std::chrono::steady_clock::now();
		const auto elapsed = std::chrono::duration_cast< std::chrono::milliseconds >(now - m_lastUploadStatisticsDump);

		if ( elapsed < std::chrono::seconds{1} )
		{
			return;
		}

		m_lastUploadStatisticsDump = now;

		const auto elapsedSeconds = static_cast< double >(elapsed.count()) / 1000.0;

		for ( const auto & screen : m_screenSnapshot )
		{
			for ( const auto & surface : screen->surfaces() )
			{
				const auto stats = surface->uploadStatistics();

				surface->resetUploadStatistics();

				if ( stats.uploadCount == 0 )
				{
					continue;
				}

				constexpr auto MiB{1024.0 * 1024.0};

				const auto movedMiB = static_cast< double >(stats.uploadedBytes) / MiB;
				const auto fullMiB = static_cast< double >(stats.fullBytes) / MiB;
				const auto bandMiB = static_cast< double >(stats.bandBytes) / MiB;
				const auto regionMiB = static_cast< double >(stats.regionBytes) / MiB;
				const auto writeMS = static_cast< double >(stats.writeDurationUS) / 1000.0;

				/* NOTE: 'realised' is what the row-band upload actually saved against the former
				 * full-image behaviour; 'ceiling' is what a tight bounding-box upload would reach.
				 * Both are >= 1.0; a realised gain of 1.0 means every upload took the full path. */
				const auto realisedGain = stats.uploadedBytes > 0 ? static_cast< double >(stats.fullBytes) / static_cast< double >(stats.uploadedBytes) : 1.0;
				const auto ceilingGain = stats.regionBytes > 0 ? static_cast< double >(stats.fullBytes) / static_cast< double >(stats.regionBytes) : 1.0;

				TraceInfo{ClassId} <<
					"[UPLOAD-STATS] '" << surface->name() << "' over " << elapsedSeconds << " s : " <<
					stats.uploadCount << " uploads (" << static_cast< double >(stats.uploadCount) / elapsedSeconds << "/s), " <<
					stats.partialCount << " partial ; " <<
					"moved " << movedMiB << " MiB (" << movedMiB / elapsedSeconds << " MiB/s) where a full upload was " << fullMiB << " MiB "
					"=> REALISED GAIN x" << realisedGain << " ; " <<
					"upload " << writeMS << " ms (" << writeMS / elapsedSeconds << " ms/s) ; "
					"row-band reference " << bandMiB << " MiB, bounding-box " << regionMiB << " MiB (ceiling x" << ceilingGain << ") ; " <<
					stats.saturatedCount << " full-surface, " << stats.unknownRegionCount << " region-less.";
			}
		}
	}

	bool
	Manager::onWindowResized () noexcept
	{
		/* NOTE: Called on the main thread only (Core::onWindowChanged()). The observers are notified AFTER the
		 * representation lock is released (Ave Robustus II, CP.31/32): an observer that calls back into the overlay
		 * (a render request ending in render() or updateVideoMemory(), a surface update) would otherwise self-deadlock on
		 * the non-recursive mutex, or wait on the render thread. The payloads are copies taken under the lock. */
		std::array< uint32_t, 2 > framebufferSize{0, 0};
		std::array< float, 2 > contentScale{1.0F, 1.0F};
		bool scaleChanged = false;

		{
			/* NOTE: This can collide with the successive call to Manager::processFrameUpdates() in the rendering loop. */
			const Base::MutexLock lock{m_physicalRepresentationUpdateMutex};
			const RepresentationPass pass{m_representationPassThread};

			/* Step 1: Update shared framebuffer properties with new window dimensions.
			 * Capture the previous content scale first so we can emit OverlayScaleChanged only on a real
			 * HiDPI-scale transition (cross-monitor move / fractional-scale change), separately from the
			 * always-emitted OverlayResized (which fires on any size change). */
			const auto previousScaleX = m_framebufferProperties.screenScaleX();
			const auto previousScaleY = m_framebufferProperties.screenScaleY();

			this->updateFramebufferProperties();

			/* Step 2: Force ALL screens (visible or not) to recalculate their pixel dimensions.
			 * The 'true' parameter triggers invalidate() on all surfaces, causing them to
			 * recreate their transition buffers at the new size.
			 * NOTE: Surfaces are NOT automatically committed. The active buffer continues to render
			 * with the old content/size until the application explicitly calls commitTransitionBuffer().
			 * This allows asynchronous renderers (e.g., CEF) to prepare new content before committing. */
			this->snapshotScreens();

			for ( const auto & screen : m_screenSnapshot )
			{
				if ( screen->empty() )
				{
					continue;
				}

				if ( !screen->processSurfaceUpdates(true) )
				{
					TraceError{ClassId} << "Failed to process screen '" << screen->name() << "' updates!";
				}

				TraceDebug{ClassId} << "The screen '" << screen->name() << "' resized.";
			}

			m_screenSnapshot.clear();

			/* Step 3: Copy the notification payloads. The scale transition is the overlay's own one: with a forced
			 * scale (OverlayForceScaleKey) the window content scale differs from the overlay scale on every resize, and
			 * comparing against it emitted OverlayScaleChanged each time. */
			const auto & windowState = m_resourceManager.graphicsRenderer().window().state();

			framebufferSize = {windowState.framebufferWidth, windowState.framebufferHeight};
			contentScale = {m_framebufferProperties.screenScaleX(), m_framebufferProperties.screenScaleY()};
			scaleChanged = std::abs(contentScale[0] - previousScaleX) > 0.001F || std::abs(contentScale[1] - previousScaleY) > 0.001F;
		}

		/* Step 4: Notify observers of the resize completion, the lock released (the surfaces' redraw requests first). */
		this->flushDeferredRedraw();

		this->notify(OverlayResized, framebufferSize);

		/* Step 5: On a real HiDPI content-scale transition, emit OverlayScaleChanged in addition to
		 * OverlayResized. Observers that must react to scale specifically (e.g. an OSR web-view latch)
		 * can key on this single signal instead of caching and diffing the scale themselves. */
		if ( scaleChanged )
		{
			this->notify(OverlayScaleChanged, contentScale);
		}

		TraceDebug{ClassId} <<
			"The overlay manager received last windows properties. " "\n"
			"Width: " << framebufferSize[0] << "\n"
			"Height: " << framebufferSize[1] << "\n"
			"ScaleX: " << contentScale[0] << "\n"
			"ScaleY: " << contentScale[1] << "\n";

		return true;
	}

	void
	Manager::render (const std::shared_ptr< RenderTarget::Abstract > & renderTarget, const CommandBuffer & commandBuffer) const noexcept
	{
		/* NOTE: The lock order of the overlay is m_physicalRepresentationUpdateMutex THEN m_screensAccess (onWindowResized()
		 * and updateVideoMemory() take them so). This took the screens first and the representation lock further down:
		 * a resize on the window's thread during a frame could deadlock (lock-order inversion found by ThreadSanitizer,
		 * 2026-10-12). std::scoped_lock takes both without an order of its own. */
		const Base::MutexPairLock lock{m_physicalRepresentationUpdateMutex, m_screensAccess};

		/* Check if the overlay is enabled and there is something to render.
		 * NOTE: ImGUI screens live in a separate container, so an ImGUI-only
		 * overlay (no UIScreen) must still be rendered. */
#ifdef IMGUI_ENABLED
		const bool nothingToRender = m_screens.empty() && m_ImGUIScreens.empty();
#else
		const bool nothingToRender = m_screens.empty();
#endif

		if ( !this->isEnabled() || nothingToRender )
		{
			return;
		}

		if constexpr ( IsDebug )
		{
			if ( m_surfaceGeometry == nullptr || !m_surfaceGeometry->isCreated() )
			{
				TraceError{ClassId} << "The surface geometry is no ready !";

				return;
			}
		}

		for ( const auto & screen : m_screens | std::views::values )
		{
			if ( screen->empty() || !screen->isVisible() )
			{
				continue;
			}

			/* NOTE: Select the right program based on alpha blending mode and pixel format.
			 * Index layout: bit 0 = premultiplied alpha, bit 1 = BGRA source. */
			const size_t programIndex = (screen->premultipliedAlpha() ? 1 : 0) | (screen->isUsingBGRAFormat() ? 2 : 0);
			const auto & program = m_programs[programIndex];

			/* Bind the graphics pipeline. */
			commandBuffer.bind(*program->graphicsPipeline());

			/* NOTE: Set dynamic viewport and scissor based on current render target extent. */
			{
				const auto & extent3D = renderTarget->extent();

				const VkViewport viewport{
					.x = 0.0F,
					.y = 0.0F,
					.width = static_cast< float >(extent3D.width),
					.height = static_cast< float >(extent3D.height),
					.minDepth = 0.0F,
					.maxDepth = 1.0F
				};
				vkCmdSetViewport(commandBuffer.handle(), 0, 1, &viewport);

				const VkRect2D scissor{
					.offset = {
						.x = 0,
						.y = 0
					},
					.extent = {
						.width = extent3D.width,
						.height = extent3D.height
					}
				};
				vkCmdSetScissor(commandBuffer.handle(), 0, 1, &scissor);
			}

			/* Bind the geometry VBO and the optional IBO. */
			commandBuffer.bind(*m_surfaceGeometry, 0);

			const auto pipelineLayout = program->pipelineLayout();

			screen->render(renderTarget, commandBuffer, *pipelineLayout, *m_surfaceGeometry);
		}

#ifdef IMGUI_ENABLED
		/* NOTE: ImGUI relies on a single global context, so exactly one
		 * NewFrame()/Render() cycle is allowed per frame. We open the frame
		 * once, let every visible ImGUI screen emit its widgets, then submit
		 * all the draw data in a single pass. */
		const bool hasVisibleImGUIScreen = std::ranges::any_of(m_ImGUIScreens | std::views::values, [] (const auto & screen) {
			return screen->isVisible();
		});

		if ( m_ImGUIInitialized && hasVisibleImGUIScreen )
		{
			ImGui_ImplVulkan_NewFrame();
			ImGui_ImplGlfw_NewFrame();
			ImGui::NewFrame();

			/* Computed by NewFrame() from the windows the previous frame drew. */
			m_ImGUICapturesPointer = ImGui::GetIO().WantCaptureMouse;

			for ( const auto & screen : m_ImGUIScreens | std::views::values )
			{
				if ( screen->isVisible() )
				{
					screen->draw();
				}
			}

			ImGui::Render();

			auto * drawData = ImGui::GetDrawData();

			/* ⚠️ ImGUI 1.92 (ImGuiBackendFlags_RendererHasTextures) uploads its textures — the font atlas on the first
			 * frame, then whenever glyphs are added — INSIDE RenderDrawData(), with its own vkQueueSubmit() +
			 * vkQueueWaitIdle() on the graphics queue it was given, outside the device lock every engine queue access
			 * takes (Vulkan/Queue.cpp). On a single queue family (Apple GPUs) the loaders' uploads share that queue:
			 * UNASSIGNED-Threading-MultipleThreads-Write (macOS, 2026-09-29, the editor panel's first frame racing the
			 * gizmo uploads). The pending textures are therefore updated HERE, under that lock; RenderDrawData() skips
			 * a texture whose status is already OK. */
			if ( drawData != nullptr && drawData->Textures != nullptr )
			{
				for ( auto * texture : *drawData->Textures )
				{
					if ( texture->Status != ImTextureStatus_OK )
					{
						const auto deviceLock = m_ImGUIDescriptorPool->device()->lockAccess();

						ImGui_ImplVulkan_UpdateTexture(texture);
					}
				}
			}

			ImGui_ImplVulkan_RenderDrawData(drawData, commandBuffer.handle());
		}
		else
		{
			m_ImGUICapturesPointer = false;
		}
#endif
	}

	bool
	Manager::onNotification (const ObservableTrait * observable, int notificationCode, const Base::Any & /*data*/) noexcept
	{
		if ( observable->is(Window::getClassUID()) )
		{
			/* NOTE: On the window creation, we use the initial size to set the overlay physical size. */
			if ( notificationCode == Window::Created )
			{
				this->updateFramebufferProperties();

				return true;
			}

			if constexpr ( ObserverDebugEnabled )
			{
				TraceDebug{ClassId} << "Event #" << notificationCode << " from the window ignored.";
			}

			return true;
		}

		/* NOTE: Don't know what it is, goodbye! */
		TraceDebug{ClassId} <<
			"Received an unhandled notification (Code:" << notificationCode << ") from observable (UID:" << observable->classUID() << ")  ! "
			"Forgetting it ...";

		return false;
	}

	std::shared_ptr< DescriptorSetLayout >
	Manager::getDescriptorSetLayout (LayoutManager & layoutManager) noexcept
	{
		auto descriptorSetLayout = layoutManager.getDescriptorSetLayout(ClassId);

		if ( descriptorSetLayout == nullptr )
		{
			descriptorSetLayout = layoutManager.prepareNewDescriptorSetLayout(ClassId);
			descriptorSetLayout->setIdentifier(ClassId, ClassId, "DescriptorSetLayout");

			descriptorSetLayout->declareCombinedImageSampler(0, VK_SHADER_STAGE_FRAGMENT_BIT);

			if ( !layoutManager.createDescriptorSetLayout(descriptorSetLayout) )
			{
				return nullptr;
			}
		}

		return descriptorSetLayout;
	}

	bool
	Manager::setInputExclusiveScreen (const std::string & name) noexcept
	{
		const auto screen = this->screen(name);

		if ( screen == nullptr )
		{
			return false;
		}

		const Base::MutexLock lock{m_screensAccess};

		m_inputExclusiveScreen = screen;

		return true;
	}

#ifdef IMGUI_ENABLED

	bool
	Manager::initImGUI () noexcept
	{
		auto & graphicsRenderer = m_resourceManager.graphicsRenderer();

		if ( !graphicsRenderer.usable() )
		{
			TraceError{ClassId} << "No Vulkan graphics layer !";

			return false;
		}

		const auto & filesystem = m_primaryServices.fileSystem();

		/* NOTE: ImGui opens these itself with a UTF-8 → wide conversion on Windows
		 * (ImFileOpen()), so the paths must be UTF-8 and not the ANSI code page that
		 * std::filesystem::path::string() would yield there. */
		m_iniFilepath = IO::toU8String(filesystem.configDirectory("imgui.ini"));
		m_logFilepath = IO::toU8String(filesystem.cacheDirectory("imgui_log.txt"));

		/* NOTE: Initialize ImGUI library. */
		{
			IMGUI_CHECKVERSION();

			ImGui::CreateContext();

			ImGuiIO & io = ImGui::GetIO(); (void)io;
			io.IniFilename = m_iniFilepath.data();
			io.LogFilename = m_logFilepath.data();
			io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
			io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;

			// Setup Dear ImGui style
			ImGui::StyleColorsDark();
			//ImGui::StyleColorsClassic();
		}

		/* NOTE: Initialize ImGUI with GLFW. */
		if ( !ImGui_ImplGlfw_InitForVulkan(graphicsRenderer.window().handle(), true) )
		{
			Tracer::error(ClassId, "Unable to initialize ImGUI with GLFW !");

			return false;
		}

		/* NOTE: Initialize ImGUI with Vulkan. */
		{
			const auto & device = graphicsRenderer.device();

			/* NOTE: ImGUI draws inside the engine overlay render pass (post-process
			 * framebuffer). Its main pipeline must be created against that render pass,
			 * otherwise the Vulkan backend silently skips pipeline creation (see
			 * imgui_impl_vulkan.cpp) and nothing is ever drawn. */
			const auto * overlayFramebuffer = graphicsRenderer.overlayFramebuffer();

			if ( overlayFramebuffer == nullptr )
			{
				Tracer::error(ClassId, "No overlay framebuffer available to create the ImGUI pipeline !");

				return false;
			}

			/* Create the descriptor pool.
			 * FIXME: These are fancy numbers ! */
			const auto sizes = std::vector< VkDescriptorPoolSize >{
				{.type=VK_DESCRIPTOR_TYPE_SAMPLER, .descriptorCount=1000},
				{.type=VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, .descriptorCount=1000},
				{.type=VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, .descriptorCount=1000},
				{.type=VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, .descriptorCount=1000},
				{.type=VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, .descriptorCount=1000},
				{.type=VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, .descriptorCount=1000},
				{.type=VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, .descriptorCount=1000},
				{.type=VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, .descriptorCount=1000},
				{.type=VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, .descriptorCount=1000},
				{.type=VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, .descriptorCount=1000},
				{.type=VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, .descriptorCount=1000}
			};

			m_ImGUIDescriptorPool = std::make_shared< DescriptorPool >(device, sizes, 1000, VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT);
			m_ImGUIDescriptorPool->setIdentifier(ClassId, "ImGUI", "DescriptorPool");

			if ( !m_ImGUIDescriptorPool->createOnHardware() )
			{
				Tracer::fatal(ClassId, "Unable to create the ImGUI descriptor pool !");

				return false;
			}

			const auto frameCount = std::max< uint32_t >(graphicsRenderer.framesInFlight(), 2);

			// Setup Platform/Renderer backends
			ImGui_ImplVulkan_InitInfo info{};
			/* NOTE: The device's effective version (the application's target): ImGui loads no entry point above it. */
			info.ApiVersion = device->apiVersion();
			info.Instance = graphicsRenderer.vulkanInstance().handle();
			info.PhysicalDevice = device->physicalDevice()->handle();
			info.Device = device->handle();
			info.QueueFamily = device->getGraphicsFamilyIndex();
			info.Queue = device->getGraphicsQueue(QueuePriority::High)->handle();
			info.DescriptorPool = m_ImGUIDescriptorPool->handle();
			//info.DescriptorPoolSize = 1;
			info.MinImageCount = frameCount;
			info.ImageCount = frameCount;
			info.PipelineCache = VK_NULL_HANDLE;
			info.PipelineInfoMain.RenderPass = overlayFramebuffer->renderPass()->handle();
			info.PipelineInfoMain.Subpass = 0;
			info.UseDynamicRendering = false;
			info.Allocator = VK_NULL_HANDLE;
			info.CheckVkResultFn = nullptr;
			info.MinAllocationSize = 1024UL * 1024UL; // Minimum allocation size. Set to 1024*1024 to satisfy zealous best practices validation layer and waste a little memory.
			//info.CustomShaderVertCreateInfo = ;
			//info.CustomShaderFragCreateInfo = ;

			if ( !ImGui_ImplVulkan_Init(&info) )
			{
				Tracer::error(ClassId, "Unable to initialize ImGUI with Vulkan !");

				return false;
			}
		}

		return true;
	}

	void
	Manager::releaseImGUI () noexcept
	{
		ImGui_ImplVulkan_Shutdown();

		ImGui_ImplGlfw_Shutdown();

		ImGui::DestroyContext();

		m_ImGUIDescriptorPool->destroyFromHardware();
		m_ImGUIDescriptorPool = nullptr;
	}

#endif
}
