## 3. Built-in Commands

### Top-level (no dot in the command)

| Command | Effect |
|---------|--------|
| `help`, `help()`, `lsfunc()` | **Recursive dump** of every registered object with each command's description — the single entry point for discovering the whole API |
| `listObjects`, `lsobj()` | List top-level controllable object names only (no recursion) |
| `exit`, `quit`, `shutdown` | Graceful shutdown (saves settings) |
| `hardExit` | Immediate shutdown (no save) |
| `describeCommands()` | Every command as JSON: path, help, typed, parameters (name, type, arity, description, default), hints |

### Per-object (available at any depth in the tree)

| Command | Effect |
|---------|--------|
| `<path>.help()` | Recursive dump of that object's sub-tree (commands + nested objects) |
| `<path>.lsfunc()` | List commands bound at that level only, in `name() - description` form |
| `<path>.lsobj()` | List sub-objects at that level only |

Example: `Core.RendererService.help()` shows every command reachable under `RendererService`, while `Core.RendererService.lsfunc()` shows only commands directly bound there.

### Rule: registration lifecycle (parent tracking)

`ControllableTrait::registerToObject(parent)` stores a **back-pointer to the parent**, and
the destructor removes the object from the parent's sub-object map in both directions
(2026-08-05). Before that, a destroyed sub-object left a **dangling raw pointer** in the
parent's map: the identifier stayed "taken" forever (`Sub object named 'X' already exists`)
and any command dispatched to it executed on freed memory (live segfault on
`SceneManagerService.Act.*` after an act switch in projet-alpha).

- `unregisterFromParent()` (public) detaches a still-alive object — use it when an object
  is *deactivated* but kept (e.g. a game act cycling active/inactive), so the replacement
  can register under the same identifier.
- `registerToObject()` is **idempotent** for the same parent, and re-registering under a
  new parent detaches from the previous one first.
- `onRegisterToConsole()` (command binding) runs **once per object lifetime**, not per
  registration — re-registering never re-binds (no `Command already exists` spam).

### Rule: every new command is TYPED (declared signature, 2026-09-27)

A command is registered with a **typed** `bindCommand()`: its parameters are **deduced from the
lambda's C++ signature**, only their names, descriptions and defaults are written by hand, and the
lambda returns a `Console::CommandResult`. The declaration can no longer drift from the code that
runs, the help line and its "Usage: …" are generated, and the same signature is what the future
MCP server turns into a tool schema (`docs/todo/native-mcp-server.md`). Full pattern in § 7.

- **Parameter types**: `bool`, `int32_t`, `float`, `std::string`, `Console::Argument` (any scalar —
  `SettingsService.set(key, value)`), by value or const reference; `std::optional< T >` for an
  omittable argument; a trailing `std::vector< T >` for zero or more (`openFiles`). Anything else,
  a `Parameter` count that differs from the arity, a non-`CommandResult` return or a vector that is
  not last **does not compile** (static_assert with a readable message).
- **Validation happens before the lambda runs**: a missing argument, one too many, or a value that
  does not convert answers an error naming the parameter (`Argument 'width' expects an integer, got
  'abc'.`) and the lambda never sees it. Conversions: an integer parameter accepts `5` and `5.0`,
  refuses `5.5`; a float accepts an integer; a boolean accepts `true`/`false`/`1`/`0` and **refuses
  any other integer** (the legacy `asBoolean()` read `2` as true); a string parameter receives the
  token **as typed** (`01` stays `"01"` — `Argument::source()`).
- **Defaults**: `Parameter{"frameCount", "…", 5}` makes a plain `T` omittable and shows `= 5` in the
  usage. A `double` literal is refused on purpose (write `1.0F`). A required parameter after an
  omittable one, a default on an `std::optional`, or a default of the wrong type is refused **at
  registration** (traced error, command not bound).
- **Description**: what the command does, capital, final period, no "Usage:" (generated). Each
  `Parameter` description states the meaning **and the unit** ("in metres", "in EV").
- **Hints** (`Console::CommandHint::ReadOnly | Destructive | Idempotent`): how a machine client may
  treat the command. Shown in the `help` dump.
- **Results**: `CommandResult::success/info/warning/error/json/binary`, `.add(Output)` to append,
  `fromOutputs(outputs, succeeded)` for a multi-line report with mixed severities. Only `error()`
  (or `fromOutputs(…, false)`) is a failure. `json()` marks the message as a JSON document — keep it
  valid JSON (escape names), a machine client will parse it.
- **There is no untyped form any more** (owner decision 2026-09-27): the raw
  `bindCommand(name, (Arguments, Outputs) -> bool, help)` overload was deleted once all 137 commands had
  migrated, and `Command` always carries its `CommandSignature` (`signature()` returns a reference). An
  application still using the old form fails to compile at the exact call to migrate.
- **JSON answers**: build them with `Json::Value` + `Base::FastJSON::stringify()` (escapes every
  string; ⚠️ it writes floats with 5 significant digits — for coordinates keep a stream and escape
  only the strings with `Json::valueToQuotedString()`, as `getNode()` does).
