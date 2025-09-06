#pragma once

/* STL inclusions. */
#include <typeindex>

/* Forward declarations. */
namespace Json
{
	class Value;
}

namespace EmEn
{
	namespace Resources
	{
		class ContainerInterface;

		template< typename resource_t >
		class Container;
	}

	namespace Graphics
	{
		class Renderer;
	}

	class FileSystem;
}

namespace EmEn::Resources
{
	/**
	 * @brief Provides services to loading resources.
	 */
	class ServiceProvider
	{
		public:

			/**
			 * @brief Destructs the service provider.
			 */
			virtual ~ServiceProvider() = default;

			/**
			 * @brief Returns access to the file system.
			 * @return const FileSystem &
			 */
			[[nodiscard]]
			const FileSystem &
			fileSystem () const noexcept
			{
				return m_fileSystem;
			}

			/**
			 * @brief Returns access to the graphics renderer.
			 * @return Graphics::Renderer &
			 */
			[[nodiscard]]
			Graphics::Renderer &
			graphicsRenderer () const noexcept
			{
				return m_graphicsRenderer;
			}

			/**
			 * @brief Returns a reference to the container for a specific resource type.
			 * @tparam resource_t The type of the resource (e.g., SoundResource).
			 * @return Container< resource_t > *
			 */
		    template< typename resource_t >
			[[nodiscard]]
		    Container< resource_t > *
		    container () noexcept
			{
		        const std::type_index typeId = typeid(resource_t);

		        ContainerInterface * nonTypedContainer = this->getContainerInternal(typeId);

		        return static_cast< Container< resource_t > * >(nonTypedContainer);
		    }

			/**
			 * @brief Returns a reference to the container for a specific resource type.
			 * @tparam resource_t The type of the resource (e.g., SoundResource).
			 * @return const Container< resource_t > *
			 */
			template< typename resource_t >
			[[nodiscard]]
			const Container< resource_t > *
			container () const noexcept
		    {
		    	const std::type_index typeId = typeid(resource_t);

		    	const ContainerInterface * nonTypedContainer = this->getContainerInternal(typeId);

		    	return static_cast< const Container< resource_t > * >(nonTypedContainer);
		    }

			/**
			 * @brief Updates resource store from another resource JSON definition.
			 * @param root The resource JSON object.
			 * @return bool
			 */
			virtual bool update (const Json::Value & root) noexcept = 0;

		protected:

			/**
			 * @brief Constructs a service provider.
			 * @param fileSystem A reference to the filesystem.
			 * @param graphicsRenderer A reference to the graphics renderer.
			 */
			ServiceProvider (const FileSystem & fileSystem, Graphics::Renderer & graphicsRenderer)
				: m_fileSystem{fileSystem},
				m_graphicsRenderer{graphicsRenderer}
			{

			}

			/**
			 *
			 * @param typeIndex
			 * @return ContainerInterface *
			 */
			[[nodiscard]]
		    virtual ContainerInterface * getContainerInternal (const std::type_index & typeIndex) noexcept = 0;

			/**
			 *
			 * @param typeIndex
			 * @return ContainerInterface *
			 */
			[[nodiscard]]
			virtual const ContainerInterface * getContainerInternal (const std::type_index & typeIndex) const noexcept = 0;

		private:

			const FileSystem & m_fileSystem;
			Graphics::Renderer & m_graphicsRenderer;
	};
}
