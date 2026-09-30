/*
 * src/Scenes/Component/BeamConsoleAdapter.cpp
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

#include "ConsoleAdapter.hpp"

/* STL inclusions. */
#include <cmath>
#include <span>
#include <string>
#include <vector>

/* Third-party inclusions. */
#include "json/json.h"

/* Local inclusions. */
#include "Beam.hpp"
#include "FastJSON.hpp"
#include "Scenes/AbstractEntity.hpp"
#include "Scenes/Manager.hpp"
#include "Scenes/Scene.hpp"

namespace EmEn::Scenes::Component
{
	using namespace Base;
	using namespace Base::Math;

	namespace
	{
		/**
		 * @brief Returns a vector as a JSON array.
		 * @param vector The vector.
		 * @return Json::Value
		 */
		[[nodiscard]]
		Json::Value
		toJSON (const Vector< 3, float > & vector) noexcept
		{
			Json::Value array{Json::arrayValue};
			array.append(static_cast< double >(vector[X]));
			array.append(static_cast< double >(vector[Y]));
			array.append(static_cast< double >(vector[Z]));

			return array;
		}

		/**
		 * @brief Returns the full state of a beam as JSON: its curve, endpoints, target, look.
		 * @param beam The beam.
		 * @return std::string
		 */
		[[nodiscard]]
		std::string
		stateOf (const Beam & beam) noexcept
		{
			Json::Value state{Json::objectValue};
			state["name"] = beam.name();
			state["entity"] = beam.parentEntity().name();
			state["type"] = Beam::ClassId;
			state["enabled"] = beam.isEnabled();
			state["segmentCount"] = beam.segmentCount();
			state["stationCount"] = beam.stationCount();
			state["kind"] = Base::Math::to_cstring(beam.curve().kind());
			state["closed"] = beam.curve().isClosed();
			state["sourcePointCount"] = static_cast< Json::UInt64 >(beam.curve().points().size());
			state["tolerance"] = static_cast< double >(beam.tolerance());
			state["length"] = static_cast< double >(beam.length());
			state["start"] = toJSON(beam.start());
			state["end"] = toJSON(beam.end());

			if ( const auto target = beam.endTarget(); target != nullptr )
			{
				state["endTarget"] = target->name();
				state["endTargetOffset"] = toJSON(beam.endTargetOffset());
			}
			else
			{
				state["endTarget"] = Json::nullValue;
			}

			if ( const auto material = beam.material(); material != nullptr )
			{
				Json::Value color{Json::arrayValue};
				color.append(static_cast< double >(material->color().red()));
				color.append(static_cast< double >(material->color().green()));
				color.append(static_cast< double >(material->color().blue()));

				state["color"] = color;
				state["luminance"] = static_cast< double >(material->luminance());
				state["halfWidth"] = static_cast< double >(material->halfWidth());
				state["coreExponent"] = static_cast< double >(material->coreExponent());
				state["arcAmplitude"] = static_cast< double >(material->arcAmplitude());
				state["arcFrequency"] = static_cast< double >(material->arcFrequency());
				state["arcOctaves"] = material->arcOctaves();
				state["arcSeed"] = material->arcSeed();
				state["arcRestrikeRate"] = static_cast< double >(material->arcRestrikeRate());
				state["arcDrift"] = static_cast< double >(material->arcDrift());
			}

			return FastJSON::stringify(state);
		}

		/**
		 * @brief The answer of a setter: the confirmation, then the beam's NEW state as JSON.
		 * @param beam The beam, after the change.
		 * @param message The confirmation.
		 * @return Console::CommandResult
		 */
		[[nodiscard]]
		Console::CommandResult
		changed (const Beam & beam, std::string message) noexcept
		{
			return changedState(std::move(message), stateOf(beam));
		}

		/**
		 * @brief Returns whether three coordinates are finite.
		 * @return bool
		 */
		[[nodiscard]]
		bool
		finite (float x, float y, float z) noexcept
		{
			return std::isfinite(x) && std::isfinite(y) && std::isfinite(z);
		}

		/**
		 * @brief The console adapter of the Beam component: its endpoints, the entity its end follows, and its look.
		 */
		class BeamConsoleAdapter final : public ConsoleAdapter< Beam >
		{
			public:

				explicit
				BeamConsoleAdapter (const Manager & sceneManager) noexcept
					: ConsoleAdapter< Beam >{sceneManager}
				{

				}

			private:

				/**
				 * @brief Runs an action on the material of a beam, refusing a beam that has none.
				 * @param entityName The entity address.
				 * @param componentName The component name.
				 * @param action A callable taking `Beam &` and `Graphics::Material::BeamResource &`.
				 * @return Console::CommandResult
				 */
				template< typename action_t >
				[[nodiscard]]
				Console::CommandResult
				withMaterial (const std::string & entityName, const std::string & componentName, action_t && action) const noexcept
				{
					return this->act(entityName, componentName, [&action] (Beam & beam) {
						const auto material = beam.material();

						if ( material == nullptr )
						{
							return Console::CommandResult::error("Beam '" + beam.name() + "' has no material.");
						}

						return std::forward< action_t >(action)(beam, *material);
					});
				}

				void
				onRegisterToConsole () noexcept override
				{
					const Console::Parameter entity{entityParameter("the beam")};
					const Console::Parameter component{componentParameter(Beam::ClassId)};

					this->bindCommand("getState", "Returns the state of a Beam as JSON: its curve (kind, closed, stations, length), its endpoints (entity space, metres), the entity its end follows, and its look (colour, luminance in nits, half width, arc).",
						{entity, component},
						[this] (const std::string & entityName, const std::string & componentName) {
							return this->act(entityName, componentName, [] (Beam & beam) {
								return Console::CommandResult::json(stateOf(beam));
							});
						}, Console::CommandHint::ReadOnly);

					this->bindCommand("setPoints", "Makes a Beam follow a curve through points in its entity's space: \"x y z; x y z; ...\" — its first and last points are its start and end. Kinds: Polyline (a laser through relays), UniformBSpline (smooth, not through them), CatmullRom (through every point).",
						{
							entity,
							component,
							{"kind", "Polyline, UniformBSpline or CatmullRom (a Bezier path is set from code: Math::BSpline)."},
							{"points", "The points, \"x y z; x y z; ...\" (metres, UP is +Y), 2 at least."},
							{"closed", "true to close the curve on itself.", false}
						},
						[this] (const std::string & entityName, const std::string & componentName, const std::string & kind, const std::string & text, bool closed) {
							const auto points = parsePointList(text);

							if ( !points.has_value() || points->size() < 2 )
							{
								return Console::CommandResult::error("Malformed points: \"x y z; x y z; ...\", finite numbers, 2 points at least.");
							}

							if ( kind != "Polyline" && kind != "UniformBSpline" && kind != "CatmullRom" )
							{
								return Console::CommandResult::error("Unknown kind '" + kind + "' (Polyline, UniformBSpline or CatmullRom).");
							}

							return this->act(entityName, componentName, [&kind, &points, closed] (Beam & beam) {
								const std::span< const Vector< 3, float > > source{*points};

								if ( kind == "Polyline" )
								{
									beam.setPolyline(source, closed);
								}
								else if ( kind == "UniformBSpline" )
								{
									beam.setUniformBSpline(source, closed);
								}
								else
								{
									beam.setCatmullRom(source, closed);
								}

								return changed(beam, "Beam '" + beam.name() + "' curve set.");
							});
						}, Console::CommandHint::Idempotent);

					this->bindCommand("setTolerance", "Sets the chord tolerance of a Beam's curve tessellation: the largest distance between the curve and the drawn stations' polyline, in entity units.",
						{entity, component, {"tolerance", "The tolerance, > 0 (0.01 = 1 cm)."}},
						[this] (const std::string & entityName, const std::string & componentName, float tolerance) {
							if ( !std::isfinite(tolerance) || tolerance <= 0.0F )
							{
								return Console::CommandResult::error("The tolerance must be a finite number above 0.");
							}

							return this->act(entityName, componentName, [tolerance] (Beam & beam) {
								beam.setTolerance(tolerance);

								return changed(beam, "Beam '" + beam.name() + "' tolerance set.");
							});
						}, Console::CommandHint::Idempotent);

					this->bindCommand("setStart", "Sets the start of a Beam — the first point of its curve —, in its entity's space.",
						{entity, component, {"x", "X, in metres."}, {"y", "Y, in metres (UP is +Y)."}, {"z", "Z, in metres."}},
						[this] (const std::string & entityName, const std::string & componentName, float x, float y, float z) {
							if ( !finite(x, y, z) )
							{
								return Console::CommandResult::error("The start must be finite.");
							}

							return this->act(entityName, componentName, [x, y, z] (Beam & beam) {
								beam.setStart({x, y, z});

								return changed(beam, "Beam '" + beam.name() + "' start set.");
							});
						}, Console::CommandHint::Idempotent);

					this->bindCommand("setEnd", "Sets the end of a Beam — the last point of its curve —, in its entity's space. Forgets the entity it followed.",
						{entity, component, {"x", "X, in metres."}, {"y", "Y, in metres (UP is +Y)."}, {"z", "Z, in metres."}},
						[this] (const std::string & entityName, const std::string & componentName, float x, float y, float z) {
							if ( !finite(x, y, z) )
							{
								return Console::CommandResult::error("The end must be finite.");
							}

							return this->act(entityName, componentName, [x, y, z] (Beam & beam) {
								beam.setEnd({x, y, z});

								return changed(beam, "Beam '" + beam.name() + "' end set.");
							});
						}, Console::CommandHint::Idempotent);

					this->bindCommand("setEndTarget", "Makes the end of a Beam follow another entity of the active scene, every logic tick.",
						{
							entity,
							component,
							{"target", "The followed entity's address (listEntities() gives every address)."},
							{"x", "Offset from the target's origin, X, in the TARGET's space (metres)."},
							{"y", "Offset Y, in metres."},
							{"z", "Offset Z, in metres."}
						},
						[this] (const std::string & entityName, const std::string & componentName, const std::string & targetAddress, float x, float y, float z) {
							if ( !finite(x, y, z) )
							{
								return Console::CommandResult::error("The offset must be finite.");
							}

							/* The beam and its target resolved under ONE exclusive access to the scene. */
							auto result = Console::CommandResult::error("No active scene !");

							this->sceneManager().withExclusiveActiveScene([&] (const std::shared_ptr< Scene > & scene) {
								std::shared_ptr< Abstract > found;
								std::shared_ptr< AbstractEntity > target;
								std::string error;

								if ( !resolveComponent(*scene, entityName, componentName, found, error) || !resolveEntity(*scene, targetAddress, target, error) )
								{
									result = Console::CommandResult::error(error);

									return;
								}

								auto * beam = dynamic_cast< Beam * >(found.get());

								if ( beam == nullptr )
								{
									result = Console::CommandResult::error("The component '" + componentName + "' of '" + entityName + "' is a " + found->getComponentType() + ", not a " + Beam::ClassId + ".");

									return;
								}

								beam->setEndTarget(target, {x, y, z});

								result = changed(*beam, "Beam '" + beam->name() + "' now follows '" + targetAddress + "'.");
							}, true);

							return result;
						}, Console::CommandHint::Idempotent);

					this->bindCommand("setEnabled", "Shows or hides a Beam.",
						{entity, component, {"state", "true to show it, false to hide it."}},
						[this] (const std::string & entityName, const std::string & componentName, bool state) {
							return this->act(entityName, componentName, [state] (Beam & beam) {
								beam.setEnabled(state);

								return changed(beam, std::string{"Beam '"} + beam.name() + (state ? "' shown." : "' hidden."));
							});
						}, Console::CommandHint::Idempotent);

					this->bindCommand("setColor", "Sets the colour of a Beam (LINEAR, a hue: its brightness is the luminance).",
						{entity, component, {"red", "Red, 0-1."}, {"green", "Green, 0-1."}, {"blue", "Blue, 0-1."}},
						[this] (const std::string & entityName, const std::string & componentName, float red, float green, float blue) {
							if ( !finite(red, green, blue) || red < 0.0F || green < 0.0F || blue < 0.0F )
							{
								return Console::CommandResult::error("The colour channels must be finite, 0 or more.");
							}

							return this->withMaterial(entityName, componentName, [red, green, blue] (Beam & beam, Graphics::Material::BeamResource & material) {
								material.setColor(PixelFactory::Color< float >{red, green, blue, 1.0F});

								return changed(beam, "Beam '" + beam.name() + "' colour set.");
							});
						}, Console::CommandHint::Idempotent);

					this->bindCommand("setLuminance", "Sets the luminance of a Beam's core, in nits (cd/m²): a monitor 200-300, a fluorescent tube ~10 000, a welding arc 10^6.",
						{entity, component, {"nits", "The luminance, 0 or more."}},
						[this] (const std::string & entityName, const std::string & componentName, float nits) {
							if ( !std::isfinite(nits) || nits < 0.0F )
							{
								return Console::CommandResult::error("The luminance must be a finite number of nits, 0 or more.");
							}

							return this->withMaterial(entityName, componentName, [nits] (Beam & beam, Graphics::Material::BeamResource & material) {
								material.setLuminance(nits);

								return changed(beam, "Beam '" + beam.name() + "' luminance set.");
							});
						}, Console::CommandHint::Idempotent);

					this->bindCommand("setWidth", "Sets the half width of a Beam and its cross-section profile.",
						{entity, component, {"halfWidth", "The half width, in entity units (metres), 0 or more."}, {"coreExponent", "The profile (1 - side²)^exponent: 1 soft glow, 4 hard core (at least 0.1)."}},
						[this] (const std::string & entityName, const std::string & componentName, float halfWidth, float coreExponent) {
							if ( !std::isfinite(halfWidth) || !std::isfinite(coreExponent) || halfWidth < 0.0F )
							{
								return Console::CommandResult::error("The half width must be finite, 0 or more; the exponent finite.");
							}

							return this->withMaterial(entityName, componentName, [halfWidth, coreExponent] (Beam & beam, Graphics::Material::BeamResource & material) {
								material.setHalfWidth(halfWidth);
								material.setCoreExponent(coreExponent);

								return changed(beam, "Beam '" + beam.name() + "' width set.");
							});
						}, Console::CommandHint::Idempotent);

					this->bindCommand("setArc", "Sets how far and how finely a Beam wanders across its curve (pinned at both ends): an amplitude of 0 is a laser. Refused on a beam of 2 stations (getState's stationCount: a straight beam of 1 segment), which cannot wander.",
						{entity, component, {"amplitude", "The largest offset across the beam, in entity units, 0 or more."}, {"frequency", "Noise cycles along the whole beam, 0 or more."}, {"octaves", "Noise octaves (the detail), 1-8."}},
						[this] (const std::string & entityName, const std::string & componentName, float amplitude, float frequency, int32_t octaves) {
							if ( !std::isfinite(amplitude) || !std::isfinite(frequency) || amplitude < 0.0F || frequency < 0.0F || octaves < 1 )
							{
								return Console::CommandResult::error("The amplitude and frequency must be finite, 0 or more; the octaves 1 or more.");
							}

							return this->withMaterial(entityName, componentName, [amplitude, frequency, octaves] (Beam & beam, Graphics::Material::BeamResource & material) {
								/* Two stations have no interior one to displace: the arc would be accepted and never drawn. */
								if ( amplitude > 0.0F && beam.stationCount() < 3 )
								{
									return Console::CommandResult::error("Beam '" + beam.name() + "' has " + std::to_string(beam.stationCount()) + " stations: it cannot wander. Build it with more segments (Beam::DefaultSegmentCount is " + std::to_string(Beam::DefaultSegmentCount) + ") or give it a curve.");
								}

								material.setArc(amplitude, frequency, static_cast< uint32_t >(octaves));

								return changed(beam, "Beam '" + beam.name() + "' arc set.");
							});
						}, Console::CommandHint::Idempotent);

					this->bindCommand("setArcMotion", "Sets how a Beam's arc lives: its seed, its re-strike rate and its drift.",
						{entity, component, {"seed", "The seed, 0 or more: two beams of the same seed draw the same arc."}, {"restrikeRate", "New arcs per second, 0 = never."}, {"drift", "Noise cycles per second the arc scrolls along the beam."}},
						[this] (const std::string & entityName, const std::string & componentName, int32_t seed, float restrikeRate, float drift) {
							if ( seed < 0 || !std::isfinite(restrikeRate) || !std::isfinite(drift) || restrikeRate < 0.0F )
							{
								return Console::CommandResult::error("The seed must be 0 or more, the re-strike rate finite and 0 or more, the drift finite.");
							}

							return this->withMaterial(entityName, componentName, [seed, restrikeRate, drift] (Beam & beam, Graphics::Material::BeamResource & material) {
								material.setArcMotion(static_cast< uint32_t >(seed), restrikeRate, drift);

								return changed(beam, "Beam '" + beam.name() + "' arc motion set.");
							});
						}, Console::CommandHint::Idempotent);
				}
		};
	}

	void
	appendBeamConsoleAdapters (const Manager & sceneManager, std::vector< std::unique_ptr< Console::ControllableTrait > > & adapters) noexcept
	{
		adapters.emplace_back(std::make_unique< BeamConsoleAdapter >(sceneManager));
	}
}
