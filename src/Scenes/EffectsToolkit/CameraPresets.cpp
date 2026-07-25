/*
 * src/Scenes/EffectsToolkit/CameraPresets.cpp
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

#include "CameraPresets.hpp"

/* Local inclusions. */
#include "Graphics/Effects/Lens/ColorGrading.hpp"
#include "Graphics/Effects/Lens/DustAndHair.hpp"
#include "Graphics/Effects/Lens/FilmGrain.hpp"
#include "Graphics/Effects/Lens/Flicker.hpp"
#include "Graphics/Effects/Lens/FrameMasking.hpp"
#include "Graphics/Effects/Lens/VerticalJitter.hpp"
#include "Graphics/Effects/Lens/Vignetting.hpp"
#include "LensPresets.hpp"
#include "Scenes/Component/Camera.hpp"
#include "Tracer.hpp"

namespace EmEn::Scenes::EffectsToolkit::CameraPresets
{
	using namespace Graphics::Effects::Lens;

	/**
	 * @brief [Internal] Applies a full optics/exposure block, then replaces the lens stack.
	 * @param camera A reference to the camera.
	 * @param aperture The f-stop.
	 * @param focalLength The focal length in millimeters.
	 * @param exposureCompensation The exposure bias in EV.
	 * @param depthOfField Materializes the depth of field.
	 * @param HDR Materializes the HDR tone mapping.
	 * @param lensEffects The lens effect stack (replaces the current one).
	 * @return void
	 */
	static
	void
	configureCamera (Component::Camera & camera, float aperture, float focalLength, float exposureCompensation, bool depthOfField, bool HDR, const std::vector< std::shared_ptr< Graphics::DirectPostProcessEffect > > & lensEffects = {}) noexcept
	{
		camera.clearLensEffects();

		camera.setAperture(aperture);
		camera.setFocalLength(focalLength);
		camera.setAutoFocus(true);
		camera.setAutoExposure(true);
		camera.setExposureCompensation(exposureCompensation);
		camera.enableDepthOfField(depthOfField);
		camera.enableHDR(HDR);

		for ( const auto & effect : lensEffects )
		{
			camera.addLensEffect(effect);
		}
	}

	/**
	 * @brief [Internal] Storage slot for the user style behind CameraPreset::Custom.
	 * @note Function-local statics: no global construction order issue. Written once at
	 * setup (logic thread), read on style application — same-thread usage expected.
	 */
	static
	CameraStyle &
	customStyleSlot () noexcept
	{
		static CameraStyle style;

		return style;
	}

	static
	bool &
	customStyleDefined () noexcept
	{
		static bool defined{false};

		return defined;
	}

	void
	Apply (Component::Camera & camera, const CameraStyle & style) noexcept
	{
		camera.clearLensEffects();

		camera.setAperture(style.aperture);
		camera.setFocalLength(style.focalLength);

		if ( style.manualFocus )
		{
			/* NOTE: Setting a manual focus distance disables the auto-focus. */
			camera.setFocusDistance(style.focusDistance);
		}
		else
		{
			camera.setAutoFocus(true);
		}

		camera.setAutoExposure(style.autoExposure);
		camera.setExposureCompensation(style.exposureCompensation);
		camera.enableDepthOfField(style.depthOfField);
		camera.enableHDR(style.HDR);

		/* The stack is a factory: fresh effect instances at every application. */
		if ( style.lensStackFactory )
		{
			for ( const auto & effect : style.lensStackFactory() )
			{
				camera.addLensEffect(effect);
			}
		}
	}

	void
	setCustomStyle (const CameraStyle & style) noexcept
	{
		customStyleSlot() = style;
		customStyleDefined() = true;
	}

	void
	Apply (Component::Camera & camera, CameraPreset preset) noexcept
	{
		switch ( preset )
		{
			case CameraPreset::Custom :
				if ( customStyleDefined() )
				{
					Apply(camera, customStyleSlot());
				}
				else
				{
					Tracer::warning("CameraPresets", "No custom camera style registered (see setCustomStyle()) : falling back to Neutral.");

					Neutral(camera);
				}
				break;

			case CameraPreset::HighQuality :
				HighQuality(camera);
				break;

			case CameraPreset::HumanEye :
				HumanEye(camera);
				break;

			case CameraPreset::VintageBlackAndWhite :
				VintageBlackAndWhite(camera);
				break;

			case CameraPreset::Super8 :
				Super8(camera);
				break;

			case CameraPreset::Analog80s :
				Analog80s(camera);
				break;

			case CameraPreset::VHSAnalog80s :
				VHSAnalog80s(camera);
				break;

			case CameraPreset::SatelliteAnalog80s :
				SatelliteAnalog80s(camera);
				break;

			case CameraPreset::VHSPureSignal :
				VHSPureSignal(camera);
				break;

			case CameraPreset::SatellitePureSignal :
				SatellitePureSignal(camera);
				break;

			case CameraPreset::GoldenHour :
				GoldenHour(camera);
				break;

			case CameraPreset::BlueHour :
				BlueHour(camera);
				break;

			case CameraPreset::Retro8Bits :
				Retro8Bits(camera);
				break;

			case CameraPreset::Normal :
			default:
				Neutral(camera);
				break;
		}
	}

	void
	Neutral (Component::Camera & camera) noexcept
	{
		camera.clearLensEffects();

		camera.enableDepthOfField(false);
		camera.enableHDR(false);
		camera.setAutoFocus(true);
		camera.setAutoExposure(true);
		camera.setExposureCompensation(0.0F);
	}

	void
	HighQuality (Component::Camera & camera) noexcept
	{
		camera.clearLensEffects();

		/* Modern digital cinema package: fast prime lens, clean sensor. */
		camera.setAperture(2.8F);
		camera.setFocalLength(50.0F);
		camera.setAutoFocus(true);
		camera.setAutoExposure(true);
		camera.setExposureCompensation(0.0F);
		camera.enableDepthOfField(true);
		camera.enableHDR(true);
	}

	void
	HumanEye (Component::Camera & camera) noexcept
	{
		camera.clearLensEffects();

		/* The eye: short focal length and modest effective aperture — bokeh exists but
		 * stays subtle (visible on very close subjects only). The iris adapts (auto
		 * exposure); accommodation is instantaneous and silent (auto focus). */
		camera.setAperture(8.0F);
		camera.setFocalLength(17.0F);
		camera.setAutoFocus(true);
		camera.setAutoExposure(true);
		camera.setExposureCompensation(0.0F);
		camera.enableDepthOfField(true);
		camera.enableHDR(true);

		/* Peripheral vision falloff: a wide, very soft darkening. */
		auto vignetteEffect = std::make_shared< Vignetting >(0.3F);
		vignetteEffect->setRadius(0.55F);
		vignetteEffect->setSoftness(0.6F);

		camera.addLensEffect(vignetteEffect);
	}

	void
	VintageBlackAndWhite (Component::Camera & camera) noexcept
	{
		camera.clearLensEffects();

		/* 1960s film noir package over a classic prime lens. */
		camera.setAperture(5.6F);
		camera.setFocalLength(40.0F);
		camera.setAutoFocus(true);
		camera.setAutoExposure(true);
		camera.setExposureCompensation(0.0F);
		camera.enableDepthOfField(true);
		camera.enableHDR(true);

		/* Reuse the validated Hitchcock 60s lens stack (grain, high-contrast B&W,
		 * projector artifacts, vignette, gate mask). */
		for ( const auto & effect : LensPresets::Hitchcock60s() )
		{
			camera.addLensEffect(effect);
		}
	}

	void
	Analog80s (Component::Camera & camera) noexcept
	{
		/* Studio broadcast camera: deep focus video optics over the clean CRT stack. */
		configureCamera(camera, 4.0F, 25.0F, 0.0F, false, true, LensPresets::Analog80s());
	}

	void
	VHSAnalog80s (Component::Camera & camera) noexcept
	{
		/* VHS camcorder (small sensor, no bokeh, video overexposure) on a CRT. */
		configureCamera(camera, 1.8F, 8.0F, 0.2F, false, true, LensPresets::VHSToAnalog80s());
	}

	void
	SatelliteAnalog80s (Component::Camera & camera) noexcept
	{
		configureCamera(camera, 4.0F, 25.0F, 0.0F, false, true, LensPresets::SatelliteToAnalog80s());
	}

	void
	VHSPureSignal (Component::Camera & camera) noexcept
	{
		configureCamera(camera, 1.8F, 8.0F, 0.2F, false, true, LensPresets::VHSToPureSignal());
	}

	void
	SatellitePureSignal (Component::Camera & camera) noexcept
	{
		configureCamera(camera, 4.0F, 25.0F, 0.0F, false, true, LensPresets::SatelliteToPureSignal());
	}

	void
	GoldenHour (Component::Camera & camera) noexcept
	{
		/* Warm anamorphic cinema: photographic DoF, overexposed toward the sun. */
		configureCamera(camera, 2.8F, 65.0F, 0.3F, true, true, LensPresets::GoldenHour());
	}

	void
	BlueHour (Component::Camera & camera) noexcept
	{
		/* Cool cinematic twilight: photographic DoF, underexposed. */
		configureCamera(camera, 2.8F, 50.0F, -0.4F, true, true, LensPresets::BlueHour());
	}

	void
	Retro8Bits (Component::Camera & camera) noexcept
	{
		/* Pixel-art display: raw palette, NO photometry (no DoF, no HDR). */
		configureCamera(camera, 2.8F, 50.0F, 0.0F, false, false, LensPresets::Retro8Bits());
	}

	void
	Super8 (Component::Camera & camera) noexcept
	{
		camera.clearLensEffects();

		/* Super 8 amateur camera: fast f/1.9 lens (the standard on those bodies),
		 * short focal length, amateur metering slightly overexposing. */
		camera.setAperture(1.9F);
		camera.setFocalLength(25.0F);
		camera.setAutoFocus(true);
		camera.setAutoExposure(true);
		camera.setExposureCompensation(0.3F);
		camera.enableDepthOfField(true);
		camera.enableHDR(true);

		/* 1. Vertical jitter: hand-held gate instability, stronger than 35mm cinema.
		 * Overrides fragment fetching — must be first. */
		auto jitterEffect = std::make_shared< VerticalJitter >(0.004F);

		/* 2. Flicker: amateur projector lamp instability. */
		auto flickerEffect = std::make_shared< Flicker >(0.1F);

		/* 3. Warm faded reversal stock: lifted blacks, warm hue, muted contrast. */
		auto colorGrading = std::make_shared< ColorGrading >();
		colorGrading->setSaturation(1.1F);
		colorGrading->setHue(0.08F);
		colorGrading->setContrast(1.1F);
		colorGrading->setBrightness(0.05F);
		colorGrading->setGamma(1.05F);

		/* 4. Coarse grain: 8mm stock grain is much bigger than 35mm. */
		auto grainEffect = std::make_shared< FilmGrain >(0.28F);
		grainEffect->setSize(2.2F);

		/* 5. Dust and hair: home projection, no film cleaning. */
		auto dustEffect = std::make_shared< DustAndHair >(0.45F);

		/* 6. Vignetting: cheap lens corner falloff, pronounced. */
		auto vignetteEffect = std::make_shared< Vignetting >(0.8F);
		vignetteEffect->setRadius(0.3F);
		vignetteEffect->setSoftness(0.5F);

		/* 7. Frame masking: rounded 8mm gate. */
		auto frameMasking = std::make_shared< FrameMasking >(0.08F);
		frameMasking->setEdgeSoftness(0.03F);

		camera.addLensEffect(jitterEffect);
		camera.addLensEffect(flickerEffect);
		camera.addLensEffect(colorGrading);
		camera.addLensEffect(grainEffect);
		camera.addLensEffect(dustEffect);
		camera.addLensEffect(vignetteEffect);
		camera.addLensEffect(frameMasking);
	}
}
