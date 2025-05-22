/*
 * src/Scenes/Component/MultipleVisuals.hpp
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

#pragma once

/* STL inclusions. */
#include <any>
#include <memory>
#include <string>
#include <vector>

/* Local inclusions for inheritances. */
#include "Abstract.hpp"
#include "Libs/ObserverTrait.hpp"

/* Local inclusions for usages. */
#include "Graphics/RenderableInstance/Multiple.hpp"

namespace EmEn::Scenes::Component
{
	/**
	 * @brief Defines a renderable instance suitable for the scene node tree.
	 * @extends EmEn::Scenes::Component::Abstract The base class for each entity component.
	 * @extends EmEn::Libs::ObserverTrait This class must dispatch modifications from renderable instance to the entity.
	 */
	class MultipleVisuals final : public Abstract, public Libs::ObserverTrait
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"MultipleVisuals"};

			/**
			 * @brief Constructs a multiple visuals component.
			 * @param name The name of the renderable instance.
			 * @param parentEntity A reference to the parent entity.
			 * @param renderable A reference to a renderable smart pointer.
			 * @param coordinates A list of sub-coordinates.
			 */
			MultipleVisuals (const std::string & name, const AbstractEntity & parentEntity, const std::shared_ptr< Graphics::Renderable::Interface > & renderable, const std::vector< Libs::Math::CartesianFrame< float > > & coordinates) noexcept;

			/** @copydoc EmEn::Scenes::Component::Abstract::getRenderableInstance() const */
			[[nodiscard]]
			std::shared_ptr< Graphics::RenderableInstance::Abstract >
			getRenderableInstance () const noexcept override
			{
				return m_renderableInstance;
			}

			/** @copydoc EmEn::Scenes::Component::Abstract::getComponentType() */
			[[nodiscard]]
			const char *
			getComponentType () const noexcept override
			{
				return ClassId;
			}

			/** @copydoc EmEn::Scenes::Component::Abstract::isComponent() */
			[[nodiscard]]
			bool
			isComponent (const char * classID) const noexcept override
			{
				return strcmp(ClassId, classID) == 0;
			}

			/** @copydoc EmEn::Scenes::Component::Abstract::boundingBox() const */
			[[nodiscard]]
			const Libs::Math::Space3D::AACuboid< float > &
			boundingBox () const noexcept override
			{
				return m_renderableInstance->renderable()->boundingBox();
			}

			/** @copydoc EmEn::Scenes::Component::Abstract::boundingSphere() const */
			[[nodiscard]]
			const Libs::Math::Space3D::Sphere< float > &
			boundingSphere () const noexcept override
			{
				return m_renderableInstance->renderable()->boundingSphere();
			}

			/** @copydoc EmEn::Scenes::Component::Abstract::move() */
			void move (const Libs::Math::CartesianFrame< float > & worldCoordinates) noexcept override;

			/** @copydoc EmEn::Scenes::Component::Abstract::processLogics() */
			void processLogics (const Scene & scene) noexcept override;

			/** @copydoc EmEn::Scenes::Component::Abstract::shouldBeRemoved() */
			[[nodiscard]]
			bool
			shouldBeRemoved () const noexcept override
			{
				return m_renderableInstance->isBroken();
			}

		private:

			/** @copydoc EmEn::Animations::AnimatableInterface::playAnimation() */
			bool playAnimation (uint8_t animationID, const Libs::Variant & value, size_t cycle) noexcept override;

			/** @copydoc EmEn::Libs::ObserverTrait::onNotification() */
			[[nodiscard]]
			bool onNotification (const ObservableTrait * observable, int notificationCode, const std::any & data) noexcept override;

			std::shared_ptr< Graphics::RenderableInstance::Multiple > m_renderableInstance;
			std::vector< Libs::Math::CartesianFrame< float > > m_coordinates;
	};
}
