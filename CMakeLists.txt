cmake_minimum_required(VERSION 3.27)
project(CNG C)

# Установка стандарта C
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

# Исходные файлы
set(SOURCES lab9/Enc_Dec_File.c)

# Создание исполняемого файла
add_executable(CNG ${SOURCES})

# Линковка с необходимыми библиотеками
target_link_libraries(CNG bcrypt ncrypt)

# Установка свойств выходного файла
set_target_properties(CNG PROPERTIES
        SUFFIX ".exe"           # Выходной файл с расширением .exe
        OUTPUT_NAME "CNG"       # Имя выходного файла
)
