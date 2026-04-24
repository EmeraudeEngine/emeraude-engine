/*
 * src/Graphics/Renderable/SkeletalDataTrait.hpp
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
#include <memory>
#include <vector>

/* Local inclusions for usages. */
#include "Libs/Animation/Skin.hpp"

/* Forward declarations. */
namespace EmEn::Animations
{
	class SkeletonResource;
	class AnimationClipResource;
}

namespace EmEn::Graphics::Renderable
{
	/**
	 * @brief Trait carrying shared skeletal animation data on a renderable.
	 *
	 * The skeleton and animation clips are shared resources (multiple meshes can
	 * reference the same skeleton and clips from a single asset).
	 * The skin is per-mesh (it maps this mesh's vertex bone indices to skeleton joints).
	 *
	 * Inherited by MeshResource and SimpleMeshResource.
	 */
	class SkeletalDataTrait
	{
		public:

			/**
			 * @brief Returns whether this renderable has skeletal data.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			hasSkeletalData () const noexcept
			{
				return m_skeleton != nullptr;
			}

			/**
			 * @brief Returns the skeleton resource.
			 * @return const std::shared_ptr< Animations::SkeletonResource > &
			 */
			[[nodiscard]]
			const std::shared_ptr< Animations::SkeletonResource > &
			skeletonResource () const noexcept
			{
				return m_skeleton;
			}

			/**
			 * @brief Returns the skin binding (per-mesh).
			 * @return const Libs::Animation::Skin< float > &
			 */
			[[nodiscard]]
			const Libs::Animation::Skin< float > &
			skin () const noexcept
			{
				return m_skin;
			}

			/**
			 * @brief Returns the animation clip resources.
			 * @return const std::vector< std::shared_ptr< Animations::AnimationClipResource > > &
			 */
			[[nodiscard]]
			const std::vector< std::shared_ptr< Animations::AnimationClipResource > > &
			animationClips () const noexcept
			{
				return m_animationClips;
			}

			/**
			 * @brief Sets all skeletal data at once.
			 * @param skeleton The skeleton resource.
			 * @param skin The per-mesh skin binding.
			 * @param clips The animation clip resources.
			 */
			void
			setSkeletalData (
				std::shared_ptr< Animations::SkeletonResource > skeleton,
				Libs::Animation::Skin< float > skin,
				std::vector< std::shared_ptr< Animations::AnimationClipResource > > clips
			) noexcept
			{
				m_skeleton = std::move(skeleton);
				m_skin = std::move(skin);
				m_animationClips = std::move(clips);
			}

			/**
			 * @brief Sets skeletal data without animation clips.
			 * @note Use addAnimationClips() to attach clips separately.
			 * @param skeleton The skeleton resource.
			 * @param skin The per-mesh skin binding.
			 */
			void
			setSkeletalData (
				std::shared_ptr< Animations::SkeletonResource > skeleton,
				Libs::Animation::Skin< float > skin
			) noexcept
			{
				m_skeleton = std::move(skeleton);
				m_skin = std::move(skin);
			}

			/**
			 * @brief Appends animation clips to the existing skeletal data.
			 * @param clips The animation clip resources to add.
			 */
			void
			addAnimationClips (std::vector< std::shared_ptr< Animations::AnimationClipResource > > clips) noexcept
			{
				m_animationClips.insert(m_animationClips.end(),
					std::make_move_iterator(clips.begin()),
					std::make_move_iterator(clips.end()));
			}

			/**
			 * @brief Replaces the animation clip list (drops any previously attached clips).
			 * @note Skeleton and skin bindings are preserved.
			 * @param clips The new clip list.
			 */
			void
			setAnimationClips (std::vector< std::shared_ptr< Animations::AnimationClipResource > > clips) noexcept
			{
				m_animationClips = std::move(clips);
			}

		protected:

			SkeletalDataTrait () noexcept = default;
			~SkeletalDataTrait () = default;

			SkeletalDataTrait (const SkeletalDataTrait &) = default;
			SkeletalDataTrait (SkeletalDataTrait &&) = default;
			SkeletalDataTrait & operator= (const SkeletalDataTrait &) = default;
			SkeletalDataTrait & operator= (SkeletalDataTrait &&) = default;

		private:

			std::shared_ptr< Animations::SkeletonResource > m_skeleton;
			Libs::Animation::Skin< float > m_skin;
			std::vector< std::shared_ptr< Animations::AnimationClipResource > > m_animationClips;
	};
}
