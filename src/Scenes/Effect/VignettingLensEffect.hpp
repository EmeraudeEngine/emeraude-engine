/*
 * src/Scenes/Effect/VignettingLensEffect.hpp
 * This file is part of Emeraude-Engine
 *
 * Copyright (C) 2010-2025 - Sébastien Léon Claude Christian Bémelmans "LondNoir" <londnoir@gmail.com>
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
 * https://github.com/londnoir/emeraude-engine
 *
 * --- THIS IS AUTOMATICALLY GENERATED, DO NOT CHANGE ---
 */

#pragma once

/* Local inclusions for inheritances. */
#include "Saphir/FramebufferEffectInterface.hpp"

/* Forward declarations */
namespace EmEn
{
	namespace Resources
	{
		class Manager;
	}

	namespace Graphics
	{
		class ImageResource;
	}

	namespace Vulkan
	{
		class Image;
	}
}

namespace EmEn::Scenes
{
	/**
	 * @brief The vignetting lens effect class;
	 * @extends EmEn::Saphir::FramebufferEffectInterface This is a framebuffer effect.
	 */
	class VignettingLensEffect final : public Saphir::FramebufferEffectInterface
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"VignettingLensEffect"};

			/**
			 * @brief Constructs a vignetting lens effect.
			 * @param resourceManager A reference to the resource manager.
			 * @param strength The strength of the effect. Default 1.
			 */
			explicit VignettingLensEffect (Resources::Manager & resourceManager, float strength = 1.0F) noexcept;

			/** @copydoc EmEn::Saphir::FramebufferEffectInterface::generateFragmentShaderCode() */
			[[nodiscard]]
			bool generateFragmentShaderCode (Saphir::Generator::Abstract & generator, Saphir::FragmentShader & fragmentShader) const noexcept override;

			/**
			 * @brief Sets the effect strength.
			 * @param value The amount
			 * @return void
			 */
			void setStrength (float value) noexcept;

			/**
			 * @brief Returns the effect strength.
			 * @return float
			 */
			[[nodiscard]]
			float strength () const noexcept;

		private:

			static constexpr auto VignettingTexture = "em_VignettingTexture";
			static constexpr auto Strength = "em_Strength";

			/** @copydoc EmEn::Saphir::FramebufferEffectInterface::requestScreenCoordinates() */
			[[nodiscard]]
			bool requestScreenCoordinates () const noexcept override;

			std::shared_ptr< Graphics::ImageResource > m_image;
			std::shared_ptr< Vulkan::Image > m_texture{};
			float m_strength{1.0F};
	};
}
