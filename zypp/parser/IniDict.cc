/*---------------------------------------------------------------------\
|                          ____ _   __ __ ___                          |
|                         |__  / \ / / . \ . \                         |
|                           / / \ V /|  _/  _/                         |
|                          / /__ | | | | | |                           |
|                         /_____||_| |_| |_|                           |
|                                                                      |
\---------------------------------------------------------------------*/
/** \file	zypp/parser/IniDict.cc
 *
*/
#include <iostream>
#include "zypp/base/Logger.h"
#include <map>
#include <string>
#include "zypp/parser/IniDict.h"

using namespace std;
///////////////////////////////////////////////////////////////////
namespace zypp
{ /////////////////////////////////////////////////////////////////
  ///////////////////////////////////////////////////////////////////
  namespace parser
  { /////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////
    //
    //	CLASS NAME : IniDict
    //
    ///////////////////////////////////////////////////////////////////

    ///////////////////////////////////////////////////////////////////
    //
    //	METHOD NAME : IniDict::IniDict
    //	METHOD TYPE : Ctor
    //
    IniDict::IniDict( const InputStream &is, const ProgressData::ReceiverFnc & progress )
    {
      parse(is, progress );
    }

    ///////////////////////////////////////////////////////////////////
    //
    //	METHOD NAME : IniDict::~IniDict
    //	METHOD TYPE : Dtor
    //
    IniDict::~IniDict()
    {}

    void IniDict::consume( const std::string &section )
    {
      // do nothing for now.
    }

    void IniDict::consume( const std::string &section, const std::string &key, const std::string &value )
    {
      //MIL << endl;
      _dict[section][key] = value;
      //MIL << this->size() << endl;
    }


    IniDict::entry_const_iterator IniDict::entriesBegin(const std::string &section) const
    {
      SectionSet::const_iterator secit = _dict.find(section);
      if ( secit == _dict.end() )
      {
        return _empty_map.begin();
      }
      
      return (secit->second).begin();
    }
    
    IniDict::entry_const_iterator IniDict::entriesEnd(const std::string &section) const
    {
      SectionSet::const_iterator secit = _dict.find(section);
      if ( secit == _dict.end() )
      {
        return _empty_map.end();
      }
      
      return (secit->second).end();
    }
    
    
    IniDict::section_const_iterator IniDict::sectionsBegin() const
    {
      return make_map_key_begin( _dict );
    }
    
    IniDict::section_const_iterator IniDict::sectionsEnd() const
    {
      return make_map_key_end( _dict );
    }
    
    void IniDict::insertEntry( const std::string &section,
                               const std::string &key,
                               const std::string &value )
    {
      consume( section, key, value );
    }
      
    void IniDict::deleteSection( const std::string &section )
    {
      _dict.erase(section);
    }
    
    /******************************************************************
    **
    **	FUNCTION NAME : operator<<
    **	FUNCTION TYPE : std::ostream &
    */
    std::ostream & operator<<( std::ostream & str, const IniDict & obj )
    {
      for ( IniDict::section_const_iterator si = obj.sectionsBegin();
            si != obj.sectionsEnd();
            ++si )
      {
        str << "[" << *si << "]" << endl;
        for ( IniDict::entry_const_iterator ei = obj.entriesBegin(*si);
              ei != obj.entriesEnd(*si);
              ++ei )
        {
          str << ei->first << " = " << ei->second << endl;
        }
        str << endl;
      }
      return str;
    }

    /////////////////////////////////////////////////////////////////
  } // namespace parser
  ///////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////
} // namespace zypp
///////////////////////////////////////////////////////////////////
