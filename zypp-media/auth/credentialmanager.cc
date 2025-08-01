/*---------------------------------------------------------------------\
|                          ____ _   __ __ ___                          |
|                         |__  / \ / / . \ . \                         |
|                           / / \ V /|  _/  _/                         |
|                          / /__ | | | | | |                           |
|                         /_____||_| |_| |_|                           |
|                                                                      |
\---------------------------------------------------------------------*/
/** \file zypp-media/auth/credentialmanager.cc
 *
 */

#include "credentialmanager.h"

#include <iostream>
#include <fstream>

#include <utility>
#include <zypp-media/MediaConfig>
#include <zypp-core/base/Function.h>
#include <zypp-core/base/Logger.h>
#include <zypp-core/base/Easy.h>
#include <zypp-core/fs/PathInfo.h>

#include <zypp-media/auth/CredentialFileReader>
#include <zypp-media/MediaException>

#include <boost/interprocess/sync/file_lock.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>
#include <boost/interprocess/sync/sharable_lock.hpp>

namespace bpci = boost::interprocess;


using std::endl;

#define USER_CREDENTIALS_FILE ".zypp/credentials.cat"

//////////////////////////////////////////////////////////////////////
namespace zypp
{ ////////////////////////////////////////////////////////////////////
  //////////////////////////////////////////////////////////////////////
  namespace media
  { ////////////////////////////////////////////////////////////////////

  //////////////////////////////////////////////////////////////////////
  //
  // CLASS NAME : AuthDataComparator
  //
  //////////////////////////////////////////////////////////////////////

  bool AuthDataComparator::operator()( const AuthData_Ptr & lhs, const AuthData_Ptr & rhs ) const
  {
    static const url::ViewOption vopt = url::ViewOption::DEFAULTS
                                      - url::ViewOption::WITH_USERNAME
                                      - url::ViewOption::WITH_PASSWORD
                                      - url::ViewOption::WITH_QUERY_STR;
    // std::less semantic!
    int cmp = lhs->url().asString(vopt).compare( rhs->url().asString(vopt) );
    if ( ! cmp )
      cmp = lhs->username().compare( rhs->username() );
    return( cmp < 0 );
  }

  //////////////////////////////////////////////////////////////////////
  //
  // CLASS NAME : CredManagerOptions
  //
  //////////////////////////////////////////////////////////////////////

  CredManagerOptions::CredManagerOptions(const Pathname & rootdir)
    : globalCredFilePath(rootdir / MediaConfig::instance().credentialsGlobalFile())
    , customCredFileDir(rootdir / MediaConfig::instance().credentialsGlobalDir())
  {
    char * homedir = getenv("HOME");
    if (homedir)
      userCredFilePath = rootdir / homedir / USER_CREDENTIALS_FILE;
  }


  //////////////////////////////////////////////////////////////////////
  //
  // CLASS NAME : CredentialManager::Impl
  //
  struct CredentialManager::Impl
  {
    Impl(CredManagerOptions &&options);

    Impl(const Impl &) = delete;
    Impl(Impl &&) = delete;
    Impl &operator=(const Impl &) = delete;
    Impl &operator=(Impl &&) = delete;

    ~Impl()
    {}

    void init_globalCredentials();
    void init_userCredentials();

    bool processCredentials(AuthData_Ptr & cred);

    AuthData_Ptr getCred(const Url & url) const;
    AuthData_Ptr getCredFromFile(const Pathname & file);
    void saveGlobalCredentials();
    void saveUserCredentials();


    CredManagerOptions _options;

    CredentialSet _credsGlobal;
    CredentialSet _credsUser;
    CredentialSet _credsTmp;

    bool _globalDirty;
    bool _userDirty;
  };
  //////////////////////////////////////////////////////////////////////


  //////////////////////////////////////////////////////////////////////
  //
  // CLASS NAME : CredentialManager::Impl
  //
  //////////////////////////////////////////////////////////////////////

  CredentialManager::Impl::Impl(CredManagerOptions &&options)
    : _options(std::move(options))
    , _globalDirty(false)
    , _userDirty(false)
  {
    init_globalCredentials();
    init_userCredentials();
  }


  void CredentialManager::Impl::init_globalCredentials()
  {
    if (_options.globalCredFilePath.empty())
      DBG << "global cred file not known" << endl;
    else if (PathInfo(_options.globalCredFilePath).isExist())
    {
    /*  list<Pathname> entries;
      if (filesystem::readdir(entries, _options.globalCredFilePath, false) != 0)
        ZYPP_THROW(Exception("failed to read directory"));

      for_(it, entries.begin(), entries.end())*/

      CredentialFileReader(_options.globalCredFilePath,
          bind(&Impl::processCredentials, this, _1));
    }
    else
      DBG << "global cred file does not exist (" << _options.globalCredFilePath << ")" << endl;

    _credsGlobal = _credsTmp; _credsTmp.clear();
    DBG << "Got " << _credsGlobal.size() << " global records." << endl;
  }


  void CredentialManager::Impl::init_userCredentials()
  {
    if (_options.userCredFilePath.empty())
      DBG << "user cred file not known" << endl;
    else if (PathInfo(_options.userCredFilePath).isExist())
    {
    /*  list<Pathname> entries;
      if (filesystem::readdir(entries, _options.userCredFilePath, false ) != 0)
        ZYPP_THROW(Exception("failed to read directory"));

      for_(it, entries.begin(), entries.end())*/
      CredentialFileReader(_options.userCredFilePath,
          bind(&Impl::processCredentials, this, _1));
    }
    else
      DBG << "user cred file does not exist (" << _options.userCredFilePath << ")" << endl;

    _credsUser = _credsTmp; _credsTmp.clear();
    DBG << "Got " << _credsUser.size() << " user records." << endl;
  }


  bool CredentialManager::Impl::processCredentials(AuthData_Ptr & cred)
  {
    _credsTmp.insert(cred);
    return true;
  }


  AuthData_Ptr CredentialManager::findIn(const CredentialManager::CredentialSet & set,
                             const Url & url,
                             url::ViewOption vopt)
  {
    const std::string & username = url.getUsername();
    for( CredentialManager::CredentialIterator it = set.begin(); it != set.end(); ++it )
    {
      if ( !(*it)->url().isValid() )
        continue;

      // this ignores url params - not sure if it is good or bad...
      if ( url.asString(vopt).find((*it)->url().asString(vopt)) == 0 )
      {
        if ( username.empty() || username == (*it)->username() )
          return *it;
      }
    }

    return AuthData_Ptr();
  }

  AuthData_Ptr CredentialManager::Impl::getCred(const Url & url) const
  {
    AuthData_Ptr result;

    // compare the urls via asString(), but ignore password
    // default url::ViewOption will take care of that.
    // operator==(Url,Url) compares the whole Url

    url::ViewOption vopt;
    vopt = vopt
      - url::ViewOption::WITH_USERNAME
      - url::ViewOption::WITH_PASSWORD
      - url::ViewOption::WITH_QUERY_STR;

    // search in global credentials
    result = findIn(_credsGlobal, url, vopt);

    // search in home credentials
    if (!result)
      result = findIn(_credsUser, url, vopt);

    if (result)
      DBG << "Found credentials for '" << url << "':" << endl << *result << endl;
    else
      DBG << "No credentials for '" << url << "'" << endl;

    return result;
  }


  AuthData_Ptr CredentialManager::Impl::getCredFromFile(const Pathname & file)
  {
    AuthData_Ptr result;

    Pathname credfile;
    if (file.absolute())
      // get from that file
      credfile = file;
    else
      // get from /etc/zypp/credentials.d, delete the leading path
      credfile = _options.customCredFileDir / file.basename();

    PathInfo pi { credfile };
    if ( pi.userMayR() ) try {
      // make sure only our thread accesses the file
      bpci::file_lock lockFile ( credfile.c_str() );
      bpci::scoped_lock lock( lockFile );

      CredentialFileReader(credfile, bind(&Impl::processCredentials, this, _1));
    }
    catch ( ... ) {
      WAR << pi << " failed to lock file for reading." << endl;
    }

    if (_credsTmp.empty())
      WAR << pi << " does not contain valid credentials or is not readable." << endl;
    else
    {
      result = *_credsTmp.begin();
      _credsTmp.clear();
    }

    return result;
  }

  static int save_creds_in_file(
      CredentialManager::CredentialSet &creds,
      const Pathname & file,
      const mode_t mode)
  {
    int ret = 0;
    filesystem::assert_file_mode( file, mode );

    const auto now = time( nullptr );

    PathInfo pi { file };
    if ( pi.userMayRW() ) try {
      // make sure only our thread accesses the file
      bpci::file_lock lockFile ( file.c_str() );
      bpci::scoped_lock lock( lockFile );

      std::ofstream fs(file.c_str());
      for ( auto& credentials : creds )
      {
        credentials->dumpAsIniOn( fs );
        credentials->setLastDatabaseUpdate( now );
        fs << endl;
      }
      if ( !fs ) {
        WAR << pi << " failed to write credentials to file." << endl;
        ret = 1;
      }
      fs.close();
    }
    catch ( ... ) {
      WAR << pi << " failed to lock file for writing." << endl;
      ret = 1;
    }

    return ret;
  }

  void  CredentialManager::Impl::saveGlobalCredentials()
  {
    save_creds_in_file(_credsGlobal, _options.globalCredFilePath, 0640);
  }

  void  CredentialManager::Impl::saveUserCredentials()
  {
    save_creds_in_file(_credsUser, _options.userCredFilePath, 0600);
  }


  //////////////////////////////////////////////////////////////////////
  //
  // CLASS NAME : CredentialManager
  //
  //////////////////////////////////////////////////////////////////////

  CredentialManager::CredentialManager(CredManagerOptions opts)
    : _pimpl(new Impl(std::move(opts)))
  {}


  AuthData_Ptr CredentialManager::getCred(const Url & url)
  {
    std::string credfile = url.getQueryParam("credentials");
    if (credfile.empty())
      return _pimpl->getCred(url);
    return _pimpl->getCredFromFile(credfile);
  }


  AuthData_Ptr CredentialManager::getCredFromFile(const Pathname & file)
  { return _pimpl->getCredFromFile(file); }


  void CredentialManager::addCred(const AuthData & cred)
  {
    if ( !cred.url().isValid() )
      ZYPP_THROW( MediaInvalidCredentialsException( "URL must be valid in order to save AuthData." ) );

    Pathname credfile = cred.url().getQueryParam("credentials");
    if (credfile.empty())
      //! \todo ask user where to store these creds. saving to user creds for now
      addUserCred(cred);
    else
      saveInFile(cred, credfile);
  }

  time_t CredentialManager::timestampForCredDatabase ( const zypp::Url &url )
  {
    Pathname credfile;
    if ( url.isValid() ) {
      credfile = url.getQueryParam("credentials");
    }

    if (credfile.empty())
      credfile = _pimpl->_options.userCredFilePath;

    zypp::PathInfo pi(credfile);
    if ( pi.isExist() && pi.isFile() )
      return pi.mtime();

    return 0;
  }

  void CredentialManager::addGlobalCred(const AuthData & cred)
  {
    if ( !cred.url().isValid() )
      ZYPP_THROW( MediaInvalidCredentialsException( "URL must be valid in order to save AuthData." ) );

    AuthData_Ptr c_ptr;
    c_ptr.reset(new AuthData(cred)); // FIX for child classes if needed
    std::pair<CredentialIterator, bool> ret = _pimpl->_credsGlobal.insert(c_ptr);
    if (ret.second)
      _pimpl->_globalDirty = true;
    else if ((*ret.first)->password() != cred.password())
    {
      _pimpl->_credsGlobal.erase(ret.first);
      _pimpl->_credsGlobal.insert(c_ptr);
      _pimpl->_globalDirty = true;
    }
  }


  void CredentialManager::addUserCred(const AuthData & cred)
  {
    if ( !cred.url().isValid() )
      ZYPP_THROW( MediaInvalidCredentialsException( "URL must be valid in order to save AuthData." ) );

    AuthData_Ptr c_ptr;
    c_ptr.reset(new AuthData(cred)); // FIX for child classes if needed
    std::pair<CredentialIterator, bool> ret = _pimpl->_credsUser.insert(c_ptr);
    if (ret.second)
      _pimpl->_userDirty = true;
    else if ((*ret.first)->password() != cred.password())
    {
      _pimpl->_credsUser.erase(ret.first);
      _pimpl->_credsUser.insert(c_ptr);
      _pimpl->_userDirty = true;
    }
  }


  void CredentialManager::save()
  {
    if (_pimpl->_globalDirty)
      _pimpl->saveGlobalCredentials();
    if (_pimpl->_userDirty)
      _pimpl->saveUserCredentials();
    _pimpl->_globalDirty = false;
    _pimpl->_userDirty = false;
  }


  void CredentialManager::saveInGlobal(const AuthData & cred)
  {
    addGlobalCred(cred);
    save();
  }


  void CredentialManager::saveInUser(const AuthData & cred)
  {
    addUserCred(cred);
    save();
  }


  void CredentialManager::saveInFile(const AuthData & cred, const Pathname & credFile)
  {
    AuthData_Ptr c_ptr;
    c_ptr.reset(new AuthData(cred)); // FIX for child classes if needed
    c_ptr->setUrl(Url()); // don't save url in custom creds file
    CredentialManager::CredentialSet creds;
    creds.insert(c_ptr);

    int ret = 0;
    if (credFile.absolute())
      ret = save_creds_in_file(creds, credFile, 0640);
    else
      ret = save_creds_in_file(
          creds, _pimpl->_options.customCredFileDir / credFile, 0600);

    if (!ret)
    {
      //! \todo figure out the reason(?), call back to user
      ERR << "error saving the credentials" << endl;
    }
  }


  void CredentialManager::clearAll(bool global)
  {
    if (global)
    {
      if (!filesystem::unlink(_pimpl->_options.globalCredFilePath))
        ERR << "could not delete user credentials file "
            << _pimpl->_options.globalCredFilePath << endl;
      _pimpl->_credsUser.clear();
    }
    else
    {
      if (!filesystem::unlink(_pimpl->_options.userCredFilePath))
        ERR << "could not delete global credentials file"
            << _pimpl->_options.userCredFilePath << endl;
      _pimpl->_credsGlobal.clear();
    }
  }


  CredentialManager::CredentialIterator CredentialManager::credsGlobalBegin() const
  { return _pimpl->_credsGlobal.begin(); }

  CredentialManager::CredentialIterator CredentialManager::credsGlobalEnd() const
  { return _pimpl->_credsGlobal.end(); }

  CredentialManager::CredentialSize CredentialManager::credsGlobalSize() const
  { return _pimpl->_credsGlobal.size(); }

  bool CredentialManager::credsGlobalEmpty() const
  { return _pimpl->_credsGlobal.empty(); }


  CredentialManager::CredentialIterator CredentialManager::credsUserBegin() const
  { return _pimpl->_credsUser.begin(); }

  CredentialManager::CredentialIterator CredentialManager::credsUserEnd() const
  { return _pimpl->_credsUser.end(); }

  CredentialManager::CredentialSize CredentialManager::credsUserSize() const
  { return _pimpl->_credsUser.size(); }

  bool CredentialManager::credsUserEmpty() const
  { return _pimpl->_credsUser.empty(); }


    ////////////////////////////////////////////////////////////////////
  } // media
  //////////////////////////////////////////////////////////////////////
  ////////////////////////////////////////////////////////////////////
} // zypp
//////////////////////////////////////////////////////////////////////
