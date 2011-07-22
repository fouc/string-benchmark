
cmake_minimum_required (VERSION 2.6)

project (bytestring-benchmark)

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake" ${CMAKE_MODULE_PATH}) ## Find BoehmGC, V8

find_package(PythonLibs)
find_package(PerlLibs)
find_package(BoehmGC)
find_package(Qt4 4.4.3 COMPONENTS QtCore)

if( QT4_FOUND )
    include(${QT_USE_FILE})
endif( QT4_FOUND )

if(CMAKE_COMPILER_IS_GNUCXX)
    set(CMAKE_CXX_FLAGS "-Wall -Wextra -pedantic")
    set(CMAKE_CXX_FLAGS "-march=native")
endif()

## Build bstring
add_library( bstring SHARED third-party/bstrlib/bstrwrap.cpp third-party/bstrlib/bstrlib.c )

## Add new benchmarks here:
set(benchmarks new cat cmp slice)

foreach( benchmark ${benchmarks} )
    ## std::string
    add_executable( ${benchmark}-std-string "string-${benchmark}.cpp" )
    set_target_properties( ${benchmark}-std-string PROPERTIES COMPILE_FLAGS -DUSE_STD_STRING )

    ## __gnu_cxx::rope
    if(CMAKE_COMPILER_IS_GNUCXX)
        add_executable( ${benchmark}-ext-rope "string-${benchmark}.cpp" )
        set_target_properties( ${benchmark}-ext-rope PROPERTIES COMPILE_FLAGS -DUSE_EXT_ROPE )
        target_link_libraries( ${benchmark}-ext-rope ${BOEHMGC_LIBRARIES})
    endif()

    ## Paul Hsieh's Better String Library
    include_directories( third-party/bstrlib )
    add_executable( ${benchmark}-bstring "string-${benchmark}.cpp" )
    set_target_properties( ${benchmark}-bstring PROPERTIES COMPILE_FLAGS -DUSE_BSTRLIB )
    target_link_libraries( ${benchmark}-bstring bstring )

    ## Maxim Yegorushkin's boost::const_string
    include_directories( third-party )
    add_executable( ${benchmark}-yegorushkin-const-string "string-${benchmark}.cpp" )
    set_target_properties( ${benchmark}-yegorushkin-const-string PROPERTIES COMPILE_FLAGS -DUSE_CONST_STRING )

    ## NullString class
    add_executable( ${benchmark}-nop "string-${benchmark}.cpp" )
    set_target_properties( ${benchmark}-nop PROPERTIES COMPILE_FLAGS -DUSE_NOTHING )

    ## string::string + BoehmGC
    if( BOEHMGC_FOUND )
        include_directories( ${BOEHMGC_INCLUDE_DIRS} )
        add_executable( ${benchmark}-std-string-gc "string-${benchmark}.cpp" )
        set_target_properties( ${benchmark}-std-string-gc PROPERTIES COMPILE_FLAGS -DUSE_STD_STRING_GC )
        target_link_libraries( ${benchmark}-std-string-gc ${BOEHMGC_LIBRARIES})

        ## BoehmGC CORD
        include_directories( ${BOEHMGC_INCLUDE_DIRS} ${CORD_INCLUDE_DIRS} )
        add_executable( ${benchmark}-gc-cord-dynamic "string-${benchmark}.cpp" )
        set_target_properties( ${benchmark}-gc-cord-dynamic PROPERTIES COMPILE_FLAGS -DUSE_GC_CORD )
        target_link_libraries( ${benchmark}-gc-cord-dynamic ${BOEHMGC_LIBRARIES} cord)
    endif( BOEHMGC_FOUND )

    ## Python String Object
    if( PYTHONLIBS_FOUND )
        include_directories( ${PYTHON_INCLUDE_DIRS} )
        add_executable( ${benchmark}-python-string "string-${benchmark}.cpp" )
        set_target_properties( ${benchmark}-python-string PROPERTIES COMPILE_FLAGS -DUSE_PYTHON_STRING )
        target_link_libraries( ${benchmark}-python-string ${PYTHON_LIBRARIES})
    endif( PYTHONLIBS_FOUND )

    ## Perl scalars
    if( PERLLIBS_FOUND )
        include_directories( ${PERL_INCLUDE_PATH} )
        add_executable( ${benchmark}-perl-string "string-${benchmark}.cpp" )
        set_target_properties( ${benchmark}-perl-string PROPERTIES COMPILE_FLAGS -DUSE_PERL_STRING )
        target_link_libraries( ${benchmark}-perl-string ${PERL_LIBRARY})
    endif( PERLLIBS_FOUND )

    ## Qt4 QString
    if( QT4_FOUND )
        add_executable( ${benchmark}-qt4-string "string-${benchmark}.cpp" )
        set_target_properties( ${benchmark}-qt4-string PROPERTIES COMPILE_FLAGS -DUSE_QT4_STRING )
        target_link_libraries( ${benchmark}-qt4-string ${QT_LIBRARIES} )
    endif( QT4_FOUND )
endforeach(benchmark)
