/*---------------------------------------------------------------------\
|                          ____ _   __ __ ___                          |
|                         |__  / \ / / . \ . \                         |
|                           / / \ V /|  _/  _/                         |
|                          / /__ | | | | | |                           |
|                         /_____||_| |_| |_|                           |
|                                                                      |
\---------------------------------------------------------------------*/

#include <iostream>
#include <fstream>

#include <zypp/base/LogTools.h>
#include <zypp/base/Regex.h>
#include <utility>
#include <zypp-core/base/UserRequestException>
#include <zypp-media/MediaException>
#include <zypp/ZYppCallbacks.h>
#include <zypp/MediaSetAccess.h>
#include <zypp/PathInfo.h>
#include <zypp/TmpPath.h>
//#include <zypp/source/MediaSetAccessReportReceivers.h>

#undef ZYPP_BASE_LOGGER_LOGGROUP
#define ZYPP_BASE_LOGGER_LOGGROUP "zypp::fetcher"

using std::endl;

///////////////////////////////////////////////////////////////////
namespace zypp
{ /////////////////////////////////////////////////////////////////

IMPL_PTR_TYPE(MediaSetAccess);

class MediaSetAccess::Impl {
public:

  Impl( MirroredOrigin &&origin, Pathname &&prefered_attach_point )
    : _origin(std::move(origin))
    , _prefAttachPoint(std::move(prefered_attach_point))
  { }

  Impl( std::string &&label_r, MirroredOrigin &&origin, Pathname &&prefered_attach_point)
    : _origin(std::move(origin))
    , _prefAttachPoint(std::move(prefered_attach_point))
    , _label(std::move( label_r ))
  { }

  /** Media Origin configuration */
  MirroredOrigin _origin;

  /**
   * Prefered mount point.
   *
   * \see MediaManager::open(Url,Pathname)
   * \see MediaHandler::_attachPoint
   */
  Pathname _prefAttachPoint;

  std::string _label;

  using MediaMap = std::map<media::MediaNr, media::MediaAccessId>;
  using VerifierMap = std::map<media::MediaNr, media::MediaVerifierRef>;

  /** Mapping between media number and Media Access ID */
  MediaMap _medias;
  /** Mapping between media number and corespondent verifier */
  VerifierMap _verifiers;
};

///////////////////////////////////////////////////////////////////

  MediaSetAccess::MediaSetAccess(Url url, Pathname  prefered_attach_point)
    : MediaSetAccess( { MirroredOrigin( std::move(url)) }, std::move(prefered_attach_point) )
  {}

  MediaSetAccess::MediaSetAccess(std::string label_r, Url url, Pathname prefered_attach_point)
    : MediaSetAccess( std::move(label_r), { MirroredOrigin( std::move(url)) }, std::move(prefered_attach_point) )
  {}

  MediaSetAccess::MediaSetAccess(MirroredOrigin origin, Pathname prefered_attach_point)
    : _pimpl( std::make_unique<Impl>( std::move(origin), std::move(prefered_attach_point) ) )
  { }

  MediaSetAccess::MediaSetAccess(std::string label_r, MirroredOrigin origin, Pathname prefered_attach_point)
    : _pimpl( std::make_unique<Impl>( std::move(label_r), std::move(origin), std::move(prefered_attach_point) ) )
  { }

  MediaSetAccess::~MediaSetAccess()
  {
    try
    {
      media::MediaManager manager;
      for ( const auto & mm : _pimpl->_medias )
        manager.close( mm.second );
    }
    catch(...) {} // don't let exception escape a dtor.
  }


  void MediaSetAccess::setVerifier( unsigned media_nr, const media::MediaVerifierRef& verifier )
  {
    if (_pimpl->_medias.find(media_nr) != _pimpl->_medias.end())
    {
      // the media already exists, set theverifier
      media::MediaAccessId id = _pimpl->_medias[media_nr];
      media::MediaManager media_mgr;
      media_mgr.addVerifier( id, verifier );
      // remove any saved verifier for this media
      _pimpl->_verifiers.erase(media_nr);
    }
    else
    {
      // save the verifier in the map, and set it when
      // the media number is first attached
      _pimpl->_verifiers[media_nr] = verifier;
    }
  }

  const std::string &MediaSetAccess::label() const
  { return _pimpl->_label; }

  void MediaSetAccess::setLabel(const std::string &label_r)
  { _pimpl->_label = label_r; }

  void MediaSetAccess::releaseFile( const OnMediaLocation & on_media_file )
  {
    releaseFile( on_media_file.filename(), on_media_file.medianr() );
  }

  void MediaSetAccess::releaseFile( const Pathname & file, unsigned media_nr)
  {
    media::MediaManager media_mgr;
    media::MediaAccessId media = getMediaAccessId( media_nr);
    DBG << "Going to release file " << file
        << " from media number " << media_nr << endl;

    if ( ! media_mgr.isAttached(media) )
      return; //disattached media is free

    media_mgr.releaseFile (media, file);
  }

  void MediaSetAccess::dirInfo( filesystem::DirContent &retlist, const Pathname &dirname,
                                bool dots, unsigned media_nr )
  {
    media::MediaManager media_mgr;
    media::MediaAccessId media = getMediaAccessId(media_nr);

    // try to attach the media
    if ( ! media_mgr.isAttached(media) )
        media_mgr.attach(media);

    media_mgr.dirInfo(media, retlist, dirname, dots);
  }

  struct ProvideFileOperation
  {
    Pathname result;
    void operator()( media::MediaAccessId media, const OnMediaLocation &file )
    {
      media::MediaManager media_mgr;
      media_mgr.provideFile( media, file );
      result = media_mgr.localPath( media, file.filename() );
    }
  };

  struct ProvideDirTreeOperation
  {
    Pathname result;
    void operator()( media::MediaAccessId media, const OnMediaLocation &file )
    {
      const auto &fName = file.filename();
      media::MediaManager media_mgr;
      media_mgr.provideDirTree( media, fName );
      result = media_mgr.localPath( media, fName );
    }
  };

  struct ProvideDirOperation
  {
    Pathname result;
    void operator()( media::MediaAccessId media, const OnMediaLocation &file )
    {
      const auto &fName = file.filename();
      media::MediaManager media_mgr;
      media_mgr.provideDir( media, fName );
      result = media_mgr.localPath( media, fName );
    }
  };

  struct ProvideFileExistenceOperation
  {
    bool result;
    ProvideFileExistenceOperation()
        : result(false)
    {}

    void operator()( media::MediaAccessId media, const OnMediaLocation &file )
    {
      const auto &fName = file.filename();
      media::MediaManager media_mgr;
      result = media_mgr.doesFileExist( media, fName );
    }
  };

  Pathname MediaSetAccess::provideFile( const OnMediaLocation &resource, ProvideFileOptions options )
  {
    ProvideFileOperation op;
    provide( std::ref(op), resource, options );
    return op.result;
  }

  Pathname MediaSetAccess::provideFile( const OnMediaLocation & resource, ProvideFileOptions options, const Pathname &deltafile )
  {
    return provideFile( OnMediaLocation( resource ).setDeltafile( deltafile ), options );
  }

  Pathname MediaSetAccess::provideFile(const Pathname & file, unsigned media_nr, ProvideFileOptions options )
  {
    return provideFile( OnMediaLocation( file, media_nr ), options );
  }

  Pathname MediaSetAccess::provideOptionalFile( const Pathname & file, unsigned media_nr )
  {
    try
    {
      return provideFile( OnMediaLocation( file, media_nr ).setOptional( true ), PROVIDE_NON_INTERACTIVE );
    }
    catch ( const media::MediaFileNotFoundException & excpt_r )
    { ZYPP_CAUGHT( excpt_r ); }
    catch ( const media::MediaForbiddenException & excpt_r )
    { ZYPP_CAUGHT( excpt_r ); }
    catch ( const media::MediaNotAFileException & excpt_r )
    { ZYPP_CAUGHT( excpt_r ); }
   return Pathname();
  }

  ManagedFile MediaSetAccess::provideFileFromUrl(const Url &file_url, ProvideFileOptions options)
  {
    Url url(file_url);
    Pathname path(url.getPathName());

    url.setPathName ("/");
    MediaSetAccess access( zypp::MirroredOrigin{url} );

    ManagedFile tmpFile = filesystem::TmpFile::asManagedFile();

    bool optional = options & PROVIDE_NON_INTERACTIVE;
    Pathname file = access.provideFile( OnMediaLocation(path, 1).setOptional( optional ), options );

    //prevent the file from being deleted when MediaSetAccess gets out of scope
    if ( filesystem::hardlinkCopy(file, tmpFile) != 0 )
      ZYPP_THROW(Exception("Can't copy file from " + file.asString() + " to " +  tmpFile->asString() ));

    return tmpFile;
  }

  ManagedFile MediaSetAccess::provideOptionalFileFromUrl( const Url & file_url )
  {
    try
    {
      return provideFileFromUrl( file_url, PROVIDE_NON_INTERACTIVE );
    }
    catch ( const media::MediaFileNotFoundException & excpt_r )
    { ZYPP_CAUGHT( excpt_r ); }
    catch ( const media::MediaNotAFileException & excpt_r )
    { ZYPP_CAUGHT( excpt_r ); }
   return ManagedFile();
  }

  bool MediaSetAccess::doesFileExist(const Pathname & file, unsigned media_nr )
  {
    ProvideFileExistenceOperation op;
    OnMediaLocation resource(file, media_nr);
    provide( std::ref(op), resource, PROVIDE_DEFAULT );
    return op.result;
  }

  void MediaSetAccess::precacheFiles(const std::vector<OnMediaLocation> &files)
  {
    media::MediaManager media_mgr;

    for ( const auto &resource : files ) {
           unsigned media_nr(resource.medianr());
      media::MediaAccessId media = getMediaAccessId( media_nr );

      if ( !media_mgr.isOpen( media ) ) {
        MIL << "Skipping precache of file " << resource.filename() << " media is not open";
        continue;
      }

      if ( ! media_mgr.isAttached(media) )
        media_mgr.attach(media);

      media_mgr.precacheFiles( media, { resource } );
    }
  }

  void MediaSetAccess::provide( const ProvideOperation& op,
                                const OnMediaLocation &resource,
                                ProvideFileOptions options )
  {
    const auto &file(resource.filename());
    unsigned media_nr(resource.medianr());

    callback::SendReport<media::MediaChangeReport> report;
    media::MediaManager media_mgr;

    media::MediaAccessId media = 0;

    do
    {
      // get the mediaId, but don't try to attach it here
      media = getMediaAccessId( media_nr);

      try
      {
        DBG << "Going to try to provide " << (resource.optional() ? "optional" : "") << " file " << file
            << " from media number " << media_nr << endl;
        // try to attach the media
        if ( ! media_mgr.isAttached(media) )
          media_mgr.attach(media);
        op(media, resource);
        break;
      }
      catch ( media::MediaException & excp )
      {
        ZYPP_CAUGHT(excp);
        media::MediaChangeReport::Action user = media::MediaChangeReport::ABORT;
        unsigned int devindex = 0;
        std::vector<std::string> devices;
        media_mgr.getDetectedDevices(media, devices, devindex);

        do
        {
          // set up the reason
          media::MediaChangeReport::Error reason = media::MediaChangeReport::INVALID;

          if( typeid(excp) == typeid( media::MediaFileNotFoundException )  ||
              typeid(excp) == typeid( media::MediaNotAFileException ) )
          {
            reason = media::MediaChangeReport::NOT_FOUND;
          }
          else if( typeid(excp) == typeid( media::MediaNotDesiredException)  ||
              typeid(excp) == typeid( media::MediaNotAttachedException) )
          {
            reason = media::MediaChangeReport::WRONG;
          }
          else if( typeid(excp) == typeid( media::MediaTimeoutException) ||
                   typeid(excp) == typeid( media::MediaTemporaryProblemException))
          {
            reason = media::MediaChangeReport::IO_SOFT;
          }

          // Propagate the original error if _no_ callback receiver is connected, or
          // non_interactive mode (for optional files) is used (except for wrong media).
          if ( ! callback::SendReport<media::MediaChangeReport>::connected()
             || (( options & PROVIDE_NON_INTERACTIVE ) && reason != media::MediaChangeReport::WRONG ) )
          {
              MIL << "Can't provide file. Non-Interactive mode." << endl;
              ZYPP_RETHROW(excp);
          }
          else
          {
            // release all media before requesting another (#336881)
            media_mgr.releaseAll();

            zypp::Url u = _pimpl->_origin.authority().url();
            user = report->requestMedia (
              u,
              media_nr,
              _pimpl->_label,
              reason,
              excp.asUserHistory(),
              devices,
              devindex
            );

            // if the user changes the primary URL, we can no longer use the mirrors,
            // so we drop them and the settings for the primary too!
            if ( u != _pimpl->_origin.authority().url() ) {
              MIL << "User changed the URL, dropping all mirrors" << std::endl;
              _pimpl->_origin.clearMirrors();
              _pimpl->_origin.setAuthority(u);
            }
          }

          MIL << "ProvideFile exception caught, callback answer: " << user << endl;

          if( user == media::MediaChangeReport::ABORT )
          {
            DBG << "Aborting" << endl;
            AbortRequestException aexcp("Aborting requested by user");
            aexcp.remember(excp);
            ZYPP_THROW(aexcp);
          }
          else if ( user == media::MediaChangeReport::IGNORE )
          {
            DBG << "Skipping" << endl;
            SkipRequestException nexcp("User-requested skipping of a file");
            nexcp.remember(excp);
            ZYPP_THROW(nexcp);
          }
          else if ( user == media::MediaChangeReport::EJECT )
          {
            DBG << "Eject: try to release" << endl;
            try
            {
              media_mgr.releaseAll();
              media_mgr.release (media, devindex < devices.size() ? devices[devindex] : "");
            }
            catch ( const Exception & e)
            {
              ZYPP_CAUGHT(e);
            }
          }
          else if ( user == media::MediaChangeReport::RETRY  ||
            user == media::MediaChangeReport::CHANGE_URL )
          {
            // retry
            DBG << "Going to try again" << endl;
            // invalidate current media access id
            media_mgr.close(media);
            _pimpl->_medias.erase(media_nr);

            // not attaching, media set will do that for us
            // this could generate uncaught exception (#158620)
            break;
          }
          else
          {
            DBG << "Don't know, let's ABORT" << endl;
            ZYPP_RETHROW ( excp );
          }
        } while( user == media::MediaChangeReport::EJECT );
      }

      // retry or change URL
    } while( true );
  }

  Pathname MediaSetAccess::provideDir(const Pathname & dir,
                                      bool recursive,
                                      unsigned media_nr,
                                      ProvideFileOptions options )
  {
    OnMediaLocation resource(dir, media_nr);
    if ( recursive )
    {
        ProvideDirTreeOperation op;
        provide( std::ref(op), resource, options );
        return op.result;
    }
    ProvideDirOperation op;
    provide( std::ref(op), resource, options );
    return op.result;
  }

  media::MediaAccessId MediaSetAccess::getMediaAccessId (media::MediaNr medianr)
  {
    if ( _pimpl->_medias.find( medianr ) != _pimpl->_medias.end() )
    {
      return _pimpl->_medias[medianr];
    }

    MirroredOrigin rewrittenOrigin = _pimpl->_origin;
    if ( medianr > 1 ) {
      for ( auto &url : rewrittenOrigin ) {
        url.setUrl ( rewriteUrl (url.url(), medianr) );
      }
    }
    media::MediaManager media_mgr;
    media::MediaAccessId id = media_mgr.open( rewrittenOrigin, _pimpl->_prefAttachPoint );
    _pimpl->_medias[medianr] = id;

    try
    {
      if ( _pimpl->_verifiers.find(medianr) != _pimpl->_verifiers.end() )
      {
        // a verifier is set for this media
        // FIXME check the case where the verifier exists
        // but we have no access id for the media
        media_mgr.delVerifier( id );
        media_mgr.addVerifier( id, _pimpl->_verifiers[medianr] );
        // remove any saved verifier for this media
        _pimpl->_verifiers.erase( medianr );
      }
    }
    catch ( const Exception &e )
    {
      ZYPP_CAUGHT(e);
      WAR << "Verifier not found" << endl;
    }

    return id;
  }


  Url MediaSetAccess::rewriteUrl (const Url & url_r, const media::MediaNr medianr)
  {
    std::string scheme = url_r.getScheme();
    if (scheme == "cd" || scheme == "dvd")
      return url_r;

    DBG << "Rewriting url " << url_r << endl;

    if( scheme == "iso")
    {
      // TODO the iso parameter will not be required in the future, this
      // code has to be adapted together with the MediaISO change.
      // maybe some MediaISOURL interface should be used.
      std::string isofile = url_r.getQueryParam("iso");
      str::regex e("^(.*)(cd|dvd|media)[0-9]+\\.iso$", str::regex::icase);

      str::smatch what;
      if(str::regex_match(isofile, what, e))
      {
        Url url( url_r);
        isofile = what[1] + what[2] + str::numstring(medianr) + ".iso";
        url.setQueryParam("iso", isofile);
        DBG << "Url rewrite result: " << url << endl;
        return url;
      }
    }
    else
    {
      std::string pathname = url_r.getPathName();
      str::regex e("^(.*)(cd|dvd|media)[0-9]+(/)?$", str::regex::icase);
      str::smatch what;
      if(str::regex_match(pathname, what, e))
      {
        Url url( url_r);
        pathname = what[1] + what[2] + str::numstring(medianr) + what[3];
        url.setPathName(pathname);
        DBG << "Url rewrite result: " << url << endl;
        return url;
      }
    }
    return url_r;
  }

  void MediaSetAccess::release()
  {
    DBG << "Releasing all media IDs held by this MediaSetAccess" << endl;
    media::MediaManager manager;
    for ( auto m = _pimpl->_medias.begin(); m != _pimpl->_medias.end(); ++m )
      manager.release(m->second, "");
  }

  std::ostream & MediaSetAccess::dumpOn( std::ostream & str ) const
  {
    str << "MediaSetAccess (URL='" << _pimpl->_origin.authority().url() << "', attach_point_hint='" << _pimpl->_prefAttachPoint << "')";
    return str;
  }

/////////////////////////////////////////////////////////////////
} // namespace zypp
///////////////////////////////////////////////////////////////////
