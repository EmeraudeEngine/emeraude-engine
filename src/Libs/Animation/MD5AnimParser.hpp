/*
 * src/Libs/Animation/MD5AnimParser.hpp
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
 * https://github.com/londnoir/emeraude-engine
 *
 * --- THIS IS AUTOMATICALLY GENERATED, DO NOT CHANGE ---
 */

#pragma once

/* STL inclusions. */
#include <array>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

/* Local inclusions. */
#include "AnimationChannel.hpp"
#include "AnimationClip.hpp"
#include "Libs/Math/Matrix.hpp"
#include "Libs/Math/Quaternion.hpp"
#include "Libs/Math/Vector.hpp"

namespace EmEn::Libs::Animation
{
	/**
	 * @brief Parser for id Tech MD5 animation files (.md5anim).
	 * Produces an AnimationClip compatible with the engine's skeletal animation pipeline.
	 * @tparam precision_t Floating point type. Default float.
	 */
	template< typename precision_t = float >
	requires (std::is_floating_point_v< precision_t >)
	class MD5AnimParser final
	{
		public:

			/**
			 * @brief Parses a .md5anim file and returns an AnimationClip.
			 * @param filepath Path to the .md5anim file.
			 * @param clipName Name for the resulting clip (e.g., "idle", "walk").
			 * @return AnimationClip< precision_t > The parsed clip (empty if parsing fails).
			 */
			[[nodiscard]]
			static
			AnimationClip< precision_t >
			parse (const std::filesystem::path & filepath, const std::string & clipName) noexcept
			{
				std::ifstream file(filepath);

				if ( !file.is_open() )
				{
					std::cerr << "[MD5AnimParser] Failed to open '" << filepath << "' !\n";

					return {};
				}

				return parseStream(file, clipName);
			}

			/**
			 * @brief Parses a .md5anim stream and returns an AnimationClip.
			 * @param stream Input stream with .md5anim content.
			 * @param clipName Name for the resulting clip.
			 * @return AnimationClip< precision_t > The parsed clip (empty if parsing fails).
			 */
			[[nodiscard]]
			static
			AnimationClip< precision_t >
			parseStream (std::istream & stream, const std::string & clipName) noexcept
			{
				int numFrames = 0;
				int numJoints = 0;
				int frameRate = 24;
				int numAnimatedComponents = 0;

				struct HierarchyEntry
				{
					std::string name;
					int parent{-1};
					int flags{0};
					int firstComponent{0};
				};

				struct BaseFrameEntry
				{
					std::array< float, 3 > pos{};
					std::array< float, 3 > orient{};
				};

				std::vector< HierarchyEntry > hierarchy;
				std::vector< BaseFrameEntry > baseframe;
				std::vector< std::vector< float > > frames;

				std::string line;

				/* ---- Phase 1: Parse the file ---- */
				while ( std::getline(stream, line) )
				{
					std::istringstream ss(line);
					std::string token;
					ss >> token;

					if ( token == "numFrames" )
					{
						ss >> numFrames;
					}
					else if ( token == "numJoints" )
					{
						ss >> numJoints;
					}
					else if ( token == "frameRate" )
					{
						ss >> frameRate;
					}
					else if ( token == "numAnimatedComponents" )
					{
						ss >> numAnimatedComponents;
					}
					else if ( token == "hierarchy" )
					{
						hierarchy.resize(numJoints);

						for ( int i = 0; i < numJoints; ++i )
						{
							std::getline(stream, line);
							std::istringstream hs(line);

							std::string name;
							hs >> name;

							/* Remove quotes from joint name. */
							if ( name.front() == '"' && name.back() == '"' )
							{
								name = name.substr(1, name.size() - 2);
							}

							hs >> hierarchy[i].parent >> hierarchy[i].flags >> hierarchy[i].firstComponent;
							hierarchy[i].name = std::move(name);
						}

						/* Skip closing brace. */
						std::getline(stream, line);
					}
					else if ( token == "bounds" )
					{
						/* Skip bounds block — not needed for animation. */
						for ( int i = 0; i < numFrames; ++i )
						{
							std::getline(stream, line);
						}

						std::getline(stream, line);
					}
					else if ( token == "baseframe" )
					{
						baseframe.resize(numJoints);

						for ( int i = 0; i < numJoints; ++i )
						{
							std::getline(stream, line);
							std::istringstream bs(line);
							char trash;

							bs >> trash >> baseframe[i].pos[0] >> baseframe[i].pos[1] >> baseframe[i].pos[2]
							   >> trash >> trash >> baseframe[i].orient[0] >> baseframe[i].orient[1] >> baseframe[i].orient[2];
						}

						std::getline(stream, line);
					}
					else if ( token == "frame" )
					{
						std::vector< float > frameData;
						frameData.reserve(numAnimatedComponents);

						while ( std::getline(stream, line) )
						{
							/* Trim. */
							auto start = line.find_first_not_of(" \t");

							if ( start == std::string::npos )
							{
								continue;
							}

							if ( line[start] == '}' )
							{
								break;
							}

							std::istringstream fs(line);
							float value;

							while ( fs >> value )
							{
								frameData.push_back(value);
							}
						}

						frames.push_back(std::move(frameData));
					}
				}

				if ( hierarchy.empty() || frames.empty() || frameRate <= 0 )
				{
					std::cerr << "[MD5AnimParser] Invalid .md5anim data !\n";

					return {};
				}

				/* ---- Phase 2: Build per-joint keyframes ---- */

				const auto jointCount = static_cast< size_t >(numJoints);
				const auto frameCount = static_cast< size_t >(numFrames);
				const auto frameDuration = static_cast< precision_t >(1) / static_cast< precision_t >(frameRate);

				/* Per-joint, per-frame transforms (in engine space). */
				struct JointPose
				{
					Math::Vector< 3, precision_t > translation;
					Math::Quaternion< precision_t > rotation;
				};

				/* For each joint, collect translation and rotation keyframes. */
				std::vector< std::vector< VectorKeyFrame< precision_t > > > translationKeys(jointCount);
				std::vector< std::vector< QuaternionKeyFrame< precision_t > > > rotationKeys(jointCount);

				for ( size_t j = 0; j < jointCount; ++j )
				{
					translationKeys[j].reserve(frameCount);
					rotationKeys[j].reserve(frameCount);
				}

				for ( size_t f = 0; f < frameCount; ++f )
				{
					const auto time = static_cast< precision_t >(f) * frameDuration;
					const auto & frameData = frames[f];

					for ( size_t j = 0; j < jointCount; ++j )
					{
						const auto & h = hierarchy[j];
						const auto & bf = baseframe[j];

						/* Start from baseframe. */
						float px = bf.pos[0];
						float py = bf.pos[1];
						float pz = bf.pos[2];
						float qx = bf.orient[0];
						float qy = bf.orient[1];
						float qz = bf.orient[2];

						/* Override with animated components based on flags. */
						int componentIndex = h.firstComponent;

						if ( h.flags & 1  ) { px = frameData[componentIndex++]; }
						if ( h.flags & 2  ) { py = frameData[componentIndex++]; }
						if ( h.flags & 4  ) { pz = frameData[componentIndex++]; }
						if ( h.flags & 8  ) { qx = frameData[componentIndex++]; }
						if ( h.flags & 16 ) { qy = frameData[componentIndex++]; }
						if ( h.flags & 32 ) { qz = frameData[componentIndex++]; }

						/* Compute quaternion W component. */
						const float qwSquared = 1.0F - (qx * qx + qy * qy + qz * qz);
						const float qw = (qwSquared > 0.0F) ? -std::sqrt(qwSquared) : 0.0F;

						/* Convert from MD5 space to engine space. */
						const auto pos = md5ToEnginePosition({px, py, pz});
						const auto rot = md5ToEngineRotation({qx, qy, qz, qw});

						translationKeys[j].push_back({time, pos});
						rotationKeys[j].push_back({time, rot});
					}
				}

				/* ---- Phase 3: Build AnimationClip channels ---- */

				std::vector< AnimationChannel< precision_t > > channels;
				channels.reserve(jointCount * 2);

				for ( size_t j = 0; j < jointCount; ++j )
				{
					/* Translation channel. */
					AnimationChannel< precision_t > translationChannel;
					translationChannel.jointIndex = static_cast< int32_t >(j);
					translationChannel.target = ChannelTarget::Translation;
					translationChannel.interpolation = ChannelInterpolation::Linear;
					translationChannel.vectorKeyFrames = std::move(translationKeys[j]);
					channels.push_back(std::move(translationChannel));

					/* Rotation channel. */
					AnimationChannel< precision_t > rotationChannel;
					rotationChannel.jointIndex = static_cast< int32_t >(j);
					rotationChannel.target = ChannelTarget::Rotation;
					rotationChannel.interpolation = ChannelInterpolation::Linear;
					rotationChannel.quaternionKeyFrames = std::move(rotationKeys[j]);
					channels.push_back(std::move(rotationChannel));
				}

				return AnimationClip< precision_t >{clipName, std::move(channels)};
			}

		private:

			/** @brief Scale factor to convert idTech unit system to engine units. */
			static constexpr auto IDTechUnitScale = static_cast< precision_t >(0.01);

			/**
			 * @brief Converts an MD5 position from ID Tech coordinate space to engine space.
			 * @note MD5 uses right-handed Y-up. Engine uses Y-down.
			 * Combined transform: (md5.y, -md5.z, md5.x) * IDTechUnitScale.
			 */
			static
			Math::Vector< 3, precision_t >
			md5ToEnginePosition (const std::array< float, 3 > & md5Pos) noexcept
			{
				return {
					static_cast< precision_t >(md5Pos[1]) * IDTechUnitScale,
					-static_cast< precision_t >(md5Pos[2]) * IDTechUnitScale,
					static_cast< precision_t >(md5Pos[0]) * IDTechUnitScale
				};
			}

			/**
			 * @brief Converts an MD5 quaternion from ID Tech coordinate space to engine space.
			 * @note R_engine = M * R_md5 * M^T where M maps (x,y,z)_md5 → (y,-z,x)_engine.
			 */
			static
			Math::Quaternion< precision_t >
			md5ToEngineRotation (const std::array< float, 4 > & md5Orient) noexcept
			{
				const Math::Quaternion< precision_t > qMD5{
					static_cast< precision_t >(md5Orient[0]),
					static_cast< precision_t >(md5Orient[1]),
					static_cast< precision_t >(md5Orient[2]),
					static_cast< precision_t >(md5Orient[3])
				};

				const auto rotMD5 = qMD5.toRotationMatrix4();

				constexpr auto Zero = static_cast< precision_t >(0);
				constexpr auto One = static_cast< precision_t >(1);
				constexpr auto NegOne = static_cast< precision_t >(-1);

				const Math::Matrix< 4, precision_t > M{
					Zero, One,    Zero, Zero,
					Zero, Zero, NegOne, Zero,
					One,  Zero,   Zero, Zero,
					Zero, Zero,   Zero, One
				};

				const Math::Matrix< 4, precision_t > MT{
					Zero, Zero, One,  Zero,
					One,  Zero, Zero, Zero,
					Zero, NegOne, Zero, Zero,
					Zero, Zero, Zero, One
				};

				const auto rotEngine = M * rotMD5 * MT;

				return Math::Quaternion< precision_t >{rotEngine};
			}
	};

	using MD5AnimParserF = MD5AnimParser< float >;
	using MD5AnimParserD = MD5AnimParser< double >;
}
