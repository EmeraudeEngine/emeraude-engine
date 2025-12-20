/*
 * src/Overlay/UIScreen.hpp
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
#include <algorithm>
#include <memory>
#include <string>
#include <vector>

/* Local inclusions for inheritances. */
#include "Libs/NameableTrait.hpp"

/* Local inclusions for usages. */
#include "FramebufferProperties.hpp"
#include "Graphics/Renderer.hpp"
#include "Surface.hpp"
#include "Tracer.hpp"

namespace EmEn::Graphics::Geometry
{
	class IndexedVertexResource;
}
namespace EmEn::Overlay
{
	/**
	 * @brief Defines an overlaying screen object.
	 * @details There are no physical properties. This is just a group of surfaces and dispatch input event to it.
	 * @exception EmEn::Libs::NameableTrait A UI screen has a name.
	 */
	class UIScreen final : public Libs::NameableTrait
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"UIScreen"};

			/**
			 * @brief Constructs a default UI screen.
			 * @param name A string [std::move].
			 * @param framebufferProperties A reference to an array [width, height].
			 * @param graphicsRenderer A reference to the graphics renderer.
			 * @param enableKeyboardListener Enables the keyboard listener at creation.
			 * @param enablePointerListener Enables the pointer listener at creation.
			 */
			UIScreen (std::string name, const FramebufferProperties & framebufferProperties, Graphics::Renderer & graphicsRenderer, bool enableKeyboardListener, bool enablePointerListener) noexcept
				: NameableTrait{std::move(name)},
				m_graphicsRenderer{graphicsRenderer},
				m_framebufferProperties{framebufferProperties},
				m_isListeningKeyboard{enableKeyboardListener},
				m_isListeningPointer{enablePointerListener}
			{

			}

			/**
			 * @brief Returns whether the UI screen has surfaces declared.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			empty () const noexcept
			{
				return m_surfaces.empty();
			}

			/**
			 * @brief Sets the alpha is premultiplied for this screen.
			 * @param state The state.
			 * @return void
			 */
			void
			setPremultipliedAlpha (bool state) noexcept
			{
				m_premultipliedAlpha = state;
			}

			/**
			 * @brief Returns whether the alpha is premultiplied for this screen.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			premultipliedAlpha () const noexcept
			{
				return m_premultipliedAlpha;
			}

			/**
			 * @brief Sets the source format to BGRA for this screen.
			 * @note By default, the source format is RGBA. CEF provides BGRA pixels.
			 * @param state The state. True for BGRA, false for RGBA.
			 * @return void
			 */
			void
			useBGRAFormat (bool state) noexcept
			{
				m_useBGRAFormat = state;
			}

			/**
			 * @brief Returns whether the source format is BGRA for this screen.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isUsingBGRAFormat () const noexcept
			{
				return m_useBGRAFormat;
			}

			/**
			 * @brief Sets the UI screen visibility.
			 * @param state The state
			 * @return void
			 */
			void
			setVisibility (bool state) noexcept
			{
				m_isVisible = state;
			}

			/**
			 * @brief Returns whether the UI screen is visible.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isVisible () const noexcept
			{
				return m_isVisible;
			}

			/**
			 * @brief Enables the listening of keyboard events.
			 * @param state The state.
			 * @return void
			 */
			void
			enableKeyboardListening (bool state) noexcept
			{
				m_isListeningKeyboard = state;
			}

			/**
			 * @brief Returns whether the keyboard is listened.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isListeningKeyboard () const noexcept
			{
				return m_isListeningKeyboard;
			}

			/**
			 * @brief Enables the listening of pointer events.
			 * @param state The state.
			 * @return void
			 */
			void
			enablePointerListening (bool state) noexcept
			{
				m_isListeningPointer = state;
			}

			/**
			 * @brief Returns whether the pointer is listened.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isListeningPointer () const noexcept
			{
				return m_isListeningPointer;
			}

			/**
			 * @brief Creates a specialized surface.
			 * @tparam surface_t The type of surface.
			 * @tparam ctor_args The type of surface constructor optional arguments.
			 * @param name A reference to a string.
			 * @param args Additional arguments to pass to a specific surface constructor. Default none.
			 * @return std::shared_ptr< surface_t >
			 */
			template< typename surface_t, typename... ctor_args >
			std::shared_ptr< surface_t >
			createSurface (const std::string & name, ctor_args... args) noexcept requires (std::is_base_of_v< Surface, surface_t >)
			{
				const std::lock_guard< std::mutex > lock{m_surfacesMutex};

				if ( !m_framebufferProperties.isValid() )
				{
					TraceError{ClassId} << "The screen size are not initialized !";

					return nullptr;
				}

					const auto existingIt = std::ranges::find_if(m_surfaces, [&name] (const auto & s) {
					return s->name() == name;
				});

				if ( existingIt != m_surfaces.end() )
				{
					TraceError{ClassId} << "The UI screen '" << this->name() << "' contains already a surface named '" << name << "' !";

					return nullptr;
				}

				auto surface = std::make_shared< surface_t >(m_framebufferProperties, name, std::forward< ctor_args >(args)...);

				if ( !surface->createOnHardware(m_graphicsRenderer) )
				{
					TraceError{ClassId} <<
						"Unable to create the surface '" << name << "' on the GPU !" "\n"
						"Framebuffer : " << m_framebufferProperties;

					return nullptr;
				}

				m_surfaces.emplace_back(surface);

				this->sortSurfacesByDepth();

				return surface;
			}

			/**
			 * @brief Processes pending updates for all surfaces in this screen.
			 * @details This method iterates through all surfaces and processes their pending updates:
			 * - If forceInvalidate is true (window resize), all surfaces are invalidated first
			 * - Each surface then recreates it's back buffer if needed and uploads content to GPU
			 * @param forceInvalidate When true, forces all surfaces to invalidate and recalculate
			 * their pixel dimensions (used during window resize). When false, only processes
			 * surfaces that have pending local changes.
			 */
			void processSurfaceUpdates (bool forceInvalidate) noexcept;

			/**
			 * @brief Render this screen. The manager does tests for visibility.
			 * @param renderTarget A reference to a render target smart pointer.
			 * @param commandBuffer A reference to a command buffer.
			 * @param pipelineLayout A reference to the overlay manager pipeline layout.
			 * @param surfaceGeometry A reference to geometry.
			 * @return void
			 */
			void render (const std::shared_ptr< Graphics::RenderTarget::Abstract > & renderTarget, const Vulkan::CommandBuffer & commandBuffer, const Vulkan::PipelineLayout & pipelineLayout, const Graphics::Geometry::IndexedVertexResource & surfaceGeometry) const noexcept;

			/**
			 * @brief Destroys a surface by its name.
			 * @param name A reference to a string.
			 * @return bool
			 */
			bool destroySurface (const std::string & name) noexcept;

			/**
			 * @brief Deletes all surfaces.
			 * @return void
			 */
			void clearSurfaces () noexcept;

			/**
			 * @brief Returns the screen surfaces list sorted by depth.
			 * @return const std::vector< std::shared_ptr< Surface > > &
			 */
			[[nodiscard]]
			const std::vector< std::shared_ptr< Surface > > &
			surfaces () const noexcept
			{
				return m_surfaces;
			}

			/**
			 * @brief Returns a pointer to a named surface or nullptr.
			 * @param name The name of the surface.
			 * @return std::shared_ptr< const Surface >
			 */
			[[nodiscard]]
			std::shared_ptr< const Surface > getSurface (const std::string & name) const noexcept;

			/**
			 * @brief Returns a pointer to a named surface or nullptr.
			 * @param name The name of the surface.
			 * @return std::shared_ptr< Surface >
			 */
			[[nodiscard]]
			std::shared_ptr< Surface > getSurface (const std::string & name) noexcept;

			/**
			 * @brief Sets an exclusive surface to receive inputs.
			 * @param name A reference to a string.
			 * @return bool
			 */
			[[nodiscard]]
			bool setInputExclusiveSurface (const std::string & name) noexcept;

			/**
			 * @brief Disables a previous input exclusive surface.
			 * @return void
			 */
			void
			disableInputExclusiveSurface () noexcept
			{
				m_inputExclusiveSurface.reset();
			}

			/**
			 * @brief Returns whether an input exclusive surface has been set.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isInputExclusiveSurfaceEnabled () const noexcept
			{
				return !m_inputExclusiveSurface.expired();
			}

			/**
			 * @brief Returns the surface set as input exclusive.
			 * @warning This can be nullptr if the surface was destroyed or never set.
			 * @return std::shared_ptr< Surface >
			 */
			[[nodiscard]]
			std::shared_ptr< Surface >
			inputExclusiveSurface () const noexcept
			{
				return m_inputExclusiveSurface.lock();
			}

			/**
			 * @brief Transfers the key press event to surfaces.
			 * @param key The keyboard universal key code. Example, QWERTY keyboard 'A' key gives the ASCII code '65' on all platforms.
			 * @param scancode The OS dependent scancode.
			 * @param modifiers The modifier keys mask.
			 * @param repeat Repeat state.
			 * @return bool
			 */
			bool onKeyPress (int32_t key, int32_t scancode, int32_t modifiers, bool repeat) const noexcept;

			/**
			 * @brief Transfers the key release event to surfaces.
			 * @param key The keyboard universal key code. Example, QWERTY keyboard 'A' key gives the ASCII code '65' on all platforms.
			 * @param scancode The OS dependent scancode..
			 * @param modifiers The modifier keys mask.
			 * @return bool
			 */
			bool onKeyRelease (int32_t key, int32_t scancode, int32_t modifiers) const noexcept;

			/**
			 * @brief Transfers the character-typed event to surfaces.
			 * @param unicode The Unicode number.
			 * @return bool
			 */
			bool onCharacterType (uint32_t unicode) const noexcept;

			/**
			 * @brief Transfers the pointer movement event to surfaces.
			 * @param positionX The x position of the cursor.
			 * @param positionY The Y position of the cursor.
			 * @return bool
			 */
			bool onPointerMove (float positionX, float positionY) const noexcept;

			/**
			 * @brief Transfers the pointer press event to surfaces.
			 * @param positionX The x position of the cursor.
			 * @param positionY The Y position of the cursor.
			 * @param buttonNumber The mouse button.
			 * @param modifiers Modification keys held.
			 * @return bool
			 */
			bool onButtonPress (float positionX, float positionY, int32_t buttonNumber, int32_t modifiers) const noexcept;

			/**
			 * @brief Transfers the pointer release event to surfaces.
			 * @param positionX The x position of the cursor.
			 * @param positionY The Y position of the cursor.
			 * @param buttonNumber The mouse button.
			 * @param modifiers Modification keys held.
			 * @return bool
			 */
			bool onButtonRelease (float positionX, float positionY, int32_t buttonNumber, int32_t modifiers) const noexcept;

			/**
			 * @brief Transfers the mouse wheel event to surfaces.
			 * @param positionX The x position of the cursor.
			 * @param positionY The Y position of the cursor.
			 * @param xOffset The mouse wheel x offset.
			 * @param yOffset The mouse wheel y offset.
			 * @param modifiers The keyboard modifiers pressed during the scroll.
			 * @return bool
			 */
			bool onMouseWheel (float positionX, float positionY, float xOffset, float yOffset, int32_t modifiers = 0) const noexcept;

		private:

			/**
			 * @brief Sort surfaces by depth when adding or removing surface from the screen.
			 * @return void
			 */
			void sortSurfacesByDepth () noexcept;

			/**
			 * @brief STL streams printable object.
			 * @param out A reference to the stream output.
			 * @param obj A reference to the object to print.
			 * @return std::ostream &
			 */
			friend std::ostream & operator<< (std::ostream & out, const UIScreen & obj);
			
			Graphics::Renderer & m_graphicsRenderer;
			const FramebufferProperties & m_framebufferProperties;
			std::vector< std::shared_ptr< Surface > > m_surfaces;
			std::weak_ptr< Surface > m_inputExclusiveSurface;
			mutable std::mutex m_surfacesMutex;
			bool m_isVisible{false};
			bool m_isListeningKeyboard{false};
			bool m_isListeningPointer{false};
			bool m_premultipliedAlpha{false};
			bool m_useBGRAFormat{false};
	};

	/**
	 * @brief Stringifies the object.
	 * @param obj A reference to the object to print.
	 * @return std::string
	 */
	inline
	std::string
	to_string (const UIScreen & obj)
	{
		std::stringstream output;

		output << obj;

		return output.str();
	}
}
