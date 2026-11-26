/*
 * src/Scenes/Manager.console.cpp
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

#include "Manager.hpp"

/* STL inclusions. */
#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <iomanip>
#include <iterator>
#include <map>
#include <memory>
#include <optional>
#include <ranges>
#include <span>
#include <sstream>
#include <string>
#include <string_view>
#include <system_error>
#include <type_traits>
#include <utility>
#include <vector>

/* Third-party inclusions. */
#include "volk.h"

/* Local inclusions. */
#include "Component/Camera.hpp" // IWYU pragma: keep (componentBuilder< Component::Camera > needs the complete type)
#include "Component/CharacterController.hpp"
#include "Component/ConsoleAdapter.hpp"
#include "Component/Microphone.hpp" // IWYU pragma: keep (componentBuilder< Component::Microphone > needs the complete type)
#include "Component/Vehicle.hpp"
#include "Component/Visual.hpp"
#include "Console/ControllableTrait.hpp"
#include "Console/JSONNumber.hpp"
#include "Console/Output.hpp"
#include "Console/Parameter.hpp"
#include "Console/Types.hpp"
#include "CoreTypes.hpp"
#include "FastJSON.hpp"
#include "FileSystem.hpp"
#include "Graphics/CelestialBody.hpp"
#include "Graphics/FrameCapture.hpp"
#include "Graphics/Geometry/IndexedVertexResource.hpp"
#include "Graphics/Geometry/ResourceGenerator.hpp"
#include "Graphics/Geometry/Types.hpp"
#include "Graphics/ImposterAtlas.hpp"
#include "Graphics/Material/Interface.hpp"
#include "Graphics/Material/SkyAtmosphereResource.hpp"
#include "Graphics/Material/StandardResource.hpp"
#include "Graphics/RasterizationOptions.hpp"
#include "Graphics/Renderable/AbstractBackground.hpp"
#include "Graphics/Renderable/BasicGroundResource.hpp"
#include "Graphics/Renderable/DynamicSkyResource.hpp"
#include "Graphics/Renderable/MultiLayerMeshResource.hpp"
#include "Graphics/Renderable/SkyBoxResource.hpp"
#include "Graphics/Renderable/Types.hpp"
#include "Graphics/RenderableInstance/Abstract.hpp"
#include "Graphics/Renderer.hpp"
#include "Graphics/RenderTarget/Abstract.hpp"
#include "Graphics/RenderTarget/ImposterBake.hpp"
#include "Graphics/SelectionOutline.hpp"
#include "Graphics/SkyAtmosphere.hpp"
#include "Graphics/TextureResource/StoreTexture.hpp"
#include "Graphics/TextureResource/Texture2D.hpp"
#include "GroundTriangles.hpp"
#include "IO/IO.hpp"
#include "json/json.h"
#include "Logging/Severity.hpp"
#include "Math/CartesianFrame.hpp"
#include "Math/Space3D/AACuboid.hpp"
#include "Math/Vector.hpp"
#include "Physics/CharacterController.hpp"
#include "Physics/MovableTrait.hpp"
#include "Physics/Vehicle.hpp"
#include "PixelFactory/Color.hpp"
#include "PixelFactory/FileIO.hpp"
#include "PixelFactory/Pixmap.hpp"
#include "PixelFactory/Reduction.hpp"
#include "PixelFactory/Types.hpp"
#include "PrimaryServices.hpp"
#include "Resources/Container.hpp"
#include "Resources/Manager.hpp"
#include "Scenes/AbstractEntity.hpp"
#include "Scenes/AVConsole/Manager.hpp"
#include "Scenes/Component/Abstract.hpp"
#include "Scenes/GroundLevelInterface.hpp"
#include "Scenes/LightSet.hpp"
#include "Scenes/Loaders/GLTFLoader.hpp"
#include "Scenes/Loaders/LoaderOptions.hpp"
#include "Scenes/Loaders/SceneData.hpp"
#include "Scenes/Node.hpp"
#include "Scenes/PhysicsRecorder.hpp"
#include "Scenes/Scene.hpp"
#include "Scenes/SceneDataConsumer.hpp"
#include "Scenes/SeaLevelInterface.hpp"
#include "Scenes/StaticEntity.hpp"
#include "String.hpp"
#include "VertexFactory/CapUVMapping.hpp"
#include "Vulkan/Image.hpp"
#include "Vulkan/TextureInterface.hpp"
#include "Vulkan/TransferManager.hpp"

/* Forward declarations. */
namespace EmEn::Console
{
	class Argument;
}

namespace EmEn::Scenes
{
	using namespace Base;

	/* NOTE: The hand-streamed JSON answers write their floats through the console's ONE rule (Console/JSONNumber.hpp). */
	using Console::jsonNumber;
	using Console::writeJSONVector;

	namespace
	{
		/** @brief The largest exposure, in stops, dumpRenderTarget() accepts on either side: 2^64 is still a finite float. */
		constexpr auto MaxDumpExposureEV{64.0F};

		/**
		 * @brief Writes the state of a kinematic character's controller as a member of a JSON object (getNodePhysics).
		 * @param info The stream of the object.
		 * @param controller The character's controller.
		 */
		void
		writeCharacterState (std::stringstream & info, const auto & controller) noexcept
		{
			info << R"(,"character":{"grounded":)" << (controller.isGrounded() ? "true" : "false");
			info << R"(,"flying":)" << (controller.isFlying() ? "true" : "false");
			info << R"(,"velocity":)";
			writeJSONVector(info, controller.velocity());
			info << R"(,"groundNormal":)";
			writeJSONVector(info, controller.groundNormal());
			info << R"(,"supportKey":)" << controller.supportKey() << '}';
		}

		/**
		 * @brief Writes the drive train and the wheels of a vehicle as a member of a JSON object (getNodePhysics).
		 * @param info The stream of the object.
		 * @param controller The vehicle's controller.
		 */
		void
		writeVehicleState (std::stringstream & info, const auto & controller) noexcept
		{
			info << R"(,"vehicle":{"engineRPM":)" << jsonNumber(controller.engineRPM()) << R"(,"gear":)" << controller.gear() << R"(,"clutch":)" << jsonNumber(controller.clutch()) << R"(,"wheels":[)";

			const char * separator = "";

			for ( const auto & wheel : controller.wheels() )
			{
				info << separator << R"({"contact":)" << (wheel.contact ? "true" : "false");
				info << R"(,"suspensionLength":)" << jsonNumber(wheel.suspensionLength) << R"(,"angularVelocity":)" << jsonNumber(wheel.angularVelocity);
				info << R"(,"steerAngle":)" << jsonNumber(wheel.steerAngle) << R"(,"slipRatio":)" << jsonNumber(wheel.slipRatio) << R"(,"slipAngle":)" << jsonNumber(wheel.slipAngle);
				info << R"(,"suspensionImpulse":)" << jsonNumber(wheel.suspensionImpulse) << R"(,"longitudinalImpulse":)" << jsonNumber(wheel.longitudinalImpulse) << R"(,"lateralImpulse":)" << jsonNumber(wheel.lateralImpulse);
				info << R"(,"driveTorque":)" << jsonNumber(wheel.driveTorque) << R"(,"brakeTorque":)" << jsonNumber(wheel.brakeTorque);
				info << R"(,"longitudinalSpeed":)" << jsonNumber(wheel.longitudinalSpeed) << R"(,"load":)" << jsonNumber(wheel.load) << R"(,"tractionFactor":)" << jsonNumber(wheel.tractionFactor) << R"(,"brakeFactor":)" << jsonNumber(wheel.brakeFactor) << '}';
				separator = ",";
			}

			info << "]}";
		}

		/**
		 * @brief Returns the physics state of a node as a JSON object (getNodePhysics).
		 * @note Called under the scene's exclusive access: the logic thread moves the node otherwise.
		 * @param node The node.
		 * @param sceneCycle The physics cycle this state belongs to.
		 * @return std::string
		 */
		[[nodiscard]]
		std::string
		nodePhysicsJSON (const Node & node, size_t sceneCycle) noexcept
		{
			/* ⚠️ WORLD position, not local: a node parented under another reports a local position
			 * that says nothing about where it rests in the world, which is what a physics
			 * measurement is about. */
			const auto worldCoordinates = node.getWorldCoordinates();
			const auto worldPosition = worldCoordinates.position();
			const auto & localPosition = node.localCoordinates().position();
			const auto & velocity = node.linearVelocity();

			const auto * groundedSource = "None";

			switch ( node.groundedSource() )
			{
				case Physics::GroundedSource::Ground : groundedSource = "Ground"; break;
				case Physics::GroundedSource::Boundary : groundedSource = "Boundary"; break;
				case Physics::GroundedSource::Entity : groundedSource = "Entity"; break;
				case Physics::GroundedSource::None : break;
			}

			std::stringstream info;
			info << "{";
			info << "\"name\":" << Json::valueToQuotedString(node.name().c_str(), node.name().size()) << ",";
			info << "\"worldPosition\":";
			writeJSONVector(info, worldPosition);
			info << ",\"localPosition\":";
			writeJSONVector(info, localPosition);
			info << ",\"worldUpward\":";
			writeJSONVector(info, worldCoordinates.upwardVector());
			info << ",\"worldBackward\":";
			writeJSONVector(info, worldCoordinates.backwardVector());
			info << ",\"linearVelocity\":";
			writeJSONVector(info, velocity);
			info << ",\"angularVelocity\":";
			writeJSONVector(info, node.angularVelocity());
			/* The point the physics turns the body about: the explicit centre of mass, else its shape's centroid. */
			info << ",\"worldCenterOfMass\":";
			writeJSONVector(info, node.getWorldCenterOfMass());
			info << ",";
			info << "\"movable\":" << (node.isMovable() ? "true" : "false") << ",";
			info << "\"simulationPaused\":" << (node.isSimulationPaused() ? "true" : "false") << ",";
			info << "\"grounded\":" << (node.isGrounded() ? "true" : "false") << ",";
			info << R"("groundedSource":")" << groundedSource << "\",";
			/* The physics cycle this state belongs to: a client polling at its own rate samples by cycle,
			 * never by wall-clock time (the physics ticks at a fixed WorldPhysicsUpdateFrequency). */
			info << "\"sceneCycle\":" << sceneCycle;

			/* A kinematic character (P4): its controller's state (its node velocity is not what it does). */
			if ( const auto character = node.characterController(); character != nullptr )
			{
				writeCharacterState(info, character->controller());
			}

			/* A wheeled vehicle: its drive train and its wheels (decisions 15). */
			if ( const auto vehicle = node.vehicle(); vehicle != nullptr )
			{
				writeVehicleState(info, vehicle->controller());
			}
			info << "}";

			return info.str();
		}

		/** @brief The share of the pixels the automatic exposure of a dump keeps at or below 1. */
		constexpr auto AutomaticExposureShare{0.99};

		/**
		 * @brief Writes 0 into every pixel which holds a non-finite component.
		 * @note Neither an 8-bit rendition nor the Radiance RGBE encoding has a value for a NaN or an infinity: the
		 * count is what tells the reader that the image held some.
		 * @param pixmap A reference to a pixmap of floats.
		 * @return size_t The number of pixels changed.
		 */
		[[nodiscard]]
		size_t
		zeroNonFinitePixels (PixelFactory::Pixmap< float > & pixmap) noexcept
		{
			const std::span< float > values{pixmap.data()};
			const auto colorCount = static_cast< size_t >(pixmap.colorCount());

			size_t count = 0;

			for ( size_t offset = 0; colorCount > 0 && offset + colorCount <= values.size(); offset += colorCount )
			{
				const auto pixel = values.subspan(offset, colorCount);

				if ( std::ranges::all_of(pixel, [] (float value) {return std::isfinite(value);}) )
				{
					continue;
				}

				std::ranges::fill(pixel, 0.0F);

				++count;
			}

			return count;
		}

		/**
		 * @brief Returns the value which a share of the pixels do not exceed on their brightest colour channel.
		 * @param pixmap A reference to an RGB or RGBA pixmap of FINITE floats (zeroNonFinitePixels()).
		 * @param share The share of the pixels, in [0, 1]: 0.99 gives the 99th percentile.
		 * @return float 0 for an empty pixmap or one with fewer than three channels.
		 */
		[[nodiscard]]
		float
		brightestChannelPercentile (const PixelFactory::Pixmap< float > & pixmap, double share) noexcept
		{
			const std::span< const float > values{pixmap.data()};
			const auto colorCount = static_cast< size_t >(pixmap.colorCount());

			if ( colorCount < 3 || values.size() < colorCount )
			{
				return 0.0F;
			}

			std::vector< float > peaks;
			peaks.reserve(values.size() / colorCount);

			for ( size_t offset = 0; offset + colorCount <= values.size(); offset += colorCount )
			{
				peaks.push_back(std::ranges::max(values.subspan(offset, 3)));
			}

			const auto rank = static_cast< std::ptrdiff_t >(std::clamp(share, 0.0, 1.0) * static_cast< double >(peaks.size() - 1));
			const auto position = std::next(peaks.begin(), rank);

			std::ranges::nth_element(peaks, position);

			return *position;
		}

		/**
		 * @brief Renders linear floats as an 8-bit sRGB image: each colour channel times a scale, clamped to [0, 1],
		 * then encoded with the sRGB transfer. The alpha channel is dropped.
		 * @param source A reference to an RGB or RGBA pixmap of floats.
		 * @param scale The factor applied before the transfer (2 to the power of the exposure in stops).
		 * @param destination A reference to the 8-bit RGB pixmap to fill.
		 * @return bool
		 */
		[[nodiscard]]
		bool
		exposeToSRGB (const PixelFactory::Pixmap< float > & source, float scale, PixelFactory::Pixmap< uint8_t > & destination) noexcept
		{
			const std::span< const float > values{source.data()};
			const auto colorCount = static_cast< size_t >(source.colorCount());

			if ( colorCount < 3 || !destination.initialize(source.width(), source.height(), PixelFactory::ChannelMode::RGB) )
			{
				return false;
			}

			auto output = destination.data().begin();

			for ( size_t offset = 0; offset + colorCount <= values.size(); offset += colorCount )
			{
				for ( const auto value : values.subspan(offset, 3) )
				{
					*output = PixelFactory::linearToSRGBByte(value * scale);

					++output;
				}
			}

			return true;
		}

		/** @brief What dumpRenderTarget() was asked, once its arguments are validated. */
		struct DumpRequest
		{
			/** The exposure of the 8-bit rendition of a floating-point target, in stops; automatic when absent. */
			std::optional< float > exposure;
			uint32_t arrayLayer{0};
		};

		/** @brief One layer of a render target's colour image to dump, and where its files go. */
		struct LayerDump
		{
			std::string targetId;
			/** The path of the files without their extension: <captures>/rt-dump-<timestamp>-layer<N>. */
			std::string basePath;
			uint32_t arrayLayer{0};
		};

		/**
		 * @brief Returns the factor applied to the linear values of a floating-point dump before the sRGB transfer.
		 * @param exposure The exposure asked for, in stops; absent for the automatic one.
		 * @param percentile The 99th percentile of the brightest channel (brightestChannelPercentile()).
		 * @return float
		 */
		[[nodiscard]]
		float
		dumpExposureScale (std::optional< float > exposure, float percentile) noexcept
		{
			if ( exposure.has_value() )
			{
				return std::exp2(*exposure);
			}

			/* NOTE: A black image has no percentile to bring to 1: it is left as it is. */
			return percentile > 0.0F ? 1.0F / percentile : 1.0F;
		}

		/**
		 * @brief Dumps one layer of an 8-bit colour image as a PNG.
		 * @param transferManager A reference to the transfer manager.
		 * @param image A reference to the colour image of the target.
		 * @param dump A reference to the layer and its destination.
		 * @param exposureGiven Whether an exposure was passed: it does not apply here, and the reply says so.
		 * @param outputs A reference to the console outputs.
		 * @return bool True when the file was written.
		 */
		[[nodiscard]]
		bool
		dumpLayerAsBytes (Vulkan::TransferManager & transferManager, const Vulkan::Image & image, const LayerDump & dump, bool exposureGiven, Console::Outputs & outputs) noexcept
		{
			PixelFactory::Pixmap< uint8_t > pixmap;

			/* NOTE: The render pass leaves the color attachment in SHADER_READ_ONLY_OPTIMAL (it is sampled). */
			if ( !transferManager.downloadImage(image, dump.arrayLayer, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT, pixmap) )
			{
				outputs.emplace_back(Severity::Error, std::stringstream{} << "Unable to download the image of '" << dump.targetId << "' !");

				return false;
			}

			const auto filepath = dump.basePath + ".png";

			if ( !PixelFactory::FileIO::write(pixmap, Base::IO::u8path(filepath)) )
			{
				outputs.emplace_back(Severity::Error, std::stringstream{} << "Unable to write the dump to " << filepath);

				return false;
			}

			if ( exposureGiven )
			{
				outputs.emplace_back(Severity::Warning, std::stringstream{} << "Render target '" << dump.targetId << "' is an 8-bit target: the exposure applies to a floating-point one, it was ignored.");
			}

			outputs.emplace_back(Severity::Success, std::stringstream{} << "Render target '" << dump.targetId << "' (layer " << dump.arrayLayer << ") dumped: " << filepath);

			return true;
		}

		/**
		 * @brief Dumps one layer of a floating-point (HDR) colour image: its linear values as a Radiance file, and an
		 * EXPOSED 8-bit sRGB image beside it (owner decision 2026-11-14).
		 * @note A clamp alone gives a white image on a photometric scene: without an exposure, the 99th percentile of
		 * the brightest channel maps to 1, and the reply states what was applied.
		 * @param transferManager A reference to the transfer manager.
		 * @param image A reference to the colour image of the target.
		 * @param dump A reference to the layer and its destination.
		 * @param exposure The exposure asked for, in stops; absent for the automatic one.
		 * @param outputs A reference to the console outputs.
		 * @return bool True when both files were written.
		 */
		[[nodiscard]]
		bool
		dumpLayerAsFloats (Vulkan::TransferManager & transferManager, const Vulkan::Image & image, const LayerDump & dump, std::optional< float > exposure, Console::Outputs & outputs) noexcept
		{
			PixelFactory::Pixmap< float > linear;

			if ( !transferManager.downloadImage(image, dump.arrayLayer, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT, linear) )
			{
				outputs.emplace_back(Severity::Error, std::stringstream{} << "Unable to download the image of '" << dump.targetId << "' !");

				return false;
			}

			const auto nonFinitePixels = zeroNonFinitePixels(linear);
			const auto percentile = brightestChannelPercentile(linear, AutomaticExposureShare);
			const auto scale = dumpExposureScale(exposure, percentile);
			const auto filepath = dump.basePath + ".png";
			const auto linearFilepath = dump.basePath + ".hdr";

			PixelFactory::Pixmap< uint8_t > rendition;

			if ( !exposeToSRGB(linear, scale, rendition) || !PixelFactory::FileIO::write(rendition, Base::IO::u8path(filepath)) )
			{
				outputs.emplace_back(Severity::Error, std::stringstream{} << "Unable to write the dump to " << filepath);

				return false;
			}

			if ( !PixelFactory::FileIO::write(linear, Base::IO::u8path(linearFilepath)) )
			{
				outputs.emplace_back(Severity::Error, std::stringstream{} << "Unable to write the linear values to " << linearFilepath);

				return false;
			}

			if ( nonFinitePixels > 0 )
			{
				outputs.emplace_back(Severity::Warning, std::stringstream{} << "Render target '" << dump.targetId << "' (layer " << dump.arrayLayer << ") holds " << nonFinitePixels << " pixel(s) with a non-finite value (NaN or infinity): written as 0 in both files.");
			}

			std::stringstream message;
			message << "Render target '" << dump.targetId << "' (layer " << dump.arrayLayer << ") dumped: " << filepath << " (8-bit sRGB, exposure " << std::log2(scale) << " EV, ";

			if ( exposure.has_value() )
			{
				message << "as asked; the 99th percentile of the brightest channel is " << percentile;
			}
			else
			{
				message << "automatic: the 99th percentile of the brightest channel, " << percentile << ", maps to 1";
			}

			message << ") and " << linearFilepath << " (the linear values, RGB)";

			outputs.emplace_back(Severity::Success, message);

			return true;
		}

		/** @brief What the depths of an image layer are, for the reply of a dump and for its grey rendition. */
		struct DepthStatistics
		{
			double mean{0.0};
			size_t finiteCount{0};
			/** The pixels still at 1: nothing was drawn there since the clear. */
			size_t atFarPlane{0};
			float minimum{0.0F};
			float maximum{0.0F};
		};

		/**
		 * @brief Measures the depths of a one-channel pixmap, the non-finite values left aside.
		 * @param depths A reference to the pixmap.
		 * @return DepthStatistics
		 */
		[[nodiscard]]
		DepthStatistics
		measureDepths (const PixelFactory::Pixmap< float > & depths) noexcept
		{
			DepthStatistics statistics;
			double sum = 0.0;

			for ( const auto depth : depths.data() )
			{
				if ( !std::isfinite(depth) )
				{
					continue;
				}

				statistics.minimum = statistics.finiteCount == 0 ? depth : std::min(statistics.minimum, depth);
				statistics.maximum = statistics.finiteCount == 0 ? depth : std::max(statistics.maximum, depth);
				statistics.atFarPlane += depth >= 1.0F ? 1U : 0U;

				sum += static_cast< double >(depth);

				++statistics.finiteCount;
			}

			if ( statistics.finiteCount > 0 )
			{
				statistics.mean = sum / static_cast< double >(statistics.finiteCount);
			}

			return statistics;
		}

		/**
		 * @brief Renders depths as an 8-bit grey image: the range found stretched from black (the nearest) to white.
		 * @note A raw depth is unreadable as it is (a perspective depth crowds near 1): the stretch shows the shapes,
		 * the statistics of the reply and the float file give the values.
		 * @param depths A reference to the one-channel pixmap of depths.
		 * @param statistics A reference to its statistics (measureDepths()).
		 * @param destination A reference to the 8-bit one-channel pixmap to fill.
		 * @return bool
		 */
		[[nodiscard]]
		bool
		stretchToGrey (const PixelFactory::Pixmap< float > & depths, const DepthStatistics & statistics, PixelFactory::Pixmap< uint8_t > & destination) noexcept
		{
			if ( depths.colorCount() != 1 || !destination.initialize(depths.width(), depths.height(), PixelFactory::ChannelMode::Grayscale) )
			{
				return false;
			}

			const auto range = statistics.maximum - statistics.minimum;

			std::ranges::transform(depths.data(), destination.data().begin(), [&statistics, range] (float depth) {
				/* A flat image or a non-finite value has no place in the range: drawn black. */
				if ( !(range > 0.0F) || !std::isfinite(depth) )
				{
					return uint8_t{0};
				}

				return static_cast< uint8_t >(std::lround(255.0F * std::clamp((depth - statistics.minimum) / range, 0.0F, 1.0F)));
			});

			return true;
		}

		/**
		 * @brief Dumps one layer of a depth image (a shadow map): an 8-bit grey PNG to look at, and the depths
		 * themselves as a Portable FloatMap (owner decisions 2026-11-14).
		 * @param transferManager A reference to the transfer manager.
		 * @param image A reference to the depth image of the target.
		 * @param dump A reference to the layer and its destination.
		 * @param exposureGiven Whether an exposure was passed: it does not apply here, and the reply says so.
		 * @param outputs A reference to the console outputs.
		 * @return bool True when both files were written.
		 */
		[[nodiscard]]
		bool
		dumpLayerAsDepth (Vulkan::TransferManager & transferManager, const Vulkan::Image & image, const LayerDump & dump, bool exposureGiven, Console::Outputs & outputs) noexcept
		{
			/* NOTE: Said here with its remedy: the download would only name the missing usage. */
			if ( (image.createInfo().usage & VK_IMAGE_USAGE_TRANSFER_SRC_BIT) == 0 )
			{
				outputs.emplace_back(Severity::Error, std::stringstream{} << "Render target '" << dump.targetId << "' holds a depth image created without read-back: launch with " << Graphics::Renderer::DepthReadbackArgument << " to dump it.");

				return false;
			}

			PixelFactory::Pixmap< float > depths;

			/* NOTE: A shadow map rests in DEPTH_STENCIL_READ_ONLY_OPTIMAL between two frames: its render pass ends there. */
			if ( !transferManager.downloadImage(image, dump.arrayLayer, VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL, VK_IMAGE_ASPECT_DEPTH_BIT, depths) )
			{
				outputs.emplace_back(Severity::Error, std::stringstream{} << "Unable to download the depth image of '" << dump.targetId << "' !");

				return false;
			}

			const auto statistics = measureDepths(depths);
			const auto filepath = dump.basePath + ".png";
			const auto depthFilepath = dump.basePath + ".pfm";

			PixelFactory::Pixmap< uint8_t > rendition;

			if ( !stretchToGrey(depths, statistics, rendition) || !PixelFactory::FileIO::write(rendition, Base::IO::u8path(filepath)) )
			{
				outputs.emplace_back(Severity::Error, std::stringstream{} << "Unable to write the dump to " << filepath);

				return false;
			}

			if ( !PixelFactory::FileIO::write(depths, Base::IO::u8path(depthFilepath)) )
			{
				outputs.emplace_back(Severity::Error, std::stringstream{} << "Unable to write the depths to " << depthFilepath);

				return false;
			}

			if ( exposureGiven )
			{
				outputs.emplace_back(Severity::Warning, std::stringstream{} << "Render target '" << dump.targetId << "' is a depth target: the exposure applies to a floating-point colour one, it was ignored.");
			}

			const auto pixelCount = depths.data().size();

			outputs.emplace_back(Severity::Success, std::stringstream{} << "Render target '" << dump.targetId << "' (layer " << dump.arrayLayer << ") dumped: " << filepath << " (8-bit grey, the depths " << statistics.minimum << " to " << statistics.maximum << " stretched from black to white) and " << depthFilepath << " (the depths, 32-bit floats). Mean " << statistics.mean << "; " << statistics.atFarPlane << " of " << pixelCount << " pixel(s) at the far plane (1); " << (pixelCount - statistics.finiteCount) << " non-finite.");

			return true;
		}

		/**
		 * @brief Dumps the requested layer of a render-to-texture target or of a shadow map into the captures directory.
		 * @param scene A reference to the scene which owns the target.
		 * @param renderTarget A reference to the render target.
		 * @param request A reference to the validated request.
		 * @param outputs A reference to the console outputs: what was written, or why nothing was.
		 * @return bool True when the layer was dumped.
		 */
		[[nodiscard]]
		bool
		dumpRenderTargetLayer (Scene & scene, const Graphics::RenderTarget::Abstract & renderTarget, const DumpRequest & request, Console::Outputs & outputs) noexcept
		{
			const auto * textureInterface = dynamic_cast< const Vulkan::TextureInterface * >(&renderTarget);

			if ( textureInterface == nullptr || textureInterface->image() == nullptr )
			{
				outputs.emplace_back(Severity::Error, std::stringstream{} << "Render target '" << renderTarget.id() << "' has no readable image !");

				return false;
			}

			/* A target that never rendered still holds its image in the UNDEFINED layout:
			 * the SHADER_READ_ONLY readback below is then undefined behaviour (measured:
			 * process crash, even after waitIdle). The on-demand policies make this state
			 * perfectly reachable — an unfired "once" probe, a suspended target. */
			if ( !renderTarget.hasBeenRendered() )
			{
				outputs.emplace_back(Severity::Error, std::stringstream{} << "Render target '" << renderTarget.id() << "' has never been rendered — nothing to dump yet.");

				return false;
			}

			const auto & image = *textureInterface->image();

			/* NOTE: Refused here with the target's name; the download refuses it too, never clamps it. */
			if ( const auto layerCount = image.createInfo().arrayLayers; request.arrayLayer >= layerCount )
			{
				outputs.emplace_back(Severity::Error, std::stringstream{} << "Render target '" << renderTarget.id() << "' has " << layerCount << " layer(s): the layer " << request.arrayLayer << " does not exist !");

				return false;
			}

			auto & renderer = scene.AVConsoleManager().graphicsRenderer();

			/* NOTE: This runs on the console thread while the render thread draws INTO this very target. Nothing is
			 * waited for here: the transfer manager submits the copy on the queue which renders the target, and that
			 * queue's order places it between two passes (Vulkan::TransferManager::setReadBackQueue()). A
			 * Device::waitIdle() stood here until 2026-11-15 — "brutal, but correct" — and was not: the render thread
			 * submitted its next pass before the copy, on another queue, and two faces of a shadow cube were read
			 * empty. */
			/* NOTE: The layer is in the name: two faces dumped within the same second would otherwise share it. One layer
			 * per call: layer 0 (the +X face of a cubemap) tells a black/dead probe from a live one, the others show what
			 * each face baked.
			 * ⚠️ The stem is UNIQUE, as a screenshot's (Graphics::FrameCapture::firstFreeStem()): a second already used
			 * by a dump of this layer moves to the next free one. Two dumps of one layer within a second used to share
			 * a name, and the second was refused ("the file ... already exists"): 4 of 10 in a series (2026-11-15). */
			const auto captures = renderer.primaryServices().fileSystem().userDataDirectory("captures");
			const auto layerName = "-layer" + std::to_string(request.arrayLayer);
			const std::array< std::string, 3 > fileEnds{layerName + ".png", layerName + ".hdr", layerName + ".pfm"};
			const std::array< std::string_view, 3 > suffixes{fileEnds[0], fileEnds[1], fileEnds[2]};

			const LayerDump dump{
				.targetId = renderTarget.id(),
				.basePath = Base::IO::toU8String(captures / Base::IO::u8path("rt-dump-" + std::to_string(Graphics::FrameCapture::firstFreeStem(captures, "rt-dump-", suffixes)) + layerName)),
				.arrayLayer = request.arrayLayer
			};

			if ( (image.createInfo().usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT) != 0 )
			{
				return dumpLayerAsDepth(renderer.transferManager(), image, dump, request.exposure.has_value(), outputs);
			}

			if ( image.createInfo().format == VK_FORMAT_R16G16B16A16_SFLOAT )
			{
				return dumpLayerAsFloats(renderer.transferManager(), image, dump, request.exposure, outputs);
			}

			return dumpLayerAsBytes(renderer.transferManager(), image, dump, request.exposure.has_value(), outputs);
		}
	}

	void
	Manager::onRegisterToConsole () noexcept
	{
		/* Component control (owner decision 2026-09-27): one sub-object per component type, every command naming
		 * its entity and component — no targeting state. */
		Component::appendLightConsoleAdapters(*this, m_componentConsoleAdapters);
		Component::appendCameraConsoleAdapters(*this, m_componentConsoleAdapters);
		Component::appendEnvironmentConsoleAdapters(*this, m_componentConsoleAdapters);
		Component::appendAnimationConsoleAdapters(*this, m_componentConsoleAdapters);
		Component::appendPhysicsConsoleAdapters(*this, m_componentConsoleAdapters);
		Component::appendAudioConsoleAdapters(*this, m_componentConsoleAdapters);
		Component::appendVisualConsoleAdapters(*this, m_componentConsoleAdapters);
		Component::appendBeamConsoleAdapters(*this, m_componentConsoleAdapters);
		Component::appendPathConsoleAdapters(*this, m_componentConsoleAdapters);

		for ( const auto & adapter : m_componentConsoleAdapters )
		{
			adapter->registerToObject(*this);
		}

		/* THE access of this service's commands: whatever reads or writes the RUNNING scene does it under the scene's
		 * exclusive access — between two rendering frames and two logic cycles —, never on a mere snapshot of the scene
		 * (activeSceneSnapshot() keeps the scene alive and orders nothing against the logic and rendering threads).
		 * The body takes the scene and answers.
		 * ⚠️ Inside: never the scene manager's accesses again (a shared access asked under this one waits for itself),
		 * and nothing long — a resource is prepared BEFORE, a file is written AFTER. */
		const auto inActiveScene = [this] (const auto & body, Console::CommandResult noActiveScene = Console::CommandResult::error("No active scene !")) {
			auto result = std::move(noActiveScene);

			this->withExclusiveActiveScene([&result, &body] (const std::shared_ptr< Scene > & scene) {
				result = body(scene);
			}, true);

			return result;
		};

		/* A command that PREPARES something for the running scene (a resource to load or to generate): it prepares with
		 * no access held, then writes the scene here — if the scene it prepared for is still the active one. */
		const auto inSameActiveScene = [inActiveScene] (const std::shared_ptr< Scene > & preparedFor, const auto & body) {
			return inActiveScene([&preparedFor, &body] (const std::shared_ptr< Scene > & activeScene) {
				if ( activeScene != preparedFor )
				{
					return Console::CommandResult::error("The active scene changed while the command prepared its content: nothing was added !");
				}

				return body(activeScene);
			});
		};

		/* The legacy TARGET family (targetScene() then listNodes()…, item console-explicit-addressing): the targeted scene
		 * may be the RUNNING one — then it is read under its exclusive access like everything else; a scene that is not
		 * active is processed by no thread (and only this thread, the main one, enables a scene). */
		const auto inTargetedScene = [this] (const auto & body) {
			const auto scene = m_consoleMemory.scene();

			if ( scene == nullptr )
			{
				return Console::CommandResult::error("You must target a scene before !");
			}

			std::optional< Console::CommandResult > result;

			this->withExclusiveActiveScene([&result, &scene, &body] (const std::shared_ptr< Scene > & activeScene) {
				if ( activeScene == scene )
				{
					result = body(scene);
				}
			}, true);

			return result.has_value() ? std::move(*result) : body(scene);
		};

		this->bindCommand("listEntities", "Lists the entities of the ACTIVE scene as JSON: every node (whole hierarchy, with its parent) and static entity, with its ADDRESS (what the component commands take as `entity`: the name when unique, else the shortest unique path suffix Parent/Child) and its components (name and type).", [this] () {
			auto result = Console::CommandResult::error("No active scene !");

			this->withExclusiveActiveScene([&result] (const std::shared_ptr< Scene > & scene) {
				Json::Value entities{Json::arrayValue};

				const auto addresses = Component::entityAddresses(*scene);

				const auto describe = [&addresses] (const AbstractEntity & entity, const char * kind) {
					Json::Value entry{Json::objectValue};
					entry["name"] = entity.name();
					entry["kind"] = kind;

					if ( const auto addressIt = addresses.find(&entity); addressIt != addresses.end() )
					{
						entry["address"] = addressIt->second;
					}

					Json::Value components{Json::arrayValue};

					entity.forEachComponent([&components] (const Component::Abstract & component) {
						Json::Value item{Json::objectValue};
						item["name"] = component.name();
						item["type"] = component.getComponentType();
						components.append(std::move(item));
					});

					entry["components"] = std::move(components);

					return entry;
				};

				/* Depth first from the root's children: the root itself is not an entity anyone names. */
				std::vector< std::shared_ptr< Node > > pending;

				for ( const auto & child : scene->root()->children() | std::views::values )
				{
					pending.emplace_back(child);
				}

				while ( !pending.empty() )
				{
					const auto node = pending.back();
					pending.pop_back();

					auto entry = describe(*node, "node");

					if ( const auto parent = node->parent(); parent != nullptr && !parent->isRoot() )
					{
						entry["parent"] = parent->name();
					}

					entities.append(std::move(entry));

					for ( const auto & child : node->children() | std::views::values )
					{
						pending.emplace_back(child);
					}
				}

				scene->forEachStaticEntities([&entities, &describe] (const StaticEntity & entity) {
					entities.append(describe(entity, "staticEntity"));
				});

				result = Console::CommandResult::json(FastJSON::stringify(entities));
			}, true);

			return result;
		}, Console::CommandHint::ReadOnly);

		this->bindCommand("listEntityComponents", "Lists the components of one entity of the ACTIVE scene as JSON (name and type): the names the component commands take.",
			{
				{"entity", "The entity address: its name when unique, else the shortest unique path suffix Parent/Child (listEntities() gives every address)."}
			},
			[this] (const std::string & entityAddress) {
				auto result = Console::CommandResult::error("No active scene !");

				this->withExclusiveActiveScene([&result, &entityAddress] (const std::shared_ptr< Scene > & scene) {
					std::shared_ptr< AbstractEntity > entity;
					std::string error;

					if ( !Component::resolveEntity(*scene, entityAddress, entity, error) )
					{
						result = Console::CommandResult::error(error);

						return;
					}

					Json::Value components{Json::arrayValue};

					entity->forEachComponent([&components] (const Component::Abstract & component) {
						Json::Value item{Json::objectValue};
						item["name"] = component.name();
						item["type"] = component.getComponentType();
						components.append(std::move(item));
					});

					result = Console::CommandResult::json(FastJSON::stringify(components));
				}, true);

				return result;
			}, Console::CommandHint::ReadOnly);

		/* The selection outline's SET (one style for all): a command resolves the entity and applies one change. */
		const auto changeHighlight = [this] (const std::string & entityAddress, const auto & change) {
			auto result = Console::CommandResult::error("No active scene !");

			this->withExclusiveActiveScene([&result, &entityAddress, &change] (const std::shared_ptr< Scene > & scene) {
				std::shared_ptr< AbstractEntity > entity;
				std::string error;

				if ( !Component::resolveEntity(*scene, entityAddress, entity, error) )
				{
					result = Console::CommandResult::error(error);

					return;
				}

				result = change(*scene, entity);
			}, true);

			return result;
		};

		const Console::Parameter highlightEntityParameter{"entity", "The entity address: its name when unique, else the shortest unique path suffix Parent/Child (listEntities() gives every address)."};

		this->bindCommand("highlightEntity", "Outlines ONE entity of the ACTIVE scene (the selection outline: full where visible, dimmed where hidden), replacing the highlighted set.",
			{highlightEntityParameter},
			[changeHighlight] (const std::string & entityAddress) {
				return changeHighlight(entityAddress, [&entityAddress] (Scene & scene, const std::shared_ptr< AbstractEntity > & entity) {
					scene.setHighlightedEntity(entity);

					return Console::CommandResult::success("Entity '" + entityAddress + "' highlighted.");
				});
			}, Console::CommandHint::Idempotent);

		this->bindCommand("addHighlight", "Adds an entity of the ACTIVE scene to the highlighted set (one outline style for the whole set).",
			{highlightEntityParameter},
			[changeHighlight] (const std::string & entityAddress) {
				return changeHighlight(entityAddress, [&entityAddress] (Scene & scene, const std::shared_ptr< AbstractEntity > & entity) {
					return scene.addHighlightedEntity(entity) ?
						Console::CommandResult::success("Entity '" + entityAddress + "' added to the highlight.") :
						Console::CommandResult::success("Entity '" + entityAddress + "' was already highlighted.");
				});
			}, Console::CommandHint::Idempotent);

		this->bindCommand("removeHighlight", "Removes an entity of the ACTIVE scene from the highlighted set.",
			{highlightEntityParameter},
			[changeHighlight] (const std::string & entityAddress) {
				return changeHighlight(entityAddress, [&entityAddress] (Scene & scene, const std::shared_ptr< AbstractEntity > & entity) {
					return scene.removeHighlightedEntity(entity) ?
						Console::CommandResult::success("Entity '" + entityAddress + "' removed from the highlight.") :
						Console::CommandResult::success("Entity '" + entityAddress + "' was not highlighted.");
				});
			}, Console::CommandHint::Idempotent);

		this->bindCommand("clearHighlight", "Removes the selection outline of the ACTIVE scene (empties the highlighted set).", [this] () {
			auto result = Console::CommandResult::error("No active scene !");

			this->withExclusiveActiveScene([&result] (const std::shared_ptr< Scene > & scene) {
				scene->clearHighlightedEntities();

				result = Console::CommandResult::success("Highlight cleared.");
			}, true);

			return result;
		}, Console::CommandHint::Idempotent);

		this->bindCommand("getHighlights", "Returns the addresses of the highlighted entities of the ACTIVE scene, as a JSON array (in the order they were added).", [this] () {
			auto result = Console::CommandResult::error("No active scene !");

			this->withExclusiveActiveScene([&result] (const std::shared_ptr< Scene > & scene) {
				const auto addresses = Component::entityAddresses(*scene);

				Json::Value list{Json::arrayValue};

				for ( const auto & entity : scene->highlightedEntities() )
				{
					const auto found = addresses.find(entity.get());

					list.append(found != addresses.end() ? found->second : entity->name());
				}

				result = Console::CommandResult::json(FastJSON::stringify(list));
			}, true);

			return result;
		}, Console::CommandHint::ReadOnly);

		this->bindCommand("setHighlightStyle", "Sets the look of the selection outline (engine-wide).",
			{
				{"red", "Displayed (sRGB) red, 0-1."},
				{"green", "Displayed (sRGB) green, 0-1."},
				{"blue", "Displayed (sRGB) blue, 0-1."},
				{"width", "The line width, in pixels (1-8)."},
				{"hiddenOpacity", "The opacity where the entity is hidden behind other geometry, 0-1 (0 = visible parts only, 1 = x-ray)."}
			},
			[this] (float red, float green, float blue, float width, float hiddenOpacity) {
				/* The style belongs to the renderer and the rendering frame reads it: changed between two frames — the
				 * exclusive access holds them off —, with or without an active scene. */
				this->withExclusiveActiveScene([this, red, green, blue, width, hiddenOpacity] (const std::shared_ptr< Scene > & /*activeScene*/) {
					auto & outline = m_resourceManager.graphicsRenderer().selectionOutline();

					outline.setColor(Base::PixelFactory::Color< float >{red, green, blue, 1.0F});
					outline.setWidth(width);
					outline.setHiddenOpacity(hiddenOpacity);
				}, false);

				return Console::CommandResult::success("Highlight style set.");
			}, Console::CommandHint::Idempotent);

		this->bindCommand("createScene", "Creates a scene with a camera + microphone node and a neutral ambient light (5000 lx), then enables it.",
			{
				{"name", "The scene name (must not exist yet)."},
				{"boundary", "The scene half-size, in metres (the ground, if any, spans twice this)."},
				{"cameraNode", "The name of the node that carries the primary camera and microphone."},
				{"camX", "Camera world X, in metres."},
				{"camY", "Camera world Y, in metres (UP is +Y)."},
				{"camZ", "Camera world Z, in metres."},
				{"backgroundName", "A skybox resource name (listResources(SkyBoxResource)); an unknown name is an error; no background when omitted."},
				{"groundMaterial", "A material resource name, or 'default'; no ground when omitted."}
			},
			[this] (const std::string & name, float boundary, const std::string & cameraNodeName, float camX, float camY, float camZ, const std::optional< std::string > & backgroundName, const std::optional< std::string > & groundMaterialName) {
				if ( this->hasSceneNamed(name) )
				{
					return Console::CommandResult::error("Scene '" + name + "' already exists !");
				}

				Console::Outputs outputs;

				/* Optional background. */
				std::shared_ptr< Graphics::Renderable::AbstractBackground > background;

				if ( backgroundName.has_value() )
				{
					auto * skyBoxContainer = m_resourceManager.container< Graphics::Renderable::SkyBoxResource >();

					/* ⚠️ Checked BEFORE the scene exists (owner decision 2026-09-27): getResource() silently
					 * answers the "Default" skybox for an unknown name, so a typo used to build a scene with
					 * the wrong sky and report success. */
					if ( skyBoxContainer == nullptr || !skyBoxContainer->isResourceExists(*backgroundName) )
					{
						return Console::CommandResult::error("Unknown skybox '" + *backgroundName + "' (listResources(SkyBoxResource) lists them). No scene was created.");
					}

					background = skyBoxContainer->getResource(*backgroundName);
				}

				/* Optional ground. */
				std::shared_ptr< Scenes::GroundLevelInterface > groundLevel;

				if ( groundMaterialName.has_value() )
				{
					const auto & matName = *groundMaterialName;

					std::shared_ptr< Graphics::Material::Interface > materialResource;

					if ( matName == "default" )
					{
						materialResource = m_resourceManager.container< Graphics::Material::StandardResource >()->getDefaultResource();
					}
					else
					{
						materialResource = m_resourceManager.container< Graphics::Material::StandardResource >()->getResource(matName);
					}

					if ( materialResource == nullptr )
					{
						outputs.emplace_back(Severity::Warning, std::stringstream{} << "Ground material '" << matName << "' not found, skipping ground.");
					}
					else
					{
						auto ground = std::make_shared< Graphics::Renderable::BasicGroundResource >(m_resourceManager, "ConsoleGround");

						if ( ground->load(boundary * 2.0F, 8, materialResource, {}, boundary * 2.0F) )
						{
							groundLevel = ground;
						}
						else
						{
							outputs.emplace_back(Severity::Warning, "Ground geometry failed to load !");
						}
					}
				}

				auto scene = this->newScene(name, boundary, background, groundLevel);

				if ( scene == nullptr )
				{
					outputs.emplace_back(Severity::Error, std::stringstream{} << "Failed to create scene '" << name << "' !");

					return Console::CommandResult::fromOutputs(std::move(outputs), false);
				}

				/* Create camera+microphone node BEFORE enabling the scene. */
				auto cameraNode = scene->root()->createChild(cameraNodeName, {}, 0);

				if ( cameraNode != nullptr )
				{
					cameraNode->setPosition({camX, camY, camZ}, Base::Math::TransformSpace::World);
					cameraNode->componentBuilder< Component::Camera >(cameraNodeName + "Camera").asPrimary().build(true);
					cameraNode->componentBuilder< Component::Microphone >(cameraNodeName + "Microphone").asPrimary().build();
				}

				/* Setup a neutral ambient lighting (photometric: the intensity is an
				 * illuminance in lux, overcast-like default). */
				scene->lightSet().enable();
				scene->lightSet().setAmbientLightColor({1.0F, 1.0F, 1.0F, 1.0F});
				scene->lightSet().setAmbientLightIntensity(5000.0F);

				if ( this->enableScene(scene) )
				{
					outputs.emplace_back(Severity::Success, std::stringstream{} << "Scene '" << name << "' created and enabled (boundary: " << boundary << ", camera: " << cameraNodeName << ").");
				}
				else
				{
					outputs.emplace_back(Severity::Warning, std::stringstream{} << "Scene '" << name << "' created but failed to enable.");
				}

				return Console::CommandResult::fromOutputs(std::move(outputs), true);
			});

		this->bindCommand("enableScene", "Enables (activates) a registered scene.",
			{
				{"name", "The scene name (listScenes() lists them)."}
			},
			[this] (const std::string & name) {
				const auto scene = this->getScene(name);

				if ( scene == nullptr )
				{
					return Console::CommandResult::error("Scene '" + name + "' not found !");
				}

				if ( !this->enableScene(scene) )
				{
					return Console::CommandResult::error("Failed to enable scene '" + name + "' !");
				}

				return Console::CommandResult::success("Scene '" + name + "' enabled.");
			}, Console::CommandHint::Idempotent);

		this->bindCommand("deleteScene", "Deletes a registered scene.",
			{
				{"name", "The scene name (listScenes() lists them)."}
			},
			[this] (const std::string & name) {
				if ( !this->deleteScene(name) )
				{
					return Console::CommandResult::error("Failed to delete scene '" + name + "' !");
				}

				return Console::CommandResult::success("Scene '" + name + "' deleted.");
			}, Console::CommandHint::Destructive);

		this->bindCommand("createNode", "Creates a root-level node in the active scene, at the origin or at a world position (give all three coordinates or none).",
			{
				{"name", "The node name."},
				{"x", "World X, in metres."},
				{"y", "World Y, in metres (UP is +Y)."},
				{"z", "World Z, in metres."}
			},
			[inActiveScene] (const std::string & name, std::optional< float > x, std::optional< float > y, std::optional< float > z) {
				const auto coordinateCount = static_cast< int >(x.has_value()) + static_cast< int >(y.has_value()) + static_cast< int >(z.has_value());

				if ( coordinateCount != 0 && coordinateCount != 3 )
				{
					return Console::CommandResult::error("createNode(): give all three coordinates (x, y, z) or none.");
				}

				return inActiveScene([&] (const std::shared_ptr< Scene > & activeScene) {
					auto node = activeScene->root()->createChild(name, {}, activeScene->lifetimeMS());

					if ( node == nullptr )
					{
						return Console::CommandResult::error("Failed to create node '" + name + "' !");
					}

					std::stringstream message;

					if ( coordinateCount == 3 )
					{
						node->setPosition({*x, *y, *z}, Base::Math::TransformSpace::World);

						message << "Node '" << name << "' created at (" << *x << ", " << *y << ", " << *z << ").";
					}
					else
					{
						message << "Node '" << name << "' created at origin.";
					}

					return Console::CommandResult::success(message.str());
				});
			});

		this->bindCommand("destroyNode", "Destroys a root-level node of the active scene.",
			{
				{"name", "The node name."}
			},
			[inActiveScene] (const std::string & name) {
				return inActiveScene([&] (const std::shared_ptr< Scene > & activeScene) {
					if ( !activeScene->root()->destroyChild(name) )
					{
						return Console::CommandResult::error("Node '" + name + "' not found !");
					}

					return Console::CommandResult::success("Node '" + name + "' destroyed.");
				});
			}, Console::CommandHint::Destructive);

		this->bindCommand("attachCamera", "Attaches a primary camera to a node of the active scene and sets it as the active camera.",
			{
				{"nodeName", "The node that will carry the camera."},
				{"cameraName", "The name of the new camera component."}
			},
			[inActiveScene] (const std::string & nodeName, const std::string & cameraName) {
				return inActiveScene([&] (const std::shared_ptr< Scene > & activeScene) {
					auto node = activeScene->root()->findChild(nodeName);

					if ( node == nullptr )
					{
						return Console::CommandResult::error("Node '" + nodeName + "' not found !");
					}

					const auto camera = node->componentBuilder< Component::Camera >(cameraName).asPrimary().build(true);

					if ( camera == nullptr )
					{
						return Console::CommandResult::error("Failed to attach camera '" + cameraName + "' !");
					}

					/* Set as the active camera for the scene. */
					activeScene->setActiveCamera(camera);

					return Console::CommandResult::success("Camera '" + cameraName + "' attached to node '" + nodeName + "' and set as active.");
				});
			});

		this->bindCommand("attachMicrophone", "Attaches a primary microphone to a node of the active scene (the default microphone node is removed).",
			{
				{"nodeName", "The node that will carry the microphone."},
				{"microphoneName", "The name of the new microphone component."}
			},
			[inActiveScene] (const std::string & nodeName, const std::string & micName) {
				return inActiveScene([&] (const std::shared_ptr< Scene > & activeScene) {
					auto node = activeScene->root()->findChild(nodeName);

					if ( node == nullptr )
					{
						return Console::CommandResult::error("Node '" + nodeName + "' not found !");
					}

					/* Remove default microphone node if it exists. */
					activeScene->root()->destroyChild("DefaultMicrophoneNode");

					const auto microphone = node->componentBuilder< Component::Microphone >(micName).asPrimary().build();

					if ( microphone == nullptr )
					{
						return Console::CommandResult::error("Failed to attach microphone '" + micName + "' !");
					}

					return Console::CommandResult::success("Microphone '" + micName + "' attached to node '" + nodeName + "'.");
				});
			});

		this->bindCommand("setGround", "Sets a flat ground spanning the active scene boundary.",
			{
				{"materialName", "A material resource name, or 'default'.", "default"}
			},
			[this, inSameActiveScene] (const std::string & matName) {
				/* The scene this command prepares for: re-checked under the exclusive access, where the scene is written. */
				const auto preparedFor = this->activeSceneSnapshot();

				if ( preparedFor == nullptr )
				{
					return Console::CommandResult::error("No active scene !");
				}

				/* NOTE: A scene's boundary never changes. */
				const auto boundary = preparedFor->boundary();

				std::shared_ptr< Graphics::Material::Interface > materialResource;

				if ( matName == "default" )
				{
					materialResource = m_resourceManager.container< Graphics::Material::StandardResource >()->getDefaultResource();
				}
				else
				{
					materialResource = m_resourceManager.container< Graphics::Material::StandardResource >()->getResource(matName);
				}

				if ( materialResource == nullptr )
				{
					return Console::CommandResult::error("Material '" + matName + "' not found !");
				}

				auto ground = std::make_shared< Graphics::Renderable::BasicGroundResource >(m_resourceManager, "ConsoleGround");

				if ( !ground->load(boundary, 8, materialResource, {}, boundary) )
				{
					return Console::CommandResult::error("Ground geometry failed to load !");
				}

				return inSameActiveScene(preparedFor, [&] (const std::shared_ptr< Scene > & activeScene) {
					activeScene->setGroundLevel(ground);

					return Console::CommandResult::success("Ground set with material '" + matName + "'.");
				});
			});

		this->bindCommand("getSea", "Returns the sea of the active scene as JSON: its 'level' at rest (m) and 'shelter', how the land calms it — 'minimumPassage' and 'fade' (m), the 'swellShare', 'waveShare' and 'rippleShare' a sheltered water keeps — or null for a sea the land does not calm.",
			[inActiveScene] () {
				return inActiveScene([] (const std::shared_ptr< Scene > & activeScene) {
					const auto seaLevel = activeScene->seaLevel();

					if ( seaLevel == nullptr )
					{
						return Console::CommandResult::error("The active scene has no sea !");
					}

					Json::Value sea{Json::objectValue};
					sea["level"] = static_cast< double >(seaLevel->getLevel());
					sea["shelter"] = Json::Value{Json::nullValue};

					if ( const auto settings = seaLevel->shelterSettings(); settings.has_value() )
					{
						Json::Value shelter{Json::objectValue};
						shelter["minimumPassage"] = static_cast< double >(settings->minimumPassage);
						shelter["fade"] = static_cast< double >(settings->fade);
						shelter["swellShare"] = static_cast< double >(settings->swellShare);
						shelter["waveShare"] = static_cast< double >(settings->waveShare);
						shelter["rippleShare"] = static_cast< double >(settings->rippleShare);
						sea["shelter"] = shelter;
					}

					sea["lightThroughWater"] = activeScene->isSeaLightEnabled();
					sea["caustics"] = Json::Value{Json::nullValue};

					if ( const auto settings = seaLevel->causticSettings(); settings.has_value() )
					{
						Json::Value caustics{Json::objectValue};
						caustics["strength"] = static_cast< double >(settings->strength);
						caustics["softening"] = static_cast< double >(settings->softening);
						caustics["fadeDepth"] = static_cast< double >(settings->fadeDepth);
						caustics["temporalResponse"] = static_cast< double >(settings->temporalResponse);
						sea["caustics"] = caustics;
					}

					sea["shafts"] = Json::Value{Json::nullValue};

					if ( const auto settings = seaLevel->shaftSettings(); settings.has_value() )
					{
						Json::Value shafts{Json::objectValue};
						shafts["strength"] = static_cast< double >(settings->strength);
						shafts["anisotropy"] = static_cast< double >(settings->anisotropy);
						sea["shafts"] = shafts;
					}

					{
						const auto settings = seaLevel->soundSettings();

						Json::Value sound{Json::objectValue};
						sound["gain"] = static_cast< double >(settings.gain);
						sound["gainHF"] = static_cast< double >(settings.gainHF);
						sound["reverb"] = static_cast< double >(settings.reverb);
						sound["transitionDepth"] = static_cast< double >(settings.transitionDepth);
						sound["listenerImmersion"] = static_cast< double >(activeScene->listenerImmersion());
						sea["sound"] = sound;
					}

					return Console::CommandResult::json(FastJSON::stringify(sea));
				});
			}, Console::CommandHint::ReadOnly);

		this->bindCommand("setSeaShelteredShares", "Sets what a water sheltered by the land keeps of the sea's waves, applied to the next frame: 0 nothing, 1 everything.",
			{
				{"swell", "The share of the swell (the largest waves), 0-1."},
				{"waves", "The share of the middle waves, 0-1."},
				{"ripples", "The share of the ripples (the smallest waves), 0-1."}
			},
			[inActiveScene] (float swell, float waves, float ripples) {
				return inActiveScene([&] (const std::shared_ptr< Scene > & activeScene) {
					auto settings = activeScene->seaShelterSettings();

					if ( !settings.has_value() )
					{
						return Console::CommandResult::error("The active scene has no sea the land calms !");
					}

					settings->swellShare = swell;
					settings->waveShare = waves;
					settings->rippleShare = ripples;

					if ( !settings->isValid() || !activeScene->setSeaShelterSettings(*settings) )
					{
						return Console::CommandResult::error("Refused: each share must be a number from 0 to 1 !");
					}

					return Console::CommandResult::success("Sheltered shares set.");
				});
			}, Console::CommandHint::Idempotent);

		this->bindCommand("setSeaShelter", "Sets which water the swell reaches and rebuilds the sea's shelter map at once (some tens of milliseconds): the narrowest passage the swell enters and the distance over which it calms down past it.",
			{
				{"minimumPassage", "The narrowest passage the swell enters, in metres (0 or more)."},
				{"fade", "The distance over which the sea calms down, in metres (0 or more)."}
			},
			[inActiveScene] (float minimumPassage, float fade) {
				return inActiveScene([&] (const std::shared_ptr< Scene > & activeScene) {
					auto settings = activeScene->seaShelterSettings();

					if ( !settings.has_value() )
					{
						return Console::CommandResult::error("The active scene has no sea the land calms !");
					}

					settings->minimumPassage = minimumPassage;
					settings->fade = fade;

					if ( !settings->isValid() || !activeScene->setSeaShelterSettings(*settings) )
					{
						return Console::CommandResult::error("Refused: each distance must be a number, 0 or more !");
					}

					return Console::CommandResult::success("Sea shelter set, its map rebuilt.");
				});
			}, Console::CommandHint::Idempotent);

		this->bindCommand("setSeaCaustics", "Sets the caustics the sea's waves draw under it, applied to the next frame: how much of them is drawn, how fast they soften with the depth, the depth they fade over, and how far the temporal passes give up their history under them.",
			{
				{"strength", "How much of the caustics is drawn: 0 none, 1 all of it."},
				{"softening", "The low-pass of the waves, in metres per metre of depth (0 or more): higher = a softer net sooner."},
				{"fadeDepth", "The depth the caustics fade over, in metres (more than 0)."},
				{"temporalResponse", "How far a temporal pass (anti-aliasing) gives up its history where the caustics are drawn in full: 0 not at all (it averages the moving pattern), 1 the most."}
			},
			[inActiveScene] (float strength, float softening, float fadeDepth, float temporalResponse) {
				return inActiveScene([&] (const std::shared_ptr< Scene > & activeScene) {
					const auto seaLevel = activeScene->seaLevel();

					if ( seaLevel == nullptr || !seaLevel->causticSettings().has_value() )
					{
						return Console::CommandResult::error("The active scene has no sea that draws caustics !");
					}

					const SeaCausticSettings settings{.strength = strength, .softening = softening, .fadeDepth = fadeDepth, .temporalResponse = temporalResponse};

					if ( !settings.isValid() || !seaLevel->setCausticSettings(settings) )
					{
						return Console::CommandResult::error("Refused: the strength must be a number from 0 to 1, the softening 0 or more, the fade depth more than 0, the temporal response from 0 to 1 !");
					}

					return Console::CommandResult::success("Sea caustics set.");
				});
			}, Console::CommandHint::Idempotent);

		this->bindCommand("setSeaShafts", "Sets the sun's shafts the sea draws in its water (seen from under it), applied to the next frame: how much of them is drawn, and how forward the water scatters the sun's light.",
			{
				{"strength", "How much of the shafts is drawn: 0 none (the water scatters the sun's light evenly), 1 all of it."},
				{"anisotropy", "The anisotropy of the scattering of the sun's light toward the eye, 0 to 0.95: 0 the same in every direction; higher, the shafts show looking toward the sun and the water darkens looking away from it."}
			},
			[inActiveScene] (float strength, float anisotropy) {
				return inActiveScene([&] (const std::shared_ptr< Scene > & activeScene) {
					const auto seaLevel = activeScene->seaLevel();

					if ( seaLevel == nullptr || !seaLevel->shaftSettings().has_value() )
					{
						return Console::CommandResult::error("The active scene has no sea that draws shafts !");
					}

					const SeaShaftSettings settings{.strength = strength, .anisotropy = anisotropy};

					if ( !settings.isValid() || !seaLevel->setShaftSettings(settings) )
					{
						return Console::CommandResult::error("Refused: the strength must be a number from 0 to 1, the anisotropy from 0 to 0.95 !");
					}

					return Console::CommandResult::success("Sea shafts set.");
				});
			}, Console::CommandHint::Idempotent);

		this->bindCommand("setSeaSound", "Sets what the sea does to the sound for a listener under its level, heard from the next logic cycle: every sound of the world is muffled and reverberates (the music is left alone). getSea answers the values and the listener's immersion.",
			{
				{"gain", "The gain of every sound heard from under the level, 0 to 1."},
				{"gainHF", "The gain of its high frequencies (a low-pass, a shelf at 5 kHz), 0 to 1: 1 muffles nothing, 0.02 leaves almost no treble."},
				{"reverb", "How much of the water's reverberation is heard, 0 (none) to 1."},
				{"transitionDepth", "The height over which the sound goes from the air's to the water's, centred on the sea level, in metres, 0.01 to 10."}
			},
			[inActiveScene] (float gain, float gainHF, float reverb, float transitionDepth) {
				return inActiveScene([&] (const std::shared_ptr< Scene > & activeScene) {
					const auto seaLevel = activeScene->seaLevel();

					if ( seaLevel == nullptr )
					{
						return Console::CommandResult::error("The active scene has no sea !");
					}

					const SeaSoundSettings settings{.gain = gain, .gainHF = gainHF, .reverb = reverb, .transitionDepth = transitionDepth};

					if ( !seaLevel->setSoundSettings(settings) )
					{
						return Console::CommandResult::error("Refused: the gain, the high-frequency gain and the reverberation must be numbers from 0 to 1, the transition depth from 0.01 to 10 !");
					}

					return Console::CommandResult::success("Sea sound set.");
				});
			}, Console::CommandHint::Idempotent);

		this->bindCommand("setSeaLight", "Sets whether the sea of the active scene acts on the light that goes under it (the absorption along the light's own path, the caustics): off, what lies under the sea level is lit as in the air — the comparison the look is judged with.",
			{
				{"enabled", "true: the sea acts on the light (the default). false: it does not."}
			},
			[inActiveScene] (bool enabled) {
				return inActiveScene([&] (const std::shared_ptr< Scene > & activeScene) {
					if ( activeScene->seaLevel() == nullptr )
					{
						return Console::CommandResult::error("The active scene has no sea !");
					}

					activeScene->enableSeaLight(enabled);

					return Console::CommandResult::success(enabled ? "The sea acts on the light under it." : "The sea no longer acts on the light under it.");
				});
			}, Console::CommandHint::Idempotent);

		this->bindCommand("getGroundLevel", "Returns the ground height of the active scene at a world X/Z as JSON: 'position' from the bilinear height field (getLevelAt()), 'surface' the exact height of the rendered triangle there (what the physics collides; null outside the ground), and the ground normal.",
			{
				{"x", "The world X coordinate."},
				{"z", "The world Z coordinate."}
			},
			[inActiveScene] (float positionX, float positionZ) {
				return inActiveScene([&] (const std::shared_ptr< Scene > & activeScene) {
					const auto groundLevel = activeScene->groundLevel();

					if ( groundLevel == nullptr )
					{
						return Console::CommandResult::error("The active scene has no ground !");
					}

					if ( !std::isfinite(positionX) || !std::isfinite(positionZ) )
					{
						return Console::CommandResult::error("The coordinates must be finite !");
					}

					const Math::Vector< 3, float > position{positionX, 0.0F, positionZ};
					const auto level = groundLevel->getLevelAt(position);

					/* The exact surface: the rendered triangle over the point. */
					class SurfaceProbe final : public GroundTriangleVisitor
					{
						public:

							explicit SurfaceProbe (const Math::Vector< 3, float > & point) noexcept : m_x{point.x()}, m_z{point.z()} { }

							void
							onTriangle (const Math::Space3D::Triangle< float > & triangle, uint32_t /*featureId*/) noexcept override
							{
								if ( !m_found )
								{
									m_found = surfaceHeightOver(triangle, m_x, m_z, m_height);
								}
							}

							[[nodiscard]]
							bool found () const noexcept { return m_found; }

							[[nodiscard]]
							float height () const noexcept { return m_height; }

						private:

							float m_x;
							float m_z;
							float m_height{0.0F};
							bool m_found{false};
					};

					SurfaceProbe probe{position};
					constexpr float ProbeReach{0.01F};
					static_cast< void >(groundLevel->visitTriangles(Math::Space3D::AACuboid< float >{Math::Vector< 3, float >{positionX + ProbeReach, 0.0F, positionZ + ProbeReach}, Math::Vector< 3, float >{positionX - ProbeReach, 0.0F, positionZ - ProbeReach}}, probe));

					std::stringstream info;
					info << R"({"position":)";
					writeJSONVector(info, Math::Vector< 3, float >{positionX, level, positionZ});
					info << R"(,"surface":)";

					if ( probe.found() && std::isfinite(probe.height()) )
					{
						info << std::setprecision(9) << probe.height();
					}
					else
					{
						info << "null";
					}

					info << R"(,"normal":)";
					writeJSONVector(info, groundLevel->getNormalAt(position));
					info << '}';

					return Console::CommandResult::json(info.str());
				});
			});

		this->bindCommand("loadGLTF", "Loads a glTF file into the active scene, synchronously (AI/bench harness, small assets only).",
			{
				{"filePath", "Path of the .gltf/.glb file."},
				{"ignored", "IGNORED — the former swapZ (the deleted per-asset axis flip); kept so existing scripts that pass it still work."},
				{"createLights", "Whether the file's KHR_lights_punctual lights are created in the scene.", false}
			},
			[this, inSameActiveScene] (const std::string & filePath, const std::optional< Console::Argument > & /*ignored*/, bool createLights) {
				/* The scene this command prepares for: re-checked under the exclusive access, where the scene is written. */
				const auto preparedFor = this->activeSceneSnapshot();

				if ( preparedFor == nullptr )
				{
					return Console::CommandResult::error("No active scene !");
				}

				/* NOTE: The console argument is UTF-8: IO::u8path(), never the narrow path constructor (the ANSI code
				 * page on Windows, where a non-ASCII path "did not exist"). */
				const auto filepath = Base::IO::u8path(filePath);

				if ( !EmEn::Base::IO::exists(filepath) )
				{
					return Console::CommandResult::error("File '" + filePath + "' does not exist !");
				}

				/* NOTE: the second argument used to be swapZ, the per-asset chirality workaround. That
				 * layer is gone: the mirror is now uniform across every asset, and it is corrected once,
				 * in the projection. createLights keeps its position so existing scripts still work. */
				Loaders::GLTFLoader loader{m_resourceManager};

				Loaders::LoaderOptions options;
				options.environmentReflectionIntensity = 1.0F;
				loader.setOptions(options);

				Loaders::SceneData sceneData;

				/* ⚠️ Synchronous load on purpose: this is an AI/bench harness command for SMALL
				 * assets. A multi-second load stalls the main loop and risks the compositor
				 * killing the surface (see the blocking-load caution) — do not feed it scenes. */
				if ( !loader.load(filepath, sceneData) )
				{
					return Console::CommandResult::error("Failed to load glTF file '" + filePath + "' !");
				}

				/* The file is loaded above with no access held; only the construction of its entities holds the exclusive
				 * access (the frames and the logic wait for it: small assets, as said). */
				return inSameActiveScene(preparedFor, [&] (const std::shared_ptr< Scene > & activeScene) {
					SceneDataConsumer consumer;
					consumer.setCreateLights(createLights);

					if ( !consumer.build(sceneData, *activeScene) )
					{
						return Console::CommandResult::error("Failed to build the scene from '" + filePath + "' !");
					}

					return Console::CommandResult::success("glTF file '" + filePath + "' loaded into scene '" + activeScene->name() + "'.");
				});
			});

		this->bindCommand("addMesh", "Places a mesh resource in the active scene as a lit static entity.",
			{
				{"meshResource", "The mesh resource name (listResources(MeshResource))."},
				{"entityName", "The name of the new static entity."},
				{"x", "World X, in metres."},
				{"y", "World Y, in metres (UP is +Y)."},
				{"z", "World Z, in metres."},
				{"scale", "Uniform scale factor (greater than 0).", 1.0F}
			},
			[this, inSameActiveScene] (const std::string & meshName, const std::string & entityName, float x, float y, float z, float scale) {
				if ( scale <= 0.0F )
				{
					return Console::CommandResult::error("addMesh(): scale must be greater than 0.");
				}

				/* The scene this command prepares for: re-checked under the exclusive access, where the scene is written. */
				const auto preparedFor = this->activeSceneSnapshot();

				if ( preparedFor == nullptr )
				{
					return Console::CommandResult::error("No active scene !");
				}

				auto * meshContainer = m_resourceManager.container< Graphics::Renderable::MultiLayerMeshResource >();

				if ( meshContainer == nullptr )
				{
					return Console::CommandResult::error("MeshResource container not available !");
				}

				/* ⚠️ Checked BEFORE the fetch (owner decision 2026-09-27 for createScene's skybox, applied here on
				 * 2026-11-17): getResource() silently answers the default resource for an unknown name, so a typo
				 * placed the wrong mesh and reported success. */
				if ( !meshContainer->isResourceExists(meshName) )
				{
					return Console::CommandResult::error("Unknown mesh '" + meshName + "' (listResources(MeshResource) lists them). Nothing was added.");
				}

				auto mesh = meshContainer->getResource(meshName);

				if ( mesh == nullptr )
				{
					return Console::CommandResult::error("Mesh '" + meshName + "' not found !");
				}

				return inSameActiveScene(preparedFor, [&] (const std::shared_ptr< Scene > & activeScene) {
					/* The requested scale is the AUTHOR's: it goes on the entity frame. It used to be written into the
					 * SHARED resource's uniform scale (every instance of the mesh, and its definition's own unit lost)
					 * and into the instance matrix; since the instance draws the mesh's unit (2026-09-28,
					 * RenderableInstance::Abstract::applyLocalTransformation()), that doubled it. */
					const Base::Math::Vector< 3, float > position{x, y, z};
					auto entity = activeScene->createStaticEntity(entityName, Base::Math::CartesianFrame< float >{position, scale});

					if ( entity == nullptr )
					{
						return Console::CommandResult::error("Failed to create entity '" + entityName + "' !");
					}

					const auto visual = entity->componentBuilder< Component::Visual >(entityName + "Visual").build(mesh, Graphics::RenderableInstance::Lighting::Lit);

					if ( visual == nullptr )
					{
						return Console::CommandResult::error("Failed to create visual component !");
					}

					std::stringstream message;
					message << "Mesh '" << meshName << "' placed at (" << x << ", " << y << ", " << z << ") as '" << entityName << "' (scale: " << scale << ").";

					return Console::CommandResult::success(message.str());
				});
			});

		this->bindCommand("setBackground", "Sets the active scene background skybox, optionally deriving the scene lighting from its sky manifest.",
			{
				{"skyboxName", "The skybox resource name (listResources(SkyBoxResource))."},
				{"applyLighting", "Whether the sky manifest drives the scene lighting (ambient illuminance, stars).", false}
			},
			[this, inSameActiveScene] (const std::string & name, bool applyLighting) {
				/* The scene this command prepares for: re-checked under the exclusive access, where the scene is written. */
				const auto preparedFor = this->activeSceneSnapshot();

				if ( preparedFor == nullptr )
				{
					return Console::CommandResult::error("No active scene !");
				}

				auto * skyBoxContainer = m_resourceManager.container< Graphics::Renderable::SkyBoxResource >();

				if ( skyBoxContainer == nullptr )
				{
					return Console::CommandResult::error("SkyBox resource container not available !");
				}

				/* ⚠️ Checked BEFORE the fetch, as createScene does (owner decision 2026-09-27): an unknown name was served
				 * the default skybox and answered a success. */
				if ( !skyBoxContainer->isResourceExists(name) )
				{
					return Console::CommandResult::error("Unknown skybox '" + name + "' (listResources(SkyBoxResource) lists them). The background was not changed.");
				}

				auto skyBox = skyBoxContainer->getResource(name);

				if ( skyBox == nullptr )
				{
					return Console::CommandResult::error("SkyBox resource '" + name + "' not found !");
				}

				return inSameActiveScene(preparedFor, [&] (const std::shared_ptr< Scene > & activeScene) {
					activeScene->setBackground(skyBox);

					/* Optional: derive the scene lighting from the sky manifest. */
					if ( applyLighting )
					{
						if ( activeScene->applyBackgroundLighting() )
						{
							std::stringstream message;
							message << "Background set to '" << name << "' with sky-driven lighting (ambient: " <<
								(skyBox->isLoaded() ? std::to_string(skyBox->ambientIlluminance()) : std::string{"deferred"}) << " lx, stars: " <<
								(skyBox->isLoaded() ? std::to_string(skyBox->stars().size()) : std::string{"deferred"}) << ").";

							return Console::CommandResult::success(message.str());
						}

						return Console::CommandResult::warning("Background set, but the lighting derivation failed.");
					}

					return Console::CommandResult::success("Background set to '" + name + "'.");
				});
			});

		/* The PHYSICAL SKY (Renderable::DynamicSkyResource) of the active scene: its look, at runtime. */
		/* NOTE: These commands keep a snapshot of the scene, on purpose: what they read and change guards itself — the
		 * sky material's settings under its own lock (SkyAtmosphereResource::m_propertiesAccess), the atmosphere's ground
		 * albedo under another (SkyAtmosphere::m_groundAlbedoAccess, applied by the rendering thread). The scene's
		 * background pointer is written by this thread only (setBackground(), under the exclusive access). */
		const auto dynamicSkyMaterial = [this] () -> std::shared_ptr< Graphics::Material::SkyAtmosphereResource > {
			const auto activeScene = this->activeSceneSnapshot();

			if ( activeScene == nullptr )
			{
				return nullptr;
			}

			const auto background = activeScene->background();

			if ( background == nullptr || !background->is(Graphics::Renderable::DynamicSkyResource::getClassUID()) )
			{
				return nullptr;
			}

			return std::static_pointer_cast< Graphics::Renderable::DynamicSkyResource >(background)->skyMaterial();
		};

		this->bindCommand("getDynamicSky", "Returns the physical sky of the active scene as JSON: its planet's name and radii, its ground albedo and its look (tint, intensity, saturation, star disc, limb darkening).", [dynamicSkyMaterial] () {
			const auto material = dynamicSkyMaterial();

			if ( material == nullptr )
			{
				return Console::CommandResult::warning("The active scene has no dynamic sky !");
			}

			Json::Value state{Json::objectValue};
			state["material"] = material->name();

			if ( const auto atmosphere = material->atmosphere(); atmosphere != nullptr )
			{
				state["planet"] = atmosphere->parameters().planet;
				state["groundRadiusKm"] = static_cast< double >(atmosphere->parameters().groundRadius);
				state["topRadiusKm"] = static_cast< double >(atmosphere->parameters().topRadius);

				const auto albedo = atmosphere->groundAlbedo();
				state["groundAlbedo"] = Json::Value{Json::arrayValue};
				state["groundAlbedo"].append(static_cast< double >(albedo.x()));
				state["groundAlbedo"].append(static_cast< double >(albedo.y()));
				state["groundAlbedo"].append(static_cast< double >(albedo.z()));
			}

			const auto tint = material->tint();
			state["tint"] = Json::Value{Json::arrayValue};
			state["tint"].append(static_cast< double >(tint.red()));
			state["tint"].append(static_cast< double >(tint.green()));
			state["tint"].append(static_cast< double >(tint.blue()));
			state["intensity"] = static_cast< double >(material->intensity());
			state["saturation"] = static_cast< double >(material->saturation());
			state["starDisc"] = material->isStarDiscEnabled();
			state["limbDarkening"] = static_cast< double >(material->limbDarkening());

			return Console::CommandResult::json(FastJSON::stringify(state));
		}, Console::CommandHint::ReadOnly);

		this->bindCommand("setDynamicSkyGrading", "Sets the tint and the intensity of the physical sky (artistic; white and 1 leave it physical). A known intensity is also the MEASUREMENT scale of the direct path, where the scene colour is not pre-exposed.",
			{
				{"red", "Linear tint, red."},
				{"green", "Linear tint, green."},
				{"blue", "Linear tint, blue."},
				{"intensity", "The scale of the sky's radiance."}
			},
			[dynamicSkyMaterial] (float red, float green, float blue, float intensity) {
				const auto material = dynamicSkyMaterial();

				if ( material == nullptr )
				{
					return Console::CommandResult::error("The active scene has no dynamic sky !");
				}

				material->setGrading(Base::PixelFactory::Color< float >{red, green, blue, 1.0F}, intensity);

				return Console::CommandResult::success("Dynamic sky grading set.");
			}, Console::CommandHint::Idempotent);

		this->bindCommand("setDynamicSkyGroundAlbedo", "Sets the ground albedo of the physical sky: the planet's surface shown under the horizon, lit by the star through the air (it also feeds the sky's multiple scattering). Linear reflectances in [0, 1]; the next frame applies it.",
			{
				{"red", "Linear reflectance, red, 0-1."},
				{"green", "Linear reflectance, green, 0-1."},
				{"blue", "Linear reflectance, blue, 0-1."}
			},
			[dynamicSkyMaterial] (float red, float green, float blue) {
				const auto material = dynamicSkyMaterial();

				if ( material == nullptr )
				{
					return Console::CommandResult::error("The active scene has no dynamic sky !");
				}

				const auto atmosphere = material->atmosphere();

				if ( atmosphere == nullptr )
				{
					return Console::CommandResult::error("The dynamic sky has no atmosphere !");
				}

				if ( !atmosphere->setGroundAlbedo(Base::Math::Vector< 3, float >{red, green, blue}) )
				{
					return Console::CommandResult::error("The ground albedo takes a finite linear reflectance in [0, 1] per channel !");
				}

				return Console::CommandResult::success("Dynamic sky ground albedo set.");
			}, Console::CommandHint::Idempotent);

		this->bindCommand("setDynamicSkySaturation", "Sets the saturation of the physical sky (artistic): 0 grey, 1 physical.",
			{
				{"saturation", "The saturation factor."}
			},
			[dynamicSkyMaterial] (float saturation) {
				const auto material = dynamicSkyMaterial();

				if ( material == nullptr )
				{
					return Console::CommandResult::error("The active scene has no dynamic sky !");
				}

				material->setSaturation(saturation);

				return Console::CommandResult::success("Dynamic sky saturation set.");
			}, Console::CommandHint::Idempotent);

		this->bindCommand("setDynamicSkyStar", "Shows or hides the star's disc of the physical sky and sets its limb darkening.",
			{
				{"enabled", "Whether the disc is drawn."},
				{"limbDarkening", "The linear limb-darkening coefficient, 0-1 (0 a flat disc, the Sun about 0.6)."}
			},
			[dynamicSkyMaterial] (bool enabled, float limbDarkening) {
				const auto material = dynamicSkyMaterial();

				if ( material == nullptr )
				{
					return Console::CommandResult::error("The active scene has no dynamic sky !");
				}

				material->enableStarDisc(enabled);
				material->setLimbDarkening(limbDarkening);

				return Console::CommandResult::success("Dynamic sky star set.");
			}, Console::CommandHint::Idempotent);

		this->bindCommand("listScenes", "Lists every registered scene name.", [this] () {
			std::stringstream list;

			list << "Scenes : " "\n";

			for ( const auto & sceneName : this->getSceneNames() )
			{
				list << " - '" << sceneName << "'" "\n";
			}

			return Console::CommandResult::info(list.str());
		}, Console::CommandHint::ReadOnly);

		this->bindCommand("getActiveSceneName", "Returns the name of the currently active scene.", [this] () {
			const auto activeScene = this->activeSceneSnapshot();

			if ( activeScene == nullptr )
			{
				return Console::CommandResult::warning("No active scene !");
			}

			return Console::CommandResult::info("The active scene is '" + activeScene->name() + "'");
		}, Console::CommandHint::ReadOnly);

		this->bindCommand("getWorldSoundCount", "Returns how many world sounds (Scene::playSoundAt(): sounds the scene owns, outliving the entity that caused them) still play in the active scene.", [inActiveScene] () {
			return inActiveScene([&] (const std::shared_ptr< Scene > & activeScene) {
				return Console::CommandResult::info("World sounds playing: " + std::to_string(activeScene->activeWorldSoundCount()) + " / " + std::to_string(Scene::MaxWorldSounds) + ".");
			}, Console::CommandResult::warning("No active scene !"));
		}, Console::CommandHint::ReadOnly);

		/* NOTE: The compass is a Core-level debug display, bound to F9 on the keyboard. The console
		 * keeps its OWN entry point: it is the deterministic way to drive it (an injected keyPress()
		 * does reach Core since 2026-09-13, but a command states what it does and answers). Every
		 * debug display an AI session must drive has to be reachable from here, not only from a key
		 * binding. */
		this->bindCommand("toggleCompass", "Toggles the world orientation compass (same as the F9 key). Bright spheres: R=X+, G=Y+, B=Z+; complementary: Cyan=X-, Magenta=Y-, Yellow=Z-.", [this, inActiveScene] () {
			return inActiveScene([&] (const std::shared_ptr< Scene > & activeScene) {
				if ( activeScene->toggleCompassDisplay(m_resourceManager) )
				{
					return Console::CommandResult::success("World compass displayed.");
				}

				return Console::CommandResult::success("World compass hidden.");
			});
		});

		this->bindCommand("targetActiveScene", "Targets the active scene for subsequent node/entity commands (listNodes, targetNode, ...).", [this] () {
			const auto activeScene = this->activeSceneSnapshot();

			if ( activeScene == nullptr )
			{
				return Console::CommandResult::error("No active scene !");
			}

			m_consoleMemory.target(activeScene);

			return Console::CommandResult::success("Now targeting scene '" + activeScene->name() + "'.");
		}, Console::CommandHint::Idempotent);

		this->bindCommand("targetScene", "Targets the named scene for subsequent node/entity commands.",
			{
				{"sceneName", "The scene name (listScenes() lists them)."}
			},
			[this] (const std::string & name) {
				const auto scene = this->getScene(name);

				if ( scene == nullptr )
				{
					return Console::CommandResult::error("The scene '" + name + "' doesn't exists !");
				}

				m_consoleMemory.target(scene);

				return Console::CommandResult::success("Now targeting scene '" + scene->name() + "'.");
			}, Console::CommandHint::Idempotent);

		this->bindCommand("listNodes", "Lists root-level nodes of the currently targeted scene. Requires targetScene() or targetActiveScene() first.", [inTargetedScene] () {
			return inTargetedScene([] (const std::shared_ptr< Scene > & scene) {
				std::stringstream list;
				list << "Nodes : " "\n";

				for ( const auto & key: scene->root()->children() | std::views::keys )
				{
					list << " - '" <<  key << "'" "\n";
				}

				return Console::CommandResult::info(list.str());
			});
		}, Console::CommandHint::ReadOnly);

		this->bindCommand("targetNode", "Targets a root-level node of the currently targeted scene (for moveNodeTo()).",
			{
				{"nodeName", "The node name (listNodes() lists them)."}
			},
			[this, inTargetedScene] (const std::string & name) {
				return inTargetedScene([this, &name] (const std::shared_ptr< Scene > & scene) {
					const auto sceneNode = scene->root()->findChild(name);

					if ( sceneNode == nullptr )
					{
						return Console::CommandResult::error("The node '" + name + "' doesn't exists !");
					}

					m_consoleMemory.target(sceneNode);

					return Console::CommandResult::success("Now targeting node '" + sceneNode->name() + "' from scene '" + scene->name() + "'.");
				});
			}, Console::CommandHint::Idempotent);

		this->bindCommand("listStaticEntities", "Lists static entities of the currently targeted scene.", [inTargetedScene] () {
			return inTargetedScene([] (const std::shared_ptr< Scene > & scene) {
				std::stringstream list;
				list << "Static entities : " "\n";

				scene->forEachStaticEntities([&list] (const auto & entity) {
					list << " - '" <<  entity.name() << "'" "\n";
				});

				return Console::CommandResult::info(list.str());
			});
		}, Console::CommandHint::ReadOnly);

		this->bindCommand("targetStaticEntity", "Targets a static entity of the currently targeted scene.",
			{
				{"name", "The static entity name (listStaticEntities() lists them)."}
			},
			[this, inTargetedScene] (const std::string & name) {
				return inTargetedScene([this, &name] (const std::shared_ptr< Scene > & scene) {
					const auto staticEntity = scene->findStaticEntity(name);

					if ( staticEntity == nullptr )
					{
						return Console::CommandResult::error("The static entity '" + name + "' doesn't exists !");
					}

					m_consoleMemory.target(staticEntity);

					return Console::CommandResult::success("Now targeting static entity '" + staticEntity->name() + "' from scene '" + scene->name() + "'.");
				});
			}, Console::CommandHint::Idempotent);

		this->bindCommand("getRenderStatistics", "Returns what the last frame's render lists submit, per geometry LOD: batches, instances, triangles (view, then shadows).", [inActiveScene] () {
			return inActiveScene([&] (const std::shared_ptr< Scene > & activeScene) {
				const auto print = [] (std::stringstream & output, const char * title, const Scene::RenderListStatistics & statistics) {
					uint64_t batches = 0;
					uint64_t instances = 0;
					uint64_t triangles = 0;

					output << title;

					if ( statistics.targets > 0 )
					{
						output << " (" << statistics.targets << " pass(es), one per cascade on a cascaded map)";
					}

					output << "\n";

					for ( size_t level = 0; level < statistics.batches.size(); ++level )
					{
						/* NOTE: The three arrays have the same size (Scene::RenderListStatistics). */
						const auto offset = static_cast< std::ptrdiff_t >(level);
						const auto levelBatches = *std::next(statistics.batches.cbegin(), offset);
						const auto levelInstances = *std::next(statistics.instances.cbegin(), offset);
						const auto levelTriangles = *std::next(statistics.triangles.cbegin(), offset);

						/* The view ladder always prints; a level past it only when something was drawn with it. */
						if ( level >= Graphics::Renderable::MaxLODLevels && levelBatches == 0 )
						{
							continue;
						}

						output << "  LOD " << level << ": " << levelBatches << " batches, " << levelInstances << " instances, " << levelTriangles << " triangles\n";

						batches += levelBatches;
						instances += levelInstances;
						triangles += levelTriangles;
					}

					output << "  total: " << batches << " batches, " << instances << " instances, " << triangles << " triangles\n";
				};

				std::stringstream info;

				print(info, "View lists (last frame)", activeScene->viewRenderStatistics());
				print(info, "Shadow lists (last frame)", activeScene->shadowRenderStatistics());

				return Console::CommandResult::info(info.str());
			});
		}, Console::CommandHint::ReadOnly);

		this->bindCommand("getStateSyncStatistics", "Returns how often a rendered frame read a logic state the logic thread was rewriting (must be 0).",
			{
				{"reset", "Whether the measurement window is reset after reading.", false}
			},
			[inActiveScene] (bool reset) {
				return inActiveScene([&] (const std::shared_ptr< Scene > & activeScene) {
					const auto statistics = activeScene->stateSyncStatistics(reset);

					std::stringstream info;
					info << "Logic-to-render state synchronisation" << (reset ? " (window reset)" : "") << "\n";
					info << "  Frames measured: " << statistics.frames << "\n";

					if ( statistics.frames > 0 )
					{
						const auto frames = static_cast< double >(statistics.frames);

						info << "  Frames whose latched state was overwritten: " << statistics.overwrittenFrames << " (" << (100.0 * static_cast< double >(statistics.overwrittenFrames) / frames) << " %)\n";
						info << "  Logic publications inside a frame: mean " << (static_cast< double >(statistics.publicationsDuringFrames) / frames) << ", max " << statistics.maxPublicationsDuringFrame << "\n";
						info << "  Latch-to-end: mean " << (statistics.latchToEndMSSum / frames) << " ms, max " << statistics.latchToEndMSMax << " ms\n";
					}

					return Console::CommandResult::info(info.str());
				});
			});

		/* NOTE: A snapshot of the scene is enough here, on purpose: everything this command reads has its own lock (the
		 * scene's bake target, the target's list of baked atlases, its pending jobs), and the files are written with no
		 * access held — the frames do not wait for the disk. */
		this->bindCommand("writeImposterAtlases", "Writes the albedo (premultiplied, mip 0) of every imposter atlas the active scene baked to the captures directory.", [this] () {
			const auto activeScene = this->activeSceneSnapshot();

			if ( activeScene == nullptr )
			{
				return Console::CommandResult::error("No active scene !");
			}

			const auto bakeTarget = activeScene->existingImposterBakeTarget();

			if ( bakeTarget == nullptr )
			{
				return Console::CommandResult::warning("The active scene baked no imposter.");
			}

			auto & renderer = activeScene->AVConsoleManager().graphicsRenderer();
			const auto captureDirectory = renderer.primaryServices().fileSystem().userDataDirectory("captures");

			std::stringstream report;
			size_t written = 0;

			for ( const auto & atlas : bakeTarget->bakedAtlases() )
			{
				auto fileName = atlas->name();

				std::ranges::replace(fileName, '/', '-');

				/* NOTE: A resource name is UTF-8: IO::u8path() in, IO::toU8String() out (this report is a console reply). */
				const auto filepath = captureDirectory / Base::IO::u8path(fileName + "-albedo.png");

				if ( atlas->writeAlbedo(renderer.transferManager(), filepath) )
				{
					report << Base::IO::toU8String(filepath) << "\n";

					++written;
				}
			}

			report << written << " imposter atlas(es) written, " << bakeTarget->pendingJobs() << " bake(s) still queued.";

			return Console::CommandResult::info(report.str());
		});

		this->bindCommand("getNode", "Returns a root-level node of the active scene as JSON (name, address, local position, child count).",
			{
				{"name", "The node name."}
			},
			[inActiveScene] (const std::string & name) {
				return inActiveScene([&] (const std::shared_ptr< Scene > & activeScene) {
					auto node = activeScene->root()->findChild(name);

					if ( node == nullptr )
					{
						return Console::CommandResult::error("Node '" + name + "' not found !");
					}

					const auto & coords = node->localCoordinates();
					const auto & pos = coords.position();

					std::stringstream info;
					info << "{";
					info << "\"name\":" << Json::valueToQuotedString(node->name().c_str(), node->name().size()) << ",";
					info << R"("address":")" << node.get() << "\",";
					info << "\"position\":";
					writeJSONVector(info, pos);
					info << ",";
					info << "\"childCount\":" << node->children().size();
					info << "}";

					return Console::CommandResult::json(info.str());
				});
			}, Console::CommandHint::ReadOnly);

		this->bindCommand("addTexturedShape", "Spawns a generated shape textured with a data-store image — the tool for inspecting UV mapping. Lit: in a scene with no light it renders BLACK.",
			{
				{"shape", "One of: sphere, cuboid, plane, cylinder, cone, torus, disk, capsule, hemisphere."},
				{"imageName", "A Texture2D resource name from the data stores."},
				{"entityName", "The name of the new static entity."},
				{"x", "World X, in metres."},
				{"y", "World Y, in metres (UP is +Y)."},
				{"z", "World Z, in metres."},
				{"size", "The shape's characteristic size (radius or edge), in metres (greater than 0).", 1.0F}
			},
			[this, inSameActiveScene] (const std::string & shapeName, const std::string & imageName, const std::string & entityName, float x, float y, float z, float size) {
				/* NOTE: The command's own guard on its input (a NaN passes "size <= 0"). The console's argument parser
				 * refuses a number that is not finite before any command runs ("Argument 'x' expects a float, got 'nan'."):
				 * an operator never gets this reply. */
				if ( !std::isfinite(x) || !std::isfinite(y) || !std::isfinite(z) || !std::isfinite(size) )
				{
					return Console::CommandResult::error("addTexturedShape(): the position and the size must be finite numbers.");
				}

				if ( size <= 0.0F )
				{
					return Console::CommandResult::error("addTexturedShape(): size must be greater than 0.");
				}

				/* The scene this command prepares for: re-checked under the exclusive access, where the scene is written. */
				const auto preparedFor = this->activeSceneSnapshot();

				if ( preparedFor == nullptr )
				{
					return Console::CommandResult::error("No active scene !");
				}

				/* ⚠️ Resource names carry the shape, the size AND the image: two calls that differ in any
				 * of them must not silently serve each other's cached resource. */
				const auto suffix = shapeName + "-" + std::to_string(size) + "-" + imageName;

				/* ⚠️ SYNCHRONOUS on purpose. The unsuffixed getOrCreateResource() runs its factory on the
				 * thread pool, which for a console command means the caller gets a success reply before
				 * anything exists — and the factory would outlive the locals it captured. These are small
				 * procedural assets, so blocking here is the correct trade. */
				const auto material = m_resourceManager.container< Graphics::Material::StandardResource >()
					->getOrCreateResourceSync("ConsoleUV-" + imageName, [this, &imageName] (Graphics::Material::StandardResource & newMaterial) {
						const auto texture = Graphics::TextureResource::getStoreTexture(*m_resourceManager.container< Graphics::TextureResource::Texture2D >(), imageName, {.sRGB = true});

						if ( texture == nullptr || !newMaterial.setAlbedoComponent(texture) )
						{
							return newMaterial.setManualLoadSuccess(false);
						}

						return newMaterial.setManualLoadSuccess(true);
					});

				if ( material == nullptr )
				{
					return Console::CommandResult::error("Failed to build a material from image '" + imageName + "' !");
				}

				/* Texture coordinates are the whole point of this command, so they are requested
				 * explicitly rather than left to whatever the default happens to be. */
				const Graphics::Geometry::ResourceGenerator generator{m_resourceManager, Graphics::Geometry::EnableTangentSpace | Graphics::Geometry::EnablePrimaryTextureCoordinates};

				std::shared_ptr< Graphics::Geometry::IndexedVertexResource > geometry;

				if ( shapeName == "sphere" ) { geometry = generator.sphere(size, 32, 16, "ConsoleUVGeo-" + suffix); }
				else if ( shapeName == "cuboid" ) { geometry = generator.cuboid(size, size, size, "ConsoleUVGeo-" + suffix); }
				else if ( shapeName == "plane" ) { geometry = generator.plane(size, size, 1, 1, "ConsoleUVGeo-" + suffix); }
				else if ( shapeName == "cylinder" ) { geometry = generator.cylinder(size, size, size * 2.0F, 32, 4, {}, "ConsoleUVGeo-" + suffix); }
				else if ( shapeName == "cone" ) { geometry = generator.cone(size, size * 2.0F, 32, 4, {}, "ConsoleUVGeo-" + suffix); }
				else if ( shapeName == "torus" ) { geometry = generator.torus(size, size * 0.3F, 32, 32, "ConsoleUVGeo-" + suffix); }
				else if ( shapeName == "disk" ) { geometry = generator.disk(size, size * 0.4F, 32, 1, Base::VertexFactory::CapUVMapping::Planar, "ConsoleUVGeo-" + suffix); }
				else if ( shapeName == "capsule" ) { geometry = generator.capsule(size * 0.5F, size * 2.0F, 32, 16, "ConsoleUVGeo-" + suffix); }
				else if ( shapeName == "hemisphere" ) { geometry = generator.hemisphere(size, 32, 16, "ConsoleUVGeo-" + suffix); }
				else
				{
					return Console::CommandResult::error("Unknown shape '" + shapeName + "' ! Known: sphere, cuboid, plane, cylinder, cone, torus, disk, capsule, hemisphere");
				}

				if ( geometry == nullptr )
				{
					return Console::CommandResult::error("Failed to generate the '" + shapeName + "' geometry !");
				}

				const auto mesh = m_resourceManager.container< Graphics::Renderable::MultiLayerMeshResource >()
					->getOrCreateResourceSync("ConsoleUVMesh-" + suffix, [&geometry, &material] (Graphics::Renderable::MultiLayerMeshResource & meshResource) {
						return meshResource.load(geometry, material);
					});

				if ( mesh == nullptr )
				{
					return Console::CommandResult::error("Failed to build the mesh !");
				}

				/* The material, the geometry and the mesh exist: only the entity and its visual are created under the access. */
				return inSameActiveScene(preparedFor, [&] (const std::shared_ptr< Scene > & activeScene) {
					const Base::Math::Vector< 3, float > position{x, y, z};
					auto entity = activeScene->createStaticEntity(entityName, position);

					if ( entity == nullptr )
					{
						return Console::CommandResult::error("Failed to create entity '" + entityName + "' !");
					}

					/* Lit, like addMesh: a UV check must be readable in the scene it lives in. ⚠️ In a scene
					 * with no light the shape renders BLACK — that is a missing light, not a broken UV. */
					if ( entity->componentBuilder< Component::Visual >(entityName + "Visual").build(mesh, Graphics::RenderableInstance::Lighting::Lit) == nullptr )
					{
						return Console::CommandResult::error("Failed to attach the visual !");
					}

					return Console::CommandResult::success("'" + shapeName + "' textured with '" + imageName + "' added as '" + entityName + "'.");
				});
			});

		this->bindCommand("getNodePhysics", "Returns the physics state of a root-level node of the active scene as JSON: world/local position, world orientation (upward and backward vectors), linear and angular velocity, the world centre of mass (the point the physics turns it about), movable, simulation paused (asleep), grounded and WHICH surface it rests on (Ground/Boundary/Entity), and the scene's physics cycle the state belongs to.",
			{
				{"name", "The node name."}
			},
			[inActiveScene] (const std::string & name) {
				return inActiveScene([&] (const std::shared_ptr< Scene > & activeScene) {
					const auto node = activeScene->root()->findChild(name);

					if ( node == nullptr )
					{
						return Console::CommandResult::error("Node '" + name + "' not found !");
					}

					return Console::CommandResult::json(nodePhysicsJSON(*node, activeScene->cycle()));
				});
			}, Console::CommandHint::ReadOnly);

		this->bindCommand("setCharacterVelocity", "Sets the velocity a root-level node's kinematic character controller wants to move at (world, m/s; its vertical part is ignored, except when the character flies).",
			{
				{"name", "The node name."},
				{"x", "The world X velocity (m/s)."},
				{"y", "The world Y velocity (ignored: the controller handles gravity and jumps)."},
				{"z", "The world Z velocity (m/s)."}
			},
			[inActiveScene] (const std::string & name, float velocityX, float velocityY, float velocityZ) {
				return inActiveScene([&] (const std::shared_ptr< Scene > & activeScene) {
					const auto node = activeScene->root()->findChild(name);

					if ( node == nullptr )
					{
						return Console::CommandResult::error("Node '" + name + "' not found !");
					}

					const auto character = node->characterController();

					if ( character == nullptr )
					{
						return Console::CommandResult::error("Node '" + name + "' has no character controller !");
					}

					if ( !character->controller().setWantedVelocity({velocityX, velocityY, velocityZ}) )
					{
						return Console::CommandResult::error("The velocity must be finite !");
					}

					return Console::CommandResult::success("Character '" + name + "' wants to move.");
				});
			});

		this->bindCommand("setVehicleInput", "Sets the driver's inputs of a root-level node's wheeled vehicle (its input script, if any, overrides them every cycle).",
			{
				{"name", "The node name."},
				{"forward", "The throttle, -1 to 1 (negative: reverse); clamped."},
				{"right", "The steering, -1 (left) to 1 (right); clamped."},
				{"brake", "The brake, 0 to 1; clamped."},
				{"handBrake", "The hand brake, 0 to 1; clamped."}
			},
			[inActiveScene] (const std::string & name, float forward, float right, float brake, float handBrake) {
				return inActiveScene([&] (const std::shared_ptr< Scene > & activeScene) {
					const auto node = activeScene->root()->findChild(name);

					if ( node == nullptr )
					{
						return Console::CommandResult::error("Node '" + name + "' not found !");
					}

					const auto vehicle = node->vehicle();

					if ( vehicle == nullptr )
					{
						return Console::CommandResult::error("Node '" + name + "' has no vehicle !");
					}

					if ( !vehicle->controller().setInput(forward, right, brake, handBrake) )
					{
						return Console::CommandResult::error("The inputs must be finite !");
					}

					return Console::CommandResult::success("Vehicle '" + name + "' driven.");
				});
			});

		this->bindCommand("characterJump", "Makes a root-level node's kinematic character controller jump (only when it stands on the ground).",
			{
				{"name", "The node name."},
				{"speed", "The launch speed (m/s), > 0."}
			},
			[inActiveScene] (const std::string & name, float speed) {
				return inActiveScene([&] (const std::shared_ptr< Scene > & activeScene) {
					const auto node = activeScene->root()->findChild(name);

					if ( node == nullptr )
					{
						return Console::CommandResult::error("Node '" + name + "' not found !");
					}

					const auto character = node->characterController();

					if ( character == nullptr )
					{
						return Console::CommandResult::error("Node '" + name + "' has no character controller !");
					}

					if ( !character->controller().jump(speed) )
					{
						return Console::CommandResult::error("The speed must be finite and > 0 !");
					}

					return Console::CommandResult::success("Character '" + name + "' jumps.");
				});
			});

		/* NOTE: The four recording commands keep a snapshot of the scene, on purpose: the recorder is its own protocol
		 * (PhysicsRecorder: one mutex between this thread and the logic thread's sample(), which resolves the nodes). */
		this->bindCommand("recordNodePhysics", "Records the physics state of root-level nodes of the active scene on EVERY logic cycle of a range, on the logic thread (after the collisions): the same cycles for every run, no sampling gap. Then getPhysicsRecordingStatus() until Complete, and savePhysicsRecording().",
			{
				{"nodeNames", "The root node names, comma-separated, as ONE quoted argument (\"A,B,C\"), 1 to 64."},
				{"firstCycle", "The first scene cycle to record (a cycle already past starts at once)."},
				{"cycleCount", "The number of cycles to record (1 to 36000, 10 min at 60 Hz)."}
			},
			[this] (const std::string & nodeNames, int32_t firstCycle, int32_t cycleCount) {
				const auto activeScene = this->activeSceneSnapshot();

				if ( activeScene == nullptr )
				{
					return Console::CommandResult::error("No active scene !");
				}

				if ( firstCycle < 0 || cycleCount <= 0 )
				{
					return Console::CommandResult::error("firstCycle must be >= 0 and cycleCount > 0 !");
				}

				std::vector< std::string > names;

				for ( auto name : String::explode(nodeNames, ',', false) )
				{
					name = String::trim(name);

					if ( !name.empty() )
					{
						names.emplace_back(std::move(name));
					}
				}

				std::string error;

				if ( !activeScene->physicsRecorder().start(std::move(names), static_cast< size_t >(firstCycle), static_cast< size_t >(cycleCount), error) )
				{
					return Console::CommandResult::error(error);
				}

				return Console::CommandResult::json(activeScene->physicsRecorder().status());
			});

		this->bindCommand("getPhysicsRecordingStatus", "Returns the state of the active scene's physics recording as JSON: state (Idle, Waiting, Recording, Complete), firstCycle, cycleCount, recordedCycles, nodes.",
			[this] () {
				const auto activeScene = this->activeSceneSnapshot();

				if ( activeScene == nullptr )
				{
					return Console::CommandResult::error("No active scene !");
				}

				return Console::CommandResult::json(activeScene->physicsRecorder().status());
			}, Console::CommandHint::ReadOnly);

		this->bindCommand("stopPhysicsRecording", "Stops the active scene's physics recording; the cycles recorded so far can still be saved.",
			[this] () {
				const auto activeScene = this->activeSceneSnapshot();

				if ( activeScene == nullptr )
				{
					return Console::CommandResult::error("No active scene !");
				}

				activeScene->physicsRecorder().stop();

				return Console::CommandResult::json(activeScene->physicsRecorder().status());
			});

		this->bindCommand("savePhysicsRecording", "Writes the completed physics recording of the active scene to a JSON file in the captures directory, releases it, and answers the file path.",
			[this] () {
				const auto activeScene = this->activeSceneSnapshot();

				if ( activeScene == nullptr )
				{
					return Console::CommandResult::error("No active scene !");
				}

				const auto captureDirectory = activeScene->AVConsoleManager().graphicsRenderer().primaryServices().fileSystem().userDataDirectory("captures");
				const auto seconds = std::chrono::duration_cast< std::chrono::seconds >(std::chrono::system_clock::now().time_since_epoch()).count();

				auto filepath = captureDirectory / Base::IO::u8path("physics-recording-" + std::to_string(seconds) + ".json");

				/* A unique name: two saves in one second must not overwrite each other. */
				std::error_code existsError;

				for ( int suffix = 1; std::filesystem::exists(filepath, existsError) && suffix < 1000; ++suffix )
				{
					filepath = captureDirectory / Base::IO::u8path("physics-recording-" + std::to_string(seconds) + "-" + std::to_string(suffix) + ".json");
				}

				std::string error;

				if ( !activeScene->physicsRecorder().write(filepath, error) )
				{
					return Console::CommandResult::error(error);
				}

				return Console::CommandResult::info(Base::IO::toU8String(filepath));
			});

		this->bindCommand("setNodePosition", "Moves a root-level node of the active scene to world coordinates.",
			{
				{"nodeName", "The node name."},
				{"x", "World X, in metres."},
				{"y", "World Y, in metres (UP is +Y)."},
				{"z", "World Z, in metres."}
			},
			[inActiveScene] (const std::string & name, float x, float y, float z) {
				return inActiveScene([&] (const std::shared_ptr< Scene > & activeScene) {
					auto node = activeScene->root()->findChild(name);

					if ( node == nullptr )
					{
						return Console::CommandResult::error("Node '" + name + "' not found !");
					}

					node->setPosition({x, y, z}, Math::TransformSpace::World);

					std::stringstream message;
					message << "Node '" << name << "' moved to (" << x << ", " << y << ", " << z << ").";

					return Console::CommandResult::success(message.str());
				});
			}, Console::CommandHint::Idempotent);

		this->bindCommand("enableTBN", "Enables or disables the TBN space debug rendering on the active scene's renderables.",
			{
				{"enabled", "1 (true) draws the tangent/bitangent/normal of every vertex, 0 (false) stops.", true}
			},
			[this] (bool state) {
				auto result = Console::CommandResult::error("No active scene !");

				/* EXCLUSIVE: this command writes configuration the rendering frame reads — the bit of every instance (its
				 * flag word is written before its component is linked, or under this access: RenderableInstance::Abstract)
				 * and the renderer's switch. They change together, between two frames and two logic cycles. */
				this->withExclusiveActiveScene([this, state, &result] (const std::shared_ptr< Scene > & activeScene) {
					auto & renderer = m_resourceManager.graphicsRenderer();

					if ( state && !renderer.isTBNSpaceRenderingAvailable() )
					{
						result = Console::CommandResult::error("The TBN space rendering is unavailable: the device has no geometry shader stage !");

						return;
					}

					renderer.enableTBNSpaceRendering(state);

					size_t toggled = 0;
					size_t skipped = 0;

					activeScene->forEachRenderableInstance([state, &toggled, &skipped] (const auto & renderableInstance) {
						if ( renderableInstance == nullptr )
						{
							return;
						}

						/* NOTE: A geometry without a vertex tangent space cannot show it: never enabled (a geometry still
						 * loading is decided by the instance itself, once). */
						if ( state && renderableInstance->geometryLacksTangentSpace() )
						{
							++skipped;

							return;
						}

						renderableInstance->enableDisplayTBNSpace(state);
						++toggled;
					});

					std::stringstream message;
					message << "TBN space rendering " << (state ? "enabled" : "disabled") << " on " << toggled << " renderable instance(s) of scene '" << activeScene->name() << "'";

					if ( skipped > 0 )
					{
						message << ", skipped " << skipped << " whose geometry has no tangent space";
					}

					message << '.';

					result = Console::CommandResult::success(message.str());
				}, true);

				return result;
			}, Console::CommandHint::Idempotent);

		this->bindCommand("setNodeLookAt", "Orients a root-level node of the active scene toward a world point.",
			{
				{"nodeName", "The node name."},
				{"x", "Target world X, in metres."},
				{"y", "Target world Y, in metres (UP is +Y)."},
				{"z", "Target world Z, in metres."}
			},
			[inActiveScene] (const std::string & name, float x, float y, float z) {
				return inActiveScene([&] (const std::shared_ptr< Scene > & activeScene) {
					auto node = activeScene->root()->findChild(name);

					if ( node == nullptr )
					{
						return Console::CommandResult::error("Node '" + name + "' not found !");
					}

					node->lookAt({x, y, z}, false);

					std::stringstream message;
					message << "Node '" << name << "' looking at (" << x << ", " << y << ", " << z << ").";

					return Console::CommandResult::success(message.str());
				});
			}, Console::CommandHint::Idempotent);

		this->bindCommand("dumpRenderTarget", "Dumps one layer of the image of the first render-to-texture target, else shadow map, of the active scene whose id contains a fragment, as a PNG in the captures directory. A floating-point (HDR) target gives two files: an exposed 8-bit sRGB PNG, and a Radiance .hdr holding the linear values. A shadow map (launch with --enable-depth-readback) gives a grey PNG of its depths and a .pfm holding them as floats.",
			{
				{"nameFragment", "A substring of the render target id."},
				{"layer", "The array layer to dump: a cubemap face (0 = +X, 1 = -X, 2 = +Y, 3 = -Y, 4 = +Z, 5 = -Z), a shadow cascade; 0 for a plain target.", 0},
				{"exposure", "For a floating-point target, the exposure of the PNG in stops (EV): the values are multiplied by 2^exposure before the sRGB transfer. Omit it for an automatic exposure (the 99th percentile of the brightest channel maps to 1)."}
			},
			[this] (const std::string & nameFragment, int32_t layer, std::optional< float > exposure) {
				if ( layer < 0 )
				{
					return Console::CommandResult::error("dumpRenderTarget(): the layer cannot be negative.");
				}

				/* A boundary: the value comes from the console or from an MCP client. */
				if ( exposure.has_value() && (!std::isfinite(*exposure) || std::abs(*exposure) > MaxDumpExposureEV) )
				{
					return Console::CommandResult::error("dumpRenderTarget(): the exposure is a finite number of stops between -64 and 64.");
				}

				const DumpRequest request{.exposure = exposure, .arrayLayer = static_cast< uint32_t >(layer)};

				Console::Outputs outputs;
				bool dumped = false;

				this->withSharedActiveScene([&] (const std::shared_ptr< Scene > & scene) {
					const auto dumpMatchingTarget = [&] (const std::shared_ptr< Graphics::RenderTarget::Abstract > & renderTarget) {
						if ( dumped || renderTarget->id().find(nameFragment) == std::string::npos )
						{
							return;
						}

						dumped = dumpRenderTargetLayer(*scene, *renderTarget, request, outputs);
					};

					/* The render-to-textures first (their colour), then the shadow maps (their depth). */
					scene->forEachRenderToTexture(dumpMatchingTarget);
					scene->forEachRenderToShadowMap(dumpMatchingTarget);
				}, true);

				if ( !dumped && outputs.empty() )
				{
					outputs.emplace_back(Severity::Error, std::stringstream{} << "No render-to-texture nor shadow map target matching '" << nameFragment << "' in the active scene.");
				}

				return Console::CommandResult::fromOutputs(std::move(outputs), dumped);
			});
	}
}
