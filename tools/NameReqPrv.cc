#define INCLUDE_TESTSETUP_WITHOUT_BOOST
#include "zypp/../tests/lib/TestSetup.h"
#undef  INCLUDE_TESTSETUP_WITHOUT_BOOST

#include <zypp/PoolQuery.h>

static std::string appname( "NameReqPrv" );

#define message cout
using std::flush;

int errexit( const std::string & msg_r = std::string(), int exit_r = 100 )
{
  if ( ! msg_r.empty() )
  {
    cerr << endl << msg_r << endl << endl;
  }
  return exit_r;
}

int usage( const std::string & msg_r = std::string(), int exit_r = 100 )
{
  if ( ! msg_r.empty() )
  {
    cerr << endl << msg_r << endl << endl;
  }
  cerr << "Usage: " << appname << " [--root ROOTDIR] [OPTIONS] NAME... [[OPTIONS] NAME...]..." << endl;
  cerr << "  Load all enabled repositories (no refresh) and search for" << endl;
  cerr << "  occurrences of NAME (regex) in package names, provides or" << endl;
  cerr << "  requires." << endl;
  cerr << "  --root   Load repos from the system located below ROOTDIR. If ROOTDIR" << endl;
  cerr << "           denotes a sover testcase, the testcase is loaded." << endl;
  cerr << "  -i/-I    turn on/off case insensitive search (default on)" << endl;
  cerr << "  -n/-N    turn on/off looking for names       (default on)" << endl;
  cerr << "  -p/-P    turn on/off looking for provides    (default on)" << endl;
  cerr << "  -r/-R    turn on/off looking for requires    (default off)" << endl;
  cerr << "  -a       short for -n -p -r" << endl;
  cerr << "  -A       short for -n -P -R" << endl;
  cerr << "TODO: Waiting for PoolQuery::allMatches switch and need to beautify output." << endl;
  cerr << "" << endl;
  return exit_r;
}

void tableOut( const std::string & s1 = std::string(),
               const std::string & s2 = std::string(),
               const std::string & s3 = std::string(),
               const std::string & s4 = std::string(),
               const std::string & s5 = std::string() )
{
  message << "  ";
#define TABEL(N) static unsigned w##N = 0; if ( ! s##N.empty() ) w##N = std::max( w##N, s##N.size() ); message << str::form( " %-*s ", w##N, s##N.c_str() )
#define TABER(N) static unsigned w##N = 0; if ( ! s##N.empty() ) w##N = std::max( w##N, s##N.size() ); message << str::form( " %*s ", w##N, s##N.c_str() )
  TABER( 1 ); TABEL( 2 ); TABEL( 3 ); TABEL( 4 ); TABEL( 5 );
#undef TABEL
  message << endl;
}

/******************************************************************
**
**      FUNCTION NAME : main
**      FUNCTION TYPE : int
*/
int main( int argc, char * argv[] )
{
  INT << "===[START]==========================================" << endl;
  appname = Pathname::basename( argv[0] );
  --argc,++argv;

  if ( ! argc )
  {
    return usage();
  }

  ///////////////////////////////////////////////////////////////////

  ZConfig::instance();
  Pathname sysRoot("/");
  sat::Pool satpool( sat::Pool::instance() );

  if ( (*argv) == std::string("--root") )
  {
    --argc,++argv;
    if ( ! argc )
      return errexit("--root requires an argument.");

    if ( ! PathInfo( *argv ).isDir() )
      return errexit("--root requires a directory.");

    sysRoot = *argv;
    --argc,++argv;
  }

  if ( TestSetup::isTestcase( sysRoot ) )
  {
    message << str::form( "*** Load Testcase from '%s'", sysRoot.c_str() ) << endl;
    TestSetup test;
    test.loadTestcaseRepos( sysRoot );
  }
  else if ( TestSetup::isTestSetup( sysRoot ) )
  {
    message << str::form( "*** Load TestSetup from '%s'", sysRoot.c_str() ) << endl;
    TestSetup test( sysRoot, Arch_x86_64 );
    test.loadRepos();
  }
  else
  {
    // a system
    message << str::form( "*** Load system at '%s'", sysRoot.c_str() ) << endl;
    if ( 1 )
    {
      message << "*** load target '" << Repository::systemRepoAlias() << "'\t" << endl;
      getZYpp()->initializeTarget( sysRoot );
      getZYpp()->target()->load();
      message << satpool.systemRepo() << endl;
    }

    if ( 1 )
    {
      RepoManager repoManager( sysRoot );
      RepoInfoList repos = repoManager.knownRepositories();
      for_( it, repos.begin(), repos.end() )
      {
        RepoInfo & nrepo( *it );

        if ( ! nrepo.enabled() )
          continue;

        if ( ! repoManager.isCached( nrepo ) )
        {
          message << str::form( "*** omit uncached repo '%s' (do 'zypper refresh')", nrepo.name().c_str() ) << endl;
          continue;
        }

        message << str::form( "*** load repo '%s'\t", nrepo.name().c_str() ) << flush;
        try
        {
          repoManager.loadFromCache( nrepo );
          message << satpool.reposFind( nrepo.alias() ) << endl;
        }
        catch ( const Exception & exp )
        {
          message << exp.asString() + "\n" + exp.historyAsString() << endl;
          message << str::form( "*** omit broken repo '%s' (do 'zypper refresh')", nrepo.name().c_str() ) << endl;
          continue;
        }
      }
    }
  }

  ///////////////////////////////////////////////////////////////////

  bool ignorecase( true );
  bool names     ( true );
  bool provides  ( true );
  bool requires  ( false );

  for ( ; argc; --argc,++argv )
  {
    if ( (*argv)[0] == '-' )
    {
      switch ( (*argv)[1] )
      {
        case 'a':  names =	true, 	requires = provides =	true;	break;
        case 'A':  names =	true, 	requires = provides =	false;	break;
        case 'i': ignorecase =	true;	break;
        case 'I': ignorecase =	false;	break;
        case 'n': names =	true;	break;
        case 'N': names =	false;	break;
        case 'r': requires =	true;	break;
        case 'R': requires =	false;	break;
        case 'p': provides =	true;	break;
        case 'P': provides =	false;	break;
      }
      continue;
    }

    PoolQuery q;
    std::string qstr( *argv );
    q.addString( qstr );
    q.setMatchRegex();
    q.setCaseSensitive( ! ignorecase );

    if ( names )
      q.addAttribute( sat::SolvAttr::name );
    if ( provides )
      q.addDependency( sat::SolvAttr::provides );
    if ( requires )
      q.addDependency( sat::SolvAttr::requires );

    message << *argv << " [" << (ignorecase?'i':'_') << (names?'n':'_') << (requires?'r':'_') << (provides?'p':'_') << "] {" << endl;

    for_( it, q.begin(), q.end() )
    {
      tableOut( str::numstring( it->id() ), it->asString(), it->repository().alias(), it->vendor().asString() );
      //message << "  " << *it << "(" << it->vendor() << ")";
      if ( ! it.matchesEmpty() )
      {
        for_( match, it.matchesBegin(), it.matchesEnd() )
        {
          //tableOut( match->inSolvAttr().asString().substr( 9, 1 ), match->asString() );
          tableOut( "", "", "", match->inSolvAttr().asString().substr( 9, 1 )+" " +match->asString() );
          //message << endl << "    " << match->inSolvAttr() << "\t" << match->asString();
        }
      }
      //message << endl;
    }

    message << "}" << endl;
  }

  INT << "===[END]============================================" << endl << endl;
  return 0;
}