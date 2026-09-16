#ifndef SHOWIMAGEPAGE_H
#define SHOWIMAGEPAGE_H

#include <QWidget>

namespace Ui
{
class ShowImagePage;
}

namespace palmvein
{

class ShowImagePage : public QWidget
{
    Q_OBJECT

public:
    explicit ShowImagePage(QWidget *parent = nullptr);
    ~ShowImagePage();

Q_SIGNALS:

    void backRequested();

private Q_SLOTS:

    void onBackClicked();

private:

    Ui::ShowImagePage *ui;
};

}

#endif // ShowImagePage_H