/********************************************************************************
** Form generated from reading UI file 'MainWindow.ui'
**
** Created by: Qt User Interface Compiler version 6.8.2
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_MAINWINDOW_H
#define UI_MAINWINDOW_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QLabel>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QStackedWidget>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_MainWindow
{
public:
    QWidget *centralwidget;
    QVBoxLayout *centralLayout;
    QStackedWidget *stackedWidget;
    QWidget *mainPage;
    QVBoxLayout *mainPageLayout;
    QSpacerItem *mainTopSpacer;
    QLabel *label_title;
    QLabel *label_subtitle;
    QSpacerItem *mainMiddleSpacer;
    QPushButton *button_capture;
    QPushButton *button_show_image;
    QSpacerItem *mainBottomSpacer;
    QMenuBar *menubar;
    QStatusBar *statusbar;

    void setupUi(QMainWindow *MainWindow)
    {
        if (MainWindow->objectName().isEmpty())
            MainWindow->setObjectName("MainWindow");
        MainWindow->resize(474, 367);
        centralwidget = new QWidget(MainWindow);
        centralwidget->setObjectName("centralwidget");
        centralLayout = new QVBoxLayout(centralwidget);
        centralLayout->setSpacing(0);
        centralLayout->setObjectName("centralLayout");
        centralLayout->setContentsMargins(0, 0, 0, 0);
        stackedWidget = new QStackedWidget(centralwidget);
        stackedWidget->setObjectName("stackedWidget");
        mainPage = new QWidget();
        mainPage->setObjectName("mainPage");
        mainPageLayout = new QVBoxLayout(mainPage);
        mainPageLayout->setSpacing(14);
        mainPageLayout->setObjectName("mainPageLayout");
        mainPageLayout->setContentsMargins(48, 48, 48, 48);
        mainTopSpacer = new QSpacerItem(20, 40, QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Expanding);

        mainPageLayout->addItem(mainTopSpacer);

        label_title = new QLabel(mainPage);
        label_title->setObjectName("label_title");
        QFont font;
        font.setPointSize(20);
        font.setBold(true);
        label_title->setFont(font);
        label_title->setAlignment(Qt::AlignCenter);

        mainPageLayout->addWidget(label_title);

        label_subtitle = new QLabel(mainPage);
        label_subtitle->setObjectName("label_subtitle");
        label_subtitle->setAlignment(Qt::AlignCenter);

        mainPageLayout->addWidget(label_subtitle);

        mainMiddleSpacer = new QSpacerItem(20, 20, QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Expanding);

        mainPageLayout->addItem(mainMiddleSpacer);

        button_capture = new QPushButton(mainPage);
        button_capture->setObjectName("button_capture");
        button_capture->setMinimumSize(QSize(0, 44));

        mainPageLayout->addWidget(button_capture);

        button_show_image = new QPushButton(mainPage);
        button_show_image->setObjectName("button_show_image");
        button_show_image->setMinimumSize(QSize(0, 44));

        mainPageLayout->addWidget(button_show_image);

        mainBottomSpacer = new QSpacerItem(20, 40, QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Expanding);

        mainPageLayout->addItem(mainBottomSpacer);

        stackedWidget->addWidget(mainPage);

        centralLayout->addWidget(stackedWidget);

        MainWindow->setCentralWidget(centralwidget);
        menubar = new QMenuBar(MainWindow);
        menubar->setObjectName("menubar");
        menubar->setGeometry(QRect(0, 0, 474, 27));
        MainWindow->setMenuBar(menubar);
        statusbar = new QStatusBar(MainWindow);
        statusbar->setObjectName("statusbar");
        MainWindow->setStatusBar(statusbar);

        retranslateUi(MainWindow);

        stackedWidget->setCurrentIndex(0);


        QMetaObject::connectSlotsByName(MainWindow);
    } // setupUi

    void retranslateUi(QMainWindow *MainWindow)
    {
        MainWindow->setWindowTitle(QCoreApplication::translate("MainWindow", "MainWindow", nullptr));
        label_title->setText(QCoreApplication::translate("MainWindow", "Palm Vein System", nullptr));
        label_subtitle->setText(QCoreApplication::translate("MainWindow", "Capture and review palm vein images", nullptr));
        button_capture->setText(QCoreApplication::translate("MainWindow", "Capture image", nullptr));
        button_show_image->setText(QCoreApplication::translate("MainWindow", "View images", nullptr));
    } // retranslateUi

};

namespace Ui {
    class MainWindow: public Ui_MainWindow {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_MAINWINDOW_H
