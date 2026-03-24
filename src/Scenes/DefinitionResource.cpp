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
 * https://github.com/londnoir/emeraude-engine
 *
 * --- THIS IS AUTOMATICALLY GENERATED, DO NOT CHANGE ---
 */

#include "DefinitionResource.hpp"

/* Local inclusions. */
#include "Libs/FastJSON.hpp"
#include "Resources/Manager.hpp"
#include "Scenes/Scene.hpp"
#include "Scenes/Component/Camera.hpp"
#include "Scenes/Component/Microphone.hpp"
#include "Scenes/Component/Visual.hpp"
#include "Graphics/Renderable/SkyBoxResource.hpp"
#include "Graphics/Renderable/BasicGroundResource.hpp"
#include "Graphics/Renderable/MeshResource.hpp"
#include "Graphics/Material/BasicResource.hpp"
#include "Graphics/Material/StandardResource.hpp"
#include "Tracer.hpp"

namespace EmEn::Scenes
{
	using namespace Libs;
	using namespace Graphics;

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

			return false;
		}

		const auto & root = rootCheck.value();

		/* Checks if additional stores before loading (optional) */
		this->serviceProvider().update(root);

		return this->load(root);
	}

	bool
	DefinitionResource::load (const Json::Value & data) noexcept
	{
		m_root = data;

		return true;
	}

	std::string
	DefinitionResource::getSceneName () const noexcept
	{
		if ( m_root.isMember(FastJSON::NameKey) && m_root[FastJSON::NameKey].isString() )
		{
			return m_root[FastJSON::NameKey].asString();
		}

		return "NoName";
	}

	float
	DefinitionResource::getBoundary (float defaultBoundary) const noexcept
	{
		return FastJSON::getValue< float >(m_root, BoundaryKey).value_or(defaultBoundary);
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

		const auto properties = m_root[FastJSON::PropertiesKey];

		scene.setEnvironmentPhysicalProperties({
			FastJSON::getValue< float >(properties, SurfaceGravityKey).value_or(Physics::Gravity::Earth< float >),
			FastJSON::getValue< float >(properties, AtmosphericDensityKey).value_or(Physics::Density::EarthStandardAir< float >),
			FastJSON::getValue< float >(properties, PlanetRadiusKey).value_or(Physics::Radius::Earth< float >)
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

		if ( type == "Basic" )
		{
			const auto boundary = scene.boundary();
			const auto gridDivision = FastJSON::getValue< uint32_t >(gnd, GridDivisionKey).value_or(8);
			const auto uvMultiplier = FastJSON::getValue< float >(gnd, UVMultiplierKey).value_or(boundary);

			/* Resolve material. */
			std::shared_ptr< Material::Interface > materialResource;

			if ( gnd.isMember(MaterialKey) && gnd[MaterialKey].isObject() )
			{
				const auto & mat = gnd[MaterialKey];
				const auto matType = FastJSON::getValue< std::string >(mat, TypeKey).value_or("Basic");
				const auto matResource = FastJSON::getValue< std::string >(mat, ResourceKey).value_or("");

				if ( matType == "Standard" && !matResource.empty() )
				{
					materialResource = this->serviceProvider().container< Material::StandardResource >()->getResource(matResource);
				}
				else
				{
					materialResource = this->serviceProvider().container< Material::BasicResource >()->getDefaultResource();
				}
			}
			else
			{
				materialResource = this->serviceProvider().container< Material::BasicResource >()->getDefaultResource();
			}

			if ( materialResource == nullptr )
			{
				TraceWarning{ClassId} << "Ground material not found !";

				return false;
			}

			auto ground = std::make_shared< Renderable::BasicGroundResource >(this->serviceProvider(), scene.name() + "Ground");

			if ( ground->load(boundary, gridDivision, materialResource, {}, uvMultiplier) )
			{
				scene.setGroundLevel(ground);

				return true;
			}

			TraceWarning{ClassId} << "Ground geometry failed to load !";
		}
		else
		{
			TraceWarning{ClassId} << "Ground type '" << type << "' not yet supported.";
		}

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
		const auto type = FastJSON::getValue< std::string >(lit, TypeKey).value_or("Static");

		if ( type == "Static" )
		{
			auto & staticLighting = scene.lightSet().enableAsStaticLighting();

			/* Ambient. */
			if ( lit.isMember(AmbientKey) && lit[AmbientKey].isObject() )
			{
				const auto & amb = lit[AmbientKey];

				float r = 0.3F, g = 0.4F, b = 0.6F, a = 1.0F;

				if ( amb.isMember(ColorKey) && amb[ColorKey].isArray() && amb[ColorKey].size() >= 3 )
				{
					r = amb[ColorKey][0U].asFloat();
					g = amb[ColorKey][1U].asFloat();
					b = amb[ColorKey][2U].asFloat();

					if ( amb[ColorKey].size() >= 4 )
					{
						a = amb[ColorKey][3U].asFloat();
					}
				}

				const auto intensity = FastJSON::getValue< float >(amb, IntensityKey).value_or(0.25F);

				staticLighting.setAmbientParameters({r, g, b, a}, intensity);
			}

			/* Light. */
			if ( lit.isMember(LightKey) && lit[LightKey].isObject() )
			{
				const auto & light = lit[LightKey];

				float r = 1.0F, g = 0.95F, b = 0.8F, a = 1.0F;

				if ( light.isMember(ColorKey) && light[ColorKey].isArray() && light[ColorKey].size() >= 3 )
				{
					r = light[ColorKey][0U].asFloat();
					g = light[ColorKey][1U].asFloat();
					b = light[ColorKey][2U].asFloat();

					if ( light[ColorKey].size() >= 4 )
					{
						a = light[ColorKey][3U].asFloat();
					}
				}

				const auto intensity = FastJSON::getValue< float >(light, IntensityKey).value_or(1.2F);

				staticLighting.setLightParameters({r, g, b, a}, intensity);
			}

			/* Direction. */
			if ( lit.isMember(DirectionKey) && lit[DirectionKey].isArray() && lit[DirectionKey].size() >= 3 )
			{
				const auto x = lit[DirectionKey][0U].asFloat();
				const auto y = lit[DirectionKey][1U].asFloat();
				const auto z = lit[DirectionKey][2U].asFloat();

				staticLighting.setAsDirectionalLight({x, y, z}, true);
			}

			return true;
		}

		TraceWarning{ClassId} << "Lighting type '" << type << "' not yet supported.";

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
			if ( nodeDef.isMember(PositionKey) && nodeDef[PositionKey].isArray() && nodeDef[PositionKey].size() >= 3 )
			{
				const auto x = nodeDef[PositionKey][0U].asFloat();
				const auto y = nodeDef[PositionKey][1U].asFloat();
				const auto z = nodeDef[PositionKey][2U].asFloat();

				node->setPosition({x, y, z}, Math::TransformSpace::World);
			}

			/* LookAt. */
			if ( nodeDef.isMember(LookAtKey) && nodeDef[LookAtKey].isArray() && nodeDef[LookAtKey].size() >= 3 )
			{
				const auto x = nodeDef[LookAtKey][0U].asFloat();
				const auto y = nodeDef[LookAtKey][1U].asFloat();
				const auto z = nodeDef[LookAtKey][2U].asFloat();

				node->lookAt({x, y, z}, false);
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
							scene.setActiveCamera(camera.get());
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
						const auto scale = FastJSON::getValue< float >(compDef, ScaleKey).value_or(1.0F);

						if ( !meshName.empty() )
						{
							auto * meshContainer = this->serviceProvider().container< Renderable::MeshResource >();

							if ( meshContainer != nullptr )
							{
								auto mesh = meshContainer->getResource(meshName);

								if ( mesh != nullptr )
								{
									node->componentBuilder< Component::Visual >(compName)
										.setup([scale] (auto & component) {
											component.getRenderableInstance()->enableLighting();
											component.getRenderableInstance()->setTransformationMatrix(Math::Matrix4F::scaling(scale));
										}).build(mesh);
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
			Math::Vector< 3, float > position{0.0F, 0.0F, 0.0F};

			if ( entityDef.isMember(PositionKey) && entityDef[PositionKey].isArray() && entityDef[PositionKey].size() >= 3 )
			{
				position[0] = entityDef[PositionKey][0U].asFloat();
				position[1] = entityDef[PositionKey][1U].asFloat();
				position[2] = entityDef[PositionKey][2U].asFloat();
			}

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
						const auto scale = FastJSON::getValue< float >(compDef, ScaleKey).value_or(1.0F);

						if ( !meshName.empty() )
						{
							auto * meshContainer = this->serviceProvider().container< Renderable::MeshResource >();

							if ( meshContainer != nullptr )
							{
								auto mesh = meshContainer->getResource(meshName);

								if ( mesh != nullptr )
								{
									entity->componentBuilder< Component::Visual >(compName)
										.setup([scale] (auto & component) {
											component.getRenderableInstance()->enableLighting();
											component.getRenderableInstance()->setTransformationMatrix(Math::Matrix4F::scaling(scale));
										}).build(mesh);
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
