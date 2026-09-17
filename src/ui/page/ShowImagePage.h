#ifndef SHOWIMAGEPAGE_H
#define SHOWIMAGEPAGE_H

#include <QString>
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

    // 浏览图片
    void onBrowseClicked();

private:

    // 获取图片目录
    QString getImageDirectory() const;

    // 显示图片
    void showImage(const QString &filePath);
    
    Ui::ShowImagePage *ui;
};

}

#endif // ShowImagePage_H