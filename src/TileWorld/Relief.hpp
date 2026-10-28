/*
 * src/TileWorld/Relief.hpp
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
#include <cstdint>
#include <iterator> // IWYU pragma: keep (std::pair: IWYU adds then removes it across the TUs, libstdc++ mapping)
#include <optional>
#include <utility>
#include <vector>

/* Local inclusions for usages. */
#include "Types.hpp"

/* Forward declarations. */
namespace EmEn::TileWorld
{
	class Layout;
}

namespace EmEn::TileWorld
{
	/** @brief How the relief is drawn before the tiles constrain it. */
	struct ReliefParameters
	{
		float amplitude{0.0F}; /**< Peak-to-mean height, metres; 0 = flat. Finite, 0 to 1000. */
		float wavelength{256.0F}; /**< The noise's feature size, metres. Finite, > 0. */
		uint32_t seed{1};
	};

	/** @brief What constrain() / solve() did. */
	struct ReliefReport
	{
		float largestViolation{0.0F}; /**< The worst remaining excess over a rule, metres (0 when converged). */
		float amplitude{0.0F}; /**< The amplitude finally used (solve() halves it on a failure). */
		uint32_t sweeps{0}; /**< The sweeps of the last constrain(). */
		uint32_t attempts{0}; /**< The constrain() runs solve() made. */
		bool converged{false};
	};

	/**
	 * @brief The heights at the tile corners: (columns + 1) × (rows + 1), shared by the neighbouring tiles, so the
	 * surface is continuous by construction.
	 * @note constrain() enforces every tile's SlopeRule by cyclic projections onto the convex sets the rules define
	 * (each rule is a set of |h_a − h_b| ≤ c constraints; the flat relief satisfies them all, so the sets intersect and
	 * the projections converge — L. G. Gubin, B. T. Polyak, E. V. Raik, "The method of projections for finding the
	 * common point of convex sets", USSR Comp. Math. & Math. Phys. 7(6), 1967). Each projection moves both corners
	 * symmetrically: the mean height is kept. Deterministic (a fixed sweep order), bounded (MaxSweeps).
	 * @note Thread: built and read on one thread; read-only (shareable) once handed to the builder.
	 */
	class EMEN_API Relief final
	{
		public:

			/** @brief The most projection sweeps constrain() runs. */
			static constexpr uint32_t MaxSweeps{4096};

			/** @brief The tolerance under which a rule counts as met, metres. */
			static constexpr float Tolerance{0.001F};

			/** @brief The amplitude halvings solve() tries before it falls back to a flat relief. */
			static constexpr uint32_t MaxRetries{4};

			/** @brief Constructs an empty relief. */
			Relief () noexcept = default;

			/**
			 * @brief Draws the corner heights for a layout's grid from seeded noise (Base::PortableRandom-seeded).
			 * @return bool False when the layout is empty or a parameter is out of range (nothing changed).
			 */
			[[nodiscard]]
			bool generate (const Layout & layout, const ReliefParameters & parameters) noexcept;

			/**
			 * @brief Makes the relief flat at a height (0 by default) for a layout's grid.
			 * @return bool False when the layout is empty.
			 */
			[[nodiscard]]
			bool flatten (const Layout & layout, float height = 0.0F) noexcept;

			/**
			 * @brief Enforces every tile's slope rule (the layout must be the one the relief was sized for, validated).
			 * @return ReliefReport converged = every rule met within Tolerance.
			 */
			[[nodiscard]]
			ReliefReport constrain (const Layout & layout) noexcept;

			/**
			 * @brief Generates then constrains; when the rules are not met within MaxSweeps, halves the amplitude and
			 * starts again, at most MaxRetries times, then flattens (owner decision 2026-10-28: the world always builds).
			 * Every retry and the fallback are logged with their numbers.
			 * @return std::optional< ReliefReport > Nothing only when the layout is empty or a parameter out of range.
			 */
			[[nodiscard]]
			std::optional< ReliefReport > solve (const Layout & layout, const ReliefParameters & parameters) noexcept;

			/**
			 * @brief Sets one corner's height by hand (an editor's tool); constrain() then bends the rest around it.
			 * @return bool False outside the lattice or for a non-finite height.
			 */
			[[nodiscard]]
			bool setHeight (uint32_t cornerColumn, uint32_t cornerRow, float height) noexcept;

			/** @brief The height of a corner of the lattice (column 0..columns, row 0..rows); 0 outside. */
			[[nodiscard]]
			float height (uint32_t cornerColumn, uint32_t cornerRow) const noexcept;

			/** @brief A cell's four corner heights in the WORLD frame (NW, NE, SE, SW). */
			[[nodiscard]]
			CornerHeights cellHeights (uint32_t column, uint32_t row) const noexcept;

			/** @brief The corners along X (the layout's columns + 1; 0 when empty). */
			[[nodiscard]]
			uint32_t
			cornerColumns () const noexcept
			{
				return m_cornerColumns;
			}

			/** @brief The corners along Z (the layout's rows + 1; 0 when empty). */
			[[nodiscard]]
			uint32_t
			cornerRows () const noexcept
			{
				return m_cornerRows;
			}

			/** @brief The lowest and the highest corner. */
			[[nodiscard]]
			std::pair< float, float > range () const noexcept;

		private:

			std::vector< float > m_heights;
			uint32_t m_cornerColumns{0};
			uint32_t m_cornerRows{0};
	};
}
