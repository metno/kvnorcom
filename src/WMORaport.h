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
#ifndef __WMORaport_h__
#define __WMORaport_h__
#include <map> 
#include <list>
#include <string>
#include <sstream>

namespace wmoraport {
typedef enum{SYNOP, METAR, TEMP, PILO,AREP, DRAU, BATH, TIDE} WmoRaport;
}


class WMORaport{
 public:
  typedef std::list<std::string>                          MsgList;
  typedef std::list<std::string>::iterator               IMsgList;
  typedef std::list<std::string>::const_iterator        CIMsgList;
  typedef std::map<std::string, MsgList >                   MsgMap;
  typedef std::map<std::string, MsgList >::iterator        IMsgMap;
  typedef std::map<std::string, MsgList >::const_iterator CIMsgMap;
  typedef std::map<wmoraport::WmoRaport,const MsgMap*>  MsgMapsList;

 protected:
  typedef enum{START, ENDMSG, TYPE, BL,
		 SYNOP, METAR, TEMP, PILO,AREP, DRAU, BATH, TIDE, 
		 CONTINUE, ENDOFINPUT} Token;

  Token getToken(std::istringstream &inputStream,
		 std::string &buf);
  void cleanCR(std::string &buf)const;
  bool decode(std::istringstream &ist);
  std::string tokenString(Token token);
  void pushError(Token token, Token prevToken, const std::string &buf);

  std::ostringstream errorStr;
  int lineno;
  bool warnAsError;
  std::string putBackBuffer;

  MsgMap synop_;
  MsgMap temp_;
  MsgMap metar_;
  MsgMap pilo_;
  MsgMap arep_;
  MsgMap drau_;
  MsgMap bath_;
  MsgMap tide_;
  MsgMap errorMap_;


 public:
  WMORaport(bool warnAsError=false);
  WMORaport(const WMORaport &);
  ~WMORaport();

  WMORaport& operator=(const WMORaport &rhs);

  bool split(const std::string &raport);

  std::string error(){ return errorStr.str();}

  MsgMapsList getRaports( const std::list<wmoraport::WmoRaport> &raports )const;

  friend std::ostream& operator<<(std::ostream& output,
				  const WMORaport& r);
  
};

std::ostream& operator<<(std::ostream& output, wmoraport::WmoRaport raportType );



#endif
