#ifndef PALM_VEIN_MAIN_WINDOW_H
#define PALM_VEIN_MAIN_WINDOW_H

#include <QMainWindow>

namespace Ui
{
	class MainWindow;
}

class MainWindow : public QMainWindow
{
	public:
		explicit MainWindow(QWidget *parent = nullptr);
		~MainWindow() override;

	private:
		Ui::MainWindow *ui;
};

#endif
