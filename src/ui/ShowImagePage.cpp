#include "ShowImagePage.h"

#include "ui_ShowImagePage.h"

namespace palmvein
{

ShowImagePage::ShowImagePage(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::ShowImagePage)
{
    ui->setupUi(this);


    connect(
        ui->button_exit,
        &QPushButton::clicked,
        this,
        &ShowImagePage::onBackClicked
    );
}


ShowImagePage::~ShowImagePage()
{
    delete ui;
}


void ShowImagePage::onBackClicked()
{
    Q_EMIT backRequested();
}

}