/*
 * src/Graphics/CloudShapeResource.hpp
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
#include <string>
#include <vector>

/* Local inclusions for inheritances. */
#include "Vulkan/TextureInterface.hpp"
#include "Resources/ResourceTrait.hpp"

/* Local inclusions for usages. */
#include "Math/Vector.hpp"

/* Forward declarations. */
namespace EmEn
{
	namespace Resources
	{
		template< typename resource_t >
		class Container;
	}

	namespace Graphics
	{
		class Renderer;
	}
}

namespace EmEn::Graphics
{
	/**
	 * @brief The SHAPE of a volumetric cloud: a procedurally grown cumulus, voxelised into a 3D texture.
	 * @note The voxel grid is two 8-bit channels (`VK_FORMAT_R8G8_UNORM`):
	 * - **R — density** in [0, 1]. 1 is the full extinction of the cloud (the component's optical
	 *   thickness, not stored here), 0 is clear air. The shell of the shape ramps over
	 *   @ref Parameters::edgeSoftness, which is what the renderer's detail noise erodes into wisps.
	 * - **G — a conservative distance to the nearest non-empty voxel**, in units of
	 *   @ref MaxSkipDistance, rounded DOWN when quantised. The march skips empty air by that much.
	 *   ⚠️ Read it at mip 0 only: an average of distances is not a distance, and a coarser mip can
	 *   overshoot into the cloud.
	 * @note The grid lives in NORMALISED shape space: X spans [-1, 1], Y and Z span
	 * ±@ref proportions()`.y/z`, with cubic voxels. The component scales that box to metres; the
	 * shape itself has no unit, which is what lets a gizmo scale keep the look (the component
	 * carries the optical thickness, see Scenes::Component::CloudVolume).
	 * @note The growth follows the hierarchical model of A. Bouthors & F. Neyret, *Modeling Clouds
	 * Shape*, Eurographics 2004 (short paper): a MASS — here one ellipsoid dome filling the box —
	 * then generations of smaller puffs budding on the upper surface of their parent. The union is
	 * a smooth minimum (I. Quilez, *Smooth minimum*, 2013), cut flat at the base — a cumulus's
	 * condensation level — and bulged by the billowy cellular noise of Schneider & Vos (*The
	 * Real-time Volumetric Cloudscapes of Horizon Zero Dawn*, SIGGRAPH 2015).
	 * @note ⚠️ Never grow the mass out of spheres again: a sphere bounded by the box height cannot
	 * fill a box twice as wide as it is tall (5-8 % of the voxels, a cloud 3-5x smaller than its
	 * box, measured 2026-09-24). @ref occupancy() is traced at every growth to keep that visible.
	 * @note Deterministic: the same parameters grow the same cloud on every platform (integer hash
	 * random stream, no standard-library distribution).
	 * @extends EmEn::Vulkan::TextureInterface The shape is bound as a 3D texture (bindless 3D array).
	 * @extends EmEn::Resources::ResourceTrait The shape is a loadable resource, deduplicated by name.
	 */
	class EMEN_API CloudShapeResource final : public Vulkan::TextureInterface, public Resources::ResourceTrait
	{
		friend class Resources::Container< CloudShapeResource >;

		using ResourceTrait::load;

		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"CloudShapeResource"};

			/** @brief Defines the resource dependency complexity. */
			static constexpr auto Complexity{Resources::DepComplexity::None};

			/** @brief The distance, in normalised shape units, that a G value of 1 encodes. */
			static constexpr auto MaxSkipDistance{0.5F};

			/**
			 * @brief The generation parameters. Two shapes with the same parameters are the same shape.
			 */
			struct EMEN_API Parameters
			{
				/** @brief The seed of the growth. */
				uint32_t seed{1};
				/** @brief Voxels along X, the longest horizontal axis. Clamped to [16, 256]. */
				uint32_t resolution{128};
				/** @brief Height over width. Clamped to [0.1, 1]. */
				float heightRatio{0.6F};
				/** @brief Depth over width. Clamped to [0.1, 1]. */
				float depthRatio{0.8F};
				/** @brief The total number of puffs grown, core included. Clamped to [1, 256]. */
				uint32_t puffCount{48};
				/** @brief The billow displacement, as a fraction of a QUARTER of the dome's vertical semi-axis. Clamped to [0, 1]. */
				float billowStrength{0.35F};
				/** @brief How flat and wide the base is, in [0, 1]: 1 cuts the dome at its equator, 0 half a semi-axis below its centre (a belly narrowing to a small base). */
				float flatBase{0.45F};
				/** @brief The thickness of the density ramp at the surface, in normalised shape units. Clamped to [one voxel, 0.5]. */
				float edgeSoftness{0.08F};
			};

			/**
			 * @brief Constructs a cloud shape resource.
			 * @param serviceProvider A reference to the resource service provider.
			 * @param name A string for the resource name [std::move].
			 * @param resourceFlags The resource flag bits. Default none.
			 */
			CloudShapeResource (Resources::AbstractServiceProvider & serviceProvider, std::string name, uint32_t resourceFlags = 0) noexcept
				: ResourceTrait{serviceProvider, std::move(name), resourceFlags}
			{

			}

			/**
			 * @brief Destructs the cloud shape resource.
			 */
			~CloudShapeResource () override
			{
				this->destroyFromHardware();
			}

			/**
			 * @brief Returns the unique identifier for this class [Thread-safe].
			 * @return size_t
			 */
			static
			size_t
			getClassUID () noexcept
			{
				return Base::Hash::FNV1a(ClassId);
			}

			/** @copydoc EmEn::Base::ObservableTrait::classUID() const */
			[[nodiscard]]
			size_t
			classUID () const noexcept override
			{
				return getClassUID();
			}

			/** @copydoc EmEn::Base::ObservableTrait::is() const */
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

			/**
			 * @copydoc EmEn::Resources::ResourceTrait::load()
			 * @note The default shape: a cumulus grown from the default parameters.
			 */
			bool load () noexcept override;

			/**
			 * @copydoc EmEn::Resources::ResourceTrait::load(const Json::Value &)
			 * @note Reads the generation parameters: `Seed`, `Resolution`, `HeightRatio`, `DepthRatio`,
			 * `PuffCount`, `BillowStrength`, `FlatBase`, `EdgeSoftness`. A missing key keeps its default.
			 */
			bool load (const Json::Value & data) noexcept override;

			/**
			 * @brief Grows a cloud shape from generation parameters.
			 * @param parameters A reference to the parameters.
			 * @return bool
			 */
			bool load (const Parameters & parameters) noexcept;

			/** @copydoc EmEn::Resources::ResourceTrait::memoryOccupied() const noexcept */
			[[nodiscard]]
			size_t
			memoryOccupied () const noexcept override
			{
				return sizeof(*this) + m_voxels.size();
			}

			/** @copydoc EmEn::Vulkan::TextureInterface::isCreated() */
			[[nodiscard]]
			bool isCreated () const noexcept override;

			/** @copydoc EmEn::Vulkan::TextureInterface::type() */
			[[nodiscard]]
			Vulkan::TextureType
			type () const noexcept override
			{
				return Vulkan::TextureType::Texture3D;
			}

			/** @copydoc EmEn::Vulkan::TextureInterface::dimensions() */
			[[nodiscard]]
			uint32_t
			dimensions () const noexcept override
			{
				return 3;
			}

			/** @copydoc EmEn::Vulkan::TextureInterface::isCubemapTexture() */
			[[nodiscard]]
			bool
			isCubemapTexture () const noexcept override
			{
				return false;
			}

			/** @copydoc EmEn::Vulkan::TextureInterface::image() */
			[[nodiscard]]
			std::shared_ptr< Vulkan::Image >
			image () const noexcept override
			{
				return m_image;
			}

			/** @copydoc EmEn::Vulkan::TextureInterface::imageView() */
			[[nodiscard]]
			std::shared_ptr< Vulkan::ImageView >
			imageView () const noexcept override
			{
				return m_imageView;
			}

			/** @copydoc EmEn::Vulkan::TextureInterface::sampler() */
			[[nodiscard]]
			std::shared_ptr< Vulkan::Sampler >
			sampler () const noexcept override
			{
				return m_sampler;
			}

			/** @copydoc EmEn::Vulkan::TextureInterface::request3DTextureCoordinates() */
			[[nodiscard]]
			bool
			request3DTextureCoordinates () const noexcept override
			{
				return true;
			}

			/**
			 * @brief Returns the generation parameters, as sanitised by load().
			 * @return const Parameters &
			 */
			[[nodiscard]]
			const Parameters &
			parameters () const noexcept
			{
				return m_parameters;
			}

			/**
			 * @brief Returns the half extents of the shape box in normalised units: X is 1.
			 * @note ⚠️ Derived from the VOXEL counts, not from the requested ratios, so that voxels stay
			 * cubic: the box a component scales is exactly the box the grid fills.
			 * @return Base::Math::Vector< 3, float >
			 */
			[[nodiscard]]
			Base::Math::Vector< 3, float >
			proportions () const noexcept
			{
				return m_proportions;
			}

			/**
			 * @brief Returns the grid width in voxels.
			 * @return uint32_t
			 */
			[[nodiscard]]
			uint32_t
			width () const noexcept
			{
				return m_width;
			}

			/**
			 * @brief Returns the grid height in voxels.
			 * @return uint32_t
			 */
			[[nodiscard]]
			uint32_t
			height () const noexcept
			{
				return m_height;
			}

			/**
			 * @brief Returns the grid depth in voxels.
			 * @return uint32_t
			 */
			[[nodiscard]]
			uint32_t
			depth () const noexcept
			{
				return m_depth;
			}

			/**
			 * @brief Returns the voxel data, two bytes per voxel (density, skip distance), X fastest.
			 * @return const std::vector< uint8_t > &
			 */
			[[nodiscard]]
			const std::vector< uint8_t > &
			voxels () const noexcept
			{
				return m_voxels;
			}

			/**
			 * @brief Returns the fraction of voxels holding any density, in [0, 1].
			 * @note A load-time measurement of the grown shape, traced once — a generator change that
			 * silently empties the grid (or fills it) shows here before anyone looks at a frame.
			 * @return float
			 */
			[[nodiscard]]
			float
			occupancy () const noexcept
			{
				return m_occupancy;
			}

			/**
			 * @brief Returns the normalised half extents a parameter set grows into, WITHOUT growing it.
			 * @note The single rule generate() applies (voxel counts rounded from the clamped ratios),
			 * exposed so that a component can size its box while the shape is still loading on the
			 * thread pool.
			 * @param parameters A reference to the parameters.
			 * @return Base::Math::Vector< 3, float >
			 */
			[[nodiscard]]
			static Base::Math::Vector< 3, float > proportionsOf (const Parameters & parameters) noexcept;

			/**
			 * @brief Builds the canonical resource name of a parameter set.
			 * @note Two components asking for the same parameters share ONE shape through the container.
			 * @param parameters A reference to the parameters.
			 * @return std::string
			 */
			[[nodiscard]]
			static std::string resourceName (const Parameters & parameters) noexcept;

		private:

			/** @copydoc EmEn::Resources::ResourceTrait::onDependenciesLoaded() */
			[[nodiscard]]
			bool onDependenciesLoaded () noexcept override;

			/**
			 * @brief Grows the shape and fills the voxel grid from m_parameters.
			 * @return bool
			 */
			bool generate () noexcept;

			/**
			 * @brief Creates the 3D image, its view and its sampler, and uploads the grid.
			 * @param renderer A reference to the graphics renderer.
			 * @return bool
			 */
			bool createOnHardware (Renderer & renderer) noexcept;

			/**
			 * @brief Releases the GPU objects.
			 * @return void
			 */
			void destroyFromHardware () noexcept;

			Parameters m_parameters;
			std::vector< uint8_t > m_voxels;
			std::shared_ptr< Vulkan::Image > m_image;
			std::shared_ptr< Vulkan::ImageView > m_imageView;
			std::shared_ptr< Vulkan::Sampler > m_sampler;
			Base::Math::Vector< 3, float > m_proportions{1.0F, 1.0F, 1.0F};
			uint32_t m_width{0};
			uint32_t m_height{0};
			uint32_t m_depth{0};
			float m_occupancy{0.0F};
	};
}

/* Expose the resource manager as a convenient type. */
namespace EmEn::Resources
{
	using CloudShapes = Container< Graphics::CloudShapeResource >;
}
