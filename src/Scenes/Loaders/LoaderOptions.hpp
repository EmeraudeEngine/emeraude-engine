/*
 * src/Scenes/Loaders/LoaderOptions.hpp
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

#pragma once

/* Project configuration. */
#include "emeraude_export.hpp"

/* STL inclusions. */
#include <cstdint>
#include <functional>
#include <string>
#include <unordered_set>

/* Forward declarations. */
namespace EmEn::Scenes::Loaders
{
	struct MeshDescriptor;
}

namespace EmEn::Scenes::Loaders
{
	/**
	 * @brief Options that affect resource loading (shared by all loaders).
	 * @note flattenHierarchy is NOT here — it only affects scene building,
	 * not resource loading, and belongs in Scenes::SceneDataConsumer.
	 */
	struct EMEN_API LoaderOptions
	{
		std::unordered_set< std::string > excludedNodeNames;
		/**
		 * @brief Optional per-mesh hook invoked right after a mesh's renderable, geometry
		 * and materials have been registered. Lets the caller patch the descriptor in place
		 * (e.g. enable IBL reflection on PBR materials, override geometry, swap a renderable).
		 * Called once per loaded mesh, in load order, before nodes are wired.
		 */
		std::function< void (MeshDescriptor &) > onMeshLoaded;
		/**
		 * @brief Resolves references, payloads, inherits and variants in addition to sublayers.
		 *
		 * @note USD-only. Sublayer composition alone yields a stage's own geometry; anything
		 * pulled in by a REFERENCE — a PointInstancer's prototypes, typically — needs this.
		 *
		 * ⚠️ It is off by default because the cost is unbounded: on the whole Intel Jungle Ruins
		 * stage it was measured at 24 minutes and 15 GB resident, still growing linearly when it
		 * was killed. Enable it per ELEMENT, never on a full stage, until deferred loading exists.
		 */
		bool resolveReferences{false};
		bool skipSkinning{false};
		/**
		 * @brief Negates the X / Y / Z axis of the asset AT IMPORT, in the geometry itself.
		 *
		 * ⚠️ "swap" here means **negate this axis**, not "exchange two axes". `swapZ` maps
		 * `(x, y, z)` to `(x, y, -z)`.
		 *
		 * The whole import is routed through `AxisFlip` (see `AxisFlip.hpp` for the full rule):
		 * vertex positions, normals and tangents, node transforms, joint bind poses, inverse bind
		 * matrices, animation keyframes — and the triangle winding, which follows the PARITY of
		 * these three flags.
		 *
		 * **What this is for.** The engine's world→screen mapping is orientation-REVERSING
		 * (measured; `docs/coordinate-system.md` § OPEN DEFECT), while importers convert with a
		 * ROTATION, which cannot undo a chirality difference. Chiral content — text baked in a
		 * texture, an asset with a readable left and right — therefore lands MIRRORED on screen.
		 * Enabling an ODD number of these flags makes the import orientation-reversing too, which
		 * cancels the pipeline's reversal and puts the asset back the right way round.
		 *
		 * ⚠️⚠️ **For glTF and FBX the flag to use is `swapZ`, not `swapY`.** The consumer still
		 * applies its 180° X rotation `diag(1,-1,-1)`; composed with `swapZ` it gives
		 * `diag(1,-1,1)` — chirality fixed, up axis still up. `swapY` also fixes the chirality but
		 * renders the scene UPSIDE DOWN.
		 *
		 * ⚠️ Enabling a flag changes the SIGN OF A WORLD COORDINATE for the whole asset: every
		 * camera position, lookAt target and actor placement hard-coded against that asset must be
		 * revisited on the flipped axis. Default `false` on all three, so existing scenes are
		 * untouched.
		 *
		 * @note Two flags cancel each other's effect on chirality (a 180° rotation about the third
		 * axis), which is occasionally what an asset needs for orientation, never for mirroring.
		 */
		bool swapX{false};
		bool swapY{false};
		bool swapZ{false};
		/**
		 * @brief Forces every material part of the loaded model to render
		 * double-sided (back-face culling disabled, CullingMode::None),
		 * regardless of what the asset declares. OR-ed with the per-material
		 * asset flag (glTF doubleSided / FBX ufbx double_sided feature).
		 *
		 * Use for assets whose format/exporter cannot carry a double-sided
		 * flag the loader can read - e.g. Mixamo FBX rigs, which ship plain
		 * FbxSurfacePhong materials; ufbx only surfaces double_sided for
		 * glTF-style materials, so these models always import single-sided
		 * and thin shells (inner armor, cloth) show see-through holes.
		 *
		 * @note Two-sided lighting is already handled engine-side (back-face
		 * shading normals are flipped), so enabling this yields correctly lit
		 * back-faces, not inward-lit ones.
		 */
		bool forceDoubleSided{false};
		/**
		 * @brief Strips the translation track of every root joint from animation
		 * clips produced by `loadAnimationClipsOnly`. Rotation and scale tracks
		 * are kept intact, and non-root joints are not touched.
		 *
		 * Mixamo (and many other DCC) per-action FBX clips bake forward
		 * locomotion into the root bone — the model physically translates
		 * meters during the clip. When the actor's displacement is also driven
		 * by gameplay code (physics force, navmesh, etc.), the two motions
		 * stack and the model snaps backward at every loop boundary. Enabling
		 * this flag turns Mixamo locomotion clips into "in-place" clips at
		 * load time without re-exporting from the DCC.
		 *
		 * @note Has no effect on `load()` (full-pipeline import) — only on
		 * `loadAnimationClipsOnly()`. Tracked TODO: a future "root-motion
		 * mode" will instead extract the root delta and feed it to the actor
		 * as actual displacement (foot-planting, no sliding) — see
		 * `dependencies/emeraude-engine/docs/` and the engine TODO list.
		 */
		bool stripRootMotion{false};
		/**
		 * @brief Uniform scale applied at load time, coherently across the
		 * full skinned-mesh pipeline: vertex positions, joint local
		 * translations, inverse bind matrix translation columns, and
		 * animation translation keyframes (both in `load()` embedded clips
		 * and `loadAnimationClipsOnly()` external clips). Rotations and
		 * scales of joint TRS, plus the per-vertex influence weights, are
		 * never touched.
		 *
		 * The scale must be passed identically to BOTH the rig load and
		 * every subsequent `loadAnimationClipsOnly()` call against that rig
		 * — otherwise animation keyframes would describe translations in a
		 * different unit than the scaled bind pose, and joints would snap
		 * to wrong positions at every keyframe (visual: the rig would
		 * collapse on the first animated frame).
		 *
		 * Default `1.0F` is a no-op. Set `< 1.0` to shrink, `> 1.0` to
		 * enlarge. Also propagates to the bounding box of the produced
		 * renderables, so collision shapes derived from the bbox reflect
		 * the scaled size automatically.
		 */
		float uniformScale{1.0F};
		/**
		 * @brief Intensity of the environment (image-based) specular reflection given to every
		 * material the loader produces. `0.0F` disables it entirely.
		 *
		 * ⚠️ Default `0.0F` — OFF, opt-in per asset (owner decision, Aug 2026). The environment
		 * cubemap is UNOCCLUDED and its sample is scaled to the sky's ABSOLUTE luminance
		 * (`LightGenerator::reflectionIntensity()` × environmentLuminance): any smooth dielectric
		 * or metal INDOORS mirrors the outdoor sky/vegetation at full photometric brightness even
		 * in shade — measured on Sponza (glass roughness 0, metal doors metalness 0.88): glowing
		 * green panels in a dark corridor. Until reflections are occluded (local probes /
		 * specular occlusion — separate work), only scenes whose materials actually SEE the
		 * environment should enable this (e.g. an object showcase like DamagedHelmet passes 1.0F).
		 *
		 * When enabled it feeds TWO channels at once:
		 * 1. the IBL specular sampled from the scene's environment cubemap;
		 * 2. the REFLECTIVITY published to the material-properties G-buffer, which SSR and RTR read.
		 *    See `LightGenerator::materialPropertiesExpression()`: declaring a reflection promotes
		 *    the material from priority 3 (`metalness * (1 - roughness)`, which collapses to ~0 on a
		 *    rough surface) to priority 2 (`max(iblIntensity * (1 - roughness), metalness)`, where
		 *    metal keeps a high reflectivity whatever its roughness). A rough metal asset with the
		 *    option off therefore gets NEITHER image-based specular NOR screen-space/ray-traced
		 *    reflections — that trade-off is the price of the missing occlusion, not a bug.
		 */
		float environmentReflectionIntensity{0.0F};
	};
}
