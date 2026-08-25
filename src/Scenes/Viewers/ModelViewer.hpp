/*
 * src/Scenes/Viewers/ModelViewer.hpp
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
 */

#pragma once

/* Project configuration. */
#include "emeraude_export.hpp"

/* STL inclusions. */
#include <filesystem>
#include <memory>

/* Forward declarations. */
namespace EmEn
{
	namespace Resources
	{
		class Manager;
	}

	namespace Scenes
	{
		class Manager;
		class Scene;
	}

	class Settings;
}

namespace EmEn::Scenes::Viewers
{
	/**
	 * @brief Builds a ready-to-enable scene displaying a composite asset file (glTF, FBX, USD, WAD, ...).
	 * @details The asset is imported through the scene loader matching the file extension,
	 * lit by a neutral setup (ambient plus one key light), rendered through an HDR camera
	 * with automatic exposure, and framed by the scene orbit controller (drag to rotate,
	 * wheel to dolly).
	 * @note This is a Core default behavior for a model file dropped onto the window.
	 * @warning The import runs synchronously on the calling thread. It is intended for
	 * reasonably sized assets, a multi-second import stalls the main loop.
	 */
	class EMEN_API ModelViewer final
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"ModelViewer"};

			/** @brief Reserved name of the scene built by this viewer. */
			static constexpr auto SceneName{"+ModelViewer"};

			/**
			 * @brief Constructs a model viewer.
			 * @param resourceManager A reference to the resource manager.
			 * @param sceneManager A reference to the scene manager.
			 * @param settings A reference to the settings.
			 */
			ModelViewer (Resources::Manager & resourceManager, Manager & sceneManager, Settings & settings) noexcept
				: m_resourceManager{resourceManager},
				m_sceneManager{sceneManager},
				m_settings{settings}
			{

			}

			/**
			 * @brief Returns whether this viewer can display a file.
			 * @details True for composite assets handled by a scene loader (glTF, FBX, USD,
			 * WAD, ...) and for raw geometry files readable by the vertex factory (OBJ, STL, ...).
			 * @param sceneManager A reference to the scene manager, for the loader registry.
			 * @param filepath A reference to a filesystem path.
			 * @return bool
			 */
			[[nodiscard]]
			static bool handlesFile (const Manager & sceneManager, const std::filesystem::path & filepath) noexcept;

			/**
			 * @brief Creates the viewer scene from a composite asset or raw geometry file.
			 * @note An existing viewer scene with the same name is deleted first.
			 * The returned scene is not enabled, the caller decides when.
			 * @param filepath A reference to a filesystem path to a loadable asset.
			 * @return std::shared_ptr< Scene > The scene, or nullptr on failure.
			 */
			[[nodiscard]]
			std::shared_ptr< Scene > createScene (const std::filesystem::path & filepath) noexcept;

		private:

			/**
			 * @brief Imports a raw geometry file as a single mesh wearing a neutral clay material.
			 * @param filepath A reference to a filesystem path.
			 * @param scene A reference to the viewer scene.
			 * @return bool
			 */
			[[nodiscard]]
			bool importGeometry (const std::filesystem::path & filepath, Scene & scene) noexcept;

			/** @brief Half-size of the cubic viewer scene, in meters. */
			static constexpr auto SceneBoundary{1000.0F};

			/** @brief Margin applied to the framing distance. */
			static constexpr auto FramingMargin{1.2F};

			/** @brief Focal length of the viewer camera, in millimeters. A classic
			 * portrait length, keeping the perspective distortion low. */
			static constexpr auto ViewerFocalLength{50.0F};

			/** @brief Time budget waiting for the imported meshes to publish their extents, in milliseconds. */
			static constexpr auto ExtentsWaitBudgetMS{5000U};

			Resources::Manager & m_resourceManager;
			Manager & m_sceneManager;
			Settings & m_settings;
	};
}
