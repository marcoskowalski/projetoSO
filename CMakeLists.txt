# 3.20 basta para este projeto; exigir 4.0 impedia quem tem um CMake mais
# antigo (o do Ubuntu 24.04 é o 3.28) de sequer configurar o build.
cmake_minimum_required(VERSION 3.20)
project(ProjetoSO CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(headers)

# Todo o código do simulador (menos o main) fica numa biblioteca estática
# interna, para o executável e os testes usarem exatamente o mesmo código.
add_library(nucleo STATIC
        src/RelogioGlobal.cpp
        src/Tarefa.cpp
        src/TCB.cpp
        src/Configuracao.cpp
        src/LeitorConfig.cpp
        src/Escalonador.cpp
        src/EscalonadorRM.cpp
        src/EscalonadorEDF.cpp
        src/Simulador.cpp)

add_executable(ProjetoSO src/main.cpp)
target_link_libraries(ProjetoSO PRIVATE nucleo)

# Testes automatizados: "ctest" ou ./teste_simulador
enable_testing()
add_executable(teste_simulador testes/teste_simulador.cpp)
target_link_libraries(teste_simulador PRIVATE nucleo)
add_test(NAME simulador COMMAND teste_simulador)
