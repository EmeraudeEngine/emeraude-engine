/*
 * src/Graphics/Renderable/SpriteResource.cpp
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

#include "SpriteResource.hpp"

/* Project configuration. */
#include "emeraude_base_config.hpp"

/* Local inclusions. */
#include "Resources/Container.hpp"
#include "FastJSON.hpp"
#include "Graphics/Geometry/IndexedVertexResource.hpp"
#include "Graphics/Material/StandardResource.hpp"
#include "Graphics/Material/Helpers.hpp"
#include "Graphics/TextureResource/AnimatedTexture2D.hpp"
#include "Graphics/TextureResource/Texture2D.hpp"
#include "VertexFactory/ShapeBuilder.hpp"
#include "Types.hpp"

namespace EmEn::Graphics::Renderable
{
	using namespace Base;
	using namespace Base::Math;
	using namespace Base::VertexFactory;
	using namespace Saphir;
	using namespace Saphir::Keys;

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

		if ( !this->setMaterial(this->serviceProvider().container< Material::StandardResource >()->getDefaultResource()) )
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

		const auto material = this->serviceProvider().container< Material::StandardResource >()
			->getOrCreateResource("SpriteMaterial" + this->name(), [&, data] (auto & materialResource) {
				if ( !data.isMember(Material::JKData) || !data[Material::JKData].isObject() )
				{
					TraceError{ClassId} << "The key '" << Material::JKData << "' JSON structure is not present or not an object !";

					return materialResource.setManualLoadSuccess(false);
				}

				const auto & componentData = data[Material::JKData];

				/* Check the texture resource type. */
				if ( const auto fillingType = Material::getFillingTypeFromJSON(data) )
				{
					switch ( fillingType.value() )
					{
						case FillingType::Texture :
						{
							const auto textureResource = this->serviceProvider().container< TextureResource::Texture2D >()
								->getResource(FastJSON::getValue< std::string >(componentData, Material::JKName)
								.value_or(Resources::Default));

							if ( !materialResource.setAlbedoComponent(textureResource, true) )
							{
								return materialResource.setManualLoadSuccess(false);
							}
						}
							break;

						case FillingType::AnimatedTexture :
						{
							const auto textureResource = this->serviceProvider().container< TextureResource::AnimatedTexture2D >()
								->getResource(FastJSON::getValue< std::string >(componentData, Material::JKName)
								.value_or(Resources::Default));

							if ( !materialResource.setAlbedoComponent(textureResource, true) )
							{
								return materialResource.setManualLoadSuccess(false);
							}
						}
							break;

						default:
							TraceError{ClassId} << "Unhandled material type (" << to_string(fillingType.value()) << ") for sprite !";

							return materialResource.setManualLoadSuccess(false);
					}
				}
				else
				{
					TraceError{ClassId} << "Undefined material type for sprite !";

					return materialResource.setManualLoadSuccess(false);
				}

				/* A sprite shows a PICTURE: it must never be lit. The ambient and light passes would
				 * add their own contribution on top of a texel that already IS the final colour. The
				 * unlit path writes `fragmentColor().rgb * emissionMultiplier()`, and that multiplier
				 * only exists when an AutoIllumination component is present: without it the sprite
				 * writes its raw [0,1] colour and reads black under the photometric exposure. So the
				 * component comes FIRST, then the flag; the amount and the emissive strength below
				 * only refine the luminance it carries. */
				materialResource.setAutoIlluminationComponent(1.0F);
				materialResource.enableUnlit();

				/* Check the blending mode. */
				materialResource.enableBlendingFromJson(data);

				/* Check the optional global auto-illumination amount. */
				if ( const auto autoIllumination = FastJSON::getValue< float >(data, Material::JKAutoIllumination).value_or(0.0F); autoIllumination > 0.0F )
				{
					materialResource.setAutoIlluminationAmount(autoIllumination);
				}

				/* Check the optional emissive strength — the PHOTOMETRIC half of the emission.
				 * The amount above is clamped to [0,1] and acts as the emissive MASK, so it
				 * cannot carry a brightness: `AutoIllumination: 1.0` alone emits exactly 1 nit,
				 * which is invisible next to any real light source. A self-illuminating sprite
				 * (a flame, an explosion, a neon sign) declares its LUMINANCE in cd/m^2 here,
				 * same key and same contract as StandardResource and as the glTF extension
				 * KHR_materials_emissive_strength. */
				if ( data.isMember(EmissiveStrengthString) )
				{
					materialResource.setEmissiveStrength(FastJSON::getValue< float >(data, EmissiveStrengthString).value_or(1.0F));
				}

				/* A sprite texture is intrinsically alpha-mapped, and that texture alpha keeps priority
				 * over the uniform opacity: it is sampled through the enableAlpha flag above. A global
				 * opacity below 1.0 additionally makes the whole sprite uniformly translucent (blending);
				 * left at the default 1.0 the sprite stays a binary CUTOUT at the historical 0.5 threshold
				 * instead of becoming a blended surface.
				 * Either way the material enters the RT alpha-test path — Material::Interface::isAlphaTest
				 * reads AlphaTestEnabled/BlendingEnabled — so sprites alpha-test against their texture at
				 * hit time and do appear in reflections. */
				if ( const auto opacity = FastJSON::getValue< float >(data, Material::JKOpacity).value_or(1.0F); opacity < 1.0F )
				{
					materialResource.setOpacityComponent(opacity);
				}

				if ( materialResource.blendingMode() == BlendingMode::None )
				{
					materialResource.enableAlphaTest(0.5F);
				}

				return materialResource.setManualLoadSuccess(true);
			}, 0);

		if ( !this->setMaterial(material) )
		{
			TraceError{ClassId} << "Unable to load sprite material '" << material->name() << "' !";

			return this->setLoadSuccess(false);
		}

		const auto isAnimated = Material::getFillingTypeFromJSON(data) == FillingType::AnimatedTexture;
		const auto centerAtBottom = FastJSON::getValue< bool >(data, JKCenterAtBottom).value_or(false);
		const auto flip = FastJSON::getValue< bool >(data, JKFlip).value_or(false);

		if ( !this->prepareGeometry(isAnimated, centerAtBottom, flip) )
		{
			Tracer::error(ClassId, "Unable to get default Geometry to generate the default Sprite !");

			return this->setLoadSuccess(false);
		}

		if ( data.isMember(JKUniformScale) )
		{
			this->setUniformScale(FastJSON::getValue< float >(data, JKUniformScale).value_or(1.0F));
		}

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
		const std::scoped_lock lock{s_lockGeometryLoading};

		std::stringstream resourceName;
		resourceName << "QuadSprite" << isAnimated << centerAtBottom << flip;

		uint32_t flags = Geometry::EnableNormal | Geometry::EnablePrimaryTextureCoordinates;

		if ( isAnimated )
		{
			flags |= Geometry::Enable3DPrimaryTextureCoordinates;
		}

		/* NOTE: Must be sync to get the geometry ASAP. */
		m_geometry = this->serviceProvider().container< Geometry::IndexedVertexResource >()
			->getOrCreateResource(resourceName.str(), [isAnimated, centerAtBottom, flip] (auto & geometryResource) {
				Shape< float, uint32_t > shape{2 * MaxFrames};

				ShapeBuilder< float, uint32_t > builder{shape};
				builder.beginConstruction(ConstructionMode::TriangleStrip);
				builder.options().enableGlobalNormal(Vector< 3, float >::positiveZ());

				const auto Ua = flip ? 1.0F : 0.0F;
				const auto Ub = flip ? 0.0F : 1.0F;

				/* ⚠️⚠️ AUTHORED FOR Y-UP. A and C are the BOTTOM edge, B and D the TOP one, and V is 1 at
				 * the bottom because Vulkan puts V=0 at the top of an image.
				 * Before the Y-up migration this read `centerAtBottom ? -1 : -0.5` for the bottom
				 * pair, with V=0 on it. That was correct while getSpriteModelMatrix() built its frame
				 * with a DOWNWARD Y column: model -Y then pointed to world UP, so the quad rose from
				 * its anchor. The migration renamed that column to UPWARD — preserving handedness,
				 * which is why no mirror test caught it — and the quad silently turned over: it hung
				 * BELOW its anchor and, with CenterAtBottom, a sprite anchored on the ground was
				 * buried under it. Invisible at any exposure, no error anywhere, the renderable
				 * present and drawn. It also rendered upside down, which the burial hid. */
				const Vector< 3, float > positionA{-0.5F, centerAtBottom ? 0.0F : -0.5F, 0.0F};
				const Vector< 3, float > positionB{-0.5F, centerAtBottom ? 1.0F :  0.5F, 0.0F};
				const Vector< 3, float > positionC{ 0.5F, centerAtBottom ? 0.0F : -0.5F, 0.0F};
				const Vector< 3, float > positionD{ 0.5F, centerAtBottom ? 1.0F :  0.5F, 0.0F};

				if ( isAnimated )
				{
					for ( uint32_t frameIndex = 0; frameIndex < MaxFrames; frameIndex++ )
					{
						const auto depth = static_cast< float >(frameIndex);

						builder.newGroup();

				/* ⚠️⚠️ EMISSION ORDER IS A, C, B, D — NOT A, B, C, D, and it is the SECOND half of the
				 * Y-up fallout. The strip's first triangle is emitted in the order given, so A, B, C
				 * yields (B-A) x (C-A) = -Z: a geometric front face opposite to the declared global
				 * normal (+Z). The billboard turns the frame's +Z toward the camera, so that front
				 * face looked away from it and back-face culling removed EVERY sprite, from every
				 * angle — the quad rotates with the camera, so no viewpoint could catch its far side.
				 * It survived for years because the pre-Y-up projection was MIRRORED, and a mirrored
				 * projection reverses on-screen winding: the wrong winding cancelled the mirror. The
				 * Y-up migration removed the mirror and the compensation became the defect, exactly
				 * like the loader winding swaps deleted at the time — this one just lives here.
				 * Measured, not reasoned: reverting to A, B, C, D makes every sprite vanish again.
				 * A, C, B, D covers the same two triangles with the winding matching the normal, and
				 * each vertex keeps its own UV so the mapping is untouched. */
						builder.setPosition(positionA);
						builder.setTextureCoordinates(Ua, 1.0F, depth);
						builder.newVertex();

						builder.setPosition(positionC);
						builder.setTextureCoordinates(Ub, 1.0F, depth);
						builder.newVertex();

						builder.setPosition(positionB);
						builder.setTextureCoordinates(Ua, 0.0F, depth);
						builder.newVertex();

						builder.setPosition(positionD);
						builder.setTextureCoordinates(Ub, 0.0F, depth);
						builder.newVertex();
					}
				}
				else
				{
					builder.newGroup();

					/* ⚠️ A, C, B, D — see the note in the animated branch above. */
					builder.setPosition(positionA);
					builder.setTextureCoordinates(Ua, 1.0F, 0.0F);
					builder.newVertex();

					builder.setPosition(positionC);
					builder.setTextureCoordinates(Ub, 1.0F, 0.0F);
					builder.newVertex();

					builder.setPosition(positionB);
					builder.setTextureCoordinates(Ua, 0.0F, 0.0F);
					builder.newVertex();

					builder.setPosition(positionD);
					builder.setTextureCoordinates(Ub, 0.0F, 0.0F);
					builder.newVertex();
				}

				builder.endConstruction();

				return geometryResource.load(shape);
			}, flags);

		this->setReadyForInstantiation(false);

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
		if constexpr ( IsDebug )
		{
			/* NOTE: Check the geometry resource. */
			if ( !this->geometry(0)->isCreated() )
			{
				TraceError{ClassId} << "The geometry for '" << this->name() << "' (" << this->classLabel() << ") is not created!";

				return false;
			}

			/* NOTE: Check material resource. */
			if ( !this->material(0)->isCreated() )
			{
				TraceError{ClassId} << "The material for '" << this->name() << "' (" << this->classLabel() << ") is not created!";

				return false;
			}
		}

		this->setReadyForInstantiation(true);

		return true;
	}
}
