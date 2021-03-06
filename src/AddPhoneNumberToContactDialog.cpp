/*
 * Класс служит для добавления номера к существующему контакту.
 */

#include "AddPhoneNumberToContactDialog.h"
#include "ui_AddPhoneNumberToContactDialog.h"

#include "Global.h"

#include <QMessageBox>

AddPhoneNumberToContactDialog::AddPhoneNumberToContactDialog(QWidget* parent) :
    QDialog(parent),
    ui(new Ui::AddPhoneNumberToContactDialog)
{
    ui->setupUi(this);

    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    setWindowFlags(windowFlags() & Qt::WindowMinimizeButtonHint);

    QRegularExpression regExp("^[0-9]*$");
    QValidator* validator = new QRegularExpressionValidator(regExp, this);
    ui->lineEdit_page->setValidator(validator);

    ui->tableView->verticalHeader()->setSectionsClickable(false);
    ui->tableView->horizontalHeader()->setSectionsClickable(false);

    connect(ui->tableView, &QAbstractItemView::doubleClicked, this, &AddPhoneNumberToContactDialog::addPhoneNumber);
    connect(ui->comboBox_list, static_cast<void (QComboBox::*)(qint32)>(&QComboBox::currentIndexChanged), this, &AddPhoneNumberToContactDialog::currentIndexChanged);

    m_page = "1";

    m_go = "default";

    loadContacts();
}

AddPhoneNumberToContactDialog::~AddPhoneNumberToContactDialog()
{
    delete ui;
}

/**
 * Получает неизвестный номер из классов CallHistoryDialog и PopupWindow.
 */
void AddPhoneNumberToContactDialog::setPhoneNumber(const QString& phoneNumber)
{
    m_phoneNumber = phoneNumber;
}

/**
 * Выполняет привязку номера к существующему контакту.
 */
void AddPhoneNumberToContactDialog::addPhoneNumber(const QModelIndex &index)
{
    QSqlQuery query(m_db);

    query.prepare("SELECT EXISTS (SELECT entry_phone FROM entry_phone WHERE entry_phone = ?)");
    query.addBindValue(m_phoneNumber);
    query.exec();
    query.next();

    if (query.value(0) != 0)
    {
        MsgBoxError(tr("Данный номер уже привязан к контакту!"));

        return;
    }

    QString id = m_queryModel->data(m_queryModel->index(index.row(), 0)).toString();

    query.prepare("SELECT COUNT(entry_phone) FROM entry_phone WHERE entry_id = ?");
    query.addBindValue(id);
    query.exec();
    query.first();

    if (query.value(0).toInt() < 5)
    {
        QMessageBox msgBox;
        msgBox.setText(tr("Добавление номера"));
        msgBox.setInformativeText(tr("Вы действительно хотите добавить номер к выбранному контакту?"));
        msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
        msgBox.setButtonText(QMessageBox::Yes, tr("Да"));
        msgBox.setButtonText(QMessageBox::No, tr("Нет"));
        qint32 reply = msgBox.exec();

        switch (reply)
        {
        case QMessageBox::Yes:
            query.prepare("INSERT INTO fones (entry_id, fone)"
                           "VALUES(?, ?)");
            query.addBindValue(id);
            query.addBindValue(m_phoneNumber);
            query.exec();

            emit sendData(true);

            close();

            msgBox.close();

            MsgBoxInformation(tr("Номер успешно добавлен!"));
            break;
        case QMessageBox::No:
            msgBox.close();
            break;
        default:
            break;
        }
    }
    else
        MsgBoxError(tr("Контакту не может быть присвоено больше 5 номеров!"));
}

/**
 * Выполняет вывод и обновление списка контактов,
 * к которым привязано не более 5 номеров, с и без фильтра.
 */
void AddPhoneNumberToContactDialog::loadContacts()
{
    if (!m_queryModel.isNull())
        m_queryModel->deleteLater();

    m_queryModel = new QSqlQueryModel(this);

    QString queryString = "SELECT entry_id, entry_name, GROUP_CONCAT(DISTINCT entry_phone "
                          "ORDER BY entry_id SEPARATOR '\n'), entry_city, entry_comment FROM entry_phone ";

    QString queryCountString = "SELECT COUNT(DISTINCT entry_id) FROM entry_phone ";

    QString searchString;

    if (m_filter == true)
    {
        switch (ui->comboBox->currentIndex())
        {
        case 0:
            searchString = "WHERE entry_name LIKE '%" + ui->lineEdit->text().replace(QRegularExpression("\'"), "\'\'") + "%' ";
            break;
        case 1:
            searchString = "WHERE entry_phone LIKE '%" + ui->lineEdit->text().replace(QRegularExpression("\'"), "\'\'") + "%' ";
            break;
        case 2:
            searchString = "WHERE entry_comment LIKE '%" + ui->lineEdit->text().replace(QRegularExpression("\'"), "\'\'") + "%' ";
            break;
        }
    }

    queryCountString.append(searchString);

    QSqlQuery query(m_db);

    query.prepare(queryCountString);
    query.exec();
    query.first();

    qint32 count = query.value(0).toInt();

    QString pages = ui->label_pages->text();

    if (count <= ui->comboBox_list->currentText().toInt())
        pages = "1";
    else
    {
        qint32 remainder = count % ui->comboBox_list->currentText().toInt();

        if (remainder)
            remainder = 1;
        else
            remainder = 0;

        pages = QString::number(count / ui->comboBox_list->currentText().toInt() + remainder);
    }
    if (m_go == "previous" && m_page != "1")
        m_page = QString::number(m_page.toInt() - 1);
    else if (m_go == "previousStart" && m_page != "1")
        m_page = "1";
    else if (m_go == "next" && m_page.toInt() < pages.toInt())
        m_page = QString::number(m_page.toInt() + 1);
    else if (m_go == "next" && m_page.toInt() >= pages.toInt())
        m_page = pages;
    else if (m_go == "nextEnd" && m_page.toInt() < pages.toInt())
        m_page = pages;
    else if (m_go == "enter" && ui->lineEdit_page->text().toInt() > 0 && ui->lineEdit_page->text().toInt() <= pages.toInt())
        m_page = ui->lineEdit_page->text();
    else if (m_go == "enter" && ui->lineEdit_page->text().toInt() > pages.toInt()) {}
    else if (m_go == "default" && m_page.toInt() >= pages.toInt())
        m_page = pages;

    ui->lineEdit_page->setText(m_page);

    ui->label_pages->setText(tr("из ") + pages);

    queryString.append(searchString);
    queryString.append("GROUP BY entry_id HAVING COUNT(entry_phone) <> 5 ORDER BY entry_name ASC LIMIT ");

    if (ui->lineEdit_page->text() == "1")
        queryString.append("0," + QString::number(ui->lineEdit_page->text().toInt() *
                                                  ui->comboBox_list->currentText().toInt()));
    else
        queryString.append("" + QString::number(ui->lineEdit_page->text().toInt() *
                                                ui->comboBox_list->currentText().toInt() -
                                                ui->comboBox_list->currentText().toInt()) + " , "
                           + QString::number(ui->comboBox_list->currentText().toInt()));

    m_queryModel->setQuery(queryString);

    m_queryModel->setHeaderData(0, Qt::Horizontal, tr("ID"));
    m_queryModel->setHeaderData(1, Qt::Horizontal, tr("ФИО / Название"));
    m_queryModel->setHeaderData(2, Qt::Horizontal, tr("Телефон"));
    m_queryModel->setHeaderData(3, Qt::Horizontal, tr("Город"));
    m_queryModel->setHeaderData(4, Qt::Horizontal, tr("Заметка"));

    ui->tableView->setModel(m_queryModel);

    ui->tableView->horizontalHeader()->setDefaultSectionSize(maximumWidth());

    ui->tableView->resizeRowsToContents();
    ui->tableView->resizeColumnsToContents();

    if (ui->tableView->model()->columnCount() != 0)
    {
        ui->tableView->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
        ui->tableView->horizontalHeader()->setSectionResizeMode(4, QHeaderView::Stretch);
    }
}

/**
 * Выполняет операции для последующего поиска по списку.
 */
void AddPhoneNumberToContactDialog::searchFunction()
{
    m_go = "default";

    if (ui->lineEdit->text().isEmpty())
    {
        if (m_filter)
            ui->tableView->scrollToTop();

        m_filter = false;

        loadContacts();

        return;
    }

    m_filter = true;

    m_page = "1";

    ui->tableView->scrollToTop();

    loadContacts();
}

/**
 * Выполняет обработку события смены количества выводимых контактов на странице.
 */
void AddPhoneNumberToContactDialog::currentIndexChanged()
{
    m_go = "default";

    loadContacts();
}

/**
 * Выполняет операции для последующего перехода на предыдущую страницу.
 */
void AddPhoneNumberToContactDialog::on_previousButton_clicked()
{
    ui->tableView->scrollToTop();

    m_go = "previous";

    loadContacts();
}

/**
 * Выполняет операции для последующего перехода на следующую страницу.
 */
void AddPhoneNumberToContactDialog::on_nextButton_clicked()
{
    ui->tableView->scrollToTop();

    m_go = "next";

    loadContacts();
}

/**
 * Выполняет операции для последующего перехода на первую страницу.
 */
void AddPhoneNumberToContactDialog::on_previousStartButton_clicked()
{
    ui->tableView->scrollToTop();

    m_go = "previousStart";

    loadContacts();
}

/**
 * Выполняет операции для последующего перехода на последнюю страницу.
 */
void AddPhoneNumberToContactDialog::on_nextEndButton_clicked()
{  
    ui->tableView->scrollToTop();

    m_go = "nextEnd";

    loadContacts();
}

/**
 * Выполняет операции для последующего перехода на заданную страницу.
 */
void AddPhoneNumberToContactDialog::on_lineEdit_page_returnPressed()
{
    ui->tableView->scrollToTop();

    m_go = "enter";

    loadContacts();
}

/**
 * Выполняет поиск по списку при нажатии клавиши Enter,
 * находясь в поле поиска.
 */
void AddPhoneNumberToContactDialog::on_lineEdit_returnPressed()
{
    searchFunction();
}

/**
 * Выполняет поиск по списку при нажатии кнопки поиска.
 */
void AddPhoneNumberToContactDialog::on_searchButton_clicked()
{
    searchFunction();
}
