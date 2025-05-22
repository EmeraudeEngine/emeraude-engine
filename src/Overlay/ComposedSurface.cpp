/*
 * src/Overlay/ComposedSurface.cpp
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

#include "ComposedSurface.hpp"

/* STL inclusions. */
#include <array>
#include <cstdint>
#include <memory>

/* Local inclusions. */
#include "Surface.hpp"
#include "Constants.hpp"
#include "Manager.hpp"
#include "Layouts/Main.hpp"
#include "Libs/Math/Space2D/AARectangle.hpp"
#include "Libs/Math/Vector.hpp"
#include "Libs/PixelFactory/Pixmap.hpp"
#include "Tracer.hpp"
#include "Window.hpp"
#include "Graphics/Renderer.hpp"

namespace EmEn::Overlay
{
	using namespace EmEn::Libs;
	using namespace EmEn::Libs::Math;
	using namespace EmEn::Libs::PixelFactory;
	using namespace Graphics;
	using namespace Vulkan;

	ComposedSurface::ComposedSurface (const FramebufferProperties & framebufferProperties, const std::string & name, const Math::Space2D::AARectangle< float > & geometry, float depth) noexcept
		: Surface(framebufferProperties, name, geometry, depth)
	{

	}

	void
	ComposedSurface::setGeometry (float width, float height) noexcept
	{
		if ( width <= 0.0F || height <= 0.0F )
		{
			Tracer::warning(ClassId, "Invalid size !");

			return;
		}

		m_rectangle.setWidth(width);
		m_rectangle.setHeight(height);

		this->resize();
	}

	void
	ComposedSurface::setGeometry (unsigned int width, unsigned int height) noexcept
	{
		// FIXME: Size must use the overlay notified size !
		const auto windowSize = Window::instance()->getFramebufferSize();
		const auto surfaceWidth = static_cast< float >(windowSize[0]) / static_cast< float >(width);
		const auto surfaceHeight = static_cast< float >(windowSize[1]) / static_cast< float >(height);

		this->setGeometry(surfaceWidth, surfaceHeight);
	}

	/*void
	ComposedSurface::setPosition (float positionX, float positionY) noexcept
	{
		m_rectangle.setOffsetX(positionX);
		m_rectangle.setOffsetY(positionY);
	}*/

	void
	ComposedSurface::setPosition (int /*positionX*/, int /*positionY*/) noexcept
	{
		// FIXME: Size must use the overlay notified size !
		//const auto windowSize =  Window::instance()->getFramebufferSize();
		//const auto surfaceX = static_cast< float >(windowSize[0]) / static_cast< float >(positionX);
		//const auto surfaceY = static_cast< float >(windowSize[1]) / static_cast< float >(positionY);

		//this->setPosition(surfaceX, surfaceY);
	}

	void
	ComposedSurface::setPosition (Position position) noexcept
	{
		switch ( position )
		{
			case Position::Center :
				this->setPosition(0.0F, 0.0F);
				break;

			case Position::Bottom :
				this->setPosition(0.0F, -1.0F + m_rectangle.height());
				break;

			case Position::Top :
				this->setPosition(0.0F,  1.0F - m_rectangle.height());
				break;

			case Position::Left :
				this->setPosition(-1.0F + m_rectangle.width(), 0.0F);
				break;

			case Position::Right :
				this->setPosition(1.0F - m_rectangle.width(), 0.0F);
				break;

			case Position::BottomLeft :
				this->setPosition(-1.0F + m_rectangle.width(), -1.0F + m_rectangle.height());
				break;

			case Position::BottomRight :
				this->setPosition(1.0F - m_rectangle.width(), -1.0F + m_rectangle.height());
				break;

			case Position::TopLeft :
				this->setPosition(-1.0F + m_rectangle.width(), 1.0F - m_rectangle.height());
				break;

			case Position::TopRight :
				this->setPosition(1.0F - m_rectangle.width(), 1.0F - m_rectangle.height());
				break;
		}
	}

	Layouts::Main &
	ComposedSurface::rootLayout () noexcept
	{
		return m_rootLayout;
	}

	const Layouts::Main &
	ComposedSurface::rootLayout () const noexcept
	{
		return m_rootLayout;
	}

	void
	ComposedSurface::drawFinished () noexcept
	{
		if ( !this->onDrawFinished(m_data) )
		{
			return;
		}

		//m_texture->requestUpdate(true);
	}

	std::array< float, 2 >
	ComposedSurface::realSize () const noexcept
	{
		// FIXME: Size must use the overlay notified size !
		const auto windowSize = Window::instance()->getFramebufferSize();

		return {
			static_cast< float >(windowSize[0]) * m_rectangle.width(),
			static_cast< float >(windowSize[1]) * m_rectangle.height()
		};
	}

	std::array< float, 2 >
	ComposedSurface::offsets () const noexcept
	{
		// FIXME: Size must use the overlay notified size !
		const auto windowSize = Window::instance()->getFramebufferSize();

		return {
			static_cast< float >(windowSize[0]) * (m_rectangle.left() - m_rectangle.width() + 1.0F) * Half< float >,
			static_cast< float >(windowSize[1]) * (m_rectangle.top() - m_rectangle.height() + 1.0F) * Half< float >
		};
	}

	const Vector< 2, float > &
	ComposedSurface::textureOffset () const noexcept
	{
		return m_textureOffset;
	}

	void
	ComposedSurface::setTextureOffsetX (float offsetX) noexcept
	{
		m_textureOffset[X] = offsetX;
	}

	void
	ComposedSurface::setTextureOffsetY (float offsetY) noexcept
	{
		m_textureOffset[Y] = offsetY;
	}

	void
	ComposedSurface::setTextureOffset (const Vector< 2, float > & offset) noexcept
	{
		m_textureOffset = offset;
	}

	bool
	ComposedSurface::onResize (const Pixmap< uint8_t > & /*pixmap*/) noexcept
	{
		return true;
	}

	bool
	ComposedSurface::onDrawFinished (Pixmap< uint8_t > & /*pixmap*/) noexcept
	{
		return true;
	}

	bool
	ComposedSurface::updateGeometry () noexcept
	{
		/*if ( m_geometry == nullptr )
		{
			m_geometry = IndexedVertexResource::generateSurface(m_rectangle.width(), m_rectangle.height(), true, m_textureProportion[X], m_textureProportion[Y], true);

			if ( m_geometry == nullptr )
			{
				Tracer::error(ClassId, "Unable to create the geometry of the surface in video memory !");

				return false;
			}
		}
		else
		{
			std::array< float, 4 * (3 + 2) > positions{
				/ * Bottom-left * /
				-m_rectangle.width(), -m_rectangle.height(), 0.0F, 0.0F, 0.0F,
				/ * Bottom-right * /
				 m_rectangle.width(), -m_rectangle.height(), 0.0F, m_textureProportion[X], 0.0F,
				/ * Top-right * /
				-m_rectangle.width(),  m_rectangle.height(), 0.0F, 0.0F, m_textureProportion[Y],
				/ * Top-left * /
				 m_rectangle.width(),  m_rectangle.height(), 0.0F, m_textureProportion[X], m_textureProportion[Y]
			};

			if  ( !m_geometry->update(positions.data(), positions.size()) )
			{
				Tracer::error(ClassId, "Unable to update the geometry of the surface in video memory !");

				return false;
			}
		}*/

		return true;
	}

	bool
	ComposedSurface::updateTexture (float /*screenWidth*/, float /*screenHeight*/) noexcept
	{
		/*const auto width = static_cast< size_t >(screenWidth * m_rectangle.width() * (1.0F / m_textureProportion[X]));
		const auto height = static_cast< size_t >(screenHeight * m_rectangle.height() * (1.0F / m_textureProportion[Y]));

		if ( !m_data.initialize(width, height, ChannelMode::RGBA) )
		{
			Tracer::error(ClassId, "Unable to allocate Pixmap for surface texture !");

			return false;
		}

		if ( m_texture != nullptr )
		{
			m_texture->destroy();
			m_texture.reset();
		}

		m_texture = std::make_shared< TextureRectangle >();

		if ( !m_texture->initialize(m_data) )
		{
			Tracer::error(ClassId, "Unable to create the texture of the surface in video memory !");

			return false;
		}*/

		return true;
	}

	bool
	ComposedSurface::resize () noexcept
	{
		if ( !this->updateGeometry() )
		{
			Tracer::error(ClassId, "Unable to create/update surface geometry !");

			return false;
		}

		// FIXME: Size must use the overlay notified size !
		const auto windowSize = Window::instance()->getFramebufferSize();

		if ( !this->updateTexture(windowSize[0], windowSize[1]) )
		{
			Tracer::error(ClassId, "Unable to create/update surface texture !");

			return false;
		}

		return this->onResize(m_data);
	}
}
