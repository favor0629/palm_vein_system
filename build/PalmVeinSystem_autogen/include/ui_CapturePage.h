/********************************************************************************
** Form generated from reading UI file 'CapturePage.ui'
**
** Created by: Qt User Interface Compiler version 6.8.2
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_CAPTUREPAGE_H
#define UI_CAPTUREPAGE_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_CapturePage
{
public:
    QVBoxLayout *capturePageLayout;
    QLabel *label_status;
    QLabel *label_show;
    QHBoxLayout *captureButtonLayout;
    QPushButton *button_capture;
    QPushButton *button_exit;

    void setupUi(QWidget *CapturePage)
    {
        if (CapturePage->objectName().isEmpty())
            CapturePage->setObjectName("CapturePage");
        CapturePage->resize(400, 300);
        capturePageLayout = new QVBoxLayout(CapturePage);
        capturePageLayout->setSpacing(12);
        capturePageLayout->setObjectName("capturePageLayout");
        capturePageLayout->setContentsMargins(24, 20, 24, 20);
        label_status = new QLabel(CapturePage);
        label_status->setObjectName("label_status");
        label_status->setAlignment(Qt::AlignCenter);
        label_status->setWordWrap(true);

        capturePageLayout->addWidget(label_status);

        label_show = new QLabel(CapturePage);
        label_show->setObjectName("label_show");
        label_show->setMinimumSize(QSize(320, 220));
        label_show->setAlignment(Qt::AlignCenter);
        label_show->setFrameShape(QFrame::StyledPanel);
        label_show->setFrameShadow(QFrame::Raised);
        QSizePolicy sizePolicy(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Expanding);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(1);
        sizePolicy.setHeightForWidth(label_show->sizePolicy().hasHeightForWidth());
        label_show->setSizePolicy(sizePolicy);

        capturePageLayout->addWidget(label_show);

        captureButtonLayout = new QHBoxLayout();
        captureButtonLayout->setSpacing(12);
        captureButtonLayout->setObjectName("captureButtonLayout");
        button_capture = new QPushButton(CapturePage);
        button_capture->setObjectName("button_capture");
        button_capture->setMinimumSize(QSize(0, 42));

        captureButtonLayout->addWidget(button_capture);

        button_exit = new QPushButton(CapturePage);
        button_exit->setObjectName("button_exit");
        button_exit->setMinimumSize(QSize(0, 42));

        captureButtonLayout->addWidget(button_exit);


        capturePageLayout->addLayout(captureButtonLayout);


        retranslateUi(CapturePage);

        QMetaObject::connectSlotsByName(CapturePage);
    } // setupUi

    void retranslateUi(QWidget *CapturePage)
    {
        CapturePage->setWindowTitle(QCoreApplication::translate("CapturePage", "Capture Image", nullptr));
        label_status->setText(QCoreApplication::translate("CapturePage", "Camera is starting...", nullptr));
        label_show->setText(QCoreApplication::translate("CapturePage", "No camera preview", nullptr));
        button_capture->setText(QCoreApplication::translate("CapturePage", "Capture", nullptr));
        button_exit->setText(QCoreApplication::translate("CapturePage", "Back", nullptr));
    } // retranslateUi

};

namespace Ui {
    class CapturePage: public Ui_CapturePage {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_CAPTUREPAGE_H
