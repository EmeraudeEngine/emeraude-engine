/*
 * src/Graphics/Renderable/DynamicSkyResource.hpp
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

/* Local inclusions for inheritances. */
#include "AbstractBackground.hpp"

/* Local inclusions for usages. */
#include "Resources/Container.hpp"

namespace EmEn::Graphics::Renderable
{
	/**
	 * @brief The DynamicSkyResource class
	 * @extends EmEn::Graphics::Renderable::AbstractBackground The is a specialized background.
	 */
	class DynamicSkyResource final : public AbstractBackground
	{
		friend class Resources::Container< DynamicSkyResource >;

		using ResourceTrait::load;

		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"DynamicSkyResource"};

			/** @brief Observable class unique identifier. */
			static const size_t ClassUID;

			/** @brief Defines the resource dependency complexity. */
			static constexpr auto Complexity{Resources::DepComplexity::Complex};

			/**
			 * @brief Constructs a dynamic skybox resource.
			 * @param name A string for the skybox [std::move].
			 * @param renderableFlags The resource flag bits. Default none.
			 */
			explicit
			DynamicSkyResource (std::string name, uint32_t renderableFlags = 0) noexcept
				: AbstractBackground{std::move(name), renderableFlags}
			{

			}

			/** @copydoc EmEn::Libs::ObservableTrait::classUID() const */
			[[nodiscard]]
			size_t
			classUID () const noexcept override
			{
				return ClassUID;
			}

			/** @copydoc EmEn::Libs::ObservableTrait::is() const */
			[[nodiscard]]
			bool
			is (size_t classUID) const noexcept override
			{
				return classUID == ClassUID;
			}

			/** @copydoc EmEn::Graphics::Renderable::Interface::layerCount() const */
			[[nodiscard]]
			uint32_t
			layerCount () const noexcept override
			{
				return 1;
			}

			/** @copydoc EmEn::Graphics::Renderable::Interface::isOpaque() const */
			[[nodiscard]]
			bool
			isOpaque (uint32_t /*layerIndex*/) const noexcept override
			{
				return true;
			}

			/** @copydoc EmEn::Graphics::Renderable::Interface::geometry() const */
			[[nodiscard]]
			const Geometry::Interface *
			geometry () const noexcept override
			{
				return m_geometry.get();
			}

			/** @copydoc EmEn::Graphics::Renderable::Interface::material() const */
			[[nodiscard]]
			const Material::Interface *
			material (uint32_t /*layerIndex*/) const noexcept override
			{
				return m_material.get();
			}

			/** @copydoc EmEn::Graphics::Renderable::Interface::layerRasterizationOptions() const */
			[[nodiscard]]
			const RasterizationOptions *
			layerRasterizationOptions (uint32_t /*layerIndex*/) const noexcept override
			{
				return nullptr;
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

			/** @copydoc EmEn::Resources::ResourceTrait::load(const Json::Value &) */
			bool load (const Json::Value & data) noexcept override;

			/** @copydoc EmEn::Resources::ResourceTrait::memoryOccupied() const noexcept */
			[[nodiscard]]
			size_t
			memoryOccupied () const noexcept override
			{
				return sizeof(*this);
			}

		private:

			/**
			 * @brief setGeometry
			 * @param geometry
			 * @return bool
			 */
			bool setGeometry (const std::shared_ptr< Geometry::Interface > & geometry) noexcept;

			/**
			 * @brief setMaterial
			 * @param material
			 * @return bool
			 */
			bool setMaterial (const std::shared_ptr< Material::Interface > & material) noexcept;

			std::shared_ptr< Geometry::Interface > m_geometry;
			std::shared_ptr< Material::Interface > m_material;
	};
}

/* Expose the resource manager as a convenient type. */
namespace EmEn::Resources
{
	using DynamicSkies = Container< Graphics::Renderable::DynamicSkyResource >;
}
