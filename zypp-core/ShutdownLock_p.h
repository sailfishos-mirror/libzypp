/*---------------------------------------------------------------------\
|                          ____ _   __ __ ___                          |
|                         |__  / \ / / . \ . \                         |
|                           / / \ V /|  _/  _/                         |
|                          / /__ | | | | | |                           |
|                         /_____||_| |_| |_|                           |
|                                                                      |
\---------------------------------------------------------------------*/
/** \file	zypp/ShutdownLock_p.h
 *
*/

#ifndef ZYPP_SHUTDOWNLOCK_P_H_INCLUDED
#define ZYPP_SHUTDOWNLOCK_P_H_INCLUDED

#include <string>

#include <zypp-core/Globals.h>
#include <zypp-core/base/PtrTypes.h>

namespace zypp
{

class ExternalProgramWithSeperatePgid;

/**
 * Attempts to create a lock to prevent the system
 * from going into hibernate/shutdown. The lock is automatically
 * released when the object is destroyed.
 */
class ShutdownLock
{
public:
  ShutdownLock( const std::string &who, const std::string &reason );
   ~ShutdownLock();

private:
   shared_ptr<ExternalProgramWithSeperatePgid> _prog;

};

}


#endif
