/*
 * src/Graphics/Types.cpp
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

/* STL inclusions. */
#include <iostream>

namespace EmEn::Graphics
{
	const char *
	to_cstring (RenderTargetType value) noexcept
	{
		switch ( value )
		{
			case RenderTargetType::View :
				return ViewString;

			case RenderTargetType::CubicView :
				return CubicViewString;

			case RenderTargetType::ShadowMap :
				return ShadowMapString;

			case RenderTargetType::ShadowCubemap :
				return ShadowCubemapString;

			case RenderTargetType::Texture :
				return TextureString;

			case RenderTargetType::Cubemap :
				return CubemapString;
		}

		return nullptr;
	}

	const char *
	to_cstring (RenderPassType value) noexcept
	{
		switch ( value )
		{
			case RenderPassType::SimplePass :
				return SimplePassString;

			case RenderPassType::AmbientPass :
				return AmbientPassString;

			case RenderPassType::DirectionalLightPass :
				return DirectionalLightPassString;

			case RenderPassType::DirectionalLightPassNoShadow :
				return DirectionalLightPassNoShadowString;

			case RenderPassType::PointLightPass :
				return PointLightPassString;

			case RenderPassType::PointLightPassNoShadow :
				return PointLightPassNoShadowString;

			case RenderPassType::SpotLightPass :
				return SpotLightPassString;

			case RenderPassType::SpotLightPassNoShadow :
				return SpotLightPassNoShadowString;

			case RenderPassType::None :
				return NoneString;
		}

		return nullptr;
	}

	RenderPassType
	to_RenderPassType (const std::string & value) noexcept
	{
		if ( value == SimplePassString )
		{
			return RenderPassType::SimplePass;
		}

		if ( value == AmbientPassString )
		{
			return RenderPassType::AmbientPass;
		}

		if ( value == DirectionalLightPassString )
		{
			return RenderPassType::DirectionalLightPass;
		}

		if ( value == DirectionalLightPassNoShadowString )
		{
			return RenderPassType::DirectionalLightPassNoShadow;
		}

		if ( value == PointLightPassString )
		{
			return RenderPassType::PointLightPass;
		}

		if ( value == PointLightPassNoShadowString )
		{
			return RenderPassType::PointLightPassNoShadow;
		}

		if ( value == SpotLightPassString )
		{
			return RenderPassType::SpotLightPass;
		}

		if ( value == SpotLightPassNoShadowString )
		{
			return RenderPassType::SpotLightPassNoShadow;
		}

		if ( value == NoneString )
		{
			return RenderPassType::None;
		}

		std::cerr << __PRETTY_FUNCTION__ << " : Unknown '" << value << "' type ! Returning 'None' by default." << "\n";

		return RenderPassType::None;
	}

	const char *
	to_cstring (ModelMatrixType value) noexcept
	{
		switch ( value )
		{
			case ModelMatrixType::UniformBufferObject :
				return UniformBufferObjectString;

			case ModelMatrixType::VertexBufferObject :
				return VertexBufferObjectString;

			case ModelMatrixType::Invalid :
				return InvalidString;
		}

		return nullptr;
	}

	ModelMatrixType
	to_ModelMatrixType (const std::string & value) noexcept
	{
		if ( value == UniformBufferObjectString )
		{
			return ModelMatrixType::UniformBufferObject;
		}

		if ( value == VertexBufferObjectString )
		{
			return ModelMatrixType::VertexBufferObject;
		}

		if ( value == InvalidString )
		{
			return ModelMatrixType::Invalid;
		}

		std::cerr << __PRETTY_FUNCTION__ << " : Unknown '" << value << "' type ! Returning 'Invalid' by default." << "\n";

		return ModelMatrixType::Invalid;
	}

	const char *
	to_cstring (FillingType value) noexcept
	{
		switch ( value )
		{
			case FillingType::Value :
				return ValueString;

			case FillingType::Color :
				return ColorString;

			case FillingType::Gradient :
				return GradientString;

			case FillingType::Texture :
				return TextureString;

			case FillingType::VolumeTexture :
				return VolumeTextureString;

			case FillingType::Cubemap :
				return CubemapString;

			case FillingType::AnimatedTexture :
				return AnimatedTextureString;

			case FillingType::AlphaChannelAsValue :
				return AlphaChannelAsValueString;

			case FillingType::None :
				return NoneString;
		}

		return nullptr;
	}

	FillingType
	to_FillingType (const std::string & value) noexcept
	{
		if ( value == ValueString )
		{
			return FillingType::Value;
		}

		if ( value == ColorString )
		{
			return FillingType::Color;
		}

		if ( value == GradientString )
		{
			return FillingType::Gradient;
		}

		if ( value == TextureString )
		{
			return FillingType::Texture;
		}

		if ( value == VolumeTextureString )
		{
			return FillingType::VolumeTexture;
		}

		if ( value == CubemapString )
		{
			return FillingType::Cubemap;
		}

		if ( value == AnimatedTextureString )
		{
			return FillingType::AnimatedTexture;
		}

		if ( value == AlphaChannelAsValueString )
		{
			return FillingType::AlphaChannelAsValue;
		}

		if ( value == NoneString )
		{
			return FillingType::None;
		}

		std::cerr << __PRETTY_FUNCTION__ << " : Unknown '" << value << "' type ! Returning 'None' by default." << "\n";

		return FillingType::None;
	}

	const char *
	to_cstring (BlendingMode value) noexcept
	{
		switch ( value )
		{
			case BlendingMode::Normal :
				return NormalBlendingString;

			case BlendingMode::Add :
				return AddBlendingString;

			case BlendingMode::Multiply :
				return MultiplyBlendingString;

			case BlendingMode::Screen :
				return ScreenBlendingString;

			case BlendingMode::None :
				return NoneString;
		}

		return nullptr;
	}

	BlendingMode
	to_BlendingMode (const std::string & value) noexcept
	{
		if ( value == NormalBlendingString )
		{
			return BlendingMode::Normal;
		}

		if ( value == AddBlendingString )
		{
			return BlendingMode::Add;
		}

		if ( value == MultiplyBlendingString )
		{
			return BlendingMode::Multiply;
		}

		if ( value == ScreenBlendingString )
		{
			return BlendingMode::Screen;
		}

		if ( value == NoneString )
		{
			return BlendingMode::None;
		}

		std::cerr << __PRETTY_FUNCTION__ << " : Unknown '" << value << "' type ! Returning 'None' by default." << "\n";

		return BlendingMode::None;
	}

	const char *
	to_cstring (ComponentType value) noexcept
	{
		switch ( value )
		{
			case ComponentType::Ambient :
				return AmbientString;

			case ComponentType::Diffuse :
				return DiffuseString;

			case ComponentType::Specular :
				return SpecularString;

			case ComponentType::Albedo :
				return AlbedoString;

			case ComponentType::AutoIllumination :
				return AutoIlluminationString;

			case ComponentType::Opacity :
				return OpacityString;

			case ComponentType::Normal :
				return NormalString;

			case ComponentType::Displacement :
				return DisplacementString;

			case ComponentType::Reflection :
				return ReflectionString;

			case ComponentType::Roughness :
				return RoughnessString;

			case ComponentType::Metalness :
				return MetalnessString;

			case ComponentType::Invalid :
				return InvalidString;
		}

		return nullptr;
	}

	ComponentType
	to_ComponentType (const std::string & value) noexcept
	{
		if ( value == AmbientString )
		{
			return ComponentType::Ambient;
		}

		if ( value == DiffuseString )
		{
			return ComponentType::Diffuse;
		}

		if ( value == SpecularString )
		{
			return ComponentType::Specular;
		}

		if ( value == NormalString )
		{
			return ComponentType::Normal;
		}

		if ( value == DisplacementString )
		{
			return ComponentType::Displacement;
		}

		if ( value == OpacityString )
		{
			return ComponentType::Opacity;
		}

		if ( value == AutoIlluminationString )
		{
			return ComponentType::AutoIllumination;
		}

		if ( value == ReflectionString )
		{
			return ComponentType::Reflection;
		}

		if ( value == InvalidString )
		{
			return ComponentType::Invalid;
		}

		std::cerr << __PRETTY_FUNCTION__ << " : Unknown '" << value << "' type ! Returning 'Invalid' by default." << "\n";

		return ComponentType::Invalid;
	}

	const char *
	to_cstring (Topology value) noexcept
	{
		switch ( value )
		{
			case Topology::PointList :
				return PointListString;

			case Topology::LineLine :
				return LineLineString;

			case Topology::LineStrip :
				return LineStripString;

			case Topology::TriangleList :
				return TriangleListString;

			case Topology::TriangleStrip :
				return TriangleStripString;

			case Topology::TriangleFan :
				return TriangleFanString;

			case Topology::LineListWithAdjacency :
				return LineListWithAdjacencyString;

			case Topology::LineStripWithAdjacency :
				return LineStripWithAdjacencyString;

			case Topology::TriangleListWithAdjacency :
				return TriangleListWithAdjacencyString;

			case Topology::TriangleStripWithAdjacency :
				return TriangleStripWithAdjacencyString;

			case Topology::PatchList :
				return PatchListString;

			case Topology::CustomData :
				return CustomDataString;

			case Topology::Invalid :
				return InvalidString;
		}

		return nullptr;
	}

	Topology
	to_TopologyType (const std::string & value) noexcept
	{
		if ( value == PointListString )
		{
			return Topology::PointList;
		}

		if ( value == LineLineString )
		{
			return Topology::LineLine;
		}

		if ( value == LineStripString )
		{
			return Topology::LineStrip;
		}

		if ( value == TriangleListString )
		{
			return Topology::TriangleList;
		}

		if ( value == TriangleStripString )
		{
			return Topology::TriangleStrip;
		}

		if ( value == TriangleFanString )
		{
			return Topology::TriangleFan;
		}

		if ( value == LineListWithAdjacencyString )
		{
			return Topology::LineListWithAdjacency;
		}

		if ( value == LineStripWithAdjacencyString )
		{
			return Topology::LineStripWithAdjacency;
		}

		if ( value == TriangleListWithAdjacencyString )
		{
			return Topology::TriangleListWithAdjacency;
		}

		if ( value == TriangleStripWithAdjacencyString )
		{
			return Topology::TriangleStripWithAdjacency;
		}

		if ( value == PatchListString )
		{
			return Topology::PatchList;
		}

		if ( value == CustomDataString )
		{
			return Topology::CustomData;
		}

		if ( value == InvalidString )
		{
			return Topology::Invalid;
		}

		std::cerr << __PRETTY_FUNCTION__ << " : Unknown '" << value << "' type ! Returning 'Invalid' by default." << "\n";

		return Topology::Invalid;
	}

	const char *
	to_cstring (PolygonMode value) noexcept
	{
		switch ( value )
		{
			case PolygonMode::Fill :
				return FillString;

			case PolygonMode::Line :
				return LineString;

			case PolygonMode::Point :
				return PointString;

			case PolygonMode::FillRectangle :
				return FillRectangleString;

			case PolygonMode::Invalid :
				return InvalidString;
		}

		return nullptr;
	}

	PolygonMode
	to_PolygonMode (const std::string & value) noexcept
	{
		if ( value == FillString )
		{
			return PolygonMode::Fill;
		}

		if ( value == LineString )
		{
			return PolygonMode::Line;
		}

		if ( value == PointString )
		{
			return PolygonMode::Point;
		}

		if ( value == FillRectangleString )
		{
			return PolygonMode::FillRectangle;
		}

		if ( value == InvalidString )
		{
			return PolygonMode::Invalid;
		}

		std::cerr << __PRETTY_FUNCTION__ << " : Unknown '" << value << "' type ! Returning 'Invalid' by default." << "\n";

		return PolygonMode::Invalid;
	}

	const char *
	to_cstring (CullingMode value) noexcept
	{
		switch ( value )
		{
			case CullingMode::None :
				return NoneString;

			case CullingMode::Back :
				return BackString;

			case CullingMode::Front :
				return FrontString;

			case CullingMode::Both :
				return BothString;

			case CullingMode::Invalid :
				return InvalidString;
		}

		return nullptr;
	}

	CullingMode
	to_CullingMode (const std::string & value) noexcept
	{
		if ( value == BackString )
		{
			return CullingMode::Back;
		}

		if ( value == FrontString )
		{
			return CullingMode::Front;
		}

		if ( value == BothString )
		{
			return CullingMode::Both;
		}

		if ( value == NoneString )
		{
			return CullingMode::None;
		}

		std::cerr << __PRETTY_FUNCTION__ << " : Unknown '" << value << "' type ! Returning 'None' by default." << "\n";

		return CullingMode::None;
	}

	const char *
	to_cstring (BufferType value) noexcept
	{
		switch ( value )
		{
			case BufferType::ColorBuffer :
				return ColorBufferString;

			case BufferType::DepthBuffer :
				return DepthBufferString;

			case BufferType::StencilBuffer :
				return StencilBufferString;

			case BufferType::Invalid :
				return InvalidString;
		}

		return nullptr;
	}

	BufferType
	to_BufferType (const std::string & value) noexcept
	{
		if ( value == ColorBufferString )
		{
			return BufferType::ColorBuffer;
		}

		if ( value == DepthBufferString )
		{
			return BufferType::DepthBuffer;
		}

		if ( value == StencilBufferString )
		{
			return BufferType::StencilBuffer;
		}

		if ( value == InvalidString )
		{
			return BufferType::Invalid;
		}

		std::cerr << __PRETTY_FUNCTION__ << " : Unknown '" << value << "' type ! Returning 'Invalid' by default." << "\n";

		return BufferType::Invalid;
	}

	const char *
	to_cstring (VertexAttributeType value) noexcept
	{
		switch ( value )
		{
			case VertexAttributeType::Position :
				return PositionString;

			case VertexAttributeType::Tangent :
				return TangentString;

			case VertexAttributeType::Binormal :
				return BinormalString;

			case VertexAttributeType::Normal :
				return NormalString;

			case VertexAttributeType::VertexColor :
				return VertexColorString;

			case VertexAttributeType::Primary2DTextureCoordinates :
				return Primary2DTextureCoordinatesString;

			case VertexAttributeType::Primary3DTextureCoordinates :
				return Primary3DTextureCoordinatesString;

			case VertexAttributeType::Secondary2DTextureCoordinates :
				return Secondary2DTextureCoordinatesString;

			case VertexAttributeType::Secondary3DTextureCoordinates :
				return Secondary3DTextureCoordinatesString;

			case VertexAttributeType::ModelMatrixR0 :
			case VertexAttributeType::ModelMatrixR1 :
			case VertexAttributeType::ModelMatrixR2 :
			case VertexAttributeType::ModelMatrixR3 :
				return ModelMatrixString;

			case VertexAttributeType::NormalModelMatrixR0 :
			case VertexAttributeType::NormalModelMatrixR1 :
			case VertexAttributeType::NormalModelMatrixR2 :
				return NormalModelMatrixString;

			case VertexAttributeType::ModelPosition :
				return ModelPositionString;

			case VertexAttributeType::ModelScaling :
				return ModelScalingString;
		}

		return nullptr;
	}

	VertexAttributeType
	to_VertexAttributeType (const std::string & value) noexcept
	{
		if ( value == PositionString )
		{
			return VertexAttributeType::Position;
		}

		if ( value == TangentString )
		{
			return VertexAttributeType::Tangent;
		}

		if ( value == BinormalString )
		{
			return VertexAttributeType::Binormal;
		}

		if ( value == NormalString )
		{
			return VertexAttributeType::Normal;
		}

		if ( value == VertexColorString )
		{
			return VertexAttributeType::VertexColor;
		}

		if ( value == Primary2DTextureCoordinatesString )
		{
			return VertexAttributeType::Primary2DTextureCoordinates;
		}

		if ( value == Primary3DTextureCoordinatesString )
		{
			return VertexAttributeType::Primary3DTextureCoordinates;
		}

		if ( value == Secondary2DTextureCoordinatesString )
		{
			return VertexAttributeType::Secondary2DTextureCoordinates;
		}

		if ( value == Secondary3DTextureCoordinatesString )
		{
			return VertexAttributeType::Secondary3DTextureCoordinates;
		}

		if ( value == ModelMatrixString )
		{
			return VertexAttributeType::ModelMatrixR0;
		}

		if ( value == NormalModelMatrixString )
		{
			return VertexAttributeType::NormalModelMatrixR0;
		}

		std::cerr << __PRETTY_FUNCTION__ << " : Unknown '" << value << "' type ! Returning 'Position' by default." << "\n";

		return VertexAttributeType::Position;
	}

	const char *
	to_cstring (CubemapFace value) noexcept
	{
		switch ( value )
		{
			case CubemapFace::PositiveX :
				return PositiveXString;

			case CubemapFace::NegativeX :
				return NegativeXString;

			case CubemapFace::PositiveY :
				return PositiveYString;

			case CubemapFace::NegativeY :
				return NegativeYString;

			case CubemapFace::PositiveZ :
				return PositiveZString;

			case CubemapFace::NegativeZ :
				return NegativeZString;
		}

		return nullptr;
	}

	CubemapFace
	to_CubemapFace (const std::string & value) noexcept
	{
		if ( value == PositiveXString )
		{
			return CubemapFace::PositiveX;
		}

		if ( value == NegativeXString )
		{
			return CubemapFace::NegativeX;
		}

		if ( value == PositiveYString )
		{
			return CubemapFace::PositiveY;
		}

		if ( value == NegativeYString )
		{
			return CubemapFace::NegativeY;
		}

		if ( value == PositiveZString )
		{
			return CubemapFace::PositiveZ;
		}

		if ( value == NegativeZString )
		{
			return CubemapFace::NegativeZ;
		}

		std::cerr << __PRETTY_FUNCTION__ << " : Unknown '" << value << "' type ! Returning 'PositiveX' by default." << "\n";

		return CubemapFace::PositiveX;
	}

	const char *
	to_cstring (FilterQuality value) noexcept
	{
		switch ( value )
		{
			case FilterQuality::None :
				return NoneString;

			case FilterQuality::Custom :
				return CustomString;

			case FilterQuality::Linear :
				return LinearString;

			case FilterQuality::Bilinear :
				return BilinearString;

			case FilterQuality::Trilinear :
				return TrilinearString;
		}

		return nullptr;
	}

	FilterQuality
	to_FilterQuality (const std::string & value) noexcept
	{
		if ( value == NoneString )
		{
			return FilterQuality::None;
		}

		if ( value == CustomString )
		{
			return FilterQuality::Custom;
		}

		if ( value == LinearString )
		{
			return FilterQuality::Linear;
		}

		if ( value == BilinearString )
		{
			return FilterQuality::Bilinear;
		}

		if ( value == TrilinearString )
		{
			return FilterQuality::Trilinear;
		}

		std::cerr << __PRETTY_FUNCTION__ << " : Unknown '" << value << "' type ! Returning 'None' by default." << "\n";

		return FilterQuality::None;
	}

	const char *
	to_cstring (ShapeType value) noexcept
	{
		switch ( value )
		{
			case ShapeType::Custom :
				return CustomString;

			case ShapeType::Triangle :
				return TriangleString;

			case ShapeType::Quad :
				return QuadString;

			case ShapeType::Cube :
				return CubeString;

			case ShapeType::Sphere :
				return SphereString;

			case ShapeType::GeodesicSphere :
				return GeodesicSphereString;

			case ShapeType::Cylinder :
				return CylinderString;

			case ShapeType::Cone :
				return ConeString;

			case ShapeType::Disk :
				return DiskString;

			case ShapeType::Torus :
				return TorusString;

			case ShapeType::Tetrahedron :
				return TetrahedronString;

			case ShapeType::Hexahedron :
				return HexahedronString;

			case ShapeType::Octahedron :
				return OctahedronString;

			case ShapeType::Dodecahedron :
				return DodecahedronString;

			case ShapeType::Icosahedron :
				return IcosahedronString;

			case ShapeType::Arrow :
				return ArrowString;

			case ShapeType::Axis :
				return AxisString;
		}

		return nullptr;
	}

	ShapeType
	to_ShapeType (const std::string & value) noexcept
	{
		if ( value == CustomString )
		{
			return ShapeType::Custom;
		}

		if ( value == TriangleString )
		{
			return ShapeType::Triangle;
		}

		if ( value == QuadString )
		{
			return ShapeType::Quad;
		}

		if ( value == CubeString )
		{
			return ShapeType::Cube;
		}

		if ( value == SphereString )
		{
			return ShapeType::Sphere;
		}

		if ( value == GeodesicSphereString )
		{
			return ShapeType::GeodesicSphere;
		}

		if ( value == CylinderString )
		{
			return ShapeType::Cylinder;
		}

		if ( value == ConeString )
		{
			return ShapeType::Cone;
		}

		if ( value == DiskString )
		{
			return ShapeType::Disk;
		}

		if ( value == TorusString )
		{
			return ShapeType::Torus;
		}

		if ( value == TetrahedronString )
		{
			return ShapeType::Tetrahedron;
		}

		if ( value == HexahedronString )
		{
			return ShapeType::Hexahedron;
		}

		if ( value == OctahedronString )
		{
			return ShapeType::Octahedron;
		}

		if ( value == DodecahedronString )
		{
			return ShapeType::Dodecahedron;
		}

		if ( value == IcosahedronString )
		{
			return ShapeType::Icosahedron;
		}

		if ( value == ArrowString )
		{
			return ShapeType::Arrow;
		}

		if ( value == AxisString )
		{
			return ShapeType::Axis;
		}

		std::cerr << __PRETTY_FUNCTION__ << " : Unknown '" << value << "' type ! Returning 'Cube' by default." << "\n";

		return ShapeType::Cube;
	}

	const char *
	to_cstring (LightType type) noexcept
	{
		switch ( type )
		{
			case LightType::Directional :
				return DirectionalLightString;

			case LightType::Point :
				return PointLightString;

			case LightType::Spot :
				return SpotLightString;

			default:
				return nullptr;
		}
	}

	const char *
	to_cstring (AntiAliasingType type) noexcept
	{
		switch ( type )
		{
			case AntiAliasingType::MSAA :
				return MSAAString;

			case AntiAliasingType::SSAA :
				return SSAAString;

			case AntiAliasingType::TXAA :
				return TXAAString;

			case AntiAliasingType::FXAA :
				return FXAAString;

			case AntiAliasingType::SMAA :
				return SMAAString;

			case AntiAliasingType::TAA :
				return TAAString;

			default:
				return nullptr;
		}
	}
}
