/*
  Kvalobs - Free Quality Control Software for Meteorological Observations 

  $Id: norcom2kv.cc,v 1.6.6.5 2007/09/27 09:02:37 paule Exp $                                                       

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
#include <stdlib.h>
#include <unistd.h>
#include "App.h"
#include "CollectWmoReports.h"
#include <fileutil/pidfileutil.h>
#include <milog/milog.h>
#include "InitLogger.h"
#include "decodeArgv0.h"
#include <kvalobs/kvPath.h>

using namespace std;

string progname;


std::string rundir() {
  auto conf = App::getConfiguration();
  auto rdir=getDir(conf, "rundir");

  if( rdir.empty() ) {
    rdir=kvalobs::kvPath(kvalobs::rundir);
  }

  return fixPath(rdir);
}


int
main(int argn, char **argv)
{
  bool error;
  std::string pidfile;
  int         ret;
  dnmi::file::PidFileHelper pidFile;

  progname=getCmdNameFromArgv0( argv[0] );
  App::setConfFile( progname+".conf" );
  InitLogger(argn, argv, App::getConfiguration(), progname);
  pidfile = dnmi::file::createPidFileName( rundir(), progname );

  App app(argn, argv);
  
  if(dnmi::file::isRunningPidFile(pidfile, error)){
    if(error){
      LOGFATAL("An error occured while reading the pidfile:" << std::endl
	       << pidfile << " remove the file if it exist and"
	       << std::endl << "norcom2kv is not running. " << 
	       "If it is running and there is problems. Kill norcom2kv and"
	       << std::endl << "restart it." << std::endl << std::endl);
      return 1;
    }else{
      LOGFATAL("Is '" << progname << "' allready running?" << std::endl
	       << "If not remove the pidfile: " << pidfile);
      return 1;
    }
  }
  pidFile.createPidFile(pidfile);
  CollectWmoReports collectSynop(app);
  ret=collectSynop.run();
  return ret;

}
