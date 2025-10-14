/*
 * src/Testing/test_ObserverPattern.cpp
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

#include <gtest/gtest.h>

/* STL inclusions. */
#include <cstddef>
#include <any>

/* Local inclusions. */
#include "Libs/ObserverTrait.hpp"
#include "Libs/ObservableTrait.hpp"

using namespace Libs;

class Noise final : public ObservableTrait
{
	public:

		Noise () = default;

		/**
		 * @brief Returns the unique identifier for this class [Thread-safe].
		 * @return size_t
		 */
		static
		size_t
		getClassUID () noexcept
		{
			static const size_t classUID = EmEn::Libs::Hash::FNV1a("Noise");

			return classUID;
		}

		/** @copydoc EmEn::Libs::ObservableTrait::classUID() const */
		[[nodiscard]]
		size_t
		classUID () const noexcept override
		{
			return getClassUID();
		}

		/** @copydoc EmEn::Libs::ObservableTrait::is() const */
		[[nodiscard]]
		bool
		is (size_t classUID) const noexcept override
		{
			return classUID == getClassUID();
		}

		void
		doSomething () noexcept
		{
			this->notify(0);
		}
};

class DummyClass final : public ObservableTrait
{
	public:

		DummyClass () = default;

		/**
		 * @brief Returns the unique identifier for this class [Thread-safe].
		 * @return size_t
		 */
		static
		size_t
		getClassUID () noexcept
		{
			static const size_t classUID = EmEn::Libs::Hash::FNV1a("DummyClass");

			return classUID;
		}

		/** @copydoc EmEn::Libs::ObservableTrait::classUID() const */
		[[nodiscard]]
		size_t
		classUID () const noexcept override
		{
			return getClassUID();
		}

		/** @copydoc EmEn::Libs::ObservableTrait::is() const */
		[[nodiscard]]
		bool
		is (size_t classUID) const noexcept override
		{
			return classUID == getClassUID();
		}

		void
		doSomething () noexcept
		{
			this->notify(0);
		}
};

class Watcher final : public ObserverTrait
{
	public:

		Watcher () = default;

		[[nodiscard]]
		bool
		noiseEventIntercepted () const noexcept
		{
			return m_noiseEventIntercepted;
		}

		[[nodiscard]]
		bool
		dummyEventIntercepted () const noexcept
		{
			return m_dummyEventIntercepted;
		}

	private:

		bool
		onNotification (const ObservableTrait * observable, int /*notificationCode*/, const std::any & /*data*/) noexcept override
		{
			if ( observable->is(Noise::getClassUID()) )
			{
				m_noiseEventIntercepted = true;

				return true;
			}

			if ( observable->is(DummyClass::getClassUID()) )
			{
				m_dummyEventIntercepted = true;

				return true;
			}

			return false;
		}

		bool m_noiseEventIntercepted{false};
		bool m_dummyEventIntercepted{false};
};

TEST(Observer, uniqueIdentifier)
{
	const Noise noisyInstance{};

	const DummyClass instanceA{};

	ASSERT_EQ(instanceA.is(DummyClass::getClassUID()), true);

	const DummyClass instanceB{};

	ASSERT_EQ(instanceB.is(DummyClass::getClassUID()), true);
}

TEST(Observer, watch)
{
	Noise noisyInstance{};

	Watcher watcher{};

	DummyClass instance{};

	watcher.observe(&instance);
	watcher.observe(&noisyInstance);

	ASSERT_EQ(watcher.noiseEventIntercepted(), false);
	ASSERT_EQ(watcher.dummyEventIntercepted(), false);

	instance.doSomething();

	ASSERT_EQ(watcher.noiseEventIntercepted(), false);
	ASSERT_EQ(watcher.dummyEventIntercepted(), true);

	noisyInstance.doSomething();

	ASSERT_EQ(watcher.noiseEventIntercepted(), true);
	ASSERT_EQ(watcher.dummyEventIntercepted(), true);
}
