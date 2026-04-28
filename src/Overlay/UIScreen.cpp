/*
 * src/Overlay/UIScreen.cpp
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

#include "UIScreen.hpp"

/* STL inclusions. */
#include <cstddef>
#include <ranges>

/* Local inclusions. */
#include "Libs/NameableTrait.hpp"
#include "Vulkan/CommandBuffer.hpp"
#include "Graphics/Geometry/IndexedVertexResource.hpp"
#include "Surface.hpp"
#include "Tracer.hpp"

namespace EmEn::Overlay
{
	using namespace Libs;
	using namespace Graphics;
	using namespace Vulkan;

	bool
	UIScreen::processSurfaceUpdates (bool forceInvalidate) noexcept
	{
		auto errors = 0;

		for ( const auto & surface : m_surfaces )
		{
			/* NOTE: When forceInvalidate is true (window resize), all surfaces must
			 * recalculate their pixel dimensions based on new FramebufferProperties. */
			if ( forceInvalidate )
			{
				surface->invalidate();
			}

			if ( !surface->processUpdates(m_graphicsRenderer) )
			{
				TraceError{ClassId} << "The UI screen '" << this->name() << "' physical representation update failed ! Disabling it ...";

				this->setVisibility(false);

				errors++;

				break;
			}
		}

		return errors == 0;
	}

	void
	UIScreen::render (const std::shared_ptr< RenderTarget::Abstract > & /*renderTarget*/, const CommandBuffer & commandBuffer, const PipelineLayout & pipelineLayout, const Geometry::IndexedVertexResource & surfaceGeometry) const noexcept
	{
		const std::lock_guard< std::mutex > lock{m_surfacesMutex};

		for ( const auto & surface : m_surfaces )
		{
			if ( !surface->isVisible() )
			{
				continue;
			}

			if ( surface->descriptorSet() == nullptr || !surface->descriptorSet()->isCreated() )
			{
				TraceWarning{ClassId} << "The surface " << surface->name() << " doesn't have a descriptor set !";

				continue;
			}

			vkCmdPushConstants(
				commandBuffer.handle(),
				pipelineLayout.handle(),
				VK_SHADER_STAGE_VERTEX_BIT,
				0,
				Matrix4Alignment * sizeof(float),
				surface->modelMatrix().data()
			);

			/* Bind the surface texture. */
			commandBuffer.bind(
				*surface->descriptorSet(),
				pipelineLayout,
				VK_PIPELINE_BIND_POINT_GRAPHICS,
				0
			);

			/* Draw the surface. */
			commandBuffer.draw(surfaceGeometry, 0, 1);
		}
	}

	bool
	UIScreen::destroySurface (const std::string & name) noexcept
	{
		const std::lock_guard< std::mutex > lock{m_surfacesMutex};

		/* NOTE: Wait for the GPU to finish all pending work before destroying the surface.
		 * The rendering thread records Vulkan commands (bind descriptor set, draw) that reference
		 * the surface's texture. vkQueueSubmit returns immediately while the GPU executes
		 * asynchronously. Without this wait, erasing the surface frees the Vulkan texture
		 * while the GPU is still reading it, causing VK_ERROR_DEVICE_LOST. */
		m_graphicsRenderer.device()->waitIdle("UIScreen::destroySurface()");

		const auto surfaceIt = std::ranges::find_if(m_surfaces, [&name] (const auto & s) {
			return s->name() == name;
		});

		if ( surfaceIt == m_surfaces.end() )
		{
			TraceWarning{ClassId} << "There is no surface named '" << name << "' in the screen to erase !";

			return false;
		}

		m_surfaces.erase(surfaceIt);

		/* NOTE: Surfaces above the erased one keep their previous index until we shift
		 * their internal depth back to match the new positions. */
		this->recomputeDepths();

		return true;
	}

	void
	UIScreen::clearSurfaces () noexcept
	{
		const std::lock_guard< std::mutex > lock{m_surfacesMutex};

		/* NOTE: Same GPU sync rationale as destroySurface(). */
		m_graphicsRenderer.device()->waitIdle("UIScreen::clearSurfaces()");

		m_surfaces.clear();
	}

	std::shared_ptr< const Surface >
	UIScreen::getSurface (const std::string & name) const noexcept
	{
		const auto surfaceIt = std::ranges::find_if(m_surfaces, [&name] (const auto & s) {
			return s->name() == name;
		});

		if ( surfaceIt == m_surfaces.end() )
		{
			TraceWarning{ClassId} << "There is no surface named '" << name << "' in the screen !";

			return nullptr;
		}

		return *surfaceIt;
	}

	std::shared_ptr< Surface >
	UIScreen::getSurface (const std::string & name) noexcept
	{
		const auto surfaceIt = std::ranges::find_if(m_surfaces, [&name] (const auto & s) {
			return s->name() == name;
		});

		if ( surfaceIt == m_surfaces.end() )
		{
			TraceWarning{ClassId} << "There is no surface named '" << name << "' in the screen !";

			return nullptr;
		}

		return *surfaceIt;
	}

	bool
	UIScreen::setInputExclusiveSurface (const std::string & name) noexcept
	{
		const auto surface = this->getSurface(name);

		if ( surface == nullptr )
		{
			return false;
		}

		m_inputExclusiveSurface = surface;

		return true;
	}

	bool
	UIScreen::onKeyPress (int32_t key, int32_t scancode, int32_t modifiers, bool repeat) const noexcept
	{
		if constexpr ( KeyboardInputDebugEnabled )
		{
			TraceDebug{ClassId} << "The screen '" << this->name() << "' received a dispatchable keyboard key press event!";
		}

		const auto dispatchEvent = [key, scancode, modifiers, repeat] (const std::shared_ptr< Surface > & surface) -> bool {
			if ( !surface->isVisible() || !surface->isListeningKeyboard() || !surface->isFocused() )
			{
				return false;
			}

			return surface->onKeyPress(key, scancode, modifiers, repeat);
		};

		if ( const auto exclusiveSurface = m_inputExclusiveSurface.lock() )
		{
			return dispatchEvent(exclusiveSurface);
		}

		return std::ranges::any_of(std::views::reverse(m_surfaces), [dispatchEvent] (const auto & surface) -> bool {
			return dispatchEvent(surface);
		});
	}

	bool
	UIScreen::onKeyRelease (int32_t key, int32_t scancode, int32_t modifiers) const noexcept
	{
		if constexpr ( KeyboardInputDebugEnabled )
		{
			TraceDebug{ClassId} << "The screen '" << this->name() << "' received a dispatchable keyboard key release event!";
		}

		const auto dispatchEvent = [key, scancode, modifiers] (const std::shared_ptr< Surface > & surface) -> bool {
			if ( !surface->isVisible() || !surface->isListeningKeyboard() || !surface->isFocused() )
			{
				return false;
			}

			return surface->onKeyRelease(key, scancode, modifiers);
		};

		if ( const auto exclusiveSurface = m_inputExclusiveSurface.lock() )
		{
			return dispatchEvent(exclusiveSurface);
		}

		return std::ranges::any_of(std::views::reverse(m_surfaces), [dispatchEvent] (const auto & surface) -> bool {
			return dispatchEvent(surface);
		});
	}

	bool
	UIScreen::onCharacterType (uint32_t unicode) const noexcept
	{
		if constexpr ( KeyboardInputDebugEnabled )
		{
			TraceDebug{ClassId} << "The screen '" << this->name() << "' received a dispatchable keyboard character type event!";
		}

		const auto dispatchEvent = [unicode] (const std::shared_ptr< Surface > & surface) -> bool {
			if ( !surface->isVisible() || !surface->isListeningKeyboard() || !surface->isFocused() )
			{
				return false;
			}

			return surface->onCharacterType(unicode);
		};

		if ( const auto exclusiveSurface = m_inputExclusiveSurface.lock() )
		{
			return dispatchEvent(exclusiveSurface);
		}

		return std::ranges::any_of(std::views::reverse(m_surfaces), [dispatchEvent] (const auto & surface) -> bool {
			return dispatchEvent(surface);
		});
	}

	bool
	UIScreen::onPointerMove (float positionX, float positionY) const noexcept
	{
		if constexpr ( PointerHeavyInputDebugEnabled )
		{
			TraceDebug{ClassId} << "The screen '" << this->name() << "' received a dispatchable pointer move event!";
		}

		const auto dispatchEvent = [positionX, positionY] (const std::shared_ptr< Surface > & surface) -> bool {
			/* NOTE: Always check if the pointer is over the surface. */
			const auto pointerOver = surface->isBelowPoint(positionX, positionY);

			if ( !surface->isVisible() || !surface->isListeningPointer() )
			{
				surface->setPointerOverState(pointerOver);

				return false;
			}

			if ( pointerOver )
			{
				/* NOTE: If the pointer wasn't over the surface before, generate an entering event. */
				if ( !surface->isPointerWasOver() )
				{
					surface->setPointerOverState(true);

					surface->onPointerEnter(positionX, positionY);
				}

				return surface->onPointerMove(positionX, positionY);
			}

			/* NOTE: If the pointer was over the surface before, generate a leaving event. */
			if ( surface->isPointerWasOver() )
			{
				surface->setPointerOverState(false);

				surface->onPointerLeave(positionX, positionY);
			}

			return false;
		};

		if ( const auto exclusiveSurface = m_inputExclusiveSurface.lock() )
		{
			return dispatchEvent(exclusiveSurface);
		}

		return std::ranges::any_of(std::views::reverse(m_surfaces), [dispatchEvent] (const auto & surface) -> bool {
			return dispatchEvent(surface);
		});
	}

	bool
	UIScreen::onButtonPress (float positionX, float positionY, int32_t buttonNumber, int32_t modifiers) const noexcept
	{
		if constexpr ( PointerInputDebugEnabled )
		{
			TraceDebug{ClassId} << "The screen '" << this->name() << "' received a dispatchable pointer button press event!";
		}

		const auto dispatchEvent = [positionX, positionY, buttonNumber, modifiers] (const std::shared_ptr< Surface > & surface) -> bool {
			if ( surface->isVisible() && surface->isListeningPointer() && surface->isBelowPoint(positionX, positionY) )
			{
				surface->setFocusedState(true);

				return surface->onButtonPress(positionX, positionY, buttonNumber, modifiers);
			}

			surface->setFocusedState(false);

			return false;
		};

		if ( const auto exclusiveSurface = m_inputExclusiveSurface.lock() )
		{
			TraceDebug{ClassId} << "Dispatch to EXCLUSIVE surface '" << exclusiveSurface->name() << "' !";

			return dispatchEvent(exclusiveSurface);
		}

		return std::ranges::any_of(std::views::reverse(m_surfaces), [dispatchEvent] (const auto & surface) -> bool {
			TraceDebug{ClassId} << "Dispatch to surface '" << surface->name() << "' !";
			return dispatchEvent(surface);
		});
	}

	bool
	UIScreen::onButtonRelease (float positionX, float positionY, int32_t buttonNumber, int32_t modifiers) const noexcept
	{
		if constexpr ( PointerInputDebugEnabled )
		{
			TraceDebug{ClassId} << "The screen '" << this->name() << "' received a dispatchable pointer button release event!";
		}

		const auto dispatchEvent = [positionX, positionY, buttonNumber, modifiers] (const std::shared_ptr< Surface > & surface) -> bool {
			if ( surface->isVisible() && surface->isListeningPointer() && surface->isBelowPoint(positionX, positionY) )
			{
				return surface->onButtonRelease(positionX, positionY, buttonNumber, modifiers);
			}

			return false;
		};

		if ( const auto exclusiveSurface = m_inputExclusiveSurface.lock() )
		{
			return dispatchEvent(exclusiveSurface);
		}

		return std::ranges::any_of(std::views::reverse(m_surfaces), [dispatchEvent] (const auto & surface) -> bool {
			return dispatchEvent(surface);
		});
	}

	bool
	UIScreen::onMouseWheel (float positionX, float positionY, float xOffset, float yOffset, int32_t modifiers) const noexcept
	{
		if constexpr ( PointerHeavyInputDebugEnabled )
		{
			TraceDebug{ClassId} << "The screen '" << this->name() << "' received a dispatchable mouse wheel event!";
		}

		const auto dispatchEvent = [positionX, positionY, xOffset, yOffset, modifiers] (const std::shared_ptr< Surface > & surface) -> bool {
			if ( surface->isVisible() && surface->isListeningPointer() && surface->isBelowPoint(positionX, positionY) )
			{
				return surface->onMouseWheel(positionX, positionY, xOffset, yOffset, modifiers);
			}

			return false;
		};

		if ( const auto exclusiveSurface = m_inputExclusiveSurface.lock() )
		{
			return dispatchEvent(exclusiveSurface);
		}

		return std::ranges::any_of(std::views::reverse(m_surfaces), [dispatchEvent] (const auto & surface) -> bool {
			return dispatchEvent(surface);
		});
	}

	void
	UIScreen::recomputeDepths () noexcept
	{
		/* NOTE: Caller holds m_surfacesMutex. We walk the stack and let each surface
		 * derive its internal depth from its index. The actual step value is owned by
		 * Surface::setStackIndex(). */
		for ( size_t index = 0; index < m_surfaces.size(); ++index )
		{
			m_surfaces[index]->setStackIndex(index);
		}
	}

	bool
	UIScreen::bringToFront (const std::string & name) noexcept
	{
		const std::lock_guard< std::mutex > lock{m_surfacesMutex};

		const auto it = std::ranges::find_if(m_surfaces, [&name] (const auto & s) {
			return s->name() == name;
		});

		if ( it == m_surfaces.end() )
		{
			TraceWarning{ClassId} << "bringToFront: no surface named '" << name << "' in screen '" << this->name() << "'.";

			return false;
		}

		/* Already at the top — nothing to do. */
		if ( std::next(it) == m_surfaces.end() )
		{
			return true;
		}

		auto surface = std::move(*it);
		m_surfaces.erase(it);
		m_surfaces.emplace_back(std::move(surface));

		this->recomputeDepths();

		return true;
	}

	bool
	UIScreen::sendToBack (const std::string & name) noexcept
	{
		const std::lock_guard< std::mutex > lock{m_surfacesMutex};

		const auto it = std::ranges::find_if(m_surfaces, [&name] (const auto & s) {
			return s->name() == name;
		});

		if ( it == m_surfaces.end() )
		{
			TraceWarning{ClassId} << "sendToBack: no surface named '" << name << "' in screen '" << this->name() << "'.";

			return false;
		}

		/* Already at the bottom — nothing to do. */
		if ( it == m_surfaces.begin() )
		{
			return true;
		}

		auto surface = std::move(*it);
		m_surfaces.erase(it);
		m_surfaces.insert(m_surfaces.begin(), std::move(surface));

		this->recomputeDepths();

		return true;
	}

	bool
	UIScreen::bringForward (const std::string & name) noexcept
	{
		const std::lock_guard< std::mutex > lock{m_surfacesMutex};

		const auto it = std::ranges::find_if(m_surfaces, [&name] (const auto & s) {
			return s->name() == name;
		});

		if ( it == m_surfaces.end() )
		{
			TraceWarning{ClassId} << "bringForward: no surface named '" << name << "' in screen '" << this->name() << "'.";

			return false;
		}

		/* Already at the top — no-op. */
		if ( std::next(it) == m_surfaces.end() )
		{
			return true;
		}

		std::iter_swap(it, std::next(it));

		this->recomputeDepths();

		return true;
	}

	bool
	UIScreen::sendBackward (const std::string & name) noexcept
	{
		const std::lock_guard< std::mutex > lock{m_surfacesMutex};

		const auto it = std::ranges::find_if(m_surfaces, [&name] (const auto & s) {
			return s->name() == name;
		});

		if ( it == m_surfaces.end() )
		{
			TraceWarning{ClassId} << "sendBackward: no surface named '" << name << "' in screen '" << this->name() << "'.";

			return false;
		}

		/* Already at the bottom — no-op. */
		if ( it == m_surfaces.begin() )
		{
			return true;
		}

		std::iter_swap(it, std::prev(it));

		this->recomputeDepths();

		return true;
	}

	bool
	UIScreen::moveAbove (const std::string & name, const std::string & reference) noexcept
	{
		if ( name == reference )
		{
			TraceWarning{ClassId} << "moveAbove: a surface cannot be moved relative to itself ('" << name << "').";

			return false;
		}

		const std::lock_guard< std::mutex > lock{m_surfacesMutex};

		const auto srcIt = std::ranges::find_if(m_surfaces, [&name] (const auto & s) {
			return s->name() == name;
		});

		if ( srcIt == m_surfaces.end() )
		{
			TraceWarning{ClassId} << "moveAbove: no surface named '" << name << "' in screen '" << this->name() << "'.";

			return false;
		}

		const auto refIt = std::ranges::find_if(m_surfaces, [&reference] (const auto & s) {
			return s->name() == reference;
		});

		if ( refIt == m_surfaces.end() )
		{
			TraceWarning{ClassId} << "moveAbove: no reference surface named '" << reference << "' in screen '" << this->name() << "'.";

			return false;
		}

		/* Extract the source surface, then insert just past the reference. The
		 * reference iterator must be re-located after the erase since indices shift. */
		auto surface = std::move(*srcIt);
		m_surfaces.erase(srcIt);

		const auto refItAfterErase = std::ranges::find_if(m_surfaces, [&reference] (const auto & s) {
			return s->name() == reference;
		});

		m_surfaces.insert(std::next(refItAfterErase), std::move(surface));

		this->recomputeDepths();

		return true;
	}

	bool
	UIScreen::moveBelow (const std::string & name, const std::string & reference) noexcept
	{
		if ( name == reference )
		{
			TraceWarning{ClassId} << "moveBelow: a surface cannot be moved relative to itself ('" << name << "').";

			return false;
		}

		const std::lock_guard< std::mutex > lock{m_surfacesMutex};

		const auto srcIt = std::ranges::find_if(m_surfaces, [&name] (const auto & s) {
			return s->name() == name;
		});

		if ( srcIt == m_surfaces.end() )
		{
			TraceWarning{ClassId} << "moveBelow: no surface named '" << name << "' in screen '" << this->name() << "'.";

			return false;
		}

		const auto refIt = std::ranges::find_if(m_surfaces, [&reference] (const auto & s) {
			return s->name() == reference;
		});

		if ( refIt == m_surfaces.end() )
		{
			TraceWarning{ClassId} << "moveBelow: no reference surface named '" << reference << "' in screen '" << this->name() << "'.";

			return false;
		}

		auto surface = std::move(*srcIt);
		m_surfaces.erase(srcIt);

		const auto refItAfterErase = std::ranges::find_if(m_surfaces, [&reference] (const auto & s) {
			return s->name() == reference;
		});

		m_surfaces.insert(refItAfterErase, std::move(surface));

		this->recomputeDepths();

		return true;
	}

	std::optional< size_t >
	UIScreen::indexOf (const std::string & name) const noexcept
	{
		const std::lock_guard< std::mutex > lock{m_surfacesMutex};

		const auto it = std::ranges::find_if(m_surfaces, [&name] (const auto & s) {
			return s->name() == name;
		});

		if ( it == m_surfaces.end() )
		{
			return std::nullopt;
		}

		return static_cast< size_t >(std::distance(m_surfaces.begin(), it));
	}

	std::vector< std::string >
	UIScreen::stackOrder () const noexcept
	{
		const std::lock_guard< std::mutex > lock{m_surfacesMutex};

		std::vector< std::string > order;
		order.reserve(m_surfaces.size());

		for ( const auto & surface : m_surfaces )
		{
			order.emplace_back(surface->name());
		}

		return order;
	}

	std::ostream &
	operator<< (std::ostream & out, const UIScreen & obj)
	{
		const auto exclusiveSurface = obj.m_inputExclusiveSurface.lock();

		out <<
			"UI screen '" << obj.name() << "' data :" "\n"
			"Is visible : " << ( obj.isVisible() ? "YES" : "NO" ) << "\n" <<
			"Is listening to the keyboard : " << ( obj.isListeningKeyboard() ? "YES" : "NO" ) << "\n" <<
			"Is listening to the mouse/pointer : " << ( obj.isListeningPointer() ? "YES" : "NO" ) << "\n" <<
			"Has input exclusive surface : " << ( exclusiveSurface == nullptr ? "[No]" : exclusiveSurface->name() ) << '\n';

		if ( obj.m_surfaces.empty() )
		{
			out << "No surfaces present." "\n";
		}
		else
		{
			out <<
				"Surfaces : " "\n"
				"==============================================================================" "\n";

			for ( const auto & surface : obj.m_surfaces )
			{
				out <<
					*surface <<
					"==============================================================================" "\n";
			}
		}

		return out;
	}
}
