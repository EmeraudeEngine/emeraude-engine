/*
 * src/Graphics/ImageResource.hpp
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

/* Local inclusions for inheritances. */
#include "Resources/ResourceTrait.hpp"

/* Local inclusions for usages. */
#include "Libs/PixelFactory/Pixmap.hpp"
#include "Resources/Container.hpp"

namespace EmEn::Graphics
{
	/**
	 * @class ImageResource
	 * @brief Provides 2D image data as a loadable resource for texture rendering.
	 *
	 * ImageResource wraps the PixelFactory::Pixmap<uint8_t> class to provide 2D image
	 * data within the Emeraude Engine resource management system. It serves as the primary
	 * data source for Texture1D, Texture2D, and TextureCubemap objects.
	 *
	 * The resource supports loading from standard image file formats (PNG, JPEG, TGA)
	 * through PixelFactory::FileIO and follows the fail-safe pattern by always providing
	 * a valid resource, never returning nullptr.
	 *
	 * When loaded without a file path (default resource), it generates:
	 * - Debug builds: A 64x64 magenta image with a black X pattern for visual debugging
	 * - Release builds: A 64x64 Perlin noise texture
	 *
	 * The resource provides convenient accessors for image properties including dimensions,
	 * grayscale detection, and average color computation, which are useful for runtime
	 * optimization and analysis.
	 *
	 * @note All image data is stored in RGBA format with 8-bit unsigned integer components.
	 * @see Libs::PixelFactory::Pixmap
	 * @see Resources::Images Type alias for the resource container
	 * @see Graphics::Texture1D
	 * @see Graphics::Texture2D
	 * @see Graphics::TextureCubemap
	 * @version 0.8.35
	 */
	class ImageResource final : public Resources::ResourceTrait
	{
		friend class Resources::Container< ImageResource >;

		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"ImageResource"};

			/** @brief Defines the resource dependency complexity. */
			static constexpr auto Complexity{Resources::DepComplexity::None};

			/**
			 * @brief Constructs an image resource with the specified name.
			 *
			 * Creates a new ImageResource instance. The actual image data is not loaded
			 * until one of the load() methods is called.
			 *
			 * @param name The unique identifier for this resource (moved into the object).
			 * @param resourceFlags Optional resource flag bits for future use (currently unused).
			 * @version 0.8.35
			 */
			explicit
			ImageResource (std::string name, uint32_t resourceFlags = 0) noexcept
				: ResourceTrait{std::move(name), resourceFlags}
			{

			}

			/**
			 * @brief Returns the unique class identifier for ImageResource.
			 *
			 * Provides a compile-time computed FNV-1a hash of the ClassId string,
			 * used for runtime type identification throughout the resource system.
			 *
			 * @return Unique identifier as a size_t hash value.
			 * @note Thread-safe: Uses static initialization.
			 * @version 0.8.35
			 */
			static
			size_t
			getClassUID () noexcept
			{
				static const size_t classUID = EmEn::Libs::Hash::FNV1a(ClassId);

				return classUID;
			}

			/**
			 * @copydoc EmEn::Libs::ObservableTrait::classUID() const
			 * @version 0.8.35
			 */
			[[nodiscard]]
			size_t
			classUID () const noexcept override
			{
				return getClassUID();
			}

			/**
			 * @copydoc EmEn::Libs::ObservableTrait::is() const
			 * @version 0.8.35
			 */
			[[nodiscard]]
			bool
			is (size_t classUID) const noexcept override
			{
				return classUID == getClassUID();
			}

			/**
			 * @copydoc EmEn::Resources::ResourceTrait::classLabel() const
			 * @version 0.8.35
			 */
			[[nodiscard]]
			const char *
			classLabel () const noexcept override
			{
				return ClassId;
			}

			/**
			 * @brief Loads the default 2D image resource (fail-safe fallback).
			 *
			 * Generates a procedural image when no specific file path is provided:
			 * - Debug builds: Creates a 64x64 magenta image with a black X pattern
			 *   for easy visual identification of missing textures during development.
			 * - Release builds: Generates a 64x64 Perlin noise texture for a more
			 *   natural fallback appearance in production.
			 *
			 * This method ensures the resource always has valid data, following the
			 * fail-safe design pattern where resources never fail to load.
			 *
			 * @param serviceProvider The resource service provider (unused in this implementation).
			 * @return true if the default image was successfully generated, false otherwise.
			 * @post If successful, m_pixmap contains a valid 64x64 RGBA image.
			 * @version 0.8.35
			 */
			bool load (Resources::AbstractServiceProvider & serviceProvider) noexcept override;

			/**
			 * @brief Loads a 2D image from a file on disk.
			 *
			 * Reads image data from the specified file path using PixelFactory::FileIO.
			 * Supports standard image formats including PNG, JPEG, and TGA.
			 *
			 * The loaded image is validated to ensure it meets texture requirements
			 * (valid dimensions, color format, etc.). If validation fails, the pixmap
			 * is cleared and the resource is marked as failed.
			 *
			 * @param serviceProvider The resource service provider (unused in this implementation).
			 * @param filepath The filesystem path to the image file to load.
			 * @return true if the image was successfully loaded and validated, false otherwise.
			 * @post If successful, m_pixmap contains the loaded image data in RGBA format.
			 * @note Supported formats depend on PixelFactory::FileIO capabilities (PNG, JPEG, TGA).
			 * @version 0.8.35
			 */
			bool load (Resources::AbstractServiceProvider & serviceProvider, const std::filesystem::path & filepath) noexcept override;

			/**
			 * @brief JSON-based loading is not supported for image resources.
			 *
			 * This method always fails and logs an error, as image data cannot be
			 * meaningfully embedded in JSON format. Use the file-based load() method instead.
			 *
			 * @param serviceProvider The resource service provider (unused).
			 * @param data The JSON data (unused).
			 * @return Always returns false.
			 * @note This method exists for interface compliance but is not functional.
			 * @version 0.8.35
			 */
			bool load (Resources::AbstractServiceProvider & serviceProvider, const Json::Value & data) noexcept override;

			/**
			 * @brief Returns the total memory occupied by this resource in bytes.
			 *
			 * Calculates the memory footprint including both the object size and the
			 * underlying pixmap data buffer.
			 *
			 * @return Total memory usage in bytes (object size + pixel data).
			 * @version 0.8.35
			 */
			[[nodiscard]]
			size_t
			memoryOccupied () const noexcept override
			{
				return sizeof(*this) + m_pixmap.bytes< size_t >();
			}

			/**
			 * @brief Returns the underlying pixmap containing the image data.
			 *
			 * Provides const access to the internal PixelFactory::Pixmap object which
			 * stores the raw pixel data and provides low-level image operations.
			 *
			 * @return Const reference to the pixmap containing RGBA pixel data.
			 * @note The pixmap format is always RGBA with 8-bit unsigned integer components.
			 * @version 0.8.35
			 */
			[[nodiscard]]
			const Libs::PixelFactory::Pixmap< uint8_t > &
			data () const noexcept
			{
				return m_pixmap;
			}

			/**
			 * @brief Returns the width of the image in pixels.
			 *
			 * @return Image width in pixels (horizontal dimension).
			 * @version 0.8.35
			 */
			[[nodiscard]]
			uint32_t
			width () const noexcept
			{
				return m_pixmap.width();
			}

			/**
			 * @brief Returns the height of the image in pixels.
			 *
			 * @return Image height in pixels (vertical dimension).
			 * @version 0.8.35
			 */
			[[nodiscard]]
			uint32_t
			height () const noexcept
			{
				return m_pixmap.height();
			}

			/**
			 * @brief Checks whether the image is grayscale.
			 *
			 * Determines if all pixels have equal R, G, and B channel values,
			 * indicating a grayscale image. This can be used to optimize texture
			 * storage or processing by using single-channel formats.
			 *
			 * @return true if the image is grayscale (R=G=B for all pixels), false otherwise.
			 * @note The alpha channel is not considered in this determination.
			 * @version 0.8.35
			 */
			[[nodiscard]]
			bool
			isGrayScale () const noexcept
			{
				return m_pixmap.isGrayScale();
			}

			/**
			 * @brief Computes the average color across all pixels in the image.
			 *
			 * Calculates the mean RGBA values by sampling all pixels in the image.
			 * This is useful for dominant color extraction, thumbnail generation,
			 * or lighting approximations.
			 *
			 * @return The average color as a normalized float RGBA color (components in [0.0, 1.0]).
			 * @note Computation complexity is O(width * height).
			 * @version 0.8.35
			 */
			[[nodiscard]]
			Libs::PixelFactory::Color< float >
			averageColor () const noexcept
			{
				return m_pixmap.averageColor();
			}

		private:

			Libs::PixelFactory::Pixmap< uint8_t > m_pixmap;
	};
}

/* Expose the resource manager as a convenient type. */
namespace EmEn::Resources
{
	using Images = Container< Graphics::ImageResource >;
}
