/*
 * Класс служит для создания напоминания.
 */

#include "AddReminderDialog.h"
#include "ui_AddReminderDialog.h"

#include <QDebug>

AddReminderDialog::AddReminderDialog(QWidget* parent) :
    QDialog(parent),
    ui(new Ui::AddReminderDialog)
{
    ui->setupUi(this);

    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    setWindowFlags(windowFlags() & Qt::WindowMinimizeButtonHint);

    my_number = global::getSettingsValue(global::getExtensionNumber("extensions"), "extensions_name").toString();

    employee.append(my_number);

    ui->employee->setText(my_number);

    QString language = global::getSettingsValue("language", "settings").toString();

    if (language == "Русский")
        ui->calendarWidget->setLocale(QLocale::Russian);
    else if (language == "Українська")
        ui->calendarWidget->setLocale(QLocale::Ukrainian);
    else if (language == "English")
        ui->calendarWidget->setLocale(QLocale::English);

    ui->calendarWidget->setGridVisible(true);
    ui->calendarWidget->setMinimumDate(QDate::currentDate());
    ui->calendarWidget->setSelectedDate(QDate::currentDate());

    ui->timeEdit->setTime(QTime::currentTime());

    connect(ui->textEdit, &QTextEdit::textChanged, this, &AddReminderDialog::onTextChanged);
    connect(ui->saveButton, &QAbstractButton::clicked, this, &AddReminderDialog::onSave);
    connect(ui->chooseEmployeeButton, &QAbstractButton::clicked, this, &AddReminderDialog::onChooseEmployee);
}

AddReminderDialog::~AddReminderDialog()
{
    delete ui;
}

/**
 * Получает список выбранных сотрудников из классов
 * ChooseEmployee и InternalContactsDialog.
 */
void AddReminderDialog::receiveEmployee(const QStringList& employee)
{
    this->employee = employee;

    if (this->employee.length() == 1)
        ui->employee->setText(this->employee.first());
    else
        ui->employee->setText(tr("Группа") + " (" + QString::number(this->employee.length()) + ")");
}

/**
 * Выполняет открытие окна для выбора списка сотрудников,
 * которые получат напоминание.
 */
void AddReminderDialog::onChooseEmployee()
{
    if (!chooseEmployee.isNull())
        chooseEmployee->close();

    chooseEmployee = new ChooseEmployee;
    chooseEmployee->setValues(employee);
    connect(chooseEmployee, &ChooseEmployee::sendEmployee, this, &AddReminderDialog::receiveEmployee);
    connect(this, &AddReminderDialog::getPos, chooseEmployee, &ChooseEmployee::setPos);
    emit getPos(this->pos().x(), this->pos().y());
    chooseEmployee->show();
    chooseEmployee->setAttribute(Qt::WA_DeleteOnClose);
}

/**
 * Выполняет проверку введенных данных и их последующее сохранение в БД.
 */
void AddReminderDialog::onSave()
{
    QDate date = ui->calendarWidget->selectedDate();
    QTime time(ui->timeEdit->time().hour(), ui->timeEdit->time().minute(), 0);
    QDateTime dateTime = QDateTime(date, time);
    QString note = ui->textEdit->toPlainText().trimmed();

    if (dateTime < QDateTime::currentDateTime())
    {
        QMessageBox::critical(this, tr("Ошибка"), tr("Указано прошедшее время!"), QMessageBox::Ok);

        return;
    }

    if (note.isEmpty())
    {
        QMessageBox::critical(this, tr("Ошибка"), tr("Содержание напоминания не может быть пустым!"), QMessageBox::Ok);

        return;
    }

    QSqlQuery query(db);

    if (callId.isEmpty())
    {
        if (ui->employee->text() != my_number)
        {
            query.prepare("INSERT INTO reminders (phone_from, phone_to, datetime, content, viewed, completed, active) VALUES(?, ?, ?, ?, false, false, true)");
            query.addBindValue(my_number);
            query.addBindValue(employee.first());
            query.addBindValue(dateTime);
            query.addBindValue(note);
            query.exec();

            if (employee.length() > 1)
            {
                qint32 id = query.lastInsertId().toInt();

                query.prepare("UPDATE reminders SET group_id = ? WHERE id = ?");
                query.addBindValue(id);
                query.addBindValue(id);
                query.exec();

                for (qint32 i = 1; i < employee.length(); ++i)
                {
                    query.prepare("INSERT INTO reminders (group_id, phone_from, phone_to, datetime, content, viewed, completed, active) VALUES(?, ?, ?, ?, ?, false, false, true)");
                    query.addBindValue(id);
                    query.addBindValue(my_number);
                    query.addBindValue(employee.at(i));
                    query.addBindValue(dateTime);
                    query.addBindValue(note);
                    query.exec();
                }
            }
        }
        else
        {
            query.prepare("INSERT INTO reminders (phone_from, phone_to, datetime, content, active) VALUES(?, ?, ?, ?, true)");
            query.addBindValue(my_number);
            query.addBindValue(ui->employee->text());
            query.addBindValue(dateTime);
            query.addBindValue(note);
            query.exec();
        }
    }
    else
    {
        if (ui->employee->text() != my_number)
        {
            query.prepare("INSERT INTO reminders (phone_from, phone_to, datetime, content, call_id, viewed, completed, active) VALUES(?, ?, ?, ?, ?, false, false, true)");
            query.addBindValue(my_number);
            query.addBindValue(employee.first());
            query.addBindValue(dateTime);
            query.addBindValue(note);
            query.addBindValue(callId);
            query.exec();

            if (employee.length() > 1)
            {
                qint32 id = query.lastInsertId().toInt();

                query.prepare("UPDATE reminders SET group_id = ? WHERE id = ?");
                query.addBindValue(id);
                query.addBindValue(id);
                query.exec();

                for (qint32 i = 1; i < employee.length(); ++i)
                {
                    query.prepare("INSERT INTO reminders (group_id, phone_from, phone_to, datetime, content, call_id, viewed, completed, active) VALUES(?, ?, ?, ?, ?, ?, false, false, true)");
                    query.addBindValue(id);
                    query.addBindValue(my_number);
                    query.addBindValue(employee.at(i));
                    query.addBindValue(dateTime);
                    query.addBindValue(note);
                    query.addBindValue(callId);
                    query.exec();
                }
            }
        }
        else
        {
            query.prepare("INSERT INTO reminders (phone_from, phone_to, datetime, content, call_id, active) VALUES(?, ?, ?, ?, ?, true)");
            query.addBindValue(my_number);
            query.addBindValue(ui->employee->text());
            query.addBindValue(dateTime);
            query.addBindValue(note);
            query.addBindValue(callId);
            query.exec();
        }
    }

    if (!chooseEmployee.isNull())
        chooseEmployee->close();

    emit sendData(true);

    close();

    if (ui->employee->text() != my_number)
        QMessageBox::information(this, tr("Уведомление"), tr("Напоминание успешно отправлено!"), QMessageBox::Ok);
    else
        QMessageBox::information(this, tr("Уведомление"), tr("Напоминание успешно добавлено!"), QMessageBox::Ok);
}

/**
 * Получает уникальный id звонка из класса PopupWindow
 * или id контакта из классов ViewContactDialog и ViewOrgContactDialog.
 */
void AddReminderDialog::setCallId(const QString& callId)
{
    this->callId = callId;
}

/**
 * Выполняет удаление последнего символа в тексте,
 * если его длина превышает 255 символов.
 */
void AddReminderDialog::onTextChanged()
{
    int m_maxDescriptionLength = 255;

    if (ui->textEdit->toPlainText().length() > m_maxDescriptionLength)
    {
        int diff = ui->textEdit->toPlainText().length() - m_maxDescriptionLength;

        QString newStr = ui->textEdit->toPlainText();
        newStr.chop(diff);

        ui->textEdit->setText(newStr);

        QTextCursor cursor(ui->textEdit->textCursor());
        cursor.movePosition(QTextCursor::End, QTextCursor::MoveAnchor);

        ui->textEdit->setTextCursor(cursor);
    }
}

/**
 * Выполняет обработку нажатий клавиш.
 * Особая обработка для клавиш Esc и Enter.
 */
void AddReminderDialog::keyPressEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_Escape)
        QDialog::close();
    else if (event->key() == Qt::Key_Return)
    {
        if (ui->textEdit->hasFocus())
            return;
        else
            onSave();
    }
    else
        QDialog::keyPressEvent(event);
}

/**
 * Выполняет обработку закрытия окна.
 */
void AddReminderDialog::closeEvent(QCloseEvent* event)
{
    QDialog::closeEvent(event);

    if (!chooseEmployee.isNull())
        chooseEmployee->close();
}

/**
 * Выполняет прибавление к текущему времени 5 минут.
 */
void AddReminderDialog::on_add5MinButton_clicked()
{
    ui->timeEdit->setTime(ui->timeEdit->time().addSecs(300));
}

/**
 * Выполняет прибавление к текущему времени 10 минут.
 */
void AddReminderDialog::on_add10MinButton_clicked()
{
    ui->timeEdit->setTime(ui->timeEdit->time().addSecs(600));
}

/**
 * Выполняет прибавление к текущему времени 30 минут.
 */
void AddReminderDialog::on_add30MinButton_clicked()
{
    ui->timeEdit->setTime(ui->timeEdit->time().addSecs(1800));
}

/**
 * Выполняет прибавление к текущему времени 60 минут.
 */
void AddReminderDialog::on_add60MinButton_clicked()
{
    ui->timeEdit->setTime(ui->timeEdit->time().addSecs(3600));
}
