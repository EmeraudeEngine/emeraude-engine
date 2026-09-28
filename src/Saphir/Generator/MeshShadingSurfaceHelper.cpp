/*
 * src/Saphir/Generator/MeshShadingSurfaceHelper.cpp
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

#include "MeshShadingSurfaceHelper.hpp"

/* STL inclusions. */
#include <sstream>
#include <string>

/* Local inclusions. */
#include "Abstract.hpp"
#include "Graphics/Geometry/MeshShadingSurface.hpp"
#include "Graphics/Material/Interface.hpp"
#include "Graphics/RenderTarget/Abstract.hpp"
#include "Saphir/Code.hpp"
#include "Saphir/Keys.hpp"
#include "Saphir/MeshShader.hpp"
#include "Saphir/Program.hpp"
#include "Saphir/TaskShader.hpp"
#include "Tracer.hpp"

namespace EmEn::Saphir::Generator
{
	using namespace Keys;
	using Graphics::Geometry::MeshShadingSurface;

	namespace
	{
		constexpr auto TracerTag{"MeshShadingSurface"};

		/** @brief Quad size per metre of camera distance: ~4 px per quad at 1620p and a 70° lens. */
		constexpr auto MeshShadingSurfaceDetail{"0.004"};

		/**
		 * @brief The payload both stages declare, verbatim.
		 * @return std::string
		 */
		std::string
		payloadDeclaration ()
		{
			return std::string{
				"struct MeshSurfaceTile" "\n"
				"{" "\n"
				"\t" "vec2 origin;" "\n"
				"\t" "float quadSize;" "\n"
				"\t" "float skirtDepth;" "\n"
				"\t" "uint meshletsPerSide;" "\n"
				"\t" "uint quadsPerMeshlet;" "\n"
				"\t" "uint skirtEdges;" "\n"
				"};" "\n"
				"taskPayloadSharedEXT MeshSurfaceTile "} + MeshSurfacePayload + ";";
		}
	}

	const char *
	meshSurfaceInstanceIndexExpression () noexcept
	{
		static const std::string expression = "floatBitsToUint(" + MatrixPC(PushConstant::Component::MeshSurfaceView) + ".w)";

		return expression.c_str();
	}

	bool
	declareMeshSurfaceCullingMatrix (const Abstract & generator, TaskShader & taskShader, std::string & expression) noexcept
	{
		expression.clear();

		const auto & renderTarget = generator.renderTarget();

		if ( renderTarget->isCubemap() || renderTarget->isCascadedShadowMap() )
		{
			return true;
		}

		/* The model matrix, as AbstractVertexStage synthesises it: the InstanceTransforms entry (stride 2) or the
		 * pushed one. */
		const auto & program = *generator.shaderProgram();
		std::string model;

		if ( program.wasInstanceTransformsEnabled() )
		{
			if ( !generator.declareInstanceTransformsBlock(taskShader) )
			{
				return false;
			}

			model = std::string{"ubInstanceTransforms.instanceMatrices["} + meshSurfaceInstanceIndexExpression() + " * 2]";
		}

		/* The branches of Abstract::declareMatrixPushConstantBlock() (no MDI, no instancing for a surface). */
		if ( program.wasAdvancedMatricesEnabled() )
		{
			if ( !generator.declareViewUniformBlock(taskShader) )
			{
				return false;
			}

			expression = ViewUB(UniformBlock::Component::ProjectionMatrix, false) + " * " + MatrixPC(PushConstant::Component::ViewMatrix) + " * " +
				(model.empty() ? MatrixPC(PushConstant::Component::ModelMatrix) : model);
		}
		else if ( !model.empty() )
		{
			expression = MatrixPC(PushConstant::Component::ViewProjectionMatrix) + " * " + model;
		}
		else
		{
			expression = MatrixPC(PushConstant::Component::ModelViewProjectionMatrix);
		}

		return true;
	}

	bool
	generateMeshShadingSurface (const Abstract & generator, const Graphics::Material::Interface & material, TaskShader & taskShader, MeshShader & meshShader, const std::string & cullingMatrix) noexcept
	{
		const std::string grid{MatrixPC(PushConstant::Component::MeshSurfaceGrid)};
		const std::string view{MatrixPC(PushConstant::Component::MeshSurfaceView)};
		const std::string handover{MaterialUB(UniformBlock::Component::ParallaxHandover)};
		const std::string heightScale{MaterialUB(UniformBlock::Component::HeightScale)};
		const std::string tile{MeshSurfacePayload};
		const auto meshletQuads = std::to_string(MeshShadingSurface::MeshletQuads);
		const auto maxSubdivision = std::to_string(MeshShadingSurface::MaxTileSubdivision);
		/* A DETAIL surface over a terrain (engine item mesh-shading-surface-on-heightfield): the tiles stand on the
		 * terrain's height clipmap (declareHeightfieldSurface() was called on both stages), one tile per terrain cell. */
		const bool heightfieldBase = generator.isHeightfieldSurfaceEnabled();

		taskShader.setTaskPayload(payloadDeclaration());
		meshShader.setTaskPayload(payloadDeclaration());

		/* The task stage reads the handover band (material UBO). */
		if ( !generator.declareMaterialUniformBlock(material, taskShader, 0) )
		{
			return false;
		}

		/* ---- TASK: one workgroup per tile. ----
		 * A tile's subdivision is a pure function of its box and the camera: a tile evaluates its four NEIGHBOURS with the
		 * same functions, to hang skirts only on the edges it shares with a COARSER neighbour (the only edges that can
		 * crack: between two tiles of the same subdivision the edge vertices are computed by the same formula). Skirts
		 * on every edge used to show as bright one-pixel lines along the terrain's convex folds: the skirt's top ties in
		 * depth with the fold, and its vertical strip has degenerate UV derivatives (engine item
		 * mesh-shading-surface-on-heightfield, 2026-09-28). */
		{
			/* The tile's base height range (x = bottom, y = top): the flat plane y = 0, or the terrain cell's four lattice
			 * heights (the drawn surface is planar between them, its extremes are at the corners). */
			Declaration::Function heightRange{"msTileHeightRange", GLSL::FloatVector2};
			heightRange.addInParameter(GLSL::FloatVector2, "origin");

			if ( heightfieldBase )
			{
				Code{heightRange, Location::Output} <<
					"const float h00 = hfHeight(origin, 0);" << Line::End <<
					"const float h10 = hfHeight(origin + vec2(" << grid << ".z, 0.0), 0);" << Line::End <<
					"const float h01 = hfHeight(origin + vec2(0.0, " << grid << ".z), 0);" << Line::End <<
					"const float h11 = hfHeight(origin + vec2(" << grid << ".z), 0);" << Line::End <<
					"return vec2(min(min(h00, h10), min(h01, h11)), max(max(h00, h10), max(h01, h11)));";
			}
			else
			{
				Code{heightRange, Location::Output} << "return vec2(0.0);";
			}

			/* The distance from the camera to the nearest point of the tile's box. */
			Declaration::Function distance{"msTileDistance", GLSL::Float};
			distance.addInParameter(GLSL::FloatVector2, "origin");
			distance.addInParameter(GLSL::FloatVector2, "range");
			Code{distance, Location::Output} <<
				"const vec2 nearest = clamp(" << view << ".xz, origin, origin + vec2(" << grid << ".z));" << Line::End <<
				"return length(vec3(nearest.x, clamp(" << view << ".y, range.x, range.y), nearest.y) - " << view << ".xyz);";

			/* Geometry share of the relief at a distance: 1 − smoothstep over the handover band, 1 without one. */
			Declaration::Function share{"msGeometryShare", GLSL::Float};
			share.addInParameter(GLSL::Float, "tileDistance");
			Code{share, Location::Output} <<
				"return " << handover << ".y > " << handover << ".x ? 1.0 - smoothstep(" << handover << ".x, " << handover << ".y, tileDistance) : 1.0;";

			/* Quads per tile side at a distance: a power of two, one beyond the handover band. */
			Declaration::Function subdivision{"msTileSubdivision", GLSL::UnsignedInteger};
			subdivision.addInParameter(GLSL::Float, "tileDistance");
			Code{subdivision, Location::Output} <<
				"if ( msGeometryShare(tileDistance) <= 0.0 ) { return 1u; }" << Line::End <<
				"const float targetQuad = max(tileDistance, 0.05) * " << MeshShadingSurfaceDetail << ";" << Line::End <<
				"return uint(clamp(exp2(ceil(log2(" << grid << ".z / targetQuad))), 1.0, " << maxSubdivision << ".0));";

			/* The subdivision of the tile at an origin (a neighbour). */
			Declaration::Function neighbour{"msNeighbourSubdivision", GLSL::UnsignedInteger};
			neighbour.addInParameter(GLSL::FloatVector2, "origin");
			Code{neighbour, Location::Output} <<
				"return msTileSubdivision(msTileDistance(origin, msTileHeightRange(origin)));";

			if ( !taskShader.declare(heightRange) || !taskShader.declare(distance) || !taskShader.declare(share) || !taskShader.declare(subdivision) || !taskShader.declare(neighbour) )
			{
				return false;
			}
		}

		Code{taskShader} <<
			"const vec2 tileOrigin = " << grid << ".xy + vec2(gl_WorkGroupID.xy) * " << grid << ".z;" << Line::End <<
			"const vec2 tileRange = msTileHeightRange(tileOrigin);" << Line::End <<
			"const float tileBottom = tileRange.x;" << Line::End <<
			"const float tileTop = tileRange.y;" << Line::End;

		/* Frustum culling: the tile's box, from its base surface down to the deepest relief, against the four side
		 * planes and the w > 0 half-space of the clip volume. Never the near and far planes: the depth convention
		 * and the shadow pass's depth clamp make them unreliable, and they cut little on a ground. A tile is kept
		 * unless ALL its corners are out on the SAME side: conservative, a kept tile may still be invisible.
		 * ⚠️ A culled tile still reaches the ONE EmitMeshTasksEXT at the end, with a zero count: no early exit in a
		 * branch, which the specification allows but leaves the whole workgroup's termination to the driver. */
		Code{taskShader} << "bool tileCulled = false;" << Line::End;

		if ( !cullingMatrix.empty() )
		{
			Code{taskShader} <<
				"{" << Line::End <<
				"	const mat4 cullMatrix = " << cullingMatrix << ";" << Line::End <<
				"	const float tileDepth = " << heightScale << " / " << grid << ".w;" << Line::End <<
				"	uint outside = 0x1Fu;" << Line::End <<
				"	for ( uint corner = 0u; corner < 8u; ++corner ) {" << Line::End <<
				"		const vec3 p = vec3(tileOrigin.x + ((corner & 1u) != 0u ? " << grid << ".z : 0.0), (corner & 4u) != 0u ? tileBottom - tileDepth : tileTop, tileOrigin.y + ((corner & 2u) != 0u ? " << grid << ".z : 0.0));" << Line::End <<
				"		const vec4 c = cullMatrix * vec4(p, 1.0);" << Line::End <<
				"		outside &= (c.x < -c.w ? 1u : 0u) | (c.x > c.w ? 2u : 0u) | (c.y < -c.w ? 4u : 0u) | (c.y > c.w ? 8u : 0u) | (c.w <= 0.0 ? 16u : 0u);" << Line::End <<
				"	}" << Line::End <<
				"	tileCulled = outside != 0u;" << Line::End <<
				"}" << Line::End;
		}

		Code{taskShader} <<
			"const float tileDistance = msTileDistance(tileOrigin, tileRange);" << Line::End <<
			"const float geometryShare = msGeometryShare(tileDistance);" << Line::End <<
			"const uint subdivision = msTileSubdivision(tileDistance);" << Line::End <<
			"const uint quadsPerMeshlet = min(subdivision, " << meshletQuads << "u);" << Line::End <<
			"const uint meshletsPerSide = subdivision / quadsPerMeshlet;" << Line::End <<
			"if ( gl_LocalInvocationIndex == 0u ) {" << Line::End <<
			"	/* Skirts only toward a COARSER neighbour (W, E, N, S bits, as msEdges()). */" << Line::End <<
			"	uint skirtEdges = 0u;" << Line::End <<
			"	if ( !tileCulled && geometryShare > 0.0 && subdivision > 1u ) {" << Line::End <<
			"		const float g = " << grid << ".z;" << Line::End <<
			"		skirtEdges |= msNeighbourSubdivision(tileOrigin - vec2(g, 0.0)) < subdivision ? 1u : 0u;" << Line::End <<
			"		skirtEdges |= msNeighbourSubdivision(tileOrigin + vec2(g, 0.0)) < subdivision ? 2u : 0u;" << Line::End <<
			"		skirtEdges |= msNeighbourSubdivision(tileOrigin - vec2(0.0, g)) < subdivision ? 4u : 0u;" << Line::End <<
			"		skirtEdges |= msNeighbourSubdivision(tileOrigin + vec2(0.0, g)) < subdivision ? 8u : 0u;" << Line::End <<
			"	}" << Line::End <<
			"	" << tile << ".origin = tileOrigin;" << Line::End <<
			"	" << tile << ".quadSize = " << grid << ".z / float(subdivision);" << Line::End <<
			"	/* The deepest relief at this tile, in metres: the skirts go down to it (none when the tile is flat). */" << Line::End <<
			"	" << tile << ".skirtDepth = geometryShare > 0.0 ? " << heightScale << " / " << grid << ".w * geometryShare : 0.0;" << Line::End <<
			"	" << tile << ".meshletsPerSide = meshletsPerSide;" << Line::End <<
			"	" << tile << ".quadsPerMeshlet = quadsPerMeshlet;" << Line::End <<
			"	" << tile << ".skirtEdges = skirtEdges;" << Line::End <<
			"}" << Line::End <<
			"barrier();" << Line::End <<
			"EmitMeshTasksEXT(tileCulled ? 0u : meshletsPerSide * meshletsPerSide, 1u, 1u);";

		/* ---- MESH: the material's displacement, then the vertex and primitive sources. ---- */
		std::string displacement;

		if ( !material.generateSurfaceDisplacementCode(generator, meshShader, "msUV", "msUVStep", "msMetresPerUV", "msDistance", "msDepth", displacement) )
		{
			displacement = "const float msDepth = 0.0;";
		}

		/* The meshlet: its place in the tile, the skirt edges it owns (W, E, N, S bits) and its counts. */
		std::stringstream counts;
		counts <<
			"((" << tile << ".quadsPerMeshlet + 1u) * (" << tile << ".quadsPerMeshlet + 1u) + bitCount(msEdges(gl_WorkGroupID.x)) * (" << tile << ".quadsPerMeshlet + 1u))";

		Declaration::Function edges{"msEdges", GLSL::UnsignedInteger};
		edges.addInParameter(GLSL::UnsignedInteger, "meshletIndex");
		Code{edges, Location::Output} <<
			"if ( " << tile << ".skirtDepth <= 0.0 ) { return 0u; }" << Line::End <<
			"const uint mx = meshletIndex % " << tile << ".meshletsPerSide;" << Line::End <<
			"const uint mz = meshletIndex / " << tile << ".meshletsPerSide;" << Line::End <<
			"const uint last = " << tile << ".meshletsPerSide - 1u;" << Line::End <<
			"return ((mx == 0u ? 1u : 0u) | (mx == last ? 2u : 0u) | (mz == 0u ? 4u : 0u) | (mz == last ? 8u : 0u)) & " << tile << ".skirtEdges;";

		/* The n-th set edge bit, and the grid index of the k-th vertex along an edge. */
		Declaration::Function nthEdge{"msNthEdge", GLSL::UnsignedInteger};
		nthEdge.addInParameter(GLSL::UnsignedInteger, "mask");
		nthEdge.addInParameter(GLSL::UnsignedInteger, "n");
		Code{nthEdge, Location::Output} <<
			"uint seen = 0u;" << Line::End <<
			"for ( uint bit = 0u; bit < 4u; ++bit ) {" << Line::End <<
			"	if ( (mask & (1u << bit)) != 0u ) { if ( seen == n ) { return bit; } ++seen; }" << Line::End <<
			"}" << Line::End <<
			"return 0u;";

		Declaration::Function edgeVertex{"msEdgeVertex", GLSL::UnsignedInteger};
		edgeVertex.addInParameter(GLSL::UnsignedInteger, "edge");
		edgeVertex.addInParameter(GLSL::UnsignedInteger, "k");
		edgeVertex.addInParameter(GLSL::UnsignedInteger, "q");
		Code{edgeVertex, Location::Output} <<
			"/* W: (0, k), E: (q, k), N: (k, 0), S: (k, q) in the (q + 1)² lattice. */" << Line::End <<
			"return edge == 0u ? k * (q + 1u) : (edge == 1u ? k * (q + 1u) + q : (edge == 2u ? k : q * (q + 1u) + k));";

		if ( !meshShader.declare(edges) || !meshShader.declare(nthEdge) || !meshShader.declare(edgeVertex) )
		{
			return false;
		}

		/* The terrain's surface between lattice points, EXACTLY as the CDLOD patch and the ray-tracing proxy draw it:
		 * two planar triangles per cell, split along (x, z)–(x + 1, z + 1). A bilinear read would part from both by up
		 * to |h00 + h11 − h10 − h01| / 4 — metres on a rough relief, and a traced surface above the drawn one
		 * blackens the ray-traced effects (docs/caution-points.md § CDLOD terrain). */
		if ( heightfieldBase )
		{
			Declaration::Function baseHeight{"msBaseHeight", GLSL::Float};
			baseHeight.addInParameter(GLSL::FloatVector2, "xz");
			Code{baseHeight, Location::Output} <<
				"const float cell = hfSurface.grid.x;" << Line::End <<
				"const vec2 lattice = floor(xz / cell);" << Line::End <<
				"const vec2 f = (xz / cell) - lattice;" << Line::End <<
				"const vec2 corner = lattice * cell;" << Line::End <<
				"const float h00 = hfHeight(corner, 0);" << Line::End <<
				"const float h11 = hfHeight(corner + vec2(cell), 0);" << Line::End <<
				"if ( f.x >= f.y ) { const float h10 = hfHeight(corner + vec2(cell, 0.0), 0); return h00 + f.x * (h10 - h00) + f.y * (h11 - h10); }" << Line::End <<
				"const float h01 = hfHeight(corner + vec2(0.0, cell), 0);" << Line::End <<
				"return h00 + f.y * (h01 - h00) + f.x * (h11 - h01);";

			if ( !meshShader.declare(baseHeight) )
			{
				return false;
			}
		}

		std::stringstream prologue;
		prologue <<
			"\t" "const uint q = " << tile << ".quadsPerMeshlet;" "\n"
			"\t" "const uint gridVertices = (q + 1u) * (q + 1u);" "\n"
			"\t" "const uint mx = gl_WorkGroupID.x % " << tile << ".meshletsPerSide;" "\n"
			"\t" "const uint mz = gl_WorkGroupID.x / " << tile << ".meshletsPerSide;" "\n"
			"\t" "vec2 lattice;" "\n"
			"\t" "bool skirt = false;" "\n"
			"\t" "if ( " << MeshShader::VertexIndex << " < gridVertices ) {" "\n"
			"\t\t" "lattice = vec2(float(" << MeshShader::VertexIndex << " % (q + 1u)), float(" << MeshShader::VertexIndex << " / (q + 1u)));" "\n"
			"\t" "} else {" "\n"
			"\t\t" "const uint s = " << MeshShader::VertexIndex << " - gridVertices;" "\n"
			"\t\t" "const uint edge = msNthEdge(msEdges(gl_WorkGroupID.x), s / (q + 1u));" "\n"
			"\t\t" "const uint k = s % (q + 1u);" "\n"
			"\t\t" "const uint index = msEdgeVertex(edge, k, q);" "\n"
			"\t\t" "lattice = vec2(float(index % (q + 1u)), float(index / (q + 1u)));" "\n"
			"\t\t" "skirt = true;" "\n"
			"\t" "}" "\n"
			"\t" "const vec2 msXZ = " << tile << ".origin + (vec2(float(mx), float(mz)) * float(q) + lattice) * " << tile << ".quadSize;" "\n";

		if ( heightfieldBase )
		{
			/* On the terrain: the CDLOD's own UV (hfSurface.textureCoordinates: U, V per metre and at the origin) so the
			 * texture does not jump at the window's border, the relief pushed along the terrain's normal, and the
			 * CDLOD's frame (T = dP/du along +X, B = N × T). `hfPosition` feeds the per-pixel frame outputs. */
			prologue <<
				"\t" "const float msBase = msBaseHeight(msXZ);" "\n"
				"\t" "const vec3 msBaseNormal = normalize(hfNormalAt(msXZ, 0));" "\n"
				"\t" "const vec2 msUV = (msXZ * hfSurface.textureCoordinates.xy) + hfSurface.textureCoordinates.zw;" "\n"
				"\t" "const float msUVStep = " << tile << ".quadSize * hfSurface.textureCoordinates.x;" "\n"
				"\t" "const float msMetresPerUV = 1.0 / hfSurface.textureCoordinates.x;" "\n"
				"\t" "const float msDistance = length(vec3(msXZ.x, msBase, msXZ.y) - " << view << ".xyz);" "\n"
				"\t" << displacement << "\n"
				"\t" "const vec3 hfPosition = vec3(msXZ.x, msBase, msXZ.y);" "\n"
				"\t" "const vec3 " << Attribute::Position << " = skirt ? vec3(msXZ.x, msBase - " << tile << ".skirtDepth, msXZ.y) : hfPosition - (msBaseNormal * msDepth);" "\n"
				"\t" "const vec3 " << Attribute::Normal << " = msBaseNormal;" "\n"
				"\t" "const vec3 " << Attribute::Tangent << " = normalize(vec3(msBaseNormal.y, -msBaseNormal.x, 0.0));" "\n"
				"\t" "const vec3 " << Attribute::Binormal << " = cross(msBaseNormal, " << Attribute::Tangent << ");" "\n"
				"\t" "const vec2 " << Attribute::Primary2DTextureCoordinates << " = msUV;" "\n";
		}
		else
		{
			prologue <<
				"\t" "/* The flat grid's own UV and frame (VertexGridResource): u along +X, v along +Z, T = +X, N = +Y, B = N × T = −Z. */" "\n"
				"\t" "const vec2 msUV = (msXZ - " << grid << ".xy) * " << grid << ".w;" "\n"
				"\t" "const float msUVStep = " << tile << ".quadSize * " << grid << ".w;" "\n"
				"\t" "const float msMetresPerUV = 1.0 / " << grid << ".w;" "\n"
				"\t" "const float msDistance = length(vec3(msXZ.x, 0.0, msXZ.y) - " << view << ".xyz);" "\n"
				"\t" << displacement << "\n"
				"\t" "const vec3 " << Attribute::Position << " = vec3(msXZ.x, skirt ? -" << tile << ".skirtDepth : -msDepth, msXZ.y);" "\n"
				"\t" "const vec3 " << Attribute::Normal << " = vec3(0.0, 1.0, 0.0);" "\n"
				"\t" "const vec3 " << Attribute::Tangent << " = vec3(1.0, 0.0, 0.0);" "\n"
				"\t" "const vec3 " << Attribute::Binormal << " = vec3(0.0, 0.0, -1.0);" "\n"
				"\t" "const vec2 " << Attribute::Primary2DTextureCoordinates << " = msUV;" "\n";
		}

		meshShader.setVertexSource(counts.str(), prologue.str(), {
			Graphics::VertexAttributeType::Position,
			Graphics::VertexAttributeType::Normal,
			Graphics::VertexAttributeType::Tangent,
			Graphics::VertexAttributeType::Binormal,
			Graphics::VertexAttributeType::Primary2DTextureCoordinates
		});

		std::stringstream primitiveCount;
		primitiveCount << "(2u * " << tile << ".quadsPerMeshlet * " << tile << ".quadsPerMeshlet + bitCount(msEdges(gl_WorkGroupID.x)) * 4u * " << tile << ".quadsPerMeshlet)";

		std::stringstream primitives;
		primitives <<
			"\t\t" "const uint q = " << tile << ".quadsPerMeshlet;" "\n"
			"\t\t" "const uint gridPrimitives = 2u * q * q;" "\n"
			"\t\t" "if ( " << MeshShader::PrimitiveIndex << " < gridPrimitives ) {" "\n"
			"\t\t\t" "const uint quad = " << MeshShader::PrimitiveIndex << " / 2u;" "\n"
			"\t\t\t" "const uint TL = (quad / q) * (q + 1u) + quad % q;" "\n"
			"\t\t\t" "const uint BL = TL + q + 1u;" "\n" <<
			(heightfieldBase
				/* On a terrain: the CDLOD's diagonal (TL-BR), same winding — the surface msBaseHeight() and the ray-tracing
				 * proxy describe. */
				? "\t\t\t" "gl_PrimitiveTriangleIndicesEXT[" + std::string{MeshShader::PrimitiveIndex} + "] = (" + MeshShader::PrimitiveIndex + " & 1u) == 0u ? uvec3(TL, BL, BL + 1u) : uvec3(TL, BL + 1u, TL + 1u);" "\n"
				/* The flat grid's winding: its strip rows give (TL, BL, TR) and (TR, BL, BR). */
				: "\t\t\t" "gl_PrimitiveTriangleIndicesEXT[" + std::string{MeshShader::PrimitiveIndex} + "] = (" + MeshShader::PrimitiveIndex + " & 1u) == 0u ? uvec3(TL, BL, TL + 1u) : uvec3(TL + 1u, BL, BL + 1u);" "\n") <<
			"\t\t" "} else {" "\n"
			"\t\t\t" "/* Skirts: per edge segment, two triangles in each winding (visible from both sides of the crack). */" "\n"
			"\t\t\t" "const uint s = " << MeshShader::PrimitiveIndex << " - gridPrimitives;" "\n"
			"\t\t\t" "const uint slot = s / (4u * q);" "\n"
			"\t\t\t" "const uint k = (s % (4u * q)) / 4u;" "\n"
			"\t\t\t" "const uint corner = s % 4u;" "\n"
			"\t\t\t" "const uint edge = msNthEdge(msEdges(gl_WorkGroupID.x), slot);" "\n"
			"\t\t\t" "const uint top0 = msEdgeVertex(edge, k, q);" "\n"
			"\t\t\t" "const uint top1 = msEdgeVertex(edge, k + 1u, q);" "\n"
			"\t\t\t" "const uint bottom0 = (q + 1u) * (q + 1u) + slot * (q + 1u) + k;" "\n"
			"\t\t\t" "const uint bottom1 = bottom0 + 1u;" "\n"
			"\t\t\t" "gl_PrimitiveTriangleIndicesEXT[" << MeshShader::PrimitiveIndex << "] = corner == 0u ? uvec3(top0, bottom0, top1) : (corner == 1u ? uvec3(top1, bottom0, bottom1) : (corner == 2u ? uvec3(top0, top1, bottom0) : uvec3(top1, bottom1, bottom0)));" "\n"
			"\t\t" "}" "\n";

		meshShader.setPrimitiveSource(primitiveCount.str(), primitives.str());

		TraceDebug{TracerTag} << "Mesh-shading surface stages generated for material '" << material.name() << "'.";

		return true;
	}
}
