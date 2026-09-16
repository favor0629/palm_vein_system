/********************************************************************************
** Form generated from reading UI file 'ShowImagePage.ui'
**
** Created by: Qt User Interface Compiler version 6.8.2
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_SHOWIMAGEPAGE_H
#define UI_SHOWIMAGEPAGE_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_ShowImagePage
{
public:
    QVBoxLayout *showImagePageLayout;
    QLabel *label_show_image;
    QPushButton *button_exit;

    void setupUi(QWidget *ShowImagePage)
    {
        if (ShowImagePage->objectName().isEmpty())
            ShowImagePage->setObjectName("ShowImagePage");
        ShowImagePage->resize(400, 300);
        showImagePageLayout = new QVBoxLayout(ShowImagePage);
        showImagePageLayout->setSpacing(12);
        showImagePageLayout->setObjectName("showImagePageLayout");
        showImagePageLayout->setContentsMargins(24, 20, 24, 20);
        label_show_image = new QLabel(ShowImagePage);
        label_show_image->setObjectName("label_show_image");
        label_show_image->setMinimumSize(QSize(320, 260));
        label_show_image->setAlignment(Qt::AlignCenter);
        label_show_image->setFrameShape(QFrame::StyledPanel);
        label_show_image->setFrameShadow(QFrame::Raised);
        QSizePolicy sizePolicy(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Expanding);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(1);
        sizePolicy.setHeightForWidth(label_show_image->sizePolicy().hasHeightForWidth());
        label_show_image->setSizePolicy(sizePolicy);

        showImagePageLayout->addWidget(label_show_image);

        button_exit = new QPushButton(ShowImagePage);
        button_exit->setObjectName("button_exit");
        button_exit->setMinimumSize(QSize(0, 42));

        showImagePageLayout->addWidget(button_exit);


        retranslateUi(ShowImagePage);

        QMetaObject::connectSlotsByName(ShowImagePage);
    } // setupUi

    void retranslateUi(QWidget *ShowImagePage)
    {
        ShowImagePage->setWindowTitle(QCoreApplication::translate("ShowImagePage", "View Images", nullptr));
        label_show_image->setText(QCoreApplication::translate("ShowImagePage", "No captured images", nullptr));
        button_exit->setText(QCoreApplication::translate("ShowImagePage", "Back", nullptr));
    } // retranslateUi

};

namespace Ui {
    class ShowImagePage: public Ui_ShowImagePage {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_SHOWIMAGEPAGE_H
