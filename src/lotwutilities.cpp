/***************************************************************************
                          lotwutilities.cpp  -  description
                             -------------------
    begin                : apr 2020
    copyright            : (C) 2020 by Jaime Robles
    email                : jaime@robles.es
 ***************************************************************************/

/*****************************************************************************
 * This file is part of KLog.                                                *
 *                                                                           *
 *    KLog is free software: you can redistribute it and/or modify           *
 *    it under the terms of the GNU General Public License as published by   *
 *    the Free Software Foundation, either version 3 of the License, or      *
 *    (at your option) any later version.                                    *
 *                                                                           *
 *    KLog is distributed in the hope that it will be useful,                *
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 *    GNU General Public License for more details.                           *
 *                                                                           *
 *    You should have received a copy of the GNU General Public License      *
 *    along with KLog.  If not, see <https://www.gnu.org/licenses/>.         *
 *                                                                           *
 *****************************************************************************/

#include "lotwutilities.h"
#include "callsign.h"
#include <QCoreApplication>
#include <QUrl>
#include <QNetworkRequest>
#include <QFile>
//#include <QDebug>

LoTWUtilities::LoTWUtilities(const QString &_klogDir, const QString &_klogVersion, const QString &_parentFunction, DataProxy_SQLite *dp)
{
    Q_UNUSED(_parentFunction);
#ifdef QT_DEBUG
  //qDebug() << Q_FUNC_INFO << ": "  << _klogDir << " - " << _parentFunction;
#else
#endif
    dataProxy = dp;
    calendar = new QCalendarWidget;
    util = new Utilities(Q_FUNC_INFO);
    util->setLongPrefixes(dataProxy->getLongPrefixes());
    util->setSpecialCalls(dataProxy->getSpecialCallsigns());

    manager = new QNetworkAccessManager(this);

    reply = nullptr;
    file = new QFile;
    //url = new QUrl;
    klogDir = _klogDir;
    klogVersion = _klogVersion;
    downloadAborted = false;
    stationCallsign.clear();
    startDate.clear();
    lotwQuery.clear();
    lotwUser.clear();
    lotwPassword.clear();
    fileName = "lotwDownload.adi";

    pDialog = new QProgressDialog(nullptr);
    pDialog->cancel();
    firstDate = QDate::currentDate();
    calendar->setToolTip(tr("Double click on the date that you want to use as the start date for downloading QSOs."));

    connect(calendar, SIGNAL(activated(QDate)), this, SLOT(slotCalendarDateSelected(QDate)));

   //qDebug() << "LoTWUtilities::LoTWUtilities(): - END" ;
}

LoTWUtilities::~LoTWUtilities()
{
    delete(dataProxy);
    delete(util);
    delete(file);
    delete(manager);
      //qDebug() << "LoTWUtilities::~LoTWUtilities" ;
}

void LoTWUtilities::slotCalendarDateSelected(const QDate _d)
{
   //qDebug() << "LoTWUtilities::slotCalendarDateSelected: " << _d.toString("yyyyMMdd") ;
    firstDate = _d;
    startThefullDownload();
}

void LoTWUtilities::setFileName(const QString &_fn)
{
    //qDebug() << "LoTWUtilities::setFileName: " << _fn ;
    if (_fn.length()>0)
    {
        fileName = _fn;
    }
    //qDebug() << "LoTWUtilities::setFileName - END"  ;
}

QString LoTWUtilities::getFileName()
{
    //qDebug() << "LoTWUtilities::getFileName: " << fileName  ;
    return fileName;
}

bool LoTWUtilities::selectQuery(const int _queryId)
{
     //qDebug() << "LoTWUtilities::selectQuery: - Start: " << QString::number(_queryId);
    bool savePassword = true;
    if (lotwPassword.length()<1)
    {
        savePassword = false;

        bool ok;
        lotwPassword = QInputDialog::getText(nullptr, tr("KLog - LoTW password needed"),
                                                   tr("Please enter your LoTW password: "), QLineEdit::Password, "", &ok);
        if (!ok)
        {
             //qDebug() << "LoTWUtilities::selectQuery: - END 1" <<  QT_ENDL;
            return false;
        }
    }
    switch (_queryId)
    {
    case 1: // Normal query
        lotwQuery = QString("https://lotw.arrl.org/lotwuser/lotwreport.adi?login=%1&password=%2&qso_query=1&qso_qsl=no&qso_owncall=%3&qso_startdate=%4").arg(lotwUser).arg(lotwPassword).arg(stationCallsign).arg(startDate);
        break;
    case 2:
        lotwQuery = QString("https://lotw.arrl.org/lotwuser/lotwreport.adi?login=%1&password=%2&qso_query=1&qso_qsl=no&qso_owncall=%3&qso_startdate=%4").arg(lotwUser).arg(lotwPassword).arg(stationCallsign).arg(firstDate.toString("yyyyMMdd"));
        break;
    default:
    {
        lotwQuery = QString("https://lotw.arrl.org/lotwuser/lotwreport.adi?login=%1&password=%2&qso_query=1&qso_qsl=no&qso_owncall=%3&qso_startdate=%4").arg(lotwUser).arg(lotwPassword).arg(stationCallsign).arg(startDate);
    }
    }

    if (!savePassword)
    {// We delete the password as soon as possible if the user is not willing to save it
        lotwPassword = QString();
    }
    url = QUrl(lotwQuery);

     //qDebug() << "LoTWUtilities::selectQuery: - END" <<  QT_ENDL;

    return true;
}

bool LoTWUtilities::setStationCallSign(const QString &_call)
{
    //qDebug() << "LoTWUtilities::setStationCallSign: " << _call;
    if (!util->isValidCall(_call))
    {
        //qDebug() << "LoTWUtilities::setStationCallSign: FALSE 1" ;
        return false;
    }
    if (((dataProxy->getStationCallSignsFromLog(-1)).contains(_call)))
    {
        //qDebug() << "LoTWUtilities::setStationCallSign: TRUE" ;
        stationCallsign = _call;
        QDate date = dataProxy->getFirstQSODateFromCall(stationCallsign);
        //qDebug() << "LoTWUtilities::setStationCallSign: Date: " << startDate ;
        if (date.isValid())
        {
            startDate = date.toString("yyyyMMdd");
             //qDebug() << "LoTWUtilities::setStationCallSign: StartDate" << startDate ;
        }
        else
        {
            startDate.clear();
             //qDebug() << "LoTWUtilities::setStationCallSign: StartDate not valid Date";
             //qDebug() << Q_FUNC_INFO << "False 2 - END";
            return false;
        }

        //qDebug() << "LoTWUtilities::setStationCallSign: startDate: " << startDate ;
        //qDebug() << Q_FUNC_INFO << "True 1 - END";
        return true;
    }
    else if (dataProxy->getHowManyQSOInLog(-1) <1)
    {
        //qDebug() << "LoTWUtilities::setStationCallSign:TRUE Empty log" ;
        stationCallsign = _call;
        //qDebug() << Q_FUNC_INFO << "True 2 - END";
        return true;
    }
    else
    {
        //qDebug() << "LoTWUtilities::setStationCallSign: Not a single QSO in the log with that StationCallsign: " << stationCallsign ;
        QMessageBox msgBox;
        msgBox.setIcon(QMessageBox::Question);
        msgBox.setWindowTitle(tr("KLog - LoTW Station callsign"));
        QString aux = QString(tr("There is not a single QSO in the log with that station callsign.") );
        msgBox.setText(tr("Are you sure that you want to use that station callsign (%1)?").arg(_call));
        msgBox.setDetailedText(aux);
        msgBox.setStandardButtons(QMessageBox::Yes|QMessageBox::No);
        msgBox.setDefaultButton(QMessageBox::Yes);
        int ret = msgBox.exec();

        if (ret == QMessageBox::Yes)
        {
            stationCallsign = _call;
            //qDebug() << Q_FUNC_INFO << "True 3 - END";
            return true;
        }
        else
        {
            //qDebug() << "LoTWUtilities::setStationCallSign: FALSE 2" ;
            //qDebug() << Q_FUNC_INFO << "FALSE 3 - END";
            return false;
        }
    }
}

void LoTWUtilities::startRequest(QUrl url)
{
    //qDebug() << "LoTWUtilities::startRequest: " << url.toString() ;
    QByteArray agent = QString("KLog-" + klogVersion).toUtf8();
    QNetworkRequest request;
    request.setUrl(url);
    request.setHeader(QNetworkRequest::UserAgentHeader, agent);
    //request.setRawHeader("User-Agent", agent);
    //reply = manager->get(QNetworkRequest(url));
    reply = manager->get(request);
    //qDebug() << "LoTWUtilities::startRequest - 10" ;
    // Whenever more data is received from the network,
    // this readyRead() signal is emitted
    connect(reply, SIGNAL(readyRead()), this, SLOT(slotReadyRead()));

    // Also, downloadProgress() signal is emitted when data is received
    connect(reply, SIGNAL(downloadProgress(qint64,qint64)), this, SLOT(slotDownloadProgress(qint64)));
    // This signal is emitted when the reply has finished processing.
    // After this signal is emitted,
    // there will be no more updates to the reply's data or metadata.
    connect(reply, SIGNAL(finished()), this, SLOT(slotFinished()));
    //qDebug() << "LoTWUtilities::startRequest: - END";
}

int LoTWUtilities::download()
{
    //qDebug() << "LoTWUtilities::download - Start";
    if (!selectQuery(-1))
    {
        //qDebug() << "LoTWUtilities::download - END-1";
        return -1;
    }
    //qDebug() << "LoTWUtilities::download: - 10";
    QFileInfo fileInfo(url.path());

    //qDebug() << "LoTWUtilities::download: - 11";
    if (QFile::exists(fileName))
    {
        //qDebug() << "LoTWUtilities::download: - 12";
        QMessageBox msgBox;
        msgBox.setIcon(QMessageBox::Question);
        msgBox.setWindowTitle(tr("KLog - LoTW File already exists"));
        QString aux = QString(tr("There is a file already existing with the name that will be used.") );
        msgBox.setText(tr("The file %1 already exist. Do you want to overwrite?").arg(fileName));
        msgBox.setDetailedText(aux);
        msgBox.setStandardButtons(QMessageBox::Yes|QMessageBox::No);
        msgBox.setDefaultButton(QMessageBox::Yes);
        int ret = msgBox.exec();
        //qDebug() << "LoTWUtilities::download: - 13";
        if (ret == QMessageBox::No)
        {
             //qDebug() << "LoTWUtilities::download - END- 2";
            return -2;
        }
      }
        //qDebug() << "LoTWUtilities::download: - 20 Filename: -" << fileName << "-";
        if (!file->isOpen())
        {
            //qDebug() << "LoTWUtilities::download: - 21 Filename: -";
            file->setFileName(fileName);
            //qDebug() << "LoTWUtilities::download: - 22 Filename: -";
        }
        //qDebug() << "LoTWUtilities::download: - 23 Filename: -";

      if (!file->open(QIODevice::WriteOnly)) /* Flawfinder: ignore */
      {
          QMessageBox msgBox;
          msgBox.setIcon(QMessageBox::Warning);
          msgBox.setWindowTitle(tr("KLog - LoTW Can't write the file"));
          QString aux = QString(tr("KLog was not able to save the file %1.\nError returned: %2") ).arg(fileName).arg(file->errorString());
          msgBox.setText(tr("The file %1 already exists.").arg(fileName));
          msgBox.setDetailedText(aux);
          msgBox.setStandardButtons(QMessageBox::Ok);
          msgBox.setDefaultButton(QMessageBox::Ok);
          msgBox.exec();
          //file->close();
          //delete file;
          //file = nullptr;
           //qDebug() << "LoTWUtilities::download - END - 3";
          return -3;
      }
    //qDebug() << "LoTWUtilities::download: - 30";
      // used for progressDialog
      // This will be set true when canceled from progress dialog
    downloadAborted = false;
    //qDebug() << "LoTWUtilities::download: - 31";
    //progressDialog = new QProgressDialog(nullptr);
    //qDebug() << "LoTWUtilities::download: - 40";
    pDialog->setLabelText(tr("Downloading data to file: %1.").arg(fileName));
    //qDebug() << "LoTWUtilities::download: - 41";
    pDialog->setWindowTitle(tr("KLog - LoTW download"));
    //qDebug() << "LoTWUtilities::download: - 42";
    pDialog->setWindowModality(Qt::WindowModal);
    //qDebug() << "LoTWUtilities::download: - 43";
    pDialog->reset();
    //qDebug() << "LoTWUtilities::download: - 44";
    pDialog->setRange(0, 0);
    //qDebug() << "LoTWUtilities::download: - 45";
    pDialog->setMinimumDuration(0);
    //qDebug() << "LoTWUtilities::download: - 46";
    pDialog->show();
    //qDebug() << "LoTWUtilities::download: - 47";

    connect(pDialog, SIGNAL(canceled()), this, SLOT(slotCancelDownload()));
    //qDebug() << "LoTWUtilities::download: - 50";

    startRequest(url);
     //qDebug() << "LoTWUtilities::download - END";
    return 1;
}

int LoTWUtilities::fullDownload()
{
   //qDebug() << "LoTWUtilities::fullDownload";
    QDate date = dataProxy->getFirstQSODateFromCall(stationCallsign);
   //qDebug() << "LoTWUtilities::fullDownload: Date: " << startDate ;
    if (date.isValid())
    {
        startDate = date.toString("yyyyMMdd");
        //qDebug() << "LoTWUtilities::fullDownload: StartDate" << startDate ;
    }
    QMessageBox msgBox;
    msgBox.setIcon(QMessageBox::Question);
    msgBox.setWindowTitle(tr("KLog - LoTW Start date selection"));
    QString aux = QString(tr("This is the first date of a QSO with the callsign %1 in this log If you think that in LoTW you may have previous QSOs, answer No.").arg(stationCallsign) );
    msgBox.setText(tr("Do you want to use this date (%1) as start date?").arg(startDate));
    msgBox.setDetailedText(aux);
    msgBox.setStandardButtons(QMessageBox::Yes|QMessageBox::No);
    msgBox.setDefaultButton(QMessageBox::Yes);
    int ret = msgBox.exec();
    if (ret == QMessageBox::Yes)
    {
         //qDebug() << "LoTWUtilities::fulldownload - Yes";
          firstDate = date;
          startThefullDownload();
          return 1;
    }

    calendar->setSelectedDate(firstDate);
    calendar->setGridVisible(true);
    calendar->setMaximumDate(QDate::currentDate());
    calendar->show();
   //qDebug() << "LoTWUtilities::fullDownload - END";
    return 2;
}

int LoTWUtilities::startThefullDownload()
{
   //qDebug() << "LoTWUtilities::startThefulldownload - Start";
    if (calendar->isVisible())
    {
        calendar->close();
    }

    if (!selectQuery(2))
    {
       //qDebug() << "LoTWUtilities::startThefulldownload - END-1";
        return -1;
    }
   //qDebug() << "LoTWUtilities::startThefulldownload: - 10";
    QFileInfo fileInfo(url.path());

   //qDebug() << "LoTWUtilities::startThefulldownload: - 11";
   if (QFile::exists(fileName))
   {
       //qDebug() << "LoTWUtilities::startThefulldownload: - 12";
       QMessageBox msgBox;
       msgBox.setIcon(QMessageBox::Question);
       msgBox.setWindowTitle(tr("KLog - LoTW File already exists"));
       QString aux = QString(tr("There is a file already existing with the name that will be used.") );
       msgBox.setText(tr("The file %1 already exist. Do you want to overwrite?").arg(fileName));
       msgBox.setDetailedText(aux);
       msgBox.setStandardButtons(QMessageBox::Yes|QMessageBox::No);
       msgBox.setDefaultButton(QMessageBox::Yes);
       int ret = msgBox.exec();
       //qDebug() << "LoTWUtilities::startThefulldownload: - 13";
       if (ret == QMessageBox::No)
       {
            //qDebug() << "LoTWUtilities::startThefulldownload - END- 2";
           return -2;
       }
     }
       //qDebug() << "LoTWUtilities::startThefulldownload: - 20 Filename: -" << fileName << "-";
       if (!file->isOpen())
       {
           //qDebug() << "LoTWUtilities::startThefulldownload: - 21 Filename: -";
           file->setFileName(fileName);
           //qDebug() << "LoTWUtilities::startThefulldownload: - 22 Filename: -";
       }
       //qDebug() << "LoTWUtilities::startThefulldownload: - 23 Filename: -";

     if (!file->open(QIODevice::WriteOnly)) /* Flawfinder: ignore */
     {
         QMessageBox msgBox;
         msgBox.setIcon(QMessageBox::Warning);
         msgBox.setWindowTitle(tr("KLog - LoTW Can't write the file"));
         QString aux = QString(tr("KLog was not able to save the file %1.\nError returned: %2") ).arg(fileName).arg(file->errorString());
         msgBox.setText(tr("The file %1 already exists.").arg(fileName));
         msgBox.setDetailedText(aux);
         msgBox.setStandardButtons(QMessageBox::Ok);
         msgBox.setDefaultButton(QMessageBox::Ok);
         msgBox.exec();
         //file->close();
         //delete file;
         //file = nullptr;
          //qDebug() << "LoTWUtilities::startThefulldownload - END - 3";
         return -3;
     }
   //qDebug() << "LoTWUtilities::startThefulldownload: - 30";
     // used for progressDialog
     // This will be set true when canceled from progress dialog
   downloadAborted = false;
   //qDebug() << "LoTWUtilities::startThefulldownload: - 31";
   //progressDialog = new QProgressDialog(nullptr);
   //qDebug() << "LoTWUtilities::startThefulldownload: - 40";
   pDialog->setLabelText(tr("Downloading data to file: %1.").arg(fileName));
   //qDebug() << "LoTWUtilities::startThefulldownload: - 41";
   pDialog->setWindowTitle(tr("KLog - LoTW download"));
   //qDebug() << "LoTWUtilities::startThefulldownload: - 42";
   pDialog->setWindowModality(Qt::WindowModal);
   //qDebug() << "LoTWUtilities::startThefulldownload: - 43";
   pDialog->reset();
   //qDebug() << "LoTWUtilities::startThefulldownload: - 44";
   pDialog->setRange(0, 0);
   //qDebug() << "LoTWUtilities::startThefulldownload: - 45";
   pDialog->setMinimumDuration(0);
   //qDebug() << "LoTWUtilities::startThefulldownload: - 46";
   pDialog->show();
   //qDebug() << "LoTWUtilities::startThefulldownload: - 47";

   connect(pDialog, SIGNAL(canceled()), this, SLOT(slotCancelDownload()));
   //qDebug() << "LoTWUtilities::startThefulldownload: - 50";

   startRequest(url);
    //qDebug() << "LoTWUtilities::startThefulldownload - END";

    return 1;
}

void LoTWUtilities::slotDownloadProgress(qint64 bytesRead) {
    //qDebug() << "LoTWUtilities::slotDownloadProgress: " << QString::number(bytesRead);
    if (downloadAborted)
    {
         //qDebug() << "LoTWUtilities::slotDownloadProgress: CANCELLED";
        return;
    }

    pDialog->setValue(bytesRead);
     //qDebug() << "LoTWUtilities::slotDownloadProgress - END ";
}

void LoTWUtilities::slotReadyRead()
{
    //qDebug() << "LoTWUtilities::slotReadyRead: " << reply->readLine();
    if (file)
    {
        file->write(reply->readAll());
    }
    //qDebug() << "LoTWUtilities::slotReadyRead - END";
}

void LoTWUtilities::slotFinished()
{
    //qDebug() << "LoTWUtilities::slotFinished - Started";
    // when canceled
     if (downloadAborted)
     {
         if (file)
         {
             file->close();
             file->remove();
             //delete file;
             //file = nullptr;
         }
         //reply->deleteLater();
         pDialog->cancel();
         reply->close();
         //qDebug() << "LoTWUtilities::slotFinished - END Canceled";
         return;
     }
    //qDebug() << "LoTWUtilities::slotFinished - 10";
     // download finished normally
    pDialog->cancel();
    //qDebug() << "LoTWUtilities::slotFinished - 11";
    file->flush();
    //qDebug() << "LoTWUtilities::slotFinished - 12";
    file->close();
    //qDebug() << "LoTWUtilities::slotFinished - 13";

     // get redirection url
     QVariant redirectionTarget = reply->attribute(QNetworkRequest::RedirectionTargetAttribute);
     //qDebug() << "LoTWUtilities::slotFinished - 14";
     if (reply->error())
     {
         file->remove();
         QMessageBox msgBox;
         msgBox.setIcon(QMessageBox::Warning);
         msgBox.setWindowTitle(tr("KLog - LoTW Download error"));
         QString aux;
         msgBox.setText(tr("There was an error (%1) while downloading the file from LoTW.").arg(QString::number(reply->error())));
         aux = QString(tr("The downloading error details are: %1") ).arg(reply->errorString());
         msgBox.setDetailedText(aux);
         msgBox.setStandardButtons(QMessageBox::Ok);
         msgBox.setDefaultButton(QMessageBox::Ok);
         msgBox.exec();
    }
    else if (!redirectionTarget.isNull())
    {
         //qDebug() << "LoTWUtilities::slotFinished - Redirection";
        QUrl newUrl = url.resolved(redirectionTarget.toUrl());
        QMessageBox msgBox;
        msgBox.setIcon(QMessageBox::Question);
        msgBox.setWindowTitle(tr("KLog - LoTW Redirection found"));
        QString aux = QString(tr("The remote server redirected our connection to %1") ).arg(newUrl.toString());
        msgBox.setText(tr("Do you want to follow the redirection?"));
        msgBox.setDetailedText(aux);
        msgBox.setStandardButtons(QMessageBox::Yes|QMessageBox::No);
        msgBox.setDefaultButton(QMessageBox::Yes);
        int ret = msgBox.exec();
        if (ret == QMessageBox::Yes)
        {
            url = newUrl;
            //reply->deleteLater();
            file->open(QIODevice::WriteOnly); /* Flawfinder: ignore */
            file->resize(0);
            startRequest(url);
             //qDebug() << "LoTWUtilities::slotFinished - END-1";
            return;
        }
     }
    else
    {
        //qDebug() << "LoTWUtilities::slotFinished:  " ;
    }
    //qDebug() << "LoTWUtilities::slotReadyRead - Going to parse ...";
    parseDownloadedFile(file->fileName());
    //qDebug() << "LoTWUtilities::slotReadyRead - END";
}

void LoTWUtilities::slotCancelDownload()
{
     //qDebug() << "LoTWUtilities::slotCancelDownload - Start";
    downloadAborted = true;
    reply->abort();
     //qDebug() << "LoTWUtilities::slotCancelDownload - END";
}

void LoTWUtilities::setUser(const QString &_call)
{
     //qDebug() << "LoTWUtilities::setUser: " << _call;
    lotwUser = _call;
     //qDebug() << "LoTWUtilities::setUser: END";
}

void LoTWUtilities::setPass(const QString &_pass)
{
     //qDebug() << "LoTWUtilities::setPass: " << _pass;
    lotwPassword = _pass;
     //qDebug() << "LoTWUtilities::setPass: END";
}

bool LoTWUtilities::getIsReady()
{
     //qDebug() << "LoTWUtilities::getIsReady: user/station: -" << lotwUser <<"/" << stationCallsign << "-";
    if ((lotwUser.length()>1) && (stationCallsign.length()>1))
    {
         //qDebug() << "LoTWUtilities::getIsReady: true";
        return true;
    }
    else
    {
         //qDebug() << "LoTWUtilities::getIsReady: false";
      return false;
    }
}

void LoTWUtilities::parseDownloadedFile(const QString &_fn)
{
    //qDebug() << "LoTWUtilities::parseDownloadedFile: " << _fn;
    QString _fileName = _fn;
    QMessageBox msgBox;
    QString aux;

    QFile file( _fileName );
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) /* Flawfinder: ignore */
    {
        //qDebug() << "LoTWUtilities::parseDownloadedFile File not found" << _fileName;
        msgBox.setIcon(QMessageBox::Warning);
        msgBox.setWindowTitle(tr("KLog - LoTW File not found"));
        msgBox.setText(tr("KLog can't find the downloaded file."));
        aux = QString(tr("It was not possible for find the file %1 that has been just downloaded.") ).arg(_fn);

        msgBox.setDetailedText(aux);
        msgBox.setStandardButtons(QMessageBox::Ok);
        msgBox.setDefaultButton(QMessageBox::Ok);
        msgBox.exec();
        //qDebug() << "LoTWUtilities::parseDownloadedFile: END-1";
        return ;
    }
    else
    {
        //qint64 startOfFile = file.pos();
        // Look for a Header
        bool hasHeader = false;
        int numQSO = 0;
        QString Lotw_owncall = QString("OWNCALL: %1").arg(stationCallsign.toUpper());
        //bool hasOwnCall = false;
        //bool hasProgramID = false;
        bool userPasswordError = false;
        while ((!file.atEnd()) && (!hasHeader))
        {
            QByteArray line = file.readLine();
            QString lineU = line.toUpper();
            //qDebug() << "LoTWUtilities::parseDownloadedFile: lineU: " << lineU;
            if (lineU.contains("<EOH>"))
            {
                 //qDebug() << "LoTWUtilities::parseDownloadedFile: EOH found";
                hasHeader = true;
            }
            //else if (lineU.contains("<PROGRAMID:4>LOTW"))
            //{
            //     //qDebug() << "LoTWUtilities::parseDownloadedFile: ProgramID found";
            //    hasProgramID = true;
            //}
            //else if (lineU.contains(Lotw_owncall))
            //{
            //     //qDebug() << "LoTWUtilities::parseDownloadedFile: OWNCALL found";
            //    hasOwnCall = true;
            //}
            else if (lineU.contains("<APP_LOTW_NUMREC:"))
            {
                QStringList data;
                data << lineU.split('>', QT_SKIP);

                numQSO = (data.at(1)).toInt();
                 //qDebug() << "LoTWUtilities::parseDownloadedFile: QSOs: " << QString::number(numQSO);
            }
            else if (lineU.contains("<I>USERNAME/PASSWORD INCORRECT</I>"))
            {
                userPasswordError = true;
            }
        }
        // WE HAVE JUST FINISHED TO READ THE HEADER OR THE FILE, IF IT IS NOT AN ADIF
        if (!hasHeader || (numQSO<1))
        {
             //qDebug() << "LoTWUtilities::parseDownloadedFile Header not found" << _fileName;
            QString aux;
            if (userPasswordError)
            {
                msgBox.setWindowTitle(tr("KLog - LoTW user/password error"));
                msgBox.setText(tr("LoTW server did not recognized your user/password"));
                aux = QString(tr("Check your user and password and ensure your are using the right one before trying again.") );
            }
            else if(numQSO<1)
            {
                msgBox.setWindowTitle(tr("KLog - LoTW No QSOs "));
                msgBox.setText(tr("LoTW sent no QSOs"));
                aux = QString(tr("It seems that LoTW has no QSO with the Station Callsign you are using (%1).") ).arg(stationCallsign);
            }
            else
            {
                msgBox.setWindowTitle(tr("KLog - LoTW Unknown error"));
                msgBox.setText(tr("KLog can't recognize the file that has been downloaded from LoTW."));
                aux = QString(tr("Try again and send the downloaded file (%1) to the KLog developer for analysis.") ).arg(_fileName);
            }

            msgBox.setIcon(QMessageBox::Warning);

            msgBox.setDetailedText(aux);
            msgBox.setStandardButtons(QMessageBox::Ok);
            msgBox.setDefaultButton(QMessageBox::Ok);
            msgBox.exec();
            //qDebug() << "LoTWUtilities::parseDownloadedFile: END-2";
            file.remove();
            return ;
        }

        msgBox.setIcon(QMessageBox::Information);
        msgBox.setWindowTitle(tr("KLog - LoTW download"));
        msgBox.setText(tr("KLog downloaded %1 QSOs successfully. Do you want to update your log with the downloaded data?").arg(QString::number(numQSO)));
        aux = QString(tr("Now KLog will process the downloaded QSO and update your local log.") );

        msgBox.setDetailedText(aux);
        msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
        msgBox.setDefaultButton(QMessageBox::Yes);
        if (msgBox.exec() == QMessageBox::No)
        {
            //qDebug() << "LoTWUtilities::parseDownloadedFile: END-2";
            return ;
        }
        //file.seek(startOfFile);
        emit actionProcessLoTWDownloadedFile(_fileName);
    }


   //Procesar los QSOs y meterlos en una tabla? o en un QStringList o alguna otra estructura


    //qDebug() << "LoTWUtilities::parseDownloadedFile - END" ;
}

/*void LoTWUtilities::showMessage(const int _messageIcon, const QString &_msg, const QString &_msgExt)
{

}
*/
