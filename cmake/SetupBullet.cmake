if ( NOT TARGET_BINARY_FOR_SETUP )
	message(FATAL_ERROR "TARGET_BINARY_FOR_SETUP is not SET !")
endif ()

message("Enabling Bullet library from local source ...")

find_package(Bullet REQUIRED)

target_include_directories(${TARGET_BINARY_FOR_SETUP} PUBLIC ${BULLET_INCLUDE_DIRS})
target_link_directories(${TARGET_BINARY_FOR_SETUP} PUBLIC ${BULLET_LIBRARIES})

message("Bullet library enabled !")
message(" - Headers : ${BULLET_INCLUDE_DIRS}")
message(" - Libraries : ${BULLET_LIBRARIES}")

set(BULLET_ENABLED On)

