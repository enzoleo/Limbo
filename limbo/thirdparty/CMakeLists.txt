add_subdirectory(CThreadPool)
if(ALGORIHTMS)
add_subdirectory(dlx)
endif(ALGORIHTMS)
add_subdirectory(flex)
add_subdirectory(lefdef)
add_subdirectory(libdivide)

if(ALGORIHTMS OR SOLVERS)
set(LEMON_ENABLE_GLPK OFF CACHE STRING "Turn off GLPK for Lemon")
set(LEMON_ENABLE_ILOG OFF CACHE STRING "Turn off ILOG for Lemon")
#set(LEMON_ENABLE_COIN OFF CACHE STRING "Turn off COIN for Lemon")
set(LEMON_ENABLE_SOPLEX OFF CACHE STRING "Turn off SOPLEX for Lemon")
add_subdirectory(lemon)
# lemon is installed to /include by default 
# create symbolic link 
if(INSTALL_LIMBO)
    install(CODE
        "EXECUTE_PROCESS(COMMAND ln -s ${CMAKE_INSTALL_PREFIX}/include/lemon ${CMAKE_INSTALL_PREFIX}/include/limbo/thirdparty/lemon)"
        )
endif(INSTALL_LIMBO)
endif(ALGORIHTMS OR SOLVERS)

if(PARSERS)
add_subdirectory(gzstream)
endif(PARSERS)
