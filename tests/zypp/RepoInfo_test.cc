
#include <iostream>
#include <fstream>
#include <list>
#include <string>

#include <zypp/base/Logger.h>
#include <zypp/base/Exception.h>

#include <zypp/RepoInfo.h>

#include <boost/test/unit_test.hpp>
#include <boost/test/parameterized_test.hpp>
#include <boost/test/unit_test_log.hpp>

#include "KeyRingTestReceiver.h"

#include "WebServer.h"

using boost::unit_test::test_suite;
using boost::unit_test::test_case;
using namespace boost::unit_test::log;

using namespace zypp;
using namespace zypp::filesystem;
using namespace zypp::repo;

BOOST_AUTO_TEST_CASE(repoinfo_test)
{
  WebServer web((Pathname(TESTS_SRC_DIR) + "/data/Mirrorlist/remote-site").c_str(), 10001);
  BOOST_REQUIRE( web.start() );

  Url weburl (web.url());
  weburl.setPathName("/metalink.xml");

  RepoInfo ri;

  ri.setMirrorlistUrl(weburl);

  BOOST_CHECK_EQUAL(ri.location(), weburl);
  BOOST_CHECK_EQUAL(ri.url().asString(), "http://ftp-stud.hs-esslingen.de/pub/fedora/linux/updates/13/x86_64/");

  BOOST_REQUIRE( !ri.repoOriginsEmpty() );
  BOOST_CHECK_EQUAL( ri.repoOrigins().begin()->authority().url().asString(), "http://ftp-stud.hs-esslingen.de/pub/fedora/linux/updates/13/x86_64/" );

  std::ostringstream ostr;
  ri.dumpAsIniOn(ostr);

  BOOST_CHECK( ostr.str().find("baseurl=") == std::string::npos );

  web.stop();
}
