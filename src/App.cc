/*
  Kvalobs - Free Quality Control Software for Meteorological Observations 

  $Id: App.cc,v 1.12.6.7 2007/09/27 09:02:37 paule Exp $                                                       

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
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <signal.h> 
#include <string.h>
#include <utility>
#include <fstream>
#include <sstream>
#include "boost/regex.hpp"
#include "boost/foreach.hpp"
#include "boost/lexical_cast.hpp"
#include "boost/algorithm/string.hpp"
#include "boost/filesystem.hpp"
#include "milog/milog.h"
#include "miutil/timeconvert.h"
#include "miconfparser/miconfparser.h"
#include "kvalobs/kvPath.h"
#include "kvsubscribe/HttpSendData.h"
#include "App.h"

using namespace std;
using namespace milog;
using namespace boost;

using miutil::conf::ConfSection;
using miutil::conf::ValElementList;
using miutil::conf::ValElement;
using miutil::conf::CIValElementList;

namespace fs=boost::filesystem;
namespace pt=boost::posix_time;

extern string progname;

namespace{
volatile sig_atomic_t sigTerm=0;
void sig_term(int);
void setSigHandlers();
void usage();
}


namespace {

App::RaportDef
getRaportConf( ConfSection *myConf ) {
   App::RaportDef raports;
   App::RaportDefValue rapVal;
   string::size_type i;
   string val;
   string reportType;
   string decoder;
   ValElementList valelem=myConf->getValue("raports");

   for ( ValElement &raport : valelem ) {
      val = raport.valAsString();

      i = val.find(":");
      if( i != string::npos ) {
         reportType = val.substr(0, i );
         decoder = val.substr( i + 1 );
      } else {
         reportType = val;
         decoder="";
      }

      boost::trim( reportType );
      boost::to_upper( reportType );

      if( decoder.empty() )
         decoder = boost::to_lower_copy( reportType );

      boost::trim( decoder );

      if( reportType == "AREP" )
         raports.push_back( make_pair( decoder, wmoraport::AREP ) );
      else if( reportType == "BATH" )
         raports.push_back( make_pair( decoder, wmoraport::BATH ) );
      else if( reportType == "DRAU" )
         raports.push_back( make_pair( decoder, wmoraport::DRAU ) );
      else if( reportType == "METAR" )
         raports.push_back( make_pair( decoder, wmoraport::METAR ) );
      else if( reportType == "PILO" )
         raports.push_back( make_pair( decoder, wmoraport::PILO ) );
      else if( reportType == "SYNOP" )
         raports.push_back( make_pair( decoder, wmoraport::SYNOP ) );
      else if( reportType == "TEMP" )
         raports.push_back( make_pair( decoder, wmoraport::TEMP ) );
      else if( reportType == "TIDE" )
         raports.push_back( make_pair( decoder, wmoraport::TIDE ) );
      else if( reportType == "BUFR_SURFACE" )
    	  raports.push_back( make_pair( decoder, wmoraport::BUFR_SURFACE ) );
      else
         LOGWARN( "Param <raports>: Unknown wmo raport '" << reportType << "' (" << val << ").");
   }

   if( raports.empty() ) {
      raports.push_back( make_pair("synop", wmoraport::SYNOP ) );
      raports.push_back( make_pair("bufr", wmoraport::BUFR_SURFACE ) );
   }

   ostringstream ost;
   ost << "Defined decoders:";

   for( App::RaportDefValue &v : raports )
	   ost << "\n    " << v.first << ":" << v.second;

   LOGINFO( ost.str() );
   return raports;
}

string
getDir( ConfSection *conf, const char *key )
{
   string val = boost::trim_copy(conf->getValue( key ).valAsString(""));
   if( val.rbegin() != val.rend() && *val.rbegin() != '/')
     val += "/";

   return val;
}

void
createDir( std::string &dir )
{
   try {
      if( ! fs::exists( dir ) ) {
         fs::create_directories( dir );
      }

      if( ! fs::is_directory( dir ) ) {
         LOGERROR("'" << dir <<"' exist, but is not a directory.");
         exit( 1 );
      }
   }
   catch( fs::filesystem_error &ex ) {
      LOGERROR("Failed to create directory '" << dir << "'.");
      exit( 1 );
   }

   if( dir.rbegin() != dir.rend() && *dir.rbegin() != '/')
      dir += "/";
}

TKvDataSrcList getKvServers(ConfSection *conf){
  TKvDataSrcList kvservers;

  if( !conf ){
    kvservers.push_back("");
  } else {
    for( ValElement &e: conf->getValue("kvservers"))
      kvservers.push_back(e.valAsString());

    if(kvservers.empty())
      kvservers.push_back("");  // So kvservers.front(), does'nt crash.
  }
  return kvservers;
}
}

void App::options(int argn, char **argv)
{
  for (int k=1; k<argn; k++){
     if (string(argv[k])=="--test"){
        test_=true;
     }
  }
}

App::App(int argn, 
         char **argv )
  :KvBaseApp( argn, argv),test_(false),
   refDataList(getKvServers(App::getConfiguration())),
   debug_(false),
   http(refDataList.front()){
   string            kvservers;
   ConfSection       *myConf=App::getConfiguration();

   if(!myConf){
      LOGFATAL("Cant read the configuration file <" << kvPath("sysconfdir") + "/" + progname +".conf>");
      exit(1);
   }

   if(refDataList.empty() || (refDataList.size()==1 && refDataList.front().empty())){
     LOGFATAL("At least one server to receive data must be specified!" << endl
              << "Servers is set up in the configuration file" <<endl <<
              "norcom2kv.conf!");
     exit(1);
   } else {
      ostringstream ost;
      for( string &server: refDataList)
        ost << " " << server;
      LOGINFO("Pushing data to kvDataInputd on: " << ost.str());
      refDataList.pop_front(); // This server is all ready configured with the http client.
   }


   debug_ = myConf->getValue("debug").valAsBool(false);

   if (myConf->getValue("ignore_files_before_startup").valAsBool(false))
     ignoreFilesBeforeStartup = pt::second_clock::universal_time();
   else
     ignoreFilesBeforeStartup = pt::ptime( pt::neg_infin );

   data2kvdir_=getDir(myConf, "workdir");

   if ( data2kvdir_.empty() )
      data2kvdir_ = kvPath("localstatedir", "norcom2kv")+"/data2kv/";

   tmpdir_ = data2kvdir_ + "tmp/";
   logdir_ = getDir(myConf, "logdir");

   if ( logdir_.empty() )
     logdir_   = kvPath("logdir" );

   createDir( data2kvdir_ );
   createDir( tmpdir_ );
   createDir( logdir_ );

   synopdir_=myConf->getValue("synopdir").valAsString("");

   if(synopdir_.empty())
     usage();

   synopdir_=checkdir(synopdir_);
   raports = getRaportConf( myConf );

   setSigHandlers();
}


App::~App()
{
}


std::string 
App::checkdir(const std::string &dir_, bool rwaccess)
{
   struct  stat sbuf;
   int     mode= R_OK;
   std::string dir(dir_);

   if(rwaccess)
      mode |= W_OK;

   if(stat(dir.c_str(), &sbuf)<0){
      LOGFATAL("Cant stat the file/path <" << dir << ">\n" <<
               "-- " << strerror(errno) << endl);
      exit(1);
   }

   if(!S_ISDIR(sbuf.st_mode)){
      LOGFATAL("The logdir=<" << dir << "> is not a directory!");
      exit(1);
   }

   if(access(dir.c_str(), mode)<0){
      LOGFATAL("norcom2kv must have read/write access to the '" <<
               dir <<"' directory\n");
      exit(1);
   }

   if(dir.rbegin()!=dir.rend() && *dir.rbegin()!='/')
      dir.append("/");

   return dir;
}


kvalobs::datasource::Result
App::sendDataToKvalobs(const std::string &message, 
                       const std::string &obsType,
                       std::string &sentTo)
{
  kvalobs::datasource::Result resToReturn;
  ostringstream ost;
  try {
    ost << http.host();
    resToReturn = http.newData(message, obsType);
  }
  catch( const std::exception &ex) {
    resToReturn.res = kvalobs::datasource::ERROR;
    resToReturn.message=ex.what();
    ost << " (FAILED)";
  }

  for( string &server : refDataList ) {
    try {
      ost << ", " << server;
      resToReturn = http.newData(server, message, obsType);
    }
    catch( const std::exception &ex) {
      ost << " (FAILED)";
    }
  }

  sentTo = ost.str();
  return resToReturn;
}



bool
App::saveFInfoList(const std::string &name, const FInfoList &infoList)
{
   ofstream fost(name.c_str(), ios::out);

   if(!fost){
      LOGERROR("Cant save infoList to file: " << endl << name);
      return false;
   }


   CIFInfoList it=infoList.begin();

   if(it==infoList.end()){
      LOGINFO("There is now information in the infoList to save!");
      fost.close();
      return true;
   }

   LOGINFO("Save Info for file(s). In file: " << name);


   for(;it!=infoList.end(); it++){
      fost << it->first << ":" << it->second.offset() << ":"
            << it->second.crc() << endl;
   }

   fost.close();

   return true;

}

bool
App::readFInfoList(const std::string &name, FInfoList &infoList)
{
   regex reg("(.*):(.*):(.*)");
   smatch what;
   ifstream fist(name.c_str());
   string   line;

   milog::LogContext cntxt("Init from file: "+progname +"_finfo.dat !");

   if(!fist){
      LOGINFO("No file information file!");
      return true;
   }

   while(getline(fist, line)){
      LOGDEBUG(line);

      if(regex_match(line, what, reg)){
         LOGDEBUG("Matched size:" << what.size());

         string fname(what[1].first, what[1].second);

         File fd(fname);

         if(!fd.ok()){
            LOGINFO("<" << fname << "> No longer exist!");
            continue;
         }

         FInfo fi(fd,
                  lexical_cast<long>(string(what[2].first, what[2].second)),
                  lexical_cast<unsigned int>(string(what[3].first, what[3].second))
         );

         infoList[fi.name()]=fi;
         LOGINFO("Adding <" << fi.name() << "> to the list of known files!");

      }else{
         LOGDEBUG("NOT Matched");
      }

   }

   return true;
}

void 
App::doShutdown()
{
   sigTerm=1;
}

bool 
App::inShutdown()const
{
   return sigTerm>0;
}


wmoraport::WmoRaports
App::
getRaportsToCollect()const
{
   wmoraport::WmoRaports ret;

   BOOST_FOREACH( RaportDefValue v, raports) {
      ret.insert( v.second );
   }

   return ret;
}

std::string
App::
getDecoder( wmoraport::WmoRaport raportType ) const
{
   BOOST_FOREACH( RaportDefValue v, raports) {
      if( v.second == raportType )
         return v.first;
   }

   return "";
}

namespace{
void
sig_term(int)
{
   sigTerm=1;
}


void
setSigHandlers()
{
   sigset_t         oldmask;
   struct sigaction act, oldact;


   act.sa_handler=sig_term;
   sigemptyset(&act.sa_mask);
   act.sa_flags=0;

   if(sigaction(SIGTERM, &act, &oldact)<0){
      LOGFATAL("Can't install signal handler for SIGTERM\n");
      exit(1);
   }

   act.sa_handler=sig_term;
   sigemptyset(&act.sa_mask);
   act.sa_flags=0;

   if(sigaction(SIGINT, &act, &oldact)<0){
      LOGFATAL("Can't install signal handler for SIGTERM\n");
      exit(1);
   }
}

void
usage()
{
   LOGFATAL(" \n\nUSE \n\n" <<
            "   " << progname << " OPTIONS \n"
            "     OPTIONS: \n"
            "       --test  [Use this to save additonal files to 'tmpdir'.\n"
            "                This files can be used for regression testing]\n"
            "       --traclevel[=level]\n"
            "       --loglevel[=level]\n"
            "       --help  print this help and exit.\n\n"
            "       \nlevel may be one of: \n"
            "        FATAL or 0\n"
            "        ERROR or 1\n"
            "        WARN  or 2\n"
            "        INFO  or 3\n"
            "        DEBUG or 4\n\n"
            "        DEBUGN where N is in the set [1,5].\n"
            "     - Default level is DEBUG, if no level is specified.\n"
            "\n\n");
   exit(1);
}
}
