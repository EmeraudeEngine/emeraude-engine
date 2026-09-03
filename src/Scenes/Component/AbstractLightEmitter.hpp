/*
 * src/Scenes/Component/AbstractLightEmitter.hpp
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
#include <algorithm>
#include <cstdint>
#include <array>
#include <memory>
#include <string>

/* Local inclusions for inheritances. */
#include "Abstract.hpp"
#include "Scenes/AVConsole/AbstractVirtualDevice.hpp"
#include "ObserverTrait.hpp"

/* Local inclusions for usages. */
#include "Graphics/RenderTarget/ShadowMap.hpp"
#include "Math/Space3D/Sphere.hpp"
#include "PixelFactory/Color.hpp"
#include "Scenes/AVConsole/Types.hpp"

/* Forward declarations. */
namespace EmEn
{
	namespace Vulkan
	{
		class DescriptorSet;
		class TextureInterface;
	}

	namespace Graphics
	{
		class SharedUniformBuffer;

		namespace RenderTarget
		{
			class Abstract;
		}
	}

	namespace Scenes
	{
		class BindlessTextureSet;
	}

	namespace Saphir::Declaration
	{
		class UniformBlock;
	}
}

namespace EmEn::Scenes::Component
{
	/**
	 * @brief Base class of light emitters.
	 * @extends EmEn::Scenes::Component::Abstract The base class for each entity component.
	 * @extends EmEn::Scenes::AVConsole::AbstractVirtualDevice This can act as a virtual video device.
	 * @extends EmEn::Base::ObserverTrait Observes color projection textures for async loading completion.
	 */
	class EMEN_API AbstractLightEmitter : public Abstract, public AVConsole::AbstractVirtualDevice, public Base::ObserverTrait
	{
		public:

			static constexpr auto TracerTag{"LightEmitter"};

			/** @brief Sentinel value indicating no bindless color projection texture is registered. */
			static constexpr uint32_t NoColorProjectionTexture{UINT32_MAX};

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
			static constexpr auto DefaultColor{Base::PixelFactory::White};
			static constexpr auto DefaultIntensity{1.0F};
			/* ⚠️ A NULL RADIUS MEANS UNBOUNDED REACH, on the CPU as on the GPU: the shader gates
			 * its distance attenuation on `if ( lightRadius > 0.0 )`, and `touch()` answers true
			 * without testing. The two MUST keep saying the same thing — while `touch()` still
			 * built a sphere from it, a zero radius made an invalid sphere, `isColliding()`
			 * refused it outright, and the light was culled from every draw in the scene while
			 * the shader stood ready to light it. See `SpotLight::touch()`. */
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

			/** @copydoc EmEn::Scenes::AVConsole::AbstractVirtualDevice::videoType() */
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
			void enable (bool state) noexcept;

			/**
			 * @brief Toggles the state of the light.
			 * @return bool
			 */
			bool toggle () noexcept;

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
			void setColor (const Base::PixelFactory::Color< float > & color) noexcept;

			/**
			 * @brief Sets the PHOTOMETRIC intensity of the light, in the unit its type is measured in.
			 * @note This is the GPU-facing quantity: ILLUMINANCE in lux for a directional light
			 * (the distance is constant, so an illuminance is the natural description), LUMINOUS
			 * INTENSITY in candela for a point or spot light. Prefer the per-type helpers, which
			 * take the unit content is actually authored in — `DirectionalLight::setIlluminance()`
			 * (lux) and `PointLight`/`SpotLight::setLuminousPower()` (lumens, as a bulb is sold) —
			 * and convert through `Graphics::Photometry`.
			 * @warning ⚠️ A photometric value is only meaningful with a PHYSICAL (inverse-square)
			 * attenuation. The point/spot falloff is still the radius-bounded artistic
			 * `max(1 - (d/r)², 0)`, so these units are currently PROPORTIONAL, not absolute — see
			 * `TODO.md` § "Photometric lighting + absolute exposure", phase 1.
			 * @param intensity The photometric intensity (lux for directional, candela otherwise).
			 * @return void
			 */
			void setIntensity (float intensity) noexcept;

			/**
			 * @brief Returns the light color.
			 * @return const Libraries::PixelFactory::Color< float > &
			 */
			[[nodiscard]]
			const Base::PixelFactory::Color< float > &
			color () const noexcept
			{
				return m_color;
			}

			/**
			 * @brief Returns the light photometric intensity.
			 * @note Lux for a directional light, candela for a point or spot light. See
			 * setIntensity() for the unit contract and its current limit.
			 * @return float
			 */
			[[nodiscard]]
			float
			intensity () const noexcept
			{
				return m_intensity;
			}

			/**
			 * @brief Uploads the PUBLISHED uniform block to the shared UBO.
			 * @note ⚠️ Reads the render state slot, never the logic-side block. Before Aug 2026 this
			 * uploaded the live logic block straight from the render thread, so a light that moves
			 * (a carried torch, a lamp on a vehicle, an animated sun, or any CSM light — refit to the
			 * camera every tick by construction) had its SAMPLING matrix one tick ahead of the map it
			 * addresses. On screen: a straight-edged bite out of a spot's lit pool, moving frame to
			 * frame. See docs/shadow-mapping.md § Temporal coherence.
			 * @param readStateIndex The render state-valid index to read data.
			 * @return bool
			 */
			/**
			 * @brief Element count of the LARGEST light uniform block (the CSM one).
			 * @note Keep in sync with DirectionalLight::CSM_BufferSize — LightSet sizes the shared
			 * UBO on the same maximum.
			 */
			static constexpr auto MaxUniformBlockElementCount{84UL};

			/**
			 * @brief Upper bound on frame-in-flight regions a light UBO can be split into.
			 * @note The real count is the swap-chain image count; this only sizes the per-region
			 * bookkeeping.
			 */
			static constexpr auto MaxFrameRegionCount{8UL};

			bool updateVideoMemory (uint32_t readStateIndex, uint32_t frameIndex) noexcept;

			/** @copydoc EmEn::Scenes::Component::Abstract::publishStateForRendering(uint32_t) */
			void publishStateForRendering (uint32_t writeStateIndex) noexcept override;

			/**
			 * @brief Fills EVERY render state slot and uploads one, at creation time.
			 * @note ⚠️ Creation happens outside the publish/render cadence, so without this the
			 * shared UBO would hold uninitialised bytes until the first logic tick published and the
			 * first frame uploaded — a window the engine has already paid for once, as a CSM light
			 * whose only upload ever performed was the one from createOnHardware() with an all-zero
			 * buffer (black colour, zero intensity, null matrices).
			 * @return bool
			 */
			bool primeVideoMemory () noexcept;

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
			uint32_t UBOAlignment () const noexcept;

			/**
			 * @brief Returns the light offset in bytes in the UBO.
			 * @note This is the same as UBOIndex() * UBOAlignment().
			 * @return uint32_t
			 */
			[[nodiscard]]
			uint32_t UBOOffset () const noexcept;

			/**
			 * @brief Returns the light descriptor set.
			 * @param useShadowMap Whether to return the shadow-enabled descriptor set.
			 * @return const Vulkan::DescriptorSet *
			 */
			[[nodiscard]]
			virtual const Vulkan::DescriptorSet * descriptorSet (bool useShadowMap) const noexcept;

			/**
			 * @brief Returns whether this light has a shadow-enabled descriptor set.
			 * @return bool
			 */
			[[nodiscard]]
			virtual
			bool
			hasShadowDescriptorSet () const noexcept
			{
				return false;
			}

			/**
			 * @brief Returns whether an absolute position is within the light radius.
			 * @return bool
			 */
			[[nodiscard]]
			virtual bool touch (const Base::Math::Vector< 3, float > & position) const noexcept = 0;

			/**
			 * @brief Returns whether the light reaches a world bounding sphere, read from the PUBLISHED state.
			 * @note ⚠️ This is the RENDER-THREAD overload, used for the per-instance light culling.
			 * It reads the light position from the published uniform block of the slot the frame
			 * latched — never from the parent entity. The render thread iterates a SNAPSHOT of the
			 * light set, and a light can outlive its entity there: LightSet::remove() retires a
			 * light instead of destroying it, so the frames that snapshotted it stay valid, while
			 * the entity that carried it may already be gone. Reading the parent from here was a
			 * pure-virtual call on a destroyed node, measured on game-logic the moment a barrel
			 * exploded.
			 * @param target A reference to the target world bounding sphere.
			 * @param readStateIndex The render state slot latched by the frame.
			 * @return bool
			 */
			[[nodiscard]]
			virtual bool touch (const Base::Math::Space3D::Sphere< float > & target, uint32_t readStateIndex) const noexcept = 0;

			/**
			 * @brief Creates the light on the GPU with the shadow map if requested.
			 * @param scene A reference to the scene.
			 * @return bool
			 */
			[[nodiscard]]
			virtual bool createOnHardware (Scene & scene) noexcept = 0;

			/**
			 * @brief Removes the light from the GPU.
			 * @param scene A reference to the scene.
			 * @return void
			 */
			virtual void destroyFromHardware (Scene & scene) noexcept = 0;

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
			 * @param useShadow States the use of a shadow map.
			 * @param useColorProjection States the use of a color projection texture.
			 * @return Saphir::Declaration::UniformBlock
			 */
			[[nodiscard]]
			virtual Saphir::Declaration::UniformBlock getUniformBlock (uint32_t set, uint32_t binding, bool useShadow, bool useColorProjection) const noexcept = 0;

			/**
			 * @brief Sets the PCF (Percentage-Closer Filtering) radius for soft shadow edges.
			 * @param radius The filter radius in normalized texture coordinates.
			 * @return void
			 */
			virtual void setPCFRadius (float radius) noexcept = 0;

			/**
			 * @brief Returns the PCF radius.
			 * @return float
			 */
			[[nodiscard]]
			virtual float PCFRadius () const noexcept = 0;

			/**
			 * @brief Sets the shadow bias to prevent shadow acne.
			 * @param bias The shadow bias value.
			 * @return void
			 */
			virtual void setShadowBias (float bias) noexcept = 0;

			/**
			 * @brief Returns the shadow bias.
			 * @return float
			 */
			[[nodiscard]]
			virtual float shadowBias () const noexcept = 0;

			/**
			 * @brief Sets a color projection texture for this light.
			 * @param texture A shared pointer to the texture interface (2D for spot/directional, cubemap for point).
			 * @return void
			 */
			void setColorProjectionTexture (const std::shared_ptr< Vulkan::TextureInterface > & texture) noexcept;

			/**
			 * @brief Returns the color projection texture.
			 * @return std::shared_ptr< Vulkan::TextureInterface >
			 */
			[[nodiscard]]
			std::shared_ptr< Vulkan::TextureInterface >
			colorProjectionTexture () const noexcept
			{
				return m_colorProjectionTexture;
			}

			/**
			 * @brief Returns whether this light has a color projection texture assigned.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			hasColorProjectionTexture () const noexcept
			{
				return m_colorProjectionTexture != nullptr;
			}

			/**
			 * @brief Returns the bindless index for the color projection texture.
			 * @return uint32_t The bindless index, or NoColorProjectionTexture if none.
			 */
			[[nodiscard]]
			uint32_t
			colorProjectionBindlessIndex () const noexcept
			{
				return m_colorProjectionBindlessIndex;
			}

			/**
			 * @brief Returns the frame index for animated color projection textures.
			 * @return uint32_t The frame index, or NoColorProjectionTexture if static or none.
			 */
			[[nodiscard]]
			uint32_t
			colorProjectionFrameIndex () const noexcept
			{
				return m_colorProjectionFrameIndex;
			}

			/**
			 * @brief Sets the color projection boost factor.
			 * @note When boost > 0, the projection formula becomes (1.0 + projectionColor * boost),
			 *	   making bright areas of the texture amplify light intensity. When boost == 0 (default),
			 *	   the original multiplicative behavior is preserved.
			 * @param boost The boost factor.
			 * @return void
			 */
			void
			setColorProjectionBoost (float boost) noexcept
			{
				m_colorProjectionBoost = std::max(0.0F, boost);
			}

			/**
			 * @brief Returns the color projection boost factor.
			 * @return float
			 */
			[[nodiscard]]
			float
			colorProjectionBoost () const noexcept
			{
				return m_colorProjectionBoost;
			}

			/**
			 * @brief Returns whether this light type uses a cubemap for color projection.
			 * @note Override in PointLight to return true.
			 * @return bool
			 */
			[[nodiscard]]
			virtual
			bool
			usesCubemapColorProjection () const noexcept
			{
				return false;
			}

			/**
			 * @brief Returns an intensified color by a value.
			 * @param color A reference to a color.
			 * @param intensity The intensity value.
			 * @return Base::Math::Vector< 4, float >
			 */
			static
			Base::Math::Vector< 4, float >
			intensifiedColor (const Base::PixelFactory::Color< float > & color, float intensity) noexcept
			{
				return {color.red() * intensity, color.green() * intensity, color.blue() * intensity, 1.0F};
			}

		protected:

			/**
			 * @brief Returns the published uniform block of a render state slot.
			 * @note Render-thread read; the slot must be the one latched by the frame. This is the
			 * ONLY light state the render thread may read for a geometric decision — the logic-side
			 * members (`m_buffer`, the parent entity) belong to the logic thread.
			 * @param readStateIndex The render state slot latched by the frame.
			 * @return const std::array< float, MaxUniformBlockElementCount > &
			 */
			[[nodiscard]]
			const std::array< float, MaxUniformBlockElementCount > &
			publishedBlock (uint32_t readStateIndex) const noexcept
			{
				return m_publishedBlocks[readStateIndex];
			}

			/**
			 * @brief Constructs an abstract light emitter.
			 * @param componentName A reference to a string.
			 * @param parentEntity A reference to the parent entity.
			 * @param shadowMapResolution The shadow map resolution. 0 means no shadow casting.
			 */
			AbstractLightEmitter (const std::string & componentName, const AbstractEntity & parentEntity, uint32_t shadowMapResolution) noexcept
				: Abstract{componentName, parentEntity},
				AbstractVirtualDevice{componentName, AVConsole::DeviceType::Video, AVConsole::ConnexionType::Output},
				m_shadowMapResolution{shadowMapResolution}
			{
				this->enableFlag(Enabled);
			}

			/** @copydoc EmEn::Scenes::AVConsole::AbstractVirtualDevice::getWorldCoordinates() */
			[[nodiscard]]
			Base::Math::CartesianFrame< float >
			getWorldCoordinates () const noexcept override
			{
				/* FIXME: function name shadowing here !
				 * Maybe there is a simpler inheritance to do. */
				return Abstract::getWorldCoordinates();
			}

			/** @copydoc EmEn::Scenes::AVConsole::AbstractVirtualDevice::updateDeviceFromCoordinates() */
			void updateDeviceFromCoordinates (const Base::Math::CartesianFrame< float > & worldCoordinates, const Base::Math::Vector< 3, float > & worldVelocity) noexcept final;

			/**
			 * @brief Adds the light to the shared uniform buffer.
			 * @param sharedBufferUniform A reference to the shared uniform buffer smart pointer.
			 * @return bool
			 */
			[[nodiscard]]
			bool addToSharedUniformBuffer (const std::shared_ptr< Graphics::SharedUniformBuffer > & sharedBufferUniform) noexcept;

			/**
			 * @brief Removes the light from the shared uniform buffer.
			 * @return void
			 */
			void removeFromSharedUniformBuffer () noexcept;

			/**
			 * @brief Declares to update light on the GPU.
			 * @return void
			 */
			void requestVideoMemoryUpdate () noexcept;

			/**
			 * @brief Registers the color projection texture in the bindless texture manager.
			 * @return void
			 */
			void registerColorProjectionInBindless () noexcept;

			/**
			 * @brief Unregisters the color projection texture from the bindless texture manager and stops observing.
			 * @param useCubemap True for cubemap textures (PointLight), false for 2D textures (Directional/Spot).
			 * @return void
			 */
			void unregisterColorProjectionFromBindless (bool useCubemap) noexcept;

			static constexpr auto ShadowMapName{"ShadowMapSampler"};

			BindlessTextureSet * m_bindlessTextureSet{nullptr};
			uint32_t m_colorProjectionFrameIndex{NoColorProjectionTexture};
			bool m_colorProjectionIsCubeArray{false};

		private:

			/** @copydoc EmEn::Base::ObserverTrait::onNotification() */
			[[nodiscard]]
			bool onNotification (const Base::ObservableTrait * observable, int notificationCode, const std::any & data) noexcept override;

			/** @copydoc EmEn::Scenes::Component::Abstract::onSuspend() */
			void
			onSuspend () noexcept override
			{

			}

			/** @copydoc EmEn::Scenes::Component::Abstract::onWakeup() */
			void
			onWakeup () noexcept override
			{

			}

			/** @copydoc EmEn::Scenes::AVConsole::AbstractVirtualDevice::onOutputDeviceConnected() */
			void onOutputDeviceConnected (EngineContext & engineContext, AbstractVirtualDevice & targetDevice) noexcept final;

			/**
			 * @brief Creates the shadow descriptor set with the shadow map bound to binding 1.
			 * @param scene A reference to the scene.
			 * @return bool
			 */
			virtual bool createShadowDescriptorSet (Scene & scene) noexcept = 0;

			/**
			 * @brief Returns the field of view or the near value to update projection matrix.
			 * @return float
			 */
			[[nodiscard]]
			virtual float getFovOrNear () const noexcept = 0;

			/**
			 * @brief Returns the distance or the far value to update projection matrix.
			 * @return float
			 */
			[[nodiscard]]
			virtual float getDistanceOrFar () const noexcept = 0;

			/**
			 * @brief Returns the type of projection matrix.
			 * @return float
			 */
			[[nodiscard]]
			virtual bool isOrthographicProjection () const noexcept = 0;

			/**
			 * @brief Writes this light's uniform block into a destination buffer.
			 * @note ⚠️ Runs on the LOGIC thread, during publishStateForRendering() — it must not
			 * touch anything the render thread owns. It replaced onVideoMemoryUpdate(), which wrote
			 * straight into the GPU buffer from the render thread and was the reason a light's data
			 * never went through the two-state contract.
			 * @warning The destination holds MaxUniformBlockElementCount floats; the shared UBO's
			 * block is sized on the LARGEST light layout, so a shorter block simply leaves the tail
			 * untouched.
			 * @param destination Where to write the block.
			 * @return void
			 */
			virtual void writeUniformBlock (float * destination) noexcept = 0;

			/**
			 * @brief Event when the color light changes.
			 * @param color A reference to a color.
			 * @return void
			 */
			virtual void onColorChange (const Base::PixelFactory::Color< float > & color) noexcept = 0;

			/**
			 * @brief Event when the color intensity changes.
			 * @param intensity The amount.
			 * @return void
			 */
			virtual void onIntensityChange (float intensity) noexcept = 0;

			/* Flag names */
			static constexpr auto Enabled{UnusedFlag + 0UL};
			/* NOTE: VideoMemoryUpdateRequested was retired in Aug 2026 — a single dirty flag cannot
			 * drive N render state slots without leaving one of them stale. See m_logicGeneration. */
			static constexpr auto ShadowMapEnabled{UnusedFlag + 2UL};

			Base::PixelFactory::Color< float > m_color{DefaultColor};
			float m_intensity{DefaultIntensity};
			uint32_t m_shadowMapResolution{0};
			std::shared_ptr< Graphics::SharedUniformBuffer > m_sharedUniformBuffer;
			/** @brief Published copies of the uniform block, one per render state slot. */
			std::array< std::array< float, MaxUniformBlockElementCount >, 2 > m_publishedBlocks{};
			/** @brief Logic generation each published slot was filled from. */
			std::array< uint32_t, 2 > m_publishedGeneration{};
			/**
			 * @brief Bumped by requestVideoMemoryUpdate() on the logic thread.
			 * @note ⚠️ A GENERATION, not a boolean. A single dirty flag consumed by the first
			 * publish would leave the OTHER slot holding the previous value for ever — the classic
			 * 1-in-N staleness a naive double-buffering fix manufactures. Each slot compares against
			 * this counter, so each one refreshes the first time it is published after a change.
			 * Starts at 1 so slot generation 0 forces the initial fill.
			 */
			uint32_t m_logicGeneration{1};
			/**
			 * @brief Published generation last uploaded, PER frame-in-flight region.
			 * @note ⚠️ Per region, not global: each region is a distinct piece of GPU memory and
			 * needs its own copy of a change. One shared counter would upload the first region and
			 * leave the others on the previous value.
			 */
			std::array< uint32_t, MaxFrameRegionCount > m_uploadedGeneration{};
			/**
			 * @brief Frame region this light's dynamic offset currently addresses.
			 * @note Set by updateVideoMemory() at frame begin, so UBOOffset() cannot address a
			 * region other than the one this frame actually wrote. Render thread only.
			 */
			uint32_t m_currentFrameRegion{0};
			uint32_t m_sharedUBOIndex{0};
			std::shared_ptr< Vulkan::TextureInterface > m_colorProjectionTexture;
			uint32_t m_colorProjectionBindlessIndex{NoColorProjectionTexture};
			float m_colorProjectionBoost{0.0F};
	};
}
