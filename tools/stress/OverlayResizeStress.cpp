/*
 * tools/stress/OverlayResizeStress.cpp
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

/* STL inclusions. */
#include <array>
#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <memory>
#include <mutex>
#include <string>
#include <thread>

/* Local inclusions. */
#include "Core.hpp"
#include "Overlay/Manager.hpp"
#include "Overlay/Surface.hpp"
#include "Overlay/UIScreen.hpp"
#include "PlatformManager.hpp"

/*
 * The ThreadSanitizer proof of Overlay::Manager::onWindowResized() (item overlay-resize-notifies-under-lock, Ave
 * Robustus II CP.31/32; built with EMERAUDE_ENABLE_TSAN only). A windowed, CEF-less engine renders while the main
 * thread resizes the window every ResizePeriod; the logic thread pushes user notifications, so the Notifier draws its
 * surface from the logic thread, from its timer and from its OverlayResized handler. A second observer of the overlay
 * calls back into the manager from its OverlayResized handler (updateVideoMemory(), which takes the representation
 * lock): with the notification emitted under that lock, the main thread locks a mutex it already holds.
 * 2026-10-13 (the overlay geometry and redraw protocol, docs/caution-points.md § "Overlay lock order"): the
 * logic thread also moves and resizes a surface of its own every cycle while the render thread reads its geometry,
 * and the observer calls back into the manager from its RedrawRequested handler too — the resize pass invalidates
 * every surface, which requests a redraw from INSIDE the pass.
 * The verdict is TSan's: run it with TSAN_OPTIONS=halt_on_error=1 (procedure: projet-alpha docs/unit-testing.md).
 */
namespace
{
	using namespace EmEn;

	/* How long the resizes run, the pause between two of them, and the two sizes they alternate. */
	constexpr std::chrono::seconds RunDuration{30};
	constexpr std::chrono::milliseconds ResizePeriod{100};
	constexpr std::array< int32_t, 2 > SmallSize{960, 540};
	constexpr std::array< int32_t, 2 > LargeSize{1280, 720};
	/* One user notification every this many logic cycles. */
	constexpr size_t NotificationPeriod{6};
	/* The stress surface's screen and name. */
	constexpr auto StressScreenName{"OverlayResizeStress"};
	constexpr auto StressSurfaceName{"MovingSurface"};

	/** @brief An observer of the overlay manager that calls back into it on every resize and every redraw request. */
	class ReentrantObserver final : public Base::ObserverTrait
	{
		public:

			explicit
			ReentrantObserver (Overlay::Manager & overlayManager) noexcept
				: m_overlayManager{overlayManager},
				m_mainThread{std::this_thread::get_id()}
			{

			}

			[[nodiscard]]
			uint64_t
			resizes () const noexcept
			{
				return m_resizes.load();
			}

			[[nodiscard]]
			uint64_t
			scaleChanges () const noexcept
			{
				return m_scaleChanges.load();
			}

			[[nodiscard]]
			uint64_t
			redrawRequests () const noexcept
			{
				return m_redrawRequests.load();
			}

		private:

			bool
			onNotification (const Base::ObservableTrait * observable, int notificationCode, const Base::Any & /*data*/) noexcept override
			{
				if ( observable != &m_overlayManager )
				{
					return false;
				}

				switch ( notificationCode )
				{
					case Overlay::Manager::OverlayResized :
						/* NOTE: The call back into the manager the item is about. */
						m_overlayManager.updateVideoMemory();
						m_resizes.fetch_add(1U);
						break;

					case Overlay::Manager::OverlayScaleChanged :
						m_scaleChanges.fetch_add(1U);
						break;

					case Overlay::Manager::RedrawRequested :
						m_redrawRequests.fetch_add(1U);

						/* NOTE: The call back into the manager of the redraw item, on the MAIN thread only (where the resize
						 * pass requests redraws from inside the pass), once at a time: the pass it runs may request a
						 * redraw again (a rebuilt surface), which must not recurse without bound. Not from the logic
						 * thread: a pass there rebuilds a surface while the render thread holds a recorded, unsubmitted
						 * frame (see the engine item overlay-surface-rebuilt-while-a-frame-is-recorded). */
						if ( std::this_thread::get_id() == m_mainThread && m_callbackThread.load() != std::this_thread::get_id() )
						{
							m_callbackThread.store(std::this_thread::get_id());

							m_overlayManager.updateVideoMemory();

							m_callbackThread.store(std::thread::id{});
						}
						break;

					default :
						break;
				}

				return true;
			}

			Overlay::Manager & m_overlayManager;
			std::thread::id m_mainThread;
			std::atomic< std::thread::id > m_callbackThread;
			std::atomic< uint64_t > m_resizes{0};
			std::atomic< uint64_t > m_scaleChanges{0};
			std::atomic< uint64_t > m_redrawRequests{0};
	};

	class OverlayResizeStress final : public Core
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"OverlayResizeStress"};

			OverlayResizeStress (int argc, char * * argv) noexcept
				: Core{argc, argv, "EmeraudeOverlayResizeStress", {1, 0, 0}, "EmeraudeEngine", "emeraude-engine.org"},
				m_observer{this->overlayManager()}
			{

			}

			OverlayResizeStress (const OverlayResizeStress & copy) noexcept = delete;
			OverlayResizeStress (OverlayResizeStress && copy) noexcept = delete;
			OverlayResizeStress & operator= (const OverlayResizeStress & copy) noexcept = delete;
			OverlayResizeStress & operator= (OverlayResizeStress && copy) noexcept = delete;
			~OverlayResizeStress () override = default;

			[[nodiscard]]
			size_t
			classUID () const noexcept override
			{
				return Base::Hash::FNV1a(ClassId);
			}

			[[nodiscard]]
			bool
			is (size_t classUID) const noexcept override
			{
				return classUID == Base::Hash::FNV1a(ClassId);
			}

			/** @brief Whether the overlay received resizes (none means the run proves nothing). */
			[[nodiscard]]
			bool
			resized () const noexcept
			{
				return m_observer.resizes() > 0;
			}

		private:

			bool
			onCoreStarted (const Arguments & /*arguments*/, Settings & /*settings*/) noexcept override
			{
				if ( this->window().isWindowLessMode() )
				{
					std::cerr << "[OverlayResizeStress] A window is required: do not pass --window-less !" "\n";

					return false;
				}

				m_observer.observe(&this->overlayManager());

				/* NOTE: A surface of our own, moved and resized by the logic thread (onCoreProcessLogics()). */
				const auto screen = this->overlayManager().createScreen(StressScreenName, false, false);

				if ( screen == nullptr )
				{
					std::cerr << "[OverlayResizeStress] Unable to create the stress screen !" "\n";

					return false;
				}

				auto surface = screen->createSurface< Overlay::Surface >(StressSurfaceName, Base::Math::Space2D::AARectangle< float >{0.1F, 0.1F, 0.3F, 0.3F}, true);

				if ( surface == nullptr )
				{
					std::cerr << "[OverlayResizeStress] Unable to create the stress surface !" "\n";

					return false;
				}

				screen->setVisibility(true);

				{
					const std::scoped_lock lock{m_surfaceAccess};

					m_surface = std::move(surface);
				}

				m_start = std::chrono::steady_clock::now();
				m_nextResize = m_start;

				std::cout << "[OverlayResizeStress] Resizing every " << ResizePeriod.count() << " ms for " << RunDuration.count() << " s." "\n";

				return true;
			}

			void
			onCoreMainLoopCycle () noexcept override
			{
				if ( m_stopping )
				{
					return;
				}

				const auto now = std::chrono::steady_clock::now();

				if ( now - m_start >= RunDuration )
				{
					m_stopping = true;

					this->stop();

					return;
				}

				if ( now >= m_nextResize )
				{
					const auto & size = m_resizeRequests % 2U == 0U ? SmallSize : LargeSize;

					if ( this->window().resize(size[0], size[1]) )
					{
						++m_resizeRequests;
					}

					m_nextResize = now + ResizePeriod;
				}

				/* NOTE: The main loop may sleep on the system events: wake it for the next resize. */
				this->scheduleMainLoopCycle(ResizePeriod.count());
			}

			void
			onCoreProcessLogics (size_t engineCycle) noexcept override
			{
				if ( engineCycle % NotificationPeriod == 0 )
				{
					this->notifyUser("Logic cycle #" + std::to_string(engineCycle), 300);
				}

				/* NOTE: A copy taken under the lock, used outside it: onBeforeCoreStop() releases the surface on the main
				 * thread while this thread still runs. */
				std::shared_ptr< Overlay::Surface > surface;

				{
					const std::scoped_lock lock{m_surfaceAccess};

					surface = m_surface;
				}

				if ( surface == nullptr )
				{
					return;
				}

				/* NOTE: Every geometry setter in turn, from the logic thread, while the render thread reads the geometry. */
				const auto step = static_cast< float >(engineCycle % 10U) * 0.02F;

				switch ( engineCycle % 4U )
				{
					case 0U :
						surface->setGeometry({0.1F + step, 0.1F, 0.3F, 0.3F});
						break;

					case 1U :
						surface->setPosition(0.2F, 0.1F + step);
						break;

					case 2U :
						surface->setSize(0.25F + step, 0.25F);
						break;

					default :
						surface->move(0.01F, -0.01F);
						break;
				}

				m_geometryChanges.fetch_add(1U);
			}

			bool
			onBeforeCoreStop () noexcept override
			{
				m_observer.forget(&this->overlayManager());

				{
					const std::scoped_lock lock{m_surfaceAccess};

					m_surface.reset();
				}

				static_cast< void >(this->overlayManager().destroyScreen(StressScreenName));

				std::cout << "[OverlayResizeStress] " << m_resizeRequests << " resize requests, " << m_observer.resizes() << " OverlayResized, " << m_observer.scaleChanges() << " OverlayScaleChanged, " << m_observer.redrawRequests() << " RedrawRequested, " << m_geometryChanges.load() << " geometry changes." "\n";

				return true;
			}

			ReentrantObserver m_observer;
			std::shared_ptr< Overlay::Surface > m_surface;
			std::mutex m_surfaceAccess;
			std::atomic< uint64_t > m_geometryChanges{0};
			std::chrono::steady_clock::time_point m_start;
			std::chrono::steady_clock::time_point m_nextResize;
			uint64_t m_resizeRequests{0};
			bool m_stopping{false};
	};
}

int
main (int argc, char * * argv)
{
	/* NOTE: Before any thread, as every consumer's main() (PlatformManager::prepareVulkanLoaderEnvironment()). */
	static_cast< void >(EmEn::PlatformManager::prepareVulkanLoaderEnvironment());

	OverlayResizeStress stress{argc, argv};

	const auto exitCode = stress.run();

	if ( !stress.resized() )
	{
		std::cerr << "[OverlayResizeStress] The overlay was never resized: the run proves nothing !" "\n";

		return 1;
	}

	return exitCode;
}
