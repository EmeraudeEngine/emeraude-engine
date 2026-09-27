## Development Patterns

### Adding a New Generator
1. Inherit from `Generator::Abstract`
2. Implement `computeProgramCacheKey()` - **REQUIRED** for cache system
3. Implement `prepareUniformSets()`, `onGenerateShadersCode()`, `onCreateDataLayouts()`, `onGraphicsPipelineConfiguration()`
4. Cache key must include all parameters that affect shader output

### Extending an Existing Generator
1. Identify generator type (Scene/Overlay/Shadow)
2. Add conditions in generate methods
3. Test all material/geometry combinations
4. Verify cache performance (input hash)

### Debugging Generation Failures
1. Examine detailed logs (material vs geometry)
2. Verify tangent export (Blender/Maya)
3. Simplify material OR enrich geometry
4. Test with default material first
