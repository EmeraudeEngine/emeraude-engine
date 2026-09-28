/*
 * src/Graphics/OceanWaves.hpp
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

/* STL inclusions. */
#include <array>
#include <complex>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace EmEn
{
	namespace Vulkan
	{
		class Device;
		class Image;
		class ImageView;
		class DescriptorSetLayout;
		class DescriptorPool;
		class DescriptorSet;
		class PipelineLayout;
		class ComputePipeline;
		class CommandBuffer;
	}

	namespace Saphir
	{
		class ShaderManager;
	}
}

namespace EmEn::Graphics
{
	/**
	 * @brief The sea state an OceanWaves spectrum is built from.
	 */
	struct OceanWaveParameters
	{
		/** @brief Tile side of each cascade, in metres, from the largest. Each cascade holds its own wave band. */
		std::array< float, 3 > cascadeSizes{512.0F, 64.0F, 8.0F};
		/** @brief Wind speed at 10 m above the sea, in m/s. */
		float windSpeed{10.0F};
		/** @brief Direction the waves travel toward, in radians, from +X toward +Z. */
		float windDirection{0.0F};
		/** @brief Distance over which the wind has blown, in metres (JONSWAP fetch). */
		float fetch{100000.0F};
		/** @brief Gravity, in m/s². */
		float gravity{9.81F};
		/** @brief Horizontal displacement factor (λ): 0 = round crests, 1 = Tessendorf's choppy crests. */
		float choppiness{1.0F};
		/** @brief Seed of the spectrum's Gaussian draws (deterministic). */
		uint32_t seed{1};
	};

	/**
	 * @brief FFT ocean waves: a directional JONSWAP spectrum evolved and inverse-transformed on the GPU every frame into
	 * displacement and slope textures, one layer per cascade.
	 * @note Engine item ocean-fft-surface, owner decisions 2026-09-28. References: J. Tessendorf, "Simulating Ocean Water",
	 * SIGGRAPH 2001 course notes (the evolution h(k, t) and the choppy displacement); C. J. Horvath, "Empirical directional
	 * wave spectra for computer graphics", DigiPro 2015 (JONSWAP with the Donelan-Banner spreading).
	 * @note Compute only — it runs where MoltenVK runs. Per frame: one evolution pass (the eight real fields packed two by
	 * two into four complex signals: an inverse FFT of A + iB is a + ib when a and b are real), a 256-point Stockham FFT per
	 * row then per column, each entirely in one workgroup's shared memory, and a resolve pass.
	 * @note Outputs (RGBA16F, 2D arrays, GENERAL layout after recordUpdate()): displacement = (λ Dx, h, λ Dz, λ ∂Dx/∂z),
	 * slopes = (∂h/∂x, ∂h/∂z, λ ∂Dx/∂x, λ ∂Dz/∂z) — enough for the choppy surface's normal and its Jacobian (foam).
	 */
	class OceanWaves final
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"OceanWaves"};

			/** @brief Texels per cascade side (the FFT size). */
			static constexpr uint32_t Resolution{256};

			/** @brief Number of cascades. */
			static constexpr uint32_t CascadeCount{3};

			/**
			 * @brief Constructs an ocean wave generator.
			 * @param device The device.
			 * @param shaderManager The shader manager (compute shader compilation).
			 */
			OceanWaves (std::shared_ptr< Vulkan::Device > device, Saphir::ShaderManager & shaderManager) noexcept;

			/** @brief Destructs the generator. */
			~OceanWaves ();

			OceanWaves (const OceanWaves & copy) noexcept = delete;
			OceanWaves (OceanWaves && copy) noexcept = delete;
			OceanWaves & operator= (const OceanWaves & copy) noexcept = delete;
			OceanWaves & operator= (OceanWaves && copy) noexcept = delete;

			/**
			 * @brief Builds the spectrum on the CPU, uploads it and creates the images and the compute pipelines.
			 * @param parameters The sea state.
			 * @return bool
			 */
			bool create (const OceanWaveParameters & parameters) noexcept;

			/**
			 * @brief Releases every GPU resource.
			 * @return void
			 */
			void destroy () noexcept;

			/**
			 * @brief Records the evolution, the FFTs and the resolve for a time into a command buffer.
			 * @note The outputs are left in GENERAL layout, their writes made visible to the vertex, fragment and compute
			 * stages and to transfers.
			 * @param commandBuffer A recording command buffer of a queue with compute.
			 * @param time The simulation time, in seconds.
			 * @return void
			 */
			void recordUpdate (const Vulkan::CommandBuffer & commandBuffer, float time) const noexcept;

			/**
			 * @brief Runs the GPU generator once and compares its outputs with a CPU reference of the same spectrum.
			 * @note The reference computes each of the eight fields separately with a double-precision Cooley-Tukey FFT —
			 * another algorithm than the GPU's, which also checks the two-by-two packing.
			 * @param time The simulation time, in seconds.
			 * @param report Receives a one-line summary (errors, peaks, RMS heights).
			 * @return bool True when every channel matches within the half-float precision of the outputs.
			 */
			bool selfTest (float time, std::string & report) noexcept;

			/**
			 * @brief Returns the displacement view (2D array, one layer per cascade).
			 * @return const Vulkan::ImageView *
			 */
			[[nodiscard]]
			const Vulkan::ImageView *
			displacementView () const noexcept
			{
				return m_displacementView.get();
			}

			/**
			 * @brief Returns the slope view (2D array, one layer per cascade).
			 * @return const Vulkan::ImageView *
			 */
			[[nodiscard]]
			const Vulkan::ImageView *
			slopeView () const noexcept
			{
				return m_slopeView.get();
			}

			/**
			 * @brief Returns the sea state.
			 * @return const OceanWaveParameters &
			 */
			[[nodiscard]]
			const OceanWaveParameters &
			parameters () const noexcept
			{
				return m_parameters;
			}

			/**
			 * @brief Builds the initial spectrum h₀(k) of one cascade, in the FFT's native order (index n → wave number
			 * n for n < N/2, n − N above).
			 * @param parameters The sea state.
			 * @param cascade The cascade index.
			 * @param spectrum Receives N² complex amplitudes, row-major (index = z · N + x).
			 * @return void
			 */
			static void buildInitialSpectrum (const OceanWaveParameters & parameters, uint32_t cascade, std::vector< std::complex< float > > & spectrum) noexcept;

		private:

			/**
			 * @brief Uploads the initial spectra and moves every image to GENERAL.
			 * @return bool
			 */
			bool uploadSpectrum () noexcept;

			/**
			 * @brief Reads the two output images back (layer after layer, RGBA as floats).
			 * @param displacement Receives N² × cascades × 4 floats.
			 * @param slopes Receives N² × cascades × 4 floats.
			 * @return bool
			 */
			bool readback (std::vector< float > & displacement, std::vector< float > & slopes) const noexcept;

			std::shared_ptr< Vulkan::Device > m_device;
			Saphir::ShaderManager * m_shaderManager;
			OceanWaveParameters m_parameters;
			std::shared_ptr< Vulkan::Image > m_spectrumImage;
			std::array< std::shared_ptr< Vulkan::Image >, 2 > m_fieldImages;
			std::shared_ptr< Vulkan::Image > m_displacementImage;
			std::shared_ptr< Vulkan::Image > m_slopeImage;
			std::shared_ptr< Vulkan::ImageView > m_spectrumView;
			std::array< std::shared_ptr< Vulkan::ImageView >, 2 > m_fieldViews;
			std::shared_ptr< Vulkan::ImageView > m_displacementView;
			std::shared_ptr< Vulkan::ImageView > m_slopeView;
			std::shared_ptr< Vulkan::DescriptorSetLayout > m_descriptorSetLayout;
			std::shared_ptr< Vulkan::DescriptorPool > m_descriptorPool;
			std::unique_ptr< Vulkan::DescriptorSet > m_descriptorSet;
			std::shared_ptr< Vulkan::PipelineLayout > m_pipelineLayout;
			std::unique_ptr< Vulkan::ComputePipeline > m_evolvePipeline;
			std::unique_ptr< Vulkan::ComputePipeline > m_fftPipeline;
			std::unique_ptr< Vulkan::ComputePipeline > m_resolvePipeline;
	};
}
