## 9. JSON Scene Input

TCP lines starting with `{` are routed to a registered JSON handler (not the normal command parser). This enables complete scene creation from a single JSON document.

- **Registration:** `Controller::setJsonHandler()` sets a `std::function< bool (const std::string &, Outputs &) >` callback
- **Setup:** `Core` registers the handler in `initializeSecondaryLevel()`, routing to `SceneManager::loadSceneFromJson()`
- **Flow:** TCP input -> `poll()` detects `{` prefix -> `m_jsonHandler(json, outputs)` -> scene built and enabled
- **Format:** See [`docs/ai-runtime-control.md`](../../ai-runtime-control.md) section 4 for the full JSON scene specification
