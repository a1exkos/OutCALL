#ifndef ADDORGCONTACTDIALOG_H
#define ADDORGCONTACTDIALOG_H

#include <QDialog>
#include <QValidator>
#include <QSqlQuery>
#include <QLineEdit>
#include <QKeyEvent>

namespace Ui {
class AddOrgContactDialog;
}

class AddOrgContactDialog : public QDialog
{
    Q_OBJECT

signals:
    void sendData(bool update);

public:
    explicit AddOrgContactDialog(QWidget* parent = 0);
    ~AddOrgContactDialog();

    void setValues(const QString& number);

private slots:
    void onSave();
    void onTextChanged();

    bool isPhone(QString* str);

    void keyPressEvent(QKeyEvent* event);

private:
    Ui::AddOrgContactDialog* ui;

    QSqlDatabase m_db;

    QList<QLineEdit*> m_phones;
    QList<QLineEdit*> m_phonesComments;

    QMap<QString, QLineEdit*> m_managers;
};

#endif // ADDORGCONTACTDIALOG_H
