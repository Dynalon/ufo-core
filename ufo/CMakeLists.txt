cmake_minimum_required(VERSION 2.6)

# --- Set sources -------------------------------------------------------------
set(ufocore_SRCS
    ufo-arch-graph.c
    ufo-buffer.c
    ufo-configurable.c
    ufo-config.c
    ufo-cpu-node.c
    ufo-cpu-task-iface.c
    ufo-dummy-task.c
    ufo-gpu-node.c
    ufo-gpu-task-iface.c
    ufo-graph.c
    ufo-group.c
    ufo-input-task.c
    ufo-node.c
    ufo-output-task.c
    ufo-plugin-manager.c
    ufo-profiler.c
    ufo-remote-node.c
    ufo-remote-task.c
    ufo-resources.c
    ufo-scheduler.c
    ufo-task-iface.c
    ufo-task-graph.c
    ufo-task-node.c
    )

set(ufocore_HDRS
    ufo-arch-graph.h
    ufo-buffer.h
    ufo-configurable.h
    ufo-config.h
    ufo-cpu-node.h
    ufo-cpu-task-iface.h
    ufo-dummy-task.h
    ufo-gpu-node.h
    ufo-gpu-task-iface.h
    ufo-graph.h
    ufo-group.h
    ufo-input-task.h
    ufo-node.h
    ufo-output-task.h
    ufo-plugin-manager.h
    ufo-profiler.h
    ufo-remote-node.h
    ufo-remote-task.h
    ufo-resources.h
    ufo-scheduler.h
    ufo-task-iface.h
    ufo-task-graph.h
    ufo-task-node.h
    )


# --- Find packages and libraries ---------------------------------------------
find_program(INTROSPECTION_SCANNER "g-ir-scanner")
find_program(INTROSPECTION_COMPILER "g-ir-compiler")
find_program(GLIB2_MKENUMS glib-mkenums REQUIRED)

# --- Add enum generation targets ---------------------------------------------
add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/ufo-enums.h
    COMMAND ${GLIB2_MKENUMS}
    ARGS
        --template ufo-enums.h.template
        ${ufocore_HDRS} > ${CMAKE_CURRENT_BINARY_DIR}/ufo-enums.h
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    DEPENDS ${ufocore_HDRS}
            ${CMAKE_CURRENT_SOURCE_DIR}/ufo-enums.h.template
)

add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/ufo-enums.c
    COMMAND ${GLIB2_MKENUMS}
    ARGS
        --template ufo-enums.c.template
        ${ufocore_HDRS} > ${CMAKE_CURRENT_BINARY_DIR}/ufo-enums.c
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    DEPENDS ${ufocore_HDRS} ${CMAKE_CURRENT_BINARY_DIR}/ufo-enums.h
            ${CMAKE_CURRENT_SOURCE_DIR}/ufo-enums.c.template
)

# --- Target ------------------------------------------------------------------
get_property(LIB64 GLOBAL PROPERTY FIND_LIBRARY_USE_LIB64_PATHS)

set(LIBDIR      "lib${LIB_SUFFIX}")
set(INCLUDEDIR  "include/ufo")

include_directories("${CMAKE_CURRENT_BINARY_DIR}")
add_definitions(-DUFO_COMPILATION)

if(CMAKE_BUILD_TYPE MATCHES "Release")
    add_definitions(-DG_DISABLE_ASSERT)
endif()

add_library(ufo SHARED
            ${ufocore_SRCS}
            ${ufocore_NODOC_SRCS}
            ${CMAKE_CURRENT_BINARY_DIR}/ufo-enums.c)

set_target_properties(ufo PROPERTIES
    VERSION ${PACKAGE_VERSION}
    SOVERSION ${UFO_SO_VERSION})

target_link_libraries(ufo ${UFOCORE_DEPS})

install(TARGETS ufo
        ARCHIVE DESTINATION ${LIBDIR}
        LIBRARY DESTINATION ${LIBDIR})

install(FILES ${ufocore_HDRS}
              ${CMAKE_CURRENT_SOURCE_DIR}/ufo.h
              ${CMAKE_CURRENT_BINARY_DIR}/ufo-enums.h
        DESTINATION ${INCLUDEDIR})


# --- pkg-config --------------------------------------------------------------
set(UFO_PKG_PREFIX      "${CMAKE_INSTALL_PREFIX}")
set(UFO_PKG_EXEC_PREFIX "${UFO_PKG_PREFIX}/bin")
set(UFO_PKG_INCLUDEDIR  "${UFO_PKG_PREFIX}/include")
set(UFO_PKG_GIRDIR      "${UFO_PKG_PREFIX}/share/gir-1.0")
set(UFO_PKG_LIBDIR      "${UFO_PKG_PREFIX}/${LIBDIR}")
set(UFO_PKG_TYPELIBDIR  "${UFO_PKG_PREFIX}/${LIBDIR}/girepository-1.0")

# FIXME: inside the ufo.pc.in we should set the lib names that we found out, not
# hard coded values
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/ufo.pc.in"
               "${CMAKE_CURRENT_BINARY_DIR}/ufo.pc" @ONLY IMMEDIATE)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/ufo.pc
        DESTINATION ${LIBDIR}/pkgconfig)


# --- Introspection files -----------------------------------------------------
if (INTROSPECTION_SCANNER AND INTROSPECTION_COMPILER)
    option(WITH_GIR "Build introspection files" ON)

    if (WITH_GIR)
        set(GIR_PREFIX "Ufo-${UFO_GIR_VERSION}")
        set(GIR_XML "${GIR_PREFIX}.gir")
        set(GIR_TYPELIB "${GIR_PREFIX}.typelib")
        set(_gir_input)

        foreach(_src ${ufocore_SRCS} ${ufocore_HDRS})
            list(APPEND _gir_input "${CMAKE_CURRENT_SOURCE_DIR}/${_src}")
        endforeach()

        add_custom_command(OUTPUT ${GIR_XML}
            COMMAND ${INTROSPECTION_SCANNER}
                    --namespace=Ufo
                    --nsversion=${UFO_GIR_VERSION}
                    --library=ufo
                    --no-libtool
                    --include=GObject-2.0
                    --include=GModule-2.0
                    --include=GLib-2.0
                    -I${OPENCL_INCLUDE_DIRS}
                    -I${CMAKE_CURRENT_SOURCE_DIR}/..
                    -DUFO_COMPILATION
                    --output ${GIR_XML}
                    --warn-all
                    ${_gir_input} > /dev/null
            DEPENDS ${ufocore_SRCS}
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})

        add_custom_command(OUTPUT ${GIR_TYPELIB}
            COMMAND ${INTROSPECTION_COMPILER}
                    -o ${GIR_TYPELIB}
                    ${GIR_XML}
            DEPENDS ${GIR_XML}
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})

        add_custom_target(gir ALL DEPENDS ${GIR_XML} ${GIR_TYPELIB})
        add_dependencies(gir ufo)

        install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${GIR_XML}
            DESTINATION share/gir-1.0)

        install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${GIR_TYPELIB}
            DESTINATION ${LIBDIR}/girepository-1.0)
    endif()
endif()


# --- Generate config.h -------------------------------------------------------
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config.h.in ${CMAKE_CURRENT_BINARY_DIR}/config.h)


# --- Build API reference -----------------------------------------------------
pkg_check_modules(GTK_DOC gtk-doc)
if(GTK_DOC_FOUND)
    option(WITH_GTK_DOC "Build API reference" ON)
    if (WITH_GTK_DOC)
        set(docs_dir "${CMAKE_CURRENT_BINARY_DIR}/../docs")
        set(docs_out "${docs_dir}/reference")
        set(_xml_doc_input)

        # Create xml entries for Ufo-docs.xml.in
        foreach(_src ${ufocore_SRCS})
            string(REPLACE ".c" ".xml" _xml_doc ${_src})
            list(APPEND _xml_doc_input "<xi:include href=\"xml/${_xml_doc}\"/>")
        endforeach()

        string(REPLACE ";" "\n" _xml_doc_input ${_xml_doc_input})

        get_directory_property(_current_include_dirs INCLUDE_DIRECTORIES)
        set(GTK_DOC_CFLAGS)
        foreach(_incl ${_current_include_dirs})
            set(GTK_DOC_CFLAGS "-I${_incl} ${GTK_DOC_CFLAGS}")
        endforeach()

        set(GTK_DOC_LDFLAGS)
        foreach(_lib ${UFOCORE_DEPS})
            if (NOT ${_lib} MATCHES "^[/]")
                set(GTK_DOC_LDFLAGS "-l${_lib} ${GTK_DOC_LDFLAGS}")
            endif()
        endforeach()

        configure_file("${CMAKE_CURRENT_SOURCE_DIR}/../docs/Ufo-docs.xml.in" "${docs_out}/Ufo-docs.xml")
        configure_file("${CMAKE_CURRENT_SOURCE_DIR}/../docs/scangobj.sh.in" "${docs_out}/scangobj.sh")

        set(reference_files
            "${docs_out}/index.html"
            "${docs_out}/UfoScheduler.html"
            "${docs_out}/style.css"
            "${docs_out}/Ufo.devhelp2"
            "${docs_out}/home.png"
            "${docs_out}/left.png"
            "${docs_out}/right.png"
            "${docs_out}/up.png")

        find_program(GTK_DOC_SCAN gtkdoc-scan REQUIRED)
        find_program(GTK_DOC_SCANGOBJ gtkdoc-scangobj REQUIRED)
        find_program(GTK_DOC_MKDB gtkdoc-mkdb REQUIRED)
        find_program(GTK_DOC_MKHTML gtkdoc-mkhtml REQUIRED)

        add_custom_command(OUTPUT ${docs_out}/Ufo-decl.txt
            COMMAND ${GTK_DOC_SCAN}
                    --module=Ufo
                    --source-dir=${CMAKE_CURRENT_SOURCE_DIR}
                    DEPENDS ${ufocore_SRCS}
            WORKING_DIRECTORY ${docs_out})

        add_custom_command(OUTPUT ${docs_out}/Ufo.args
            COMMAND sh scangobj.sh
            DEPENDS ${ufocore_SRCS} ${docs_out}/Ufo-decl.txt
            WORKING_DIRECTORY ${docs_out})

        add_custom_command(OUTPUT ${docs_out}/sgml.stamp
            COMMAND ${GTK_DOC_MKDB}
                    --module=Ufo
                    --source-dir=${CMAKE_CURRENT_SOURCE_DIR}
                    --sgml-mode
                    --output-format=xml
            DEPENDS ${docs_out}/Ufo.args ${docs_out}/Ufo-sections.txt ${ufocore_SRCS}
            WORKING_DIRECTORY ${docs_out})

        add_custom_command(OUTPUT ${docs_dir}/html.stamp
            COMMAND ${GTK_DOC_MKHTML}
                    Ufo
                    ${docs_out}/Ufo-docs.xml
            DEPENDS ${docs_out}/sgml.stamp
            WORKING_DIRECTORY ${docs_out})

        add_custom_target(reference ALL DEPENDS ${docs_dir}/html.stamp)

        install(FILES ${reference_files} DESTINATION share/gtk-doc/html/Ufo)
    endif()
endif(GTK_DOC_FOUND)