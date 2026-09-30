/*
 * src/Scenes/Component/PathConsoleAdapter.cpp
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
#include <sstream>
#include <string>
#include <vector>

/* Third-party inclusions. */
#include "json/json.h"

/* Local inclusions. */
#include "FastJSON.hpp"
#include "Path.hpp"
#include "Scenes/AbstractEntity.hpp"

namespace EmEn::Scenes::Component
{
	using namespace Base;
	using namespace Base::Math;

	namespace
	{
		/**
		 * @brief Returns the full state of a path as JSON: its curve, its polyline, its look, its modes.
		 * @param path The path.
		 * @return std::string
		 */
		[[nodiscard]]
		std::string
		stateOf (const Path & path) noexcept
		{
			Json::Value state{Json::objectValue};
			state["name"] = path.name();
			state["entity"] = path.parentEntity().name();
			state["type"] = Path::ClassId;
			state["kind"] = Path::kindName(path.kind());
			state["closed"] = path.isClosed();
			state["enabled"] = path.isEnabled();
			state["debugMode"] = path.isDebugMode();
			state["tolerance"] = static_cast< double >(path.tolerance());
			state["sourcePointCount"] = static_cast< Json::UInt64 >(path.sourcePoints().size());
			state["polylinePointCount"] = static_cast< Json::UInt64 >(path.polyline().size());
			state["length"] = static_cast< double >(path.length());

			Json::Value debugColor{Json::arrayValue};
			debugColor.append(static_cast< double >(path.debugColor().red()));
			debugColor.append(static_cast< double >(path.debugColor().green()));
			debugColor.append(static_cast< double >(path.debugColor().blue()));
			debugColor.append(static_cast< double >(path.debugColor().alpha()));
			state["debugColor"] = debugColor;

			if ( const auto material = path.material(); material != nullptr )
			{
				Json::Value color{Json::arrayValue};
				color.append(static_cast< double >(material->color().red()));
				color.append(static_cast< double >(material->color().green()));
				color.append(static_cast< double >(material->color().blue()));

				state["color"] = color;
				state["luminance"] = static_cast< double >(material->luminance());
				state["halfWidth"] = static_cast< double >(material->halfWidth());
				state["widthInPixels"] = material->isWidthInPixels();
				state["roundJoins"] = material->areJoinsRound();
				state["miterLimit"] = static_cast< double >(material->miterLimit());
			}

			return FastJSON::stringify(state);
		}

		/**
		 * @brief The answer of a setter: the confirmation, then the path's NEW state as JSON.
		 * @param path The path, after the change.
		 * @param message The confirmation.
		 * @return Console::CommandResult
		 */
		[[nodiscard]]
		Console::CommandResult
		changed (const Path & path, std::string message) noexcept
		{
			return changedState(std::move(message), stateOf(path));
		}

		/**
		 * @brief The console adapter of the Path component: its curve, its look, its modes.
		 */
		class PathConsoleAdapter final : public ConsoleAdapter< Path >
		{
			public:

				explicit
				PathConsoleAdapter (const Manager & sceneManager) noexcept
					: ConsoleAdapter< Path >{sceneManager}
				{

				}

			private:

				/**
				 * @brief Runs an action on the material of a path, refusing a path that has none, then tells the path.
				 * @param entityName The entity address.
				 * @param componentName The component name.
				 * @param action A callable taking `Graphics::Material::PathResource &`.
				 * @param message The confirmation.
				 * @return Console::CommandResult
				 */
				template< typename action_t >
				[[nodiscard]]
				Console::CommandResult
				withMaterial (const std::string & entityName, const std::string & componentName, action_t && action, std::string message) const noexcept
				{
					return this->act(entityName, componentName, [&action, &message] (Path & path) {
						const auto material = path.material();

						if ( material == nullptr )
						{
							return Console::CommandResult::error("Path '" + path.name() + "' has no material.");
						}

						action(*material);

						path.markLookChanged();

						return changed(path, "Path '" + path.name() + "' " + message);
					});
				}

				void
				onRegisterToConsole () noexcept override
				{
					const Console::Parameter entity{entityParameter("the path")};
					const Console::Parameter component{componentParameter(Path::ClassId)};

					this->bindCommand("getState", "Returns the state of a Path as JSON: its curve (kind, closed, tolerance, point counts, length), its look (colour, luminance in nits, half width, pixels or metres, joins) and its modes (enabled, debug).",
						{entity, component},
						[this] (const std::string & entityName, const std::string & componentName) {
							return this->act(entityName, componentName, [] (Path & path) {
								return Console::CommandResult::json(stateOf(path));
							});
						}, Console::CommandHint::ReadOnly);

					this->bindCommand("setPoints", "Sets the curve of a Path from points in its entity's space: \"x y z; x y z; ...\". Kinds: Polyline (as they are), UniformBSpline (smooth, not through them), CatmullRom (through every point).",
						{
							entity,
							component,
							{"kind", "Polyline, UniformBSpline or CatmullRom (a Bezier path is set from code: Math::BSpline)."},
							{"points", "The points, \"x y z; x y z; ...\" (metres, UP is +Y)."},
							{"closed", "true to close the curve on itself.", false}
						},
						[this] (const std::string & entityName, const std::string & componentName, const std::string & kind, const std::string & text, bool closed) {
							const auto points = parsePointList(text);

							if ( !points.has_value() )
							{
								return Console::CommandResult::error("Malformed points: \"x y z; x y z; ...\", finite numbers.");
							}

							if ( kind != "Polyline" && kind != "UniformBSpline" && kind != "CatmullRom" )
							{
								return Console::CommandResult::error("Unknown kind '" + kind + "' (Polyline, UniformBSpline or CatmullRom).");
							}

							return this->act(entityName, componentName, [&kind, &points, closed] (Path & path) {
								const std::span< const Vector< 3, float > > source{points.value()};

								if ( kind == "Polyline" )
								{
									path.setPolyline(source, closed);
								}
								else if ( kind == "UniformBSpline" )
								{
									path.setUniformBSpline(source, closed);
								}
								else
								{
									path.setCatmullRom(source, closed);
								}

								return changed(path, "Path '" + path.name() + "' curve set.");
							});
						}, Console::CommandHint::Idempotent);

					this->bindCommand("setTolerance", "Sets the chord tolerance of a Path's curve tessellation: the largest distance between the curve and the drawn polyline, in entity units.",
						{entity, component, {"tolerance", "The tolerance, > 0 (0.01 = 1 cm)."}},
						[this] (const std::string & entityName, const std::string & componentName, float tolerance) {
							if ( !std::isfinite(tolerance) || tolerance <= 0.0F )
							{
								return Console::CommandResult::error("The tolerance must be a finite number above 0.");
							}

							return this->act(entityName, componentName, [tolerance] (Path & path) {
								path.setTolerance(tolerance);

								return changed(path, "Path '" + path.name() + "' tolerance set.");
							});
						}, Console::CommandHint::Idempotent);

					this->bindCommand("setColor", "Sets the colour of a Path (LINEAR, a hue: its brightness is the luminance).",
						{entity, component, {"red", "Red, 0-1."}, {"green", "Green, 0-1."}, {"blue", "Blue, 0-1."}},
						[this] (const std::string & entityName, const std::string & componentName, float red, float green, float blue) {
							if ( !std::isfinite(red) || !std::isfinite(green) || !std::isfinite(blue) || red < 0.0F || green < 0.0F || blue < 0.0F )
							{
								return Console::CommandResult::error("The colour channels must be finite, 0 or more.");
							}

							return this->withMaterial(entityName, componentName, [red, green, blue] (Graphics::Material::PathResource & material) {
								material.setColor(PixelFactory::Color< float >{red, green, blue, 1.0F});
							}, "colour set.");
						}, Console::CommandHint::Idempotent);

					this->bindCommand("setLuminance", "Sets the luminance of a Path, in nits (cd/m²): a monitor 200-300, a fluorescent tube ~10 000.",
						{entity, component, {"nits", "The luminance, 0 or more."}},
						[this] (const std::string & entityName, const std::string & componentName, float nits) {
							if ( !std::isfinite(nits) || nits < 0.0F )
							{
								return Console::CommandResult::error("The luminance must be a finite number of nits, 0 or more.");
							}

							return this->withMaterial(entityName, componentName, [nits] (Graphics::Material::PathResource & material) {
								material.setLuminance(nits);
							}, "luminance set.");
						}, Console::CommandHint::Idempotent);

					this->bindCommand("setWidth", "Sets the half width of a Path, in entity units (metres) or in pixels (constant on screen).",
						{entity, component, {"halfWidth", "The half width, 0 or more."}, {"inPixels", "true for pixels, false for entity units."}},
						[this] (const std::string & entityName, const std::string & componentName, float halfWidth, bool inPixels) {
							if ( !std::isfinite(halfWidth) || halfWidth < 0.0F )
							{
								return Console::CommandResult::error("The half width must be finite, 0 or more.");
							}

							return this->withMaterial(entityName, componentName, [halfWidth, inPixels] (Graphics::Material::PathResource & material) {
								material.setWidth(halfWidth, inPixels);
							}, "width set.");
						}, Console::CommandHint::Idempotent);

					this->bindCommand("setJoins", "Sets the joins of a Path: mitered (bevelled past the miter limit, butt caps) or round (round caps too).",
						{entity, component, {"round", "true for round joins and caps."}, {"miterLimit", "The miter limit (the SVG stroke-miterlimit), at least 1.", 4.0F}},
						[this] (const std::string & entityName, const std::string & componentName, bool round, float miterLimit) {
							if ( !std::isfinite(miterLimit) || miterLimit < 1.0F )
							{
								return Console::CommandResult::error("The miter limit must be finite, 1 or more.");
							}

							return this->withMaterial(entityName, componentName, [round, miterLimit] (Graphics::Material::PathResource & material) {
								material.setJoins(round, miterLimit);
							}, "joins set.");
						}, Console::CommandHint::Idempotent);

					this->bindCommand("setEnabled", "Shows or hides a Path.",
						{entity, component, {"state", "true to show it, false to hide it."}},
						[this] (const std::string & entityName, const std::string & componentName, bool state) {
							return this->act(entityName, componentName, [state] (Path & path) {
								path.setEnabled(state);

								return changed(path, std::string{"Path '"} + path.name() + (state ? "' shown." : "' hidden."));
							});
						}, Console::CommandHint::Idempotent);

					this->bindCommand("setDebugMode", "Switches a Path's DEBUG mode: always on top, after the tone mapping, in its display colour (setDebugColor).",
						{entity, component, {"state", "true for the debug mode, false for the world path."}},
						[this] (const std::string & entityName, const std::string & componentName, bool state) {
							return this->act(entityName, componentName, [state] (Path & path) {
								path.setDebugMode(state);

								return changed(path, std::string{"Path '"} + path.name() + (state ? "' in debug mode." : "' back in the world."));
							});
						}, Console::CommandHint::Idempotent);

					this->bindCommand("setDebugColor", "Sets the colour of a Path's debug mode, AS DISPLAYED (sRGB), with its opacity.",
						{entity, component, {"red", "Red, 0-1."}, {"green", "Green, 0-1."}, {"blue", "Blue, 0-1."}, {"opacity", "Opacity, 0-1.", 1.0F}},
						[this] (const std::string & entityName, const std::string & componentName, float red, float green, float blue, float opacity) {
							const auto valid = [] (float value) noexcept {
								return std::isfinite(value) && value >= 0.0F && value <= 1.0F;
							};

							if ( !valid(red) || !valid(green) || !valid(blue) || !valid(opacity) )
							{
								return Console::CommandResult::error("The colour channels and the opacity must be within 0-1.");
							}

							return this->act(entityName, componentName, [red, green, blue, opacity] (Path & path) {
								path.setDebugColor(PixelFactory::Color< float >{red, green, blue, opacity});

								return changed(path, "Path '" + path.name() + "' debug colour set.");
							});
						}, Console::CommandHint::Idempotent);
				}
		};
	}

	void
	appendPathConsoleAdapters (const Manager & sceneManager, std::vector< std::unique_ptr< Console::ControllableTrait > > & adapters) noexcept
	{
		adapters.emplace_back(std::make_unique< PathConsoleAdapter >(sceneManager));
	}
}
