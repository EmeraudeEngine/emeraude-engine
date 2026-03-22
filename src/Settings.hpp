/*
 * src/Settings.hpp
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
 * https://github.com/londnoir/emeraude-engine
 *
 * --- THIS IS AUTOMATICALLY GENERATED, DO NOT CHANGE ---
 */

#pragma once

/* Application configuration. */
#include "emeraude_config.hpp"

/* STL inclusions. */
#include <cstdint>
#include <map>
#include <vector>
#include <string>
#include <optional>
#include <variant>
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
						  std::is_same_v< variable_t, int16_t > ||
						  std::is_same_v< variable_t, uint16_t > ||
						  std::is_same_v< variable_t, int32_t > ||
						  std::is_same_v< variable_t, uint32_t > ||
						  std::is_same_v< variable_t, int64_t > ||
						  std::is_same_v< variable_t, uint64_t > ||
						  std::is_same_v< variable_t, float > ||
						  std::is_same_v< variable_t, double > ||
						  std::is_same_v< variable_t, std::string >;

	/** @brief Type-safe variant holding any setting value. */
	using SettingValue = std::variant< bool, int16_t, uint16_t, int32_t, uint32_t, int64_t, uint64_t, float, double, std::string >;

	/**
	 * @class SettingStore
	 * @brief Holds all settings entries for a single named scope (store).
	 *
	 * A store represents one hierarchical level of the settings file, identified by a
	 * slash-delimited path key (e.g. "Graphics/Vulkan"). It maintains two independent
	 * maps: one for scalar values and one for ordered arrays of values. Both maps use
	 * heterogeneous lookup (@c std::less<>) so that @c std::string_view keys can be
	 * searched without constructing a temporary @c std::string.
	 *
	 * This class is a private implementation detail of @ref EmEn::Settings and must
	 * never be instantiated or referenced directly by engine or application code.
	 * All interaction must go through the @ref EmEn::Settings API, which enforces
	 * thread-safety via @c std::shared_mutex.
	 *
	 * @note Only @ref EmEn::Settings is granted friend access to this class.
	 * @todo Simplify internal storage by merging @c m_variables and @c m_arrays into a
	 *       single @c std::map<std::string, SettingValue, std::less<>> and reworking
	 *       the accessor layer accordingly.
	 * @see EmEn::Settings
	 * @version 0.8.61
	 */
	class SettingStore final
	{
		friend class Settings;

		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"SettingStore"};

			/**
			 * @brief Constructs an empty setting store.
			 */
			SettingStore () noexcept = default;

			/**
			 * @brief Returns a read-only reference to the scalar variables map.
			 *
			 * The returned map associates variable names to their @ref SettingValue.
			 * The caller must not hold this reference across a write operation on the
			 * owning @ref EmEn::Settings instance.
			 *
			 * @return A const reference to the internal scalar variables map.
			 */
			[[nodiscard]]
			const std::map< std::string, SettingValue, std::less<> > &
			variables () const noexcept
			{
				return m_variables;
			}

			/**
			 * @brief Returns a read-only reference to the array variables map.
			 *
			 * Each entry in the returned map associates a variable name with an ordered
			 * sequence of @ref SettingValue elements, as read from a JSON array node.
			 * The caller must not hold this reference across a write operation on the
			 * owning @ref EmEn::Settings instance.
			 *
			 * @return A const reference to the internal array variables map.
			 */
			[[nodiscard]]
			const std::map< std::string, std::vector< SettingValue >, std::less<> > &
			arrays () const noexcept
			{
				return m_arrays;
			}

			/**
			 * @brief Inserts or replaces a scalar variable in this store.
			 *
			 * If a variable with the same @p name already exists, its value is
			 * overwritten. The operation does not affect the array map.
			 *
			 * @param name The variable name used as the map key.
			 * @param value The @ref SettingValue to store.
			 */
			void
			setVariable (const std::string & name, const SettingValue & value)
			{
				m_variables[name] = value;
			}

			/**
			 * @brief Appends a value to the array associated with @p name.
			 *
			 * If no array entry for @p name exists it is created implicitly. Values
			 * are appended in insertion order, preserving the original JSON array
			 * sequence.
			 *
			 * @param name The array variable name used as the map key.
			 * @param value The @ref SettingValue to append.
			 */
			void
			setVariableInArray (const std::string & name, const SettingValue & value)
			{
				m_arrays[name].emplace_back(value);
			}

			/**
			 * @brief Removes all elements from the array identified by @p variableName.
			 *
			 * If no array with the given name exists the call is a no-op.
			 * The array entry itself remains in the map with an empty vector.
			 *
			 * @param variableName The name of the array variable to clear.
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
			 * @brief Removes a variable from both the scalar and array maps.
			 *
			 * If @p name is present in neither map the call is a no-op. This method
			 * removes from both maps simultaneously so callers need not know which
			 * storage a key belongs to.
			 *
			 * @param name The variable name to erase.
			 */
			void
			removeKey (const std::string & name) noexcept
			{
				m_variables.erase(name);
				m_arrays.erase(name);
			}

			/**
			 * @brief Returns @c true when both the scalar and array maps are empty.
			 * @return @c true if the store contains no variables and no arrays, @c false otherwise.
			 */
			[[nodiscard]]
			bool
			empty () const noexcept
			{
				return m_variables.empty() && m_arrays.empty();
			}

			/**
			 * @brief Returns @c true when a scalar variable with the given name exists.
			 * @param variableName The variable name to look up.
			 * @return @c true if the scalar map contains @p variableName, @c false otherwise.
			 */
			[[nodiscard]]
			bool
			variableExists (std::string_view variableName) const noexcept
			{
				return m_variables.contains(variableName);
			}

			/**
			 * @brief Returns @c true when an array variable with the given name exists.
			 * @param variableName The array variable name to look up.
			 * @return @c true if the array map contains @p variableName, @c false otherwise.
			 */
			[[nodiscard]]
			bool
			arrayExists (std::string_view variableName) const noexcept
			{
				return m_arrays.contains(variableName);
			}

			/**
			 * @brief Returns a pointer to the scalar value stored under @p variableName.
			 *
			 * Returns a raw pointer into the internal map so that the caller can
			 * inspect the active type via @c std::get_if without copying the variant.
			 * The pointer is invalidated by any subsequent write to this store.
			 *
			 * @param variableName The variable name to look up.
			 * @return A non-owning pointer to the matching @ref SettingValue, or
			 *         @c nullptr if no such variable exists.
			 */
			[[nodiscard]]
			const SettingValue *
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
			 * @brief Returns a pointer to the value vector stored under @p variableName.
			 *
			 * Returns a raw pointer into the internal map so that the caller can
			 * iterate the array without copying. The pointer is invalidated by any
			 * subsequent write to this store.
			 *
			 * @param variableName The array variable name to look up.
			 * @return A non-owning pointer to the matching value vector, or @c nullptr
			 *         if no such array variable exists.
			 */
			[[nodiscard]]
			const std::vector< SettingValue > *
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
			 * @brief Removes all scalar variables and all array variables from this store.
			 */
			void
			clear () noexcept
			{
				m_variables.clear();
				m_arrays.clear();
			}

		private:

			/* TODO: Simplify by using only "std::map< std::string, SettingValue, std::less<> > m_data;" and rework accessors. */
			std::map< std::string, SettingValue, std::less<> > m_variables;
			std::map< std::string, std::vector< SettingValue >, std::less<> > m_arrays;
	};

	/**
	 * @class Settings
	 * @brief Engine-wide persistent settings service backed by a JSON file.
	 *
	 * Settings organizes all configuration values in a two-level namespace:
	 * a *store key* (the slash-delimited path prefix, e.g. @c "Graphics/Vulkan")
	 * and a *variable name* (the last path component after the final @c '/').
	 * Each store key maps to a @ref SettingStore that holds scalar and array
	 * values as @ref SettingValue variants.
	 *
	 * The service is initialized by @ref ServiceInterface::initialize(). During
	 * initialization the JSON file is located (using command-line overrides
	 * @c --settings-filepath or @c --settings-filename, or the default
	 * @ref Filename in the application config directory) and parsed into memory.
	 * On successful termination the in-memory state is written back to disk
	 * unless @c --disable-settings-autosave or @c --reset-settings are present
	 * on the command line, or the instance was constructed as a child process.
	 *
	 * **Thread safety:** All public read operations acquire a shared lock on
	 * @c m_storeAccess. All write operations acquire a unique (exclusive) lock.
	 * It is safe to call @ref get() from multiple threads concurrently.
	 *
	 * **Type coercion:** @ref get() and @ref getOrSetDefault() delegate to the
	 * internal @ref convertValueTo() helper, which uses @c std::get_if (never
	 * @c std::get) for cross-type conversion. No C++ exceptions are thrown even
	 * when the stored type differs from the requested one; the @p defaultValue is
	 * returned instead. Numeric narrowing (e.g. @c uint64_t to @c int32_t on
	 * large values) is documented per-overload.
	 *
	 * **Setting paths:** Use forward-slash-separated strings such as
	 * @c "Audio/OpenAL/bufferSize". A path with no @c '/' resolves to the root
	 * store with an empty key.
	 *
	 * @note Child-process instances (constructed with @p childProcess @c = @c true)
	 *       are read-only proxies. They do not enable auto-save and do not write
	 *       the file on termination.
	 * @see EmEn::SettingStore, EmEn::SettingValue, EmEn::SettingType
	 * @version 0.8.61
	 */
	class Settings final : public ServiceInterface
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"SettingsService"};

			/** @brief Default filename for the settings JSON file on disk. */
			static constexpr auto Filename{"settings.json"};

			/** @brief JSON key that records the engine version that wrote the file. */
			static constexpr auto VersionKey{"WrittenByAppVersion"};

			/** @brief JSON key that records the calendar date the file was written. */
			static constexpr auto DateKey{"WrittenAtDate"};

			/**
			 * @brief Constructs the settings service and binds it to the application services.
			 *
			 * The constructor only stores references and sets the @p childProcess flag;
			 * no file I/O is performed here. Call @ref ServiceInterface::initialize() to
			 * load the settings file. When @p childProcess is @c false the auto-save flag
			 * is enabled immediately so that settings are persisted on normal exit.
			 *
			 * @param arguments A reference to the parsed command-line arguments, used to
			 *        resolve the settings file path and control verbosity / autosave.
			 * @param fileSystem A reference to the engine file-system service, used to
			 *        resolve the default settings file path inside the config directory.
			 * @param childProcess Pass @c true when this instance is a read-only proxy
			 *        running inside a child process (e.g. the CEF browser process). Child
			 *        instances never write the file on termination.
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
			 * @brief Destroys the settings service.
			 *
			 * File persistence is handled inside @ref ServiceInterface::terminate(), not
			 * in the destructor. Ensure @ref ServiceInterface::terminate() is called before
			 * the object is destroyed to guarantee the settings file is written.
			 */
			~Settings () override = default;

			/**
			 * @brief Returns the resolved path of the settings JSON file.
			 *
			 * The path is populated during @ref ServiceInterface::initialize().
			 * Before initialization or when no valid path could be resolved the
			 * returned path is empty.
			 *
			 * @return A const reference to the settings file path.
			 */
			[[nodiscard]]
			const std::filesystem::path &
			filepath () const noexcept
			{
				return m_filepath;
			}

			/**
			 * @brief Enables or disables automatic file persistence at application shutdown.
			 *
			 * When @p state is @c true the file will be written during
			 * @ref ServiceInterface::terminate() unless the @c --reset-settings flag is
			 * present on the command line. Child-process instances ignore this flag.
			 *
			 * @param state @c true to enable auto-save, @c false to disable it.
			 */
			void
			saveAtExit (bool state) noexcept
			{
				m_saveAtExit = state;
			}

			/**
			 * @brief Returns @c true when the settings file will be saved on application shutdown.
			 * @return @c true if auto-save is enabled, @c false otherwise.
			 */
			[[nodiscard]]
			bool
			isSaveAtExitEnabled () const noexcept
			{
				return m_saveAtExit;
			}

			/**
			 * @brief Returns @c true when this instance was constructed as a child-process proxy.
			 *
			 * Child-process instances never write the settings file on termination and
			 * do not process verbose or autosave command-line flags.
			 *
			 * @return @c true if this is a child-process settings instance, @c false otherwise.
			 */
			[[nodiscard]]
			bool
			isChildProcess () const noexcept
			{
				return m_childProcess;
			}

			/**
			 * @brief Stores a typed scalar value at the given settings path.
			 *
			 * The path is split on the last @c '/' to derive the store key and the
			 * variable name. The store is created implicitly if it does not yet exist.
			 * An existing entry with the same path is silently overwritten.
			 *
			 * This method acquires an exclusive lock on the internal mutex and is
			 * therefore safe to call from any thread.
			 *
			 * @tparam variable_t Any type satisfying the @ref SettingType concept.
			 * @param settingPath Slash-delimited path, e.g. @c "Graphics/Vulkan/msaa".
			 * @param value The value to store.
			 */
			template< SettingType variable_t >
			void
			set (std::string_view settingPath, const variable_t & value)
			{
				const std::unique_lock< std::shared_mutex > lock{m_storeAccess};

				const auto & [key, variableName] = Settings::parseAccessKey(settingPath);

				m_stores[std::string{key}].setVariable(std::string{variableName}, value);
			}

			/**
			 * @brief Stores a C-string value at the given settings path.
			 *
			 * Convenience overload that converts @p value to @c std::string and
			 * delegates to the templated @ref set() overload.
			 *
			 * @param settingPath Slash-delimited path, e.g. @c "App/locale".
			 * @param value Null-terminated C-string to store.
			 */
			void
			set (std::string_view settingPath, const char * value)
			{
				this->set(settingPath, std::string{value});
			}

			/**
			 * @brief Appends a typed value to the array at the given settings path.
			 *
			 * The path is split on the last @c '/' to derive the store key and the
			 * array variable name. The store and the array are created implicitly if
			 * they do not yet exist. Values are appended in call order.
			 *
			 * This method acquires an exclusive lock on the internal mutex and is
			 * therefore safe to call from any thread.
			 *
			 * @tparam variable_t Any type satisfying the @ref SettingType concept.
			 * @param settingPath Slash-delimited path, e.g. @c "Audio/OpenAL/extensions".
			 * @param value The value to append to the array.
			 */
			template< SettingType variable_t >
			void
			setInArray (std::string_view settingPath, const variable_t & value)
			{
				const std::unique_lock< std::shared_mutex > lock{m_storeAccess};

				const auto & [key, variableName] = Settings::parseAccessKey(settingPath);

				m_stores[std::string{key}].setVariableInArray(std::string{variableName}, value);
			}

			/**
			 * @brief Appends a C-string value to the array at the given settings path.
			 *
			 * Convenience overload that converts @p value to @c std::string and
			 * delegates to the templated @ref setInArray() overload.
			 *
			 * @param settingPath Slash-delimited path, e.g. @c "Audio/OpenAL/extensions".
			 * @param value Null-terminated C-string to append.
			 */
			void
			setInArray (std::string_view settingPath, const char * value)
			{
				this->setInArray(settingPath, std::string{value});
			}

			/**
			 * @brief Returns @c true when a scalar variable exists at the given path.
			 *
			 * Acquires a shared lock; safe to call concurrently with other read operations.
			 *
			 * @param settingPath Slash-delimited path to the variable.
			 * @return @c true if a scalar entry exists at @p settingPath, @c false otherwise.
			 */
			[[nodiscard]]
			bool variableExists (std::string_view settingPath) const noexcept;

			/**
			 * @brief Returns @c true when an array variable exists at the given path.
			 *
			 * An array is considered to exist even if it is empty (i.e. it was registered
			 * but never populated, or was cleared via @ref clearArray()).
			 * Acquires a shared lock; safe to call concurrently with other read operations.
			 *
			 * @param settingPath Slash-delimited path to the array variable.
			 * @return @c true if an array entry exists at @p settingPath, @c false otherwise.
			 */
			[[nodiscard]]
			bool arrayExists (std::string_view settingPath) const noexcept;

			/**
			 * @brief Returns @c true when an array variable is absent or contains no elements.
			 *
			 * Returns @c true both when the store does not exist and when the array entry
			 * exists but has been cleared. Acquires a shared lock; safe to call concurrently
			 * with other read operations.
			 *
			 * @param settingPath Slash-delimited path to the array variable.
			 * @return @c true if the array is absent or empty, @c false if it has at least one element.
			 */
			[[nodiscard]]
			bool isArrayEmpty (std::string_view settingPath) const noexcept;

			/**
			 * @brief Returns the value stored at @p key, coerced to @p variable_t.
			 *
			 * If no value is found at @p key, or if the stored type cannot be coerced
			 * to @p variable_t (see @ref convertValueTo()), @p defaultValue is returned.
			 * The call never writes to the store and never throws. Acquires a shared lock.
			 *
			 * Cross-type coercion rules (applied by @ref convertValueTo()):
			 * - Integer types are mutually convertible; signed-to-unsigned conversion
			 *   returns @p defaultValue when the stored value is negative.
			 * - @c float and @c double are mutually convertible; narrowing precision loss
			 *   is silent.
			 * - Boolean values coerce to numeric @c 1 / @c 0.
			 * - @c std::string is returned only when the stored type is exactly @c std::string.
			 *
			 * @tparam variable_t Any type satisfying the @ref SettingType concept.
			 * @param key Slash-delimited path, e.g. @c "Graphics/Vulkan/msaa".
			 * @param defaultValue Value returned when the setting is absent or incompatible.
			 * @return The stored value coerced to @p variable_t, or @p defaultValue.
			 */
			template< SettingType variable_t >
			[[nodiscard]]
			variable_t
			get (const std::string & key, const variable_t & defaultValue = {}) const noexcept
			{
				const std::shared_lock< std::shared_mutex > lock{m_storeAccess};

				if ( const auto value = this->getVariable(key) )
				{
					return Settings::convertValueTo< variable_t >(*value, defaultValue);
				}

				return defaultValue;
			}

			/**
			 * @brief Returns the stored value at @p key, registering @p defaultValue if absent.
			 *
			 * Unlike @ref get(), this method writes @p defaultValue into the store when
			 * the key does not exist, ensuring that subsequent calls to @ref get() (or
			 * file serialization) include the key. The store is created implicitly if
			 * needed. An exclusive lock is held for the entire read-or-write operation.
			 *
			 * @tparam variable_t Any type satisfying the @ref SettingType concept.
			 * @param key Slash-delimited path, e.g. @c "Audio/OpenAL/bufferSize".
			 * @param defaultValue Value to store and return when the key is absent.
			 * @return The stored value coerced to @p variable_t, or @p defaultValue if
			 *         the key was absent (in which case @p defaultValue is now stored).
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
					return Settings::convertValueTo< variable_t >(*valuePtr, defaultValue);
				}

				store.setVariable(std::string{variableName}, defaultValue);

				return defaultValue;
			}

			/**
			 * @brief Returns all elements of a settings array coerced to @p variable_t.
			 *
			 * Iterates over every @ref SettingValue in the stored array and attempts to
			 * extract a value of type @p variable_t using @c std::get_if. Elements whose
			 * active type does not match @p variable_t are silently skipped, except for
			 * one special case: when @p variable_t is @c float, elements stored as
			 * @c double are narrowed and included.
			 *
			 * Returns an empty vector when the store or the array does not exist.
			 * Acquires a shared lock; safe to call concurrently with other reads.
			 *
			 * @tparam variable_t Any type satisfying the @ref SettingType concept.
			 * @param settingPath Slash-delimited path to the array variable.
			 * @return A vector of @p variable_t values; incompatible elements are omitted.
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
					if ( const auto * p = std::get_if< variable_t >(&item) )
					{
						list.push_back(*p);
					}
					else if constexpr ( std::is_same_v< variable_t, float > )
					{
						if ( const auto * d = std::get_if< double >(&item) )
						{
							list.push_back(static_cast< float >(*d));
						}
					}
				}

				return list;
			}

			/**
			 * @brief Removes all elements from the array at the given settings path.
			 *
			 * The array entry is preserved in the store (it will still be serialized as
			 * an empty JSON array). If the store or the array does not exist the call is
			 * a no-op. Acquires an exclusive lock.
			 *
			 * @param settingPath Slash-delimited path to the array variable to clear.
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
			 * @brief Removes a setting entry (scalar or array) at the given path.
			 *
			 * Erases the variable name from both the scalar and array maps of the
			 * resolved store. If the store or the variable does not exist the call is
			 * a no-op. Acquires an exclusive lock.
			 *
			 * @param settingPath Slash-delimited path to the variable to remove.
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
			 * @brief Removes all stores and all their contents from memory.
			 *
			 * After this call @ref variableExists() and @ref arrayExists() return
			 * @c false for every path. The on-disk file is not affected until the next
			 * call to @ref save() or the normal auto-save on shutdown.
			 * Acquires an exclusive lock.
			 */
			void
			clear ()
			{
				const std::unique_lock< std::shared_mutex > lock{m_storeAccess};

				m_stores.clear();
			}

			/**
			 * @brief Writes the current in-memory settings to the file immediately.
			 *
			 * This is a manual save; it bypasses the @ref isSaveAtExitEnabled() flag.
			 * The output file path is the one resolved during @ref ServiceInterface::initialize().
			 * Acquires a shared lock (read-only pass over the stores).
			 *
			 * @return @c true on success, @c false when no file path is set or if the
			 *         write operation fails.
			 */
			[[nodiscard]]
			bool save () const noexcept;

			/**
			 * @brief Serializes the current in-memory settings to a compact JSON string.
			 *
			 * Produces the same logical content as @ref Settings::save() but writes to a
			 * @c std::string rather than a file. The JSON is formatted without
			 * pretty-printing (no indentation) for transport or logging purposes.
			 * Acquires a shared lock.
			 *
			 * @return A JSON string representing all stores and their variables,
			 *         or an empty string if the JsonCpp writer fails.
			 */
			[[nodiscard]]
			std::string toJsonString () const noexcept;

		private:

			/** @copydoc EmEn::ServiceInterface::onInitialize() */
			bool onInitialize () noexcept override;

			/** @copydoc EmEn::ServiceInterface::onTerminate() */
			bool onTerminate () noexcept override;

			/**
			 * @brief Splits a slash-delimited settings path into a store key and a variable name.
			 *
			 * Splits at the last @c '/' in @p settingPath. Everything before the final
			 * slash becomes the store key (possibly itself slash-delimited for deeply
			 * nested paths); everything after it becomes the variable name. When no
			 * @c '/' is present the store key is an empty view and the full input is
			 * treated as the variable name (root store).
			 *
			 * @param settingPath The raw path to parse, e.g. @c "Graphics/Vulkan/msaa".
			 * @return A pair of @c std::string_view where @c first is the store key and
			 *         @c second is the variable name. Both views reference the storage of
			 *         @p settingPath and must not outlive it.
			 */
			[[nodiscard]]
			static std::pair< std::string_view, std::string_view > parseAccessKey (std::string_view settingPath) noexcept;

			/**
			 * @brief Looks up a scalar variable across the store map.
			 *
			 * Splits @p settingPath via @ref parseAccessKey(), locates the store, and
			 * returns a copy of the @ref SettingValue if found. Returns @c std::nullopt
			 * when either the store or the variable does not exist.
			 *
			 * @note This method does not acquire any lock; callers must hold an
			 *       appropriate lock on @c m_storeAccess before calling.
			 *
			 * @param settingPath Slash-delimited path to the variable.
			 * @return The stored value wrapped in @c std::optional, or @c std::nullopt.
			 */
			[[nodiscard]]
			std::optional< SettingValue > getVariable (std::string_view settingPath) const noexcept;

			/**
			 * @brief Converts a @ref SettingValue variant to a @c Json::Value for serialization.
			 *
			 * Uses @c std::visit to dispatch on the active type of @p value. Each
			 * alternative is mapped to its natural JsonCpp representation. 64-bit integer
			 * types are cast to @c Json::Int64 / @c Json::UInt64 explicitly to avoid
			 * ambiguity in the JsonCpp API.
			 *
			 * This static helper consolidates the two formerly duplicated serialization
			 * lambdas that existed in @ref writeFile() and @ref toJsonString().
			 *
			 * @param value The @ref SettingValue to convert.
			 * @return A @c Json::Value holding the same data in a form suitable for writing.
			 */
			[[nodiscard]]
			static Json::Value settingValueToJson (const SettingValue & value) noexcept;

			/**
			 * @brief Extracts and coerces a @ref SettingValue to the requested type.
			 *
			 * Uses @c std::get_if exclusively (never @c std::get) to stay compatible
			 * with the @c -fno-exceptions build flag. Each target type @p variable_t
			 * tries an exact-type match first, then applies widening or narrowing rules:
			 *
			 * - **bool**: Accepts any integer type; @c true when the value is @c > 0.
			 * - **int32_t / int64_t**: Accept bool, and wider or co-signed integers with
			 *   possible silent truncation on large values.
			 * - **uint32_t / uint64_t**: Accept bool and positive signed integers;
			 *   negative signed values return @p defaultValue.
			 * - **float**: Accepts @c double with narrowing; accepts integer types.
			 * - **double**: Accepts @c float (widening); accepts integer types.
			 * - **std::string**: No coercion — only matched when the active type is
			 *   exactly @c std::string.
			 *
			 * @tparam variable_t Any type satisfying the @ref SettingType concept.
			 * @param value The @ref SettingValue variant to extract from.
			 * @param defaultValue Returned when no coercion rule produces a valid result.
			 * @return The coerced value, or @p defaultValue if no rule applies.
			 */
			template< SettingType variable_t >
			[[nodiscard]]
			static
			variable_t
			convertValueTo (const SettingValue & value, const variable_t & defaultValue)
			{
				if constexpr ( std::is_same_v< variable_t, bool > )
				{
					if ( const auto * p = std::get_if< bool >(&value) )
					{
						return *p;
					}

					if ( const auto * p = std::get_if< int32_t >(&value) )
					{
						return *p > 0;
					}

					if ( const auto * p = std::get_if< uint32_t >(&value) )
					{
						return *p > 0;
					}

					if ( const auto * p = std::get_if< int64_t >(&value) )
					{
						return *p > 0;
					}

					if ( const auto * p = std::get_if< uint64_t >(&value) )
					{
						return *p > 0;
					}

					if ( const auto * p = std::get_if< int16_t >(&value) )
					{
						return *p > 0;
					}

					if ( const auto * p = std::get_if< uint16_t >(&value) )
					{
						return *p > 0;
					}
				}

				if constexpr ( std::is_same_v< variable_t, int16_t > )
				{
					if ( const auto * p = std::get_if< int16_t >(&value) )
					{
						return *p;
					}

					if ( const auto * p = std::get_if< bool >(&value) )
					{
						return *p ? static_cast< variable_t >(1) : static_cast< variable_t >(0);
					}

					if ( const auto * p = std::get_if< uint16_t >(&value) )
					{
						/* NOTE: Possible loss of precision on high number. */
						return static_cast< variable_t >(*p);
					}

					if ( const auto * p = std::get_if< int32_t >(&value) )
					{
						/* NOTE: Possible loss of precision on high number. */
						return static_cast< variable_t >(*p);
					}

					if ( const auto * p = std::get_if< uint32_t >(&value) )
					{
						/* NOTE: Possible loss of precision on high number. */
						return static_cast< variable_t >(*p);
					}

					if ( const auto * p = std::get_if< int64_t >(&value) )
					{
						/* NOTE: Possible loss of precision on high number. */
						return static_cast< variable_t >(*p);
					}

					if ( const auto * p = std::get_if< uint64_t >(&value) )
					{
						/* NOTE: Possible loss of precision on high number. */
						return static_cast< variable_t >(*p);
					}
				}

				if constexpr ( std::is_same_v< variable_t, uint16_t > )
				{
					if ( const auto * p = std::get_if< uint16_t >(&value) )
					{
						return *p;
					}

					if ( const auto * p = std::get_if< bool >(&value) )
					{
						return *p ? static_cast< variable_t >(1) : static_cast< variable_t >(0);
					}

					if ( const auto * p = std::get_if< int16_t >(&value) )
					{
						return *p >= 0 ? static_cast< variable_t >(*p) : defaultValue;
					}

					if ( const auto * p = std::get_if< int32_t >(&value) )
					{
						/* NOTE: Possible loss of precision on high number. */
						return *p >= 0 ? static_cast< variable_t >(*p) : defaultValue;
					}

					if ( const auto * p = std::get_if< uint32_t >(&value) )
					{
						/* NOTE: Possible loss of precision on high number. */
						return static_cast< variable_t >(*p);
					}

					if ( const auto * p = std::get_if< int64_t >(&value) )
					{
						/* NOTE: Possible loss of precision on high number. */
						return *p >= 0 ? static_cast< variable_t >(*p) : defaultValue;
					}

					if ( const auto * p = std::get_if< uint64_t >(&value) )
					{
						/* NOTE: Possible loss of precision on high number. */
						return static_cast< variable_t >(*p);
					}
				}

				if constexpr ( std::is_same_v< variable_t, int32_t > )
				{
					if ( const auto * p = std::get_if< int32_t >(&value) )
					{
						return *p;
					}

					if ( const auto * p = std::get_if< bool >(&value) )
					{
						return *p ? static_cast< variable_t >(1) : static_cast< variable_t >(0);
					}

					if ( const auto * p = std::get_if< uint32_t >(&value) )
					{
						/* NOTE: Possible loss of precision on high number. */
						return static_cast< variable_t >(*p);
					}

					if ( const auto * p = std::get_if< int64_t >(&value) )
					{
						/* NOTE: Possible loss of precision on high number. */
						return static_cast< variable_t >(*p);
					}

					if ( const auto * p = std::get_if< uint64_t >(&value) )
					{
						/* NOTE: Possible loss of precision on high number. */
						return static_cast< variable_t >(*p);
					}

					if ( const auto * p = std::get_if< int16_t >(&value) )
					{
						return static_cast< variable_t >(*p);
					}

					if ( const auto * p = std::get_if< uint16_t >(&value) )
					{
						return static_cast< variable_t >(*p);
					}
				}

				if constexpr ( std::is_same_v< variable_t, uint32_t > )
				{
					if ( const auto * p = std::get_if< uint32_t >(&value) )
					{
						return *p;
					}

					if ( const auto * p = std::get_if< bool >(&value) )
					{
						return *p ? static_cast< variable_t >(1) : static_cast< variable_t >(0);
					}

					if ( const auto * p = std::get_if< int32_t >(&value) )
					{
						return *p >= 0 ? static_cast< variable_t >(*p) : defaultValue;
					}

					if ( const auto * p = std::get_if< int64_t >(&value) )
					{
						/* NOTE: Possible loss of precision on high number. */
						return *p >= 0 ? static_cast< variable_t >(*p) : defaultValue;
					}

					if ( const auto * p = std::get_if< uint64_t >(&value) )
					{
						/* NOTE: Possible loss of precision on high number. */
						return static_cast< variable_t >(*p);
					}

					if ( const auto * p = std::get_if< int16_t >(&value) )
					{
						return *p >= 0 ? static_cast< variable_t >(*p) : defaultValue;
					}

					if ( const auto * p = std::get_if< uint16_t >(&value) )
					{
						return static_cast< variable_t >(*p);
					}
				}

				if constexpr ( std::is_same_v< variable_t, int64_t > )
				{
					if ( const auto * p = std::get_if< int64_t >(&value) )
					{
						return *p;
					}

					if ( const auto * p = std::get_if< bool >(&value) )
					{
						return *p ? static_cast< variable_t >(1) : static_cast< variable_t >(0);
					}

					if ( const auto * p = std::get_if< int32_t >(&value) )
					{
						return static_cast< variable_t >(*p);
					}

					if ( const auto * p = std::get_if< uint32_t >(&value) )
					{
						return static_cast< variable_t >(*p);
					}

					if ( const auto * p = std::get_if< uint64_t >(&value) )
					{
						/* NOTE: Possible loss of precision on high number. */
						return static_cast< variable_t >(*p);
					}

					if ( const auto * p = std::get_if< int16_t >(&value) )
					{
						return static_cast< variable_t >(*p);
					}

					if ( const auto * p = std::get_if< uint16_t >(&value) )
					{
						return static_cast< variable_t >(*p);
					}
				}

				if constexpr ( std::is_same_v< variable_t, uint64_t > )
				{
					if ( const auto * p = std::get_if< uint64_t >(&value) )
					{
						return *p;
					}

					if ( const auto * p = std::get_if< bool >(&value) )
					{
						return *p ? static_cast< variable_t >(1) : static_cast< variable_t >(0);
					}

					if ( const auto * p = std::get_if< int32_t >(&value) )
					{
						return *p >= 0 ? static_cast< variable_t >(*p) : defaultValue;
					}

					if ( const auto * p = std::get_if< uint32_t >(&value) )
					{
						return static_cast< variable_t >(*p);
					}

					if ( const auto * p = std::get_if< int64_t >(&value) )
					{
						return *p >= 0 ? static_cast< variable_t >(*p) : defaultValue;
					}

					if ( const auto * p = std::get_if< int16_t >(&value) )
					{
						return *p >= 0 ? static_cast< variable_t >(*p) : defaultValue;
					}

					if ( const auto * p = std::get_if< uint16_t >(&value) )
					{
						return static_cast< variable_t >(*p);
					}
				}

				if constexpr ( std::is_same_v< variable_t, float > )
				{
					if ( const auto * p = std::get_if< float >(&value) )
					{
						return *p;
					}

					if ( const auto * p = std::get_if< double >(&value) )
					{
						/* NOTE: Possible loss of precision on high number. */
						return static_cast< variable_t >(*p);
					}

					if ( const auto * p = std::get_if< bool >(&value) )
					{
						return *p ? static_cast< variable_t >(1) : static_cast< variable_t >(0);
					}

					if ( const auto * p = std::get_if< int32_t >(&value) )
					{
						return static_cast< variable_t >(*p);
					}

					if ( const auto * p = std::get_if< uint32_t >(&value) )
					{
						return static_cast< variable_t >(*p);
					}

					if ( const auto * p = std::get_if< int64_t >(&value) )
					{
						/* NOTE: Possible loss of precision on high number. */
						return static_cast< variable_t >(*p);
					}

					if ( const auto * p = std::get_if< uint64_t >(&value) )
					{
						/* NOTE: Possible loss of precision on high number. */
						return static_cast< variable_t >(*p);
					}

					if ( const auto * p = std::get_if< int16_t >(&value) )
					{
						return static_cast< variable_t >(*p);
					}

					if ( const auto * p = std::get_if< uint16_t >(&value) )
					{
						return static_cast< variable_t >(*p);
					}
				}

				if constexpr ( std::is_same_v< variable_t, double > )
				{
					if ( const auto * p = std::get_if< double >(&value) )
					{
						return *p;
					}

					if ( const auto * p = std::get_if< float >(&value) )
					{
						return static_cast< variable_t >(*p);
					}

					if ( const auto * p = std::get_if< bool >(&value) )
					{
						return *p ? static_cast< variable_t >(1) : static_cast< variable_t >(0);
					}

					if ( const auto * p = std::get_if< int32_t >(&value) )
					{
						return static_cast< variable_t >(*p);
					}

					if ( const auto * p = std::get_if< uint32_t >(&value) )
					{
						return static_cast< variable_t >(*p);
					}

					if ( const auto * p = std::get_if< int64_t >(&value) )
					{
						return static_cast< variable_t >(*p);
					}

					if ( const auto * p = std::get_if< uint64_t >(&value) )
					{
						return static_cast< variable_t >(*p);
					}

					if ( const auto * p = std::get_if< int16_t >(&value) )
					{
						return static_cast< variable_t >(*p);
					}

					if ( const auto * p = std::get_if< uint16_t >(&value) )
					{
						return static_cast< variable_t >(*p);
					}
				}

				if constexpr ( std::is_same_v< variable_t, std::string > )
				{
					if ( const auto * p = std::get_if< std::string >(&value) )
					{
						return *p;
					}
				}

				return defaultValue;
			}

			/**
			 * @brief Recursively parses one JSON object node into the store map.
			 *
			 * Traverses the members of @p data. Object members trigger a recursive call
			 * with an extended @p key (@c "parentKey/childName"). Array members are
			 * stored via @ref SettingStore::setVariableInArray(). Scalar members are
			 * stored via @ref SettingStore::setVariable(). Unsupported JSON types (null,
			 * nested objects exceeding depth) are silently skipped.
			 *
			 * @param data The JSON object node to parse.
			 * @param key  The accumulated store-key prefix for this recursion level.
			 *             Pass an empty string for the root level.
			 * @return @c true on success, @c false if a recursive level fails.
			 */
			[[nodiscard]]
			bool readLevel (const Json::Value & data, const std::string & key) noexcept;

			/**
			 * @brief Parses the JSON settings file at @p filepath into the store map.
			 *
			 * Reads and parses the entire file using FastJSON, then delegates to
			 * @ref readLevel() starting at the root node. Caller is responsible for
			 * holding the write lock before calling this method.
			 *
			 * @param filepath Absolute path to the JSON settings file.
			 * @return @c true on success, @c false if the file cannot be parsed.
			 */
			[[nodiscard]]
			bool readFile (const std::filesystem::path & filepath) noexcept;

			/**
			 * @brief Serializes all stores to a JSON settings file at @p filepath.
			 *
			 * Writes a JSON file with a header section (@ref VersionKey, @ref DateKey)
			 * followed by the nested store content. The file is formatted with tab
			 * indentation for human readability. Caller is responsible for holding an
			 * appropriate lock before calling this method.
			 *
			 * @param filepath Absolute path to the output JSON file.
			 * @return @c true on success, @c false if serialization or disk write fails.
			 */
			[[nodiscard]]
			bool writeFile (const std::filesystem::path & filepath) const noexcept;

			/**
			 * @brief Writes a human-readable representation of all settings to @p out.
			 *
			 * Iterates over every store and prints its key as a section header followed
			 * by each scalar variable and array in @c "name = value" format. Boolean
			 * values are printed as @c "On" / @c "Off". Acquires a shared lock on
			 * @c m_storeAccess before reading.
			 *
			 * @param out The output stream to write to.
			 * @param obj The @ref Settings instance to serialize.
			 * @return A reference to @p out for chaining.
			 */
			friend std::ostream & operator<< (std::ostream & out, const Settings & obj);

			const Arguments & m_arguments;
			const FileSystem & m_fileSystem;
			std::map< std::string, SettingStore, std::less<> > m_stores; ///< Map of store-key to SettingStore; the key is the slash-delimited path prefix (empty string for the root store).
			std::filesystem::path m_filepath;
			mutable std::shared_mutex m_storeAccess; ///< Reader-writer mutex protecting @c m_stores; shared for reads, exclusive for writes.
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
