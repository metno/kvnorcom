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
#include <sstream>
#include <algorithm>
#include <boost/regex.hpp>
#include <boost/foreach.hpp>
#include <boost/algorithm/string.hpp>
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
regex synopType("^ *(AA|BB|OO)XX +(\\d{4}.)? *(\\w+)? *");
regex synopIsNil("^\\s*\\d+ *NIL *=?\\s*");
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
skipEmptyLines( std::istream &ist )
{
   string line;
   string tmp;

   while( getline( ist, line, '\n' ) ) {
      tmp = boost::trim_left_copy( line );

      if( ! tmp.empty() )
         break;
   }

   return line;
}


bool
WMORaport::
doSYNOP( std::istream &ist, const std::string &header )
{
   bool skip = false;
   ostringstream buf;
   string::size_type i;
   string line;
   string tmp;
   cmatch what;
   string ident;
   bool first=true;

   line = skipEmptyLines( ist );

   if( line.empty() )
      return true;

   do {
      boost::trim_right( line );
      if( ! regex_match( line.c_str(), what, ::synopType ) ){

         i = line.find("=");

         if( i != string::npos ) {
            line.erase( i+1 ); //Clean eventually rubbish from the end.
            buf << line << "\n";

            if( ! skip && ! ident.empty()  ) {
               line=buf.str();
               boost::trim( line );
               if( ! regex_match( line.c_str(), what, ::synopIsNil ) )
                  synop_[ident].push_back( line );
            }

            buf.str("");
         } else {
            buf << line << "\n";
         }
      } else {
         buf.str();
         ident = line;
         if( what[1] == "OO" )
              skip = true; //SYNOP mobile
         else
            skip = false;
      }
   }while( getline( ist, line, '\n') );

   //Check if we have one left over without an = at the end.
   if( ! skip ) {
      line = buf.str();
      if( ! line.empty() ) {
         boost::trim( line );
         line +="=";
         if( ! regex_match( line.c_str(), what, ::synopIsNil ) )
            synop_[ident].push_back( line );
      }
   }

   //Remove all elements with an empty key in synop_. This
   //are garbage. We test the key after we have trimmed it.

   MsgMap::iterator itTmp;
   MsgMap::iterator it=synop_.begin();
   while( it != synop_.end() ) {
      if( boost::trim_copy(it->first) == "" ) {
         itTmp = it;
         ++it;
         synop_.erase( itTmp );
      }else {
         ++it;
      }
   }
   return true;
}

bool
WMORaport::
doMETAR( std::istream &ist, const std::string &header )
{
   ostringstream buf;
   string::size_type i;
   string line;
   cmatch what;
   string ident;

   line = skipEmptyLines( ist );

   if( line.empty() )
      return true;


   do {
      if( ! regex_match( line.c_str(), what, ::metarType ) ){
         boost::trim_right( line );

         if( ! boost::starts_with( line, "NIL" ) )
            cout << "ERROR [" << line << "]" << endl;
      }else {
         if( what[2].length() != 0 )
            ident = what[2];

         line = what[3];
         boost::trim_right( line );

         if( line.empty() )
            continue;

         i = line.find("=");

         if( i != string::npos ) {
            line.erase( i+1 );
            buf << line << "\n";
            line=buf.str();
            boost::trim( line );
            metar_[ident].push_back( line );
            buf.str("");
         } else {
            buf << line << "\n";
         }
      }
   } while( getline( ist, line, '\n') );

   //Check if we have one left over without an = at the end.
   line = buf.str();
   if( ! line.empty() ) {
      line += "=";
      boost::trim( line );
      metar_[ident].push_back( line );
   }

}

bool
WMORaport::
doTEMP( std::istream &ist, const std::string &header )
{
   errorStr << "TEMP: not implemented: " << header << endl;
   return true;
}

bool
WMORaport::
doPILO( std::istream &ist, const std::string &header )
{
   errorStr << "PILO: not implemented: " << header << endl;
}

bool
WMORaport::
doAREP( std::istream &ist, const std::string &header )
{
   errorStr << "AREP: not implemented: " << header << endl;
   return true;
}

bool
WMORaport::
doDRAU( std::istream &ist, const std::string &header )
{
   errorStr << "DRAU: not implemented: " << header << endl;
   return true;
}

bool
WMORaport::
doBATH( std::istream &ist, const std::string &header )
{
   errorStr << "BATH: not implemented: " << header << endl;
   return true;
}

bool
WMORaport::
doTIDE( std::istream &ist, const std::string &header )
{
   errorStr << "TIDE: not implemented: " << header << endl;
   return true;
}

bool
WMORaport::
doBUFR_SURFACE( std::istream &ist, const std::string &header )
{
   errorStr << "BUFR (SURFACE): not implemented: " << header << endl;
   return true;
}


bool
WMORaport::
doDispatch( doRaport func, wmoraport::WmoRaport raportType,
            std::istream &ist,  const string &header )
{
   if( ! raportsToCollect.empty() &&
         raportsToCollect.find( raportType ) == raportsToCollect.end() )
      return true;

   return (this->*func)( ist, header );
}

bool
WMORaport::
dispatch( std::istream &ist )
{
   cmatch what;
   string buf;

   //Skip blank lines at the beginning.
   buf = skipEmptyLines( ist );

   if( buf.empty() )
      return true;

   boost::trim( buf );

   if(regex_match(buf.c_str(), what, ::synop)){
      return doDispatch( &WMORaport::doSYNOP, wmoraport::SYNOP,
                         ist, buf );
   }else if(regex_match(buf.c_str(), what, ::metar)){
      return doDispatch( &WMORaport::doMETAR, wmoraport::METAR,
                         ist, buf );
   }else if(regex_match(buf.c_str(), what, ::temp)){
      return doDispatch( &WMORaport::doTEMP, wmoraport::TEMP,
                         ist, buf );
   }else if(regex_match(buf.c_str(), what, ::pilo)){
      return doDispatch( &WMORaport::doPILO, wmoraport::PILO,
                         ist, buf );
   }else if(regex_match(buf.c_str(), what, ::arep)){
      return doDispatch( &WMORaport::doAREP, wmoraport::AREP,
                         ist, buf );
   }else if(regex_match(buf.c_str(), what, ::drau)){
      return doDispatch( &WMORaport::doDRAU, wmoraport::DRAU,
                         ist, buf );
   }else if(regex_match(buf.c_str(), what, ::bath)){
      return doDispatch( &WMORaport::doBATH, wmoraport::BATH,
                         ist, buf );
   }else if(regex_match(buf.c_str(), what, ::tide)){
      return doDispatch( &WMORaport::doTIDE, wmoraport::TIDE,
                         ist, buf );
   }else if(regex_match(buf.c_str(), what, ::tide)){
      return doDispatch( &WMORaport::doBUFR_SURFACE, wmoraport::BUFR_SURFACE,
                         ist, buf );
   }else {
      errorStr << "UNKNOWN: bulletin: " << buf << endl;
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
getMessage( std::istream &ist, std::ostream &msg )
{
   cmatch what;
   string line;
   string buf;
   ostringstream empty;
   bool zczcFound=false;
   int nEmpty;

   //Search for the start of message (ZCZC nnn).
   //nnnn is optional.
   while( ! zczcFound &&  getline( ist, line, '\n' ) ) {
      ++lineno;
      if(! regex_match( line.c_str(), what, ::zczc)) {
         notMatchedInGetMessage << line << "\n";
         continue;
      }
      zczcFound = true;
   }

   if( ! zczcFound ) //Possibly the end of stream
      return false;

   nEmpty = 0;

   //Search for the end mark (NNNN).
   while( getline( ist, line ) ) {
      ++lineno;
      buf = boost::trim_copy( line );

      if( buf.empty() ) {
         empty << line << '\n';
         ++nEmpty;
         continue;
      }

      //We are a bit tolerant here in
      if( buf == "NNNN" && nEmpty > 4 && nEmpty <= 8 )
         return true;

      if( nEmpty > 0 ) {
         msg << empty.str();
         empty.str("");
         nEmpty = 0;
      }

      msg << line << "\n";
   }

   //We have reached the end of the input stream
   //without finding the end mark (NNNN). We
   //pretend the end mark i found and return true
   //anyway.
   return true;
}

bool
WMORaport::
decode(std::istream &ist)
{
   stringstream msg;
   string tmp;
   int i=0;

   notMatchedInGetMessage.str("");

   while( getMessage( ist, msg ) ){
      tmp = msg.str();
      if( ! dispatch( msg ) ) {
         errorStr << "ERROR: can't split bulletin segment[" << endl
               << tmp << "]" << endl;
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
   std::istringstream inputStream;
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

   output << " ---- SYNOP BEGIN ----" << endl;
   for( itm=r.synop_.begin();
        itm!=r.synop_.end(); itm++){
      output << "<<" << itm->first <<">>" << endl;
      for(itl=itm->second.begin();
            itl!=itm->second.end();
            itl++){
         output << *itl;
      }

      output << endl;
   }
   output << " ---- SYNOP END ----" << endl;

   output << " ---- TEMP BEGIN ----" << endl;
   for(itm=r.temp_.begin();
         itm!=r.temp_.end();
         itm++){
      output << itm->first << endl;
      for(itl=itm->second.begin();
            itl!=itm->second.end();
            itl++){
         output << *itl;
      }
      output << endl;
   }
   output << " ---- TEMP END ----" << endl;

   output << " ---- METAR BEGIN ----" << endl;
   for(itm=r.metar_.begin();
         itm!=r.metar_.end();
         itm++){
      output << "<<" << itm->first <<">>"<< endl;
      for(itl=itm->second.begin();
            itl!=itm->second.end();
            itl++){
         output << *itl;
      }

      output << endl;
   }
   output << " ---- METAR END ----" << endl;

   for(itm=r.pilo_.begin();
         itm!=r.pilo_.end();
         itm++){
      output << itm->first << endl;
      for(itl=itm->second.begin();
            itl!=itm->second.end();
            itl++){
         output << *itl;
      }

      output << endl;
   }

   for(itm=r.arep_.begin();
         itm!=r.arep_.end();
         itm++){
      output << itm->first << endl;
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
      output << itm->first << endl;
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
      output << itm->first << endl;
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
      output << itm->first << endl;
      for(itl=itm->second.begin();
            itl!=itm->second.end();
            itl++){
         output << *itl;
      }
      output << endl;
   }

   output << endl;

   if(r.errorMap_.begin() != r.errorMap_.end())
   {
      output << "-------ERROR MAP (BEGIN) ---------" << endl;

      for(itm=r.errorMap_.begin();
            itm!=r.errorMap_.end();
            itm++){
         output << itm->first << endl;
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
      default:
         out << "UNKNOWN";
   }
   return out;
}
