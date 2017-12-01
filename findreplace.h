#ifndef FINDREPLACE_H
#define FINDREPLACE_H

#include <QDialog>

namespace Ui {
class FindReplace;
}

class FindReplace : public QDialog
{
    Q_OBJECT

public:
    Ui::FindReplace *ui;
    explicit FindReplace(QWidget *parent = 0);
    ~FindReplace();

private slots:
    void button_Enabed();

};

#endif // FINDREPLACE_H
