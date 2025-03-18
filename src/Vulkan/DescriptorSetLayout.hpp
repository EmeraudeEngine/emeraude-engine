/*
 * src/Vulkan/DescriptorSetLayout.hpp
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
#include <cstddef>
#include <cstdint>
#include <ostream>
#include <memory>
#include <string>
#include <vector>

/* Local inclusions for inheritances. */
#include "AbstractDeviceDependentObject.hpp"

namespace EmEn::Vulkan
{
	/**
	 * @brief The DescriptorSetLayout class
	 * @extends EmEn::Vulkan::AbstractDeviceDependentObject This is a device dependant vulkan object.
	 */
	class DescriptorSetLayout final : public AbstractDeviceDependentObject
	{
		public:

			enum Flag : uint8_t
			{
				UseLocationVBO = 1
			};

			/** @brief Class identifier. */
			static constexpr auto ClassId{"VulkanDescriptorSetLayout"};

			/**
			 * @brief Constructs a descriptor set layout.
			 * @param device A reference to a smart pointer of a device.
			 * @param UUID A reference to a string [std::move].
			 * @param createFlags The create info flags. Default none.
			 */
			explicit DescriptorSetLayout (const std::shared_ptr< Device > & device, std::string UUID, VkDescriptorSetLayoutCreateFlags createFlags = 0) noexcept;

			/**
			 * @brief Constructs a descriptor set layout with create info.
			 * @param device A reference to a smart pointer to device where the render pass will be performed.
			 * @param UUID A reference to a string [std::move].
			 * @param createInfo A reference to a create info.
			 */
			DescriptorSetLayout (const std::shared_ptr< Device > & device, std::string UUID, const VkDescriptorSetLayoutCreateInfo & createInfo) noexcept;

			/**
			 * @brief Copy constructor.
			 * @param copy A reference to the copied instance.
			 */
			DescriptorSetLayout (const DescriptorSetLayout & copy) noexcept = default;

			/**
			 * @brief Move constructor.
			 * @param copy A reference to the copied instance.
			 */
			DescriptorSetLayout (DescriptorSetLayout && copy) noexcept = default;

			/**
			 * @brief Copy assignment.
			 * @param copy A reference to the copied instance.
			 */
			DescriptorSetLayout & operator= (const DescriptorSetLayout & copy) noexcept = default;

			/**
			 * @brief Move assignment.
			 * @param copy A reference to the copied instance.
			 */
			DescriptorSetLayout & operator= (DescriptorSetLayout && copy) noexcept = default;

			/**
			 * @brief Destructs the descriptor set layout.
			 */
			~DescriptorSetLayout () override;

			/** @copydoc EmEn::Vulkan::AbstractDeviceDependentObject::createOnHardware() */
			bool createOnHardware () noexcept override;

			/** @copydoc EmEn::Vulkan::AbstractDeviceDependentObject::destroyFromHardware() */
			bool destroyFromHardware () noexcept override;

			/**
			 * @brief Returns the UUID of the descriptor set layout.
			 * @return const std::string &
			 */
			[[nodiscard]]
			const std::string &
			UUID () const noexcept
			{
				return m_UUID;
			}

			/**
			 * @brief Declares a set layout binding.
			 * @param setLayoutBinding
			 * @return bool
			 */
			bool declare (VkDescriptorSetLayoutBinding setLayoutBinding) noexcept;

			/**
			 * @brief Declares a sampler binding.
			 * @param binding The binding point.
			 * @param stageFlags Define at which stage of the shader the bond appears. Default all.
			 * @param descriptorCount Define the number of descriptor to bind. Default 1.
			 * @param pImmutableSamplers The immutable samplers to bind. Default none.
			 * @return bool
			 */
			bool
			declareSampler (uint32_t binding, VkShaderStageFlags stageFlags = VK_SHADER_STAGE_ALL, uint32_t descriptorCount = 1, const VkSampler * pImmutableSamplers = nullptr) noexcept
			{
				return this->declare(VkDescriptorSetLayoutBinding{
					binding,
					VK_DESCRIPTOR_TYPE_SAMPLER,
					descriptorCount,
					stageFlags,
					pImmutableSamplers
				});
			}

			/**
			 * @brief Declare combined image sampler binding.
			 * @param binding The binding point.
			 * @param stageFlags Define at which stage of the shader the bond appears. Default all.
			 * @param descriptorCount Define the number of descriptor to bind. Default 1.
			 * @param pImmutableSamplers he immutable samplers to bind. Default none.
			 * @return bool
			 */
			bool
			declareCombinedImageSampler (uint32_t binding, VkShaderStageFlags stageFlags = VK_SHADER_STAGE_ALL, uint32_t descriptorCount = 1, const VkSampler * pImmutableSamplers = nullptr) noexcept
			{
				return this->declare(VkDescriptorSetLayoutBinding{
					binding,
					VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
					descriptorCount,
					stageFlags,
					pImmutableSamplers
				});
			}

			/**
			 * @brief Declares sampled image binding.
			 * @param binding The binding point.
			 * @param stageFlags Define at which stage of the shader the bond appears. Default all.
			 * @param descriptorCount Define the number of descriptor to bind. Default 1.
			 * @return bool
			 */
			bool
			declareSampledImage (uint32_t binding, VkShaderStageFlags stageFlags = VK_SHADER_STAGE_ALL, uint32_t descriptorCount = 1) noexcept
			{
				return this->declare(VkDescriptorSetLayoutBinding{
					binding,
					VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
					descriptorCount,
					stageFlags,
					nullptr
				});
			}

			/**
			 * @brief Declares storage image binding.
			 * @param binding The binding point.
			 * @param stageFlags Define at which stage of the shader the bond appears. Default all.
			 * @param descriptorCount Define the number of descriptor to bind. Default 1.
			 * @return bool
			 */
			bool
			declareStorageImage (uint32_t binding, VkShaderStageFlags stageFlags = VK_SHADER_STAGE_ALL, uint32_t descriptorCount = 1) noexcept
			{
				return this->declare(VkDescriptorSetLayoutBinding{
					binding,
					VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
					descriptorCount,
					stageFlags,
					nullptr
				});
			}

			/**
			 * @brief Declares uniform texel buffer binding.
			 * @param binding The binding point.
			 * @param stageFlags Define at which stage of the shader the bond appears. Default all.
			 * @param descriptorCount Define the number of descriptor to bind. Default 1.
			 * @return bool
			 */
			bool
			declareUniformTexelBuffer (uint32_t binding, VkShaderStageFlags stageFlags = VK_SHADER_STAGE_ALL, uint32_t descriptorCount = 1) noexcept
			{
				return this->declare(VkDescriptorSetLayoutBinding{
					binding,
					VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER,
					descriptorCount,
					stageFlags,
					nullptr
				});
			}

			/**
			 * @brief Declares storage texel buffer binding.
			 * @param binding The binding point.
			 * @param stageFlags Define at which stage of the shader the bond appears. Default all.
			 * @param descriptorCount Define the number of descriptor to bind. Default 1.
			 * @return bool
			 */
			bool
			declareStorageTexelBuffer (uint32_t binding, VkShaderStageFlags stageFlags = VK_SHADER_STAGE_ALL, uint32_t descriptorCount = 1) noexcept
			{
				return this->declare(VkDescriptorSetLayoutBinding{
					binding,
					VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER,
					descriptorCount,
					stageFlags,
					nullptr
				});
			}

			/**
			 * @brief Declares uniform buffer binding.
			 * @param binding The binding point.
			 * @param stageFlags Define at which stage of the shader the bond appears. Default all.
			 * @param descriptorCount Define the number of descriptor to bind. Default 1.
			 * @return bool
			 */
			bool
			declareUniformBuffer (uint32_t binding, VkShaderStageFlags stageFlags = VK_SHADER_STAGE_ALL, uint32_t descriptorCount = 1) noexcept
			{
				return this->declare(VkDescriptorSetLayoutBinding{
					binding,
					VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
					descriptorCount,
					stageFlags,
					nullptr
				});
			}

			/**
			 * @brief Declares storage buffer binding.
			 * @param binding The binding point.
			 * @param stageFlags Define at which stage of the shader the bond appears. Default all.
			 * @param descriptorCount Define the number of descriptor to bind. Default 1.
			 * @return bool
			 */
			bool
			declareStorageBuffer (uint32_t binding, VkShaderStageFlags stageFlags = VK_SHADER_STAGE_ALL, uint32_t descriptorCount = 1) noexcept
			{
				return this->declare(VkDescriptorSetLayoutBinding{
					binding,
					VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
					descriptorCount,
					stageFlags,
					nullptr
				});
			}

			/**
			 * @brief Declares uniform buffer dynamic binding.
			 * @param binding The binding point.
			 * @param stageFlags Define at which stage of the shader the bond appears. Default all.
			 * @param descriptorCount Define the number of descriptor to bind. Default 1.
			 * @return bool
			 */
			bool
			declareUniformBufferDynamic (uint32_t binding, VkShaderStageFlags stageFlags = VK_SHADER_STAGE_ALL, uint32_t descriptorCount = 1) noexcept
			{
				return this->declare(VkDescriptorSetLayoutBinding{
					binding,
					VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,
					descriptorCount,
					stageFlags,
					nullptr
				});
			}

			/**
			 * @brief Declares storage texel dynamic binding.
			 * @param binding The binding point.
			 * @param stageFlags Define at which stage of the shader the bond appears. Default all.
			 * @param descriptorCount Define the number of descriptor to bind. Default 1.
			 * @return bool
			 */
			bool
			declareStorageTexelDynamic (uint32_t binding, VkShaderStageFlags stageFlags = VK_SHADER_STAGE_ALL, uint32_t descriptorCount = 1) noexcept
			{
				return this->declare(VkDescriptorSetLayoutBinding{
					binding,
					VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC,
					descriptorCount,
					stageFlags,
					nullptr
				});
			}

			/**
			 * @brief Declares input attachment binding.
			 * @param binding The binding point.
			 * @param stageFlags Define at which stage of the shader the bond appears. Default all.
			 * @param descriptorCount Define the number of descriptor to bind. Default 1.
			 * @return bool
			 */
			bool
			declareInputAttachment (uint32_t binding, VkShaderStageFlags stageFlags = VK_SHADER_STAGE_ALL, uint32_t descriptorCount = 1) noexcept
			{
				return this->declare(VkDescriptorSetLayoutBinding{
					binding,
					VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT,
					descriptorCount,
					stageFlags,
					nullptr
				});
			}

			/**
			 * @brief Declares inline uniform block binding.
			 * @param binding The binding point.
			 * @param stageFlags Define at which stage of the shader the bond appears. Default all.
			 * @param descriptorCount Define the number of descriptor to bind. Default 1.
			 * @return bool
			 */
			bool
			declareInlineUniformBlockEXT (uint32_t binding, VkShaderStageFlags stageFlags = VK_SHADER_STAGE_ALL, uint32_t descriptorCount = 1) noexcept
			{
				return this->declare(VkDescriptorSetLayoutBinding{
					binding,
					VK_DESCRIPTOR_TYPE_INLINE_UNIFORM_BLOCK_EXT,
					descriptorCount,
					stageFlags,
					nullptr
				});
			}

			/**
			 * @brief Declares acceleration structure binding.
			 * @param binding The binding point.
			 * @param stageFlags Define at which stage of the shader the bond appears. Default all.
			 * @param descriptorCount Define the number of descriptor to bind. Default 1.
			 * @return bool
			 */
			bool
			declareAccelerationStructureKHR (uint32_t binding, VkShaderStageFlags stageFlags = VK_SHADER_STAGE_ALL, uint32_t descriptorCount = 1) noexcept
			{
				return this->declare(VkDescriptorSetLayoutBinding{
					binding,
					VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR,
					descriptorCount,
					stageFlags,
					nullptr
				});
			}

			/**
			 * @brief Declares acceleration structure binding.
			 * @param binding The binding point.
			 * @param stageFlags Define at which stage of the shader the bond appears. Default all.
			 * @param descriptorCount Define the number of descriptor to bind. Default 1.
			 * @return bool
			 */
			bool
			declareAccelerationStructureNV (uint32_t binding, VkShaderStageFlags stageFlags = VK_SHADER_STAGE_ALL, uint32_t descriptorCount = 1) noexcept
			{
				return this->declare(VkDescriptorSetLayoutBinding{
					binding,
					VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_NV,
					descriptorCount,
					stageFlags,
					nullptr
				});
			}

			/**
			 * @brief Returns the descriptor pool handle.
			 * @return VkDescriptorSetLayout
			 */
			[[nodiscard]]
			VkDescriptorSetLayout
			handle () const noexcept
			{
				return m_handle;
			}

			/**
			 * @brief Returns the descriptor pool create info.
			 * @return const VkDescriptorSetLayoutCreateInfo &
			 */
			[[nodiscard]]
			const VkDescriptorSetLayoutCreateInfo &
			createInfo () const noexcept
			{
				return m_createInfo;
			}

			/**
			 * @brief Returns the descriptor set layout hash.
			 * @return size_t
			 */
			[[nodiscard]]
			size_t
			hash () const noexcept
			{
				return DescriptorSetLayout::getHash(m_setLayoutBindings, m_createInfo.flags);
			}

			/**
			 * @brief STL streams printable object.
			 * @param out A reference to the stream output.
			 * @param obj A reference to the object to print.
			 * @return std::ostream &
			 */
			friend std::ostream & operator<< (std::ostream & out, const DescriptorSetLayout & obj);

			/**
			 * @brief Stringifies the object.
			 * @param obj A reference to the object to print.
			 * @return std::string
			 */
			friend std::string to_string (const DescriptorSetLayout & obj) noexcept;

			/**
			 * @brief Returns a hash for a descriptor layout according to constructor params.
			 * @return size_t
			 */
			[[nodiscard]]
			static size_t getHash (const std::vector< VkDescriptorSetLayoutBinding > & bindings, VkDescriptorSetLayoutCreateFlags flags) noexcept;

		private:

			VkDescriptorSetLayout m_handle{VK_NULL_HANDLE};
			VkDescriptorSetLayoutCreateInfo m_createInfo{};
			std::string m_UUID;
			std::vector< VkDescriptorSetLayoutBinding > m_setLayoutBindings;
	};
}
