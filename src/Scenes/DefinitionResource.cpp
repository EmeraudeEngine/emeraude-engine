/*
 * src/Scenes/DefinitionResource.cpp
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

#include "DefinitionResource.hpp"

/* STL inclusions. */
#include <optional>

/* Local inclusions. */
#include "Graphics/Material/StandardResource.hpp"
#include "Graphics/Renderable/BasicGroundResource.hpp"
#include "Graphics/Renderable/MultiLayerMeshResource.hpp"
#include "Graphics/Renderable/SkyBoxResource.hpp"
#include "FastJSON.hpp"
#include "Resources/Manager.hpp"
#include "Scenes/Component/Camera.hpp"
#include "Scenes/Component/Microphone.hpp"
#include "Scenes/Component/Visual.hpp"
#include "Scenes/Scene.hpp"
#include "Tracer.hpp"

namespace EmEn::Scenes
{
	using namespace Base;
	using namespace Graphics;

	namespace
	{
		/** @brief The finest ground grid a definition may ask for: (1024 + 1)² ≈ 1 M vertices (owner, 2026-09-30). */
		constexpr uint32_t MaxGridDivision{1024};

		/**
		 * @brief Reads an optional number of a definition.
		 * @note Owner ruling (2026-09-30): an INVALID value (wrong type, out of range, non-finite, refused by the
		 * predicate) is treated as an absent one — the default applies — with a warning naming the key.
		 * @tparam value_t The type of the number.
		 * @tparam predicate_t A callable bool (value_t).
		 * @param parent A reference to the JSON object holding the key.
		 * @param key The key.
		 * @param defaultValue The value of an absent or invalid key.
		 * @param isValid The range the value must be in.
		 * @param requirement The range in words, for the warning.
		 * @return value_t
		 */
		template< typename value_t, typename predicate_t >
		[[nodiscard]]
		value_t
		readNumber (const Json::Value & parent, const char * key, value_t defaultValue, predicate_t && isValid, const char * requirement) noexcept
		{
			if ( !parent.isObject() || !parent.isMember(key) )
			{
				return defaultValue;
			}

			const auto value = FastJSON::asValue< value_t >(parent[key]);

			if ( !value.has_value() || !std::forward< predicate_t >(isValid)(*value) )
			{
				TraceWarning{DefinitionResource::ClassId} << "The key '" << key << "' must be " << requirement << " ! Using the default (" << defaultValue << ").";

				return defaultValue;
			}

			return *value;
		}

		/**
		 * @brief Reads an optional number of a definition that only has to be a finite number.
		 * @tparam value_t The type of the number.
		 * @param parent A reference to the JSON object holding the key.
		 * @param key The key.
		 * @param defaultValue The value of an absent or invalid key.
		 * @return value_t
		 */
		template< typename value_t >
		[[nodiscard]]
		value_t
		readNumber (const Json::Value & parent, const char * key, value_t defaultValue) noexcept
		{
			return readNumber(parent, key, defaultValue, [] (value_t /*value*/) {
				return true;
			}, "a finite number");
		}

		/**
		 * @brief Reads an optional 3D vector of a definition (an array of at least three finite numbers).
		 * @note An invalid value warns and reads as absent (owner ruling, 2026-09-30).
		 * @param parent A reference to the JSON object holding the key.
		 * @param key The key.
		 * @return std::optional< Math::Vector< 3, float > >
		 */
		[[nodiscard]]
		std::optional< Math::Vector< 3, float > >
		readVector (const Json::Value & parent, const char * key) noexcept
		{
			if ( !parent.isObject() || !parent.isMember(key) )
			{
				return std::nullopt;
			}

			auto vector = FastJSON::getValue< Math::Vector< 3, float > >(parent, key);

			if ( !vector.has_value() )
			{
				TraceWarning{DefinitionResource::ClassId} << "The key '" << key << "' must be an array of three finite numbers ! Ignored.";
			}

			return vector;
		}
	}

	bool
	DefinitionResource::load () noexcept
	{
		return false;
	}

	bool
	DefinitionResource::load (const std::filesystem::path & filepath) noexcept
	{
		const auto rootCheck = FastJSON::getRootFromFile(filepath);

		if ( !rootCheck )
		{
			TraceError{ClassId} << "Unable to parse the resource file " << filepath << " !";

			static_cast< void >(this->failLoading());

			return false;
		}

		/* NOTE: jsoncpp's member access aborts on anything but an object (or null). */
		if ( !rootCheck->isObject() )
		{
			TraceError{ClassId} << "The resource file " << filepath << " does not hold a JSON object !";

			static_cast< void >(this->failLoading());

			return false;
		}

		/* Checks if additional stores before loading (optional) */
		this->serviceProvider().update(*rootCheck);

		return this->load(*rootCheck);
	}

	bool
	DefinitionResource::load (const Json::Value & data) noexcept
	{
		if ( !this->beginLoading() )
		{
			return false;
		}

		if ( !data.isObject() )
		{
			TraceError{ClassId} << "The scene definition '" << this->name() << "' is not a JSON object !";

			return this->setLoadSuccess(false);
		}

		m_root = data;

		return this->setLoadSuccess(true);
	}

	std::string
	DefinitionResource::getSceneName () const noexcept
	{
		return FastJSON::getValue< std::string >(m_root, FastJSON::NameKey).value_or("NoName");
	}

	float
	DefinitionResource::getBoundary (float defaultBoundary) const noexcept
	{
		return readNumber(m_root, BoundaryKey, defaultBoundary, [] (float value) {
			return value > 0.0F;
		}, "a finite number greater than 0");
	}

	bool
	DefinitionResource::buildScene (Scene & scene) noexcept
	{
		if ( m_root.empty() )
		{
			Tracer::error(ClassId, "No data ! Load a JSON file or set a JSON string before.");

			return false;
		}

		this->readProperties(scene);
		this->readBackground(scene);
		this->readGround(scene);
		this->readLighting(scene);

		/* Read nodes (recursive, attached to root node). */
		if ( m_root.isMember(NodesKey) && m_root[NodesKey].isArray() )
		{
			this->readNodes(scene, scene.root(), m_root[NodesKey]);
		}

		/* Read static entities. */
		if ( m_root.isMember(StaticEntitiesKey) && m_root[StaticEntitiesKey].isArray() )
		{
			this->readStaticEntities(scene);
		}

		return true;
	}

	Json::Value
	DefinitionResource::getExtraData () const noexcept
	{
		if ( !m_root.isMember(ExtraDataKey) || !m_root[ExtraDataKey].isObject() )
		{
			return {};
		}

		return m_root[ExtraDataKey];
	}

	bool
	DefinitionResource::readProperties (Scene & scene) noexcept
	{
		if ( !m_root.isMember(FastJSON::PropertiesKey) || !m_root[FastJSON::PropertiesKey].isObject() )
		{
			return false;
		}

		const auto & properties = m_root[FastJSON::PropertiesKey];

		scene.setEnvironmentPhysicalProperties({
			readNumber(properties, SurfaceGravityKey, Physics::Gravity::Earth< float >),
			readNumber(properties, AtmosphericDensityKey, Physics::Density::EarthStandardAir< float >),
			readNumber(properties, PlanetRadiusKey, Physics::Radius::Earth< float >)
		});

		return true;
	}

	bool
	DefinitionResource::readBackground (Scene & scene) noexcept
	{
		if ( !m_root.isMember(BackgroundKey) || !m_root[BackgroundKey].isObject() )
		{
			return false;
		}

		const auto & bg = m_root[BackgroundKey];
		const auto type = FastJSON::getValue< std::string >(bg, TypeKey).value_or("SkyBox");

		if ( type == "SkyBox" )
		{
			const auto resourceName = FastJSON::getValue< std::string >(bg, ResourceKey).value_or("");

			if ( resourceName.empty() )
			{
				TraceWarning{ClassId} << "Background SkyBox has no 'Resource' specified !";

				return false;
			}

			auto * container = this->serviceProvider().container< Renderable::SkyBoxResource >();

			if ( container != nullptr )
			{
				auto skyBox = container->getResource(resourceName);

				if ( skyBox != nullptr )
				{
					scene.setBackground(skyBox);

					/* OPT-IN: derive the scene lighting (ambient + directional) from the
					 * background photometric description. */
					if ( FastJSON::getValue< bool >(bg, ApplyLightingKey).value_or(false) )
					{
						scene.applyBackgroundLighting();
					}

					return true;
				}

				TraceWarning{ClassId} << "SkyBox resource '" << resourceName << "' not found !";
			}
		}
		else
		{
			TraceWarning{ClassId} << "Background type '" << type << "' not yet supported.";
		}

		return false;
	}

	bool
	DefinitionResource::readGround (Scene & scene) noexcept
	{
		if ( !m_root.isMember(GroundKey) || !m_root[GroundKey].isObject() )
		{
			return false;
		}

		const auto & gnd = m_root[GroundKey];
		const auto type = FastJSON::getValue< std::string >(gnd, TypeKey).value_or("Basic");
		const auto boundary = scene.boundary();
		const auto gridDivision = readNumber(gnd, GridDivisionKey, 64U, [] (uint32_t value) {
			return value >= 1 && value <= MaxGridDivision;
		}, "an integer from 1 to 1024");
		const auto uvMultiplier = readNumber(gnd, UVMultiplierKey, boundary);
		const auto shiftHeight = readNumber(gnd, ShiftHeightKey, 0.0F);

		auto * materials = this->serviceProvider().container< Material::StandardResource >();

		if ( materials == nullptr )
		{
			TraceError{ClassId} << "No material container !";

			return false;
		}

		/* Resolve material. */
		std::shared_ptr< Material::Interface > materialResource;

		if ( gnd.isMember(MaterialKey) && gnd[MaterialKey].isObject() )
		{
			const auto & mat = gnd[MaterialKey];
			const auto matType = FastJSON::getValue< std::string >(mat, TypeKey).value_or("Basic");
			const auto matResource = FastJSON::getValue< std::string >(mat, ResourceKey).value_or("");

			/* One lit material: the legacy "Basic", "Standard" and "PBR" type strings are
			 * accepted as synonyms of the single StandardResource (material merge, Lot 4). */
			if ( ( matType == "Standard" || matType == "PBR" ) && !matResource.empty() )
			{
				materialResource = materials->getResource(matResource);
			}
			else
			{
				materialResource = materials->getDefaultResource();
			}
		}
		else
		{
			materialResource = materials->getDefaultResource();
		}

		if ( materialResource == nullptr )
		{
			TraceWarning{ClassId} << "Ground material not found !";

			return false;
		}

		auto ground = std::make_shared< Renderable::BasicGroundResource >(this->serviceProvider(), scene.name() + "Ground");

		bool loaded = false;

		if ( type == "Basic" )
		{
			loaded = ground->load(boundary, gridDivision, materialResource, {}, uvMultiplier);
		}
		else if ( type == "PerlinNoise" )
		{
			Base::VertexFactory::PerlinNoiseParams< float > noise;

			if ( gnd.isMember(NoiseKey) && gnd[NoiseKey].isObject() )
			{
				const auto & n = gnd[NoiseKey];
				noise.size = readNumber(n, SizeKey, 1.0F);
				noise.factor = readNumber(n, FactorKey, 0.5F);
			}

			loaded = ground->loadPerlinNoise(boundary, gridDivision, materialResource, noise, {}, uvMultiplier, shiftHeight);
		}
		else if ( type == "DiamondSquare" )
		{
			Base::VertexFactory::DiamondSquareParams< float > noise;

			if ( gnd.isMember(NoiseKey) && gnd[NoiseKey].isObject() )
			{
				const auto & n = gnd[NoiseKey];
				noise.factor = readNumber(n, FactorKey, 0.89F);
				noise.roughness = readNumber(n, RoughnessKey, 0.5F);
				noise.seed = readNumber(n, SeedKey, 0);
				/* Optional: the per-level decay of the generator (1 = Brownian, the default; higher damps the finest levels). */
				noise.hurst = readNumber(n, HurstKey, 1.0F);
			}

			loaded = ground->loadDiamondSquare(boundary, gridDivision, materialResource, noise, {}, uvMultiplier, shiftHeight);
		}
		else
		{
			TraceWarning{ClassId} << "Ground type '" << type << "' not yet supported.";

			return false;
		}

		if ( loaded )
		{
			scene.setGroundLevel(ground);

			return true;
		}

		TraceWarning{ClassId} << "Ground geometry failed to load !";

		return false;
	}

	bool
	DefinitionResource::readLighting (Scene & scene) noexcept
	{
		if ( !m_root.isMember(LightingKey) || !m_root[LightingKey].isObject() )
		{
			return false;
		}

		const auto & lit = m_root[LightingKey];

		/* Ambient: photometric contract, the intensity is an illuminance in lux
		 * (see LightSet::setAmbientLightIntensity()). */
		if ( lit.isMember(AmbientKey) && lit[AmbientKey].isObject() )
		{
			const auto & amb = lit[AmbientKey];

			/* The colour: three or four finite components, none negative; anything else warns and keeps white. */
			PixelFactory::Color< float > color{1.0F, 1.0F, 1.0F, 1.0F};

			if ( amb.isMember(ColorKey) )
			{
				const auto readColor = FastJSON::getValue< PixelFactory::Color< float > >(amb, ColorKey);

				if ( readColor.has_value() && readColor->red() >= 0.0F && readColor->green() >= 0.0F && readColor->blue() >= 0.0F && readColor->alpha() >= 0.0F )
				{
					color = *readColor;
				}
				else
				{
					TraceWarning{ClassId} << "The key '" << ColorKey << "' must be an array of three or four finite numbers, none negative ! Using white.";
				}
			}

			const auto illuminance = readNumber(amb, IntensityKey, 100.0F, [] (float value) {
				return value >= 0.0F;
			}, "a finite illuminance of at least 0 lx");

			auto & lightSet = scene.lightSet();
			lightSet.enable();
			lightSet.setAmbientLightColor(color);
			lightSet.setAmbientLightIntensity(illuminance);

			return true;
		}

		/* NOTE: The legacy "Static" lighting type is gone. Directional lights are
		 * declared as entity components or derived from the background (sky) contract. */
		TraceWarning{ClassId} << "The scene lighting block declares no 'Ambient' object. "
			"Lights are declared as entity components or derived from the background contract.";

		return false;
	}

	bool
	DefinitionResource::readNodes (Scene & scene, const std::shared_ptr< Node > & parentNode, const Json::Value & nodesArray) noexcept
	{
		for ( const auto & nodeDef : nodesArray )
		{
			const auto name = FastJSON::getValue< std::string >(nodeDef, FastJSON::NameKey).value_or("");

			if ( name.empty() )
			{
				TraceWarning{ClassId} << "Node without name, skipping.";

				continue;
			}

			auto node = parentNode->createChild(name, {}, scene.lifetimeMS());

			if ( node == nullptr )
			{
				TraceWarning{ClassId} << "Failed to create node '" << name << "'.";

				continue;
			}

			/* Position. */
			if ( const auto position = readVector(nodeDef, PositionKey); position.has_value() )
			{
				node->setPosition(*position, Math::TransformSpace::World);
			}

			/* LookAt. */
			if ( const auto target = readVector(nodeDef, LookAtKey); target.has_value() )
			{
				node->lookAt(*target, false);
			}

			/* Components. */
			if ( nodeDef.isMember(ComponentsKey) && nodeDef[ComponentsKey].isArray() )
			{
				for ( const auto & compDef : nodeDef[ComponentsKey] )
				{
					const auto compType = FastJSON::getValue< std::string >(compDef, TypeKey).value_or("");
					const auto compName = FastJSON::getValue< std::string >(compDef, FastJSON::NameKey).value_or(name + compType);
					const auto isPrimary = FastJSON::getValue< bool >(compDef, PrimaryKey).value_or(false);

					if ( compType == "Camera" )
					{
						auto builder = node->componentBuilder< Component::Camera >(compName);

						if ( isPrimary )
						{
							builder.asPrimary();
						}

						auto camera = builder.build(true);

						if ( camera != nullptr && isPrimary )
						{
							scene.setActiveCamera(camera);
						}
					}
					else if ( compType == "Microphone" )
					{
						auto builder = node->componentBuilder< Component::Microphone >(compName);

						if ( isPrimary )
						{
							builder.asPrimary();
						}

						builder.build();
					}
					else if ( compType == "Visual" )
					{
						const auto meshName = FastJSON::getValue< std::string >(compDef, MeshKey).value_or("");
						const auto scale = readNumber(compDef, ScaleKey, 1.0F, [] (float value) {
							return value > 0.0F;
						}, "a finite number greater than 0");

						if ( !meshName.empty() )
						{
							auto * meshContainer = this->serviceProvider().container< Renderable::MultiLayerMeshResource >();

							if ( meshContainer != nullptr )
							{
								auto mesh = meshContainer->getResource(meshName);

								if ( mesh != nullptr )
								{
									node->componentBuilder< Component::Visual >(compName)
										.setup([scale] (auto & component) {
											component.getRenderableInstance()->setTransformationMatrix(Math::Matrix4F::scaling(scale));
										}).build(mesh, Graphics::RenderableInstance::Lighting::Lit);
								}
								else
								{
									TraceWarning{ClassId} << "Mesh '" << meshName << "' not found for component '" << compName << "'.";
								}
							}
						}
					}
					else if ( !compType.empty() )
					{
						TraceWarning{ClassId} << "Component type '" << compType << "' not yet supported on nodes.";
					}
				}
			}

			/* Recursive children nodes. */
			if ( nodeDef.isMember(NodesKey) && nodeDef[NodesKey].isArray() )
			{
				this->readNodes(scene, node, nodeDef[NodesKey]);
			}
		}

		return true;
	}

	bool
	DefinitionResource::readStaticEntities (Scene & scene) noexcept
	{
		const auto & entities = m_root[StaticEntitiesKey];

		for ( const auto & entityDef : entities )
		{
			const auto name = FastJSON::getValue< std::string >(entityDef, FastJSON::NameKey).value_or("");

			if ( name.empty() )
			{
				TraceWarning{ClassId} << "StaticEntity without name, skipping.";

				continue;
			}

			/* Position. */
			const auto position = readVector(entityDef, PositionKey).value_or(Math::Vector< 3, float >{0.0F, 0.0F, 0.0F});

			auto entity = scene.createStaticEntity(name, position);

			if ( entity == nullptr )
			{
				TraceWarning{ClassId} << "Failed to create static entity '" << name << "'.";

				continue;
			}

			/* Components. */
			if ( entityDef.isMember(ComponentsKey) && entityDef[ComponentsKey].isArray() )
			{
				for ( const auto & compDef : entityDef[ComponentsKey] )
				{
					const auto compType = FastJSON::getValue< std::string >(compDef, TypeKey).value_or("");
					const auto compName = FastJSON::getValue< std::string >(compDef, FastJSON::NameKey).value_or(name + compType);

					if ( compType == "Visual" )
					{
						const auto meshName = FastJSON::getValue< std::string >(compDef, MeshKey).value_or("");
						const auto scale = readNumber(compDef, ScaleKey, 1.0F, [] (float value) {
							return value > 0.0F;
						}, "a finite number greater than 0");

						if ( !meshName.empty() )
						{
							auto * meshContainer = this->serviceProvider().container< Renderable::MultiLayerMeshResource >();

							if ( meshContainer != nullptr )
							{
								auto mesh = meshContainer->getResource(meshName);

								if ( mesh != nullptr )
								{
									entity->componentBuilder< Component::Visual >(compName)
										.setup([scale] (auto & component) {
											component.getRenderableInstance()->setTransformationMatrix(Math::Matrix4F::scaling(scale));
										}).build(mesh, Graphics::RenderableInstance::Lighting::Lit);
								}
								else
								{
									TraceWarning{ClassId} << "Mesh '" << meshName << "' not found for entity '" << name << "'.";
								}
							}
						}
					}
					else if ( !compType.empty() )
					{
						TraceWarning{ClassId} << "Component type '" << compType << "' not yet supported on static entities.";
					}
				}
			}
		}

		return true;
	}
}
