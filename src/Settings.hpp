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
#include <cstddef>
#include <array>
#include <filesystem>

/* Local inclusion for inheritances. */
#include "ServiceInterface.hpp"

/* Local inclusion for usages. */
#include "SettingStore.hpp"
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
			 * @param fileSystem A reference to file system.
			 * @param childProcess Declares a child process.
			 */
			Settings (const Arguments & arguments, const FileSystem & fileSystem, bool childProcess) noexcept
				: ServiceInterface{ClassId},
				m_arguments{arguments},
				m_fileSystem{fileSystem}
			{
				if ( s_instance != nullptr )
				{
					std::cerr << __PRETTY_FUNCTION__ << ", constructor called twice !" "\n";

					std::terminate();
				}

				s_instance = this;

				m_flags[ChildProcess] = childProcess;

				if ( !m_flags[ChildProcess] )
				{
					m_flags[SaveAtExit] = true;
				}
			}

			/**
			 * @brief Destructs the settings manager.
			 */
			~Settings () override
			{
				s_instance = nullptr;
			}

			/** @copydoc EmEn::ServiceInterface::usable() */
			[[nodiscard]]
			bool
			usable () const noexcept override
			{
				if ( !m_flags[ServiceInitialized] )
				{
					return false;
				}

				return !m_filepath.empty();
			}

			/**
			 * @brief Returns the file path for these settings.
			 * @return const std::filesystem::path &
			 */
			[[nodiscard]]
			const std::filesystem::path &
			filepath () noexcept
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
				m_flags[SaveAtExit] = state;
			}

			/**
			 * @brief Returns whether the settings will be saved at application shutdown.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isSaveAtExitEnabled () const noexcept
			{
				return m_flags[SaveAtExit];
			}

			/**
			 * @brief Returns whether the service is from a child process.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isChildProcess () const noexcept
			{
				return m_flags[ChildProcess];
			}

			/**
			 * @brief Checks the present of a variable in the settings.
			 * @param key The key in settings to check.
			 * @return bool
			 */
			[[nodiscard]]
			bool variableExists (const std::string & key) const noexcept;

			/**
			 * @brief Checks the present of an array in the settings.
			 * @param key The key in settings to check.
			 * @return bool
			 */
			[[nodiscard]]
			bool arrayExists (const std::string & key) const noexcept;

			/**
			 * @brief Checks the present of a store in the settings.
			 * @param key The store name in settings to check.
			 * @return bool
			 */
			[[nodiscard]]
			bool storeExists (const std::string & key) const noexcept;

			/**
			 * @brief Returns a single variable from settings.
			 * @tparam variable_t The variable type.
			 * @param key A reference to a string for the variable name.
			 * @param defaultValue The default value. Default none.
			 * @return variable_t
			 */
			template< typename variable_t >
			[[nodiscard]]
			variable_t
			get (const std::string & key, const variable_t & defaultValue = {}) const noexcept requires ( std::is_same_v< variable_t, bool > || std::is_same_v< variable_t, int32_t > || std::is_same_v< variable_t, uint32_t > || std::is_same_v< variable_t, int64_t > || std::is_same_v< variable_t, uint64_t > || std::is_same_v< variable_t, float > || std::is_same_v< variable_t, double > || std::is_same_v< variable_t, std::string > )
			{
				const auto value = this->get(key);

				if ( value.has_value() )
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
				}

				if ( !m_flags[ChildProcess] )
				{
					TraceWarning{ClassId} << "The key '" << key << "' doesn't exists in the settings file and won't be saved because access is read-only.";
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
			template< typename variable_t >
			[[nodiscard]]
			variable_t
			get (const std::string & key, const variable_t & defaultValue = {}) noexcept requires ( std::is_same_v< variable_t, bool > || std::is_same_v< variable_t, int32_t > || std::is_same_v< variable_t, uint32_t > || std::is_same_v< variable_t, int64_t > || std::is_same_v< variable_t, uint64_t > || std::is_same_v< variable_t, float > || std::is_same_v< variable_t, double > || std::is_same_v< variable_t, std::string > )
			{
				const auto value = this->get(key);

				if ( value.has_value() )
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
				}

				this->set(key, defaultValue);

				return defaultValue;
			}

			/**
			 * @brief Saves the settings in the file.
			 * @return bool
			 */
			[[nodiscard]]
			bool save () const noexcept;

			/**
			 * @brief Clears the settings.
			 * @return void
			 */
			void
			clear () noexcept
			{
				m_store.clear();
			}

			/**
			 * @brief Sets a value in the store.
			 * @tparam variable_t The type of the value.
			 * @param key A reference to a string for the variable name.
			 * @param value A reference to a value to save.
			 * @return void
			 */
			template< typename variable_t >
			void
			set (const std::string & key, const variable_t & value) noexcept requires ( std::is_same_v< variable_t, bool > || std::is_same_v< variable_t, int32_t > || std::is_same_v< variable_t, uint32_t > || std::is_same_v< variable_t, int64_t > || std::is_same_v< variable_t, uint64_t > || std::is_same_v< variable_t, float > || std::is_same_v< variable_t, double > || std::is_same_v< variable_t, std::string > )
			{
				std::string name{};

				this->parseKey(key, name)->setVariable(name, value);
			}

			/** @copydoc EmEn::Settings::set(const std::string & key, const variable_t & value) noexcept */
			void
			set (const std::string & key, const char * value) noexcept
			{
				this->set(key, std::string{value});
			}

			/**
			 * @brief Sets a value in an array of the store.
			 * @tparam variable_t The type of the value.
			 * @param key A reference to a string for the array name.
			 * @param value A reference to a value to save.
			 * @return void
			 */
			template< typename variable_t >
			void
			setInArray (const std::string & key, const variable_t & value) noexcept requires ( std::is_same_v< variable_t, bool > || std::is_same_v< variable_t, int32_t > || std::is_same_v< variable_t, uint32_t > || std::is_same_v< variable_t, int64_t > || std::is_same_v< variable_t, uint64_t > || std::is_same_v< variable_t, float > || std::is_same_v< variable_t, double > || std::is_same_v< variable_t, std::string > )
			{
				std::string name{};

				this->parseKey(key, name)->setVariableInArray(name, value);
			}

			/** @copydoc EmEn::Settings::setInArray(const std::string & key, const variable_t & value) noexcept */
			void
			setInArray (const std::string & key, const char * value) noexcept
			{
				this->setInArray(key, std::string{value});
			}

			/**
			 * @brief Returns a vector of typed data.
			 * @note If one or more item of the array do not fit the desired type, it will be ignored.
			 * @tparam variable_t The type of the value.
			 * @param key A reference to a string for the variable name.
			 * @return std::vector< std::string >
			 */
			template< typename variable_t >
			[[nodiscard]]
			std::vector< std::string >
			getArrayAs (const std::string & key) const noexcept requires ( std::is_same_v< variable_t, bool > || std::is_same_v< variable_t, int32_t > || std::is_same_v< variable_t, uint32_t > || std::is_same_v< variable_t, int64_t > || std::is_same_v< variable_t, uint64_t > || std::is_same_v< variable_t, float > || std::is_same_v< variable_t, double > || std::is_same_v< variable_t, std::string > )
			{
				std::string name;

				const auto * store = this->parseKey(key, name);

				/* NOTE : Check if the variable is actually an array. */
				if ( store == nullptr || !store->arrayExists(name) )
				{
					return {};
				}

				const auto * array = store->getArrayPointer(name);

				std::vector< variable_t > list;
				list.reserve(array->size());

				for ( const auto & item : *array )
				{
					if constexpr ( std::is_same_v< variable_t, float > )
					{
						if ( item.type() == typeid(double) )
						{
							list.emplace_back(static_cast< float >(std::any_cast< double >(item)));
						}
					}
					else
					{
						if ( item.type() == typeid(variable_t) )
						{
							list.emplace_back(std::any_cast< variable_t >(item));
						}
					}
				}

				return list;
			}

			/**
			 * @brief Returns whether a variable as an array is empty.
			 * @param key A reference to a string for the variable name.
			 * @return bool
			 */
			[[nodiscard]]
			bool isArrayEmpty (const std::string & key) const noexcept;

			/**
			 * @brief Returns the instance of the settings manager.
			 * @return Settings *
			 */
			[[nodiscard]]
			static
			Settings *
			instance () noexcept
			{
				return s_instance;
			}

		private:

			/** @copydoc EmEn::ServiceInterface::onInitialize() */
			bool onInitialize () noexcept override;

			/** @copydoc EmEn::ServiceInterface::onTerminate() */
			bool onTerminate () noexcept override;

			/**
			 * @brief Returns a variable from settings.
			 * @param key The name of the configuration key.
			 * @return std::string
			 */
			[[nodiscard]]
			std::any get (const std::string & key) const noexcept;

			/**
			 * @brief Reads a sublevel of the settings file.
			 * @param data A reference to the JSON node for the level to read.
			 * @param store A reference to the setting store.
			 * @return bool
			 */
			[[nodiscard]]
			bool readLevel (const Json::Value & data, SettingStore & store) noexcept;

			/**
			 * @brief Reads a settings file.
			 * @param filepath A reference to a path.
			 * @return bool
			 */
			[[nodiscard]]
			bool readFile (const std::filesystem::path & filepath) noexcept;

			/**
			 * @brief Writes a sublevel to a settings file.
			 * @param store A reference to the current store.
			 * @param data A reference to the JSON node being written.
			 * @return bool
			 */
			[[nodiscard]]
			bool writeLevel (const SettingStore & store, Json::Value & data) const noexcept;

			/**
			 * @brief Writes a settings file.
			 * @param filepath A reference to a path.
			 * @return bool
			 */
			[[nodiscard]]
			bool writeFile (const std::filesystem::path & filepath) const noexcept;

			/**
			 * @brief Parse a JSON path key a returns the store and the last part of the path.
			 * @note This method will return nullptr, if the store is not found or the last
			 * part is not part of variables from the store or a store itself.
			 * @param key A reference to a string for the JSON path key.
			 * @param variableName A reference to the string for the variable name.
			 * @return const SettingStore *
			 */
			[[nodiscard]]
			const SettingStore * parseKey (const std::string & key, std::string & variableName) const noexcept;

			/**
			 * @brief Parse a JSON path key a returns the store and the last part of the path.
			 * @note This version will build the path.
			 * @param key A reference to a string for the JSON path key.
			 * @param variableName A reference to the string for the variable name.
			 * @return SettingStore *
			 */
			[[nodiscard]]
			SettingStore * parseKey (const std::string & key, std::string & variableName) noexcept;

			/**
			 * @brief STL streams printable object.
			 * @param out A reference to the stream output.
			 * @param obj A reference to the object to print.
			 * @return std::ostream &
			 */
			friend std::ostream & operator<< (std::ostream & out, const Settings & obj);

			static Settings * s_instance;

			/* Flag names. */
			static constexpr auto ServiceInitialized{0UL};
			static constexpr auto ChildProcess{1UL};
			static constexpr auto ShowInformation{2UL};
			static constexpr auto SaveAtExit{3UL};

			const Arguments & m_arguments;
			const FileSystem & m_fileSystem;
			SettingStore m_store;
			std::filesystem::path m_filepath;
			std::array< bool, 8 > m_flags{
				false/*ServiceInitialized*/,
				false/*ChildProcess*/,
				false/*ShowInformation*/,
				false/*SaveAtExit*/,
				false/*UNUSED*/,
				false/*UNUSED*/,
				false/*UNUSED*/,
				false/*UNUSED*/
			};
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
