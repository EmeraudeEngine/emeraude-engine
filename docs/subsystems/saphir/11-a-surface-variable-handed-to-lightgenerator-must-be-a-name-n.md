## A surface variable handed to `LightGenerator` must be a NAME, never an expression

`declareSurfaceAlbedo()` and its siblings store the string they are given and the generator
**concatenates a swizzle onto it** — `m_surfaceAlbedo + ".rgb"` at five sites today
(`LightGenerator.cpp:941`, `:1045`, `:1213`, `:1234`, `LightGenerator.PBR.cpp:551`), and `+ ".a"`
elsewhere.

So a compound expression passed in becomes `a * b.rgb` instead of `(a * b).rgb`. **That still
compiles**, silently swizzling the wrong operand — there is no GLSL error and no engine log. Fold
the expression into a variable declared once and pass that variable's name (the folded
`SurfaceAlbedoFinal = albedo * svPrimaryVertexColor` of the vertex-colour path is the canonical
example).

⚠️ Recorded when `BasicResource` was removed (`a16195166`) — this contract long predates the merge
and outlives it.
