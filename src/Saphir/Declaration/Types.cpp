/*
 * src/Saphir/Declaration/Types.cpp
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

#include "Types.hpp"

namespace EmEn::Saphir::Declaration
{
	using namespace Keys;

	Key
	to_cstring (VariableType type) noexcept
	{
		switch ( type )
		{
			case VariableType::Void :
				return GLSL::Void;

			case VariableType::Float :
				return GLSL::Float;

			case VariableType::Double :
				return GLSL::Double;

			case VariableType::UnsignedInteger :
				return GLSL::UnsignedInteger;

			case VariableType::Integer :
				return GLSL::Integer;

			case VariableType::AtomicUnsignedInteger :
				return GLSL::AtomicUnsignedInteger;

			case VariableType::Boolean :
				return GLSL::Boolean;

			case VariableType::FloatVector2 :
				return GLSL::FloatVector2;

			case VariableType::FloatVector3 :
				return GLSL::FloatVector3;

			case VariableType::FloatVector4 :
				return GLSL::FloatVector4;

			case VariableType::DoubleVector2 :
				return GLSL::DoubleVector2;

			case VariableType::DoubleVector3 :
				return GLSL::DoubleVector3;

			case VariableType::DoubleVector4 :
				return GLSL::DoubleVector4;

			case VariableType::UIntVector2 :
				return GLSL::UIntVector2;

			case VariableType::UIntVector3 :
				return GLSL::UIntVector3;

			case VariableType::UIntVector4 :
				return GLSL::UIntVector4;

			case VariableType::SIntVector2 :
				return GLSL::SIntVector2;

			case VariableType::SIntVector3 :
				return GLSL::SIntVector3;

			case VariableType::SIntVector4 :
				return GLSL::SIntVector4;

			case VariableType::BooleanVector2 :
				return GLSL::BooleanVector2;

			case VariableType::BooleanVector3 :
				return GLSL::BooleanVector3;

			case VariableType::BooleanVector4 :
				return GLSL::BooleanVector4;

			case VariableType::Matrix2 :
				return GLSL::Matrix2;

			case VariableType::Matrix3 :
				return GLSL::Matrix3;

			case VariableType::Matrix4 :
				return GLSL::Matrix4;

			case VariableType::Matrix2x2 :
				return GLSL::Matrix2x2;

			case VariableType::Matrix2x3 :
				return GLSL::Matrix2x3;

			case VariableType::Matrix2x4 :
				return GLSL::Matrix2x4;

			case VariableType::Matrix3x3 :
				return GLSL::Matrix3x3;

			case VariableType::Matrix3x2 :
				return GLSL::Matrix3x2;

			case VariableType::Matrix3x4 :
				return GLSL::Matrix3x4;

			case VariableType::Matrix4x2 :
				return GLSL::Matrix4x2;

			case VariableType::Matrix4x3 :
				return GLSL::Matrix4x3;

			case VariableType::Matrix4x4 :
				return GLSL::Matrix4x4;

			case VariableType::DoubleMatrix2 :
				return GLSL::DoubleMatrix2;

			case VariableType::DoubleMatrix3 :
				return GLSL::DoubleMatrix3;

			case VariableType::DoubleMatrix4 :
				return GLSL::DoubleMatrix4;

			case VariableType::DoubleMatrix2x2 :
				return GLSL::DoubleMatrix2x2;

			case VariableType::DoubleMatrix2x3 :
				return GLSL::DoubleMatrix2x3;

			case VariableType::DoubleMatrix2x4 :
				return GLSL::DoubleMatrix2x4;

			case VariableType::DoubleMatrix3x3 :
				return GLSL::DoubleMatrix3x3;

			case VariableType::DoubleMatrix3x2 :
				return GLSL::DoubleMatrix3x2;

			case VariableType::DoubleMatrix3x4 :
				return GLSL::DoubleMatrix3x4;

			case VariableType::DoubleMatrix4x2 :
				return GLSL::DoubleMatrix4x2;

			case VariableType::DoubleMatrix4x3 :
				return GLSL::DoubleMatrix4x3;

			case VariableType::DoubleMatrix4x4 :
				return GLSL::DoubleMatrix4x4;

			case VariableType::Sampler1D :
				return GLSL::Sampler1D;

			case VariableType::Sampler1DArray :
				return GLSL::Sampler1DArray;

			case VariableType::Sampler2D :
				return GLSL::Sampler2D;

			case VariableType::Sampler2DArray :
				return GLSL::Sampler2DArray;

			case VariableType::Sampler2DMS :
				return GLSL::Sampler2DMS;

			case VariableType::Sampler2DMSArray :
				return GLSL::Sampler2DMSArray;

			case VariableType::Sampler3D :
				return GLSL::Sampler3D;

			case VariableType::SamplerCube :
				return GLSL::SamplerCube;

			case VariableType::SamplerCubeArray :
				return GLSL::SamplerCubeArray;

			case VariableType::SamplerBuffer :
				return GLSL::SamplerBuffer;

			case VariableType::UIntSamplerBuffer :
				return GLSL::UIntSamplerBuffer;

			case VariableType::UIntSamplerCubeArray :
				return GLSL::UIntSamplerCubeArray;

			case VariableType::UIntSamplerCube :
				return GLSL::UIntSamplerCube;

			case VariableType::UIntSampler3D :
				return GLSL::UIntSampler3D;

			case VariableType::UIntSampler2DMSArray :
				return GLSL::UIntSampler2DMSArray;

			case VariableType::UIntSampler2DMS :
				return GLSL::UIntSampler2DMS;

			case VariableType::UIntSampler2DRectangle :
				return GLSL::UIntSampler2DRectangle;

			case VariableType::UIntSampler2D :
				return GLSL::UIntSampler2D;

			case VariableType::UIntSampler2DArray :
				return GLSL::UIntSampler2DArray;

			case VariableType::UIntSampler1D :
				return GLSL::UIntSampler1D;

			case VariableType::UIntSampler1DArray :
				return GLSL::UIntSampler1DArray;

			case VariableType::SIntSamplerBuffer :
				return GLSL::SIntSamplerBuffer;

			case VariableType::SIntSamplerCubeArray :
				return GLSL::SIntSamplerCubeArray;

			case VariableType::SIntSamplerCube :
				return GLSL::SIntSamplerCube;

			case VariableType::SIntSampler3D :
				return GLSL::SIntSampler3D;

			case VariableType::SIntSampler2DMSArray :
				return GLSL::SIntSampler2DMSArray;

			case VariableType::SIntSampler2DMS :
				return GLSL::SIntSampler2DMS;

			case VariableType::SIntSampler2DRectangle :
				return GLSL::SIntSampler2DRectangle;

			case VariableType::SIntSampler2D :
				return GLSL::SIntSampler2D;

			case VariableType::SIntSampler2DArray :
				return GLSL::SIntSampler2DArray;

			case VariableType::SIntSampler1D :
				return GLSL::SIntSampler1D;

			case VariableType::SIntSampler1DArray :
				return GLSL::SIntSampler1DArray;

			case VariableType::Sampler1DShadow :
				return GLSL::Sampler1DShadow;

			case VariableType::Sampler1DArrayShadow :
				return GLSL::Sampler1DArrayShadow;

			case VariableType::Sampler2DShadow :
				return GLSL::Sampler2DShadow;

			case VariableType::Sampler2DArrayShadow :
				return GLSL::Sampler2DArrayShadow;

			case VariableType::Sampler2DRectangle :
				return GLSL::Sampler2DRectangle;

			case VariableType::Sampler2DRectangleShadow :
				return GLSL::Sampler2DRectangleShadow;

			case VariableType::SamplerCubeShadow :
				return GLSL::SamplerCubeShadow;

			case VariableType::SamplerCubeArrayShadow :
				return GLSL::SamplerCubeArrayShadow;

			case VariableType::Image1D :
				return GLSL::Image1D;

			case VariableType::Image1DArray :
				return GLSL::Image1DArray;

			case VariableType::Image2D :
				return GLSL::Image2D;

			case VariableType::Image2DArray :
				return GLSL::Image2DArray;

			case VariableType::Image2DRectangle :
				return GLSL::Image2DRectangle;

			case VariableType::Image2DMS :
				return GLSL::Image2DMS;

			case VariableType::Image2DMSArray :
				return GLSL::Image2DMSArray;

			case VariableType::Image3D :
				return GLSL::Image3D;

			case VariableType::ImageCube :
				return GLSL::ImageCube;

			case VariableType::ImageCubeArray :
				return GLSL::ImageCubeArray;

			case VariableType::ImageBuffer :
				return GLSL::ImageBuffer;

			case VariableType::UIntImage1D :
				return GLSL::UIntImage1D;

			case VariableType::UIntImage1DArray :
				return GLSL::UIntImage1DArray;

			case VariableType::UIntImage2D :
				return GLSL::UIntImage2D;

			case VariableType::UIntImage2DArray :
				return GLSL::UIntImage2DArray;

			case VariableType::UIntImage2DRectangle :
				return GLSL::UIntImage2DRectangle;

			case VariableType::UIntImage2DMS :
				return GLSL::UIntImage2DMS;

			case VariableType::UIntImage2DMSArray :
				return GLSL::UIntImage2DMSArray;

			case VariableType::UIntImage3D :
				return GLSL::UIntImage3D;

			case VariableType::UIntImageCube :
				return GLSL::UIntImageCube;

			case VariableType::UIntImageCubeArray :
				return GLSL::UIntImageCubeArray;

			case VariableType::UIntImageBuffer :
				return GLSL::UIntImageBuffer;

			case VariableType::SIntImage1D :
				return GLSL::SIntImage1D;

			case VariableType::SIntImage1DArray :
				return GLSL::SIntImage1DArray;

			case VariableType::SIntImage2D :
				return GLSL::SIntImage2D;

			case VariableType::SIntImage2DArray :
				return GLSL::SIntImage2DArray;

			case VariableType::SIntImage2DRectangle :
				return GLSL::SIntImage2DRectangle;

			case VariableType::SIntImage2DMS :
				return GLSL::SIntImage2DMS;

			case VariableType::SIntImage2DMSArray :
				return GLSL::SIntImage2DMSArray;

			case VariableType::SIntImage3D :
				return GLSL::SIntImage3D;

			case VariableType::SIntImageCube :
				return GLSL::SIntImageCube;

			case VariableType::SIntImageCubeArray :
				return GLSL::SIntImageCubeArray;

			case VariableType::SIntImageBuffer :
				return GLSL::SIntImageBuffer;

			case VariableType::Texture1D :
				return GLSL::Texture1D;

			case VariableType::Texture2D :
				return GLSL::Texture2D;

			case VariableType::Texture3D :
				return GLSL::Texture3D;

			case VariableType::TextureCube :
				return GLSL::TextureCube;

			case VariableType::Texture2DRectangle :
				return GLSL::Texture2DRectangle;

			case VariableType::Texture1DArray :
				return GLSL::Texture1DArray;

			case VariableType::Texture2DArray :
				return GLSL::Texture2DArray;

			case VariableType::TextureCubeArray :
				return GLSL::TextureCubeArray;

			case VariableType::TextureBuffer :
				return GLSL::TextureBuffer;

			case VariableType::Texture2DMS :
				return GLSL::Texture2DMS;

			case VariableType::Texture2DMSArray :
				return GLSL::Texture2DMSArray;

			case VariableType::UIntTexture1D :
				return GLSL::UIntTexture1D;

			case VariableType::UIntTexture2D :
				return GLSL::UIntTexture2D;

			case VariableType::UIntTexture3D :
				return GLSL::UIntTexture3D;

			case VariableType::UIntTextureCube :
				return GLSL::UIntTextureCube;

			case VariableType::UIntTexture2DRectangle :
				return GLSL::UIntTexture2DRectangle;

			case VariableType::UIntTexture1DArray :
				return GLSL::UIntTexture1DArray;

			case VariableType::UIntTexture2DArray :
				return GLSL::UIntTexture2DArray;

			case VariableType::UIntTextureCubeArray :
				return GLSL::UIntTextureCubeArray;

			case VariableType::UIntTextureBuffer :
				return GLSL::UIntTextureBuffer;

			case VariableType::UIntTexture2DMS :
				return GLSL::UIntTexture2DMS;

			case VariableType::UIntTexture2DMSArray :
				return GLSL::UIntTexture2DMSArray;

			case VariableType::SIntTexture1D :
				return GLSL::SIntTexture1D;

			case VariableType::SIntTexture2D :
				return GLSL::SIntTexture2D;

			case VariableType::SIntTexture3D :
				return GLSL::SIntTexture3D;

			case VariableType::SIntTextureCube :
				return GLSL::SIntTextureCube;

			case VariableType::SIntTexture2DRectangle :
				return GLSL::SIntTexture2DRectangle;

			case VariableType::SIntTexture1DArray :
				return GLSL::SIntTexture1DArray;

			case VariableType::SIntTexture2DArray :
				return GLSL::SIntTexture2DArray;

			case VariableType::SIntTextureCubeArray :
				return GLSL::SIntTextureCubeArray;

			case VariableType::SIntTextureBuffer :
				return GLSL::SIntTextureBuffer;

			case VariableType::SIntTexture2DMS :
				return GLSL::SIntTexture2DMS;

			case VariableType::SIntTexture2DMSArray :
				return GLSL::SIntTexture2DMSArray;

			case VariableType::Sampler :
				return GLSL::Sampler;

			case VariableType::SamplerShadow :
				return GLSL::SamplerShadow;

			case VariableType::SubpassInput :
				return GLSL::SubpassInput;

			case VariableType::SubpassInputMS :
				return GLSL::SubpassInputMS;

			case VariableType::UIntSubpassInput :
				return GLSL::UIntSubpassInput;

			case VariableType::UIntSubpassInputMS :
				return GLSL::UIntSubpassInputMS;

			case VariableType::SIntSubpassInput :
				return GLSL::SIntSubpassInput;

			case VariableType::SIntSubpassInputMS :
				return GLSL::SIntSubpassInputMS;

			case VariableType::Structure :
				return "Structure";

			default:
				return nullptr;
		}
	}

	uint32_t
	size_bytes (VariableType type) noexcept
	{
		switch ( type )
		{
			/* Literals */
			case VariableType::Void :
				return 0;

			case VariableType::Float :
				return sizeof(float);

			case VariableType::Double :
				return sizeof(double);

			case VariableType::UnsignedInteger :
			case VariableType::AtomicUnsignedInteger :
				return sizeof(unsigned int);

			case VariableType::Integer :
				return sizeof(int);

			case VariableType::Boolean :
				return sizeof(bool);

			/* Vectors */
			case VariableType::FloatVector2 :
			case VariableType::FloatVector3 :
			case VariableType::FloatVector4 :
				return 4 * sizeof(float);

			case VariableType::DoubleVector2 :
			case VariableType::DoubleVector3 :
			case VariableType::DoubleVector4 :
				return 4 * sizeof(double);

			case VariableType::UIntVector2 :
			case VariableType::UIntVector3 :
			case VariableType::UIntVector4 :
				return 4 * sizeof(unsigned int);

			case VariableType::SIntVector2 :
			case VariableType::SIntVector3 :
			case VariableType::SIntVector4 :
				return 4 * sizeof(int);

			case VariableType::BooleanVector2 :
			case VariableType::BooleanVector3 :
			case VariableType::BooleanVector4 :
				return 4 * sizeof(bool);

			/* Matrices */
			case VariableType::Matrix2 :
			case VariableType::Matrix2x2 :
				return 4 * sizeof(float);

			case VariableType::Matrix3 :
			case VariableType::Matrix3x3 :
				return 12 * sizeof(float); // FIXME: check alignement

			case VariableType::Matrix4 :
			case VariableType::Matrix4x4 :
				return 16 * sizeof(float);

			case VariableType::Matrix2x3 :
			case VariableType::Matrix2x4 :
			case VariableType::Matrix3x2 :
			case VariableType::Matrix4x2 :
				return 8 * sizeof(float); // FIXME: check alignement

			case VariableType::Matrix3x4 :
			case VariableType::Matrix4x3 :
				return 12 * sizeof(float); // FIXME: check alignement

			case VariableType::DoubleMatrix2 :
			case VariableType::DoubleMatrix2x2 :
				return 4 * sizeof(double);

			case VariableType::DoubleMatrix3 :
			case VariableType::DoubleMatrix3x3 :
				return 12 * sizeof(double); // FIXME: check alignement

			case VariableType::DoubleMatrix4 :
			case VariableType::DoubleMatrix4x4 :
				return 16 * sizeof(double);

			case VariableType::DoubleMatrix2x3 :
			case VariableType::DoubleMatrix2x4 :
			case VariableType::DoubleMatrix3x2 :
			case VariableType::DoubleMatrix4x2 :
				return 8 * sizeof(double); // FIXME: check alignement

			case VariableType::DoubleMatrix3x4 :
			case VariableType::DoubleMatrix4x3 :
				return 12 * sizeof(double); // FIXME: check alignement

			default:
				return 0;
		}
	}
}
