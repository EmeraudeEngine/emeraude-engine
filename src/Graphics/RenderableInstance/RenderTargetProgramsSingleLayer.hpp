#pragma once

/* STL inclusions. */
#include <array>

/* Local inclusions for inheritances. */
#include "RenderTargetProgramsInterface.hpp"

/* Local inclusions for usages. */
#include "Tracer.hpp"

namespace EmEn::Graphics::RenderableInstance
{
	/**
	 * @brief This class holds main rendering programs for a renderable instance for a single layer mesh.
	 * @extends EmEn::Graphics::RenderableInstance::RenderTargetProgramsInterface This is a render target program.
	 */
	class RenderTargetProgramsSingleLayer final : public RenderTargetProgramsInterface
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"RenderTargetProgramsSingleLayer"};

			/**
			 * @brief Constructs a render target program for the main rendering.
			 */
			RenderTargetProgramsSingleLayer () noexcept = default;

			/** @copydoc EmEn::Graphics::RenderableInstance::RenderTargetProgramsInterface::setRenderProgram() */
			bool
			setRenderProgram (RenderPassType renderPassType, uint32_t layer, const std::shared_ptr< Saphir::Program > & program) override
			{
				if ( layer > 0 )
				{
					Tracer::error(ClassId, "The parameter layer is not available with this render target program version (setRenderProgram()) !");

					return false;
				}

				m_renderPrograms[static_cast< uint32_t >(renderPassType)] = program;

				return true;
			}

			/** @copydoc EmEn::Graphics::RenderableInstance::RenderTargetProgramsInterface::setShadowCastingProgram() */
			bool
			setShadowCastingProgram (uint32_t layer, const std::shared_ptr< Saphir::Program > & program) noexcept override
			{
				if ( layer > 0 )
				{
					Tracer::error(ClassId, "The parameter layer is not available with this render target program version (setShadowCastingProgram()) !");

					return false;
				}

				m_shadowCastingProgram = program;

				return true;
			}

			/** @copydoc EmEn::Graphics::RenderableInstance::RenderTargetProgramsInterface::setTBNSpaceProgram() */
			bool
			setTBNSpaceProgram (uint32_t layer, const std::shared_ptr< Saphir::Program > & program) noexcept override
			{
				if ( layer > 0 )
				{
					Tracer::error(ClassId, "The parameter layer is not available with this render target program version (setTBNSpaceProgram()) !");

					return false;
				}

				m_TBNSpaceProgram = program;

				return true;
			}

			/** @copydoc EmEn::Graphics::RenderableInstance::RenderTargetProgramsInterface::setReadyToRender() */
			void
			setReadyToRender () noexcept override
			{
				m_isReadyToRender = true;
			}

			/** @copydoc EmEn::Graphics::RenderableInstance::RenderTargetProgramsInterface::isReadyToRender() const */
			[[nodiscard]]
			bool
			isReadyToRender () const noexcept override
			{
				return m_isReadyToRender;
			}

			/** @copydoc EmEn::Graphics::RenderableInstance::RenderTargetProgramsInterface::setReadyToCastShadows() */
			void
			setReadyToCastShadows () noexcept override
			{
				m_isReadyToCastShadows = true;
			}

			/** @copydoc EmEn::Graphics::RenderableInstance::RenderTargetProgramsInterface::isReadyToCastShadows() const */
			[[nodiscard]]
			bool
			isReadyToCastShadows () const noexcept override
			{
				return m_isReadyToCastShadows;
			}

			/** @copydoc EmEn::Graphics::RenderableInstance::RenderTargetProgramsInterface::layerCount() const */
			[[nodiscard]]
			uint32_t
			layerCount () const noexcept override
			{
				return 1U;
			}

			/** @copydoc EmEn::Graphics::RenderableInstance::RenderTargetProgramsInterface::renderProgram() const */
			[[nodiscard]]
			std::shared_ptr< Saphir::Program >
			renderProgram (RenderPassType renderPassType, uint32_t /*layer*/) const noexcept override
			{
				return m_renderPrograms[static_cast< uint32_t >(renderPassType)];
			}

			/** @copydoc EmEn::Graphics::RenderableInstance::RenderTargetProgramsInterface::TBNSpaceProgram() const */
			[[nodiscard]]
			std::shared_ptr< Saphir::Program >
			TBNSpaceProgram (uint32_t /*layer*/) const noexcept override
			{
				return m_TBNSpaceProgram;
			}

			/** @copydoc EmEn::Graphics::RenderableInstance::RenderTargetProgramsInterface::shadowCastingProgram() const */
			[[nodiscard]]
			std::shared_ptr< Saphir::Program >
			shadowCastingProgram (uint32_t /*layer*/) const noexcept override
			{
				return m_shadowCastingProgram;
			}

		private:

			std::array< std::shared_ptr< Saphir::Program >, 8 > m_renderPrograms{
				nullptr, // SimplePass
				nullptr, // AmbientPass
				nullptr, // DirectionalLightPass
				nullptr, // DirectionalLightPassNoShadow
				nullptr, // PointLightPass
				nullptr, // PointLightPassNoShadow
				nullptr, // SpotLightPass
				nullptr // SpotLightPassNoShadow
			};
			std::shared_ptr< Saphir::Program > m_shadowCastingProgram;
			std::shared_ptr< Saphir::Program  > m_TBNSpaceProgram;
			bool m_isReadyToRender{false};
			bool m_isReadyToCastShadows{false};
	};
}
