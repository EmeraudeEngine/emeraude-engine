/*
 * src/Overlay/Elements/Picture.cpp
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

#include "Picture.hpp"

/* Local inclusions. */
#include "Libs/PixelFactory/Processor.hpp"
#include "Graphics/ImageResource.hpp"

namespace EmEn::Overlay::Elements
{
	using namespace EmEn::Libs;
	using namespace EmEn::Libs::Math;
	using namespace EmEn::Libs::PixelFactory;
	using namespace Graphics;

	Picture::Picture (const FramebufferProperties & framebufferProperties, const std::string & name, const Math::Space2D::AARectangle< float > & geometry, float depth) noexcept
		: ComposedSurface(framebufferProperties, name, geometry, depth)
	{

	}

	void
	Picture::setImage (const std::shared_ptr< ImageResource > & image, Alignment alignment) noexcept
	{
		m_image = image;
		m_alignment = alignment;
	}

	std::shared_ptr< Graphics::ImageResource >
	Picture::image () const noexcept
	{
		return m_image;
	}

	void
	Picture::setOpacity (float value) noexcept
	{
		m_opacity = clampToUnit(value);
	}

	float
	Picture::opacity () const noexcept
	{
		return m_opacity;
	}

	bool
	Picture::onDrawFinished (Pixmap< uint8_t > & pixmap) noexcept
	{
		switch ( m_alignment )
		{
			/* Simply resize the image to the surface dimension. */
			case Alignment::FitToSurface :
				pixmap = Processor< uint8_t >::resize(m_image->data(), pixmap.width(), pixmap.height());
				break;
		}

		auto alpha = std::round(255.0F * m_opacity);

		return pixmap.fillChannel(Channel::Alpha, static_cast< uint8_t >(alpha));
	}
}
