/*
  Kvalobs - Free Quality Control Software for Meteorological Observations

  $Id: WMORaport.h,v 1.3.6.1 2007/09/27 09:02:37 paule Exp $

  Copyright (C) 2007 met.no

  Contact information:
  Norwegian Meteorological Institute
  Box 43 Blindern
  0313 OSLO
  NORWAY
  email: kvalobs-dev@met.no

  This file is part of KVALOBS

  KVALOBS is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License as
  published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  KVALOBS is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License along
  with KVALOBS; if not, write to the Free Software Foundation Inc.,
  51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include <iostream>
#include <boost/version.hpp>
#include <boost/filesystem.hpp>
#include "decodeArgv0.h"

namespace fs = boost::filesystem;
using namespace std;

#if BOOST_VERSION >= 104000
#
#  if BOOST_VERSION >= 104400
#    define BOOST_FILESYSTEM_VERSION 3
#  else
#    define BOOST_FILESYSTEM_VERSION 2
#  endif
# include <boost/system/error_code.hpp>
# include <boost/filesystem.hpp>
#else
#  error  "No compatible boost filesystem version found!  BOOST_VERSION"
#endif

using namespace std;
namespace fs = boost::filesystem;

namespace {
#if BOOST_FILESYSTEM_VERSION==2
string
filenameAsString( const fs::path &path) {
	return path.native_file_string();
}

string
filename(  const fs::path &path ) {
	return path.leaf();
}
#else
string
filenameAsString( const fs::path &path) {
	return path.string();
}

string
filename(  const fs::path &path ) {
	return path.filename().string();
}

#endif
}

std::string
getConfFileFromArgv0( const std::string &confdir, const std::string &argv0, bool checkIfFileExist  )
{
	fs::path myname( argv0 );
	string cmdname = filename( myname );
	fs::path confile = fs::path(confdir) / fs::path(cmdname + ".conf");

	if( checkIfFileExist && ! fs::exists(confile) ) {
		cerr << "FAILED to lookup confile '" << confile << "'."
				<< "The confile is interpretet from the cmdname '" << argv0 << "'!"
				<< endl << endl;
		exit( 2 );
	}

	return filenameAsString( confile );
}


std::string
getCmdNameFromArgv0( const std::string &argv0 )
{
	fs::path myname( argv0 );
	return filename( myname );
}



