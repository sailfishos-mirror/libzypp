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

#include "normal_p.h"
#include "final_p.h"

namespace zyppng {

  DlNormalFileState::DlNormalFileState( DownloadPrivate &parent ) : BasicDownloaderStateBase( parent )
  {
    MIL << "About to enter DlNormalFileState for url " << parent._spec.url() << std::endl;
  }


  DlNormalFileState::DlNormalFileState( std::shared_ptr<Request> &&oldReq, DownloadPrivate &parent ) : BasicDownloaderStateBase( std::move(oldReq), parent )
  {
    MIL << "About to enter DlNormalFileState for url " << parent._spec.url() << std::endl;
  }

  std::shared_ptr<FinishedState> DlNormalFileState::transitionToFinished()
  {
    return std::make_shared<FinishedState>( std::move(_error), stateMachine() );
  }


}
