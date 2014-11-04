/*
  Kvalobs - Free Quality Control Software for Meteorological Observations 

  $Id: testWMORaport.cc,v 1.1.6.2 2007/09/27 09:02:37 paule Exp $                                                       

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
#include <fstream>
#include <sstream>
#include <utility>
#include <boost/assign.hpp>
#include <boost/foreach.hpp>
#include "WMORaport.h"
#include "decodeArgv0.h"

using namespace std;


typedef std::pair<std::string, wmoraport::WmoRaport> RaportDefValue;
typedef std::list< RaportDefValue > RaportDef;


bool
readFile(const std::string &filename, std::string &content);

std::string
getDecoder( wmoraport::WmoRaport raportType, const RaportDef &raports);

void
printWmoRaport(const WMORaport &wmoRaports,
		       const wmoraport::WmoRaports &whatToCollect,
		       const RaportDef &rapDef);


int
main(int argn, char **argv)
{
	string myName=getCmdNameFromArgv0( argv[0]) ;
//	cerr << "Conffile: " << getConfFileFromArgv0( SYSCONFDIR, argv[0], false ) << endl;
//	cerr << "progname: " << myName << endl;

   wmoraport::WmoRaports whatToCollect;
  WMORaport wmo;
  string    raport;
  RaportDef rapDef;

  if(argn<2){
    cout << "\nBruk\n\n\t" << myName << " filename\n\n";
    return 1;
  }

  if(!readFile(argv[1], raport)){
    cout << "Cant open file <" << argv[1] << ">\n\n";
    return 1;
  }

  //  cout << raport;
  
  boost::assign::insert( whatToCollect )(wmoraport::SYNOP)(wmoraport::METAR)(wmoraport::BUFR_SURFACE);
  boost::assign::push_back( rapDef )( make_pair(string("bufr"), wmoraport::BUFR_SURFACE ) )
		  (make_pair(string("metar"), wmoraport::METAR) )(make_pair(string("synop"),wmoraport::SYNOP));
  if(!wmo.split(raport, whatToCollect )){
    cerr << wmo.error() << endl;
  }

  printWmoRaport( wmo, whatToCollect, rapDef );
  //cout << wmo << endl;

 cout << " ------- ERROR BEGIN ---------- " << endl;
 cerr << wmo.error() << endl;
 cout << " ------- ERROR END ---------- " << endl;
}

std::string
getDecoder( wmoraport::WmoRaport raportType, const RaportDef &raports)
{
   BOOST_FOREACH( RaportDefValue v, raports) {
      if( v.second == raportType )
         return v.first;
   }

   return "";
}



void
printWmoRaport(const WMORaport &wmoRaports,
		       const wmoraport::WmoRaports &whatToCollect,
		       const RaportDef &rapDef)
{
	ostringstream        ost;

	WMORaport::MsgMapsList raports;
	bool                 kvServerIsUp;
	bool                 tryToResend;
	string decoder;
	string theDecoder;
	ostringstream ostDecoder;
	raports=wmoRaports.getRaports( whatToCollect );

	BOOST_FOREACH(WMORaport::MsgMapsList::value_type raport, raports ) {
		theDecoder = getDecoder( raport.first, rapDef );

		if( theDecoder.empty() ) {
			cerr << "No decoder defined for wmo report <" << raport.first << ">. Cant send data to kvalobs." << endl;
			continue;
		}

		BOOST_FOREACH( WMORaport::MsgMap::value_type msgValList, *raport.second ) {
			ostDecoder.str("");
			ostDecoder << theDecoder;
			if( ! msgValList.first.decoderExtra.empty() )
				ostDecoder << "/"<<msgValList.first.decoderExtra;

			decoder = ostDecoder.str();

			BOOST_FOREACH( WMORaport::MsgList::value_type msg, msgValList.second ){
				ost.str("");

				if( msgValList.first.addWhatInFront )
					ost << msgValList.first.what << " " << endl << msg;
				else
					ost << msg;

				cerr << "-------- To Send -----------------------------\n";
				cerr << decoder << "\n" <<ost.str() << "\n\n" << endl;
				cerr << "-------- End Send ----------------------------\n";			}
		}
	}
}



bool
readFile(const std::string &filename, std::string &content)
{
	const int N=256;
	bool error = false;
	ifstream fist(filename.c_str(),  ios_base::binary | ios_base::in);
	char buf[N];
	int n;
	int size=0;
	int zerros=0;
	ostringstream ost(ios_base::binary | ios_base::out);

	ost.exceptions(std::fstream::failbit | std::fstream::badbit );

	content.clear();

	if(!fist){
		return false;
	}

	while( ! fist.eof() && !fist.fail() ) {
		fist.read(buf, N);
		n=fist.gcount();

		if( n > 0 ) {
			for( int i=0; i<n; ++i ) {
				if( buf[i]=='\0' )
					++zerros;
			}

			try {
				ost.write( buf, n );
				size += n;
			}
			catch( const std::exception &ex ) {
				cerr << "Exception: " << ex.what() << " size: " << size << " zeros: " << zerros << " n=" << n<< endl;
				error = true;
				break;
			}

		}
	}

	if( error && !fist.eof() ) {
		cerr << "ERROR error: " << (error?"true":"false")<< " ! eof" << endl;
		return false;
	}

	content=ost.str();

	cerr << "Content: size: " << content.size() << " readN: " << size << " zeros: " << zerros << endl;
	return true;
}
