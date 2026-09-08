/*
 * src/SettingsKeyRestoration.hpp
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
#include <array>
#include <filesystem>
#include <span>
#include <string>
#include <string_view>
#include <vector>

/* Local inclusions for usages. */
#include "SettingKeys.hpp"

namespace Json
{
	class Value;
}

namespace EmEn
{
	class Settings;

	/**
	 * @brief Describes the settings entries that survive a settings reset.
	 *
	 * A settings reset (the per-release version guard of @ref Core, or the @c --reset-settings
	 * argument) renames @c settings.json to a timestamped backup and starts the store over.
	 * Some entries are not "tuning" but the identity of an installation — the remote console
	 * gate, the window geometry, a validation-layer setup — and losing them on every release is
	 * a regression for whoever set them. This class is the declaration of what is carried over:
	 * the entries listed here are read back from the backup file that was just written and
	 * re-injected into the fresh store.
	 *
	 * The list is **additive across the cascade**: a default-constructed instance already
	 * carries the engine base (@ref EngineKeys), the application adds its own entries on top,
	 * and can withdraw an engine entry it does not want with @ref forget(). @ref blank() gives
	 * an instance without the engine base. The list is independent of the reset trigger: the
	 * @c resetSettingsOnNewVersion flag says *when* a release deserves a clean file, this class
	 * says *what* is kept, and the two are meant to evolve separately.
	 *
	 * Two granularities exist. A **key** (@ref keep()) is one variable or one array, addressed
	 * by its full slash-delimited path. A **store** (@ref keepStore()) is a whole sub-tree —
	 * every variable, array and nested store below the path. The file header stamps
	 * (@ref Settings::EngineVersionKey, @ref Settings::ApplicationVersionKey, @ref Settings::DateKey)
	 * are refused: carrying them over would defeat the version guard.
	 *
	 * Typical declaration, with the list kept as constant data above the constructor:
	 * @code
	 * constexpr std::array< std::string_view, 2 > RestoredSettingsKeys{
	 *	 EmEn::VkInstanceEnableDebugKey,
	 *	 EmEn::VkInstanceRequestedValidationLayersKey
	 * };
	 *
	 * Application::Application (int argc, char * * argv) noexcept
	 *	 : Core{argc, argv, Name, Version, Organization, Domain, true, EmEn::SettingsKeyRestoration{RestoredSettingsKeys}}
	 * {}
	 * @endcode
	 *
	 * @note The instance is consumed by the @ref Core constructor because the reset runs there,
	 * before any virtual hook of the application is reachable; this is why the contract is a
	 * value passed to the constructor and not an overridable method.
	 * @see EmEn::Core::resetSettingsIfOutdated(), EmEn::Core::executeResetSettings()
	 * @version 0.9.68
	 */
	class EMEN_LEAN_API SettingsKeyRestoration final
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"SettingsKeyRestoration"};

			/**
			 * @brief The engine base: keys every application built on the engine keeps across a reset.
			 * @details The system notification permission the user answered once, the remote console
			 * gate and its bind address (a closed-by-default door a user opened on purpose), and the
			 * windowed-mode size. A default-constructed instance starts from this list; @ref blank()
			 * does not.
			 */
			static constexpr std::array< std::string_view, 6 > EngineKeys{
				CorePermissionsNotificationsKey,
				ConsoleEnableRemoteListenerKey,
				ConsoleRemoteListenerAddressKey,
				WindowWidthKey,
				WindowHeightKey,
				GLFWWaylandEnableLibDecorKey
			};

			/**
			 * @brief What a @ref restore() call did, for the caller's report to the user.
			 */
			struct Report final
			{
				std::vector< std::string > restored; ///< Entries found in the backup and written to the target store (keys and stores alike).
				std::vector< std::string > absent; ///< Declared entries the backup did not contain (or contained under an incompatible shape).
			};

			/**
			 * @brief Constructs a restoration carrying the engine base (@ref EngineKeys).
			 */
			SettingsKeyRestoration () noexcept;

			/**
			 * @brief Constructs a restoration carrying the engine base plus the application entries.
			 * @details Convenience for the declaration-as-constant-data pattern: a
			 * @c std::array< std::string_view, N > converts to both spans implicitly.
			 * @param keys Full paths of variables or arrays to keep. Duplicates and reserved keys are dropped with a warning.
			 * @param stores Store paths whose whole sub-tree is kept. Default none.
			 */
			explicit SettingsKeyRestoration (std::span< const std::string_view > keys, std::span< const std::string_view > stores = {}) noexcept;

			/**
			 * @brief Returns a restoration without the engine base.
			 * @return SettingsKeyRestoration
			 */
			[[nodiscard]]
			static SettingsKeyRestoration blank () noexcept;

			/**
			 * @brief Declares one variable or one array to keep.
			 * @param settingPath The full slash-delimited path, e.g. @c "Core/Video/VulkanInstance/EnableDebug".
			 * Empty paths, duplicates and the file header stamps are refused (a warning is traced for the latter).
			 * @return SettingsKeyRestoration & for chaining.
			 */
			SettingsKeyRestoration & keep (std::string_view settingPath) noexcept;

			/**
			 * @brief Declares a whole store sub-tree to keep.
			 * @param storePath The slash-delimited store path, e.g. @c "Core/Video/Window". The root store
			 * (empty path) is refused, as it carries the file header stamps.
			 * @return SettingsKeyRestoration & for chaining.
			 */
			SettingsKeyRestoration & keepStore (std::string_view storePath) noexcept;

			/**
			 * @brief Withdraws a previously declared key or store.
			 * @details This is how an application drops an entry of the engine base it does not want.
			 * @param path The exact path passed to @ref keep() or @ref keepStore().
			 * @return SettingsKeyRestoration & for chaining.
			 */
			SettingsKeyRestoration & forget (std::string_view path) noexcept;

			/**
			 * @brief Returns whether nothing is declared.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			empty () const noexcept
			{
				return m_keys.empty() && m_stores.empty();
			}

			/**
			 * @brief Returns the declared keys (variables and arrays), in declaration order.
			 * @return const std::vector< std::string > &
			 */
			[[nodiscard]]
			const std::vector< std::string > &
			keys () const noexcept
			{
				return m_keys;
			}

			/**
			 * @brief Returns the declared store paths, in declaration order.
			 * @return const std::vector< std::string > &
			 */
			[[nodiscard]]
			const std::vector< std::string > &
			stores () const noexcept
			{
				return m_stores;
			}

			/**
			 * @brief Reads the declared entries back from a backup file into a settings store.
			 * @details The backup is parsed once; each declared key is looked up along its path in
			 * the JSON tree and written to @p target with the type the file gives it (a scalar
			 * through Settings::setValue(), an array element by element after clearing the target
			 * array), each declared store is imported as a sub-tree through Settings::importJSON().
			 * Entries the backup does not contain — or contains under another shape, an object where
			 * a key was declared — are reported as absent and skipped. Nothing else in @p target is
			 * touched, so this is meant to run right after Settings::clear().
			 * @param backupFilepath The file the reset just wrote. Must exist.
			 * @param target The store to write into.
			 * @param report Filled with what was restored and what was not found.
			 * @return @c true when the backup was parsed; @c false when it could not be read, in which
			 * case @p target is untouched and @p report is empty.
			 */
			[[nodiscard]]
			bool restore (const std::filesystem::path & backupFilepath, Settings & target, Report & report) const noexcept;

		private:

			/**
			 * @brief Returns whether a key path names a file header stamp.
			 * @param settingPath The path to check.
			 * @return bool
			 */
			[[nodiscard]]
			static bool isReservedKey (std::string_view settingPath) noexcept;

			/**
			 * @brief Walks a slash-delimited path down a JSON tree.
			 * @param root The parsed backup root.
			 * @param path The path to follow, one object member per segment.
			 * @return A pointer to the node, or @c nullptr when a segment is missing or an intermediate node is not an object.
			 */
			[[nodiscard]]
			static const Json::Value * findNode (const Json::Value & root, std::string_view path) noexcept;

			/**
			 * @brief Restores one declared key from the backup tree.
			 * @param root The parsed backup root.
			 * @param settingPath The declared key path.
			 * @param target The store to write into.
			 * @return @c true when the key was found and written, @c false when absent.
			 */
			static bool restoreKey (const Json::Value & root, const std::string & settingPath, Settings & target) noexcept;

			std::vector< std::string > m_keys;
			std::vector< std::string > m_stores;
	};
}
