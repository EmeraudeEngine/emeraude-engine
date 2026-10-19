/*
 * src/Scenes/Loaders/USDParser.cpp
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

#include "USDParser.hpp"

/* STL inclusions. */
#include <algorithm>
#include <array>
#include <cmath>
#include <iterator>
#include <limits>
#include <map>
#include <numbers>
#include <ranges>
#include <unordered_map>
#include <utility>

/* Third-party inclusions. */
#include "lightusd.hh"
#include "usdGeom.hh"
#include "usdLux.hh"
#include "composition.hh"
#include "asset-resolution.hh"
#include "tydra/render-data.hh"
#include "tydra/render-data-converter.hh"
#include "tydra/scene-access.hh"

/* Local inclusions. */
#include "BlobTrait.hpp"
#include "Math/Quaternion.hpp"
#include "Math/Space3D/AACuboid.hpp"
#include "Scenes/Loaders/SceneData.hpp"
#include "Tracer.hpp"

namespace EmEn::Scenes::Loaders::USD
{
	using namespace Base::Math;
	using namespace Base::PixelFactory;
	using namespace Base::VertexFactory;

	namespace
	{
		/* NOTE: The loader's name: the analysis reports under it, as before the split. */
		constexpr auto ClassId{"USDLoader"};

		/* Above this, a prim tree is noise rather than information. */
		constexpr size_t PrimTreeReportLimit{80};

		/** @brief Copies a USD matrix into plain storage. */
		[[nodiscard]]
		std::array< std::array< double, 4 >, 4 >
		toArray (const lightusd::value::matrix4d & matrix) noexcept
		{
			const auto & m = matrix.m;

			return {{
				{m[0][0], m[0][1], m[0][2], m[0][3]},
				{m[1][0], m[1][1], m[1][2], m[1][3]},
				{m[2][0], m[2][1], m[2][2], m[2][3]},
				{m[3][0], m[3][1], m[3][2], m[3][3]}
			}};
		}

		/**
		 * @brief Counters gathered while walking a composed stage.
		 */
		struct Inventory
		{
			std::map< std::string, size_t > primTypeCounts;
			size_t primCount{0};
			size_t maxDepth{0};
			size_t pointInstancerCount{0};
			size_t materialCount{0};
			size_t meshCount{0};
		};

		void
		collectInventory (const lightusd::Prim & root, size_t rootDepth, Inventory & inventory) noexcept
		{
			/* NOTE: an ITERATIVE walk (explicit stack, children pushed in reverse: the same order as the former recursion):
			 * the depth of a prim tree comes from the FILE, a recursion overflows the stack on a deep one (triad 2026-09-30). */
			std::vector< std::pair< const lightusd::Prim *, size_t > > pending{{&root, rootDepth}};

			while ( !pending.empty() )
			{
				const auto [primPointer, depth] = pending.back();
				pending.pop_back();

				const auto & prim = *primPointer;

				inventory.primCount++;
				inventory.maxDepth = std::max(inventory.maxDepth, depth);

				/* `prim_type_name()` carries the authored schema name ("Mesh", "Xform", …). It is empty
				 * for a typeless prim (a pure "over" or a bare def), which is itself worth counting. */
				auto typeName = prim.prim_type_name();

				if ( typeName.empty() )
				{
					typeName = prim.type_name();
				}

				if ( typeName.empty() )
				{
					typeName = "<typeless>";
				}

				inventory.primTypeCounts[typeName]++;

				if ( typeName == "PointInstancer" )
				{
					inventory.pointInstancerCount++;
				}
				else if ( typeName == "Mesh" )
				{
					inventory.meshCount++;
				}
				else if ( typeName == "Material" )
				{
					inventory.materialCount++;
				}

				for ( const auto & child : std::views::reverse(prim.children()) )
				{
					pending.emplace_back(&child, depth + 1);
				}
			}
		}

		void
		reportPrimTree (const lightusd::Prim & root, size_t rootDepth, size_t & remaining) noexcept
		{
			/* NOTE: an ITERATIVE walk (explicit stack, children pushed in reverse: the same order as the former recursion):
			 * the depth of a prim tree comes from the FILE, a recursion overflows the stack on a deep one (triad 2026-09-30). */
			std::vector< std::pair< const lightusd::Prim *, size_t > > pending{{&root, rootDepth}};

			while ( !pending.empty() )
			{
				const auto [primPointer, depth] = pending.back();
				pending.pop_back();

				const auto & prim = *primPointer;

				if ( remaining == 0 )
				{
					return;
				}

				remaining--;

				auto typeName = prim.prim_type_name();

				if ( typeName.empty() )
				{
					typeName = prim.type_name();
				}

				TraceInfo{ClassId} << std::string(depth * 2, ' ') << "/" << prim.element_name() << "  [" << ( typeName.empty() ? "<typeless>" : typeName ) << "]";

				for ( const auto & child : std::views::reverse(prim.children()) )
				{
					pending.emplace_back(&child, depth + 1);
				}
			}
		}

		void
		reportInventory (const std::string & sourceName, const lightusd::Stage & stage) noexcept
		{
			Inventory inventory;

			for ( const auto & prim : stage.root_prims() )
			{
				collectInventory(prim, 1, inventory);
			}

			const auto & metas = stage.metas();

			TraceInfo{ClassId} <<
				sourceName << " stage composed: " <<
				stage.root_prims().size() << " root prims, " <<
				inventory.primCount << " prims total, depth " << inventory.maxDepth << ", " <<
				inventory.meshCount << " meshes, " <<
				inventory.materialCount << " materials, " <<
				inventory.pointInstancerCount << " point instancers, " <<
				metas.subLayers.size() << " sublayers.";

			/* upAxis and metersPerUnit decide the whole conversion; endTimeCode says whether there
			 * is any animation at all. Reporting them removes three assumptions from every later
			 * diagnosis. */
			const auto upAxisName = [] (lightusd::Axis axis) -> const char * {
				switch ( axis )
				{
					case lightusd::Axis::X :
						return "X";

					case lightusd::Axis::Y :
						return "Y";

					case lightusd::Axis::Z :
						return "Z";

					default:
						return "<invalid>";
				}
			};

			TraceInfo{ClassId} <<
				"Stage metrics: upAxis " << upAxisName(metas.upAxis.get_value()) <<
				", metersPerUnit " << metas.metersPerUnit.get_value() <<
				", timeCodes " << metas.startTimeCode.get_value() << " to " << metas.endTimeCode.get_value() <<
				" at " << metas.timeCodesPerSecond.get_value() << " per second.";

			for ( const auto & [typeName, count] : inventory.primTypeCounts )
			{
				TraceInfo{ClassId} << "  " << count << " x " << typeName;
			}

			/* Bounded on purpose: an element is worth dumping whole, a full stage is not. */
			if ( inventory.primCount <= PrimTreeReportLimit )
			{
				auto remaining = PrimTreeReportLimit;

				for ( const auto & prim : stage.root_prims() )
				{
					reportPrimTree(prim, 0, remaining);
				}
			}
		}

		void
		collectEnvironmentLights (const lightusd::Prim & root, SceneData & output) noexcept
		{
			/* NOTE: an ITERATIVE walk (explicit stack, children pushed in reverse: the same order as the former recursion):
			 * the depth of a prim tree comes from the FILE, a recursion overflows the stack on a deep one (triad 2026-09-30). */
			std::vector< const lightusd::Prim * > pending{&root};

			while ( !pending.empty() )
			{
				const auto & prim = *pending.back();
				pending.pop_back();

				/* ⚠️ Tydra's RenderLight carries colour, intensity and exposure but NOT the dome's image
				 * path, so the DomeLight prim is read directly. Without this the asset's own sky — the
				 * 8K HDR that produces every reference render Intel ships — is silently dropped, and the
				 * scene ends up lit by whatever generic sky the demo installed instead. */
				if ( const auto * domeLight = prim.as< lightusd::DomeLight >(); domeLight != nullptr )
				{
					LightDescriptor descriptor;
					descriptor.name = prim.element_name();
					descriptor.type = LightType::Environment;

					float intensity = 1.0F;
					float exposure = 0.0F;

					if ( const auto value = domeLight->intensity.get_value(); value.has_value() )
					{
						float scalar = 1.0F;

						if ( value.get_scalar(&scalar) )
						{
							intensity = scalar;
						}
					}

					if ( const auto value = domeLight->exposure.get_value(); value.has_value() )
					{
						float scalar = 0.0F;

						if ( value.get_scalar(&scalar) )
						{
							exposure = scalar;
						}
					}

					/* USD folds exposure into the intensity as a power of two, exactly like a stop. */
					descriptor.intensity = intensity * std::pow(2.0F, exposure);

					if ( const auto value = domeLight->file.get_value(); value.has_value() )
					{
						lightusd::value::AssetPath assetPath;

						if ( value.value().get_scalar(&assetPath) )
						{
							const auto & rawPath = assetPath.GetAssetPath();

							/* NOTE: The RAW authored path: the loader resolves it against the stage directory (a file system
							 * access, never part of the analysis). */
							descriptor.textureAssetPath = rawPath;
						}
					}

					output.lights.emplace_back(std::move(descriptor));
				}

				for ( const auto & child : std::views::reverse(prim.children()) )
				{
					pending.push_back(&child);
				}
			}
		}

		void
		collectInstancers (const lightusd::Prim & root, const std::string & rootPath, float metersPerUnit, std::vector< Instancer > & instancers) noexcept
		{
			/* NOTE: an ITERATIVE walk (explicit stack, children pushed in reverse: the same order as the former recursion):
			 * the depth of a prim tree comes from the FILE, a recursion overflows the stack on a deep one (triad 2026-09-30). */
			std::vector< std::pair< const lightusd::Prim *, std::string > > pending{{&root, rootPath}};

			while ( !pending.empty() )
			{
				const auto entry = std::move(pending.back());
				pending.pop_back();

				const auto & prim = *entry.first;
				const auto & primPath = entry.second;

				using namespace Base::Math;

				if ( const auto * pointInstancer = prim.as< lightusd::GeomPointInstancer >(); pointInstancer != nullptr )
				{
					Instancer instancer;
					instancer.path = primPath;

					/* ⚠️ AXIS BAKE, INSTANCE EDITION. The vertices of a prototype went through
					 *
					 *     C : engine = (usd.x, usd.z, -usd.y)
					 *
					 * in buildMeshes(). An instance carries a TRANSFORM, not a point, so it cannot be fed
					 * through C directly: a transform changes basis by CONJUGATION, C·T·C⁻¹. Applied to an
					 * already-baked prototype vertex, that reproduces the source placement exactly:
					 *
					 *     C·(T·p) = (C·T·C⁻¹)·(C·p)
					 *
					 * C is the rotation of +90° about X (it sends y to z and z to -y, determinant +1), so
					 * the conjugation has a closed form on each part of the transform, and no matrix is
					 * needed:
					 *   - a translation is simply carried through C;
					 *   - a rotation quaternion is conjugated by C's own quaternion;
					 *   - a scale, being sign-blind, has its Y and Z factors SWAPPED.
					 *
					 * Skipping the conjugation and merely permuting the position is the trap: the forest
					 * lands in the right places, and every plant is rotated wrong — which reads as a bad
					 * asset rather than a bad conversion. */
					/* ⚠️ The axis flip is COMPOSED INTO the bake — one place, so an instance can never end
					 * up mirrored differently from the prototype it draws. */
					const auto bakePosition = [metersPerUnit] (const auto & position) {
						return Vector< 3, float >{static_cast< float >(position[0]) * metersPerUnit, static_cast< float >(position[2]) * metersPerUnit, -static_cast< float >(position[1]) * metersPerUnit};
					};

					/* The quaternion of C, a +90° rotation about X: (sin(45°), 0, 0, cos(45°)). */
					const Quaternion< float > axisChange{std::numbers::sqrt2_v< float > / 2.0F, 0.0F, 0.0F, std::numbers::sqrt2_v< float > / 2.0F};
					const auto axisChangeInverse = axisChange.conjugated();

					std::vector< lightusd::value::point3f > positions;
					std::vector< lightusd::value::quath > orientations;
					std::vector< lightusd::value::float3 > scales;

					if ( const auto value = pointInstancer->positions.get_value(); value.has_value() )
					{
						if ( !value.value().get_scalar(&positions) )
						{
							TraceWarning{ClassId} << "Instancer '" << primPath << "' has time-sampled positions, which are not read.";
						}
					}

					if ( const auto value = pointInstancer->protoIndices.get_value(); value.has_value() )
					{
						std::vector< int32_t > indices;

						if ( value.value().get_scalar(&indices) )
						{
							instancer.prototypeIndices = std::move(indices);
						}
					}

					if ( const auto value = pointInstancer->orientations.get_value(); value.has_value() )
					{
						(void)value.value().get_scalar(&orientations);
					}

					if ( const auto value = pointInstancer->scales.get_value(); value.has_value() )
					{
						(void)value.value().get_scalar(&scales);
					}

					/* The prototypes relationship is the ONLY thing tying an instance to what it draws:
					 * `protoIndices[i]` indexes INTO this target list, not into anything global. */
					if ( pointInstancer->prototypes.has_value() )
					{
						const auto & relation = pointInstancer->prototypes.value();

						if ( relation.is_path() )
						{
							instancer.prototypePaths.emplace_back(relation.targetPath.full_path_name());
						}
						else if ( relation.is_pathvector() )
						{
							for ( const auto & target : relation.targetPathVector )
							{
								instancer.prototypePaths.emplace_back(target.full_path_name());
							}
						}
					}

					instancer.instances.reserve(positions.size());

					for ( size_t index = 0; index < positions.size(); ++index )
					{
						auto rotation = Quaternion< float >{};

						if ( index < orientations.size() )
						{
							/* USD stores an orientation as a HALF-precision quaternion, real part first.
							 * The engine's constructor takes (x, y, z, w). */
							const auto & source = orientations[index];

							const Quaternion< float > sourceRotation{
								lightusd::value::half_to_float(source.imag[0]),
								lightusd::value::half_to_float(source.imag[1]),
								lightusd::value::half_to_float(source.imag[2]),
								lightusd::value::half_to_float(source.real)
							};

							rotation = axisChange * sourceRotation * axisChangeInverse;
							rotation.normalize();
						}

						auto scale = Vector< 3, float >{1.0F, 1.0F, 1.0F};

						if ( index < scales.size() )
						{
							scale[0] = scales[index][0];
							scale[1] = scales[index][2];
							scale[2] = scales[index][1];
						}

						/* ⚠️ The instance rotation is conjugated by the flip AFTER the USD axis change, and
						 * the scale is left alone (M·diag(s)·M = diag(s)). Mirroring the position without
						 * the rotation puts every plant in the right spot facing the wrong way. */
						instancer.instances.emplace_back(CartesianFrame< float >::fromQuaternion(bakePosition(positions[index]), rotation, scale));
					}

					if ( !instancer.instances.empty() )
					{
						instancers.emplace_back(std::move(instancer));
					}
					else
					{
						TraceWarning{ClassId} << "Instancer '" << primPath << "' declares no usable position.";
					}
				}

				for ( const auto & child : std::views::reverse(prim.children()) )
				{
					pending.emplace_back(&child, primPath + "/" + child.element_name());
				}
			}
		}

		void
		collectLightPlacements (const lightusd::tydra::XformNode & root, std::map< std::string, LightPlacement > & placements) noexcept
		{
			/* NOTE: an ITERATIVE walk (explicit stack, children pushed in reverse: the same order as the former recursion):
			 * the depth of a prim tree comes from the FILE, a recursion overflows the stack on a deep one (triad 2026-09-30). */
			std::vector< const lightusd::tydra::XformNode * > pending{&root};

			while ( !pending.empty() )
			{
				const auto & node = *pending.back();
				pending.pop_back();

				/* Every node is recorded, not just the lights: the map is keyed by absolute prim path and the
				 * caller looks up whatever Tydra reports, so filtering by prim type here would only be a
				 * second place to keep in sync with UsdLux. A whole stage is a few thousand entries. */
				const auto & world = node.get_world_matrix();

				/* ⚠️ USD is a ROW-VECTOR convention: the translation sits in the LAST ROW (m[3][0..2]) and the
				 * local basis vectors are the ROWS, not the columns. Reading it column-major transposes the
				 * rotation — which does not fail, it just aims every light somewhere plausible and wrong. */
				LightPlacement placement;

				placement.position = {
					static_cast< float >(world.m[3][0]),
					static_cast< float >(world.m[3][1]),
					static_cast< float >(world.m[3][2])
				};

				/* ⚠️ A UsdLux light emits along its own LOCAL -Z. That is the specification for DiskLight,
				 * RectLight, DistantLight and the shaping cone alike — not -Y, and not the stage's up axis
				 * (this stage is Z-up, which makes the two easy to confuse). */
				placement.direction = {
					-static_cast< float >(world.m[2][0]),
					-static_cast< float >(world.m[2][1]),
					-static_cast< float >(world.m[2][2])
				};

				placements.emplace(node.absolute_path.full_path_name(), placement);

				for ( const auto & child : std::views::reverse(node.children) )
				{
					pending.push_back(&child);
				}
			}
		}

		size_t
		buildLights (const lightusd::tydra::RenderScene & renderScene, float metersPerUnit, const std::map< std::string, LightPlacement > & placements, SceneData & output) noexcept
		{
			using namespace Base::Math;

			/* ⚠️ THE ANCHOR. See the header for what was measured and why this factor and not another.
			 * `intensity` is read as a luminance in cd/m², scaled by the emitter's area because
			 * `normalize = false` is what every light of this asset declares, then normalized by 4*pi. */
			const auto toCandela = [] (const lightusd::tydra::RenderLight & light, float area) {
				return light.intensity * std::pow(2.0F, light.exposure) * area / (4.0F * std::numbers::pi_v< float >);
			};

			size_t translated = 0;
			size_t unplaced = 0;

			for ( const auto & renderLight : renderScene.lights )
			{
				LightDescriptor descriptor;
				descriptor.name = renderLight.name.empty() ? "usd-light-" + std::to_string(translated) : renderLight.name;
				descriptor.color = {renderLight.color[0], renderLight.color[1], renderLight.color[2], 1.0F};

				/* ⚠️⚠️ THE PLACEMENT COMES FROM THE XFORM TREE, NEVER FROM `renderLight`. Tydra's light
				 * converter fills seventeen fields — colour, intensity, exposure, temperature, cone,
				 * shadows — and touches `transform`, `position` and `direction` in NO code path at all
				 * (`render-light-converter.cc` contains zero occurrences of the three words). They keep
				 * the struct's defaults: position (0,0,0) and direction (0,-1,0).
				 *
				 * Measured on WorldLobby.usdz: 29 fixtures read at their correct 3750 cd, all stacked at
				 * the world ORIGIN twenty metres from the room they belong to, aiming horizontally once
				 * baked. The frame came out PURE BLACK with a light set reporting 25 spots and 4 points,
				 * and not one warning anywhere — the loader had faithfully read fields nobody wrote.
				 *
				 * Joined on `abs_path`, which the converter DOES fill and which is unique by
				 * construction. The element name is NOT usable: all 25 ceiling fixtures of this asset are
				 * named "LightBloomDisc". */
				const auto placement = placements.find(renderLight.abs_path);

				if ( placement == placements.cend() )
				{
					/* Reported, never guessed: a light silently left at the origin is precisely the defect
					 * described above, and it must not be reintroduced by a failed lookup. */
					unplaced++;

					TraceWarning{ClassId} <<
						"Light '" << renderLight.abs_path << "' has no entry in the stage's xform tree; "
						"it is dropped rather than placed at the origin.";

					continue;
				}

				/* Radii and lengths are LENGTHS: they go through `metersPerUnit` like a position. */
				const auto radius = renderLight.radius * metersPerUnit;

				switch ( renderLight.type )
				{
					/* A dome has an image and no position. It is collected as an Environment by
					 * collectEnvironmentLights(), which also reads the image path Tydra drops. */
					case lightusd::tydra::RenderLight::Type::Dome :
					case lightusd::tydra::RenderLight::Type::Portal :
						continue;

					case lightusd::tydra::RenderLight::Type::Distant :
						descriptor.type = LightType::Directional;
						/* A directional light carries an ILLUMINANCE in lux and has no area — the
						 * 4*pi normalisation of a punctual emitter does not apply to a parallel beam. */
						descriptor.intensity = renderLight.intensity * std::pow(2.0F, renderLight.exposure);
						break;

					case lightusd::tydra::RenderLight::Type::Point :
					case lightusd::tydra::RenderLight::Type::Sphere :
						descriptor.type = LightType::Point;
						/* Sphere: the full 4*pi*r^2. */
						descriptor.intensity = toCandela(renderLight, 4.0F * std::numbers::pi_v< float > * radius * radius);
						break;

					case lightusd::tydra::RenderLight::Type::Disk :
					case lightusd::tydra::RenderLight::Type::Rect :
					case lightusd::tydra::RenderLight::Type::Cylinder :
					case lightusd::tydra::RenderLight::Type::Geometry :
					{
						const auto area = renderLight.type == lightusd::tydra::RenderLight::Type::Rect
							? renderLight.width * metersPerUnit * renderLight.height * metersPerUnit
							: std::numbers::pi_v< float > * radius * radius;

						/* ⚠️ A cone narrower than the full hemisphere makes it a SPOT, and the engine
						 * needs both angles. USD carries one angle plus a SOFTNESS in [0,1]; the inner
						 * cone is that softness eating into the outer one. A softness of 0 therefore
						 * gives a hard edge (inner == outer), which is what these fixtures declare. */
						if ( renderLight.shapingConeAngle < 180.0F )
						{
							descriptor.type = LightType::Spot;
							descriptor.outerConeAngle = renderLight.shapingConeAngle;
							descriptor.innerConeAngle = renderLight.shapingConeAngle * (1.0F - std::clamp(renderLight.shapingConeSoftness, 0.0F, 1.0F));
						}
						else
						{
							descriptor.type = LightType::Point;
						}

						descriptor.intensity = toCandela(renderLight, area);
					}
						break;
				}

				output.lights.emplace_back(descriptor);

				/* ⚠️ A LIGHT WITHOUT A NODE IS NEVER INSTANTIATED. The descriptor carries no transform
				 * on purpose — the placement belongs to the hierarchy — so a node must be emitted and
				 * declared a root, exactly as buildMeshes() does for a mesh. Forgetting this yields a
				 * scene whose light list is full and whose image is black, with nothing reported. */
				NodeDescriptor node;
				node.name = "usd:light/" + descriptor.name + "-" + std::to_string(output.lights.size() - 1);
				node.lightIndex = output.lights.size() - 1;

				/* Same bake as the geometry — engine = (x, z, -y) * metersPerUnit. A light placed with
				 * any other convention lands somewhere else than the room it is meant to light. */
				node.localFrame.setPosition(placement->second.position[0] * metersPerUnit,
					placement->second.position[2] * metersPerUnit,
					-placement->second.position[1] * metersPerUnit);

				/* A spot and a directional light AIM. The direction goes through the direction bake —
				 * no `metersPerUnit`, a direction has no length to convert. */
				if ( descriptor.type == LightType::Spot || descriptor.type == LightType::Directional )
				{
					/* ⚠️ The aim is mirrored with the placement: a light moved but not re-aimed lights
					 * the wrong wall, and nothing reports it. */
					const auto direction = Vector< 3, float >{
						placement->second.direction[0],
						placement->second.direction[2],
						-placement->second.direction[1]};

					if ( !direction.isZero() )
					{
						/* A frame is oriented toward a TARGET, so the direction becomes a point one
						 * unit ahead of the light. */
						node.localFrame.lookAt(node.localFrame.position() + direction.normalized(), false);
					}
				}

				output.nodes.emplace_back(std::move(node));
				output.rootNodeIndices.push_back(output.nodes.size() - 1);

				translated++;
			}

			if ( translated > 0 )
			{
				TraceInfo{ClassId} << translated << " punctual light(s) translated and placed.";
			}

			if ( unplaced > 0 )
			{
				TraceWarning{ClassId} << unplaced << " light(s) dropped for want of a placement — see the warnings above.";
			}

			return translated;
		}

		std::vector< Material >
		describeMaterials (const lightusd::tydra::RenderScene & renderScene, const std::string & resourcePrefix) noexcept
		{
			std::vector< Material > materials;
			materials.reserve(renderScene.materials.size());

			/* The asset identifier of a texture slot, as Tydra reports it (empty: no usable texture). The loader resolves
			 * it — out of the archive, or next to the stage — and gives it the colour space of its slot. */
			const auto textureIdentifier = [&renderScene] (int32_t textureId) -> std::string {
				if ( textureId < 0 || static_cast< size_t >(textureId) >= renderScene.textures.size() )
				{
					return {};
				}

				const auto imageId = renderScene.textures[static_cast< size_t >(textureId)].texture_image_id;

				if ( imageId < 0 || static_cast< size_t >(imageId) >= renderScene.images.size() )
				{
					return {};
				}

				return renderScene.images[static_cast< size_t >(imageId)].asset_identifier;
			};

			size_t materialIndex = 0;

			for ( const auto & renderMaterial : renderScene.materials )
			{
				const auto materialName = resourcePrefix + "/material/" + ( renderMaterial.name.empty() ? std::to_string(materialIndex) : renderMaterial.name ) + "-" + std::to_string(materialIndex);

				/* ⚠️ `getOrCreateResource()` runs this factory ON THE THREAD POOL. Every capture must
				 * be BY VALUE: a reference to `renderMaterial` — owned by the render scene, itself a
				 * local of load() — dangles the moment load() returns, and the crash lands far from
				 * here. The textures are therefore resolved BEFORE the lambda, on this thread, and
				 * only the resulting shared pointers and plain values cross over. */
				const auto & shaderSource = renderMaterial.surfaceShader;

				auto albedoTexture = shaderSource.has_value() ? textureIdentifier(shaderSource.value().diffuseColor.texture_id) : std::string{};
				auto roughnessTexture = shaderSource.has_value() ? textureIdentifier(shaderSource.value().roughness.texture_id) : std::string{};
				auto metalnessTexture = shaderSource.has_value() ? textureIdentifier(shaderSource.value().metallic.texture_id) : std::string{};
				auto normalTexture = shaderSource.has_value() ? textureIdentifier(shaderSource.value().normal.texture_id) : std::string{};

				/* ⚠️ OPACITY IS PART OF THE SURFACE, and ignoring it does not make glass "a bit wrong":
				 * a UsdPreviewSurface pane declares `inputs:diffuseColor = (0, 0, 0)` with
				 * `inputs:opacity = 0` — black BECAUSE it is meant to be seen through. Read as an opaque
				 * material, a whole curtain wall renders as a solid BLACK BLOCK, and the failure reads as
				 * a lighting or a material bug rather than a missing input.
				 *
				 * Measured on World Lobby: exactly 6 materials carry opacity 0 (`Glass` ×4,
				 * `Clear_Glass`, `Tinted_Glass`), each with a real `ior` (1.20 to 1.52); all 135 others
				 * are opaque. So this is six materials deciding whether the building has windows.
				 *
				 * ⚠️ `opacityThreshold` FIRST: a non-zero threshold is a CUTOUT — an alpha TEST that
				 * keeps the material opaque (opaque list, depth write, no sorting), never blending.
				 * Reaching for blending there would defeat the flag and cost sorting for nothing.
				 *
				 * NOT read yet, and deliberately: `ior`. Refraction and its Fresnel belong to the
				 * transmission path (`setTransmissionComponent()`), which is a separate calibration —
				 * mapping opacity to plain alpha blending already turns the black block into glazing. */
				const auto opacityValue = shaderSource.has_value() ? std::clamp(shaderSource.value().opacity.value, 0.0F, 1.0F) : 1.0F;
				const auto opacityThreshold = shaderSource.has_value() ? shaderSource.value().opacityThreshold.value : 0.0F;
				const auto isCutout = opacityThreshold > 0.0F;
				const auto isTranslucent = !isCutout && opacityValue < 1.0F;

				const auto albedoValue = shaderSource.has_value()
					? Color< float >{shaderSource.value().diffuseColor.value[0], shaderSource.value().diffuseColor.value[1], shaderSource.value().diffuseColor.value[2], opacityValue}
					: Color< float >{0.5F, 0.5F, 0.5F, 1.0F};
				const auto roughnessValue = shaderSource.has_value() ? shaderSource.value().roughness.value : 0.5F;
				const auto metalnessValue = shaderSource.has_value() ? shaderSource.value().metallic.value : 0.0F;

				if ( isTranslucent || isCutout )
				{
					TraceInfo{ClassId} <<
						"Material '" << materialName << "' is " << ( isCutout ? "a CUTOUT (alpha test)" : "TRANSLUCENT" ) <<
						" — opacity " << opacityValue << ", threshold " << opacityThreshold << ".";
				}

				materials.push_back(Material{
					.resourceName = materialName,
					.albedoTexture = std::move(albedoTexture),
					.roughnessTexture = std::move(roughnessTexture),
					.metalnessTexture = std::move(metalnessTexture),
					.normalTexture = std::move(normalTexture),
					.albedoValue = albedoValue,
					.roughnessValue = roughnessValue,
					.metalnessValue = metalnessValue,
					.opacityValue = opacityValue,
					.opacityThreshold = opacityThreshold,
					.isTranslucent = isTranslucent,
					.isCutout = isCutout
				});

				materialIndex++;
			}

			return materials;
		}

		void
		collectDrawList (const lightusd::tydra::RenderScene & renderScene, Asset & asset) noexcept
		{
			const auto & prototypePaths = asset.prototypePaths;

			/* ⚠️ The node hierarchy is where a mesh gets its PLACE and its SIZE. Tydra pre-computes
			 * `global_matrix` for every node, so nothing has to be accumulated here — but ignoring
			 * it altogether leaves every mesh at the origin, at its authoring scale. Measured on
			 * Jungle Ruins, that turned the pyramid into a speck a few pixels across.
			 *
			 * A mesh referenced by several nodes is therefore built once per node: same geometry,
			 * different placement. */
			/* ⚠️ THREE KINDS OF MESH COME OUT OF TYDRA, and only one of them is drawn where it stands.
			 *
			 * 1. `/_class_/…` — USD CLASSES. Abstract templates that exist to be inherited from; they
			 *    are not scene content. Tydra converts them like anything else, and drawing them puts
			 *    a full copy of every prototype at the asset's origin, on top of each other. Measured
			 *    on one Anthurium element: 12 meshes out of Tydra for 6 real prototypes.
			 * 2. A PROTOTYPE subtree of a PointInstancer — built, kept, but NEVER given a node: it is
			 *    drawn by its instances, and a node here would draw it one extra time, alone, at
			 *    whatever place the asset stores its prototypes.
			 * 3. Everything else — ordinary geometry, which gets its node as usual.
			 *
			 * Whether a class ends up drawn is not a matter of taste: `_class_` is USD's own marker
			 * for "not renderable", and honouring it is part of reading the format. */
			auto & drawList = asset.drawList;

			size_t skippedClassCount = 0;

			std::map< std::string, lightusd::value::matrix4d > prototypeRootMatrices;

			/* NOTE: an ITERATIVE walk (explicit stack, children pushed in reverse: the same order as the former recursion):
			 * the depth of the node tree comes from the FILE (triad 2026-09-30). */
			std::vector< const lightusd::tydra::Node * > pending;

			for ( const auto & rootNode : std::views::reverse(renderScene.nodes) )
			{
				pending.push_back(&rootNode);
			}

			while ( !pending.empty() )
			{
				const auto & node = *pending.back();
				pending.pop_back();

				if ( node.abs_path.starts_with("/_class_") )
				{
					skippedClassCount++;

					continue;
				}

				if ( std::ranges::find(prototypePaths, node.abs_path) != prototypePaths.end() )
				{
					prototypeRootMatrices.emplace(node.abs_path, node.global_matrix);
				}

				if ( node.nodeType == lightusd::tydra::NodeType::Mesh && node.id >= 0 )
				{
					drawList.push_back(Placement{.meshIndex = static_cast< size_t >(node.id), .worldMatrix = toArray(node.global_matrix), .primPath = node.abs_path});
				}

				for ( const auto & child : std::views::reverse(node.children) )
				{
					pending.push_back(&child);
				}
			}

			/* ⚠️ A prototype's own placement is NOT its instances' placement. Its mesh is baked in the
			 * prototype's local space and every instance then positions it; folding the prototype's
			 * world matrix in as well would offset the entire forest by wherever the asset happens to
			 * park its prototypes.
			 *
			 * Tydra only hands out ABSOLUTE matrices, so this pass assumes a prototype root sits at
			 * the identity — true of every element of Jungle Ruins. Rather than trust that silently,
			 * the assumption is CHECKED: a prototype root carrying a transform says so in the log
			 * instead of producing a forest quietly shifted off the terrain. */
			for ( const auto & [primPath, matrix] : prototypeRootMatrices )
			{
				bool isIdentity = true;

				for ( size_t row = 0; row < 4 && isIdentity; ++row )
				{
					for ( size_t column = 0; column < 4; ++column )
					{
						const auto expected = ( row == column ) ? 1.0 : 0.0;

						if ( std::abs(matrix.m[row][column] - expected) > 1e-5 )
						{
							isIdentity = false;

							break;
						}
					}
				}

				if ( !isIdentity )
				{
					TraceWarning{ClassId} <<
						"Prototype '" << primPath << "' carries a transform of its own, which this pass does not "
						"factor out: its instances will be offset by it.";
				}
			}

			TraceInfo{ClassId} <<
				drawList.size() << " mesh placements collected from the node hierarchy (" <<
				skippedClassCount << " class subtrees skipped, " << prototypePaths.size() << " instancer prototypes declared).";

		}
	}

	void
	setLoadOptions (lightusd::USDLoadOptions & options, uint64_t sourceBytes) noexcept
	{
		const auto limit = sourceBytes > std::numeric_limits< size_t >::max() / MemoryLimitPerFileByte
			? std::numeric_limits< size_t >::max()
			: std::max(MinimumMemoryLimit, static_cast< size_t >(sourceBytes) * MemoryLimitPerFileByte);

		/* NOTE: lightusd counts in MiB, in an int32_t. */
		options.max_memory_limit_in_mb = static_cast< int32_t >(std::min< size_t >(limit / (1024ULL * 1024ULL), std::numeric_limits< int32_t >::max()));
	}

	Asset::Asset () noexcept = default;

	Asset::~Asset () = default;

	Asset::Asset (Asset &&) noexcept = default;

	Asset & Asset::operator= (Asset &&) noexcept = default;

	bool
	composeStage (lightusd::AssetResolutionResolver & resolver, lightusd::Layer & rootLayer, bool resolveReferences, const std::string & sourceName, lightusd::Stage & stage, StageMetrics & metrics) noexcept
	{
		std::string warning;
		std::string error;

		lightusd::Layer sublayered;

		lightusd::SublayersCompositionOptions sublayerOptions;
		sublayerOptions.allow_parent_relative_paths = true;

		if ( !lightusd::CompositeSublayers(resolver, rootLayer, &sublayered, &warning, &error, sublayerOptions) )
		{
			TraceError{ClassId} << "Unable to composite sublayers of '" << sourceName << "' : " << error;

			return false;
		}

		if ( !warning.empty() )
		{
			TraceWarning{ClassId} << "While compositing the sublayers of '" << sourceName << "' : " << warning;
			warning.clear();
		}

		/* ⚠️ `CompositeAllArcs()` is DELIBERATELY not called here (owner decision, 2026-08-08).
		 * It resolves references, payloads, inherits and variants EAGERLY, in one pass. On Jungle
		 * Ruins that means ingesting 450 MB of ASCII prototype layers before anything can be
		 * drawn: measured at 24 minutes and 15 GB of resident memory, growing linearly with no
		 * convergence in sight — a cost that never amortises.
		 *
		 * Sublayer composition alone completes in seconds and already yields the whole
		 * non-instanced scene (84 meshes, 87 materials on that asset). Prototype references are
		 * therefore resolved by US, per element, when the element is actually needed — which is
		 * the deferred-loading strategy this project chose from the start.
		 *
		 * Consequence to keep in mind: `inherits` and `variants` are NOT applied on this path.
		 * Variant selection through LoaderOptions lands with the on-demand resolver, not before. */
		auto composited = std::make_unique< lightusd::Layer >();

		if ( resolveReferences )
		{
			lightusd::AllArcsCompositionOptions arcOptions;
			arcOptions.references.allow_parent_relative_paths = true;
			arcOptions.payload.allow_parent_relative_paths = true;

			/* ⚠️ The parsed-layer cache (tinyusdz 73cf5f680, "owned by the caller and kept alive across the whole
			 * fixed-point loop"). Without it every reference arc re-reads and re-parses its file: JungleRuins'
			 * QueenForest read its 248 MB `queenforest_classes.usda` about 190 times — 46 GB in 145 s, 87 % of the
			 * scene's load with RiverForest (measured 2026-10-04 from /proc/<pid>/io). Copy-on-write values also
			 * make every referencing prim SHARE the parsed arrays. */
			std::map< std::string, lightusd::Layer > layerCache;
			arcOptions.references.layer_cache = &layerCache;
			arcOptions.payload.layer_cache = &layerCache;

			/* ⚠️ ONE `CompositeAllArcs()` pass is not a composition: it applies references BEFORE payloads, so an arc
			 * that a payload brings in (a Kit export's `prepend references` inside the payloaded `assets_*.usd`
			 * layers) is left unresolved. tinyusdz's own driver (examples/tusdcat) repeats it until no arc is left
			 * unresolved; so does this one, bounded (an asset that cannot be found keeps its arc forever). */
			constexpr uint32_t MaxCompositionPasses{16};

			const auto hasUnresolvedArcs = [] (const lightusd::Layer & candidate) {
				return candidate.check_unresolved_references() || candidate.check_unresolved_payload() || candidate.check_unresolved_inherits() || candidate.check_unresolved_variant() || candidate.check_unresolved_specializes();
			};

			auto current = std::move(sublayered);
			uint32_t pass = 0;

			while ( pass < MaxCompositionPasses && hasUnresolvedArcs(current) )
			{
				lightusd::Layer next;

				if ( !lightusd::CompositeAllArcs(resolver, current, &next, &warning, &error, arcOptions) )
				{
					TraceError{ClassId} << "Unable to composite the arcs of '" << sourceName << "' (pass " << pass + 1 << ") : " << error;

					return false;
				}

				if ( !warning.empty() )
				{
					TraceWarning{ClassId} << "While compositing the arcs of '" << sourceName << "' (pass " << pass + 1 << ") : " << warning;
					warning.clear();
				}

				current = std::move(next);
				++pass;
			}

			if ( hasUnresolvedArcs(current) )
			{
				TraceWarning{ClassId} << "'" << sourceName << "' still has unresolved arcs after " << pass << " composition passes (a missing asset, or a deeper nesting).";
			}

			*composited = std::move(current);
		}
		else
		{
			*composited = std::move(sublayered);
		}

		/* The stage metrics must be read BEFORE the layer is consumed by the converter. */
		const auto & metas = composited->metas();
		const auto upAxis = metas.upAxis.get_value();
		metrics.metersPerUnit = static_cast< float >(metas.metersPerUnit.get_value());
		/* NOTE: copied here too — the log at the end runs after the converter consumed the layer, and read
		 * garbage there (denormals, "0 to inf") until 2026-10-04. */
		metrics.startTimeCode = metas.startTimeCode.get_value();
		metrics.endTimeCode = metas.endTimeCode.get_value();
		metrics.zUp = upAxis == lightusd::Axis::Z;

		if ( upAxis != lightusd::Axis::Z )
		{
			TraceWarning{ClassId} << "'" << sourceName << "' declares a non-Z up axis; the bake below assumes Z-up and will be wrong.";
		}

		/* ⚠️ Two Tydra front-ends exist and they are NOT equivalent:
		 *   - `LayerToRenderSceneConverter` works from a Layer. Its in-place entry point is
		 *     refused at runtime ("destructive source transfer is not implemented safely yet"),
		 *     and its plain entry point returns the node hierarchy with ZERO meshes. Experimental.
		 *   - `RenderSceneConverter` works from a Stage and is the mature path. It is the one used.
		 *
		 * So the layer is turned into a Stage first, with the in-place variant that frees each
		 * PrimSpec as it converts. */
		if ( !lightusd::LayerToStageInPlace(std::move(composited), &stage, &warning, &error) )
		{
			TraceError{ClassId} << "Unable to build a stage from '" << sourceName << "' : " << error;

			return false;
		}

		/* ⚠️ THIS IS NOT COSMETIC LOGGING. tinyusdz reports through `warn` what it silently gave
		 * up on, and dropping that string on the floor is what cost a whole session: every
		 * PointInstancer of this asset was being discarded by `LayerToStage` — the prim type was
		 * missing from its reconstruction table — and it said so, here, every single time.
		 * A prim that cannot be reconstructed takes its ENTIRE SUBTREE with it, so a swallowed
		 * warning is not a missing detail, it is a missing half of the scene. */
		if ( !warning.empty() )
		{
			TraceWarning{ClassId} << "While building the stage of '" << sourceName << "' : " << warning;
			warning.clear();
		}

		return true;
	}

	std::optional< Asset >
	analyse (const lightusd::Stage & stage, const lightusd::AssetResolutionResolver & resolver, const std::vector< std::string > & searchPaths, const std::string & usdFilename, const StageMetrics & metrics, const std::string & resourcePrefix, const std::string & sourceName) noexcept
	{
		Asset asset;
		asset.resourcePrefix = resourcePrefix;
		asset.metrics = metrics;

		reportInventory(sourceName, stage);

		for ( const auto & prim : stage.root_prims() )
		{
			collectEnvironmentLights(prim, asset.lighting);
		}

		/* PointInstancers are read from the PRIMS: Tydra has no notion of them whatsoever, so
		 * nothing downstream would ever mention the vegetation. */
		auto & instancers = asset.instancers;

		for ( const auto & prim : stage.root_prims() )
		{
			collectInstancers(prim, "/" + prim.element_name(), metrics.metersPerUnit, instancers);
		}

		auto & prototypePaths = asset.prototypePaths;

		for ( const auto & instancer : instancers )
		{
			prototypePaths.insert(prototypePaths.end(), instancer.prototypePaths.begin(), instancer.prototypePaths.end());
		}

		std::ranges::sort(prototypePaths);

		const auto [duplicatesBegin, duplicatesEnd] = std::ranges::unique(prototypePaths);

		prototypePaths.erase(duplicatesBegin, duplicatesEnd);

		/* Tydra hands back renderer-ready data: triangulated faces, indexed vertices, resolved
		 * material bindings. Re-deriving that from raw prims would be duplicated work — the
		 * engine's own job starts at the translation into native scene logic. */
		lightusd::tydra::RenderSceneConverterEnv env{stage};
		env.usd_filename = usdFilename;

		/* ⚠️ Tydra carries its OWN resolver, and it is NOT the one composition used. Left as a
		 * filesystem resolver on an archive, a texture is not merely "unresolved": Tydra drops the
		 * whole UsdUVTexture, so `renderScene.textures` comes back EMPTY and the materials look
		 * like the asset never had any. */
		env.asset_resolver = resolver;
		env.set_search_paths(searchPaths);

		/* ⚠️ METADATA ONLY: the engine decodes every texture itself (archiveTexture() / resolveTexture(), from the
		 * archive or the stage directory) and reads nothing but `asset_identifier`. Left at its default (true), Tydra
		 * decoded all of them first — in fp32 for the 8-bit sRGB ones — then stored the UNRESOLVED asset path, which a
		 * Kit export's composition anchors TWICE (`<layer dir>/<layer dir>/../../Materials/Bake/…`): the 85 baked
		 * textures of WorldLobby.usdz were "not in the archive" and its materials kept their flat colour. The
		 * metadata-only branch stores the path resolved by tinyusdz's own resolver, which strips that doubled anchor
		 * (2026-10-04). */
		env.scene_config.load_texture_assets = false;

		lightusd::tydra::RenderSceneConverter converter;
		asset.renderScene = std::make_unique< lightusd::tydra::RenderScene >();
		auto & renderScene = *asset.renderScene;

		if ( !converter.ConvertToRenderScene(env, &renderScene) )
		{
			TraceError{ClassId} << "Unable to convert '" << sourceName << "' to a render scene : " << converter.GetError();

			return std::nullopt;
		}

		if ( !converter.GetWarning().empty() )
		{
			TraceWarning{ClassId} << "While converting '" << sourceName << "' : " << converter.GetWarning();
		}

		asset.materials = describeMaterials(renderScene, resourcePrefix);

		TraceInfo{ClassId} << asset.materials.size() << " materials translated.";

		collectDrawList(renderScene, asset);

		std::map< std::string, LightPlacement > lightPlacements;

		{
			lightusd::tydra::XformNode xformRoot;

			if ( lightusd::tydra::BuildXformNodeFromStage(stage, &xformRoot) )
			{
				collectLightPlacements(xformRoot, lightPlacements);
			}
			else
			{
				/* Not fatal, and deliberately loud: the geometry carries its own transforms from
				 * Tydra, so the stage still renders — lit by nothing. Silence here would reproduce
				 * exactly the black-frame-without-an-error the placement pass was written to end. */
				TraceError{ClassId} << "Unable to build the xform hierarchy of '" << sourceName << "'; every punctual light will be dropped.";
			}
		}

		asset.punctualLightCount = buildLights(renderScene, metrics.metersPerUnit, lightPlacements, asset.lighting);

		return asset;
	}

	size_t
	forEachMesh (const Asset & asset, const std::function< void (Mesh) > & consumer) noexcept
	{
		const auto & renderScene = *asset.renderScene;
		const auto & prototypePaths = asset.prototypePaths;
		const auto metersPerUnit = asset.metrics.metersPerUnit;

		/* ⚠️ AXIS AND UNIT BAKING — the single site where USD's convention is left behind.
		 *
		 * USD here is Z-up right-handed; the engine's law is Y-UP (docs/coordinate-system.md).
		 * A point one unit "up" in USD is (0, 0, 1) and must become (0, +1, 0):
		 *
		 *     engine.x =  usd.x        engine.y =  usd.z        engine.z = -usd.y
		 *
		 * ⚠️ The Z sign is NOT decoration: flipping the vertical axis alone gives a determinant
		 * of -1, which mirrors the geometry instead of turning it. Negating Z as well keeps the
		 * determinant at +1, so handedness — and therefore the winding — survives the bake.
		 * Verified on screen: the first attempt (y = -usd.z, z = usd.y) rendered upside down.
		 *
		 * applied to POSITIONS, NORMALS and TANGENTS alike. Converting positions and forgetting
		 * the vectors is the classic failure of this exercise: the shape looks right and the
		 * lighting is subtly wrong, which then gets blamed on the material system.
		 *
		 * `metersPerUnit` scales positions ONLY — a direction has no length to convert. */
		/* ⚠️ The SAME axis bake feeds positions, normals AND tangents, so they cannot drift apart.
		 * A normal needs no special case: the inverse-transpose of a diagonal ±1 matrix is the
		 * matrix itself. (The per-asset AxisFlip layer that used to compose in here is deleted.) */

		const auto bakePosition = [metersPerUnit] (const auto & p) {
			return Vector< 3, float >{static_cast< float >(p[0]) * metersPerUnit, static_cast< float >(p[2]) * metersPerUnit, -static_cast< float >(p[1]) * metersPerUnit};
		};

		const auto bakeDirection = [] (const auto & d) {
			return Vector< 3, float >{static_cast< float >(d[0]), static_cast< float >(d[2]), -static_cast< float >(d[1])};
		};

		const auto isPrototype = [&prototypePaths] (const std::string & path) {
			return std::ranges::any_of(prototypePaths, [&path] (const auto & prototypePath) {
				return path == prototypePath || path.starts_with(prototypePath + "/");
			});
		};

		size_t delivered = 0;
		size_t meshIndex = 0;

		for ( const auto & [sourceMeshIndex, worldMatrix, primPath] : asset.drawList )
		{
			if ( sourceMeshIndex >= renderScene.meshes.size() )
			{
				meshIndex++;

				continue;
			}

			const bool instancedOnly = isPrototype(primPath);

			const auto & renderMesh = renderScene.meshes[sourceMeshIndex];

			/* USD matrices are row-major and applied to ROW vectors: p' = p * M, so the
			 * translation sits in row 3. Getting this transposed is silent: a symmetric
			 * transform still looks plausible. */
			const auto toWorldPosition = [&worldMatrix] (const auto & p) {
				const auto x = static_cast< double >(p[0]);
				const auto y = static_cast< double >(p[1]);
				const auto z = static_cast< double >(p[2]);

				return std::array< double, 3 >{
					(x * worldMatrix[0][0]) + (y * worldMatrix[1][0]) + (z * worldMatrix[2][0]) + worldMatrix[3][0],
					(x * worldMatrix[0][1]) + (y * worldMatrix[1][1]) + (z * worldMatrix[2][1]) + worldMatrix[3][1],
					(x * worldMatrix[0][2]) + (y * worldMatrix[1][2]) + (z * worldMatrix[2][2]) + worldMatrix[3][2]};
			};

			/* A direction ignores the translation. The 3x3 block is used as-is rather than its
			 * inverse transpose: correct for rotation and uniform scale, which is what this
			 * asset uses. A non-uniform scale would need the inverse transpose — the day one
			 * shows up, the lighting will look wrong on stretched meshes. */
			const auto toWorldDirection = [&worldMatrix] (const auto & d) {
				const auto x = static_cast< double >(d[0]);
				const auto y = static_cast< double >(d[1]);
				const auto z = static_cast< double >(d[2]);

				return std::array< double, 3 >{
					(x * worldMatrix[0][0]) + (y * worldMatrix[1][0]) + (z * worldMatrix[2][0]),
					(x * worldMatrix[0][1]) + (y * worldMatrix[1][1]) + (z * worldMatrix[2][1]),
					(x * worldMatrix[0][2]) + (y * worldMatrix[1][2]) + (z * worldMatrix[2][2])};
			};

			const auto & indices = renderMesh.faceVertexIndices();
			const auto vertexCount = renderMesh.points.size();

			if ( vertexCount == 0 || indices.size() < 3 )
			{
				meshIndex++;

				continue;
			}

			/* Tydra hands back normals, texture coordinates and tangents as raw byte buffers.
			 * They are only usable when their vertex count matches the point count — anything
			 * else means a per-face or per-face-vertex layout this pass does not unpack. */
			const auto * normals = renderMesh.normals.vertex_count() == vertexCount
				? reinterpret_cast< const float * >(renderMesh.normals.data.data())
				: nullptr;

			const float * texCoords = nullptr;

			if ( const auto it = renderMesh.texcoords.find(0); it != renderMesh.texcoords.end() && it->second.vertex_count() == vertexCount )
			{
				texCoords = reinterpret_cast< const float * >(it->second.data.data());
			}

			auto shape = std::make_shared< Shape< float > >();

			const auto triangleCount = indices.size() / 3;

			const bool buildSuccess = shape->build([&] (auto & groups, auto & vertices, auto & triangles) {
				vertices.resize(vertexCount);

				for ( size_t index = 0; index < vertexCount; ++index )
				{
					vertices[index].setPosition(bakePosition(toWorldPosition(renderMesh.points[index])));

					if ( normals != nullptr )
					{
						vertices[index].setNormal(bakeDirection(toWorldDirection(&normals[index * 3])));
					}

					if ( texCoords != nullptr )
					{
						vertices[index].setTextureCoordinates(Vector< 2, float >{texCoords[index * 2], texCoords[(index * 2) + 1]});
					}
				}

				triangles.reserve(triangleCount);
				groups.emplace_back(0U, static_cast< uint32_t >(triangleCount));

				for ( size_t triangle = 0; triangle < triangleCount; ++triangle )
				{
					const auto a = indices[triangle * 3];
					const auto b = indices[(triangle * 3) + 1];
					const auto c = indices[(triangle * 3) + 2];

					/* The winding is kept as authored: the (a, c, b) swap that used to sit here
					 * compensated the engine's ORIENTATION-REVERSING projection — NOT the axis
					 * bake, whose determinant is deliberately kept at +1 a few lines above — and
					 * that mirror went away with the Y-up convention flip. */
					triangles.emplace_back(a, b, c);
				}

				return true;
			}, texCoords != nullptr);

			if ( !buildSuccess )
			{
				TraceWarning{ClassId} << "Unable to build the shape of mesh '" << renderMesh.prim_name << "'.";

				meshIndex++;

				continue;
			}

			if ( normals != nullptr )
			{
				shape->declareNormalsAvailable();
			}
			else
			{
				/* No inversion: nothing reflects the vertex data any more (the winding swap that
				 * used to force it is gone with the orientation-reversing projection). */
				shape->computeTriangleNormal(false);
				shape->computeVertexNormal();
			}

			/* Where a mesh actually landed after the bake. Without it, "I cannot see anything"
			 * is indistinguishable from "it is behind me", and the camera gets aimed by guesswork. */
			const auto & bounds = shape->boundingBox();

			TraceInfo{ClassId} <<
				"Mesh '" << renderMesh.prim_name << "' baked into [" <<
				bounds.minimum()[0] << ", " << bounds.minimum()[1] << ", " << bounds.minimum()[2] << "] .. [" <<
				bounds.maximum()[0] << ", " << bounds.maximum()[1] << ", " << bounds.maximum()[2] << "].";

			const auto resourceName = asset.resourcePrefix + "/mesh/" + ( renderMesh.prim_name.empty() ? std::to_string(meshIndex) : renderMesh.prim_name ) + "-" + std::to_string(meshIndex);

			/* `material_id` is the whole-mesh binding. GeomSubsets carry per-face bindings and are
			 * not honoured yet — a mesh with several material subsets takes the first one. */
			std::optional< size_t > material;

			if ( renderMesh.material_id >= 0 )
			{
				material = static_cast< size_t >(renderMesh.material_id);
			}

			consumer(Mesh{
				.resourceName = resourceName,
				.primName = renderMesh.prim_name,
				.primPath = primPath,
				.shape = std::move(shape),
				.material = material,
				.doubleSided = renderMesh.doubleSided,
				.instancedOnly = instancedOnly
			});

			delivered++;
			meshIndex++;
		}

		return delivered;
	}
}
