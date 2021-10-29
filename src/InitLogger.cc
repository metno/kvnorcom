/*
 Kvalobs - Free Quality Control Software for Meteorological Observations

 $Id: InitLogger.cc,v 1.1.6.3 2007/09/27 09:02:37 paule Exp $

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
#include <string.h>
#include <stdlib.h>
//#include <string>
#include <iostream>
#include <milog/milog.h>
#include <unistd.h>
#include "InitLogger.h"
#include <iostream>
#include <kvalobs/kvPath.h>
#include "App.h"

using namespace milog;
using namespace std;

namespace {
LogLevel getLogLevel(const std::string &loglevel);
LogLevel getLogLevel(const char *c);
}

void InitLogger(int argn, char **argv, miutil::conf::ConfSection *conf, const std::string &logname) {
  char *name;
  string filename;
  LogLevel traceLevel = milog::NOTSET;
  LogLevel logLevel = milog::NOTSET;
  FLogStream *fs;
  StdErrStream *trace;

  filename = getDir(conf, "logdir");
  if( filename.empty() ) {
    filename=kvPath("logdir");
  }
  
  filename=fixPath(filename);
  filename=filename+logname+".log";

  if (conf) {
    logLevel = getLogLevel(conf->getValue("loglevel").valAsString("INFO"));
    traceLevel = getLogLevel(conf->getValue("tracelevel").valAsString("DEBUG"));
  }

  for (int i = 0; i < argn; i++) {
    if (strcmp("--tracelevel", argv[i]) == 0) {
      i++;

      if (i < argn)
        traceLevel = getLogLevel(argv[i]);
    } else if (strcmp("--loglevel", argv[i]) == 0) {
      i++;

      if (i < argn)
        logLevel = getLogLevel(argv[i]);
    }
  }

  try {
    fs = new FLogStream(4, 1024 * 1024 * 4);  //4 Mb

    if (!fs->open(filename)) {
      std::cerr << "FATAL: Can't initialize the Logging system.\n";
      std::cerr << "------ Cant open the Logfile <" << filename << ">\n";
      delete fs;
      exit(1);
    }

    trace = new StdErrStream();

    if (!LogManager::createLogger(logname, trace)) {
      std::cerr << "FATAL: Can't initialize the Logging system.\n";
      std::cerr << "------ Cant create logger\n";
      exit(1);
    }

    if (!LogManager::addStream(logname, fs)) {
      std::cerr << "FATAL: Can't initialize the Logging system.\n";
      std::cerr << "------ Cant add filelogging to the Logging system\n";
      exit(1);
    }

    cerr << "Tracelevel: " << traceLevel << endl << "Loglevel:   " << logLevel << endl;

    trace->loglevel(traceLevel);
    fs->loglevel(logLevel);

    LogManager::setDefaultLogger(logname);
  } catch (...) {
    std::cerr << "FATAL: Can't initialize the Logging system.\n";
    std::cerr << "------ OUT OF MEMMORY!!!\n";
    exit(1);
  }

  std::cerr << "Logging to file <" << filename << ">!\n";
}

namespace {
LogLevel getLogLevel(const std::string &str) {
  return getLogLevel(str.c_str());
}
LogLevel getLogLevel(const char *str) {
  if (strcasecmp("FATAL", str) == 0) {
    return milog::FATAL;
  } else if (strcasecmp("ERROR", str) == 0) {
    return milog::ERROR;
  } else if (strcasecmp("WARN", str) == 0) {
    return milog::WARN;
  } else if (strcasecmp("DEBUG", str) == 0) {
    return milog::DEBUG;
  } else if (strcasecmp("DEBUG", str) == 0) {
    return milog::DEBUG;
  } else if (strcasecmp("DEBUG1", str) == 0) {
    return milog::DEBUG1;
  } else if (strcasecmp("DEBUG2", str) == 0) {
    return milog::DEBUG2;
  } else if (strcasecmp("DEBUG3", str) == 0) {
    return milog::DEBUG3;
  } else if (strcasecmp("DEBUG4", str) == 0) {
    return milog::DEBUG4;
  } else if (strcasecmp("DEBUG5", str) == 0) {
    return milog::DEBUG5;
  } else if (strcasecmp("DEBUG6", str) == 0) {
    return milog::DEBUG6;
  } else if (strcasecmp("INFO", str) == 0) {
    return milog::INFO;
  } else if (strcasecmp("0", str) == 0) {
    return milog::FATAL;
  } else if (strcasecmp("1", str) == 0) {
    return milog::ERROR;
  } else if (strcasecmp("2", str) == 0) {
    return milog::WARN;
  } else if (strcasecmp("3", str) == 0) {
    return milog::INFO;
  } else if (strcasecmp("4", str) == 0) {
    return milog::DEBUG;
  } else if (strcasecmp("5", str) == 0) {
    return milog::DEBUG1;
  } else if (strcasecmp("6", str) == 0) {
    return milog::DEBUG2;
  } else if (strcasecmp("7", str) == 0) {
    return milog::DEBUG3;
  } else if (strcasecmp("8", str) == 0) {
    return milog::DEBUG4;
  } else if (strcasecmp("9", str) == 0) {
    return milog::DEBUG5;
  } else if (strcasecmp("10", str) == 0) {
    return milog::DEBUG5;
  } else if (strcasecmp("11", str) == 0) {
    return milog::DEBUG6;
  } else {
    return milog::NOTSET;
  }
}
}

