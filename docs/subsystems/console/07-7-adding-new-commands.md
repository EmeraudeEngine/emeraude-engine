## 7. Adding New Commands

Commands are registered in `onRegisterToConsole()` overrides with a **typed** `bindCommand()`
(rules: § 3 "every new command is TYPED"; machinery: `TypedBinding.hpp`, `Parameter.hpp`,
`CommandSignature.hpp`, `CommandResult.hpp`):

```cpp
// In YourService.console.cpp
void
YourService::onRegisterToConsole () noexcept
{
	// With parameters: one Parameter per lambda argument, in order.
	this->bindCommand("setLens", "Sets the lens of the active camera.",
		{
			{"focal", "Focal length, in millimetres."},
			{"sensorWidth", "Sensor width, in millimetres; unchanged when omitted."}
		},
		[this] (float focal, std::optional< float > sensorWidth) {
			if ( !this->hasCamera() )
			{
				return Console::CommandResult::error("No active camera !");
			}

			// ...
			return Console::CommandResult::success("Lens set.");
		}, Console::CommandHint::Idempotent);

	// Without parameters: no list at all.
	this->bindCommand("getState", "Returns the state as JSON.", [this] () {
		return Console::CommandResult::json(this->stateAsJson());
	}, Console::CommandHint::ReadOnly);
}
```

**Rules:**
- Only **services** inherit `ControllableTrait` (not runtime objects like scenes or entities)
- Services register via `registerToObject(parentService)` to build the hierarchy
- Commands execute on the **main thread** — safe to access engine state
- Convention: separate `*.console.cpp` file (e.g., `Settings.console.cpp`)
- Layout: the lambda body is indented one level deeper than the `[this] (…) {` line, and the
  closing `}, hints);` sits at the lambda's level (see `Window.console.cpp`, `Renderer.console.cpp`)
- A lambda capturing a helper returns errors as `CommandResult::error(reason)` — never write into
  a shared `Outputs` (see `src/Act.cpp` `activeCamera`)
- An argument that is out of range is an **error**, not silently clamped (`temporalCapture(0)`)
