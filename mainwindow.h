#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

class QPrinter;
class MdiChild;
class QMdiSubWindow;
class QSignalMapper;
class QActionGroup;
class FindReplace;

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private slots:
    void updateMenus();
    void updateWindowMenu();        //更新窗口菜单
    void setActiveSubWindow(QWidget * window);
    void showTextRowAndCol();       //显示文本的行号和列号
    void printPreview(QPrinter *printer);//打印预览
    void slotAlignment(QAction *a); //更改对齐方式
    void slotViewMode(QAction *a);//视图模式
    void findNextF();   //“查找”的“查找下一个”
    void findNextR();   //“替换”的“查找下一个”
    void replace();     //替换
    void replaceAll();  //全部替换

    void on_actionNew_triggered();
    void on_actionOpen_triggered();
    void on_actionSave_triggered();
    void on_actionSaveAs_triggered();
    void on_actionExit_triggered();
    void on_actionUndo_triggered();
    void on_actionRedo_triggered();
    void on_actionCut_triggered();
    void on_actionCopy_triggered();
    void on_actionPaste_triggered();
    void on_actionClose_triggered();
    void on_actionCloseAll_triggered();
    void on_actionTile_triggered();
    void on_actionCascade_triggered();
    void on_actionNext_triggered();
    void on_actionPrevious_triggered();
    void on_actionAbout_triggered();
    void on_actionDel_triggered();
    void on_actionSelectAll_triggered();
    void on_actionClear_triggered();
    void on_actionzoomIn_triggered();
    void on_actionzoomOut_triggered();
    void on_actionHighLightShow_triggered(bool checked);
    void on_actionToobarShow_triggered(bool checked);
    void on_actionStatusBarShow_triggered(bool checked);
    void on_actionSwitch_triggered(bool checked);
    void on_actionPDF_triggered();
    void on_actionPrintPreview_triggered();
    void on_actionPrint_triggered();
    void on_actionFont_triggered();
    void on_actionFontColor_triggered();
    void on_actionFind_triggered();
    void on_actionReplace_triggered();

protected:
    void closeEvent(QCloseEvent *event);    //关闭事件
    void dragEnterEvent(QDragEnterEvent *event);
    void dropEvent(QDropEvent *event);

private:
    Ui::MainWindow *ui;
    bool sence;         //是否区分大小写，1区分 0不区分
    bool upFind;        //向前还是向后查找，1向前 0向后
    FindReplace *findReplace;
    QSignalMapper *windowMapper;    //信号映射器
    QActionGroup *actGrp;
    QActionGroup *actViewMode;
    QActionGroup *actGrp2;
    MdiChild * activeMdiChild();
    MdiChild * createMdiChild();
    QMdiSubWindow * findMdiChild(const QString &fileName);
    void readSettings();    //读取窗口设置
    void writeSettings();   //写入窗口设置
    void initWindow();      //初始化窗口
    void highlightCurrentLine();
    void doFind(QString findText);
    bool doReplaceAllFind(QString findText);
};

#endif // MAINWINDOW_H
