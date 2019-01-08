# project(qking-core)

file(GLOB_RECURSE QK_SOURCE ./ *.cc *.c)
message("===================QKING File=======================")
foreach(dir ${QK_SOURCE})
  message("${dir}")
endforeach()
message("----------------------------------------------------")
set(QK_SOURCE ${QK_SOURCE} PARENT_SCOPE)
# add_library("qking-core" ${QK_SOURCE})

#normal build
set(DEFINES_QKING QKING_NDEBUG)

#for debug only
# set(DEFINES_QKING DEBUG)
# set(DEFINES_QKING ${DEFINES_QKING} QKING_RAX_DEBUG)
# set(DEFINES_QKING ${DEFINES_QKING} JMEM_STATS)

set(DEFINES_QKING ${DEFINES_QKING} QKING_CPOINTER_32_BIT)
set(DEFINES_QKING ${DEFINES_QKING} QKING_ENABLE_ERROR_MESSAGES)
set(DEFINES_QKING ${DEFINES_QKING} QKING_ENABLE_EXTERNAL_CONTEXT)
set(DEFINES_QKING ${DEFINES_QKING} QKING_ENABLE_LOGGING)
set(DEFINES_QKING ${DEFINES_QKING} CONFIG_DISABLE_COMPILER_BUILTIN)
set(DEFINES_QKING ${DEFINES_QKING})

set(DEFINES_QKING_D)
foreach(dir ${DEFINES_QKING})
  set(DEFINES_QKING_D ${DEFINES_QKING_D} "-D${dir}")
endforeach()
set(DEFINES_QKING_D ${DEFINES_QKING_D} PARENT_SCOPE)

message("===================QKING Defines=======================")
foreach(dir ${DEFINES_QKING_D})
  message("${dir}")
endforeach()
message("----------------------------------------------------")
# target_compile_definitions("qking-core" PRIVATE ${DEFINES_QKING})

set(QK_INCLUDE_DIRS
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/base"
    "${CMAKE_CURRENT_SOURCE_DIR}/core"
    "${CMAKE_CURRENT_SOURCE_DIR}/core/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/core/api"
    "${CMAKE_CURRENT_SOURCE_DIR}/core/debugger"
    "${CMAKE_CURRENT_SOURCE_DIR}/core/ecma/base"
    "${CMAKE_CURRENT_SOURCE_DIR}/core/ecma/builtin-objects"
    "${CMAKE_CURRENT_SOURCE_DIR}/core/ecma/builtin-objects/typedarray"
    "${CMAKE_CURRENT_SOURCE_DIR}/core/ecma/operations"
    "${CMAKE_CURRENT_SOURCE_DIR}/core/jcontext"
    "${CMAKE_CURRENT_SOURCE_DIR}/core/jmem"
    "${CMAKE_CURRENT_SOURCE_DIR}/core/jrt"
    "${CMAKE_CURRENT_SOURCE_DIR}/core/lit"
    "${CMAKE_CURRENT_SOURCE_DIR}/core/vm"
    "${CMAKE_CURRENT_SOURCE_DIR}/core/parser/regexp"
    "${CMAKE_CURRENT_SOURCE_DIR}/decoder"
    "${CMAKE_CURRENT_SOURCE_DIR}/handler"
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/port/default/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/rax/vdom"
    "${CMAKE_CURRENT_SOURCE_DIR}/rax")
set(QK_INCLUDE_DIRS ${QK_INCLUDE_DIRS} PARENT_SCOPE)
# target_include_directories("qking-core" PRIVATE ${QK_INCLUDE_DIRS})

message("=============QKING inner include File===============")
foreach(dir ${QK_INCLUDE_DIRS})
  message("${dir}")
endforeach()
message("----------------------------------------------------")

set(QK_INTERFACE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/../ ${CMAKE_CURRENT_SOURCE_DIR}/rax ${CMAKE_CURRENT_SOURCE_DIR}rax/vdom)
set(QK_INTERFACE_DIRS ${QK_INTERFACE_DIRS} PARENT_SCOPE)
# target_include_directories("qking-core" PUBLIC ${QK_INTERFACE_DIRS})

# target_link_libraries("qking-core" json11)