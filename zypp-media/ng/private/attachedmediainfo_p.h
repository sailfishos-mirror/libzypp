/*---------------------------------------------------------------------\
|                          ____ _   __ __ ___                          |
|                         |__  / \ / / . \ . \                         |
|                           / / \ V /|  _/  _/                         |
|                          / /__ | | | | | |                           |
|                         /_____||_| |_| |_|                           |
|                                                                      |
\----------------------------------------------------------------------/
*
* This file contains private API, this might break at any time between releases.
* You have been warned!
*
*/
#ifndef ZYPP_MEDIA_PRIVATE_ATTACHEDMEDIAINFO_P_H_INCLUDED
#define ZYPP_MEDIA_PRIVATE_ATTACHEDMEDIAINFO_P_H_INCLUDED

#include "providefwd_p.h"
#include "providequeue_p.h"
#include "zypp-core/base/ReferenceCounted.h"
#include <zypp-media/ng/ProvideSpec>
#include <string>
#include <chrono>
#include <optional>

namespace zyppng {

  class ProvidePrivate;

  DEFINE_PTR_TYPE(AttachedMediaInfo);

  class AttachedMediaInfo : public zypp::base::ReferenceCounted, private zypp::base::NonCopyable {

  protected:
    void unref_to( unsigned int refCnt ) const override;
    void ref_to( unsigned refCnt ) const override;

  public:
    AttachedMediaInfo( const std::string &id, ProvideQueue::Config::WorkerType workerType, const zypp::MirroredOrigin &originConfig, ProvideMediaSpec &spec );
    AttachedMediaInfo( const std::string &id, ProvideQueueWeakRef backingQueue, ProvideQueue::Config::WorkerType workerType, const zypp::MirroredOrigin &originConfig, const ProvideMediaSpec &mediaSpec, const std::optional<zypp::Pathname> &mnt = {} );

    void setName( std::string &&name );
    const std::string &name() const;

    zypp::Url attachedUrl() const;

    /*!
     * Returns true if \a other requests the same medium as this instance
     */
    bool isSameMedium (const zypp::MirroredOrigin &origin, const ProvideMediaSpec &spec );

    static bool isSameMedium ( const zypp::MirroredOrigin &originA, const ProvideMediaSpec &specA, const zypp::MirroredOrigin &originB, const ProvideMediaSpec &specB );

    std::string _name;
    ProvideQueueWeakRef _backingQueue; //< if initialized contains a weak reference to the queue that owns this medium
    ProvideQueue::Config::WorkerType _workerType;
    zypp::MirroredOrigin _originConfig; //< baseUrl and mirrors with config
    ProvideMediaSpec _spec;
    std::optional<zypp::Pathname> _localMountPoint; // if initialized tells where the workers mounted to medium
    mutable std::optional<std::chrono::steady_clock::time_point> _idleSince; ///< Set if the medium is idle
  };

}

#endif
