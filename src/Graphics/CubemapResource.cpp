/*
 * src/Graphics/CubemapResource.cpp
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

#include "CubemapResource.hpp"

/* STL inclusions. */
#include <algorithm>
#include <array>

/* Local inclusions. */
#include "Libs/PixelFactory/FileIO.hpp"
#include "Graphics/TextureResource/Abstract.hpp"
#include "Resources/Manager.hpp"

namespace EmEn::Graphics
{
	using namespace Libs;
	using namespace Libs::Math;
	using namespace Libs::PixelFactory;

	bool
	CubemapResource::load (Resources::AbstractServiceProvider & /*serviceProvider*/) noexcept
	{
		if ( !this->beginLoading() )
		{
			return false;
		}

		if constexpr ( IsDebug )
		{
			constexpr size_t size{32};

			constexpr std::array< Color< float >, CubemapFaceCount > colors{
				Red, Cyan,
				Green, Magenta,
				Blue, Yellow
			};

			for ( size_t faceIndex = 0; faceIndex < CubemapFaceCount; faceIndex++ )
			{
				if ( !m_faces.at(faceIndex).initialize(size, size, ChannelMode::RGBA) )
				{
					TraceError{ClassId} << "Unable to load the default pixmap for face #" << faceIndex << " !";

					return this->setLoadSuccess(false);
				}

				if ( !m_faces.at(faceIndex).fill(colors.at(faceIndex)) )
				{
					TraceError{ClassId} << "Unable to fill the default pixmap for face #" << faceIndex << " !";

					return this->setLoadSuccess(false);
				}
			}
		}
		else
		{
			constexpr size_t size{512};

			/* Create a retro sunset gradient (orange -> pink -> purple -> dark blue). */
			Gradient< float, float > sunsetGradient;
			sunsetGradient.addColorAt(0.0F, Color< float >{0.05F, 0.05F, 0.15F, 1.0F});  /* Top: Dark blue */
			sunsetGradient.addColorAt(0.3F, Color< float >{0.3F, 0.1F, 0.4F, 1.0F});     /* Purple */
			sunsetGradient.addColorAt(0.5F, Color< float >{0.9F, 0.3F, 0.4F, 1.0F});     /* Pink/Rose */
			sunsetGradient.addColorAt(0.7F, Color< float >{1.0F, 0.5F, 0.2F, 1.0F});     /* Orange */
			sunsetGradient.addColorAt(1.0F, Color< float >{0.95F, 0.8F, 0.6F, 1.0F});    /* Bottom: Light orange/yellow */

			for ( size_t faceIndex = 0; faceIndex < CubemapFaceCount; faceIndex++ )
			{
				if ( !m_faces.at(faceIndex).initialize(size, size, ChannelMode::RGBA) )
				{
					TraceError{ClassId} << "Unable to load the default pixmap for face #" << faceIndex << " !";

					return this->setLoadSuccess(false);
				}

				/* Apply gradient differently based on face orientation. */
				switch ( faceIndex )
				{
					case 0: /* PositiveX (Right) */
					case 1: /* NegativeX (Left) */
					case 4: /* PositiveZ (Front) */
					case 5: /* NegativeZ (Back) */
						/* Side faces: horizontal gradient (left to right). */
						if ( !m_faces.at(faceIndex).fillHorizontal(sunsetGradient) )
						{
							TraceError{ClassId} << "Unable to fill gradient for face #" << faceIndex << " !";

							return this->setLoadSuccess(false);
						}
						break;

					case 2: /* PositiveY (Top) */
						/* Top face: solid dark blue (sky). */
						if ( !m_faces.at(faceIndex).fill(Color< float >{0.05F, 0.05F, 0.15F, 1.0F}) )
						{
							TraceError{ClassId} << "Unable to fill top face !";

							return this->setLoadSuccess(false);
						}
						break;

					case 3: /* NegativeY (Bottom) */
						/* Bottom face: solid light orange (horizon glow). */
						if ( !m_faces.at(faceIndex).fill(Color< float >{0.95F, 0.8F, 0.6F, 1.0F}) )
						{
							TraceError{ClassId} << "Unable to fill bottom face !";

							return this->setLoadSuccess(false);
						}
						break;
				}
			}
		}

		return this->setLoadSuccess(true);
	}

	bool
	CubemapResource::load (Resources::AbstractServiceProvider & serviceProvider, const std::filesystem::path & filepath) noexcept
	{
		/* Check for a JSON file. */
		if ( IO::getFileExtension(filepath) == "json" )
		{
			return ResourceTrait::load(serviceProvider, filepath);
		}
		
		/* Tries to read the pixmap. */
		Pixmap< uint8_t, uint32_t > basemap{};

		if ( !FileIO::read(filepath, basemap) )
		{
			TraceError{ClassId} << "Unable to load the image file '" << filepath << "' !";

			return false;
		}

		return this->load(basemap);
	}

	bool
	CubemapResource::load (Resources::AbstractServiceProvider & serviceProvider, const Json::Value & data) noexcept
	{
		if ( !this->beginLoading() )
		{
			return false;
		}
		
		/* Checks file format. */
		if ( !data.isMember(FileFormatKey) || !data[FileFormatKey].isString() )
		{
			TraceError{ClassId} << "There is no valid '" << FileFormatKey << "' key in cubemap definition !";

			return this->setLoadSuccess(false);
		}

		const auto fileFormat = data[FileFormatKey].asString();

		/* Checks if cubemap is packed onto one image. */
		if ( !data.isMember(PackedKey) )
		{
			TraceError{ClassId} << "There is no '" << PackedKey << "' key in cubemap definition !";

			return this->setLoadSuccess(false);
		}

		const auto & fileSystem = serviceProvider.fileSystem();

		if ( data[PackedKey].asBool() )
		{
			const auto filepath = fileSystem.getFilepathFromDataDirectories("data-stores/Cubemaps", this->name() + '.' + PackedKey + '.' + fileFormat);

			if ( filepath.empty() )
			{
				return this->setLoadSuccess(false);
			}

			Pixmap< uint8_t > basemap{};

			if ( !FileIO::read(filepath, basemap) )
			{
				TraceError{ClassId} << "Unable to read the packed cubemap file '" << filepath << "' !";

				return this->setLoadSuccess(false);
			}

			return this->load(basemap);
		}

		for ( size_t faceIndex = 0; faceIndex < CubemapFaceCount; faceIndex++ )
		{
			const auto filepath = fileSystem.getFilepathFromDataDirectories("data-stores/Cubemaps", this->name() + '.' + CubemapFaceNames.at(faceIndex) + '.' + fileFormat);

			if ( filepath.empty() )
			{
				return this->setLoadSuccess(false);
			}

			if ( !FileIO::read(filepath, m_faces.at(faceIndex)) )
			{
				TraceError{ClassId} << "Unable to load plane '" << CubemapFaceNames.at(faceIndex) << "' from file '" << filepath << "' !";

				return this->setLoadSuccess(false);
			}

			if ( !TextureResource::Abstract::validatePixmap(ClassId, this->name(), m_faces.at(faceIndex)) )
			{
				TraceError{ClassId} << "Unable to use the pixmap #" << faceIndex << " for face '" << CubemapFaceNames.at(faceIndex) << "' to create a cubemap !";

				return this->setLoadSuccess(false);
			}
		}

		return this->setLoadSuccess(true);
	}

	bool
	CubemapResource::load (const Pixmap< uint8_t > & pixmap) noexcept
	{
		if ( !this->beginLoading() )
		{
			return false;
		}
		
		if ( !pixmap.isValid() )
		{
			Tracer::error(ClassId, "Unable to use this pixmap to create a cubemap !");

			return this->setLoadSuccess(false);
		}

		const auto width = static_cast< uint32_t >(pixmap.width() / 3);
		const auto height = static_cast< uint32_t >(pixmap.height() / 2);

		const std::array< Space2D::AARectangle< uint32_t >, CubemapFaceCount > rectangles{{
			/* PositiveX */
			{0, 0, width, height},
			/* NegativeX */
			{0, height, width, height},
			/* PositiveY */
			{width, 0, width, height},
			/* NegativeY */
			{width, height, width, height},
			/* PositiveZ */
			{width + width, 0, width, height},
			/* NegativeZ */
			{width + width, height, width, height}
		}};

		for ( size_t faceIndex = 0; faceIndex < CubemapFaceCount; faceIndex++ )
		{
			m_faces.at(faceIndex) = Processor< uint8_t >::crop(pixmap, rectangles.at(faceIndex));

			if ( !TextureResource::Abstract::validatePixmap(ClassId, this->name(), m_faces.at(faceIndex)) )
			{
				TraceError{ClassId} << "Unable to use the pixmap #" << faceIndex << " for face '" << CubemapFaceNames.at(faceIndex) << "' to create a cubemap !";

				return this->setLoadSuccess(false);
			}
		}

		return this->setLoadSuccess(true);
	}

	bool
	CubemapResource::load (const CubemapPixmaps & pixmaps) noexcept
	{
		if ( !this->beginLoading() )
		{
			return false;
		}
		
		for ( size_t faceIndex = 0; faceIndex < CubemapFaceCount; faceIndex++ )
		{
			m_faces.at(faceIndex) = pixmaps.at(faceIndex);

			if ( !TextureResource::Abstract::validatePixmap(ClassId, this->name(), m_faces.at(faceIndex)) )
			{
				TraceError{ClassId} << "Unable to use the pixmap #" << faceIndex << " for face '" << CubemapFaceNames.at(faceIndex) << "' to create a cubemap !";

				return this->setLoadSuccess(false);
			}
		}

		return this->setLoadSuccess(true);
	}

	const Pixmap< uint8_t > &
	CubemapResource::data (size_t faceIndex) const noexcept
	{
		if ( faceIndex >= CubemapFaceCount )
		{
			Tracer::error(ClassId, "Face index overflow !");

			faceIndex = 0;
		}

		return m_faces.at(faceIndex);
	}

	bool
	CubemapResource::isGrayScale () const noexcept
	{
		return std::ranges::all_of(m_faces, [] (const auto & pixmap) {
			if ( !pixmap.isValid() )
			{
				return false;
			}
			
			return pixmap.isGrayScale();
		});
	}

	Color< float >
	CubemapResource::averageColor () const noexcept
	{
		if ( !this->isLoaded() )
		{
			return {};
		}
		
		constexpr auto ratio{1.0F / static_cast< float >(CubemapFaceCount)};

		auto red = 0.0F;
		auto green = 0.0F;
		auto blue = 0.0F;

		for ( const auto & face : m_faces )
		{
			const auto averageColor = face.averageColor();

			red += averageColor.red() * ratio;
			green += averageColor.green() * ratio;
			blue += averageColor.blue() * ratio;
		}

		return {red, green, blue, 1.0F};
	}
}
