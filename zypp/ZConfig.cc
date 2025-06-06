/*---------------------------------------------------------------------\
|                          ____ _   __ __ ___                          |
|                         |__  / \ / / . \ . \                         |
|                           / / \ V /|  _/  _/                         |
|                          / /__ | | | | | |                           |
|                         /_____||_| |_| |_|                           |
|                                                                      |
\---------------------------------------------------------------------*/
/** \file	zypp/ZConfig.cc
 *
*/
extern "C"
{
#include <features.h>
#include <sys/utsname.h>
#if __GLIBC_PREREQ (2,16)
#include <sys/auxv.h>	// getauxval for PPC64P7 detection
#endif
#include <unistd.h>
#include <solv/solvversion.h>
}
#include <iostream>
#include <fstream>
#include <optional>
#include <zypp-core/APIConfig.h>
#include <zypp/base/LogTools.h>
#include <zypp/base/IOStream.h>
#include <zypp-core/base/InputStream>
#include <zypp/base/String.h>
#include <zypp/base/Regex.h>

#include <zypp/ZConfig.h>
#include <zypp/ZYppFactory.h>
#include <zypp/PathInfo.h>
#include <zypp-core/parser/IniDict>

#include <zypp/sat/Pool.h>
#include <zypp/sat/detail/PoolImpl.h>

#include <zypp-media/MediaConfig>

using std::endl;
using namespace zypp::filesystem;
using namespace zypp::parser;

#undef ZYPP_BASE_LOGGER_LOGGROUP
#define ZYPP_BASE_LOGGER_LOGGROUP "zconfig"

///////////////////////////////////////////////////////////////////
namespace zypp
{ /////////////////////////////////////////////////////////////////
  /** \addtogroup ZyppConfig Zypp Configuration Options
   *
   * The global \c zypp.conf configuration file is per default located in \c /etc/zypp/.
   * An alternate config file can be set using the environment varaible \c ZYPP_CONF=<PATH>
   * (see \ref zypp-envars).
   *
   * \section ZyppConfig_ZyppConfSample Sample zypp.conf
   * \include ../zypp.conf
   */
  ///////////////////////////////////////////////////////////////////
  namespace
  { /////////////////////////////////////////////////////////////////

    ///////////////////////////////////////////////////////////////////
    // From rpm's lib/rpmrc.c
#   if defined(__linux__) && defined(__x86_64__)
    static inline void cpuid(uint32_t op, uint32_t op2, uint32_t *eax, uint32_t *ebx, uint32_t *ecx, uint32_t *edx)
    {
        asm volatile (
            "cpuid\n"
        : "=a" (*eax), "=b" (*ebx), "=c" (*ecx), "=d" (*edx)
        : "a" (op), "c" (op2));
    }

    /* From gcc's gcc/config/i386/cpuid.h */
    /* Features (%eax == 1) */
    /* %ecx */
    #define bit_SSE3        (1 << 0)
    #define bit_SSSE3       (1 << 9)
    #define bit_FMA         (1 << 12)
    #define bit_CMPXCHG16B  (1 << 13)
    #define bit_SSE4_1      (1 << 19)
    #define bit_SSE4_2      (1 << 20)
    #define bit_MOVBE       (1 << 22)
    #define bit_POPCNT      (1 << 23)
    #define bit_OSXSAVE     (1 << 27)
    #define bit_AVX         (1 << 28)
    #define bit_F16C        (1 << 29)

    /* Extended Features (%eax == 0x80000001) */
    /* %ecx */
    #define bit_LAHF_LM     (1 << 0)
    #define bit_LZCNT       (1 << 5)

    /* Extended Features (%eax == 7) */
    /* %ebx */
    #define bit_BMI         (1 << 3)
    #define bit_AVX2        (1 << 5)
    #define bit_BMI2        (1 << 8)
    #define bit_AVX512F     (1 << 16)
    #define bit_AVX512DQ    (1 << 17)
    #define bit_AVX512CD    (1 << 28)
    #define bit_AVX512BW    (1 << 30)
    #define bit_AVX512VL    (1u << 31)

    static int get_x86_64_level(void)
    {
        int level = 1;

        unsigned int op_1_ecx = 0, op_80000001_ecx = 0, op_7_ebx = 0, unused = 0;
        cpuid(1, 0, &unused, &unused, &op_1_ecx, &unused);
        cpuid(0x80000001, 0, &unused, &unused, &op_80000001_ecx, &unused);
        cpuid(7, 0, &unused, &op_7_ebx, &unused, &unused);

        const unsigned int op_1_ecx_lv2 = bit_SSE3 | bit_SSSE3 | bit_CMPXCHG16B | bit_SSE4_1 | bit_SSE4_2 | bit_POPCNT;
        if ((op_1_ecx & op_1_ecx_lv2) == op_1_ecx_lv2 && (op_80000001_ecx & bit_LAHF_LM))
            level = 2;

        const unsigned int op_1_ecx_lv3 = bit_FMA | bit_MOVBE | bit_OSXSAVE | bit_AVX | bit_F16C;
        const unsigned int op_7_ebx_lv3 = bit_BMI | bit_AVX2 | bit_BMI2;
        if (level == 2 && (op_1_ecx & op_1_ecx_lv3) == op_1_ecx_lv3 && (op_7_ebx & op_7_ebx_lv3) == op_7_ebx_lv3
            && (op_80000001_ecx & bit_LZCNT))
            level = 3;

        const unsigned int op_7_ebx_lv4 = bit_AVX512F | bit_AVX512DQ | bit_AVX512CD | bit_AVX512BW | bit_AVX512VL;
        if (level == 3 && (op_7_ebx & op_7_ebx_lv4) == op_7_ebx_lv4)
            level = 4;

        return level;
    }
#   endif
    ///////////////////////////////////////////////////////////////////

    /** Determine system architecture evaluating \c uname and \c /proc/cpuinfo.
    */
    Arch _autodetectSystemArchitecture()
    {
      struct ::utsname buf;
      if ( ::uname( &buf ) < 0 )
      {
        ERR << "Can't determine system architecture" << endl;
        return Arch_noarch;
      }

      Arch architecture( buf.machine );
      MIL << "Uname architecture is '" << buf.machine << "'" << endl;

      if ( architecture == Arch_x86_64 )
      {
#if     defined(__linux__) && defined(__x86_64__)
        switch ( get_x86_64_level() )
        {
          case 2:
            architecture = Arch_x86_64_v2;
            WAR << "CPU has 'x86_64': architecture upgraded to '" << architecture << "'" << endl;
            break;
          case 3:
            architecture = Arch_x86_64_v3;
            WAR << "CPU has 'x86_64': architecture upgraded to '" << architecture << "'" << endl;
            break;
          case 4:
            architecture = Arch_x86_64_v4;
            WAR << "CPU has 'x86_64': architecture upgraded to '" << architecture << "'" << endl;
            break;
        }
#       endif
      }
      else if ( architecture == Arch_i686 )
      {
        // some CPUs report i686 but dont implement cx8 and cmov
        // check for both flags in /proc/cpuinfo and downgrade
        // to i586 if either is missing (cf bug #18885)
        std::ifstream cpuinfo( "/proc/cpuinfo" );
        if ( cpuinfo )
        {
          for( iostr::EachLine in( cpuinfo ); in; in.next() )
          {
            if ( str::hasPrefix( *in, "flags" ) )
            {
              if (    in->find( "cx8" ) == std::string::npos
                   || in->find( "cmov" ) == std::string::npos )
              {
                architecture = Arch_i586;
                WAR << "CPU lacks 'cx8' or 'cmov': architecture downgraded to '" << architecture << "'" << endl;
              }
              break;
            }
          }
        }
        else
        {
          ERR << "Cant open " << PathInfo("/proc/cpuinfo") << endl;
        }
      }
      else if ( architecture == Arch_sparc || architecture == Arch_sparc64 )
      {
        // Check for sun4[vum] to get the real arch. (bug #566291)
        std::ifstream cpuinfo( "/proc/cpuinfo" );
        if ( cpuinfo )
        {
          for( iostr::EachLine in( cpuinfo ); in; in.next() )
          {
            if ( str::hasPrefix( *in, "type" ) )
            {
              if ( in->find( "sun4v" ) != std::string::npos )
              {
                architecture = ( architecture == Arch_sparc64 ? Arch_sparc64v : Arch_sparcv9v );
                WAR << "CPU has 'sun4v': architecture upgraded to '" << architecture << "'" << endl;
              }
              else if ( in->find( "sun4u" ) != std::string::npos )
              {
                architecture = ( architecture == Arch_sparc64 ? Arch_sparc64 : Arch_sparcv9 );
                WAR << "CPU has 'sun4u': architecture upgraded to '" << architecture << "'" << endl;
              }
              else if ( in->find( "sun4m" ) != std::string::npos )
              {
                architecture = Arch_sparcv8;
                WAR << "CPU has 'sun4m': architecture upgraded to '" << architecture << "'" << endl;
              }
              break;
            }
          }
        }
        else
        {
          ERR << "Cant open " << PathInfo("/proc/cpuinfo") << endl;
        }
      }
      else if ( architecture == Arch_armv8l || architecture == Arch_armv7l || architecture == Arch_armv6l )
      {
        std::ifstream platform( "/etc/rpm/platform" );
        if (platform)
        {
          for( iostr::EachLine in( platform ); in; in.next() )
          {
            if ( str::hasPrefix( *in, "armv8hl-" ) )
            {
              architecture = Arch_armv8hl;
              WAR << "/etc/rpm/platform contains armv8hl-: architecture upgraded to '" << architecture << "'" << endl;
              break;
            }
            if ( str::hasPrefix( *in, "armv7hl-" ) )
            {
              architecture = Arch_armv7hl;
              WAR << "/etc/rpm/platform contains armv7hl-: architecture upgraded to '" << architecture << "'" << endl;
              break;
            }
            if ( str::hasPrefix( *in, "armv6hl-" ) )
            {
              architecture = Arch_armv6hl;
              WAR << "/etc/rpm/platform contains armv6hl-: architecture upgraded to '" << architecture << "'" << endl;
              break;
            }
          }
        }
      }
#if __GLIBC_PREREQ (2,16)
      else if ( architecture == Arch_ppc64 )
      {
        const char * platform = (const char *)getauxval( AT_PLATFORM );
        int powerlvl = 0;
        if ( platform && sscanf( platform, "power%d", &powerlvl ) == 1 && powerlvl > 6 )
          architecture = Arch_ppc64p7;
      }
#endif
      return architecture;
    }

     /** The locale to be used for texts and messages.
     *
     * For the encoding to be used the preference is
     *
     *    LC_ALL, LC_CTYPE, LANG
     *
     * For the language of the messages to be used, the preference is
     *
     *    LANGUAGE, LC_ALL, LC_MESSAGES, LANG
     *
     * Note that LANGUAGE can contain more than one locale name, it can be
     * a list of locale names like for example
     *
     *    LANGUAGE=ja_JP.UTF-8:de_DE.UTF-8:fr_FR.UTF-8

     * \todo Support dynamic fallbacklists defined by LANGUAGE
     */
    Locale _autodetectTextLocale()
    {
      Locale ret( Locale::enCode );
      const char * envlist[] = { "LC_ALL", "LC_MESSAGES", "LANG", NULL };
      for ( const char ** envvar = envlist; *envvar; ++envvar )
      {
        const char * envlang = getenv( *envvar );
        if ( envlang )
        {
          std::string envstr( envlang );
          if ( envstr != "POSIX" && envstr != "C" )
          {
            Locale lang( envstr );
            if ( lang )
            {
              MIL << "Found " << *envvar << "=" << envstr << endl;
              ret = lang;
              break;
            }
          }
        }
      }
      MIL << "Default text locale is '" << ret << "'" << endl;
#warning HACK AROUND BOOST_TEST_CATCH_SYSTEM_ERRORS
      setenv( "BOOST_TEST_CATCH_SYSTEM_ERRORS", "no", 1 );
      return ret;
    }

    inline Pathname _autodetectZyppConfPath()
    {
      const char *env_confpath = getenv( "ZYPP_CONF" );
      return env_confpath ? env_confpath : "/etc/zypp/zypp.conf";
    }

   /////////////////////////////////////////////////////////////////
  } // namespace zypp
  ///////////////////////////////////////////////////////////////////

  /** Mutable option. */
  template<class Tp>
      struct Option
      {
        using value_type = Tp;

        /** No default ctor, explicit initialisation! */
        Option( value_type initial_r )
          : _val( std::move(initial_r) )
        {}

        Option & operator=( value_type newval_r )
        { set( std::move(newval_r) ); return *this; }

        /** Get the value.  */
        const value_type & get() const
        { return _val; }

        /** Autoconversion to value_type.  */
        operator const value_type &() const
        { return _val; }

        /** Set a new value.  */
        void set( value_type newval_r )
        { _val = std::move(newval_r); }

        private:
          value_type _val;
      };

  /** Mutable option with initial value also remembering a config value. */
  template<class Tp>
      struct DefaultOption : public Option<Tp>
      {
        using value_type = Tp;
        using option_type = Option<Tp>;

        explicit DefaultOption( value_type initial_r )
        : Option<Tp>( initial_r )
        , _default( std::move(initial_r) )
        {}

        DefaultOption & operator=( value_type newval_r )
        { this->set( std::move(newval_r) ); return *this; }

        /** Reset value to the current default. */
        void restoreToDefault()
        { this->set( _default.get() ); }

        /** Reset value to a new default. */
        void restoreToDefault( value_type newval_r )
        { setDefault( std::move(newval_r) ); restoreToDefault(); }

        /** Get the current default value. */
        const value_type & getDefault() const
        { return _default.get(); }

        /** Set a new default value. */
        void setDefault( value_type newval_r )
        { _default.set( std::move(newval_r) ); }

        private:
          option_type _default;
      };

  ///////////////////////////////////////////////////////////////////
  //
  //	CLASS NAME : ZConfig::Impl
  //
  /** ZConfig implementation.
   * \todo Enrich section and entry definition by some comment
   * (including the default setting and provide some method to
   * write this into a sample zypp.conf.
  */
  class ZConfig::Impl
  {
    using MultiversionSpec = std::set<std::string>;

    /// Settings that follow a changed Target
    struct TargetDefaults
    {
      TargetDefaults()
      : solver_focus                        ( ResolverFocus::Default )
      , solver_onlyRequires                 ( false )
      , solver_allowVendorChange            ( false )
      , solver_dupAllowDowngrade            ( true )
      , solver_dupAllowNameChange           ( true )
      , solver_dupAllowArchChange           ( true )
      , solver_dupAllowVendorChange         ( false )
      , solver_cleandepsOnRemove            ( false )
      , solver_upgradeTestcasesToKeep       ( 2 )
      , solverUpgradeRemoveDroppedPackages  ( true )
      {}

      bool consume( const std::string & entry, const std::string & value )
      {
        if ( entry == "solver.focus" )
        {
          fromString( value, solver_focus );
        }
        else if ( entry == "solver.onlyRequires" )
        {
          solver_onlyRequires.set( str::strToBool( value, solver_onlyRequires ) );
        }
        else if ( entry == "solver.allowVendorChange" )
        {
          solver_allowVendorChange.set( str::strToBool( value, solver_allowVendorChange ) );
        }
        else if ( entry == "solver.dupAllowDowngrade" )
        {
          solver_dupAllowDowngrade.set( str::strToBool( value, solver_dupAllowDowngrade ) );
        }
        else if ( entry == "solver.dupAllowNameChange" )
        {
          solver_dupAllowNameChange.set( str::strToBool( value, solver_dupAllowNameChange ) );
        }
        else if ( entry == "solver.dupAllowArchChange" )
        {
          solver_dupAllowArchChange.set( str::strToBool( value, solver_dupAllowArchChange ) );
        }
        else if ( entry == "solver.dupAllowVendorChange" )
        {
          solver_dupAllowVendorChange.set( str::strToBool( value, solver_dupAllowVendorChange ) );
        }
        else if ( entry == "solver.cleandepsOnRemove" )
        {
          solver_cleandepsOnRemove.set( str::strToBool( value, solver_cleandepsOnRemove ) );
        }
        else if ( entry == "solver.upgradeTestcasesToKeep" )
        {
          solver_upgradeTestcasesToKeep.set( str::strtonum<unsigned>( value ) );
        }
        else if ( entry == "solver.upgradeRemoveDroppedPackages" )
        {
          solverUpgradeRemoveDroppedPackages.restoreToDefault( str::strToBool( value, solverUpgradeRemoveDroppedPackages.getDefault() ) );
        }
        else
          return false;

        return true;
      }

      ResolverFocus       solver_focus;
      Option<bool>        solver_onlyRequires;
      Option<bool>        solver_allowVendorChange;
      Option<bool>        solver_dupAllowDowngrade;
      Option<bool>        solver_dupAllowNameChange;
      Option<bool>        solver_dupAllowArchChange;
      Option<bool>        solver_dupAllowVendorChange;
      Option<bool>        solver_cleandepsOnRemove;
      Option<unsigned>    solver_upgradeTestcasesToKeep;
      DefaultOption<bool> solverUpgradeRemoveDroppedPackages;
    };

    public:
      Impl()
        : _parsedZyppConf         	( _autodetectZyppConfPath() )
        , cfg_arch                	( defaultSystemArchitecture() )
        , cfg_textLocale          	( defaultTextLocale() )
        , cfg_cache_path		{ "/var/cache/zypp" }
        , cfg_metadata_path		{ "" }	// empty - follows cfg_cache_path
        , cfg_solvfiles_path		{ "" }	// empty - follows cfg_cache_path
        , cfg_packages_path		{ "" }	// empty - follows cfg_cache_path
        , updateMessagesNotify		( "" )
        , repo_add_probe          	( false )
        , repo_refresh_delay      	( 10 )
        , repoLabelIsAlias              ( false )
        , download_use_deltarpm   	( APIConfig(LIBZYPP_CONFIG_USE_DELTARPM_BY_DEFAULT) )
        , download_use_deltarpm_always  ( false )
        , download_media_prefer_download( true )
        , download_mediaMountdir	( "/var/adm/mount" )
        , commit_downloadMode		( DownloadDefault )
        , gpgCheck			( true )
        , repoGpgCheck			( indeterminate )
        , pkgGpgCheck			( indeterminate )
        , apply_locks_file		( true )
        , pluginsPath			( "/usr/lib/zypp/plugins" )
        , geoipEnabled ( true )
        , geoipHosts { "download.opensuse.org" }
      {
        MIL << "libzypp: " LIBZYPP_VERSION_STRING << " (" << LIBZYPP_CODESTREAM << ")" << endl;
        if ( PathInfo(_parsedZyppConf).isExist() )
        {
          parser::IniDict dict( _parsedZyppConf );
          for ( IniDict::section_const_iterator sit = dict.sectionsBegin();
                sit != dict.sectionsEnd();
                ++sit )
          {
            const std::string& section(*sit);
            //MIL << section << endl;
            for ( IniDict::entry_const_iterator it = dict.entriesBegin(*sit);
                  it != dict.entriesEnd(*sit);
                  ++it )
            {
              std::string entry(it->first);
              std::string value(it->second);

              if ( _mediaConf.setConfigValue( section, entry, value ) )
                continue;

              //DBG << (*it).first << "=" << (*it).second << endl;
              if ( section == "main" )
              {
                if ( _initialTargetDefaults.consume( entry, value ) )
                  continue;

                if ( entry == "lock_timeout" ) {
                  str::strtonum( value, cfg_lockTimeout );
                }
                else if ( entry == "arch" )
                {
                  Arch carch( value );
                  if ( carch != cfg_arch )
                  {
                    WAR << "Overriding system architecture (" << cfg_arch << "): " << carch << endl;
                    cfg_arch = carch;
                  }
                }
                else if ( entry == "cachedir" )
                {
                  cfg_cache_path.restoreToDefault( value );
                }
                else if ( entry == "metadatadir" )
                {
                  cfg_metadata_path.restoreToDefault( value );
                }
                else if ( entry == "solvfilesdir" )
                {
                  cfg_solvfiles_path.restoreToDefault( value );
                }
                else if ( entry == "packagesdir" )
                {
                  cfg_packages_path.restoreToDefault( value );
                }
                else if ( entry == "configdir" )
                {
                  cfg_config_path = Pathname(value);
                }
                else if ( entry == "reposdir" )
                {
                  cfg_known_repos_path = Pathname(value);
                }
                else if ( entry == "servicesdir" )
                {
                  cfg_known_services_path = Pathname(value);
                }
                else if ( entry == "varsdir" )
                {
                  cfg_vars_path = Pathname(value);
                }
                else if ( entry == "repo.add.probe" )
                {
                  repo_add_probe = str::strToBool( value, repo_add_probe );
                }
                else if ( entry == "repo.refresh.delay" )
                {
                  str::strtonum(value, repo_refresh_delay);
                }
                else if ( entry == "repo.refresh.locales" )
                {
                  std::vector<std::string> tmp;
                  str::split( value, back_inserter( tmp ), ", \t" );

                  boost::function<Locale(const std::string &)> transform(
                    [](const std::string & str_r)->Locale{ return Locale(str_r); }
                  );
                  repoRefreshLocales.insert( make_transform_iterator( tmp.begin(), transform ),
                                             make_transform_iterator( tmp.end(), transform ) );
                }
                else if ( entry == "download.use_deltarpm" )
                {
                  download_use_deltarpm = str::strToBool( value, download_use_deltarpm );
                }
                else if ( entry == "download.use_deltarpm.always" )
                {
                  download_use_deltarpm_always = str::strToBool( value, download_use_deltarpm_always );
                }
                else if ( entry == "download.media_preference" )
                {
                  download_media_prefer_download.restoreToDefault( str::compareCI( value, "volatile" ) != 0 );
                }
                else if ( entry == "download.media_mountdir" )
                {
                  download_mediaMountdir.restoreToDefault( Pathname(value) );
                }
                else if ( entry == "download.use_geoip_mirror") {
                  geoipEnabled = str::strToBool( value, geoipEnabled );
                }
                else if ( entry == "commit.downloadMode" )
                {
                  commit_downloadMode.set( deserializeDownloadMode( value ) );
                }
                else if ( entry == "gpgcheck" )
                {
                  gpgCheck.restoreToDefault( str::strToBool( value, gpgCheck ) );
                }
                else if ( entry == "repo_gpgcheck" )
                {
                  repoGpgCheck.restoreToDefault( str::strToTriBool( value ) );
                }
                else if ( entry == "pkg_gpgcheck" )
                {
                  pkgGpgCheck.restoreToDefault( str::strToTriBool( value ) );
                }
                else if ( entry == "vendordir" )
                {
                  cfg_vendor_path = Pathname(value);
                }
                else if ( entry == "multiversiondir" )
                {
                  cfg_multiversion_path = Pathname(value);
                }
                else if ( entry == "multiversion.kernels" )
                {
                  cfg_kernel_keep_spec = value;
                }
                else if ( entry == "solver.checkSystemFile" )
                {
                  solver_checkSystemFile = Pathname(value);
                }
                else if ( entry == "solver.checkSystemFileDir" )
                {
                  solver_checkSystemFileDir = Pathname(value);
                }
                else if ( entry == "multiversion" )
                {
                  MultiversionSpec & defSpec( _multiversionMap.getDefaultSpec() );
                  str::splitEscaped( value, std::inserter( defSpec, defSpec.end() ), ", \t" );
                }
                else if ( entry == "locksfile.path" )
                {
                  locks_file = Pathname(value);
                }
                else if ( entry == "locksfile.apply" )
                {
                  apply_locks_file = str::strToBool( value, apply_locks_file );
                }
                else if ( entry == "update.datadir" )
                {
                  // ignore, this is a constant anyway and should not be user configurabe
                  // update_data_path = Pathname(value);
                }
                else if ( entry == "update.scriptsdir" )
                {
                  // ignore, this is a constant anyway and should not be user configurabe
                  // update_scripts_path = Pathname(value);
                }
                else if ( entry == "update.messagessdir" )
                {
                  // ignore, this is a constant anyway and should not be user configurabe
                  // update_messages_path = Pathname(value);
                }
                else if ( entry == "update.messages.notify" )
                {
                  updateMessagesNotify.set( value );
                }
                else if ( entry == "rpm.install.excludedocs" )
                {
                  rpmInstallFlags.setFlag( target::rpm::RPMINST_EXCLUDEDOCS,
                                           str::strToBool( value, false ) );
                }
                else if ( entry == "history.logfile" )
                {
                  history_log_path = Pathname(value);
                }
                else if ( entry == "ZYPP_SINGLE_RPMTRANS" || entry == "techpreview.ZYPP_SINGLE_RPMTRANS" )
                {
                  DBG << "ZYPP_SINGLE_RPMTRANS=" << value << endl;
                  ::setenv( "ZYPP_SINGLE_RPMTRANS", value.c_str(), 0 );
                }
                else if ( entry == "techpreview.ZYPP_MEDIANETWORK" )
                {
                  DBG << "techpreview.ZYPP_MEDIANETWORK=" << value << endl;
                  ::setenv( "ZYPP_MEDIANETWORK", value.c_str(), 1 );
                }
              }
            }
          }
        }
        else
        {
          MIL << _parsedZyppConf << " not found, using defaults instead." << endl;
          _parsedZyppConf = _parsedZyppConf.extend( " (NOT FOUND)" );
        }

        // legacy:
        if ( getenv( "ZYPP_TESTSUITE_FAKE_ARCH" ) )
        {
          Arch carch( getenv( "ZYPP_TESTSUITE_FAKE_ARCH" ) );
          if ( carch != cfg_arch )
          {
            WAR << "ZYPP_TESTSUITE_FAKE_ARCH: Overriding system architecture (" << cfg_arch << "): " << carch << endl;
            cfg_arch = carch;
          }
        }
        MIL << "ZConfig singleton created." << endl;
      }

      Impl(const Impl &) = delete;
      Impl(Impl &&) = delete;
      Impl &operator=(const Impl &) = delete;
      Impl &operator=(Impl &&) = delete;
      ~Impl() {}

      /** bsc#1237044: Provide \ref announceSystemRoot to allow commands
       * using --root without launching a Target. An announced SystemRoot
       * is used rather than / when no Target is up. This enables sub
       * components (e.g. VarReplacer) to refer to the right context.
       *
       * Setting or re-setting a Target clears any announced SystemRoot.
       */
      Pathname _autodetectSystemRoot() const
      {
        Target_Ptr target( getZYpp()->getTarget() );
        return target ? target->root() : _announced_root_path;
      }

      void notifyTargetChanged()
      {
        _announced_root_path = Pathname();  // first of all reset any previously _announced_root_path

        Pathname newRoot { _autodetectSystemRoot() };
        MIL << "notifyTargetChanged (" << newRoot << ")" << endl;

        if ( newRoot.emptyOrRoot() ) {
          _currentTargetDefaults.reset(); // to initial settigns from /
        }
        else {
          _currentTargetDefaults = TargetDefaults();

          Pathname newConf { newRoot/_autodetectZyppConfPath() };
          if ( PathInfo(newConf).isExist() ) {
            parser::IniDict dict( newConf );
            for ( const auto & [entry,value] : dict.entries( "main" ) ) {
              (*_currentTargetDefaults).consume( entry, value );
            }
          }
          else {
            MIL << _parsedZyppConf << " not found, using defaults." << endl;
          }
        }
      }

    public:
    /** Remember any parsed zypp.conf. */
    Pathname _parsedZyppConf;
    Pathname _announced_root_path;

    long cfg_lockTimeout = 0; // signed!

    Arch     cfg_arch;
    Locale   cfg_textLocale;

    DefaultOption<Pathname> cfg_cache_path;	// Settings from the config file are also remembered
    DefaultOption<Pathname> cfg_metadata_path;	// 'default'. Cleanup in RepoManager e.g needs to tell
    DefaultOption<Pathname> cfg_solvfiles_path;	// whether settings in effect are config values or
    DefaultOption<Pathname> cfg_packages_path;	// custom settings applied vie set...Path().

    Pathname cfg_config_path;
    Pathname cfg_known_repos_path;
    Pathname cfg_known_services_path;
    Pathname cfg_vars_path;
    Pathname cfg_repo_mgr_root_path;

    Pathname cfg_vendor_path;
    Pathname cfg_multiversion_path;
    std::string cfg_kernel_keep_spec;
    Pathname locks_file;

    DefaultOption<std::string> updateMessagesNotify;

    bool	repo_add_probe;
    unsigned	repo_refresh_delay;
    LocaleSet	repoRefreshLocales;
    bool	repoLabelIsAlias;

    bool download_use_deltarpm;
    bool download_use_deltarpm_always;
    DefaultOption<bool> download_media_prefer_download;
    DefaultOption<Pathname> download_mediaMountdir;

    Option<DownloadMode> commit_downloadMode;

    DefaultOption<bool>		gpgCheck;
    DefaultOption<TriBool>	repoGpgCheck;
    DefaultOption<TriBool>	pkgGpgCheck;

    Pathname solver_checkSystemFile;
    Pathname solver_checkSystemFileDir;

    MultiversionSpec &		multiversion()		{ return getMultiversion(); }
    const MultiversionSpec &	multiversion() const	{ return getMultiversion(); }

    bool apply_locks_file;

    target::rpm::RpmInstFlags rpmInstallFlags;

    Pathname history_log_path;

    std::string userData;

    Option<Pathname> pluginsPath;

    bool geoipEnabled;

    std::vector<std::string> geoipHosts;

    /* Other config singleton instances */
    MediaConfig &_mediaConf = MediaConfig::instance();


  public:
    const TargetDefaults & targetDefaults() const { return _currentTargetDefaults ? *_currentTargetDefaults : _initialTargetDefaults; }
    TargetDefaults &       targetDefaults()       { return _currentTargetDefaults ? *_currentTargetDefaults : _initialTargetDefaults; }
  private:
    TargetDefaults                _initialTargetDefaults; ///< Initial TargetDefaults from /
    std::optional<TargetDefaults> _currentTargetDefaults; ///< TargetDefaults while --root

  private:
    // HACK for bnc#906096: let pool re-evaluate multiversion spec
    // if target root changes. ZConfig returns data sensitive to
    // current target root.
    // TODO Actually we'd need to scan the target systems zypp.conf and
    // overlay all system specific values.
    struct MultiversionMap
    {
      using SpecMap = std::map<Pathname, MultiversionSpec>;

      MultiversionSpec & getSpec( Pathname root_r, const Impl & zConfImpl_r )	// from system at root
      {
        // _specMap[]     - the plain zypp.conf value
        // _specMap[/]    - combine [] and multiversion.d scan
        // _specMap[root] - scan root/zypp.conf and root/multiversion.d

        if ( root_r.empty() )
          root_r = "/";
        bool cacheHit = _specMap.count( root_r );
        MultiversionSpec & ret( _specMap[root_r] );	// creates new entry on the fly

        if ( ! cacheHit )
        {
          // bsc#1193488: If no (/root)/.../zypp.conf exists use the default zypp.conf
          // multiversion settings. It is a legacy that the packaged multiversion setting
          // in zypp.conf (the kernel) may differ from the builtin default (empty).
          // But we want a missing config to behave similar to the default one, otherwise
          // a bare metal install easily runs into trouble.
          if ( root_r == "/" || scanConfAt( root_r, ret, zConfImpl_r ) == 0 )
            ret = _specMap[Pathname()];
          scanDirAt( root_r, ret, zConfImpl_r );	// add multiversion.d at root_r
          using zypp::operator<<;
          MIL << "MultiversionSpec '" << root_r << "' = " << ret << endl;
        }
        return ret;
      }

      MultiversionSpec & getDefaultSpec()	// Spec from zypp.conf parsing; called before any getSpec
      {	return _specMap[Pathname()]; }

    private:
      int scanConfAt( const Pathname& root_r, MultiversionSpec & spec_r, const Impl & zConfImpl_r )
      {
        static const str::regex rx( "^multiversion *= *(.*)" );
        str::smatch what;
        return iostr::simpleParseFile( InputStream( Pathname::assertprefix( root_r, _autodetectZyppConfPath() ) ),
                                [&]( int num_r, std::string line_r )->bool
                                {
                                  if ( line_r[0] == 'm' && str::regex_match( line_r, what, rx ) )
                                  {
                                    str::splitEscaped( what[1], std::inserter( spec_r, spec_r.end() ), ", \t" );
                                    return false;	// stop after match
                                  }
                                  return true;
                                } );
      }

      void scanDirAt( const Pathname& root_r, MultiversionSpec & spec_r, const Impl & zConfImpl_r )
      {
        // NOTE:  Actually we'd need to scan and use the root_r! zypp.conf values.
        Pathname multiversionDir( zConfImpl_r.cfg_multiversion_path );
        if ( multiversionDir.empty() )
          multiversionDir = ( zConfImpl_r.cfg_config_path.empty()
                            ? Pathname("/etc/zypp")
                            : zConfImpl_r.cfg_config_path ) / "multiversion.d";

        filesystem::dirForEach( Pathname::assertprefix( root_r, multiversionDir ),
                                [&spec_r]( const Pathname & dir_r, const char *const & name_r )->bool
                                {
                                  MIL << "Parsing " << dir_r/name_r << endl;
                                  iostr::simpleParseFile( InputStream( dir_r/name_r ),
                                                          [&spec_r]( int num_r, std::string line_r )->bool
                                                          {
                                                            DBG << "  found " << line_r << endl;
                                                            spec_r.insert( std::move(line_r) );
                                                            return true;
                                                          } );
                                  return true;
                                } );
      }

    private:
      SpecMap _specMap;
    };

    MultiversionSpec & getMultiversion() const
    { return _multiversionMap.getSpec( _autodetectSystemRoot(), *this ); }

    mutable MultiversionMap _multiversionMap;
  };
  ///////////////////////////////////////////////////////////////////

  ///////////////////////////////////////////////////////////////////
  //
  //	METHOD NAME : ZConfig::instance
  //	METHOD TYPE : ZConfig &
  //
  ZConfig & ZConfig::instance()
  {
    static ZConfig _instance; // The singleton
    return _instance;
  }

  ///////////////////////////////////////////////////////////////////
  //
  //	METHOD NAME : ZConfig::ZConfig
  //	METHOD TYPE : Ctor
  //
  ZConfig::ZConfig()
  : _pimpl( new Impl )
  {
    about( MIL );
  }

  ///////////////////////////////////////////////////////////////////
  //
  //	METHOD NAME : ZConfig::~ZConfig
  //	METHOD TYPE : Dtor
  //
  ZConfig::~ZConfig( )
  {}

  long ZConfig::lockTimeout() const
  {
    const char * env = getenv("ZYPP_LOCK_TIMEOUT");
    if ( env ) {
      return str::strtonum<long>( env );
    }
    return _pimpl->cfg_lockTimeout;
  }

  void ZConfig::notifyTargetChanged()
  { return _pimpl->notifyTargetChanged(); }

  Pathname ZConfig::systemRoot() const
  { return _pimpl->_autodetectSystemRoot(); }

  Pathname ZConfig::repoManagerRoot() const
  {
    return ( _pimpl->cfg_repo_mgr_root_path.empty()
             ? systemRoot() : _pimpl->cfg_repo_mgr_root_path );
  }

  void ZConfig::setRepoManagerRoot(const zypp::filesystem::Pathname &root)
  { _pimpl->cfg_repo_mgr_root_path = root; }

  void ZConfig::announceSystemRoot( const Pathname & root_r )
  { _pimpl->_announced_root_path = root_r; }

  ///////////////////////////////////////////////////////////////////
  //
  // system architecture
  //
  ///////////////////////////////////////////////////////////////////

  Arch ZConfig::defaultSystemArchitecture()
  {
    static Arch _val( _autodetectSystemArchitecture() );
    return _val;
  }

  Arch ZConfig::systemArchitecture() const
  { return _pimpl->cfg_arch; }

  void ZConfig::setSystemArchitecture( const Arch & arch_r )
  {
    if ( arch_r != _pimpl->cfg_arch )
    {
      WAR << "Overriding system architecture (" << _pimpl->cfg_arch << "): " << arch_r << endl;
      _pimpl->cfg_arch = arch_r;
    }
  }

  ///////////////////////////////////////////////////////////////////
  //
  // text locale
  //
  ///////////////////////////////////////////////////////////////////

  Locale ZConfig::defaultTextLocale()
  {
    static Locale _val( _autodetectTextLocale() );
    return _val;
  }

  Locale ZConfig::textLocale() const
  { return _pimpl->cfg_textLocale; }

  void ZConfig::setTextLocale( const Locale & locale_r )
  {
    if ( locale_r != _pimpl->cfg_textLocale )
    {
      WAR << "Overriding text locale (" << _pimpl->cfg_textLocale << "): " << locale_r << endl;
      _pimpl->cfg_textLocale = locale_r;
      // Propagate changes
      sat::Pool::instance().setTextLocale( locale_r );
    }
  }

  ///////////////////////////////////////////////////////////////////
  // user data
  ///////////////////////////////////////////////////////////////////

  bool ZConfig::hasUserData() const
  { return !_pimpl->userData.empty(); }

  std::string ZConfig::userData() const
  { return _pimpl->userData; }

  bool ZConfig::setUserData( const std::string & str_r )
  {
    for_( ch, str_r.begin(), str_r.end() )
    {
      if ( *ch < ' ' && *ch != '\t' )
      {
        ERR << "New user data string rejectded: char " << (int)*ch << " at position " <<  (ch - str_r.begin()) << endl;
        return false;
      }
    }
    MIL << "Set user data string to '" << str_r << "'" << endl;
    _pimpl->userData = str_r;
    return true;
  }

  ///////////////////////////////////////////////////////////////////

  Pathname ZConfig::repoCachePath() const
  {
    return ( _pimpl->cfg_cache_path.get().empty()
             ? Pathname("/var/cache/zypp") : _pimpl->cfg_cache_path.get() );
  }

  Pathname ZConfig::pubkeyCachePath() const
  {
    return repoCachePath()/"pubkeys";
  }

  void ZConfig::setRepoCachePath(const zypp::filesystem::Pathname &path_r)
  {
    _pimpl->cfg_cache_path = path_r;
  }

  Pathname ZConfig::repoMetadataPath() const
  {
    return ( _pimpl->cfg_metadata_path.get().empty()
        ? (repoCachePath()/"raw") : _pimpl->cfg_metadata_path.get() );
  }

  void ZConfig::setRepoMetadataPath(const zypp::filesystem::Pathname &path_r)
  {
    _pimpl->cfg_metadata_path = path_r;
  }

  Pathname ZConfig::repoSolvfilesPath() const
  {
    return ( _pimpl->cfg_solvfiles_path.get().empty()
        ? (repoCachePath()/"solv") : _pimpl->cfg_solvfiles_path.get() );
  }

  void ZConfig::setRepoSolvfilesPath(const zypp::filesystem::Pathname &path_r)
  {
    _pimpl->cfg_solvfiles_path = path_r;
  }

  Pathname ZConfig::repoPackagesPath() const
  {
    return ( _pimpl->cfg_packages_path.get().empty()
        ? (repoCachePath()/"packages") : _pimpl->cfg_packages_path.get() );
  }

  void ZConfig::setRepoPackagesPath(const zypp::filesystem::Pathname &path_r)
  {
    _pimpl->cfg_packages_path = path_r;
  }

  Pathname ZConfig::builtinRepoCachePath() const
  { return _pimpl->cfg_cache_path.getDefault().empty() ? Pathname("/var/cache/zypp") : _pimpl->cfg_cache_path.getDefault(); }

  Pathname ZConfig::builtinRepoMetadataPath() const
  { return _pimpl->cfg_metadata_path.getDefault().empty() ? (builtinRepoCachePath()/"raw") : _pimpl->cfg_metadata_path.getDefault(); }

  Pathname ZConfig::builtinRepoSolvfilesPath() const
  { return _pimpl->cfg_solvfiles_path.getDefault().empty() ? (builtinRepoCachePath()/"solv") : _pimpl->cfg_solvfiles_path.getDefault(); }

  Pathname ZConfig::builtinRepoPackagesPath() const
  { return _pimpl->cfg_packages_path.getDefault().empty() ? (builtinRepoCachePath()/"packages") : _pimpl->cfg_packages_path.getDefault(); }

  ///////////////////////////////////////////////////////////////////

  Pathname ZConfig::configPath() const
  {
    return ( _pimpl->cfg_config_path.empty()
        ? Pathname("/etc/zypp") : _pimpl->cfg_config_path );
  }

  Pathname ZConfig::knownReposPath() const
  {
    return ( _pimpl->cfg_known_repos_path.empty()
        ? (configPath()/"repos.d") : _pimpl->cfg_known_repos_path );
  }

  Pathname ZConfig::knownServicesPath() const
  {
    return ( _pimpl->cfg_known_services_path.empty()
        ? (configPath()/"services.d") : _pimpl->cfg_known_services_path );
  }

  Pathname ZConfig::needrebootFile() const
  { return configPath()/"needreboot"; }

  Pathname ZConfig::needrebootPath() const
  { return configPath()/"needreboot.d"; }

  void ZConfig::setGeoipEnabled( bool enable )
  { _pimpl->geoipEnabled = enable; }

  bool ZConfig::geoipEnabled () const
  { return _pimpl->geoipEnabled; }

  Pathname ZConfig::geoipCachePath() const
  { return builtinRepoCachePath()/"geoip.d"; }

  const std::vector<std::string> ZConfig::geoipHostnames () const
  { return _pimpl->geoipHosts; }

  Pathname ZConfig::varsPath() const
  {
    return ( _pimpl->cfg_vars_path.empty()
        ? (configPath()/"vars.d") : _pimpl->cfg_vars_path );
  }

  Pathname ZConfig::vendorPath() const
  {
    return ( _pimpl->cfg_vendor_path.empty()
        ? (configPath()/"vendors.d") : _pimpl->cfg_vendor_path );
  }

  Pathname ZConfig::locksFile() const
  {
    return ( _pimpl->locks_file.empty()
        ? (configPath()/"locks") : _pimpl->locks_file );
  }

  ///////////////////////////////////////////////////////////////////

  bool ZConfig::repo_add_probe() const
  { return _pimpl->repo_add_probe; }

  unsigned ZConfig::repo_refresh_delay() const
  { return _pimpl->repo_refresh_delay; }

  LocaleSet ZConfig::repoRefreshLocales() const
  { return _pimpl->repoRefreshLocales.empty() ? Target::requestedLocales("") :_pimpl->repoRefreshLocales; }

  bool ZConfig::repoLabelIsAlias() const
  { return _pimpl->repoLabelIsAlias; }

  void ZConfig::repoLabelIsAlias( bool yesno_r )
  { _pimpl->repoLabelIsAlias = yesno_r; }

  bool ZConfig::download_use_deltarpm() const
  { return _pimpl->download_use_deltarpm; }

  bool ZConfig::download_use_deltarpm_always() const
  { return download_use_deltarpm() && _pimpl->download_use_deltarpm_always; }

  bool ZConfig::download_media_prefer_download() const
  { return _pimpl->download_media_prefer_download; }

  void ZConfig::set_download_media_prefer_download( bool yesno_r )
  { _pimpl->download_media_prefer_download.set( yesno_r ); }

  void ZConfig::set_default_download_media_prefer_download()
  { _pimpl->download_media_prefer_download.restoreToDefault(); }

  long ZConfig::download_max_concurrent_connections() const
  { return _pimpl->_mediaConf.download_max_concurrent_connections(); }

  long ZConfig::download_min_download_speed() const
  { return _pimpl->_mediaConf.download_min_download_speed(); }

  long ZConfig::download_max_download_speed() const
  { return _pimpl->_mediaConf.download_max_download_speed(); }

  long ZConfig::download_max_silent_tries() const
  { return _pimpl->_mediaConf.download_max_silent_tries(); }

  long ZConfig::download_transfer_timeout() const
  { return _pimpl->_mediaConf.download_transfer_timeout(); }

  Pathname ZConfig::download_mediaMountdir() const		{ return _pimpl->download_mediaMountdir; }
  void ZConfig::set_download_mediaMountdir( Pathname newval_r )	{ _pimpl->download_mediaMountdir.set( std::move(newval_r) ); }
  void ZConfig::set_default_download_mediaMountdir()		{ _pimpl->download_mediaMountdir.restoreToDefault(); }

  DownloadMode ZConfig::commit_downloadMode() const
  { return _pimpl->commit_downloadMode; }


  bool ZConfig::gpgCheck() const			{ return _pimpl->gpgCheck; }
  TriBool ZConfig::repoGpgCheck() const			{ return _pimpl->repoGpgCheck; }
  TriBool ZConfig::pkgGpgCheck() const			{ return _pimpl->pkgGpgCheck; }

  void ZConfig::setGpgCheck( bool val_r )		{ _pimpl->gpgCheck.set( val_r ); }
  void ZConfig::setRepoGpgCheck( TriBool val_r )	{ _pimpl->repoGpgCheck.set( val_r ); }
  void ZConfig::setPkgGpgCheck( TriBool val_r )		{ _pimpl->pkgGpgCheck.set( val_r ); }

  void ZConfig::resetGpgCheck()				{ _pimpl->gpgCheck.restoreToDefault(); }
  void ZConfig::resetRepoGpgCheck()			{ _pimpl->repoGpgCheck.restoreToDefault(); }
  void ZConfig::resetPkgGpgCheck()			{ _pimpl->pkgGpgCheck.restoreToDefault(); }


  ResolverFocus ZConfig::solver_focus() const           { return _pimpl->targetDefaults().solver_focus; }
  bool ZConfig::solver_onlyRequires() const             { return _pimpl->targetDefaults().solver_onlyRequires; }
  bool ZConfig::solver_allowVendorChange() const        { return _pimpl->targetDefaults().solver_allowVendorChange; }
  bool ZConfig::solver_dupAllowDowngrade() const        { return _pimpl->targetDefaults().solver_dupAllowDowngrade; }
  bool ZConfig::solver_dupAllowNameChange() const       { return _pimpl->targetDefaults().solver_dupAllowNameChange; }
  bool ZConfig::solver_dupAllowArchChange() const       { return _pimpl->targetDefaults().solver_dupAllowArchChange; }
  bool ZConfig::solver_dupAllowVendorChange() const     { return _pimpl->targetDefaults().solver_dupAllowVendorChange; }
  bool ZConfig::solver_cleandepsOnRemove() const        { return _pimpl->targetDefaults().solver_cleandepsOnRemove; }
  unsigned ZConfig::solver_upgradeTestcasesToKeep() const { return _pimpl->targetDefaults().solver_upgradeTestcasesToKeep; }

  bool ZConfig::solverUpgradeRemoveDroppedPackages() const          { return _pimpl->targetDefaults().solverUpgradeRemoveDroppedPackages; }
  void ZConfig::setSolverUpgradeRemoveDroppedPackages( bool val_r ) { _pimpl->targetDefaults().solverUpgradeRemoveDroppedPackages.set( val_r ); }
  void ZConfig::resetSolverUpgradeRemoveDroppedPackages()           { _pimpl->targetDefaults().solverUpgradeRemoveDroppedPackages.restoreToDefault(); }


  Pathname ZConfig::solver_checkSystemFile() const
  { return ( _pimpl->solver_checkSystemFile.empty()
      ? (configPath()/"systemCheck") : _pimpl->solver_checkSystemFile ); }

  Pathname ZConfig::solver_checkSystemFileDir() const
  { return ( _pimpl->solver_checkSystemFileDir.empty()
      ? (configPath()/"systemCheck.d") : _pimpl->solver_checkSystemFileDir ); }


  namespace
  {
    inline void sigMultiversionSpecChanged()
    {
      sat::detail::PoolMember::myPool().multiversionSpecChanged();
    }
  }

  const std::set<std::string> & ZConfig::multiversionSpec() const	{ return _pimpl->multiversion(); }
  void ZConfig::multiversionSpec( std::set<std::string> new_r )		{ _pimpl->multiversion().swap( new_r );		sigMultiversionSpecChanged(); }
  void ZConfig::clearMultiversionSpec()					{ _pimpl->multiversion().clear();		sigMultiversionSpecChanged(); }
  void ZConfig::addMultiversionSpec( const std::string & name_r )	{ _pimpl->multiversion().insert( name_r );	sigMultiversionSpecChanged(); }
  void ZConfig::removeMultiversionSpec( const std::string & name_r )	{ _pimpl->multiversion().erase( name_r );	sigMultiversionSpecChanged(); }

  bool ZConfig::apply_locks_file() const
  { return _pimpl->apply_locks_file; }

  Pathname ZConfig::update_dataPath()
#if LEGACY(1735)
  const
#endif
  {
    return Pathname("/var/adm");
  }

  Pathname ZConfig::update_messagesPath()
#if LEGACY(1735)
  const
#endif
  {
    return Pathname(update_dataPath()/"update-messages");
  }

  Pathname ZConfig::update_scriptsPath()
#if LEGACY(1735)
  const
#endif
  {
    return Pathname(update_dataPath()/"update-scripts");
  }

  std::string ZConfig::updateMessagesNotify() const
  { return _pimpl->updateMessagesNotify; }

  void ZConfig::setUpdateMessagesNotify( const std::string & val_r )
  { _pimpl->updateMessagesNotify.set( val_r ); }

  void ZConfig::resetUpdateMessagesNotify()
  { _pimpl->updateMessagesNotify.restoreToDefault(); }

  ///////////////////////////////////////////////////////////////////

  target::rpm::RpmInstFlags ZConfig::rpmInstallFlags() const
  { return _pimpl->rpmInstallFlags; }


  Pathname ZConfig::historyLogFile() const
  {
    return ( _pimpl->history_log_path.empty() ?
        Pathname("/var/log/zypp/history") : _pimpl->history_log_path );
  }

  Pathname ZConfig::credentialsGlobalDir() const
  {
    return _pimpl->_mediaConf.credentialsGlobalDir();
  }

  Pathname ZConfig::credentialsGlobalFile() const
  {
    return _pimpl->_mediaConf.credentialsGlobalFile();
  }

  ///////////////////////////////////////////////////////////////////

  std::string ZConfig::distroverpkg() const
  { return "system-release"; }

  ///////////////////////////////////////////////////////////////////

  Pathname ZConfig::pluginsPath() const
  { return _pimpl->pluginsPath.get(); }

  std::string ZConfig::multiversionKernels() const
  {
    return _pimpl->cfg_kernel_keep_spec;
  }

  ///////////////////////////////////////////////////////////////////

  std::ostream & ZConfig::about( std::ostream & str ) const
  {
    str << "libzypp: " LIBZYPP_VERSION_STRING << " (" << LIBZYPP_CODESTREAM << ")" << endl;

    str << "libsolv: " << solv_version;
    if ( ::strcmp( solv_version, LIBSOLV_VERSION_STRING ) )
      str << " (built against " << LIBSOLV_VERSION_STRING << ")";
    str << endl;

    str << "zypp.conf: '" << _pimpl->_parsedZyppConf << "'" << endl;
    str << "TextLocale: '" << textLocale() << "' (" << defaultTextLocale() << ")" << endl;
    str << "SystemArchitecture: '" << systemArchitecture() << "' (" << defaultSystemArchitecture() << ")" << endl;
    return str;
  }

  /////////////////////////////////////////////////////////////////
} // namespace zypp
///////////////////////////////////////////////////////////////////
