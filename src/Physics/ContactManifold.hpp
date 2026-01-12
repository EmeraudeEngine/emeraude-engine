/*
 * src/Physics/ContactManifold.hpp
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

/* Local inclusions. */
#include "Libs/StaticVector.hpp"
#include "ContactPoint.hpp"

namespace EmEn::Physics
{
	/**
	 * @brief Represents a collision manifold containing multiple contact points between two bodies.
	 * @note A manifold groups all contacts from a single collision (e.g., box-box can have up to 4 contacts).
	 */
	class ContactManifold final
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"ContactManifold"};

			/** @brief Maximum number of contact points per manifold (box-box worst case). */
			static constexpr size_t MaxContactPoints = 4;

			/**
			 * @brief Constructs an empty contact manifold.
			 */
			ContactManifold () noexcept = default;

			/**
			 * @brief Constructs a contact manifold for two bodies.
			 * @param bodyA Pointer to the first movable body.
			 * @param bodyB Pointer to the second movable body. Default none.
			 */
			explicit
			ContactManifold (MovableTrait * bodyA, MovableTrait * bodyB = nullptr) noexcept
				: m_bodyA{bodyA},
				  m_bodyB{bodyB}
			{

			}

			/**
			 * @brief Adds a contact point to the manifold.
			 * @param contact The contact point to add.
			 * @return bool True if added successfully, false if manifold is full.
			 */
			bool
			addContact (const ContactPoint & contact) noexcept
			{
				if ( m_contacts.size() >= MaxContactPoints )
				{
					return false;
				}

				m_contacts.push_back(contact);

				return true;
			}

			/**
			 * @brief Adds a contact point by constructing it in place.
			 * @param worldPosition The contact position in world space.
			 * @param worldNormal The contact normal from body A to body B.
			 * @param depth The penetration depth.
			 * @return bool True if added successfully, false if manifold is full.
			 */
			bool
			addContact (const Libs::Math::Vector< 3, float > & worldPosition, const Libs::Math::Vector< 3, float > & worldNormal, float depth) noexcept
			{
				if ( m_contacts.size() >= MaxContactPoints )
				{
					return false;
				}

				m_contacts.emplace_back(worldPosition, worldNormal, depth, m_bodyA, m_bodyB);

				return true;
			}

			/**
			 * @brief Prepares the manifold for solving by computing cached values.
			 * @note This computes relative positions (rA, rB) for each contact point.
			 */
			void prepare () noexcept;

			/**
			 * @brief Clears all contact points.
			 */
			void
			clear () noexcept
			{
				m_contacts.clear();
			}

			/**
			 * @brief Returns whether the manifold has any contacts.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			hasContacts () const noexcept
			{
				return !m_contacts.empty();
			}

			/**
			 * @brief Returns the number of contact points.
			 * @return size_t
			 */
			[[nodiscard]]
			size_t
			contactCount () const noexcept
			{
				return m_contacts.size();
			}

			/**
			 * @brief Returns the contact points.
			 * @return Libs::StaticVector< ContactPoint, MaxContactPoints > &
			 */
			[[nodiscard]]
			Libs::StaticVector< ContactPoint, MaxContactPoints > &
			contacts () noexcept
			{
				return m_contacts;
			}

			/**
			 * @brief Returns the contact points (const).
			 * @return const Libs::StaticVector< ContactPoint, MaxContactPoints > &
			 */
			[[nodiscard]]
			const Libs::StaticVector< ContactPoint, MaxContactPoints > &
			contacts () const noexcept
			{
				return m_contacts;
			}

			/**
			 * @brief Returns the first body.
			 * @return MovableTrait *
			 */
			[[nodiscard]]
			MovableTrait *
			bodyA () const noexcept
			{
				return m_bodyA;
			}

			/**
			 * @brief Returns the second body.
			 * @return MovableTrait *
			 */
			[[nodiscard]]
			MovableTrait *
			bodyB () const noexcept
			{
				return m_bodyB;
			}

		private:

			/**
			 * @brief STL streams printable object.
			 * @param out A reference to the stream output.
			 * @param obj A reference to the object to print.
			 * @return std::ostream &
			 */
			friend std::ostream & operator<< (std::ostream & out, const ContactManifold & obj);

			MovableTrait * m_bodyA{nullptr};
			MovableTrait * m_bodyB{nullptr};
			Libs::StaticVector< ContactPoint, MaxContactPoints > m_contacts;
	};

	inline
	std::ostream &
	operator<< (std::ostream & out, const ContactManifold & obj)
	{
		out <<
			"Contact manifold :" "\n"
			"Body A : " << ( obj.m_bodyA ? "Present" : "None" ) << "\n"
			"Body B : " << ( obj.m_bodyB ? "Present" : "None" ) << "\n";

		if ( obj.hasContacts() )
		{
			for ( const auto & contact : obj.contacts() )
			{
				out << contact << "\n";
			}
		}
		else
		{
			out << "No contact !" << "\n";
		}

		return out;
	}

	/**
	 * @brief Stringifies the object.
	 * @param obj A reference to the object to print.
	 * @return std::string
	 */
	inline
	std::string
	to_string (const ContactManifold & obj) noexcept
	{
		std::stringstream output;

		output << obj;

		return output.str();
	}
}
