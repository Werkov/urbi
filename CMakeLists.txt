cmake_minimum_required(VERSION 2.8)
project(kernel)

find_package(qibuild)

#find python
find_program(_python_executable
    NAMES python2 python python.exe
    NO_CMAKE_FIND_ROOT_PATH)
  if (NOT _python_executable)
    qi_error("needs python executable in PATH")
  endif()


add_subdirectory(sdk-remote)

qi_create_config_h(KCONFIG_H
   ${CMAKE_CURRENT_SOURCE_DIR}/include/urbi/kernel/config.h.in
   urbi/kernel/config.h
)


include_directories(${CMAKE_CURRENT_SOURCE_DIR}/sdk-remote/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/sdk-remote/src)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/sdk-remote/libport/include)
include_directories(${CMAKE_CURRENT_BINARY_DIR}/sdk-remote/libport/include)
include_directories(${CMAKE_CURRENT_BINARY_DIR}/include/urbi)

add_definitions(-DLIBPORT_URBI_UFLOAT_DOUBLE)
add_definitions(-DLIBPORT_LIBSFX="")
add_definitions(-DLIBPORT_LIBDIRNAME="lib")

if (HAVE_CUSTOM_BISON)

#invoke flex on utoken.l
qi_generate_src(
  ${CMAKE_CURRENT_BINARY_DIR}/src/parser/utoken.cc.raw
  SRC src/parser/utoken.l
  COMMAND flex -+ -o${CMAKE_CURRENT_BINARY_DIR}/src/parser/utoken.cc.raw ${CMAKE_CURRENT_SOURCE_DIR}/src/parser/utoken.l
)

#fix flex output
qi_generate_src(
  ${CMAKE_CURRENT_BINARY_DIR}/src/parser/utoken.cc
  SRC ${CMAKE_CURRENT_BINARY_DIR}/src/parser/utoken.cc.raw
  COMMAND ${_python_executable} ${CMAKE_CURRENT_SOURCE_DIR}/dev/fix-utoken.py ${CMAKE_CURRENT_BINARY_DIR}/src/parser/utoken.cc.raw ${CMAKE_CURRENT_BINARY_DIR}/src/parser/utoken.cc
)

# You need a bison patched by gostai, in the urbi-sdk/bison repository.
set(BISON_OPTS
  --defines --report=all -xml
)
set(BISON
  bison
)

qi_generate_src(
  ${CMAKE_CURRENT_BINARY_DIR}/src/parser/ugrammar.cc
  SRC src/parser/ugrammar.y
  COMMAND ${BISON} ${BISON_OPTS} ${CMAKE_CURRENT_SOURCE_DIR}/src/parser/ugrammar.y -o  ${CMAKE_CURRENT_BINARY_DIR}/src/parser/ugrammar.cc
)
else()
  qi_generate_src(
  ${CMAKE_CURRENT_BINARY_DIR}/src/parser/ugrammar.cc
  SRC src/parser/ugrammar.y
  COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/src/parser/pregenerated/ ${CMAKE_CURRENT_BINARY_DIR}/src/parser/
  )
  qi_generate_src(
  ${CMAKE_CURRENT_BINARY_DIR}/src/parser/utoken.cc
  SRC src/parser/ugrammar.y
  COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/src/parser/pregenerated/ ${CMAKE_CURRENT_BINARY_DIR}/src/parser/
  )
endif()

# Go for plugin
function(astgen target command)
 qi_generate_src(
   ${CMAKE_CURRENT_SOURCE_DIR}/${target}
  SRC src/ast/ast.yml
  COMMAND ${_python_executable} ${CMAKE_CURRENT_SOURCE_DIR}/dev/ast-${command}-gen ${CMAKE_CURRENT_SOURCE_DIR}/src/ast < ${CMAKE_CURRENT_SOURCE_DIR}/src/ast/ast.yml
 )
endfunction()


astgen(src/ast/all.cc all)
astgen(src/ast/cloner.cc cloner)
astgen(src/ast/transformer.cc transformer)
astgen(src/ast/fwd.hh fwd)
astgen(src/ast/default-visitor.hh default-visitor)
astgen(src/ast/visitor.hh visitor)
astgen(src/ast/pretty-printer.cc pretty-printer)
astgen(src/ast/dot-printer.cc dot-printer)
astgen(src/ast/serializer.hh serializer)
astgen(src/ast/implicit.hh nodes)


# To generate ast stuffs, you need to run the following commands
#./dev/ast-nodes-gen  src/ast/ < src/ast/ast.yml
#./dev/ast-all-gen  src/ast/ < src/ast/ast.yml
#./dev/ast-cloner-gen  src/ast/ < src/ast/ast.yml
#./dev/ast-dot-printer-gen  src/ast/ < src/ast/ast.yml
#./dev/ast-fwd-gen  src/ast/ < src/ast/ast.yml
#./dev/ast-node-mk-gen  src/ast/ < src/ast/ast.yml
#./dev/ast-pretty-printer-gen  src/ast/ < src/ast/ast.yml
#./dev/ast-readme-gen  src/ast/ < src/ast/ast.yml
#./dev/ast-serializer-gen  src/ast/ < src/ast/ast.yml
#./dev/ast-transformer-gen  src/ast/ < src/ast/ast.yml
#./dev/ast-visitor-gen  src/ast/ < src/ast/ast.yml


set(UOBJECT_PLUGIN_SRC
${CMAKE_CURRENT_BINARY_DIR}/include/urbi/kernel/config.h
src/ast/implicit.hh
src/ast/visitor.hh
src/ast/all.cc
  src/ast/analyzer.cc
src/ast/analyzer.hh
src/ast/catches-type.cc
src/ast/catches-type.hh
src/ast/cloner.hxx
src/ast/cloner.cc
src/ast/default-visitor.hh
src/ast/dot-print.cc
src/ast/dot-print.hh
src/ast/dot-printer.cc
src/ast/event-match.cc
src/ast/event-match.hh
src/ast/event-match.hxx
src/ast/exps-type.cc
src/ast/exps-type.hh
src/ast/factory.cc
src/ast/factory.hh
src/ast/flavor.cc
src/ast/flavor.hh
src/ast/flavor.hxx
src/ast/formal.cc
src/ast/formal.hh
src/ast/fwd.hh
src/ast/loc.cc
src/ast/loc.hh
src/ast/local-declarations-type.cc
src/ast/local-declarations-type.hh
src/ast/nary-fwd.hh
src/ast/new-clone.hh
src/ast/new-clone.hxx
src/ast/parametric-ast.cc
src/ast/parametric-ast.hh
src/ast/parametric-ast.hxx
src/ast/pretty-printer.cc
src/ast/print.cc
src/ast/print.hh
src/ast/symbols-type.cc
src/ast/symbols-type.hh
src/ast/transformer.cc
src/binder/bind.cc
src/binder/bind.hh
src/binder/binder.cc
src/binder/binder.hh
src/binder/binder.hxx
src/binder/fwd.hh
src/console-server.cc
src/eval/action.hh
src/eval/ast.cc
src/eval/ast.hh
src/eval/ast.hxx
src/eval/call.cc
src/eval/call.hh
src/eval/call.hxx
src/eval/exec.hh
src/eval/exec.hxx
src/eval/fwd.hh
src/eval/raise.cc
src/eval/raise.hh
src/eval/send-message.cc
src/eval/send-message.hh
src/eval/send-message.hxx
src/flower/flow.cc
src/flower/flow.hh
src/flower/flower.cc
src/flower/flower.hh
src/kernel/connection-set.cc
src/kernel/connection-set.hh
src/kernel/connection.cc
src/kernel/connection.hh
src/kernel/server-timer.cc
src/kernel/server-timer.hh
src/kernel/uconnection.cc
src/kernel/ughostconnection.cc
src/kernel/ughostconnection.hh
src/kernel/uobject.cc
src/kernel/uobject.hh
src/kernel/userver.cc
src/kernel/utypes.cc
src/kernel/uvalue-cast.cc
src/kernel/uvalue-cast.hh
src/object/barrier.cc
src/object/centralized-slots.cc
src/object/code.cc
src/object/code.hh
src/object/cxx-helper.hh
src/object/cxx-object.cc
src/object/date.cc
src/object/dictionary.cc
src/object/directory.cc
src/object/duration.cc
src/object/event-handler.cc
src/object/event.cc
src/object/executable.cc
src/object/file.cc
src/object/finalizable.cc
src/object/finalizable.hh
src/object/float.cc
src/object/format-info.cc
src/object/format-info.hh
src/object/format-info.hxx
src/object/formatter.cc
src/object/formatter.hh
src/object/global.cc
src/object/hash-slots.hh
src/object/hash-slots.hxx
src/object/hash.cc
src/object/ioservice.cc
src/object/ioservice.hh
src/object/job.cc
src/object/list.cc
src/object/lobby.cc
src/object/local.mk
src/object/location.cc
src/object/matrix.cc
src/object/object-class.cc
src/object/object-class.hh
src/object/object-size.cc
src/object/object.cc
src/object/path.cc
src/object/position.cc
src/object/primitive.cc
src/object/profile.cc
src/object/profile.hh
src/object/register.cc
src/object/root-classes.cc
src/object/root-classes.hh
src/object/semaphore.cc
src/object/semaphore.hh
src/object/server.cc
src/object/server.hh
src/object/slot.cc
src/object/socket.cc
src/object/socket.hh
src/object/sorted-vector-slots.hh
src/object/sorted-vector-slots.hxx
src/object/string.cc
src/object/subscription.cc
src/object/symbols.cc
src/object/system.cc
src/object/system.hh
src/object/tag.cc
src/object/uconnection.cc
src/object/uconnection.hh
src/object/urbi-exception.cc
src/object/uvalue.cc
src/object/uvalue.hh
src/object/vector-slots.hh
src/object/vector-slots.hxx
src/object/vector.cc
src/parser/flex-lexer.hh
src/parser/fwd.hh
src/parser/is-keyword.cc
src/parser/is-keyword.hh
src/parser/metavar-map.hh
src/parser/metavar-map.hxx
src/parser/parse.cc
src/parser/parse.hh
src/parser/parser-impl.cc
src/parser/parser-impl.hh
src/parser/parser-impl.hxx
src/parser/parser-utils.cc
src/parser/parser-utils.hh
src/parser/transform.cc
src/parser/transform.hh
src/parser/uparser.cc
src/parser/uparser.hh
src/parser/utoken.hh
${CMAKE_CURRENT_BINARY_DIR}/src/parser/utoken.cc
src/parser/ugrammar.cc
src/rewrite/desugarer.cc
src/rewrite/desugarer.hh
src/rewrite/fwd.hh
src/rewrite/pattern-binder.cc
src/rewrite/pattern-binder.hh
src/rewrite/rescoper.cc
src/rewrite/rescoper.hh
src/rewrite/rewrite.cc
src/rewrite/rewrite.hh
src/runner/exception.cc
src/runner/exception.hh
src/runner/job.cc
src/runner/job.hh
src/runner/job.hxx
src/runner/local.mk
src/runner/raise.cc
src/runner/shell.cc
src/runner/shell.hh
src/runner/sneaker.cc
src/runner/sneaker.hh
src/runner/stacks.cc
src/runner/stacks.hh
src/runner/stacks.hxx
src/runner/state.cc
src/runner/state.hh
src/runner/state.hxx
src/urbi/iostream.cc
src/urbi/iostream.hh
src/urbi/sdk.cc
sdk-remote/src/libuco/urbi-main.cc
)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)
include_directories(${CMAKE_CURRENT_BINARY_DIR}/src)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories(${CMAKE_CURRENT_BINARY_DIR}/include)
include_directories(${CMAKE_CURRENT_BINARY_DIR}/include/urbi/parser/)


# For ast/factory.cc
add_definitions(-D__SRCDIR__="")
add_definitions(-DBUILDING_URBI_SDK)
# uobject-remote is renamed as uobject using a set_target_properties
# but this one needs to be correct, because DEPENDS does not honor OUTPUT_NAME
qi_create_lib(uobject
  SUBFOLDER gostai/engine
  DEPENDS port sched BOOST BOOST_SIGNALS BOOST_FILESYSTEM BOOST_DATE_TIME  BOOST_SYSTEM BOOST_THREAD
  SRC ${UOBJECT_PLUGIN_SRC}
)



qi_stage_lib(uobject)


# Add a simple test:
enable_testing()
add_subdirectory(src/object/urbi)

add_subdirectory(share)
#Requires lib uobject
add_subdirectory(sdk-remote/src/uobjects)
add_subdirectory(tests)