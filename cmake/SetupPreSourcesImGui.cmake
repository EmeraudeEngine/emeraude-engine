message("Enabling ImGUI library from local source ...")

set(IMGUI_VERSION "1.9.7git")
set(IMGUI_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/dependencies/imgui)
set(IMGUI_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/dependencies/imgui)

set(IMGUI_SOURCE_FILES
	${IMGUI_SOURCE_DIR}/imgui.cpp

	${IMGUI_SOURCE_DIR}/imgui_demo.cpp
	${IMGUI_SOURCE_DIR}/imgui_draw.cpp
	${IMGUI_SOURCE_DIR}/imgui_tables.cpp
	${IMGUI_SOURCE_DIR}/imgui_widgets.cpp

	${IMGUI_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
	${IMGUI_SOURCE_DIR}/backends/imgui_impl_vulkan.cpp
)

# Dear ImGui is third-party code compiled from source inside the engine target: like a SYSTEM header, it is not judged
# by the cascade's paranoid warning set (Ave Robustus II, projet-alpha docs/plans/ave-robustus-ii.md § 6.2) — its
# warnings are upstream's to fix. The cascade's own code that CALLS ImGui is still fully checked.
if ( MSVC )
	set_source_files_properties(${IMGUI_SOURCE_FILES} PROPERTIES COMPILE_OPTIONS "/w")
else ()
	set_source_files_properties(${IMGUI_SOURCE_FILES} PROPERTIES COMPILE_OPTIONS "-w")
endif ()
