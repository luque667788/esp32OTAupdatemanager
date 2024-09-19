file(GLOB LIB_SOURCES "lib/*.c")

idf_component_register(SRCS "main.c" ${LIB_SOURCES}
                    INCLUDE_DIRS ".")
idf_build_set_property(COMPILE_OPTIONS "-Wno-format-nonliteral;-Wno-format-security;-Wformat=0" APPEND)