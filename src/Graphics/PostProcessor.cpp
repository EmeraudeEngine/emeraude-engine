/*
 * src/Graphics/PostProcessor.cpp
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

#include "PostProcessor.hpp"

/* Local inclusions. */
#include "Graphics/RenderTarget/View.hpp"
#include "Saphir/FramebufferEffectInterface.hpp"
#include "Tracer.hpp"

namespace EmEn::Graphics
{
	using namespace EmEn::Libs;
	using namespace EmEn::Libs::Math;
	using namespace EmEn::Saphir;
	using namespace EmEn::Vulkan;

	PostProcessor::PostProcessor (unsigned int width, unsigned int height, unsigned int colorBufferBits, unsigned int depthBufferBits, unsigned int stencilBufferBits, unsigned int samples) noexcept
	{

	}

	bool
	PostProcessor::resize (unsigned int /*width*/, unsigned int /*height*/) noexcept
	{
		return false;
	}

	bool
	PostProcessor::usable () const noexcept
	{
		return false;
	}

	void
	PostProcessor::begin () noexcept
	{

	}

	void
	PostProcessor::end () noexcept
	{

	}

	void
	PostProcessor::setEffectsList (const FramebufferEffectsList & effectsList) noexcept
	{

	}

	void
	PostProcessor::clearEffects () noexcept
	{

	}

	void
	PostProcessor::render (const Space2D::AARectangle< uint32_t > & /*region*/) const noexcept
	{

	}

	bool
	PostProcessor::isMultisamplingEnabled () const noexcept
	{
		return false;
	}

	void
	PostProcessor::setBackgroundColor (const PixelFactory::Color< float > & /*color*/) noexcept
	{

	}

	bool
	PostProcessor::loadGeometry () noexcept
	{
		return false;
	}

	bool
	PostProcessor::loadProgram () noexcept
	{
		return false;
	}

	bool
	PostProcessor::buildFramebuffer (unsigned int /*width*/, unsigned int /*height*/, unsigned int /*colorBufferBits*/, unsigned int /*depthBufferBits*/, unsigned int /*stencilBufferBits*/, unsigned int /*samples*/) noexcept
	{
		return false;
	}
}
