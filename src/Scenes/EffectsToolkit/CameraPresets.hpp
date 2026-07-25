/*
 * src/Scenes/EffectsToolkit/CameraPresets.hpp
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
#include <memory>
#include <vector>

/* Forward declarations. */
namespace EmEn::Scenes::Component
{
	class Camera;
}

namespace EmEn::Graphics
{
	class DirectPostProcessEffect;
}

namespace EmEn::Scenes::EffectsToolkit
{
	/**
	 * @brief Camera preset tokens, usable at camera creation through the Toolkit
	 * (Toolkit::generatePerspectiveCamera(), default 'Normal') or at runtime through
	 * CameraPresets::Apply().
	 */
	enum class CameraPreset : uint8_t
	{
		/** @brief Bare camera: no lens effects, no depth of field, no HDR, automatic modes. */
		Normal,
		/** @brief Modern digital cinema camera: clean image, f/2.8 50mm, DoF + HDR. */
		HighQuality,
		/** @brief Human eye perception: subtle DoF, adaptive exposure, peripheral falloff. */
		HumanEye,
		/** @brief Vintage 1960s black & white film camera (Hitchcock lens stack). */
		VintageBlackAndWhite,
		/** @brief Super 8 amateur film camera: f/1.9, coarse grain, jitter, warm stock. */
		Super8,
		/** @brief 1980s studio broadcast camera on a clean CRT: deep focus, CRT artifacts. */
		Analog80s,
		/** @brief VHS camcorder played on a CRT: tape artifacts + CRT display, deep focus. */
		VHSAnalog80s,
		/** @brief Poor satellite feed on a CRT: ghosting + chroma bleeding, deep focus. */
		SatelliteAnalog80s,
		/** @brief VHS camcorder on a modern screen: tape artifacts, washed-out stock. */
		VHSPureSignal,
		/** @brief Poor satellite feed on a modern screen. */
		SatellitePureSignal,
		/** @brief Warm anamorphic cinema at golden hour: f/2.8 65mm, +0.3 EV. */
		GoldenHour,
		/** @brief Cool cinematic twilight: f/2.8 50mm, -0.4 EV. */
		BlueHour,
		/** @brief Retro 8-bits pixel-art display: raw palette, no photometry at all. */
		Retro8Bits,
		/** @brief User-provided style, registered through CameraPresets::setCustomStyle(). */
		Custom
	};

	/**
	 * @brief Full declarative description of a camera style — the EXTENSION CONTRACT.
	 * @note An engine consumer defines its own photographic style by filling this block:
	 * optics, exposure, DoF/HDR materialization and the lens-effect stack. The stack is a
	 * FACTORY, not instances: every application produces fresh effects, so two cameras
	 * carrying the same style never share effect state. Apply it directly with
	 * CameraPresets::Apply(camera, style), or register it once with setCustomStyle() to
	 * make it available through the CameraPreset::Custom token (Toolkit creation, cycles).
	 */
	struct CameraStyle
	{
		/* Optics. */
		float aperture{2.8F}; /**< Lens aperture, as an f-number. */
		float focalLength{50.0F}; /**< Focal length, in millimeters. */
		float focusDistance{10.0F}; /**< Manual focus plane distance, in meters (see manualFocus). */
		/* Exposure. */
		float exposureCompensation{0.0F}; /**< Exposure bias, in EV. */
		/* Lens effect stack FACTORY (may be empty for a clean image). */
		std::function< std::vector< std::shared_ptr< Graphics::DirectPostProcessEffect > > () > lensStackFactory;
		/* Modes and materialization. */
		bool manualFocus{false}; /**< Use focusDistance instead of the auto-focus. */
		bool autoExposure{true}; /**< Adaptive exposure (the EV bias applies on top). */
		bool depthOfField{false}; /**< Materializes the DepthOfField effect. */
		bool HDR{false}; /**< Materializes the ToneMapping (HDR) effect. */
	};
}

namespace EmEn::Scenes::EffectsToolkit::CameraPresets
{
	/**
	 * @brief Applies a camera preset from its token.
	 * @note Dispatches to the matching preset function below; 'Normal' resets the camera
	 * to the bare state (Neutral); 'Custom' applies the style registered through
	 * setCustomStyle() (falls back to Neutral with a warning when none is registered).
	 * @param camera A reference to the camera to configure.
	 * @param preset The preset token.
	 * @return void
	 */
	EMEN_API void Apply (Component::Camera & camera, CameraPreset preset) noexcept;

	/**
	 * @brief Applies a user-defined camera style (the extension contract).
	 * @note Replaces the camera's whole photographic setup with the declared one.
	 * @param camera A reference to the camera to configure.
	 * @param style A reference to the style declaration.
	 * @return void
	 */
	EMEN_API void Apply (Component::Camera & camera, const CameraStyle & style) noexcept;

	/**
	 * @brief Registers the user style behind the CameraPreset::Custom token.
	 * @note Engine-consumer entry point: register once (typically at scene setup, on the
	 * logic thread), then use CameraPreset::Custom anywhere a token is accepted
	 * (Toolkit::generatePerspectiveCamera(), runtime cycles...).
	 * @param style A reference to the style declaration (copied).
	 * @return void
	 */
	EMEN_API void setCustomStyle (const CameraStyle & style) noexcept;

	/**
	 * @brief Camera presets — full photographic configurations (physical camera model).
	 * @note Each preset configures a Camera like a real camera BODY + LENS package:
	 * the physical options (aperture, focal length, focus mode, exposure,
	 * DoF/HDR materialization) AND the matching single-pass lens effects.
	 * Applying a preset REPLACES the camera's current photographic setup.
	 * Since every camera keeps its own setup, two cameras in a scene can carry
	 * different presets: switching the active camera switches the whole look.
	 */

	/**
	 * @brief Resets the camera to a bare, neutral state.
	 * @note No lens effects, no depth of field, no HDR, neutral exposure, automatic modes.
	 * @param camera A reference to the camera to configure.
	 * @return void
	 */
	EMEN_API void Neutral (Component::Camera & camera) noexcept;

	/**
	 * @brief Modern high-quality digital cinema camera.
	 * @note Clean image (no lens artifacts), f/2.8 50mm optics, depth of field and
	 * HDR tone mapping enabled, auto-focus and auto-exposure.
	 * @param camera A reference to the camera to configure.
	 * @return void
	 */
	EMEN_API void HighQuality (Component::Camera & camera) noexcept;

	/**
	 * @brief Human eye perception.
	 * @note The eye barely shows bokeh (small effective aperture, short focal length):
	 * depth of field is enabled but subtle. HDR with auto-exposure (the iris adapts).
	 * A wide, very soft vignette approximates the peripheral vision falloff.
	 * @param camera A reference to the camera to configure.
	 * @return void
	 */
	EMEN_API void HumanEye (Component::Camera & camera) noexcept;

	/**
	 * @brief Vintage 1960s black & white film camera.
	 * @note Reuses the LensPresets::Hitchcock60s() lens stack (grain, high-contrast B&W,
	 * projector artifacts) over f/5.6 40mm optics with depth of field and HDR.
	 * @param camera A reference to the camera to configure.
	 * @return void
	 */
	EMEN_API void VintageBlackAndWhite (Component::Camera & camera) noexcept;

	/**
	 * @brief Super 8 amateur film camera (early 1970s).
	 * @note Fast f/1.9 lens (shallow focus on close subjects), warm faded stock, coarse
	 * grain, gate jitter, projector flicker, dust and frame vignette. HDR enabled with a
	 * slight overexposure bias (amateur metering).
	 * @param camera A reference to the camera to configure.
	 * @return void
	 */
	EMEN_API void Super8 (Component::Camera & camera) noexcept;

	/* NOTE: The presets below promote the LensPresets catalog to full camera presets
	 * (owner-decided merge): era-consistent optics + exposure over the lens stacks.
	 * Video/broadcast cameras have small sensors → deep focus (no DoF); the cinematic
	 * grades keep the photographic DoF; Retro8Bits disables the photometry entirely. */

	/** @brief 1980s studio broadcast camera on a clean CRT (f/4, deep focus, HDR). */
	EMEN_API void Analog80s (Component::Camera & camera) noexcept;

	/** @brief VHS camcorder on a CRT (f/1.8, deep focus, +0.2 EV video overexposure). */
	EMEN_API void VHSAnalog80s (Component::Camera & camera) noexcept;

	/** @brief Poor satellite feed on a CRT (f/4, deep focus, HDR). */
	EMEN_API void SatelliteAnalog80s (Component::Camera & camera) noexcept;

	/** @brief VHS camcorder on a modern screen (f/1.8, deep focus, +0.2 EV). */
	EMEN_API void VHSPureSignal (Component::Camera & camera) noexcept;

	/** @brief Poor satellite feed on a modern screen (f/4, deep focus, HDR). */
	EMEN_API void SatellitePureSignal (Component::Camera & camera) noexcept;

	/** @brief Warm anamorphic cinema at golden hour (f/2.8 65mm, DoF + HDR, +0.3 EV). */
	EMEN_API void GoldenHour (Component::Camera & camera) noexcept;

	/** @brief Cool cinematic twilight (f/2.8 50mm, DoF + HDR, -0.4 EV). */
	EMEN_API void BlueHour (Component::Camera & camera) noexcept;

	/** @brief Retro 8-bits pixel-art display (no DoF, no HDR — raw palette). */
	EMEN_API void Retro8Bits (Component::Camera & camera) noexcept;
}
