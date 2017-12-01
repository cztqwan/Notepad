#ifndef MDICHILD_H
#define MDICHILD_H

#include <QTextEdit>

class MdiChild : public QTextEdit
{
    Q_OBJECT
public:
    MdiChild();
    void newFile();                             //新建文件
    bool loadFile(const QString &fileName);     //加载文件
    bool save();                                //保存操作
    bool saveAs();                              //另存为操作
    bool saveFile(const QString &fileName);     //保存文件
    QString currentFile(){return curFile;}      //返回当前文件路径

protected:
    void closeEvent(QCloseEvent *event);       //关闭事件
    void contextMenuEvent(QContextMenuEvent *event);       //右键菜单事件
    void wheelEvent(QWheelEvent *event);       //滚轮事件

private slots:
    void documentWasModified();                 //文件更改时，窗口显示更改状态标志

private:
    bool maybeSave();                           //是否需要保存
    void setCurrentFile(const QString &fileName);      //设置当前文件
    QString curFile;                            //保存当前文件路径
    bool isUntitled;                            //作为当前文件是否被保存到硬盘上的标志

signals:
    void Del();//删除信号
};

#endif // MDICHILD_H
