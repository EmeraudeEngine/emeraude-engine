/*
 * src/AVConsole/AbstractVirtualDevice.hpp
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
#include <memory>
#include <set>
#include <string>

/* Local inclusions for usages. */
#include "Libs/Math/CartesianFrame.hpp"
#include "Types.hpp"

namespace EmEn::AVConsole
{
	/**
	 * @brief This is the base class of each virtual multimedia device in the 3D world.
	 * @extends std::enable_shared_from_this
	 */
	class AbstractVirtualDevice : public std::enable_shared_from_this< AbstractVirtualDevice >
	{
		public:

			/**
			 * @brief Copy constructor.
			 * @param copy A reference to the copied instance.
			 */
			AbstractVirtualDevice (const AbstractVirtualDevice & copy) noexcept = default;

			/**
			 * @brief Move constructor.
			 * @param copy A reference to the copied instance.
			 */
			AbstractVirtualDevice (AbstractVirtualDevice && copy) noexcept = default;

			/**
			 * @brief Copy assignment.
			 * @param copy A reference to the copied instance.
			 * @return AbstractVirtualDevice &
			 */
			AbstractVirtualDevice & operator= (const AbstractVirtualDevice & copy) noexcept = default;

			/**
			 * @brief Move assignment.
			 * @param copy A reference to the copied instance.
			 * @return AbstractVirtualDevice &
			 */
			AbstractVirtualDevice & operator= (AbstractVirtualDevice && copy) noexcept = default;

			/**
			 * @brief Destructs an abstract device.
			 */
			virtual ~AbstractVirtualDevice () = default;

			/**
			 * @brief Returns the device id.
			 * @return const std::string &
			 */
			[[nodiscard]]
			const std::string &
			id () const noexcept
			{
				return m_id;
			}

			/**
			 * @brief Returns the device type.
			 * @return DeviceType
			 */
			[[nodiscard]]
			DeviceType
			deviceType () const noexcept
			{
				return m_type;
			}

			/**
			 * @brief Returns the device allowed a connexion type.
			 * @return ConnexionType
			 */
			[[nodiscard]]
			ConnexionType
			allowedConnexionType () const noexcept
			{
				return m_allowedConnexionType;
			}

			/**
			 * @brief Returns whether at least one virtual device is connected as input.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			hasInputConnected () const noexcept
			{
				return !m_inputs.empty();
			}

			/**
			 * @brief Returns all virtual devices connected to input.
			 * @return const std::set< std::shared_ptr< AbstractVirtualAudioDevice > > &
			 */
			[[nodiscard]]
			const std::set< std::shared_ptr< AbstractVirtualDevice > > &
			inputs () const noexcept
			{
				return m_inputs;
			}

			/**
			 * @brief Returns whether at least one virtual device is connected as output.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			hasOutputConnected () const noexcept
			{
				return !m_outputs.empty();
			}

			/**
			 * @brief Returns all virtual devices connected to output.
			 * @return const std::set< std::shared_ptr< AbstractVirtualAudioDevice > > &
			 */
			[[nodiscard]]
			const std::set< std::shared_ptr< AbstractVirtualDevice > > &
			outputs () const noexcept
			{
				return m_outputs;
			}

			/**
			 * @brief Returns whether a device is connected.
			 * @param device A reference to a virtual device smart pointer.
			 * @param direction The connexion direction tested. Both means as input or output.
			 * @return bool
			 */
			[[nodiscard]]
			bool isConnectedWith (const std::shared_ptr< AbstractVirtualDevice > & device, ConnexionType direction) const noexcept;

			/**
			 * @brief Checks if a device can be connected to output.
			 * @param targetDevice A reference to a virtual device smart pointer.
			 * @return bool
			 */
			[[nodiscard]]
			bool canConnect (const std::shared_ptr< AbstractVirtualDevice > & targetDevice) const noexcept;

			/**
			 * @brief Connects a virtual device to output.
			 * @note this[Output] -> target[Input]
			 * @param targetDevice A reference to a virtual device smart pointer.
			 * @param quietly Do not fire connexion event. Default false.
			 * @return bool
			 */
			bool connect (const std::shared_ptr< AbstractVirtualDevice > & targetDevice, bool quietly = false) noexcept;

			/**
			 * @brief Interconnects a virtual device between an existing output.
			 * @note this[Output] -> target[Input]
			 * @param targetDevice A reference to a virtual device smart pointer.
			 * @return bool
			 */
			bool interconnect (const std::shared_ptr< AbstractVirtualDevice > & targetDevice) noexcept;

			/**
			 * @brief Interconnects a virtual device between all existing outputs.
			 * @note this[Output] -> target[Input]
			 * @param targetDevice A reference to a virtual device smart pointer.
			 * @param outputDeviceName A reference to a string.
			 * @return bool
			 */
			bool interconnect (const std::shared_ptr< AbstractVirtualDevice > & targetDevice, const std::string & outputDeviceName) noexcept;

			/**
			 * @brief Disconnects the output of this virtual device from the input of a virtual device.
			 * @param targetDevice A reference to a virtual device smart pointer.
			 * @param quietly Do not fire disconnection event. Default false.
			 * @return bool
			 */
			bool disconnect (const std::shared_ptr< AbstractVirtualDevice > & targetDevice, bool quietly = false) noexcept;

			/**
			 * @brief Disconnects the device from everything.
			 * @return void
			 */
			void disconnectFromAll () noexcept;

			/**
			 * @brief Returns a printable state of connexions.
			 * @return std::string
			 */
			[[nodiscard]]
			std::string getConnexionState () const noexcept;

			/**
			 * @brief Updates the device from object coordinates in world space holding it.
			 * @param worldCoordinates A reference to the coordinates of the device.
			 * @param worldVelocity A reference to the velocity vector of the device.
			 * @return void
			 */
			virtual void updateDeviceFromCoordinates (const Libs::Math::CartesianFrame< float > & worldCoordinates, const Libs::Math::Vector< 3, float > & worldVelocity) noexcept = 0;

			/**
			 * @brief Returns the video device type.
			 * @note Ignored on audio device.
			 * @todo This should not be here !
			 * @return VideoType
			 */
			[[nodiscard]]
			virtual
			VideoType
			videoType () const noexcept
			{
				/* NOTE: A video device should override this method! */
				assert(m_type == DeviceType::Audio);

				return VideoType::NotVideoDevice;
			}

			/**
			 * @brief Updates the video device properties.
			 * @note Ignored on audio device.
			 * @todo This should not be here !
			 * @param isPerspectiveProjection Declares the perspective type of image.
			 * @param distance The maximal distance of the view.
			 * @param fovOrNear The field of view. Ignored if the projection is orthographic and can be used as near value override.
			 * @return void
			 */
			virtual
			void
			updateProperties (bool /*isPerspectiveProjection*/, float /*distance*/, float /*fovOrNear*/) noexcept
			{
				/* NOTE: A video device should override this method! */
				assert(m_type == DeviceType::Audio);
			}

			/**
			 * @brief Event fired when a virtual device is connected to input.
			 * @note This method uses a pointer instead of reference to ease the dynamic cast. It will never be null.
			 * @param sourceDevice A pointer to the virtual device.
			 * @return void
			 */
			virtual void onSourceConnected (AbstractVirtualDevice * /*sourceDevice*/) noexcept
			{

			}

			/**
			 * @brief Event fired when a virtual device is connected to output.
			 * @note This method uses a pointer instead of reference to ease the dynamic cast. It will never be null.
			 * @param targetDevice A pointer to the virtual device.
			 * @return void
			 */
			virtual void onTargetConnected (AbstractVirtualDevice * /*targetDevice*/) noexcept
			{

			}

			/**
			 * @brief Event fired when a virtual device is disconnected to input.
			 * @note This method uses a pointer instead of reference to ease the dynamic cast. It will never be null.
			 * @param sourceDevice A pointer to the virtual device.
			 * @return void
			 */
			virtual void onSourceDisconnected (AbstractVirtualDevice * /*sourceDevice*/) noexcept
			{

			}

			/**
			 * @brief Event fired when a virtual device is disconnected to output.
			 * @note This method uses a pointer instead of reference to ease the dynamic cast. It will never be null.
			 * @param targetDevice A pointer to the virtual device.
			 * @return void
			 */
			virtual void onTargetDisconnected (AbstractVirtualDevice * /*targetDevice*/) noexcept
			{

			}

		protected:

			/**
			 * @brief Constructs an abstract device.
			 * @param name A reference to a string for the device name.
			 * @param type The type of the device.
			 * @param allowedConnexionType The type of connexion this virtual device allows.
			 */
			explicit AbstractVirtualDevice (const std::string & name, DeviceType type, ConnexionType allowedConnexionType) noexcept;

		private:

			/**
			 * @brief Builds a device id.
			 * @param name A reference to a string for the device name.
			 * @return std::string
			 */
			[[nodiscard]]
			static std::string buildDeviceId (const std::string & name) noexcept;

			/**
			 * @brief Connects back a connected virtual device.
			 * @param sourceDevice A reference to a virtual device smart pointer.
			 * @param quietly Do not fire connexion event. Default false.
			 * @return bool
			 */
			bool connectBack (const std::shared_ptr< AbstractVirtualDevice > & sourceDevice, bool quietly) noexcept;

			/**
			 * @brief Disconnects back a connected virtual device.
			 * @param sourceDevice A reference to a virtual device smart pointer.
			 * @param quietly Do not fire disconnection event. Default false.
			 * @return bool
			 */
			bool disconnectBack (const std::shared_ptr< AbstractVirtualDevice > & sourceDevice, bool quietly) noexcept;

			static size_t s_deviceCount;

			std::string m_id;
			DeviceType m_type;
			ConnexionType m_allowedConnexionType;
			std::set< std::shared_ptr< AbstractVirtualDevice > > m_inputs;
			std::set< std::shared_ptr< AbstractVirtualDevice > > m_outputs;
	};
}
