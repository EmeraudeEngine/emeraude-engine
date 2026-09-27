## 8. CEF Integration (JavaScript)

All console commands are also accessible from JavaScript in CEF pages via:

```javascript
window.engine.execute("Core.SettingsService.getJson()");
```

Pages implement `onEngineResponse(command, outputs)` to receive results:

```javascript
function onEngineResponse(command, outputs) {
    if (command.indexOf("getJson") !== -1 && outputs.length > 0) {
        const data = JSON.parse(outputs[0].message);
        // Use data...
    }
}
```
