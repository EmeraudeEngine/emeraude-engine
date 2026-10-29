/*
 * src/Scenes/Toolkit.cpp
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

#include "Toolkit.hpp"

/* STL inclusions. */
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <iterator>
#include <limits>
#include <memory>
#include <span>
#include <vector>

/* Local inclusions. */
#include "Graphics/Geometry/IndexedVertexResource.hpp"
#include "Graphics/Geometry/ResourceGenerator.hpp"
#include "Graphics/ImageResource.hpp"
#include "Graphics/ImposterAtlas.hpp"
#include "Graphics/Material/StandardResource.hpp"
#include "Graphics/RenderTarget/ImposterBake.hpp"
#include "Graphics/TexturePacker.hpp"
#include "Graphics/TextureResource/StoreTexture.hpp"
#include "Graphics/TextureResource/Texture2D.hpp"
#include "Math/OctahedralMapping.hpp"
#include "PixelFactory/Types.hpp"
#include "Scenes/Component/DirectionalLight.hpp"
#include "Scenes/Component/MultipleVisuals.hpp"
#include "Scenes/DirectionalShadowOptions.hpp"
#include "Settings.hpp"
#include "VertexFactory/ShapeAssembler.hpp"
#include "VertexFactory/ShapeBuilder.hpp"
#include "VertexFactory/ShapeBuilderOptions.hpp"
#include "VertexFactory/ShapeTriangle.hpp"
#include "VertexFactory/ShapeVertex.hpp"

/* Forward declarations. */
namespace EmEn::Graphics::Renderable
{
}

namespace EmEn::Scenes
{
	using namespace Base;
	using namespace Base::Math;
	using namespace Base::PixelFactory;
	using namespace Base::VertexFactory;
	using namespace Graphics;

	std::atomic< size_t > Toolkit::s_autoEntityCount{0};

	float
	Toolkit::viewDistanceSetting () const noexcept
	{
		return m_settings.getOrSetDefaultInRange< float >(GraphicsViewDistanceKey, DefaultGraphicsViewDistance, 0.0F, MaxGraphicsViewDistance, true);
	}

	std::shared_ptr< Material::Interface >
	Toolkit::vegetationMaterial (const std::string & name, VegetationSurface surface) const noexcept
	{
		auto * materials = m_resourceManager.container< Material::StandardResource >();

		/* 1. A store material of that name (a JSON in Materials/) wins: it is the surcharge. */
		if ( materials->isResourceExists(name) )
		{
			return materials->getResource(name, false);
		}

		/* 2. The images of the convention. ⚠️ Synchronous: this runs while a scene is being built. */
		const auto * images = m_resourceManager.container< ImageResource >();
		const auto hasImage = [images, &name] (const char * suffix) {
			return images->isResourceExists(name + suffix);
		};

		if ( !hasImage("-albedo") )
		{
			TraceError{ClassId} << "No vegetation material '" << name << "': neither a store material nor the image '" << name << "-albedo' !";

			return nullptr;
		}

		auto * textures = m_resourceManager.container< TextureResource::Texture2D >();
		const bool foliage = surface == VegetationSurface::Foliage;
		std::shared_ptr< TextureResource::Texture2D > albedo;
		std::shared_ptr< TextureResource::Texture2D > alphaMask;
		auto alphaChannel = PixelFactory::Channel::Red;

		/* NOTE: The texture packing (graphics § 39): the foliage mask goes into its albedo's A — one texture, and the
		 * mask reaches the coverage-preserving mips, which only read an alpha channel — before any texture is requested. */
		if ( foliage && hasImage("-alpha") )
		{
			auto * imageContainer = m_resourceManager.container< ImageResource >();
			const std::array< PackSource, 2 > sources{{
				{.image = imageContainer->getResource(name + "-albedo", false), .samplingKey = {}, .role = ComponentType::Albedo, .channel = PixelFactory::Channel::Red},
				{.image = imageContainer->getResource(name + "-alpha", false), .samplingKey = {}, .role = ComponentType::Opacity, .channel = PixelFactory::Channel::Red}
			}};

			for ( const auto & packedRole : TexturePacker{m_resourceManager}.pack(sources, name) )
			{
				if ( packedRole.role == ComponentType::Albedo )
				{
					albedo = packedRole.texture;
				}
				else if ( packedRole.role == ComponentType::Opacity )
				{
					alphaMask = packedRole.texture;
					alphaChannel = packedRole.channel;
				}
			}
		}

		if ( albedo == nullptr )
		{
			albedo = TextureResource::getStoreTexture(*textures, name + "-albedo", {.sRGB = true}, false);
		}

		if ( alphaMask == nullptr && foliage && hasImage("-alpha") )
		{
			alphaMask = textures->getResource(name + "-alpha", false);
		}

		/* NOTE: A normal map identity: BC5 (graphics § 39, D8). */
		const auto normal = hasImage("-normal") ? TextureResource::getStoreTexture(*textures, name + "-normal", {.normalMap = true}, false) : nullptr;
		const auto roughness = !foliage && hasImage("-roughness") ? textures->getResource(name + "-roughness", false) : nullptr;
		const std::string materialName = std::string{foliage ? "Vegetation/Foliage/" : "Vegetation/Bark/"} + name;

		return materials->getOrCreateResourceSync(materialName, [&albedo, &normal, &alphaMask, &roughness, foliage, alphaChannel] (Material::StandardResource & material) {
			if ( foliage )
			{
				/* The cut-out: the mask (packed in the albedo's A, else a separate red channel) or the colour image's own
				 * alpha. The alpha test keeps the card OPAQUE (depth write, no sorting): a leaf is either there or not. */
				if ( alphaMask != nullptr )
				{
					material.setAlbedoComponent(albedo);
					material.setOpacityComponent(alphaMask, 1.0F, alphaChannel);
				}
				else
				{
					material.setAlbedoComponent(albedo, true);
				}

				/* HASHED, not a fixed 0.5: a needle mask covering 23 % averages to 0.23 a few mips down and a
				 * fixed threshold then drops the whole card — distant pines were bare trunks. The hashed test
				 * keeps 23 % of the pixels at every distance (StandardResource::enableHashedAlphaTest()). */
				material.enableAlphaTest(0.5F);
				material.enableHashedAlphaTest();
				material.setRoughnessComponent(0.5F);
			}
			else
			{
				material.setAlbedoComponent(albedo);

				if ( roughness != nullptr )
				{
					material.setRoughnessComponent(roughness);
				}
				else
				{
					material.setRoughnessComponent(0.85F);
				}
			}

			material.setMetalnessComponent(0.0F);

			if ( normal != nullptr )
			{
				material.setNormalComponent(normal);
			}

			return material.setManualLoadSuccess(true);
		});
	}

	namespace
	{
		/**
		 * @brief Appends a range of a shape's triangles to a builder in construction, each vertex tinted with a colour.
		 * @param builder A reference to the builder (triangles, vertex colours enabled).
		 * @param shape A reference to the source shape.
		 * @param first The first triangle.
		 * @param end Past the last triangle (<= the shape's triangle count).
		 * @param colour The vertex colour.
		 */
		void
		appendTinted (ShapeBuilder< float > & builder, const Shape< float > & shape, size_t first, size_t end, const Color< float > & colour) noexcept
		{
			const auto & triangles = shape.triangles();

			for ( size_t index = first; index < end; ++index )
			{
				const auto & triangle = triangles[index];

				for ( uint32_t corner = 0; corner < 3; ++corner )
				{
					const auto & vertex = shape.vertex(triangle.vertexIndex(corner));

					builder.setPosition(vertex.position());
					builder.setNormal(vertex.normal());
					builder.setVertexColor(colour);
					builder.newVertex();
				}
			}
		}
	}

	BuiltEntity< StaticEntity, Component::Visual >
	Toolkit::generateHLODProxy (const std::string & entityName, const Shape< float > & layered, std::span< const HLODLayer > layers, const HLODOptions & options) noexcept
	{
		const auto & groups = layered.groups();

		if ( layered.empty() || groups.size() != layers.size() )
		{
			TraceError{ClassId} << "generateHLODProxy(): '" << entityName << "' has " << groups.size() << " layers but " << layers.size() << " descriptions !";

			return {};
		}

		/* The two parts, normals and vertex colours (no texture coordinates: the proxy has no texture). */
		Shape< float > body;
		Shape< float > glow;
		const ShapeBuilderOptions< float > builderOptions{true, false, true, false, false};
		ShapeBuilder< float > bodyBuilder{body, builderOptions};
		ShapeBuilder< float > glowBuilder{glow, builderOptions};
		const auto & triangles = layered.triangles();

		bodyBuilder.beginConstruction(ConstructionMode::Triangles);
		glowBuilder.beginConstruction(ConstructionMode::Triangles);

		for ( size_t group = 0; group < groups.size(); ++group )
		{
			const auto & layer = *std::next(layers.begin(), static_cast< std::ptrdiff_t >(group));
			const auto first = static_cast< size_t >(groups[group].first);

			if ( layer.role == HLODRole::Omit || (layer.role == HLODRole::Glow && options.glowMaterial == nullptr) )
			{
				continue;
			}

			auto & builder = layer.role == HLODRole::Glow ? glowBuilder : bodyBuilder;
			/* A group runs from its first triangle to the next group's (the last one: to the end). */
			const auto end = std::min< size_t >(triangles.size(), group + 1 < groups.size() ? static_cast< size_t >(groups[group + 1].first) : triangles.size());

			appendTinted(builder, layered, first, end, layer.colour);
		}

		bodyBuilder.endConstruction();
		glowBuilder.endConstruction();

		/* One renderable: the body, then the glow, each a layer. */
		Shape< float > proxy;
		ShapeAssembler< float > assembler{proxy};
		std::vector< std::shared_ptr< Material::Interface > > materials;
		std::vector< RasterizationOptions > rasterization;

		if ( !body.empty() && assembler.merge(body, true) )
		{
			materials.emplace_back(m_resourceManager.container< Material::StandardResource >()->getOrCreateResourceSync("+HLODVertexColour", [] (Material::StandardResource & material) {
				/* The vertex colour IS the albedo (the neutral white factor, no texture), matte. */
				material.enableVertexColor();
				material.setRoughnessComponent(0.9F);

				return material.setManualLoadSuccess(true);
			}));
			rasterization.emplace_back();
		}

		if ( !glow.empty() && assembler.merge(glow, true) )
		{
			materials.emplace_back(options.glowMaterial);
			rasterization.emplace_back(options.glowRasterization);
		}

		if ( materials.empty() || std::ranges::any_of(materials, [] (const auto & material) { return material == nullptr; }) )
		{
			TraceError{ClassId} << "generateHLODProxy(): '" << entityName << "' has nothing to draw or a material failed !";

			return {};
		}

		const Geometry::ResourceGenerator generator{m_resourceManager, Geometry::EnableNormal | Geometry::EnableVertexColor};
		const auto geometry = generator.shape(proxy, entityName + "HLOD");

		if ( geometry == nullptr )
		{
			TraceError{ClassId} << "generateHLODProxy(): unable to build the geometry of '" << entityName << "' !";

			return {};
		}

		const auto renderable = m_resourceManager.container< Renderable::MultiLayerMeshResource >()->getOrCreateResource(entityName + "HLOD", [geometry, materials, rasterization] (Renderable::MultiLayerMeshResource & mesh) {
			return mesh.load(geometry, materials, rasterization);
		});

		if ( renderable == nullptr )
		{
			return {};
		}

		auto built = this->generateRenderableInstance< StaticEntity >(entityName, renderable);

		if ( !built.isValid() )
		{
			return {};
		}

		built.entity()->setCollidable(false);

		auto instance = built.component()->getRenderableInstance();
		instance->setDrawDistanceRange(options.switchDistance, std::numeric_limits< float >::max());

		if ( !options.castShadows )
		{
			instance->disableShadowCasting();
		}

		return built;
	}

	Toolkit::TreeImposter
	Toolkit::bakeTreeImposter (const std::string & label, const std::shared_ptr< Renderable::Abstract > & tree, const Space3D::Sphere< float > & bounds) noexcept
	{
		if ( m_scene == nullptr || tree == nullptr || !std::isfinite(bounds.radius()) || bounds.radius() <= 0.0F )
		{
			TraceError{ClassId} << "Unable to bake the imposter of '" << label << "': no scene, no tree, or no bounds !";

			return {};
		}

		auto & renderer = m_scene->AVConsoleManager().graphicsRenderer();

		/* NOTE: The atlas is baked in THIS scene, from this scene's tree: its resources are named per scene (owner
		 * ruling 2026-09-30). A shared "Imposter/<label>" handed the second scene baking the same label the FIRST
		 * scene's material and atlas, while its own atlas, owned by nobody, died in the bake's command buffer. */
		const auto resourceName = "Imposter/" + m_scene->name() + "/" + label;

		auto atlas = std::make_shared< ImposterAtlas >(resourceName);

		if ( !atlas->create(renderer) )
		{
			return {};
		}

		const auto bakeTarget = m_scene->imposterBakeTarget(atlas->size());

		if ( bakeTarget == nullptr )
		{
			return {};
		}

		/* The rig: far below the scene, inside its boundary. The camera looks along -Z at the plane z = 0 of the rig,
		 * where the copies are laid, each scaled to a sphere of ImposterRigRadius in a cell twice as wide: the
		 * orthographic box is the whole grid. The camera stands far (ImposterCameraDistance): the coarsest LOD. */
		const auto gridSize = atlas->gridSize();
		const auto cellSide = 2.0F * ImposterRigRadius;
		const auto gridSide = cellSide * static_cast< float >(gridSize);
		const Vector< 3, float > rigOrigin{0.0F, -0.5F * m_scene->boundary(), 0.0F};

		bakeTarget->configureCamera(CartesianFrame< float >{rigOrigin + Vector< 3, float >{0.0F, 0.0F, ImposterCameraDistance}}, gridSide);

		const auto radius = bounds.radius() * ImposterMargin;
		const auto scale = ImposterRigRadius / radius;
		const auto & centre = bounds.position();

		std::vector< CartesianFrame< float > > copies;
		copies.reserve(static_cast< size_t >(gridSize) * gridSize);

		for ( uint32_t cellY = 0; cellY < gridSize; ++cellY )
		{
			for ( uint32_t cellX = 0; cellX < gridSize; ++cellX )
			{
				/* The view of this cell: the direction toward its camera, and the camera frame the shader reads it
				 * back with (the SAME two functions its GLSL transcription uses). */
				const auto view = imposterCellFrame(hemiOctahedralCellDirection< float >(cellX, cellY, gridSize));

				/* The copy's rotation R maps the view's right / up / back onto the rig's X / Y / Z (the bake camera's
				 * own axes), so its columns — what the frame holds — are R's rows read vertically. */
				const Vector< 3, float > upward{view.right[Y], view.up[Y], view.back[Y]};
				const Vector< 3, float > backward{view.right[Z], view.up[Z], view.back[Z]};
				const Vector< 3, float > rotatedCentre{
					Vector< 3, float >::dotProduct(view.right, centre),
					Vector< 3, float >::dotProduct(view.up, centre),
					Vector< 3, float >::dotProduct(view.back, centre)
				};

				/* Column cellX from the left, row cellY from the TOP of the image: the rig's +Y is the image's up. */
				const Vector< 3, float > cellCentre{
					(static_cast< float >(cellX) + 0.5F - static_cast< float >(gridSize) * 0.5F) * cellSide,
					(static_cast< float >(gridSize) * 0.5F - static_cast< float >(cellY) - 0.5F) * cellSide,
					0.0F
				};

				copies.emplace_back(cellCentre - rotatedCentre * scale, upward, backward, Vector< 3, float >{scale, scale, scale});
			}
		}

		const auto previousCursor = this->cursor();

		this->setCursor(rigOrigin);

		const auto rig = this->generateEntity< StaticEntity >("ImposterRig/" + label);

		this->setCursor(previousCursor);

		if ( rig == nullptr )
		{
			TraceError{ClassId} << "Unable to create the imposter rig of '" << label << "' !";

			return {};
		}

		const auto copiesVisual = rig->componentBuilder< Component::MultipleVisuals >("ImposterCopies")
			.setup([] (Component::MultipleVisuals & visuals) {
				visuals.getRenderableInstance()->setBakeOnly()->disableShadowCasting()->disableRayTracing();
			})
			.build(tree, copies, Graphics::RenderableInstance::Lighting::Lit);

		if ( copiesVisual == nullptr || !bakeTarget->enqueue(copiesVisual->getRenderableInstance(), atlas) )
		{
			TraceError{ClassId} << "Unable to queue the imposter bake of '" << label << "' !";

			return {};
		}

		/* The billboard: a [-1, 1]² quad the vertex stage turns toward the eye (AbstractVertexStage::
		 * enableImposterBillboarding()), shaded by the imposter material. */
		const Vector< 4, float > imposterBounds{centre[X], centre[Y], centre[Z], radius};

		const auto material = m_resourceManager.container< Material::StandardResource >()->getOrCreateResourceSync(resourceName, [atlas, imposterBounds] (Material::StandardResource & imposterMaterial) {
			if ( !imposterMaterial.setImposterAtlas(atlas, imposterBounds) )
			{
				return false;
			}

			/* Foliage and bark in one card: the leaves dominate a canopy seen from afar. */
			imposterMaterial.setRoughnessComponent(0.6F);
			imposterMaterial.setMetalnessComponent(0.0F);

			return imposterMaterial.setManualLoadSuccess(true);
		});

		if ( material == nullptr )
		{
			TraceError{ClassId} << "Unable to create the imposter material of '" << label << "' !";

			return {};
		}

		const Geometry::ResourceGenerator generator{m_resourceManager, Geometry::EnableTangentSpace | Geometry::EnablePrimaryTextureCoordinates};

		const auto quad = generator.quad(2.0F, 2.0F, resourceName + "/Quad");

		if ( quad == nullptr )
		{
			return {};
		}

		auto renderable = m_resourceManager.container< Renderable::MeshResource >()->getOrCreateResourceSync(resourceName, [quad, material] (Renderable::MeshResource & meshResource) {
			return meshResource.load(quad, material);
		});

		if ( renderable == nullptr )
		{
			TraceError{ClassId} << "Unable to create the imposter renderable of '" << label << "' !";

			return {};
		}

		return {.atlas = atlas, .renderable = renderable};
	}

	std::shared_ptr< Node >
	Toolkit::generateNode (const std::string & entityName, GenPolicy genPolicy, bool movable) noexcept
	{
		const auto name = (entityName.empty() ? std::string{"AutoNode"} : entityName) + std::to_string(s_autoEntityCount.fetch_add(1, std::memory_order_relaxed));

		if ( m_scene == nullptr )
		{
			TraceError{ClassId} << "There is no linked scene to create a scene node '" << name << "' !";

			return nullptr;
		}

		std::shared_ptr< Node > parent{};

		switch ( m_nodeGenerationPolicy )
		{
			/* NOTE: A reusable scene node has been set. Returning it ... */
			case GenPolicy::Reusable :
				if ( m_previousNode != nullptr )
				{
					return m_previousNode;
				}
				break;

			/* NOTE: A parent scene node has been set: the new node goes directly under it. This used to create an
			 * intermediate node of the same name under it first, then the node under that one — an empty node in
			 * the graph and the cursor offset applied twice (triad 2026-09-30). */
			case GenPolicy::Parent :
				parent = m_previousNode;
				break;

			default:
				break;
		}

		/* NOTE: A policy without its node (setParentNode(nullptr), a failed build passed on) falls back to the root:
		 * it used to dereference null. */
		if ( parent == nullptr )
		{
			parent = m_scene->root();
		}

		auto childNode = parent->createChild(name, m_cursorFrame, m_scene->lifetimeMS());

		if ( childNode == nullptr )
		{
			TraceError{ClassId} << "Unable to create a scene node '" << name << "' !";

			return nullptr;
		}

		childNode->setMovingAbility(movable);

		/* Change the new policy if requested. */
		switch ( genPolicy )
		{
			case GenPolicy::Reusable :
				this->setReusableNode(childNode);
				break;

			case GenPolicy::Parent :
				this->setParentNode(childNode);
				break;

			default:
				break;
		}

		return childNode;
	}

	std::shared_ptr< Node >
	Toolkit::generateNode (const Vector< 3, float > & lookAt, const std::string & entityName, GenPolicy genPolicy, bool movable) noexcept
	{
		auto node = this->generateNode(entityName, genPolicy, movable);

		if ( node == nullptr )
		{
			return nullptr;
		}

		node->lookAt(lookAt, true);

		return node;
	}

	std::shared_ptr< StaticEntity >
	Toolkit::generateStaticEntity (const std::string & entityName, GenPolicy genPolicy) noexcept
	{
		const auto name = (entityName.empty() ? std::string{"AutoNode"} : entityName) + std::to_string(s_autoEntityCount.fetch_add(1, std::memory_order_relaxed));

		if ( m_scene == nullptr )
		{
			TraceError{ClassId} << "There is no linked scene to create a static entity '" << name << "' !";

			return nullptr;
		}

		if ( m_staticEntityGenerationPolicy == GenPolicy::Reusable && m_previousStaticEntity != nullptr )
		{
			return m_previousStaticEntity;
		}

		auto staticEntity = m_scene->createStaticEntity(name, m_cursorFrame);

		if ( staticEntity == nullptr )
		{
			TraceError{ClassId} << "Unable to create a static entity '" << name << "' !";

			return nullptr;
		}

		/* Change the new policy if requested. */
		if ( genPolicy == GenPolicy::Reusable )
		{
			this->setReusableStaticEntity(staticEntity);
		}

		return staticEntity;
	}

	std::shared_ptr< StaticEntity >
	Toolkit::generateStaticEntity (const Vector< 3, float > & lookAt, const std::string & entityName, GenPolicy genPolicy) noexcept
	{
		auto staticEntity = this->generateStaticEntity(entityName, genPolicy);

		if ( staticEntity == nullptr )
		{
			return nullptr;
		}

		staticEntity->lookAt(lookAt, false);

		return staticEntity;
	}

	BuiltEntity< Node, Component::CelestialClock >
	Toolkit::generateCelestialClock (const std::string & entityName, const Component::CelestialClock::Options & options, const DirectionalShadowOptions & sunShadows) noexcept
	{
		/* The sun's pivot: a MOVABLE node, the clock writes its world position every logic cycle. */
		auto pivot = this->generateNode(entityName, GenPolicy::Simple, true);

		if ( pivot == nullptr )
		{
			return {};
		}

		/* The light, under the caller's shadow budget. It is built dark: the clock writes the direction and the
		 * photometry of its start date as soon as it is bound. */
		const auto darkLight = [] (Component::DirectionalLight & light) {
			light.setColor(White);
			light.setIlluminance(0.0F);
			light.useDirectionVector(false);
		};

		const auto light = sunShadows.build(*pivot, entityName, darkLight);

		if ( light == nullptr )
		{
			TraceError{ClassId} << "Unable to create the directional light of the sun of the celestial clock '" << entityName << "' !";

			return {};
		}

		auto component = pivot->componentBuilder< Component::CelestialClock >(entityName + "Clock")
			.setup([&pivot, &light, &options] (Component::CelestialClock & clock) {
				clock.configure(options);
				clock.bindSun(pivot, light);
			}).build();

		if ( component == nullptr )
		{
			TraceError{ClassId} << "Unable to create the celestial clock component '" << entityName << "' !";

			return {};
		}

		/* One pivot and one light per moon, WITHOUT a shadow map: a second shadowed directional light would double the
		 * shadow cost at dusk for a fraction of a lux. */
		for ( size_t index = 0; index < options.moons.size(); ++index )
		{
			const auto moonName = entityName + "Moon" + std::to_string(index);
			const auto moonPivot = this->generateNode(moonName, GenPolicy::Simple, true);

			if ( moonPivot == nullptr )
			{
				TraceError{ClassId} << "Unable to create the node of the moon '" << moonName << "' !";

				continue;
			}

			const auto moonLight = DirectionalShadowOptions{}.build(*moonPivot, moonName, darkLight);

			if ( moonLight == nullptr )
			{
				TraceError{ClassId} << "Unable to create the directional light of the moon '" << moonName << "' !";

				continue;
			}

			component->bindMoon(index, moonPivot, moonLight);
		}

		if ( options.autoStart )
		{
			component->start();
		}

		TraceInfo{ClassId} << "Celestial clock '" << entityName << "': latitude " << options.latitude << "°, solar day " << component->solarDay() << " s, " << options.timeScale << "× real time, " << options.moons.size() << " moon(s), " << options.sun.zenithIlluminance << " lx at the zenith" << (sunShadows.shadowMapResolution > 0 ? ", shadow-mapped sun" : ", no shadow map") << ", pivot under '" << pivot->parent()->name() << "'.";

		return {pivot, component};
	}

	std::vector< CartesianFrame< float > >
	Toolkit::generateRandomCoordinates (size_t count, float min, float max) noexcept
	{
		std::vector< CartesianFrame< float > > coordinates{count};

		for ( auto & coordinate : coordinates )
		{
			coordinate.setPosition(
				m_randomizer.value(min, max),
				m_randomizer.value(min, max),
				m_randomizer.value(min, max)
			);
		}

		return coordinates;
	}

	std::shared_ptr< Material::Interface >
	Toolkit::getColoredMaterialResource (const Color< float > & color) const noexcept
	{
		std::stringstream materialName;
		materialName << "+Color(" << color << ")";

		return m_resourceManager.container< Material::StandardResource >()->getOrCreateResource(materialName.str(), [color] (auto & material) {
			material.setAlbedoComponent(color);
			/* NOTE: Quick debug color, kept lit but perfectly matte and dielectric
			 * to avoid any highlight altering the requested color on screen. */
			material.setRoughnessComponent(0.8F);
			material.setMetalnessComponent(0.0F);

			return material.setManualLoadSuccess(true);
		});
	}
}
