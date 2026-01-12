/*
 * src/Graphics/VolumetricImageResource.hpp
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

#pragma once

/* STL inclusions. */
#include <cstdint>
#include <vector>

/* Local inclusions for inheritances. */
#include "Resources/ResourceTrait.hpp"

/* Local inclusions for usages. */
#include "Libs/PixelFactory/Color.hpp"
#include "Resources/Container.hpp"

namespace EmEn::Graphics
{
	/**
	 * @class VolumetricImageResource
	 * @brief Provides 3D volumetric image data as a loadable resource for 3D texture rendering.
	 *
	 * VolumetricImageResource stores 3D volumetric data as a contiguous byte array with
	 * explicit width, height, and depth dimensions. It serves as the exclusive data source
	 * for Texture3D objects within the Emeraude Engine resource management system.
	 *
	 * Unlike ImageResource which wraps PixelFactory::Pixmap, this class uses a raw
	 * std::vector<uint8_t> buffer to store volumetric data, providing direct memory
	 * access for efficient GPU uploads. The data is stored in row-major order with
	 * dimensions organized as [depth][height][width][channels].
	 *
	 * The resource follows the fail-safe pattern by always providing a valid resource.
	 * When loaded without a file path (default resource), it generates a 32x32x32 RGB
	 * gradient cube where voxel colors vary smoothly across X (red), Y (green), and
	 * Z (blue) axes.
	 *
	 * The class provides accessors for volumetric properties including dimensions,
	 * channel count, data validity, grayscale detection, and average color computation,
	 * which are essential for runtime texture analysis and optimization.
	 *
	 * @note All volumetric data defaults to RGBA format (4 channels) with 8-bit unsigned integer components.
	 * @note File loading support is planned but not yet implemented.
	 * @see Resources::VolumetricImages Type alias for the resource container
	 * @see Graphics::Texture3D
	 * @todo File loading for volumetric data formats (.raw, .vox, DICOM) is not yet implemented.
	 * @version 0.8.35
	 */
	class VolumetricImageResource final : public Resources::ResourceTrait
	{
		friend class Resources::Container< VolumetricImageResource >;

		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"VolumetricImageResource"};

			/** @brief Defines the resource dependency complexity. */
			static constexpr auto Complexity{Resources::DepComplexity::None};

			/**
			 * @brief Constructs a volumetric image resource with the specified name.
			 *
			 * Creates a new VolumetricImageResource instance. The actual volumetric data
			 * is not loaded until one of the load() methods is called.
			 *
			 * @param name The unique identifier for this resource (moved into the object).
			 * @param resourceFlags Optional resource flag bits for future use (currently unused).
			 * @version 0.8.35
			 */
			explicit
			VolumetricImageResource (std::string name, uint32_t resourceFlags = 0) noexcept
				: ResourceTrait{std::move(name), resourceFlags}
			{

			}

			/**
			 * @brief Returns the unique class identifier for VolumetricImageResource.
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
				return Libs::Hash::FNV1a(ClassId);
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
			 * @brief Loads the default 3D volumetric image resource (fail-safe fallback).
			 *
			 * Generates a procedural 32x32x32 RGBA volumetric gradient when no specific
			 * file path is provided. The gradient produces smooth color transitions across
			 * all three spatial axes:
			 * - X axis (width): Red channel from 0 to 255
			 * - Y axis (height): Green channel from 0 to 255
			 * - Z axis (depth): Blue channel from 0 to 255
			 * - Alpha channel: Constant 255 (fully opaque)
			 *
			 * This provides a visually distinct and useful default for 3D texture debugging
			 * and testing, following the fail-safe design pattern where resources never
			 * fail to load.
			 *
			 * @param serviceProvider The resource service provider (unused in this implementation).
			 * @return true if the default volumetric data was successfully generated, false otherwise.
			 * @post If successful, m_data contains 32x32x32x4 bytes of RGBA gradient data.
			 * @version 0.8.35
			 */
			bool load (Resources::AbstractServiceProvider & serviceProvider) noexcept override;

			/**
			 * @brief Loads 3D volumetric data from a file on disk.
			 *
			 * Currently not implemented. This method will eventually support loading
			 * volumetric data from various file formats including raw binary volumes,
			 * voxel formats (.vox), and potentially medical imaging formats (DICOM).
			 *
			 * @param serviceProvider The resource service provider (unused).
			 * @param filepath The filesystem path to the volumetric data file.
			 * @return Always returns false (not yet implemented).
			 * @warning This method logs a warning and always fails.
			 * @todo Implement file loading for volumetric data formats (.raw, .vox, DICOM).
			 * @version 0.8.35
			 */
			bool load (Resources::AbstractServiceProvider & serviceProvider, const std::filesystem::path & filepath) noexcept override;

			/**
			 * @brief JSON-based loading is not supported for volumetric image resources.
			 *
			 * This method always fails and logs an error, as volumetric data cannot be
			 * meaningfully embedded in JSON format due to its size. Use the file-based
			 * load() method instead when file loading is implemented.
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
			 * underlying volumetric data buffer.
			 *
			 * @return Total memory usage in bytes (object size + voxel data).
			 * @version 0.8.35
			 */
			[[nodiscard]]
			size_t
			memoryOccupied () const noexcept override
			{
				return sizeof(*this) + m_data.size();
			}

			/**
			 * @brief Returns the raw volumetric data buffer.
			 *
			 * Provides const access to the internal byte vector containing voxel data.
			 * The data is organized in row-major order as [depth][height][width][channels],
			 * where each voxel's color channels are stored contiguously.
			 *
			 * @return Const reference to the vector containing all voxel data.
			 * @note Total buffer size = width * height * depth * colorCount bytes.
			 * @version 0.8.35
			 */
			[[nodiscard]]
			const std::vector< uint8_t > &
			data () const noexcept
			{
				return m_data;
			}

			/**
			 * @brief Returns the width (X dimension) of the volumetric data.
			 *
			 * @return Width in voxels (horizontal dimension).
			 * @version 0.8.35
			 */
			[[nodiscard]]
			uint32_t
			width () const noexcept
			{
				return m_width;
			}

			/**
			 * @brief Returns the height (Y dimension) of the volumetric data.
			 *
			 * @return Height in voxels (vertical dimension).
			 * @version 0.8.35
			 */
			[[nodiscard]]
			uint32_t
			height () const noexcept
			{
				return m_height;
			}

			/**
			 * @brief Returns the depth (Z dimension) of the volumetric data.
			 *
			 * @return Depth in voxels (Z-axis dimension).
			 * @version 0.8.35
			 */
			[[nodiscard]]
			uint32_t
			depth () const noexcept
			{
				return m_depth;
			}

			/**
			 * @brief Returns the number of color channels per voxel.
			 *
			 * Indicates the number of color components for each voxel. Common values:
			 * - 1: Grayscale (single intensity value)
			 * - 3: RGB (red, green, blue)
			 * - 4: RGBA (red, green, blue, alpha) - default
			 *
			 * @return Number of color channels (typically 1, 3, or 4).
			 * @version 0.8.35
			 */
			[[nodiscard]]
			uint32_t
			colorCount () const noexcept
			{
				return m_colorCount;
			}

			/**
			 * @brief Returns the total number of bytes in the volumetric data buffer.
			 *
			 * Computes the total size of the raw data buffer. This equals
			 * width * height * depth * colorCount.
			 *
			 * @return Total buffer size in bytes.
			 * @version 0.8.35
			 */
			[[nodiscard]]
			size_t
			bytes () const noexcept
			{
				return m_data.size();
			}

			/**
			 * @brief Validates whether the volumetric data is in a usable state.
			 *
			 * Checks that all required conditions are met for valid volumetric data:
			 * - Data buffer is not empty
			 * - Width, height, and depth are all greater than zero
			 * - Color channel count is greater than zero
			 *
			 * @return true if the volumetric data is valid and usable, false otherwise.
			 * @note This does not validate buffer size consistency (expected: width * height * depth * colorCount).
			 * @version 0.8.35
			 */
			[[nodiscard]]
			bool
			isValid () const noexcept
			{
				return !m_data.empty() && m_width > 0 && m_height > 0 && m_depth > 0 && m_colorCount > 0;
			}

			/**
			 * @brief Checks whether the volumetric data is grayscale.
			 *
			 * Determines if all voxels have equal R, G, and B channel values (if applicable),
			 * indicating grayscale data. This can be used to optimize texture storage or
			 * processing by using single-channel formats.
			 *
			 * For volumes with fewer than 3 channels, returns true only if colorCount is 1.
			 *
			 * @return true if the volume is grayscale (R=G=B for all voxels), false otherwise.
			 * @note Computation complexity is O(width * height * depth).
			 * @note The alpha channel (if present) is not considered in this determination.
			 * @version 0.8.35
			 */
			[[nodiscard]]
			bool isGrayScale () const noexcept;

			/**
			 * @brief Computes the average color across all voxels in the volume.
			 *
			 * Calculates the mean RGBA values by sampling all voxels in the volumetric data.
			 * This is useful for dominant color extraction, volume preview generation, or
			 * lighting approximations in volumetric rendering.
			 *
			 * @return The average color as a normalized float RGBA color (components in [0.0, 1.0]).
			 * @return Returns black (0, 0, 0, 0) if the volume is empty or invalid.
			 * @note Computation complexity is O(width * height * depth).
			 * @version 0.8.35
			 */
			[[nodiscard]]
			Libs::PixelFactory::Color< float > averageColor () const noexcept;

		private:

			std::vector< uint8_t > m_data;
			uint32_t m_width{0};
			uint32_t m_height{0};
			uint32_t m_depth{0};
			uint32_t m_colorCount{4}; /* RGBA by default. */
	};
}

/* Expose the resource manager as a convenient type. */
namespace EmEn::Resources
{
	using VolumetricImages = Container< Graphics::VolumetricImageResource >;
}
