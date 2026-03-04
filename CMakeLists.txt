cmake_minimum_required(VERSION 3.15)
project(PageRank_CPP LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Include directory degli header
include_directories(include)

# Opzioni di debugging
option(USE_ASAN "Abilita AddressSanitizer" OFF)

# Sorgenti
file(GLOB_RECURSE SOURCES "src/*.cpp")
add_executable(pagerank ${SOURCES})

# --- OTTIMIZZAZIONI ---

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(pagerank PRIVATE 
        -O3 
        -Wall -Wextra 
        -pthread
        -march=native       
        -ffast-math 
        # --- NUOVI FLAG DI OTTIMIZZAZIONE ---
        -funroll-loops      # Aiuta la CPU a eseguire più iterazioni del loop computeChunk in parallelo
        -ftree-vectorize    # Forza il compilatore a usare istruzioni SIMD sui vettori dei Rank
        -fno-plt            # Riduce l'overhead delle chiamate a funzioni di libreria (migliora leggermente il tempo di I/O)
    )
    # Ottimizzazioni del Linker per ridurre i tempi di caricamento
    target_link_options(pagerank PRIVATE -pthread -Wl,-O1 -Wl,--as-needed)
endif()

# --- FIX PER IL CHECK IPO (LTO) ---
include(CheckIPOSupported) # Nota la 'd' finale
check_ipo_supported(RESULT supported OUTPUT error)

if(supported)
    set_target_properties(pagerank PROPERTIES INTERPROCEDURAL_OPTIMIZATION TRUE)
    message(STATUS "LTO (IPO) abilitato con successo.")
else()
    message(STATUS "LTO non supportato o errore: ${error}")
endif()

# --- SANITIZERS ---
if(USE_ASAN)
    target_compile_options(pagerank PRIVATE -fsanitize=address -fno-omit-frame-pointer)
    target_link_options(pagerank PRIVATE -fsanitize=address)
endif()