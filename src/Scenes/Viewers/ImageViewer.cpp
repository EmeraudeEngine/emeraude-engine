/*
 * src/Scenes/Viewers/ImageViewer.cpp
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

#include "ImageViewer.hpp"

/* STL inclusions. */
#include <atomic>
#include <cmath>
#include <cstdint>
#include <string>
#include <utility>

/* Local inclusions. */
#include "Graphics/Geometry/ResourceGenerator.hpp"
#include "Graphics/ImageResource.hpp"
#include "Graphics/Material/StandardResource.hpp"
#include "Graphics/RasterizationOptions.hpp"
#include "Graphics/Renderable/MeshResource.hpp"
#include "Graphics/TextureResource/Texture2D.hpp"
#include "IO/IO.hpp"
#include "PixelFactory/FileIO.hpp"
#include "Resources/Manager.hpp"
#include "Scenes/Component/Camera.hpp"
#include "Scenes/Component/Visual.hpp"
#include "Scenes/Manager.hpp"
#include "Scenes/Scene.hpp"
#include "Tracer.hpp"

namespace EmEn::Scenes::Viewers
{
	using namespace Base;
	using namespace Base::Math;
	using namespace Graphics;

	std::shared_ptr< Scene >
	ImageViewer::createScene (const std::filesystem::path & filepath) noexcept
	{
		/* NOTE: The image is read on the calling thread, the aspect
		 * ratio is needed right away to size the quad. */
		PixelFactory::Pixmap< uint8_t > pixmap;

		constexpr PixelFactory::ReadOptions readOptions{.targetChannelMode = PixelFactory::TargetChannelMode::RGBA};

		if ( !PixelFactory::FileIO::read(filepath, pixmap, readOptions) )
		{
			TraceError{ClassId} << "Unable to read the image file '" << IO::toU8String(filepath) << "' !";

			return nullptr;
		}

		const auto ratio = static_cast< float >(pixmap.width()) / static_cast< float >(pixmap.height());
		const auto quadWidth = QuadHeight * ratio;

		/* NOTE: A leftover viewer scene blocks the name, remove it first. */
		if ( m_sceneManager.hasSceneNamed(SceneName) && !m_sceneManager.deleteScene(SceneName) )
		{
			return nullptr;
		}

		const auto scene = m_sceneManager.newScene(SceneName, SceneBoundary);

		if ( scene == nullptr )
		{
			return nullptr;
		}

		/* NOTE: Each viewing session gets its own resource names, a resource
		 * container always returns an existing resource for a known name. */
		static std::atomic< uint32_t > s_sessionCount{0};

		const auto suffix = std::to_string(++s_sessionCount);

		/* NOTE: The light set stays disabled and the material is unlit without any
		 * emissive scaling : with the camera out of HDR, texels reach the screen unmodified. */
		const auto image = m_resourceManager.container< ImageResource >()->getOrCreateResource("+ImageViewerImage" + suffix, [pixmap = std::move(pixmap)] (ImageResource & imageResource) mutable {
			return imageResource.load(std::move(pixmap));
		});

		const auto texture = m_resourceManager.container< TextureResource::Texture2D >()->getOrCreateResource("+ImageViewerTexture" + suffix, [image] (TextureResource::Texture2D & textureResource) {
			return textureResource.load(image);
		});

		const auto material = m_resourceManager.container< Material::StandardResource >()->getOrCreateResource("+ImageViewerMaterial" + suffix, [texture] (Material::StandardResource & materialResource) {
			if ( !materialResource.setAlbedoComponent(texture, true) )
			{
				return materialResource.setManualLoadSuccess(false);
			}

			materialResource.enableUnlit();

			return materialResource.setManualLoadSuccess(true);
		});

		const Geometry::ResourceGenerator generator{m_resourceManager, Geometry::EnablePrimaryTextureCoordinates};

		const auto geometry = generator.quad(quadWidth, QuadHeight, "+ImageViewerQuad" + suffix);

		/* NOTE: Double-sided : the orbit can pass behind the picture, the back face
		 * shows the image mirrored like a slide viewed from behind. */
		RasterizationOptions rasterizationOptions;
		rasterizationOptions.setCullingMode(CullingMode::None);

		const auto mesh = m_resourceManager.container< Renderable::MeshResource >()->getOrCreateResource("+ImageViewerMesh" + suffix, [geometry, material, rasterizationOptions] (Renderable::MeshResource & meshResource) {
			return meshResource.load(geometry, material, rasterizationOptions);
		});

		if ( mesh == nullptr )
		{
			m_sceneManager.deleteScene(SceneName);

			return nullptr;
		}

		/* NOTE: The quad faces +Z at the scene origin. */
		const auto pictureEntity = scene->createStaticEntity("Picture");

		if ( pictureEntity == nullptr )
		{
			m_sceneManager.deleteScene(SceneName);

			return nullptr;
		}

		pictureEntity->componentBuilder< Component::Visual >("Picture")
			.setup([] (Component::Visual & component) {
				component.getRenderableInstance()->setLightingState(false);
			})
			.build(mesh);

		/* NOTE: The camera node stays a direct child of the scene root, the
		 * orbit controller positions it in parent space. */
		const auto cameraNode = scene->root()->createChild("ViewerCamera");

		if ( cameraNode == nullptr )
		{
			m_sceneManager.deleteScene(SceneName);

			return nullptr;
		}

		const auto camera = cameraNode->componentBuilder< Component::Camera >("ViewerCamera")
			.asPrimary()
			.build();

		/* NOTE: A classic portrait focal length keeps the perspective distortion low
		 * when orbiting around the picture. The camera stays out of HDR : with the
		 * light set disabled and an unlit material, texels pass through unmodified. */
		camera->setFocalLength(ViewerFocalLength);

		/* NOTE: Distance framing the quad in the vertical field of view, with a margin. */
		const auto fieldOfView = Radian(camera->fieldOfView());
		const auto framingDistance = FramingMargin * (0.5F * std::max(QuadHeight, quadWidth)) / std::tan(0.5F * fieldOfView);

		auto & orbitController = scene->orbitController();
		orbitController.setDistanceLimits(QuadHeight * 0.05F, SceneBoundary * 0.5F);
		orbitController.setTarget({0.0F, 0.0F, 0.0F});
		orbitController.setOrientation(0.0F, 0.0F);
		orbitController.setDistance(framingDistance);
		orbitController.controlNode(cameraNode);

		return scene;
	}
}
