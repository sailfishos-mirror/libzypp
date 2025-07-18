# Collect all files that may contain translatable texts
FILE( GLOB_RECURSE POT_FILE_DEPENDS RELATIVE ${LIBZYPP_SOURCE_DIR} "*.h" "*.cc" )
SET( POT_FILE_DEPENDS_ZYPP ${POT_FILE_DEPENDS} PARENT_SCOPE )

ADD_DEFINITIONS( -DZYPP_DLL )
INCLUDE_DIRECTORIES( ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_BINARY_DIR}/sywu/include )
INCLUDE_DIRECTORIES( ${LIBZYPP_SOURCE_DIR} ${LIBZYPP_BINARY_DIR} ${LIBZYPP_SOURCE_DIR}/libs/zypp-core SYSTEM )


FIND_PACKAGE(Rpm REQUIRED)
IF ( NOT RPM_FOUND)
  MESSAGE( FATAL_ERROR " rpm-devel not found" )
ENDIF ( NOT RPM_FOUND)



# add_definitions( -DENABLE_SYWU )
# add_subdirectory(sywu)

IF (ENABLE_ZCHUNK_COMPRESSION)
  INCLUDE_DIRECTORIES( ${ZCHUNK_INCLUDEDIR} )
ENDIF(ENABLE_ZCHUNK_COMPRESSION)
#FILE(WRITE filename "message to write"... )

SET( zypp_SRCS
  Application.cc
  Capabilities.cc
  Capability.cc
  CapMatch.cc
  Changelog.cc
  CpeId.cc
  Dep.cc
  Digest.cc
  DiskUsageCounter.cc
  DownloadMode.cc
  Edition.cc
  PluginScript.cc
  PluginScriptException.cc
  PluginExecutor.cc
  Fetcher.cc
  FileChecker.cc
  Glob.cc
  HistoryLog.cc
  HistoryLogData.cc
  IdString.cc
  InstanceId.cc
  KeyRing.cc
  KeyRingContexts.cc
  Locks.cc
  MediaSetAccess.cc
  Package.cc
  Patch.cc
  PathInfo.cc
  Pattern.cc
  PoolItem.cc
  PoolItemBest.cc
  PoolQuery.cc
  PoolQueryResult.cc
  ProblemSolution.cc
  Product.cc
  ProvideFilePolicy.cc
  PurgeKernels.cc
  Range.cc
  Rel.cc
  RepoInfo.cc
  RepoManager.cc
  RepoManagerOptions.cc
  Repository.cc
  RepoStatus.cc
  ResKind.cc
  ResObject.cc
  Resolvable.cc
  Resolver.cc
  ResolverFocus.cc
  ResolverProblem.cc
  ResPool.cc
  ResPoolProxy.cc
  ResStatus.cc
  ServiceInfo.cc
  Signature.cc
  SrcPackage.cc
  SysContent.cc
  Target.cc
  TmpPath.cc
  VendorAttr.cc
  VendorSupportOptions.cc
  ZYpp.cc
  ZYppCallbacks.cc
  ZYppCommitPolicy.cc
  ZYppCommitResult.cc
  ZYppFactory.cc
)
SET( zypp_EARLY_SRCS
  ZConfig.cc
  Arch.cc
  Locale.cc
  CountryCode.cc
  LanguageCode.cc
)

SET( zypp_HEADERS
  APIConfig.h
  Application.h
  Arch.h
  Bit.h
  Bitmap.h
  Callback.h
  Capabilities.h
  Capability.h
  CapMatch.h
  Changelog.h
  CheckSum.h
  ContentType.h
  CountryCode.h
  CpeId.h
  Dep.h
  Digest.h
  DiskUsageCounter.h
  DownloadMode.h
  Edition.h
  PluginScript.h
  PluginScriptException.h
  PluginExecutor.h
  Fetcher.h
  FileChecker.h
  Glob.h
  HistoryLog.h
  HistoryLogData.h
  IdString.h
  IdStringType.h
  InstanceId.h
  KeyContext.h
  KeyManager.h
  KeyRing.h
  KeyRingContexts.h
  KVMap.h
  LanguageCode.h
  Locale.h
  Locks.h
  ManagedFile.h
  MediaProducts.h
  MediaSetAccess.h
  Vendor.h
  OnMediaLocation.h
  Package.h
  PackageKeyword.h
  Patch.h
  PathInfo.h
  Pattern.h
  PoolItem.h
  PoolItemBest.h
  PoolQuery.h
  PoolQueryUtil.tcc
  PoolQueryResult.h
  ProblemSolution.h
  ProblemTypes.h
  Product.h
  ProvideFilePolicy.h
  PublicKey.h
  PurgeKernels.h
  Range.h
  RelCompare.h
  Rel.h
  RepoInfo.h
  RepoManager.h
  RepoManagerFlags.h
  RepoManagerOptions.h
  Repository.h
  RepoStatus.h
  Filter.h
  ResFilters.h
  ResKind.h
  ResObject.h
  ResObjects.h
  Resolvable.h
  Resolver.h
  ResolverFocus.h
  ResolverNamespace.h
  ResolverProblem.h
  ResPool.h
  ResPoolProxy.h
  ResStatus.h
  ResTraits.h
  ServiceInfo.h
  Signature.h
  SrcPackage.h
  SysContent.h
  Target.h
  TmpPath.h
  UserData.h
  VendorAttr.h
  VendorSupportOptions.h
  ZConfig.h
  ZYppCallbacks.h
  ZYppCommit.h
  ZYppCommitPolicy.h
  ZYppCommitResult.h
  ZYppFactory.h
  ZYpp.h
)

INSTALL(  FILES ${zypp_HEADERS} DESTINATION "${INCLUDE_INSTALL_DIR}/zypp" )

SET( zypp_base_SRCS
  base/Backtrace.cc
  base/SerialNumber.cc
  base/Random.cc
  base/Measure.cc
  base/SetRelationMixin.cc
  base/StrMatcher.h
  base/StrMatcher.cc
)

SET( zypp_base_HEADERS
  base/Backtrace.h
  base/Collector.h
  base/DrunkenBishop.h
  base/SerialNumber.h
  base/Env.h
  base/Random.h
  base/Algorithm.h
  base/Counter.h
  base/Debug.h
  base/Functional.h
  base/Json.h
  base/LocaleGuard.h
  base/Measure.h
  base/NamedValue.h
  base/ReferenceCounted.h
  base/SetRelationMixin.h
  base/SetTracker.h
  base/Signal.h
  base/StrMatcher.h
  base/TypeTraits.h
  base/ValueTransform.h
)

INSTALL(  FILES
  ${zypp_base_HEADERS}
  DESTINATION ${INCLUDE_INSTALL_DIR}/zypp/base
)


SET( zypp_media_detail_HEADERS
  media/detail/DownloadProgressTracker.h
  media/detail/MediaNetworkRequestExecutor.h
  media/detail/OptionalDownloadProgressReport.h
)

SET( zypp_media_SRCS
  media/detail/DownloadProgressTracker.cc
  media/detail/MediaNetworkRequestExecutor.cc
  media/detail/OptionalDownloadProgressReport.cc
  media/MediaHandler.cc
  media/MediaHandlerFactory.cc
  media/MediaNFS.cc
  media/MediaCD.cc
  media/MediaDIR.cc
  media/MediaDISK.cc
  media/MediaCIFS.cc
  media/MediaNetworkCommonHandler.cc
  media/MediaCurl.cc
  media/MediaCurl2.cc
  media/MediaISO.cc
  media/MediaPlugin.cc
  media/MediaSource.cc
  media/MediaManager.cc
  media/MediaPriority.cc
  media/UrlResolverPlugin.cc
)

SET( zypp_media_HEADERS
  media/MediaCD.h
  media/MediaCIFS.h
  media/MediaCurl.h
  media/MediaCurl2.h
  media/MediaNetworkCommonHandler.h
  media/MediaDIR.h
  media/MediaDISK.h
  media/MediaException.h
  media/MediaHandler.h
  media/MediaHandlerFactory.h
  media/MediaISO.h
  media/MediaPlugin.h
  media/MediaManager.h
  media/MediaNFS.h
  media/MediaSource.h
  media/MediaPriority.h
  media/UrlResolverPlugin.h
)

INSTALL(  FILES
  ${zypp_media_HEADERS}
  DESTINATION ${INCLUDE_INSTALL_DIR}/zypp/media
)

SET( zypp_ng_SRCS
  ng/context.cc
  ng/progressobserveradaptor.cc
  ng/reporthelper.cc
  ng/repomanager.cc
  ng/userrequest.cc
  ng/repo/downloader.cc
  ng/repo/refresh.cc
  ng/repo/workflows/plaindir.cc
  ng/repo/workflows/repodownloaderwf.cc
  ng/repo/workflows/repomanagerwf.cc
  ng/repo/workflows/rpmmd.cc
  ng/repo/workflows/serviceswf.cc
  ng/repo/workflows/susetags.cc
  ng/workflows/checksumwf.cc
  ng/workflows/contextfacade.cc
  ng/workflows/downloadwf.cc
  ng/workflows/keyringwf.cc
  ng/workflows/mediafacade.cc
  ng/workflows/repoinfowf.cc
  ng/workflows/signaturecheckwf.cc
)

SET( zypp_ng_HEADERS
  ng/context.h
  ng/Context
  ng/progressobserveradaptor.h
  ng/reporthelper.h
  ng/repomanager.h
  ng/RepoManager
  ng/userrequest.h
  ng/UserRequest
  ng/repo/downloader.h
  ng/repo/Downloader
  ng/repo/refresh.h
  ng/repo/Refresh
  ng/repo/workflows/plaindir.h
  ng/repo/workflows/repodownloaderwf.h
  ng/repo/workflows/repomanagerwf.h
  ng/repo/workflows/rpmmd.h
  ng/repo/workflows/serviceswf.h
  ng/repo/workflows/susetags.h
  ng/workflows/checksumwf.h
  ng/workflows/contextfacade.h
  ng/workflows/downloadwf.h
  ng/workflows/keyringwf.h
  ng/workflows/mediafacade.h
  ng/workflows/logichelpers.h
  ng/workflows/repoinfowf.h
  ng/workflows/signaturecheckwf.h
)

SET( zypp_ng_private_HEADERS
  ng/private/context_p.h
  ng/private/repomanager_p.h
)

#INSTALL( FILES
#  ${zypp_ng_HEADERS}
#  DESTINATION ${INCLUDE_INSTALL_DIR}/zypp/repo
#)


SET( zypp_parser_SRCS
  parser/HistoryLogReader.cc
  parser/RepoFileReader.cc
  parser/RepoindexFileReader.cc
  parser/ServiceFileReader.cc
  parser/ProductFileReader.cc
)

SET( zypp_parser_HEADERS
  parser/HistoryLogReader.h
  parser/RepoFileReader.h
  parser/RepoindexFileReader.h
  parser/ServiceFileReader.h
  parser/ProductFileReader.h
)

INSTALL(  FILES
  ${zypp_parser_HEADERS}
  DESTINATION ${INCLUDE_INSTALL_DIR}/zypp/parser
)

SET( zypp_parser_susetags_SRCS
  parser/susetags/RepoIndex.cc
  parser/susetags/ContentFileReader.cc
)

SET( zypp_parser_susetags_HEADERS
  parser/susetags/RepoIndex.h
  parser/susetags/ContentFileReader.h
)

INSTALL(  FILES
  ${zypp_parser_susetags_HEADERS}
  DESTINATION ${INCLUDE_INSTALL_DIR}/zypp/parser/susetags
)

SET( zypp_parser_xml_SRCS
  parser/xml/Node.cc
  parser/xml/ParseDef.cc
  parser/xml/ParseDefConsume.cc
  parser/xml/ParseDefException.cc
  parser/xml/Reader.cc
  parser/xml/XmlString.cc
  parser/xml/libxmlfwd.cc
)

SET( zypp_parser_xml_HEADERS
  parser/xml/Parse.h
  parser/xml/Node.h
  parser/xml/ParseDef.h
  parser/xml/ParseDefConsume.h
  parser/xml/ParseDefException.h
  parser/xml/ParseDefTraits.h
  parser/xml/Reader.h
  parser/xml/XmlString.h
  parser/xml/libxmlfwd.h
)

INSTALL(  FILES
  ${zypp_parser_xml_HEADERS}
  DESTINATION ${INCLUDE_INSTALL_DIR}/zypp/parser/xml
)

SET( zypp_parser_yum_SRCS
  parser/yum/RepomdFileReader.cc
)

SET( zypp_parser_yum_HEADERS
  parser/yum/RepomdFileReader.h
)

INSTALL(  FILES
  ${zypp_parser_yum_HEADERS}
  DESTINATION ${INCLUDE_INSTALL_DIR}/zypp/parser/yum
)

SET( zypp_pool_SRCS
  pool/PoolImpl.cc
  pool/PoolStats.cc
)

SET( zypp_pool_HEADERS
  pool/PoolImpl.h
  pool/PoolStats.h
  pool/PoolTraits.h
  pool/ByIdent.h
)

INSTALL(  FILES
  ${zypp_pool_HEADERS}
  DESTINATION ${INCLUDE_INSTALL_DIR}/zypp/pool
)

# SET( zypp_solver_detail_SRCS )

SET( zypp_solver_HEADERS
  solver/Types.h
)

INSTALL(  FILES
  ${zypp_solver_HEADERS}
  DESTINATION ${INCLUDE_INSTALL_DIR}/zypp/solver
)

SET( zypp_solver_detail_SRCS
  solver/detail/ProblemSolutionIgnore.cc
  solver/detail/ProblemSolutionCombi.cc
  solver/detail/Resolver.cc
  solver/detail/SolutionAction.cc
  solver/detail/Testcase.cc
  solver/detail/SolverQueueItem.cc
  solver/detail/SolverQueueItemInstall.cc
  solver/detail/SolverQueueItemDelete.cc
  solver/detail/SolverQueueItemUpdate.cc
  solver/detail/SolverQueueItemInstallOneOf.cc
  solver/detail/SolverQueueItemLock.cc
  solver/detail/SATResolver.cc
  solver/detail/SystemCheck.cc
)

SET( zypp_solver_detail_HEADERS
  solver/detail/ProblemSolutionIgnore.h
  solver/detail/ProblemSolutionCombi.h
  solver/detail/Resolver.h
  solver/detail/SolutionAction.h
  solver/detail/Testcase.h
  solver/detail/Types.h
  solver/detail/SolverQueueItem.h
  solver/detail/SolverQueueItemInstall.h
  solver/detail/SolverQueueItemDelete.h
  solver/detail/SolverQueueItemUpdate.h
  solver/detail/SolverQueueItemInstallOneOf.h
  solver/detail/SolverQueueItemLock.h
  solver/detail/ItemCapKind.h
  solver/detail/SATResolver.h
  solver/detail/SystemCheck.h
)

INSTALL(  FILES
  ${zypp_solver_detail_HEADERS}
  DESTINATION ${INCLUDE_INSTALL_DIR}/zypp/solver/detail
)

SET( zypp_sat_SRCS
  sat/Pool.cc
  sat/Solvable.cc
  sat/SolvableSet.cc
  sat/SolvableSpec.cc
  sat/SolvIterMixin.cc
  sat/Map.cc
  sat/Queue.cc
  sat/FileConflicts.cc
  sat/Transaction.cc
  sat/WhatProvides.cc
  sat/WhatObsoletes.cc
  sat/LocaleSupport.cc
  sat/LookupAttr.cc
  sat/SolvAttr.cc
)

SET( zypp_sat_HEADERS
  sat/Pool.h
  sat/Solvable.h
  sat/SolvableSet.h
  sat/SolvableType.h
  sat/SolvableSpec.h
  sat/SolvIterMixin.h
  sat/Map.h
  sat/Queue.h
  sat/FileConflicts.h
  sat/Transaction.h
  sat/WhatProvides.h
  sat/WhatObsoletes.h
  sat/LocaleSupport.h
  sat/LookupAttr.h
  sat/LookupAttrTools.h
  sat/SolvAttr.h
)

INSTALL(  FILES
  ${zypp_sat_HEADERS}
  DESTINATION ${INCLUDE_INSTALL_DIR}/zypp/sat
)

SET( zypp_sat_detail_SRCS
  sat/detail/PoolImpl.cc
)

SET( zypp_sat_detail_HEADERS
  sat/detail/PoolMember.h
  sat/detail/PoolImpl.h
)

INSTALL(  FILES
  ${zypp_sat_detail_HEADERS}
  DESTINATION ${INCLUDE_INSTALL_DIR}/zypp/sat/detail
)


SET( zypp_target_SRCS
  target/RpmPostTransCollector.cc
  target/RequestedLocalesFile.cc
  target/SolvIdentFile.cc
  target/HardLocksFile.cc
  target/commitpackagepreloader.cc
  target/CommitPackageCache.cc
  target/CommitPackageCacheImpl.cc
  target/CommitPackageCacheReadAhead.cc
  target/TargetCallbackReceiver.cc
  target/TargetException.cc
  target/TargetImpl.cc
  target/TargetImpl.commitFindFileConflicts.cc

)

SET( zypp_target_HEADERS
  target/RpmPostTransCollector.h
  target/RequestedLocalesFile.h
  target/SolvIdentFile.h
  target/HardLocksFile.h
  target/CommitPackageCache.h
  target/CommitPackageCacheImpl.h
  target/CommitPackageCacheReadAhead.h
  target/TargetCallbackReceiver.h
  target/TargetException.h
  target/TargetImpl.h
)


SET( zypp_target_detail_HEADERS
  target/private/commitpackagepreloader_p.h
)

INSTALL(  FILES
  ${zypp_target_HEADERS}
  DESTINATION ${INCLUDE_INSTALL_DIR}/zypp/target
)

SET( zypp_target_modalias_SRCS
  target/modalias/Modalias.cc
)

SET( zypp_target_modalias_HEADERS
  target/modalias/Modalias.h
)

INSTALL(  FILES
  ${zypp_target_modalias_HEADERS}
  DESTINATION ${INCLUDE_INSTALL_DIR}/zypp/target/modalias
)

SET( zypp_target_rpm_SRCS
  target/rpm/BinHeader.cc
  target/rpm/RpmCallbacks.cc
  target/rpm/RpmDb.cc
  target/rpm/RpmException.cc
  target/rpm/RpmHeader.cc
  target/rpm/librpmDb.cc
)

SET( zypp_target_rpm_HEADERS
  target/rpm/BinHeader.h
  target/rpm/RpmCallbacks.h
  target/rpm/RpmFlags.h
  target/rpm/RpmDb.h
  target/rpm/RpmException.h
  target/rpm/RpmHeader.h
  target/rpm/librpm.h
  target/rpm/librpmDb.h
)

INSTALL(  FILES
  ${zypp_target_rpm_HEADERS}
  DESTINATION ${INCLUDE_INSTALL_DIR}/zypp/target/rpm
)

SET( zypp_ui_SRCS
  ui/Selectable.cc
  ui/SelectableImpl.cc
  ui/Status.cc
  ui/UserWantedPackages.cc
)

SET( zypp_ui_HEADERS
  ui/SelFilters.h
  ui/Selectable.h
  ui/SelectableImpl.h
  ui/SelectableTraits.h
  ui/Status.h
  ui/UserWantedPackages.h
)

INSTALL(  FILES
  ${zypp_ui_HEADERS}
  DESTINATION ${INCLUDE_INSTALL_DIR}/zypp/ui
)

INSTALL(  FILES
  ${zypp_url_HEADERS}
  DESTINATION ${INCLUDE_INSTALL_DIR}/zypp/url
)

SET( zypp_zypp_detail_SRCS
  zypp_detail/ZYppImpl.cc
)

SET( zypp_zypp_detail_HEADERS
  zypp_detail/keyring_p.h
  zypp_detail/urlcredentialextractor_p.h
  zypp_detail/ZYppImpl.h
  zypp_detail/ZYppReadOnlyHack.h
)

INSTALL(  FILES
  ${zypp_zypp_detail_HEADERS}
  DESTINATION ${INCLUDE_INSTALL_DIR}/zypp/zypp_detail
)

SET( zypp_repo_SRCS
  repo/RepoException.cc
  repo/RepoMirrorList.cc
  repo/RepoType.cc
  repo/ServiceType.cc
  repo/PackageProvider.cc
  repo/SrcPackageProvider.cc
  repo/RepoProvideFile.cc
  repo/DeltaCandidates.cc
  repo/Applydeltarpm.cc
  repo/PackageDelta.cc
  repo/SUSEMediaVerifier.cc
  repo/MediaInfoDownloader.cc
  repo/RepoVariables.cc
  repo/RepoInfoBase.cc
  repo/PluginRepoverification.cc
  repo/PluginServices.cc
)

SET( zypp_repo_HEADERS
  repo/RepoException.h
  repo/RepoMirrorList.h
  repo/RepoType.h
  repo/ServiceType.h
  repo/PackageProvider.h
  repo/SrcPackageProvider.h
  repo/RepoProvideFile.h
  repo/DeltaCandidates.h
  repo/Applydeltarpm.h
  repo/PackageDelta.h
  repo/SUSEMediaVerifier.h
  repo/MediaInfoDownloader.h
  repo/RepoVariables.h
  repo/RepoInfoBase.h
  repo/PluginRepoverification.h
  repo/PluginServices.h
)

INSTALL( FILES
  ${zypp_repo_HEADERS}
  DESTINATION ${INCLUDE_INSTALL_DIR}/zypp/repo
)

SET( zypp_repo_yum_SRCS
  repo/yum/RepomdFileCollector.cc
)

SET( zypp_repo_yum_HEADERS
  repo/yum/RepomdFileCollector.h
)

SET( zypp_repo_susetags_SRCS
)

SET( zypp_repo_susetags_HEADERS
)


####################################################################

SET( zypp_misc_HEADERS
  Misc.h
  misc/DefaultLoadSystem.h
  misc/CheckAccessDeleted.h
  misc/TestcaseSetup.h
  misc/LoadTestcase.h
)

SET( zypp_misc_SRCS
  misc/DefaultLoadSystem.cc
  misc/CheckAccessDeleted.cc
  misc/TestcaseSetup.cc
  misc/LoadTestcase.cc
)

INSTALL( FILES
  ${zypp_misc_HEADERS}
  DESTINATION ${INCLUDE_INSTALL_DIR}/zypp/misc
)

####################################################################


SET ( zypp_core_compat_HEADERS
  ByteCount.h
  Url.h
  Globals.h
  ExternalProgram.h
  Date.h
  AutoDispose.h
  TriBool.h
  Pathname.h
  ByteArray.h
  ProgressData.h
)

INSTALL( FILES
  ${zypp_core_compat_HEADERS}
  DESTINATION ${INCLUDE_INSTALL_DIR}/zypp
)

SET ( zypp_core_base_compat_HEADERS
  base/Xml.h
  base/Function.h
  base/Flags.h
  base/StringV.h
  base/Iterator.h
  base/NonCopyable.h
  base/Exception.h
  base/Unit.h
  base/String.h
  base/Regex.h
  base/IOTools.h
  base/IOStream.h
  base/Gettext.h
  base/EnumClass.h
  base/Logger.h
  base/Easy.h
  base/ProfilingFormater.h
  base/ExternalDataSource.h
  base/LogTools.h
  base/LogControl.h
  base/Hash.h
  base/PtrTypes.h
  base/Errno.h
  base/Iterable.h
  base/DefaultIntegral.h
  base/DtorReset.h
  base/GzStream.h
  base/InputStream.h
  base/ProvideNumericId.h
  base/SimpleStreambuf.h
  base/Sysconfig.h
  base/UserRequestException.h
  base/WatchFile.h
  base/fXstream.h
)

IF (ENABLE_ZCHUNK_COMPRESSION)
list( APPEND zypp_core_base_compat_HEADERS
  base/ZckStream.h
)
ENDIF(ENABLE_ZCHUNK_COMPRESSION)

INSTALL( FILES
  ${zypp_core_base_compat_HEADERS}
  DESTINATION ${INCLUDE_INSTALL_DIR}/zypp/base
)

SET ( zypp_core_url_compat_HEADERS
  url/UrlException.h
  url/UrlUtils.h
  url/UrlBase.h
)

INSTALL( FILES
  ${zypp_core_url_compat_HEADERS}
  DESTINATION ${INCLUDE_INSTALL_DIR}/zypp/url
)


SET ( zypp_media_compat_HEADERS
  media/CredentialFileReader.h
  media/CredentialManager.h
  media/MediaBlockList.h
  media/MediaUserAuth.h
  media/MetaLinkParser.h
  media/ProxyInfo.h
  media/TransferSettings.h
  media/Mount.h
  media/CurlConfig.h
)

INSTALL( FILES
  ${zypp_media_compat_HEADERS}
  DESTINATION ${INCLUDE_INSTALL_DIR}/zypp/media
)

SET ( zypp_core_parser_xml_compat_HEADERS
  parser/xml/XmlEscape.h
)

INSTALL( FILES
  ${zypp_core_parser_xml_compat_HEADERS}
  DESTINATION ${INCLUDE_INSTALL_DIR}/zypp/parser/xml
)


SET ( zypp_core_parser_compat_HEADERS
  parser/IniDict.h
  parser/IniParser.h
  parser/ParseException.h
)

INSTALL( FILES
  ${zypp_core_parser_compat_HEADERS}
  DESTINATION ${INCLUDE_INSTALL_DIR}/zypp/parser
)

IF ( LIBPROXY_FOUND )
  SET(zypp_media_proxyinfo_libproxy_HEADERS media/proxyinfo/ProxyInfoLibproxy.h)
ENDIF( LIBPROXY_FOUND )

SET( zypp_media_compat_proxyinfo_HEADERS
  media/proxyinfo/ProxyInfoImpl.h
  media/proxyinfo/ProxyInfoSysconfig.h
  ${zypp_media_proxyinfo_libproxy_HEADERS}
  media/proxyinfo/ProxyInfos.h
)

INSTALL(  FILES
  ${zypp_media_compat_proxyinfo_HEADERS}
  DESTINATION ${INCLUDE_INSTALL_DIR}/zypp/media/proxyinfo
)

SET( zypp_lib_SRCS
${zypp_misc_SRCS}
${zypp_pool_SRCS}
${zypp_parser_susetags_SRCS}
${zypp_parser_xml_SRCS}
${zypp_parser_yum_SRCS}
${zypp_parser_SRCS}
${zypp_media_proxyinfo_SRCS}
${zypp_media_SRCS}
${zypp_ng_SRCS}
${zypp_url_SRCS}
${zypp_repo_SRCS}
${zypp_repo_yum_SRCS}
${zypp_repo_susetags_SRCS}
${zypp_repo_data_SRCS}
${zypp_target_rpm_SRCS}
${zypp_target_hal_SRCS}
${zypp_target_modalias_SRCS}
${zypp_target_SRCS}
${zypp_solver_detail_SRCS}
${zypp_ui_SRCS}
${zypp_SRCS}
${zypp_zypp_detail_SRCS}
${zypp_sat_SRCS}
${zypp_sat_detail_SRCS}
${zypp_EARLY_SRCS}
${zypp_base_SRCS}
)

SET( zypp_lib_HEADERS
${zypp_target_rpm_HEADERS}
${zypp_parser_tagfile_HEADERS}
${zypp_parser_susetags_HEADERS}
${zypp_parser_yum_HEADERS}
${zypp_parser_xml_HEADERS}
${zypp_parser_HEADERS}
${zypp_ui_HEADERS}
${zypp_media_HEADERS}
${zypp_media_detail_HEADERS}
${zypp_media_compat_proxyinfo_HEADERS}
${zypp_ng_HEADERS}
${zypp_ng_private_HEADERS}
${zypp_base_HEADERS}
${zypp_solver_HEADERS}
${zypp_solver_detail_HEADERS}
${zypp_sat_HEADERS}
${zypp_sat_detail_HEADERS}
${zypp_url_HEADERS}
${zypp_HEADERS}
${zypp_zypp_detail_HEADERS}
${zypp_repo_HEADERS}
${zypp_source_susetags_HEADERS}
${zypp_target_modalias_HEADERS}
${zypp_target_HEADERS}
${zypp_target_detail_HEADERS}
${zypp_pool_HEADERS}
${zypp_misc_HEADERS}
${zypp_core_compat_HEADERS}
${zypp_core_base_compat_HEADERS}
${zypp_core_url_compat_HEADERS}
${zypp_core_parser_compat_HEADERS}
${zypp_media_compat_HEADERS}
)

# Default loggroup for all files
SET_LOGGROUP( "zypp" ${zypp_lib_SRCS} )

# override some defaults
SET_LOGGROUP( "libsolv" ${zypp_sat_SRCS} )

if ( ZYPP_CXX_CLANG_TIDY OR ZYPP_CXX_CPPCHECK )

  message(NOTICE "Enabling static analysis: ${ZYPP_CXX_CLANG_TIDY} ${ZYPP_CXX_CPPCHECK}")

  set_source_files_properties(
      target/TargetImpl.cc
      PROPERTIES
      SKIP_LINTING ON
  )

  if (ZYPP_CXX_CLANG_TIDY)
    set( CMAKE_CXX_CLANG_TIDY ${ZYPP_CXX_CLANG_TIDY} )
  endif(ZYPP_CXX_CLANG_TIDY)

  if (ZYPP_CXX_CPPCHECK)
    set(CMAKE_CXX_CPPCHECK ${ZYPP_CXX_CPPCHECK})
  endif(ZYPP_CXX_CPPCHECK)

endif( ZYPP_CXX_CLANG_TIDY OR ZYPP_CXX_CPPCHECK )

# due to limitations in cmake object libraries feature( object files are not derived transiently, they are only added to the target if they are added DIRECTLY)
# we have this function that adds all zypp objects to a specified target.
function(add_zypp_lib target shared_or_static )

  set ( _link_mode PRIVATE )
  if ( ${shared_or_static} STREQUAL STATIC )
    message(NOTICE "Building ${target} static")
    set ( _link_mode INTERFACE )
  else()
    message(NOTICE "Building ${target} shared")
  endif()

  ADD_LIBRARY( ${target} ${shared_or_static} )

  SET_TARGET_PROPERTIES ( ${target} PROPERTIES VERSION "${LIBZYPP_VERSION_INFO}" )
  SET_TARGET_PROPERTIES ( ${target} PROPERTIES SOVERSION "${LIBZYPP_SOVERSION_INFO}" )

  target_link_libraries( ${target} PRIVATE zypp_lib_compiler_flags )
  TARGET_LINK_LIBRARIES( ${target} PRIVATE zypp-objlib )
  target_link_libraries( ${target} PRIVATE commit-proto-obj )

  # CAUTION, link those libs always first and always in that order,
  # otherwise you get symbol lookup errors when linking against libzypp
  TARGET_LINK_LIBRARIES( ${target} ${_link_mode} zypp-common )
  TARGET_LINK_LIBRARIES( ${target} ${_link_mode} zypp-curl )
  TARGET_LINK_LIBRARIES( ${target} ${_link_mode} zypp-media )


  # CAUTION, link zypp-tui.a at last, to prevent undefined references
  # in case tui functions are used in libzypp.
  TARGET_LINK_LIBRARIES( ${target} ${_link_mode} zypp-tui )

  TARGET_LINK_LIBRARIES( ${target} ${_link_mode} zypp-core )

  TARGET_LINK_LIBRARIES( ${target} ${_link_mode} ${LibSolv_LIBRARIES} )
  TARGET_LINK_LIBRARIES( ${target} ${_link_mode} ${RPM_LIBRARY} ${RPMIO_LIBRARY} )
  TARGET_LINK_LIBRARIES( ${target} ${_link_mode} ${GETTEXT_LIBRARIES} )
  TARGET_LINK_LIBRARIES( ${target} ${_link_mode} ${Boost_THREAD_LIBRARY})
  target_link_libraries( ${target} ${_link_mode} ${YAML_CPP_LIBRARIES})

  IF ( UDEV_FOUND )
    TARGET_LINK_LIBRARIES( ${target} ${_link_mode} ${UDEV_LIBRARY} )
  ELSE ( UDEV_FOUND )
    IF ( HAL_FOUND )
      TARGET_LINK_LIBRARIES( ${target} ${_link_mode} ${HAL_LIBRARY} ${HAL_STORAGE_LIBRARY} ${DBUS_LIBRARY} )
    ENDIF ( HAL_FOUND )
  ENDIF ( UDEV_FOUND )
endfunction()


ADD_LIBRARY( zypp-objlib OBJECT ${zypp_lib_SRCS} ${zypp_lib_HEADERS}  )

target_link_libraries( zypp-objlib PRIVATE zypp_lib_compiler_flags )

target_include_directories( zypp-objlib PUBLIC ${RPM_INCLUDE_DIR})

# fix includes not relative to rpm
target_include_directories( zypp-objlib PUBLIC ${RPM_INCLUDE_DIR}/rpm)

# rpm verify function and callback states were introduced in rpm-4.15
if( RPM_LIB_VER VERSION_GREATER_EQUAL "4.15.0"  AND  RPM_LIB_VER VERSION_LESS "5.0.0")
  target_compile_definitions( zypp-objlib PUBLIC HAVE_RPM_VERIFY_TRANSACTION_STEP )
endif()

if( RPM_LIB_VER VERSION_GREATER_EQUAL "5.0.0" )
      MESSAGE( STATUS "rpm found: enable rpm-4 compat interface." )
      target_compile_definitions( zypp-objlib PUBLIC _RPM_5)
endif ()

add_zypp_lib( zypp SHARED )

set_target_properties ( zypp PROPERTIES LINK_FLAGS "-Wl,--version-script=${CMAKE_CURRENT_SOURCE_DIR}/libzypp.map" )
INSTALL(TARGETS zypp LIBRARY DESTINATION ${LIB_INSTALL_DIR} )

# library with more symbols for tests etc
add_zypp_lib( zypp-allsym STATIC )

#this part does nothing yet, only when switching away from using a map file to
#a approach where symbols are exported explicitly it would make a difference
#ZYPPNG symbols need to be added to the map file to be exported
IF( EXPORT_NG_API )
ADD_DEFINITIONS(-DEXPORT_EXPERIMENTAL_API)
INCLUDE(GenerateExportHeader)
message("Exporting experimental libzypp-ng API")
GENERATE_EXPORT_HEADER(
  zypp
  BASE_NAME zypp-ng
  PREFIX_NAME LIB
)
ENDIF ( EXPORT_NG_API )

# install XML schemas
FILE( GLOB YUM_SCHEMA_FILES ${LIBZYPP_SOURCE_DIR}/zypp/parser/yum/schema/*.rng )
INSTALL(FILES ${YUM_SCHEMA_FILES} DESTINATION "${CMAKE_INSTALL_PREFIX}/share/zypp/schema/yum" )

ADD_CUSTOM_TARGET ( buildmon
  COMMAND test ! -x ./buildmon || ./buildmon
)

ADD_CUSTOM_COMMAND (
  TARGET zypp POST_BUILD
  COMMAND test ! -x ./buildmon || ./buildmon
)
