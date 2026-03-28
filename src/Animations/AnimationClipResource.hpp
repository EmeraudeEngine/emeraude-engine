/*
 * src/Animations/AnimationClipResource.hpp
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

/* Local inclusions for inheritances. */
#include "Resources/ResourceTrait.hpp"

/* Local inclusions for usages. */
#include "Libs/Animation/AnimationClip.hpp"
#include "Resources/Container.hpp"

namespace EmEn::Animations
{
	/**
	 * @brief A managed resource wrapping a skeletal animation clip (keyframe data).
	 * @extends EmEn::Resources::ResourceTrait Every animation clip is a named, shared resource.
	 */
	class AnimationClipResource final : public Resources::ResourceTrait
	{
		friend class Resources::Container< AnimationClipResource >;

		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"AnimationClipResource"};

			/** @brief Defines the resource dependency complexity. */
			static constexpr auto Complexity{Resources::DepComplexity::None};

			/**
			 * @brief Constructs an animation clip resource.
			 * @param serviceProvider A reference to the service provider.
			 * @param name The name of the resource [std::move].
			 * @param resourceFlags The resource flag bits. Default none.
			 */
			AnimationClipResource (Resources::AbstractServiceProvider & serviceProvider, std::string name, uint32_t resourceFlags = 0) noexcept
				: ResourceTrait{serviceProvider, std::move(name), resourceFlags}
			{

			}

			/**
			 * @brief Returns the unique identifier for this class [Thread-safe].
			 * @return size_t
			 */
			static
			size_t
			getClassUID () noexcept
			{
				return Libs::Hash::FNV1a(ClassId);
			}

			/** @copydoc EmEn::Libs::ObservableTrait::classUID() const */
			[[nodiscard]]
			size_t
			classUID () const noexcept override
			{
				return getClassUID();
			}

			/** @copydoc EmEn::Libs::ObservableTrait::is() const */
			[[nodiscard]]
			bool
			is (size_t classUID) const noexcept override
			{
				return classUID == getClassUID();
			}

			/** @copydoc EmEn::Resources::ResourceTrait::classLabel() const */
			[[nodiscard]]
			const char *
			classLabel () const noexcept override
			{
				return ClassId;
			}

			/** @copydoc EmEn::Resources::ResourceTrait::load() */
			bool load () noexcept override;

			/** @copydoc EmEn::Resources::ResourceTrait::load(const std::filesystem::path &) */
			bool load (const std::filesystem::path & filepath) noexcept override;

			/** @copydoc EmEn::Resources::ResourceTrait::load(const Json::Value &) */
			bool load (const Json::Value & data) noexcept override;

			/**
			 * @brief Loads the resource from an existing animation clip.
			 * @param clip The animation clip data to take ownership of.
			 * @return bool
			 */
			bool load (Libs::Animation::AnimationClip< float > clip) noexcept;

			/** @copydoc EmEn::Resources::ResourceTrait::memoryOccupied() const noexcept */
			[[nodiscard]]
			size_t
			memoryOccupied () const noexcept override
			{
				return sizeof(*this);
			}

			/**
			 * @brief Returns the animation clip data.
			 * @return const Libs::Animation::AnimationClip< float > &
			 */
			[[nodiscard]]
			const Libs::Animation::AnimationClip< float > &
			clip () const noexcept
			{
				return m_clip;
			}

		private:

			/** @copydoc EmEn::Resources::ResourceTrait::onDependenciesLoaded() */
			[[nodiscard]]
			bool
			onDependenciesLoaded () noexcept override
			{
				return true;
			}

			Libs::Animation::AnimationClip< float > m_clip;
	};
}

/* Expose the resource manager as a convenient type. */
namespace EmEn::Resources
{
	using AnimationClips = Container< Animations::AnimationClipResource >;
}
