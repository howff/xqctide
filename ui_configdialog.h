/********************************************************************************
** Form generated from reading UI file 'configdialog.ui'
**
** Created by: Qt User Interface Compiler version 6.4.2
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_CONFIGDIALOG_H
#define UI_CONFIGDIALOG_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_ConfigWidget
{
public:
    QVBoxLayout *verticalLayout;
    QGroupBox *groupBox1;
    QVBoxLayout *verticalLayout_2;
    QCheckBox *tlMustBeOnChart;
    QCheckBox *tlMustBeUnique;
    QGroupBox *groupBox2;
    QVBoxLayout *verticalLayout_3;
    QCheckBox *tsMustBeOnChart;
    QCheckBox *tsMustBeUnique;
    QCheckBox *tsMustHaveKnownRef;
    QGroupBox *groupBox3;
    QHBoxLayout *horizontalLayout;
    QLabel *zoomLabel;
    QComboBox *zoomComboBox;
    QSpacerItem *verticalSpacer;

    void setupUi(QWidget *ConfigWidget)
    {
        if (ConfigWidget->objectName().isEmpty())
            ConfigWidget->setObjectName("ConfigWidget");
        ConfigWidget->resize(419, 248);
        verticalLayout = new QVBoxLayout(ConfigWidget);
        verticalLayout->setObjectName("verticalLayout");
        groupBox1 = new QGroupBox(ConfigWidget);
        groupBox1->setObjectName("groupBox1");
        verticalLayout_2 = new QVBoxLayout(groupBox1);
        verticalLayout_2->setObjectName("verticalLayout_2");
        tlMustBeOnChart = new QCheckBox(groupBox1);
        tlMustBeOnChart->setObjectName("tlMustBeOnChart");

        verticalLayout_2->addWidget(tlMustBeOnChart);

        tlMustBeUnique = new QCheckBox(groupBox1);
        tlMustBeUnique->setObjectName("tlMustBeUnique");

        verticalLayout_2->addWidget(tlMustBeUnique);


        verticalLayout->addWidget(groupBox1);

        groupBox2 = new QGroupBox(ConfigWidget);
        groupBox2->setObjectName("groupBox2");
        verticalLayout_3 = new QVBoxLayout(groupBox2);
        verticalLayout_3->setObjectName("verticalLayout_3");
        tsMustBeOnChart = new QCheckBox(groupBox2);
        tsMustBeOnChart->setObjectName("tsMustBeOnChart");

        verticalLayout_3->addWidget(tsMustBeOnChart);

        tsMustBeUnique = new QCheckBox(groupBox2);
        tsMustBeUnique->setObjectName("tsMustBeUnique");

        verticalLayout_3->addWidget(tsMustBeUnique);

        tsMustHaveKnownRef = new QCheckBox(groupBox2);
        tsMustHaveKnownRef->setObjectName("tsMustHaveKnownRef");

        verticalLayout_3->addWidget(tsMustHaveKnownRef);


        verticalLayout->addWidget(groupBox2);

        groupBox3 = new QGroupBox(ConfigWidget);
        groupBox3->setObjectName("groupBox3");
        horizontalLayout = new QHBoxLayout(groupBox3);
        horizontalLayout->setObjectName("horizontalLayout");
        zoomLabel = new QLabel(groupBox3);
        zoomLabel->setObjectName("zoomLabel");

        horizontalLayout->addWidget(zoomLabel);

        zoomComboBox = new QComboBox(groupBox3);
        zoomComboBox->addItem(QString());
        zoomComboBox->addItem(QString());
        zoomComboBox->addItem(QString());
        zoomComboBox->addItem(QString());
        zoomComboBox->setObjectName("zoomComboBox");

        horizontalLayout->addWidget(zoomComboBox);


        verticalLayout->addWidget(groupBox3);

        verticalSpacer = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);

        verticalLayout->addItem(verticalSpacer);


        retranslateUi(ConfigWidget);

        QMetaObject::connectSlotsByName(ConfigWidget);
    } // setupUi

    void retranslateUi(QWidget *ConfigWidget)
    {
        ConfigWidget->setWindowTitle(QCoreApplication::translate("ConfigWidget", "Configuration", nullptr));
        groupBox1->setTitle(QCoreApplication::translate("ConfigWidget", "Tidal Levels", nullptr));
        tlMustBeOnChart->setText(QCoreApplication::translate("ConfigWidget", "Must be on chart", nullptr));
        tlMustBeUnique->setText(QCoreApplication::translate("ConfigWidget", "Must be unique", nullptr));
        groupBox2->setTitle(QCoreApplication::translate("ConfigWidget", "Tidal Streams", nullptr));
        tsMustBeOnChart->setText(QCoreApplication::translate("ConfigWidget", "Must be on chart", nullptr));
        tsMustBeUnique->setText(QCoreApplication::translate("ConfigWidget", "Must be unique", nullptr));
        tsMustHaveKnownRef->setText(QCoreApplication::translate("ConfigWidget", "Must have known reference", nullptr));
        groupBox3->setTitle(QCoreApplication::translate("ConfigWidget", "Zoom", nullptr));
        zoomLabel->setText(QCoreApplication::translate("ConfigWidget", "Default zoom out level:", nullptr));
        zoomComboBox->setItemText(0, QCoreApplication::translate("ConfigWidget", "Full size", nullptr));
        zoomComboBox->setItemText(1, QCoreApplication::translate("ConfigWidget", "Half size", nullptr));
        zoomComboBox->setItemText(2, QCoreApplication::translate("ConfigWidget", "Quarter size", nullptr));
        zoomComboBox->setItemText(3, QCoreApplication::translate("ConfigWidget", "Eighth size", nullptr));

    } // retranslateUi

};

namespace Ui {
    class ConfigWidget: public Ui_ConfigWidget {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_CONFIGDIALOG_H
