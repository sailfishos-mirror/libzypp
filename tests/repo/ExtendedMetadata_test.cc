#include <iostream>
#include <sstream>
#include <fstream>
#include <list>
#include <string>

#include <boost/test/unit_test.hpp>

#include <zypp/base/LogTools.h>
#include <zypp/base/Exception.h>
#include <zypp/PathInfo.h>
#include <zypp/TmpPath.h>
#include <zypp/Package.h>
#include <zypp/RepoManager.h>
#include <zypp/sat/Pool.h>
#include "KeyRingTestReceiver.h"

#include "TestSetup.h"

using boost::unit_test::test_case;

using namespace zypp;
using namespace zypp::repo;
using namespace zypp::filesystem;

#define TEST_DIR TESTS_SRC_DIR "/repo/yum/data/extensions"

BOOST_AUTO_TEST_CASE(extended_metadata)
{
  KeyRingTestReceiver rec;
  //rec.answerAcceptUnknownKey(true);
  rec.answerAcceptUnsignedFile(true);


//  rec.answerImportKey(true);
  Pathname repodir(TEST_DIR );

  sat::Pool pool(sat::Pool::instance());

  TestSetup test( Arch_x86_64 );
  test.loadRepo(repodir.absolutename().asDirUrl(), "updates");

  Repository repo = pool.reposFind("updates");

  BOOST_CHECK_EQUAL( repo.generatedTimestamp(), Date(1227279057) );
  BOOST_CHECK_EQUAL( repo.suggestedExpirationTimestamp(), Date(1227279057 + 3600) );

  // check that the attributes of product compatibility are ok
  int count = 0;
  std::vector<CpeId> cpeids;
  std::vector<std::string> labels;

  for_( it,
        repo.compatibleWithProductBegin(),
        repo.compatibleWithProductEnd() )
  {
      cpeids.push_back(it.cpeId());
      labels.push_back(it.label());
      count++;
  }

  // there were 2 compatible products
  BOOST_CHECK_EQUAL( count, 2 );
  BOOST_CHECK_EQUAL( cpeids[0], "cpe:/o:opensuse" );
  BOOST_CHECK_EQUAL( cpeids[1], "cpe:/o:sle" );

  BOOST_CHECK_EQUAL( labels[0], "openSUSE 11.0" );
  BOOST_CHECK_EQUAL( labels[1], "SLE 11.0" );

  cpeids.clear();
  labels.clear();
  count = 0;

  for_( it,
        repo.updatesProductBegin(),
        repo.updatesProductEnd() )
  {
      cpeids.push_back(it.cpeId());
      labels.push_back(it.label());
      count++;
  }

  // the repo updates one product
  BOOST_CHECK_EQUAL( count, 1 );
  BOOST_CHECK_EQUAL( cpeids[0], "cpe:/o:sle" );
  BOOST_CHECK_EQUAL( labels[0], "SLE 11.0" );

  // because this product updates something, it _is_ an update repo
  BOOST_CHECK( repo.isUpdateRepo() );

  BOOST_CHECK( repo.providesUpdatesFor(CpeId("cpe:/o:sle")) );
  BOOST_CHECK( ! repo.providesUpdatesFor(CpeId("cpe:/o:windows")) );
  // reuse to count solvables
  count = 0;

  /**
   * Now check for the extended metadata of the packages
   */
  for_( it, repo.solvablesBegin(), repo.solvablesEnd() )
  {
      sat::Solvable s = *it;
      MIL << s << endl;
      MIL << s.kind() << endl;
      if ( s.ident() == "wt" )
      {
          count++;
          Package::Ptr p = asKind<Package>(makeResObject(s));
          BOOST_CHECK(p);
          BOOST_CHECK(p->maybeUnsupported() );
          BOOST_CHECK_EQUAL(p->vendorSupport(), VendorSupportUnknown );

      }
      else if ( s.ident() == "foobar" )
      {
            count++;
            Package::Ptr p = asKind<Package>(makeResObject(s));
            BOOST_CHECK(p);
            BOOST_CHECK_EQUAL(p->vendorSupport(), VendorSupportUnsupported );
            BOOST_CHECK(p->maybeUnsupported() );
      }
      else if ( s.ident() == "foofoo" )
      {
          count++;
          Package::Ptr p = asKind<Package>(makeResObject(s));
          BOOST_CHECK(p);
          // if it is level 3 support it cant be unsupported
          BOOST_CHECK_EQUAL(p->vendorSupport(), VendorSupportLevel3 );
          BOOST_CHECK(! p->maybeUnsupported() );
      }
      // superseded chains....
      //    oldfoo -> newfoo -> superfoo(L3)
      //    foomess -> superfoo(L3)
      //            -> brokenfoo -> foomess(cycle)
      //                         -> notexists
      else if ( s.ident() == "oldfoo" )
      {
          //    oldfoo -> newfoo -> superfoo(L3)
          count++;
          Package::Ptr p = asKind<Package>(makeResObject(s));
          BOOST_CHECK(p);
          // if it is superseeded it counts as maybeUnsuopported
          BOOST_CHECK_EQUAL(p->vendorSupport(), VendorSupportSuperseded );
          BOOST_CHECK( p->maybeUnsupported() );
          const auto & supersededBy { p->supersededBy() };
          BOOST_CHECK_EQUAL( supersededBy.size(), 1 );
          BOOST_CHECK_EQUAL( supersededBy[0], "newfoo" );
          const auto & supersededByItems { p->supersededByItems() };
          BOOST_CHECK_EQUAL( supersededByItems.first.size(), 1 );
          BOOST_CHECK_EQUAL( supersededByItems.first[0], "superfoo" );
          BOOST_CHECK_EQUAL( supersededByItems.second.size(), 0 );
      }
      else if ( s.ident() == "newfoo" )
      {
          //    oldfoo -> newfoo -> superfoo(L3)
          count++;
          Package::Ptr p = asKind<Package>(makeResObject(s));
          BOOST_CHECK(p);
          // if it is superseeded it counts as maybeUnsuopported
          BOOST_CHECK_EQUAL(p->vendorSupport(), VendorSupportSuperseded );
          BOOST_CHECK( p->maybeUnsupported() );
          const auto &  supersededBy { p->supersededBy() };
          BOOST_CHECK_EQUAL( supersededBy.size(), 1 );
          BOOST_CHECK_EQUAL( supersededBy[0], "superfoo" );
          const auto & supersededByItems { p->supersededByItems() };
          BOOST_CHECK_EQUAL( supersededByItems.first.size(), 1 );
          BOOST_CHECK_EQUAL( supersededByItems.first[0], "superfoo" );
          BOOST_CHECK_EQUAL( supersededByItems.second.size(), 0 );
      }
      else if ( s.ident() == "superfoo" )
      {
          //    oldfoo -> newfoo -> superfoo(L3)
          count++;
          Package::Ptr p = asKind<Package>(makeResObject(s));
          BOOST_CHECK(p);
          BOOST_CHECK_EQUAL(p->vendorSupport(), VendorSupportLevel3 );
          BOOST_CHECK( not p->maybeUnsupported() );
          const auto &  supersededBy { p->supersededBy() };
          BOOST_CHECK_EQUAL( supersededBy.size(), 0 );
          const auto & supersededByItems { p->supersededByItems() };
          BOOST_CHECK_EQUAL( supersededByItems.first.size(), 0 );
          BOOST_CHECK_EQUAL( supersededByItems.second.size(), 0 );
      }
      else if ( s.ident() == "foomess" )
      {
          //    foomess -> superfoo(L3)
          //            -> brokenfoo -> foomess(cycle)
          //                         -> notexists
          count++;
          Package::Ptr p = asKind<Package>(makeResObject(s));
          BOOST_CHECK(p);
          // if it is superseeded it counts as maybeUnsuopported
          BOOST_CHECK_EQUAL(p->vendorSupport(), VendorSupportSuperseded );
          BOOST_CHECK( p->maybeUnsupported() );
          const auto &  supersededBy { p->supersededBy() };
          BOOST_CHECK_EQUAL( supersededBy.size(), 2 );
          BOOST_CHECK_EQUAL( supersededBy[0], "brokenfoo" );
          BOOST_CHECK_EQUAL( supersededBy[1], "superfoo" );
          const auto & supersededByItems { p->supersededByItems() };
          BOOST_CHECK_EQUAL( supersededByItems.first.size(), 1 );
          BOOST_CHECK_EQUAL( supersededByItems.first[0], "superfoo" );
          BOOST_CHECK_EQUAL( supersededByItems.second.size(), 1 );
          BOOST_CHECK_EQUAL( supersededByItems.second[0], "notexists" );
      }
      else if ( s.ident() == "brokenfoo" )
      {
          //    foomess -> superfoo(L3)
          //            -> brokenfoo -> foomess(cycle)
          //                         -> notexists
          count++;
          Package::Ptr p = asKind<Package>(makeResObject(s));
          BOOST_CHECK(p);
          // if it is superseeded it counts as maybeUnsuopported
          BOOST_CHECK_EQUAL(p->vendorSupport(), VendorSupportSuperseded );
          BOOST_CHECK( p->maybeUnsupported() );
          const auto &  supersededBy { p->supersededBy() };
          BOOST_CHECK_EQUAL( supersededBy.size(), 2 );
          BOOST_CHECK_EQUAL( supersededBy[0], "foomess" );
          BOOST_CHECK_EQUAL( supersededBy[1], "notexists" );
          const auto & supersededByItems { p->supersededByItems() };
          BOOST_CHECK_EQUAL( supersededByItems.first.size(), 1 );
          BOOST_CHECK_EQUAL( supersededByItems.first[0], "superfoo" );
          BOOST_CHECK_EQUAL( supersededByItems.second.size(), 1 );
          BOOST_CHECK_EQUAL( supersededByItems.second[0], "notexists" );
      }
      else
      {
          BOOST_FAIL(str::form("Repo has package not contemplated in test: %s", s.ident().c_str()).c_str());
      }

    }

    // check that we actually found all testeable
    // resolvables
    BOOST_CHECK_EQUAL(count, 8);

}
