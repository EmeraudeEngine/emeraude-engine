/*
 * src/Saphir/Declaration/Types.hpp
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

/* STL inclusions. */
#include <cstddef>
#include <cstdint>

/* Local inclusions for usages. */
#include "Saphir/Keys.hpp"

namespace EmEn::Saphir::Declaration
{
	/** @brief GLSL input primitive type enumerations. */
	enum class InputPrimitiveType
	{
		Points,
		Lines,
		LineAdjacency,
		Triangles,
		TrianglesAdjacency
	};

	/** @brief GLSL output primitive type enumerations. */
	enum class OutputPrimitiveType
	{
		Points,
		LineStrip,
		TriangleStrip
	};

	/**
	 * @brief The GLSL variable type enumerations.
	 * @note The enumeration is used to compute size in bytes for declarations.
	 */
	enum class VariableType
	{
		Void,
		Float,
		Double,
		UnsignedInteger,
		Integer,
		AtomicUnsignedInteger,
		Boolean,
		FloatVector2,
		FloatVector3,
		FloatVector4,
		DoubleVector2,
		DoubleVector3,
		DoubleVector4,
		UIntVector2,
		UIntVector3,
		UIntVector4,
		SIntVector2,
		SIntVector3,
		SIntVector4,
		BooleanVector2,
		BooleanVector3,
		BooleanVector4,
		Matrix2,
		Matrix3,
		Matrix4,
		Matrix2x2,
		Matrix2x3,
		Matrix2x4,
		Matrix3x3,
		Matrix3x2,
		Matrix3x4,
		Matrix4x2,
		Matrix4x3,
		Matrix4x4,
		DoubleMatrix2,
		DoubleMatrix3,
		DoubleMatrix4,
		DoubleMatrix2x2,
		DoubleMatrix2x3,
		DoubleMatrix2x4,
		DoubleMatrix3x3,
		DoubleMatrix3x2,
		DoubleMatrix3x4,
		DoubleMatrix4x2,
		DoubleMatrix4x3,
		DoubleMatrix4x4,
		Sampler1D,
		Sampler1DArray,
		Sampler2D,
		Sampler2DArray,
		Sampler2DMS,
		Sampler2DMSArray,
		Sampler3D,
		SamplerCube,
		SamplerCubeArray,
		SamplerBuffer,
		UIntSamplerBuffer,
		UIntSamplerCubeArray,
		UIntSamplerCube,
		UIntSampler3D,
		UIntSampler2DMSArray,
		UIntSampler2DMS,
		UIntSampler2DRectangle,
		UIntSampler2D,
		UIntSampler2DArray,
		UIntSampler1D,
		UIntSampler1DArray,
		SIntSamplerBuffer,
		SIntSamplerCubeArray,
		SIntSamplerCube,
		SIntSampler3D,
		SIntSampler2DMSArray,
		SIntSampler2DMS,
		SIntSampler2DRectangle,
		SIntSampler2D,
		SIntSampler2DArray,
		SIntSampler1D,
		SIntSampler1DArray,
		Sampler1DShadow,
		Sampler1DArrayShadow,
		Sampler2DShadow,
		Sampler2DArrayShadow,
		Sampler2DRectangle,
		Sampler2DRectangleShadow,
		SamplerCubeShadow,
		SamplerCubeArrayShadow,
		Image1D,
		Image1DArray,
		Image2D,
		Image2DArray,
		Image2DRectangle,
		Image2DMS,
		Image2DMSArray,
		Image3D,
		ImageCube,
		ImageCubeArray,
		ImageBuffer,
		UIntImage1D,
		UIntImage1DArray,
		UIntImage2D,
		UIntImage2DArray,
		UIntImage2DRectangle,
		UIntImage2DMS,
		UIntImage2DMSArray,
		UIntImage3D,
		UIntImageCube,
		UIntImageCubeArray,
		UIntImageBuffer,
		SIntImage1D,
		SIntImage1DArray,
		SIntImage2D,
		SIntImage2DArray,
		SIntImage2DRectangle,
		SIntImage2DMS,
		SIntImage2DMSArray,
		SIntImage3D,
		SIntImageCube,
		SIntImageCubeArray,
		SIntImageBuffer,
		Texture1D,
		Texture2D,
		Texture3D,
		TextureCube,
		Texture2DRectangle,
		Texture1DArray,
		Texture2DArray,
		TextureCubeArray,
		TextureBuffer,
		Texture2DMS,
		Texture2DMSArray,
		UIntTexture1D,
		UIntTexture2D,
		UIntTexture3D,
		UIntTextureCube,
		UIntTexture2DRectangle,
		UIntTexture1DArray,
		UIntTexture2DArray,
		UIntTextureCubeArray,
		UIntTextureBuffer,
		UIntTexture2DMS,
		UIntTexture2DMSArray,
		SIntTexture1D,
		SIntTexture2D,
		SIntTexture3D,
		SIntTextureCube,
		SIntTexture2DRectangle,
		SIntTexture1DArray,
		SIntTexture2DArray,
		SIntTextureCubeArray,
		SIntTextureBuffer,
		SIntTexture2DMS,
		SIntTexture2DMSArray,
		Sampler,
		SamplerShadow,
		SubpassInput,
		SubpassInputMS,
		UIntSubpassInput,
		UIntSubpassInputMS,
		SIntSubpassInput,
		SIntSubpassInputMS,
		Structure
	};

	/**
	 * @brief Converts a variable type to GLSL type string.
	 * @param type The variable type.
	 * @return Key
	 */
	[[nodiscard]]
	Key to_cstring (VariableType type) noexcept;

	/**
	 * @brief Returns the size in bytes from a GLSL type.
	 * @param type The variable type.
	 * @return uint32_t
	 */
	[[nodiscard]]
	uint32_t size_bytes (VariableType type) noexcept;

	/**
	 * @brief The GLSL memory layout enumeration.
	 * @note
	 * - packed: This layout type means that the implementation determines everything about how the fields are laid out in the block. The OpenGL API must be used to query the layout for the members of a particular block. Each member of a block will have a particular byte offset, which you can use to determine how to upload its data. Also, members of a block can be optimized out if they are found by the implementation to not affect the result of the shader. Therefore, the active components of a block may not be all of the components it was defined with.
	 * - shared: This layout type works like packed, with two exceptions. First, it guarantees that all of the variables defined in the block are considered active; this means nothing is optimized out. Second, it guarantees that the members of the block will have the same layout as a block definition in another program, so long as:
	 * 		The blocks in different programs use the exact same block definition (ignoring differences in variable names and the block name itself). Be careful about specifying the default matrix storage order, as this can implicitly change the definition of blocks following them.
	 * 		All members of the block use explicit sizes for arrays.
	 * 		Because of these guarantees, buffer-backed blocks declared shared can be used with any program that defines a block with the same elements in the same order. This even works across different types of buffer-backed blocks. You can use a buffer as a uniform buffer at one point, then use it as a shader storage buffer in another. OpenGL guarantees that all of the offsets and alignments will match between two shared blocks that define the same members in the same order. In short, it allows the user to share buffers between multiple programs.
	 * - std140: This layout alleviates the need to query the offsets for definitions. The rules of std140 layout explicitly state the layout arrangement of any interface block declared with this layout. This also means that such an interface block can be shared across programs, much like shared. The only downside to this layout type is that the rules for packing elements into arrays/structs can introduce a lot of unnecessary padding.
	 * 		The rules for std140 layout are covered quite well in the OpenGL specification (OpenGL 4.5, Section 7.6.2.2, page 137). Among the most important is the fact that arrays of types are not necessarily tightly packed. An array of floats in such a block will not be the equivalent to an array of floats in C/C++. The array stride (the bytes between array elements) is always rounded up to the size of a vec4 (ie: 16-bytes). So arrays will only match their C/C++ definitions if the type is a multiple of 16 bytes
	 * 		Warning: Implementations sometimes get the std140 layout wrong for vec3 components. You are advised to manually pad your structures/arrays out and avoid using vec3 at all.
	 *  - std430: This layout works like std140, except with a few optimizations in the alignment and strides for arrays and structs of scalars and vector elements (except for vec3 elements, which remain unchanged from std140). Specifically, they are no longer rounded up to a multiple of 16 bytes. So an array of `float`s will match with a C++ array of `float`s.
	 * 		Note that this layout can only be used with shader storage blocks, not uniform blocks.
	 */
	enum class MemoryLayout
	{
		Shared,
		Packed,
		Std140,
		Std430
	};

	/**
	 * @brief The GLSL matrix storage order enumeration.
	 */
	enum class MatrixStorageOrder
	{
		Default, /* This won't add any code. */
		ColumnMajor, /* This is the real GLSL default. */
		RowMajor
	};
}
