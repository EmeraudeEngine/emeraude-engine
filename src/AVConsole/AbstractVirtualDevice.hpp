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
#include <mutex>
#include <unordered_set>
#include <string>
#include <atomic>

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
			AbstractVirtualDevice (const AbstractVirtualDevice & copy) noexcept = delete;

			/**
			 * @brief Move constructor.
			 * @param copy A reference to the copied instance.
			 */
			AbstractVirtualDevice (AbstractVirtualDevice && copy) noexcept = delete;

			/**
			 * @brief Copy assignment.
			 * @param copy A reference to the copied instance.
			 * @return AbstractVirtualDevice &
			 */
			AbstractVirtualDevice & operator= (const AbstractVirtualDevice & copy) noexcept = delete;

			/**
			 * @brief Move assignment.
			 * @param copy A reference to the copied instance.
			 * @return AbstractVirtualDevice &
			 */
			AbstractVirtualDevice & operator= (AbstractVirtualDevice && copy) noexcept = delete;

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
			 * @brief Returns whether at least one virtual device is connected as an input.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			hasInputConnected () const noexcept
			{
				const std::lock_guard< std::mutex > lock{m_IOAccess};

				return !m_inputDevicesConnected.empty();
			}

			/**
			 * @brief Returns whether at least one virtual device is connected as an output.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			hasOutputConnected () const noexcept
			{
				const std::lock_guard< std::mutex > lock{m_IOAccess};

				return !m_outputDevicesConnected.empty();
			}

			/**
			 * @brief Executes a function over each input.
			 * @tparam function_t The type of function. Signature: void (const std::shared_ptr< AbstractVirtualDevice > &)
			 * @param processInput
			 * @return void
			 */
			template< typename function_t >
			void
			forEachInputs (function_t && processInput) const noexcept requires (std::is_invocable_v< function_t, const std::shared_ptr< AbstractVirtualDevice > & >)
			{
				const std::lock_guard< std::mutex > lock{m_IOAccess};

				for ( const auto & input : m_inputDevicesConnected )
				{
					processInput(input.lock());
				}
			}

			/**
			 * @brief Executes a function over each output.
			 * @tparam function_t The type of function. Signature: void (const std::shared_ptr< AbstractVirtualDevice > &)
			 * @param processOutput
			 * @return void
			 */
			template< typename function_t >
			void
			forEachOutputs (function_t && processOutput) const noexcept requires (std::is_invocable_v< function_t, const std::shared_ptr< AbstractVirtualDevice > & >)
			{
				const std::lock_guard< std::mutex > lock{m_IOAccess};

				for ( const auto & output : m_outputDevicesConnected )
				{
					processOutput(output.lock());
				}
			}

			/**
			 * @brief Returns whether a device is connected.
			 * @param device A reference to a virtual device smart pointer.
			 * @param direction The connexion direction tested. ConnexionType::Both means as input or output.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isConnectedWith (const std::shared_ptr< AbstractVirtualDevice > & device, ConnexionType direction) const noexcept
			{
				const std::lock_guard< std::mutex > lock{m_IOAccess};

				if ( direction == ConnexionType::Input || direction == ConnexionType::Both )
				{
					return m_inputDevicesConnected.contains(device);
				}

				/* NOTE: Both are already tested above. */
				if ( direction == ConnexionType::Output /*|| direction == ConnexionType::Both*/ )
				{
					return m_outputDevicesConnected.contains(device);
				}

				return false;
			}

			/**
			 * @brief Checks if a target device input can be connected to this device output.
			 * @param targetDevice A reference to a virtual device smart pointer.
			 * @return ConnexionResult
			 */
			[[nodiscard]]
			ConnexionResult
			canConnect (const std::shared_ptr< AbstractVirtualDevice > & targetDevice) const noexcept
			{
				/* NOTE: Avoid connecting an audio device with a video device -> non sens! */
				if ( m_type != targetDevice->m_type )
				{
					return ConnexionResult::DifferentDeviceType;
				}

				/* NOTE: As connexions go from device output to another device input, this device must allow outputting! */
				if ( m_allowedConnexionType == ConnexionType::Input || targetDevice->m_allowedConnexionType == ConnexionType::Output )
				{
					return ConnexionResult::NotAllowed;
				}

				return ConnexionResult::Success;
			}

			/**
			 * @brief Connects a virtual device to output.
			 * @note this[Output] -> target[Input]
			 * @param managers A reference to the audio video managers.
			 * @param targetDevice A reference to a virtual device smart pointer.
			 * @param fireEvents Set to fire or not events on connexion.
			 * @return ConnexionResult
			 */
			ConnexionResult connect (AVManagers & managers, const std::shared_ptr< AbstractVirtualDevice > & targetDevice, bool fireEvents) noexcept;

			/**
			 * @brief Interconnects a virtual device between all existing outputs.
			 * @note this[Output] -> target[Input]
			 * @param managers A reference to the audio video managers.
			 * @param intermediateDevice A reference to a virtual device smart pointer.
			 * @param fireEvents Set to fire or not events on connexion.
			 * @return ConnexionResult
			 */
			ConnexionResult interconnect (AVManagers & managers, const std::shared_ptr< AbstractVirtualDevice > & intermediateDevice, bool fireEvents) noexcept;

			/**
			 * @brief Interconnects a virtual device between a specific output.
			 * @note this[Output] -> target[Input]
			 * @param managers A reference to the audio video managers.
			 * @param intermediateDevice A reference to a virtual device smart pointer.
			 * @param outputDeviceName A reference to a string to filter an output. If this device does not exist, the method will perform no connexion.
			 * @param fireEvents Set to fire or not events on connexion.
			 * @return ConnexionResult
			 */
			ConnexionResult interconnect (AVManagers & managers, const std::shared_ptr< AbstractVirtualDevice > & intermediateDevice, const std::string & outputDeviceName, bool fireEvents) noexcept;

			/**
			 * @brief Disconnects the output of this virtual device from the input of a virtual device.
			 * @param managers A reference to the audio video managers.
			 * @param targetDevice A reference to a virtual device smart pointer.
			 * @param fireEvents Set to fire or not events on disconnection.
			 * @return ConnexionResult
			 */
			ConnexionResult disconnect (AVManagers & managers, const std::shared_ptr< AbstractVirtualDevice > & targetDevice, bool fireEvents) noexcept;

			/**
			 * @brief Disconnects the device from everything.
			 * @param managers A reference to the audio video managers.
			 * @param fireEvents Set to fire or not events on disconnection.
			 * @return void
			 */
			void disconnectFromAll (AVManagers & managers, bool fireEvents) noexcept;

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
			 * @param isPerspectiveProjection Declares the perspective type of the image.
			 * @param distance The maximal distance of the view.
			 * @param fovOrNear The field of view. Ignored if the projection is orthographic and can be used as a near-value override.
			 * @return void
			 */
			virtual
			void
			updateProperties (bool /*isPerspectiveProjection*/, float /*distance*/, float /*fovOrNear*/) noexcept
			{
				/* NOTE: A video device should override this method! */
				assert(m_type == DeviceType::Audio);
			}

		protected:

			/**
			 * @brief Constructs an abstract device.
			 * @param name A reference to a string for the device name.
			 * @param type The type of the device.
			 * @param allowedConnexionType The type of connexion this virtual device allows.
			 */
			explicit
			AbstractVirtualDevice (const std::string & name, DeviceType type, ConnexionType allowedConnexionType) noexcept
				: m_id{buildDeviceId(name)},
				m_type{type},
				m_allowedConnexionType{allowedConnexionType}
			{

			}

			/**
			 * @brief Event fired when a virtual device is connected to input.
			 * @note This method uses a pointer instead of a reference to ease the dynamic cast. It will never be null.
			 * @param managers A reference to the audio video managers.
			 * @param inputDevice A reference to the virtual device.
			 * @return void
			 */
			virtual
			void
			onInputDeviceConnected (AVManagers & /*managers*/, AbstractVirtualDevice & /*inputDevice*/) noexcept
			{

			}

			/**
			 * @brief Event fired when a virtual device is connected to output.
			 * @note This method uses a pointer instead of a reference to ease the dynamic cast. It will never be null.
			 * @param managers A reference to the audio video managers.
			 * @param outputDevice A reference to the virtual device.
			 * @return void
			 */
			virtual
			void
			onOutputDeviceConnected (AVManagers & /*managers*/, AbstractVirtualDevice & /*outputDevice*/) noexcept
			{

			}

			/**
			 * @brief Event fired when a virtual device is disconnected to input.
			 * @note This method uses a pointer instead of a reference to ease the dynamic cast. It will never be null.
			 * @param managers A reference to the audio video managers.
			 * @param inputDevice A reference to the virtual device.
			 * @return void
			 */
			virtual
			void
			onInputDeviceDisconnected (AVManagers & /*managers*/, AbstractVirtualDevice & /*inputDevice*/) noexcept
			{

			}

			/**
			 * @brief Event fired when a virtual device is disconnected to output.
			 * @note This method uses a pointer instead of a reference to ease the dynamic cast. It will never be null.
			 * @param managers A reference to the audio video managers.
			 * @param outputDevice A reference to the virtual device.
			 * @return void
			 */
			virtual
			void
			onOutputDeviceDisconnected (AVManagers & /*managers*/, AbstractVirtualDevice & /*outputDevice*/) noexcept
			{

			}

		private:

			/**
			 * @brief Builds a device id.
			 * @param name A reference to a string for the device name.
			 * @return std::string
			 */
			[[nodiscard]]
			static
			std::string
			buildDeviceId (const std::string & name) noexcept
			{
				std::stringstream deviceId;

				deviceId << name << '_' << s_deviceCount++;

				return deviceId.str();
			}

			static std::atomic_size_t s_deviceCount;

			const std::string m_id;
			const DeviceType m_type;
			const ConnexionType m_allowedConnexionType;
			std::unordered_set< std::weak_ptr< AbstractVirtualDevice >, WeakPtrOwnerHash, WeakPtrOwnerEqual > m_inputDevicesConnected;
			std::unordered_set< std::weak_ptr< AbstractVirtualDevice >, WeakPtrOwnerHash, WeakPtrOwnerEqual > m_outputDevicesConnected;
			mutable std::mutex m_IOAccess;
	};
}
