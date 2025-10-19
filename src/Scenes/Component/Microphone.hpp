/*
 * src/Scenes/Component/Microphone.hpp
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
#include <string>

/* Local inclusions for inheritances. */
#include "AVConsole/AbstractVirtualDevice.hpp"
#include "Abstract.hpp"

namespace EmEn::Scenes::Component
{
	/**
	 * @brief This class defines a physical point of capturing sound in the world.
	 * Like ears from a creature or microphone from a camera.
	 * @extends EmEn::Scenes::Component::Abstract The base class for each entity component.
	 * @extends EmEn::AVConsole::AbstractVirtualDevice This is a virtual audio device.
	 */
	class Microphone final : public Abstract, public AVConsole::AbstractVirtualDevice
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"Microphone"};

			/**
			 * @brief Constructs a microphone.
			 * @param componentName A reference to a string.
			 * @param parentEntity A reference to the parent entity.
			 */
			Microphone (const std::string & componentName, const AbstractEntity & parentEntity) noexcept
				: Abstract{componentName, parentEntity},
				AbstractVirtualDevice{componentName, AVConsole::DeviceType::Audio, AVConsole::ConnexionType::Output}
			{

			}

			/** @copydoc EmEn::Scenes::Component::Abstract::getComponentType() const noexcept */
			[[nodiscard]]
			const char *
			getComponentType () const noexcept override
			{
				return ClassId;
			}

			/** @copydoc EmEn::Scenes::Component::Abstract::isComponent() const noexcept */
			[[nodiscard]]
			bool
			isComponent (const char * classID) const noexcept override
			{
				return strcmp(ClassId, classID) == 0;
			}

			/** @copydoc EmEn::Scenes::Component::Abstract::move(const Libs::Math::CartesianFrame< float > &) noexcept */
			void
			move (const Libs::Math::CartesianFrame< float > & worldCoordinates) noexcept override
			{
				this->updateDeviceFromCoordinates(worldCoordinates, this->getWorldVelocity());
			}

			/** @copydoc EmEn::Scenes::Component::Abstract::processLogics(const Scene &) noexcept */
			void
			processLogics (const Scene & /*scene*/) noexcept override
			{

			}

			/** @copydoc EmEn::Scenes::Component::Abstract::shouldBeRemoved() const noexcept */
			[[nodiscard]]
			bool
			shouldBeRemoved () const noexcept override
			{
				return false;
			}

		private:

			/** @copydoc EmEn::AVConsole::AbstractVirtualDevice::updateDeviceFromCoordinates() noexcept */
			void updateDeviceFromCoordinates (const Libs::Math::CartesianFrame< float > & worldCoordinates, const Libs::Math::Vector< 3, float > & worldVelocity) noexcept override;

			/** @copydoc EmEn::AVConsole::AbstractVirtualDevice::onOutputDeviceConnected() noexcept */
			void
			onOutputDeviceConnected (AVConsole::AVManagers & /*managers*/, AbstractVirtualDevice & targetDevice) noexcept override
			{
				targetDevice.updateDeviceFromCoordinates(this->getWorldCoordinates(), this->getWorldVelocity());
			}

			/** @copydoc EmEn::Animations::AnimatableInterface::playAnimation() noexcept */
			bool playAnimation (uint8_t animationID, const Libs::Variant & value, size_t cycle) noexcept override;

			/**
			 * @brief STL streams printable object.
			 * @param out A reference to the stream output.
			 * @param obj A reference to the object to print.
			 * @return std::ostream &
			 */
			friend std::ostream & operator<< (std::ostream & out, const Microphone & obj);
	};

	inline
	std::ostream &
	operator<< (std::ostream & out, const Microphone & obj)
	{
		const auto coordinates = obj.getWorldCoordinates();
		const auto velocity = obj.getWorldVelocity();

		return out <<
			"Audio Listener information" "\n"
			"Position: " << coordinates.position() << "\n"
			"Forward: " << coordinates.forwardVector() << "\n"
			"Velocity: " << velocity << "\n";
	}

	/**
	 * @brief Stringifies the object.
	 * @param obj A reference to the object to print.
	 * @return std::string
	 */
	inline
	std::string
	to_string (const Microphone & obj) noexcept
	{
		std::stringstream output;

		output << obj;

		return output.str();
	}
}
