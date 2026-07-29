/*
 * src/Graphics/CubemapResource.hpp
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

#pragma once

/* STL inclusions. */
#include <array>
#include <cstdint>
#include <vector>

/* Local inclusions for inheritances. */
#include "Resources/ResourceTrait.hpp"

/* Local inclusions for usages. */
#include "PixelFactory/Pixmap.hpp"
#include "Resources/Container.hpp"
#include "Types.hpp"

namespace EmEn::Graphics
{
	/**
	 * @brief The cubemap resource class.
	 * @extends EmEn::Resources::ResourceTrait This is a loadable resource.
	 */
	class EMEN_API CubemapResource final : public Resources::ResourceTrait
	{
		friend class Resources::Container< CubemapResource >;

		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"CubemapResource"};

			/** @brief Defines the resource dependency complexity. */
			static constexpr auto Complexity{Resources::DepComplexity::None};

			/**
			 * @brief Constructs a cubemap resource.
			 * @param serviceProvider A reference to the service provider.
			 * @param name A string for the resource name [std::move].
			 * @param resourceFlags The resource flag bits. Default none. (Unused yet)
			 */
			CubemapResource (Resources::AbstractServiceProvider & serviceProvider, std::string name, uint32_t resourceFlags = 0) noexcept
				: ResourceTrait{serviceProvider, std::move(name), resourceFlags}
			{

			}

			/**
			 * @brief Returns the unique identifier for this class [Thread-safe].
			 * @return size_t
			 */
			static
			size_t
			getClassUID () noexcept
			{
				return Base::Hash::FNV1a(ClassId);
			}

			/** @copydoc EmEn::Base::ObservableTrait::classUID() const */
			[[nodiscard]]
			size_t
			classUID () const noexcept override
			{
				return getClassUID();
			}

			/** @copydoc EmEn::Base::ObservableTrait::is() const */
			[[nodiscard]]
			bool
			is (size_t classUID) const noexcept override
			{
				return classUID == getClassUID();
			}

			/** @copydoc EmEn::Resources::ResourceTrait::classLabel() const */
			[[nodiscard]]
			const char *
			classLabel () const noexcept override
			{
				return ClassId;
			}

			/** @copydoc EmEn::Resources::ResourceTrait::load() */
			bool load () noexcept override;

			/** @copydoc EmEn::Resources::ResourceTrait::load(const std::filesystem::path &) */
			bool load (const std::filesystem::path & filepath) noexcept override;

			/** @copydoc EmEn::Resources::ResourceTrait::load(const Json::Value &) */
			bool load (const Json::Value & data) noexcept override;

			/** @copydoc EmEn::Resources::ResourceTrait::memoryOccupied() const noexcept */
			[[nodiscard]]
			size_t
			memoryOccupied () const noexcept override
			{
				size_t bytes = sizeof(*this);

				for ( const auto & pixmap : m_faces )
				{
					bytes += pixmap.bytes< size_t >();
				}

				for ( const auto & face : m_facesHDR )
				{
					bytes += face.size() * sizeof(uint16_t);
				}

				return bytes;
			}

			/**
			 * @brief Loads a cubemap from a packed pixmap.
			 * @param pixmap A reference to the pixmap.
			 * @return bool
			 */
			bool load (const Base::PixelFactory::Pixmap< uint8_t > & pixmap) noexcept;

			/**
			 * @brief Loads a cubemap from an equirectangular (panoramic 2:1) pixmap.
			 * @param equirectangular A reference to the equirectangular source pixmap.
			 * @param faceSize The desired size (width and height) of each cube face in pixels.
			 * @return bool
			 */
			bool loadEquirectangular (const Base::PixelFactory::Pixmap< uint8_t > & equirectangular, uint32_t faceSize) noexcept;

			/**
			 * @brief Loads a cubemap from a packed pixmap.
			 * @param pixmaps A reference to a fixed array of pixmaps.
			 * @return bool
			 */
			bool load (const CubemapPixmaps & pixmaps) noexcept;

			/**
			 * @brief Loads a cubemap filled with a solid color.
			 * @param color The color to fill all faces with.
			 * @param size The size of each face (width and height in pixels).
			 * @return bool
			 */
			bool load (const Base::PixelFactory::Color< float > & color, uint32_t size) noexcept;

			/**
			 * @brief Returns the pixmap.
			 * @param faceIndex The face number.
			 * @return const Libraries::PixelFactory::Pixmap< uint8_t > &
			 */
			[[nodiscard]]
			const Base::PixelFactory::Pixmap< uint8_t > & data (size_t faceIndex) const noexcept;

			/**
			 * @brief Returns faces of the cubemap.
			 * @warning Empty pixmaps for an HDR cubemap (see isHDR() / hdrFaceData()).
			 * @return const CubemapPixmaps &
			 */
			[[nodiscard]]
			const CubemapPixmaps &
			faces () const noexcept
			{
				return m_faces;
			}

			/**
			 * @brief Returns whether the cubemap holds HDR (RGBA16F) data.
			 * @note HDR sources (Radiance .hdr) keep their dynamic range: the faces are stored
			 * as half-float texels (see hdrFaceData()), the LDR faces() stay empty.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isHDR () const noexcept
			{
				return m_isHDR;
			}

			/**
			 * @brief Returns the RGBA16F texel data of a face (HDR cubemaps only).
			 * @param faceIndex The face index [0-5].
			 * @return const std::vector< uint16_t > &
			 */
			[[nodiscard]]
			const std::vector< uint16_t > &
			hdrFaceData (size_t faceIndex) const noexcept
			{
				return m_facesHDR.at(faceIndex);
			}

			/**
			 * @brief Returns the size of the cubemap.
			 * @note Returns the width of the first cubemap face.
			 * @return uint32_t
			 */
			[[nodiscard]]
			uint32_t
			cubeSize () const noexcept
			{
				return m_isHDR ? m_cubeSize : m_faces[0].width();
			}

			/**
			 * @brief Returns whether pixmaps are all gray scale.
			 * @return bool
			 */
			[[nodiscard]]
			bool isGrayScale () const noexcept;

			/**
			 * @brief Returns the average color of the cubemap.
			 * @return Libraries::PixelFactory::Color< float >
			 */
			[[nodiscard]]
			Base::PixelFactory::Color< float > averageColor () const noexcept;

			/**
			 * @brief Returns the factor F such as the illuminance a sky made of this cubemap
			 * pours on the ground is `E = luminance x F`, in lux.
			 * @note MEASURED on the actual texels (sRGB-decoded luma x cos(zenith), integrated
			 * over the upper hemisphere): pi for a uniform dome, much less for a sky whose dome
			 * is partly dark (owner decision, review session Jul 2026 — the uniform-dome pi
			 * over-lit every non-uniform sky). HDR cubemaps are calibrated so their factor IS
			 * pi by construction. Lazily computed, then cached.
			 * @return float
			 */
			[[nodiscard]]
			float hemisphereIlluminanceFactor () const noexcept;

		private:

			/* JSON keys */
			static constexpr auto PackedKey{"Packed"};
			static constexpr auto EquirectangularKey{"Equirectangular"};
			static constexpr auto FileFormatKey{"FileFormat"};

			/**
			 * @brief Loads an HDR equirectangular pixmap as a calibrated RGBA16F cubemap.
			 * @note The Unity-like photometric calibration (owner decision D6): an HDRI holds
			 * RELATIVE radiances, so the loader measures the illuminance the upper hemisphere
			 * pours on the ground and normalizes the data so a scale of 1 behaves like a
			 * UNIFORM DOME of luminance 1 (E = pi lux). The Background manifest "Luminance"
			 * (nits) then means exactly the same thing for LDR and HDR sources, and the sun
			 * keeps its full relative punch (clamped to the half-float maximum, 65504x the sky).
			 * @param equirectangular A reference to the source pixmap (linear radiances).
			 * @param faceSize The face size in pixels.
			 * @return bool
			 */
			bool loadEquirectangularHDR (const Base::PixelFactory::Pixmap< float > & equirectangular, uint32_t faceSize) noexcept;

			CubemapPixmaps m_faces;
			std::array< std::vector< uint16_t >, CubemapFaceCount > m_facesHDR{};
			Base::PixelFactory::Color< float > m_averageColorHDR;
			mutable float m_hemisphereIlluminanceFactor{-1.0F};
			uint32_t m_cubeSize{0};
			bool m_isHDR{false};
	};
}

/* Expose the resource manager as a convenient type. */
namespace EmEn::Resources
{
	using Cubemaps = Container< Graphics::CubemapResource >;
}
