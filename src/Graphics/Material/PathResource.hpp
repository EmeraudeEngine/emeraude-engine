/*
 * src/Graphics/Material/PathResource.hpp
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
#include <array>
#include <cstdint>
#include <memory>
#include <string>

/* Local inclusions for inheritances. */
#include "Interface.hpp"

/* Local inclusions for usages. */
#include "Math/Vector.hpp"
#include "Physics/SurfacePhysicalProperties.hpp"
#include "PixelFactory/Color.hpp"

/* Forward declarations. */
namespace EmEn::Resources
{
	template< typename resource_t >
	class Container;
}

namespace EmEn::Graphics
{
	class SharedUniformBuffer;
}

namespace EmEn::Graphics::Material
{
	/**
	 * @brief The material of a PATH (Scenes::Component::Path): a camera-facing ribbon along a polyline, OPAQUE and UNLIT,
	 * of a solid colour emitted at a luminance in nits (owner decisions 2026-09-29).
	 * @extends EmEn::Graphics::Material::Interface This is a material.
	 * @note The vertex stage BUILDS the geometry by vertex pulling (AbstractVertexStage::enablePathRibbon(), Saphir
	 * `PathGLSL.hpp`): the points come from the scene's path SSBO, never from a vertex buffer.
	 * @note Opaque, depth tested and written, in the scene pass: it writes the G-buffer like any unlit surface (a normal
	 * facing the eye, neutral material properties) and a real velocity. The colour is a RADIANCE — a linear hue × a
	 * luminance in nits (Graphics/Photometry.hpp) — so the exposure treats it like any emitter: bright at night, modest
	 * in full sun. For a colour exact on screen whatever the exposure, the path's DEBUG mode draws after the tone
	 * mapping instead.
	 * @note The UBO holds the look only (radiance, width, joins): the points travel through the frame-buffered path SSBO.
	 */
	class EMEN_LEAN_API PathResource final : public Interface
	{
		friend class Resources::Container< PathResource >;

		using ResourceTrait::load;

		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"MaterialPathResource"};

			/** @brief Defines the resource dependency complexity. */
			static constexpr auto Complexity{Resources::DepComplexity::None};

			/* JSON keys. */
			static constexpr auto JKColor{"Color"};
			static constexpr auto JKLuminance{"Luminance"};
			static constexpr auto JKHalfWidth{"HalfWidth"};
			static constexpr auto JKWidthInPixels{"WidthInPixels"};
			static constexpr auto JKRoundJoins{"RoundJoins"};
			static constexpr auto JKMiterLimit{"MiterLimit"};

			/* Defaults: a 10 cm wide white line at the luminance of a lit screen, mitered joins, butt caps. */
			static constexpr auto DefaultLuminance{250.0F};
			static constexpr auto DefaultHalfWidth{0.05F};
			/** @brief The SVG default (stroke-miterlimit): a miter longer than 4 half widths becomes a bevel (~29°). */
			static constexpr auto DefaultMiterLimit{4.0F};

			/**
			 * @brief Constructs a path material.
			 * @param serviceProvider A reference to the service provider.
			 * @param name The name of the resource [std::move].
			 * @param resourceFlags The resource flag bits. Default none. (Unused yet)
			 */
			PathResource (Resources::AbstractServiceProvider & serviceProvider, const std::string & name, uint32_t resourceFlags = 0) noexcept;

			/** @brief Deleted copy and move. */
			PathResource (const PathResource & copy) noexcept = delete;
			PathResource (PathResource && copy) noexcept = delete;
			PathResource & operator= (const PathResource & copy) noexcept = delete;
			PathResource & operator= (PathResource && copy) noexcept = delete;

			/** @brief Destructs the path material. */
			~PathResource () override
			{
				this->destroy();
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

			/** @copydoc EmEn::Graphics::Material::Interface::updateVideoMemory() */
			bool updateVideoMemory () noexcept override;

			/** @copydoc EmEn::Graphics::Material::Interface::isComplex() */
			[[nodiscard]]
			bool
			isComplex () const noexcept override
			{
				return false;
			}

			/** @copydoc EmEn::Graphics::Material::Interface::setupLightGenerator() */
			[[nodiscard]]
			bool
			setupLightGenerator (Saphir::LightGenerator & /*lightGenerator*/) const noexcept override
			{
				/* Unlit: the path emits its colour. */
				return true;
			}

			/** @copydoc EmEn::Graphics::Material::Interface::prepareVertexStage() */
			[[nodiscard]]
			bool prepareVertexStage (Saphir::Generator::Abstract & generator, Saphir::AbstractVertexStage & vertexShader) const noexcept override;

			/** @copydoc EmEn::Graphics::Material::Interface::generateVertexShaderCode() */
			[[nodiscard]]
			bool generateVertexShaderCode (Saphir::Generator::Abstract & generator, Saphir::AbstractVertexStage & vertexShader) const noexcept override;

			/** @copydoc EmEn::Graphics::Material::Interface::generateFragmentShaderCode() */
			[[nodiscard]]
			bool generateFragmentShaderCode (Saphir::Generator::Abstract & generator, Saphir::LightGenerator & lightGenerator, Saphir::FragmentShader & fragmentShader) const noexcept override;

			/**
			 * @copydoc EmEn::Graphics::Material::Interface::requiresAlphaTestedShadows()
			 * @note Always: the depth-only programs (the SELECTION depth pass — a path casts no shadow) must build the ribbon
			 * in their vertex stage and discard outside the round capsule, exactly as the scene pass does.
			 */
			[[nodiscard]]
			bool
			requiresAlphaTestedShadows () const noexcept override
			{
				return true;
			}

			/** @copydoc EmEn::Graphics::Material::Interface::generateShadowVertexCode() */
			[[nodiscard]]
			bool generateShadowVertexCode (const Saphir::Generator::Abstract & generator, Saphir::AbstractVertexStage & vertexShader) const noexcept override;

			/** @copydoc EmEn::Graphics::Material::Interface::generateShadowAlphaTestCode() */
			[[nodiscard]]
			bool generateShadowAlphaTestCode (const Saphir::Generator::Abstract & generator, Saphir::FragmentShader & fragmentShader) const noexcept override;

			/** @copydoc EmEn::Graphics::Material::Interface::surfacePhysicalProperties() const */
			[[nodiscard]]
			const Physics::SurfacePhysicalProperties &
			surfacePhysicalProperties () const noexcept override
			{
				return m_physicalSurfaceProperties;
			}

			/** @copydoc EmEn::Graphics::Material::Interface::surfacePhysicalProperties() */
			[[nodiscard]]
			Physics::SurfacePhysicalProperties &
			surfacePhysicalProperties () noexcept override
			{
				return m_physicalSurfaceProperties;
			}

			/** @copydoc EmEn::Graphics::Material::Interface::frameCount() */
			[[nodiscard]]
			uint32_t
			frameCount () const noexcept override
			{
				return 1;
			}

			/** @copydoc EmEn::Graphics::Material::Interface::duration() */
			[[nodiscard]]
			uint32_t
			duration () const noexcept override
			{
				return 0;
			}

			/** @copydoc EmEn::Graphics::Material::Interface::frameIndexAt() */
			[[nodiscard]]
			uint32_t
			frameIndexAt (uint32_t /*sceneTime*/) const noexcept override
			{
				return 0;
			}

			/**
			 * @copydoc EmEn::Graphics::Material::Interface::enableBlending()
			 * @note A world path is OPAQUE: refused with a warning (the debug mode is the translucent one).
			 */
			void enableBlending (BlendingMode mode) noexcept override;

			/** @copydoc EmEn::Graphics::Material::Interface::blendingMode() */
			[[nodiscard]]
			BlendingMode
			blendingMode () const noexcept override
			{
				return BlendingMode::None;
			}

			/** @copydoc EmEn::Graphics::Material::Interface::fragmentColor() */
			[[nodiscard]]
			std::string fragmentColor () const noexcept override;

			/** @copydoc EmEn::Graphics::Material::Interface::descriptorSetLayout() */
			[[nodiscard]]
			std::shared_ptr< Vulkan::DescriptorSetLayout >
			descriptorSetLayout () const noexcept override
			{
				return m_descriptorSetLayout;
			}

			/** @copydoc EmEn::Graphics::Material::Interface::UBOIndex() */
			[[nodiscard]]
			uint32_t
			UBOIndex () const noexcept override
			{
				return m_sharedUBOIndex;
			}

			/** @copydoc EmEn::Graphics::Material::Interface::UBOAlignment() */
			[[nodiscard]]
			uint32_t UBOAlignment () const noexcept override;

			/** @copydoc EmEn::Graphics::Material::Interface::UBOOffset() */
			[[nodiscard]]
			uint32_t UBOOffset () const noexcept override;

			/** @copydoc EmEn::Graphics::Material::Interface::descriptorSet() */
			[[nodiscard]]
			const Vulkan::DescriptorSet *
			descriptorSet () const noexcept override
			{
				return m_descriptorSet.get();
			}

			/** @copydoc EmEn::Graphics::Material::Interface::getUniformBlock() */
			[[nodiscard]]
			Saphir::Declaration::UniformBlock getUniformBlock (uint32_t set, uint32_t binding) const noexcept override;

			/**
			 * @brief Sets the colour of the path (LINEAR, a hue: its brightness is the luminance).
			 * @param color A reference to a colour.
			 * @return void
			 */
			void setColor (const Base::PixelFactory::Color< float > & color) noexcept;

			/**
			 * @brief Returns the colour of the path (linear).
			 * @return const Base::PixelFactory::Color< float > &
			 */
			[[nodiscard]]
			const Base::PixelFactory::Color< float > &
			color () const noexcept
			{
				return m_color;
			}

			/**
			 * @brief Sets the luminance of the path, in nits (cd/m²): a monitor 200-300, a fluorescent tube ~10 000.
			 * @param nits The luminance. Clamped to 0 or above.
			 * @return void
			 */
			void setLuminance (float nits) noexcept;

			/**
			 * @brief Returns the luminance of the path, in nits.
			 * @return float
			 */
			[[nodiscard]]
			float
			luminance () const noexcept
			{
				return m_luminance;
			}

			/**
			 * @brief Sets the half width of the path and its unit.
			 * @param halfWidth The half width: in the entity's units (metres for an unscaled entity), or in pixels.
			 * Clamped to 0 or above.
			 * @param inPixels Whether the half width is in pixels (constant on screen) rather than in entity units.
			 * @return void
			 */
			void setWidth (float halfWidth, bool inPixels) noexcept;

			/**
			 * @brief Returns the half width (entity units or pixels, see isWidthInPixels()).
			 * @return float
			 */
			[[nodiscard]]
			float
			halfWidth () const noexcept
			{
				return m_properties[StyleOffset];
			}

			/**
			 * @brief Returns whether the width is in pixels.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isWidthInPixels () const noexcept
			{
				return m_properties[StyleOffset + 1] > 0.5F;
			}

			/**
			 * @brief Sets the joins: mitered (bevelled past the miter limit, butt caps), or round (round caps too).
			 * @param round Whether the joins and the caps are round.
			 * @param miterLimit The miter limit (the SVG stroke-miterlimit), at least 1.
			 * @return void
			 */
			void setJoins (bool round, float miterLimit = DefaultMiterLimit) noexcept;

			/**
			 * @brief Returns whether the joins and caps are round.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			areJoinsRound () const noexcept
			{
				return m_properties[StyleOffset + 2] > 0.5F;
			}

			/**
			 * @brief Returns the miter limit.
			 * @return float
			 */
			[[nodiscard]]
			float
			miterLimit () const noexcept
			{
				return m_properties[StyleOffset + 3];
			}

			/**
			 * @brief Sets the DEPTH OFFSET: how far every corner of the ribbon moves toward the eye, along its own ray (its
			 * place on screen, and a pixel width, unchanged), so that a path LYING ON A SURFACE is not cut by it where the
			 * camera-facing ribbon dips below (owner decisions 2026-09-30: an offset toward the eye, in metres, off by default).
			 * @note In WORLD units (metres), never more than half the way to the eye. What is closer to the path than the
			 * offset stops hiding it: keep it small (a few centimetres). Applied alike by the scene pass, the velocity and
			 * the selection outline's depth; the debug mode draws on top anyway.
			 * @param offset The offset, 0 (none, the default) or more; a non-finite value is 0.
			 * @return void
			 */
			void setDepthOffset (float offset) noexcept;

			/**
			 * @brief Makes the ribbon LIE FLAT in the surface an up vector is normal to — a trail on the ground, a road
			 * marking — instead of facing the eye (owner decision 2026-09-30: the answer to a path lying on a surface).
			 * @note Its width runs along cross(segment, up): the ribbon tilts with the path's own slope, not with a slope
			 * across it. Seen edge-on it vanishes, like any marking. Widths in ENTITY UNITS only: a pixel width keeps facing
			 * the eye, and so does the debug mode (visibility first). Lift the points a centimetre or two above the surface
			 * (the depth test between two coplanar surfaces is a draw).
			 * @param state Whether the ribbon lies flat.
			 * @param up The surface's normal in the ENTITY's space (normalized here). Default +Y; a zero one leaves it facing.
			 * @return void
			 */
			void setFlat (bool state, const Base::Math::Vector< 3, float > & up = Base::Math::Vector< 3, float >::positiveY()) noexcept;

			/**
			 * @brief Returns whether the ribbon lies flat (setFlat()).
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isFlat () const noexcept
			{
				return m_properties[PlacementOffset + 1] != 0.0F || m_properties[PlacementOffset + 2] != 0.0F || m_properties[PlacementOffset + 3] != 0.0F;
			}

			/**
			 * @brief Returns the up vector of a flat ribbon (entity space, unit), or zero when it faces the eye.
			 * @return Base::Math::Vector< 3, float >
			 */
			[[nodiscard]]
			Base::Math::Vector< 3, float >
			flatUp () const noexcept
			{
				return {m_properties[PlacementOffset + 1], m_properties[PlacementOffset + 2], m_properties[PlacementOffset + 3]};
			}

			/**
			 * @brief Returns the depth offset, in world units.
			 * @return float
			 */
			[[nodiscard]]
			float
			depthOffset () const noexcept
			{
				return m_properties[PlacementOffset];
			}

		private:

			/** @copydoc EmEn::Graphics::Material::Interface::create() */
			[[nodiscard]]
			bool create (Renderer & renderer) noexcept override;

			/** @copydoc EmEn::Graphics::Material::Interface::destroy() */
			void destroy () noexcept override;

			/** @copydoc EmEn::Graphics::Material::Interface::getSharedUniformBufferIdentifier() */
			[[nodiscard]]
			std::string
			getSharedUniformBufferIdentifier () const noexcept override
			{
				return ClassId;
			}

			/** @copydoc EmEn::Graphics::Material::Interface::createElementInSharedBuffer() */
			[[nodiscard]]
			bool createElementInSharedBuffer (Renderer & renderer, const std::string & identifier) noexcept override;

			/** @copydoc EmEn::Graphics::Material::Interface::createDescriptorSetLayout() */
			[[nodiscard]]
			bool createDescriptorSetLayout (Vulkan::LayoutManager & layoutManager, const std::string & identifier) noexcept override;

			/** @copydoc EmEn::Graphics::Material::Interface::createDescriptorSet() */
			[[nodiscard]]
			bool createDescriptorSet (Renderer & renderer, const Vulkan::UniformBufferObject & uniformBufferObject) noexcept override;

			/**
			 * @brief Writes the radiance (linear colour × luminance) into the UBO copy.
			 * @return void
			 */
			void updateRadiance () noexcept;

			/**
			 * @brief Requests an upload of the UBO copy, once per frame at most, when the material is on the GPU.
			 * @return void
			 */
			void markVideoMemoryDirty () noexcept;

			/* UBO layout (std140, three vec4 — Keys PathRadiance, PathStyle, PathPlacement). */
			static constexpr auto RadianceOffset{0UL};
			static constexpr auto StyleOffset{4UL};
			static constexpr auto PlacementOffset{8UL};
			static constexpr auto PropertiesSize{12UL};

			Physics::SurfacePhysicalProperties m_physicalSurfaceProperties;
			Base::PixelFactory::Color< float > m_color{Base::PixelFactory::White};
			std::array< float, PropertiesSize > m_properties{
				/* Radiance (rgb), unused. */
				0.0F, 0.0F, 0.0F, 0.0F,
				/* Half width, in pixels, round, miter limit. */
				DefaultHalfWidth, 0.0F, 0.0F, DefaultMiterLimit,
				/* Depth offset toward the eye (world units), the flat ribbon's up vector (zero = facing the eye). */
				0.0F, 0.0F, 0.0F, 0.0F
			};
			std::shared_ptr< SharedUniformBuffer > m_sharedUniformBuffer;
			std::shared_ptr< Vulkan::DescriptorSetLayout > m_descriptorSetLayout;
			std::unique_ptr< Vulkan::DescriptorSet > m_descriptorSet;
			Renderer * m_renderer{nullptr}; ///< Set by create(): the flusher of dynamic properties (owned by the engine).
			float m_luminance{DefaultLuminance};
			uint32_t m_sharedUBOIndex{0};
			bool m_videoMemoryUpdated{false}; ///< Raised by markVideoMemoryDirty(), cleared by updateVideoMemory().
	};
}

/* Expose the resource manager as a convenient type. */
namespace EmEn::Resources
{
	using PathMaterials = Container< Graphics::Material::PathResource >;
}
