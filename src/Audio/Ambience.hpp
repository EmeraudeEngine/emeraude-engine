/*
 * src/Audio/Ambience.hpp
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
#include <cstddef>
#include <cstdint>
#include <vector>
#include <memory>

/* Local inclusions for usages. */
#include "Libs/Utility.hpp"
#include "AmbienceChannel.hpp"
#include "AmbienceSound.hpp"
#include "Manager.hpp"

namespace EmEn::Audio
{
	/**
	 * @brief The ambience class.
	 * @note [OBS][STATIC-OBSERVER]
	 * @extends EmEn::Libs::ObserverTrait This component observes sound loading events.
	 */
	class Ambience final : public Libs::ObserverTrait
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"Ambience"};

			/**
			 * @brief Constructs an ambience.
			 * @param audioManager A reference to the audio manager to request sources.
			 */
			explicit
			Ambience (Manager & audioManager) noexcept
				: m_audioManager{audioManager}
			{

			}

			/**
			 * @brief Copy constructor.
			 * @param copy A reference to the copied instance.
			 */
			Ambience (const Ambience & copy) noexcept = delete;

			/**
			 * @brief Move constructor.
			 * @param copy A reference to the copied instance.
			 */
			Ambience (Ambience && copy) noexcept = delete;

			/**
			 * @brief Copy assignment.
			 * @param copy A reference to the copied instance.
			 */
			Ambience & operator= (const Ambience & copy) noexcept = delete;

			/**
			 * @brief Move assignment.
			 * @param copy A reference to the copied instance.
			 */
			Ambience & operator= (Ambience && copy) noexcept = delete;

			/**
			 * @brief Destructs the ambience.
			 */
			~Ambience () override;

			/**
			 * @brief Returns whether the ambience sound set is playing.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isPlaying () const noexcept
			{
				return m_active;
			}

			/**
			 * @brief Sets the number of sounds that can be played simultaneously.
			 * @note This will change after a call to start().
			 * @param count The number of channels.
			 * @return void
			 */
			void
			setChannelCount (size_t count) noexcept
			{
				m_channelCount = std::max(static_cast< size_t >(1), count);
			}

			/**
			 * @brief Returns the number of sounds that can be played simultaneously.
			 * @return size_t
			 */
			[[nodiscard]]
			size_t
			channelCount () const noexcept
			{
				return m_channelCount;
			}

			/**
			 * @brief Sets a distance around the listener where sound will be played randomly.
			 * @param radius A value in engine metric units.
			 * @return void
			 */
			void setRadius (float radius) noexcept;

			/**
			 * @brief Returns the distance around the listener where sound will be played randomly.
			 * @return float
			 */
			[[nodiscard]]
			float
			radius () const noexcept
			{
				return m_radius;
			}

			/**
			 * @brief Sets a random delay range.
			 * @param delayA A delay in milliseconds.
			 * @param delayB Another delay.
			 * @return void
			 */
			void
			setRandomDelayRange (uint32_t delayA, uint32_t delayB) noexcept
			{
				m_minDelay = std::min(delayA, delayB);
				m_maxDelay = std::max(delayA, delayB);
			}

			/**
			 * @brief Sets the loop sound effect.
			 * @param soundResource A reference to a sound resource smart pointer.
			 * @param gain The loop channel gain. Default 0.75F.
			 * @return bool
			 */
			bool setLoopSound (const std::shared_ptr< SoundResource > & soundResource, float gain = DefaultGain) noexcept;

			/**
			 * @brief Adds a sound effect to the ambience list.
			 * @param sound A reference to a sound effect.
			 * @return bool
			 */
			bool addSound (const AmbienceSound & sound) noexcept;

			/**
			 * @brief Adds a sound effect to the ambience list.
			 * @param sound A reference to a sound resource smart pointer.
			 * @param channelGain The channel gain. Default 0.75.
			 * @param relative The relative state of the channel to the listener. Default true.
			 * @param minPitch The minimum pitch value. Default 1.0.
			 * @param maxPitch The maximum pitch value. Default 1.0.
			 * @param velocity The radial velocity value. Default 0.0.
			 * @return bool
			 */
			bool addSound (const std::shared_ptr< SoundResource > & sound, float channelGain = DefaultGain,  bool relative = true, float minPitch = 1.0F, float maxPitch = 1.0F, float velocity = 0.0F) noexcept;

			/**
			 * @brief Enables a direct filter.
			 * @warning This method only works with the EFX extension.
			 * @param filter A reference to a filter smart pointer.
			 * @return bool
			 */
			bool enableDirectFilter (const std::shared_ptr< Filters::Abstract > & filter) noexcept;

			/**
			 * @brief Disables the direct filter.
			 * @warning This method only works with the EFX extension.
			 * @return void
			 */
			void disableDirectFilter () noexcept;

			/**
			 * @brief Starts the ambience.
			 * @return void
			 */
			void start () noexcept;

			/**
			 * @brief Stops all sound from the ambience.
			 * @return void
			 */
			void stop () noexcept;

			/**
			 * @brief Updates the logics.
			 * @return void
			 */
			void update () noexcept;

			/**
			 * @brief Loads a sound set from a JSON file.
			 * @param resourceManager A reference to the resource manager.
			 * @param filepath The path to the filesystem path.
			 * @return bool
			 */
			bool loadSoundSet (Resources::Manager & resourceManager, const std::filesystem::path & filepath) noexcept;

			/**
			 * @brief Reset the current sound set.
			 * @return void
			 */
			void reset () noexcept;

		private:

			/** @copydoc EmEn::Libs::ObserverTrait::onNotification() */
			[[nodiscard]]
			bool onNotification (const Libs::ObservableTrait * observable, int notificationCode, const std::any & data) noexcept override;

			/**
			 * @brief Returns a random delay.
			 * @return uint32_t
			 */
			[[nodiscard]]
			uint32_t
			getRandomDelay () const noexcept
			{
				return Libs::Utility::quickRandom(m_minDelay, m_maxDelay);
			}

			/**
			 * @brief Returns whether this ambience needs a loop channel.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			requestLoopChannel () const noexcept
			{
				return m_loopSound != nullptr;
			}

			/**
			 * @brief Returns whether this ambience needs a sound effect channel.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			requestSoundEffectChannels () const noexcept
			{
				return !m_soundEffects.empty();
			}

			/**
			 * @brief Allocates all sources.
			 * @return bool
			 */
			bool allocateSources () noexcept;

			/**
			 * @brief Releases all sources.
			 * @return void
			 */
			void releaseSources () noexcept;

			/**
			 * @brief Picks a random sound from the ambience list.
			 * @return AmbienceSound &
			 */
			[[nodiscard]]
			AmbienceSound &
			getRandomSound () noexcept
			{
				return m_soundEffects[Libs::Utility::quickRandom(static_cast< size_t >(0), m_soundEffects.size() - 1)];
			}

			/* JSON key. */
			static constexpr auto JKLoopSoundEffect{"LoopSoundEffect"};
			static constexpr auto JKSoundEffectCollection{"SoundEffectCollection"};
			static constexpr auto JKChannelCount{"ChannelCount"};
			static constexpr auto JKRadius{"Radius"};
			static constexpr auto JKResourceName{"ResourceName"};
			static constexpr auto JKGain{"Gain"};
			static constexpr auto JKRelative{"Relative"};
			static constexpr auto JKMinimumPitch{"MinimumPitch"};
			static constexpr auto JKMaximumPitch{"MaximumPitch"};
			static constexpr auto JKRadialVelocity{"RadialVelocity"};

			/* Default values */
			static constexpr auto DefaultChannelCount{4UL};
			static constexpr auto DefaultGain{0.75F};
			static constexpr auto DefaultRadius{1000.0F};
			static constexpr auto DefaultMinDelay{500};
			static constexpr auto DefaultMaxDelay{3000};

			Manager & m_audioManager;
			std::shared_ptr< Source > m_loopedSource;
			std::vector< AmbienceChannel > m_channels;
			std::shared_ptr< Filters::Abstract > m_directFilter;
			std::shared_ptr< SoundResource > m_loopSound;
			std::vector< AmbienceSound > m_soundEffects;
			size_t m_channelCount{DefaultChannelCount};
			float m_loopedChannelGain{DefaultGain};
			float m_radius{DefaultRadius};
			uint32_t m_minDelay{DefaultMinDelay};
			uint32_t m_maxDelay{DefaultMaxDelay};
			bool m_active{false};
	};
}
