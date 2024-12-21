cmake_minimum_required(VERSION 3.27)
project(MEPHI_4KURS_C C)

set(CMAKE_C_STANDARD 11)
#LAB6
add_executable(Server lab5/lab5.c)
#add_executable(Client lab6/socket_client.c)
#add_executable(Server lab6/pipe_server.c)
#add_executable(Client1 lab6/pipe_client.c
#add_executable(Client2 lab6/pipe_client.c)
#target_link_libraries(Server ws2_32)
#target_link_libraries(Client1 ws2_32)
#target_link_libraries(Client2 ws2_32)

#LAB7
#set (SERVICE_EXECUTABLE_DIR "C:/Program Files/")
#add_executable(USB
#        lab7/ServiceUSB.c)
#
#install(TARGETS USB RUNTIME  DESTINATION ${SERVICE_EXECUTABLE_DIR})
#target_link_libraries(USB cfgmgr32 )
#
#add_executable(ServiceManager
#        lab7/ServiceManager.c lab7/ServiceManager.h)
#add_dependencies(ServiceManager USB)
#add_custom_target(AutoInstall ALL
#        COMMAND ${CMAKE_COMMAND} --install ${CMAKE_BINARY_DIR} --prefix ${SERVICE_EXECUTABLE_DIR}
#        COMMENT "Installing project to ${SERVICE_EXECUTABLE_DIR}"
#        DEPENDS USB ServiceManager
#)