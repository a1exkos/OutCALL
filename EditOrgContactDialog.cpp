#include "EditOrgContactDialog.h"
#include "ui_EditOrgContactDialog.h"

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QMessageBox>
#include <QDesktopWidget>

EditOrgContactDialog::EditOrgContactDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::EditOrgContactDialog)
{
    ui->setupUi(this);

    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    setWindowFlags(windowFlags() & Qt::WindowMinimizeButtonHint);

    ui->label_6->setText(tr("1<span style=\"color: red;\">*</span>"));
    ui->label_3->setText(tr("Название организации:<span style=\"color: red;\">*</span>"));

    connect(ui->Comment, &QTextEdit::textChanged, this, &EditOrgContactDialog::onTextChanged);
    connect(ui->backButton, &QPushButton::clicked, this, &EditOrgContactDialog::onReturn);
    connect(ui->saveButton, &QPushButton::clicked, this, &EditOrgContactDialog::onSave);

    phonesList = { ui->FirstNumber, ui->SecondNumber, ui->ThirdNumber, ui->FourthNumber, ui->FifthNumber };
}

EditOrgContactDialog::~EditOrgContactDialog()
{
    delete ui;
}

void EditOrgContactDialog::onReturn()
{
    emit sendData(false, this->pos().x(), this->pos().y());

    close();
}

void EditOrgContactDialog::setPos(int x, int y)
{
    int nDesktopHeight;
    int nDesktopWidth;
    int nWidgetHeight = QWidget::height();
    int nWidgetWidth = QWidget::width();

    QDesktopWidget desktop;
    QRect rcDesktop = desktop.availableGeometry(this);

    nDesktopWidth = rcDesktop.width();
    nDesktopHeight = rcDesktop.height();

    if (x < 0 && (nDesktopHeight - y) > nWidgetHeight)
    {
        x = 0;
        this->move(x, y);
    }
    else if (x < 0 && ((nDesktopHeight - y) < nWidgetHeight))
    {
        x = 0;
        y = nWidgetHeight;
        this->move(x, y);
    }
    else if ((nDesktopWidth - x) < nWidgetWidth && (nDesktopHeight-y) > nWidgetHeight)
    {
        x = nWidgetWidth;
        this->move(x, y);
    }
    else if ((nDesktopWidth - x) < nWidgetWidth && ((nDesktopHeight - y) < nWidgetHeight))
    {
        x = nWidgetWidth;
        y = nWidgetHeight;
        this->move(x, y);
    }
    else if (x > 0 && ((nDesktopHeight - y) < nWidgetHeight))
    {
        y = nWidgetHeight;
        this->move(x, y);
    }
    else
    {
        this->move(x, y);
    }
}

void EditOrgContactDialog::onSave()
{
    QSqlDatabase db;
    QSqlQuery query(db);

    QString orgName = QString(ui->OrgName->text());

    if (QString(ui->OrgName->text()).isEmpty())
    {
         ui->label_15->setText(tr("<span style=\"color: red;\">Заполните обязательное поле!</span>"));

         ui->OrgName->setStyleSheet("border: 1px solid red");

         return;
    }
    else
    {
        ui->label_15->setText(tr(""));

        ui->OrgName->setStyleSheet("border: 1px solid grey");
    }

    if (QString(ui->FirstNumber->text()).isEmpty())
    {
        ui->label_14->show();
        ui->label_14->setText(tr("<span style=\"color: red;\">Заполните обязательное поле!</span>"));

        ui->FirstNumber->setStyleSheet("border: 1px solid red");

        return;
    }
    else
    {
        ui->label_14->setText(tr(""));

        ui->FirstNumber->setStyleSheet("border: 1px solid grey");
    }

    for (int i = 0; i < phonesList.length(); ++i)
    {
        if (!phonesList.at(i)->text().isEmpty())
        {
            QString phone = QString(phonesList.at(i)->text());

            if (isPhone(&phone) && !isInternalPhone(&phone))
                phonesList.at(i)->setStyleSheet("border: 1px solid grey");
            else
            {
                phonesList.at(i)->setStyleSheet("border: 1px solid red");

                QMessageBox::critical(this, QObject::tr("Ошибка"), QObject::tr("Номер не соответствует формату!"), QMessageBox::Ok);

                return;
            }
        }
    }

    if (!QString(ui->OrgName->text()).isEmpty() && !QString(ui->FirstNumber->text()).isEmpty())
    {
        ui->label_15->setText(tr(""));
        ui->label_14->setText(tr(""));

        ui->OrgName->setStyleSheet("border: 1px solid grey");

        for(int i = 0; i < phonesList.length(); ++i)
            phonesList.at(i)->setStyleSheet("border: 1px solid grey");
    }

    for (int i = 0; i < phonesList.length(); ++i)
        if (!phonesList.at(i)->text().isEmpty())
        {
            query.prepare("SELECT EXISTS (SELECT entry_phone FROM entry_phone WHERE entry_phone = '" + phonesList.at(i)->text() + "' AND NOT entry_id = " + updateID + ")");
            query.exec();
            query.next();

            if (query.value(0) != 0)
            {
                phonesList.at(i)->setStyleSheet("border: 1px solid red");

                QMessageBox::critical(this, QObject::tr("Ошибка"), QObject::tr("Введены существующие номера!"), QMessageBox::Ok);

                return;
            }
        }

    QString vyborId = QString(ui->VyborID->text());

    if (!vyborId.isEmpty())
    {
        if (isVyborID(&vyborId))
            ui->VyborID->setStyleSheet("border: 1px solid grey");
        else
        {
            ui->VyborID->setStyleSheet("border: 1px solid red");

            QMessageBox::critical(this, QObject::tr("Ошибка"), QObject::tr("VyborID не соответствует формату!"), QMessageBox::Ok);

            return;
        }
    }

    query.prepare("UPDATE entry SET entry_type = ?, entry_name = ?, entry_org_name = ?, entry_city = ?, entry_address = ?, "
                  "entry_email = ?, entry_vybor_id = ?, entry_comment = ? WHERE id = ?");
    query.addBindValue("org");
    query.addBindValue(orgName);
    query.addBindValue(orgName);
    query.addBindValue(ui->City->text());
    query.addBindValue(ui->Address->text());
    query.addBindValue(ui->Email->text());
    query.addBindValue(ui->VyborID->text());
    query.addBindValue(ui->Comment->toPlainText());
    query.addBindValue(updateID);
    query.exec();

    for (int i = 0; i < phonesList.length(); ++i)
        if (!phonesList.at(i)->text().isEmpty())
        {
            if(i >= oldPhonesList.length())
            {
                query.prepare("INSERT INTO fones (entry_id, fone)"
                               "VALUES(?, ?)");
                query.addBindValue(updateID);
                query.addBindValue(phonesList.at(i)->text());
                query.exec();
            }
            else
            {
                query.prepare("UPDATE fones SET fone = ? WHERE entry_id = ? AND fone = ?");
                query.addBindValue(phonesList.at(i)->text());
                query.addBindValue(updateID);
                query.addBindValue(oldPhonesList.at(i));
                query.exec();
            }
        }

    emit sendData(true, this->pos().x(), this->pos().y());

    close();

    QMessageBox::information(this, QObject::tr("Уведомление"), QObject::tr("Запись успешно изменена!"), QMessageBox::Ok);
}

bool EditOrgContactDialog::isInternalPhone(QString* str)
{
    int pos = 0;

    QRegularExpressionValidator validator1(QRegularExpression("[0-9]{4}"));
    QRegularExpressionValidator validator2(QRegularExpression("[2][0-9]{2}"));

    if (validator1.validate(*str, pos) == QValidator::Acceptable)
        return true;

    if (validator2.validate(*str, pos) == QValidator::Acceptable)
        return true;

    return false;
}

bool EditOrgContactDialog::isPhone(QString* str)
{
    int pos = 0;

    QRegularExpressionValidator validator(QRegularExpression("[\\+]?[0-9]{1,12}"));

    if (validator.validate(*str, pos) == QValidator::Acceptable)
        return true;

    return false;
}

bool EditOrgContactDialog::isVyborID(QString* str)
{
    int pos = 0;

    QRegularExpressionValidator validator(QRegularExpression("[0-9]*"));

    if (validator.validate(*str, pos) == QValidator::Acceptable)
        return true;

    return false;
}

void EditOrgContactDialog::setOrgValuesContacts(QString i)
{
    updateID = i;

    QSqlDatabase db;
    QSqlQuery query(db);

    query.prepare("SELECT entry_phone FROM entry_phone WHERE entry_id = " + updateID);
    query.exec();

    while (query.next())
        oldPhonesList.append(query.value(0).toString());

    for (int i = 0; i < oldPhonesList.length(); ++i)
        phonesList.at(i)->setText(oldPhonesList.at(i));

    query.prepare("SELECT DISTINCT entry_org_name, entry_city, entry_address, entry_email, entry_vybor_id, "
                  "entry_comment FROM entry WHERE id = " + updateID);
    query.exec();
    query.next();

    ui->OrgName->setText(query.value(0).toString());
    ui->City->setText(query.value(1).toString());
    ui->Address->setText(query.value(2).toString());
    ui->Email->setText(query.value(3).toString());
    ui->VyborID->setText(query.value(4).toString());
    ui->Comment->setText(query.value(5).toString());
}

void EditOrgContactDialog::onTextChanged()
{
    if (ui->Comment->toPlainText().trimmed().length() > 255)
        ui->Comment->textCursor().deletePreviousChar();
}

void EditOrgContactDialog::keyPressEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_Return)
    {
        if (ui->Comment->hasFocus())
            return;
        else
            onSave();
    }
    else
        QDialog::keyPressEvent(event);
}
