/*
 * src/Graphics/CubemapResource.cpp
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

#include "CubemapResource.hpp"

/* STL inclusions. */
#include <algorithm>
#include <array>
#include <cmath>
#include <numbers>

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
	CubemapResource::load () noexcept
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

			/* Create a retro sunset gradient mapped by elevation angle.
			 * Position 0.0 = zenith (straight up), 1.0 = nadir (straight down).
			 * Extra stops are concentrated on the horizon (0.5) for a richer,
			 * more defined horizon glow. */
			Gradient< float, float > sunsetGradient;
			sunsetGradient.addColorAt(0.00F, Color< float >{0.02F, 0.02F, 0.10F, 1.0F}); /* Zenith: Deep night blue */
			sunsetGradient.addColorAt(0.15F, Color< float >{0.08F, 0.05F, 0.25F, 1.0F}); /* Upper sky: Dark indigo */
			sunsetGradient.addColorAt(0.30F, Color< float >{0.30F, 0.10F, 0.40F, 1.0F}); /* Mid sky: Purple */
			sunsetGradient.addColorAt(0.40F, Color< float >{0.55F, 0.15F, 0.45F, 1.0F}); /* Lower sky: Warm magenta */
			sunsetGradient.addColorAt(0.46F, Color< float >{0.85F, 0.25F, 0.40F, 1.0F}); /* Above horizon: Hot pink */
			sunsetGradient.addColorAt(0.50F, Color< float >{1.00F, 0.40F, 0.30F, 1.0F}); /* Horizon: Bright rose-orange */
			sunsetGradient.addColorAt(0.54F, Color< float >{1.00F, 0.55F, 0.20F, 1.0F}); /* Below horizon: Vivid orange */
			sunsetGradient.addColorAt(0.60F, Color< float >{1.00F, 0.50F, 0.15F, 1.0F}); /* Warm orange */
			sunsetGradient.addColorAt(0.70F, Color< float >{0.95F, 0.60F, 0.25F, 1.0F}); /* Golden orange */
			sunsetGradient.addColorAt(0.85F, Color< float >{0.85F, 0.70F, 0.45F, 1.0F}); /* Soft peach */
			sunsetGradient.addColorAt(1.00F, Color< float >{0.75F, 0.65F, 0.50F, 1.0F}); /* Nadir: Warm sand */

			constexpr auto invSize = 1.0F / static_cast< float >(size);

			for ( size_t faceIndex = 0; faceIndex < CubemapFaceCount; faceIndex++ )
			{
				if ( !m_faces.at(faceIndex).initialize(size, size, ChannelMode::RGBA) )
				{
					TraceError{ClassId} << "Unable to load the default pixmap for face #" << faceIndex << " !";

					return this->setLoadSuccess(false);
				}

				/* Fill each texel using the 3D direction vector's elevation angle.
				 * This produces a seamless spherical gradient across all cube faces. */
				for ( size_t row = 0; row < size; row++ )
				{
					const auto t = 2.0F * (static_cast< float >(row) + 0.5F) * invSize - 1.0F;

					for ( size_t col = 0; col < size; col++ )
					{
						const auto s = 2.0F * (static_cast< float >(col) + 0.5F) * invSize - 1.0F;

						/* Compute the 3D direction vector for this texel based on face. */
						float dx, dy, dz;

						switch ( faceIndex )
						{
							case 0: /* PositiveX */ dx =  1.0F; dy = -t;    dz = -s;    break;
							case 1: /* NegativeX */ dx = -1.0F; dy = -t;    dz =  s;    break;
							case 2: /* PositiveY */ dx =  s;    dy =  1.0F; dz =  t;    break;
							case 3: /* NegativeY */ dx =  s;    dy = -1.0F; dz = -t;    break;
							case 4: /* PositiveZ */ dx =  s;    dy = -t;    dz =  1.0F; break;
							default: /* NegativeZ */ dx = -s;   dy = -t;    dz = -1.0F; break;
						}

						/* Normalize and extract the elevation (Y component).
						 * Y = +1 (zenith) → position 0.0 (dark blue)
						 * Y =  0 (horizon) → position 0.5 (pink)
						 * Y = -1 (nadir) → position 1.0 (light orange) */
						const auto normalizedY = dy / std::sqrt(dx * dx + dy * dy + dz * dz);
						const auto gradientPosition = 0.5F * (1.0F - normalizedY);

						m_faces.at(faceIndex).setPixel(
							row * size + col,
							sunsetGradient.colorAt(gradientPosition)
						);
					}
				}
			}
		}

		return this->setLoadSuccess(true);
	}

	bool
	CubemapResource::load (const std::filesystem::path & filepath) noexcept
	{
		/* Check for a JSON file. */
		if ( IO::getFileExtension(filepath) == "json" )
		{
			return ResourceTrait::load(filepath);
		}
		
		/* Tries to read the pixmap. */
		Pixmap< uint8_t, uint32_t > basemap{};

		if ( !FileIO::read(filepath, basemap) )
		{
			TraceError{ClassId} << "Unable to load the image file '" << filepath << "' !";

			return false;
		}

		/* Auto-detect format based on aspect ratio. */
		const auto ratio = static_cast< float >(basemap.width()) / static_cast< float >(basemap.height());

		if ( std::abs(ratio - 2.0F) < 0.01F )
		{
			/* 2:1 aspect ratio → equirectangular panoramic image. */
			return this->loadEquirectangular(basemap, basemap.height() / 2);
		}

		/* 3:2 aspect ratio or other → packed cubemap (existing behavior). */
		return this->load(basemap);
	}

	bool
	CubemapResource::load (const Json::Value & data) noexcept
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

		const auto & fileSystem = this->serviceProvider().fileSystem();

		/* Checks if cubemap is packed onto one image. */
		if ( data.isMember(PackedKey) && data[PackedKey].asBool() )
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

		/* Checks if cubemap is an equirectangular (panoramic 2:1) image. */
		if ( data.isMember(EquirectangularKey) && data[EquirectangularKey].asBool() )
		{
			const auto filepath = fileSystem.getFilepathFromDataDirectories("data-stores/Cubemaps", this->name() + '.' + EquirectangularKey + '.' + fileFormat);

			if ( filepath.empty() )
			{
				return this->setLoadSuccess(false);
			}

			Pixmap< uint8_t > equirectangular{};

			if ( !FileIO::read(filepath, equirectangular) )
			{
				TraceError{ClassId} << "Unable to read the equirectangular cubemap file '" << filepath << "' !";

				return this->setLoadSuccess(false);
			}

			const uint32_t faceSize = data.isMember("Size")
				? data["Size"].asUInt()
				: equirectangular.height() / 2;

			return this->loadEquirectangular(equirectangular, faceSize);
		}

		/* Unpacked mode: load individual face files. */
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
	CubemapResource::loadEquirectangular (const Pixmap< uint8_t > & equirectangular, uint32_t faceSize) noexcept
	{
		if ( !this->beginLoading() )
		{
			return false;
		}

		if ( !equirectangular.isValid() )
		{
			Tracer::error(ClassId, "Unable to use this equirectangular pixmap to create a cubemap !");

			return this->setLoadSuccess(false);
		}

		/* NOTE: Pixmap UV wrapping is enabled by default, which handles
		 * the longitude seam (u wrap-around) automatically during sampling. */

		constexpr auto pi = std::numbers::pi_v< float >;
		constexpr auto twoPi = 2.0F * pi;
		const auto invSize = 1.0F / static_cast< float >(faceSize);

		for ( size_t faceIndex = 0; faceIndex < CubemapFaceCount; faceIndex++ )
		{
			if ( !m_faces.at(faceIndex).initialize(faceSize, faceSize, ChannelMode::RGBA) )
			{
				TraceError{ClassId} << "Unable to initialize the pixmap for face #" << faceIndex << " !";

				return this->setLoadSuccess(false);
			}

			for ( uint32_t row = 0; row < faceSize; row++ )
			{
				const auto t = 2.0F * (static_cast< float >(row) + 0.5F) * invSize - 1.0F;

				for ( uint32_t col = 0; col < faceSize; col++ )
				{
					const auto s = 2.0F * (static_cast< float >(col) + 0.5F) * invSize - 1.0F;

					/* Compute the 3D direction vector for this texel based on face. */
					float dx, dy, dz;

					switch ( faceIndex )
					{
						case 0: /* PositiveX */ dx =  1.0F; dy = -t;    dz = -s;    break;
						case 1: /* NegativeX */ dx = -1.0F; dy = -t;    dz =  s;    break;
						case 2: /* PositiveY */ dx =  s;    dy =  1.0F; dz =  t;    break;
						case 3: /* NegativeY */ dx =  s;    dy = -1.0F; dz = -t;    break;
						case 4: /* PositiveZ */ dx =  s;    dy = -t;    dz =  1.0F; break;
						default: /* NegativeZ */ dx = -s;   dy = -t;    dz = -1.0F; break;
					}

					/* Normalize direction vector. */
					const auto length = std::sqrt(dx * dx + dy * dy + dz * dz);
					const auto nx = dx / length;
					const auto ny = dy / length;
					const auto nz = dz / length;

					/* Convert to equirectangular UV coordinates. */
					const auto theta = std::atan2(nz, nx);
					const auto phi = std::asin(ny);
					const auto u = theta / twoPi + 0.5F;
					const auto v = 0.5F - phi / pi;

					/* Sample the equirectangular source with bilinear interpolation. */
					const auto color = equirectangular.linearSample(u, v);

					m_faces.at(faceIndex).setPixel(col, row, color);
				}
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

	bool
	CubemapResource::load (const Color< float > & color, uint32_t size) noexcept
	{
		if ( !this->beginLoading() )
		{
			return false;
		}

		for ( size_t faceIndex = 0; faceIndex < CubemapFaceCount; faceIndex++ )
		{
			if ( !m_faces.at(faceIndex).initialize(size, size, ChannelMode::RGBA) )
			{
				TraceError{ClassId} << "Unable to initialize the pixmap for face #" << faceIndex << " !";

				return this->setLoadSuccess(false);
			}

			if ( !m_faces.at(faceIndex).fill(color) )
			{
				TraceError{ClassId} << "Unable to fill the pixmap for face #" << faceIndex << " !";

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
