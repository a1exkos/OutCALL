#include "EditOrgContactDialog.h"
#include "ui_EditOrgContactDialog.h"
#include "EditContactDialog.h"
#include "ViewContactDialog.h"

#include <QVariantList>
#include <QVariantMap>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlQueryModel>
#include <QTableView>
#include <QPlainTextEdit>
#include <QString>
#include <QMessageBox>
#include <QClipboard>
#include <QScrollBar>
#include <QStringList>

EditOrgContactDialog::EditOrgContactDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::EditOrgContactDialog)
{
    ui->setupUi(this);

    QRegExp RegExp("^[\\+]?[0-9]{1,12}$");
    validator = new QRegExpValidator(RegExp, this);
    ui->FirstNumber->setValidator(validator);
    ui->SecondNumber->setValidator(validator);
    ui->ThirdNumber->setValidator(validator);
    ui->FourthNumber->setValidator(validator);
    ui->FifthNumber->setValidator(validator);

    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    ui->label_6->setText("1<span style=\"color: red;\">*</span>");
    ui->label_3->setText("Название организации:<span style=\"color: red;\">*</span>");

    connect(ui->saveButton, &QAbstractButton::clicked, this, &EditOrgContactDialog::onSave);
    connect(ui->tableView, SIGNAL(clicked(const QModelIndex &)), this, SLOT(onTableClicked(const QModelIndex &)));
    connect(ui->tableView, SIGNAL(doubleClicked(const QModelIndex &)), this, SLOT(showCard(const QModelIndex &)));

    onComboBoxSelected();
}

EditOrgContactDialog::~EditOrgContactDialog()
{
    delete validator;
    delete query_model;
    delete ui;
}

void EditOrgContactDialog::onSave()
{
    QSqlDatabase db;
    QSqlQuery query(db);
    QString orgName = QString(ui->OrgName->text());

    query.prepare("UPDATE entry SET entry_type = ?, entry_name = ?, entry_org_name = ?, entry_city = ?, entry_address = ?, entry_email = ?, entry_vybor_id = ?, entry_comment = ? WHERE id = ?");
    query.addBindValue("org");
    query.addBindValue(orgName);
    query.addBindValue(orgName);
    query.addBindValue(ui->City->text());
    query.addBindValue(ui->Address->text());
    query.addBindValue(ui->Email->text());
    query.addBindValue(ui->VyborID->text());
    query.addBindValue(ui->Comment->toPlainText());
    query.addBindValue(updateID);

    if (QString(ui->OrgName->text()).isEmpty() == true)
    {
         ui->label_15->setText("<span style=\"color: red;\">Заполните обязательное поле!</span>");
         ui->OrgName->setStyleSheet("border: 1px solid red");
    }
    else { ui->label_15->hide();  ui->OrgName->setStyleSheet("border: 1px solid grey"); }
    if (QString(ui->FirstNumber->text()).isEmpty() == true)
    {
        ui->label_14->setText("<span style=\"color: red;\">Заполните обязательное поле!</span>");
        ui->FirstNumber->setStyleSheet("border: 1px solid red");
    }
    else { ui->label_14->hide(); ui->FirstNumber->setStyleSheet("border: 1px solid grey"); }
    if (QString(ui->OrgName->text()).isEmpty() == false && QString(ui->FirstNumber->text()).isEmpty() == false)
    {
        ui->label_15->hide();
        ui->OrgName->setStyleSheet("border: 1px solid grey");
        ui->label_14->hide();
        ui->FirstNumber->setStyleSheet("border: 1px solid grey");

        numbers.clear();
        QSqlQuery query1(db);
        query1.prepare("SELECT EXISTS (SELECT entry_phone FROM entry_phone WHERE entry_phone = '" + ui->FirstNumber->text() + "' AND NOT entry_id = " + updateID + ")");
        query1.exec();
        query1.next();
        if (query1.value(0) != 0)
        {
            ui->FirstNumber->setStyleSheet("border: 1px solid red");
            numbers << QString(ui->FirstNumber->text());
        }
        query1.prepare("SELECT EXISTS (SELECT entry_phone FROM entry_phone WHERE entry_phone = '" + ui->SecondNumber->text() + "' AND NOT entry_id = " + updateID + ")");
        query1.exec();
        query1.next();
        if (query1.value(0) != 0)
        {
            ui->SecondNumber->setStyleSheet("border: 1px solid red");
            numbers << QString(ui->SecondNumber->text());
        }
        query1.prepare("SELECT EXISTS (SELECT entry_phone FROM entry_phone WHERE entry_phone = '" + ui->ThirdNumber->text() + "' AND NOT entry_id = " + updateID + ")");
        query1.exec();
        query1.next();
        if (query1.value(0) != 0)
        {
            ui->ThirdNumber->setStyleSheet("border: 1px solid red");
            numbers << QString(ui->ThirdNumber->text());
        }
        query1.prepare("SELECT EXISTS (SELECT entry_phone FROM entry_phone WHERE entry_phone = '" + ui->FourthNumber->text() + "' AND NOT entry_id = " + updateID + ")");
        query1.exec();
        query1.next();
        if (query1.value(0) != 0)
        {
            ui->FourthNumber->setStyleSheet("border: 1px solid red");
            numbers << QString(ui->FourthNumber->text());
        }
        query1.prepare("SELECT EXISTS (SELECT entry_phone FROM entry_phone WHERE entry_phone = '" + ui->FifthNumber->text() + "' AND NOT entry_id = " + updateID + ")");
        query1.exec();
        query1.next();
        if (query1.value(0) != 0)
        {
            ui->FifthNumber->setStyleSheet("border: 1px solid red");
            numbers << QString(ui->FifthNumber->text());
        }

        if (!numbers.isEmpty())
        {
            QString str = numbers.join(", ");
            QMessageBox::critical(this, trUtf8("Ошибка"), trUtf8("Введены существующие номера!\n%1").arg(str), QMessageBox::Ok);
        }
        else
        {
            query.exec();
            QString firstNum = QString(ui->FirstNumber->text());
            QString secondNum = QString(ui->SecondNumber->text());
            QString thirdNum = QString(ui->ThirdNumber->text());
            QString fourthNum = QString(ui->FourthNumber->text());
            QString fifthNum = QString(ui->FifthNumber->text());
            QString sql1 = QString("select COUNT(phone) from phone where entry_id = %1").arg(updateID);
            query1.prepare(sql1);
            query1.exec();
            query1.next();
            int count = query1.value(0).toInt();

            if (firstNum != 0)
            {
                if (count > 0)
                {
                    query1.prepare("UPDATE phone SET phone = ? WHERE entry_id = ? AND phone = ?");
                    query1.addBindValue(firstNum);
                    query1.addBindValue(updateID);
                    query1.addBindValue(firstNumber);
                    query1.exec();
                    count--;
                }
                else
                {
                    query1.prepare("INSERT INTO phone (entry_id, phone) VALUES(?, ?)");
                    query1.addBindValue(updateID);
                    query1.addBindValue(firstNum);
                    query1.exec();
                }

            }
            if (secondNum != 0)
            {
                if (count > 0)
                {
                    query1.prepare("UPDATE phone SET phone = ? WHERE entry_id = ? AND phone = ?");
                    query1.addBindValue(secondNum);
                    query1.addBindValue(updateID);
                    query1.addBindValue(secondNumber);
                    query1.exec();
                    count--;
                }
                else
                {
                    query1.prepare("INSERT INTO phone (entry_id, phone) VALUES(?, ?)");
                    query1.addBindValue(updateID);
                    query1.addBindValue(secondNum);
                    query1.exec();
                }
            }
            if (thirdNum != 0)
            {
                if (count > 0)
                {
                    query1.prepare("UPDATE phone SET phone = ? WHERE entry_id = ? AND phone = ?");
                    query1.addBindValue(thirdNum);
                    query1.addBindValue(updateID);
                    query1.addBindValue(thirdNumber);
                    query1.exec();
                    count--;
                }
                else
                {
                    query1.prepare("INSERT INTO phone (entry_id, phone) VALUES(?, ?)");
                    query1.addBindValue(updateID);
                    query1.addBindValue(thirdNum);
                    query1.exec();
                }
            }
            if (fourthNum != 0)
            {
                if (count > 0)
                {
                    query1.prepare("UPDATE phone SET phone = ? WHERE entry_id = ? AND phone = ?");
                    query1.addBindValue(fourthNum);
                    query1.addBindValue(updateID);
                    query1.addBindValue(fourthNumber);
                    query1.exec();
                    count--;
                }
                else
                {
                    query1.prepare("INSERT INTO phone (entry_id, phone) VALUES(?, ?)");
                    query1.addBindValue(updateID);
                    query1.addBindValue(fourthNum);
                    query1.exec();
                }
            }
            if (fifthNum != 0)
            {
                if (count > 0)
                {
                    query1.prepare("UPDATE phone SET phone = ? WHERE entry_id = ? AND phone = ?");
                    query1.addBindValue(fifthNum);
                    query1.addBindValue(updateID);
                    query1.addBindValue(fifthNumber);
                    query1.exec();
                    count--;
                }
                else
                {
                    query1.prepare("INSERT INTO phone (entry_id, phone) VALUES(?, ?)");
                    query1.addBindValue(updateID);
                    query1.addBindValue(fifthNum);
                    query1.exec();
                }
            }
            ui->label_16->setText("<span style=\"color: green;\">Запись успешно добавлена!</span>");
            emit sendData(true);
        }
    }
}

void EditOrgContactDialog::onTableClicked(const QModelIndex &index)
{
    if (index.isValid()) {
        QString cellText = index.data().toString();
        QClipboard *clipboard = QApplication::clipboard();
        clipboard->setText(cellText);
    }
}

void EditOrgContactDialog::showCard(const QModelIndex &index)
{
    QString id = query_model->data(query_model->index(index.row(), 0)).toString();
    viewContactDialog = new ViewContactDialog;
    viewContactDialog->setValuesContacts(id);
    viewContactDialog->exec();
    viewContactDialog->deleteLater();
}

void EditOrgContactDialog::deleteObjects()
{
    for (int i = 0; i < widgets.size(); ++i)
    {
        widgets[i]->deleteLater();
    }
    qDeleteAll(layouts);
    qDeleteAll(buttons);
    widgets.clear();
    layouts.clear();
    buttons.clear();
}

void EditOrgContactDialog::onUpdate()
{
    if (update == "default")
    {
        query_model->setQuery("SELECT ep.entry_id, ep.entry_name, GROUP_CONCAT(DISTINCT ep.entry_phone ORDER BY ep.entry_id SEPARATOR '\n'), ep.entry_comment FROM entry_phone ep WHERE ep.entry_type = 'person' AND ep.entry_person_org_id = '" + updateID + "' GROUP BY ep.entry_id");
    }

    verticalScroll = ui->tableView->verticalScrollBar();
    horizontalScroll = ui->tableView->horizontalScrollBar();
    int valueV = verticalScroll->value();
    int valueH = horizontalScroll->value();

    query_model->setHeaderData(0, Qt::Horizontal, QObject::tr("ID"));
    query_model->setHeaderData(1, Qt::Horizontal, QObject::tr("ФИО"));
    query_model->setHeaderData(2, Qt::Horizontal, QObject::tr("Телефон"));
    query_model->setHeaderData(3, Qt::Horizontal, QObject::tr("Заметка"));
    query_model->insertColumn(4);
    query_model->setHeaderData(4, Qt::Horizontal, tr("Редактирование"));
    ui->tableView->setModel(query_model);

    ui->tableView->setSelectionBehavior(QAbstractItemView::SelectRows);

    deleteObjects();

    for (int row_index = 0; row_index < ui->tableView->model()->rowCount(); ++row_index)
    {
        ui->tableView->setIndexWidget(query_model->index(row_index, 4), createEditButton(row_index));
    }

    ui->tableView->horizontalHeader()->setDefaultSectionSize(maximumWidth());
    ui->tableView->resizeRowsToContents();
    ui->tableView->resizeColumnsToContents();
    ui->tableView->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    ui->tableView->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Stretch);

    if (update == "default")
    {
        ui->tableView->verticalScrollBar()->setSliderPosition(valueV);
        ui->tableView->horizontalScrollBar()->setSliderPosition(valueH);
    }

    update = "default";
}

void EditOrgContactDialog::onEdit()
{
    QString id = sender()->property("id").toString();
    editContactDialog = new EditContactDialog;
    editContactDialog->setWindowTitle("Редактирование физ. лица");
    editContactDialog->setValuesContacts(id);
    editContactDialog->exec();
    editContactDialog->deleteLater();
    onUpdate();
}

QWidget* EditOrgContactDialog::createEditButton(int &row_index)
{
    QWidget* wgt = new QWidget;
    QBoxLayout* l = new QHBoxLayout;
    QPushButton* editButton = new QPushButton("Редактировать");
    connect(editButton, SIGNAL(clicked(bool)), SLOT(onEdit()));
    editButton->setFocusPolicy(Qt::NoFocus);
    QString id = query_model->data(query_model->index(row_index, 0)).toString();
    editButton->setProperty("id", id);
    l->addWidget(editButton);
    wgt->setLayout(l);
    widgets.append(wgt);
    layouts.append(l);
    buttons.append(editButton);
    return wgt;
}

void EditOrgContactDialog::onComboBoxSelected()
{
    ui->comboBox->addItem("Поиск по ФИО");
    ui->comboBox->addItem("Поиск по номеру телефона");
    ui->comboBox->addItem("Поиск по заметке");
}

void EditOrgContactDialog::on_lineEdit_returnPressed()
{
    update = "filter";
    QComboBox::AdjustToContents;

    if (ui->comboBox->currentText() == "Поиск по ФИО")
    {
        QString entry_name = ui->lineEdit->text();
        query_model->setQuery("SELECT ep.entry_id, ep.entry_name, GROUP_CONCAT(DISTINCT ep.entry_phone ORDER BY ep.entry_id SEPARATOR '\n'), ep.entry_comment FROM entry_phone ep WHERE ep.entry_type = 'person' AND ep.entry_person_org_id = '" + updateID + "' AND ep.entry_name LIKE '%" + entry_name + "%' GROUP BY ep.entry_id");

        onUpdate();
    }

    if (ui->comboBox->currentText() == "Поиск по номеру телефона")
    {
        QString entry_phone = ui->lineEdit->text();
        query_model->setQuery("SELECT ep.entry_id, ep.entry_name, GROUP_CONCAT(DISTINCT ep.entry_phone ORDER BY ep.entry_id SEPARATOR '\n'), ep.entry_comment FROM entry_phone ep WHERE ep.entry_type = 'person' AND ep.entry_person_org_id = '" + updateID + "' AND ep.entry_phone LIKE '%" + entry_phone + "%' GROUP BY ep.entry_id");

        onUpdate();
    }

    if (ui->comboBox->currentText() == "Поиск по заметке")
    {
        QString entry_comment = ui->lineEdit->text();
        query_model->setQuery("SELECT ep.entry_id, ep.entry_name, GROUP_CONCAT(DISTINCT ep.entry_phone ORDER BY ep.entry_id SEPARATOR '\n'), ep.entry_comment FROM entry_phone ep WHERE ep.entry_type = 'person' AND ep.entry_person_org_id = '" + updateID + "' AND ep.entry_comment LIKE '%" + entry_comment + "%' GROUP BY ep.entry_id");

        onUpdate();
    }
}

void EditOrgContactDialog::setOrgValuesContacts(QString &i)
{
    updateID = i;
    QSqlDatabase db;
    QSqlQuery query(db);
    QString sql = QString("select entry_phone from entry_phone where entry_id = %1").arg(updateID);
    query.prepare(sql);
    query.exec();
    query.next();
    firstNumber = query.value(0).toString();
    query.next();
    secondNumber = query.value(0).toString();
    query.next();
    thirdNumber = query.value(0).toString();
    query.next();
    fourthNumber = query.value(0).toString();
    query.next();
    fifthNumber = query.value(0).toString();
    sql = QString("select distinct entry_org_name, entry_city, entry_address, entry_email, entry_vybor_id, entry_comment from entry where id = %1").arg(updateID);
    query.prepare(sql);
    query.exec();
    query.next();
    QString entryORGName = query.value(0).toString();
    QString entryCity = query.value(1).toString();
    QString entryAddress = query.value(2).toString();
    QString entryEmail = query.value(3).toString();
    QString entryVyborID = query.value(4).toString();
    QString entryComment = query.value(5).toString();
    ui->FirstNumber->setText(firstNumber);
    ui->SecondNumber->setText(secondNumber);
    ui->ThirdNumber->setText(thirdNumber);
    ui->FourthNumber->setText(fourthNumber);
    ui->FifthNumber->setText(fifthNumber);
    ui->OrgName->setText(entryORGName);
    ui->City->setText(entryCity);
    ui->Address->setText(entryAddress);
    ui->Email->setText(entryEmail);
    ui->VyborID->setText(entryVyborID);
    ui->Comment->setText(entryComment);

    query_model = new QSqlQueryModel;

    query_model->setQuery("SELECT ep.entry_id, ep.entry_name, GROUP_CONCAT(DISTINCT ep.entry_phone ORDER BY ep.entry_id SEPARATOR '\n'), ep.entry_comment FROM entry_phone ep WHERE ep.entry_type = 'person' AND ep.entry_person_org_id = '" + updateID + "' GROUP BY ep.entry_id");
    query_model->setHeaderData(0, Qt::Horizontal, QObject::tr("ID"));
    query_model->setHeaderData(1, Qt::Horizontal, QObject::tr("ФИО"));
    query_model->setHeaderData(2, Qt::Horizontal, QObject::tr("Телефон"));
    query_model->setHeaderData(3, Qt::Horizontal, QObject::tr("Заметка"));
    query_model->insertColumn(4);
    query_model->setHeaderData(4, Qt::Horizontal, tr("Редактирование"));
    ui->tableView->setModel(query_model);

    ui->tableView->setSelectionBehavior(QAbstractItemView::SelectRows);

    deleteObjects();

    for (int row_index = 0; row_index < ui->tableView->model()->rowCount(); ++row_index)
    {
        ui->tableView->setIndexWidget(query_model->index(row_index, 4), createEditButton(row_index));
    }

    ui->tableView->horizontalHeader()->setDefaultSectionSize(maximumWidth());
    ui->tableView->resizeRowsToContents();
    ui->tableView->resizeColumnsToContents();
    ui->tableView->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    ui->tableView->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Stretch);

    update = "default";
}

void EditOrgContactDialog::setOrgValuesCallHistory(QString &number)
{
    ui->FirstNumber->setText(number);
}
