/*
 * src/Graphics/VertexBufferFormatManager.hpp
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
#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

/* Local inclusions for inheritances. */
#include "ServiceInterface.hpp"

/* Local inclusions for usages. */
#include "Mutex.hpp"
#include "VertexBufferFormat.hpp"

namespace EmEn
{
	namespace Graphics::Geometry
	{
		class Interface;
	}

	namespace Saphir
	{
		class VertexShader;
	}
}

namespace EmEn::Graphics
{
	/**
	 * @brief The vertex buffer format manager service class.
	 * @extends EmEn::ServiceInterface This is a service.
	 */
	class EMEN_API VertexBufferFormatManager final : public ServiceInterface
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"VertexBufferFormatManagerService"};

			/**
			 * @brief Constructs a vertex buffer format manager.
			 */
			VertexBufferFormatManager () noexcept
				: ServiceInterface{ClassId}
			{

			}

			/**
			 * @brief Enables the print into the console of generated format. (DEBUG).
			 * @param state The state.
			 */
			void
			enablePrintGeneratedFormat (bool state) noexcept
			{
				m_printGeneratedFormat = state;
			}

			/**
			 * @brief Creates or returns an existing vertex buffer format from a vertex shader and parameters.
			 * @warning The output vertex format can be nullptr!
			 * @param vertexShader A reference to a vertex shader.
			 * @param topology The primitive topology of the geometry.
			 * @param geometryFlagBits The geometry flag bits describing the vertex attributes the geometry provides.
			 * @return std::shared_ptr< VertexBufferFormat >
			 */
			[[nodiscard]]
			std::shared_ptr< VertexBufferFormat > getVertexBufferFormat (const Saphir::VertexShader & vertexShader, Topology topology, uint32_t geometryFlagBits) noexcept;

			/**
			 * @brief Creates or returns an existing vertex buffer format from a combination of a geometry and a vertex shader.
			 * @warning The output vertex format can be nullptr!
			 * @param vertexShader A reference to a vertex shader.
			 * @param geometry A reference to a geometry interface.
			 * @return std::shared_ptr< VertexBufferFormat >
			 */
			[[nodiscard]]
			std::shared_ptr< VertexBufferFormat > getVertexBufferFormat (const Saphir::VertexShader & vertexShader, const Geometry::Interface & geometry) noexcept;

			/**
			 * @brief Returns a string showing a VBO usage.
			 * @param geometry A reference to the geometry using the VBO.
			 * @param vertexBufferFormat A reference to the vertex buffer format using the VBO.
			 * @return std::string
			 */
			[[nodiscard]]
			static std::string getVertexBufferObjectUsage (const Geometry::Interface & geometry, const VertexBufferFormat & vertexBufferFormat) noexcept;

			/**
			 * @brief Returns a string showing a VBO usage.
			 * @param geometryFlagBits The geometry flag bits.
			 * @param vertexBufferFormat A reference to the vertex buffer format using the VBO.
			 * @return std::string
			 */
			[[nodiscard]]
			static std::string getVertexBufferObjectUsage (uint32_t geometryFlagBits, const VertexBufferFormat & vertexBufferFormat) noexcept;

		private:

			/** @copydoc EmEn::ServiceInterface::onInitialize() */
			bool onInitialize () noexcept override;

			/** @copydoc EmEn::ServiceInterface::onTerminate() */
			bool onTerminate () noexcept override;

			/**
			 * @brief Begins to bind declarations to a new vertex buffer format.
			 * @param binding The binding point.
			 * @return bool
			 */
			bool beginBinding (uint32_t binding) noexcept EMEN_REQUIRES(m_listAccess);

			/**
			 * @brief Declares a vertex attribute.
			 * @param attribute The vertex attribute type.
			 * @return bool
			 */
			bool declareAttribute (VertexAttributeType attribute) noexcept EMEN_REQUIRES(m_listAccess);

			/**
			 * @brief Declares a jump over an existing vertex attribute.
			 * @param attribute The vertex attribute type.
			 */
			void declareJump (VertexAttributeType attribute) noexcept EMEN_REQUIRES(m_listAccess);

			/**
			 * @brief Finishes the new vertex buffer format creation.
			 * @param topology The geometry primitive to use.
			 * @param bufferFlags The vertex buffer object flags.
			 * @return bool
			 */
			[[nodiscard]] 
			bool endBinding (Topology topology, uint32_t bufferFlags) const noexcept EMEN_REQUIRES(m_listAccess);

			/**
			 * @brief Resets the members that build a new vertex buffer format.
			 */
			void
			resetBuildingParameters () noexcept EMEN_REQUIRES(m_listAccess)
			{
				m_stagingVertexBufferFormat.reset();
				m_currentBinding = 0;
				m_bindingOffset = 0;
				m_bindingElementCount = 0;
			}

			Base::Mutex m_listAccess;
			std::vector< std::shared_ptr< VertexBufferFormat > > m_vertexBufferFormats EMEN_GUARDED_BY(m_listAccess);
			/* Building parameters */
			std::shared_ptr< VertexBufferFormat > m_stagingVertexBufferFormat EMEN_GUARDED_BY(m_listAccess);
			uint32_t m_currentBinding EMEN_GUARDED_BY(m_listAccess){0};
			uint32_t m_bindingOffset EMEN_GUARDED_BY(m_listAccess){0};
			size_t m_bindingElementCount EMEN_GUARDED_BY(m_listAccess){0};
			bool m_printGeneratedFormat{false};
	};
}
