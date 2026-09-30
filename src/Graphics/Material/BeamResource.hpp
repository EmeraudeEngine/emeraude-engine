/*
 * src/Graphics/Material/BeamResource.hpp
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
	 * @brief The material of a BEAM: a laser, an electric arc — Half-Life's env_beam / env_laser, made photometric.
	 * @extends EmEn::Graphics::Material::Interface This is a material.
	 * @note Structurally different from StandardResource, hence its own class (owner decision, 2026-09-28): the vertex
	 * stage BUILDS the geometry (a camera-facing ribbon along the beam, displaced by an arc noise —
	 * AbstractVertexStage::enableBeamRibbon(), Saphir `BeamGLSL.hpp`), nothing is lit, and the output is pure emitted
	 * light: a LUMINANCE in nits (Graphics/Photometry.hpp, the emissive contract) added over the scene.
	 * @note An emissive overlay, not a surface: additive blending, no G-buffer write (writesGeometryBuffer()), no depth
	 * write, no shadow, out of the TLAS (Scenes::Component::Beam sets the instance up so).
	 * @note Its UBO holds the LOOK only — colour, luminance, width, arc — which changes rarely. The curve is the beam's
	 * STATIONS, published per render state slot and staged into the frame-buffered path SSBO (Scenes::Component::Beam),
	 * never this UBO: a material UBO lives in ONE frame region.
	 * @note Drawn by one renderable per beam on its own pulled-vertex geometry (the component owns both).
	 */
	class EMEN_LEAN_API BeamResource final : public Interface
	{
		friend class Resources::Container< BeamResource >;

		using ResourceTrait::load;

		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"MaterialBeamResource"};

			/** @brief Defines the resource dependency complexity. */
			static constexpr auto Complexity{Resources::DepComplexity::None};

			/* JSON keys. */
			static constexpr auto JKColor{"Color"};
			static constexpr auto JKLuminance{"Luminance"};
			static constexpr auto JKHalfWidth{"HalfWidth"};
			static constexpr auto JKCoreExponent{"CoreExponent"};
			static constexpr auto JKArcAmplitude{"ArcAmplitude"};
			static constexpr auto JKArcFrequency{"ArcFrequency"};
			static constexpr auto JKArcOctaves{"ArcOctaves"};
			static constexpr auto JKSeed{"Seed"};
			static constexpr auto JKRestrikeRate{"RestrikeRate"};
			static constexpr auto JKDrift{"Drift"};

			/* Defaults: a thin red laser, straight. */
			static constexpr auto DefaultLuminance{5000.0F};
			static constexpr auto DefaultHalfWidth{0.01F};
			static constexpr auto DefaultCoreExponent{2.0F};
			static constexpr auto DefaultArcFrequency{4.0F};
			static constexpr auto DefaultArcOctaves{5.0F};

			/**
			 * @brief Constructs a beam material.
			 * @param serviceProvider A reference to the service provider.
			 * @param name The name of the resource [std::move].
			 * @param resourceFlags The resource flag bits. Default none. (Unused yet)
			 */
			BeamResource (Resources::AbstractServiceProvider & serviceProvider, const std::string & name, uint32_t resourceFlags = 0) noexcept;

			/**
			 * @brief Copy constructor.
			 * @param copy A reference to the copied instance.
			 */
			BeamResource (const BeamResource & copy) noexcept = delete;

			/**
			 * @brief Move constructor.
			 * @param copy A reference to the copied instance.
			 */
			BeamResource (BeamResource && copy) noexcept = delete;

			/**
			 * @brief Copy assignment.
			 * @param copy A reference to the copied instance.
			 * @return BeamResource &
			 */
			BeamResource & operator= (const BeamResource & copy) noexcept = delete;

			/**
			 * @brief Move assignment.
			 * @param copy A reference to the copied instance.
			 * @return BeamResource &
			 */
			BeamResource & operator= (BeamResource && copy) noexcept = delete;

			/**
			 * @brief Destructs the beam material.
			 */
			~BeamResource () override
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

			/** @copydoc EmEn::Graphics::Material::Interface::writesGeometryBuffer() */
			[[nodiscard]]
			bool
			writesGeometryBuffer () const noexcept override
			{
				return false;
			}

			/**
			 * @copydoc EmEn::Graphics::Material::Interface::reactiveMaskExpression()
			 * @note 1 wherever the beam draws: an arc re-strikes in place, with no motion the history could follow, and a
			 * laser that sweeps leaves a light the TAA would otherwise average into a trail.
			 */
			[[nodiscard]]
			std::string reactiveMaskExpression () const noexcept override;

			/** @copydoc EmEn::Graphics::Material::Interface::setupLightGenerator() */
			[[nodiscard]]
			bool
			setupLightGenerator (Saphir::LightGenerator & /*lightGenerator*/) const noexcept override
			{
				/* Unlit: the beam IS light. */
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
			 * @note A beam is light: its blending is additive, always. Any other mode is refused with a warning.
			 */
			void enableBlending (BlendingMode mode) noexcept override;

			/** @copydoc EmEn::Graphics::Material::Interface::blendingMode() */
			[[nodiscard]]
			BlendingMode
			blendingMode () const noexcept override
			{
				return BlendingMode::Add;
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
			 * @brief Sets the colour of the beam.
			 * @note LINEAR, like every uniform colour of a material (StandardResource's albedo colour included): it is
			 * multiplied by the luminance as it is. Its brightness is the luminance's job — a pure hue is the intent.
			 * @param color A reference to a colour.
			 * @return void
			 */
			void setColor (const Base::PixelFactory::Color< float > & color) noexcept;

			/**
			 * @brief Returns the colour of the beam (linear).
			 * @return const Base::PixelFactory::Color< float > &
			 */
			[[nodiscard]]
			const Base::PixelFactory::Color< float > &
			color () const noexcept
			{
				return m_color;
			}

			/**
			 * @brief Sets the luminance of the beam's core, in nits (cd/m²) — Graphics/Photometry.hpp.
			 * @note References: a monitor 200-300 nits, a fluorescent tube ~10 000, a welding arc 10⁶ and more. The
			 * scene is pre-exposed: pick it against the exposure of the scene the beam lives in.
			 * @param nits The luminance. Clamped to 0 or above.
			 * @return void
			 */
			void setLuminance (float nits) noexcept;

			/**
			 * @brief Returns the luminance of the beam's core, in nits.
			 * @return float
			 */
			[[nodiscard]]
			float
			luminance () const noexcept
			{
				return m_luminance;
			}

			/**
			 * @brief Sets the half width of the beam, in the entity's units (metres for an unscaled entity).
			 * @note Below one pixel the ribbon keeps one pixel and its light is scaled down by the ratio (BeamGLSL).
			 * @param halfWidth The half width. Clamped to 0 or above.
			 * @return void
			 */
			void setHalfWidth (float halfWidth) noexcept;

			/**
			 * @brief Returns the half width of the beam.
			 * @return float
			 */
			[[nodiscard]]
			float
			halfWidth () const noexcept
			{
				return m_properties[ShapeOffset];
			}

			/**
			 * @brief Sets the cross-section profile: the light falls as (1 - side²)^exponent across the ribbon.
			 * @param exponent 1 is a soft glow, 4 a hard core. Clamped to at least 0.1.
			 * @return void
			 */
			void setCoreExponent (float exponent) noexcept;

			/**
			 * @brief Sets the arc: how far and how finely the beam wanders across its line. An amplitude of 0 is a
			 * straight laser.
			 * @param amplitude The largest offset across the beam, in the entity's units — Cascade's NoiseRange: the arc never
			 * exceeds it and its mean offset is half of it (Saphir::BeamGLSL::ArcGain). Clamped to 0 or above.
			 * @param frequency The number of noise cycles along the whole beam (its coarsest bends). Clamped to 0 or above.
			 * @param octaves The number of noise octaves (the detail of the discharge), 1 to Saphir::BeamGLSL::MaxOctaves (8).
			 * @return void
			 */
			void setArc (float amplitude, float frequency, uint32_t octaves) noexcept;

			/**
			 * @brief Sets how the arc lives: its seed, its re-strike rate and its drift.
			 * @param seed The seed: two beams of the same seed draw the same arc.
			 * @param restrikeRate How many new arcs per second (the seed changes, the arc jumps); 0 = never.
			 * @param drift How fast the noise scrolls along the beam between re-strikes, in noise cycles per second.
			 * @return void
			 */
			void setArcMotion (uint32_t seed, float restrikeRate, float drift) noexcept;

			/**
			 * @brief Returns the largest distance the arc can move a point of the beam from its line (culling margin).
			 * @return float
			 */
			[[nodiscard]]
			float
			arcAmplitude () const noexcept
			{
				return m_properties[ShapeOffset + 1];
			}

			/**
			 * @brief Returns the arc frequency, in noise cycles along the whole beam.
			 * @return float
			 */
			[[nodiscard]]
			float
			arcFrequency () const noexcept
			{
				return m_properties[ShapeOffset + 2];
			}

			/**
			 * @brief Returns the number of noise octaves of the arc.
			 * @return uint32_t
			 */
			[[nodiscard]]
			uint32_t
			arcOctaves () const noexcept
			{
				return static_cast< uint32_t >(m_properties[ShapeOffset + 3]);
			}

			/**
			 * @brief Returns the cross-section exponent.
			 * @return float
			 */
			[[nodiscard]]
			float
			coreExponent () const noexcept
			{
				return m_properties[RadianceOffset + 3];
			}

			/**
			 * @brief Returns the arc seed.
			 * @return uint32_t
			 */
			[[nodiscard]]
			uint32_t
			arcSeed () const noexcept
			{
				return static_cast< uint32_t >(m_properties[MotionOffset]);
			}

			/**
			 * @brief Returns the arc re-strike rate, in Hz (0 = never).
			 * @return float
			 */
			[[nodiscard]]
			float
			arcRestrikeRate () const noexcept
			{
				return m_properties[MotionOffset + 1];
			}

			/**
			 * @brief Returns the arc drift, in noise cycles per second.
			 * @return float
			 */
			[[nodiscard]]
			float
			arcDrift () const noexcept
			{
				return m_properties[MotionOffset + 2];
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

			/* UBO layout (std140, three vec4 — Keys BeamRadiance, BeamShape, BeamMotion). */
			static constexpr auto RadianceOffset{0UL};
			static constexpr auto ShapeOffset{4UL};
			static constexpr auto MotionOffset{8UL};
			static constexpr auto PropertiesSize{12UL};

			Physics::SurfacePhysicalProperties m_physicalSurfaceProperties;
			Base::PixelFactory::Color< float > m_color{Base::PixelFactory::Red};
			std::array< float, PropertiesSize > m_properties{
				/* Radiance (rgb), core exponent. */
				0.0F, 0.0F, 0.0F, DefaultCoreExponent,
				/* Half width, arc amplitude, arc frequency, octaves. */
				DefaultHalfWidth, 0.0F, DefaultArcFrequency, DefaultArcOctaves,
				/* Seed, re-strike rate, drift, unused. */
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
	using BeamMaterials = Container< Graphics::Material::BeamResource >;
}
