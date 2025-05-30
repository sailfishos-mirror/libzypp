#include <zypp-core/zyppng/base/EventLoop>
#include <zypp-core/zyppng/base/EventDispatcher>
#include <zypp-curl/ng/network/Downloader>
#include <zypp-curl/ng/network/DownloadSpec>
#include <zypp-curl/ng/network/NetworkRequestError>
#include <zypp-curl/ng/network/NetworkRequestDispatcher>
#include <zypp-curl/ng/network/Request>
#include <zypp-media/auth/CredentialManager>
#include <zypp/Digest.h>
#include <zypp/TmpPath.h>
#include <zypp/PathInfo.h>
#include <zypp/ZConfig.h>
#include <iostream>
#include <fstream>
#include <random>
#include "WebServer.h"
#include "TestTools.h"

#include <boost/test/unit_test.hpp>
#include <boost/test/data/test_case.hpp>

#define BOOST_TEST_REQ_SUCCESS(REQ) \
  do { \
      BOOST_REQUIRE_MESSAGE( REQ->state() == zyppng::Download::Finished && !REQ->hasError(), zypp::str::Format(" %1% != zyppng::Download::Success (%2%)") % REQ->state() % REQ->errorString() ); \
      BOOST_REQUIRE_EQUAL( REQ->lastRequestError().type(), zyppng::NetworkRequestError::NoError ); \
      BOOST_REQUIRE( REQ->errorString().empty() ); \
  } while(false)

#define BOOST_TEST_REQ_FAILED(REQ) \
  do { \
      BOOST_REQUIRE_EQUAL( REQ->state(), zyppng::Download::Finished ); \
      BOOST_REQUIRE( REQ->hasError() ); \
      BOOST_REQUIRE_NE( REQ->lastRequestError().type(), zyppng::NetworkRequestError::NoError ); \
      BOOST_REQUIRE( !REQ->errorString().empty() ); \
  } while(false)

namespace bdata = boost::unit_test::data;

bool withSSL[] = {true, false};

BOOST_DATA_TEST_CASE( dltest_basic, bdata::make( withSSL ), withSSL)
{
  auto ev = zyppng::EventLoop::create();

  zyppng::Downloader::Ptr downloader = std::make_shared<zyppng::Downloader>();

  //make sure the data here is big enough to cross the threshold of 256 bytes so we get a progress signal emitted and not only the alive signal.
  std::string dummyContent = "This is just some dummy content,\nto test downloading and signals.\n"
                             "This is just some dummy content,\nto test downloading and signals.\n"
                             "This is just some dummy content,\nto test downloading and signals.\n"
                             "This is just some dummy content,\nto test downloading and signals.\n";

  WebServer web((zypp::Pathname(TESTS_SRC_DIR)/"data"/"dummywebroot").c_str(), 10001, withSSL );
  web.addRequestHandler("getData", WebServer::makeResponse("200", dummyContent ) );
  BOOST_REQUIRE( web.start() );

  zypp::filesystem::TmpFile targetFile;
  zyppng::Url weburl (web.url());
  weburl.setPathName("/handler/getData");
  zyppng::TransferSettings set = web.transferSettings();

  bool gotStarted = false;
  bool gotFinished = false;
  bool gotProgress = false;
  bool gotAlive = false;
  off_t lastProgress = 0;
  off_t totalDL = 0;

  std::vector<zyppng::Download::State> allStates;


  zyppng::Download::Ptr dl = downloader->downloadFile(  zyppng::DownloadSpec(weburl, targetFile.path(), dummyContent.length()) );
  dl->spec().setTransferSettings( set );

  dl->sigFinished().connect([&]( zyppng::Download & ){
    gotFinished = true;
    ev->quit();
  });

  dl->sigStarted().connect([&]( zyppng::Download & ){
    gotStarted = true;
  });

  dl->sigAlive().connect([&]( zyppng::Download &, off_t ){
    gotAlive = true;
  });

  dl->sigStateChanged().connect([&]( zyppng::Download &, zyppng::Download::State state ){
    allStates.push_back( state );
  });

  dl->sigProgress().connect([&]( zyppng::Download &, off_t dltotal, off_t dlnow ){
    gotProgress = true;
    lastProgress = dlnow;
    totalDL = dltotal;
  });
  dl->start();
  ev->run();

  BOOST_TEST_REQ_SUCCESS( dl );
  BOOST_REQUIRE( gotStarted );
  BOOST_REQUIRE( gotFinished );
  BOOST_REQUIRE( gotProgress );
  BOOST_REQUIRE( gotAlive );
  BOOST_REQUIRE_EQUAL( totalDL, dummyContent.length() );
  BOOST_REQUIRE_EQUAL( lastProgress, dummyContent.length() );
  BOOST_REQUIRE ( allStates == std::vector<zyppng::Download::State>({zyppng::Download::InitialState, zyppng::Download::DlMetaLinkInfo, zyppng::Download::Finished}) );
}


// Globals are HORRIBLY broken here, do not use any static globals they might not be initialized at the point
// of calling the initializer of the MirrorSet vector, so we are manually initializing the Byte and KByte Units here
const auto makeBytes( zypp::ByteCount::SizeType size ) {
  return zypp::ByteCount( size, zypp::ByteCount::Unit( 1LL, "B", 0 ) );
};

const auto makeKBytes( zypp::ByteCount::SizeType size ) {
  return zypp::ByteCount( size, zypp::ByteCount::Unit( 1024LL, "KiB", 1 ) );
};


struct MirrorSet
{
  std::string name; //<dataset name, used only in debug output if the test fails
  std::string filename; //< the name of the file we want to query, the test will base all other handlers and filenames on this
  zypp::ByteCount dlTotal;
  std::vector< std::pair<int, std::string> > mirrors; //all mirrors injected into the metalink file
  int expectedFileDownloads; //< how many downloads are direct file downloads
  int expectedHandlerDownloads; //< how many started downloads are handler requests
  std::vector<zyppng::Download::State> expectedStates;
  bool expectSuccess; //< should the download work out?
  zypp::ByteCount chunkSize = makeKBytes( 256 );

  //zck data
  zypp::CheckSum  headerChecksum; //< ZChunk header checksum
  zypp::ByteCount headerSize;     //< ZChunk header size
  std::string     deltaFilePath;  //< ZChunk delta file

  std::ostream & operator<<( std::ostream & str ) const {
    str << "MirrorSet{ " << name << " }";
    return str;
  }
};

namespace boost{ namespace test_tools{ namespace tt_detail{
template<>
struct print_log_value< MirrorSet > {
    void    operator()( std::ostream& str, MirrorSet const& set) {
      set.operator<<( str );
    }
};
}}}

std::vector< MirrorSet > generateMirr ()
{
  std::vector< MirrorSet > res;

  //all mirrors good:
  res.push_back( MirrorSet() );
  res.back().name = "All good mirrors";
  res.back().filename = "test.txt";
  res.back().dlTotal = makeBytes(2148018);
  res.back().expectSuccess = true;
  res.back().expectedHandlerDownloads  = 1;
  res.back().expectedFileDownloads  = 9;
  res.back().expectedStates = { zyppng::Download::InitialState, zyppng::Download::DlMetaLinkInfo, zyppng::Download::PrepareMulti, zyppng::Download::DlMetalink, zyppng::Download::Finished};
  for ( int i = 100 ; i >= 10; i -= 10 )
    res.back().mirrors.push_back( std::make_pair( i, "/test.txt") );

  //all mirrors good big chunk
  res.push_back( MirrorSet() );
  res.back().name = "All good mirrors, 1024 chunk size";
  res.back().filename = "test.txt";
  res.back().dlTotal = makeBytes(2148018);
  res.back().expectSuccess = true;
  res.back().expectedHandlerDownloads  = 1;
  res.back().expectedFileDownloads  = 3;
  res.back().chunkSize = makeKBytes(1024);
  res.back().expectedStates = { zyppng::Download::InitialState, zyppng::Download::DlMetaLinkInfo, zyppng::Download::PrepareMulti, zyppng::Download::DlMetalink, zyppng::Download::Finished};
  for ( int i = 100 ; i >= 10; i -= 10 )
    res.back().mirrors.push_back( std::make_pair( i, "/test.txt") );

  //no mirrors:
  res.push_back( MirrorSet() );
  res.back().name = "Empty mirrors";
  res.back().filename = "test.txt";
  res.back().dlTotal = makeBytes (2148018);
  res.back().expectSuccess = true;
  res.back().expectedHandlerDownloads  = 10;
  res.back().expectedFileDownloads  = 0;
  res.back().expectedStates = { zyppng::Download::InitialState, zyppng::Download::DlMetaLinkInfo, zyppng::Download::PrepareMulti, zyppng::Download::DlMetalink, zyppng::Download::Finished};

  //only broken mirrors:
  res.push_back( MirrorSet() );
  res.back().name = "All broken mirrors";
  res.back().filename = "test.txt";
  res.back().dlTotal = makeBytes(2148018);
  res.back().expectSuccess = true;
  res.back().expectedHandlerDownloads  = 2; //has to fall back to url handler download
  res.back().expectedFileDownloads  = 10; //should try all mirrors and fail
  res.back().expectedStates = { zyppng::Download::InitialState, zyppng::Download::DlMetaLinkInfo, zyppng::Download::PrepareMulti, zyppng::Download::DlMetalink, zyppng::Download::DlSimple, zyppng::Download::Finished};
  for ( int i = 100 ; i >= 10; i -= 10 )
    res.back().mirrors.push_back( std::make_pair( i, "/doesnotexist.txt") );

  //only broken mirrors:
  res.push_back( MirrorSet() );
  res.back().name = "All broken mirrors by params";
  res.back().filename = "test.txt";
  res.back().dlTotal = makeBytes(2148018);
  res.back().expectSuccess = true;
  res.back().expectedHandlerDownloads  = 2; //has to fall back to url handler download
  res.back().expectedFileDownloads  = 0; // Setting up the mirrors will fail before even starting a download, so we should only see requests to the handler directly
  res.back().expectedStates = { zyppng::Download::InitialState, zyppng::Download::DlMetaLinkInfo, zyppng::Download::PrepareMulti, zyppng::Download::DlMetalink, zyppng::Download::DlSimple, zyppng::Download::Finished};
  for ( int i = 100 ; i >= 10; i -= 10 )
    res.back().mirrors.push_back( std::make_pair( i, "/test.txt?auth=foobar") );

  //some broken mirrors:
  res.push_back( MirrorSet() );
  res.back().name = "Some broken mirrors less URLs than blocks";
  res.back().filename = "test.txt";
  res.back().dlTotal = makeBytes(2148018);
  res.back().expectSuccess = true;
  res.back().expectedHandlerDownloads = 1;
  res.back().expectedFileDownloads = 9 + 3; // 3 should fail due to broken mirrors
  res.back().expectedStates = { zyppng::Download::InitialState, zyppng::Download::DlMetaLinkInfo, zyppng::Download::PrepareMulti, zyppng::Download::DlMetalink, zyppng::Download::Finished};
  for ( int i = 10 ; i >= 5; i-- ) {
    if ( i % 2 ) {
      res.back().mirrors.push_back( std::make_pair( i*10, "/doesnotexist.txt") );
    } else {
      res.back().mirrors.push_back( std::make_pair( i*10, "/test.txt") );
    }
  }

  //some broken mirrors with more URLs than blocks:
  res.push_back( MirrorSet() );
  res.back().name = "Some broken mirrors more URLs than blocks";
  res.back().filename = "test.txt";
  res.back().dlTotal = makeBytes(2148018);
  res.back().expectSuccess = true;
  res.back().expectedHandlerDownloads = 1;
  //it is not really possible to know how many times the downloads will fail, there are
  //5 broken mirrors in the set, but if a working mirror is done before the last broken
  //URL is picked from the dataset, not all broken URLs will be used
  res.back().expectedFileDownloads  = -1;
  res.back().expectedStates = { zyppng::Download::InitialState, zyppng::Download::DlMetaLinkInfo, zyppng::Download::PrepareMulti, zyppng::Download::DlMetalink, zyppng::Download::Finished};
  for ( int i = 10 ; i >= 1; i-- ) {
    if ( i % 2 ) {
      res.back().mirrors.push_back( std::make_pair( i*10, "/doesnotexist.txt") );
    } else {
      res.back().mirrors.push_back( std::make_pair( i*10, "/test.txt") );
    }
  }

  //mirrors where some return a invalid block
  res.push_back( MirrorSet() );
  res.back().name = "Some mirrors return broken blocks";
  res.back().filename = "test.txt";
  res.back().dlTotal = makeBytes(2148018);
  res.back().expectSuccess = true;
  res.back().expectedHandlerDownloads  = 1;
  //it is not really possible to know how many times the downloads will fail, there are
  //5 broken mirrors in the set, but if a working mirror is done before the last broken
  //URL is picked from the dataset, not all broken URLs will be used
  res.back().expectedFileDownloads  = -1;
  res.back().expectedStates = { zyppng::Download::InitialState, zyppng::Download::DlMetaLinkInfo, zyppng::Download::PrepareMulti, zyppng::Download::DlMetalink, zyppng::Download::Finished};
  for ( int i = 10 ; i >= 1; i-- ) {
    if ( i % 2 ) {
      res.back().mirrors.push_back( std::make_pair( i*10, "/handler/random") );
    } else {
      res.back().mirrors.push_back( std::make_pair( i*10, "/test.txt") );
    }
  }
  //mirrors where some return a invalid range response ( 206 status but no range def )
  res.push_back( MirrorSet() );
  res.back().name = "Some mirrors return boken range responses";
  res.back().filename = "test.txt";
  res.back().dlTotal = makeBytes(2148018);
  res.back().expectSuccess = true;
  res.back().expectedHandlerDownloads  = 1;
  //it is not really possible to know how many times the downloads will fail, there are
  //5 broken mirrors in the set, but if a working mirror is done before the last broken
  //URL is picked from the dataset, not all broken URLs will be used
  res.back().expectedFileDownloads  = -1;
  res.back().expectedStates = { zyppng::Download::InitialState, zyppng::Download::DlMetaLinkInfo, zyppng::Download::PrepareMulti, zyppng::Download::DlMetalink, zyppng::Download::Finished};
  for ( int i = 10 ; i >= 1; i-- ) {
    if ( i % 2 ) {
      res.back().mirrors.push_back( std::make_pair( i*10, "/handler/brokenrange") );
    } else {
      res.back().mirrors.push_back( std::make_pair( i*10, "/test.txt") );
    }
  }

  //all return a invalid range response ( 206 status but no range def )
  res.push_back( MirrorSet() );
  res.back().name = "All mirrors return boken range responses";
  res.back().filename = "test.txt";
  res.back().dlTotal = makeBytes(2148018);
  res.back().expectSuccess = true;
  res.back().expectedHandlerDownloads  = 2; //has to fall back to url handler download
  res.back().expectedFileDownloads  = 10; //should try all mirrors and fail
  res.back().expectedStates = { zyppng::Download::InitialState, zyppng::Download::DlMetaLinkInfo, zyppng::Download::PrepareMulti, zyppng::Download::DlMetalink, zyppng::Download::DlSimple  , zyppng::Download::Finished};
  for ( int i = 10 ; i >= 1; i-- ) {
    res.back().mirrors.push_back( std::make_pair( i*10, "/handler/brokenrange") );
  }

#if ENABLE_ZCHUNK_COMPRESSION
  //all mirrors good with zck:
  res.push_back( MirrorSet() );
  res.back().name = "All good mirrors with zck";
  res.back().filename = "primary.xml.zck";
  res.back().dlTotal = makeBytes(274638);
  res.back().expectSuccess = true;
  res.back().expectedHandlerDownloads  = 3; // query if metalink avail + dl metalink + dl zck head because request is reused
  res.back().expectedFileDownloads  = 5; // 5 Chunks ( chunk size is at least 4K but is likely a few bytes bigger )
  res.back().expectedStates = { zyppng::Download::InitialState, zyppng::Download::DetectMetaLink, zyppng::Download::DlMetaLinkInfo, zyppng::Download::PrepareMulti, zyppng::Download::DlZChunkHead, zyppng::Download::DlZChunk, zyppng::Download::Finished};
  res.back().headerChecksum = zypp::CheckSum( zypp::Digest::sha256(), "90a1a1b99ba3b6c8ae9f14b0c8b8c43141c69ec3388bfa3b9915fbeea03926b7");
  res.back().headerSize     = makeBytes(11717);
  res.back().deltaFilePath  = "primary-deltatemplate.xml.zck";
  res.back().chunkSize      = makeKBytes(4);
  for ( int i = 100 ; i >= 10; i -= 10 )
    res.back().mirrors.push_back( std::make_pair( i, "/primary.xml.zck") );
#endif

  return res;
}


//create one URL line for a metalink template file
std::string makeUrl ( int pref, const zyppng::Url &url )
{
  return ( zypp::str::Format( "<url preference=\"%1%\" location=\"de\" type=\"%2%\">%3%</url>" ) % pref % url.getScheme() % url.asCompleteString() );
};


static bool requestWantsMetaLink ( const WebServer::Request &req )
{
  auto it = req.params.find( "HTTP_ACCEPT" );
  if ( it != req.params.end() ) {
    if ( (*it).second.find("application/metalink+xml")  != std::string::npos ||
         (*it).second.find("application/metalink4+xml") != std::string::npos ) {
      return true;
    }
  }
  return false;
}

//creates a request handler for the Mock WebServer that returns the metalink data
//specified in \a data if the request has the metalink accept handler
WebServer::RequestHandler makeMetaFileHandler ( const std::string &fName, const std::string *data )
{
  return [ data, fName ]( WebServer::Request &req ){
    if ( requestWantsMetaLink( req ) ) {
      req.rout << WebServer::makeResponseString( "200", { "Content-Type: application/metalink+xml; charset=utf-8\r\n" }, *data );
      return;
    }
    req.rout << "Location: /"<<fName<<"\r\n\r\n";
    return;
  };
};

//creates a request handler for the Mock WebServer that returns a junk block of
//data for a range request, otherwise relocates the request
WebServer::RequestHandler makeJunkBlockHandler ( const std::string &fName )
{
  return [ fName ]( WebServer::Request &req ) {
    auto it = req.params.find( "HTTP_RANGE" );
    if ( it != req.params.end() && zypp::str::startsWith( it->second, "bytes=" ) ) {
        //bytes=786432-1048575
        std::string range = it->second.substr( 6 ); //remove bytes=
        size_t dash = range.find_first_of( "-" );
        off_t start = -1;
        off_t end = -1;
        if ( dash != std::string::npos ) {
          start = std::stoll( range.substr( 0, dash ) );
          end   = std::stoll( range.substr( dash+1 ) );
        }

        if ( start != -1 && end != -1 ) {
          std::string block;
          for ( off_t curr = start; curr <= end; curr++ ) {
            block += 'a';
          }
          req.rout << "Status: 206 Partial Content\r\n"
                   << "Accept-Ranges: bytes\r\n"
                   << "Content-Length: "<< block.length() <<"\r\n"
                   << "Content-Range: bytes "<<start<<"-"<<end<<"/"<<block.length()<<"\r\n"
                   <<"\r\n"
                   << block;
          return;
        }
    }
    req.rout << "Location: /"<<fName<<"\r\n\r\n";
    return;
  };
};

//creates a request handler for the Mock WebServer that returns a 206 response but does not specify the range details
WebServer::RequestHandler makeBrokenRangeHandler ( const std::string &fName )
{
  return [ fName ]( WebServer::Request &req ){
    auto it = req.params.find( "HTTP_RANGE" );
    if ( it != req.params.end() && zypp::str::startsWith( it->second, "bytes=" ) ) {
        //bytes=786432-1048575
        std::string range = it->second.substr( 6 ); //remove bytes=
        size_t dash = range.find_first_of( "-" );
        off_t start = -1;
        off_t end = -1;
        if ( dash != std::string::npos ) {
          start = std::stoll( range.substr( 0, dash ) );
          end   = std::stoll( range.substr( dash+1 ) );
        }

        if ( start != -1 && end != -1 ) {
          std::string block;
          for ( off_t curr = start; curr <= end; curr++ ) {
            block += 'a';
          }
          req.rout << "Status: 206 Partial Content\r\n"
                   << "Accept-Ranges: bytes\r\n"
                   << "Content-Length: "<< block.length() <<"\r\n"
                   <<"\r\n"
                   << block;
          return;
        }
    }
    req.rout << "Location: /"<<fName<<"\r\n\r\n";
    return;
  };
};

int maxConcurrentDLs[] = { 1, 2, 4, 8, 10, 15 };

BOOST_DATA_TEST_CASE( test1, bdata::make( generateMirr() ) * bdata::make( withSSL ) * bdata::make( maxConcurrentDLs )  , elem, withSSL, maxDLs )
{

  zypp::Pathname testRoot = zypp::Pathname(TESTS_SRC_DIR)/"zyppng/data/downloader";

  // std::cout << "Starting with test " << elem.name << std::endl;

  // each URL in the metalink file has a preference , a schema and of course the URL, we need to adapt those to our test setup
  // so we generate the file on the fly from a template in the test data
  std::string metaTempl = TestTools::readFile ( testRoot/( zypp::str::Format("%1%.meta") % elem.filename ).str() );
  BOOST_REQUIRE( !metaTempl.empty() );

  auto ev = zyppng::EventLoop::create();

  WebServer web( testRoot.c_str(), 10001, withSSL );
  BOOST_REQUIRE( web.start() );

  zypp::filesystem::TmpFile targetFile;
  std::shared_ptr<zyppng::Downloader> downloader = std::make_shared<zyppng::Downloader>();
  downloader->requestDispatcher()->setMaximumConcurrentConnections( maxDLs );

  //first metalink download, generate a fully valid one
  zyppng::Url weburl (web.url());
  weburl.setPathName( zypp::str::Format("/handler/%1%") % elem.filename );

  std::string urls;
  if ( elem.mirrors.size() ) {
    for ( const auto &mirr : elem.mirrors ) {
      zyppng::Url mirrUrl ( web.url().asCompleteString() + mirr.second );
      // mirrUrl.setPathName( mirr.second );
      urls += makeUrl( mirr.first, mirrUrl ) + "\n";
    }
  }

  std::string metaFile = zypp::str::Format( metaTempl ) % urls;
  web.addRequestHandler( elem.filename, makeMetaFileHandler(  elem.filename, &metaFile ) );
  web.addRequestHandler( "random", makeJunkBlockHandler( elem.filename ) );
  web.addRequestHandler( "brokenrange", makeBrokenRangeHandler( elem.filename ) );

  int expectedDownloads = elem.expectedHandlerDownloads + elem.expectedFileDownloads;
  int startedDownloads = 0;
  int finishedDownloads = 0;
  bool downloadHadError = false;
  bool gotProgress = false;
  bool gotAlive = false;
  bool gotMultiDLState = false;
  std::vector<zyppng::Download::State> allStates;
  off_t gotTotal = 0;
  off_t lastProgress = 0;

  int countHandlerReq = 0; //the requests made to the handler slot
  int countFileReq = 0;    //the requests made to the file directly, e.g. a mirror read from the metalink file

  auto dl = downloader->downloadFile( zyppng::DownloadSpec(weburl, targetFile) );
  dl->spec().setTransferSettings( web.transferSettings() )
    .setPreferredChunkSize( elem.chunkSize )
    .setHeaderSize( elem.headerSize )
    .setHeaderChecksum( elem.headerChecksum )
    .setDeltaFile( elem.deltaFilePath.empty() ? zypp::Pathname() : testRoot/elem.deltaFilePath )
    .setExpectedFileSize( elem.dlTotal );

  dl->dispatcher().sigDownloadStarted().connect( [&]( zyppng::NetworkRequestDispatcher &, zyppng::NetworkRequest &req){
    startedDownloads++;
    if ( req.url() == weburl )
      countHandlerReq++;
    else
      countFileReq++;
  });

  dl->dispatcher().sigDownloadFinished().connect( [&]( zyppng::NetworkRequestDispatcher &, zyppng::NetworkRequest &req ){
    finishedDownloads++;

    if ( !downloadHadError )
      downloadHadError = req.hasError();
  });

  dl->sigFinished().connect([&]( zyppng::Download & ){
    ev->quit();
  });

  dl->sigAlive().connect([&]( zyppng::Download &, off_t dlnow ){
    gotAlive = true;
    lastProgress = dlnow;
  });

  dl->sigProgress().connect([&]( zyppng::Download &, off_t dltotal, off_t dlnow ){
    gotProgress = true;
    gotTotal = dltotal;
    lastProgress = dlnow;
  });

  dl->sigStateChanged().connect([&]( zyppng::Download &, zyppng::Download::State state ){
    if ( state == zyppng::Download::DlMetalink || state == zyppng::Download::DlZChunk )
      gotMultiDLState = true;
  });

  dl->sigStateChanged().connect([&]( zyppng::Download &, zyppng::Download::State state ){
    allStates.push_back( state );
  });

  dl->start();
  ev->run();

  //std::cout << dl->errorString() << std::endl;

  if ( elem.expectSuccess )
    BOOST_TEST_REQ_SUCCESS( dl );
  else
    BOOST_TEST_REQ_FAILED ( dl );

  if ( elem.expectedHandlerDownloads > -1 && elem.expectedFileDownloads > -1 ) {
    BOOST_REQUIRE_EQUAL( startedDownloads, expectedDownloads );
  }

  BOOST_REQUIRE_EQUAL( startedDownloads, finishedDownloads );
  BOOST_REQUIRE( gotAlive );
  BOOST_REQUIRE( gotProgress );
  BOOST_REQUIRE( gotMultiDLState );
  if ( elem.dlTotal > 0 )
    BOOST_REQUIRE_EQUAL( lastProgress, elem.dlTotal );
  BOOST_REQUIRE_EQUAL( lastProgress, gotTotal );

  if ( elem.expectedHandlerDownloads > -1 )
    BOOST_REQUIRE_EQUAL( countHandlerReq, elem.expectedHandlerDownloads );

  if ( elem.expectedFileDownloads > -1 )
    BOOST_REQUIRE_EQUAL( countFileReq, elem.expectedFileDownloads );

  if ( !elem.expectedStates.empty() )
    BOOST_REQUIRE( elem.expectedStates == allStates );
}

//tests:
// - broken cert
// - correct expected filesize
// - invalid filesize
// - password handling and propagation


//creates a request handler that requires a authentication to work
WebServer::RequestHandler createAuthHandler ( )
{
  return WebServer::makeBasicAuthHandler( "Basic dGVzdDp0ZXN0", []( WebServer::Request &req ) {
    if ( requestWantsMetaLink( req ) ) {
      auto it = req.params.find( "HTTPS" );
      if ( it != req.params.end() && it->second == "on" ) {
        req.rout << "Status: 307 Temporary Redirect\r\n"
                 << "Cache-Control: no-cache\r\n"
                 << "Location: /auth-https.meta\r\n\r\n";
      } else {
        req.rout << "Status: 307 Temporary Redirect\r\n"
                 << "Cache-Control: no-cache\r\n"
                 << "Location: /auth-http.meta\r\n\r\n";
      }
      return;
    }
    req.rout << "Status: 307 Temporary Redirect\r\n"
                "Location: /test.txt\r\n\r\n";
    return;
  });
};

BOOST_DATA_TEST_CASE( dltest_auth, bdata::make( withSSL ), withSSL )
{
  //don't write or read creds from real settings dir
  zypp::filesystem::TmpDir repoManagerRoot;
  zypp::ZConfig::instance().setRepoManagerRoot( repoManagerRoot.path() );

  auto ev = zyppng::EventLoop::create();

  std::shared_ptr<zyppng::Downloader> downloader = std::make_shared<zyppng::Downloader>();

  WebServer web((zypp::Pathname(TESTS_SRC_DIR)/"/zyppng/data/downloader").c_str(), 10001, withSSL );
  BOOST_REQUIRE( web.start() );

  zypp::filesystem::TmpFile targetFile;
  zyppng::Url weburl (web.url());
  weburl.setPathName("/handler/test.txt");
  zyppng::TransferSettings set = web.transferSettings();

  web.addRequestHandler( "test.txt", createAuthHandler() );
  web.addRequestHandler( "quit", [ &ev ]( WebServer::Request & ){ ev->quit();} );

  {
    auto dl = downloader->downloadFile( zyppng::DownloadSpec(weburl, targetFile.path()) );
    dl->spec().setTransferSettings(set);

    dl->sigFinished( ).connect([ &ev ]( zyppng::Download & ){
      ev->quit();
    });

    dl->start();
    ev->run();
    BOOST_TEST_REQ_FAILED( dl );
    BOOST_REQUIRE_EQUAL( dl->lastRequestError().type(), zyppng::NetworkRequestError::Unauthorized );
  }

  {
    auto dl = downloader->downloadFile( zyppng::DownloadSpec(weburl, targetFile.path()) );
    dl->spec().setTransferSettings(set);

    int gotAuthRequest = 0;

    dl->sigFinished( ).connect([ &ev ]( zyppng::Download & ){
      ev->quit();
    });

    dl->sigAuthRequired().connect( [&]( zyppng::Download &, zyppng::NetworkAuthData &auth, const std::string &availAuth ){
      gotAuthRequest++;
      if ( gotAuthRequest >= 2 )
        return;
      auth.setUsername("wrong");
      auth.setPassword("credentials");
    });

    dl->start();
    ev->run();
    BOOST_TEST_REQ_FAILED( dl );
    BOOST_REQUIRE_EQUAL( gotAuthRequest, 2 );
    BOOST_REQUIRE_EQUAL( dl->lastRequestError().type(), zyppng::NetworkRequestError::AuthFailed );
  }

  {
    int gotAuthRequest = 0;
    std::vector<zyppng::Download::State> allStates;
    auto dl = downloader->downloadFile( zyppng::DownloadSpec(weburl, targetFile.path()) );
    dl->spec().setTransferSettings(set);

    dl->sigFinished( ).connect([ &ev ]( zyppng::Download & ){
      ev->quit();
    });

    dl->sigAuthRequired().connect( [&]( zyppng::Download &, zyppng::NetworkAuthData &auth, const std::string &availAuth ){
      gotAuthRequest++;
      auth.setUrl( web.url() );
      auth.setUsername("test");
      auth.setPassword("test");
      auth.setLastDatabaseUpdate( time( nullptr ) );
    });

    dl->sigStateChanged().connect([&]( zyppng::Download &, zyppng::Download::State state ){
      allStates.push_back( state );
    });


    dl->start();
    ev->run();
    BOOST_TEST_REQ_SUCCESS( dl );
    BOOST_REQUIRE_EQUAL( gotAuthRequest, 1 );
    BOOST_REQUIRE ( allStates == std::vector<zyppng::Download::State>({ zyppng::Download::InitialState, zyppng::Download::DlMetaLinkInfo, zyppng::Download::PrepareMulti, zyppng::Download::DlMetalink, zyppng::Download::Finished}) );
  }
}
