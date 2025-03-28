/*---------------------------------------------------------------------\
|                          ____ _   __ __ ___                          |
|                         |__  / \ / / . \ . \                         |
|                           / / \ V /|  _/  _/                         |
|                          / /__ | | | | | |                           |
|                         /_____||_| |_| |_|                           |
|                                                                      |
----------------------------------------------------------------------*/

#include <downloader/private/downloader_p.h>
#include <zypp-curl/ng/network/private/mediadebug_p.h>
#include <zypp-curl/ng/network/private/networkrequesterror_p.h>
#include <zypp-curl/private/curlhelper_p.h>
#include <zypp-core/AutoDispose.h>
#include <zypp-core/fs/PathInfo.h>

#include "rangedownloader_p.h"

namespace zyppng {

  void RangeDownloaderBaseState::onRequestStarted( NetworkRequest & )
  { }

  void RangeDownloaderBaseState::onRequestProgress( NetworkRequest &, off_t , off_t, off_t , off_t  )
  {
    off_t dlnowMulti = _downloadedMultiByteCount;
    for( const auto &req : _runningRequests ) {
      dlnowMulti += req->downloadedByteCount();
    }

    if ( !assertExpectedFilesize( dlnowMulti ) )
      return;

    stateMachine()._sigProgress.emit( *stateMachine().z_func(), _fileSize, dlnowMulti );
  }

  void RangeDownloaderBaseState::onRequestFinished( NetworkRequest &req, const zyppng::NetworkRequestError &err )
  {
    auto lck = stateMachine().z_func()->shared_from_this();
    auto it = std::find_if( _runningRequests.begin(), _runningRequests.end(), [ &req ]( const std::shared_ptr<Request> &r ) {
      return ( r.get() == &req );
    });
    if ( it == _runningRequests.end() )
      return;

    auto reqLocked = *it;

    //remove from running
    _runningRequests.erase( it );

    //feed the working URL back into the mirrors in case there are still running requests that might fail
    // @TODO , finishing the transfer might never be called in case of cancelling the request, need a better way to track running transfers
    if ( reqLocked->_myMirror )
      reqLocked->_myMirror->finishTransfer( !err.isError() );

    if ( err.isError() ) {
      return handleRequestError( reqLocked, err );
    }

    _downloadedMultiByteCount += req.downloadedByteCount();
    if ( !assertExpectedFilesize( _downloadedMultiByteCount ) ) {
      return;
    }

    MIL  << req.nativeHandle() << " " << "Request finished successfully."<<std::endl;
    const auto &rngs = reqLocked->requestedRanges();
    std::for_each( rngs.begin(), rngs.end(), [&req]( const auto &b ){ DBG_MEDIA  << req.nativeHandle() << " " << "-> Block " << b._start << " finished." << std::endl; } );

    auto restartReqWithBlock = [ this ]( std::shared_ptr<Request> &req, std::vector<Block> &&blocks ) {
      MIL  << req->nativeHandle() << " " << "Reusing Request to download blocks:"<<std::endl;
      if ( !addBlockRanges( req, std::move( blocks ) ) )
        return false;

      //this is not a new request, only add to queues but do not connect signals again
      addNewRequest( req, false );
      return true;
    };

    //check if we already have enqueued all blocks if not reuse the request
    if ( _ranges.size() ) {
      MIL  << req.nativeHandle() << " " << "Reusing to download blocks: "<<std::endl;
      if ( !restartReqWithBlock( reqLocked, getNextBlocks( reqLocked->url().getScheme() ) ) ) {
        return setFailed( "Failed to restart request with new blocks." );
      }
      return;

    } else {
      //if we have failed blocks, try to download them with this mirror
      if ( !_failedRanges.empty() ) {

        auto fblks = getNextFailedBlocks( reqLocked->url().getScheme() );
        MIL  << req.nativeHandle() << " " << "Reusing to download failed blocks: "<<std::endl;
        if ( !restartReqWithBlock( reqLocked, std::move(fblks) ) ) {
          return setFailed( "Failed to restart request with previously failed blocks." );
        }
        return;
      }
    }

    //feed the working URL back into the mirrors in case there are still running requests that might fail
    _fileMirrors.push_back( reqLocked->_originalUrl );

    // make sure downloads are running, at this point
    ensureDownloadsRunning();
  }

  void RangeDownloaderBaseState::handleRequestError( std::shared_ptr<Request> req, const zyppng::NetworkRequestError &err )
  {
    bool retry = false;
    auto &parent = stateMachine();


    //Handle the auth errors explicitly, we need to give the user a way to put in new credentials
    //if we get valid new credentials we can retry the request
    if ( err.type() == NetworkRequestError::Unauthorized || err.type() == NetworkRequestError::AuthFailed ) {
      retry = parent.handleRequestAuthError( req, err );
    } else {

      //if a error happens during a multi download we try to use another mirror to download the failed block
      MIL << req->nativeHandle() << " " << "Request failed " << req->extendedErrorString() << "(" << req->url() << ")" << std::endl;
      if ( req->lastRedirectInfo ().size () )
        MIL << req->nativeHandle() << " Last redirection target was: " << req->lastRedirectInfo () << std::endl;

      NetworkRequestError dummyErr;

      const auto &fRanges = req->failedRanges();
      try {
        std::transform( fRanges.begin(), fRanges.end(), std::back_inserter(_failedRanges), [ &req ]( const auto &r ){
          Block b = std::any_cast<Block>(r.userData);;
          b._failedWithErr = req->error();
          if ( zypp::env::ZYPP_MEDIA_CURL_DEBUG() > 3 )
            DBG_MEDIA << "Adding failed block to failed blocklist: " << b._start << " " << b._len << " (" << req->error().toString() << " [" << req->error().nativeErrorString()<< "])" << std::endl;
          return b;
        });

        // try to fill the open spot right away
        ensureDownloadsRunning();
        return;

      } catch ( const zypp::Exception &ex ) {
        //we just log the exception and fall back to a normal download
        WAR << "Multipart download failed: " << ex.asString() << std::endl;
      }
    }

    //if rety is true we just enqueue the request again, usually this means authentication was updated
    if ( retry ) {
      //make sure this request will run asap
      req->setPriority( parent._defaultSubRequestPriority );

      //this is not a new request, only add to queues but do not connect signals again
      addNewRequest( req, false );
      return;
    }

    //we do not have more mirrors left we can try
    cancelAll ( err );

    // not all hope is lost, maybe a normal download can work out?
    // fall back to normal download
    _sigFailed.emit();
  }

  void RangeDownloaderBaseState::ensureDownloadsRunning()
  {
    if ( _inEnsureDownloadsRunning )
      return;

    zypp::OnScopeExit clearFlag( [this]() {
      _inEnsureDownloadsRunning = false;
    });

    _inEnsureDownloadsRunning = true;

    //check if there is still work to do
    while ( _ranges.size() || _failedRanges.size() ) {

      // download was already finished
      if ( _error.isError() )
        return;

      if ( _runningRequests.size() >= 10 )
        break;

      // prepareNextMirror will automatically call mirrorReceived() once there is a mirror ready
      const auto &res = prepareNextMirror();
      // if mirrors are delayed we stop here, once the mirrors are ready we get called again
      if ( res == MirrorHandlingStateBase::Delayed )
        return;
      else if ( res == MirrorHandlingStateBase::Failed ) {
        failedToPrepare();
        return;
      }
    }

    // check if we are done at this point
    if ( _runningRequests.empty() ) {

      if ( _failedRanges.size() || _ranges.size() ) {
        setFailed( NetworkRequestErrorPrivate::customError( NetworkRequestError::InternalError, "Unable to download all blocks." ) );
        return;
      }

      // seems we were successfull , transition to finished state
      setFinished();
    }
  }

  void RangeDownloaderBaseState::mirrorReceived( MirrorControl::MirrorPick mirror )
  {

    auto &parent = stateMachine();
    Url myUrl;
    TransferSettings settings;

    auto err = setupMirror( mirror, myUrl, settings );
    if ( err.isError() ) {
      WAR << "Failure to setup mirror " << myUrl << " with error " << err.toString() << "("<< err.nativeErrorString() << "), dropping it from the list of mirrors." << std::endl;
      // if a mirror fails , we remove it from our list
      _fileMirrors.erase( mirror.first );

      // make sure this is retried
      ensureDownloadsRunning();
      return;
    }

    auto blocks = getNextBlocks( myUrl.getScheme() );
    if ( !blocks.size() )
      blocks = getNextFailedBlocks( myUrl.getScheme() );

    if ( !blocks.size() ) {
      // We have no blocks. In theory, that should never happen, but for safety, we error out here. It is better than
      // getting stuck.
      setFailed( NetworkRequestErrorPrivate::customError( NetworkRequestError::InternalError, "Mirror requested after all blocks were downloaded." ) );
      return;
    }

    const auto &spec = parent._spec;

    std::shared_ptr<Request> req = std::make_shared<Request>( ::internal::clearQueryString( myUrl ), spec.targetPath(), NetworkRequest::WriteShared );
    req->_myMirror = mirror.second;
    req->_originalUrl = myUrl;
    req->setPriority( parent._defaultSubRequestPriority );
    req->transferSettings() = settings;

    // if we download chunks we do not want to wait for too long on mirrors that have slow activity
    // note: this sets the activity timeout, not the download timeout
    req->transferSettings().setTimeout( 2 );

    MIL << "Creating Request to download blocks via mirror: "  << myUrl << std::endl;
    if ( !addBlockRanges( req, std::move(blocks) ) ) {
      setFailed( NetworkRequestErrorPrivate::customError( NetworkRequestError::InternalError, "Failed to add blocks to request." ) );
      return;
    }

    // we just use a mirror once per file, remove it from the list
    _fileMirrors.erase( mirror.first );

    addNewRequest( req );

    // trigger next downloads
    ensureDownloadsRunning();
  }

  void RangeDownloaderBaseState::failedToPrepare()
  {
    // it was impossible to find a new mirror, check if we still have running requests we can wait for, if not
    // we can only fail at this point
    if ( !_runningRequests.size() ) {
      setFailed( NetworkRequestErrorPrivate::customError( NetworkRequestError::InternalError, "No valid mirror found" ) );
    }
  }

  void RangeDownloaderBaseState::reschedule()
  {
    bool triggerResched = false;
    for ( auto &req : _runningRequests ) {
      if ( req->state() == NetworkRequest::Pending ) {
        triggerResched = true;
        req->setPriority( NetworkRequest::Critical, false );
      }
    }
    if ( triggerResched )
      stateMachine()._requestDispatcher->reschedule();
  }

  void RangeDownloaderBaseState::addNewRequest(const std::shared_ptr<Request>& req , const bool connectSignals)
  {
    if ( connectSignals )
      req->connectSignals( *this );

    _runningRequests.push_back( req );
    stateMachine()._requestDispatcher->enqueue( req );

    if ( req->_myMirror )
      req->_myMirror->startTransfer();
  }

  bool RangeDownloaderBaseState::assertExpectedFilesize( off_t currentFilesize )
  {
    const off_t expFSize = stateMachine()._spec.expectedFileSize();
    if ( expFSize  > 0 && expFSize < currentFilesize ) {
      setFailed( NetworkRequestErrorPrivate::customError( NetworkRequestError::ExceededMaxLen ) );
      return false;
    }
    return true;
  }

  /**
   * Just initialize the requests ranges from the internal blocklist
   */
  bool RangeDownloaderBaseState::addBlockRanges ( const std::shared_ptr<Request>& req , const std::vector<Block>& blocks ) const
  {
    req->resetRequestRanges();
    for ( const auto &block : blocks ) {
      if ( block._checksum.size() && block._chksumtype.size() ) {
        std::optional<zypp::Digest> dig = zypp::Digest();
        if ( !dig->create( block._chksumtype ) ) {
          WAR_MEDIA << "Trying to create Digest with chksum type " << block._chksumtype << " failed " << std::endl;
          return false;
        }

        if ( zypp::env::ZYPP_MEDIA_CURL_DEBUG() > 3 )
          DBG_MEDIA << "Starting block " << block._start << " with checksum " << zypp::Digest::digestVectorToString( block._checksum ) << "." << std::endl;
        req->addRequestRange( block._start, block._len, std::move(dig), block._checksum, std::any( block ), block._relevantDigestLen, block._chksumPad );
      } else {

        if ( zypp::env::ZYPP_MEDIA_CURL_DEBUG() > 3 )
          DBG_MEDIA << "Starting block " << block._start << " without checksum." << std::endl;
        req->addRequestRange( block._start, block._len, {}, {}, std::any( block ) );
      }
    }
    return true;
  }

  void zyppng::RangeDownloaderBaseState::setFailed(NetworkRequestError &&err)
  {
    _error = std::move( err );
    cancelAll( _error );
    zypp::filesystem::unlink( stateMachine()._spec.targetPath() );
    _sigFailed.emit();
  }

  void RangeDownloaderBaseState::setFailed(std::string &&reason)
  {
    setFailed( NetworkRequestErrorPrivate::customError( NetworkRequestError::InternalError, std::move(reason) ) );
  }

  void RangeDownloaderBaseState::setFinished()
  {
    _error = NetworkRequestError();
    _sigFinished.emit();
  }

  void RangeDownloaderBaseState::cancelAll(const NetworkRequestError &err)
  {
    while( _runningRequests.size() ) {
      auto req = _runningRequests.back();
      req->disconnectSignals();
      _runningRequests.pop_back();
      stateMachine()._requestDispatcher->cancel( *req, err );
      if ( req->_myMirror )
        req->_myMirror->cancelTransfer();
    }
  }

  std::vector<RangeDownloaderBaseState::Block> RangeDownloaderBaseState::getNextBlocks( const std::string &urlScheme )
  {
    std::vector<Block> blocks;
    const auto prefSize = std::max<zypp::ByteCount>( _preferredChunkSize, zypp::ByteCount(4, zypp::ByteCount::K) );
    size_t accumulatedSize = 0;

    bool canDoRandomBlocks = ( zypp::str::hasPrefixCI( urlScheme, "http") );

    std::optional<size_t> lastBlockEnd;
    while ( _ranges.size() && accumulatedSize < prefSize ) {
      const auto &r = _ranges.front();

      if ( !canDoRandomBlocks && lastBlockEnd ) {
        if ( static_cast<const size_t>(r._start) != (*lastBlockEnd)+1 )
          break;
      }

      lastBlockEnd = r._start + r._len - 1;
      accumulatedSize += r._len;

      blocks.push_back( std::move( _ranges.front() ) );
      _ranges.pop_front();

    }
    DBG_MEDIA << "Accumulated " << blocks.size() <<  " blocks with accumulated size of: " << accumulatedSize << "." << std::endl;
    return blocks;
  }

  std::vector<RangeDownloaderBaseState::Block> RangeDownloaderBaseState::getNextFailedBlocks( const std::string &urlScheme )
  {
    const auto prefSize = std::max<zypp::ByteCount>( _preferredChunkSize, zypp::ByteCount(4, zypp::ByteCount::K) );
    // sort the failed requests by block number, this should make sure get them in offset order as well
    _failedRanges.sort( []( const auto &a , const auto &b ){ return a._start < b._start; } );

    bool canDoRandomBlocks = ( zypp::str::hasPrefixCI( urlScheme, "http") );

    std::vector<Block> fblks;
    std::optional<size_t> lastBlockEnd;
    size_t accumulatedSize = 0;
    while ( _failedRanges.size() ) {

      const auto &block =_failedRanges.front();

      //we need to check if we have consecutive blocks because only http mirrors support random request ranges
      if ( !canDoRandomBlocks && lastBlockEnd ) {
        if ( static_cast<const size_t>(block._start) != (*lastBlockEnd)+1 )
          break;
      }

      lastBlockEnd = block._start + block._len - 1;
      accumulatedSize += block._len;

      fblks.push_back( std::move( _failedRanges.front() ));
      _failedRanges.pop_front();

      fblks.back()._retryCount += 1;

      if ( accumulatedSize >= prefSize )
        break;
    }

    return fblks;
  }

  zypp::ByteCount RangeDownloaderBaseState::makeBlksize ( size_t filesize )
  {
    // this case should never happen because we never start a multi download if we do not know the filesize beforehand
    if ( filesize == 0 )  return 4 * 1024 * 1024;
    else if ( filesize < 4*1024*1024 ) return filesize;
    else if ( filesize < 8*1024*1024 ) return 4*1024*1024;
    else if ( filesize < 16*1024*1024 ) return 8*1024*1024;
    else if ( filesize < 256*1024*1024 ) return 10*1024*1024;
    return 4*1024*1024;
  }
}
