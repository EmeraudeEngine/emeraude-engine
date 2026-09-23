/*
 * src/Saphir/Generator/SceneRendering.cpp
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

#include "SceneRendering.hpp"

/* Local inclusions. */
#include "MeshShadingSurfaceHelper.hpp"
#include "Graphics/Geometry/MeshShadingSurface.hpp"

/* Local inclusions. */
#include "Graphics/BindlessTextureManager.hpp"
#include "Graphics/Material/Interface.hpp"
#include "Graphics/RenderTarget/Abstract.hpp"
#include "Graphics/Renderer.hpp"
#include "Hash/FNV1a.hpp"
#include "HeightfieldSurfaceHelper.hpp"
#include "Saphir/Code.hpp"
#include "Scenes/Scene.hpp"
#include "SkinningLayoutHelper.hpp"
#include "Vulkan/Framebuffer.hpp"
#include "Vulkan/RenderPass.hpp"

namespace EmEn::Saphir::Generator
{
	using namespace Base;
	using namespace Graphics;
	using namespace Vulkan;
	using namespace Saphir::Keys;

	void
	SceneRendering::prepareUniformSets (SetIndexes & setIndexes) noexcept
	{
		setIndexes.enableSet(SetType::PerView);

		/* Scene instance transforms SSBO (non-instanced model matrices, classic AND
		 * advanced). This decision seals the pipeline layout; the CPU binding follows
		 * setIndexes (pipeline layout) while the matrix source follows
		 * Program::wasInstanceTransformsEnabled() (shader code) — keep both conditions
		 * separate (see src/Saphir/AGENTS.md § "InstanceTransforms SSBO Path"). */
		if ( this->useInstanceTransformsSet() )
		{
			setIndexes.enableSet(SetType::PerSceneTransforms);
		}

		if ( this->isFlagEnabled(IsLightingEnabled) )
		{
			switch ( m_renderPassType )
			{
				case RenderPassType::DirectionalLightPass :
				case RenderPassType::DirectionalLightPassShadowMap :
				case RenderPassType::DirectionalLightPassCSM :
				case RenderPassType::DirectionalLightPassColorMap :
				case RenderPassType::DirectionalLightPassFull :
				case RenderPassType::DirectionalLightPassFullCSM :
				case RenderPassType::PointLightPass :
				case RenderPassType::PointLightPassShadowMap :
				case RenderPassType::PointLightPassColorMap :
				case RenderPassType::PointLightPassFull :
				case RenderPassType::SpotLightPass :
				case RenderPassType::SpotLightPassShadowMap :
				case RenderPassType::SpotLightPassColorMap :
				case RenderPassType::SpotLightPassFull :
					setIndexes.enableSet(SetType::PerLight);
					break;

				default:
					break;
			}
		}

		/* The PerModel set is the skinning SSBO of a skeletal mesh, or the surface of a heightfield
		 * (clipmaps + uniforms) — never both, a heightfield is not skinned. */
		if ( this->isSkeletalAnimationEnabled() || this->isHeightfieldSurfaceEnabled() )
		{
			setIndexes.enableSet(SetType::PerModel);
		}

		if ( this->materialEnabled() )
		{
			setIndexes.enableSet(SetType::PerModelLayer);
		}

		/* Enable the bindless texture set when bindless textures are in use. */
		if ( this->bindlessTexturesEnabled() )
		{
			setIndexes.enableSet(SetType::PerBindless);
		}
	}

	bool
	SceneRendering::onGenerateShadersCode (Program & program) noexcept
	{
		/* Configure the light generator with the material for all shaders. */
		if ( this->isLightingRequested() )
		{
			if ( m_scene == nullptr )
			{
				Tracer::error(ClassId, "There is not scene to query light information from !");

				return false;
			}

			if ( this->materialEnabled() )
			{
				if ( !this->getMaterialInterface()->setupLightGenerator(m_lightGenerator) )
				{
					TraceError{ClassId} << "Unable to configure the light generator with material '" << this->getMaterialInterface()->name() << "' !";

					return false;
				}
			}

			/* The vegetation generator bakes an occlusion in the ALPHA of the vertex color. Declared
			 * AFTER the material so it composes with a material AO texture instead of racing it. */
			if ( this->isRenderableInstanceAvailable() )
			{
				const auto * renderable = this->getRenderable();

				if ( renderable != nullptr && renderable->hasVegetationWind() )
				{
					m_lightGenerator.declareVegetationBakedOcclusion(ShaderVariable::PrimaryVertexColor);
				}
			}
		}

		/* Generate the per-vertex stage: task + mesh for a mesh-shading surface on a capable device, the vertex
		 * shader otherwise (the same geometry then draws its flat grid). */
		if ( this->isMeshShadingSurfaceEnabled() )
		{
			if ( !this->generateMeshShadingStages(program) )
			{
				/* NOTE: Error message is generated by the function. */
				return false;
			}

			if ( this->debuggingEnabled() )
			{
				program.taskShader()->traceSuccessfulGeneration();
				program.meshShader()->traceSuccessfulGeneration();
			}
		}
		else
		{
			if ( !this->generateVertexShader(program) )
			{
				/* NOTE: Error message is generated by the function. */
				return false;
			}

			if ( this->debuggingEnabled() )
			{
				program.vertexShader()->traceSuccessfulGeneration();
			}
		}

		/* Generate the fragment shader stage. */
		if ( !this->generateFragmentShader(program) )
		{
			/* NOTE: Error message is generated by the function. */
			return false;
		}

		if ( this->debuggingEnabled() )
		{
			program.fragmentShader()->traceSuccessfulGeneration();;
		}

		return true;
	}

	bool
	SceneRendering::useInstanceTransformsSet () const noexcept
	{
		if ( this->isMultiDrawIndirectEnabled() )
		{
			return false;
		}

		if ( this->renderTarget()->isCubemap() )
		{
			return false;
		}

		/* NOTE: Instanced programs carry their model matrices in a VBO — they only need
		 * the set for the {viewProjection, previousViewProjection} HEADER, read by the
		 * velocity pass. Without a velocity attachment, no set. */
		if ( this->isFlagEnabled(IsInstancingEnabled) && !m_hasVelocityAttachment )
		{
			return false;
		}

		return m_scene != nullptr && m_scene->instanceTransforms().isInitialized();
	}

	bool
	SceneRendering::isLightingRequested () const noexcept
	{
		if ( m_scene == nullptr || !m_scene->lightSet().isEnabled() )
		{
			return false;
		}

		if ( !this->isFlagEnabled(IsLightingEnabled) )
		{
			return false;
		}

		/* The material-level veto: a material declaring itself UNLIT is never lit, whatever the
		 * instance asked for. Re-lighting content that already carries its own radiance
		 * double-counts it — the exact defect the unlit path exists to prevent. */
		if ( this->materialEnabled() && this->getMaterialInterface()->isUnlit() )
		{
			return false;
		}

		return true;
	}

	bool
	SceneRendering::isAdvancedRendering () const noexcept
	{
		if ( this->getMaterialInterface()->isComplex() )
		{
			return true;
		}

		switch ( m_renderPassType )
		{
			case RenderPassType::SimplePass :
				return false;

			case RenderPassType::DirectionalLightPass :
			case RenderPassType::DirectionalLightPassShadowMap :
			case RenderPassType::DirectionalLightPassCSM :
			case RenderPassType::DirectionalLightPassColorMap :
			case RenderPassType::DirectionalLightPassFull :
			case RenderPassType::DirectionalLightPassFullCSM :
			case RenderPassType::PointLightPass :
			case RenderPassType::PointLightPassShadowMap :
			case RenderPassType::PointLightPassColorMap :
			case RenderPassType::PointLightPassFull :
			case RenderPassType::SpotLightPass :
			case RenderPassType::SpotLightPassShadowMap :
			case RenderPassType::SpotLightPassColorMap :
			case RenderPassType::SpotLightPassFull :
				return true;

			case RenderPassType::AmbientPass :
				/* Advanced matrices are needed when:
				 * 1. MRT normal output requires view-space normals (normals attachment), OR
				 * 2. Normal mapping is active (light generator produces TBN code needing V*M).
				 * Both cases require separate V and M matrices to compute the normal matrix. */
				if ( this->isLightingRequested() )
				{
					return m_hasNormalsAttachment || m_lightGenerator.usesNormalMapping();
				}

				return false;

			default:
				return false;
		}
	}

	bool
	SceneRendering::onCreateDataLayouts (Renderer & renderer, const SetIndexes & setIndexes, StaticVector< std::shared_ptr< DescriptorSetLayout >, 6 > & descriptorSetLayouts, StaticVector< VkPushConstantRange, 4 > & pushConstantRanges) noexcept
	{
		/* Prepare the descriptor set layout for the scene instance transforms SSBO.
		 * NOTE: Must come right after the PerView layout (set index order). */
		if ( setIndexes.isSetEnabled(SetType::PerSceneTransforms) )
		{
			auto descriptorSetLayout = Scenes::SceneInstanceTransforms::getDescriptorSetLayout(renderer.layoutManager());

			if ( descriptorSetLayout == nullptr )
			{
				Tracer::error(ClassId, "Unable to get the instance transforms descriptor set layout !");

				return false;
			}

			descriptorSetLayouts.emplace_back(descriptorSetLayout);
		}

		if ( m_scene != nullptr && setIndexes.isSetEnabled(SetType::PerLight) )
		{
			/* NOTE: Use the unified layout (2 bindings: UBO + shadow sampler).
			 * Shadow behavior is controlled by the scUseShadow specialization constant.
			 * The Vulkan driver eliminates dead code when scUseShadow is false. */
			auto descriptorSetLayout = Scenes::LightSet::getDescriptorSetLayout(renderer.layoutManager());

			if ( descriptorSetLayout == nullptr )
			{
				Tracer::error(ClassId, "Unable to get the light descriptor set layout !");

				return false;
			}

			descriptorSetLayouts.emplace_back(descriptorSetLayout);
		}

		/* Prepare the descriptor set layout for skeletal animation bone matrices, or for a heightfield surface. */
		if ( setIndexes.isSetEnabled(SetType::PerModel) )
		{
			auto descriptorSetLayout = this->isHeightfieldSurfaceEnabled() ?
				getHeightfieldSurfaceDescriptorSetLayout(renderer.layoutManager()) :
				getSkinningDescriptorSetLayout(renderer.layoutManager());

			if ( descriptorSetLayout == nullptr )
			{
				Tracer::error(ClassId, "Unable to get the PerModel descriptor set layout (skinning SSBO or heightfield surface) !");

				return false;
			}

			descriptorSetLayouts.emplace_back(descriptorSetLayout);
		}

		/* Prepare the descriptor set layout for the model layer. */
		if ( this->materialEnabled() && setIndexes.isSetEnabled(SetType::PerModelLayer) )
		{
			auto descriptorSetLayout = this->getMaterialInterface()->descriptorSetLayout();

			if ( descriptorSetLayout == nullptr )
			{
				Tracer::error(ClassId, "Unable to get the material descriptor set layout !");

				return false;
			}

			descriptorSetLayouts.emplace_back(descriptorSetLayout);
		}

		/* Prepare the descriptor set layout for bindless textures. */
		if ( setIndexes.isSetEnabled(SetType::PerBindless) )
		{
			auto descriptorSetLayout = renderer.bindlessTextureManager().descriptorSetLayout();

			if ( descriptorSetLayout == nullptr )
			{
				Tracer::error(ClassId, "Unable to get the bindless textures descriptor set layout !");

				return false;
			}

			descriptorSetLayouts.emplace_back(descriptorSetLayout);
		}

		/* NOTE: The per-vertex stage owns the push constants: the vertex shader (VERTEX, unchanged), or the mesh
		 * shader of a mesh-shading program, whose task stage reads them too (MESH | TASK). */
		const auto & program = this->shaderProgram();
		Abstract::generatePushConstantRanges(program->perVertexStage()->pushConstantBlockDeclarations(), pushConstantRanges, program->hasMeshShader() ? program->perVertexStageFlags() : VK_SHADER_STAGE_VERTEX_BIT);

		return true;
	}

	bool
	SceneRendering::generateVertexShader (Program & program) noexcept
	{
		/* Create the vertex shader. */
		const bool isCubemapTarget = this->renderTarget()->isCubemap();

		auto * vertexShader = program.initVertexShader(
			this->name( ) + "VertexShader",
			this->isFlagEnabled(IsInstancingEnabled),
			this->isAdvancedRendering(),
			this->isFlagEnabled(IsRenderableFacingCamera),
			isCubemapTarget,
			this->isMultiDrawIndirectEnabled()
		);
		vertexShader->setExtensionBehavior("GL_ARB_separate_shader_objects", "enable");

		if ( !this->configurePerVertexStage(program, *vertexShader) )
		{
			return false;
		}

		return vertexShader->generateSourceCode(*this);
	}

	bool
	SceneRendering::generateMeshShadingStages (Program & program) noexcept
	{
		using Graphics::Geometry::MeshShadingSurface;

		/* The meshlet ceilings: 8 × 8 quads (81 vertices, 128 triangles) plus the skirts of up to four tile edges
		 * (4 × 9 vertices, 4 × 32 triangles in both windings) — 117 / 256, inside the 256 / 256 every
		 * VK_EXT_mesh_shader device guarantees. */
		constexpr uint32_t MaxVertices = MeshShadingSurface::MeshletVertices * MeshShadingSurface::MeshletVertices + 4 * MeshShadingSurface::MeshletVertices;
		constexpr uint32_t MaxPrimitives = 2 * MeshShadingSurface::MeshletQuads * MeshShadingSurface::MeshletQuads + 4 * 4 * MeshShadingSurface::MeshletQuads;
		constexpr std::array< uint32_t, 3 > Workgroup{MeshShadingSurface::WorkgroupSize, 1, 1};

		auto * meshShader = program.initMeshShader(this->name() + "MeshShader", MeshOutputTopology::Triangles, MaxVertices, MaxPrimitives, Workgroup);
		auto * taskShader = program.initTaskShader(this->name() + "TaskShader", Workgroup);

		if ( meshShader == nullptr || taskShader == nullptr )
		{
			Tracer::error(ClassId, "Unable to create the task and mesh stages of a mesh-shading surface !");

			return false;
		}

		/* The modes Program::initVertexShader() sets on a vertex shader, for the ones a surface can have (no
		 * instancing attributes, no billboard, no MDI): the advanced matrices (the lit passes' view-space frame),
		 * and the cubemap mode of a multiview target. */
		if ( this->isAdvancedRendering() )
		{
			meshShader->enableAdvancedMatrices();
		}

		if ( this->renderTarget()->isCubemap() )
		{
			meshShader->enableCubemapMode();
		}

		/* A mesh workgroup has no gl_InstanceIndex: the InstanceTransforms slot travels in the push constants. */
		meshShader->setInstanceIndexExpression(meshSurfaceInstanceIndexExpression());

		if ( !this->configurePerVertexStage(program, *meshShader) )
		{
			return false;
		}

		/* The task stage reads the same push-constant block (the tiling and the camera). */
		if ( !this->declareMatrixPushConstantBlock(*taskShader) )
		{
			return false;
		}

		/* The task stage culls the tiles against the camera's clip volume. */
		std::string cullingMatrix;

		if ( !declareMeshSurfaceCullingMatrix(*this, *taskShader, cullingMatrix) )
		{
			return false;
		}

		if ( !generateMeshShadingSurface(*this, *this->getMaterialInterface(), *taskShader, *meshShader, cullingMatrix) )
		{
			Tracer::error(ClassId, "Unable to generate the mesh-shading surface stages !");

			return false;
		}

		return taskShader->generateSourceCode(*this) && meshShader->generateSourceCode(*this);
	}

	bool
	SceneRendering::configurePerVertexStage (Program & program, AbstractVertexStage & vertexShader) noexcept
	{
		const bool isCubemapTarget = this->renderTarget()->isCubemap();

		/* Heightfield surface: switched on FIRST, the matrices push-constant block below appends the
		 * node and the camera for it. The fragment stage rebuilds the frame per pixel, so the vertex
		 * stage hands it the world XZ and its rotations. */
		if ( this->isHeightfieldSurfaceEnabled() )
		{
			vertexShader.enableHeightfieldSurface();
			vertexShader.enableHeightfieldPixelFrame();

			if ( !declareHeightfieldSurface(vertexShader, program.setIndexes().set(SetType::PerModel), false) )
			{
				return false;
			}
		}

		/* Instanced motion history: fixes the per-instance VBO stride (previous model matrix). */
		if ( this->isInstanceMotionHistoryEnabled() )
		{
			vertexShader.enableInstanceMotionHistory();
		}

		/* Infinity view (sky background): selects the previous INFINITY view-projection for the
		 * velocity outputs. Part of the generator flags, hence of the program cache key — two
		 * instances of the same renderable, one infinity-view and one not, MUST NOT share a
		 * program. */
		if ( this->isUsingInfinityView() )
		{
			vertexShader.enableInfinityView();
		}

		/* NOTE: Cubemap rendering requires multiview extension for gl_ViewIndex. */
		if ( isCubemapTarget )
		{
			vertexShader.setExtensionBehavior("GL_EXT_multiview", "enable");
		}

		/* Scene instance transforms SSBO: the model matrix source of the non-instanced
		 * path (classic AND advanced), and the {VP, previousVP} header source of the
		 * velocity pass (instanced programs included). The matrix-source flag MUST be set
		 * before declareMatrixPushConstantBlock() (it selects the VP/V-only push block)
		 * and NEVER applies to instanced programs (their matrices live in a VBO). */
		if ( program.setIndexes().isSetEnabled(SetType::PerSceneTransforms) )
		{
			if ( !this->isFlagEnabled(IsInstancingEnabled) )
			{
				vertexShader.enableInstanceTransforms();
			}

			/* The one declaration of the layout (Abstract::declareInstanceTransformsBlock()). */
			if ( !this->declareInstanceTransformsBlock(vertexShader) )
			{
				return false;
			}
		}

		if ( !this->declareMatrixPushConstantBlock(vertexShader) )
		{
			return false;
		}

		/* Declare the view uniform block. */
		if ( !this->declareViewUniformBlock(vertexShader) )
		{
			return false;
		}

		/* Vegetation wind: the vertex stage displaces by the per-frame wind state. It needs the
		 * instance-transforms SSBO, which carries that state in its header — without the instance
		 * transforms the block is not even declared, so the request is dropped rather than
		 * producing a shader that will not compile. */
		if ( this->isRenderableInstanceAvailable() )
		{
			const auto * renderable = this->getRenderable();

			if ( renderable != nullptr && renderable->hasVegetationWind() && program.setIndexes().isSetEnabled(SetType::PerSceneTransforms) )
			{
				vertexShader.enableVegetationWind();

				/* The leaf cards also flutter (weighted by their V, so the petiole stays on its twig). */
				if ( renderable->isVegetationFoliageLayer(this->layerIndex()) )
				{
					vertexShader.enableVegetationFlutter();
				}
			}
		}

		/* Skeletal animation: declare bone attributes and SSBO. */
		if ( this->isSkeletalAnimationEnabled() )
		{
			vertexShader.enableSkinning();

			vertexShader.declare(Declaration::InputAttribute{Graphics::VertexAttributeType::BoneInfluence});
			vertexShader.declare(Declaration::InputAttribute{Graphics::VertexAttributeType::BoneWeight});

			const auto setIndex = program.setIndexes().set(SetType::PerModel);

			Declaration::ShaderStorageBlock ssbo{setIndex, 0, Declaration::MemoryLayout::Std430, "SkinningMatrices", "ubSkinningMatrices"};
			ssbo.setAccessQualifier(Declaration::AccessQualifier::ReadOnly);
			ssbo.addMember(Declaration::VariableType::Matrix4, "bones[]");
			vertexShader.declare(ssbo);
		}

		/* NOTE: The position is always required and available. */
		if ( !vertexShader.requestSynthesizeInstruction(ShaderVariable::PositionScreenSpace) )
		{
			return false;
		}

		/* A vegetation renderable carries its baked occlusion in the vertex color ALPHA, which no
		 * texture-based material ever asks for, so the fragment stage would never see it.
		 * ⚠️⚠️ Requesting it here is safe ONLY because the material does not use vertex colors: a
		 * material that does multiplies its albedo by the WHOLE vertex color
		 * (StandardResource.cpp, SurfaceAlbedoFinal), which on a tree would tint every leaf by its
		 * bending weights. Never enable vertex colors on a vegetation material without revisiting
		 * that multiply. */
		if ( vertexShader.isVegetationWindEnabled() && !vertexShader.requestSynthesizeInstruction(ShaderVariable::PrimaryVertexColor, Saphir::VariableScope::ToNextStage) )
		{
			return false;
		}

		/* Velocity outputs (motion vectors): current and previous clip-space positions,
		 * turned into an NDC delta by the fragment shader. Requires the InstanceTransforms
		 * SSBO header (previousViewProjection). */
		m_velocityOutputsEmitted = false;

		if ( m_hasVelocityAttachment && program.setIndexes().isSetEnabled(SetType::PerSceneTransforms) )
		{
			if ( !vertexShader.synthesizeVelocityClipPositions(*this, m_velocityOutputsEmitted) )
			{
				return false;
			}
		}

		/* If present, generate the material shader code. */
		if ( this->materialEnabled() && !this->getMaterialInterface()->generateVertexShaderCode(*this, vertexShader) )
		{
			TraceError{ClassId} << "Unable to generate vertex shader code part for material '" << this->getMaterialInterface()->name() << "' !";

			return false;
		}

		/* Generate the lighting shader code. */
		if ( this->isLightingRequested() )
		{
			if ( !m_lightGenerator.generateVertexShaderCode(*this, vertexShader) )
			{
				Tracer::error(ClassId, "Unable to generate vertex shader code part for lighting !");

				return false;
			}

			/* Ensure svNormalViewSpace is available as a fragment shader input for the MRT normal output.
			 * Only needed when the render target has a normals attachment (color attachment count >= 2),
			 * i.e. the internal scene render target.  The swap chain has only 1 color attachment. */
			if ( m_hasNormalsAttachment )
			{
				if ( !vertexShader.requestSynthesizeInstruction(ShaderVariable::NormalViewSpace, Saphir::VariableScope::ToNextStage) )
				{
					return false;
				}
			}
		}

		return true;
	}

	bool
	SceneRendering::generateFragmentShader (Program & program) noexcept
	{
		/* Create the fragment shader. */
		auto * fragmentShader = program.initFragmentShader(this->name( ) + "FragmentShader");
		fragmentShader->setExtensionBehavior("GL_ARB_separate_shader_objects", "enable");

		/* A heightfield's frame is rebuilt per pixel: armed BEFORE the connection, which is where the
		 * interpolated frame variables are received under other names. */
		if ( program.perVertexStage()->isHeightfieldSurfaceEnabled() )
		{
			fragmentShader->enableHeightfieldPixelFrame();

			if ( !declareHeightfieldSurface(*fragmentShader, program.setIndexes().set(SetType::PerModel), true) )
			{
				return false;
			}
		}

		/* Automatic input declarations from vertex shader. */
		if ( program.hasMeshShader() ? !fragmentShader->connectFromPreviousShader(*program.meshShader()) : !fragmentShader->connectFromPreviousShader(*program.vertexShader()) )
		{
			return false;
		}

		/* Common part of fragment shader. */
		fragmentShader->declareDefaultOutputFragment();

		/* Declare the MRT normals output only when the render target has a normals attachment. */
		if ( m_hasNormalsAttachment )
		{
			fragmentShader->declare(Declaration::OutputFragment{1, Keys::GLSL::FloatVector4, Keys::ShaderVariable::OutputNormal});
		}

		/* Declare the MRT material properties output only when the render target has the attachment. */
		if ( m_hasMaterialPropertiesAttachment )
		{
			fragmentShader->declare(Declaration::OutputFragment{2, Keys::GLSL::FloatVector4, Keys::ShaderVariable::OutputMaterialProperties});
		}

		/* Declare the MRT albedo output only when the render target has the attachment. */
		if ( m_hasAlbedoAttachment )
		{
			fragmentShader->declare(Declaration::OutputFragment{3, Keys::GLSL::FloatVector4, Keys::ShaderVariable::OutputAlbedo});
		}

		/* Declare the MRT velocity output only when the render target has the attachment. */
		if ( m_hasVelocityAttachment )
		{
			fragmentShader->declare(Declaration::OutputFragment{4, Keys::GLSL::FloatVector2, Keys::ShaderVariable::OutputVelocity});
		}

		/* If a material is present, generate the shader code (optional). */
		if ( this->materialEnabled() && !this->getMaterialInterface()->generateFragmentShaderCode(*this, m_lightGenerator, *fragmentShader) )
		{
			TraceError{ClassId} << "Unable to generate fragment shader code part for material '" << this->getMaterialInterface()->name() << "' !";

			return false;
		}

		/* NOTE: Queried twice below (light generation, then fragment output); the
		 * underlying check chases scene/material state through several virtual calls,
		 * and neither call site can change what it would return, so compute it once. */
		const bool lightingRequested = this->isLightingRequested();

		/* If the light is enabled, generate the shader code (optional). */
		if ( lightingRequested )
		{
			/* Declare the view uniform block. */
			if ( !this->declareViewUniformBlock(*fragmentShader) )
			{
				return false;
			}

			if ( !m_lightGenerator.generateFragmentShaderCode(*this, *fragmentShader) )
			{
				Tracer::error(ClassId, "Unable to generate fragment shader code part for lighting !");

				return false;
			}
		}

		/* Generates the fragment output. */
		if ( lightingRequested )
		{
			Code{*fragmentShader, Location::Output} << ShaderVariable::OutputFragment << " = " << m_lightGenerator.fragmentColor() << ';';

			if ( m_hasNormalsAttachment )
			{
				/* Write view-space normal to MRT attachment 1 for post-process effects (SSAO, SSR, RTR, RTAO).
				 * Uses the final normal (perturbed by normal mapping when active) instead of the geometric normal.
				 * Alpha packs roughness + metalness: alpha = roughness + round(metalness) * 2.0.
				 * Decode: metalness = (alpha >= 2.0) ? 1.0 : 0.0; roughness = alpha - metalness * 2.0;
				 * ⚠️ The metalness MUST be quantized to {0,1} at WRITE time: the decode's >= 2
				 * threshold assumes it. A fractional metalness (real since the packed
				 * metallic-roughness source channels are honored) would otherwise corrupt BOTH
				 * decoded values — 0.93 metal / 0.4 rough packed raw decodes as 1.0 / 0.26.
				 * Only the ambient/simple pass writes the actual normal. Light passes use additive
				 * blending, so they must output zero to preserve the existing normal value. */
				if ( m_renderPassType == RenderPassType::AmbientPass || m_renderPassType == RenderPassType::SimplePass )
				{
					const auto normalExpr = m_lightGenerator.finalNormalViewSpaceExpression();
					const auto roughnessExpr = m_lightGenerator.roughnessShaderExpression();
					const auto metalnessExpr = m_lightGenerator.metalnessShaderExpression();

					/* GEOMETRIC SPECULAR ANTIALIASING (Kaplanyan et al., "Filtering Distributions of
					 * Normals"; Tokuyoshi & Kaplanyan, "Improved Geometric Specular Antialiasing" —
					 * the Filament constants). A surface whose normal varies WITHIN the pixel is
					 * rough at pixel scale, whatever its material says, so the normal's screen-space
					 * variance is folded into the roughness written to the G-buffer.
					 *
					 * ⚠️ This is the term Sponza's cypress was missing. The alpha mask's EDGE texels
					 * carry an AUTHORED near-tangent normal (measured mean Z +0.272 against +0.676 on
					 * the needle body), so NdotV collapses on every silhouette and every NdotV-driven
					 * term spikes there at once — the ray-traced reflection, the screen-space one, and
					 * the direct specular. Measured: 74 % of the residual rim pixels are shared
					 * between the two lanes, and 42 % of them survive with the reflections switched
					 * off entirely. One cause, three symptoms; widening the roughness is the only
					 * lever that reaches all three, because all three read it.
					 *
					 * ⚠️ Both lanes inherit this for FREE: RTR and SSR decode their roughness from
					 * this very attachment (alpha = roughness + round(metalness) * 2). Nothing in
					 * either effect needs to change.
					 *
					 * ⚠️ The variance is measured on the FINAL normal — the normal-mapped one that is
					 * actually written — never on the geometric normal, or the high-frequency detail
					 * the filter exists to tame is invisible to it.
					 *
					 * ⚠️ Perceptual roughness in, perceptual roughness out: the kernel is added in
					 * ALPHA-SQUARED space (alpha = perceptual²), hence the two square roots. Adding
					 * it to the perceptual value directly over-blurs smooth surfaces badly. */
					constexpr auto SpecularAAVariance = "0.25";
					constexpr auto SpecularAAThreshold = "0.18";

					Code{*fragmentShader, Location::Output} <<
						"const vec3 saaNormalDdx = dFdx(" << normalExpr << ");" << Line::End <<
						"const vec3 saaNormalDdy = dFdy(" << normalExpr << ");" << Line::End <<
						"const float saaVariance = " << SpecularAAVariance << " * (dot(saaNormalDdx, saaNormalDdx) + dot(saaNormalDdy, saaNormalDdy));" << Line::End <<
						"const float saaAlpha = clamp(" << roughnessExpr << ", 0.0, 1.0) * clamp(" << roughnessExpr << ", 0.0, 1.0);" << Line::End <<
						"const float saaFiltered = sqrt(sqrt(clamp(saaAlpha * saaAlpha + min(2.0 * saaVariance, " << SpecularAAThreshold << "), 0.0, 1.0)));";

					Code{*fragmentShader, Location::Output} << ShaderVariable::OutputNormal << " = vec4(" << normalExpr << ", saaFiltered + round(clamp(" << metalnessExpr << ", 0.0, 1.0)) * 2.0);";
				}
				else
				{
					Code{*fragmentShader, Location::Output} << ShaderVariable::OutputNormal << " = vec4(0.0);";
				}
			}

			/* ⚠️⚠️ The debug lane must be forced in EVERY pass, not just the ambient one: the light
			 * passes blend ADDITIVELY onto the frame, so an override written only in the ambient
			 * pass is buried under the lighting of every lit pixel. The first two attempts at this
			 * instrument came back showing the ordinary frame, and the numbers read off them —
			 * "the nibble is 0.195 on the leaves against 0.205 on the stone" — were the shaded
			 * frame divided by 255, not the lane. An instrument must be PROVEN on before anything
			 * is read from it. */
			if ( this->debugMaterialPropertiesLane() > 0
				&& m_renderPassType != RenderPassType::AmbientPass && m_renderPassType != RenderPassType::SimplePass )
			{
				Code{*fragmentShader, Location::Output} << m_lightGenerator.fragmentColor() << " = vec4(0.0, 0.0, 0.0, 1.0);";
			}

			if ( m_hasMaterialPropertiesAttachment )
			{
				/* Write material properties to MRT attachment 2 for post-process effect modulation.
				 * Nibble-packed RGBA: R[hi:lo]=reflection:reserved, G[hi:lo]=aoResponse:shadowResponse,
				 * B[hi:lo]=bloomContrib:emissiveMask, A[hi:lo]=fogResponse:dofMask.
				 * Values derived from declared surface properties (metalness, roughness, AO, emissive).
				 *
				 * RGB is preserved across light passes via additive blend (write 0, add 0).
				 * Alpha uses REPLACE blend (srcAlpha=ONE, dstAlpha=ZERO) in light passes, so we
				 * MUST write A=1.0 to preserve the AmbientPass-encoded fogResponse/dofMask nibbles;
				 * writing vec4(0.0) here would zero out alpha and break AtmosphericFog/DepthOfField
				 * material modulation in every lit pixel. */
				if ( m_renderPassType == RenderPassType::AmbientPass || m_renderPassType == RenderPassType::SimplePass )
				{
					if ( m_lightGenerator.useOpacity() )
					{
						/* A BLENDED material writes its packed nibbles only where it is at least half
						 * opaque: the alpha lane (otherwise always 1, read by nobody) becomes a SELECT bit
						 * and the attachment blends by SRC_ALPHA (onGraphicsPipelineConfiguration) — an
						 * exact per-fragment replace-or-keep, no interpolation of packed data, no
						 * discard (the colour attachment keeps blending). Sponza's 35 %-opaque dirt
						 * decals, metal by glTF default, used to REPLACE the wall's reflectivity nibble
						 * over their whole extent: RTR rendered every stain as a blurred mirror. */
						Code{*fragmentShader, Location::Output} << ShaderVariable::OutputMaterialProperties << " = vec4((" << m_lightGenerator.materialPropertiesExpression() << ").rgb, step(0.5, " << m_lightGenerator.fragmentColor() << ".a));";
					}
					else
					{
						Code{*fragmentShader, Location::Output} << ShaderVariable::OutputMaterialProperties << " = " << m_lightGenerator.materialPropertiesExpression() << ";";
					}

					/* ⚠️ THE instrument for "why is that surface reflecting": the packed lane,
					 * written to the FRAME in grey. Displaying a G-buffer lane as the frame colour
					 * settled the Sponza dirt-decal report in one capture where three A/Bs had
					 * circled it — see docs/caution-points.md § Material Properties. Off by default
					 * (Core/Graphics/DebugMaterialPropertiesLane = 0) and read once per program
					 * generation, so it costs one `if` at generation time and nothing at runtime. */
					if ( this->debugMaterialPropertiesLane() > 0 )
					{
						if ( this->debugMaterialPropertiesLane() == 3 )
						{
							/* ⚠️ Lane 3 is the NORMAL attachment, not a material-properties nibble, and
							 * it answers a different question: WHERE does the G-buffer think there is a
							 * surface, and with WHICH normal.
							 * ⚠️ It was built on the theory that a blended leaf card stamps its normal
							 * over the WHOLE quad. MEASUREMENT REFUTED THAT at close range: the blend
							 * floor already discards 93.6 % of Sponza's cypress mask, the silhouettes
							 * come out crisp, and the canopy still washed out. What the lane is
							 * actually useful for is the opposite end — the mask's EDGE texels, whose
							 * authored normal is near-tangent (mean Z +0.272 against +0.676 on the
							 * needle body), which is what spikes every NdotV-driven term at once. */
							Code{*fragmentShader, Location::Output} <<
								m_lightGenerator.fragmentColor() << " = vec4(" << ShaderVariable::OutputNormal << ".rgb * 0.5 + 0.5, " << m_lightGenerator.fragmentColor() << ".a);";
						}
						else
						{
							const auto lane = this->debugMaterialPropertiesLane() == 1 ? "r" : "g";

							Code{*fragmentShader, Location::Output} <<
								m_lightGenerator.fragmentColor() << " = vec4(vec3(" << ShaderVariable::OutputMaterialProperties << "." << lane << "), " << m_lightGenerator.fragmentColor() << ".a);";
						}
					}
				}
				else
				{
					Code{*fragmentShader, Location::Output} << ShaderVariable::OutputMaterialProperties << " = vec4(0.0, 0.0, 0.0, 1.0);";
				}
			}

			if ( m_hasAlbedoAttachment )
			{
				/* MRT attachment 3 has TWO kinds of reader, hence TWO lanes (Graphics/AGENTS.md
				 * § "The albedo G-buffer: BASE colour in RGB, DIFFUSE WEIGHT in ALPHA"):
				 *  - .rgb = the surface BASE colour — the Fresnel F0 of a metal, which the
				 *    reflections (RTR, SSR) tint their resolved reflection by;
				 *  - .a   = the DIFFUSE WEIGHT `(1 - metalness) * (1 - transmission)`, so the
				 *    indirect-diffuse combines (SSGI, RTGI) re-modulate their demodulated
				 *    IRRADIANCE by `rgb * a` — the energy the diffuse lobe actually receives
				 *    (LightGenerator::diffuseWeightShaderExpression()).
				 * ⚠️ Folding the weight INTO the rgb lanes (Aug 2026, one day) zeroed the F0 of
				 * every metal and killed their reflections in both RTR and SSR.
				 * A BLENDED material (opacity) writes `a = weight * opacity` and its albedo attachment
				 * is alpha-BLENDED by that lane (onGraphicsPipelineConfiguration): a layer contributes
				 * its diffuse albedo in proportion to how much of the pixel it covers. Sponza's dirt
				 * decals — BLEND quads, opacity 0.35, glTF-default metallicFactor 1 — used to REPLACE
				 * the wall's lanes over their whole quad, transparent texels included, with
				 * "metal, weight 0": the RTGI under every decal was zero (big dark squares, Aug 2026).
				 * Only the ambient/simple pass writes it; light passes have their write mask
				 * zeroed on the G-buffer attachments (see onGraphicsPipelineConfiguration). */
				if ( m_renderPassType == RenderPassType::AmbientPass || m_renderPassType == RenderPassType::SimplePass )
				{
					if ( m_lightGenerator.useOpacity() )
					{
						Code{*fragmentShader, Location::Output} << ShaderVariable::OutputAlbedo << " = vec4((" << m_lightGenerator.albedoShaderExpression() << ").rgb, (" << m_lightGenerator.diffuseWeightShaderExpression() << ") * " << m_lightGenerator.fragmentColor() << ".a);";
					}
					else
					{
						Code{*fragmentShader, Location::Output} << ShaderVariable::OutputAlbedo << " = vec4((" << m_lightGenerator.albedoShaderExpression() << ").rgb, " << m_lightGenerator.diffuseWeightShaderExpression() << ");";
					}
				}
				else
				{
					Code{*fragmentShader, Location::Output} << ShaderVariable::OutputAlbedo << " = vec4(0.0);";
				}
			}

			if ( m_hasVelocityAttachment )
			{
				/* Write the NDC-delta motion vector to MRT attachment 4 (temporal effects).
				 * NOTE: Jitter-free by construction — no matrix used to build the clip positions
				 * carries the TAA sub-pixel jitter (it is a per-draw push constant applied to
				 * gl_Position alone), so the plain NDC delta below needs no jitter terms.
				 * Light passes have their write mask zeroed (see onGraphicsPipelineConfiguration). */
				if ( m_velocityOutputsEmitted && (m_renderPassType == RenderPassType::AmbientPass || m_renderPassType == RenderPassType::SimplePass) )
				{
					Code{*fragmentShader, Location::Output} << ShaderVariable::OutputVelocity << " = (" << ShaderVariable::ClipPositionCurrent << ".xy / " << ShaderVariable::ClipPositionCurrent << ".w) - (" << ShaderVariable::ClipPositionPrevious << ".xy / " << ShaderVariable::ClipPositionPrevious << ".w);";
				}
				else
				{
					Code{*fragmentShader, Location::Output} << ShaderVariable::OutputVelocity << " = vec2(0.0);";
				}
			}
		}
		else if ( this->materialEnabled() )
		{
			/* UNLIT path. With no lighting to add an emissive term over, a self-illuminating
			 * material's surface colour IS its emitted radiance, so the emission MULTIPLIES here
			 * (the lit path adds it instead — see LightGenerator). Without this, a skybox with a
			 * luminance of 8000 nits still rendered as its raw [0,1] cubemap, i.e. black once the
			 * scene went photometric: the simple pass wrote the surface colour verbatim. */
			const auto * material = this->getMaterialInterface();

			/* NOTE: fragmentColor() rebuilds its GLSL expression (component lookup + string
			 * concatenation) on every call; it is used up to four times below for the exact
			 * same material and layer, so compute it once. */
			const auto fragmentColorExpr = material->fragmentColor();

			if ( const auto emission = material->emissionMultiplier(); emission.empty() )
			{
				Code{*fragmentShader, Location::Output} << ShaderVariable::OutputFragment << " = " << fragmentColorExpr << ';';
			}
			else
			{
				Code{*fragmentShader, Location::Output} << ShaderVariable::OutputFragment << " = vec4((" << fragmentColorExpr << ").rgb * " << emission << ", (" << fragmentColorExpr << ").a);";
			}

			if ( m_hasNormalsAttachment )
			{
				Code{*fragmentShader, Location::Output} << ShaderVariable::OutputNormal << " = vec4(0.0, 0.0, 1.0, 0.5);";
			}

			if ( m_hasMaterialPropertiesAttachment )
			{
				Code{*fragmentShader, Location::Output} << ShaderVariable::OutputMaterialProperties << " = vec4(0.0, 1.0, 240.0 / 255.0, 1.0);";
			}

			if ( m_hasAlbedoAttachment )
			{
				/* Unlit material: its displayed color IS its albedo. */
				Code{*fragmentShader, Location::Output} << ShaderVariable::OutputAlbedo << " = vec4((" << fragmentColorExpr << ").rgb, 1.0);";
			}

			if ( m_hasVelocityAttachment )
			{
				if ( m_velocityOutputsEmitted )
				{
					Code{*fragmentShader, Location::Output} << ShaderVariable::OutputVelocity << " = (" << ShaderVariable::ClipPositionCurrent << ".xy / " << ShaderVariable::ClipPositionCurrent << ".w) - (" << ShaderVariable::ClipPositionPrevious << ".xy / " << ShaderVariable::ClipPositionPrevious << ".w);";
				}
				else
				{
					Code{*fragmentShader, Location::Output} << ShaderVariable::OutputVelocity << " = vec2(0.0);";
				}
			}
		}
		else
		{
			Code{*fragmentShader, Location::Output} << ShaderVariable::OutputFragment << " = vec4(1.0, 0.0, 1.0, 1.0);";

			if ( m_hasNormalsAttachment )
			{
				Code{*fragmentShader, Location::Output} << ShaderVariable::OutputNormal << " = vec4(0.0, 0.0, 1.0, 0.5);";
			}

			if ( m_hasMaterialPropertiesAttachment )
			{
				Code{*fragmentShader, Location::Output} << ShaderVariable::OutputMaterialProperties << " = vec4(0.0, 1.0, 240.0 / 255.0, 1.0);";
			}

			if ( m_hasAlbedoAttachment )
			{
				/* No material: neutral white albedo (identity for indirect-light modulation). */
				Code{*fragmentShader, Location::Output} << ShaderVariable::OutputAlbedo << " = vec4(1.0);";
			}

			if ( m_hasVelocityAttachment )
			{
				if ( m_velocityOutputsEmitted )
				{
					Code{*fragmentShader, Location::Output} << ShaderVariable::OutputVelocity << " = (" << ShaderVariable::ClipPositionCurrent << ".xy / " << ShaderVariable::ClipPositionCurrent << ".w) - (" << ShaderVariable::ClipPositionPrevious << ".xy / " << ShaderVariable::ClipPositionPrevious << ".w);";
				}
				else
				{
					Code{*fragmentShader, Location::Output} << ShaderVariable::OutputVelocity << " = vec2(0.0);";
				}
			}
		}

		/* Discard nearly invisible pixels to avoid blending artifacts.
		 * ⚠️ This is NOT a coverage test and must never be used as one: 0.01 is a "this texel adds
		 * nothing to the blend" floor, while an alpha test asks "does this texel cover the pixel".
		 * A blended material writes DEPTH and the whole G-buffer wherever it survives, so using this
		 * floor as the coverage gate stamped every leaf QUAD into the depth buffer as soon as the
		 * mask was minified — the box-filtered mip chain drove the two apart without bound (on
		 * Sponza's cypress, 6.41 % of the quad passing 0.01 at the base level against 26.56 % at
		 * mip 9 and 100 % at mip 11, for a real coverage of 6.34 %), and the reflections then
		 * applied on the quad instead of on the leaves. A mis-declared cutout is promoted to a real
		 * one at load time instead — StandardResource::promoteBinaryCoverageToCutout(), run from the
		 * onBeforeCreation() hook — which clears BlendingEnabled and takes it out of this branch.
		 * ⚠️ Scope, measured: this is the DISTANT-foliage half of the defect. Up close the mask is
		 * sampled near mip 0, the coverage is already correct, and the canopy still washes out — that
		 * near-field half is the grazing-normal Fresnel spike, not this.
		 * ⚠️ The flag, not blendingMode(): enableAlphaTest() clears BlendingEnabled but leaves the
		 * mode in place, so gating on the mode alone would keep emitting this test on a material
		 * that no longer blends at all. */
		if ( this->materialEnabled() )
		{
			const auto * material = this->getMaterialInterface();

			if ( material->blendingMode() != BlendingMode::None && material->isFlagEnabled(Material::BlendingEnabled) )
			{
				Code{*fragmentShader, Location::Output} << "if ( " << ShaderVariable::OutputFragment << ".a < 0.01 ) discard;";
			}
		}

		return fragmentShader->generateSourceCode(*this);
	}
	
	bool
	SceneRendering::onGraphicsPipelineConfiguration (const Program & /*program*/, GraphicsPipeline & graphicsPipeline) noexcept
	{
		const auto * renderableInstance = this->getRenderableInstance();
		const auto * renderable = renderableInstance->renderable();

		/* NOTE: Use dynamic viewport and scissor to avoid pipeline recreation on window resize. */
		{
			const StaticVector< VkDynamicState, 16 > dynamicStates{
				VK_DYNAMIC_STATE_VIEWPORT,
				VK_DYNAMIC_STATE_SCISSOR
			};

			if ( !graphicsPipeline.configureDynamicStates(dynamicStates) )
			{
				Tracer::error(ClassId, "Unable to configure the graphics pipeline dynamic states !");

				return false;
			}
		}

		/* A cubemap target renders through a mirrored projection — the front face must follow. */
		if ( !graphicsPipeline.configureRasterizationState(m_renderPassType, renderable->layerRasterizationOptions(this->layerIndex()), 0, this->renderTarget()->isCubemap()) )
		{
			Tracer::error(ClassId, "Unable to configure the graphics pipeline rasterization state !");

			return false;
		}

		if ( !graphicsPipeline.configureDepthStencilState(m_renderPassType, *renderableInstance) )
		{
			Tracer::error(ClassId, "Unable to configure the graphics pipeline depth/stencil state !");

			return false;
		}

		if ( !graphicsPipeline.configureColorBlendState(m_renderPassType, *renderable->material(this->layerIndex())) )
		{
			Tracer::error(ClassId, "Unable to configure the graphics pipeline color blend state !");

			return false;
		}

		/* When the render target has MRT attachments (G-buffer: [1]=normals,
		 * [2]=material properties), the blend state differs per attachment
		 * (requires the 'independentBlend' device feature):
		 * - Ambient pass, opaque material: full write, same as the color attachment.
		 * - Ambient pass, translucent material: the color attachment keeps its alpha
		 *   blending, but the G-buffer receives the top-most surface data at 100%.
		 *   With the duplicated alpha blending, a translucent surface used to leave
		 *   only 'outNormal.a' (packed roughness+metalness, ~0.03 for water) of its
		 *   normal in the G-buffer — RTR/SSR then reflected off the surface BELOW
		 *   instead of the visible one (flat water reflections).
		 * - Light passes (additive): the G-buffer must never be modified — write
		 *   mask zero (previously they relied on shaders emitting vec4(0.0)). */
		if ( m_hasNormalsAttachment || m_hasMaterialPropertiesAttachment || m_hasAlbedoAttachment || m_hasVelocityAttachment )
		{
			auto gBufferState = graphicsPipeline.colorBlendAttachments()[0];

			if ( Graphics::renderPassIsLightPass(m_renderPassType) )
			{
				gBufferState.colorWriteMask = 0;
			}
			else if ( gBufferState.blendEnable == VK_TRUE )
			{
				gBufferState.blendEnable = VK_FALSE;
				gBufferState.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
				gBufferState.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
				gBufferState.colorBlendOp = VK_BLEND_OP_ADD;
				gBufferState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
				gBufferState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
				gBufferState.alphaBlendOp = VK_BLEND_OP_ADD;
			}

			if ( m_hasNormalsAttachment )
			{
				graphicsPipeline.appendColorBlendAttachment(gBufferState);
			}

			if ( m_hasMaterialPropertiesAttachment )
			{
				/* Material properties of a BLENDED material: the shader's alpha lane is a SELECT bit
				 * (opacity >= 0.5), so SRC_ALPHA / ONE_MINUS_SRC_ALPHA is an exact replace-or-keep of the
				 * packed nibbles (never an interpolation), and the lane itself stays 1 either way
				 * (a + dst.a * (1 - a)). Normals keep REPLACE: their alpha lane is packed roughness +
				 * metalness, and the top-most surface's normal is the one the reflections want (water). */
				auto materialPropertiesState = gBufferState;

				if ( graphicsPipeline.colorBlendAttachments()[0].blendEnable == VK_TRUE )
				{
					materialPropertiesState.blendEnable = VK_TRUE;
					materialPropertiesState.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
					materialPropertiesState.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
					materialPropertiesState.colorBlendOp = VK_BLEND_OP_ADD;
					materialPropertiesState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
					materialPropertiesState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
					materialPropertiesState.alphaBlendOp = VK_BLEND_OP_ADD;
				}

				graphicsPipeline.appendColorBlendAttachment(materialPropertiesState);
			}

			if ( m_hasAlbedoAttachment )
			{
				/* The albedo attachment of a BLENDED material is alpha-blended by its own alpha lane,
				 * which the shader sets to `diffuseWeight * opacity`: colour = src * a + dst * (1 - a),
				 * lane a = a + dst.a * (1 - a). A transparent texel leaves the surface below untouched, a
				 * 35 %-opaque metal decal weighs it to 0.65, an opaque leaf owns it. Normals and material
				 * properties keep the REPLACE policy above (their alpha lanes are packed data, not an
				 * opacity — the flat-water fix). See Graphics/AGENTS.md § "The albedo G-buffer". */
				auto albedoState = gBufferState;

				if ( graphicsPipeline.colorBlendAttachments()[0].blendEnable == VK_TRUE )
				{
					albedoState.blendEnable = VK_TRUE;
					albedoState.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
					albedoState.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
					albedoState.colorBlendOp = VK_BLEND_OP_ADD;
					albedoState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
					albedoState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
					albedoState.alphaBlendOp = VK_BLEND_OP_ADD;
				}

				graphicsPipeline.appendColorBlendAttachment(albedoState);
			}

			if ( m_hasVelocityAttachment )
			{
				graphicsPipeline.appendColorBlendAttachment(gBufferState);
			}
		}

		return true;
	}

	size_t
	SceneRendering::computeProgramCacheKey () const noexcept
	{
		/* NOTE: Helper function to combine hash values (boost::hash_combine style). */
		const auto hashCombine = [] (size_t & seed, size_t value) noexcept {
			seed ^= value + 0x9e3779b9 + (seed << 6) + (seed >> 2);
		};

		size_t hash = 0;

		/* 1. Render pass handle (critical for pipeline compatibility). */
		if ( const auto * framebuffer = this->renderTarget()->framebuffer(); framebuffer != nullptr )
		{
			hashCombine(hash, reinterpret_cast< size_t >(framebuffer->renderPass()->handle()));
		}

		/* 2. Render target type (cubemap vs single layer). */
		hashCombine(hash, static_cast< size_t >(this->renderTarget()->isCubemap()));

		/* 3. Renderable identity (geometry + material combination via resource name). */
		if ( this->isRenderableInstanceAvailable() )
		{
			const auto * renderable = this->getRenderable();

			if ( renderable != nullptr )
			{
				hashCombine(hash, Hash::FNV1a(renderable->name()));

				/* The wind changes the vertex stage, so it changes the program; so does the flutter of the foliage layer. */
				hashCombine(hash, static_cast< size_t >(renderable->hasVegetationWind()));
				hashCombine(hash, static_cast< size_t >(renderable->isVegetationFoliageLayer(this->layerIndex())));
			}

			/* So does a heightfield surface: another vertex stage, another fragment prelude, another
			 * PerModel layout. */
			hashCombine(hash, static_cast< size_t >(this->isHeightfieldSurfaceEnabled()));
			hashCombine(hash, static_cast< size_t >(this->isMeshShadingSurfaceEnabled()));
		}

		/* 4. Layer index. */
		hashCombine(hash, static_cast< size_t >(this->layerIndex()));

		/* 5. Render pass type. */
		hashCombine(hash, static_cast< size_t >(m_renderPassType));

		/* 6. Generator flags (instancing, lighting, facing camera, etc.). */
		hashCombine(hash, static_cast< size_t >(this->flags()));

		/* 6b. ⚠️ The G-buffer debug lane CHANGES THE GENERATED SOURCE, so it belongs in the key.
		 * The cache keys on the descriptor layout and the flag bits, never on plain values — a
		 * value that alters the emitted GLSL and is left out of the key means the cached program of
		 * the previous run is served and the change never appears. Which is exactly what happened
		 * the first time this instrument was switched on: the frame came back untouched, twice. */
		hashCombine(hash, static_cast< size_t >(this->debugMaterialPropertiesLane()));

		/* 7. Material layout signature to separate incompatible pipelines (e.g. Standard Refl/Refract). */
		if ( this->materialEnabled() )
		{
			if ( const auto & layout = this->getMaterialInterface()->descriptorSetLayout(); layout != nullptr )
			{
				hashCombine(hash, layout->getHash());
			}

			/* 8. Material codegen flag bits: two materials can share a descriptor layout yet
			 * generate structurally different GLSL (e.g. the alpha-test discard). */
			hashCombine(hash, static_cast< size_t >(this->getMaterialInterface()->flags()));
		}

		return hash;
	}
}
