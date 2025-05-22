/*
 * src/Graphics/Geometry/Interface.hpp
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
#include <cstdint>
#include <string>
#include <vector>

/* Local inclusions for inheritances. */
#include "Resources/ResourceTrait.hpp"

/* Local inclusions for usages. */
#include "Libs/VertexFactory/Grid.hpp"
#include "Libs/VertexFactory/Shape.hpp"
#include "Vulkan/VertexBufferObject.hpp"
#include "Vulkan/IndexBufferObject.hpp"
#include "Graphics/Types.hpp"
#include "Types.hpp"
#include "SubGeometry.hpp"

namespace EmEn::Graphics::Geometry
{
	/**
	 * @brief This is the base of all geometry compatible with the graphic engine.
	 * @extends EmEn::Resources::ResourceTrait Every material is a resource.
	 */
	class Interface : public Resources::ResourceTrait
	{
		public:

			/**
			 * @brief Returns whether the tangent space is present in local data.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			tangentSpaceEnabled () const noexcept
			{
				return this->isFlagEnabled(EnableTangentSpace);
			}

			/**
			 * @brief Returns whether normals are present in local data.
			 * @note If the tangent space is present, normals are available.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			normalEnabled () const noexcept
			{
				if ( this->isFlagEnabled(EnableTangentSpace) )
				{
					return true;
				}

				return this->isFlagEnabled(EnableNormal);
			}

			/**
			 * @brief Returns whether the primary texture coordinates are present in local data.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			primaryTextureCoordinatesEnabled () const noexcept
			{
				return this->isFlagEnabled(EnablePrimaryTextureCoordinates);
			}

			/**
			 * @brief Returns whether alternate texture coordinates are present in local data.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			secondaryTextureCoordinatesEnabled () const noexcept
			{
				return this->isFlagEnabled(EnableSecondaryTextureCoordinates);
			}

			/**
			 * @brief Returns whether vertex colors are present in local data.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			vertexColorEnabled () const noexcept
			{
				return this->isFlagEnabled(EnableVertexColor);
			}

			/**
			 * @brief Returns whether influences are present in local data.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			influenceEnabled () const noexcept
			{
				return this->isFlagEnabled(EnableInfluence);
			}

			/**
			 * @brief Returns whether weights are present in local data.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			weightEnabled () const noexcept
			{
				return this->isFlagEnabled(EnableWeight);
			}

			/**
			 * @brief Returns whether primary texture coordinates are 3D.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			primaryTextureCoordinates3DEnabled () const noexcept
			{
				if ( !this->isFlagEnabled(EnablePrimaryTextureCoordinates) )
				{
					return false;
				}

				return this->isFlagEnabled(Enable3DPrimaryTextureCoordinates);
			}

			/**
			 * @brief Returns whether secondary texture coordinates are 3D.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			secondaryTextureCoordinates3DEnabled () const noexcept
			{
				if ( !this->isFlagEnabled(EnableSecondaryTextureCoordinates) )
				{
					return false;
				}

				return this->isFlagEnabled(Enable3DSecondaryTextureCoordinates);
			}

			/**
			 * @brief Returns whether the geometry needs dynamic VBO.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			useDynamicVertexBuffer () const noexcept
			{
				return this->isFlagEnabled(EnableDynamicVertexBuffer);
			}

			/**
			 * @brief Returns whether the geometry defines absolute positions.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			useAbsolutePosition () const noexcept
			{
				return this->isFlagEnabled(EnableAbsolutePosition);
			}

			/**
			 * @brief Returns whether the geometry use primitive restart index.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			usePrimitiveRestart () const noexcept
			{
				return this->isFlagEnabled(EnablePrimitiveRestart);
			}

			/**
			 * @brief Returns whether the geometry is ready.
			 * @return bool
			 */
			[[nodiscard]]
			virtual bool isCreated () const noexcept = 0;

			/**
			 * @brief Returns the topology to draw this geometry.
			 * @return Topology
			 */
			[[nodiscard]]
			virtual Topology topology () const noexcept = 0;

			/**
			 * @brief subGeometryCount
			 * @return uint32_t
			 */
			[[nodiscard]]
			virtual uint32_t subGeometryCount () const noexcept = 0;

			/**
			 * @brief Returns the sub-geometry range [offset, length].
			 * @param subGeometryIndex The sub-geometry index.
			 * @return std::array< uint32_t, 2 >
			 */
			[[nodiscard]]
			virtual std::array< uint32_t, 2 > subGeometryRange (uint32_t subGeometryIndex) const noexcept = 0;

			/**
			 * @brief Returns the bounding box surrounding the renderable.
			 * @return const Libs::Math::Space3D::AACuboid< float > &
			 */
			[[nodiscard]]
			virtual const Libs::Math::Space3D::AACuboid< float > & boundingBox () const noexcept = 0;

			/**
			 * @brief Returns the bounding sphere surrounding the renderable.
			 * @return const Libs::Math::Space3D::Sphere< float > &
			 */
			[[nodiscard]]
			virtual const Libs::Math::Space3D::Sphere< float > & boundingSphere () const noexcept = 0;

			/**
			 * @brief Returns the vertex buffer object.
			 * @return const Vulkan::VertexBufferObject *
			 */
			[[nodiscard]]
			virtual const Vulkan::VertexBufferObject * vertexBufferObject () const noexcept = 0;

			/**
			 * @brief Returns the index buffer object.
			 * @return const Vulkan::IndexBufferObject *
			 */
			[[nodiscard]]
			virtual const Vulkan::IndexBufferObject * indexBufferObject () const noexcept = 0;

			/**
			 * @brief Returns whether the geometry use an index buffer object for drawing.
			 * @return bool
			 */
			[[nodiscard]]
			virtual bool useIndexBuffer () const noexcept = 0;

			/**
			 * @brief Creates the geometry in the video memory from the local data.
			 * @return bool
			 */
			virtual bool create () noexcept = 0;

			/**
			 * @brief Updates the geometry in the video memory from the local data.
			 * @note The geometry must have been previously created in video memory.
			 * @return bool
			 */
			virtual bool update () noexcept = 0;

			/**
			 * @brief Destroys resource.
			 * @param clearLocalData Erase local data too.
			 * @return void
			 */
			virtual void destroy (bool clearLocalData) noexcept = 0;

		protected:

			/**
			 * @brief Constructs an abstract geometry.
			 * @param name A reference to a string for the resource name.
			 * @param geometryFlagBits The geometry resource flag bits.
			 */
			Interface (const std::string & name, uint32_t geometryFlagBits) noexcept;

			/**
			 * @brief Returns the size of the primary texture coordinates attribute.
			 * @return uint32_t
			 */
			[[nodiscard]]
			uint32_t
			getPrimaryTextureCoordinatesAttribSize () const noexcept
			{
				if ( !this->isFlagEnabled(EnablePrimaryTextureCoordinates) )
				{
					return 0;
				}

				return this->isFlagEnabled(Enable3DPrimaryTextureCoordinates) ? 3 : 2;
			}

			/**
			 * @brief Returns the size of the secondary texture coordinates attribute.
			 * @return uint32_t
			 */
			[[nodiscard]]
			uint32_t
			getSecondaryTextureCoordinatesAttribSize () const noexcept
			{
				if ( !this->isFlagEnabled(EnableSecondaryTextureCoordinates) )
				{
					return 0;
				}

				return this->isFlagEnabled(Enable3DSecondaryTextureCoordinates) ? 3 : 2;
			}

			/**
			 * @brief Returns the normals format.
			 * @return Libraries::VertexFactory::NormalType
			 */
			Libs::VertexFactory::NormalType
			getNormalsFormat () const noexcept
			{
				using namespace EmEn::Libs::VertexFactory;

				if ( this->isFlagEnabled(EnableTangentSpace) )
				{
					return NormalType::TBNSpace;
				}

				if ( this->isFlagEnabled(EnableNormal) )
				{
					return NormalType::Normal;
				}

				return NormalType::None;
			}

			/**
			 * @brief Returns the primary texture coordinates format.
			 * @return Libraries::VertexFactory::TextureCoordinatesType
			 */
			Libs::VertexFactory::TextureCoordinatesType
			getPrimaryTextureCoordinatesFormat () const noexcept
			{
				using namespace EmEn::Libs::VertexFactory;

				if ( !this->isFlagEnabled(EnablePrimaryTextureCoordinates) )
				{
					return TextureCoordinatesType::None;
				}

				return this->isFlagEnabled(Enable3DPrimaryTextureCoordinates) ? TextureCoordinatesType::UVW : TextureCoordinatesType::UV;
			}

			/**
			 * @brief Returns the secondary texture coordinates format.
			 * @return Libraries::VertexFactory::TextureCoordinatesType
			 */
			Libs::VertexFactory::TextureCoordinatesType
			getSecondaryTextureCoordinatesFormat () const noexcept
			{
				using namespace EmEn::Libs::VertexFactory;

				if ( !this->isFlagEnabled(EnableSecondaryTextureCoordinates) )
				{
					return TextureCoordinatesType::None;
				}

				return this->isFlagEnabled(Enable3DSecondaryTextureCoordinates) ? TextureCoordinatesType::UVW : TextureCoordinatesType::UV;
			}

			/**
			 * @brief Builds a simple geometry batch by specifying only the vertex count.
			 * @param length The vertex count.
			 * @param subGeometries A reference to a sub geometry list.
			 * @return bool
			 */
			static bool buildSubGeometries (std::vector< SubGeometry > & subGeometries, uint32_t length) noexcept;

			/**
			 * @brief Builds geometry batches from shape.
			 * @param shape A reference to a shape.
			 * @param subGeometries A reference to a sub geometry list.
			 * @return bool
			 */
			static bool buildSubGeometries (std::vector< SubGeometry > & subGeometries, const Libs::VertexFactory::Shape< float > & shape) noexcept;

			/**
			 * @brief Builds geometry batches from grid.
			 * @param grid A reference to a grid.
			 * @param subGeometries A reference to a sub geometry list.
			 * @return bool
			 */
			static bool buildSubGeometries (std::vector< SubGeometry > & subGeometries, const Libs::VertexFactory::Grid< float > & grid) noexcept;

		private:

			/** @copydoc EmEn::Resources::ResourceTrait::onDependenciesLoaded() */
			[[nodiscard]]
			bool onDependenciesLoaded () noexcept override;
	};
}
