/* *****************************************************************************
Copyright (c) 2016-2017, The Regents of the University of California (Regents).
All rights reserved.

Redistribution and use in source and binary forms, with or without 
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

The views and conclusions contained in the software and documentation are those
of the authors and should not be interpreted as representing official policies,
either expressed or implied, of the FreeBSD Project.

REGENTS SPECIFICALLY DISCLAIMS ANY WARRANTIES, INCLUDING, BUT NOT LIMITED TO, 
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
THE SOFTWARE AND ACCOMPANYING DOCUMENTATION, IF ANY, PROVIDED HEREUNDER IS 
PROVIDED "AS IS". REGENTS HAS NO OBLIGATION TO PROVIDE MAINTENANCE, SUPPORT, 
UPDATES, ENHANCEMENTS, OR MODIFICATIONS.

*************************************************************************** */

//
// function to read from a jsn object the value associated with a key.
// value may be a RV.name or a double value, if RV.name RV. to be stripped
//


// Written: fmckenna

#include "MDOF_BuildingModel.h"
#include <QPushButton>
#include <QScrollArea>
#include <QJsonArray>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QDebug>
#include <QFileDialog>
#include <QPushButton>
#include <sectiontitle.h>
#include <QFileInfo>
#include <string>
#include <sstream>
#include <iostream>
using namespace std;
#include <QGridLayout>
#include <SpreadsheetWidget.h>
#include <GlWidget2D.h>
#include <QHeaderView>

#include <math.h>

#include <OpenSeesParser.h>
#include <RandomVariableInputWidget.h>
#include <QTableWidget>


//#include <InputWidgetParameters.h>
#include <QValidator>


bool
readLineEditRV(QJsonObject &jsonObject, const char *key, QLineEdit *value)
{
    if (jsonObject.contains(key)) {
        QJsonValue theValue = jsonObject["factor"];
        if (theValue.isString()) {
            QString text = theValue.toString();
            text.remove(0,3); // remove RV.
            value->setText(text);

        } else if (theValue.isDouble())
            value->setText(QString::number(theValue.toDouble()));
    }
    return false;
}

// function to write to a json object a key-value pair. if value is not
// a double it is treated as a RV

bool
writeLineEditRV(QJsonObject &jsonObject, const char *key, QLineEdit *value)
{
    QString valueText = value->text();
    bool ok;

    double valueDouble = valueText.QString::toDouble(&ok);
    if (ok == true)
        jsonObject[key]=valueDouble;
    else
        jsonObject[key]= QString("RV.") + valueText;

    return true;
}

bool
readCellRV(QJsonObject &jsonObject, const char *key, QTableWidgetItem *value)
{
    if (jsonObject.contains(key)) {
        QJsonValue theValue = jsonObject[key];
        if (theValue.isString()) {
            QString text = theValue.toString();
            text.remove(0,3); // remove RV.
            value->setText(text);

        } else if (theValue.isDouble())
            value->setText(QString::number(theValue.toDouble()));
        return true;
    }
    return false;
}


bool
writeCellRV(QJsonObject &jsonObject, const char *key, QTableWidgetItem *value)
{
    QString valueText = value->text();
    bool ok;

    double valueDouble = valueText.QString::toDouble(&ok);
    if (ok == true)
        jsonObject[key]=valueDouble;
    else
        jsonObject[key]= QString("RV.") + valueText;

    return true;
}






static QLineEdit *
createTextEntry(QString text,
                QString toolTip,
                QGridLayout *theLayout,
                int row,
                int minL=100,
                int maxL=100,
                QString *unitText =0,
                bool itemRight = false)
{
    //QHBoxLayout *entryLayout = new QHBoxLayout();
    QLabel *entryLabel = new QLabel();

    entryLabel->setText(text);

    QLineEdit *res = new QLineEdit();
    res->setMinimumWidth(minL);
    res->setMaximumWidth(maxL);
    //res->setValidator(new QDoubleValidator);

    theLayout->addWidget(entryLabel,row,0);
    theLayout->addWidget(res,row,1);


    if (unitText != 0) {
        QLabel *unitLabel = new QLabel();
        unitLabel->setText(*unitText);
        unitLabel->setMinimumWidth(40);
        unitLabel->setMaximumWidth(50);
        //entryLayout->addWidget(unitLabel);
        theLayout->addWidget(unitLabel, row,2);
    }

    if (!toolTip.isEmpty()) {
        res->setToolTip(toolTip);
    }

    return res;
}

MDOF_BuildingModel::MDOF_BuildingModel(RandomVariableInputWidget *theRandomVariableIW, QWidget *parent)
  : SimCenterAppWidget(parent), theRandomVariableInputWidget(theRandomVariableIW), 
    numStories(0),
    floorHeights(0), storyHeights(0),
    fMinSelected(-1),fMaxSelected(-1), 
    sMinSelected(-1),sMaxSelected(-1),
    floorSelected(-1),storySelected(-1)
{
    numStories = 0; // originally set to 0, so that when setnumStories text later no seg fault
    buildingW = "144";
    buildingH = 120;
    Kx = "100.0";
    Fyx = "10.0";
    bx = "0.1";
    Ky ="100.0";
    Fyy = "10.0";
    by = "0.1";
    dampingRatio = "0.02";

    //
    // some QStrings to avoid duplication of units
    //

    QString blank(tr("   "));
    QString kips(tr("k"  ));
    QString g(tr("g"  ));
    QString kipsInch(tr("k/in"));
    QString inch(tr("in  "));
    QString sec(tr("sec"));
    QString percent(tr("\%   "));

    QHBoxLayout *layout = new QHBoxLayout();

    QVBoxLayout *inputLayout = new QVBoxLayout();
    QVBoxLayout *graphicLayout = new QVBoxLayout();

    //
    // create to hold major model inputs
    //

    //Building information
    QGroupBox* mainProperties = new QGroupBox("Building Information");
    QGridLayout *mainPropertiesLayout = new QGridLayout();
    inFloors = createTextEntry(tr("Number Floors"), tr("number of floors or stories in building"),mainPropertiesLayout, 0, 100, 100, &blank);
    inWeight = createTextEntry(tr("Building Weight"), tr("total building weight, each floor will have a weight given by weight/ number of floors"), mainPropertiesLayout, 1, 100, 100, &kips);
    inHeight = createTextEntry(tr("Building Height"), tr("total height of building in inches, each floor will have a height given by total height / number of floors"),mainPropertiesLayout, 2, 100, 100, &inch);
    inKx = createTextEntry(tr("Story Stiffness X dirn"), tr("story stiffnesses, that force required to push the floor above 1 inch, assuming all other stories have infinite stiffness"),mainPropertiesLayout, 3, 100, 100, &kipsInch);
    inKy = createTextEntry(tr("Story Stiffness Y dirn"), tr("story stiffnesses, that force required to push the floor above 1 inch, assuming all other stories have infinite stiffness"),mainPropertiesLayout, 4, 100, 100, &kipsInch);
    inDamping = createTextEntry(tr("Damping Ratio"),tr("for each mode of vibraation a number that specifies how quickly the structure stops vibrating in that mode"), mainPropertiesLayout, 5, 100, 100, &percent);

    inFloors->setValidator(new QIntValidator);
    //inWeight->setValidator(new QDoubleValidator);
    inHeight->setValidator(new QDoubleValidator);

    //inKx->setValidator(new QDoubleValidator);
    //inKy->setValidator(new QDoubleValidator);
    //inDamping->setValidator(new QDoubleValidator);

    inFloors->setText(QString::number(1));
    inWeight->setText(buildingW);
    inHeight->setText(QString::number(buildingH));
    inKx->setText(Kx);
    inKy->setText(Ky);
    inDamping->setText(dampingRatio);

    connect(inFloors,SIGNAL(editingFinished()), this, SLOT(on_inFloors_editingFinished()));
    connect(inWeight,SIGNAL(editingFinished()), this, SLOT(on_inWeight_editingFinished()));
    connect(inHeight,SIGNAL(editingFinished()), this, SLOT(on_inHeight_editingFinished()));
    connect(inKx,SIGNAL(editingFinished()), this, SLOT(on_inKx_editingFinished()));
    connect(inKy,SIGNAL(editingFinished()), this, SLOT(on_inKy_editingFinished()));
    connect(inDamping,SIGNAL(editingFinished()), this, SLOT(on_inDamping_editingFinished()));

    mainProperties->setLayout(mainPropertiesLayout);
    inputLayout->addWidget(mainProperties);

    //
    // widget to hold floor weight properties for when a number of floors selected
    //

    floorMassFrame = new QGroupBox("Selected Floor Weights");
    QGridLayout *floorMassFrameLayout = new QGridLayout();
    inFloorWeight = createTextEntry(tr("Floor Weight"), tr("individual floor weight, will change total weight if edit"),floorMassFrameLayout,0,100, 100, &kips);
    floorMassFrame->setLayout(floorMassFrameLayout);
    inputLayout->addWidget(floorMassFrame);
    floorMassFrame->setVisible(false);

    connect(inFloorWeight,SIGNAL(editingFinished()), this, SLOT(on_inFloorWeight_editingFinished()));

    //
    // widget to hold story properties for when a number of floors selected
    //

    storyPropertiesFrame = new QGroupBox("Selected Story Properties");
    storyPropertiesFrame->setObjectName(QString::fromUtf8("storyPropertiesFrame"));
    QGridLayout *storyPropertiesFrameLayout = new QGridLayout();
    inStoryHeight = createTextEntry(tr("Story Height"), tr("for stories selected sets story height, effects overall height if edited"), storyPropertiesFrameLayout,0,100,100,&inch);
    inStoryKx = createTextEntry(tr("Stiffness X dirn"), tr("for stories selected force required to push each floor 1 inch assuming all other floors infinite stiffness"), storyPropertiesFrameLayout,1,100,100,&kipsInch);
    inStoryFyx = createTextEntry(tr("Yield Strength X dirn"),tr("for stories selected force at which story yields, i.e. if aply more force floor will not return to original position assuming all other rigid"), storyPropertiesFrameLayout,2,100,100,&kips);
    inStoryBx = createTextEntry(tr("Hardening Ratio X dirn"), tr("for stories selected the hardening ratio defines ratio between original stiffness and current stiffness"), storyPropertiesFrameLayout,3);
    inStoryKy = createTextEntry(tr("Stiffness Y dirn"), tr("for stories selected force required to push each floor 1 inch assuming all other floors infinite stiffness"), storyPropertiesFrameLayout,4,100,100,&kipsInch);
    inStoryFyy = createTextEntry(tr("Yield Strength Y dirn"),tr("for stories selected force at which story yields, i.e. if aply more force floor will not return to original position assuming all other rigid"), storyPropertiesFrameLayout,5,100,100,&kips);
    inStoryBy = createTextEntry(tr("Hardening Ratio Y dirn"), tr("for stories selected the hardening ratio defines ratio between original stiffness and current stiffness"), storyPropertiesFrameLayout,6);

    storyPropertiesFrame->setLayout(storyPropertiesFrameLayout);
    storyPropertiesFrame->setLayout(storyPropertiesFrameLayout);

    inputLayout->addWidget(storyPropertiesFrame);
    storyPropertiesFrame->setVisible(false);

    connect(inStoryHeight, SIGNAL(editingFinished()), this, SLOT(on_inStoryHeight_editingFinished()));
    connect(inStoryKx, SIGNAL(editingFinished()), this, SLOT(on_inStoryKx_editingFinished()));
    connect(inStoryFyx,SIGNAL(editingFinished()),this, SLOT(on_inStoryFyx_editingFinished()));
    connect(inStoryBx, SIGNAL(editingFinished()),this,SLOT(on_inStoryBx_editingFinished()));
    connect(inStoryKy, SIGNAL(editingFinished()), this, SLOT(on_inStoryKy_editingFinished()));
    connect(inStoryFyy,SIGNAL(editingFinished()),this, SLOT(on_inStoryFyy_editingFinished()));
    connect(inStoryBy, SIGNAL(editingFinished()),this,SLOT(on_inStoryBy_editingFinished()));

    //
    // add spreadsheet to hold individual properties
    //

    QStringList headings;
    headings << tr("Weight") << tr("Height") << tr("K_x") << tr("Fy_x") << tr("b_x") << tr("K_y") << tr("Fy_y") << tr("b_y") << tr("zeta");
    dataTypes << SIMPLESPREADSHEET_QDouble;
    dataTypes << SIMPLESPREADSHEET_QDouble;
    dataTypes << SIMPLESPREADSHEET_QDouble;
    dataTypes << SIMPLESPREADSHEET_QDouble;
    dataTypes << SIMPLESPREADSHEET_QDouble;
    dataTypes << SIMPLESPREADSHEET_QDouble;
    dataTypes << SIMPLESPREADSHEET_QDouble;
    dataTypes << SIMPLESPREADSHEET_QDouble;
    dataTypes << SIMPLESPREADSHEET_QDouble;

    //theSpreadsheet = new SpreadsheetWidget(9, 4, headings, dataTypes,0);
    theSpreadsheet = new QTableWidget();
    inputLayout->addWidget(theSpreadsheet);

    connect(theSpreadsheet, SIGNAL(cellClicked(int,int)), this, SLOT(on_theSpreadsheet_cellClicked(int,int)));
    connect(theSpreadsheet, SIGNAL(cellEntered(int,int)), this,SLOT(on_theSpreadsheet_cellClicked(int,int)));
    connect(theSpreadsheet, SIGNAL(cellChanged(int,int)), this,SLOT(on_theSpreadsheet_cellChanged(int,int)));

    inputLayout->addStretch();

    theView = new GlWidget2D();
    theView->setController(this);
    theView->setMinimumHeight(300);
    theView->setMinimumWidth(250);
    graphicLayout->addWidget(theView);


    layout->addLayout(inputLayout);
    layout->addLayout(graphicLayout);

    layout->addStretch();
    this->setLayout(layout);

    qDebug() << "calling inFloorEditing";
    this->on_inFloors_editingFinished();

}

MDOF_BuildingModel::~MDOF_BuildingModel()
{
    if (storyHeights != 0)
        delete [] storyHeights;
    if (floorHeights != 0)
        delete [] floorHeights;
}


void
MDOF_BuildingModel::on_inFloors_editingFinished()
{
    QString textFloors =  inFloors->text();
    int numStoriesText = textFloors.toInt();

    if (numStoriesText <= 0) {
        inFloors->setText(QString(numStories));
        return;
    }

    if (numStoriesText != numStories) {

        numStories = numStoriesText;

        floorSelected = -1;
        storySelected = -1;
        fMinSelected = -1;
        fMaxSelected = -1;
        sMinSelected = -1;
        sMaxSelected = -1;

        if (floorHeights != 0)
            delete [] floorHeights;
        if (storyHeights != 0)
            delete [] storyHeights;

        floorHeights = new double[numStories+1];
        storyHeights = new double[numStories];

        //this->updateSpreadsheet();
        theSpreadsheet->clear();
        theSpreadsheet->setColumnCount(9);
        theSpreadsheet->setRowCount(numStories);

        // theSpreadsheet->horizontalHeader()->setStretchLastSection(true);// horizontalHeader()->setResizeMode(QHeaderView::ResizeToContents);
        //theSpreadsheet->setFixedWidth(344);

        QStringList headings;
        headings << tr("Weight") << tr("Height") << tr("K_x") << tr("Fy_x") << tr("b_x") << tr("K_y") << tr("Fy_y") << tr("b_y") << tr("zeta");
        theSpreadsheet->setHorizontalHeaderLabels(headings);

        // determine floor weights (only if building weight not RV)
        QString floorWeightText = "";
        buildingW = inWeight->text();
        bool ok;
        double textToDoubleW = buildingW.toDouble(&ok);
        if (ok == true) {
            double buildingWeight = textToDoubleW;
            double floorW = buildingWeight/(numStories);
            floorWeightText = QString::number(floorW);
        }

        // determine floor and story heights
        for (int i=0; i<numStories; i++) {
            floorHeights[i] = i*buildingH/(1.*numStories);
            storyHeights[i] = buildingH/(1.*numStories);
        }
        floorHeights[numStories] = buildingH;


        //
        // create spreadsheet entries
        //
        updatingPropertiesTable = true;
        for (int i=0; i<numStories; i++) {

            QTableWidgetItem *item;
            item = new QTableWidgetItem("3");
            qDebug() << "4.1.1 " << floorWeightText << " " << item->text();
            item->setToolTip(QString("weight of floor " + QString::number(i+1)));
            theSpreadsheet->setItem(i,0, item);

            item = new QTableWidgetItem(QString().setNum(storyHeights[i]));
            item->setToolTip(QString("height of story " + QString::number(i+1)));
            theSpreadsheet->setItem(i,1,item);

            item = new QTableWidgetItem(Kx);
            item->setToolTip(QString("initial stiffness of story " + QString::number(i+1) + " in x dirn"));
            theSpreadsheet->setItem(i,2,item);

            item = new QTableWidgetItem(Fyx);
            item->setToolTip(QString("yield strength of story " + QString::number(i+1) + " in x dirn"));
            theSpreadsheet->setItem(i,3,item);

            item = new QTableWidgetItem(bx);
            item->setToolTip(QString("hardening ratio of story " + QString::number(i+1) + " in x dirn"));
            theSpreadsheet->setItem(i,4, item);

            item = new QTableWidgetItem(Ky);
            item->setToolTip(QString("initial stiffness of story " + QString::number(i+1)) + " in y dirn");
            theSpreadsheet->setItem(i,5,item);

            item = new QTableWidgetItem(Fyy);
            item->setToolTip(QString("yield strength of story " + QString::number(i+1) + " in y dirn"));
            theSpreadsheet->setItem(i,6,item);

            item = new QTableWidgetItem(by);
            item->setToolTip(QString("hardening ratio of story " + QString::number(i+1) + " in y dirn"));
            theSpreadsheet->setItem(i,7, item);

            item = new QTableWidgetItem(dampingRatio);
            item->setToolTip(QString("damping ratio of mode " + QString::number(i+1)));
            theSpreadsheet->setItem(i,8, item);

        }

        theSpreadsheet->resizeRowsToContents();
        theSpreadsheet->resizeColumnsToContents();

        updatingPropertiesTable = false;

        floorSelected = -1;
        storySelected = -1;
    }

    if (theView != 0) {
        qDebug() << "UPDATING VIEW";
        this->draw(theView);
        theView->update();
    }
}



void
MDOF_BuildingModel::on_inWeight_editingFinished()
{
    buildingW =  inWeight->text();
    bool ok;
    double textToDoubleW = buildingW.toDouble(&ok);

    // determine floor weights (only if building weight not RV aka is an actual number)
    QString floorWeightText = "";
    if (ok == true) {
        double floorW = textToDoubleW/(1*numStories);
        floorWeightText = QString::number(floorW);
    }
    updatingPropertiesTable = true;
    for (int i=0; i<numStories; i++) {
        QTableWidgetItem *item = theSpreadsheet->item(i,0);
        item->setText(floorWeightText);
    }
    updatingPropertiesTable = false;
}

void
MDOF_BuildingModel::on_inHeight_editingFinished()
{
    QString textH =  inHeight->text();
    if (textH.isNull())
        return;

    double textToDoubleH = textH.toDouble();
    if (textToDoubleH != buildingH) {
        buildingH = textToDoubleH;

        for (int i=0; i<numStories; i++) {
            floorHeights[i] = i*buildingH/(1.*numStories);
            storyHeights[i] = buildingH/(1.*numStories);
        }
        floorHeights[numStories] = buildingH;
    }
    //this->updateSpreadsheet();
}


void
MDOF_BuildingModel::on_inKx_editingFinished()
{
    QString text =  inKx->text();
    if (text.isNull())
        return;
    updatingPropertiesTable = true;
    for (int i=0; i<numStories; i++) {
        QTableWidgetItem *item = theSpreadsheet->item(i,2);
        item->setText(text);
    }
    updatingPropertiesTable = false;
}

void
MDOF_BuildingModel::on_inKy_editingFinished()
{
    QString text =  inKy->text();
    if (text.isNull())
        return;
    updatingPropertiesTable = true;
    for (int i=0; i<numStories; i++) {
        QTableWidgetItem *item = theSpreadsheet->item(i,5);
        item->setText(text);
    }
    updatingPropertiesTable = false;
}

void
MDOF_BuildingModel::on_inDamping_editingFinished()
{
    QString text =  inDamping->text();
    if (text.isNull())
        return;

    updatingPropertiesTable = true;
    for (int i=0; i<numStories; i++) {
        QTableWidgetItem *item = theSpreadsheet->item(i,8);
        item->setText(text);
    }
    updatingPropertiesTable = false;
}

void MDOF_BuildingModel::on_inFloorWeight_editingFinished()
{
    /*
    if (updatingPropertiesTable == true)
        return;

    QString text =  inFloorWeight->text();
    if (text.isNull())
        return;

    double textToDouble = text.toDouble();
    for (int i=fMinSelected-1; i<fMaxSelected; i++)
        if (i != -1)
            weights[i] = textToDouble;

    buildingW = 0;
    for (int i=0; i<numStories; i++)
        buildingW = buildingW+weights[i];

    inWeight->setText(QString::number(buildingW));
 */
}


void MDOF_BuildingModel::on_inStoryHeight_editingFinished()
{
    if (updatingPropertiesTable == true)
        return;

    QString text =  inStoryHeight->text();
    if (text.isNull())
        return;

    double newStoryHeight = text.toDouble();
    double currentStoryHeight = 0;
    double *newFloorHeights = new double[numStories+1];

    // determine new floor heights, cludgy can rewrite now store storyHeights
    newFloorHeights[0] = 0;
    for (int i=0; i<sMinSelected; i++)
        newFloorHeights[i+1] = floorHeights[i+1];
    updatingPropertiesTable = true;
    for (int i=sMinSelected; i<=sMaxSelected; i++) {
        newFloorHeights[i+1] = newFloorHeights[i]+newStoryHeight;
        storyHeights[i] = newStoryHeight;
    }
    updatingPropertiesTable = false;
    for (int i=sMaxSelected+1; i<numStories; i++)
        newFloorHeights[i+1] = newFloorHeights[i]+floorHeights[i+1]-floorHeights[i];


    bool needReset = false;
    for (int i=0; i<=numStories; i++) {
        if (floorHeights[i] != newFloorHeights[i]){
            needReset = true;
            i=numStories+1;
        }
    }

    delete [] floorHeights;
    floorHeights = newFloorHeights;

    // move focus, update graphic and set analysis flag
    //  inStoryK->setFocus();
    if (needReset == true) {
        // delete old array and reset pointer
        // this->updateSpreadsheet();
        // delete old array and reset pointer
        buildingH = newFloorHeights[numStories];
        inHeight->setText(QString::number(buildingH));
    }
}

void MDOF_BuildingModel::on_inStoryKx_editingFinished()
{
    if (updatingPropertiesTable == true)
        return;

    QString text =  inStoryKx->text();
    if (text.isNull())
        return;

    double textToDouble = text.toDouble();
    bool needReset = false;
    updatingPropertiesTable = true;
    for (int i=sMinSelected; i<=sMaxSelected; i++) {
        QTableWidgetItem *item = theSpreadsheet->item(i,2);
        item->setText(text);
    }
    updatingPropertiesTable = false;
}

void MDOF_BuildingModel::on_inStoryFyx_editingFinished()
{
    if (updatingPropertiesTable == true)
        return;

    QString text =  inStoryFyx->text();
    if (text.isNull())
        return;
    updatingPropertiesTable = true;
    double textToDouble = text.toDouble();
    for (int i=sMinSelected; i<=sMaxSelected; i++) {
        QTableWidgetItem *item = theSpreadsheet->item(i,3);
        item->setText(text);
    }
    updatingPropertiesTable = false;
}

void MDOF_BuildingModel::on_inStoryBx_editingFinished()
{
    if (updatingPropertiesTable == true)
        return;

    QString text =  inStoryBx->text();
    if (text.isNull())
        return;
    updatingPropertiesTable = true;
    double textToDouble = text.toDouble();
    for (int i=sMinSelected; i<=sMaxSelected; i++) {
        QTableWidgetItem *item = theSpreadsheet->item(i,4);
        item->setText(text);
    }
    updatingPropertiesTable = false;
}

void MDOF_BuildingModel::on_inStoryKy_editingFinished()
{
    if (updatingPropertiesTable == true)
        return;

    QString text =  inStoryKy->text();
    if (text.isNull())
        return;
    updatingPropertiesTable = true;
    double textToDouble = text.toDouble();
    bool needReset = false;
    for (int i=sMinSelected; i<=sMaxSelected; i++) {
        QTableWidgetItem *item = theSpreadsheet->item(i,5);
        item->setText(text);
    }
    updatingPropertiesTable = false;
    //inStoryFy->setFocus();
    // if (needReset == true)
    //    this->updateSpreadsheet();
}

void MDOF_BuildingModel::on_inStoryFyy_editingFinished()
{
    if (updatingPropertiesTable == true)
        return;

    QString text =  inStoryFyy->text();
    if (text.isNull())
        return;
    updatingPropertiesTable = true;
    for (int i=sMinSelected; i<=sMaxSelected; i++) {
        QTableWidgetItem *item = theSpreadsheet->item(i,6);
        item->setText(text);
    }
    updatingPropertiesTable = false;
}

void MDOF_BuildingModel::on_inStoryBy_editingFinished()
{
    if (updatingPropertiesTable == true)
        return;

    QString text =  inStoryBy->text();
    if (text.isNull())
        return;
    updatingPropertiesTable = true;
    double textToDouble = text.toDouble();
    for (int i=sMinSelected; i<=sMaxSelected; i++) {
        QTableWidgetItem *item = theSpreadsheet->item(i,7);
        item->setText(text);
    }
    updatingPropertiesTable = false;
}



void MDOF_BuildingModel::on_theSpreadsheet_cellClicked(int row, int column)
{
    if (column == 0) {
        floorSelected = row+1;
        storySelected = -1;
    }
    else if (column > 0 && column < 8) {
        storySelected = row;
        floorSelected = -1;
    } else {
        storySelected = -1;
        floorSelected = -1;
    }
    fMinSelected = -1;
    fMaxSelected = -1;
    sMinSelected = -1;
    sMaxSelected = -1;

    this->draw(theView);
    theView->update();
}

/*
void
MDOF_BuildingModel::updateSpreadsheet() {

    if (theSpreadsheet == 0)
        return;

    //
    // mark spreadsheet as being updated so a chane to cells do not cause recursive calls
    //

    updatingPropertiesTable = true;

    qDebug() << "updateSpreadsheet: numStories: " << numStories;

    theSpreadsheet->clear();
    theSpreadsheet->setColumnCount(9);
    theSpreadsheet->setRowCount(numStories);

    // theSpreadsheet->horizontalHeader()->setStretchLastSection(true);// horizontalHeader()->setResizeMode(QHeaderView::ResizeToContents);
    //theSpreadsheet->setFixedWidth(344);

    QStringList headings;
    headings << tr("Weight") << tr("Height") << tr("K_x") << tr("Fy_x") << tr("b_x") << tr("K_y") << tr("Fy_y") << tr("b_y") << tr("zeta");
    theSpreadsheet->setHorizontalHeaderLabels(headings);

    for (int i=0; i<numStories; i++) {

        QTableWidgetItem *item;


        QTableWidgetItem *item1 = new QTableWidgetItem(QString().setNum(weights[i]));
        item1->setToolTip(QString("weight of floor " + QString::number(i+1)));
        theSpreadsheet->setItem(i,0, item1);

        item = new QTableWidgetItem(QString().setNum(storyHeights[i]));
        item->setToolTip(QString("height of story " + QString::number(i+1)));
        theSpreadsheet->setItem(i,1,item);

        item = new QTableWidgetItem(QString().setNum(kx[i]));
        item->setToolTip(QString("initial stiffness of story " + QString::number(i+1) + " in x dirn"));
        theSpreadsheet->setItem(i,2,item);

        item = new QTableWidgetItem(QString().setNum(fyx[i]));
        item->setToolTip(QString("yield strength of story " + QString::number(i+1) + " in x dirn"));
        theSpreadsheet->setItem(i,3,item);

        item = new QTableWidgetItem(QString().setNum(bx[i]));
        item->setToolTip(QString("hardening ratio of story " + QString::number(i+1) + " in x dirn"));
        theSpreadsheet->setItem(i,4, item);

        item = new QTableWidgetItem(QString().setNum(ky[i]));
        item->setToolTip(QString("initial stiffness of story " + QString::number(i+1)) + " in y dirn");
        theSpreadsheet->setItem(i,5,item);

        item = new QTableWidgetItem(QString().setNum(fyy[i]));
        item->setToolTip(QString("yield strength of story " + QString::number(i+1) + " in y dirn"));
        theSpreadsheet->setItem(i,6,item);

        item = new QTableWidgetItem(QString().setNum(by[i]));
        item->setToolTip(QString("hardening ratio of story " + QString::number(i+1) + " in y dirn"));
        theSpreadsheet->setItem(i,7, item);

        item = new QTableWidgetItem(QString().setNum(zeta[i]));
        item->setToolTip(QString("damping ratio of mode " + QString::number(i+1)));
        theSpreadsheet->setItem(i,8, item);

    }

    theSpreadsheet->resizeRowsToContents();
    theSpreadsheet->resizeColumnsToContents();

    updatingPropertiesTable = false;

    floorSelected = -1;
    storySelected = -1;
}
*/

void MDOF_BuildingModel::on_theSpreadsheet_cellChanged(int row, int column)
{
    if (updatingPropertiesTable == false) {

        QString text = theSpreadsheet->item(row,column)->text();
        bool ok;
        double textToDouble = text.toDouble(&ok);
        double buildingWeight = 0;
        if (column == 0) {

            // if a double value, set buildingWeight if all entries in column 0 are doubles
            if (ok == true) {
                for (int i=0; i<numStories; i++) {
                    QTableWidgetItem *floorI = theSpreadsheet->item(i,0);
                    QString text = floorI->text();
                    textToDouble = text.toDouble(&ok);
                    if (ok == false) {
                        i = numStories;
                    } else {
                        buildingWeight += textToDouble;
                    }
                }
            }
            // if still 0, all are
            if (ok == true)
                inWeight->setText(QString::number(buildingWeight));

        } else if (column  == 1) {
            if (storyHeights[row] == textToDouble)
                return;

            storyHeights[row] = textToDouble;
            for (int i=row; i<numStories; i++)
                floorHeights[i+1] = floorHeights[i]+storyHeights[i];
            buildingH = floorHeights[numStories];
            inHeight->setText(QString::number(buildingH));
        }

        // this->updateSpreadsheet();
    }
    updatingPropertiesTable = true;
}

void MDOF_BuildingModel::clear(void)
{

}



bool
MDOF_BuildingModel::outputToJSON(QJsonObject &jsonObject)
{
     jsonObject["type"]="MDOF_BuildingModel";

    int numStories = inFloors->text().toInt();
    jsonObject["numStories"]= numStories;

    writeLineEditRV(jsonObject,"weight", inWeight);
    writeLineEditRV(jsonObject,"height", inHeight);
    writeLineEditRV(jsonObject,"Kx", inKx);
    writeLineEditRV(jsonObject,"Ky", inKy);

    QJsonArray theArray;
    for (int i = 0; i <numStories; ++i) {
        QJsonObject floorData;
        writeCellRV(floorData, "weight", theSpreadsheet->item(i,0));
        writeCellRV(floorData, "height", theSpreadsheet->item(i,1));
        writeCellRV(floorData, "kx", theSpreadsheet->item(i,2));
        writeCellRV(floorData, "Fyx", theSpreadsheet->item(i,3));
        writeCellRV(floorData, "bx", theSpreadsheet->item(i,4));
        writeCellRV(floorData, "ky", theSpreadsheet->item(i,5));
        writeCellRV(floorData, "Fyy", theSpreadsheet->item(i,6));
        writeCellRV(floorData, "by", theSpreadsheet->item(i,7));
        writeCellRV(floorData, "zeta", theSpreadsheet->item(i,8));

        theArray.append(floorData);
    }
    jsonObject["ModelData"]=theArray;

    QJsonArray rvArray;
    for (int i=0; i<varNamesAndValues.size()-1; i+=2) {
        QJsonObject rvObject;
        QString name = varNamesAndValues.at(i);
        rvObject["name"]=name;
        rvObject["value"]=QString("RV.")+name;

        rvArray.append(rvObject);
    }

    jsonObject["randomVar"]=rvArray;

    return true;
}


bool
MDOF_BuildingModel::inputFromJSON(QJsonObject &jsonObject)
{
    varNamesAndValues.clear();
    int numStoriesOld = numStories;

    if (jsonObject.contains("numStories")) {
        QJsonValue theValue = jsonObject["numStories"];
        numStories = theValue.toInt();
        inFloors->setText(QString::number(numStories));
    } else {
        qDebug() << "No numStories key in json object";
        return false;
    }

    if (numStories <= 0) {
        inFloors->setText(QString(numStoriesOld));
        return true;
    }

    if (numStoriesOld != numStories) {

        floorSelected = -1;
        storySelected = -1;
        fMinSelected = -1;
        fMaxSelected = -1;
        sMinSelected = -1;
        sMaxSelected = -1;

        if (floorHeights != 0)
            delete [] floorHeights;
        if (storyHeights != 0)
            delete [] storyHeights;

        floorHeights = new double[numStories+1];
        storyHeights = new double[numStories];
    }


   readLineEditRV(jsonObject,"weight", inWeight);
   readLineEditRV(jsonObject,"height", inHeight);
   readLineEditRV(jsonObject,"Kx", inKx);
   readLineEditRV(jsonObject,"Ky", inKy);



   theSpreadsheet->clear();
   theSpreadsheet->setColumnCount(9);
   theSpreadsheet->setRowCount(numStories);

   // theSpreadsheet->horizontalHeader()->setStretchLastSection(true);// horizontalHeader()->setResizeMode(QHeaderView::ResizeToContents);
   //theSpreadsheet->setFixedWidth(344);

   QStringList headings;
   headings << tr("Weight") << tr("Height") << tr("K_x") << tr("Fy_x") << tr("b_x") << tr("K_y") << tr("Fy_y") << tr("b_y") << tr("zeta");
   theSpreadsheet->setHorizontalHeaderLabels(headings);


   if (jsonObject.contains("ModelData")) {

       QJsonValue theValue = jsonObject["ModelData"];
       if (!theValue.isArray()) {
           return false;
       }

       QJsonArray theArray = theValue.toArray();
       if (numStories != theArray.size()) {
           return false;
       }


       updatingPropertiesTable = true;
       for (int i=0; i<numStories; i++) {

           QJsonValue theFloor = theArray[i];
           QJsonObject floorData = theFloor.toObject();

           QTableWidgetItem *item;
           item = new QTableWidgetItem();
           readCellRV(floorData, "weight", item);
           item->setToolTip(QString("weight of floor " + QString::number(i+1)));
           theSpreadsheet->setItem(i, 0, item);

           item = new QTableWidgetItem();
           readCellRV(floorData, "height", item);
           item->setToolTip(QString("height of story " + QString::number(i+1)));
           theSpreadsheet->setItem(i, 2, item);

           item = new QTableWidgetItem();
           readCellRV(floorData, "kx", item);
           item->setToolTip(QString("initial stiffness of story " + QString::number(i+1) + " in x dirn"));
           theSpreadsheet->setItem(i, 1, item);

           item = new QTableWidgetItem();
           readCellRV(floorData, "Fyx", item);
           item->setToolTip(QString("yield strength of story " + QString::number(i+1) + "in x dirn"));
           theSpreadsheet->setItem(i, 3, item);

           item = new QTableWidgetItem();
           readCellRV(floorData, "bx", item);
           item->setToolTip(QString("hardening ratio of story " + QString::number(i+1) + "in x dirn"));
           theSpreadsheet->setItem(i, 4, item);

           item = new QTableWidgetItem();
           readCellRV(floorData, "ky", item);
           item->setToolTip(QString("initial stiffness of story " + QString::number(i+1) + "in y dirn"));
           theSpreadsheet->setItem(i, 5, item);

           item = new QTableWidgetItem();
           readCellRV(floorData, "Fyy", item);
           item->setToolTip(QString("yield strength of story " + QString::number(i+1) + "in y dirn"));
           theSpreadsheet->setItem(i, 6, item);


           item = new QTableWidgetItem();
           readCellRV(floorData, "by", item);
           item->setToolTip(QString("hardening ratio of story " + QString::number(i+1) + "in y dirn"));
           theSpreadsheet->setItem(i, 7, item);

           item = new QTableWidgetItem();
           readCellRV(floorData, "zeta", item);
           item->setToolTip(QString("damping ratio of mode " + QString::number(i+1)));
           theSpreadsheet->setItem(i, 8, item);
       }
   }

   QJsonArray rvArray;
   for (int i=0; i<varNamesAndValues.size()-1; i+=2) {
       QJsonObject rvObject;
       QString name = varNamesAndValues.at(i);
       rvObject["name"]=name;
       rvObject["value"]=QString("RV.")+name;

       rvArray.append(rvObject);
   }

    jsonObject["randomVar"]=rvArray;

    updatingPropertiesTable = false;

    if (theView != 0) {
        qDebug() << "UPDATING VIEW";
        this->draw(theView);
        theView->update();
    }
    this->draw(theView);
    theView->update();

    return true;
}


bool
MDOF_BuildingModel::outputAppDataToJSON(QJsonObject &jsonObject) {

    //
    // per API, need to add name of application to be called in AppLication
    // and all data to be used in ApplicationDate
    //

    jsonObject["Application"] = "MDOF_BuildingModel";
    QJsonObject dataObj;
    jsonObject["ApplicationData"]=dataObj;

    return true;
}
bool
MDOF_BuildingModel::inputAppDataFromJSON(QJsonObject &jsonObject) {

    //
    // from ApplicationData
    //

    return true;
}



 bool
 MDOF_BuildingModel::copyFiles(QString &dirName) {
    return true;
 }


 void
 MDOF_BuildingModel::draw(GlWidget2D *)
 {

     if (numStories == 0)
            return;

     theView->reset();

     for (int i=0; i<numStories; i++) {
         if (i == storySelected)
             theView->drawLine(i+1+numStories, 0.,floorHeights[i], 0. ,floorHeights[i+1], 2, 1, 0, 0);
         else if (i >= sMinSelected && i <= sMaxSelected)
             theView->drawLine(i+1+numStories, 0.,floorHeights[i], 0.,floorHeights[i+1], 2, 1, 0, 0);
         else
             theView->drawLine(i+1+numStories, 0.,floorHeights[i],0.,floorHeights[i+1], 2, 0, 0, 0);    }

     for (int i=0; i<=numStories; i++) {
         if (i == floorSelected)
             theView->drawPoint(i, 0.,floorHeights[i], 10, 1, 0, 0);
         else if (i >= fMinSelected && i <= fMaxSelected)
             theView->drawPoint(i, 0.,floorHeights[i], 10, 1, 0, 0);
         else {
             theView->drawPoint(i, 0.,floorHeights[i], 10, 0, 0, 1);
         }
     }

     theView->drawLine(0, -10., 0.0, 10., 0.0, 1.0, 0., 0., 0.);

    theView->drawBuffers();
 }

 void
 MDOF_BuildingModel::getBoundary(float &height, float &width)
 {
  height  = buildingH;
  width = 10;
 }

 void
 MDOF_BuildingModel::setSelectionBoundary(float y1, float y2)
 {
     // determine min and max of y1,y2
     float yMin = 0;
     float yMax = 0;
     if (y1 < y2) {
         yMin = y1;
         yMax = y2;
     } else {
         yMin = y2;
         yMax = y1;
     }

     // determine min and max of nodes in [y1,y2] range
     fMinSelected = -1;
     fMaxSelected = -1;

     for (int i=0; i<=numStories; i++) {
         if (floorHeights[i] >= yMin && floorHeights[i] <= yMax) {
             if (fMinSelected == -1)
                 fMinSelected = i;
             fMaxSelected = i;
         }
     }

     // determine min and max of stories in [y1, y2] range;
     sMinSelected = -1;
     sMaxSelected = -1;
     for (int i=0; i<numStories; i++) {
         double midStoryHeight = (floorHeights[i]+floorHeights[i+1])/2.;
         if (midStoryHeight >= yMin && midStoryHeight <= yMax) {
             if (sMinSelected == -1)
                 sMinSelected = i;
             sMaxSelected = i;
         }
     }

     if (fMinSelected == 0 && fMaxSelected == numStories) {

         floorMassFrame->setVisible(false);
         storyPropertiesFrame->setVisible(false);
         //spreadSheetFrame->setVisible(true);
         fMinSelected = -1;
         fMaxSelected = -1;
         sMinSelected = -1;
         sMaxSelected = -1;


     } else if (fMinSelected == fMaxSelected && fMinSelected != -1) {

         floorMassFrame->setVisible(true);
         if (sMinSelected == -1 && sMaxSelected == -1)
             storyPropertiesFrame->setVisible(false);
         else
             storyPropertiesFrame->setVisible(true);
         //spreadSheetFrame->setVisible(false);
         floorSelected=-1;
         storySelected=-1;

     } else if (fMinSelected != -1 && fMaxSelected != -1) {
         floorMassFrame->setVisible(true);
         storyPropertiesFrame->setVisible(true);
         //spreadSheetFrame->setVisible(false);
         floorSelected=-1;
         storySelected=-1;

     } else if (sMinSelected != -1 && sMaxSelected != -1) {
         floorMassFrame->setVisible(false);
         storyPropertiesFrame->setVisible(true);
         //spreadSheetFrame->setVisible(false);
         floorSelected=-1;
         storySelected=-1;

     } else {

         floorMassFrame->setVisible(false);
         storyPropertiesFrame->setVisible(false);
        // spreadSheetFrame->setVisible(true);
         floorSelected=-1;
         storySelected=-1;
     }

     this->draw(theView);
     theView->update();
     return;
 }
 \
