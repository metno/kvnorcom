/*
  Kvalobs - Free Quality Control Software for Meteorological Observations 

  $Id: CollectSynop.cc,v 1.13.6.13 2007/09/27 09:02:37 paule Exp $                                                       

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
#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <sstream>
#include <fstream>
#include <boost/foreach.hpp>
#include <boost/algorithm/string.hpp>
#include <milog/milog.h>
#include <fileutil/dir.h>
#include <fileutil/copyfile.h>
#include "CollectWmoReports.h"
#include "crc_ccitt.h"
#include <puTools/miTime.h>

using namespace std;
using namespace miutil;
using kvalobs::datasource::Result;
extern string progname;

CollectWmoReports::CollectWmoReports(App &app_)
:app(app_), ignoreFilesBefore( app.ignoreFilesBeforeStartup )
{

}

CollectWmoReports::~CollectWmoReports()
{
}




bool
CollectWmoReports::readFile(const std::string &file, 
		std::string &content)const
{
	ifstream      fs(file.c_str());
	ostringstream ost;
	char		  ch;

	if(!fs){
		LOGERROR("Cant open file <" << file << ">!");
		return false;
	}

	while(fs.get(ch)){
		ost.put(ch);
	}

	if(!fs.eof()){
		LOGERROR("Error while reading file <" << file << ">!");
		fs.close();
		return false;
	}

	fs.close();
	content=ost.str();

	return true;
}

/**
 * getFileList finds files that  contains observations that
 * is to be sendt to kvalobs.
 */

bool 
CollectWmoReports::getFileList(FileList &fileList, 
		const std::string &path_,
		const std::string &pattern)
{
	dnmi::file::Dir   dir;
	string            file;
	string            path(path_);
	string            filepath;
	struct stat       sbuf;
	File              f;
	boost::posix_time::ptime ftime;

	fileList.clear();

	if(path.empty()){
		LOGERROR("getFileList: path is empty!!!!");
		return false;
	}
	path = fixPath(path);

	if(!dir.open(path, pattern)){
		LOGERROR("Cant read directory <" << path << ">!");
		return false;
	}

	//LOGDEBUG("Scan the directory: " << path << endl <<
	//	   "Pattern: <" << (pattern.empty()?"*":pattern)
	//	   << ">");

	while(dir.hasNext()){
		file=dir.next();

		if(file==".." || file==".")
			continue;

		filepath=path;
		filepath+=file;

		if(stat(filepath.c_str(), &sbuf)<0){
			LOGERROR("Cant get modification time for the file <" <<
					filepath << ">!");
			continue;
		}

		ftime = boost::posix_time::from_time_t( sbuf.st_mtime );

		if( ftime < ignoreFilesBefore )
			continue;

		f=File(filepath, sbuf);

		if(!f.isFile())
			continue;

		fileList.push_back(f);
	}

	return !fileList.empty();
}



/**
 * returns true if the file could be received by kvalobs and
 *         is saved i the databse. false otherwise.
 */

bool
CollectWmoReports::sendMessageToKvalobs(const std::string &msg,
		const std::string &obsType,
		bool &kvServerIsUp,
		bool &tryToResend)const
{
	string sendtTo;
	Result res;

	try {
	  res=app.sendDataToKvalobs(msg, obsType, sendtTo);
	}
	catch(const std::exception &x){
		kvServerIsUp=false;
		tryToResend=true;
		LOGERROR("Cant connect to kvalobs. Is kvalobs running?" << endl <<
				"Tryed to send to: " << sendtTo);
		return false;
	}

	LOGINFO("Sendt to servers: " << sendtTo);

	kvServerIsUp=true;

	if(res.res==kvalobs::datasource::OK){
		tryToResend=false;
		return true;
	}else if(res.res==kvalobs::datasource::NOTSAVED){
		tryToResend=true;
		LOGERROR("kvalobs NOTSAVED: " <<res.message);
	}else if(res.res==kvalobs::datasource::ERROR){
		LOGERROR("kvalobs ERROR: " << res.message);
		tryToResend=true;
	}else if(res.res==kvalobs::datasource::NODECODER){
		LOGERROR("kvalobs NODECODER: " << res.message);
		tryToResend=false;
	}else  if(res.res==kvalobs::datasource::DECODEERROR){

		string msg(res.message);
		string::size_type i=msg.find("unknown station/position");

		if( i == string::npos )
			i = msg.find("Missing or unknown stationid!");

		tryToResend=false;

		//Dont log Unknown station/position. Treat it as a
		//successfull transmit to kvalobs!
		if(i!=string::npos)
			return true;

		LOGERROR("kvalobs DECODEERROR (rejected): " << res.message);
	}else{
		LOGERROR("kvalobs Unknown response from kvalobs. Check if the code is in sync"
				<< " with 'datasource.idl'!");
		tryToResend=false;
	}


	return false;
}

int 
CollectWmoReports::run()
{
	const  int DELAY=3;
	const  int RESEND_DELAY=60;
	time_t  tNow;
	time_t  newObsCheckTime=0;
	time_t  savedObsCheckTime=0;
	bool    doSleep=true;

	if(app.synopdir().empty()){
		LOGFATAL("No SYNOP directory is given!");
		return 1;
	}

	std::string stateFile(app.workdir() + progname + "_finfo.dat");
	LOGINFO("CollectWmoReports: State file '" << stateFile << "'.");

	app.readFInfoList( stateFile, fileInfoList);

	while(!app.inShutdown()){
		time(&tNow);
		doSleep=true;

		if((tNow-newObsCheckTime)>=DELAY){
			newObsCheckTime=tNow;
			doSleep=false;

			if(checkForNewObservations()){
				collectObservations();
				app.saveFInfoList( stateFile,	fileInfoList);

			}
		}

		if((tNow-savedObsCheckTime)>=RESEND_DELAY){
			savedObsCheckTime=tNow;
			tryToSendSavedObservations();
			doSleep=false;
		}

		if(doSleep)
			sleep(1);

	}

	LOGDEBUG("Return from CollectSynop!");
	return 0;
}

void
CollectWmoReports::tryToSendSavedObservations()
{
	FileList  fileList;
	IFileList it;
	string    content;
	string::size_type i;
	string    type;
	bool      kvServerIsUp;
	bool      tryToResend;

	if(!getFileList(fileList, app.data2kvdir(),"kvdata_*")){
		//LOGDEBUG("No saved observations!");
		return;
	}

	LOGDEBUG("# saved obs: " << fileList.size());

	for(it=fileList.begin();
			it!=fileList.end() && !app.inShutdown();
			it++){

		if(!readFile(it->name(), content)){
			LOGERROR("Cant read the file: " << it->name());
			continue;
		}

		i=content.find("\n");

		if(i==string::npos){
			LOGERROR("Format error: in savedfile: " << it->name()
					<<   endl << "Expecting '\\n'");
			unlink(it->name().c_str());
			continue;
		}

		type=content.substr(0, i);

		if(type.empty()){
			LOGERROR("Format error: in savedfile: " << it->name()
					<<  endl << "Expecting 'type'");
			unlink(it->name().c_str());
			continue;
		}

		content.erase(0, i+1);

		if(app.test()){
			return;
		}

		if(!sendMessageToKvalobs(content, type, kvServerIsUp, tryToResend)){

			if(!tryToResend){
				LOGERROR("SAVEDOBS, kvalobs 'says' I should delete the observation.");
				unlink(it->name().c_str());
			}else{
				LOGERROR("Cant send saved observation to kvalobs." << endl
						<<"Will try to send later!");
			}


			//Can't send the observation to kvalobs.
			//Just break out of loop.
			if(!kvServerIsUp)
				break;
		}else{
			LOGINFO("Kvalobs got the observation. Delete local copy!");
			unlink(it->name().c_str());
		}
	}
}

void
CollectWmoReports::collectObservations()
{
	IFInfoList it;
	std::string buf;
	std::string newObsPart;

	LOGINFO("New observations to collect!");

	it=fileInfoList.begin();

	for(;it!=fileInfoList.end() && !app.inShutdown(); it++){
		if(it->second.toBeCollected()){
			string fromfile(it->second.copy());

			LOGINFO("Collect file: " << it->first << endl <<
					"From the copy: " << fromfile);

			if(!readFile(fromfile, buf)){
				LOGERROR("Can't read the file: " << fromfile);

				if(!app.debug())
					unlink(fromfile.c_str());

				it->second.seen(false);
				it->second.collected(false);
				continue;
			}

			File f(fromfile);

			if(!f.ok()){
				LOGERROR("Cant stat the file: " << fromfile );

				it->second.removecopy(!app.debug());

				it->second.seen(false);
				it->second.collected(false);
				continue;
			}

			it->second.removecopy(!app.debug());

			newObsPart=getNewObsPart(buf, it);
			it->second.collected(true);

			if(!newObsPart.empty()){
				doNewObs(it->first, newObsPart);
			}
		}
	}
}

void
CollectWmoReports::doNewObs(const std::string &obsFileName, const std::string &obs)
{
	std::string err;
	string      filename(obsFileName);
	string::size_type i;

	i=filename.find_last_of("/");

	if(i!=string::npos){
		filename.erase(0, i+1);
	}

	WMORaport wmoRaport;

	if( ! wmoRaport.split(obs, app.getRaportsToCollect() ) ){
		ostringstream ost;
		std::string fname;
		fname=writeFile(app.logdir()+"norcom2kv/", "dataerror_"+filename+"_" , true, obs);

		ost << "Cant split the WMO raport. " << endl
				<< "Error: " << wmoRaport.error() << endl
				<< "Saving the raport in: " << app.logdir();

		if(fname.empty()){
			ost <<  " Failed!"<< endl;
		}else{
			ost << " with name: " << endl
					<< "  " << fname;
		}

		LOGERROR(ost.str());
		return;
	}

	err=wmoRaport.error();

	if(!err.empty()){
		std::string fname;

		fname=writeFile(app.logdir()+"norcom2kv/", "datawarn_"+filename+"_", true, obs);

		if(!fname.empty()){
			LOGWARN("It was problems with the splitting of the WMO raport." << endl
					<< "Error: " << err << endl
					<<"The observation is written to file: " << endl
					<<"  " << fname );
		}else{
			LOGWARN("It was problems with the splitting of the WMO raport." << endl
					<< "ERROR: " << err
					<< "Failed to save the observation to file in directory: "
					<< endl << "  " << app.logdir());
		}
	}

	if(app.test()){
		string fname;
		ostringstream ost;
		ost << wmoRaport << endl
				<< "--------- ERROR(S) ---------" << endl
				<< wmoRaport.error() << endl;

		fname=writeFile(app.data2kvdir(), "WMORaport_"+filename+"_",
				true, ost.str());

		if(fname.empty()){
			LOGERROR("TEST: cant save WMORaport_'obsFile' to \n"
					"directory: " << app.data2kvdir());
		}else{
			LOGINFO("TEST: saved 'splitted' WMORaport to: " << endl
					<< fname);
		}

		fname=writeFile(app.data2kvdir(), filename+"_", true, obs);

		if(fname.empty()){
			LOGERROR("TEST: cant save 'obsFile' to \n"
					"directory: " << app.data2kvdir());
		}else{
			LOGINFO("TEST: saved obsfile to: " << endl
					<< fname);
		}
	}else{
		sendWMORaport(wmoRaport);
	}
}


void
CollectWmoReports::sendWMORaport(const WMORaport &wmoRaports)
{
	ostringstream        ost;

	WMORaport::MsgMapsList raports;
	bool                 kvServerIsUp;
	bool                 tryToResend;
	string decoder;
	string theDecoder;
	ostringstream ostDecoder;
	raports=wmoRaports.getRaports( app.getRaportsToCollect() );

	BOOST_FOREACH(WMORaport::MsgMapsList::value_type raport, raports ) {
		theDecoder = app.getDecoder( raport.first );

		if( theDecoder.empty() ) {
			LOGERROR("No decoder defined for wmo report <" << raport.first << ">. Cant send data to kvalobs.");
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
				LOGDEBUG( "sendWMORaport: decoder: '"<< decoder << "'\ndata[\n"<<ost.str() << "\n]data");
				if(!sendMessageToKvalobs(ost.str(), decoder, kvServerIsUp,tryToResend)){
					string fname;
					ostringstream kvOst;

					LOGERROR("Cant send observation to kvalobs." << endl  <<
							ost.str());
					kvOst << decoder << endl << ost.str() << endl;

					if(tryToResend){
						string prefix("kvdata_"+decoder+"_");
						fname=writeFile(app.data2kvdir(), prefix, true, kvOst.str());

						if(fname.empty()){
							LOGERROR("Cant save '" << prefix << "' in directory: " << endl
									<< app.data2kvdir());
						}else{
							LOGINFO("Saved: " << endl << fname << endl);
						}
					}
				}else{
					LOGINFO("Sendt observation to kvalobs!" << endl <<
							ost.str());
				}
			}
		}
	}
}

std::string
CollectWmoReports::writeFile(const std::string &dir, 
		const std::string &fname,
		bool  fnameIsTemplate,
		const std::string &content)
{
	const int MAX_COUNT=10000;
	ostringstream ost;
	int           i=0;
	miTime        now(miTime::nowTime());
	char          ts[100];
	std::string   file;
	FILE          *fd=0;
	size_t       fwRet;
	string       prefix;

	sprintf(ts, "%04d%02d%02dT%02d%02d%02d",
			now.year(), now.month(), now.day(),
			now.hour(), now.min(), now.sec());

	ost << dir << fname << ts;
	prefix=ost.str();

	if(fnameIsTemplate){
		while(i<MAX_COUNT){
			ost.str("");
			ost << prefix ;

			if(i!=0)
				ost << "_" << i;

			file=ost.str();
			fd=fopen(file.c_str(), "r");

			if(!fd){
				fd=fopen(file.c_str(), "w");

				if(fd)
					break; //Break out of the while loop
			}else {
				fclose(fd);
				fd = 0;
			}

			i++;
		}

		if(i>=MAX_COUNT && !fd)
			return string();

	}else{
		ost << dir << fname;
		file=ost.str();
		fd=fopen(file.c_str(), "w");

		if(!fd)
			return string();
	}

	if( ! fd )
		return "";

	fwRet=fwrite(content.c_str(), content.length(), 1, fd);
	fclose(fd);

	if(fwRet!=1){
		unlink(file.c_str());
		return string();
	}

	return file;
}

std::string
CollectWmoReports::getNewObsPart(const std::string &obs, IFInfoList &it)
{
	std::string  sub;
	std::string  newObs;
	unsigned int crc;

	if(it->second.offset()>0){
		if(it->second.offset()<=obs.length()){
			sub=obs.substr(0, it->second.offset());
			crc=crc_ccitt(sub);

			if(crc==it->second.crc()){
				newObs=obs.substr(it->second.offset());

				if(newObs.empty()){
					LOGDEBUG("New observation: No new data. " <<
							"The file has only been touched.\n");
				}else{
					LOGDEBUG("New observations: appended to file!\n");
				}
			}else{
				LOGDEBUG("New Observations: overwritten file!");
				newObs=obs;
			}
		}else{
			//The file is truncated.
			LOGDEBUG("The file is truncated! (overwritten file)");
			newObs=obs;
		}
	}else{
		LOGDEBUG("New observations: New file!");
		newObs=obs;
	}

	it->second.offset(obs.length());
	it->second.crc(crc_ccitt(obs));

	return newObs;
}


bool
CollectWmoReports::checkForNewObservations()
{
	string    message;
	string    obsType;
	FileList  fileList;
	IFileList it;
	IFInfoList fiIt;
	IFInfoList tmpFiIt;
	bool       hasNewFilesToCollect=false;

	if(!getFileList(fileList, app.synopdir())){
		LOGINFO("No new observations!");
		return false;
	}

	//We deletes all entries (files) in fileInfoList
	//that no longer is in the directory. ie. all files
	//that is in fileInfoList and not in fileList.

	fiIt=fileInfoList.begin();

	while(fiIt!=fileInfoList.end()){
		for(it=fileList.begin(); it!=fileList.end(); it++){
			if(it->name()==fiIt->first)
				break;
		}

		if(it==fileList.end()){
			LOGDEBUG("Erase <" << fiIt->first << "> from fileInfoList\n");
			tmpFiIt=fiIt;
			fiIt++;
			fileInfoList.erase(tmpFiIt);
		}else{
			fiIt++;
		}
	}

	//Checks if there is new files in the dierctory. Add
	//them to the fileInfoList.

	for(it=fileList.begin();
			it!=fileList.end();
			it++){

		fiIt=fileInfoList.find(it->name());

		if(fiIt==fileInfoList.end()){
			LOGDEBUG("New entrie: <" << it->name()
					<< "> in fileInfoList\n");
			fileInfoList[it->name()]=FInfo(*it);
		}else{
			if(fiIt->second.mtime() != it->mtime()){
				LOGDEBUG("New mtime: <" << it->name() << ">");

				try{
					fiIt->second.mtimeNow();
					fiIt->second.seen(false);
					fiIt->second.collected(false);
				}catch(FInfo::StatException &ex){
					LOGINFO("The file has gone: " << it->name());
					fileInfoList.erase(fiIt);
					fiIt=fileInfoList.end();
				}

			}else if(fiIt->second.seen()){  //fiIt->second.mtime() == it->mtime()
				if(!fiIt->second.collected()){
					LOGDEBUG("mtime seen a second time: <" << it->name() << ">\n");
					fiIt=copyFile(fileInfoList, fiIt);
				}
			}else{
				fiIt->second.seen(true);
			}
		}

		if(fiIt!=fileInfoList.end()){
			if(fiIt->second.toBeCollected()){
				LOGINFO("New observation in file: " << fiIt->second.name()
						<< endl << "A copy of the file is: " <<
						fiIt->second.copy());
				hasNewFilesToCollect=true;
			}
		}
	}

	return hasNewFilesToCollect;
}


IFInfoList 
CollectWmoReports::
copyFile(FInfoList &infoList, IFInfoList it)
{
	time_t oldtime=it->second.mtime();
	miTime now(miTime::nowTime());
	char buf[32];

	sprintf(buf, "_%04d%02d%02dT%02d%02d%02d",
			now.year(), now.month(), now.day(),
			now.hour(), now.min(), now.sec());

	string tofile=app.tmpdir()+it->second.namepart()+buf;

	LOGDEBUG("Copysynopfile: " << it->second.name() << endl <<
			"-----------to: " << tofile);

	if(!miutil::file::copyfile(it->second.name(), tofile)){
		LOGWARN("Cant copy synopfile: " << it->second.name() << endl <<
				"---------------- to: " << tofile <<
				"Removing <"<< it->second.name() <<"> from InfoList!");
		infoList.erase(it);
		return infoList.end();
	}

	try{
		it->second.mtimeNow();
	}
	catch(FInfo::StatException &ex){
		LOGINFO("The synop file has gone: " << it->second.name());
		infoList.erase(it);
		return infoList.end();
	}

	if(it->second.mtime()!=oldtime){
		LOGDEBUG("Synopfile: <" << it->second.name() <<
				"> has changed after copy!" << endl <<
				"Removing copy: " << tofile);
		unlink(tofile.c_str());
		it->second.seen(false);
		it->second.collected(false);
	}else{
		it->second.removecopy(!app.debug());

		it->second.copy(tofile);
	}

	return it;
}
