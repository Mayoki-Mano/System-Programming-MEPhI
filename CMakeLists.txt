cmake_minimum_required(VERSION 3.16)

project(ShellExtensionHandler)

# Установка стандартов для компилятора
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED YES)

# Для MSVC
if(MSVC)
    # Включаем статическую линковку CRT
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MT /D_CRT_SECURE_NO_WARNINGS")

    # Настройки для отладочной и релизной сборки
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /O2 /MT")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /D_DEBUG /MTd")

    # Указание флагов для правильной сборки Windows приложения
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /SUBSYSTEM:WINDOWS")
endif()

# Указание директорий для поиска заголовочных файлов Windows SDK
if(DEFINED ENV{WindowsSdkDir})
    include_directories(
            ${CMAKE_SOURCE_DIR}/include
            $ENV{WindowsSdkDir}/Include
    )
    link_directories(
            $ENV{WindowsSdkDir}/Lib
    )
else()
    message(WARNING "Windows SDK directory not found. Make sure to set the WindowsSdkDir environment variable.")
endif()

# Указываем необходимые библиотеки для линковки
set(LINK_LIBS
        ole32.lib
        oleaut32.lib
        uuid.lib
        shlwapi.lib
        shell32.lib
        advapi32.lib
        comdlg32.lib
)

# Файлы исходного кода
set(SOURCES
        lab10/ContextMenuHandler.cpp lab10/ContextMenuHandler.h
)

# Создание DLL
add_library(ShellExtensionHandler SHARED ${SOURCES})

# Линковка с необходимыми библиотеками
target_link_libraries(ShellExtensionHandler ${LINK_LIBS})

# Указываем директорию с файлами заголовков
target_include_directories(ShellExtensionHandler PRIVATE ${CMAKE_SOURCE_DIR}/include)



# Принудительное выполнение команды MIDL для таргета ShellExtensionHandler
add_custom_command(
        TARGET ShellExtensionHandler PRE_BUILD
        COMMAND "C:/Program Files (x86)/Windows Kits/10/bin/10.0.26100.0/x64/midl.exe"
        ${CMAKE_SOURCE_DIR}/lab10/ContextMenuHandler.idl
        /h "${CMAKE_SOURCE_DIR}/lab10/ContextMenuHandler_h.h"
        /tlb "${CMAKE_SOURCE_DIR}/lab10/ContextMenuHandler.tlb"
        /target NT100
        /I "C:/Program Files (x86)/Windows Kits/10/Include/10.0.26100.0/um"
        /I "C:/Program Files (x86)/Windows Kits/10/Include/10.0.26100.0/shared"
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/lab10
        COMMENT "Running MIDL compiler to generate headers and stub files"
        VERBATIM
)

# Копирование иконок в папку с DLL
file(GLOB ICONS_FILES "${CMAKE_SOURCE_DIR}/lab10/resources/*.ico")
add_custom_command(TARGET ShellExtensionHandler POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${ICONS_FILES}
        $<TARGET_FILE_DIR:ShellExtensionHandler>)

# Директория для установки (например, для regsvr32)
set(CMAKE_INSTALL_PREFIX ${CMAKE_BINARY_DIR}/install)

# Устанавливаем DLL после сборки
install(TARGETS ShellExtensionHandler
        RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)
