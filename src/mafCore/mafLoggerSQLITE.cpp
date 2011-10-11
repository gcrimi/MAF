/*
 *  mafLoggerSQLITE.cpp
 *  mafCore
 *
 *  Created by Paolo Quadrani on 07/10/11.
 *  Copyright 2011 B3C. All rights reserved.
 *
 *  See License at: http://tiny.cc/QXJ4D
 *
 */

#include "mafLoggerSQLITE.h"
#include "mafObjectFactory.h"
#include "mafSQLITE.h"

#include <QDir>

using namespace mafCore;

int mafLoggerSQLITE::m_PrimaryLogKey = 0;

mafLoggerSQLITE::mafLoggerSQLITE(const QString code_location) : mafLogger(code_location), m_SQLITE(NULL), m_LastLogFile("") {
    initializeNewLogFile();
}

mafLoggerSQLITE::~mafLoggerSQLITE() {
    closeLastTempFile();
}

void mafLoggerSQLITE::loggedMessage(const QtMsgType type, const QString &msg) {
    QHash<QString, QVariant> recordHash;

    QString logtime(QDateTime::currentDateTime().toString("hh:mm:ss"));
    QString logtype("");
    
    switch (type) {
        case QtDebugMsg:
            if(logMode() == mafLogModeTestSuite) {
                logtype = TEST_SUITE_LOG_PREFIX;
            } else {
                logtype = "Debug";
            }
        break;
        case QtWarningMsg:
            if(logMode() == mafLogModeTestSuite) {
                logtype = TEST_SUITE_LOG_PREFIX;
            } else {
                logtype = "Warning";
            }
        break;
        case QtCriticalMsg: // System message types are defined as same type by Qt.
            logtype = "Critical";
        break;
        case QtFatalMsg:
            logtype = "Fatal";
        break;
    }

    recordHash.insert("id", QVariant(m_PrimaryLogKey));
    recordHash.insert("logtime", QVariant(logtime));
    recordHash.insert("logtype", QVariant(logtype));
    recordHash.insert("logtext", QVariant(msg));
    
    ++m_PrimaryLogKey;

    m_SQLITE->insertRecord(&recordHash);
}

void mafLoggerSQLITE::clearLogHistory() {
    closeLastTempFile();
    QFile::remove(m_LastLogFile);
    initializeNewLogFile();
}

void mafLoggerSQLITE::initializeNewLogFile() {
    REQUIRE(m_SQLITE == NULL);

    QString dbLogFilename = QDir::tempPath();
    dbLogFilename.append("/maf3Logs");
    QDir log_dir(dbLogFilename);
    if(!log_dir.exists()) {
        log_dir.mkpath(dbLogFilename);
    }
    dbLogFilename.append("/log_");
    dbLogFilename.append(QDateTime::currentDateTime().toString("yyyy_MMM_dd_hh_mm_ss"));
    dbLogFilename.append(".db");

    m_LastLogFile = dbLogFilename;

    QString logTableCreationString("create table logTable (id int primary key, logtime varchar(10), logtype varchar(30), logtext text)");
    m_SQLITE = new mafSQLITE(m_LastLogFile, "", mafCodeLocation);
    m_SQLITE->setQueryDb(logTableCreationString);
    m_SQLITE->setTableName("logTable");
}

void mafLoggerSQLITE::closeLastTempFile() {
    mafDEL(m_SQLITE);
}

bool mafLoggerSQLITE::isEqual(const mafObjectBase *obj) const {
    if(Superclass::isEqual(obj)) {
        if(this->m_SQLITE == ((mafLoggerSQLITE *)obj)->m_SQLITE &&
            this->m_LastLogFile == ((mafLoggerSQLITE *)obj)->m_LastLogFile) {
            return true;
        }
    }
    return false;
}
