/****************************************************************************
** Form interface generated from reading ui file 'configdialog.ui'
**
** Created: Wed Feb 23 10:46:43 2011
**      by: The User Interface Compiler ($Id: qt/main.cpp   3.3.8   edited Jan 11 14:47 $)
**
** WARNING! All changes made in this file will be lost!
****************************************************************************/

#ifndef FORM1_H
#define FORM1_H

#include <qvariant.h>
#include <qwidget.h>

class QVBoxLayout;
class QHBoxLayout;
class QGridLayout;
class QSpacerItem;
class QGroupBox;
class QCheckBox;
class QLabel;
class QComboBox;

class ConfigWidget : public QWidget
{
    Q_OBJECT

public:
    ConfigWidget( QWidget* parent = 0, const char* name = 0, WFlags fl = 0 );
    ~ConfigWidget();

    QGroupBox* groupBox1;
    QCheckBox* TLmustBeOnChart;
    QCheckBox* TLmustBeUnique;
    QGroupBox* groupBox2;
    QCheckBox* TSmustBeOnChart;
    QCheckBox* TSmustBeUnique;
    QCheckBox* TSmustHaveKnownRef;
    QLabel* textLabel1;
    QComboBox* zoomOutLevel;

protected:
    QVBoxLayout* Form1Layout;
    QVBoxLayout* layout6;
    QSpacerItem* spacer1;
    QSpacerItem* spacer2;
    QVBoxLayout* groupBox1Layout;
    QVBoxLayout* groupBox2Layout;
    QHBoxLayout* layout2;

protected slots:
    virtual void languageChange();

};

#endif // FORM1_H
