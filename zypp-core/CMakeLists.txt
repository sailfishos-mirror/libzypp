# Collect all files that may contain translatable texts
FILE( GLOB_RECURSE POT_FILE_DEPENDS RELATIVE ${LIBZYPP_SOURCE_DIR} "*.h" "*.cc" )
SET( POT_FILE_DEPENDS_ZYPP_CORE ${POT_FILE_DEPENDS} PARENT_SCOPE )

INCLUDE_DIRECTORIES ( ${LIBZYPP_SOURCE_DIR} )

ADD_DEFINITIONS( -DLOCALEDIR="${CMAKE_INSTALL_PREFIX}/share/locale" -DTEXTDOMAIN="zypp" -DZYPP_DLL )

SET( zypp_toplevel_headers
  ${CMAKE_CURRENT_BINARY_DIR}/APIConfig.h
  AutoDispose.h
  ByteArray.h
  ByteCount.h
  CheckSum.h
  Date.h
  Digest.h
  ExternalProgram.h
  Globals.h
  KVMap
  kvmap.h
  ManagedFile.h
  MirroredOrigin.h
  onmedialocation.h
  OnMediaLocation
  Pathname.h
  TriBool.h
  Url.h
  UserData.h
)

SET( zypp_toplevel_SRCS
  ByteCount.cc
  CheckSum.cc
  Date.cc
  Digest.cc
  ExternalProgram.cc
  MirroredOrigin.cc
  onmedialocation.cc
  Pathname.cc
  ShutdownLock.cc
  Url.cc
)
INSTALL(  FILES ${zypp_toplevel_headers} DESTINATION "${INCLUDE_INSTALL_DIR}/zypp-core" )

SET( zypp_base_HEADERS
  base/DefaultIntegral
  base/defaultintegral.h
  base/DtorReset
  base/dtorreset.h
  base/Easy.h
  base/EnumClass.h
  base/Errno.h
  base/Exception.h
  base/ExternalDataSource.h
  base/Function.h
  base/Flags.h
  base/filestreambuf.h
  base/FileStreamBuf
  base/fXstream
  base/fxstream.h
  base/Gettext.h
  base/GzStream
  base/gzstream.h
  base/Hash.h
  base/InputStream
  base/inputstream.h
  base/IOStream.h
  base/IOTools.h
  base/Iterable.h
  base/Iterator.h
  base/LogControl.h
  base/LogTools.h
  base/Logger.h
  base/NonCopyable.h
  base/ProfilingFormater.h
  base/ProvideNumericId
  base/providenumericid.h
  base/PtrTypes.h
  base/ReferenceCounted.h
  base/Regex.h
  base/SimpleStreambuf
  base/simplestreambuf.h
  base/String.h
  base/StringV.h
  base/Unit.h
  base/UserRequestException
  base/userrequestexception.h
  base/Xml.h
)

SET( zypp_base_SRCS
  base/CleanerThread.cc
  base/Exception.cc
  base/ExternalDataSource.cc
  base/filestreambuf.cc
  base/Gettext.cc
  base/gzstream.cc
  base/inputstream.cc
  base/IOStream.cc
  base/IOTools.cc
  base/LogControl.cc
  base/ProfilingFormater.cc
  base/ReferenceCounted.cc
  base/Regex.cc
  base/String.cc
  base/StringV.cc
  base/Unit.cc
  base/userrequestexception.cc
  base/Xml.cc
)

IF (ENABLE_ZCHUNK_COMPRESSION)

  list( APPEND zypp_base_SRCS
    base/zckstream.cc
  )

  list( APPEND zypp_base_HEADERS
    base/ZckStream
    base/zckstream.h
  )

ENDIF(ENABLE_ZCHUNK_COMPRESSION)

INSTALL(  FILES ${zypp_base_HEADERS} DESTINATION "${INCLUDE_INSTALL_DIR}/zypp-core/base" )


SET( zypp_fs_SRCS
  fs/PathInfo.cc
  fs/TmpPath.cc
)

SET( zypp_fs_HEADERS
  fs/PathInfo.h
  fs/TmpPath.h
  fs/WatchFile
  fs/watchfile.h
)

INSTALL(  FILES ${zypp_fs_HEADERS} DESTINATION "${INCLUDE_INSTALL_DIR}/zypp-core/fs" )


SET( zypp_rpc_SRCS
  rpc/PluginFrame.cc
  rpc/PluginFrameException.cc
)

SET( zypp_rpc_HEADERS
  rpc/PluginFrame.h
  rpc/PluginFrameException.h
)

INSTALL(  FILES ${zypp_rpc_HEADERS} DESTINATION "${INCLUDE_INSTALL_DIR}/zypp-core/rpc" )


SET( zypp_ui_SRCS
  ui/progressdata.cc
)

SET( zypp_ui_HEADERS
  ui/ProgressData
  ui/progressdata.h
)

INSTALL(  FILES ${zypp_ui_HEADERS} DESTINATION "${INCLUDE_INSTALL_DIR}/zypp-core/ui" )


SET( zypp_url_SRCS
  url/UrlUtils.cc
  url/UrlBase.cc
)

SET( zypp_url_HEADERS
  url/UrlBase.h
  url/UrlException.h
  url/UrlUtils.h
)

INSTALL(  FILES ${zypp_url_HEADERS} DESTINATION "${INCLUDE_INSTALL_DIR}/zypp-core/url" )

SET( zypp_parser_SRCS
  parser/iniparser.cc
  parser/inidict.cc
  parser/json.cc
  parser/json/JsonValue.cc
  parser/parseexception.cc
  parser/sysconfig.cc
)

SET ( zypp_parser_private_HEADERS
  parser/json/JsonBool.h
  parser/json/JsonNull.h
  parser/json/JsonNumber.h
  parser/json/JsonString.h
  parser/json/JsonValue.h
)

SET( zypp_parser_HEADERS
  parser/IniParser
  parser/iniparser.h
  parser/IniDict
  parser/inidict.h
  parser/json.h
  parser/ParseException
  parser/parseexception.h
  parser/Sysconfig
  parser/sysconfig.h
)

INSTALL(  FILES ${zypp_parser_HEADERS} DESTINATION "${INCLUDE_INSTALL_DIR}/zypp-core/parser" )

SET( zypp_parser_xml_SRCS
  parser/xml/XmlEscape.cc
)

SET( zypp_parser_xml_HEADERS
  parser/xml/XmlEscape.h
)

INSTALL(  FILES ${zypp_parser_xml_HEADERS} DESTINATION "${INCLUDE_INSTALL_DIR}/zypp-core/parser/xml" )

SET( zyppng_async_HEADERS
  zyppng/async/AsyncOp
  zyppng/async/asyncop.h
)

SET( zyppng_base_SRCS
  zyppng/base/abstracteventsource.cc
  zyppng/base/base.cc
  zyppng/base/eventdispatcher_glib.cc
  zyppng/base/eventloop_glib.cc
  zyppng/base/linuxhelpers.cc
  zyppng/base/timer.cc
  zyppng/base/threaddata.cc
  zyppng/base/socketnotifier.cc
  zyppng/base/unixsignalsource.cpp
)

SET( zyppng_base_HEADERS
  zyppng/base/AbstractEventSource
  zyppng/base/abstracteventsource.h
  zyppng/base/AutoDisconnect
  zyppng/base/autodisconnect.h
  zyppng/base/Base
  zyppng/base/base.h
  zyppng/base/EventDispatcher
  zyppng/base/eventdispatcher.h
  zyppng/base/EventLoop
  zyppng/base/eventloop.h
  zyppng/base/signals.h
  zyppng/base/SocketNotifier
  zyppng/base/socketnotifier.h
  zyppng/base/statemachine.h
  zyppng/base/Timer
  zyppng/base/timer.h
  zyppng/base/unixsignalsource.h
  zyppng/base/zyppglobal.h
)

SET( zyppng_base_private_HEADERS
  zyppng/base/private/abstracteventsource_p.h
  zyppng/base/private/base_p.h
  zyppng/base/private/eventdispatcher_glib_p.h
  zyppng/base/private/eventloop_glib_p.h
  zyppng/base/private/linuxhelpers_p.h
  zyppng/base/private/threaddata_p.h
)

SET( zyppng_core_HEADERS
  zyppng/core/bytearray.h
  zyppng/core/ByteArray
  zyppng/core/url.h
  zyppng/core/Url
  zyppng/core/idstring.h
  zyppng/core/IdString
  zyppng/core/string.h
  zyppng/core/String
)

SET( zyppng_io_private_HEADERS
  zyppng/io/private/asyncdatasource_p.h
  zyppng/io/private/iobuffer_p.h
  zyppng/io/private/iodevice_p.h
  zyppng/io/private/socket_p.h
  zyppng/io/private/sockaddr_p.h
  zyppng/io/private/abstractspawnengine_p.h
  zyppng/io/private/forkspawnengine_p.h
)

SET( zyppng_io_SRCS
  zyppng/io/asyncdatasource.cpp
  zyppng/io/iobuffer.cc
  zyppng/io/iodevice.cc
  zyppng/io/process.cpp
  zyppng/io/socket.cc
  zyppng/io/sockaddr.cpp
  zyppng/io/abstractspawnengine.cc
  zyppng/io/forkspawnengine.cc
)

SET( zyppng_io_HEADERS
  zyppng/io/AsyncDataSource
  zyppng/io/asyncdatasource.h
  zyppng/io/IODevice
  zyppng/io/iodevice.h
  zyppng/io/Process
  zyppng/io/process.h
  zyppng/io/Socket
  zyppng/io/socket.h
  zyppng/io/sockaddr.h
  zyppng/io/SockAddr
)

SET( zyppng_meta_HEADERS
  zyppng/meta/Functional
  zyppng/meta/functional.h
  zyppng/meta/FunctionTraits
  zyppng/meta/function_traits.h
  zyppng/meta/TypeTraits
  zyppng/meta/type_traits.h
)

SET( zyppng_pipelines_HEADERS
  zyppng/pipelines/Algorithm
  zyppng/pipelines/algorithm.h
  zyppng/pipelines/AsyncResult
  zyppng/pipelines/asyncresult.h
  zyppng/pipelines/Await
  zyppng/pipelines/await.h
  zyppng/pipelines/Expected
  zyppng/pipelines/expected.h
  zyppng/pipelines/Lift
  zyppng/pipelines/lift.h
  zyppng/pipelines/MTry
  zyppng/pipelines/mtry.h
  zyppng/pipelines/Redo
  zyppng/pipelines/redo.h
  zyppng/pipelines/Transform
  zyppng/pipelines/transform.h
  zyppng/pipelines/Wait
  zyppng/pipelines/wait.h
)

SET( zyppng_rpc_HEADERS
  zyppng/rpc/stompframestream.h
)

SET( zyppng_rpc_SRCS
  zyppng/rpc/stompframestream.cc
)

SET( zyppng_thread_SRCS
  zyppng/thread/asyncqueue.cc
  zyppng/thread/wakeup.cpp
)

SET( zyppng_thread_HEADERS
  zyppng/thread/AsyncQueue
  zyppng/thread/asyncqueue.h
  zyppng/thread/private/asyncqueue_p.h
  zyppng/thread/Wakeup
  zyppng/thread/wakeup.h
)

SET( zyppng_thread_private_HEADERS
)

SET( zyppng_ui_SRCS
  zyppng/ui/progressobserver.cc
  zyppng/ui/userinterface.cc
  zyppng/ui/userrequest.cc
)

SET( zyppng_ui_HEADERS
  zyppng/ui/progressobserver.h
  zyppng/ui/ProgressObserver
  zyppng/ui/userinterface.h
  zyppng/ui/UserInterface
  zyppng/ui/userrequest.h
  zyppng/ui/UserRequest
)

SET( zyppng_ui_private_HEADERS
  zyppng/ui/private/userinterface_p.h
)

SET ( zypp_HEADERS
  ${zypp_toplevel_headers}
  ${zyppng_async_HEADERS}
  ${zypp_base_HEADERS}
  ${zypp_fs_HEADERS}
  ${zypp_rpc_HEADERS}
  ${zypp_ui_HEADERS}
  ${zypp_url_HEADERS}
  ${zypp_parser_HEADERS}
  ${zypp_parser_private_HEADERS}
  ${zypp_parser_xml_HEADERS}
  ${zyppng_base_HEADERS}
  ${zyppng_base_private_HEADERS}
  ${zyppng_core_HEADERS}
  ${zyppng_io_HEADERS}
  ${zyppng_io_private_HEADERS}
  ${zyppng_meta_HEADERS}
  ${zyppng_pipelines_HEADERS}
  ${zyppng_rpc_HEADERS}
  ${zyppng_thread_HEADERS}
  ${zyppng_thread_private_HEADERS}
  ${zyppng_ui_HEADERS}
  ${zyppng_ui_private_HEADERS}
)

SET ( zypp_SRCS
  ${zypp_toplevel_SRCS}
  ${zypp_base_SRCS}
  ${zypp_fs_SRCS}
  ${zypp_rpc_SRCS}
  ${zypp_ui_SRCS}
  ${zypp_url_SRCS}
  ${zypp_parser_SRCS}
  ${zypp_parser_xml_SRCS}
  ${zyppng_base_SRCS}
  ${zyppng_io_SRCS}
  ${zyppng_rpc_SRCS}
  ${zyppng_thread_SRCS}
  ${zyppng_ui_SRCS}
)

# Default loggroup for all files
SET_LOGGROUP( "zypp-core" ${zypp_SRCS} )

# System libraries
SET(UTIL_LIBRARY util)

if ( ZYPP_CXX_CLANG_TIDY OR ZYPP_CXX_CPPCHECK )

  if (ZYPP_CXX_CLANG_TIDY)
    set( CMAKE_CXX_CLANG_TIDY ${ZYPP_CXX_CLANG_TIDY} )
  endif(ZYPP_CXX_CLANG_TIDY)

  if (ZYPP_CXX_CPPCHECK)
    set(CMAKE_CXX_CPPCHECK ${ZYPP_CXX_CPPCHECK})
  endif(ZYPP_CXX_CPPCHECK)

endif( ZYPP_CXX_CLANG_TIDY OR ZYPP_CXX_CPPCHECK )

ADD_LIBRARY( zypp-core STATIC ${zypp_SRCS} ${zypp_HEADERS} )

target_link_libraries( zypp-core PRIVATE zypp_lib_compiler_flags )
target_link_libraries( zypp-core INTERFACE ${LIBGLIB_LIBRARIES} )
target_link_libraries( zypp-core INTERFACE ${OPENSSL_LIBRARIES} )
target_link_libraries( zypp-core INTERFACE ${CRYPTO_LIBRARIES} )
target_link_libraries( zypp-core INTERFACE pthread )
target_link_libraries( zypp-core INTERFACE ${SIGCPP_LIBRARIES} )
TARGET_LINK_LIBRARIES( zypp-core INTERFACE ${ZLIB_LIBRARY} )
TARGET_LINK_LIBRARIES( zypp-core INTERFACE ${UTIL_LIBRARY} )

IF (ENABLE_ZSTD_COMPRESSION)
  TARGET_LINK_LIBRARIES( zypp-core INTERFACE ${ZSTD_LIBRARY})
ENDIF (ENABLE_ZSTD_COMPRESSION)

IF (ENABLE_ZCHUNK_COMPRESSION)
  TARGET_LINK_LIBRARIES( zypp-core INTERFACE ${ZCHUNK_LDFLAGS})
ENDIF(ENABLE_ZCHUNK_COMPRESSION)
