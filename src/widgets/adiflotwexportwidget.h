#ifndef KLOG_WIDGETS_ADIFLOTWEXPORTWIDGET_H
#define KLOG_WIDGETS_ADIFLOTWEXPORTWIDGET_H
/***************************************************************************
                          adiflotwexportwidget.h  -  description
                             -------------------
    begin                : July 2020
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
#include <QtWidgets>
#include "../dataproxy_sqlite.h"
//#include "../utilities.h"



class AdifLoTWExportWidget : public QWidget
{
    Q_OBJECT

public:
    explicit AdifLoTWExportWidget(DataProxy_SQLite *dp, const QString &_parentFunction, QWidget *parent = nullptr);
    ~AdifLoTWExportWidget();
    void setExportMode(const ExportMode _EMode);
    void setLogNumber(const int _logN);
    void setDefaultStationCallsign(const QString &_st);
    void setDefaultMyGrid(const QString &_st);

protected:
    void closeEvent(QCloseEvent *event);
    void showEvent(QShowEvent *event);

private slots:
    void slotOKPushButtonClicked();
    void slotCancelPushButtonClicked();
    void slotStationCallsignChanged();
    void slotDateChanged();
    void slotMyGridChanged();

signals:
    void selection(QString _st, QString _grid, QDate _startD, QDate _endD, ExportMode _exportMode);
    void qsosToSend(QString _call, QList<int> _qsos, ExportMode _exportMode);
    void askingToClose();

private:
    void createUI();
    QList<int> fillTable();
    void setTopLabel(const QString &_t);
    void addQSO(const int _qsoID);
    bool fillStationCallsignComboBox();
    void fillStationMyGridComboBox();
    void fillDates();
    void setDefaultStationComboBox();
    void setDefaultMyGridComboBox();
    void updateIfNeeded();
    void blockAllSignals(const bool _b);
    QString getCallOnOK();
    //QString getGridOnOK();

    DataProxy_SQLite *dataProxy;
    Utilities *util;
    QComboBox *stationCallsignComboBox, *myGridSquareComboBox;
    QDateEdit *startDate, *endDate;
    QLabel *topLabel, *numberLabel;
    QLineEdit *searchLineEdit;

    QPushButton *okButton, *cancelButton;
    ExportMode selectedEMode;

    QTableWidget *tableWidget;
    QHeaderView *hv, *hh;
    ExportMode currentExportMode;
    int logNumber, numOfQSOsInThisLog;

    QString defaultStationCallsign, defaultMyGrid;

    QString currentCall, currentGrid;
    QDate currentStart, currentEnd;
    bool starting;
    QList<int> qsos;
};


#endif
