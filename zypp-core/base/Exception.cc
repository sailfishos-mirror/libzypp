/*---------------------------------------------------------------------\
|                          ____ _   __ __ ___                          |
|                         |__  / \ / / . \ . \                         |
|                           / / \ V /|  _/  _/                         |
|                          / /__ | | | | | |                           |
|                         /_____||_| |_| |_|                           |
|                                                                      |
\---------------------------------------------------------------------*/
/** \file zypp/base/Exception.cc
 *
*/
#include <iostream>
#include <sstream>

#include <zypp-core/base/Logger.h>
#include <zypp-core/base/LogTools.h>
#include <zypp-core/base/Gettext.h>
#include <zypp-core/base/StringV.h>
#include <zypp-core/base/Exception.h>

using std::endl;

///////////////////////////////////////////////////////////////////
namespace zypp
{ /////////////////////////////////////////////////////////////////
  ///////////////////////////////////////////////////////////////////
  namespace exception_detail
  { /////////////////////////////////////////////////////////////////

    std::string CodeLocation::asString() const
    {
      return str::form( "%s(%s):%u",
                        _file.c_str(),
                        _func.c_str(),
                        _line );
    }

    std::ostream & operator<<( std::ostream & str, const CodeLocation & obj )
    { return str << obj.asString(); }

    void do_ZYPP_RETHROW(const std::exception_ptr &excpt_r, const CodeLocation &where_r)
    {
      if ( !excpt_r )
        return;

      try {
        std::rethrow_exception (excpt_r);
      } catch ( const zypp::Exception &e ) {
        Exception::log( e, where_r, "RETHROW: " );
        throw;
      } catch ( const std::exception & e ) {
        Exception::log( typeid(e).name(), where_r, "RETHROW:   " );
        throw;
      } catch (...) {
        Exception::log( "Unknown Exception", where_r, "RETHROW:   " );
        throw;
      }
    }

  std::exception_ptr do_ZYPP_FWD_EXCPT_PTR(const std::exception_ptr & excpt_r, CodeLocation &&where_r )
  {
    try {
      std::rethrow_exception( excpt_r );
    } catch ( zypp::Exception &e ) {
      Exception::log( e, where_r, "RETHROW (FWD) EXCPTR:   " );
      e.relocate( std::move(where_r) );
      return std::current_exception();
    } catch ( const std::exception & e ) {
      Exception::log( typeid(e).name(), where_r, "RETHROW (FWD) EXCPTR:   " );
      return std::current_exception();
    } catch (...) {
      Exception::log( "Unknown Exception", where_r, "RETHROW (FWD) EXCPTR:   " );
      return std::current_exception();
    }
  }

  void do_ZYPP_CAUGHT(const std::exception_ptr &excpt_r, CodeLocation &&where_r)
  {
    try {
      std::rethrow_exception( excpt_r );
    } catch ( zypp::Exception &e ) {
      Exception::log( e, where_r, "CAUGHT:  " );
    } catch ( const std::exception & e ) {
      Exception::log( typeid(e).name(), where_r, "CAUGHT:  " );
    } catch (...) {
      Exception::log( "Unknown Exception", where_r, "CAUGHT:  "  );
    }
  }

    /////////////////////////////////////////////////////////////////
  } // namespace exception_detail
  ///////////////////////////////////////////////////////////////////

  Exception::Exception()
  {}

  Exception::Exception( const std::string & msg_r )
    : _msg( msg_r )
  {}

  Exception::Exception( std::string && msg_r )
    : _msg( std::move(msg_r) )
  {}

  Exception::Exception( const std::string & msg_r, const Exception & history_r )
    : _msg( msg_r )
  { remember( history_r ); }

  Exception::Exception( std::string && msg_r, const Exception & history_r )
  : _msg( std::move(msg_r) )
  { remember( history_r ); }

  Exception::Exception( const std::string & msg_r, Exception && history_r )
  : _msg( msg_r )
  { remember( std::move(history_r) ); }

  Exception::Exception( std::string && msg_r, Exception && history_r )
  : _msg( std::move(msg_r) )
  { remember( std::move(history_r) ); }

  Exception::~Exception() throw()
  {}

  std::string Exception::asString() const
  {
    std::ostringstream str;
    dumpOn( str );
    return str.str();
  }

  std::string Exception::asUserString() const
  {
    std::ostringstream str;
    dumpOn( str );
    // call gettext to translate the message. This will
    // not work if dumpOn() uses composed messages.
    return _(str.str().c_str());
  }

  std::string Exception::asUserHistory() const
  {
    if ( historyEmpty() )
      return asUserString();

    std::string ret( asUserString() );
    if ( ret.empty() )
      return historyAsString();

    ret += '\n';
    ret += historyAsString();
    return ret;
  }

  void Exception::remember( const Exception & old_r )
  {
    if ( &old_r != this ) // no self-remember
    {
      History newh( old_r._history.begin(), old_r._history.end() );
      newh.push_front( old_r.asUserString() );
      _history.swap( newh );
    }
  }

  void Exception::remember( Exception && old_r )
  {
    if ( &old_r != this ) // no self-remember
    {
      History & newh( old_r._history );	// stealing it
      newh.push_front( old_r.asUserString() );
      _history.swap( newh );
    }
  }

  void Exception::remember( std::exception_ptr old_r )
  {
    try {
      if (old_r) {
        std::rethrow_exception(std::move(old_r));
      }
    } catch( const Exception& e ) {
      remember( e );
    } catch ( const std::exception& e ) {
      addHistory( e.what() );
    } catch ( ... ) {
      addHistory( "Remembered unknown exception" );
    }
  }

  void Exception::addHistory( const std::string & msg_r )
  { _history.push_front( msg_r ); }

  void Exception::addHistory( std::string && msg_r )
  { _history.push_front( std::move(msg_r) ); }

  std::string Exception::historyAsString() const
  {
    std::ostringstream ret;
    if ( not _history.empty() ) {
      ret << _("History:");
      for ( const std::string & entry : _history ) {
        strv::split( entry, "\n", [&ret]( std::string_view line_r, unsigned idx, bool last_r ) -> void {
          if ( not ( last_r && line_r.empty() ) )
            ret  << endl << (idx==0?" - ":"   ") << line_r;
        });
      }
    }
    return ret.str();
  }

  std::ostream & Exception::dumpOn( std::ostream & str ) const
  { return str << _msg; }

  std::ostream & Exception::dumpError( std::ostream & str ) const
  { return dumpOn( str << _where << ": " ); }

  std::ostream & operator<<( std::ostream & str, const Exception & obj )
  { return obj.dumpError( str ); }

  std::ostream & operator<<( std::ostream & str, const std::exception_ptr &excptPtr )
  {
    try {
      std::rethrow_exception (excptPtr) ;
    } catch ( const zypp::Exception &e ) {
      str << e; // forward to Exception stream operator
    } catch ( const std::exception &e ) {
      str << "std::exception: (" << e.what() << ")";
    } catch ( ... ) {
      str << "Unknown exception";
    }
    return str;
  }

  std::string Exception::strErrno( int errno_r )
  { return str::strerror( errno_r ); }

  std::string Exception::strErrno( int errno_r, std::string msg_r )
  {
    msg_r += ": ";
    return msg_r += strErrno( errno_r );
  }

  void Exception::log( const Exception & excpt_r, const CodeLocation & where_r,
                       const char *const prefix_r )
  {
    INT << where_r << " " << prefix_r << " " << excpt_r.asUserHistory() << endl;
  }

  void Exception::log( const char * typename_r, const CodeLocation & where_r,
                       const char *const prefix_r )
  {
    INT << where_r << " " << prefix_r << " exception of type " << typename_r << endl;
  }
  /////////////////////////////////////////////////////////////////
} // namespace zypp
///////////////////////////////////////////////////////////////////
