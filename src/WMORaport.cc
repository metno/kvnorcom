/*
  Kvalobs - Free Quality Control Software for Meteorological Observations 

  $Id: WMORaport.cc,v 1.6.2.2 2007/09/27 09:02:37 paule Exp $                                                       

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
#include <cctype>
#include <string>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <algorithm>
#include <boost/regex.hpp>
#include <boost/foreach.hpp>
#include <boost/algorithm/string.hpp>
#include <stdexcept>
#include <miutil/base64.h>
#include <miutil/trimstr.h>
#include "WMORaport.h"

using namespace std;
using namespace boost;

/**
 * Format of the WMO reports from NORCOM:
 * All message reports shall start with the
 * sequence ZCZC, and after that comes the message type. The message type
 * is on the form: TTAAii CCCC DTG [BBB]
 * The message type is coded in TT. Where the diffrent message types is as
 * follows:
 *    SI,SM,SN             -> synop
 *    SA,SP                -> metar
 *    UE,UF,UK,UL,UM,US,UZ -> temp
 *    UG,UH,Ui,UP,UQ,UY    -> pilo
 *    UA,UD                -> arep
 *    SS                   -> drau
 *    SO                   -> bath
 *    ISRZ                 -> tide
 */


namespace{

regex zczc("^ *ZCZC *[0-9]*\\r+");
regex synopType("(^ *((AA|BB|OO)XX +(\\d{4}.)? *(\\w+)?))?(.*)");
regex synopIsNil("^\\s*(\\d+)? *NIL *=?\\s*");
regex metarType("(^ *(METAR|SPECI))?(.*)");
//regex metarType("^ *(METAR|SPECI) *");
regex synop("^ *S(I|M|N)\\w{4} +\\w+ +\\d+ *\\w*");
regex metar("^ *S(A|P)\\w{4} +\\w+ +\\d+ *\\w*");
regex temp("^ *U(E|F|K|L|M|S|Z)\\w{4} +\\w+ +\\d+ *\\w*");
regex pilo("^ *U(G|H|I|P|Q|Y)\\w{4} +\\w+ +\\d+ *\\w*");
regex arep("^ *U(A|D)\\w{4} +\\w+ +\\d+ *\\w*");
regex drau("^ *SS\\w{4} +\\w+ +\\d+ *\\w*");
regex bath("^ *SO\\w{4} +\\w+ +\\d+ *\\w*");
regex tide("^ *ISRZ\\w{2}+ +\\w+ +\\d+ *\\w*");
regex bufrSurface("^ *IS(I|M|N)\\w{3} +\\w+ +\\d+ *\\w*");

bool
validChar( char ch, const char *valid )
{
	int i;
	for( i=0; valid[i] && valid[i]!=ch; ++i );
	return valid[i];
}

string
readToEof( std::istream &ist )
{
	const int N=256;
	char buf[N];
	int n;

	ostringstream ost(ios_base::out | ios_base::binary);

	while( ! ist.eof()  ) {
		if( ist.read(buf, N).fail() && ! ist.eof() ) {
			cerr << "ERROR 1: readToEof\n";
			break;
		}

		n=ist.gcount();

		if( n > 0 && ost.write( buf, n ).fail() ){
			cerr << "ERROR 2: readToEof\n";
			break;
		}
	}

	return ost.str();
}


void
skip( std::istream &ist, const char *what )
{
	char ch;

	while( !ist.eof() ) {
		if( ist.get( ch ).fail() )
			return;

		if( ! validChar( ch, what ) ) {
			ist.putback( ch );
			return;
		}
	}
}



bool
findZCZC( std::istream &ist, std::string &theZCZCline )
{
	const char *ZCZC="ZCZC";
	const char *validch=" 1234567890\r\n";
	int izc=0;
	char ch,prevch;
	bool found=false;
	bool isValidCh;
	theZCZCline.erase();

	//Search for the start of message (ZCZC nnn).
	//nnnn is optional.
	while( ! found && ! ist.eof() ) {
		if( ist.get( ch ).fail() && ! ist.eof())
			return false;

		if( ch == ZCZC[izc] ) {
			theZCZCline.push_back( ch );
			++izc;
		} else {
			izc=0;
			if( ZCZC[izc] != '\0')
				theZCZCline.erase();
		}


		if( ZCZC[izc] == '\0' ) //We have found ZCZC
			found = true;
	}

	if( ! found ) return false;

	do {
		prevch=ch;
		if( ist.get( ch ).fail() && ! ist.eof())
			return false;

		isValidCh = validChar( ch, validch);

		if( isValidCh )
			theZCZCline.push_back( ch );

	}while( isValidCh );

	ist.putback( ch );
	miutil::trimstr( theZCZCline );
	return prevch == '\n';
}




}

WMORaport::WMORaport(bool warnAsError_):
		  warnAsError(warnAsError_)
{
}

WMORaport::WMORaport(const WMORaport &r):
        												synop_(r.synop_),temp_(r.temp_), metar_(r.metar_), pilo_(r.pilo_),
        												arep_(r.arep_), drau_(r.drau_),bath_(r.bath_), tide_(r.tide_)
{
}

WMORaport::~WMORaport()
{
}


WMORaport& 
WMORaport::operator=(const WMORaport &rhs)
{
	if(this!=&rhs){
		synop_=rhs.synop_;
		temp_=rhs.temp_;
		metar_=rhs.metar_;
		pilo_=rhs.pilo_;
		arep_=rhs.arep_;
		drau_=rhs.drau_;
		bath_=rhs.bath_;
		tide_=rhs.tide_;
	}
	return *this;
}

std::string
WMORaport::
skipEmptyLines( std::istream &ist )const
{
	ostringstream line;
	char ch;

	skip( ist, " \t\r\n" );

	while(  ! ist.eof() ) {
		if( ist.get( ch ).fail() && ! ist.eof())
			break;;

		if( isalnum(ch) || ch==' ') {
			line << ch;
		} else {
			skip( ist, " \t\r\n" );
			break;
		}
	}

	return line.str();
}


bool
WMORaport::
readReport( std::istream &ist, std::string &report)const
{
	ostringstream buf;
	string::size_type i;
	string line;

	report.erase();
	line = skipEmptyLines( ist );

	if( line.empty() )
		return false;

	do {
		boost::trim_right( line );
		i = line.find("=");

		if( i != string::npos ) {
			line.erase( i+1 ); //Clean eventually rubbish from the end.
			buf << line << "\n";
			report = buf.str();
			return true;
		} else {
			buf << line << "\n";
		}
	}while( getline( ist, line, '\n') );

	line = buf.str();

	if( ! boost::trim_copy( line ).empty() ) {
		report = line + "\n";
		return true;
	}

	return false;
}

void
WMORaport::
removeEmptyKeys( MsgMap &msgMap )
{
	MsgMap::iterator itTmp;
	MsgMap::iterator it=msgMap.begin();

	while( it != msgMap.end() ) {
		if( boost::trim_copy(it->first.what) == "" ) {
			itTmp = it;
			++it;
			msgMap.erase( itTmp );
		}else {
			++it;
		}
	}
}


bool
WMORaport::
doSYNOP( std::istream &ist, const std::string &header, const std::string &theZCZCline )
{
	bool skip = false;
	string line;
	cmatch what;
	string ident;

	if( ! readReport( ist, line ) )
		return true;

	do {
		boost::trim_right( line );
		if( ! regex_match( line.c_str(), what, ::synopType ) ){
			//This should never happend
			continue;
		} else {
			if( what[2].length() > 0 ) {
				ident = what[2];
				if( what[3] == "OO" ) skip = true; //SYNOP mobile
				else skip = false;
			}

			line = what[6];

			if( line.empty() )
				continue;

			if( ! skip && ! ident.empty()  ) {
				boost::trim( line );
				if( ! regex_match( line.c_str(), what, ::synopIsNil ) ){
					synop_[MsgInfo(ident, true)].push_back( line );
				}
			}
		}
	}while( readReport( ist, line ) );

	removeEmptyKeys( synop_ );

	return true;
}

bool
WMORaport::
doMETAR( std::istream &ist, const std::string &header, const std::string &theZCZCline )
{
	string line;
	cmatch what;
	string ident;

	if( ! readReport( ist, line ) )
		return true;

	do {
		if( ! regex_match( line.c_str(), what, ::metarType ) ){
			//This should never happend.
			continue;
		}else {
			if( what[2].length() != 0 )
				ident = what[2];

			line = what[3];
			boost::trim( line );

			if( line.empty() )
				continue;

			metar_[MsgInfo(ident, true)].push_back( line );
		}
	} while( readReport( ist, line ) );

	removeEmptyKeys( metar_ );
}

bool
WMORaport::
doTEMP( std::istream &ist, const std::string &header, const std::string &theZCZCline )
{
	errorStr << "TEMP: not implemented: " << header << endl;
	return true;
}

bool
WMORaport::
doPILO( std::istream &ist, const std::string &header, const std::string &theZCZCline )
{
	errorStr << "PILO: not implemented: " << header << endl;
}

bool
WMORaport::
doAREP( std::istream &ist, const std::string &header, const std::string &theZCZCline )
{
	errorStr << "AREP: not implemented: " << header << endl;
	return true;
}

bool
WMORaport::
doDRAU( std::istream &ist, const std::string &header, const std::string &theZCZCline )
{
	errorStr << "DRAU: not implemented: " << header << endl;
	return true;
}

bool
WMORaport::
doBATH( std::istream &ist, const std::string &header, const std::string &theZCZCline )
{
	errorStr << "BATH: not implemented: " << header << endl;
	return true;
}

bool
WMORaport::
doTIDE( std::istream &ist, const std::string &header, const std::string &theZCZCline )
{
	errorStr << "TIDE: not implemented: " << header << endl;
	return true;
}

bool
WMORaport::
doBUFR_SURFACE( std::istream &ist, const std::string &header, const std::string &theZCZCline )
{
	static int count=0;
	++count;
	string data;
	string bufr;
	ostringstream ost;

	data = readToEof( ist );

	if( data.size() < 4 || data.substr(0, 4) != "BUFR") {
		return false;
	} else {
		if( data.substr(0, 4) != "BUFR" )
			return false;

		ost << theZCZCline << "\n" <<  header << "\n" << data;
		data = ost.str();
		miutil::encode64( data.data(), data.size(), bufr );
		bufrSurface_[MsgInfo("bufr_surface", "encoding=base64", false)].push_back( bufr );
		return true;
	}
}


bool
WMORaport::
doDispatch( doRaport func, wmoraport::WmoRaport raportType,
		std::istream &ist,  const string &header, const std::string &theZCZCline )
{
	if( ! raportsToCollect.empty() &&
			raportsToCollect.find( raportType ) == raportsToCollect.end() )
		return true;

	return (this->*func)( ist, header, theZCZCline );
}

bool
WMORaport::
dispatch( std::istream &ist, const std::string &theZCZCline )
{
	cmatch what;
	string buf;

	//Skip blank lines at the beginning and get the first line. This is the GTS header.
	buf = skipEmptyLines( ist );

	if( buf.empty() )
		return true;

	//boost::trim( buf );

	if(regex_match(buf.c_str(), what, ::synop)){
		return doDispatch( &WMORaport::doSYNOP, wmoraport::SYNOP,
				ist, buf, theZCZCline );
	}else if(regex_match(buf.c_str(), what, ::metar)){
		return doDispatch( &WMORaport::doMETAR, wmoraport::METAR,
				ist, buf, theZCZCline );
	}else if(regex_match(buf.c_str(), what, ::temp)){
		return doDispatch( &WMORaport::doTEMP, wmoraport::TEMP,
				ist, buf, theZCZCline );
	}else if(regex_match(buf.c_str(), what, ::pilo)){
		return doDispatch( &WMORaport::doPILO, wmoraport::PILO,
				ist, buf, theZCZCline );
	}else if(regex_match(buf.c_str(), what, ::arep)){
		return doDispatch( &WMORaport::doAREP, wmoraport::AREP,
				ist, buf, theZCZCline );
	}else if(regex_match(buf.c_str(), what, ::drau)){
		return doDispatch( &WMORaport::doDRAU, wmoraport::DRAU,
				ist, buf, theZCZCline );
	}else if(regex_match(buf.c_str(), what, ::bath)){
		return doDispatch( &WMORaport::doBATH, wmoraport::BATH,
				ist, buf, theZCZCline );
	}else if(regex_match(buf.c_str(), what, ::tide)){
		return doDispatch( &WMORaport::doTIDE, wmoraport::TIDE,
				ist, buf, theZCZCline );
	}else if(regex_match(buf.c_str(), what, ::bufrSurface)){
		return doDispatch( &WMORaport::doBUFR_SURFACE, wmoraport::BUFR_SURFACE,
				ist, buf, theZCZCline );
	}else {
		//errorStr << "UNKNOWN: bulletin: " << buf << endl;
		return true;
		//Unknown bulentin
	}
}



/**
 * A message starts with the sequence "ZCZC nnn\r\r\n" or
 * "ZCZC\r\r\n", ie nnn is optional. nnn represent a sequence
 * number in which the message are sendt.
 *
 * A message ends with the sequence "\n\n\n\n\n\n\nNNNN\r\r\n"
 *
 * The routine collects all data between the start sequence
 * and end sequence, excluding the sequence itself, and returns
 * the data in param msg.
 *
 * @param ist the input stream to collect data from.
 * @param[out] msg the collected data.
 */
bool
WMORaport::
getMessage( std::istream &ist, std::ostream &msg, std::string &theZCZCline )
{
	const char *NNNN="NNNN";
	int iNNN=0;
	char ch, prevCh;
	int rnCount=0;
	int count=0;
	bool complite = false;
	bool error=false;
	string buf;
	ostringstream ost( ios_base::out | ios_base::binary );

	//Search for the start of message (ZCZC nnn).
	//nnnn is optional.
	if( ! findZCZC( ist, theZCZCline ) )
		return false;

	//Search for the end mark (NNNN).
	while( !ist.eof() ) {
		if( ist.get( ch ).fail() )
			return false;

		ost << ch;

		if( NNNN[iNNN] == ch || ch == '\n' || ch == '\r' ) {
			++count;
			if( NNNN[iNNN] == ch ) {
				++iNNN;
			} else {
				++rnCount;
				iNNN = 0;
			}
		} else {
			count=0;
			iNNN=0;
			rnCount=0;
		}

		if( ! NNNN[iNNN] && rnCount > 2 ) {
			rnCount=0;
			while( true ) {
				prevCh = ch;

				if( ist.get( ch ).fail()  ) {
					return false;
				}

				if( validChar( ch, "\r\n") ) {
					++rnCount;
					++count;
					ost << ch;
				} else if( prevCh == '\n' && rnCount > 0 ) {
					ist.putback( ch );
					buf=ost.str();

					///Remove the last count chars from the buf, this is \n\n\n\n\n\n\nNNNN\r\r\n.
					buf.erase( buf.size() - count );
					msg << buf;
					return true;
				} else {
					break;
				}
			}
			return false;
			iNNN=0;
			rnCount=0;
			count=0;
		}
	}

	buf = ost.str();

	if( buf.size() == 0 )
		return false;

	msg << buf;
	//We have reached the end of the input stream
	//without finding the end mark (NNNN). We
	//pretend the end mark is found and return true
	//anyway.
	return true;
}


bool
WMORaport::
decode(std::istream &ist)
{
	stringstream msg( ios_base::out | ios_base::in | ios_base::binary );
	string tmp;
	string theZCZCline;
	int i=0;

	notMatchedInGetMessage.str("");

	while( ! ist.eof() ){
		if( getMessage( ist, msg, theZCZCline ) ) {
			tmp = msg.str();
			if( ! dispatch( msg, theZCZCline ) ) {
				errorStr << "ERROR: can't split bulletin segment[" << endl
						<< tmp << "]" << endl;
			}
		}
		msg.clear();
		msg.str("");
		++i;
	}

	string error=boost::trim_copy( notMatchedInGetMessage.str() );

	if( ! error.empty() ) {
		errorStr << " ----- BEGIN NOT MATCHED ---- " << endl;
		errorStr << notMatchedInGetMessage.str() << endl;
		errorStr << " ----- END NOT MATCHED ---- " << endl;
	}
	return true;
}

bool
WMORaport::split(const std::string &raport,
		const wmoraport::WmoRaports &collectRaports )
{
	std::istringstream inputStream( ios_base::in | ios_base::binary);
	string msg(raport);

	errorStr.str("");
	//   cleanCR(msg);
	inputStream.str(msg);
	raportsToCollect = collectRaports;
	return decode(inputStream);
}


std::ostream& 
operator<<(std::ostream& output,
		const WMORaport& r)
{
	WMORaport::CIMsgMap  itm;
	WMORaport::CIMsgList itl;

	if( r.synop_.begin() != r.synop_.end() ) {
		output << " ---- SYNOP BEGIN ----" << endl;
		for( itm=r.synop_.begin();
				itm!=r.synop_.end(); itm++){
			output << "<<" << itm->first.what <<">>" << endl;
			for(itl=itm->second.begin();
					itl!=itm->second.end();
					itl++){
				output << "["<< *itl <<"]"<< endl;
			}

			output << endl;
		}
		output << " ---- SYNOP END ----" << endl;
	}

	if( r.temp_.begin() != r.temp_.end() ) {
		output << " ---- TEMP BEGIN ----" << endl;
		for(itm=r.temp_.begin();
				itm!=r.temp_.end();
				itm++){
			output << itm->first.what << endl;
			for(itl=itm->second.begin();
					itl!=itm->second.end();
					itl++){
				output << *itl << endl;
			}
			output << endl;
		}
		output << " ---- TEMP END ----" << endl;
	}

	if( r.metar_.begin() != r.metar_.end() ) {
		output << " ---- METAR BEGIN ----" << endl;
		for(itm=r.metar_.begin();
				itm!=r.metar_.end();
				itm++){
			output << "<<" << itm->first.what <<">>"<< endl;
			for(itl=itm->second.begin();
					itl!=itm->second.end();
					itl++){
				output << *itl << endl;
			}

			output << endl;
		}
		output << " ---- METAR END ----" << endl;
	}

	if( r.pilo_.begin() != r.pilo_.end()) {
		for(itm=r.pilo_.begin();
				itm!=r.pilo_.end();
				itm++){
			output << itm->first.what << endl;
			for(itl=itm->second.begin();
					itl!=itm->second.end();
					itl++){
				output << *itl;
			}

			output << endl;
		}
	}

	for(itm=r.arep_.begin();
			itm!=r.arep_.end();
			itm++){
		output << itm->first.what << endl;
		for(itl=itm->second.begin();
				itl!=itm->second.end();
				itl++){
			output << *itl;
		}

		output << endl;
	}

	for(itm=r.drau_.begin();
			itm!=r.drau_.end();
			itm++){
		output << itm->first.what << endl;
		for(itl=itm->second.begin();
				itl!=itm->second.end();
				itl++){
			output << *itl;
		}

		output << endl;
	}

	for(itm=r.bath_.begin();
			itm!=r.bath_.end();
			itm++){
		output << itm->first.what << endl;
		for(itl=itm->second.begin();
				itl!=itm->second.end();
				itl++){
			output << *itl;
		}

		output << endl;
	}

	for(itm=r.tide_.begin();
			itm!=r.tide_.end();
			itm++){
		output << itm->first.what << endl;
		for(itl=itm->second.begin();
				itl!=itm->second.end();
				itl++){
			output << *itl;
		}
		output << endl;
	}


	if( r.bufrSurface_.begin() != r.bufrSurface_.end()) {
		output << " ---- BUFR BEGIN ----" << endl;
		for(itm=r.bufrSurface_.begin();
				itm!=r.bufrSurface_.end();
				itm++){
			output << itm->first.what << " (" << itm->first.what << ")"<< endl;
			for(itl=itm->second.begin();
					itl!=itm->second.end();
					itl++){
				output << itm->first.what  << " (" << itm->first.what << ")" << endl << *itl << endl;
			}
			output << endl;
		}
		output << " ---- BUFR END ----" << endl;
	}

	output << endl;

	if(r.errorMap_.begin() != r.errorMap_.end())
	{
		output << "-------ERROR MAP (BEGIN) ---------" << endl;

		for(itm=r.errorMap_.begin();
				itm!=r.errorMap_.end();
				itm++){
			output << itm->first.what << endl;
			for(itl=itm->second.begin();
					itl!=itm->second.end();
					itl++){
				output << *itl;
			}
			output << "-------ERROR MAP (END) ---------" << endl;
		}
	}

	return output;
}


WMORaport::MsgMapsList
WMORaport::
getRaports( const wmoraport::WmoRaports &raports )const
{
	MsgMapsList ret;

	BOOST_FOREACH( wmoraport::WmoRaport raport, raports ){
		switch( raport ) {
		case wmoraport::AREP: ret[wmoraport::AREP] = &arep_; break;
		case wmoraport::BATH: ret[wmoraport::BATH] = &bath_; break;
		case wmoraport::DRAU: ret[wmoraport::DRAU ] = &drau_; break;
		case wmoraport::METAR: ret[wmoraport::METAR] = &metar_; break;
		case wmoraport::PILO: ret[wmoraport::PILO] = &pilo_; break;
		case wmoraport::SYNOP: ret[wmoraport::SYNOP] = &synop_; break;
		case wmoraport::TEMP: ret[wmoraport::TEMP] = &temp_; break;
		case wmoraport::TIDE: ret[wmoraport::TIDE] = &tide_ ; break;
		case wmoraport::BUFR_SURFACE: ret[wmoraport::BUFR_SURFACE] = &bufrSurface_; break;
		default:
			continue;
		}
	}
	return ret;
}

std::ostream&
operator<<(std::ostream& out, wmoraport::WmoRaport raportType )
{
	switch( raportType ) {
	case wmoraport::AREP: out << "AREP"; break;
	case wmoraport::BATH: out << "BATH"; break;
	case wmoraport::DRAU: out << "DRAU"; break;
	case wmoraport::METAR: out << "METAR"; break;
	case wmoraport::PILO: out << "PILO"; break;
	case wmoraport::SYNOP: out << "SYNOP"; break;
	case wmoraport::TEMP: out << "TEMP"; break;
	case wmoraport::TIDE: out << "TIDE"; break;
	case wmoraport::BUFR_SURFACE: out << "BUFR_SURFACE"; break;
	default:
		out << "UNKNOWN";
	}
	return out;
}
