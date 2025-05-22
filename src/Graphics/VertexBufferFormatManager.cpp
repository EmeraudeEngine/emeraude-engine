/*
 * src/Graphics/VertexBufferFormatManager.cpp
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

#include "VertexBufferFormatManager.hpp"

/* STL inclusions. */
#include <sstream>

/* Local inclusions. */
#include "Graphics/Geometry/Interface.hpp"
#include "Saphir/VertexShader.hpp"
#include "Vulkan/VertexBufferObject.hpp"
#include "Tracer.hpp"

namespace EmEn::Graphics
{
	using namespace Libs;

	const size_t VertexBufferFormatManager::ClassUID{getClassUID(ClassId)};

	VertexBufferFormatManager::VertexBufferFormatManager () noexcept
		: ServiceInterface(ClassId)
	{

	}

	bool
	VertexBufferFormatManager::onInitialize () noexcept
	{
		m_flags[ServiceInitialized] = true;

		return true;
	}

	bool
	VertexBufferFormatManager::onTerminate () noexcept
	{
		const std::lock_guard< std::mutex > lock{m_building};

		m_flags[ServiceInitialized] = false;

		this->resetBuildingParameters();

		m_vertexBufferFormats.clear();

		return true;
	}

	bool
	VertexBufferFormatManager::beginBinding (uint32_t binding) noexcept
	{
		if ( m_stagingVertexBufferFormat == nullptr )
		{
			Tracer::error(ClassId, "No vertex buffer format is currently built !");

			return false;
		}

		/* NOTE: Check the binding index. */
		if ( m_stagingVertexBufferFormat->binding(binding) != nullptr )
		{
			TraceError{ClassId} << "Binding index " << binding << " already declared !";

			return false;
		}

		m_currentBinding = binding;
		m_bindingOffset = 0;
		m_bindingElementCount = 0;

		return true;
	}

	bool
	VertexBufferFormatManager::declareAttribute (VertexAttributeType attribute) noexcept
	{
		if ( m_stagingVertexBufferFormat == nullptr )
		{
			return false;
		}

		constexpr std::array< VkFormat, 5 > formats{
			VK_FORMAT_UNDEFINED,
			VK_FORMAT_R32_SFLOAT,
			VK_FORMAT_R32G32_SFLOAT,
			VK_FORMAT_R32G32B32_SFLOAT,
			VK_FORMAT_R32G32B32A32_SFLOAT
		};

		const auto attributeSize = getAttributeSize(attribute);

		if ( attributeSize == 0 )
		{
			TraceError{ClassId} << "Unhandled vertex attribute type '" << to_string(attribute) << "' !";

			return false;
		}

		m_stagingVertexBufferFormat->setInputAttribute(
			static_cast< uint32_t >(attribute),
			m_currentBinding,
			formats[attributeSize],
			m_bindingOffset
		);

		/* Advancing with offset and elements count. */
		m_bindingOffset += static_cast< uint32_t >(attributeSize * sizeof(float));
		m_bindingElementCount += attributeSize;

		return true;
	}

	void
	VertexBufferFormatManager::declareJump (VertexAttributeType attribute) noexcept
	{
		const auto attributeSize = getAttributeSize(attribute);

		if ( attributeSize > 0 )
		{
			m_bindingOffset += static_cast< uint32_t >(attributeSize * sizeof(float));
		}
		else
		{
			TraceError{ClassId} << "Unhandled vertex attribute type '" << to_string(attribute) << "' !";
		}
	}

	bool
	VertexBufferFormatManager::endBinding (Topology topology, uint32_t bufferFlags) const noexcept
	{
		if ( m_stagingVertexBufferFormat == nullptr )
		{
			return false;
		}

		/* NOTE: Save the whole binding in an index. */
		m_stagingVertexBufferFormat->setBinding(
			m_currentBinding,
			m_bindingOffset,
			(bufferFlags & PerInstance) != 0U ? VK_VERTEX_INPUT_RATE_INSTANCE : VK_VERTEX_INPUT_RATE_VERTEX
		);

		/* NOTE: Creates the binding format. */
		m_stagingVertexBufferFormat->saveBindingData(
			m_currentBinding,
			m_bindingElementCount,
			topology,
			bufferFlags
		);

		return true;
	}

	std::shared_ptr< VertexBufferFormat >
	VertexBufferFormatManager::getVertexBufferFormat (const Saphir::VertexShader & vertexShader, Topology topology, uint32_t geometryFlagBits) noexcept
	{
		const std::lock_guard lock{m_building};

		m_stagingVertexBufferFormat = std::make_shared< VertexBufferFormat >();

		{
			/* NOTE: These are the used vertex attributes in the vertex shader. */
			const auto & requestedVertexAttributes = vertexShader.vertexAttributes();

			/* VBO format for vertices. */
			{
				this->beginBinding(0);

				if ( requestedVertexAttributes.contains(VertexAttributeType::Position) )
				{
					this->declareAttribute(VertexAttributeType::Position);
				}
				else
				{
					this->declareJump(VertexAttributeType::Position);
				}

				/* Check the uses of the full tangent space or normals. */
				if (
					requestedVertexAttributes.contains(VertexAttributeType::Tangent) &&
					requestedVertexAttributes.contains(VertexAttributeType::Binormal) &&
					requestedVertexAttributes.contains(VertexAttributeType::Normal)
				)
				{
					if ( FlagTrait< uint32_t >::disabled(geometryFlagBits,Geometry::EnableTangentSpace) )
					{
						TraceError{ClassId} << "The vertex shader '" << vertexShader.name() << "' needs tangent space !";

						return nullptr;
					}

					this->declareAttribute(VertexAttributeType::Tangent);
					this->declareAttribute(VertexAttributeType::Binormal);
					this->declareAttribute(VertexAttributeType::Normal);
				}
				else if ( requestedVertexAttributes.contains(VertexAttributeType::Normal) )
				{
					if ( FlagTrait< uint32_t >::disabled(geometryFlagBits, Geometry::EnableNormal) )
					{
						TraceError{ClassId} << "The vertex shader '" << vertexShader.name() << "' needs normal attributes !";

						return nullptr;
					}

					/* Check for attribute presence to jump over. */
					if ( FlagTrait< uint32_t >::enabled(geometryFlagBits,Geometry::EnableTangentSpace) )
					{
						this->declareJump(VertexAttributeType::Tangent);
						this->declareJump(VertexAttributeType::Binormal);
					}

					this->declareAttribute(VertexAttributeType::Normal);
				}
				else
				{
					/* Check for attribute presence to jump over. */
					if ( FlagTrait< uint32_t >::enabled(geometryFlagBits, Geometry::EnableTangentSpace) )
					{
						this->declareJump(VertexAttributeType::Tangent);
						this->declareJump(VertexAttributeType::Binormal);
						this->declareJump(VertexAttributeType::Normal);
					}
					else if ( FlagTrait< uint32_t >::enabled(geometryFlagBits,Geometry::EnableNormal) )
					{
						this->declareJump(VertexAttributeType::Normal);
					}
				}

				/* Check for primary texture coordinates conformance. */
				if (
					requestedVertexAttributes.contains(VertexAttributeType::Primary2DTextureCoordinates) ||
					requestedVertexAttributes.contains(VertexAttributeType::Primary3DTextureCoordinates)
				)
				{
					if ( requestedVertexAttributes.contains(VertexAttributeType::Primary2DTextureCoordinates) )
					{
						if ( FlagTrait< uint32_t >::disabled(geometryFlagBits, Geometry::EnablePrimaryTextureCoordinates) )
						{
							TraceError{ClassId} << "The vertex shader '" << vertexShader.name() << "' needs 2D primary texture coordinates attributes !";

							return nullptr;
						}

						this->declareAttribute(VertexAttributeType::Primary2DTextureCoordinates);
					}
					else
					{
						if ( FlagTrait< uint32_t >::disabled(geometryFlagBits, Geometry::Enable3DPrimaryTextureCoordinates) )
						{
							TraceError{ClassId} << "The vertex shader '" << vertexShader.name() << "' needs 3D primary texture coordinates attributes version !";

							return nullptr;
						}

						this->declareAttribute(VertexAttributeType::Primary3DTextureCoordinates);
					}
				}
				else if ( FlagTrait< uint32_t >::enabled(geometryFlagBits, Geometry::EnablePrimaryTextureCoordinates) )
				{
					if ( FlagTrait< uint32_t >::enabled(geometryFlagBits, Geometry::Enable3DPrimaryTextureCoordinates) )
					{
						this->declareJump(VertexAttributeType::Primary3DTextureCoordinates);
					}
					else
					{
						this->declareJump(VertexAttributeType::Primary2DTextureCoordinates);
					}
				}

				/* Check for secondary texture coordinates conformance. */
				if (
					requestedVertexAttributes.contains(VertexAttributeType::Secondary2DTextureCoordinates) ||
					requestedVertexAttributes.contains(VertexAttributeType::Secondary3DTextureCoordinates)
				)
				{
					if ( requestedVertexAttributes.contains(VertexAttributeType::Secondary2DTextureCoordinates) )
					{
						if ( FlagTrait< uint32_t >::disabled(geometryFlagBits, Geometry::EnableSecondaryTextureCoordinates) )
						{
							TraceError{ClassId} << "The vertex shader '" << vertexShader.name() << "' needs 2D secondary texture coordinates attributes !";

							return nullptr;
						}

						this->declareAttribute(VertexAttributeType::Secondary2DTextureCoordinates);
					}
					else
					{
						if ( FlagTrait< uint32_t >::disabled(geometryFlagBits, Geometry::Enable3DSecondaryTextureCoordinates) )
						{
							TraceError{ClassId} << "The vertex shader '" << vertexShader.name() << "' needs 3D secondary texture coordinates attributes version !";

							return nullptr;
						}

						this->declareAttribute(VertexAttributeType::Secondary3DTextureCoordinates);
					}
				}
				else if ( FlagTrait< uint32_t >::enabled(geometryFlagBits, Geometry::EnableSecondaryTextureCoordinates) )
				{
					if ( FlagTrait< uint32_t >::enabled(geometryFlagBits, Geometry::Enable3DSecondaryTextureCoordinates) )
					{
						this->declareJump(VertexAttributeType::Secondary3DTextureCoordinates);
					}
					else
					{
						this->declareJump(VertexAttributeType::Secondary2DTextureCoordinates);
					}
				}

				/* Checking the vertex color */
				if ( requestedVertexAttributes.contains(VertexAttributeType::VertexColor) )
				{
					if ( FlagTrait< uint32_t >::disabled(geometryFlagBits, Geometry::EnableVertexColor) )
					{
						TraceError{ClassId} << "The vertex shader '" << vertexShader.name() << "' needs vertex color attributes !";

						return nullptr;
					}

					this->declareAttribute(VertexAttributeType::VertexColor);
				}
				else
				{
					/* Check for attribute presence to jump over. */
					if ( FlagTrait< uint32_t >::enabled(geometryFlagBits, Geometry::EnableVertexColor) )
					{
						this->declareJump(VertexAttributeType::VertexColor);
					}
				}

				uint32_t bufferFlags{0};

				if ( FlagTrait< uint32_t >::enabled(geometryFlagBits, Geometry::EnableDynamicVertexBuffer) )
				{
					bufferFlags |= IsDynamicVertexBuffer;
				}

				if ( FlagTrait< uint32_t >::enabled(geometryFlagBits, Geometry::EnableAbsolutePosition) )
				{
					bufferFlags |= IsPositionAbsolute;
				}

				if ( FlagTrait< uint32_t >::enabled(geometryFlagBits, Geometry::EnablePrimitiveRestart) )
				{
					bufferFlags |= RequestPrimitiveRestart;
				}

				if ( !this->endBinding(topology, bufferFlags) )
				{
					return nullptr;
				}
			}

			/* VBO format for model matrices. */
			if ( vertexShader.isInstancingEnabled() )
			{
				this->beginBinding(1);

				if ( vertexShader.isBillBoardingEnabled() )
				{
					if ( requestedVertexAttributes.contains(VertexAttributeType::ModelPosition) )
					{
						this->declareAttribute(VertexAttributeType::ModelPosition);
					}
					else
					{
						this->declareJump(VertexAttributeType::ModelPosition);
					}

					if ( requestedVertexAttributes.contains(VertexAttributeType::ModelScaling) )
					{
						this->declareAttribute(VertexAttributeType::ModelScaling);
					}
					else
					{
						this->declareJump(VertexAttributeType::ModelScaling);
					}
				}
				else
				{
					/* NOTE: Matrix 4x4 declaration. */
					if ( requestedVertexAttributes.contains(VertexAttributeType::ModelMatrixR0) )
					{
						this->declareAttribute(VertexAttributeType::ModelMatrixR0);
						this->declareAttribute(VertexAttributeType::ModelMatrixR1);
						this->declareAttribute(VertexAttributeType::ModelMatrixR2);
						this->declareAttribute(VertexAttributeType::ModelMatrixR3);
					}
					else
					{
						this->declareJump(VertexAttributeType::ModelMatrixR0);
						this->declareJump(VertexAttributeType::ModelMatrixR1);
						this->declareJump(VertexAttributeType::ModelMatrixR2);
						this->declareJump(VertexAttributeType::ModelMatrixR3);
					}

					/* Check the uses of the full tangent space or normals. */
					if ( requestedVertexAttributes.contains(VertexAttributeType::NormalModelMatrixR0) )
					{
						/* NOTE: Matrix 3x3 declaration. */
						this->declareAttribute(VertexAttributeType::NormalModelMatrixR0);
						this->declareAttribute(VertexAttributeType::NormalModelMatrixR1);
						this->declareAttribute(VertexAttributeType::NormalModelMatrixR2);
					}
					else
					{
						/* Check for attribute presence to jump over. */
						this->declareJump(VertexAttributeType::NormalModelMatrixR0);
						this->declareJump(VertexAttributeType::NormalModelMatrixR1);
						this->declareJump(VertexAttributeType::NormalModelMatrixR2);
					}
				}

				if ( !this->endBinding(Topology::CustomData, PerInstance | IsDynamicVertexBuffer) )
				{
					return nullptr;
				}
			}
		}

		const auto vertexBufferFormat = m_vertexBufferFormats.emplace_back(m_stagingVertexBufferFormat);

		this->resetBuildingParameters();

		if ( m_flags[PrintGeneratedFormat] )
		{
			TraceInfo{ClassId} << "Geometry flags : " "\n" <<
				Geometry::getFlagsString(geometryFlagBits) << "\n" <<
				*vertexBufferFormat <<
				VertexBufferFormatManager::getVertexBufferObjectUsage(geometryFlagBits, *vertexBufferFormat);
		}

		return vertexBufferFormat;
	}

	std::shared_ptr< VertexBufferFormat >
	VertexBufferFormatManager::getVertexBufferFormat (const Saphir::VertexShader & vertexShader, const Geometry::Interface & geometry) noexcept
	{
		const std::lock_guard lock{m_building};
		
		m_stagingVertexBufferFormat = std::make_shared< VertexBufferFormat >();
		
		{
			/* NOTE: These are the used vertex attributes in the vertex shader. */
			const auto & requestedVertexAttributes = vertexShader.vertexAttributes();

			/* VBO format for vertices. */
			{
				this->beginBinding(0);

				if ( requestedVertexAttributes.contains(VertexAttributeType::Position) )
				{
					this->declareAttribute(VertexAttributeType::Position);
				}
				else
				{
					this->declareJump(VertexAttributeType::Position);
				}

				/* Check the uses of the full tangent space or normals. */
				if (
					requestedVertexAttributes.contains(VertexAttributeType::Tangent) &&
					requestedVertexAttributes.contains(VertexAttributeType::Binormal) &&
					requestedVertexAttributes.contains(VertexAttributeType::Normal)
				)
				{
					if ( !geometry.tangentSpaceEnabled() )
					{
						TraceError{ClassId} <<
							"The vertex shader '" << vertexShader.name() << "' needs tangent space, "
							"but the geometry '" << geometry.name() << "' do not provide it.";

						return nullptr;
					}

					this->declareAttribute(VertexAttributeType::Tangent);
					this->declareAttribute(VertexAttributeType::Binormal);
					this->declareAttribute(VertexAttributeType::Normal);
				}
				else if ( requestedVertexAttributes.contains(VertexAttributeType::Normal) )
				{
					if ( !geometry.normalEnabled() )
					{
						TraceError{ClassId} <<
							"The vertex shader '" << vertexShader.name() << "' needs normal attributes, "
							"but the geometry '" << geometry.name() << "' do not provide these.";

						return nullptr;
					}

					/* Check for attribute presence to jump over. */
					if ( geometry.tangentSpaceEnabled() )
					{
						this->declareJump(VertexAttributeType::Tangent);
						this->declareJump(VertexAttributeType::Binormal);
					}

					this->declareAttribute(VertexAttributeType::Normal);
				}
				else
				{
					/* Check for attribute presence to jump over. */
					if ( geometry.tangentSpaceEnabled() )
					{
						this->declareJump(VertexAttributeType::Tangent);
						this->declareJump(VertexAttributeType::Binormal);
						this->declareJump(VertexAttributeType::Normal);
					}
					else if ( geometry.normalEnabled() )
					{
						this->declareJump(VertexAttributeType::Normal);
					}
				}

				/* Check for primary texture coordinates conformance. */
				if (
					requestedVertexAttributes.contains(VertexAttributeType::Primary2DTextureCoordinates) ||
					requestedVertexAttributes.contains(VertexAttributeType::Primary3DTextureCoordinates)
				)
				{
					if ( requestedVertexAttributes.contains(VertexAttributeType::Primary2DTextureCoordinates) )
					{
						if ( !geometry.primaryTextureCoordinatesEnabled() )
						{
							TraceError{ClassId} <<
								"The vertex shader '" << vertexShader.name() << "' needs 2D primary texture coordinates attributes, "
								"but the geometry '" << geometry.name() << "' do not provide these.";

							return nullptr;
						}

						this->declareAttribute(VertexAttributeType::Primary2DTextureCoordinates);
					}
					else
					{
						if ( !geometry.primaryTextureCoordinates3DEnabled() )
						{
							TraceError{ClassId} <<
								"The vertex shader '" << vertexShader.name() << "' needs 3D primary texture coordinates attributes version, "
								"but the geometry '" << geometry.name() << "' do not provide these.";

							return nullptr;
						}

						this->declareAttribute(VertexAttributeType::Primary3DTextureCoordinates);
					}
				}
				else if ( geometry.primaryTextureCoordinatesEnabled() )
				{
					if ( geometry.primaryTextureCoordinates3DEnabled() )
					{
						this->declareJump(VertexAttributeType::Primary3DTextureCoordinates);
					}
					else
					{
						this->declareJump(VertexAttributeType::Primary2DTextureCoordinates);
					}
				}

				/* Check for secondary texture coordinates conformance. */
				if (
					requestedVertexAttributes.contains(VertexAttributeType::Secondary2DTextureCoordinates) ||
					requestedVertexAttributes.contains(VertexAttributeType::Secondary3DTextureCoordinates)
				)
				{
					if ( requestedVertexAttributes.contains(VertexAttributeType::Secondary2DTextureCoordinates) )
					{
						if ( !geometry.secondaryTextureCoordinatesEnabled() )
						{
							TraceError{ClassId} <<
								"The vertex shader '" << vertexShader.name() << "' needs 2D secondary texture coordinates attributes, "
								"but the geometry '" << geometry.name() << "' do not provide these.";

							return nullptr;
						}

						this->declareAttribute(VertexAttributeType::Secondary2DTextureCoordinates);
					}
					else
					{
						if ( !geometry.secondaryTextureCoordinates3DEnabled() )
						{
							TraceError{ClassId} <<
								"The vertex shader '" << vertexShader.name() << "' needs 3D secondary texture coordinates attributes version, "
								"but the geometry '" << geometry.name() << "' do not provide these.";

							return nullptr;
						}

						this->declareAttribute(VertexAttributeType::Secondary3DTextureCoordinates);
					}
				}
				else if ( geometry.secondaryTextureCoordinatesEnabled() )
				{
					if ( geometry.secondaryTextureCoordinates3DEnabled() )
					{
						this->declareJump(VertexAttributeType::Secondary3DTextureCoordinates);
					}
					else
					{
						this->declareJump(VertexAttributeType::Secondary2DTextureCoordinates);
					}
				}

				/* Checking the vertex color */
				if ( requestedVertexAttributes.contains(VertexAttributeType::VertexColor) )
				{
					if ( !geometry.vertexColorEnabled() )
					{
						TraceError{ClassId} <<
							"The vertex shader '" << vertexShader.name() << "' needs vertex color attributes, "
							"but the geometry '" << geometry.name() << "' do not provide these.";

						return nullptr;
					}

					this->declareAttribute(VertexAttributeType::VertexColor);
				}
				else
				{
					/* Check for attribute presence to jump over. */
					if ( geometry.vertexColorEnabled() )
					{
						this->declareJump(VertexAttributeType::VertexColor);
					}
				}

				uint32_t bufferFlags{0};

				if ( geometry.useDynamicVertexBuffer() )
				{
					bufferFlags |= IsDynamicVertexBuffer;
				}

				if ( geometry.useAbsolutePosition() )
				{
					bufferFlags |= IsPositionAbsolute;
				}

				if ( geometry.usePrimitiveRestart() )
				{
					bufferFlags |= RequestPrimitiveRestart;
				}

				if ( !this->endBinding(geometry.topology(), bufferFlags) )
				{
					return nullptr;
				}
			}

			/* VBO format for model matrices. */
			if ( vertexShader.isInstancingEnabled() )
			{
				this->beginBinding(1);

				if ( vertexShader.isBillBoardingEnabled() )
				{
					if ( requestedVertexAttributes.contains(VertexAttributeType::ModelPosition) )
					{
						this->declareAttribute(VertexAttributeType::ModelPosition);
					}
					else
					{
						this->declareJump(VertexAttributeType::ModelPosition);
					}

					if ( requestedVertexAttributes.contains(VertexAttributeType::ModelScaling) )
					{
						this->declareAttribute(VertexAttributeType::ModelScaling);
					}
					else
					{
						this->declareJump(VertexAttributeType::ModelScaling);
					}
				}
				else
				{
					/* NOTE: Matrix 4x4 declaration. */
					if ( requestedVertexAttributes.contains(VertexAttributeType::ModelMatrixR0) )
					{
						this->declareAttribute(VertexAttributeType::ModelMatrixR0);
						this->declareAttribute(VertexAttributeType::ModelMatrixR1);
						this->declareAttribute(VertexAttributeType::ModelMatrixR2);
						this->declareAttribute(VertexAttributeType::ModelMatrixR3);
					}
					else
					{
						this->declareJump(VertexAttributeType::ModelMatrixR0);
						this->declareJump(VertexAttributeType::ModelMatrixR1);
						this->declareJump(VertexAttributeType::ModelMatrixR2);
						this->declareJump(VertexAttributeType::ModelMatrixR3);
					}

					/* Check the uses of the full tangent space or normals. */
					if ( requestedVertexAttributes.contains(VertexAttributeType::NormalModelMatrixR0) )
					{
						/* NOTE: Matrix 3x3 declaration. */
						this->declareAttribute(VertexAttributeType::NormalModelMatrixR0);
						this->declareAttribute(VertexAttributeType::NormalModelMatrixR1);
						this->declareAttribute(VertexAttributeType::NormalModelMatrixR2);
					}
					else
					{
						/* Check for attribute presence to jump over. */
						this->declareJump(VertexAttributeType::NormalModelMatrixR0);
						this->declareJump(VertexAttributeType::NormalModelMatrixR1);
						this->declareJump(VertexAttributeType::NormalModelMatrixR2);
					}
				}

				if ( !this->endBinding(Topology::CustomData, PerInstance | IsDynamicVertexBuffer) )
				{
					return nullptr;
				}
			}
		}

		const auto vertexBufferFormat = m_vertexBufferFormats.emplace_back(m_stagingVertexBufferFormat);

		this->resetBuildingParameters();

		if ( m_flags[PrintGeneratedFormat] )
		{
			TraceInfo{ClassId} << "Geometry flags : " "\n" <<
				Geometry::getFlagsString(geometry.flags()) << "\n" <<
				*vertexBufferFormat <<
				VertexBufferFormatManager::getVertexBufferObjectUsage(geometry, *vertexBufferFormat);
		}

		return vertexBufferFormat;
	}

	std::string
	VertexBufferFormatManager::getVertexBufferObjectUsage (const Geometry::Interface & geometry, const VertexBufferFormat & vertexBufferFormat) noexcept
	{
		std::stringstream output;

		size_t elementCount = 3;

		output << "VBO #0 enabled vertex attribute layout :" "\n\n" <<
			   ( vertexBufferFormat.isPresent(VertexAttributeType::Position) ? "[3:position]" : "{3:position}" );

		if ( geometry.tangentSpaceEnabled() )
		{
			elementCount += 9;

			output << (vertexBufferFormat.isPresent(VertexAttributeType::Tangent) ? "[3:tangent]" : "{3:tangent}" );
			output << (vertexBufferFormat.isPresent(VertexAttributeType::Binormal) ? "[3:binormal]" : "{3:binormal}" );
			output << (vertexBufferFormat.isPresent(VertexAttributeType::Normal) ? "[3:normal]" : "{3:normal}" );
		}
		else if ( geometry.normalEnabled() )
		{
			elementCount += 3;

			output << (vertexBufferFormat.isPresent(VertexAttributeType::Normal) ? "[3:normal]" : "{3:normal}" );
		}

		if ( geometry.primaryTextureCoordinatesEnabled() )
		{
			if ( geometry.primaryTextureCoordinates3DEnabled() )
			{
				elementCount += 3;

				output << (vertexBufferFormat.isPresent(VertexAttributeType::Primary3DTextureCoordinates) ? "[3:priTexCoord]" : "{3:priTexCoord}" );
			}
			else
			{
				elementCount += 2;

				output << (vertexBufferFormat.isPresent(VertexAttributeType::Primary2DTextureCoordinates) ? "[2:priTexCoord]" : "{2:priTexCoord}" );
			}
		}

		if ( geometry.secondaryTextureCoordinatesEnabled() )
		{
			if ( geometry.secondaryTextureCoordinates3DEnabled() )
			{
				elementCount += 3;

				output << (vertexBufferFormat.isPresent(VertexAttributeType::Secondary3DTextureCoordinates) ? "[3:secTexCoord]" : "{3:secTexCoord}" );
			}
			else
			{
				elementCount += 2;

				output << (vertexBufferFormat.isPresent(VertexAttributeType::Secondary2DTextureCoordinates) ? "[2:secTexCoord]" : "{2:secTexCoord}" );
			}
		}

		if ( geometry.vertexColorEnabled() )
		{
			elementCount += 4;

			output << (vertexBufferFormat.isPresent(VertexAttributeType::VertexColor) ? "[4:color]" : "{4:color}" );
		}

		output << "\n" "Total element count: " << elementCount << "\n";

		const auto VBOElementCount = geometry.vertexBufferObject()->vertexElementCount();

		if ( VBOElementCount != elementCount )
		{
			TraceError{ClassId} << "Element count per vertex mismatch between VBO (" << VBOElementCount << ") and the declaration from geometry flags (" << elementCount << ") !";
		}

		return output.str();
	}

	std::string
	VertexBufferFormatManager::getVertexBufferObjectUsage (uint32_t geometryFlagBits, const VertexBufferFormat & vertexBufferFormat) noexcept
	{
		std::stringstream output;

		size_t elementCount = 3;

		output << "VBO #0 enabled vertex attribute layout :" "\n\n" <<
			   ( vertexBufferFormat.isPresent(VertexAttributeType::Position) ? "[3:position]" : "{3:position}" );

		if ( FlagTrait< uint32_t >::enabled(geometryFlagBits, Geometry::EnableTangentSpace) )
		{
			elementCount += 9;

			output << (vertexBufferFormat.isPresent(VertexAttributeType::Tangent) ? "[3:tangent]" : "{3:tangent}" );
			output << (vertexBufferFormat.isPresent(VertexAttributeType::Binormal) ? "[3:binormal]" : "{3:binormal}" );
			output << (vertexBufferFormat.isPresent(VertexAttributeType::Normal) ? "[3:normal]" : "{3:normal}" );
		}
		else if ( FlagTrait< uint32_t >::enabled(geometryFlagBits, Geometry::EnableNormal) )
		{
			elementCount += 3;

			output << (vertexBufferFormat.isPresent(VertexAttributeType::Normal) ? "[3:normal]" : "{3:normal}" );
		}

		if ( FlagTrait< uint32_t >::enabled(geometryFlagBits,Geometry::EnablePrimaryTextureCoordinates) )
		{
			if ( FlagTrait< uint32_t >::enabled(geometryFlagBits,Geometry::Enable3DPrimaryTextureCoordinates) )
			{
				elementCount += 3;

				output << (vertexBufferFormat.isPresent(VertexAttributeType::Primary3DTextureCoordinates) ? "[3:priTexCoord]" : "{3:priTexCoord}" );
			}
			else
			{
				elementCount += 2;

				output << (vertexBufferFormat.isPresent(VertexAttributeType::Primary2DTextureCoordinates) ? "[2:priTexCoord]" : "{2:priTexCoord}" );
			}
		}

		if ( FlagTrait< uint32_t >::enabled(geometryFlagBits,Geometry::EnableSecondaryTextureCoordinates) )
		{
			if ( FlagTrait< uint32_t >::enabled(geometryFlagBits,Geometry::Enable3DSecondaryTextureCoordinates) )
			{
				elementCount += 3;

				output << (vertexBufferFormat.isPresent(VertexAttributeType::Secondary3DTextureCoordinates) ? "[3:secTexCoord]" : "{3:secTexCoord}" );
			}
			else
			{
				elementCount += 2;

				output << (vertexBufferFormat.isPresent(VertexAttributeType::Secondary2DTextureCoordinates) ? "[2:secTexCoord]" : "{2:secTexCoord}" );
			}
		}

		if ( FlagTrait< uint32_t >::enabled(geometryFlagBits,Geometry::EnableVertexColor) )
		{
			elementCount += 4;

			output << (vertexBufferFormat.isPresent(VertexAttributeType::VertexColor) ? "[4:color]" : "{4:color}" );
		}

		output << "\n" "Total element count: " << elementCount << "\n";

		return output.str();
	}
}
