/*
 * src/Graphics/Renderable/SpriteResource.cpp
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

#include "SpriteResource.hpp"

/* Local inclusions. */
#include "Libs/VertexFactory/ShapeBuilder.hpp"
#include "Libs/FastJSON.hpp"
#include "Graphics/TextureResource/Texture2D.hpp"
#include "Graphics/TextureResource/AnimatedTexture2D.hpp"
#include "Graphics/Geometry/IndexedVertexResource.hpp"
#include "Graphics/Material/BasicResource.hpp"
#include "Graphics/Material/Helpers.hpp"
#include "Resources/Manager.hpp"

/* Defining the resource manager class id. */
template<>
const char * const EmEn::Resources::Container< EmEn::Graphics::Renderable::SpriteResource >::ClassId{"SpriteContainer"};

/* Defining the resource manager ClassUID. */
template<>
const size_t EmEn::Resources::Container< EmEn::Graphics::Renderable::SpriteResource >::ClassUID{getClassUID(ClassId)};

namespace EmEn::Graphics::Renderable
{
	using namespace EmEn::Libs;
	using namespace EmEn::Libs::Math;
	using namespace EmEn::Libs::VertexFactory;
	using namespace Saphir;
	using namespace Saphir::Keys;

	const size_t SpriteResource::ClassUID{getClassUID(ClassId)};
	std::mutex SpriteResource::s_lockGeometryLoading{};

	bool
	SpriteResource::load () noexcept
	{
		if ( !this->beginLoading() )
		{
			return false;
		}

		this->setReadyForInstantiation(false);

		if ( !this->prepareGeometry(false, false, false) )
		{
			Tracer::error(ClassId, "Unable to get default Geometry to generate the default Sprite !");

			return this->setLoadSuccess(false);
		}

		if ( !this->setMaterial(Resources::Manager::instance()->container< Material::BasicResource >()->getDefaultResource()) )
		{
			return this->setLoadSuccess(false);
		}

		return this->setLoadSuccess(this->addDependency(m_material));
	}

	bool
	SpriteResource::load (const Json::Value & data) noexcept
	{
		if ( !this->beginLoading() )
		{
			return false;
		}

		this->setReadyForInstantiation(false);

		auto * manager = Resources::Manager::instance();

		const auto materialResource = manager->container< Material::BasicResource >()->getOrCreateResource(
			"SpriteMaterial" + this->name(),
			[&data, manager] (Material::BasicResource & newMaterial)
			{
				if ( !data.isMember(Material::JKData) || !data[Material::JKData].isObject() )
				{
					TraceError{ClassId} << "The key '" << Material::JKData << "' JSON structure is not present or not an object !";

					return newMaterial.setManualLoadSuccess(false);
				}

				const auto & componentData = data[Material::JKData];

				/* Check the texture type. */
				const auto type = FastJSON::getString(data, Material::JKType);

				switch ( to_FillingType(type) )
				{
					case FillingType::Texture :
					{
						const auto textureResource = manager->container< TextureResource::Texture2D >()->getResource(FastJSON::getString(componentData, Material::JKName));

						if ( !newMaterial.setTexture(textureResource, true) )
						{
							return newMaterial.setManualLoadSuccess(false);
						}
					}
						break;

					case FillingType::AnimatedTexture :
					{
						const auto textureResource = manager->container< TextureResource::AnimatedTexture2D >()->getResource(FastJSON::getString(componentData, Material::JKName));

						if ( !newMaterial.setTexture(textureResource, true) )
						{
							return newMaterial.setManualLoadSuccess(false);
						}
					}
						break;

					default:
						TraceError{ClassId} << "Unhandled material type (" << type << ") for sprite !";

						return newMaterial.setManualLoadSuccess(false);
				}

				/* Check the blending mode. */
				newMaterial.enableBlendingFromJson(data);

				/* Check the optional global auto-illumination amount. */
				const auto autoIllumination = FastJSON::getNumber< float >(data, Material::JKAutoIllumination, 0.0F);

				if ( autoIllumination > 0.0F )
				{
					newMaterial.setAutoIlluminationAmount(autoIllumination);
				}

				/* Check the optional global opacity. */
				const auto opacity = FastJSON::getNumber< float >(data, Material::JKOpacity, 1.0F);

				if ( opacity < 1.0F )
				{
					newMaterial.setOpacity(opacity);
				}

				return newMaterial.setManualLoadSuccess(true);
			},
			0
		);

		if ( !this->setMaterial(materialResource) )
		{
			TraceError{ClassId} << "Unable to load sprite material '" << materialResource->name() << "' !";

			return this->setLoadSuccess(false);
		}

		const auto isAnimated = to_FillingType(FastJSON::getString(data, Material::JKType)) == FillingType::AnimatedTexture;
		const auto centerAtBottom = FastJSON::getBoolean(data, JKCenterAtBottomKey, false);
		const auto flip = FastJSON::getBoolean(data, JKFlipKey, false);

		if ( !this->prepareGeometry(isAnimated, centerAtBottom, flip) )
		{
			Tracer::error(ClassId, "Unable to get default Geometry to generate the default Sprite !");

			return this->setLoadSuccess(false);
		}

		m_size = FastJSON::getNumber< float >(data, JKSizeKey, 1.0F);

		return this->setLoadSuccess(true);
	}

	bool
	SpriteResource::load (const std::shared_ptr< Material::Interface > & material, bool centerAtBottom, bool flip, const RasterizationOptions & /*rasterizationOptions*/) noexcept
	{
		if ( !this->beginLoading() )
		{
			return false;
		}

		if ( !this->prepareGeometry(material->isAnimated(), centerAtBottom, flip) )
		{
			Tracer::error(ClassId, "Unable to get default Geometry to generate the default Sprite !");

			return this->setLoadSuccess(false);
		}

		/* 2. Check the materials. */
		if ( material == nullptr )
		{
			TraceError{ClassId} << "Unable to set material for sprite '" << this->name() << "' !";

			return this->setLoadSuccess(false);
		}

		this->setMaterial(material/*, rasterizationOptions, 0*/);

		return this->setLoadSuccess(true);
	}

	bool
	SpriteResource::prepareGeometry (bool isAnimated, bool centerAtBottom, bool flip) noexcept
	{
		const std::lock_guard< std::mutex > lock{s_lockGeometryLoading};

		std::stringstream resourceName;
		resourceName << "QuadSprite" << isAnimated << centerAtBottom << flip;

		int flags = Geometry::EnableNormal | Geometry::EnablePrimaryTextureCoordinates;

		if ( isAnimated )
		{
			flags |= Geometry::Enable3DPrimaryTextureCoordinates;
		}

		const auto geometryResource = Resources::Manager::instance()->container< Geometry::IndexedVertexResource >()->getOrCreateResource(
			resourceName.str(),
			[isAnimated, centerAtBottom, flip] (Geometry::IndexedVertexResource & newGeometry)
			{
				Shape< float, uint32_t > shape{2 * MaxFrames};

				ShapeBuilder< float, uint32_t > builder{shape};
				builder.beginConstruction(ConstructionMode::TriangleStrip);
				builder.options().enableGlobalNormal(Vector< 3, float >::positiveZ());

				const auto Ua = flip ? 1.0F : 0.0F;
				const auto Ub = flip ? 0.0F : 1.0F;

				const Vector< 3, float > positionA{-0.5F, centerAtBottom ? -1.0F : -0.5F, 0.0F};
				const Vector< 3, float > positionB{-0.5F, centerAtBottom ?  0.0F :  0.5F, 0.0F};
				const Vector< 3, float > positionC{ 0.5F, centerAtBottom ? -1.0F : -0.5F, 0.0F};
				const Vector< 3, float > positionD{ 0.5F, centerAtBottom ?  0.0F :  0.5F, 0.0F};

				if ( isAnimated )
				{
					for ( uint32_t frameIndex = 0; frameIndex < MaxFrames; frameIndex++ )
					{
						const auto depth = static_cast< float >(frameIndex);

						builder.newGroup();

						builder.setPosition(positionA);
						builder.setTextureCoordinates(Ua, 0.0F, depth);
						builder.newVertex();

						builder.setPosition(positionB);
						builder.setTextureCoordinates(Ua, 1.0F, depth);
						builder.newVertex();

						builder.setPosition(positionC);
						builder.setTextureCoordinates(Ub, 0.0F, depth);
						builder.newVertex();

						builder.setPosition(positionD);
						builder.setTextureCoordinates(Ub, 1.0F, depth);
						builder.newVertex();
					}
				}
				else
				{
					builder.newGroup();

					builder.setPosition(positionA);
					builder.setTextureCoordinates(Ua, 0.0F, 0.0F);
					builder.newVertex();

					builder.setPosition(positionB);
					builder.setTextureCoordinates(Ua, 1.0F, 0.0F);
					builder.newVertex();

					builder.setPosition(positionC);
					builder.setTextureCoordinates(Ub, 0.0F, 0.0F);
					builder.newVertex();

					builder.setPosition(positionD);
					builder.setTextureCoordinates(Ub, 1.0F, 0.0F);
					builder.newVertex();
				}

				builder.endConstruction();

				return newGeometry.load(shape);
			},
			flags
		);

		if ( geometryResource == nullptr )
		{
			TraceError{ClassId} << "Unable to get or create the geometry resource for sprite resource '" << this->name() << "'.";

			return false;
		}

		this->setReadyForInstantiation(false);

		m_geometry = geometryResource;

		return this->addDependency(m_geometry);
	}

	bool
	SpriteResource::setMaterial (const std::shared_ptr< Material::Interface > & materialResource) noexcept
	{
		if ( materialResource == nullptr )
		{
			TraceError{ClassId} <<
				"The material resource is null ! "
				"Unable to attach it to the renderable object '" << this->name() << "' " << this << ".";

			return false;
		}

		this->setReadyForInstantiation(false);

		m_material = materialResource;

		return this->addDependency(m_material);
	}

	bool
	SpriteResource::onDependenciesLoaded () noexcept
	{
		this->setReadyForInstantiation(true);

		return true;
	}
}
