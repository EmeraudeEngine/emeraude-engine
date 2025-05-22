
#pragma once

/* STL inclusions. */
#include <cstdint>
#include <memory>

/* Local inclusions for usage. */
#include "Graphics/Types.hpp"
#include "Saphir/Program.hpp"

namespace EmEn::Graphics::RenderableInstance
{
	/**
	 * @brief Interface for all render target programs associated with a renderable instance.
	 */
	class RenderTargetProgramsInterface
	{
		public:

			/**
			 * @brief Destructs a render target program interface.
			 */
			virtual ~RenderTargetProgramsInterface () = default;

			/**
			 * @brief Sets a program for a specific render pass and a renderable instance layer.
			 * @param renderPassType The type of render pass.
			 * @param layer The geometry layer.
			 * @param program A reference to a program smart pointer.
			 * @return bool
			 */
			virtual bool setRenderProgram (RenderPassType renderPassType, uint32_t layer, const std::shared_ptr< Saphir::Program > & program) = 0;

			/**
			 * @brief Sets the shadow casting program.
			 * @param layer The geometry layer.
			 * @param program A reference to a program smart pointer.
			 * @return bool
			 */
			virtual bool setShadowCastingProgram (uint32_t layer, const std::shared_ptr< Saphir::Program > & program) noexcept = 0;

			/**
			 * @brief Sets a TBN space program for a renderable instance layer.
			 * @param layer The geometry layer.
			 * @param program A reference to a program smart pointer.
			 * @return bool
			 */
			virtual bool setTBNSpaceProgram (uint32_t layer, const std::shared_ptr< Saphir::Program > & program) noexcept = 0;

			/**
			 * @return Sets these render target programs ready to render the renderable instance.
			 * @return void
			 */
			virtual void setReadyToRender () noexcept = 0;

			/**
			 * @brief Returns whether this render target to program is ready to render the renderable instance.
			 * @return bool
			 */
			[[nodiscard]]
			virtual bool isReadyToRender () const noexcept = 0;

			/**
			 * @return Sets these render target programs ready to cast shadows.
			 * @return void
			 */
			virtual void setReadyToCastShadows () noexcept = 0;

			/**
			 * @brief Returns whether this render target to program is ready to cast shadows.
			 * @return bool
			 */
			[[nodiscard]]
			virtual bool isReadyToCastShadows () const noexcept = 0;

			/**
			 * @breif Returns the number of layers to render the renderable instance.
			 * @return uint32_t
			 */
			[[nodiscard]]
			virtual uint32_t layerCount () const noexcept = 0;

			/**
			 * @brief Returns the program to render a specific layer.
			 * @param renderPassType The type of render pass.
			 * @param layer The geometry layer.
			 * @return std::shared_ptr< Saphir::Program >
			 */
			[[nodiscard]]
			virtual std::shared_ptr< Saphir::Program > renderProgram (RenderPassType renderPassType, uint32_t layer) const noexcept = 0;

			/**
			 * @brief Returns the program to render the TBN space for a specific layer.
			 * @param layer The geometry layer.
			 * @return std::shared_ptr< Saphir::Program >
			 */
			[[nodiscard]]
			virtual std::shared_ptr< Saphir::Program > TBNSpaceProgram (uint32_t layer) const noexcept = 0;

			/**
			 * @brief Returns the program to cast shadows for a specific layer.
			 * @param layer The geometry layer.
			 * @return std::shared_ptr< Saphir::Program >
			 */
			[[nodiscard]]
			virtual std::shared_ptr< Saphir::Program > shadowCastingProgram (uint32_t layer) const noexcept = 0;

		protected:

			/**
			 * @brief Constructs a render target program interface.
			 */
			RenderTargetProgramsInterface () = default;
	};
}
