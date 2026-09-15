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
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_MainWindow
{
public:
    QWidget *centralwidget;
    QLabel *label_camera;
    QLabel *label_show_status;
    QPushButton *button_capture;
    QPushButton *button_exit;
    QMenuBar *menubar;
    QStatusBar *statusbar;

    void setupUi(QMainWindow *MainWindow)
    {
        if (MainWindow->objectName().isEmpty())
            MainWindow->setObjectName("MainWindow");
        MainWindow->resize(462, 296);
        centralwidget = new QWidget(MainWindow);
        centralwidget->setObjectName("centralwidget");
        label_camera = new QLabel(centralwidget);
        label_camera->setObjectName("label_camera");
        label_camera->setGeometry(QRect(40, 40, 361, 101));
        label_show_status = new QLabel(centralwidget);
        label_show_status->setObjectName("label_show_status");
        label_show_status->setGeometry(QRect(10, 210, 70, 22));
        button_capture = new QPushButton(centralwidget);
        button_capture->setObjectName("button_capture");
        button_capture->setGeometry(QRect(80, 160, 92, 30));
        button_exit = new QPushButton(centralwidget);
        button_exit->setObjectName("button_exit");
        button_exit->setGeometry(QRect(260, 160, 92, 30));
        MainWindow->setCentralWidget(centralwidget);
        menubar = new QMenuBar(MainWindow);
        menubar->setObjectName("menubar");
        menubar->setGeometry(QRect(0, 0, 462, 27));
        MainWindow->setMenuBar(menubar);
        statusbar = new QStatusBar(MainWindow);
        statusbar->setObjectName("statusbar");
        MainWindow->setStatusBar(statusbar);

        retranslateUi(MainWindow);

        QMetaObject::connectSlotsByName(MainWindow);
    } // setupUi

    void retranslateUi(QMainWindow *MainWindow)
    {
        MainWindow->setWindowTitle(QCoreApplication::translate("MainWindow", "MainWindow", nullptr));
        label_camera->setText(QString());
        label_show_status->setText(QCoreApplication::translate("MainWindow", "TextLabel", nullptr));
        button_capture->setText(QCoreApplication::translate("MainWindow", "capture", nullptr));
        button_exit->setText(QCoreApplication::translate("MainWindow", "exit", nullptr));
    } // retranslateUi

};

namespace Ui {
    class MainWindow: public Ui_MainWindow {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_MAINWINDOW_H
