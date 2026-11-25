/*
 * src/Scenes/SeaLevelInterface.hpp
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
#include <cmath>
#include <optional>

/* Local inclusions for usages. */
#include "Math/Vector.hpp"

/* Forward declarations. */
namespace EmEn::Graphics
{
	struct SeaCaustics;
	struct SeaSurfaceWaves;
}

namespace EmEn::Scenes
{
	/* Forward declarations. */
	class GroundLevelInterface;
	class WaterMedium;

	/**
	 * @brief How the land calms a sea whose surface moves: which water the swell reaches, and what a sheltered water
	 * keeps of its waves (owner decisions 2026-11-24, engine item ocean-sheltered-water).
	 * @note The values a sea answers (SeaLevelInterface::shelterSettings()) and takes
	 * (SeaLevelInterface::setShelterSettings()) while its scene runs — the owner tunes the look on screen.
	 */
	struct SeaShelterSettings
	{
		/** @brief The narrowest passage the swell enters, in metres. Finite, 0 or more. */
		float minimumPassage{0.0F};
		/** @brief The distance, in metres, over which the sea calms down once it has gone as far as it can. Finite, 0 or more. */
		float fade{0.0F};
		/** @brief The share a sheltered water keeps of the swell (the sea's largest waves), 0 to 1. */
		float swellShare{0.0F};
		/** @brief The share a sheltered water keeps of the middle waves, 0 to 1. */
		float waveShare{0.0F};
		/** @brief The share a sheltered water keeps of the ripples (the sea's smallest waves), 0 to 1. */
		float rippleShare{0.0F};

		/**
		 * @brief Tells whether every value is one a sea takes: finite, the distances 0 or more, the shares in 0 to 1.
		 * @return bool
		 */
		[[nodiscard]]
		bool
		isValid () const noexcept
		{
			const auto distance = [] (float value) noexcept {
				return std::isfinite(value) && value >= 0.0F;
			};
			const auto share = [] (float value) noexcept {
				return std::isfinite(value) && value >= 0.0F && value <= 1.0F;
			};

			return distance(minimumPassage) && distance(fade) && share(swellShare) && share(waveShare) && share(rippleShare);
		}
	};

	/**
	 * @brief The caustics a sea whose surface moves draws under itself: how its waves focus the light (engine item
	 * underwater-caustics-and-depth-light).
	 * @note The values a sea answers (SeaLevelInterface::causticSettings()) and takes
	 * (SeaLevelInterface::setCausticSettings()) while its scene runs — the owner tunes the look on screen.
	 */
	struct SeaCausticSettings
	{
		/** @brief How much of the caustics is drawn: 0 none, 1 all of it. */
		float strength{0.0F};
		/** @brief The low-pass of the waves' curvature, in metres per metre of depth: the waves shorter than this share of
		 * the depth are averaged out (they crossed their focus above the receiver). Finite, 0 or more. */
		float softening{0.0F};
		/** @brief The depth the caustics fade over (to 1/e), in metres. Finite, more than 0. */
		float fadeDepth{1.0F};
		/** @brief How far a temporal pass gives up its history where the caustics are drawn in full: 0 not at all (the
		 * history averages the moving pattern), 1 the most a graded reactive mask asks. Finite, from 0 to 1. */
		float temporalResponse{0.0F};

		/**
		 * @brief Tells whether every value is one a sea takes.
		 * @return bool
		 */
		[[nodiscard]]
		bool
		isValid () const noexcept
		{
			return std::isfinite(strength) && strength >= 0.0F && strength <= 1.0F &&
				std::isfinite(softening) && softening >= 0.0F &&
				std::isfinite(fadeDepth) && fadeDepth > 0.0F &&
				std::isfinite(temporalResponse) && temporalResponse >= 0.0F && temporalResponse <= 1.0F;
		}
	};

	/**
	 * @brief How a sea draws the sun's shafts in its water: the light the water scatters toward the eye, broken by the
	 * waves (the caustics, in the volume) and by what shadows the sea.
	 * @note Read and changed while the sea is drawn (SeaLevelInterface::shaftSettings() / setShaftSettings()).
	 */
	struct SeaShaftSettings
	{
		/** @brief How much of the shafts is drawn: 0 none (the water scatters the sun's light evenly), 1 all of it. */
		float strength{0.0F};
		/** @brief The anisotropy of the scattering of the sun's light toward the eye (Henyey-Greenstein g): 0 the same
		 * in every direction, toward 1 mostly forward — the shafts show looking toward the sun, the water darkens
		 * looking away from it. Finite, from 0 to 0.95. */
		float anisotropy{0.0F};

		/**
		 * @brief Tells whether every value is one a sea takes.
		 * @return bool
		 */
		[[nodiscard]]
		bool
		isValid () const noexcept
		{
			return std::isfinite(strength) && strength >= 0.0F && strength <= 1.0F &&
				std::isfinite(anisotropy) && anisotropy >= 0.0F && anisotropy <= 0.95F;
		}
	};

	/**
	 * @brief Interface to define a visible and physical sea level in a scene.
	 */
	class EMEN_API SeaLevelInterface
	{
		public:

			/**
			 * @brief Destructs the sea level interface.
			 */
			virtual ~SeaLevelInterface () = default;

			/**
			 * @brief Copy constructor (deleted: a polymorphic interface, copying would slice).
			 * @param copy A reference to the copied instance.
			 */
			SeaLevelInterface (const SeaLevelInterface & copy) noexcept = delete;

			/**
			 * @brief Move constructor (deleted: a polymorphic interface).
			 * @param copy A reference to the copied instance.
			 */
			SeaLevelInterface (SeaLevelInterface && copy) noexcept = delete;

			/**
			 * @brief Copy assignment (deleted: a polymorphic interface).
			 * @param copy A reference to the copied instance.
			 * @return SeaLevelInterface &
			 */
			SeaLevelInterface & operator= (const SeaLevelInterface & copy) noexcept = delete;

			/**
			 * @brief Move assignment (deleted: a polymorphic interface).
			 * @param copy A reference to the copied instance.
			 * @return SeaLevelInterface &
			 */
			SeaLevelInterface & operator= (SeaLevelInterface && copy) noexcept = delete;

			/**
			 * @brief Returns the constant water level height.
			 * @return float
			 */
			[[nodiscard]]
			virtual float getLevel () const noexcept = 0;

			/**
			 * @brief Returns the water level at the given position.
			 * @param worldPosition An absolute position.
			 * @return float
			 */
			[[nodiscard]]
			virtual float getLevelAt (const Base::Math::Vector< 3, float > & worldPosition) const noexcept = 0;

			/**
			 * @brief Returns a position where Y is completed by the water level at X,Z position.
			 * @param positionX The X coordinates.
			 * @param positionZ The Z coordinates.
			 * @param deltaY A difference value to add to the Y component. Default 0.0F.
			 * @return Vector< 3, float >
			 */
			[[nodiscard]]
			virtual Base::Math::Vector< 3, float > getLevelAt (float positionX, float positionZ, float deltaY = 0.0F) const noexcept = 0;

			/**
			 * @brief Returns the normal vector at the given position.
			 * @param worldPosition An absolute position.
			 * @return Vector< 3, float >
			 */
			[[nodiscard]]
			virtual Base::Math::Vector< 3, float > getNormalAt (const Base::Math::Vector< 3, float > & worldPosition) const noexcept = 0;

			/**
			 * @brief Checks if a world position is submerged (below water level).
			 * @param worldPosition An absolute position.
			 * @return bool
			 */
			[[nodiscard]]
			virtual bool isSubmerged (const Base::Math::Vector< 3, float > & worldPosition) const noexcept = 0;

			/**
			 * @brief Returns the depth at a given position (positive if submerged, negative if above).
			 * @param worldPosition An absolute position.
			 * @return float
			 */
			[[nodiscard]]
			virtual float getDepthAt (const Base::Math::Vector< 3, float > & worldPosition) const noexcept = 0;

			/**
			 * @brief Returns the water the sea declares, or nullptr when it declares none.
			 * @note The sea level answers the optics of its water (owner decision 2026-11-23, engine item
			 * underwater-view): what the underwater view reads. A renderable sea answers the water its surface material
			 * declares (Graphics::Material::Interface::waterVolume(), owner decision 2026-11-24): one statement, read by
			 * the surface's shading and by the view from below. A sea without declared water has no underwater view and
			 * is drawn as a surface seen from above.
			 * @return const WaterMedium *
			 */
			[[nodiscard]]
			virtual const WaterMedium * waterMedium () const noexcept = 0;

			/**
			 * @brief The scene presents its ground to its sea: the sea adapts to it.
			 * @note Called by the scene whenever it holds both (Scene::setGroundLevel(), Scene::setSeaLevel(), its
			 * construction), again if either changes. A sea whose surface moves learns there where the land shelters
			 * its water — a basin enclosed in an island, behind a passage too narrow for the swell — and calms its waves
			 * there (owner decisions 2026-11-24, engine item ocean-sheltered-water); a surface at rest has nothing to
			 * adapt.
			 * @note Synchronous, on the thread that builds the scene: a bounded computation over the scene's square.
			 * @param ground A reference to the scene's ground. Only read during the call.
			 * @param boundary The scene's boundary, in metres: the ground is read over the square [-boundary, boundary]
			 * on X and Z.
			 */
			virtual void adaptToGround (const GroundLevelInterface & ground, float boundary) noexcept = 0;

			/**
			 * @brief Returns how the land calms this sea, or nothing for a sea the land does not calm (a surface at
			 * rest).
			 * @note ANY THREAD.
			 * @return std::optional< SeaShelterSettings >
			 */
			[[nodiscard]]
			virtual std::optional< SeaShelterSettings > shelterSettings () const noexcept = 0;

			/**
			 * @brief Changes how the land calms this sea.
			 * @note ANY THREAD. The shares apply to the next frame. The passage and the fade apply when the ground is
			 * presented again (adaptToGround()): through a scene, call Scene::setSeaShelterSettings(), which does it.
			 * @param settings The new values. Refused when one is not valid (SeaShelterSettings::isValid()).
			 * @return bool False when the values are refused, or when the land does not calm this sea: nothing changed.
			 */
			[[nodiscard]]
			virtual bool setShelterSettings (const SeaShelterSettings & settings) noexcept = 0;

			/**
			 * @brief Returns what a pass binds to know where the MOVING surface stands (the displacement its mesh is
			 * drawn with), or nullptr when the surface is at rest — it then is the level, everywhere.
			 * @note ⚠️ RENDER THREAD, valid for the frame it is asked in. Null too while a moving surface has not been
			 * drawn yet: its displacement holds nothing before its first update.
			 * @return const Graphics::SeaSurfaceWaves *
			 */
			[[nodiscard]]
			virtual const Graphics::SeaSurfaceWaves * surfaceWaves () const noexcept = 0;

			/**
			 * @brief Returns what a light needs to draw the caustics of the MOVING surface (how its waves focus the
			 * light on what lies under them), or nullptr when the surface is at rest — it then focuses nothing.
			 * @note ⚠️ RENDER THREAD, valid for the frame it is asked in. Null too while a moving surface has not been
			 * drawn yet: its waves hold nothing before its first update.
			 * @return const Graphics::SeaCaustics *
			 */
			[[nodiscard]]
			virtual const Graphics::SeaCaustics * caustics () const noexcept = 0;

			/**
			 * @brief Returns how this sea draws its caustics, or nothing for a sea that draws none (a surface at rest).
			 * @note ANY THREAD.
			 * @return std::optional< SeaCausticSettings >
			 */
			[[nodiscard]]
			virtual std::optional< SeaCausticSettings > causticSettings () const noexcept = 0;

			/**
			 * @brief Changes how this sea draws its caustics.
			 * @note ANY THREAD. Applied from the sea's next surface update on.
			 * @param settings The new values. Refused when one is not valid (SeaCausticSettings::isValid()).
			 * @return bool False when the values are refused, or when this sea draws no caustics: nothing changed.
			 */
			[[nodiscard]]
			virtual bool setCausticSettings (const SeaCausticSettings & settings) noexcept = 0;

			/**
			 * @brief Returns how this sea draws the sun's shafts in its water, or nothing for a sea that draws none (a
			 * surface at rest: its waves break no beam).
			 * @note ANY THREAD.
			 * @return std::optional< SeaShaftSettings >
			 */
			[[nodiscard]]
			virtual std::optional< SeaShaftSettings > shaftSettings () const noexcept = 0;

			/**
			 * @brief Changes how this sea draws the sun's shafts in its water.
			 * @note ANY THREAD. Applied from the sea's next surface update on.
			 * @param settings The new values. Refused when one is not valid (SeaShaftSettings::isValid()).
			 * @return bool False when the values are refused, or when this sea draws no shafts: nothing changed.
			 */
			[[nodiscard]]
			virtual bool setShaftSettings (const SeaShaftSettings & settings) noexcept = 0;

			/**
			 * @brief Updates the water surface visibility from the camera position.
			 * @note This is not frustum-culling, but helps the water to know where the point of view is located.
			 * @param worldPosition The camera world position.
			 */
			virtual void updateVisibility (const Base::Math::Vector< 3, float > & worldPosition) noexcept = 0;

		protected:

			/**
			 * @brief Constructs a renderable sea level interface.
			 */
			SeaLevelInterface () noexcept = default;
	};
}
