#pragma once

/* Local inclusions for inheritances. */
#include "Libs/NameableTrait.hpp"
#include "Libs/ObservableTrait.hpp"
#include "Types.hpp"

namespace EmEn::Resources
{
	/**
	 * @brief The common interface for all resource containers.
	 * @extends EmEn::Libs::NameableTrait A container is nameable.
	 * @extends EmEn::Libs::ObservableTrait A container can be observed.
	 */
	class ContainerInterface : public Libs::NameableTrait, public Libs::ObservableTrait
	{
		public:

			/**
			 * @brief Destructs the container interface.
			 */
			~ContainerInterface () override = default;

			/**
			 * @brief Sets the verbosity state for the container.
			 * @param state The state.
			 * @return void
			 */
			virtual void setVerbosity (bool state) noexcept = 0;

			/**
			 * @brief Initializes the container.
			 * @return bool
			 */
			virtual bool initialize () noexcept = 0;

			/**
			 * @brief Destroys the container.
			 * @return bool
			 */
			virtual bool terminate () noexcept = 0;

			/**
			 * @brief Returns the total memory consumed by loaded resources in bytes from the container.
			 * @return size_t
			 */
			[[nodiscard]]
			virtual size_t memoryOccupied () const noexcept = 0;

			/**
			 * @brief Returns the total memory consumed by loaded, but unused resources in bytes from the container.
			 * @return size_t
			 */
			[[nodiscard]]
			virtual size_t unusedMemoryOccupied () const noexcept = 0;

			/**
			 * @brief Clean up every unused resource and returns the number of removed resources.
			 * @return size_t
			 */
			virtual size_t unloadUnusedResources () noexcept = 0;

			/**
			 * @brief Returns the complexity of the resource.
			 * @return DepComplexity
			 */
			[[nodiscard]]
			virtual DepComplexity complexity () const noexcept = 0;

		protected:

			/**
			 * @brief Constructs a container interface.
			 * @param name A reference to a string.
			 */
			explicit
			ContainerInterface (const std::string & name) noexcept
				: NameableTrait{name}
			{

			}
	};
}

