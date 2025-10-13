/*
 * src/Settings.hpp
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

/* Application configuration. */
#include "emeraude_config.hpp"

/* STL inclusions. */
#include <map>
#include <vector>
#include <string>
#include <any>
#include <filesystem>
#include <shared_mutex>

/* Local inclusion for inheritances. */
#include "ServiceInterface.hpp"

/* Local inclusion for usages. */
#include "Tracer.hpp"

/* Forward declarations. */
namespace EmEn
{
	class Arguments;
	class FileSystem;
}

namespace Json
{
	class Value;
}

namespace EmEn
{
	/** @brief Types allowed by settings. */
	template< typename variable_t >
	concept SettingType = std::is_same_v< variable_t, bool > ||
						  std::is_same_v< variable_t, int32_t > ||
						  std::is_same_v< variable_t, uint32_t > ||
						  std::is_same_v< variable_t, int64_t > ||
						  std::is_same_v< variable_t, uint64_t > ||
						  std::is_same_v< variable_t, float > ||
						  std::is_same_v< variable_t, double > ||
						  std::is_same_v< variable_t, std::string >;

	/**
	 * @brief The setting store class.
	 * @note This class is private details of Settings and should never be accessed outside Settings.
	 */
	class SettingStore final
	{
		friend class Settings;

		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"SettingStore"};

			/**
			 * @brief Constructs a default setting store.
			 */
			SettingStore () noexcept = default;

			/**
			 * @brief Returns variables.
			 * @return const std::map< std::string, std::any, std::less<> > &
			 */
			[[nodiscard]]
			const std::map< std::string, std::any, std::less<> > &
			variables () const noexcept
			{
				return m_variables;
			}

			/**
			 * @brief Returns arrays variables.
			 * @return const std::map< std::string, std::vector< std::any >, std::less<> > &
			 */
			[[nodiscard]]
			const std::map< std::string, std::vector< std::any >, std::less<> > &
			arrays () const noexcept
			{
				return m_arrays;
			}

			/**
			 * @brief Sets an "any" variable in store.
			 * @param name A reference to a string for the variable name.
			 * @param value A reference to a std::any.
			 * @return void
			 */
			void
			setVariable (const std::string & name, const std::any & value)
			{
				m_variables[name] = value;
			}

			/**
			 * @brief Sets an "any" variable in an array store.
			 * @param name A reference to a string for the variable name.
			 * @param value A reference to a std::any.
			 * @return void
			 */
			void
			setVariableInArray (const std::string & name, const std::any & value)
			{
				m_arrays[name].emplace_back(value);
			}

			/**
			 * @brief Clears an array if the variable is an array.
			 * @param variableName A reference to a string for the variable name.
			 * @return void
			 */
			void
			clearArray (std::string_view variableName) noexcept
			{
				if ( const auto arrayIt = m_arrays.find(variableName); arrayIt != m_arrays.end() )
				{
					arrayIt->second.clear();
				}
			}

			/**
			 * @brief Removes a variable from store data.
			 * @param name A reference to a string for the variable name.
			 * @return void
			 */
			void
			removeKey (const std::string & name) noexcept
			{
				m_variables.erase(name);
				m_arrays.erase(name);
			}

			/**
			 * @brief Returns whether the store is empty.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			empty () const noexcept
			{
				return m_variables.empty() && m_arrays.empty();
			}

			/**
			 * @brief Checks whether a variable is present in the store.
			 * @param variableName A reference to a string for the variable name.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			variableExists (std::string_view variableName) const noexcept
			{
				return m_variables.contains(variableName);
			}

			/**
			 * @brief Checks whether an array is present in the store.
			 * @param variableName A reference to a string for the array name.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			arrayExists (std::string_view variableName) const noexcept
			{
				return m_arrays.contains(variableName);
			}

			/**
			 * @brief Gets the value of a variable.
			 * @param variableName A reference to a string for the variable name.
			 * @return const std::any *
			 */
			[[nodiscard]]
			const std::any *
			getValuePointer (std::string_view variableName) const noexcept
			{
				const auto variableIt = m_variables.find(variableName);

				if ( variableIt == m_variables.cend() )
				{
					return nullptr;
				}

				return &variableIt->second;
			}

			/**
			 * @brief Returns const access to an array of variable.
			 * @param variableName A reference to a string for the array name.
			 * @return const std::vector< std::any > *
			 */
			[[nodiscard]]
			const std::vector< std::any > *
			getArrayPointer (std::string_view variableName) const noexcept
			{
				const auto arrayIt = m_arrays.find(variableName);

				if ( arrayIt == m_arrays.cend() )
				{
					return nullptr;
				}

				return &arrayIt->second;
			}

			/**
			 * @brief Clears store data.
			 * @return void
			 */
			void
			clear () noexcept
			{
				m_variables.clear();
				m_arrays.clear();
			}

		private:

			/* TODO: Simplify by using only "std::map< std::string, std::any, std::less<> > m_data;" and rework accessors. */
			std::map< std::string, std::any, std::less<> > m_variables;
			std::map< std::string, std::vector< std::any >, std::less<> > m_arrays;
	};

	/**
	 * @brief The settings service class.
	 * @extends EmEn::ServiceInterface This is a service.
	 */
	class Settings final : public ServiceInterface
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"SettingsService"};

			static constexpr auto Filename{"settings.json"};
			static constexpr auto VersionKey{"WrittenByAppVersion"};
			static constexpr auto DateKey{"WrittenAtDate"};

			/**
			 * @brief Constructs a settings manager.
			 * @param arguments A reference to application arguments.
			 * @param fileSystem A reference to the file system.
			 * @param childProcess Declares a child process.
			 */
			Settings (const Arguments & arguments, const FileSystem & fileSystem, bool childProcess) noexcept
				: ServiceInterface{ClassId},
				m_arguments{arguments},
				m_fileSystem{fileSystem}
			{
				m_childProcess = childProcess;

				if ( !m_childProcess )
				{
					m_saveAtExit = true;
				}
			}

			/**
			 * @brief Destructs the settings manager.
			 */
			~Settings () override = default;

			/**
			 * @brief Returns the file path for these settings.
			 * @return const std::filesystem::path &
			 */
			[[nodiscard]]
			const std::filesystem::path &
			filepath () const noexcept
			{
				return m_filepath;
			}

			/**
			 * @brief Sets if the settings must be written in file at the end of the application.
			 * @param state The state.
			 * @return void
			 */
			void
			saveAtExit (bool state) noexcept
			{
				m_saveAtExit = state;
			}

			/**
			 * @brief Returns whether the settings will be saved at application shutdown.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isSaveAtExitEnabled () const noexcept
			{
				return m_saveAtExit;
			}

			/**
			 * @brief Returns whether the service is from a child process.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isChildProcess () const noexcept
			{
				return m_childProcess;
			}

			/**
			 * @brief Sets a value in the store.
			 * @tparam variable_t The type of the value.
			 * @param settingPath A string view.
			 * @param value A reference to a value to save.
			 * @return void
			 */
			template< SettingType variable_t >
			void
			set (std::string_view settingPath, const variable_t & value)
			{
				const std::unique_lock< std::shared_mutex > lock{m_storeAccess};

				const auto & [key, variableName] = Settings::parseAccessKey(settingPath);

				m_stores[std::string{key}].setVariable(std::string{variableName}, value);
			}

			/** @copydoc EmEn::Settings::set(const std::string & key, const variable_t & value) */
			void
			set (std::string_view settingPath, const char * value)
			{
				this->set(settingPath, std::string{value});
			}

			/**
			 * @brief Sets a value in an array of the store.
			 * @tparam variable_t The type of the value.
			 * @param settingPath A string view.
			 * @param value A reference to a value to save.
			 * @return void
			 */
			template< SettingType variable_t >
			void
			setInArray (std::string_view settingPath, const variable_t & value)
			{
				const std::unique_lock< std::shared_mutex > lock{m_storeAccess};

				const auto & [key, variableName] = Settings::parseAccessKey(settingPath);

				m_stores[std::string{key}].setVariableInArray(std::string{variableName}, value);
			}

			/** @copydoc EmEn::Settings::setInArray(const std::string & key, const variable_t & value) */
			void
			setInArray (std::string_view settingPath, const char * value)
			{
				this->setInArray(settingPath, std::string{value});
			}

			/**
			 * @brief Checks the present of a variable in the settings.
			 * @param settingPath A string view.
			 * @return bool
			 */
			[[nodiscard]]
			bool variableExists (std::string_view settingPath) const noexcept;

			/**
			 * @brief Checks the present of an array in the settings.
			 * @param settingPath A string view.
			 * @return bool
			 */
			[[nodiscard]]
			bool arrayExists (std::string_view settingPath) const noexcept;

			/**
			 * @brief Returns whether a variable as an array is empty.
			 * @param settingPath A string view.
			 * @return bool
			 */
			[[nodiscard]]
			bool isArrayEmpty (std::string_view settingPath) const noexcept;

			/**
			 * @brief Returns a variable from settings with a key.
			 * @tparam variable_t The variable type.
			 * @param key A reference to a string for the variable name.
			 * @param defaultValue The default value. Default none.
			 * @return variable_t
			 */
			template< SettingType variable_t >
			[[nodiscard]]
			variable_t
			get (const std::string & key, const variable_t & defaultValue = {}) const noexcept
			{
				const std::shared_lock< std::shared_mutex > lock{m_storeAccess};

				if ( const auto value = this->getVariable(key); value.has_value() )
				{
					return Settings::convertAnyTo< variable_t >(value, defaultValue);
				}

				return defaultValue;
			}

			/**
			 * @brief Returns a single variable from settings.
			 * @tparam variable_t The variable type.
			 * @param key A reference to a string for the variable name.
			 * @param defaultValue The default value. Default none.
			 * @return variable_t
			 */
			template< SettingType variable_t >
			[[nodiscard]]
			variable_t
			getOrSetDefault (const std::string & key, const variable_t & defaultValue = {})
			{
				const std::unique_lock< std::shared_mutex > lock{m_storeAccess};

				const auto [storeKey, variableName] = Settings::parseAccessKey(key);

				auto & store = m_stores[std::string{storeKey}];

				if ( const auto * valuePtr = store.getValuePointer(variableName) )
				{
					return Settings::convertAnyTo< variable_t >(*valuePtr, defaultValue);
				}

				store.setVariable(std::string{variableName}, defaultValue);

				return defaultValue;
			}

			/**
			 * @brief Returns a vector of typed data.
			 * @note If one or more item of the array do not fit the desired type, it will be ignored.
			 * @tparam variable_t The type of the value.
			 * @param settingPath A string view.
			 * @return std::vector< std::string >
			 */
			template< SettingType variable_t >
			[[nodiscard]]
			std::vector< variable_t >
			getArrayAs (std::string_view settingPath) const noexcept
			{
				const std::shared_lock< std::shared_mutex > lock{m_storeAccess};

				const auto & [key, variableName] = Settings::parseAccessKey(settingPath);

				const auto storeIt = m_stores.find(key);

				if ( storeIt == m_stores.end() )
				{
					return {};
				}

				const auto * array = storeIt->second.getArrayPointer(variableName);

				if ( array == nullptr )
				{
					return {};
				}

				std::vector< variable_t > list;
				list.reserve(array->size());

				for ( const auto & item : *array )
				{
					if constexpr ( std::is_same_v< variable_t, float > )
					{
						if ( item.type() == typeid(double) )
						{
							list.push_back(static_cast< float >(std::any_cast< double >(item)));
						}
					}
					else
					{
						if ( item.type() == typeid(variable_t) )
						{
							list.push_back(std::any_cast< variable_t >(item));
						}
					}
				}

				return list;
			}

			/**
			 * @brief Empties an existing array.
			 * @param settingPath A string view.
			 * @return void
			 */
			void
			clearArray (std::string_view settingPath)
			{
				const std::unique_lock< std::shared_mutex > lock{m_storeAccess};

				const auto & [key, variableName] = Settings::parseAccessKey(settingPath);

				const auto storeIt = m_stores.find(key);

				if ( storeIt == m_stores.end() )
				{
					return;
				}

				storeIt->second.clearArray(variableName);
			}

			/**
			 * @brief Removes a key from settings.
			 * @param settingPath A string view.
			 * @return void
			 */
			void
			removeKey (std::string_view settingPath)
			{
				const std::unique_lock< std::shared_mutex > lock{m_storeAccess};

				const auto & [key, variableName] = Settings::parseAccessKey(settingPath);

				const auto storeIt = m_stores.find(key);

				if ( storeIt == m_stores.end() )
				{
					return;
				}

				storeIt->second.removeKey(std::string{variableName});
			}

			/**
			 * @brief Clears the settings.
			 * @return void
			 */
			void
			clear ()
			{
				const std::unique_lock< std::shared_mutex > lock{m_storeAccess};

				m_stores.clear();
			}

			/**
			 * @brief Saves the settings in the file.
			 * @return bool
			 */
			[[nodiscard]]
			bool save () const noexcept;

		private:

			/** @copydoc EmEn::ServiceInterface::onInitialize() */
			bool onInitialize () noexcept override;

			/** @copydoc EmEn::ServiceInterface::onTerminate() */
			bool onTerminate () noexcept override;

			/**
			 * @brief Parses a raw key to get the store name and the variable.
			 * @param settingPath A string view.
			 * @return std::pair< std::string_view, std::string_view >
			 */
			[[nodiscard]]
			static std::pair< std::string_view, std::string_view > parseAccessKey (std::string_view settingPath) noexcept;

			/**
			 * @brief Returns a variable from settings.
			 * @param settingPath A string view.
			 * @return std::string
			 */
			[[nodiscard]]
			std::any getVariable (std::string_view settingPath) const noexcept;

			/**
			 * @brief Converts std::any to the desired type for Settings::get() and Settings::getOrSetDefault().
			 * @tparam variable_t The type of the variable.
			 * @param value A reference to as std::any.
			 * @param defaultValue A reference to a default value.
			 * @return variable_t
			 */
			template< SettingType variable_t >
			[[nodiscard]]
			static
			variable_t
			convertAnyTo (const std::any & value, const variable_t & defaultValue)
			{
				if constexpr ( std::is_same_v< variable_t, bool > )
				{
					if ( value.type() == typeid(variable_t) )
					{
						return std::any_cast< variable_t >(value);
					}

					if ( value.type() == typeid(int32_t) )
					{
						return std::any_cast< int32_t >(value) > 0;
					}

					if ( value.type() == typeid(uint32_t) )
					{
						return std::any_cast< uint32_t >(value) > 0;
					}

					if ( value.type() == typeid(int64_t) )
					{
						return std::any_cast< int64_t >(value) > 0;
					}

					if ( value.type() == typeid(uint64_t) )
					{
						return std::any_cast< uint64_t >(value) > 0;
					}
				}

				if constexpr ( std::is_same_v< variable_t, int32_t > )
				{
					if ( value.type() == typeid(variable_t) )
					{
						return std::any_cast< variable_t >(value);
					}

					if ( value.type() == typeid(bool) )
					{
						return std::any_cast< bool >(value) ? 1 : 0;
					}

					if ( value.type() == typeid(uint32_t) )
					{
						/* NOTE: Possible loss of precision on high number. */
						return static_cast< variable_t >(std::any_cast< uint32_t >(value));
					}

					if ( value.type() == typeid(int64_t) )
					{
						/* NOTE: Possible loss of precision on high number. */
						return static_cast< variable_t >(std::any_cast< int64_t >(value));
					}

					if ( value.type() == typeid(uint64_t) )
					{
						/* NOTE: Possible loss of precision on high number. */
						return static_cast< variable_t >(std::any_cast< uint64_t >(value));
					}
				}

				if constexpr ( std::is_same_v< variable_t, uint32_t > )
				{
					if ( value.type() == typeid(variable_t) )
					{
						return std::any_cast< variable_t >(value);
					}

					if ( value.type() == typeid(bool) )
					{
						return std::any_cast< bool >(value) ? 1 : 0;
					}

					if ( value.type() == typeid(int32_t) )
					{
						const auto signedValue = std::any_cast< int32_t >(value);

						return signedValue >= 0 ? static_cast< variable_t >(signedValue) : defaultValue;
					}

					if ( value.type() == typeid(int64_t) )
					{
						const auto signedValue = std::any_cast< int64_t >(value);

						/* NOTE: Possible loss of precision on high number. */
						return signedValue >= 0 ? static_cast< variable_t >(signedValue) : defaultValue;
					}

					if ( value.type() == typeid(uint64_t) )
					{
						/* NOTE: Possible loss of precision on high number. */
						return static_cast< variable_t >(std::any_cast< uint64_t >(value));
					}
				}

				if constexpr ( std::is_same_v< variable_t, int64_t > )
				{
					if ( value.type() == typeid(variable_t) )
					{
						return std::any_cast< variable_t >(value);
					}

					if ( value.type() == typeid(bool) )
					{
						return std::any_cast< bool >(value) ? 1 : 0;
					}

					if ( value.type() == typeid(int32_t) )
					{
						return static_cast< variable_t >(std::any_cast< int32_t >(value));
					}

					if ( value.type() == typeid(uint32_t) )
					{
						return static_cast< variable_t >(std::any_cast< uint32_t >(value));
					}

					if ( value.type() == typeid(uint64_t) )
					{
						/* NOTE: Possible loss of precision on high number. */
						return static_cast< variable_t >(std::any_cast< uint64_t >(value));
					}
				}

				if constexpr ( std::is_same_v< variable_t, uint64_t > )
				{
					if ( value.type() == typeid(variable_t) )
					{
						return std::any_cast< variable_t >(value);
					}

					if ( value.type() == typeid(bool) )
					{
						return std::any_cast< bool >(value) ? 1 : 0;
					}

					if ( value.type() == typeid(int32_t) )
					{
						const auto signedValue = std::any_cast< int32_t >(value);

						return signedValue >= 0 ? static_cast< variable_t >(signedValue) : defaultValue;
					}

					if ( value.type() == typeid(uint32_t) )
					{
						return static_cast< variable_t >(std::any_cast< uint32_t >(value));
					}

					if ( value.type() == typeid(int64_t) )
					{
						const auto signedValue = std::any_cast< int64_t >(value);

						return signedValue >= 0 ? static_cast< variable_t >(signedValue) : defaultValue;
					}
				}

				if constexpr ( std::is_same_v< variable_t, float > )
				{
					if ( value.type() == typeid(variable_t) )
					{
						return std::any_cast< variable_t >(value);
					}

					if ( value.type() == typeid(double) )
					{
						/* NOTE: Possible loss of precision on high number. */
						return static_cast< variable_t >(std::any_cast< double >(value));
					}

					if ( value.type() == typeid(bool) )
					{
						return std::any_cast< bool >(value) ? 1 : 0;
					}

					if ( value.type() == typeid(int32_t) )
					{
						return static_cast< variable_t >(std::any_cast< int32_t >(value));
					}

					if ( value.type() == typeid(uint32_t) )
					{
						return static_cast< variable_t >(std::any_cast< uint32_t >(value));
					}

					if ( value.type() == typeid(int64_t) )
					{
						/* NOTE: Possible loss of precision on high number. */
						return static_cast< variable_t >(std::any_cast< int64_t >(value));
					}

					if ( value.type() == typeid(uint64_t) )
					{
						/* NOTE: Possible loss of precision on high number. */
						return static_cast< variable_t >(std::any_cast< uint64_t >(value));
					}
				}

				if constexpr ( std::is_same_v< variable_t, double > )
				{
					if ( value.type() == typeid(variable_t) )
					{
						return std::any_cast< variable_t >(value);
					}

					if ( value.type() == typeid(float) )
					{
						return static_cast< variable_t >(std::any_cast< float >(value));
					}

					if ( value.type() == typeid(bool) )
					{
						return std::any_cast< bool >(value) ? 1 : 0;
					}

					if ( value.type() == typeid(int32_t) )
					{
						return static_cast< variable_t >(std::any_cast< int32_t >(value));
					}

					if ( value.type() == typeid(uint32_t) )
					{
						return static_cast< variable_t >(std::any_cast< uint32_t >(value));
					}

					if ( value.type() == typeid(int64_t) )
					{
						return static_cast< variable_t >(std::any_cast< int64_t >(value));
					}

					if ( value.type() == typeid(uint64_t) )
					{
						return static_cast< variable_t >(std::any_cast< uint64_t >(value));
					}
				}

				if constexpr ( std::is_same_v< variable_t, std::string > )
				{
					if ( value.type() == typeid(variable_t) )
					{
						return std::any_cast< variable_t >(value);
					}
				}

				return defaultValue;
			}

			/**
			 * @brief Reads a sublevel of the settings file.
			 * @param data A reference to the JSON node for the level to read.
			 * @param key A reference to a string.
			 * @return bool
			 */
			[[nodiscard]]
			bool readLevel (const Json::Value & data, const std::string & key) noexcept;

			/**
			 * @brief Reads a settings file.
			 * @param filepath A reference to a path.
			 * @return bool
			 */
			[[nodiscard]]
			bool readFile (const std::filesystem::path & filepath) noexcept;

			/**
			 * @brief Writes a settings file.
			 * @param filepath A reference to a path.
			 * @return bool
			 */
			[[nodiscard]]
			bool writeFile (const std::filesystem::path & filepath) const noexcept;

			/**
			 * @brief STL streams printable object.
			 * @param out A reference to the stream output.
			 * @param obj A reference to the object to print.
			 * @return std::ostream &
			 */
			friend std::ostream & operator<< (std::ostream & out, const Settings & obj);

			const Arguments & m_arguments;
			const FileSystem & m_fileSystem;
			std::map< std::string, SettingStore, std::less<> > m_stores;
			std::filesystem::path m_filepath;
			mutable std::shared_mutex m_storeAccess;
			bool m_childProcess{false};
			bool m_showInformation{false};
			bool m_saveAtExit{false};
	};

	/**
	 * @brief Stringifies the object.
	 * @param obj A reference to the object to print.
	 * @return std::string
	 */
	inline
	std::string
	to_string (const Settings & obj) noexcept
	{
		std::stringstream output;

		output << obj;

		return output.str();
	}
}
