/*
 * src/Scenes/Loaders/USDLoader.cpp
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

#include "USDLoader.hpp"

/* STL inclusions. */
#include <algorithm>
#include <cctype>
#include <filesystem>
#include <map>
#include <memory>
#include <optional>
#include <system_error>
#include <tuple>
#include <utility>
#include <vector>

/* Third-party inclusions. */
#include "lightusd.hh"
#include "composition.hh"
#include "asset-resolution.hh"
#include "tydra/render-data.hh"

/* Local inclusions. */
#include "IO/IO.hpp"
#include "Graphics/Geometry/IndexedVertexResource.hpp"
#include "GeometryDeduplicator.hpp"
#include "USDParser.hpp"
#include "USDZArchive.hpp"
#include "VertexFactory/ShapeSimplifier.hpp"
#include "Graphics/ImageResource.hpp"
#include "Graphics/Material/StandardResource.hpp"
#include "Graphics/TextureResource/Texture2D.hpp"
#include "PixelFactory/FileIO.hpp"
#include "PixelFactory/StreamIO.hpp"
#include "PixelFactory/Pixmap.hpp"
#include "Graphics/Renderable/MeshResource.hpp"
#include "Graphics/Renderable/MultiLayerMeshResource.hpp"
#include "Resources/Manager.hpp"
#include "String.hpp"
#include "SceneData.hpp"
#include "Tracer.hpp"
#include "VertexFactory/Shape.hpp"

namespace EmEn::Scenes::Loaders
{
	USDLoader::USDLoader (Resources::Manager & resources) noexcept
		: m_resources{resources}
	{

	}


	size_t
	USDLoader::buildInstanceSets (const std::vector< USD::Instancer > & instancers, const std::map< std::string, size_t > & builtMeshesByPath, SceneData & output) noexcept
	{
		size_t instanceTotal = 0;

		for ( const auto & instancer : instancers )
		{
			for ( size_t prototypeIndex = 0; prototypeIndex < instancer.prototypePaths.size(); ++prototypeIndex )
			{
				const auto & prototypePath = instancer.prototypePaths[prototypeIndex];

				/* A prototype is a SUBTREE, not a mesh: it may hold several meshes, and each of
				 * them must follow every instance. Matching on the path prefix is what keeps the
				 * two apart without assuming a shape the asset never promised. */
				std::vector< size_t > prototypeMeshIndices;

				for ( const auto & [meshPath, meshIndex] : builtMeshesByPath )
				{
					if ( meshPath == prototypePath || meshPath.starts_with(prototypePath + "/") )
					{
						prototypeMeshIndices.push_back(meshIndex);
					}
				}

				if ( prototypeMeshIndices.empty() )
				{
					TraceWarning{ClassId} << "Instancer '" << instancer.path << "' references prototype '" << prototypePath << "', which produced no mesh.";

					continue;
				}

				/* An instance whose protoIndices entry does not name THIS prototype belongs to
				 * another one. An instancer with a single prototype and no index array is the
				 * common case and means "all of them". */
				std::vector< Base::Math::CartesianFrame< float > > selected;

				if ( instancer.prototypeIndices.empty() )
				{
					selected = instancer.instances;
				}
				else
				{
					selected.reserve(instancer.instances.size());

					for ( size_t index = 0; index < instancer.instances.size(); ++index )
					{
						if ( index < instancer.prototypeIndices.size() && std::cmp_equal(instancer.prototypeIndices[index], prototypeIndex) )
						{
							selected.push_back(instancer.instances[index]);
						}
					}
				}

				if ( selected.empty() )
				{
					continue;
				}

				for ( const auto meshIndex : prototypeMeshIndices )
				{
					InstanceSetDescriptor descriptor;
					descriptor.name = prototypePath;
					descriptor.meshIndex = meshIndex;
					descriptor.instances = selected;

					output.instanceSets.emplace_back(std::move(descriptor));
				}

				instanceTotal += selected.size() * prototypeMeshIndices.size();
			}
		}

		return instanceTotal;
	}

	std::filesystem::path
	USDLoader::findCaseInsensitive (const std::filesystem::path & wanted) noexcept
	{
		std::error_code pathError;

		const auto directory = wanted.parent_path();

		if ( !std::filesystem::is_directory(directory, pathError) || pathError )
		{
			return {};
		}

		const auto lowered = [] (std::string text) {
			std::ranges::transform(text, text.begin(), [] (unsigned char character) {
				return static_cast< char >(std::tolower(character));
			});

			return text;
		};

		const auto target = lowered(Base::IO::toU8String(wanted.filename()));

		const std::filesystem::directory_iterator end;

		for ( auto it = std::filesystem::directory_iterator{directory, std::filesystem::directory_options::skip_permission_denied, pathError}; !pathError && it != end; it.increment(pathError) )
		{
			if ( lowered(Base::IO::toU8String(it->path().filename())) == target )
			{
				return it->path();
			}
		}

		return {};
	}

	std::shared_ptr< Graphics::TextureResource::Abstract >
	USDLoader::archiveTexture (const std::string & assetIdentifier, bool sRGB) noexcept
	{
		using namespace Graphics;
		using namespace Base::PixelFactory;

		std::string entryName;

		if ( !m_archive->resolve(assetIdentifier, {}, entryName) )
		{
			TraceWarning{ClassId} << "Texture '" << assetIdentifier << "' is not in the archive; the material keeps its flat colour.";

			return nullptr;
		}

		/* ⚠️ The format comes from the ENTRY NAME, and only Jpeg, PNG and Targa can be decoded from
		 * memory. A format we cannot read must be refused HERE: handing such an entry to the image
		 * container yields a resource that fails to load, the texture built on it fails in turn, and
		 * the material goes down with it — strictly worse than no texture, which merely falls back
		 * to the shader's flat colour. */
		const auto extension = [&entryName] () {
			const auto dot = entryName.find_last_of('.');

			auto text = dot == std::string::npos ? std::string{} : entryName.substr(dot + 1);

			std::ranges::transform(text, text.begin(), [] (unsigned char character) {
				return static_cast< char >(std::tolower(character));
			});

			return text;
		}();

		auto format = Pixmap< uint8_t >::Format::None;

		if ( extension == "jpg" || extension == "jpeg" )
		{
			format = Pixmap< uint8_t >::Format::Jpeg;
		}
		else if ( extension == "png" )
		{
			format = Pixmap< uint8_t >::Format::PNG;
		}
		else if ( extension == "tga" )
		{
			format = Pixmap< uint8_t >::Format::Targa;
		}
		else
		{
			TraceWarning{ClassId} <<
				"Archived texture '" << entryName << "' uses the '" << extension <<
				"' format, which cannot be decoded from memory; the material keeps its flat colour. "
				"StreamIO reads JPEG, PNG and Targa.";

			return nullptr;
		}

		/* ⚠️⚠️ The archive is captured BY VALUE. `getOrCreateResource()` runs this factory on the
		 * THREAD POOL, so it executes AFTER load() has returned and after the loader itself may be
		 * gone; a bare pointer into the mapping would be read from an unmapped range, minutes later,
		 * on a worker. The shared pointer keeps the mapping alive exactly as long as some factory
		 * still needs it, and costs nothing but a refcount. */
		/* NOTE: The entry's byte range in the archive file: a released CPU copy comes back from it. */
		std::optional< EncodedSource > source;

		if ( const auto range = m_archive->entryRange(entryName); range.has_value() )
		{
			source = EncodedSource::fileRange(m_archive->filepath(), range->first, range->second, format);
		}

		auto image = m_resources.container< ImageResource >()
			->getOrCreateResource(m_resourcePrefix + "/image/" + entryName, [archive = m_archive, entryName, format, encodedSource = std::move(source)] (auto & imageResource) {
				size_t size = 0;
				const auto * data = archive->bytes(entryName, size);

				if ( data == nullptr )
				{
					return false;
				}

				Pixmap< uint8_t > pixmap;

				constexpr ReadOptions options{
					.targetChannelMode = TargetChannelMode::RGBA
				};

				/* Decoded IN PLACE from the mapping — no extraction, no intermediate copy. */
				if ( !StreamIO::read(data, size, format, pixmap, options) )
				{
					return false;
				}

				if ( encodedSource.has_value() )
				{
					imageResource.setEncodedSource(*encodedSource);
				}

				return imageResource.load(std::move(pixmap));
			});

		if ( image == nullptr )
		{
			return nullptr;
		}

		/* NOTE: Created SYNCHRONOUSLY (the function only links the source): the colour space is set when this returns,
		 * and the material setter checks it — an asynchronous creation would set it later, on a worker (texture identity,
		 * docs/caution-points.md). */
		return m_resources.container< TextureResource::Texture2D >()
			->getOrCreateResourceSync(m_resourcePrefix + "/texture/" + entryName + ( sRGB ? "-srgb" : "-data" ), [image, sRGB] (auto & textureResource) {
				/* Set sRGB BEFORE load(): the flag must be in place when the VkImage is created. */
				if ( !textureResource.enableSRGB(sRGB) )
				{
					return false;
				}

				return textureResource.load(image);
			});
	}

	std::shared_ptr< Graphics::TextureResource::Abstract >
	USDLoader::resolveTexture (const std::string & assetIdentifier, bool sRGB, const std::filesystem::path & stageDirectory) noexcept
	{
		using namespace Graphics;
		using namespace Base::PixelFactory;

		if ( assetIdentifier.empty() )
		{
			return nullptr;
		}

		if ( m_archive != nullptr )
		{
			return this->archiveTexture(assetIdentifier, sRGB);
		}

		std::error_code pathError;
		/* NOTE: A USD asset identifier is UTF-8: IO::u8path() (the narrow path constructor is the ANSI code page on
		 * Windows). */
		auto fullPath = std::filesystem::weakly_canonical(stageDirectory / Base::IO::u8path(assetIdentifier), pathError);

		if ( pathError || !EmEn::Base::IO::exists(fullPath) )
		{
			/* ⚠️⚠️ CASE. An asset authored on Windows or macOS records whatever spelling the
			 * DCC felt like, on a filesystem that does not care. On Linux it does, and the
			 * texture simply is not found. Measured on Jungle Ruins: the very same material
			 * asks for `anthurium_botany_01_BaseColor.tif` while the file on disk is
			 * `Anthurium_Botany_01_BaseColor.tif` — and asks for `..._Normal.png` with the
			 * right case.
			 *
			 * The result is the WORST kind of failure to read: the normal map lands, the base
			 * colour does not, so the geometry is lit, detailed, and pure white. It looks like
			 * a lighting or material bug, and nothing points at a filename.
			 *
			 * So a miss is retried case-insensitively within the directory. Bounded to one
			 * listing of one directory, and only on a path that already failed. */
			fullPath = USDLoader::findCaseInsensitive(fullPath);

			if ( fullPath.empty() )
			{
				TraceWarning{ClassId} << "Texture '" << assetIdentifier << "' not found next to the stage.";

				return nullptr;
			}

			TraceWarning{ClassId} << "Texture '" << assetIdentifier << "' only matched by IGNORING CASE: '" << Base::IO::toU8String(fullPath.filename()) << "'. The asset's spelling is wrong for a case-sensitive filesystem.";
		}

		/* ⚠️⚠️ A FORMAT WE CANNOT DECODE MUST NOT REACH THE RESOURCE MANAGER. `PixelFactory`
		 * reads JPEG, PNG, Targa and HDR — and Jungle Ruins ships its plant base colours as
		 * 4096² TIFF. Handing such a path to the image container yields a resource that FAILS
		 * to load, the texture built on it fails in turn, and the material goes down with it:
		 * the plants stopped rendering ENTIRELY.
		 *
		 * That is strictly worse than not finding the file at all, which simply falls back to
		 * the shader's flat colour — and it is exactly the trap this project's "loading never
		 * returns a broken resource" rule exists to prevent. So the extension is checked
		 * FIRST, and an unreadable one is reported and skipped, colour intact. */
		const auto extension = [] (std::string text) {
			std::ranges::transform(text, text.begin(), [] (unsigned char character) {
				return static_cast< char >(std::tolower(character));
			});

			return text;
		}(Base::IO::toU8String(fullPath.extension()));

		if ( extension != ".jpg" && extension != ".jpeg" && extension != ".png" && extension != ".tga" && extension != ".hdr" && extension != ".tif" && extension != ".tiff" )
		{
			TraceWarning{ClassId} <<
				"Texture '" << Base::IO::toU8String(fullPath.filename()) << "' uses the unsupported '" << extension <<
				"' format; the material keeps its flat colour. PixelFactory reads JPEG, PNG, Targa, TIFF and HDR.";

			return nullptr;
		}

		const auto imageName = m_resourcePrefix + "/image/" + Base::IO::toU8String(fullPath.filename());

		auto image = m_resources.container< ImageResource >()
			->getOrCreateResource(imageName, [fullPath] (auto & imageResource) {
				Pixmap< uint8_t > pixmap;

				constexpr ReadOptions options{
					.targetChannelMode = TargetChannelMode::RGBA
				};

				if ( !FileIO::read(fullPath, pixmap, options) )
				{
					return false;
				}

				/* NOTE: The file itself: a released CPU copy comes back from it. */
				imageResource.setEncodedSource(Graphics::EncodedSource::file(fullPath));

				return imageResource.load(std::move(pixmap));
			});

		if ( image == nullptr )
		{
			return nullptr;
		}

		/* NOTE: Created SYNCHRONOUSLY (the function only links the source): the colour space is set when this returns,
		 * and the material setter checks it — an asynchronous creation would set it later, on a worker (texture identity,
		 * docs/caution-points.md). */
		return m_resources.container< TextureResource::Texture2D >()
			->getOrCreateResourceSync(m_resourcePrefix + "/texture/" + Base::IO::toU8String(fullPath.filename()) + ( sRGB ? "-srgb" : "-data" ), [image, sRGB] (auto & textureResource) {
				/* Set sRGB BEFORE load(): the flag must be in place when the VkImage is created. */
				if ( !textureResource.enableSRGB(sRGB) )
				{
					return false;
				}

				return textureResource.load(image);
			});
	}

	std::vector< std::shared_ptr< Graphics::Material::Interface > >
	USDLoader::buildMaterials (const std::vector< USD::Material > & descriptions, const std::filesystem::path & stageDirectory) noexcept
	{
		using namespace Graphics;
		using namespace Base::PixelFactory;

		std::vector< std::shared_ptr< Material::Interface > > materials;
		materials.reserve(descriptions.size());

		for ( const auto & description : descriptions )
		{
			const auto & materialName = description.resourceName;

			const auto albedoTexture = this->resolveTexture(description.albedoTexture, true, stageDirectory);
			const auto roughnessTexture = this->resolveTexture(description.roughnessTexture, false, stageDirectory);
			const auto metalnessTexture = this->resolveTexture(description.metalnessTexture, false, stageDirectory);
			const auto normalTexture = this->resolveTexture(description.normalTexture, false, stageDirectory);

			const auto albedoValue = description.albedoValue;
			const auto roughnessValue = description.roughnessValue;
			const auto metalnessValue = description.metalnessValue;
			const auto opacityValue = description.opacityValue;
			const auto opacityThreshold = description.opacityThreshold;
			const auto isTranslucent = description.isTranslucent;
			const auto isCutout = description.isCutout;

			auto material = m_resources.container< Material::StandardResource >()
				->getOrCreateResource(materialName, [albedoTexture, roughnessTexture, metalnessTexture, normalTexture, albedoValue, roughnessValue, metalnessValue, isTranslucent, isCutout, opacityValue, opacityThreshold] (auto & materialResource) {
					if ( albedoTexture != nullptr )
					{
						materialResource.setAlbedoComponent(albedoTexture);
					}
					else
					{
						materialResource.setAlbedoComponent(albedoValue);
					}

					if ( roughnessTexture != nullptr )
					{
						materialResource.setRoughnessComponent(roughnessTexture);
					}
					else
					{
						materialResource.setRoughnessComponent(roughnessValue);
					}

					if ( metalnessTexture != nullptr )
					{
						materialResource.setMetalnessComponent(metalnessTexture);
					}
					else
					{
						materialResource.setMetalnessComponent(metalnessValue);
					}

					if ( normalTexture != nullptr )
					{
						materialResource.setNormalComponent(normalTexture);
					}

					/* USD opacity, following the engine's opacity contract. A CUTOUT
					 * (opacityThreshold > 0) is a binary alpha test on the albedo alpha at the
					 * authored threshold — the material stays opaque, casts cutout shadows and
					 * alpha-tests at RT hit time. A TRANSLUCENT material (opacity < 1, no
					 * threshold) gets the global opacity value — setOpacityComponent() enables
					 * the blending itself.
					 *
					 * No material of the reference asset exercises the cutout branch: all 141
					 * report `opacityThreshold = 0`. */
					if ( isCutout )
					{
						materialResource.enableAlphaTest(opacityThreshold);
					}
					else if ( isTranslucent )
					{
						materialResource.setOpacityComponent(opacityValue);
					}

					return materialResource.setManualLoadSuccess(true);
				});

			materials.emplace_back(std::move(material));
		}

		return materials;
	}

	std::vector< std::shared_ptr< Graphics::Geometry::Interface > >
	USDLoader::buildLevelsOfDetail (const Base::VertexFactory::Shape< float, uint32_t > & finest, const std::string & resourceName) const noexcept
	{
		using namespace Graphics;
		using namespace Base::VertexFactory;

		/* Each level about a quarter of the previous one, the allowed error growing with the distance it is seen from. */
		constexpr std::array< ShapeSimplifierOptions, 3 > Levels{{
			{.targetRatio = 0.25F, .targetError = 0.01F, .allowSloppy = true},
			{.targetRatio = 0.25F, .targetError = 0.04F, .allowSloppy = true},
			{.targetRatio = 0.25F, .targetError = 0.1F, .allowSloppy = true}
		}};

		constexpr size_t MinimumTriangles{200};

		std::vector< std::shared_ptr< Geometry::Interface > > levels;
		const Shape< float > * previous = &finest;
		std::shared_ptr< Shape< float > > current;

		for ( size_t level = 0; level < Levels.size(); ++level )
		{
			auto simplified = simplifyShape(*previous, Levels[level]);

			/* A level that vanishes or barely shrinks ends the chain: the previous one stays the coarsest. */
			if ( !simplified || simplified->triangles().size() < MinimumTriangles || simplified->triangles().size() * 10 > previous->triangles().size() * 9 )
			{
				TraceInfo{ClassId} <<
					"Level of detail " << level + 1 << " of '" << resourceName << "' not kept: " << previous->triangles().size() << " triangles became " <<
					( simplified ? std::to_string(simplified->triangles().size()) : std::string{"nothing"} ) << ".";

				break;
			}

			current = std::make_shared< Shape< float > >(std::move(*simplified));

			auto geometry = m_resources.container< Geometry::IndexedVertexResource >()
				->getOrCreateResource(resourceName + "-lod" + std::to_string(level + 1), [current] (auto & geometryResource) {
					current->computeTriangleTangent();
					current->computeVertexTangent();

					return geometryResource.load(*current);
				}, Geometry::EnableTangentSpace | Geometry::EnablePrimaryTextureCoordinates);

			if ( geometry == nullptr )
			{
				break;
			}

			levels.emplace_back(std::move(geometry));
			previous = current.get();
		}

		if ( !levels.empty() )
		{
			TraceInfo{ClassId} << "Level-of-detail chain of '" << resourceName << "': " << finest.triangles().size() << " triangles, then " << levels.size() << " coarser level(s) down to " << previous->triangles().size() << ".";
		}

		return levels;
	}

	size_t
	USDLoader::buildMeshes (const USD::Asset & asset, const std::vector< std::shared_ptr< Graphics::Material::Interface > > & materials, SceneData & output, std::map< std::string, size_t > & builtMeshesByPath) noexcept
	{
		using namespace Graphics;
		using namespace Base::VertexFactory;

		const auto defaultMaterial = [this] () -> std::shared_ptr< Material::Interface > {
			return m_resources.container< Material::StandardResource >()->getDefaultResource();
		};

		/* ⚠️ One geometry per CONTENT, one mesh per (geometry, material, sides): JungleRuins' forests give each of
		 * their 195 instancers its own prototype, three trees in all — built one per instancer, that was 6 meshes × 65
		 * copies, 23 GB of GPU memory and a lost device (2026-10-04, item geometry-content-dedup). */
		GeometryDeduplicator deduplicator;
		std::map< std::tuple< const void *, const void *, bool >, std::shared_ptr< Renderable::Abstract > > sharedMeshes;
		size_t sharedMeshCount = 0;
		/* The level-of-detail chains, by their finest geometry (LoaderOptions::generateLevelsOfDetail). */
		std::map< const void *, std::pair< std::vector< std::shared_ptr< Geometry::Interface > >, size_t > > levelOfDetailChains;
		size_t levelOfDetailMeshCount = 0;

		size_t builtCount = 0;

		/* NOTE: One mesh at a time, as the analysis builds it: its shape is held by its resources (or dropped by the
		 * deduplication) before the next one is built. */
		USD::forEachMesh(asset, [&] (USD::Mesh mesh) {
			const auto & shape = mesh.shape;
			const auto & resourceName = mesh.resourceName;

		static constexpr auto GeometryFlags = Geometry::EnableTangentSpace | Geometry::EnablePrimaryTextureCoordinates;

		std::vector< std::shared_ptr< Geometry::Interface > > coarserLevels;

		auto geometry = deduplicator.getOrCreate(*shape, GeometryFlags, [this, &resourceName, &shape, &coarserLevels] () {
			/* The coarser levels are simplified HERE, before the finest geometry's factory runs on the pool: that
			 * factory writes the tangents of this very shape. */
			if ( m_options.generateLevelsOfDetail && shape->triangles().size() >= m_options.levelOfDetailMinimumTriangles )
			{
				coarserLevels = this->buildLevelsOfDetail(*shape, resourceName);
			}

			return m_resources.container< Geometry::IndexedVertexResource >()
				->getOrCreateResource(resourceName, [shape] (auto & geometryResource) {
					shape->computeTriangleTangent();
					shape->computeVertexTangent();

					return geometryResource.load(*shape);
				}, GeometryFlags);
		});

		if ( geometry != nullptr && !coarserLevels.empty() )
		{
			std::vector< std::shared_ptr< Geometry::Interface > > chain{geometry};
			chain.insert(chain.end(), coarserLevels.begin(), coarserLevels.end());

			/* A layer per sub-geometry (group), all with the mesh's one material: the trees carry an empty group
			 * beside the real one, and a chain given fewer layers than sub-geometries is refused at load. */
			levelOfDetailChains.emplace(geometry.get(), std::make_pair(std::move(chain), std::max< size_t >(1, shape->groups().size())));
		}

		if ( geometry == nullptr )
		{
			TraceWarning{ClassId} << "Unable to create the geometry of mesh '" << mesh.primName << "'.";

			return;
		}

		/* `material_id` is the whole-mesh binding. GeomSubsets carry per-face bindings and are
		 * not honoured yet — a mesh with several material subsets takes the first one. */
		auto material = ( mesh.material.has_value() && *mesh.material < materials.size() && materials[*mesh.material] != nullptr )
			? materials[*mesh.material]
			: defaultMaterial();

		/* Two-sidedness is carried by the mesh in USD, not by the material. */
		RasterizationOptions rasterization{};

		if ( mesh.doubleSided )
		{
			rasterization.setCullingMode(CullingMode::None);
		}

		const auto meshKey = std::make_tuple(static_cast< const void * >(geometry.get()), static_cast< const void * >(material.get()), mesh.doubleSided);
		std::shared_ptr< Renderable::Abstract > renderable;

		if ( const auto sharedIt = sharedMeshes.find(meshKey); sharedIt != sharedMeshes.end() )
		{
			renderable = sharedIt->second;
			sharedMeshCount++;
		}
		else if ( const auto chainIt = levelOfDetailChains.find(geometry.get()); chainIt != levelOfDetailChains.end() )
		{
			/* Finest first; one layer per sub-geometry, every one with the mesh's material and rasterization. */
			const auto & [chain, layerCount] = chainIt->second;

			renderable = m_resources.container< Renderable::MultiLayerMeshResource >()
				->getOrCreateResource(resourceName, [chain, layerCount, material, rasterization] (auto & meshResource) {
					return meshResource.load(chain, std::vector< std::shared_ptr< Material::Interface > >(layerCount, material), std::vector< RasterizationOptions >(layerCount, rasterization));
				});

			if ( renderable != nullptr )
			{
				sharedMeshes.emplace(meshKey, renderable);
				levelOfDetailMeshCount++;
			}
		}
		else
		{
			renderable = m_resources.container< Renderable::MeshResource >()
				->getOrCreateResource(resourceName, [geometry, material, rasterization] (auto & meshResource) {
					return meshResource.load(geometry, material, rasterization);
				});

			if ( renderable != nullptr )
			{
				sharedMeshes.emplace(meshKey, renderable);
			}
		}

		if ( renderable == nullptr )
		{
			return;
		}

		MeshDescriptor descriptor;
		descriptor.renderable = renderable;
		descriptor.geometry = geometry;
		descriptor.materials = {material};

		output.meshes.emplace_back(std::move(descriptor));

		builtMeshesByPath.emplace(mesh.primPath, output.meshes.size() - 1);

		/* A prototype stops HERE: it is a resource its instances will draw, never a node.
		 * Giving it one is the classic instancing bug — the whole forest is correct, plus one
		 * stray copy of every species sitting at the prototype's own place. */
		if ( !mesh.instancedOnly )
		{
			/* One node per mesh for this pass: the render scene's own hierarchy is not walked
			 * yet, so every mesh sits at the stage origin with its baked coordinates. */
			NodeDescriptor node;
			node.name = resourceName;
			node.meshIndex = output.meshes.size() - 1;

			output.nodes.emplace_back(std::move(node));
			output.rootNodeIndices.push_back(output.nodes.size() - 1);
		}

			builtCount++;
		});

		if ( deduplicator.sharedCount() > 0 )
		{
			TraceInfo{ClassId} <<
				deduplicator.sharedCount() << " mesh(es) reused an identical geometry (" << deduplicator.sharedVertexCount() << " vertices not built again), " <<
				sharedMeshCount << " an identical mesh.";
		}

		if ( levelOfDetailMeshCount > 0 )
		{
			TraceInfo{ClassId} << levelOfDetailMeshCount << " mesh(es) drawn through a level-of-detail chain (meshoptimizer).";
		}

		return builtCount;
	}

	bool
	USDLoader::load (const std::filesystem::path & filepath, SceneData & output) noexcept
	{
		if ( !EmEn::Base::IO::exists(filepath) )
		{
			TraceError{ClassId} << "File '" << Base::IO::toU8String(filepath) << "' does not exist !";

			return false;
		}

		/* ⚠️ Composition MUST run on an absolute path. tinyusdz stores each layer's working
		 * directory as given, then resolves nested references against it: fed a relative path,
		 * a reference inside `elements/Anthurium/PI_Anthurium.usd` is looked up under
		 * `elements/Anthurium/…` relative to the PROCESS working directory, which is the
		 * executable's, not the asset's. The sublayers still compose — so the failure is
		 * partial and quiet: the hard geometry arrives and every referenced prototype silently
		 * does not. Measured on Jungle Ruins: 84 meshes in, 0 PointInstancer prototypes. */
		std::error_code pathError;
		const auto absoluteFilepath = std::filesystem::weakly_canonical(std::filesystem::absolute(filepath, pathError), pathError);

		if ( pathError )
		{
			TraceError{ClassId} << "Unable to resolve an absolute path for '" << Base::IO::toU8String(filepath) << "' : " << pathError.message();

			return false;
		}

		std::string warning;
		std::string error;

		/* A USDZ is a stage whose layers and images live INSIDE the file. Everything below —
		 * composition, then texture reading — must therefore go through the archive rather than the
		 * filesystem, so the archive is opened first and equips the resolver. */
		const auto extension = [] (std::string text) {
			std::ranges::transform(text, text.begin(), [] (unsigned char character) {
				return static_cast< char >(std::tolower(character));
			});

			return text;
		}(Base::IO::toU8String(absoluteFilepath.extension()));

		if ( extension == ".usdz" )
		{
			m_archive = std::make_shared< USDZArchive >();

			if ( !m_archive->open(absoluteFilepath) )
			{
				m_archive.reset();

				return false;
			}
		}

		/* ⚠️ `LoadUSDFromFile()` reads the ROOT LAYER ONLY. It parses the subLayers metadata but
		 * composes nothing, so a 19-sublayer stage comes back holding 2 prims and reports
		 * success. Composition in tinyusdz is an EXPLICIT, separate pipeline, and it is the one
		 * used below:
		 *
		 *   LoadLayerFromFile → CompositeSublayers → CompositeAllArcs → LayerToStage
		 *
		 * ⚠️ `CompositeSublayersInPlace()` is DECLARED in v0.9.4's composition.hh but never
		 * implemented — linking against it fails with an undefined reference. Only the plain
		 * `CompositeSublayers()` exists. `LayerToStageInPlace()` IS implemented and is used
		 * below, because it frees each PrimSpec as it converts instead of holding the layer and
		 * the stage at once — which matters when the composed layer is measured in gigabytes. */

		/* Sublayers, references and payloads are all relative to the root layer's directory —
		 * except that tinyusdz stores each sublayer's working directory as the RAW relative path
		 * it was written with ("elements/Anthurium"), never joined with the root. A reference
		 * made from inside such a sublayer is then looked up relative to the PROCESS working
		 * directory and silently fails, taking its whole prim with it.
		 *
		 * Seeding the resolver with every directory of the stage tree side-steps this without a
		 * second patch to the library: whatever relative name a nested layer asks for, the
		 * directory holding it is already declared. Measured on Jungle Ruins, this is what turns
		 * eleven silently missing `*_classes.usda` prototype layers into resolved ones. */
		lightusd::AssetResolutionResolver resolver;

		/* ⚠️ Parent-relative ('..') asset paths must be ALLOWED explicitly. tinyusdz refuses them by
		 * default as a path-traversal guard (`security_policy::ValidateAndNormalizeAssetPath`), which
		 * kills any authoring tool anchoring its arcs above the layer that declares them: an Omniverse
		 * Kit export reaches its assets through `../Source/...`, and the usd-wg sample assets share
		 * their textures through `../_common/...`. Enabling the option does NOT skip the validation —
		 * a '..' that collapses lexically still does, only a LEADING one survives, handed to the
		 * resolver to rebase against its base directory and search paths. The engine composes local
		 * assets the user selected, through a resolver whose reach is the archive table or the stage's
		 * own directory tree, so the guard buys nothing here and costs every real-world stage
		 * (owner decision, 2026-09-14).
		 *
		 * ⚠️ The flag is carried by the RESOLVER as well as by the composition options below,
		 * because Tydra reads it back (`assetResolver.get_allow_parent_relative_paths()`) to sanitize
		 * the texture and animation asset paths it loads AFTER composition. Setting it on the
		 * composition options alone leaves every '../../Materials/...' texture of such a stage
		 * rejected, one warning per image, with the stage rendering untextured.
		 *
		 * ⚠️ History: this used to be a local patch of tinyusdz relaxing the validator itself.
		 * Upstream absorbed the feature as this OPT-IN option, so the patch was dropped from
		 * ext-deps-generator (commit 1191c0e, 2026-08-31) — which silently re-armed the guard and broke
		 * every stage built on '..'. Do not patch the library again: set the option. */
		resolver.set_allow_parent_relative_paths(true);

		std::vector< std::string > searchPaths;

		if ( m_archive != nullptr )
		{
			/* Inside an archive there are no directories to walk: the table IS the namespace. */
			m_archive->equip(resolver);

			searchPaths.emplace_back(m_archive->baseDirectory());
		}
		else
		{
			searchPaths.emplace_back(Base::IO::toU8String(absoluteFilepath.parent_path()));

			std::error_code walkError;
			const std::filesystem::recursive_directory_iterator end;

			for ( auto it = std::filesystem::recursive_directory_iterator{absoluteFilepath.parent_path(), std::filesystem::directory_options::skip_permission_denied, walkError}; !walkError && it != end; it.increment(walkError) )
			{
				if ( it->is_directory(walkError) )
				{
					searchPaths.emplace_back(Base::IO::toU8String(it->path()));
				}
			}

			if ( walkError )
			{
				TraceWarning{ClassId} << "Partial stage directory scan for '" << Base::IO::toU8String(filepath.filename()) << "' : " << walkError.message();
			}

			resolver.set_search_paths(searchPaths);
		}

		lightusd::Layer layer;

		/* NOTE: The root layer's parse is budgeted on the file it comes from (the archive, for a USDZ). */
		lightusd::USDLoadOptions loadOptions;
		USD::setLoadOptions(loadOptions, Base::IO::filesize(absoluteFilepath));

		if ( m_archive != nullptr )
		{
			/* ⚠️ Through the RESOLVER, not the filesystem. `LoadLayerFromFile()` would read the
			 * archive's leading bytes and hand back the first USD it finds with an empty working
			 * directory, so every nested layer would then be looked up against the process working
			 * directory instead of the archive. */
			if ( !lightusd::LoadLayerFromAsset(resolver, m_archive->rootAssetName(), &layer, &warning, &error, loadOptions) )
			{
				TraceError{ClassId} << "Unable to read the root layer of '" << Base::IO::toU8String(filepath.filename()) << "' : " << error;

				return false;
			}
		}
		else if ( !lightusd::LoadLayerFromFile(Base::IO::toU8String(absoluteFilepath), &layer, &warning, &error, loadOptions) )
		{
			TraceError{ClassId} << "Unable to read layer '" << Base::IO::toU8String(filepath) << "' : " << error;

			return false;
		}

		if ( !warning.empty() )
		{
			TraceWarning{ClassId} << "While reading '" << Base::IO::toU8String(filepath.filename()) << "' : " << warning;
			warning.clear();
		}

		/* ---- Analysis: the composition and the whole translation, without any resource (fuzz_usd covers it). ---- */
		const auto sourceName = Base::IO::toU8String(filepath.filename());

		lightusd::Stage stage;
		USD::StageMetrics metrics;

		if ( !USD::composeStage(resolver, layer, m_options.resolveReferences, sourceName, stage, metrics) )
		{
			return false;
		}

		m_resourcePrefix = "usd:" + Base::IO::toU8String(filepath.stem());

		auto asset = USD::analyse(stage, resolver, searchPaths, m_archive != nullptr ? m_archive->rootAssetName() : Base::IO::toU8String(absoluteFilepath), metrics, m_resourcePrefix, sourceName);

		if ( !asset.has_value() )
		{
			return false;
		}

		const auto & renderScene = *asset->renderScene;

		/* ---- Lights: the environment ones resolve their image next to the stage (a file system access). ---- */
		const auto lightBase = output.lights.size();

		for ( auto & descriptor : asset->lighting.lights )
		{
			if ( descriptor.type == LightType::Environment )
			{
				/* The analysis hands the RAW authored path: kept only once resolved. */
				const auto rawPath = std::exchange(descriptor.textureAssetPath, std::string{});

				if ( !rawPath.empty() )
				{
					std::error_code domePathError;
					/* NOTE: A USD asset path is UTF-8: IO::u8path(), never the narrow path constructor (the
					 * ANSI code page on Windows). textureAssetPath is UTF-8 too. */
					const auto fullPath = std::filesystem::weakly_canonical(absoluteFilepath.parent_path() / Base::IO::u8path(rawPath), domePathError);

					if ( !domePathError && EmEn::Base::IO::exists(fullPath) )
					{
						descriptor.textureAssetPath = Base::IO::toU8String(fullPath);
					}
					else
					{
						TraceWarning{ClassId} << "Dome light image '" << rawPath << "' not found next to the stage.";
					}
				}

				TraceInfo{ClassId} <<
					"Environment light '" << descriptor.name << "': intensity " << descriptor.intensity <<
					", image '" << ( descriptor.textureAssetPath.empty() ? "<none>" : descriptor.textureAssetPath ) << "'.";
			}

			output.lights.emplace_back(std::move(descriptor));
		}

		/* ---- Materials, meshes, instances. ---- */
		std::map< std::string, size_t > builtMeshesByPath;

		const auto materials = this->buildMaterials(asset->materials, absoluteFilepath.parent_path());
		const auto builtCount = this->buildMeshes(*asset, materials, output, builtMeshesByPath);
		const auto instanceCount = USDLoader::buildInstanceSets(asset->instancers, builtMeshesByPath, output);

		/* ---- The punctual lights' nodes, after the meshes' (their light index shifted past the lights already there). ---- */
		for ( auto node : asset->lighting.nodes )
		{
			if ( node.lightIndex.has_value() )
			{
				*node.lightIndex += lightBase;
			}

			output.nodes.emplace_back(std::move(node));
			output.rootNodeIndices.push_back(output.nodes.size() - 1);
		}

		TraceInfo{ClassId} <<
			asset->instancers.size() << " point instancers read: " << instanceCount << " instances over " <<
			output.instanceSets.size() << " sets, from " << asset->prototypePaths.size() << " prototypes, " <<
			asset->punctualLightCount << " punctual lights.";

		TraceInfo{ClassId} <<
			Base::IO::toU8String(filepath.filename()) << " converted: " <<
			renderScene.meshes.size() << " source meshes (" << builtCount << " built), " <<
			renderScene.materials.size() << " materials, " <<
			renderScene.textures.size() << " textures, " <<
			renderScene.images.size() << " images, " <<
			renderScene.nodes.size() << " root nodes, " <<
			renderScene.cameras.size() << " cameras, " <<
			renderScene.lights.size() << " lights, " <<
			renderScene.instances.size() << " instances.";

		TraceInfo{ClassId} <<
			"Stage metrics: upAxis " << ( metrics.zUp ? "Z" : "non-Z" ) <<
			", metersPerUnit " << metrics.metersPerUnit <<
			", timeCodes " << metrics.startTimeCode << " to " << metrics.endTimeCode << ".";

		/* An element made entirely of instances builds no drawable node at all, and is still a
		 * complete success. Judging on meshes alone would call the vegetation a failure.
		 * Without reference resolution (an inventory of the root layer only, owner decision 2026-10-11), an empty
		 * result is the expected answer, not a failure: the stage was read and reported above. */
		return builtCount > 0 || !output.instanceSets.empty() || !m_options.resolveReferences;
	}

}
