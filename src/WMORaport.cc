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
const char *splitType[]={"METAR", "BBXX", 0};

regex zczc("^ *ZCZC *[0-9]* *");
regex nil("[0-9A-Za-z]* *NIL[ =]*", regex::icase| regex::normal);
//  regex nil("[0-9A-Za-z]* *NIL[ =]*");
regex NNNN("^ *N+ *");
regex synopType("^ *(AA|BB|OO)XX +(\\d{4}.)? *(\\w+)? *");
regex metarType("(^ *(METAR|SPECI))?(.*)");
//regex metarType("^ *(METAR|SPECI) *");
regex synop("^ *S(I|M|N)\\w+ +\\w+ +\\d+ *\\w*");
regex metar("^ *S(A|P)\\w+ +\\w+ +\\d+ *\\w*");
regex temp("^ *U(E|F|K|L|M|S|Z)\\w+ +\\w+ +\\d+ *\\w*");
regex pilo("^ *U(G|H|I|P|Q|Y)\\w+ +\\w+ +\\d+ *\\w*");
regex arep("^ *U(A|D)\\w+ +\\w+ +\\d+ *\\w*");
regex drau("^ *SS\\w+ +\\w+ +\\d+ *\\w*");
regex bath("^ *SO\\w+ +\\w+ +\\d+ *\\w*");
regex tide("^ *ISRZ\\w+ +\\w+ +\\d+ *\\w*");
regex endMsg("^ *([0-9A-Za-z/\\-\\+]| |=)* *= *");
//regex endMsg("^.*= .*");
//regex endMsg("^ *([0-9A-Za-z/]| )* *= *");
regex bl("^( |\t)*");
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

void
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

   line = skipEmptyLines( ist );

   if( line.empty() )
      return;

   boost::trim_left( line );
   if( ! regex_match( line.c_str(), what, ::synopType ) ){
      boost::trim_right( line );

      if( ! boost::starts_with( line, "NIL" ) )
         cout << "ERROR [" << line << "]" << endl;
      return;
   }
   ident = line;
   if( what[1] == "OO" )
      skip = true;


   while( getline( ist, line, '\n') ){
      if( ! regex_match( line.c_str(), what, ::synopType ) ){
         i = line.find("=");

         if( i != string::npos ) {
            line.erase( i+1 );
            buf << line << "\n";

            if( ! skip )
               synop_[ident].push_back( buf.str() );

            buf.str("");
         } else {
            buf << line << "\n";
         }
      } else {
         //New ident. I am uncertain if this is allowed, but
         //there is bulentines where this happend.
         ident = line;
         if( what[1] == "OO" )
              skip = true; //SYNOP mobile
         else
            skip = false;
      }
   }

   //Check if we have one left over without an = at the end.
   if( ! skip ) {
      line = buf.str();
      if( ! line.empty() ) {
         line += "=";
         synop_[ident].push_back( line );
      }
   }
}

void
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
      return;


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
            metar_[ident].push_back( buf.str() );
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
      metar_[ident].push_back( line );
   }

}

void
WMORaport::
doTEMP( std::istream &ist, const std::string &header )
{
   cout << "doTEMP: " << header << endl;
   //NOT implmented
}

void
WMORaport::
doPILO( std::istream &ist, const std::string &header )
{
   cout << "doPILO: " << header << endl;
   //NOT implmented
}

void
WMORaport::
doAREP( std::istream &ist, const std::string &header )
{
   cout << "doAREP: " << header << endl;
   //NOT implmented
}

void
WMORaport::
doDRAU( std::istream &ist, const std::string &header )
{
   cout << "doDRAU: " << header << endl;
   //NOT implmented
}

void
WMORaport::
doBATH( std::istream &ist, const std::string &header )
{
   cout << "doBATH: " << header << endl;
   //NOT implmented
}

void
WMORaport::
doTIDE( std::istream &ist, const std::string &header )
{
   cout << "doTIDE: " << header << endl;
   //NOT implmented
}


void
WMORaport::
dispatch( std::istream &ist )
{
   cmatch what;
   string buf;

   //Skip blank lines at the beginning.
   buf = skipEmptyLines( ist );

   if( buf.empty() )
      return;

   boost::trim_left( buf );

   if(regex_match(buf.c_str(), what, ::synop)){
      return doSYNOP( ist, buf );
   }else if(regex_match(buf.c_str(), what, ::metar)){
      return doMETAR( ist, buf );
   }else if(regex_match(buf.c_str(), what, ::temp)){
      return doTEMP( ist, buf );
   }else if(regex_match(buf.c_str(), what, ::pilo)){
      return doPILO( ist, buf );
   }else if(regex_match(buf.c_str(), what, ::arep)){
      return doAREP( ist, buf );
   }else if(regex_match(buf.c_str(), what, ::drau)){
      return doDRAU( ist, buf );
   }else if(regex_match(buf.c_str(), what, ::bath)){
      return doBATH( ist, buf );
   }else if(regex_match(buf.c_str(), what, ::tide)){
      return doTIDE( ist, buf );
   }else {
      //Unknown bulentin
   }
}


bool
WMORaport::
getMessage( std::istream &ist, std::ostream &msg )
{
   cmatch what;
   string line;
   string buf;
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

   if( ! zczcFound )
      return false;

   nEmpty = 0;

   //Search for the end mark (NNNN).
   while( getline( ist, line ) ) {
      ++lineno;
      buf = boost::trim_copy( line );

      if( buf.empty() ) {
         ++nEmpty;
         continue;
      }

      if( buf == "NNNN" && nEmpty > 0 )
         return true;

      if( nEmpty > 0 ) {
         msg << string( nEmpty, '\n');
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
   string buf;
   int i=0;

   notMatchedInGetMessage.str("");

   while( getMessage( ist, msg ) ){
      dispatch( msg );
      msg.clear();
      msg.str("");
      ++i;
   }

   string error=boost::trim_copy( notMatchedInGetMessage.str() );

   if( ! error.empty() ) {
      cout << " ----- BEGIN NOT MATCHED ---- " << endl;
      cout << notMatchedInGetMessage.str() << endl;
      cout << " ----- END NOT MATCHED ---- " << endl;
   }

}

bool
WMORaport::split(const std::string &raport)
{
   std::istringstream inputStream;
   string msg(raport);

   errorStr.str("");
   cleanCR(msg);
   inputStream.str(msg);

   return decode(inputStream);
}





void 
WMORaport::cleanCR(std::string &buf)const
{
   string::size_type i;

   i=buf.find('\r', 0);

   while(i!=string::npos){
      buf.erase(i, 1);
      i=buf.find('\r', i);
   }
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
getRaports( const std::list<wmoraport::WmoRaport> &raports )const
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
