#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "ui_findreplace.h"
#include "mdichild.h"
#include <findreplace.h>

#include <QPrinter>
#include <QPrintPreviewDialog>
#include <QPrintDialog>
#include <QFileInfo>
#include <QFileDialog>
#include <QMdiSubWindow>
#include <QMessageBox>
#include <QPushButton>
#include <QSignalMapper>
#include <QSettings>
#include <QCloseEvent>
#include <QFontDialog>
#include <QColorDialog>
#include <QUrl>
#include <QMimeData>
#include <QDragEnterEvent>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    setAcceptDrops(true);       //拖放功能
    windowMapper = new QSignalMapper(this); //创建信号映射器
    connect(windowMapper,SIGNAL(mapped(QWidget*)),this,SLOT(setActiveSubWindow(QWidget *)));
    actGrp = new QActionGroup(this);    //对齐方式
    actGrp->addAction(ui->actionleft);
    actGrp->addAction(ui->actioncenter);
    actGrp->addAction(ui->actionright);
    actGrp->addAction(ui->actionjustify);
    connect(actGrp,SIGNAL(triggered(QAction*)),this,SLOT(slotAlignment(QAction*)));//关联对齐方式的修改
    actViewMode = new QActionGroup(this);   //视图模式
    actViewMode->addAction(ui->actiontabbed);
    actViewMode->addAction(ui->actionwindow);
    connect(actViewMode,SIGNAL(triggered(QAction*)),this,SLOT(slotViewMode(QAction*)));//关联视图模式的修改

    actGrp2 = new QActionGroup(this);
    actGrp2->addAction(ui->actionTile);
    actGrp2->addAction(ui->actionCascade);

    findReplace = new FindReplace(this);//查找替换
    //查找替换相关的关联
    connect(findReplace->ui->pushButtonCancleF,SIGNAL(clicked()),findReplace,SLOT(close()));
    connect(findReplace->ui->pushButtonNextF,SIGNAL(clicked()),this,SLOT(findNextF()));//关联查找
    connect(findReplace->ui->pushButtonCancleR,SIGNAL(clicked()),findReplace,SLOT(close()));
    connect(findReplace->ui->pushButtonNextR,SIGNAL(clicked()),this,SLOT(findNextR()));
    connect(findReplace->ui->pushButtonReplace,SIGNAL(clicked()),this,SLOT(replace()));
    connect(findReplace->ui->pushButtonReplaceAll,SIGNAL(clicked()),this,SLOT(replaceAll()));

    updateMenus();      //更新菜单
    updateWindowMenu();      //更新菜单
    connect(ui->mdiArea,SIGNAL(subWindowActivated(QMdiSubWindow*)),this,SLOT(updateMenus()));//有活动窗口时更新菜单
    connect(ui->menuW,SIGNAL(aboutToShow()),this,SLOT(updateWindowMenu()));
    readSettings(); //读取窗口设置信息
    initWindow();
}

MainWindow::~MainWindow()
{
    delete ui;
}

//更新菜单
void MainWindow::updateMenus()
{
    //根据是否有活动窗口来设置各个动作是否可用
    bool hasMdiChild = (activeMdiChild()!=0);
    ui->actionSave->setEnabled(hasMdiChild);
    ui->actionSaveAs->setEnabled(hasMdiChild);
    ui->actionPaste->setEnabled(hasMdiChild);
    ui->actionClear->setEnabled(hasMdiChild);
    ui->actionSelectAll->setEnabled(hasMdiChild);
    ui->actionFind->setEnabled(hasMdiChild);
    ui->actionReplace->setEnabled(hasMdiChild);
    ui->actionFont->setEnabled(hasMdiChild);
    ui->actionFontColor->setEnabled(hasMdiChild);
    ui->actionPDF->setEnabled(hasMdiChild);
    ui->actionPrint->setEnabled(hasMdiChild);
    ui->actionPrintPreview->setEnabled(hasMdiChild);
    ui->actionSwitch->setEnabled(hasMdiChild);
    ui->actionHighLightShow->setEnabled(hasMdiChild);
    ui->actionClose->setEnabled(hasMdiChild);
    ui->actionCloseAll->setEnabled(hasMdiChild);
    ui->actionTile->setEnabled(hasMdiChild);
    ui->actionCascade->setEnabled(hasMdiChild);
    ui->actionNext->setEnabled(hasMdiChild);
    ui->actionPrevious->setEnabled(hasMdiChild);

    ui->actionzoomIn->setEnabled(hasMdiChild);
    ui->actionzoomOut->setEnabled(hasMdiChild);

    bool hasSelection = (activeMdiChild()&&activeMdiChild()->textCursor().hasSelection());//有活动窗口且选中文本

    ui->actionCut->setEnabled(hasSelection);
    ui->actionCopy->setEnabled(hasSelection);

    //有活动窗口且文档有撤销操作时撤销动作可用
    ui->actionUndo->setEnabled(activeMdiChild()&&activeMdiChild()->document()->isUndoAvailable());
    //有活动窗口且文档有恢复操作时撤销动作可用
    ui->actionRedo->setEnabled(activeMdiChild()&&activeMdiChild()->document()->isRedoAvailable());

    ui->actionPrevious->setEnabled(ui->mdiArea->subWindowList().count() > 1);//窗口数大于1时可用
    ui->actionNext->setEnabled(ui->mdiArea->subWindowList().count() > 1);

    actGrp->setEnabled(hasMdiChild);//对齐方式

    if(hasMdiChild)
    {
        this->setWindowTitle(ui->mdiArea->currentSubWindow()->windowTitle());//更新标题栏的显示
        if(!activeMdiChild()->extraSelections().empty())//判断当前的文档是否高亮显示
            ui->actionHighLightShow->setChecked(true);
        else
            ui->actionHighLightShow->setChecked(false);

        if(activeMdiChild()->wordWrapMode() == QTextOption::WrapAnywhere)//判断当前的文档是否自动换行
            ui->actionSwitch->setChecked(true);
        else
            ui->actionSwitch->setChecked(false);

        if(activeMdiChild()->alignment() == (Qt::AlignLeft|Qt::AlignLeading))
            ui->actionleft->setChecked(true);
        if(activeMdiChild()->alignment() == Qt::AlignHCenter)
            ui->actioncenter->setChecked(true);
        if(activeMdiChild()->alignment() == (Qt::AlignTrailing | Qt::AlignAbsolute))
            ui->actionright->setChecked(true);
        if(activeMdiChild()->alignment() == Qt::AlignJustify)
            ui->actionjustify->setChecked(true);
    }
}

//更新窗口列表
void MainWindow::updateWindowMenu()
{
    ui->menuWindows->clear(); //清空菜单
    QList<QMdiSubWindow *> windows = ui->mdiArea->subWindowList();
    if(!windows.isEmpty())      //如果有活动窗口，则显示间隔器
    {
        for(int i=0; i<windows.size(); ++i)     //遍历各个子窗口
        {
            MdiChild *child = qobject_cast<MdiChild *>(windows.at(i)->widget());
            QString text;
            text = tr("%1 %2").arg(i+1).arg(QFileInfo(child->currentFile()).fileName());
            QAction *action = ui->menuWindows->addAction(text);
            action->setCheckable(true);
            action->setChecked(child == activeMdiChild());
            //关联动作的触发信号到信号映射器的map槽，这个槽会发射mapped信号
            connect(action,SIGNAL(triggered()),windowMapper,SLOT(map()));
            windowMapper->setMapping(action,windows.at(i));
        }
    }
}

//获取当前活动窗口
MdiChild *MainWindow::activeMdiChild()
{
    //如果有活动窗口，则将其内部的中心部件转换为MdiChild类型，没有则直接返回0
    if(QMdiSubWindow * activeSubWindow = ui->mdiArea->activeSubWindow())
        return qobject_cast<MdiChild *>(activeSubWindow->widget());
    return 0;
}

void MainWindow::on_actionNew_triggered()
{
    MdiChild * child = createMdiChild();
    child->newFile();   //新建文件
    child->show();      //显示子窗口
    showTextRowAndCol();
}

void MainWindow::on_actionOpen_triggered()
{
    QString fileName = QFileDialog::getOpenFileName
            (this,tr("打开文件"),NULL,"(*.txt)"";;All File(*.*)");//获取文件路径
    if(!fileName.isEmpty()) //如果路径非空，则查看文件是否已经打开
    {
        QMdiSubWindow *existing = findMdiChild(fileName);
        if(existing)    //如果已经存在，将对应的子窗口设置为活动窗口
        {
            ui->mdiArea->setActiveSubWindow(existing);
            return;
        }
        MdiChild * child = createMdiChild();    //如果没有打开，则新建子窗口
        if(child->loadFile(fileName))
        {
            ui->statusBar->showMessage(tr("打开文件成功"),2000);  //状态栏
            child->show();
            showTextRowAndCol();
        }
        else
        {
            child->close();
        }
    }
}

QMdiSubWindow * MainWindow::findMdiChild(const QString &fileName)
{
    QString canonicalFilePath = QFileInfo(fileName).canonicalFilePath();
    //遍历子窗口列表，如果其文件路径和要查找的路径相同，则返回这个窗口
    foreach (QMdiSubWindow * window, ui->mdiArea->subWindowList())
    {
        MdiChild * mdiChild = qobject_cast<MdiChild *>(window->widget());
        if(mdiChild->currentFile() == canonicalFilePath)
            return window;
    }
    return 0;
}

void MainWindow::setActiveSubWindow(QWidget * window)
{
    if(window) //如果传递了窗口事件，则将其设置为活动窗口
        ui->mdiArea->setActiveSubWindow(qobject_cast<QMdiSubWindow *>(window));
}

MdiChild * MainWindow::createMdiChild()
{
    MdiChild * child = new MdiChild;    //创建MdiChild部件
    ui->mdiArea->addSubWindow(child);   //向多文档区域添加子窗口
    connect(child,SIGNAL(copyAvailable(bool)),ui->actionCut,SLOT(setEnabled(bool)));
    connect(child,SIGNAL(copyAvailable(bool)),ui->actionCopy,SLOT(setEnabled(bool)));
    connect(child->document(),SIGNAL(undoAvailable(bool)),ui->actionUndo,SLOT(setEnabled(bool)));
    connect(child->document(),SIGNAL(redoAvailable(bool)),ui->actionRedo,SLOT(setEnabled(bool)));
    connect(child,SIGNAL(cursorPositionChanged()),this,SLOT(showTextRowAndCol()));
    return child;
}

void MainWindow::on_actionSave_triggered()
{
    if(activeMdiChild() && activeMdiChild()->save())
        ui->statusBar->showMessage(tr("文件保存成功"),2000);
}

void MainWindow::on_actionSaveAs_triggered()
{
    if(activeMdiChild() && activeMdiChild()->saveAs())
        ui->statusBar->showMessage(tr("文件另存为成功"),2000);
}

void MainWindow::on_actionExit_triggered()
{
    qApp->closeAllWindows();
}

void MainWindow::on_actionUndo_triggered()
{
    if(activeMdiChild())
        activeMdiChild()->undo();
}

void MainWindow::on_actionRedo_triggered()
{
    if(activeMdiChild())
        activeMdiChild()->redo();
}

void MainWindow::on_actionClose_triggered()
{
    ui->mdiArea->closeActiveSubWindow();
}

void MainWindow::on_actionCloseAll_triggered()
{
    ui->mdiArea->closeAllSubWindows();
}

void MainWindow::on_actionCut_triggered()
{
    if(activeMdiChild())
        activeMdiChild()->cut();
}

void MainWindow::on_actionCopy_triggered()
{
    if(activeMdiChild())
        activeMdiChild()->copy();
}

void MainWindow::on_actionPaste_triggered()
{
    if(activeMdiChild())
        activeMdiChild()->paste();
}

void MainWindow::on_actionDel_triggered()
{
    if(activeMdiChild())
        activeMdiChild()->textCursor().removeSelectedText();
}


void MainWindow::on_actionSelectAll_triggered()
{
    if(activeMdiChild())
        activeMdiChild()->selectAll();
}

void MainWindow::on_actionClear_triggered()
{
    if(activeMdiChild())
        activeMdiChild()->clear();
}

void MainWindow::on_actionzoomIn_triggered()
{
    if(activeMdiChild())
        activeMdiChild()->zoomIn(2);
}

void MainWindow::on_actionzoomOut_triggered()
{
    if(activeMdiChild())
        activeMdiChild()->zoomOut(2);
}

void MainWindow::on_actionTile_triggered()
{
    ui->mdiArea->tileSubWindows();
}

void MainWindow::on_actionCascade_triggered()
{
    ui->mdiArea->cascadeSubWindows();
}

void MainWindow::on_actionNext_triggered()
{
    ui->mdiArea->activateNextSubWindow();
}

void MainWindow::on_actionPrevious_triggered()
{
    ui->mdiArea->activatePreviousSubWindow();
}

void MainWindow::on_actionAbout_triggered()
{
    QMessageBox box;
    box.setWindowTitle(tr("关于"));
    box.setIconPixmap(QPixmap(":/images/logo.png"));
    box.setText(tr("\n\n多文档文本编辑器V1.0\n\n作者：ChenZhe\n时间：2017-12-01"));
    box.addButton(tr("确定"),QMessageBox::AcceptRole);//加入自定义的按钮
    if(box.exec() == QMessageBox::Accepted)
        box.close();
}

void MainWindow::on_actionToobarShow_triggered(bool checked)
{
    if(checked == true)
    {
        ui->mainToolBar->show();
    }
    else
    {
        ui->mainToolBar->hide();
    }
}

void MainWindow::on_actionStatusBarShow_triggered(bool checked)
{
    if(checked == true)
        ui->statusBar->show();
    else
        ui->statusBar->hide();
}

void MainWindow::on_actionSwitch_triggered(bool checked)
{
    if(checked == true)
        activeMdiChild()->setWordWrapMode(QTextOption::WrapAnywhere);
    else
        activeMdiChild()->setWordWrapMode(QTextOption::NoWrap);
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    ui->mdiArea->closeAllSubWindows();  //先执行多文档区域的关闭操作
    if(ui->mdiArea->currentSubWindow()) //如果还有窗口没有关闭，则忽略事件
    {
        event->ignore();
    }
    else    //关闭程序前写入窗口设置
    {
        writeSettings();
        event->accept();
    }
}

//读取窗口设置
void MainWindow::readSettings()
{
    QSettings settings("cz","Notepad");
    //写入位置信息和大小信息
    QPoint pos = settings.value("pos",QPoint(200,200)).toPoint();
    QSize size = settings.value("size",QSize(800,500)).toSize();
    move(pos);
    resize(size);
}

//写入窗口设置
void MainWindow::writeSettings()
{
    QSettings settings("cz","Notepad");
    //写入位置信息和大小信息
    settings.setValue("pos",pos());
    settings.setValue("size",size());
}

//显示文本的行号和列号
void MainWindow::showTextRowAndCol()
{
    if(activeMdiChild())
    {
        int rowNum = activeMdiChild()->textCursor().blockNumber()+1;
        int colNum = activeMdiChild()->textCursor().columnNumber()+1;
        ui->statusBar->showMessage(tr("%1行 %2列").arg(rowNum).arg(colNum));
    }
}

//初始化窗口
void MainWindow::initWindow()
{
    setWindowTitle(tr("Notepad"));
    ui->mainToolBar->setWindowTitle(tr("工具栏"));

    ui->mdiArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    ui->mdiArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

    ui->statusBar->showMessage(tr("欢迎使用NotePad！"));

    ui->actionNew->setStatusTip(tr("新建文件"));
    ui->actionOpen->setStatusTip(tr("打开文件"));
    ui->actionSave->setStatusTip(tr("保存文件"));
    ui->actionSaveAs->setStatusTip(tr("文件另存为"));
    ui->actionExit->setStatusTip(tr("退出应用程序"));
    ui->actionUndo->setStatusTip(tr("撤销先前的操作"));
    ui->actionRedo->setStatusTip(tr("恢复先前的操作"));
    ui->actionCut->setStatusTip(tr("剪切选中的内容到剪贴板"));
    ui->actionCopy->setStatusTip(tr("复制选中的内容到剪贴板"));
    ui->actionPaste->setStatusTip(tr("粘贴剪贴板的内容到当前位置"));
    ui->actionClose->setStatusTip(tr("关闭活动窗口"));
    ui->actionCloseAll->setStatusTip(tr("关闭所有窗口"));
    ui->actionTile->setStatusTip(tr("平铺所有窗口"));
    ui->actionCascade->setStatusTip(tr("层叠所有窗口"));
    ui->actionNext->setStatusTip(tr("将焦点移动到下一个窗口"));
    ui->actionPrevious->setStatusTip(tr("将焦点移动到上一个窗口"));
    ui->actionAbout->setStatusTip(tr("显示软件介绍"));
}





void MainWindow::on_actionHighLightShow_triggered(bool checked)
{
    if(checked == true)
        highlightCurrentLine();
    else
    {
        QList<QTextEdit::ExtraSelection> extraSelections;
        activeMdiChild()->setExtraSelections(extraSelections);
        activeMdiChild()->extraSelections().clear();
    }
}

void MainWindow::highlightCurrentLine()//高亮显示当前编辑的行
{
   if(ui->actionHighLightShow->isChecked() == true)//是否选中高亮显示
    {
       QList<QTextEdit::ExtraSelection> extraSelections;
       if (!activeMdiChild()->isReadOnly())
       {
           QTextEdit::ExtraSelection selection;
           QColor lineColor = QColor(Qt::yellow).lighter(160);
           selection.format.setBackground(lineColor);
           selection.format.setProperty(QTextFormat::FullWidthSelection, true);
           selection.cursor = activeMdiChild()->textCursor();
           selection.cursor.clearSelection();
           extraSelections.append(selection);
       }
       activeMdiChild()->setExtraSelections(extraSelections);
   }
}

void MainWindow::on_actionPDF_triggered()
{
    if(activeMdiChild())
    {
        MdiChild *child = activeMdiChild();
        QString fileName = QFileDialog::getSaveFileName(
                this, tr("导出PDF"),QFileInfo(activeMdiChild()->currentFile()).completeBaseName() + ".pdf","*.pdf");
        if (!fileName.isEmpty())
        {
            if(QFileInfo(fileName).suffix().isEmpty())
                fileName.append(".pdf");
            QPrinter printer(QPrinter::HighResolution);
            printer.setOutputFormat(QPrinter::PdfFormat);
            printer.setOutputFileName(fileName);
            if(child)
            {
                child->print(&printer);
                ui->statusBar->showMessage(tr("输出PDF文档成功"),2000);
            }
            else
            {
                ui->statusBar->showMessage(tr("没有活动窗口"),2000);
            }
        }
    }
}

void MainWindow::on_actionPrintPreview_triggered()
{
    if(activeMdiChild())
    {
        QPrinter printer(QPrinter::HighResolution);
        QPrintPreviewDialog preview(&printer,this);
        connect(&preview, SIGNAL(paintRequested(QPrinter*)),this,SLOT(printPreview(QPrinter*)));
        preview.exec();
    }
}

void MainWindow::printPreview(QPrinter *printer)//打印预览
{
    activeMdiChild()->print(printer);
}

void MainWindow::on_actionPrint_triggered()
{
    QPrinter printer;
    MdiChild *child = activeMdiChild();
    QPrintDialog dlg(&printer,ui->mdiArea->activeSubWindow());
    if(dlg.exec() == QDialog::Accepted)
    {
        if(child)
        {
            child->print(&printer);
            ui->statusBar->showMessage(tr("打印成功"),2000);
        }
        else
        {
            ui->statusBar->showMessage(tr("没有活动窗口"),2000);
        }
    }
}

void MainWindow::on_actionFont_triggered()
{
    bool ok;
    QFont font = QFontDialog::getFont(&ok,activeMdiChild()->font(),this,"font");
    if(ok)
    {
        activeMdiChild()->setCurrentFont(font);//current可以undo
    }
}

void MainWindow::on_actionFontColor_triggered()
{
    QColor color = QColorDialog::getColor(Qt::black, this);
    if (color.isValid())
    {
       activeMdiChild()->setTextColor(color);
    }
}

void MainWindow::dragEnterEvent(QDragEnterEvent *event)//拖进事件
{
    if(event->mimeData()->hasFormat("text/uri-list"))
        event->acceptProposedAction();
    else
        event->ignore();
}

void MainWindow::dropEvent(QDropEvent *event)//放下事件
{
    QList<QUrl> urls = event->mimeData()->urls();
    if (urls.isEmpty())
    {
        return;
    }
    QString fileName = urls.first().toLocalFile();
    if (fileName.isEmpty())
    {
        return;
    }
    QString canonicalFilePath = QFileInfo(fileName).canonicalFilePath();
    bool hasOpened = false;//是否已经打开该文件
    foreach (QMdiSubWindow *window, ui->mdiArea->subWindowList())
    {
        MdiChild *mdiChild = qobject_cast<MdiChild *>(window->widget());
        if( mdiChild->currentFile() == canonicalFilePath)
        {
            hasOpened = true;
            ui->mdiArea->setActiveSubWindow(window);
            continue;
        }
    }
    if(!hasOpened)
    {
        MdiChild *child = new MdiChild;
        ui->mdiArea->addSubWindow(child);
        if(child->loadFile(fileName))
        {
            ui->statusBar->showMessage(tr("打开文件成功"),2000);  //状态栏
            child->show();
            showTextRowAndCol();
        }
        else
            child->close();
    }
}

void MainWindow::slotAlignment(QAction *a)//更改对齐方式
{
    if(a == ui->actionleft)
        activeMdiChild()->setAlignment(Qt::AlignLeft | Qt::AlignAbsolute);
    else if (a == ui->actioncenter)
        activeMdiChild()->setAlignment(Qt::AlignHCenter);
    else if (a == ui->actionright)
        activeMdiChild()->setAlignment(Qt::AlignRight | Qt::AlignAbsolute);
    else if (a == ui->actionjustify)
        activeMdiChild()->setAlignment(Qt::AlignJustify);
}

void MainWindow::slotViewMode(QAction *a)//视图模式
{
    if(a == ui->actiontabbed)
        ui->mdiArea->setViewMode(QMdiArea::TabbedView); //设为标签栏显示模式
    if(a == ui->actionwindow)
        ui->mdiArea->setViewMode(QMdiArea::SubWindowView);//设为窗口显示模式
    //只有在窗口模式下平铺和层叠才起作用
    bool isSubWindowView = (activeMdiChild() && ui->mdiArea->viewMode() == QMdiArea::SubWindowView);
    ui->actionCascade->setEnabled(isSubWindowView);
    ui->actionTile->setEnabled(isSubWindowView);
}

void MainWindow::on_actionFind_triggered()
{
    findReplace->show();
    findReplace->ui->tabWidget->setCurrentWidget(findReplace->ui->find);//显示查找页面
    findReplace->ui->lineEditF->setFocus(); //焦点
    ui->statusBar->showMessage(tr("正在查找"));  //状态栏
}

void MainWindow::on_actionReplace_triggered()
{
    findReplace->show();
    findReplace->ui->tabWidget->setCurrentWidget(findReplace->ui->replace);//显示查找页面
    findReplace->ui->lineEditR->setFocus(); //焦点
    ui->statusBar->showMessage(tr("正在替换"));  //状态栏
}

//查找的实现函数
void MainWindow::doFind(QString findText)
{
    if( (!sence) && (!upFind) )//不区分大小写，向后查找
    {
        if(!activeMdiChild()->find(findText))
        {
            QMessageBox::warning(this,tr("查找"),tr("找不到 %1").arg(findText));
        }
    }
    else if( sence && (!upFind) )//区分大小写，向后查找
    {
        if(!activeMdiChild()->find(findText,QTextDocument::FindCaseSensitively))
        {
            QMessageBox::warning(this,tr("查找"),tr("找不到 %1").arg(findText));
        }
    }
    else if( (!sence) && upFind )//不区分大小写，向前查找
    {
        if(!activeMdiChild()->find(findText,QTextDocument::FindBackward))
        {
            QMessageBox::warning(this,tr("查找"),tr("找不到 %1").arg(findText));
        }
    }
    else if( sence && upFind)//区分大小写，向前查找
    {
        if(!activeMdiChild()->find(findText, QTextDocument::FindCaseSensitively | QTextDocument::FindBackward))
        {
            QMessageBox::warning(this,tr("查找"),tr("找不到 %1").arg(findText));
        }
    }
}

//“查找”的“查找下一个”
void MainWindow::findNextF()
{
    QString findText = findReplace->ui->lineEditF->text();
    sence = findReplace->ui->checkBoxSenceF->isChecked();//是否区分大小写，1区分 0不区分
    upFind = findReplace->ui->radioButtonUpF->isChecked();//向前还是向后查找，1向前 0向后
    doFind(findText);
}

//“替换”的“查找下一个”
void MainWindow::findNextR()
{
    QString findText = findReplace->ui->lineEditR->text();
    sence = findReplace->ui->checkBoxSenceR->isChecked();//是否区分大小写，1区分 0不区分
    upFind = 0; //替换的查找只向下
    doFind(findText);
}

//替换
void MainWindow::replace()
{
    QString findText = findReplace->ui->lineEditR->text();//要查找的内容
    QString replaceText = findReplace->ui->lineEditReplace->text();//要替换的内容
    if(activeMdiChild()->textCursor().selectedText() == findText)
    {
        QTextCursor textCursor = activeMdiChild()->textCursor();
        textCursor.insertText(replaceText);
    }
    findNextR();//“替换”的“查找下一个”
}

//全部替换
void MainWindow::replaceAll()
{
    QTextCursor textCursor = activeMdiChild()->textCursor();
    textCursor.setPosition(0,QTextCursor::MoveAnchor);//先让光标跳到文章的最开始
    activeMdiChild()->setTextCursor(textCursor);
    sence = findReplace->ui->checkBoxSenceR->isChecked();//是否区分大小写，1区分 0不区分

    QString findText = findReplace->ui->lineEditR->text();//要查找的内容
    QString replaceText = findReplace->ui->lineEditReplace->text();//要替换的内容

    while(doReplaceAllFind(findText))//循环替换
    {
        if(activeMdiChild()->textCursor().selectedText().toLower() == findText.toLower())
        {
            QTextCursor textCursor = activeMdiChild()->textCursor();//和上面的textCursor不一样
            textCursor.insertText(replaceText);
        }
    }
}

//全部替换的查找实现函数
bool MainWindow::doReplaceAllFind(QString findText)
{
    if( (!sence) && (!upFind) )//不区分大小写，向后查找
    {
        if(!activeMdiChild()->find(findText))
        {
            return false;
        }
    }
    else if( sence && (!upFind) )//区分大小写，向后查找
    {
        if(!activeMdiChild()->find(findText,QTextDocument::FindCaseSensitively))
        {
            return false;
        }
    }
    return true;
}
