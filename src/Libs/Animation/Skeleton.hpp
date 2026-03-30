/*
 * src/Libs/Animation/Skeleton.hpp
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

/* STL inclusions. */
#include <cstddef>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

/* Local inclusions. */
#include "Joint.hpp"

namespace EmEn::Libs::Animation
{
	/**
	 * @brief Represents a skeletal hierarchy — an ordered collection of joints.
	 * @note The skeleton is immutable after construction. Joint ordering guarantees that
	 * a parent joint always has a lower index than its children (topological order).
	 * This allows forward iteration to compute world transforms without recursion.
	 * @tparam precision_t Floating point type. Default float.
	 */
	template< typename precision_t = float >
	requires (std::is_floating_point_v< precision_t >)
	class Skeleton final
	{
		public:

			/**
			 * @brief Constructs an empty skeleton.
			 */
			Skeleton () noexcept = default;

			/**
			 * @brief Constructs a skeleton from a vector of joints.
			 * @note Joints must be in topological order (parent index < child index).
			 * @param joints The ordered joint array.
			 */
			explicit
			Skeleton (std::vector< Joint< precision_t > > joints) noexcept
				: m_joints(std::move(joints))
			{
				this->buildNameIndex();
			}

			/**
			 * @brief Returns the number of joints in the skeleton.
			 * @return size_t
			 */
			[[nodiscard]]
			size_t
			jointCount () const noexcept
			{
				return m_joints.size();
			}

			/**
			 * @brief Returns true if the skeleton has no joints.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			empty () const noexcept
			{
				return m_joints.empty();
			}

			/**
			 * @brief Returns the joint at the given index.
			 * @param index The joint index.
			 * @return const Joint< precision_t > &
			 */
			[[nodiscard]]
			const Joint< precision_t > &
			joint (size_t index) const noexcept
			{
				return m_joints[index];
			}

			/**
			 * @brief Returns the entire joint array.
			 * @return const std::vector< Joint< precision_t > > &
			 */
			[[nodiscard]]
			const std::vector< Joint< precision_t > > &
			joints () const noexcept
			{
				return m_joints;
			}

			/**
			 * @brief Finds a joint index by name.
			 * @param name The joint name.
			 * @return int32_t The joint index, or NoParent (-1) if not found.
			 */
			[[nodiscard]]
			int32_t
			findJoint (const std::string & name) const noexcept
			{
				const auto it = m_nameToIndex.find(name);

				if ( it != m_nameToIndex.end() )
				{
					return it->second;
				}

				return NoParent;
			}

			/**
			 * @brief Returns indices of all root joints (joints with no parent).
			 * @return std::vector< size_t >
			 */
			[[nodiscard]]
			std::vector< size_t >
			rootJoints () const
			{
				std::vector< size_t > roots;

				for ( size_t i = 0; i < m_joints.size(); ++i )
				{
					if ( m_joints[i].isRoot() )
					{
						roots.push_back(i);
					}
				}

				return roots;
			}

			/**
			 * @brief Validates the skeleton hierarchy.
			 * @note Checks that all parent indices are valid and that parents precede children (topological order).
			 * @return bool True if the hierarchy is valid.
			 */
			[[nodiscard]]
			bool
			isValid () const noexcept
			{
				for ( size_t i = 0; i < m_joints.size(); ++i )
				{
					const auto parentIdx = m_joints[i].parentIndex;

					if ( parentIdx == NoParent )
					{
						continue;
					}

					/* Parent must exist and must precede this joint. */
					if ( parentIdx < 0 || static_cast< size_t >(parentIdx) >= i )
					{
						return false;
					}
				}

				return true;
			}

		private:

			/**
			 * @brief Builds the name-to-index lookup map from the joint array.
			 */
			void
			buildNameIndex ()
			{
				m_nameToIndex.clear();
				m_nameToIndex.reserve(m_joints.size());

				for ( size_t i = 0; i < m_joints.size(); ++i )
				{
					if ( !m_joints[i].name.empty() )
					{
						m_nameToIndex[m_joints[i].name] = static_cast< int32_t >(i);
					}
				}
			}

			std::vector< Joint< precision_t > > m_joints{};
			std::unordered_map< std::string, int32_t > m_nameToIndex{};
	};

	using SkeletonF = Skeleton< float >;
	using SkeletonD = Skeleton< double >;
}
