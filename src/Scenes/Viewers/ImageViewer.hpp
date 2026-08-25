/*
 * src/Scenes/Viewers/ImageViewer.hpp
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
}

namespace EmEn::Scenes::Viewers
{
	/**
	 * @brief Builds a ready-to-enable scene displaying a single image file.
	 * @details The scene holds an unlit quad matching the image aspect ratio, textured
	 * with the image, and a camera driven by the scene orbit controller (drag to rotate,
	 * wheel to dolly). The light set stays disabled and the camera stays out of HDR,
	 * so the image texels reach the screen unmodified.
	 * @note This is a Core default behavior for an image file dropped onto the window.
	 */
	class EMEN_API ImageViewer final
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"ImageViewer"};

			/** @brief Reserved name of the scene built by this viewer. */
			static constexpr auto SceneName{"+ImageViewer"};

			/**
			 * @brief Constructs an image viewer.
			 * @param resourceManager A reference to the resource manager.
			 * @param sceneManager A reference to the scene manager.
			 */
			ImageViewer (Resources::Manager & resourceManager, Manager & sceneManager) noexcept
				: m_resourceManager{resourceManager},
				m_sceneManager{sceneManager}
			{

			}

			/**
			 * @brief Creates the viewer scene from an image file.
			 * @note An existing viewer scene with the same name is deleted first.
			 * The returned scene is not enabled, the caller decides when.
			 * @param filepath A reference to a filesystem path to a readable image.
			 * @return std::shared_ptr< Scene > The scene, or nullptr on failure.
			 */
			[[nodiscard]]
			std::shared_ptr< Scene > createScene (const std::filesystem::path & filepath) noexcept;

		private:

			/** @brief Height of the displayed quad, in meters. The width follows the image ratio. */
			static constexpr auto QuadHeight{1.0F};

			/** @brief Half-size of the cubic viewer scene, in meters. */
			static constexpr auto SceneBoundary{100.0F};

			/** @brief Margin applied to the framing distance. */
			static constexpr auto FramingMargin{1.15F};

			/** @brief Focal length of the viewer camera, in millimeters. A classic
			 * portrait length, keeping the perspective distortion low. */
			static constexpr auto ViewerFocalLength{50.0F};

			Resources::Manager & m_resourceManager;
			Manager & m_sceneManager;
	};
}
