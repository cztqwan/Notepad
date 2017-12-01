#include "mdichild.h"
#include <QFile>
#include <QMessageBox>
#include <QTextStream>
#include <QFileInfo>
#include <QApplication>
#include <QFileDialog>
#include <QCloseEvent>
#include <QPushButton>
#include <QMenu>
#include <QScrollBar>

MdiChild::MdiChild()
{
    setWindowIcon(QIcon(":/images/file.ico"));      //设置文本窗口图标
    setAcceptDrops(false);                          //拖放功能
    //setFontPointSize(12);                           //设置字体大小
    setWordWrapMode(QTextOption::WrapAnywhere);     //设为自动换行
    setAttribute(Qt::WA_DeleteOnClose); //设置在子窗口关闭时销毁这个类的对象
    isUntitled = true;      //初始isUntitled为true,文档没有保存过
    connect(document(),SIGNAL(contentsChanged()),this,SLOT(documentWasModified())); //文档有变动时更改[*]
}

//新建文件操作
void MdiChild::newFile()
{
    static int sequenceNumber = 1;      //设置窗口编号
    isUntitled = true;                  //新建文件没有保存过
    curFile = tr("未命名文档%1.txt").arg(sequenceNumber++);  //将当前文件命名为为命名文档加编号，编号先使用再加1
    setWindowTitle(curFile + "[*]");  //设置窗口标题，使用[*]可以在文档被更改后在文件名称后显示“*”号
}

//加载文件
bool MdiChild::loadFile(const QString &fileName)
{
    QFile file(fileName);   //新建QFile对象
    //以只读的方式打开文件，出错则提示，并且返回false
    if(!file.open(QFile::ReadOnly | QFile::Text))
    {
        QMessageBox::warning(this,tr("多文档编辑器"),
                             tr("无法读取文件 %1：\n%2.")
                             .arg(fileName).arg(file.errorString()));
        return false;
    }

    QTextStream in(&file);  //新建文本流对象
    QApplication::setOverrideCursor(Qt::WaitCursor);    //设置鼠标为等待状态
    setPlainText(in.readAll()); //读取文件，添加到编辑器中
    QApplication::restoreOverrideCursor();  //恢复鼠标状态
    setCurrentFile(fileName);   //设置当前文件
    return true;
}

//保存操作
bool MdiChild::save()
{
    if(isUntitled)
        return saveAs();
    else
        return saveFile(curFile);
}

//另存为操作
bool MdiChild::saveAs()
{
    QString fileName = QFileDialog::getSaveFileName(this,tr("另存为"),curFile,"(*.txt)"";;ALL File(*.*)");//获取文件路径
    if(fileName.isEmpty())
        return false;
    return saveFile(fileName);
}

//保存文件
bool MdiChild::saveFile(const QString &fileName)
{
    QFile file(fileName);
    if(!file.open(QFile::WriteOnly|QFile::Text))
    {
        QMessageBox::warning(this,tr("多文档编辑器"),
                             tr("无法写入文件 %1：\n%2.")
                             .arg(fileName).arg(file.errorString()));
        return false;
    }
    QTextStream out(&file);  //新建文本流对象
    QApplication::setOverrideCursor(Qt::WaitCursor);    //设置鼠标为等待状态
    out << toPlainText(); //写文件
    QApplication::restoreOverrideCursor();  //恢复鼠标状态
    setCurrentFile(fileName);   //设置当前文件
    return true;
}

//设置当前文件标题
void MdiChild::setCurrentFile(const QString &fileName)
{
    curFile = QFileInfo(fileName).canonicalFilePath();//去除路径的符号链接等符号
    isUntitled = false; //文件已经被保存过了
    document()->setModified(false);
    setWindowModified(false);//窗口不显示被更改标志
    //设置窗口标题
    setWindowTitle(QFileInfo(curFile).fileName() + "[*]");//从文件路径中提取文件名
}

//根据文件是否被更改修改标题的[*]
void MdiChild::documentWasModified()
{
    setWindowModified(document()->isModified());
}

//是否关闭窗口
bool MdiChild::maybeSave()
{
    if(document()->isModified())        //如果文件被更改过
    {
        QMessageBox box;
        box.setWindowTitle(tr("警告"));
        box.setText(tr("是否保存对“%1”的更改？")
                    .arg(QFileInfo(curFile).fileName()));
        box.setIcon(QMessageBox::Warning);
        QPushButton *yesBtn = box.addButton(tr("是(&Y)"),QMessageBox::YesRole);
        box.addButton(tr("否(&N)"),QMessageBox::NoRole);
        QPushButton *cancelBtn = box.addButton(tr("取消"),QMessageBox::RejectRole);
        box.exec(); //让用户选择
        if(box.clickedButton() == yesBtn)
            return save();
        else if(box.clickedButton() == cancelBtn)
            return false;
    }
    return true;
}

//关闭事件
void MdiChild::closeEvent(QCloseEvent *event)
{
    //如果maybeSave()函数返回true,则关闭窗口，否则忽略该事件
    if(maybeSave())
        event->accept();
    else
        event->ignore();
}

//右键菜单事件
void MdiChild::contextMenuEvent(QContextMenuEvent *event)
{
    QMenu * menu = new QMenu;
    QAction * undo = menu->addAction(tr("撤销(&U)"),this,SLOT(undo()),QKeySequence::Undo);
    undo->setEnabled(document()->isUndoAvailable());
    QAction * redo = menu->addAction(tr("恢复(&R)"),this,SLOT(redo()),QKeySequence::Redo);
    redo->setEnabled(document()->isRedoAvailable());
    menu->addSeparator();
    QAction * cut = menu->addAction(tr("剪切(&T)"),this,SLOT(cut()),QKeySequence::Cut);
    cut->setEnabled(textCursor().hasSelection());
    QAction * copy = menu->addAction(tr("复制(&C)"),this,SLOT(copy()),QKeySequence::Copy);
    copy->setEnabled(textCursor().hasSelection());
    menu->addAction(tr("粘贴(&P)"),this,SLOT(paste()),QKeySequence::Paste);
    menu->addSeparator();
    QAction * del = menu->addAction(tr("删除"),this,SLOT(Del()),QKeySequence::Delete);    //发送信号
    del->setEnabled(textCursor().hasSelection());
    QAction * select = menu->addAction(tr("全选"),this,SLOT(selectAll()),QKeySequence::SelectAll);
    select->setEnabled(!document()->isEmpty());
    menu->exec(event->globalPos()); //获取鼠标位置
    delete menu;    //最后销毁菜单
}

//重写鼠标滚轮事件，实现按下Ctrl键同时滚动滚轮放大和缩小字体
void MdiChild::wheelEvent(QWheelEvent *event)
{
    if (QApplication::keyboardModifiers() == Qt::ControlModifier)//是否按下Ctrl键
    {
        if(event->delta() > 0 )//上滚
            this->zoomIn(1);//放大
        else
            this->zoomOut(1);
    }
    else//实现文本的上下滚动
    {
         if(event->delta() > 0 )//上滚
             this->verticalScrollBar()->setValue(this->verticalScrollBar()->value()-25);
         else
             this->verticalScrollBar()->setValue(this->verticalScrollBar()->value()+25);
    }
}
