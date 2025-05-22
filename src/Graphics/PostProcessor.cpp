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
	using namespace Saphir;
	using namespace Vulkan;

	PostProcessor::PostProcessor (unsigned int width, unsigned int height, unsigned int colorBufferBits, unsigned int depthBufferBits, unsigned int stencilBufferBits, unsigned int samples) noexcept
	{
		if ( width * height == 0 )
		{
			Tracer::error(ClassId, "Framebuffer dimensions are invalid !");

			return;
		}

		m_flags[ServiceInitialized] = this->buildFramebuffer(width, height, colorBufferBits, depthBufferBits, stencilBufferBits, samples);
	}

	bool
	PostProcessor::resize (unsigned int /*width*/, unsigned int /*height*/) noexcept
	{
		#ifdef VULKAN_DEV
		if ( width * height == 0 )
		{
			Tracer::error(ClassId, "Framebuffer dimensions are invalid !");

			return false;
		}

		/* Destroys the old framebuffer. */
		m_colorBuffer.destroy();
		m_extraBuffer.destroy();
		m_framebuffer.destroy();

		/* Recreate a new resized framebuffer. */
		const auto & framebuffer = Core::instance()->graphicsRenderer().defaultFramebuffer();

		const auto colorBufferBits = framebuffer.precision(BufferType::ColorBuffer);
		const auto depthBufferBits = framebuffer.precision(BufferType::DepthBuffer);
		const auto stencilBufferBits = framebuffer.precision(BufferType::StencilBuffer);
		const auto samples = static_cast< unsigned int >(framebuffer.samples());

		if ( !this->buildFramebuffer(width, height, colorBufferBits, depthBufferBits, stencilBufferBits, samples) )
		{
			Tracer::error(ClassId, "Unable to resize framebuffer !");

			return false;
		}

		m_flags[ServiceInitialized] = true;
		#endif

		return true;
	}

	bool
	PostProcessor::usable () const noexcept
	{
		return m_flags[ServiceInitialized];
	}

	void
	PostProcessor::begin () noexcept
	{
		/*m_framebuffer.enable();

		m_framebuffer.clearBuffers();*/
	}

	void
	PostProcessor::end () noexcept
	{
		//m_framebuffer.disable();
	}

	void
	PostProcessor::setEffectsList (const FramebufferEffectsList & effectsList) noexcept
	{
		m_effectsList = effectsList;

		m_flags[UpdateRequested] = true;
	}

	void
	PostProcessor::clearEffects () noexcept
	{
		if ( m_effectsList.empty() )
		{
			return;
		}

		m_effectsList.clear();

		m_flags[UpdateRequested] = true;
	}

	void
	PostProcessor::render (const Space2D::AARectangle< uint32_t > & /*region*/) const noexcept
	{
		/*if ( m_effectsList.empty() )
		{
			m_framebuffer.blitToDefaultFramebuffer(region, true);
		}
		else
		{
			m_program->use();

			auto textureUnit = 0U;

			m_program->bind(Samplers::PrimaryTexture, m_colorBuffer, textureUnit++);

			for ( auto & effect : m_effectsList )
				effect->activate(*m_program, textureUnit);

			Core::instance()->graphicsRenderer().setRasterizationMode(m_rasterizationMode);

			m_geometry->bind();
			m_geometry->draw();
		}*/
	}

	/*const Framebuffer *
	PostProcessor::framebuffer () const noexcept
	{
		return &m_framebuffer;
	}*/

	bool
	PostProcessor::isMultisamplingEnabled () const noexcept
	{
		//return m_colorBuffer.samples() > 0;
		return false;
	}

	void
	PostProcessor::setBackgroundColor (const PixelFactory::Color< float > & /*color*/) noexcept
	{
		//m_framebuffer.setClearColor(color);
	}

	bool
	PostProcessor::loadGeometry () noexcept
	{
		/* Creates the geometry */
		/*m_geometry = Geometry::IndexedVertexResource::generateSurface(1.0F, 1.0F, true);

		if ( m_geometry == nullptr )
		{
			Tracer::error(ClassId, "Unable to create the geometry of the post-processor surface in video memory !");

			return false;
		}*/

		/* Prepare the rasterization mode for the surface */
		//m_rasterizationMode.setDepthBufferStates(false, false);

		return true;
	}

	bool
	PostProcessor::loadProgram () noexcept
	{
#ifdef VULKAN_DEV
		if ( m_program == nullptr )
		{
			const auto requestScreenSize = std::any_of(m_effectsList.cbegin(), m_effectsList.cend(), [] (const auto & effect) {return effect->requestScreenSize();});
			const auto samples = m_colorBuffer.samples();

			/* Vertex Shader stage. */
			auto vertexShader = std::make_shared< VertexShader >("PostProcessor");

			{
				OLDShaderGenerator generator(vertexShader);

				if ( requestScreenSize )
					generator.declare ViewUniformBlock();

				generator.declare(InputAttribute{VertexAttributeType::Position}, true);

				generator.addOutputInstruction(Blob() << "gl_Position = vec4(" << Attribute::Position << ", 1.0);");

				/* Fragment texture coodinates in case of non-multisample render or simply requested by a shader. */
				if ( samples == 0 || std::any_of(m_effectsList.cbegin(), m_effectsList.cend(), [] (const auto & effect) {return effect->requestScreenCoordinates();}) )
				{
					generator.declare(InputAttribute{VertexAttributeType::PrimaryTextureCoordinates});

					generator.declare(ShaderStageOutput{GLSL::FloatVector2, ShaderVariable::TextureCoordinates, GLSL::Smooth});

					generator.addOutputInstruction(Blob() << ShaderVariable::TextureCoordinates << " = " << Attribute::PrimaryTextureCoordinates << ';');
				}

				/* Effect code generation. */
				if ( !m_effectsList.empty() )
				{
					auto result = std::any_of(m_effectsList.cbegin(), m_effectsList.cend(), [&generator, samples] (const auto & effect) mutable {
						return effect->generate(generator, samples);
					});

					if ( !result )
					{
						Tracer::error(ClassId, "An effect failed to be generated for the post-processor vertex stage !");

						return false;
					}
				}

				if ( !generator.writeShader() )
				{
					Tracer::error(ClassId, "Unable to generate the vertex shader for the post-processor program !");

					return false;
				}
			}

			/* Fragment Shader stage. */
			auto fragmentShader = std::make_shared< FragmentShader >("PostProcessor");

			{
				OLDShaderGenerator generator(fragmentShader);

				if ( requestScreenSize )
					generator.declare ViewUniformBlock();

				generator.declareDefaultOutputFragment();

				auto overrideFragmentFetching = std::any_of(m_effectsList.cbegin(), m_effectsList.cend(), [&generator, samples] (const auto & effect) mutable {
					if ( effect->overrideFragmentFetching() )
						return effect->generate(generator, samples);

					return false;
				});

				/* Multisample texture reading. */
				if ( !overrideFragmentFetching )
				{
					if ( samples > 0 )
					{
						generator.declare(ShaderUniform{GLSL::Sampler2DMS, Samplers::PrimaryTexture});

						generator.addInstruction(Blob() <<
							GLSL::FloatVector4 << " sampledColor = vec4(0.0, 0.0, 0.0, 1.0);" "\n\t" <<
							GLSL::IntegerVector2 << " texCoords = ivec2(gl_FragCoord.xy);" "\n\n" <<

							"\t" "for ( int subSample = 0; subSample < " << samples << "; subSample++ )" "\n"
							"\t" "{" "\n"
							"\t\t" "sampledColor += texelFetch(" << Samplers::PrimaryTexture << ", texCoords, subSample);" "\n"
							"\t" "}" "\n\n\t" <<

							GLSL::FloatVector4 << ' ' << Fragment << " = clamp(sampledColor / float(" << m_colorBuffer.samples() << "), 0.0, 1.0);"
						);
					}
					else
					{
						generator.declare(ShaderUniform{GLSL::Sampler2D, Samplers::PrimaryTexture});

						generator.declare(ShaderStageInput{GLSL::FloatVector2, ShaderVariable::TextureCoordinates, GLSL::Smooth});

						generator.addInstruction(Blob() << GLSL::FloatVector4 << ' ' << Fragment << " = texture(" << Samplers::PrimaryTexture << ", " << ShaderVariable::TextureCoordinates << ");");
					}
				}

				/* Effect code generation. */
				for ( const auto & effect : m_effectsList )
				{
					if ( effect->overrideFragmentFetching() )
						continue;

					if ( !effect->generate(generator, samples) )
					{
						Tracer::error(ClassId, "An effect failed to be generated for the post-processor fragment stage !");

						return false;
					}
				}

				generator.addOutputInstruction(Blob() << ShaderVariable::OutputFragment << " = " << Fragment << ';');

				if ( !generator.writeShader() )
				{
					Tracer::error(ClassId, "Unable to generate the fragment shader source code for the post-processor program !");

					return false;
				}
			}

			m_program = Core::instance()->programsManager().getProgram(vertexShader, fragmentShader, "PostProcessorProgram");

			if ( m_program == nullptr )
			{
				Tracer::error(ClassId, "Unable to get post-processor program !");

				return false;
			}
		}
		else
		{
			Tracer::info(ClassId, "Program already built !");
		}
#endif

		return true;
	}

	bool
	PostProcessor::buildFramebuffer (unsigned int /*width*/, unsigned int /*height*/, unsigned int /*colorBufferBits*/, unsigned int /*depthBufferBits*/, unsigned int /*stencilBufferBits*/, unsigned int /*samples*/) noexcept
	{
		#ifdef VULKAN_DEV
		if ( width == 0 || height == 0 )
		{
			Tracer::error(ClassId, "Framebuffer dimensions are invalid !");

			return false;
		}

		if ( colorBufferBits == 0 )
		{
			Tracer::error(ClassId, "Framebuffer color bits are invalid !");

			return false;
		}

		Tracer::info(ClassId, Blob() << "Initializing a " << width << "x" << height << " off-screen framebuffer. ColorBits:" << colorBufferBits << ", DepthBits:" << depthBufferBits << ", StencilBits:" << stencilBufferBits << ", Samples:" << samples << ".");

		/* Creates the framebuffer. */
		if ( !m_framebuffer.create() )
		{
			Tracer::error(ClassId, "Unable to create a framebuffer !");

			return false;
		}

		/* Creates the color buffer. */
		if ( !m_colorBuffer.initialize(width, height, colorBufferBits / 8) )
		{
			Tracer::error(ClassId, "Unable to create the color buffer !");

			return false;
		}

		m_framebuffer.addColorBuffer(m_colorBuffer);

		/* Creates the depth/stencil buffer. */
		if ( !m_extraBuffer.initialize(width, height, depthBufferBits, stencilBufferBits, samples) )
		{
			Tracer::error(ClassId, "Unable to create extra buffers !");

			return false;
		}

		m_framebuffer.addExtraBuffers(m_extraBuffer);

		if ( !m_framebuffer.check() )
		{
			Tracer::error(ClassId, "Framebuffer validation failed !");

			return false;
		}

		if ( !this->loadGeometry() )
		{
			Tracer::error(ClassId, "Unable to load the geometry !");

			return false;
		}

		if ( !this->loadProgram() )
		{
			Tracer::error(ClassId, "Unable to get the shading program !");

			return false;
		}
#endif

		return true;
	}
}
