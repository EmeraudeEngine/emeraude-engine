/*
 * src/Scenes/SceneInstanceTransforms.hpp
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

/* STL inclusions. */
#include <cstdint>
#include <memory>
#include <type_traits>
#include <vector>

/* Local inclusions for usages. */
#include "Math/Matrix.hpp"
#include "Vulkan/DescriptorSet.hpp"
#include "Vulkan/ShaderStorageBufferObject.hpp"

/* Forward declarations. */
namespace EmEn
{
	namespace Graphics
	{
		class Renderer;
	}

	namespace Vulkan
	{
		class DeferredDestructor;
		class DescriptorSetLayout;
		class Device;
		class LayoutManager;
	}
}

namespace EmEn::Scenes
{
	/**
	 * @brief Manages the per-scene "InstanceTransforms" SSBO holding per-instance model matrices
	 * for the non-instanced rendering path, plus a view header for temporal effects (motion vectors).
	 * @note One buffer per frame-in-flight (double-buffering contract, same as SceneMetaData).
	 * Slots are frame-linear: the staging cursor is reset once per rendered frame by
	 * Scene::beginRenderFrame() (called by the Renderer before any Scene::prepareRender()),
	 * then every Scene::prepareRender() of the frame (render-to-textures first, main target last)
	 * appends the entries of its visible set and uploads the staged range. Each entry slot is
	 * consumed by the draws recorded between its staging and the next prepareRender(), so the
	 * same renderable instance may hold a different slot per render target within one frame
	 * (required for sprites, whose model matrix depends on the camera position).
	 * @note The header {viewProjection, previousViewProjection} is written by the primary view
	 * target only (RenderTargetType::View) and is reserved for the velocity/motion-vector pass;
	 * the regular matrix path pushes the view-projection matrix through push constants (MDI
	 * precedent) and only reads the entries.
	 */
	class EMEN_API SceneInstanceTransforms final
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"SceneInstanceTransforms"};

			/** @brief Initial per-frame buffer capacity, in entries. Buffers grow on demand (power of two). */
			static constexpr uint32_t InitialEntryCapacity{1024};

			/**
			 * @brief GPU layout of the buffer header (std430).
			 * @note Reserved for the motion-vector pass: the previous view-projection matrices of
			 * the primary view target, in both view forms (regular and infinity). Matrices are
			 * column-major, matching GLSL mat4. The CURRENT view-projection used to sit here and was
			 * read 0 times by the generated GLSL (every path gets it from push constants or from the
			 * view UBO) — that dead slot now carries the infinity variant, at no size cost.
			 * @note Both matrices are UNJITTERED: the TAA sub-pixel jitter never travels through a
			 * matrix, it is a per-draw push constant applied to gl_Position only. This is what keeps
			 * the velocity outputs jitter-free without any subtraction in the vertex shader.
			 */
			struct Header
			{
				Base::Math::Matrix< 4, float > previousViewProjectionMatrix;
				/** @brief Previous view-projection built with the translation-free INFINITY view, for
				 * the renderables rendered with it (the sky background). Mixing the two forms is a
				 * STRUCTURAL mismatch that does not cancel on a static camera. */
				Base::Math::Matrix< 4, float > previousViewProjectionInfinityMatrix;
			};

			/**
			 * @brief GPU layout of one per-instance entry (std430).
			 * @note Indexed in shaders by gl_BaseInstance (the slot is encoded in the
			 * firstInstance parameter of the draw command).
			 */
			struct Entry
			{
				Base::Math::Matrix< 4, float > modelMatrix;
				Base::Math::Matrix< 4, float > previousModelMatrix;
			};

			static_assert(sizeof(Header) == 128, "InstanceTransforms header must match the GPU layout (2 x mat4).");

			static_assert(sizeof(Entry) == 128, "InstanceTransforms entry must match the GPU layout (2 x mat4).");
			static_assert(std::is_trivially_copyable_v< Header > && std::is_trivially_copyable_v< Entry >, "InstanceTransforms structures must be trivially copyable (raw memcpy upload).");

			/**
			 * @brief Constructs the instance transforms manager.
			 * @param device A reference to the Vulkan device smart pointer.
			 * @param deferredDestructor The renderer deferred-destruction queue.
			 */
			SceneInstanceTransforms (const std::shared_ptr< Vulkan::Device > & device, Vulkan::DeferredDestructor * deferredDestructor) noexcept;

			SceneInstanceTransforms (const SceneInstanceTransforms & copy) noexcept = delete;

			SceneInstanceTransforms (SceneInstanceTransforms && copy) noexcept = delete;

			SceneInstanceTransforms & operator= (const SceneInstanceTransforms & copy) noexcept = delete;

			SceneInstanceTransforms & operator= (SceneInstanceTransforms && copy) noexcept = delete;

			/**
			 * @brief Destructs the instance transforms manager.
			 * @note Retires the SSBOs through the deferred destructor (frames may still be in flight).
			 */
			~SceneInstanceTransforms ();

			/**
			 * @brief Returns the cached descriptor set layout for the InstanceTransforms SSBO.
			 * @note Shared between the descriptor set allocation here and the pipeline layout
			 * creation in the Saphir generators (SetType::PerSceneTransforms).
			 * @param layoutManager A reference to the layout manager.
			 * @return std::shared_ptr< Vulkan::DescriptorSetLayout >
			 */
			[[nodiscard]]
			static std::shared_ptr< Vulkan::DescriptorSetLayout > getDescriptorSetLayout (Vulkan::LayoutManager & layoutManager) noexcept;

			/**
			 * @brief Initializes per-frame SSBO storage and descriptor sets for frames-in-flight synchronization.
			 * @param renderer A reference to the graphics renderer (frames-in-flight count, layout manager, descriptor pool).
			 * @return bool
			 */
			[[nodiscard]]
			bool initializePerFrameBuffers (Graphics::Renderer & renderer) noexcept;

			/**
			 * @brief Returns whether the per-frame buffers were successfully created.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isInitialized () const noexcept
			{
				return !m_buffers.empty();
			}

			/**
			 * @brief Resets the frame-linear staging cursor and targets the frame-in-flight buffer.
			 * @note Must be called once per rendered frame, before any Scene::prepareRender().
			 * The Renderer drives this through Scene::beginRenderFrame().
			 * @param frameIndex The current frame-in-flight index.
			 */
			void
			beginFrame (uint32_t frameIndex) noexcept
			{
				m_stagedFrameIndex = frameIndex;
				m_stagedEntries.clear();
			}

			/**
			 * @brief Stages the previous view-projection matrices for the primary view target.
			 * @note Reserved for the velocity/motion-vector pass. Only the primary view target
			 * (RenderTargetType::View) writes it; render-to-texture targets must not.
			 * @warning Both matrices MUST be unjittered (see the Header note): the TAA jitter is a
			 * per-draw push constant, it must never be baked in a matrix a velocity consumer reads.
			 * @param previousViewProjectionMatrix The previous frame view-projection matrix.
			 * @param previousViewProjectionInfinityMatrix The previous frame view-projection matrix
			 * built with the infinity (translation-free) view, for the sky background.
			 */
			void
			setPreviousViewProjectionMatrices (const Base::Math::Matrix< 4, float > & previousViewProjectionMatrix, const Base::Math::Matrix< 4, float > & previousViewProjectionInfinityMatrix) noexcept
			{
				m_stagedHeader.previousViewProjectionMatrix = previousViewProjectionMatrix;
				m_stagedHeader.previousViewProjectionInfinityMatrix = previousViewProjectionInfinityMatrix;
			}

			/**
			 * @brief Stages one per-instance entry and returns its frame-linear slot.
			 * @param modelMatrix The instance world model matrix.
			 * @param previousModelMatrix The instance world model matrix of the previous frame.
			 * @return uint32_t The slot to encode in the firstInstance draw parameter.
			 */
			[[nodiscard]]
			uint32_t
			stageEntry (const Base::Math::Matrix< 4, float > & modelMatrix, const Base::Math::Matrix< 4, float > & previousModelMatrix) noexcept
			{
				const auto slot = static_cast< uint32_t >(m_stagedEntries.size());

				m_stagedEntries.emplace_back(modelMatrix, previousModelMatrix);

				return slot;
			}

			/**
			 * @brief Uploads the staged header and entries to the current frame-in-flight SSBO.
			 * @note Called at the end of every Scene::prepareRender(). The upload always covers
			 * the whole staged range from the frame start, so successive calls within one frame
			 * are cumulative and idempotent. Grows the buffer when the staged range exceeds its
			 * capacity, retiring the previous buffer through the deferred destructor.
			 * @return bool
			 */
			[[nodiscard]]
			bool updateVideoMemory () noexcept;

			/**
			 * @brief Returns the instance transforms SSBO for the given frame index.
			 * @param frameIndex The current frame-in-flight index.
			 * @return const Vulkan::ShaderStorageBufferObject *
			 */
			[[nodiscard]]
			const Vulkan::ShaderStorageBufferObject *
			buffer (uint32_t frameIndex) const noexcept
			{
				if ( frameIndex >= m_buffers.size() )
				{
					return nullptr;
				}

				return m_buffers[frameIndex].get();
			}

			/**
			 * @brief Returns the descriptor set exposing the InstanceTransforms SSBO for the given frame index.
			 * @note Bound at the SetType::PerSceneTransforms index by programs consuming the SSBO.
			 * @param frameIndex The current frame-in-flight index.
			 * @return const Vulkan::DescriptorSet *
			 */
			[[nodiscard]]
			const Vulkan::DescriptorSet *
			descriptorSet (uint32_t frameIndex) const noexcept
			{
				if ( frameIndex >= m_descriptorSets.size() )
				{
					return nullptr;
				}

				return m_descriptorSets[frameIndex].get();
			}

			/**
			 * @brief Returns the number of entries staged for the current frame.
			 * @return uint32_t
			 */
			[[nodiscard]]
			uint32_t
			entryCount () const noexcept
			{
				return static_cast< uint32_t >(m_stagedEntries.size());
			}

		private:

			/**
			 * @brief Points the frame's descriptor set (binding 0) at the frame's SSBO.
			 * @param frameIndex The frame-in-flight index.
			 * @return bool
			 */
			[[nodiscard]]
			bool writeBufferToDescriptorSet (uint32_t frameIndex) noexcept;

			/** @brief Reference to the Vulkan device for SSBO creation. */
			std::shared_ptr< Vulkan::Device > m_device;
			/** @brief Deferred-destruction queue for in-flight-safe buffer retirement. */
			Vulkan::DeferredDestructor * m_deferredDestructor{nullptr};
			/** @brief Per-frame instance transforms SSBOs (one per frame-in-flight). */
			std::vector< std::unique_ptr< Vulkan::ShaderStorageBufferObject > > m_buffers;
			/** @brief Per-frame descriptor sets (binding 0 = the frame's SSBO); rewritten on buffer growth. */
			std::vector< std::unique_ptr< Vulkan::DescriptorSet > > m_descriptorSets;
			/** @brief Staged entries for the current frame (persistent capacity, cleared by beginFrame()). */
			std::vector< Entry > m_stagedEntries;
			/** @brief Staged header for the current frame (primary view target matrices). */
			Header m_stagedHeader{};
			/** @brief Frame-in-flight index targeted by the staging, set by beginFrame(). */
			uint32_t m_stagedFrameIndex{0};
	};
}
