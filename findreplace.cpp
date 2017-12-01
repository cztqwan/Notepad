#include "findreplace.h"
#include "ui_findreplace.h"

FindReplace::FindReplace(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::FindReplace)
{
    ui->setupUi(this);
    setFixedSize(400,160);//固定大小
    setWindowIcon(QPixmap(":/images/replace.png"));
    ui->radioButtonDownF->setChecked(true);//默认向下进行查找

    ui->pushButtonNextF->setEnabled(!ui->lineEditF->text().isEmpty());
    connect(ui->lineEditF,SIGNAL(textChanged(QString)),this,SLOT(button_Enabed()));

    ui->pushButtonNextR->setEnabled(!ui->lineEditR->text().isEmpty());
    connect(ui->lineEditR,SIGNAL(textChanged(QString)),this,SLOT(button_Enabed()));

    ui->pushButtonReplace->setEnabled(!ui->lineEditR->text().isEmpty());
    ui->pushButtonReplaceAll->setEnabled(!ui->lineEditR->text().isEmpty());
}

FindReplace::~FindReplace()
{
    delete ui;
}

//判断“查找下一个”等按钮是否可用
void FindReplace::button_Enabed()
{
    ui->pushButtonNextF->setEnabled(!ui->lineEditF->text().isEmpty());
    ui->pushButtonNextR->setEnabled(!ui->lineEditR->text().isEmpty());
    ui->pushButtonReplace->setEnabled(!ui->lineEditR->text().isEmpty());
    ui->pushButtonReplaceAll->setEnabled(!ui->lineEditR->text().isEmpty());
}
