/*
 * src/Graphics/SkyAtmosphere.hpp
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
#include <array>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>

/* Third-party inclusions. */
#include "volk.h"

/* Local inclusions for usages. */
#include "Graphics/StorageTexture2D.hpp"
#include "Math/Vector.hpp"
#include "Mutex.hpp"

/* Forward declarations. */
namespace EmEn::Vulkan
{
	class CommandBuffer;
	class ComputePipeline;
	class DescriptorPool;
	class DescriptorSet;
	class DescriptorSetLayout;
	class PipelineLayout;
	class UniformBufferObject;
}

namespace EmEn::Graphics
{
	class Renderer;
}

namespace EmEn::Graphics
{
	/**
	 * @brief A planet's atmosphere: what the physical sky scatters and absorbs (Hillaire 2020).
	 * @note Data, not code (owner decision 2026-10-24: "physical, and extraterrestrial skies"): an alien sky is another
	 * set of these numbers. Distances in kilometres, coefficients per kilometre.
	 */
	struct EMEN_API AtmosphereParameters
	{
		/** @brief The planet's radius, in km. */
		float groundRadius{6360.0F};
		/** @brief The radius of the atmosphere's top, in km. */
		float topRadius{6460.0F};
		/** @brief Rayleigh (molecular) scattering at the ground, per km. Earth: 5.802, 13.558, 33.1 × 1e-3. */
		Base::Math::Vector< 3, float > rayleighScattering{5.802e-3F, 13.558e-3F, 33.1e-3F};
		/** @brief Rayleigh scale height, in km. Earth: 8. */
		float rayleighScaleHeight{8.0F};
		/** @brief Mie (aerosol) scattering at the ground, per km. Earth: 3.996e-3. */
		Base::Math::Vector< 3, float > mieScattering{3.996e-3F, 3.996e-3F, 3.996e-3F};
		/** @brief Mie absorption at the ground, per km (extinction − scattering). Earth: 0.444e-3. */
		Base::Math::Vector< 3, float > mieAbsorption{0.444e-3F, 0.444e-3F, 0.444e-3F};
		/** @brief Mie scale height, in km. Earth: 1.2. */
		float mieScaleHeight{1.2F};
		/** @brief Mie phase anisotropy g PER CHANNEL, in (-1, 1): 0.8 = strongly forward (the halo around the sun). One
		 * value on Earth; a dust whose forward peak narrows in the blue (diffraction, Mars) makes the blue aureole of its
		 * sunsets. */
		Base::Math::Vector< 3, float > miePhaseG{0.8F, 0.8F, 0.8F};
		/** @brief The absorbing layer's extinction at its peak, per km. Earth's ozone: 0.650, 1.881, 0.085 × 1e-3. */
		Base::Math::Vector< 3, float > absorptionExtinction{0.650e-3F, 1.881e-3F, 0.085e-3F};
		/** @brief Height where the absorbing layer's tent profile changes slope, in km. Earth: 25. */
		float absorptionLayerWidth{25.0F};
		/** @brief The absorbing layer's density below the width: constant + linear × height. Earth: −2/3, 1/15. */
		float absorptionBelowConstant{-2.0F / 3.0F};
		float absorptionBelowLinear{1.0F / 15.0F};
		/** @brief The absorbing layer's density above the width: constant + linear × height. Earth: 8/3, −1/15. */
		float absorptionAboveConstant{8.0F / 3.0F};
		float absorptionAboveLinear{-1.0F / 15.0F};
		/** @brief The ground's albedo, lighting the sky from below. */
		Base::Math::Vector< 3, float > groundAlbedo{0.3F, 0.3F, 0.3F};
		/** @brief The planet this atmosphere belongs to: a preset's name ("Earth", "Mars"), the identity a celestial
		 * clock checks against its own planet (`Base::Math::Astronomy::Planet::name`). Empty = unnamed. */
		std::string planet{"Earth"};

		/**
		 * @brief Returns Earth's atmosphere (Hillaire 2020's reference values, after Bruneton & Neyret 2008).
		 * @return AtmosphereParameters
		 */
		[[nodiscard]]
		static AtmosphereParameters earth () noexcept;

		/**
		 * @brief Returns a MARS-LIKE atmosphere: thin CO2 and a dust that absorbs blue — a butterscotch day and a blue
		 * sunset (the dust's forward scattering around the sun).
		 * @note An approximation chosen to look right, not a measured fit: the test of "an alien sky is data".
		 * @return AtmosphereParameters
		 */
		[[nodiscard]]
		static AtmosphereParameters mars () noexcept;

		/**
		 * @brief Returns a preset by name: "Earth" or "Mars" (case-sensitive).
		 * @param name The preset's name.
		 * @return std::optional< AtmosphereParameters >
		 */
		[[nodiscard]]
		static std::optional< AtmosphereParameters > preset (const std::string & name) noexcept;

		/**
		 * @brief Returns the transmittance from a point to the top of the atmosphere: the CPU twin of the transmittance
		 * table (SkyAtmosphere's pass 1).
		 * @note The table's own march, sample for sample (40 uniform samples, Hillaire's 0.3 offset, the same medium
		 * and the same clamped scale heights), in double precision: it differs from the table only by the table's
		 * interpolation and its half floats. A ray that meets the ground transmits nothing.
		 * @note Pure and cheap (40 samples): the logic thread calls it every cycle (owner decision 2026-10-24, "a CPU
		 * twin", against a readback of the table).
		 * @param height The point's height above the ground, in km. Clamped to [0, top − ground].
		 * @param zenithCos The cosine of the angle between the ray and the local vertical. Clamped to [-1, 1].
		 * @return Base::Math::Vector< 3, float > The transmittance per channel, in [0, 1]. Zero for a non-finite input.
		 */
		[[nodiscard]]
		Base::Math::Vector< 3, float > transmittanceToTop (float height, float zenithCos) const noexcept;

		/**
		 * @brief Returns the transmittance of a star's disc seen from a height: the transmittance averaged over the disc
		 * by area, the part hidden by the ground counting zero.
		 * @note The disc is cut into DiscStripCount horizontal strips weighted by their chords (the midpoint rule on the
		 * disc's area), each through transmittanceToTop() at its elevation. A disc high in the sky gives its centre's
		 * value; a disc on the horizon fades as it sets (the penumbra of a sunset), and gives exactly zero once the
		 * ground hides it. Limb darkening is not weighted in (a few per cent of the strip weights).
		 * @param height The observer's height above the ground, in km.
		 * @param elevation The elevation of the disc's centre above the horizontal, in radians.
		 * @param angularRadius The disc's angular radius, in radians. Clamped to [0, π/4].
		 * @return Base::Math::Vector< 3, float > The transmittance per channel, in [0, 1]. Zero for a non-finite input.
		 */
		[[nodiscard]]
		Base::Math::Vector< 3, float > discTransmittance (float height, float elevation, float angularRadius) const noexcept;

		/** @brief The number of samples of the transmittance march (the table's pass 1 takes as many). */
		static constexpr int TransmittanceSampleCount{40};

		/** @brief The number of horizontal strips discTransmittance() cuts a disc into. */
		static constexpr int DiscStripCount{16};
	};

	/**
	 * @brief What the sky needs from the frame: the star above the atmosphere and where the camera stands.
	 */
	struct SkyFrameInputs
	{
		/** @brief Unit world vector toward the star (Y up). */
		Base::Math::Vector< 3, float > sunDirection{0.0F, 0.5F, 0.866F};
		/** @brief The star's illuminance above the atmosphere × its unit-luminance colour, in lux. */
		Base::Math::Vector< 3, float > sunIlluminance{124000.0F, 124000.0F, 124000.0F};
		/** @brief Apparent angular diameter of the star, in degrees. */
		float sunAngularDiameter{0.53F};
		/** @brief The camera's height above the planet's ground, in metres (the engine's world Y). */
		float cameraAltitude{0.0F};
	};

	/**
	 * @brief The physical sky's lookup tables and the passes that build them (S. Hillaire, EGSR 2020): the
	 * TRANSMITTANCE table (256×64) and the MULTIPLE-SCATTERING table (32×32), built once per atmosphere, and the
	 * SKY-VIEW table (192×108), built every frame for the star and the camera altitude of that frame.
	 * @note Owned by a Graphics::Renderable::DynamicSkyResource and read by its material
	 * (Graphics::Material::SkyAtmosphereResource). The per-frame pass is recorded by the renderer before every pass that
	 * may draw the sky (the shadow maps' slot: the render-to-texture probes draw it too).
	 * @note The sky-view table holds radiance PER LUX of the star; a 4×1 RGBA32F "frame" texture written by the same pass
	 * carries the star's direction, illuminance and size and the camera altitude to the material — a material UBO lives in
	 * ONE frame region, so a value that changes every frame never goes there.
	 * @note ⚠️ ONE sky-view table, written in each frame's command stream with its barriers (the grab pass's model): the
	 * frames read it in submission order on the renderer's graphics queue.
	 */
	class EMEN_API SkyAtmosphere final
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"SkyAtmosphere"};

			/** @brief Transmittance table size (Hillaire: 256×64). */
			static constexpr uint32_t TransmittanceWidth{256};
			static constexpr uint32_t TransmittanceHeight{64};

			/** @brief Multiple-scattering table size (Hillaire: 32×32). */
			static constexpr uint32_t MultipleScatteringSize{32};

			/** @brief Sky-view table size (Hillaire: 192×108). */
			static constexpr uint32_t SkyViewWidth{192};
			static constexpr uint32_t SkyViewHeight{108};

			/** @brief The frame texture: 4 RGBA32F texels. */
			static constexpr uint32_t FrameTexels{4};

			/**
			 * @brief Constructs a sky atmosphere.
			 * @param name The name, for the debug labels.
			 * @param parameters A reference to the atmosphere.
			 */
			SkyAtmosphere (std::string name, const AtmosphereParameters & parameters) noexcept;

			/**
			 * @brief Copy constructor.
			 * @param copy A reference to the copied instance.
			 */
			SkyAtmosphere (const SkyAtmosphere & copy) noexcept = delete;

			/**
			 * @brief Move constructor.
			 * @param copy A reference to the copied instance.
			 */
			SkyAtmosphere (SkyAtmosphere && copy) noexcept = delete;

			/**
			 * @brief Copy assignment.
			 * @param copy A reference to the copied instance.
			 * @return SkyAtmosphere &
			 */
			SkyAtmosphere & operator= (const SkyAtmosphere & copy) noexcept = delete;

			/**
			 * @brief Move assignment.
			 * @param copy A reference to the copied instance.
			 * @return SkyAtmosphere &
			 */
			SkyAtmosphere & operator= (SkyAtmosphere && copy) noexcept = delete;

			/**
			 * @brief Destructs the sky atmosphere.
			 */
			~SkyAtmosphere ();

			/**
			 * @brief Creates the GPU objects and BAKES the transmittance and multiple-scattering tables (blocking).
			 * @note Idempotent and thread-safe: the material's creation and the renderer's first frame may both ask.
			 * @param renderer A reference to the graphics renderer.
			 * @return bool
			 */
			[[nodiscard]]
			bool createOnHardware (Renderer & renderer) noexcept;

			/**
			 * @brief Returns whether the GPU objects exist and the static tables are baked.
			 * @return bool
			 */
			[[nodiscard]]
			bool isCreated () const noexcept;

			/**
			 * @brief RENDER THREAD. Records the sky-view table and the frame texture for this frame.
			 * @param commandBuffer A reference to a command buffer, outside any render pass.
			 * @param inputs A reference to the frame's star and camera.
			 * @return bool
			 */
			bool recordFrame (const Vulkan::CommandBuffer & commandBuffer, const SkyFrameInputs & inputs) noexcept;

			/**
			 * @brief Returns the atmosphere.
			 * @return const AtmosphereParameters &
			 */
			[[nodiscard]]
			const AtmosphereParameters &
			parameters () const noexcept
			{
				return m_parameters;
			}

			/**
			 * @brief Returns the uniform buffer holding the atmosphere (the GLSL block of AtmosphereGLSL.hpp).
			 * @return const Vulkan::UniformBufferObject * Null before createOnHardware().
			 */
			[[nodiscard]]
			const Vulkan::UniformBufferObject *
			parametersBuffer () const noexcept
			{
				return m_parametersBuffer.get();
			}

			/**
			 * @brief Returns the transmittance table.
			 * @return const StorageTexture2D &
			 */
			[[nodiscard]]
			const StorageTexture2D &
			transmittance () const noexcept
			{
				return m_transmittance;
			}

			/**
			 * @brief Returns the sky-view table.
			 * @return const StorageTexture2D &
			 */
			[[nodiscard]]
			const StorageTexture2D &
			skyView () const noexcept
			{
				return m_skyView;
			}

			/**
			 * @brief Returns the frame texture.
			 * @return const StorageTexture2D &
			 */
			[[nodiscard]]
			const StorageTexture2D &
			frameData () const noexcept
			{
				return m_frameData;
			}

			/**
			 * @brief Returns the size, in bytes, of the atmosphere's uniform block (std140, eight vec4).
			 * @return uint32_t
			 */
			[[nodiscard]]
			static constexpr
			uint32_t
			parametersBlockSize () noexcept
			{
				return 8U * 4U * static_cast< uint32_t >(sizeof(float));
			}

		private:

			/**
			 * @brief Creates the descriptor sets, the pipelines and the uniform buffer.
			 * @param renderer A reference to the graphics renderer.
			 * @return bool
			 */
			[[nodiscard]]
			bool createPipelines (Renderer & renderer) noexcept;

			/**
			 * @brief Bakes the transmittance and multiple-scattering tables, and puts the per-frame textures in their
			 * sampled layout (blocking, one-time command buffer).
			 * @param renderer A reference to the graphics renderer.
			 * @return bool
			 */
			[[nodiscard]]
			bool bake (Renderer & renderer) noexcept;

			/**
			 * @brief Returns the format of the lookup tables on this device: RGBA32F where it can be stored and
			 * filtered linearly, RGBA16F otherwise (a warned precision loss).
			 * @note The tables hold radiance PER LUX of the star: half floats flush everything under 2^-24 to zero, i.e.
			 * the sky under ~0.008 nits — the whole night below a −12° sun (graphics doc 42, 2026-10-25).
			 * Linear filtering of RGBA32F is optional in Vulkan: it is queried, never assumed.
			 * @param renderer A reference to the graphics renderer.
			 * @return VkFormat
			 */
			[[nodiscard]]
			static VkFormat tableFormat (Renderer & renderer) noexcept;

			/**
			 * @brief Packs the atmosphere into its std140 block.
			 * @return std::array< float, 32 >
			 */
			[[nodiscard]]
			std::array< float, 32 > packParameters () const noexcept;

			std::string m_name;
			AtmosphereParameters m_parameters;
			StorageTexture2D m_transmittance;
			StorageTexture2D m_multipleScattering;
			StorageTexture2D m_skyView;
			StorageTexture2D m_frameData;
			std::unique_ptr< Vulkan::UniformBufferObject > m_parametersBuffer;
			std::shared_ptr< Vulkan::DescriptorSetLayout > m_descriptorSetLayout;
			std::shared_ptr< Vulkan::PipelineLayout > m_pipelineLayout;
			std::shared_ptr< Vulkan::DescriptorPool > m_descriptorPool;
			std::unique_ptr< Vulkan::DescriptorSet > m_transmittanceSet;
			std::unique_ptr< Vulkan::DescriptorSet > m_multipleScatteringSet;
			std::unique_ptr< Vulkan::DescriptorSet > m_skyViewSet;
			std::unique_ptr< Vulkan::ComputePipeline > m_transmittancePipeline;
			std::unique_ptr< Vulkan::ComputePipeline > m_multipleScatteringPipeline;
			std::unique_ptr< Vulkan::ComputePipeline > m_skyViewPipeline;
			VkFormat m_tableFormat{VK_FORMAT_R16G16B16A16_SFLOAT};
			mutable Base::Mutex m_creationAccess;
			bool m_created{false};
	};
}
