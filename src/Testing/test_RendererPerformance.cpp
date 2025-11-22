/*
 * src/Testing/test_RendererPerformance.cpp
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

#include <gtest/gtest.h>
#include <chrono>
#include <map>
#include <unordered_map>
#include <string>
#include <memory>

/**
 * @brief Performance benchmark for std::map vs std::unordered_map in Renderer cache lookups.
 * 
 * This test validates the performance improvements implemented for the Renderer cache optimizations.
 * 
 * Context:
 * - Renderer used std::map for m_renderPasses, m_samplers, m_pipelines (O(log n) lookup)
 * - Optimized to use std::unordered_map (O(1) average lookup)
 * 
 * Expected improvement: 10-20% reduction in cache lookup time for Graphics hot paths.
 */

namespace EmEn::Testing
{
	constexpr size_t BENCHMARK_ITERATIONS = 100000;
	constexpr size_t CACHE_SIZE = 100;

	/**
	 * @brief Simulates a typical Renderer cache structure.
	 */
	template<template<typename...> typename MapType>
	class RendererCacheSimulator
	{
		public:
			using CacheMap = MapType<std::string, std::shared_ptr<int>>;
			
			RendererCacheSimulator()
			{
				// Populate cache with typical resource names
				for (size_t i = 0; i < CACHE_SIZE; ++i)
				{
					std::string key;
					
					if (i % 3 == 0)
						key = "RenderPass_" + std::to_string(i);
					else if (i % 3 == 1)
						key = "Sampler_Texture2D_" + std::to_string(i);
					else
						key = "Pipeline_PBR_" + std::to_string(i);
						
					m_cache.emplace(key, std::make_shared<int>(static_cast<int>(i)));
				}
			}

			/**
			 * @brief Simulates typical Renderer::getSampler() lookup pattern.
			 */
			std::shared_ptr<int> lookup(const std::string& key)
			{
				auto it = m_cache.find(key);
				if (it != m_cache.end())
				{
					return it->second;
				}
				return nullptr;
			}

			size_t size() const { return m_cache.size(); }

		private:
			CacheMap m_cache;
	};

	/**
	 * @brief Benchmark std::map lookup performance (original implementation).
	 */
	TEST(RendererPerformance, MapLookupBenchmark)
	{
		RendererCacheSimulator<std::map> cache;
		
		// Test keys that exist in cache
		// Keys generated with: i%3==0 → RenderPass, i%3==1 → Sampler, i%3==2 → Pipeline
		std::vector<std::string> testKeys = {
			"RenderPass_42",         // 42 % 3 = 0 ✓
			"Sampler_Texture2D_43",  // 43 % 3 = 1 ✓
			"Pipeline_PBR_44",       // 44 % 3 = 2 ✓
			"RenderPass_9",          //  9 % 3 = 0 ✓
			"Sampler_Texture2D_91"   // 91 % 3 = 1 ✓
		};

		auto start = std::chrono::high_resolution_clock::now();

		// Simulate typical Renderer cache access patterns
		size_t hits = 0;
		for (size_t i = 0; i < BENCHMARK_ITERATIONS; ++i)
		{
			const auto& key = testKeys[i % testKeys.size()];
			if (cache.lookup(key) != nullptr)
			{
				hits++;
			}
		}

		auto end = std::chrono::high_resolution_clock::now();
		auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

		// All lookups should hit (cache is populated with these keys)
		EXPECT_EQ(hits, BENCHMARK_ITERATIONS);

		// Log performance for comparison
		std::cout << "[MAP] " << BENCHMARK_ITERATIONS << " lookups in " 
		          << duration.count() << " μs (" 
		          << static_cast<double>(duration.count()) / BENCHMARK_ITERATIONS 
		          << " μs per lookup)" << std::endl;
	}

	/**
	 * @brief Benchmark std::unordered_map lookup performance (optimized implementation).
	 */
	TEST(RendererPerformance, UnorderedMapLookupBenchmark)
	{
		RendererCacheSimulator<std::unordered_map> cache;
		
		// Same test keys as map benchmark for fair comparison
		// Keys generated with: i%3==0 → RenderPass, i%3==1 → Sampler, i%3==2 → Pipeline
		std::vector<std::string> testKeys = {
			"RenderPass_42",         // 42 % 3 = 0 ✓
			"Sampler_Texture2D_43",  // 43 % 3 = 1 ✓
			"Pipeline_PBR_44",       // 44 % 3 = 2 ✓
			"RenderPass_9",          //  9 % 3 = 0 ✓
			"Sampler_Texture2D_91"   // 91 % 3 = 1 ✓
		};

		auto start = std::chrono::high_resolution_clock::now();

		// Simulate typical Renderer cache access patterns
		size_t hits = 0;
		for (size_t i = 0; i < BENCHMARK_ITERATIONS; ++i)
		{
			const auto& key = testKeys[i % testKeys.size()];
			if (cache.lookup(key) != nullptr)
			{
				hits++;
			}
		}

		auto end = std::chrono::high_resolution_clock::now();
		auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

		// All lookups should hit (cache is populated with these keys)
		EXPECT_EQ(hits, BENCHMARK_ITERATIONS);

		// Log performance for comparison
		std::cout << "[UNORDERED_MAP] " << BENCHMARK_ITERATIONS << " lookups in " 
		          << duration.count() << " μs (" 
		          << static_cast<double>(duration.count()) / BENCHMARK_ITERATIONS 
		          << " μs per lookup)" << std::endl;
	}

	/**
	 * @brief Test cache miss performance (realistic scenario).
	 */
	TEST(RendererPerformance, CacheMissComparison)
	{
		RendererCacheSimulator<std::map> mapCache;
		RendererCacheSimulator<std::unordered_map> unorderedCache;
		
		// Keys that DON'T exist in cache (cache misses)
		std::vector<std::string> missKeys = {
			"NonExistent_RenderPass",
			"Missing_Sampler",
			"Invalid_Pipeline",
			"Unknown_Resource",
			"Phantom_Cache_Key"
		};

		// Benchmark map cache misses
		auto start = std::chrono::high_resolution_clock::now();
		size_t mapMisses = 0;
		for (size_t i = 0; i < BENCHMARK_ITERATIONS / 10; ++i)  // Fewer iterations for misses
		{
			const auto& key = missKeys[i % missKeys.size()];
			if (mapCache.lookup(key) == nullptr)
			{
				mapMisses++;
			}
		}
		auto mapTime = std::chrono::duration_cast<std::chrono::microseconds>(
			std::chrono::high_resolution_clock::now() - start);

		// Benchmark unordered_map cache misses
		start = std::chrono::high_resolution_clock::now();
		size_t unorderedMisses = 0;
		for (size_t i = 0; i < BENCHMARK_ITERATIONS / 10; ++i)
		{
			const auto& key = missKeys[i % missKeys.size()];
			if (unorderedCache.lookup(key) == nullptr)
			{
				unorderedMisses++;
			}
		}
		auto unorderedTime = std::chrono::duration_cast<std::chrono::microseconds>(
			std::chrono::high_resolution_clock::now() - start);

		// All lookups should miss
		EXPECT_EQ(mapMisses, BENCHMARK_ITERATIONS / 10);
		EXPECT_EQ(unorderedMisses, BENCHMARK_ITERATIONS / 10);

		std::cout << "[CACHE MISS] Map: " << mapTime.count()
		          << " μs, UnorderedMap: " << unorderedTime.count() << " μs" << std::endl;

		// unordered_map should be faster or equal (never significantly slower)
		EXPECT_LE(unorderedTime.count(), mapTime.count() * 1.2);  // Allow 20% tolerance for test variance
	}

	/**
	 * @brief Comprehensive performance comparison test.
	 * 
	 * This test should demonstrate the overall performance improvement
	 * achieved by switching from std::map to std::unordered_map.
	 */
	TEST(RendererPerformance, OverallPerformanceImprovement)
	{
		// Larger cache to better simulate real Renderer usage
		constexpr size_t LARGE_CACHE_SIZE = 500;
		constexpr size_t LARGE_ITERATIONS = 50000;

		// Create large caches
		std::map<std::string, std::shared_ptr<int>> mapCache;
		std::unordered_map<std::string, std::shared_ptr<int>> unorderedCache;

		// Populate with realistic Renderer resource patterns
		for (size_t i = 0; i < LARGE_CACHE_SIZE; ++i)
		{
			auto value = std::make_shared<int>(static_cast<int>(i));
			std::string key = "Resource_" + std::to_string(i) + "_Frame_" + std::to_string(i % 60);
			
			mapCache.emplace(key, value);
			unorderedCache.emplace(key, value);
		}

		// Create realistic lookup pattern (80% hits, 20% misses)
		std::vector<std::string> lookupKeys;
		for (size_t i = 0; i < LARGE_ITERATIONS; ++i)
		{
			if (i % 5 == 0)  // 20% cache misses
			{
				lookupKeys.push_back("Missing_Resource_" + std::to_string(i));
			}
			else  // 80% cache hits
			{
				size_t idx = i % LARGE_CACHE_SIZE;
				lookupKeys.push_back("Resource_" + std::to_string(idx) + "_Frame_" + std::to_string(idx % 60));
			}
		}

		// Benchmark std::map
		auto mapStart = std::chrono::high_resolution_clock::now();
		for (const auto& key : lookupKeys)
		{
			auto it = mapCache.find(key);
			(void)it;  // Prevent optimization
		}
		auto mapDuration = std::chrono::duration_cast<std::chrono::microseconds>(
			std::chrono::high_resolution_clock::now() - mapStart);

		// Benchmark std::unordered_map
		auto unorderedStart = std::chrono::high_resolution_clock::now();
		for (const auto& key : lookupKeys)
		{
			auto it = unorderedCache.find(key);
			(void)it;  // Prevent optimization
		}
		auto unorderedDuration = std::chrono::duration_cast<std::chrono::microseconds>(
			std::chrono::high_resolution_clock::now() - unorderedStart);

		std::cout << "[OVERALL] Map: " << mapDuration.count() 
		          << " μs, UnorderedMap: " << unorderedDuration.count() 
		          << " μs (Improvement: " 
		          << (100.0 * (mapDuration.count() - unorderedDuration.count()) / mapDuration.count())
		          << "%)" << std::endl;

		// Expect at least some improvement (unordered_map should be faster or equal)
		EXPECT_LE(unorderedDuration.count(), mapDuration.count());
		
		// With sufficient iterations, we should see measurable improvement
		if (mapDuration.count() > 1000)  // Only check if we have sufficient precision
		{
			double improvement = 100.0 * (mapDuration.count() - unorderedDuration.count()) / mapDuration.count();
			EXPECT_GE(improvement, 0.0);  // At minimum, no performance regression
		}
	}
}