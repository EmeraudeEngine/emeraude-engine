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
 * https://github.com/EmeraudeEngine/emeraude-engine
 *
 * --- THIS IS AUTOMATICALLY GENERATED, DO NOT CHANGE ---
 */

#include "CubemapResource.hpp"

/* STL inclusions. */
#include <algorithm>
#include <array>
#include <cmath>
#include <numbers>

/* STL inclusions. */
#include <cstring>

/* Local inclusions. */
#include "FileSystem.hpp"
#include "Graphics/TextureResource/Abstract.hpp"
#include "PixelFactory/FileIO.hpp"
#include "Resources/Manager.hpp"

namespace EmEn::Graphics
{
	using namespace Base;
	using namespace Base::Math;
	using namespace Base::PixelFactory;

	namespace
	{
		/**
		 * @brief Bilinear RGB sample on the RAW float data of an equirectangular pixmap.
		 * @warning Color< float > clamps its components to [0,1] on construction, which would
		 * destroy HDR radiances — every HDR sampling path must stay on raw floats.
		 */
		void
		sampleEquirectangularHDR (const Pixmap< float > & source, float u, float v, std::array< float, 3 > & radiance) noexcept
		{
			const auto width = source.width();
			const auto height = source.height();
			const auto colorCount = static_cast< size_t >(source.colorCount());
			const auto * data = source.data().data();

			/* Longitude wrap, latitude clamp. */
			u = u - std::floor(u);
			v = std::clamp(v, 0.0F, 1.0F);

			const auto realX = static_cast< float >(width - 1) * u;
			const auto realY = static_cast< float >(height - 1) * v;
			const auto loX = static_cast< size_t >(realX);
			const auto loY = static_cast< size_t >(realY);
			const auto hiX = std::min(loX + 1, static_cast< size_t >(width - 1));
			const auto hiY = std::min(loY + 1, static_cast< size_t >(height - 1));
			const auto fracX = realX - static_cast< float >(loX);
			const auto fracY = realY - static_cast< float >(loY);

			for ( size_t channel = 0; channel < 3; ++channel )
			{
				const auto topLeft = data[(loY * width + loX) * colorCount + channel];
				const auto topRight = data[(loY * width + hiX) * colorCount + channel];
				const auto bottomLeft = data[(hiY * width + loX) * colorCount + channel];
				const auto bottomRight = data[(hiY * width + hiX) * colorCount + channel];

				const auto top = topLeft + (topRight - topLeft) * fracX;
				const auto bottom = bottomLeft + (bottomRight - bottomLeft) * fracX;

				radiance[channel] = top + (bottom - top) * fracY;
			}
		}

		/**
		 * @brief Converts a float to an IEEE 754 half (binary16).
		 * @note Overflows clamp to the half maximum (65504) instead of infinity — a radiance
		 * spike must stay a finite, filterable value — and denormals flush to zero, both
		 * irrelevant at radiance scales. Truncation rounding (max 1 ULP, ~0.1%).
		 */
		[[nodiscard]]
		uint16_t
		floatToHalf (float value) noexcept
		{
			uint32_t bits = 0;
			std::memcpy(&bits, &value, sizeof(bits));

			const auto sign = static_cast< uint16_t >((bits >> 16) & 0x8000U);
			const auto exponent = static_cast< int32_t >((bits >> 23) & 0xFFU) - 127 + 15;
			const auto mantissa = bits & 0x7FFFFFU;

			if ( exponent >= 31 )
			{
				/* NOTE: 0x7BFF = 65504.0, the largest finite half. */
				return sign | 0x7BFFU;
			}

			if ( exponent <= 0 )
			{
				return sign;
			}

			return static_cast< uint16_t >(sign | (static_cast< uint32_t >(exponent) << 10) | (mantissa >> 13));
		}
	}

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
							case 0: /* PositiveX */ dx =  1.0F; dy = -t;	dz = -s;	break;
							case 1: /* NegativeX */ dx = -1.0F; dy = -t;	dz =  s;	break;
							case 2: /* PositiveY */ dx =  s;	dy =  1.0F; dz =  t;	break;
							case 3: /* NegativeY */ dx =  s;	dy = -1.0F; dz = -t;	break;
							case 4: /* PositiveZ */ dx =  s;	dy = -t;	dz =  1.0F; break;
							default: /* NegativeZ */ dx = -s;   dy = -t;	dz = -1.0F; break;
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

		const auto & fileSystem = this->serviceProvider().primaryServices().fileSystem();

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

			/* HDR source (Radiance RGBE): float pipeline, photometric calibration. */
			if ( fileFormat == "hdr" )
			{
				Pixmap< float > equirectangular{};

				if ( !FileIO::read(filepath, equirectangular) )
				{
					TraceError{ClassId} << "Unable to read the HDR equirectangular cubemap file '" << filepath << "' !";

					return this->setLoadSuccess(false);
				}

				const uint32_t faceSize = data.isMember("Size")
					? data["Size"].asUInt()
					: equirectangular.height() / 2;

				return this->loadEquirectangularHDR(equirectangular, faceSize);
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

		if ( fileFormat == "hdr" )
		{
			TraceError{ClassId} << "HDR cubemaps are only supported through the '" << EquirectangularKey << "' layout !";

			return this->setLoadSuccess(false);
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
						case 0: /* PositiveX */ dx =  1.0F; dy = -t;	dz = -s;	break;
						case 1: /* NegativeX */ dx = -1.0F; dy = -t;	dz =  s;	break;
						case 2: /* PositiveY */ dx =  s;	dy =  1.0F; dz =  t;	break;
						case 3: /* NegativeY */ dx =  s;	dy = -1.0F; dz = -t;	break;
						case 4: /* PositiveZ */ dx =  s;	dy = -t;	dz =  1.0F; break;
						default: /* NegativeZ */ dx = -s;   dy = -t;	dz = -1.0F; break;
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
	CubemapResource::loadEquirectangularHDR (const Pixmap< float > & equirectangular, uint32_t faceSize) noexcept
	{
		if ( !this->beginLoading() )
		{
			return false;
		}

		if ( !equirectangular.isValid() || equirectangular.colorCount() < 3 || faceSize == 0 )
		{
			Tracer::error(ClassId, "Unable to use this HDR equirectangular pixmap to create a cubemap !");

			return this->setLoadSuccess(false);
		}

		constexpr auto pi = std::numbers::pi_v< float >;
		constexpr auto twoPi = 2.0F * pi;

		const auto width = equirectangular.width();
		const auto height = equirectangular.height();
		const auto colorCount = static_cast< size_t >(equirectangular.colorCount());
		const auto * sourceData = equirectangular.data().data();

		/* Photometric calibration (owner decision D6, Unity-like): measure the illuminance the
		 * upper hemisphere pours on a horizontal ground — E = sum(L * cos(zenith) * dOmega),
		 * with dOmega = (2pi/W)(pi/H)cos(elevation) per equirectangular texel — and derive the
		 * factor that makes the source behave like a UNIFORM DOME of luminance 1 (E = pi lux).
		 * The Background "Luminance" key (nits) then scales an HDR sky exactly like an LDR one,
		 * and the average color stays representative. Rec.709 luma on linear radiances. */
		double hemisphereIlluminance = 0.0;
		double sphereWeightedLuma[3] = {0.0, 0.0, 0.0};
		double sphereSolidAngle = 0.0;

		for ( uint32_t row = 0; row < height; ++row )
		{
			const auto elevation = pi * (0.5F - (static_cast< float >(row) + 0.5F) / static_cast< float >(height));
			const auto texelSolidAngle = (twoPi / static_cast< float >(width)) * (pi / static_cast< float >(height)) * std::cos(elevation);
			const auto groundCosine = std::sin(elevation);

			const auto * rowData = sourceData + static_cast< size_t >(row) * width * colorCount;

			double rowSum[3] = {0.0, 0.0, 0.0};

			for ( uint32_t col = 0; col < width; ++col )
			{
				rowSum[0] += rowData[col * colorCount + 0];
				rowSum[1] += rowData[col * colorCount + 1];
				rowSum[2] += rowData[col * colorCount + 2];
			}

			for ( size_t channel = 0; channel < 3; ++channel )
			{
				sphereWeightedLuma[channel] += rowSum[channel] * texelSolidAngle;
			}

			sphereSolidAngle += static_cast< double >(texelSolidAngle) * width;

			/* Only the sky half lights the ground. */
			if ( groundCosine > 0.0F )
			{
				const auto rowLuma = 0.2126 * rowSum[0] + 0.7152 * rowSum[1] + 0.0722 * rowSum[2];

				hemisphereIlluminance += rowLuma * texelSolidAngle * groundCosine;
			}
		}

		if ( hemisphereIlluminance <= 0.0 )
		{
			Tracer::error(ClassId, "The HDR source has no energy in its upper hemisphere, unable to calibrate it !");

			return this->setLoadSuccess(false);
		}

		const auto calibration = static_cast< float >(std::numbers::pi / hemisphereIlluminance);

		/* Average color over the sphere, calibrated then clamped: the LightSet ambient tint. */
		{
			const auto red = std::clamp(static_cast< float >(sphereWeightedLuma[0] / sphereSolidAngle) * calibration, 0.0F, 1.0F);
			const auto green = std::clamp(static_cast< float >(sphereWeightedLuma[1] / sphereSolidAngle) * calibration, 0.0F, 1.0F);
			const auto blue = std::clamp(static_cast< float >(sphereWeightedLuma[2] / sphereSolidAngle) * calibration, 0.0F, 1.0F);

			m_averageColorHDR = {red, green, blue, 1.0F};
		}

		/* Project the six faces straight to calibrated RGBA16F texels. */
		const auto invSize = 1.0F / static_cast< float >(faceSize);

		for ( size_t faceIndex = 0; faceIndex < CubemapFaceCount; faceIndex++ )
		{
			auto & face = m_facesHDR.at(faceIndex);
			face.resize(static_cast< size_t >(faceSize) * faceSize * 4);

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
						case 0: /* PositiveX */ dx =  1.0F; dy = -t;	dz = -s;	break;
						case 1: /* NegativeX */ dx = -1.0F; dy = -t;	dz =  s;	break;
						case 2: /* PositiveY */ dx =  s;	dy =  1.0F; dz =  t;	break;
						case 3: /* NegativeY */ dx =  s;	dy = -1.0F; dz = -t;	break;
						case 4: /* PositiveZ */ dx =  s;	dy = -t;	dz =  1.0F; break;
						default: /* NegativeZ */ dx = -s;   dy = -t;	dz = -1.0F; break;
					}

					const auto length = std::sqrt(dx * dx + dy * dy + dz * dz);
					const auto nx = dx / length;
					const auto ny = dy / length;
					const auto nz = dz / length;

					const auto theta = std::atan2(nz, nx);
					const auto phi = std::asin(ny);
					const auto u = theta / twoPi + 0.5F;
					const auto v = 0.5F - phi / pi;

					std::array< float, 3 > radiance{};
					sampleEquirectangularHDR(equirectangular, u, v, radiance);

					auto * texel = face.data() + (static_cast< size_t >(row) * faceSize + col) * 4;
					texel[0] = floatToHalf(radiance[0] * calibration);
					texel[1] = floatToHalf(radiance[1] * calibration);
					texel[2] = floatToHalf(radiance[2] * calibration);
					texel[3] = floatToHalf(1.0F);
				}
			}
		}

		m_cubeSize = faceSize;
		m_isHDR = true;

#ifdef EMERAUDE_DEBUG_HDR_FACES
		/* TEMPORARY: tonemapped dump of the six faces for visual inspection. */
		for ( size_t faceIndex = 0; faceIndex < CubemapFaceCount; faceIndex++ )
		{
			Pixmap< uint8_t > debugFace;

			if ( debugFace.initialize(faceSize, faceSize, ChannelMode::RGB) )
			{
				auto * out = debugFace.data().data();
				const auto & face = m_facesHDR.at(faceIndex);

				const auto halfToFloat = [] (uint16_t half) {
					const auto exponent = static_cast< int32_t >((half >> 10) & 0x1FU);
					const auto mantissa = half & 0x3FFU;
					if ( exponent == 0 ) { return 0.0F; }
					return std::ldexp(1.0F + static_cast< float >(mantissa) / 1024.0F, exponent - 15);
				};

				for ( size_t index = 0; index < static_cast< size_t >(faceSize) * faceSize; ++index )
				{
					for ( size_t channel = 0; channel < 3; ++channel )
					{
						const auto value = halfToFloat(face[index * 4 + channel]);
						out[index * 3 + channel] = static_cast< uint8_t >(std::pow(value / (1.0F + value), 1.0F / 2.2F) * 255.0F);
					}
				}

				std::ignore = FileIO::write(debugFace, std::filesystem::path{"/tmp/hdrface-" + this->name() + "-" + std::to_string(faceIndex) + ".png"}, true);
			}
		}
#endif

		TraceSuccess{ClassId} <<
			"HDR cubemap '" << this->name() << "' calibrated (factor: " << calibration <<
			", upper hemisphere -> pi lux at scale 1) and converted to RGBA16F (" << faceSize << "px².";

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

		/* NOTE: Computed once at load time from the raw radiances (Color clamps to [0,1]). */
		if ( m_isHDR )
		{
			return m_averageColorHDR;
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
	float
	CubemapResource::hemisphereIlluminanceFactor () const noexcept
	{
		constexpr auto pi = std::numbers::pi_v< float >;

		if ( m_hemisphereIlluminanceFactor > 0.0F )
		{
			return m_hemisphereIlluminanceFactor;
		}

		/* HDR: the D6 calibration normalizes the upper hemisphere to pi by construction. */
		if ( m_isHDR )
		{
			m_hemisphereIlluminanceFactor = pi;

			return m_hemisphereIlluminanceFactor;
		}

		if ( !this->isLoaded() || !m_faces[0].isValid() )
		{
			/* Uniform-dome fallback, the pre-measurement behavior. */
			return pi;
		}

		/* sRGB -> linear LUT: the LDR texels are sRGB-encoded, the integral needs radiances. */
		static const auto s_linearLUT = [] {
			std::array< float, 256 > table{};

			for ( size_t index = 0; index < table.size(); ++index )
			{
				const auto value = static_cast< float >(index) / 255.0F;

				table[index] = value <= 0.04045F ? value / 12.92F : std::pow((value + 0.055F) / 1.055F, 2.4F);
			}

			return table;
		}();

		/* Integrate luma x cos(zenith) x dOmega over the SKY half of the six faces. In
		 * face-space the sky lives toward +Y (empirically validated on content-rich sources).
		 * Texel solid angle: (4 / N²) / (1 + s² + t²)^(3/2). A uniform white dome integrates
		 * to pi, matching the analytic formula this measurement replaces. */
		double illuminance = 0.0;

		for ( size_t faceIndex = 0; faceIndex < CubemapFaceCount; faceIndex++ )
		{
			const auto & face = m_faces.at(faceIndex);
			const auto faceSize = face.width();
			const auto colorCount = static_cast< size_t >(face.colorCount());
			const auto * data = face.data().data();

			/* NOTE: The nadir face never lights the ground. */
			if ( faceIndex == 3 )
			{
				continue;
			}

			/* Subsample huge faces: the integral converges long before texel resolution. */
			const uint32_t step = std::max(1U, faceSize / 512U);
			const auto stepArea = static_cast< double >(step) * step;
			const auto invSize = 1.0F / static_cast< float >(faceSize);

			for ( uint32_t row = 0; row < faceSize; row += step )
			{
				const auto t = 2.0F * (static_cast< float >(row) + 0.5F) * invSize - 1.0F;

				for ( uint32_t col = 0; col < faceSize; col += step )
				{
					const auto s = 2.0F * (static_cast< float >(col) + 0.5F) * invSize - 1.0F;

					float dx, dy, dz;

					switch ( faceIndex )
					{
						case 0: /* PositiveX */ dx =  1.0F; dy = -t;	dz = -s;	break;
						case 1: /* NegativeX */ dx = -1.0F; dy = -t;	dz =  s;	break;
						case 2: /* PositiveY */ dx =  s;	dy =  1.0F; dz =  t;	break;
						case 4: /* PositiveZ */ dx =  s;	dy = -t;	dz =  1.0F; break;
						default: /* NegativeZ */ dx = -s;   dy = -t;	dz = -1.0F; break;
					}

					const auto lengthSquared = dx * dx + dy * dy + dz * dz;
					const auto upCosine = dy / std::sqrt(lengthSquared);

					if ( upCosine <= 0.0F )
					{
						continue;
					}

					const auto * texel = data + (static_cast< size_t >(row) * faceSize + col) * colorCount;

					const auto luma =
						0.2126F * s_linearLUT[texel[0]] +
						0.7152F * s_linearLUT[texel[1]] +
						0.0722F * s_linearLUT[texel[2]];

					const auto solidAngle = (4.0F * invSize * invSize) / (lengthSquared * std::sqrt(lengthSquared));

					illuminance += static_cast< double >(luma * upCosine * solidAngle) * stepArea;
				}
			}
		}

		m_hemisphereIlluminanceFactor = std::max(1e-4F, static_cast< float >(illuminance));

		TraceInfo{ClassId} <<
			"Cubemap '" << this->name() << "' measured hemisphere illuminance factor: " <<
			m_hemisphereIlluminanceFactor << " (uniform dome = " << pi << ").";

		return m_hemisphereIlluminanceFactor;
	}

}
