/*
 * src/Saphir/Keys.hpp
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
#include <string>

namespace EmEn::Saphir
{
	using Key = const char *;

	/** @brief Shaders common grammar. */
	namespace Keys
	{
		/** @brief GLSL standard variables naming. */
		namespace GLSL
		{
			namespace Define
			{
				constexpr auto Version = "#version";
				constexpr auto Extension = "#extension";
				constexpr auto Line = "#line";
			}

			namespace Profile
			{
				constexpr auto Core = "core";
				constexpr auto Compatibility = "compatibility";
				constexpr auto EmbeddedSystem = "es";
			}

			namespace Extension
			{
				constexpr auto All = "all";
				constexpr auto Enable = "enable";
				constexpr auto Require = "require";
				constexpr auto Warn = "warn";
				constexpr auto Disable = "disable";
			}

			namespace Macro
			{
				constexpr auto File = "__FILE__";
				constexpr auto Line = "__LINE__";
				constexpr auto Version = "__VERSION__";
				constexpr auto GLCoreProfile = "GL_core_profile";
				constexpr auto GLCompatibilityProfile = "GL_compatibility_profile";
			}

			namespace Primitive
			{
				constexpr auto Points = "points";
				constexpr auto Lines = "lines";
				constexpr auto LineAdjacency = "lines_adjacency";
				constexpr auto LineStrip = "line_strip";
				constexpr auto Triangles = "triangles";
				constexpr auto TrianglesAdjacency = "triangles_adjacency";
				constexpr auto TriangleStrip = "triangle_strip";
			}

			/* Flow controls */
			constexpr auto Break = "break";
			constexpr auto Continue = "continue";
			constexpr auto Do = "do";
			constexpr auto For = "for";
			constexpr auto While = "while";
			constexpr auto Switch = "switch";
			constexpr auto Case = "case";
			constexpr auto Default = "default";
			constexpr auto If = "if";
			constexpr auto Else = "else";
			constexpr auto Discard = "discard";
			constexpr auto Return = "return";

			/* Qualifiers */
			constexpr auto Const = "const";
			constexpr auto Uniform = "uniform";
			constexpr auto In = "in";
			constexpr auto Out = "out";
			constexpr auto InOut = "inout";
			constexpr auto Buffer = "buffer";
			constexpr auto Shared = "shared";
			constexpr auto Packed = "packed";
			constexpr auto Std140 = "std140";
			constexpr auto Std430 = "std430";
			constexpr auto Attribute = "attribute";
			constexpr auto Varying = "varying";
			constexpr auto Coherent = "coherent";
			constexpr auto Volatile = "volatile";
			constexpr auto Restrict = "restrict";
			constexpr auto ReadOnly = "readonly";
			constexpr auto WriteOnly = "writeonly";
			constexpr auto Layout = "layout";
			constexpr auto Centroid = "centroid";
			constexpr auto Flat = "flat";
			constexpr auto Smooth = "smooth";
			constexpr auto NoPerspective = "noperspective";
			constexpr auto Patch = "patch";
			constexpr auto Sample = "sample";
			constexpr auto Invariant = "invariant";
			constexpr auto Precise = "precise";
			constexpr auto Subroutine = "subroutine";
			constexpr auto Struct = "struct";

			/* Layout parameters */
			constexpr auto Set = "set";
			constexpr auto Binding = "binding";
			constexpr auto Location = "location";
			constexpr auto ColumnMajor = "column_major";
			constexpr auto RowMajor = "row_major";
			constexpr auto Align = "align";
			constexpr auto Offset = "offset";
			constexpr auto MaxVertices = "max_vertices";
			constexpr auto PushConstant = "push_constant";
			constexpr auto ConstantId = "constant_id";

			/* Precisions */
			constexpr auto Precision = "precision";
			constexpr auto Low = "lowp";
			constexpr auto Medium = "mediump";
			constexpr auto High = "highp";

			/* Literal types */
			constexpr auto Void = "void";
			constexpr auto Float = "float";
			constexpr auto Double = "double";
			constexpr auto UnsignedInteger = "uint";
			constexpr auto Integer = "int";
			constexpr auto AtomicUnsignedInteger = "atomic_uint";
			constexpr auto Boolean = "bool";
			constexpr auto True = "true";
			constexpr auto False = "false";

			/* Const literal types */
			constexpr auto ConstFloat = "const float";
			constexpr auto ConstDouble = "const double";
			constexpr auto ConstUnsignedInteger = "const uint";
			constexpr auto ConstInteger = "const int";

			/* Vector (Float) types */
			constexpr auto FloatVector2 = "vec2";
			constexpr auto FloatVector3 = "vec3";
			constexpr auto FloatVector4 = "vec4";
			/* Vector (Double) types */
			constexpr auto DoubleVector2 = "dvec2";
			constexpr auto DoubleVector3 = "dvec3";
			constexpr auto DoubleVector4 = "dvec4";
			/* Vector (Unsigned integer) types */
			constexpr auto UIntVector2 = "uvec2";
			constexpr auto UIntVector3 = "uvec3";
			constexpr auto UIntVector4 = "uvec4";
			/* Vector (Signed integer) types */
			constexpr auto SIntVector2 = "ivec2";
			constexpr auto SIntVector3 = "ivec3";
			constexpr auto SIntVector4 = "ivec4";
			/* Vector (Boolean) types */
			constexpr auto BooleanVector2 = "bvec2";
			constexpr auto BooleanVector3 = "bvec3";
			constexpr auto BooleanVector4 = "bvec4";

			/* Const vector (Float) types */
			constexpr auto ConstFloatVector2 = "const vec2";
			constexpr auto ConstFloatVector3 = "const vec3";
			constexpr auto ConstFloatVector4 = "const vec4";
			/* Const vector (Double) types */
			constexpr auto ConstDoubleVector2 = "const dvec2";
			constexpr auto ConstDoubleVector3 = "const dvec3";
			constexpr auto ConstDoubleVector4 = "const dvec4";
			/* Const vector (Unsigned integer) types */
			constexpr auto ConstUIntVector2 = "const uvec2";
			constexpr auto ConstUIntVector3 = "const uvec3";
			constexpr auto ConstUIntVector4 = "const uvec4";
			/* Const vector (Signed integer) types */
			constexpr auto ConstSIntVector2 = "const ivec2";
			constexpr auto ConstSIntVector3 = "const ivec3";
			constexpr auto ConstSIntVector4 = "const ivec4";
			/* Const vector (Boolean) types */
			constexpr auto ConstBooleanVector2 = "const bvec2";
			constexpr auto ConstBooleanVector3 = "const bvec3";
			constexpr auto ConstBooleanVector4 = "const bvec4";

			/* Matrix (Float) types */
			constexpr auto Matrix2 = "mat2";
			constexpr auto Matrix3 = "mat3";
			constexpr auto Matrix4 = "mat4";
			constexpr auto Matrix2x2 = "mat2x2";
			constexpr auto Matrix2x3 = "mat2x3";
			constexpr auto Matrix2x4 = "mat2x4";
			constexpr auto Matrix3x3 = "mat3x3";
			constexpr auto Matrix3x2 = "mat3x2";
			constexpr auto Matrix3x4 = "mat3x4";
			constexpr auto Matrix4x2 = "mat4x2";
			constexpr auto Matrix4x3 = "mat4x3";
			constexpr auto Matrix4x4 = "mat4x4";
			/* Matrix (Double) types */
			constexpr auto DoubleMatrix2 = "dmat2";
			constexpr auto DoubleMatrix3 = "dmat3";
			constexpr auto DoubleMatrix4 = "dmat4";
			constexpr auto DoubleMatrix2x2 = "dmat2x2";
			constexpr auto DoubleMatrix2x3 = "dmat2x3";
			constexpr auto DoubleMatrix2x4 = "dmat2x4";
			constexpr auto DoubleMatrix3x3 = "dmat3x3";
			constexpr auto DoubleMatrix3x2 = "dmat3x2";
			constexpr auto DoubleMatrix3x4 = "dmat3x4";
			constexpr auto DoubleMatrix4x2 = "dmat4x2";
			constexpr auto DoubleMatrix4x3 = "dmat4x3";
			constexpr auto DoubleMatrix4x4 = "dmat4x4";

			/* Const matrix (Float) types */
			constexpr auto ConstMatrix2 = "const mat2";
			constexpr auto ConstMatrix3 = "const mat3";
			constexpr auto ConstMatrix4 = "const mat4";
			constexpr auto ConstMatrix2x2 = "const mat2x2";
			constexpr auto ConstMatrix2x3 = "const mat2x3";
			constexpr auto ConstMatrix2x4 = "const mat2x4";
			constexpr auto ConstMatrix3x3 = "const mat3x3";
			constexpr auto ConstMatrix3x2 = "const mat3x2";
			constexpr auto ConstMatrix3x4 = "const mat3x4";
			constexpr auto ConstMatrix4x2 = "const mat4x2";
			constexpr auto ConstMatrix4x3 = "const mat4x3";
			constexpr auto ConstMatrix4x4 = "const mat4x4";
			/* Const matrix (Double) types */
			constexpr auto ConstDoubleMatrix2 = "const dmat2";
			constexpr auto ConstDoubleMatrix3 = "const dmat3";
			constexpr auto ConstDoubleMatrix4 = "const dmat4";
			constexpr auto ConstDoubleMatrix2x2 = "const dmat2x2";
			constexpr auto ConstDoubleMatrix2x3 = "const dmat2x3";
			constexpr auto ConstDoubleMatrix2x4 = "const dmat2x4";
			constexpr auto ConstDoubleMatrix3x3 = "const dmat3x3";
			constexpr auto ConstDoubleMatrix3x2 = "const dmat3x2";
			constexpr auto ConstDoubleMatrix3x4 = "const dmat3x4";
			constexpr auto ConstDoubleMatrix4x2 = "const dmat4x2";
			constexpr auto ConstDoubleMatrix4x3 = "const dmat4x3";
			constexpr auto ConstDoubleMatrix4x4 = "const dmat4x4";

			/* Sampler (Float) types */
			constexpr auto Sampler1D = "sampler1D";
			constexpr auto Sampler1DArray = "sampler1DArray";
			constexpr auto Sampler2D = "sampler2D";
			constexpr auto Sampler2DArray = "sampler2DArray";
			constexpr auto Sampler2DMS = "sampler2DMS";
			constexpr auto Sampler2DMSArray = "sampler2DMSArray";
			constexpr auto Sampler3D = "sampler3D";
			constexpr auto SamplerCube = "samplerCube";
			constexpr auto SamplerCubeArray = "samplerCubeArray";
			constexpr auto SamplerBuffer = "samplerBuffer";
			/* Sampler (Unsigned integer) types */
			constexpr auto UIntSamplerBuffer = "usamplerBuffer";
			constexpr auto UIntSamplerCubeArray = "usamplerCubeArray";
			constexpr auto UIntSamplerCube = "usamplerCube";
			constexpr auto UIntSampler3D = "usampler3D";
			constexpr auto UIntSampler2DMSArray = "usampler2DMSArray";
			constexpr auto UIntSampler2DMS = "usampler2DMS";
			constexpr auto UIntSampler2DRectangle = "usampler2DRect";
			constexpr auto UIntSampler2D = "usampler2D";
			constexpr auto UIntSampler2DArray = "usampler2DArray";
			constexpr auto UIntSampler1D = "usampler1D";
			constexpr auto UIntSampler1DArray = "usampler1DArray";
			/* Sampler (Signed integer) types */
			constexpr auto SIntSamplerBuffer = "isamplerBuffer";
			constexpr auto SIntSamplerCubeArray = "isamplerCubeArray";
			constexpr auto SIntSamplerCube = "isamplerCube";
			constexpr auto SIntSampler3D = "isampler3D";
			constexpr auto SIntSampler2DMSArray = "isampler2DMSArray";
			constexpr auto SIntSampler2DMS = "isampler2DMS";
			constexpr auto SIntSampler2DRectangle = "isampler2DRect";
			constexpr auto SIntSampler2D = "isampler2D";
			constexpr auto SIntSampler2DArray = "isampler2DArray";
			constexpr auto SIntSampler1D = "isampler1D";
			constexpr auto SIntSampler1DArray = "isampler1DArray";

			/* Shadow sampler (Float only) types */
			constexpr auto Sampler1DShadow = "sampler1DShadow";
			constexpr auto Sampler1DArrayShadow = "sampler1DArrayShadow";
			constexpr auto Sampler2DShadow = "sampler2DShadow";
			constexpr auto Sampler2DArrayShadow = "sampler2DArrayShadow";
			constexpr auto Sampler2DRectangle = "sampler2DRect";
			constexpr auto Sampler2DRectangleShadow = "sampler2DRectShadow";
			constexpr auto SamplerCubeShadow = "samplerCubeShadow";
			constexpr auto SamplerCubeArrayShadow = "samplerCubeArrayShadow";

			/* Image (Float) types */
			constexpr auto Image1D = "image1D";
			constexpr auto Image1DArray = "image1DArray";
			constexpr auto Image2D = "image2D";
			constexpr auto Image2DArray = "image2DArray";
			constexpr auto Image2DRectangle = "image2DRect";
			constexpr auto Image2DMS = "image2DMS";
			constexpr auto Image2DMSArray = "image2DMSArray";
			constexpr auto Image3D = "image3D";
			constexpr auto ImageCube = "imageCube";
			constexpr auto ImageCubeArray = "imageCubeArray";
			constexpr auto ImageBuffer = "imageBuffer";
			/* Image (Unsigned integer) types */
			constexpr auto UIntImage1D = "uimage1D";
			constexpr auto UIntImage1DArray = "uimage1DArray";
			constexpr auto UIntImage2D = "uimage2D";
			constexpr auto UIntImage2DArray = "uimage2DArray";
			constexpr auto UIntImage2DRectangle = "uimage2DRect";
			constexpr auto UIntImage2DMS = "uimage2DMS";
			constexpr auto UIntImage2DMSArray = "uimage2DMSArray";
			constexpr auto UIntImage3D = "uimage3D";
			constexpr auto UIntImageCube = "uimageCube";
			constexpr auto UIntImageCubeArray = "uimageCubeArray";
			constexpr auto UIntImageBuffer = "uimageBuffer";
			/* Image (Signed integer) types */
			constexpr auto SIntImage1D = "iimage1D";
			constexpr auto SIntImage1DArray = "iimage1DArray";
			constexpr auto SIntImage2D = "iimage2D";
			constexpr auto SIntImage2DArray = "iimage2DArray";
			constexpr auto SIntImage2DRectangle = "iimage2DRect";
			constexpr auto SIntImage2DMS = "iimage2DMS";
			constexpr auto SIntImage2DMSArray = "iimage2DMSArray";
			constexpr auto SIntImage3D = "iimage3D";
			constexpr auto SIntImageCube = "iimageCube";
			constexpr auto SIntImageCubeArray = "iimageCubeArray";
			constexpr auto SIntImageBuffer = "iimageBuffer";

			/* === Vulkan only ==== */

			/* Texture (Float) types */
			constexpr auto Texture1D = "texture1D";
			constexpr auto Texture2D = "texture2D";
			constexpr auto Texture3D = "texture3D";
			constexpr auto TextureCube = "textureCube";
			constexpr auto Texture2DRectangle = "texture2DRect";
			constexpr auto Texture1DArray = "texture1DArray";
			constexpr auto Texture2DArray = "texture2DArray";
			constexpr auto TextureCubeArray = "textureCubeArray";
			constexpr auto TextureBuffer = "textureBuffer";
			constexpr auto Texture2DMS = "texture2DMS";
			constexpr auto Texture2DMSArray = "texture2DMSArray";
			/* Texture (Unsigned integer) types */
			constexpr auto UIntTexture1D = "utexture1D";
			constexpr auto UIntTexture2D = "utexture2D";
			constexpr auto UIntTexture3D = "utexture3D";
			constexpr auto UIntTextureCube = "utextureCube";
			constexpr auto UIntTexture2DRectangle = "utexture2DRect";
			constexpr auto UIntTexture1DArray = "utexture1DArray";
			constexpr auto UIntTexture2DArray = "utexture2DArray";
			constexpr auto UIntTextureCubeArray = "utextureCubeArray";
			constexpr auto UIntTextureBuffer = "utextureBuffer";
			constexpr auto UIntTexture2DMS = "utexture2DMS";
			constexpr auto UIntTexture2DMSArray = "utexture2DMSArray";
			/* Texture (Signed integer) types */
			constexpr auto SIntTexture1D = "itexture1D";
			constexpr auto SIntTexture2D = "itexture2D";
			constexpr auto SIntTexture3D = "itexture3D";
			constexpr auto SIntTextureCube = "itextureCube";
			constexpr auto SIntTexture2DRectangle = "itexture2DRect";
			constexpr auto SIntTexture1DArray = "itexture1DArray";
			constexpr auto SIntTexture2DArray = "itexture2DArray";
			constexpr auto SIntTextureCubeArray = "itextureCubeArray";
			constexpr auto SIntTextureBuffer = "itextureBuffer";
			constexpr auto SIntTexture2DMS = "itexture2DMS";
			constexpr auto SIntTexture2DMSArray = "itexture2DMSArray";

			/* Additional sampler types */
			constexpr auto Sampler = "sampler";
			constexpr auto SamplerShadow = "samplerShadow";

			/* Subpass (Float) types */
			constexpr auto SubpassInput = "subpassInput";
			constexpr auto SubpassInputMS = "subpassInputMS";
			/* Subpass (Unsigned integer) types */
			constexpr auto UIntSubpassInput = "usubpassInput";
			constexpr auto UIntSubpassInputMS = "usubpassInputMS";
			/* Subpass (Signed integer) types */
			constexpr auto SIntSubpassInput = "isubpassInput";
			constexpr auto SIntSubpassInputMS = "isubpassInputMS";

			/* === Reserved === */

			constexpr auto _Common = "common";
			constexpr auto _Partition = "partition";
			constexpr auto _Active = "active";
			constexpr auto _ASM = "asm";
			constexpr auto _Class = "class";
			constexpr auto _Union = "union";
			constexpr auto _Enum = "enum";
			constexpr auto _Typedef = "typedef";
			constexpr auto _Template = "template";
			constexpr auto _This = "this";
			constexpr auto _Resource = "resource";
			constexpr auto _Goto = "goto";
			constexpr auto _Inline = "inline";
			constexpr auto _NoInline = "noinline";
			constexpr auto _Public = "public";
			constexpr auto _Static = "static";
			constexpr auto _Extern = "extern";
			constexpr auto _External = "external";
			constexpr auto _Interface = "interface";
			constexpr auto _Long = "long";
			constexpr auto _Short = "short";
			constexpr auto _Half = "half";
			constexpr auto _Fixed = "fixed";
			constexpr auto _Unsigned = "unsigned";
			constexpr auto _Superp = "superp";
			constexpr auto _Input = "input";
			constexpr auto _Output = "output";
			constexpr auto _HalfVector2 = "hvec2";
			constexpr auto _HalfVector3 = "hvec3";
			constexpr auto _HalfVector4 = "hvec4";
			constexpr auto _FloatVector2 = "fvec2";
			constexpr auto _FloatVector3 = "fvec3";
			constexpr auto _FloatVector4 = "fvec4";
			constexpr auto _Filter = "filter";
			constexpr auto _SizeOf = "sizeof";
			constexpr auto _Cast = "cast";
			constexpr auto _Namespace = "namespace";
			constexpr auto _Using = "using";
			constexpr auto _Sampler3DRectangle = "sampler3DRect";

			namespace _Qualifier
			{
				constexpr auto Uniform = "uniform";
				constexpr auto In = "in";
				constexpr auto ConstIn = "const in";
				constexpr auto Out = "out";
				constexpr auto InOut = "inout";
				constexpr auto Layout = "layout";
				constexpr auto Const = "const";
				constexpr auto Precision = "precision";
				constexpr auto Location = "location";
				constexpr auto Buffer = "buffer";
				constexpr auto ColumnMajor = "column_major";
				constexpr auto RowMajor = "row_major";
				constexpr auto Set = "set";
				constexpr auto Binding = "binding";
				constexpr auto Align = "align";
				constexpr auto Offset = "offset";
				constexpr auto Struct = "struct";
				constexpr auto MaxVertices = "max_vertices";
				constexpr auto PushConstant = "push_constant";
				constexpr auto ConstantId = "constant_id";

				namespace _Interpolation
				{
					constexpr auto Flat = "flat";
					constexpr auto Smooth = "smooth";
					constexpr auto NoPerspective = "noperspective";
					constexpr auto Centroid = "centroid";
				}

				namespace _Memory
				{
					constexpr auto Coherent = "coherent";
					constexpr auto Volatile = "volatile";
					constexpr auto Restrict = "restrict";
					constexpr auto ReadOny = "readonly";
					constexpr auto WriteOnly = "writeonly";
					constexpr auto Shared = "shared";
					constexpr auto Packed = "packed";
					constexpr auto Std140 = "std140";
					constexpr auto Std430 = "std430";
				}
			}

			namespace Vertex
			{
				/*
					in int gl_VertexID;
					in int gl_InstanceID;
					in int gl_DrawID; // Requires GLSL 4.60 or ARB_shader_draw_parameters
					in int gl_BaseVertex; // Requires GLSL 4.60 or ARB_shader_draw_parameters
					in int gl_BaseInstance; // Requires GLSL 4.60 or ARB_shader_draw_parameters
				*/
				namespace In
				{
					constexpr auto VertexID = "gl_VertexID";
					constexpr auto InstanceID = "gl_InstanceID";
					constexpr auto DrawID = "gl_DrawID"; // Requires GLSL 4.60 or ARB_shader_draw_parameters
					constexpr auto BaseVertex = "gl_BaseVertex"; // Requires GLSL 4.60 or ARB_shader_draw_parameters
					constexpr auto BaseInstance = "gl_BaseInstance"; // Requires GLSL 4.60 or ARB_shader_draw_parameters
				}
				/*
					out gl_PerVertex
					{
					  vec4 gl_Position;
					  float gl_PointSize;
					  float gl_ClipDistance[];
					};
				*/
				namespace Out
				{
					constexpr auto Position = "gl_Position";
					constexpr auto PointSize = "gl_PointSize";
					constexpr auto ClipDistance = "gl_ClipDistance";
				}
			}

			namespace Tesselation
			{
				namespace In
				{

				}

				namespace Out
				{

				}
			}

			namespace Geometry
			{
				/*
					in gl_PerVertex
					{
					  vec4 gl_Position;
					  float gl_PointSize;
					  float gl_ClipDistance[];
					} gl_in[];

					in int gl_PrimitiveIDIn;
					in int gl_InvocationID; // Requires GLSL 4.0 or ARB_gpu_shader5
				*/
				namespace In
				{
					constexpr auto In = "gl_in";
					constexpr auto Position = "gl_Position";
					constexpr auto PointSize = "gl_PointSize";
					constexpr auto ClipDistance = "gl_ClipDistance";
					constexpr auto IndexedPosition = "gl_in[index].gl_Position";
					constexpr auto IndexedPointSize = "gl_in[index].gl_PointSize";
					constexpr auto IndexedClipDistance = "gl_in[index].gl_ClipDistance";
					constexpr auto PrimitiveIDIn = "gl_PrimitiveIDIn";
					constexpr auto InvocationID = "gl_InvocationID";
				}
				/*
					out gl_PerVertex
					{
					  vec4 gl_Position;
					  float gl_PointSize;
					  float gl_ClipDistance[];
					};
					out int gl_PrimitiveID;
					out int gl_Layer;
					out int gl_ViewportIndex; // Requires GL 4.1 or ARB_viewport_array.
				*/
				namespace Out
				{
					constexpr auto Position = "gl_Position";
					constexpr auto PointSize = "gl_PointSize";
					constexpr auto ClipDistance = "gl_ClipDistance";
					constexpr auto PrimitiveID = "gl_PrimitiveID";
					constexpr auto Layer = "gl_Layer";
					constexpr auto ViewportIndex = "gl_ViewportIndex";
				}

				namespace Function
				{
					constexpr auto EmitStreamVertex = "EmitStreamVertex";
					constexpr auto EndStreamPrimitive = "EndStreamPrimitive";
					constexpr auto EmitVertex = "EmitVertex";
					constexpr auto EndPrimitive = "EndPrimitive";
				}
			}

			namespace Fragment
			{
				/*
					in vec4 gl_FragCoord;
					in bool gl_FrontFacing;
					in vec2 gl_PointCoord;
					in int gl_SampleID;
					in vec2 gl_SamplePosition;
					in int gl_SampleMaskIn[];
					in float gl_ClipDistance[];
					in int gl_PrimitiveID;
					in int gl_Layer;
					in int gl_ViewportIndex;
				*/
				namespace In
				{
					constexpr auto FragCoord = "gl_FragCoord";
					constexpr auto FrontFacing = "gl_FrontFacing";
					constexpr auto PointCoord = "gl_PointCoord";
					constexpr auto SampleID = "gl_SampleID";
					constexpr auto SamplePosition = "gl_SamplePosition";
					constexpr auto SampleMaskIn = "gl_SampleMaskIn"; // []
					constexpr auto ClipDistance = "gl_ClipDistance"; // []
					constexpr auto PrimitiveID = "gl_PrimitiveID";
					constexpr auto Layer = "gl_Layer";
					constexpr auto ViewportIndex = "gl_ViewportIndex";
				}
				/*
					out float gl_FragDepth;
					out int gl_SampleMask[];
				*/
				namespace Out
				{
					constexpr auto FragDepth = "gl_FragDepth";
					constexpr auto SampleMask = "gl_SampleMask";
					constexpr auto FragColor = "gl_FragColor";
				}
			}

			namespace Functions
			{
				constexpr auto Length = "length";
			}
		}

		/** Vertex attributes naming convention. */

		/**
		 * @brief The name of fixed vertex attributes in Saphir.
		 * @note See EmEn::Graphics::VertexAttributeType for predefined vertex attribute in geometry.
		 * Use EmEn::Saphir::getVertexAttributeVariableName() to convert a attribute type to one of these variables.
		 */
		namespace Attribute
		{
			constexpr auto Position{"vaVertex"};
			constexpr auto Tangent{"vaTangent"};
			constexpr auto Binormal{"vaBinormal"};
			constexpr auto Normal{"vaNormal"};
			constexpr auto Color{"vaColor"};
			constexpr auto Primary2DTextureCoordinates{"va2DTexCoord0"};
			constexpr auto Primary3DTextureCoordinates{"va3DTexCoord0"};
			constexpr auto Secondary2DTextureCoordinates{"va2DTexCoord1"};
			constexpr auto Secondary3DTextureCoordinates{"va3DTexCoord1"};
			constexpr auto ModelMatrix{"vaModelMatrix"};
			constexpr auto NormalModelMatrix{"vaNormalModelMatrix"};
			constexpr auto ModelPosition{"vaModelPosition"};
			constexpr auto ModelScaling{"vaModelScaling"};
		}

		/** @brief Structures naming convention. */
		namespace Struct
		{
			constexpr auto CubemapFace{"CubemapFace"};
		}

		/** @brief Uniforms naming convention. */
		namespace Uniform
		{
			/* Generic */
			constexpr auto PrimarySampler{"uPrimarySampler"};
			constexpr auto SecondarySampler{"uSecondarySampler"};
			/* Material specific */
			constexpr auto AmbientSampler{"uAmbientSampler"};
			constexpr auto DiffuseSampler{"uDiffuseSampler"};
			constexpr auto SpecularSampler{"uSpecularSampler"};
			constexpr auto AutoIlluminationSampler{"uAutoIlluminationSampler"};
			constexpr auto NormalSampler{"uNormalSampler"};
			constexpr auto ReflectionSampler{"uReflectionSampler"};
			constexpr auto OpacitySampler{"uOpacitySampler"};
			constexpr auto ShadowMapSampler{"uShadowMapSampler"};
		}

		/** @brief Uniforms block variables naming convention. */
		namespace UniformBlock
		{
			constexpr auto View{"ubView"};
			constexpr auto Material{"ubMaterial"};
			constexpr auto Light{"ubLight"};

			namespace Type
			{
				constexpr auto View{"View"};
				constexpr auto CubemapView{"CubemapView"};
				constexpr auto BasicMaterial{"BasicMaterial"};
				constexpr auto StandardMaterial{"StandardMaterial"};
				constexpr auto DirectionalLight{"DirectionalLight"};
				constexpr auto PointLight{"PointLight"};
				constexpr auto SpotLight{"SpotLight"};
			}

			namespace Component
			{
				/* Generic */
				constexpr auto Instance{"instance"};
				constexpr auto Amount{"amount"};
				constexpr auto Scale{"scale"};
				/* View specific */
				constexpr auto ProjectionMatrix{"projectionMatrix"};
				constexpr auto PositionWorldSpace{"positionWorldSpace"};
				constexpr auto Velocity = "velocity";
				constexpr auto ViewProperties{"viewProperties"};
				constexpr auto AmbientLightColor{"ambientLightColor"};
				constexpr auto AmbientLightIntensity{"ambientLightIntensity"};
				/* Material specific */
				constexpr auto AmbientColor{"ambientColor"};
				constexpr auto DiffuseColor{"diffuseColor"};
				constexpr auto SpecularColor{"specularColor"};
				constexpr auto Shininess{"shininess"};
				constexpr auto AutoIlluminationColor{"autoIlluminationColor"};
				constexpr auto AutoIlluminationAmount{"autoIlluminationAmount"};
				constexpr auto Opacity{"opacity"};
				constexpr auto NormalScale{"normalScale"};
				constexpr auto ReflectionAmount{"reflectionAmount"};
				/* Light specific */
				constexpr auto Color{"color"};
				//constexpr auto PositionWorldSpace{"positionWorldSpace"}; // Shared with View
				constexpr auto DirectionWorldSpace{"directionWorldSpace"};
				constexpr auto Intensity{"intensity"};
				constexpr auto Radius{"radius"};
				constexpr auto InnerCosAngle{"innerCosAngle"};
				constexpr auto OuterCosAngle{"outerCosAngle"};
				constexpr auto ViewProjectionMatrix{"viewProjectionMatrix"};
			}
		}

		/** @brief Push constant variables naming convention. */
		namespace PushConstant
		{
			constexpr auto Matrices{"pcMatrices"};
			constexpr auto Overlay{"pcOverlay"};

			namespace Type
			{
				constexpr auto Matrices{"Matrices"};
				constexpr auto Overlay{"Overlay"};
			}

			namespace Component
			{
				/* NOTE : Regular 3D rendering. */
				constexpr auto ViewMatrix{"viewMatrix"};
				constexpr auto ModelMatrix{"modelMatrix"};
				constexpr auto ViewProjectionMatrix{"viewProjectionMatrix"};
				constexpr auto ModelViewProjectionMatrix{"modelViewProjectionMatrix"};
				/* NOTE : Specific (overlay, ...) rendering. */
				constexpr auto TransformationMatrix{"transformationMatrix"};
			}
		}

		/** @brief Interstage variables naming convention. */
		namespace ShaderVariable
		{
			constexpr auto PositionWorldSpace{"svPositionWorldSpace"};
			constexpr auto GLPositionWorldSpace{"gl_Position@WorldSpace"}; /* NOTE: Must never appear in real code. */
			constexpr auto PositionViewSpace{"svPositionViewSpace"};
			constexpr auto PositionTextureSpace{"svPositionTextureSpace"};
			constexpr auto PositionScreenSpace{"gl_Position"};
			constexpr auto PrimaryVertexColor{"svPrimaryVertexColor"};
			constexpr auto SecondaryVertexColor{"svSecondaryVertexColor"};
			constexpr auto Primary2DTextureCoordinates{"sv2DTexCoord0"};
			constexpr auto Primary3DTextureCoordinates{"sv3DTexCoord0"};
			constexpr auto Secondary2DTextureCoordinates{"sv2DTexCoord1"};
			constexpr auto Secondary3DTextureCoordinates{"sv3DTexCoord1"};
			constexpr auto ReflectionTextureCoordinates{"svReflectionTexCoord"};
			constexpr auto TangentWorldSpace{"svTangentWorldSpace"};
			constexpr auto TangentViewSpace{"svTangentViewSpace"};
			constexpr auto BinormalWorldSpace{"svBinormalWorldSpace"};
			constexpr auto BinormalViewSpace{"svBinormalViewSpace"};
			constexpr auto NormalWorldSpace{"svNormalWorldSpace"};
			constexpr auto NormalViewSpace{"svNormalViewSpace"};
			constexpr auto SpriteModelMatrix{"svSpriteModelMatrix"};
			constexpr auto ModelViewMatrix{"svModelViewMatrix"};
			constexpr auto NormalMatrix{"svNormalMatrix"};
			constexpr auto ModelViewProjectionMatrix{"svModelViewProjectionMatrix"};
			constexpr auto WorldTBNMatrix{"svWorldTBNMatrix"};
			constexpr auto ViewTBNMatrix{"svViewTBNMatrix"};
			constexpr auto WorldToTangentMatrix{"svWorldToTangent"};
			constexpr auto OutputFragment{"svOutputFragment"};
			constexpr auto FragCoord{"svFragCoord"};
			constexpr auto Light{"svLight"};
		}

		/** @brief Shader block variables naming convention. */
		namespace ShaderBlock
		{
			namespace Component
			{
				constexpr auto AttenuationFactor = "AttenuationFactor";
				constexpr auto SpotFactor = "SpotFactor";
				constexpr auto DirectionWorldSpace = "directionWorldSpace";
				constexpr auto DirectionViewSpace = "directionViewSpace";
				constexpr auto DirectionTextureSpace = "directionTextureSpace";
				constexpr auto ViewDirectionTextureSpace = "viewDirectionTextureSpace";
				constexpr auto PositionLightSpace = "positionLightSpace";
			}
		}
	}

	/**
	* @brief Returns a variable from the current matrix push constant.
	* @param componentName The push constant component name. See Keys::PushConstant::Component.
	* @return std::string
	*/
	[[nodiscard]]
	std::string
	MatrixPC (const char * componentName) noexcept;

	/**
	* @brief Returns a variable from the current overlay push constant.
	* @param componentName The push constant component name. See Keys::PushConstant::Component.
	* @return std::string
	*/
	[[nodiscard]]
	std::string
	OverlayPC (const char * componentName) noexcept;

	/**
	 * @brief Returns the full variable name from a view uniform block.
	 * @param componentName The uniform block component name. See Keys::UniformBlock::Component.
	 * @return std::string
	 */
	[[nodiscard]]
	std::string ViewUB (const char * componentName) noexcept;

	/**
	 * @brief Returns the full variable name from an indexed view uniform block.
	 * @param memberName The uniform block component name. See Keys::UniformBlock::Component.
	 * @param indexVariableName The index in the array.
	 * @param componentName The uniform block component name. See Keys::UniformBlock::Component.
	 * @return std::string
	 */
	[[nodiscard]]
	std::string CubeViewUB (const char * memberName, const char * indexVariableName, const char * componentName) noexcept;

	/**
	 * @brief Returns the full variable name from a material uniform block.
	 * @param componentName The uniform block component name. See Keys::UniformBlock::Component.
	 * @return std::string
	 */
	[[nodiscard]]
	std::string MaterialUB (const char * componentName) noexcept;

	/**
	 * @brief Returns a variable from the current light uniform block.
	 * @param componentName The uniform block component name. See Keys::UniformBlock::Component.
	 * @return std::string
	 */
	[[nodiscard]]
	std::string LightUB (const char * componentName) noexcept;
}
