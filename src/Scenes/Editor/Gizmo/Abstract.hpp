/*
 * src/Scenes/Editor/Gizmo/Abstract.hpp
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
#include <cstdint>
#include <memory>

/* Local inclusions for usages. */
#include "Libs/Math/CartesianFrame.hpp"
#include "Libs/Math/Space3D/Segment.hpp"

/* Forward declarations. */
namespace EmEn
{
	namespace Resources
	{
		class Manager;
	}

	namespace Graphics
	{
		class Renderer;
		class ViewMatricesInterface;

		namespace Geometry
		{
			class Interface;
		}

		namespace RenderTarget
		{
			class Abstract;
		}
	}

	namespace Vulkan
	{
		class CommandBuffer;
	}

	namespace Saphir
	{
		class Program;
	}
}

namespace EmEn::Scenes::Editor::Gizmo
{
	/** @brief Identifies which axis of a gizmo was hit by a pick ray. */
	enum class AxisID : uint8_t
	{
		None,
		X,
		Y,
		Z,
		All
	};

	/**
	 * @brief Abstract base class for editor gizmos (translate, rotate, scale).
	 *
	 * A gizmo is a standalone visual tool rendered on top of the scene (depth test disabled)
	 * at constant screen size regardless of camera distance. It manages its own geometry,
	 * shader program, and hit-test zones. It does NOT live in the scene graph.
	 */
	class Abstract
	{
		public:

			/**
			 * @brief Destructs the gizmo.
			 */
			virtual ~Abstract () = default;

			/** @brief Deleted copy/move. */
			Abstract (const Abstract &) noexcept = delete;
			Abstract (Abstract &&) noexcept = delete;
			Abstract & operator= (const Abstract &) noexcept = delete;
			Abstract & operator= (Abstract &&) noexcept = delete;

			/**
			 * @brief Creates the gizmo GPU resources (geometry, shader program, pipeline).
			 * @param renderer A reference to the graphics renderer.
			 * @param resourceManager A reference to the resource manager.
			 * @param renderTarget A shared pointer to the render target for pipeline compatibility.
			 * @return bool True if creation succeeded.
			 */
			[[nodiscard]]
			virtual bool create (Graphics::Renderer & renderer, Resources::Manager & resourceManager, const std::shared_ptr< const Graphics::RenderTarget::Abstract > & renderTarget) noexcept = 0;

			/**
			 * @brief Destroys the gizmo GPU resources.
			 * @return void
			 */
			virtual void destroy () noexcept;

			/**
			 * @brief Tests if a world-space ray hits one of the gizmo axes.
			 * @param ray The pick ray in world space.
			 * @return AxisID The axis that was hit, or AxisID::None.
			 */
			[[nodiscard]]
			virtual AxisID hitTest (const Libs::Math::Space3D::Segment< float > & ray) const noexcept = 0;

			/**
			 * @brief Records the gizmo draw commands into the command buffer.
			 * @param commandBuffer The active command buffer.
			 * @param viewMatrices The current view matrices for MVP computation.
			 * @return void
			 */
			virtual void render (const Vulkan::CommandBuffer & commandBuffer, const Graphics::ViewMatricesInterface & viewMatrices) const noexcept = 0;

			/**
			 * @brief Updates the gizmo scale factor to maintain constant screen size.
			 * @param cameraPosition The camera world position.
			 * @param fieldOfView The camera vertical field of view in radians.
			 * @return void
			 */
			void updateScreenScale (const Libs::Math::Vector< 3, float > & cameraPosition, float fieldOfView, float screenRatio = DefaultScreenRatio) noexcept;

			/**
			 * @brief Sets the world position and orientation of the gizmo.
			 * @param frame The CartesianFrame of the entity the gizmo is attached to.
			 * @return void
			 */
			void
			setWorldFrame (const Libs::Math::CartesianFrame< float > & frame) noexcept
			{
				m_worldFrame = frame;
			}

			/**
			 * @brief Returns the current world frame of the gizmo.
			 * @return const Libs::Math::CartesianFrame< float > &
			 */
			[[nodiscard]]
			const Libs::Math::CartesianFrame< float > &
			worldFrame () const noexcept
			{
				return m_worldFrame;
			}

			/**
			 * @brief Sets the highlighted axis (for hover feedback).
			 * @param axis The axis to highlight.
			 * @return void
			 */
			void
			setHighlightedAxis (AxisID axis) noexcept
			{
				m_highlightedAxis = axis;
			}

			/**
			 * @brief Returns the currently highlighted axis.
			 * @return AxisID
			 */
			[[nodiscard]]
			AxisID
			highlightedAxis () const noexcept
			{
				return m_highlightedAxis;
			}

			/**
			 * @brief Returns the current screen-space scale factor.
			 * @return float
			 */
			[[nodiscard]]
			float
			screenScale () const noexcept
			{
				return m_screenScale;
			}

			/**
			 * @brief Returns whether the gizmo GPU resources are created.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isCreated () const noexcept
			{
				return m_created;
			}

			/** @brief Default screen ratio for gizmo size (fraction of viewport height). */
			static constexpr float DefaultScreenRatio{0.025F};

		protected:

			/**
			 * @brief Constructs the gizmo base.
			 */
			Abstract () noexcept = default;

			/** @brief The shader program for this gizmo. */
			std::shared_ptr< Saphir::Program > m_program;

			/** @brief The geometry (VBO/IBO) for this gizmo. */
			std::shared_ptr< Graphics::Geometry::Interface > m_geometry;

			/** @brief The gizmo world-space position and orientation. */
			Libs::Math::CartesianFrame< float > m_worldFrame;

			/** @brief Current scale factor for constant screen size. */
			float m_screenScale{1.0F};

			/** @brief The axis currently under the cursor. */
			AxisID m_highlightedAxis{AxisID::None};

			/** @brief Whether GPU resources are created. */
			bool m_created{false};
	};
}
