/****************************************************************************
** Form implementation generated from reading ui file 'configdialog.ui'
**
** Created: Wed Feb 23 10:47:06 2011
**      by: The User Interface Compiler ($Id: qt/main.cpp   3.3.8   edited Jan 11 14:47 $)
**
** WARNING! All changes made in this file will be lost!
****************************************************************************/

#include "configdialog.h"

#include <qvariant.h>
#include <qpushbutton.h>
#include <qgroupbox.h>
#include <qcheckbox.h>
#include <qlabel.h>
#include <qcombobox.h>
#include <qlayout.h>
#include <qtooltip.h>
#include <qwhatsthis.h>

/*
 *  Constructs a ConfigWidget as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 */
ConfigWidget::ConfigWidget( QWidget* parent, const char* name, WFlags fl )
    : QWidget( parent, name, fl )
{
    if ( !name )
	setName( "Form1" );
    Form1Layout = new QVBoxLayout( this, 11, 6, "Form1Layout"); 

    layout6 = new QVBoxLayout( 0, 0, 6, "layout6"); 

    groupBox1 = new QGroupBox( this, "groupBox1" );
    groupBox1->setColumnLayout(0, Qt::Vertical );
    groupBox1->layout()->setSpacing( 6 );
    groupBox1->layout()->setMargin( 11 );
    groupBox1Layout = new QVBoxLayout( groupBox1->layout() );
    groupBox1Layout->setAlignment( Qt::AlignTop );

    TLmustBeOnChart = new QCheckBox( groupBox1, "TLmustBeOnChart" );
    groupBox1Layout->addWidget( TLmustBeOnChart );

    TLmustBeUnique = new QCheckBox( groupBox1, "TLmustBeUnique" );
    TLmustBeUnique->setChecked( TRUE );
    groupBox1Layout->addWidget( TLmustBeUnique );
    layout6->addWidget( groupBox1 );
    spacer1 = new QSpacerItem( 20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding );
    layout6->addItem( spacer1 );

    groupBox2 = new QGroupBox( this, "groupBox2" );
    groupBox2->setColumnLayout(0, Qt::Vertical );
    groupBox2->layout()->setSpacing( 6 );
    groupBox2->layout()->setMargin( 11 );
    groupBox2Layout = new QVBoxLayout( groupBox2->layout() );
    groupBox2Layout->setAlignment( Qt::AlignTop );

    TSmustBeOnChart = new QCheckBox( groupBox2, "TSmustBeOnChart" );
    groupBox2Layout->addWidget( TSmustBeOnChart );

    TSmustBeUnique = new QCheckBox( groupBox2, "TSmustBeUnique" );
    TSmustBeUnique->setChecked( TRUE );
    groupBox2Layout->addWidget( TSmustBeUnique );

    TSmustHaveKnownRef = new QCheckBox( groupBox2, "TSmustHaveKnownRef" );
    TSmustHaveKnownRef->setEnabled( FALSE );
    TSmustHaveKnownRef->setChecked( TRUE );
    groupBox2Layout->addWidget( TSmustHaveKnownRef );
    layout6->addWidget( groupBox2 );
    spacer2 = new QSpacerItem( 20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding );
    layout6->addItem( spacer2 );

    layout2 = new QHBoxLayout( 0, 0, 6, "layout2"); 

    textLabel1 = new QLabel( this, "textLabel1" );
    layout2->addWidget( textLabel1 );

    zoomOutLevel = new QComboBox( FALSE, this, "zoomOutLevel" );
    layout2->addWidget( zoomOutLevel );
    layout6->addLayout( layout2 );
    Form1Layout->addLayout( layout6 );
    languageChange();
    resize( QSize(419, 248).expandedTo(minimumSizeHint()) );
    clearWState( WState_Polished );
}

/*
 *  Destroys the object and frees any allocated resources
 */
ConfigWidget::~ConfigWidget()
{
    // no need to delete child widgets, Qt does it all for us
}

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void ConfigWidget::languageChange()
{
    setCaption( tr( "Form1" ) );
    groupBox1->setTitle( tr( "Tidal Levels" ) );
    TLmustBeOnChart->setText( tr( "Tidal Level must refer to the currently-loaded chart" ) );
    QToolTip::add( TLmustBeOnChart, tr( "It is not sufficient to be visible on the currently loaded chart, the Tidal Level must also explicitly reference the chart by name" ) );
    TLmustBeUnique->setText( tr( "Tidal Level must be unique" ) );
    QToolTip::add( TLmustBeUnique, tr( "There must be no other Tidal Levels at its location" ) );
    groupBox2->setTitle( tr( "Tidal Streams" ) );
    TSmustBeOnChart->setText( tr( "Tidal Stream must refer to the currently-loaded chart" ) );
    QToolTip::add( TSmustBeOnChart, tr( "It is not sufficient to be visible on the currently loaded chart, the Tidal Stream must also explicitly reference the chart by name" ) );
    TSmustBeUnique->setText( tr( "Tidal Stream must be unique" ) );
    QToolTip::add( TSmustBeUnique, tr( "There must be no other Tidal Streams at its location" ) );
    TSmustHaveKnownRef->setText( tr( "Tidal Stream must reference a known Tidal Level station (port)" ) );
    textLabel1->setText( tr( "Default zoom" ) );
    zoomOutLevel->clear();
    zoomOutLevel->insertItem( tr( "Full size" ) );
    zoomOutLevel->insertItem( tr( "Half size" ) );
    zoomOutLevel->insertItem( tr( "Quarter size" ) );
    zoomOutLevel->insertItem( tr( "Eighth size" ) );
}

