/*
 * src/Scenes/Component/CloudVolume.cpp
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

#include "CloudVolume.hpp"

/* Local inclusions. */
#include "Graphics/CloudShapeResource.hpp"
#include "Scenes/BindlessTextureSet.hpp"
#include "Scenes/Scene.hpp"
#include "Tracer.hpp"

namespace EmEn::Scenes::Component
{
	using namespace Base;
	using namespace Base::Math;

	constexpr auto TracerTag{"CloudVolume"};

	CloudVolume::CloudVolume (const std::string & componentName, const AbstractEntity & parentEntity, const std::shared_ptr< Graphics::CloudShapeResource > & shape, const Vector< 3, float > & halfExtents) noexcept
		: CloudVolume{componentName, parentEntity, shape, halfExtents, Look{}}
	{

	}

	CloudVolume::CloudVolume (const std::string & componentName, const AbstractEntity & parentEntity, const std::shared_ptr< Graphics::CloudShapeResource > & shape, const Vector< 3, float > & halfExtents, const Look & look) noexcept
		: Abstract{componentName, parentEntity},
		m_shape{shape},
		m_look{look},
		m_halfExtents{halfExtents}
	{
		m_boundingBox.set(m_halfExtents, -m_halfExtents);

		/* Every slot starts valid: the render thread may read one before the first publication. */
		m_renderStates.fill(RenderState{m_look, m_halfExtents});
	}

	CloudVolume::~CloudVolume ()
	{
		this->destroyFromHardware();
	}

	void
	CloudVolume::processLogics (const Scene & scene) noexcept
	{
		this->updateAnimations(scene.cycle());
	}

	void
	CloudVolume::publishStateForRendering (uint32_t writeStateIndex) noexcept
	{
		m_renderStates[writeStateIndex] = RenderState{m_look, m_halfExtents};
	}

	void
	CloudVolume::createOnHardware (Scene & scene) noexcept
	{
		m_bindlessTextureSet = &scene.bindlessTextureSet();

		if ( m_shape == nullptr )
		{
			TraceError{TracerTag} << "The cloud '" << this->name() << "' has no shape: it will never be drawn !";

			return;
		}

		if ( m_shape->isLoaded() )
		{
			this->registerShape();
		}
		else
		{
			/* The shape grows on the thread pool: register it the moment it lands. */
			this->observe(m_shape.get());

			/* ⚠️ Check-then-act: it may have finished between isLoaded() and observe(), and a
			 * LoadFinished sent in that window reached nobody. registerShape() is idempotent. */
			if ( m_shape->isLoaded() )
			{
				this->registerShape();
			}
		}
	}

	void
	CloudVolume::destroyFromHardware () noexcept
	{
		if ( m_shape != nullptr )
		{
			this->forget(m_shape.get());

			if ( m_bindlessTextureSet != nullptr && m_shapeBindlessIndex.load(std::memory_order_acquire) != NoShapeIndex )
			{
				m_bindlessTextureSet->unregisterTexture3D(m_shape.get());
			}
		}

		m_shapeBindlessIndex.store(NoShapeIndex, std::memory_order_release);
		m_bindlessTextureSet = nullptr;
	}

	void
	CloudVolume::registerShape () noexcept
	{
		if ( m_bindlessTextureSet == nullptr || m_shape == nullptr || m_shapeBindlessIndex.load(std::memory_order_acquire) != NoShapeIndex )
		{
			return;
		}

		/* The set deduplicates by instance: several clouds of one shape share one slot. */
		const auto index = m_bindlessTextureSet->registerTexture3D(m_shape);

		if ( index == UINT32_MAX )
		{
			TraceError{TracerTag} << "The bindless 3D array is full: the cloud '" << this->name() << "' will not be drawn !";

			return;
		}

		/* ⚠️ The box was sized from the REQUESTED parameters (the shape was not grown yet); the grown
		 * one follows the same rule, and a drift here would stretch the voxels. */
		const auto grown = m_shape->proportions();
		const auto expected = m_halfExtents / m_halfExtents[X];

		if ( std::abs(grown[Y] - expected[Y]) > 1.0e-3F || std::abs(grown[Z] - expected[Z]) > 1.0e-3F )
		{
			TraceWarning{TracerTag} << "The cloud '" << this->name() << "' box " << m_halfExtents << " does not follow its shape proportions " << grown << ": the voxels are stretched.";
		}

		m_shapeBindlessIndex.store(index, std::memory_order_release);
	}

	bool
	CloudVolume::onNotification (const ObservableTrait * observable, int notificationCode, const std::any & /*data*/) noexcept
	{
		if ( observable != m_shape.get() )
		{
			return false;
		}

		if ( notificationCode == Resources::ResourceTrait::LoadFinished )
		{
			this->registerShape();

			return false;
		}

		if ( notificationCode == Resources::ResourceTrait::LoadFailed )
		{
			TraceError{TracerTag} << "The shape of the cloud '" << this->name() << "' failed to grow: the cloud will not be drawn !";

			return false;
		}

		return true;
	}

	bool
	CloudVolume::playAnimation (uint8_t animationID, const Variant & value, size_t /*cycle*/) noexcept
	{
		switch ( animationID )
		{
			case OpticalThickness :
				m_look.opticalThickness = value.asFloat();
				return true;

			case Erosion :
				m_look.erosion = value.asFloat();
				return true;

			default :
				return false;
		}
	}
}