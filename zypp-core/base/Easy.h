/*---------------------------------------------------------------------\
|                          ____ _   __ __ ___                          |
|                         |__  / \ / / . \ . \                         |
|                           / / \ V /|  _/  _/                         |
|                          / /__ | | | | | |                           |
|                         /_____||_| |_| |_|                           |
|                                                                      |
\---------------------------------------------------------------------*/
/** \file	zypp/base/Easy.h
 *
*/
#ifndef ZYPP_BASE_EASY_H
#define ZYPP_BASE_EASY_H

#include <cstdio>
#include <zypp/base/TypeTraits.h>

/** Convenient for-loops using iterator.
 * \code
 *  std::set<std::string>; _store;
 *  for_( it, _store.begin(), _store.end() )
 *  {
 *    cout << *it << endl;
 *  }
 * \endcode
*/
#define for_(IT,BEG,END) for ( auto IT = BEG, _for_end = END; IT != _for_end; ++IT )

/** Simple C-array iterator
 * \code
 *  const char * defstrings[] = { "",  "a", "default", "two words" };
 *  for_( it, arrayBegin(defstrings), arrayEnd(defstrings) )
 *    cout << *it << endl;
 * \endcode
*/
#define arrayBegin(A) (&A[0])
#define arraySize(A)  (sizeof(A)/sizeof(*A))
#define arrayEnd(A)   (&A[0] + arraySize(A))

/**
 * \code
 * defConstStr( strANY(), "ANY" );
 * std::str str = strANY();
 * \endcode
 */
#define defConstStr(FNC,STR) inline const std::string & FNC { static const std::string val( STR ); return val; }

/** Delete copy ctor and copy assign */
#define NON_COPYABLE(CLASS)			\
  CLASS( const CLASS & ) = delete;		\
  CLASS & operator=( const CLASS & ) = delete

/** Default copy ctor and copy assign */
#define DEFAULT_COPYABLE(CLASS)			\
  CLASS( const CLASS & ) = default;		\
  CLASS & operator=( const CLASS & ) = default

/** Delete move ctor and move assign */
#define NON_MOVABLE(CLASS)			\
  CLASS( CLASS && ) = delete;			\
  CLASS & operator=( CLASS && ) = delete

/** Default move ctor and move assign */
#define DEFAULT_MOVABLE(CLASS)			\
  CLASS( CLASS && ) = default;			\
  CLASS & operator=( CLASS && ) = default

/** Delete copy ctor and copy assign but enable default move */
#define NON_COPYABLE_BUT_MOVE( CLASS ) 		\
  NON_COPYABLE(CLASS);				\
  DEFAULT_MOVABLE(CLASS)

/** Default move ctor and move assign but enable default copy */
#define NON_MOVABLE_BUT_COPY( CLASS ) 		\
  NON_MOVABLE(CLASS);				\
  DEFAULT_COPYABLE(CLASS)


/** Prevent an universal ctor to be chosen as copy ctor.
 * \code
 *  struct FeedStrings
 *  {
 *    template<typename TARG, typename X = disable_use_as_copy_ctor<FeedStrings,TARG>>
 *    FeedStrings( TARG && arg_r )
 *    : _value { std::forward<TARG>( arg_r ) }
 *    {}
 *
 *    // Same with variadic template. Could be chosen as copy_ctor.
 *    template<typename ... Us>
 *    FeedStrings( Us &&... us )
 *    : ...
 *
 *  private:
 *    std::string _value;
 * \endcode
 */
template<typename TBase, typename TDerived>
using disable_use_as_copy_ctor = std::enable_if_t<!std::is_base_of_v<TBase,std::remove_reference_t<TDerived>>>;

///////////////////////////////////////////////////////////////////
namespace zypp
{ /////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////
} // namespace zypp
///////////////////////////////////////////////////////////////////
#endif // ZYPP_BASE_EASY_H
