cmake_minimum_required(VERSION 3.19)

project(openamp_demo)

set(CMAKE_C_COMPILER ${CC})
set(CMAKE_CXX_COMPILER ${CXX})

message(STATUS "SDKTARGETSYSROOT=$ENV{SDKTARGETSYSROOT}")
message(STATUS "DEMO_TARGET=${DEMO_TARGET}")

set(SHARED_LINK_LIBS
	$ENV{SDKTARGETSYSROOT}/usr/lib64/libopen_amp.so
	$ENV{SDKTARGETSYSROOT}/usr/lib64/libmetal.so
	$ENV{SDKTARGETSYSROOT}/lib64/libsysfs.so
)

add_library(openamp STATIC
	./modules/openamp_module.c
	./modules/remoteproc_module.c
	./modules/virtio_module.c
	./modules/rpmsg_module.c
)
target_link_libraries(openamp
	${SHARED_LINK_LIBS}
)

set(SRC_FILE
	./${DEMO_TARGET}/rpmsg_main.c
)

if(DEMO_TARGET MATCHES "openamp_demo")
elseif(DEMO_TARGET MATCHES "latency_demo")
elseif(DEMO_TARGET MATCHES "rpmsg_pty_demo")
	list(APPEND SRC_FILE "./${DEMO_TARGET}/rpmsg_pty.c")
else()
	message(FATAL_ERROR "DEMO_TARGET: choose a target to build.
		[openamp_demo|rpmsg_pty_demo]")   #rsc_table_demo|cyclitest_demo|rpmsg_service_demo|gdb_demo
endif()

include_directories(
	$ENV{SDKTARGETSYSROOT}/usr/include
	./modules
	./${DEMO_TARGET}
)
add_executable(rpmsg_main ${SRC_FILE})
target_link_libraries(rpmsg_main openamp)