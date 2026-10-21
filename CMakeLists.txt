cmake_minimum_required(VERSION 4.0)
project(ProjetoSO)

set(CMAKE_CXX_STANDARD 20)

include_directories(headers)

add_executable(ProjetoSO src/main.cpp
        src/RelogioGlobal.cpp
        headers/RelogioGlobal.h
        headers/EstadoTarefa.h
        src/Tarefa.cpp
        headers/Tarefa.h
        src/Configuracao.cpp
        headers/Configuracao.h
        src/LeitorConfig.cpp
        headers/LeitorConfig.h)
