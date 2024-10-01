file(GLOB LIB_SOURCES "lib/*.c")

idf_component_register(SRCS "main.c" ${LIB_SOURCES}
                    INCLUDE_DIRS "."                  
                    EMBED_TXTFILES ${project_dir}/envdata/deviceid
                    EMBED_TXTFILES ${project_dir}/envdata/wifipass
                    EMBED_TXTFILES ${project_dir}/envdata/wifissid
                    )
idf_build_set_property(COMPILE_OPTIONS "-Wno-format-nonliteral;-Wno-format-security;-Wformat=0" APPEND)