INCLUDE_DIRECTORIES ( ${LIBZYPP_SOURCE_DIR} )

ADD_DEFINITIONS( -DLOCALEDIR="${CMAKE_INSTALL_PREFIX}/share/locale" -DTEXTDOMAIN="zypp" -DZYPP_DLL )

SET( zypp_toplevel_headers
  AutoDispose.h
  ByteArray.h
  ByteCount.h
  CheckSum.h
  Date.h
  Digest.h
  ExternalProgram.h
  Globals.h
  ManagedFile.h
  Pathname.h
  TriBool.h
  Url.h
)

SET( zypp_toplevel_SRCS
  ByteCount.cc
  CheckSum.cc
  Date.cc
  Digest.cc
  ExternalProgram.cc
  Pathname.cc
  Url.cc
)
INSTALL(  FILES ${zypp_toplevel_headers} DESTINATION "${INCLUDE_INSTALL_DIR}/zypp-core" )

SET( zypp_base_HEADERS
  base/Easy.h
  base/EnumClass.h
  base/Errno.h
  base/Exception.h
  base/ExternalDataSource.h
  base/Function.h
  base/Flags.h
  base/Gettext.h
  base/Hash.h
  base/IOStream.h
  base/IOTools.h
  base/Iterable.h
  base/Iterator.h
  base/LogControl.h
  base/LogTools.h
  base/Logger.h
  base/NonCopyable.h
  base/ProfilingFormater.h
  base/PtrTypes.h
  base/ReferenceCounted.h
  base/Regex.h
  base/String.h
  base/StringV.h
  base/Unit.h
  base/Xml.h
)

SET( zypp_base_SRCS
  base/CleanerThread.cc
  base/Exception.cc
  base/ExternalDataSource.cc
  base/Gettext.cc
  base/IOStream.cc
  base/IOTools.cc
  base/LogControl.cc
  base/ProfilingFormater.cc
  base/ReferenceCounted.cc
  base/Regex.cc
  base/String.cc
  base/StringV.cc
  base/Unit.cc
  base/Xml.cc
)

INSTALL(  FILES ${zypp_base_HEADERS} DESTINATION "${INCLUDE_INSTALL_DIR}/zypp-core/base" )


SET( zypp_fs_SRCS
  fs/PathInfo.cc
  fs/TmpPath.cc
)

SET( zypp_fs_HEADERS
  fs/PathInfo.h
  fs/TmpPath.h
)

INSTALL(  FILES ${zypp_fs_HEADERS} DESTINATION "${INCLUDE_INSTALL_DIR}/zypp-core/fs" )


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

SET( zypp_parser_xml_SRCS
  parser/xml/XmlEscape.cc
)

SET( zypp_parser_xml_HEADERS
  parser/xml/XmlEscape.h
)

INSTALL(  FILES ${zypp_parser_xml_HEADERS} DESTINATION "${INCLUDE_INSTALL_DIR}/zypp-core/parser/xml" )

SET( zyppng_base_SRCS
  zyppng/base/abstracteventsource.cc
  zyppng/base/base.cc
  zyppng/base/eventdispatcher_glib.cc
  zyppng/base/eventloop_glib.cc
  zyppng/base/linuxhelpers.cc
  zyppng/base/timer.cc
  zyppng/base/threaddata.cc
  zyppng/base/socketnotifier.cc
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

SET( zyppng_rpc_HEADERS
  zyppng/rpc/rpc.h
  zyppng/rpc/zerocopystreams.h
)

SET( zyppng_rpc_SRCS
  zyppng/rpc/rpc.cc
  zyppng/rpc/zerocopystreams.cc
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

SET ( zypp_HEADERS
  ${zypp_toplevel_headers}
  ${zypp_base_HEADERS}
  ${zypp_fs_HEADERS}
  ${zypp_url_HEADERS}
  ${zypp_parser_xml_HEADERS}
  ${zyppng_base_HEADERS}
  ${zyppng_base_private_HEADERS}
  ${zyppng_core_HEADERS}
  ${zyppng_io_HEADERS}
  ${zyppng_io_private_HEADERS}
  ${zyppng_rpc_HEADERS}
  ${zyppng_thread_HEADERS}
  ${zyppng_thread_private_HEADERS}
)

SET ( zypp_SRCS
  ${zypp_toplevel_SRCS}
  ${zypp_base_SRCS}
  ${zypp_fs_SRCS}
  ${zypp_url_SRCS}
  ${zypp_parser_xml_SRCS}
  ${zyppng_base_SRCS}
  ${zyppng_io_SRCS}
  ${zyppng_rpc_SRCS}
  ${zyppng_thread_SRCS}
)

ADD_LIBRARY( zypp-core STATIC ${zypp_SRCS} ${zypp_HEADERS} )
#we include generated headers, so we need to wait for zypp-protobuf to be ready
add_dependencies( zypp-core zypp-protobuf )

target_link_libraries( zypp-core ${LIBGLIB_LIBRARIES} )
target_link_libraries( zypp-core ${OPENSSL_LIBRARIES} )
target_link_libraries( zypp-core ${CRYPTO_LIBRARIES} )
target_link_libraries( zypp-core pthread )
target_link_libraries( zypp-core ${SIGCPP_LIBRARIES} )
