/*
 * src/Libs/StaticVector.hpp
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

/* Application configuration */
#include "emeraude_config.hpp"

/* STL inclusions. */
#include <cstdlib>
#include <cstddef>
#include <cassert>
#include <cstring>
#include <array>
#include <stdexcept>
#include <type_traits>
#include <iterator>
#include <initializer_list>
#include <algorithm>
#include <iostream>

#if IS_WINDOWS
#undef min
#undef max
#endif

namespace EmEn::Libs
{
	/**
	 * @brief A container with std::vector-like semantics and static, stack-based storage.
	 *
	 * @details The `StaticVector` class behaves like a `std::vector` (dynamic size, contiguous elements),
	 * but its memory is allocated within the object itself, typically on the stack, avoiding any
	 * heap allocation. Its capacity is fixed at compile time.
	 *
	 * It is an ideal replacement for `std::vector` for small collections of objects where performance
	 * is critical and dynamic memory allocation is undesirable (e.g., in game loops, real-time systems,
	 * or embedded environments).
	 *
	 * This implementation is exception-safe and provides a strong exception guarantee for
	 * operations that may throw.
	 *
	 * @tparam data_t The type of elements stored in the container. Must be movable.
	 * @tparam max_capacity The maximum number of elements the container can hold.
	 */
	template< typename data_t, std::size_t max_capacity >
	class StaticVector final
	{
		public:

			/* Type alias (STL style) */
			using value_type = data_t;
			using size_type = std::size_t;
			using difference_type = std::ptrdiff_t;
			using reference = data_t &;
			using const_reference = const data_t &;
			using pointer = data_t *;
			using const_pointer = const data_t *;
			using iterator = data_t *;
			using const_iterator = const data_t *;
			using reverse_iterator = std::reverse_iterator< iterator >;
			using const_reverse_iterator = std::reverse_iterator< const_iterator >;

			/**
			 * @brief Default constructor. Constructs an empty StaticVector.
			 * @complexity Constant, O(1).
			 */
			constexpr StaticVector () = default;

#if defined(__cpp_exceptions)
			/**
			 * @brief Constructs the container with `count` default-inserted instances of T.
			 * @param count The number of elements to create.
			 * @throw std::length_error If count > capacity().
			 * @complexity Linear in count, O(N).
			 */
			constexpr
			explicit
			StaticVector (size_type count)
			{
				if ( count > max_capacity )
				{
					throw std::length_error{"StaticVector::ctor(count): Capacity exceeded!"};
				}

				try
				{
					for ( size_type index = 0; index < count; ++index )
					{
						new (this->data() + m_size) data_t();

						++m_size;
					}
				}
				catch (...)
				{
					this->clear();
					throw;
				}
			}

			/**
			 * @brief Constructs the container with `count` copies of `value`.
			 * @param count The number of elements to create.
			 * @param value The value to initialize elements with.
			 * @throw std::length_error If count > capacity().
			 * @complexity Linear in count, O(N).
			 */
			constexpr
			StaticVector (size_type count, const data_t & value)
			{
				if ( count > max_capacity )
				{
					throw std::length_error{"StaticVector::ctor(count, value): Capacity exceeded!"};
				}

				try
				{
					for ( size_type index = 0; index < count; ++index )
					{
						new (this->data() + m_size) data_t(value);

						++m_size;
					}
				}
				catch (...)
				{
					this->clear();
					throw;
				}
			}

			/**
			 * @brief Copy constructor. Constructs the container with a copy of the contents of `other`.
			 * @param other Another StaticVector to copy from.
			 * @complexity Linear in the size of other, O(N).
			 */
			StaticVector (const StaticVector & other)
			{
				try
				{
					for ( size_type index = 0; index < other.m_size; ++index )
					{
						new (this->data() + m_size) data_t(other[index]);
						++m_size;
					}
				}
				catch (...)
				{
					this->clear();
					throw;
				}
			}

			/**
			 * @brief Move constructor. Constructs the container with the contents of `other`.
			 * @param other A StaticVector to move from. `other` is left in a valid but empty state.
			 * @complexity Linear in the size of other, O(N).
			 */
			StaticVector (StaticVector && other) noexcept(std::is_nothrow_move_constructible_v< data_t >)
			{
				if constexpr (std::is_nothrow_move_constructible_v< data_t >)
				{
					for ( size_type index = 0; index < other.m_size; ++index )
					{
						new (this->data() + index) data_t(std::move(other[index]));
					}

					m_size = other.m_size;

					other.m_size = 0;
				}
				else
				{
					try
					{
						for ( size_type index = 0; index < other.m_size; ++index )
						{
							new (this->data() + m_size) data_t(std::move(other[index]));
							++m_size;
						}

						other.m_size = 0;
					}
					catch (...)
					{
						this->clear();

						throw;
					}
				}
			}

			/**
			 * @brief Constructs the container with the contents of an initializer list.
			 * @param initializer_list The initializer list to construct from.
			 * @throw std::length_error If initializer_list.size() > capacity().
			 * @complexity Linear in the size of the initializer list, O(N).
			 */
			constexpr
			StaticVector (std::initializer_list< data_t > initializer_list)
			{
				if ( initializer_list.size() > max_capacity )
				{
					throw std::length_error{"StaticVector::ctor(initializer_list): Number of items exceeds capacity!"};
				}

				try
				{
					for ( const auto & item : initializer_list )
					{
						new (this->data() + m_size) data_t(item);

						++m_size;
					}
				}
				catch (...)
				{
					this->clear();
					throw;
				}
			}
#else
			/**
			 * @brief Constructs the container with `count` default-inserted instances of T.
			 * @param count The number of elements to create.
			 * @warning std::abort() is called if count > capacity().
			 * @complexity Linear in count, O(N).
			 */
			constexpr
			explicit
			StaticVector (size_type count) noexcept
			{
				if ( count > max_capacity )
				{
					std::abort();
				}

				for ( size_type index = 0; index < count; ++index )
				{
					new (reinterpret_cast< data_t * >(m_data.data()) + index) data_t();
				}

				m_size = count;
			}

			/**
			 * @brief Constructs the container with `count` copies of `value`.
			 * @param count The number of elements to create.
			 * @param value The value to initialize elements with.
			 * @warning std::abort() is called if count > capacity().
			 * @complexity Linear in count, O(N).
			 */
			constexpr
			StaticVector (size_type count, const data_t & value) noexcept
			{
				if ( count > max_capacity )
				{
					std::abort();
				}

				for ( size_type index = 0; index < count; ++index )
				{
					new (this->data() + index) data_t(value);
				}

				m_size = count;
			}

			/**
			 * @brief Copy constructor. Constructs the container with a copy of the contents of `other`.
			 * @param other Another StaticVector to copy from.
			 * @complexity Linear in the size of other, O(N).
			 */
			StaticVector (const StaticVector & other)
			{
				for ( size_type index = 0; index < other.m_size; ++index )
				{
					new (this->data() + index) data_t(other[index]);
				}

				m_size = other.m_size;
			}

			/**
			 * @brief Move constructor. Constructs the container with the contents of `other`.
			 * @param other A StaticVector to move from. `other` is left in a valid but empty state.
			 * @complexity Linear in the size of other, O(N).
			 */
			StaticVector (StaticVector && other) noexcept
			{
				for ( size_type index = 0; index < other.m_size; ++index )
				{
					new (this->data() + index) data_t(std::move(other[index]));
				}

				m_size = other.m_size;
				other.m_size = 0;
			}

			/**
			 * @brief Constructs the container with the contents of an initializer list.
			 * @param initializer_list The initializer list to construct from.
			 * @warning std::abort() is called if count > capacity().
			 * @complexity Linear in the size of the initializer list, O(N).
			 */
			constexpr
			StaticVector (std::initializer_list< data_t > initializer_list)
			{
				if ( initializer_list.size() > max_capacity )
				{
					std::abort();
				}

				for ( const auto & item : initializer_list )
				{
					new (this->data() + m_size) data_t(item);

					++m_size;
				}
			}
#endif

			/**
			 * @brief Destructs the StaticVector, calling destructors for all contained elements.
			 * @complexity Linear, O(N).
			 */
			~StaticVector () noexcept(std::is_nothrow_destructible_v< data_t >)
			{
				this->clear();
			}

			/**
			 * @brief Copy assignment operator. Replaces the contents with a copy of the contents of `other`.
			 * @param other The StaticVector to copy from.
			 * @return *this
			 * @complexity Linear in the sizes of both containers, O(N).
			 */
			StaticVector &
			operator= (const StaticVector & other)
			{
				StaticVector temp{other};
				temp.swap(*this);

				return *this;
			}

			/**
			 * @brief Move assignment operator. Replaces the contents with those of `other`.
			 * @param other The StaticVector to move from. `other` is left in a valid but empty state.
			 * @return *this
			 * @complexity Linear in the sizes of both containers, O(N).
			 */
			StaticVector &
			operator= (StaticVector && other) noexcept
			{
				other.swap(*this);

				return *this;
			}

			/**
			 * @brief Accesses the element at a specific position with bound checking.
			 * @param pos The position of the element to access.
			 * @return Reference to the requested element.
			 * @throw std::out_of_range if pos >= size().
			 * @complexity Constant, O(1).
			 */
			[[nodiscard]]
			constexpr
			reference
			at (size_type pos)
			{
				if ( pos >= m_size )
				{
#if defined(__cpp_exceptions)
					throw std::out_of_range{"StaticVector::at: Position out of range!"};
#else
					std::abort();
#endif
				}

				return (*this)[pos];
			}

			/**
			 * @brief Accesses the element at a specific position with bounds checking (const version).
			 * @param pos The position of the element to access.
			 * @return Const reference to the requested element.
			 * @throw std::out_of_range if pos >= size().
			 * @complexity Constant, O(1).
			 */
			[[nodiscard]]
			constexpr
			const_reference
			at (size_type pos) const
			{
				if ( pos >= m_size )
				{
#if defined(__cpp_exceptions)
					throw std::out_of_range{"StaticVector::at: Position out of range!"};
#else
					std::abort();
#endif
				}

				return (*this)[pos];
			}

			/**
			 * @brief Accesses the element at a specific position without bound checking.
			 * @note Calling this with `pos >= size()` results in undefined behavior.
			 * @param pos The position of the element to access.
			 * @return Reference to the element.
			 * @complexity Constant, O(1).
			 */
			[[nodiscard]]
			constexpr
			reference
			operator[] (size_type pos) noexcept
			{
				return this->data()[pos];
			}

			/**
			 * @brief Accesses the element at a specific position without bound checking (const version).
			 * @note Calling this with `pos >= size()` results in undefined behavior.
			 * @param pos The position of the element to access.
			 * @return Const reference to the element.
			 * @complexity Constant, O(1).
			 */
			[[nodiscard]]
			constexpr
			const_reference
			operator[] (size_type pos) const noexcept
			{
				return this->data()[pos];
			}

			/**
			 * @brief Accesses the first element in the container.
			 * @note Calling this on an empty container results in undefined behavior.
			 * @return Reference to the first element.
			 * @complexity Constant, O(1).
			 */
			[[nodiscard]]
			constexpr
			reference
			front () noexcept
			{
				assert(!this->empty() && "StaticVector::front: Called on empty StaticVector!");

				return (*this)[0];
			}

			/**
			 * @brief Accesses the first element in the container (const version).
			 * @note Calling this on an empty container results in undefined behavior.
			 * @return Const reference to the first element.
			 * @complexity Constant, O(1).
			 */
			[[nodiscard]]
			constexpr
			const_reference
			front () const noexcept
			{
				assert(!this->empty() && "StaticVector::front: Called on empty StaticVector!");

				return (*this)[0];
			}

			/**
			 * @brief Accesses the last element in the container.
			 * @note Calling this on an empty container results in undefined behavior.
			 * @return Reference to the last element.
			 * @complexity Constant, O(1).
			 */
			[[nodiscard]]
			constexpr
			reference
			back () noexcept
			{
				assert(!this->empty() && "StaticVector::back: Called on empty StaticVector!");

				return (*this)[m_size - 1];
			}

			/**
			 * @brief Accesses the last element in the container (const version).
			 * @note Calling this on an empty container results in undefined behavior.
			 * @return Const reference to the last element.
			 * @complexity Constant, O(1).
			 */
			[[nodiscard]]
			constexpr
			const_reference
			back () const noexcept
			{
				assert(!this->empty() && "StaticVector::back: Called on empty StaticVector!");

				return (*this)[m_size - 1];
			}

			/**
			 * @brief Returns a pointer to the underlying contiguous storage.
			 * @return A pointer to the first element. For an empty container, this is valid but should not be dereferenced.
			 * @complexity Constant, O(1).
			 */
			[[nodiscard]]
			constexpr
			data_t *
			data () noexcept
			{
				return reinterpret_cast< data_t * >(m_data.data());
			}

			/**
			 * @brief Returns a pointer to the underlying contiguous storage (const version).
			 * @return A const pointer to the first element.
			 * @complexity Constant, O(1).
			 */
			[[nodiscard]]
			constexpr
			const data_t *
			data () const noexcept
			{
				return reinterpret_cast< const data_t * >(m_data.data());
			}

			/**
			 * @brief Returns an iterator to the first element of the StaticVector.
			 * @return Iterator to the first element.
			 */
			[[nodiscard]]
			constexpr
			iterator
			begin () noexcept
			{
				return this->data();
			}

			/**
			 * @brief Returns a const iterator to the first element of the StaticVector.
			 * @return Const iterator to the first element.
			 */
			[[nodiscard]]
			constexpr
			const_iterator
			begin () const noexcept
			{
				return this->data();
			}

			/**
			 * @brief Returns a const iterator to the first element of the StaticVector.
			 * @return Const iterator to the first element.
			 */
			[[nodiscard]]
			constexpr
			const_iterator
			cbegin () const noexcept
			{
				return this->begin();
			}

			/**
			 * @brief Returns an iterator to the element following the last element.
			 * @return Iterator to the element following the last element. This iterator acts as a placeholder; attempting to dereference it results in undefined behavior.
			 */
			[[nodiscard]]
			constexpr
			iterator
			end () noexcept
			{
				return data() + m_size;
			}

			/**
			 * @brief Returns a const iterator to the element following the last element.
			 * @return Const iterator to the element following the last element.
			 */
			[[nodiscard]]
			constexpr
			const_iterator
			end () const noexcept
			{
				return data() + m_size;
			}

			/**
			 * @brief Returns a const iterator to the element following the last element.
			 * @return Const iterator to the element following the last element.
			 */
			[[nodiscard]]
			constexpr
			const_iterator
			cend () const noexcept
			{
				return end();
			}

			/**
			 * @brief Returns a reverse iterator to the first element of the reversed StaticVector.
			 * @return Reverse iterator to the first element of the reversed StaticVector.
			 */
			[[nodiscard]]
			constexpr
			reverse_iterator
			rbegin () noexcept
			{
				return reverse_iterator(this->end());
			}

			/**
			 * @brief Returns a const reverse iterator to the first element of the reversed StaticVector.
			 * @return Const reverse iterator to the first element of the reversed StaticVector.
			 */
			[[nodiscard]]
			constexpr
			const_reverse_iterator
			rbegin () const noexcept
			{
				return const_reverse_iterator(this->end());
			}

			/**
			 * @brief Returns a const reverse iterator to the first element of the reversed StaticVector.
			 * @return Const reverse iterator to the first element of the reversed StaticVector.
			 */
			[[nodiscard]]
			constexpr
			const_reverse_iterator
			crbegin () const noexcept
			{
				return rbegin();
			}

			/**
			 * @brief Returns a reverse iterator to the element following the last element of the reversed StaticVector.
			 * @return Reverse iterator to the element following the last element of the reversed StaticVector.
			 */
			[[nodiscard]]
			constexpr
			reverse_iterator
			rend () noexcept
			{
				return reverse_iterator(this->begin());
			}

			/**
			 * @brief Returns a const reverse iterator to the element following the last element of the reversed StaticVector.
			 * @return Const reverse iterator to the element following the last element of the reversed StaticVector.
			 */
			[[nodiscard]]
			constexpr
			const_reverse_iterator
			rend () const noexcept
			{
				return const_reverse_iterator(this->begin());
			}

			/**
			 * @brief Returns a const reverse iterator to the element following the last element of the reversed StaticVector.
			 * @return Const reverse iterator to the element following the last element of the reversed StaticVector.
			 */
			[[nodiscard]]
			constexpr
			const_reverse_iterator
			crend () const noexcept
			{
				return rend();
			}

			/**
			 * @brief Checks if the container has no elements.
			 * @return `true` if the container is empty, `false` otherwise.
			 * @complexity Constant, O(1).
			 */
			[[nodiscard]]
			constexpr
			bool
			empty () const noexcept
			{
				return m_size == 0;
			}

			/**
			 * @brief Checks if the container is full.
			 * @return `true` if the container is full, `false` otherwise.
			 * @complexity Constant, O(1).
			 */
			[[nodiscard]]
			constexpr
			bool
			full () const noexcept
			{
				return m_size == max_capacity;
			}

			/**
			 * @brief Returns the number of elements in the container.
			 * @return The number of elements.
			 * @complexity Constant, O(1).
			 */
			[[nodiscard]]
			constexpr
			size_type
			size () const noexcept
			{
				return m_size;
			}

			/**
			 * @brief Returns the maximum number of elements the container can hold.
			 * @return The maximum number of elements.
			 * @complexity Constant, O(1).
			 */
			[[nodiscard]]
			constexpr
			size_type
			max_size () const noexcept
			{
				return max_capacity;
			}

			/**
			 * @brief Returns the maximum number of elements the container can hold.
			 * @return The maximum number of elements.
			 * @complexity Constant, O(1).
			 */
			[[nodiscard]]
			constexpr
			size_type
			capacity () const noexcept
			{
				return max_capacity;
			}

			/**
			 * @brief Exchanges the contents of the container with those of `other`.
			 * @param other The StaticVector to swap contents with.
			 * @complexity Linear in the size of the larger container, O(N).
			 * @note Does not invalidate iterators, they remain valid and point to the same elements which are now in the other container.
			 */
			void
			swap (StaticVector & other) noexcept(std::is_nothrow_move_constructible_v< data_t > && std::is_nothrow_swappable_v< data_t >)
			{
				size_type commonSize = std::min(m_size, other.m_size);

				for ( size_type index = 0; index < commonSize; ++index )
				{
					std::swap((*this)[index], other[index]);
				}

				if ( m_size > other.m_size )
				{
					for ( size_type index = commonSize; index < m_size; ++index )
					{
						new (other.data() + index) data_t(std::move((*this)[index]));
						(*this)[index].~data_t();
					}
				}
				else if ( other.m_size > m_size )
				{
					for ( size_type index = commonSize; index < other.m_size; ++index )
					{
						new (this->data() + index) data_t(std::move(other[index]));
						other[index].~data_t();
					}
				}

				std::swap(m_size, other.m_size);
			}

			/**
			 * @brief Exchanges the contents with `other` using a fast, bitwise copy.
			 * @note This operation is only available for trivially copyable types. Using it with other types will result in a compile-time error.
			 * @param other The StaticVector to swap contents with.
			 * @complexity Linear in capacity, O(Capacity).
			 */
			void
			quick_swap (StaticVector & other) noexcept requires (std::is_trivially_copyable_v< data_t >)
			{
				alignas(data_t) std::array< std::byte, sizeof(data_t) * max_capacity > temp_data;

				std::memcpy(temp_data.data(), m_data.data(), sizeof(data_t) * m_size);
				std::memcpy(m_data.data(), other.m_data.data(), sizeof(data_t) * other.m_size);
				std::memcpy(other.m_data.data(), temp_data.data(), sizeof(data_t) * m_size);

				std::swap(m_size, other.m_size);
			}

			/**
			 * @brief Resizes the container to contain `newSize` elements.
			 * @details If `newSize` is smaller than the current size, the content is reduced.
			 * If `newSize` is greater, new elements are default-constructed.
			 * @param newSize The new size of the container.
			 * @throw std::length_error If newSize > capacity().
			 * @complexity Linear in the difference between the current size and `newSize`.
			 */
			constexpr
			void
			resize (size_type newSize)
			{
				if ( newSize > max_capacity )
				{
#if defined(__cpp_exceptions)
					throw std::length_error{"StaticVector::resize: Capacity exceeded!"};
#else
					std::abort();
#endif
				}

				if ( newSize > m_size )
				{
#if defined(__cpp_exceptions)
					try
					{
						while ( m_size < newSize )
						{
							new (this->data() + m_size) data_t();
							++m_size;
						}
					}
					catch (...)
					{
						throw;
					}
#else
					for ( size_type index = m_size; index < newSize; ++index )
					{
						new (this->data() + index) data_t();
					}
					
					m_size = newSize;
#endif
				}
				else
				{
					size_type oldSize = m_size;
					m_size = newSize;

					for ( size_type index = newSize; index < oldSize; ++index )
					{
						if constexpr ( !std::is_trivially_destructible_v<data_t> )
						{
							this->data()[index].~data_t();
						}
					}
				}
			}

			/**
			 * @brief Resizes the container to contain `newSize` elements.
			 * @details If `newSize` is smaller than the current size, the content is reduced.
			 * If `newSize` is greater, new elements are initialized with a copy of `value`.
			 * @param newSize The new size of the container.
			 * @param value The value to initialize new elements with.
			 * @throw std::length_error If newSize > capacity().
			 * @complexity Linear in the difference between the current size and `newSize`.
			 */
			constexpr
			void
			resize (size_type newSize, const data_t & value)
			{
				if ( newSize > max_capacity )
				{
#if defined(__cpp_exceptions)
					throw std::length_error{"StaticVector::resize: Capacity exceeded!"};
#else
					std::abort();
#endif
				}

				if ( newSize > m_size )
				{
#if defined(__cpp_exceptions)
					try
					{
						while ( m_size < newSize )
						{
							new (this->data() + m_size) data_t(value);
							++m_size;
						}
					}
					catch (...)
					{
						throw;
					}
#else
					for ( size_type index = m_size; index < newSize; ++index )
					{
						new (this->data() + index) data_t(value);
					}

					m_size = newSize;
#endif
				}
				else
				{
					size_type oldSize = m_size;
					m_size = newSize;

					if constexpr ( !std::is_trivially_destructible_v< data_t > )
					{
						for ( size_type index = newSize; index < oldSize; ++index )
						{
							this->data()[index].~data_t();
						}
					}
				}
			}

			/**
			 * @brief Erases all elements from the container. After this call, size() returns zero.
			 * @complexity Linear, O(N).
			 */
			constexpr
			void
			clear () noexcept
			{
				if constexpr ( !std::is_trivially_destructible_v< data_t > )
				{
					for ( size_type index = 0; index < m_size; ++index )
					{
						this->data()[index].~data_t();
					}
				}

				m_size = 0;
			}

			/**
			 * @brief Appends an element constructed in-place to the end of the container.
			 * @param args Arguments to forward to the constructor of the element.
			 * @return A reference to the newly constructed element.
			 * @throw std::length_error if size() >= capacity().
			 * @note Provides a strong exception guarantee. No changes are made if the constructor throws.
			 * @complexity Constant, O(1).
			 */
			template< typename... Args >
			reference
			emplace_back (Args && ... args)
			{
				if ( m_size >= max_capacity )
				{
#if defined(__cpp_exceptions)
					throw std::length_error{"StaticVector::emplace_back: Capacity exceeded!"};
#else
					std::cerr << "StaticVector::emplace_back: Capacity (" << max_capacity << ") exceeded!" << std::endl;

					std::abort();
#endif
				}

				/* WARNING: Don't use data_t{} */
				new (this->data() + m_size) data_t(std::forward< Args >(args)...);

				return this->data()[m_size++];
			}

			/**
			 * @brief Appends a copy of the given element to the end of the container.
			 * @param value The value of the element to append.
			 * @complexity Constant, O(1).
			 */
			void
			push_back (const data_t & value)
			{
				this->emplace_back(value);
			}

			/**
			 * @brief Appends a moved element to the end of the container.
			 * @param value The element to move from.
			 * @complexity Constant, O(1).
			 */
			void
			push_back (data_t && value)
			{
				this->emplace_back(std::move(value));
			}

			/**
			 * @brief Removes the last element of the container.
			 * @note Calling pop_back on an empty container is undefined behavior.
			 * @complexity Constant, O(1).
			 */
			constexpr
			void
			pop_back () noexcept
			{
				if ( m_size > 0 )
				{
					--m_size;

					if constexpr (!std::is_trivially_destructible_v< data_t >)
					{
						this->data()[m_size].~data_t();
					}
				}
			}

		/**
		 * @brief Erases the element at the specified position.
		 * @param pos Iterator to the element to remove.
		 * @return Iterator following the last removed element.
		 * @note Calling erase with an invalid iterator results in undefined behavior.
		 * @complexity Linear in the distance between pos and the end, O(N).
		 */
		constexpr
		iterator
		erase (iterator pos) noexcept(std::is_nothrow_move_assignable_v< data_t >)
		{
			assert(pos >= this->begin() && pos < this->end() && "StaticVector::erase: Iterator out of range!");

			if ( pos < this->end() - 1 )
			{
				std::move(pos + 1, this->end(), pos);
			}

			--m_size;

			if constexpr (!std::is_trivially_destructible_v< data_t >)
			{
				this->data()[m_size].~data_t();
			}

			return pos;
		}

		/**
		 * @brief Erases the elements in the range [first, last).
		 * @param first Iterator to the first element to remove.
		 * @param last Iterator to the element following the last element to remove.
		 * @return Iterator following the last removed element.
		 * @note Calling erase with invalid iterators results in undefined behavior.
		 * @complexity Linear in the distance between first and the end, O(N).
		 */
		constexpr
		iterator
		erase (iterator first, iterator last) noexcept(std::is_nothrow_move_assignable_v< data_t >)
		{
			assert(first >= this->begin() && first <= this->end() && "StaticVector::erase: First iterator out of range!");
			assert(last >= first && last <= this->end() && "StaticVector::erase: Last iterator out of range!");

			if ( first == last )
			{
				return first;
			}

			size_type numToErase = static_cast< size_type >(last - first);

			if ( last < this->end() )
			{
				std::move(last, this->end(), first);
			}

			size_type oldSize = m_size;
			m_size -= numToErase;

			if constexpr (!std::is_trivially_destructible_v< data_t >)
			{
				for ( size_type index = m_size; index < oldSize; ++index )
				{
					this->data()[index].~data_t();
				}
			}

			return first;
		}

		private:

			alignas(data_t) std::array< std::byte, sizeof(data_t) * max_capacity > m_data{};
			size_t m_size{0};
	};

	/**
	 * @brief Performs a three-way comparison between two StaticVector instances.
	 * @details This operator enables all other comparison operators (`==`, `!=`, `<`, `>`, `<=`, `>=`).
	 * The comparison is done lexicographically.
	 * @tparam data_t The element type.
	 * @tparam max_capacity The capacity of the vectors.
	 * @param lhs The left-hand side StaticVector.
	 * @param rhs The right-hand side StaticVector.
	 * @return A std::strong_ordering value indicating the result of the comparison.
	 */
	template< typename data_t, std::size_t max_capacity >
	[[nodiscard]]
	constexpr
	auto
	operator<=> (const StaticVector< data_t, max_capacity > & lhs, const StaticVector< data_t, max_capacity > & rhs)
	{
#if IS_MACOS && __MAC_OS_X_VERSION_MAX_ALLOWED < 150000
		const std::size_t min_size = std::min(lhs.size(), rhs.size());

		for ( std::size_t i = 0; i < min_size; ++i )
		{
			if ( auto cmp = lhs[i] <=> rhs[i]; cmp != 0 )
			{
				return cmp;
			}
		}

		return lhs.size() <=> rhs.size();
#else
		return std::lexicographical_compare_three_way(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
#endif
	}

	/**
	 * @brief Performs an equality comparison between two StaticVector instances.
	 * @details This operator is provided for types that do not support three-way comparison (like std::shared_ptr).
	 * The compiler will automatically generate operator!= from this.
	 * @param lhs The left-hand side StaticVector.
	 * @param rhs The right-hand side StaticVector.
	 * @return `true` if the contents are equal, `false` otherwise.
	 */
	template< typename data_t, std::size_t max_capacity >
	[[nodiscard]]
	constexpr
	bool
	operator== (const StaticVector< data_t, max_capacity > & lhs, const StaticVector< data_t, max_capacity > & rhs)
	{
		if ( lhs.size() != rhs.size() )
		{
			return false;
		}

		return std::equal(lhs.begin(), lhs.end(), rhs.begin());
	}
}
