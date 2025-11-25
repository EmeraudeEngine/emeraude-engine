/*
 * src/Vulkan/LayoutManager.cpp
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

#include "LayoutManager.hpp"

/* Local inclusions. */
#include "Device.hpp"
#include "PipelineLayout.hpp"
#include "DescriptorSetLayout.hpp"
#include "Tracer.hpp"

namespace EmEn::Vulkan
{
	using namespace Libs;

	bool
	LayoutManager::onInitialize () noexcept
	{
		if ( m_device == nullptr || !m_device->isCreated() )
		{
			Tracer::error(ClassId, "No device set !");

			return false;
		}

		return true;
	}

	bool
	LayoutManager::onTerminate () noexcept
	{
		m_descriptorSetLayouts.clear();
		m_pipelineLayouts.clear();

		m_device.reset();

		return true;
	}

	std::shared_ptr< DescriptorSetLayout >
	LayoutManager::getDescriptorSetLayout (const std::string & UUID) const noexcept
	{
		const auto descriptorSetLayoutIt = m_descriptorSetLayouts.find(UUID);

		if ( descriptorSetLayoutIt == m_descriptorSetLayouts.cend() )
		{
			return nullptr;
		}

		return descriptorSetLayoutIt->second;
	}

	std::shared_ptr< DescriptorSetLayout >
	LayoutManager::prepareNewDescriptorSetLayout (const std::string & UUID, VkDescriptorSetLayoutCreateFlags createFlags) const noexcept
	{
		return std::make_shared< DescriptorSetLayout >(m_device, UUID, createFlags);
	}

	bool
	LayoutManager::createDescriptorSetLayout (const std::shared_ptr< DescriptorSetLayout > & descriptorSetLayout) noexcept
	{
		/* NOTE: Descriptor set layout identifier must be unique. */
		if ( m_descriptorSetLayouts.contains(descriptorSetLayout->UUID()) )
		{
			TraceError{ClassId} << "The manager already holds a descriptor set layout named '" << descriptorSetLayout->UUID() << "' !";

			return false;
		}

		/* NOTE: Do not save incomplete descriptor set layout. */
		if ( !descriptorSetLayout->createOnHardware() )
		{
			TraceError{ClassId} << "The descriptor set layout '" << descriptorSetLayout->UUID() << "' is not created !";

			return false;
		}

		return m_descriptorSetLayouts.emplace(descriptorSetLayout->UUID(), descriptorSetLayout).second;
	}

	std::shared_ptr< PipelineLayout >
	LayoutManager::getPipelineLayout (const StaticVector< std::shared_ptr< DescriptorSetLayout >, 4 > & descriptorSetLayouts, const StaticVector< VkPushConstantRange, 4 > & pushConstantRanges, VkPipelineLayoutCreateFlags createFlags) noexcept
	{
		/* FIXME: Find a better way to create an UUID. */
		std::stringstream pipelineLayoutUUIDStream;

		for ( const auto & descriptorSetLayout : descriptorSetLayouts )
		{
			pipelineLayoutUUIDStream << descriptorSetLayout->UUID();
		}

		for ( const auto & pushConstantRange : pushConstantRanges )
		{
			pipelineLayoutUUIDStream << "PC" << pushConstantRange.stageFlags << ':' << pushConstantRange.offset << ':' << pushConstantRange.size;
		}

		const auto pipelineLayoutUUID = pipelineLayoutUUIDStream.str();

		const auto pipelineLayoutIt = m_pipelineLayouts.find(pipelineLayoutUUID);

		if ( pipelineLayoutIt != m_pipelineLayouts.cend() )
		{
			//TraceDebug{ClassId} << "Reusing pipeline layout named '" << pipelineLayoutUUID << "' ...";

			return pipelineLayoutIt->second;
		}

		auto pipelineLayout = std::make_shared< PipelineLayout >(m_device, pipelineLayoutUUID, descriptorSetLayouts, pushConstantRanges, createFlags);
		pipelineLayout->setIdentifier(ClassId, pipelineLayout->UUID(), "PipelineLayout");

		if ( !pipelineLayout->createOnHardware() )
		{
			TraceError{ClassId} << "The pipeline layout '" << pipelineLayout->UUID() << "' is not created !";

			return nullptr;
		}

		return m_pipelineLayouts.emplace(pipelineLayout->UUID(), pipelineLayout).first->second;
	}
}
