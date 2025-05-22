/*
 * src/Scenes/Component/AbstractLightEmitter.hpp
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

/* STL inclusions. */
#include <cstdint>
#include <memory>
#include <string>

/* Local inclusions for inheritances. */
#include "AVConsole/AbstractVirtualDevice.hpp"
#include "Abstract.hpp"

/* Local inclusions for usages. */
#include "AVConsole/Manager.hpp"
#include "AVConsole/Types.hpp"
#include "Libs/PixelFactory/Color.hpp"
#include "Vulkan/SharedUniformBuffer.hpp"

/* Forward declarations. */
namespace EmEn
{
	namespace Graphics
	{
		namespace RenderTarget
		{
			class Abstract;
		}

		class Renderer;
	}

	namespace Saphir::Declaration
	{
		class UniformBlock;
	}

	namespace Vulkan
	{
		class DescriptorSet;
	}

	namespace Scenes
	{
		class LightSet;
	}
}

namespace EmEn::Scenes::Component
{
	/**
	 * @brief Base class of light emitters.
	 * @extends EmEn::Scenes::Component::Abstract The base class for each entity component.
	 * @extends EmEn::AVConsole::AbstractVirtualDevice This can act as a virtual video device.
	 */
	class AbstractLightEmitter : public Abstract, public AVConsole::AbstractVirtualDevice
	{
		public:

			static constexpr auto TracerTag{"LightEmitter"};

			/** @brief Animatable Interface key. */
			enum AnimationID : uint8_t
			{
				EmittingState,
				Color,
				Intensity,
				Radius,
				InnerAngle,
				OuterAngle
			};

			/* Default variables. */
			static constexpr auto DefaultColor{Libs::PixelFactory::White};
			static constexpr auto DefaultIntensity{1.0F};
			static constexpr auto DefaultRadius{0.0F};
			static constexpr auto DefaultInnerAngle{30.0F};
			static constexpr auto DefaultOuterAngle{45.0F};

			/**
			 * @brief Copy constructor.
			 * @param copy A reference to the copied instance.
			 */
			AbstractLightEmitter (const AbstractLightEmitter & copy) noexcept = delete;

			/**
			 * @brief Move constructor.
			 * @param copy A reference to the copied instance.
			 */
			AbstractLightEmitter (AbstractLightEmitter && copy) noexcept = delete;

			/**
			 * @brief Copy assignment.
			 * @param copy A reference to the copied instance.
			 * @return AbstractLightEmitter &
			 */
			AbstractLightEmitter & operator= (const AbstractLightEmitter & copy) noexcept = delete;

			/**
			 * @brief Move assignment.
			 * @param copy A reference to the copied instance.
			 * @return AbstractLightEmitter &
			 */
			AbstractLightEmitter & operator= (AbstractLightEmitter && copy) noexcept = delete;

			/**
			 * @brief Destructs the abstract light emitter.
			 */
			~AbstractLightEmitter () override = default;

			/** @copydoc EmEn::AVConsole::AbstractVirtualDevice::videoType() */
			[[nodiscard]]
			AVConsole::VideoType
			videoType () const noexcept override
			{
				return AVConsole::VideoType::Light;
			}

			/**
			 * @brief Sets the state of the light.
			 * @param state The state.
			 * @return void
			 */
			void
			enable (bool state) noexcept
			{
				this->setFlag(Enabled, state);

				if ( state )
				{
					this->enableFlag(VideoMemoryUpdateRequested);
				}
			}

			/**
			 * @brief Toggles the state of the light.
			 * @return bool
			 */
			bool
			toggle () noexcept
			{
				if ( this->isFlagEnabled(Enabled) )
				{
					this->disableFlag(Enabled);

					return false;
				}

				this->enableFlag(Enabled);
				this->enableFlag(VideoMemoryUpdateRequested);

				return true;
			}

			/**
			 * @brief Returns whether the light is emitting.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isEnabled () const noexcept
			{
				return this->isFlagEnabled(Enabled);
			}

			/**
			 * @brief Sets the light color.
			 * @param color A reference to a color.
			 * @return void
			 */
			void
			setColor (const Libs::PixelFactory::Color< float > & color) noexcept
			{
				m_color = color;

				this->onColorChange(m_color);

				this->requestVideoMemoryUpdate();
			}

			/**
			 * @brief Sets the intensity of the light.
			 * @param intensity An arbitrary value.
			 * @return void
			 */
			void
			setIntensity (float intensity) noexcept
			{
				m_intensity = intensity;

				this->onIntensityChange(m_intensity);

				this->requestVideoMemoryUpdate();
			}

			/**
			 * @brief Returns the light color.
			 * @return const Libraries::PixelFactory::Color< float > &
			 */
			[[nodiscard]]
			const Libs::PixelFactory::Color< float > &
			color () const noexcept
			{
				return m_color;
			}

			/**
			 * @brief Returns the light intensity.
			 * @return float
			 */
			[[nodiscard]]
			float
			intensity () const noexcept
			{
				return m_intensity;
			}

			/**
			 * @brief Returns the matrix to compute the light space.
			 * @return Libs::Math::Matrix< 4, float >
			 */
			[[nodiscard]]
			Libs::Math::Matrix< 4, float > getLightSpaceMatrix () const noexcept;

			/**
			 * @brief Updates the UBO with the light data.
			 * @return bool
			 */
			bool updateVideoMemory () noexcept;

			/**
			 * @brief Enables the shadow casting.
			 * @note The shadow map must have been requested at light creation.
			 * @param state The state.
			 * @return void
			 */
			void
			enableShadowCasting (bool state) noexcept
			{
				if ( this->shadowMapResolution() == 0 )
				{
					TraceInfo{TracerTag} << "The shadow map texture wasn't requested at light creation ! Cancelling ...";

					return;
				}

				this->setFlag(ShadowMapEnabled, state);
			}

			/**
			 * @brief Returns whether the shadow casting is enabled.
			 * @note The shadow map must have been requested at light creation.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isShadowCastingEnabled () const noexcept
			{
				if ( this->shadowMapResolution() == 0 )
				{
					TraceInfo{TracerTag} << "The shadow map texture wasn't requested at light creation ! Cancelling ...";

					return false;
				}

				return this->isFlagEnabled(ShadowMapEnabled);
			}

			/**
			 * @brief Returns the shadow map resolution.
			 * @note If 0 is returned, there is no shadow map for this light.
			 * @return size_t
			 */
			[[nodiscard]]
			uint32_t
			shadowMapResolution () const noexcept
			{
				return m_shadowMapResolution;
			}

			/**
			 * @brief Returns whether the light is created on the GPU.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isCreated () const noexcept
			{
				return m_sharedUniformBuffer != nullptr;
			}

			/**
			 * @brief Returns the light position in the UBO.
			 * @return uint32_t
			 */
			[[nodiscard]]
			uint32_t
			UBOIndex () const noexcept
			{
				return m_sharedUBOIndex;
			}

			/**
			 * @brief Returns the light alignment in the UBO.
			 * @return uint32_t
			 */
			[[nodiscard]]
			uint32_t
			UBOAlignment () const noexcept;

			/**
			 * @brief Returns the light offset in bytes in the UBO.
			 * @note This is the same as UBOIndex() * UBOAlignment().
			 * @return uint32_t
			 */
			[[nodiscard]]
			uint32_t UBOOffset () const noexcept;

			/**
			 * @brief Returns the light descriptor set.
			 * @return const Vulkan::DescriptorSet *
			 */
			[[nodiscard]]
			const Vulkan::DescriptorSet * descriptorSet () const noexcept;

			/**
			 * @brief Returns whether an absolute position is within the light radius.
			 * @return bool
			 */
			[[nodiscard]]
			virtual bool touch (const Libs::Math::Vector< 3, float > & position) const noexcept = 0;

			/**
			 * @brief Creates the light on the GPU with the shadow map if requested.
			 * @param lightSet A reference to the light set.
			 * @param renderer A reference to the graphic renderer.
			 * @param AVConsoleManager A reference to master control manager.
			 * @return bool
			 */
			[[nodiscard]]
			virtual bool createOnHardware (LightSet & lightSet, Graphics::Renderer & renderer, AVConsole::Manager & AVConsoleManager) noexcept = 0;

			/**
			 * @brief Removes the light from the GPU.
			 * @return void
			 */
			virtual void destroyFromHardware () noexcept = 0;

			/**
			 * @brief Gives access to the light shadow map.
			 * @return std::shared_ptr< Graphics::RenderTarget::Abstract >
			 */
			[[nodiscard]]
			virtual std::shared_ptr< Graphics::RenderTarget::Abstract > shadowMap () const noexcept = 0;

			/**
			 * @brief Returns the uniform block explaining how the light works.
			 * @param set The set index.
			 * @param binding The binding point in the set.
			 * @param useShadow States the use of a shadow.
			 * @return Saphir::Declaration::UniformBlock
			 */
			[[nodiscard]]
			virtual Saphir::Declaration::UniformBlock getUniformBlock (uint32_t set, uint32_t binding, bool useShadow) const noexcept = 0;

			/**
			 * @brief Returns an intensified color by a value.
			 * @param color A reference to a color.
			 * @param intensity The intensity value.
			 * @return Libs::Math::Vector< 4, float >
			 */
			static
			Libs::Math::Vector< 4, float >
			intensifiedColor (const Libs::PixelFactory::Color< float > & color, float intensity) noexcept
			{
				return {color.red() * intensity, color.green() * intensity, color.blue() * intensity, 1.0F};
			}

		protected:

			/**
			 * @brief Constructs an abstract light emitter.
			 * @param name The name of the light.
			 * @param parentEntity A reference to the parent entity.
			 * @param shadowMapResolution The shadow map resolution. 0 means no shadow casting.
			 */
			AbstractLightEmitter (const std::string & name, const AbstractEntity & parentEntity, uint32_t shadowMapResolution) noexcept;

			/** @copydoc EmEn::AVConsole::AbstractVirtualDevice::updateDeviceFromCoordinates() */
			void updateDeviceFromCoordinates (const Libs::Math::CartesianFrame< float > & worldCoordinates, const Libs::Math::Vector< 3, float > & worldVelocity) noexcept final;

			/** @copydoc EmEn::AVConsole::AbstractVirtualDevice::updateProperties() */
			void updateProperties (bool isPerspectiveProjection, float distance, float fovOrNear) noexcept final;

			/**
			 * @brief Adds the light to the shared uniform buffer.
			 * @param sharedBufferUniform A reference to the shared uniform buffer smart pointer.
			 * @return bool
			 */
			[[nodiscard]]
			bool addToSharedUniformBuffer (const std::shared_ptr< Vulkan::SharedUniformBuffer > & sharedBufferUniform) noexcept;

			/**
			 * @brief Removes the light from the shared uniform buffer.
			 * @return void
			 */
			void removeFromSharedUniformBuffer () noexcept;

			/**
			 * @brief Declares to update light on the GPU.
			 * @return void
			 */
			void
			requestVideoMemoryUpdate () noexcept
			{
				if ( this->isEnabled() )
				{
					this->enableFlag(VideoMemoryUpdateRequested);
				}
			}

			static constexpr auto ShadowMapName{"ShadowMapSampler"};

		private:

			/**
			 * @brief Event when the video memory is updating.
			 * @param UBO A reference to the shared uniform buffer object.
			 * @param index The light index in the shared uniform buffer object.
			 * @return bool
			 */
			[[nodiscard]]
			virtual bool onVideoMemoryUpdate (Vulkan::SharedUniformBuffer & UBO, uint32_t index) noexcept = 0;

			/**
			 * @brief Event when the color light changes.
			 * @param color A reference to a color.
			 * @return void
			 */
			virtual void onColorChange (const Libs::PixelFactory::Color< float > & color) noexcept = 0;

			/**
			 * @brief Event when the color intensity changes.
			 * @param intensity The amount.
			 * @return void
			 */
			virtual void onIntensityChange (float intensity) noexcept = 0;

			/* Flag names */
			static constexpr auto Enabled{UnusedFlag + 0UL};
			static constexpr auto VideoMemoryUpdateRequested{UnusedFlag + 1UL};
			static constexpr auto ShadowMapEnabled{UnusedFlag + 2UL};

			Libs::PixelFactory::Color< float > m_color{DefaultColor};
			float m_intensity{DefaultIntensity};
			uint32_t m_shadowMapResolution{0};
			std::shared_ptr< Vulkan::SharedUniformBuffer > m_sharedUniformBuffer;
			uint32_t m_sharedUBOIndex{0};
	};
}
