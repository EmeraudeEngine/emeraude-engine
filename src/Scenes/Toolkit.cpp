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

/* Local inclusions. */
#include "Graphics/ImageResource.hpp"
#include "Graphics/Material/StandardResource.hpp"
#include "Graphics/TextureResource/Texture2D.hpp"

namespace EmEn::Scenes
{
	using namespace Base;
	using namespace Base::Math;
	using namespace Base::PixelFactory;
	using namespace Base::VertexFactory;
	using namespace Graphics;

	size_t Toolkit::s_autoEntityCount{0};

	std::shared_ptr< Material::Interface >
	Toolkit::vegetationMaterial (const std::string & name, VegetationSurface surface) noexcept
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

		if ( !hasImage("-color_a") )
		{
			TraceError{ClassId} << "No vegetation material '" << name << "': neither a store material nor the image '" << name << "-color_a' !";

			return nullptr;
		}

		auto * textures = m_resourceManager.container< TextureResource::Texture2D >();
		const auto albedo = textures->getResource(name + "-color_a", false);
		const auto normal = hasImage("-normal") ? textures->getResource(name + "-normal", false) : nullptr;
		const bool foliage = surface == VegetationSurface::Foliage;
		const auto alphaMask = foliage && hasImage("-alpha") ? textures->getResource(name + "-alpha", false) : nullptr;
		const auto roughness = !foliage && hasImage("-roughness") ? textures->getResource(name + "-roughness", false) : nullptr;
		const std::string materialName = std::string{foliage ? "Vegetation/Foliage/" : "Vegetation/Bark/"} + name;

		return materials->getOrCreateResourceSync(materialName, [&albedo, &normal, &alphaMask, &roughness, foliage] (Material::StandardResource & material) {
			if ( foliage )
			{
				/* The cut-out: a separate mask (red channel) or the colour image's own alpha. The alpha test keeps the
				 * card OPAQUE (depth write, no sorting): a leaf is either there or not. */
				if ( alphaMask != nullptr )
				{
					material.setAlbedoComponent(albedo);
					material.setOpacityComponent(alphaMask);
				}
				else
				{
					material.setAlbedoComponent(albedo, true);
				}

				material.enableAlphaTest(0.5F);
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

	std::shared_ptr< Node >
	Toolkit::generateNode (const std::string & entityName, GenPolicy genPolicy, bool movable) noexcept
	{
		const auto name = String::incrementalLabel(entityName.empty() ? "AutoNode" : entityName, s_autoEntityCount);

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
				if ( m_previousNode == nullptr )
				{
					break;
				}

				return m_previousNode;

			/* NOTE: A parent scene node has been set. Returning it ... */
			case GenPolicy::Parent :
				if ( m_previousNode == nullptr )
				{
					break;
				}

				parent = m_previousNode->createChild(name, m_cursorFrame, m_scene->lifetimeMS());
				break;

			default:
				parent = m_scene->root();
				break;
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
		const auto name = String::incrementalLabel(entityName.empty() ? "AutoNode" : entityName, s_autoEntityCount);

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

	BuiltEntity< Node, Component::SunCourse >
	Toolkit::generateSunCourse (const std::string & entityName, const Component::SunCourse::Options & course, const DirectionalShadowOptions & shadows) noexcept
	{
		/* The pivot: a MOVABLE node, the course writes its world position every logic cycle. */
		auto pivot = this->generateNode(entityName, GenPolicy::Simple, true);

		if ( pivot == nullptr )
		{
			return {};
		}

		/* The light, under the caller's shadow budget. It is built dark: the course writes the
		 * direction and the photometry of its start phase as soon as it is bound. */
		const auto light = shadows.build(*pivot, entityName, [] (Component::DirectionalLight & sun) {
			sun.setColor(White);
			sun.setIlluminance(0.0F);
			sun.useDirectionVector(false);
		});

		if ( light == nullptr )
		{
			TraceError{ClassId} << "Unable to create the directional light of the sun course '" << entityName << "' !";

			return {};
		}

		auto component = pivot->componentBuilder< Component::SunCourse >(entityName + "Course")
			.setup([&pivot, &light, &course] (Component::SunCourse & sunCourse) {
				sunCourse.bind(pivot, light);
				sunCourse.configure(course);
			}).build();

		if ( component == nullptr )
		{
			TraceError{ClassId} << "Unable to create the sun course component '" << entityName << "' !";

			return {};
		}

		if ( course.autoStart )
		{
			component->start();
		}

		TraceInfo{ClassId} << "Sun course '" << entityName << "': " << course.dayDuration << " s of day, " << course.noonElevation << "° at noon, " << course.zenithIlluminance << " lx at the zenith" << (shadows.shadowMapResolution > 0 ? ", shadow-mapped" : ", no shadow map") << ", pivot under '" << pivot->parent()->name() << "'.";

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
