/*---------------------------------------------------------------------\
|                          ____ _   __ __ ___                          |
|                         |__  / \ / / . \ . \                         |
|                           / / \ V /|  _/  _/                         |
|                          / /__ | | | | | |                           |
|                         /_____||_| |_| |_|                           |
|                                                                      |
----------------------------------------------------------------------/
*
* This file contains private API, this might break at any time between releases.
* You have been warned!
*
*/
#ifndef ZYPP_NG_MEDIA_CURL_PRIVATE_NETWORKREQUESTERROR_P_H_INCLUDED
#define ZYPP_NG_MEDIA_CURL_PRIVATE_NETWORKREQUESTERROR_P_H_INCLUDED

#include <zypp-curl/ng/network/networkrequesterror.h>
#include <map>

namespace zyppng {

class NetworkRequestErrorPrivate
{
public:
  NetworkRequestErrorPrivate( NetworkRequestError::Type code, std::string &&msg, std::map<std::string, boost::any> && extraInfo );

  NetworkRequestErrorPrivate *clone () const;

  NetworkRequestError::Type _errorCode = NetworkRequestError::InternalError;
  std::string _errorMessage;
  std::map<std::string, boost::any> _extraInfo;

  static zyppng::NetworkRequestError customError( NetworkRequestError::Type t, std::string &&errorMsg = "", std::map<std::string, boost::any> &&extraInfo = {} );
  static zyppng::NetworkRequestError fromCurlError( NetworkRequest &req, int nativeCode , const std::string &nativeError );
  static zyppng::NetworkRequestError fromCurlMError ( int nativeCode );
  static std::string typeToString( NetworkRequestError::Type t );
};

}

#endif
