
cmake_minimum_required(VERSION 3.27)
project(DeleteDriver C)

# Установка стандарта C
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

# Проверка архитектуры (только x64)
if(NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
    message(FATAL_ERROR "This driver supports only x64 architecture")
endif()
add_definitions(-D_AMD64_=1)

# Путь к WDK (настройте под свою систему)
set(WDK_PATH "C:/Program Files (x86)/Windows Kits/10")
set(WDK_VERSION "10.0.26100.0")  # Указываем версию явно
set(WDK_INCLUDE "${WDK_PATH}/Include/${WDK_VERSION}/km")
set(WDK_LIB "${WDK_PATH}/Lib/${WDK_VERSION}/km/x64")

# Проверка наличия WDK
if(NOT EXISTS "${WDK_INCLUDE}/fltKernel.h")
    message(FATAL_ERROR "WDK include path not found: ${WDK_INCLUDE}")
endif()
if(NOT EXISTS "${WDK_LIB}/fltmgr.lib")
    message(FATAL_ERROR "WDK library path not found: ${WDK_LIB}")
endif()

foreach(flag_var IN ITEMS CMAKE_C_FLAGS CMAKE_C_FLAGS_DEBUG CMAKE_C_FLAGS_RELEASE)
    string(REPLACE "/RTC0" "" ${flag_var} "${${flag_var}}")
    string(REPLACE "/RTCsu" "" ${flag_var} "${${flag_var}}")
    string(REPLACE "/RTC1" "" ${flag_var} "${${flag_var}}")
endforeach()
# Включение заголовочных файлов для kernel-mode
include_directories(${WDK_INCLUDE})

# Компиляторные опции для kernel-mode драйверов
add_compile_options(
        /kernel           # Режим ядра
        /W4               # Уровень предупреждений 4
        /wd4100           # Отключить предупреждение о неиспользуемых параметрах
        /Zi               # Отладочная информация
        /Od               # Отключить оптимизацию (для отладки)
        /DWIN32           # Определение WIN32
        /D_DEBUG          # Определение отладки
        /DUNICODE         # Поддержка Unicode
        /D_UNICODE        # Поддержка Unicode (для WDK)
        /GS-              # Отключить проверки буфера
        /RTC-             # Отключить все runtime-проверки
)

# Линкерные опции для kernel-mode драйверов
add_link_options(
        /SUBSYSTEM:NATIVE      # Нативная подсистема для ядра
        /DRIVER                # Указывает, что это драйвер
        /DEBUG                 # Генерировать отладочную информацию
        /ENTRY:DriverEntry     # Точка входа
        /NODEFAULTLIB          # Исключить стандартные библиотеки
        /INCREMENTAL:NO        # Отключить инкрементальную компоновку
        /MANIFEST:NO           # Без манифеста
        /OPT:REF               # Оптимизация ссылок (удаляет неиспользуемый код)
        /OPT:ICF               # Сворачивание идентичных функций
)

# Исходные файлы
set(SOURCES lab8/deleteDriver.c)

# Создание драйвера как MODULE
add_library(DeleteDriver MODULE ${SOURCES})

# Установка свойств
set_target_properties(DeleteDriver PROPERTIES
        SUFFIX ".sys"           # Выходной файл с расширением .sys
        OUTPUT_NAME "DeleteDriver"  # Имя выходного файла
)

# Связывание только с kernel-mode библиотеками
target_link_libraries(DeleteDriver PRIVATE
        "${WDK_LIB}/ntoskrnl.lib"  # Основная библиотека ядра
        "${WDK_LIB}/fltmgr.lib"    # Библиотека MiniFilter
)

# Установка языка компоновки и отключение CRT
set_target_properties(DeleteDriver PROPERTIES
        LINKER_LANGUAGE C
        MSVC_RUNTIME_LIBRARY ""   # Отключает добавление CRT по умолчанию
)