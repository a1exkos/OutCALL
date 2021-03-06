/*
 * Класс служит для просмотра и взаимодействия с данными физ. лица.
 */

#include "ViewContactDialog.h"
#include "ui_ViewContactDialog.h"

#include "AsteriskManager.h"
#include "Global.h"

#include <QMessageBox>
#include <QDesktopWidget>

ViewContactDialog::ViewContactDialog(QWidget* parent) :
    QDialog(parent),
    ui(new Ui::ViewContactDialog)
{
    ui->setupUi(this);

    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    setWindowFlags(windowFlags() & Qt::WindowMinimizeButtonHint);

    ui->tableView->verticalHeader()->setSectionsClickable(false);
    ui->tableView->horizontalHeader()->setSectionsClickable(false);

    connect(ui->callsTabWidget, &QTabWidget::currentChanged,    this, &ViewContactDialog::tabSelected);
    connect(ui->comboBox_days,  &QComboBox::currentTextChanged, this, &ViewContactDialog::onUpdate);

    connect(ui->playAudio,         &QAbstractButton::clicked, this, &ViewContactDialog::onPlayAudio);
    connect(ui->editButton,        &QAbstractButton::clicked, this, &ViewContactDialog::onEdit);
    connect(ui->callButton,        &QAbstractButton::clicked, this, &ViewContactDialog::onCall);
    connect(ui->playAudioPhone,    &QAbstractButton::clicked, this, &ViewContactDialog::onPlayAudioPhone);
    connect(ui->openAccessButton,  &QAbstractButton::clicked, this, &ViewContactDialog::onOpenAccess);
    connect(ui->addReminderButton, &QAbstractButton::clicked, this, &ViewContactDialog::onAddReminder);

    connect(ui->tableView, &QAbstractItemView::clicked,       this, &ViewContactDialog::getData);
    connect(ui->tableView, &QAbstractItemView::doubleClicked, this, &ViewContactDialog::viewNotes);

    if (!g_ordersDbOpened)
        ui->openAccessButton->hide();

    ui->comboBox_list->setVisible(false);

    ui->playAudio->setDisabled(true);
    ui->playAudioPhone->setDisabled(true);

    m_phones = { ui->firstNumber, ui->secondNumber, ui->thirdNumber, ui->fourthNumber, ui->fifthNumber };
    m_phonesComments = { ui->firstNumberComment, ui->secondNumberComment, ui->thirdNumberComment, ui->fourthNumberComment, ui->fifthNumberComment };

    QSqlQuery query(m_db);

    query.prepare(QueryStringGetGroups());
    query.exec();

    while(query.next())
    {
        QLineEdit* line = new QLineEdit(this);
        line->setReadOnly(true);
        line->setStyleSheet("*{ background-color: #fffff8;}");

        QLabel* label = new QLabel(this);

        m_managers.insert(query.value(0).toString(), line);
        label->setText(query.value(1).toString() + " (" + query.value(0).toString() + "):");

        ui->gridLayout->addWidget(label);
        ui->gridLayout->addWidget(line);
    }
}

ViewContactDialog::~ViewContactDialog()
{
    delete ui;
}

/**
 * Выполняет обработку появления окна.
 */
void ViewContactDialog::showEvent(QShowEvent* event)
{
    QDialog::showEvent(event);

    QSqlQuery query(m_db);

    query.prepare("SELECT DISTINCT entry_org_name FROM entry WHERE id = "
                  "(SELECT DISTINCT entry_person_org_id FROM entry WHERE id = " + m_contactId + ")");
    query.exec();

    if (query.first())
    {
        ui->organization->setText(query.value(0).toString());
        ui->organization->QWidget::setToolTip(query.value(0).toString());
        ui->organization->setCursorPosition(0);
    }
}

/**
 * Выполняет открытие окна добавления напоминания.
 */
void ViewContactDialog::onAddReminder()
{
    if (!m_addReminderDialog.isNull())
        m_addReminderDialog->close();

    m_addReminderDialog = new AddReminderDialog;
    m_addReminderDialog->setCallId(m_contactId);
    m_addReminderDialog->show();
    m_addReminderDialog->setAttribute(Qt::WA_DeleteOnClose);
}

/**
 * Выполняет открытие базы заказов.
 */
void ViewContactDialog::onOpenAccess()
{
    QString hostName_3 = global::getSettingsValue("hostName_3", "settings").toString();
    QString databaseName_3 = global::getSettingsValue("databaseName_3", "settings").toString();
    QString userName_3 = global::getSettingsValue("userName_3", "settings").toString();
    QByteArray password3 = global::getSettingsValue("password_3", "settings").toByteArray();
    QString password_3 = QString(QByteArray::fromBase64(password3));
    QString port_3 = global::getSettingsValue("port_3", "settings").toString();

    QSqlDatabase dbOrders = QSqlDatabase::addDatabase("QODBC", "Orders");

    dbOrders.setDatabaseName("DRIVER={SQL Server Native Client 10.0};"
                            "Server="+hostName_3+","+port_3+";"
                            "Database="+databaseName_3+";"
                            "Uid="+userName_3+";"
                            "Pwd="+password_3);
    dbOrders.open();

    if (dbOrders.isOpen())
    {
        QString userId = global::getSettingsValue("user_login", "settings").toString();

        QSqlQuery query(dbOrders);

        query.prepare("INSERT INTO CallTable (UserID, ClientID)"
                    "VALUES (?, ?)");
        query.addBindValue(userId);
        query.addBindValue(ui->vyborId->text().toInt());
        query.exec();

        ui->openAccessButton->setDisabled(true);

        dbOrders.close();
    }
    else
        MsgBoxError(tr("Отсутствует подключение к базе заказов!"));
}

/**
 * Выполняет операции для последующего выбора номера контакта и совершения звонка.
 */
void ViewContactDialog::onCall()
{
    if (m_numbers.count() == 1)
    {
        QString number = m_numbers.at(0);
        QString protocol = global::getSettingsValue(g_personalNumber, "extensions").toString();

        g_asteriskManager->originateCall(g_personalNumber, number, protocol, g_personalNumber);
    }
    else
    {
        if (!m_chooseNumber.isNull())
            m_chooseNumber->close();

        QString id = m_contactId;
        m_chooseNumber = new ChooseNumber;
        m_chooseNumber->setValues(id, qint32(0));
        m_chooseNumber->show();
        m_chooseNumber->setAttribute(Qt::WA_DeleteOnClose);
    }
}

/**
 * Выполняет скрытие текущего окна и открытие окна редактирования.
 */
void ViewContactDialog::onEdit()
{
    hide();

    m_editContactDialog = new EditContactDialog;
    m_editContactDialog->setValues(m_contactId);
    connect(m_editContactDialog, &EditContactDialog::sendData, this, &ViewContactDialog::receiveData);
    connect(this, &ViewContactDialog::getPos, m_editContactDialog, &EditContactDialog::setPos);
    emit getPos(this->pos().x(), this->pos().y());
    m_editContactDialog->show();
    m_editContactDialog->setAttribute(Qt::WA_DeleteOnClose);
}

/**
 * Получает id контакта из классов ContactsDialog,
 * ViewOrgContactDialog, PopupWindow.
 */
void ViewContactDialog::setValues(const QString& id)
{
    m_contactId = id;

    QSqlQuery query(m_db);

    query.prepare("SELECT fone, comment FROM fones WHERE entry_id = " + m_contactId + " ORDER BY priority");
    query.exec();

    while (query.next())
    {
         m_numbers.append(query.value(0).toString());
         m_comments.append(query.value(1).toString());
    }

    for (qint32 i = 0; i < m_numbers.length(); ++i)
    {
        m_phones.at(i)->setText(m_numbers.at(i));
        m_phonesComments.at(i)->setText(m_comments.at(i));
    }

    query.prepare("SELECT group_number, manager_number FROM managers WHERE entry_id = " + m_contactId);
    query.exec();

    while (query.next())
        if (m_managers.keys().contains(query.value(0).toString()))
            m_managers.value(query.value(0).toString())->setText(query.value(1).toString());

    query.prepare("SELECT DISTINCT entry_person_fname, entry_person_mname, entry_person_lname, entry_region, entry_city, "
                  "entry_address, entry_email, entry_vybor_id, entry_comment FROM entry WHERE id = " + m_contactId);
    query.exec();
    query.next();

    ui->firstName->setText(query.value(0).toString());
    ui->patronymic->setText(query.value(1).toString());
    ui->lastName->setText(query.value(2).toString());
    ui->region->setText(query.value(3).toString());

    ui->city->setText(query.value(4).toString());
    ui->city->QWidget::setToolTip(query.value(4).toString());
    ui->city->setCursorPosition(0);

    ui->address->setText(query.value(5).toString());
    ui->address->QWidget::setToolTip(query.value(5).toString());
    ui->address->setCursorPosition(0);

    ui->email->setText(query.value(6).toString());
    ui->email->QWidget::setToolTip(query.value(6).toString());
    ui->email->setCursorPosition(0);

    ui->vyborId->setText(query.value(7).toString());

    ui->comment->setText(query.value(8).toString());

    if (ui->vyborId->text() == "0")
        ui->openAccessButton->hide();

    m_page = "1";

    updateCount();
}

/**
 * Выполняет вывод и обновление истории звонков данного физ. лица.
 */
void ViewContactDialog::loadCalls()
{
    deleteObjects();

    setPage();

    m_queryModel = new QSqlQueryModel(this);

    QString queryString;

    if (ui->callsTabWidget->currentWidget()->objectName() == "allCalls")
    {
        queryString = "SELECT IF(";

        for (qint32 i = 0; i < m_numbers.length(); ++i)
        {
            if (i > 0)
                queryString.append(" || ");

            queryString.append("src = '"+m_numbers[i]+"'");
        }

        queryString.append(", extfield2, extfield1), ");
    }
    else if (ui->callsTabWidget->currentWidget()->objectName() == "placedCalls")
        queryString = "SELECT extfield1, ";
    else
        queryString = "SELECT extfield2, ";

    queryString.append("src, dst, disposition, datetime, uniqueid, recordpath FROM cdr WHERE datetime >= DATE_SUB(CURRENT_DATE, INTERVAL '" + ui->comboBox_days->currentText() + "' DAY) ");

    if (ui->callsTabWidget->currentWidget()->objectName() == "missedCalls")
        queryString.append("AND disposition <> 'ANSWERED' ");
    else if (ui->callsTabWidget->currentWidget()->objectName() == "answeredCalls")
        queryString.append("AND disposition = 'ANSWERED' ");

    queryString.append("AND ( ");

    for (qint32 i = 0; i < m_numbers.length(); ++i)
    {
        if (ui->callsTabWidget->currentWidget()->objectName() == "allCalls")
        {
            if (i == 0)
                queryString.append(" src = '" + m_numbers[i] + "' OR dst = '" + m_numbers[i] + "' ");
            else
                queryString.append(" OR src = '" + m_numbers[i] + "' OR dst = '" + m_numbers[i] + "' ");
        }
        else if (ui->callsTabWidget->currentWidget()->objectName() == "missedCalls" || ui->callsTabWidget->currentWidget()->objectName() == "answeredCalls")
        {
            if (i == 0)
                queryString.append(" src = '" + m_numbers[i] + "' ");
            else
                queryString.append(" OR src = '" + m_numbers[i] + "' ");
        }
        else if (ui->callsTabWidget->currentWidget()->objectName() == "placedCalls")
        {
            if (i == 0)
                queryString.append(" dst = '" + m_numbers[i] + "' ");
            else
                queryString.append(" OR dst = '" + m_numbers[i] + "' ");
        }
    }

    queryString.append(" ) ");

    queryString.append("ORDER BY datetime DESC LIMIT ");

    if (ui->lineEdit_page->text() == "1")
        queryString.append("0," + QString::number(ui->lineEdit_page->text().toInt() * ui->comboBox_list->currentText().toInt()) + " ");
    else
       queryString.append(QString::number(ui->lineEdit_page->text().toInt() * ui->comboBox_list->currentText().toInt() -
                                          ui->comboBox_list->currentText().toInt()) + " , " +
                          QString::number(ui->comboBox_list->currentText().toInt()));

    m_queryModel->setQuery(queryString, m_dbCalls);

    m_queryModel->setHeaderData(0, Qt::Horizontal, tr("Имя"));
    m_queryModel->setHeaderData(1, Qt::Horizontal, tr("Откуда"));
    m_queryModel->setHeaderData(2, Qt::Horizontal, tr("Кому"));
    m_queryModel->insertColumn(4);
    m_queryModel->setHeaderData(4, Qt::Horizontal, tr("Статус"));
    m_queryModel->setHeaderData(5, Qt::Horizontal, tr("Дата и время"));
    m_queryModel->insertColumn(6);
    m_queryModel->setHeaderData(6, Qt::Horizontal, tr("Заметка"));

    ui->tableView->setModel(m_queryModel);

    ui->tableView->setColumnHidden(3, true);

    if (ui->callsTabWidget->currentWidget()->objectName() == "missedCalls" || ui->callsTabWidget->currentWidget()->objectName() == "answeredCalls")
        ui->tableView->setColumnHidden(4, true);

    ui->tableView->setColumnHidden(7, true);
    ui->tableView->setColumnHidden(8, true);

    for (qint32 row_index = 0; row_index < ui->tableView->model()->rowCount(); ++row_index)
    {
        QString extfield = m_queryModel->data(m_queryModel->index(row_index, 0)).toString();
        QString src = m_queryModel->data(m_queryModel->index(row_index, 1)).toString();
        QString dst = m_queryModel->data(m_queryModel->index(row_index, 2)).toString();
        QString dialogStatus = m_queryModel->data(m_queryModel->index(row_index, 3)).toString();
        QString uniqueid = m_queryModel->data(m_queryModel->index(row_index, 7)).toString();

        if (extfield.isEmpty())
            ui->tableView->setIndexWidget(m_queryModel->index(row_index, 0), loadName(src, dst));

        if (ui->callsTabWidget->currentWidget()->objectName() == "allCalls" || ui->callsTabWidget->currentWidget()->objectName() == "placedCalls")
            ui->tableView->setIndexWidget(m_queryModel->index(row_index, 4), loadStatus(dialogStatus));

        QSqlQuery query(m_db);

        query.prepare("SELECT EXISTS(SELECT note FROM calls WHERE uniqueid = "+uniqueid+")");
        query.exec();
        query.first();

        if (query.value(0).toBool())
        {
            ui->tableView->setIndexWidget(m_queryModel->index(row_index, 6), loadNote(uniqueid));

            ui->tableView->resizeRowToContents(row_index);
        }
        else
            ui->tableView->setRowHeight(row_index, 34);
    }

    ui->tableView->horizontalHeader()->setDefaultSectionSize(maximumWidth());

    ui->tableView->resizeColumnsToContents();

    if (ui->tableView->model()->columnCount() != 0)
        ui->tableView->horizontalHeader()->setSectionResizeMode(6, QHeaderView::Stretch);

    ui->playAudio->setDisabled(true);
    ui->playAudioPhone->setDisabled(true);
}

/**
 * Выполняет установку страницы для перехода.
 */
void ViewContactDialog::setPage()
{
    QString pages = ui->label_pages->text();

    if (m_countRecords <= ui->comboBox_list->currentText().toInt())
        pages = "1";
    else
    {
        qint32 remainder = m_countRecords % ui->comboBox_list->currentText().toInt();

        if (remainder)
            remainder = 1;
        else
            remainder = 0;

        pages = QString::number(m_countRecords / ui->comboBox_list->currentText().toInt() + remainder);
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
    else if (m_go == "default" && m_page == "1")
        m_page = "1";

    ui->lineEdit_page->setText(m_page);
    ui->label_pages->setText(tr("из ") + pages);
}

/**
 * Выполняет установку виджета для поля "Заметка".
 */
QWidget* ViewContactDialog::loadNote(const QString& uniqueid)
{
    QWidget* widget = new QWidget(this);
    QHBoxLayout* layout = new QHBoxLayout(widget);
    QLabel* noteLabel = new QLabel(widget);

    layout->addWidget(noteLabel);

    QSqlQuery query(m_db);

    query.prepare("SELECT note FROM calls WHERE uniqueid = '" + uniqueid + "' ORDER BY datetime DESC");
    query.exec();
    query.first();

    QString note = query.value(0).toString();

    QRegularExpressionMatchIterator hrefIterator = m_hrefRegExp.globalMatch(note);
    QStringList hrefs, hrefsNoCharacters, hrefsReplaceCharacters;

    note.replace("<", "&lt;").replace(">", "&gt;");

    while (hrefIterator.hasNext())
    {
        QRegularExpressionMatch match = hrefIterator.next();
        QString href = match.captured(1);

        hrefs << href;
        href.remove(QRegularExpression("[\\,\\.\\;\\:\\'\\\"\\-\\!\\?\\^\\`\\~\\*\\№\\%\\&\\$\\#\\<\\>\\(\\)\\[\\]\\{\\}]+$"));
        hrefsNoCharacters << href;
    }

    QStringList firstCharList, lastCharList;

    for (qint32 i = 0; i < hrefs.length(); ++i)
    {
        QString hrefReplaceCharacters = QString(hrefs.at(i)).replace("<", "&lt;").replace(">", "&gt;");
        hrefsReplaceCharacters << hrefReplaceCharacters;
        hrefReplaceCharacters = hrefReplaceCharacters.remove(hrefsNoCharacters.at(i));

        if (hrefReplaceCharacters.isEmpty())
            lastCharList << " ";
        else
            lastCharList << hrefReplaceCharacters;
    }

    note.replace(QRegularExpression("\\n"), QString(" <br> "));

    qint32 index = 0;

    for (qint32 i = 0; i < hrefsReplaceCharacters.length(); ++i)
    {
        if (i == 0)
            index = note.indexOf(hrefsReplaceCharacters.at(i));
        else
            index = note.indexOf(hrefsReplaceCharacters.at(i), index + hrefsReplaceCharacters.at(i - 1).size());

        if (index > 0)
            firstCharList << note.at(index - 1);
        else
            firstCharList << "";
    }

    for (qint32 i = 0; i < hrefs.length(); ++i)
    {
        qint32 size;

        if (firstCharList.at(i) == "")
            size = hrefsReplaceCharacters.at(i).size();
        else
            size = hrefsReplaceCharacters.at(i).size() + 1;

        note.replace(note.indexOf(QRegularExpression("( |^|\\^|\\.|\\,|\\(|\\)|\\[|\\]|\\{|\\}|\\;|\\'|\\\"|[a-zA-Z0-9а-яА-Я]|\\`|\\~|\\%|\\$|\\#|\\№|\\@|\\&|\\/|\\\\|\\!|\\*)" + QRegularExpression::escape(hrefsReplaceCharacters.at(i)) + "( |$)")),
                     size, QString(firstCharList.at(i) + "<a href='" + hrefsNoCharacters.at(i) + "'>" + hrefsNoCharacters.at(i) + "</a>" + lastCharList.at(i)));
    }

    noteLabel->setText(note);
    noteLabel->setOpenExternalLinks(true);
    noteLabel->setWordWrap(true);

    m_widgets.append(widget);

    return widget;
}

/**
 * Выполняет установку виджета для поля "Статус".
 */
QWidget* ViewContactDialog::loadStatus(const QString& dialogStatus)
{
    QWidget* widget = new QWidget(this);
    QHBoxLayout* layout = new QHBoxLayout(widget);
    QLabel* statusLabel = new QLabel(widget);

    if (dialogStatus == "NO ANSWER")
        statusLabel->setText(tr("Пропущенный") + " ");
    else if (dialogStatus == "BUSY")
        statusLabel->setText(tr("Занято") + " ");
    else if (dialogStatus == "CANCEL")
        statusLabel->setText(tr("Отколено") + " ");
    else if (dialogStatus == "ANSWERED")
        statusLabel->setText(tr("Принятый") + " ");

    layout->addWidget(statusLabel);
    layout->setContentsMargins(3, 0, 0, 0);

    m_widgets.append(widget);

    return widget;
}

/**
 * Выполняет установку виджета для поля "Имя".
 */
QWidget* ViewContactDialog::loadName(const QString& src, const QString& dst)
{
    QWidget* widget = new QWidget(this);
    QHBoxLayout* layout = new QHBoxLayout(widget);
    QLabel* nameLabel = new QLabel(widget);

    qint32 counter = 0;

    for (qint32 i = 0; i < m_numbers.length(); ++i)
    {
        if (src == m_numbers[i])
        {
            nameLabel->setText(dst);
            counter++;
        }
    }

    if (counter == 0)
        nameLabel->setText(src);

    layout->addWidget(nameLabel);
    layout->setContentsMargins(3, 0, 0, 0);

    m_widgets.append(widget);

    return widget;
}

/**
 * Выполняет удаление объектов класса.
 */
void ViewContactDialog::deleteObjects()
{
    if (!m_queryModel.isNull())
    {
        for (qint32 i = 0; i < m_widgets.size(); ++i)
            m_widgets[i]->deleteLater();

        m_widgets.clear();

        m_queryModel->deleteLater();
    }
}

/**
 * Выполняет обновление количества записей в истории звонков.
 */
void ViewContactDialog::updateCount()
{
    QSqlQuery query(m_dbCalls);

    QString queryString = "SELECT COUNT(*) FROM cdr WHERE datetime >= DATE_SUB(CURRENT_DATE, INTERVAL '" + ui->comboBox_days->currentText() + "' DAY) ";

    if (ui->callsTabWidget->currentWidget()->objectName() == "missedCalls")
        queryString.append("AND disposition <> 'ANSWERED' ");
    else if (ui->callsTabWidget->currentWidget()->objectName() == "answeredCalls")
        queryString.append("AND disposition = 'ANSWERED' ");

    queryString.append("AND ( ");

    for (qint32 i = 0; i < m_numbers.length(); ++i)
    {
        if (ui->callsTabWidget->currentWidget()->objectName() == "allCalls")
        {
            if (i == 0)
                queryString.append(" src = '" + m_numbers[i] + "' OR dst = '" + m_numbers[i] + "'");
            else
                queryString.append(" OR src = '" + m_numbers[i] + "' OR dst = '" + m_numbers[i] + "'");
        }
        else if (ui->callsTabWidget->currentWidget()->objectName() == "missedCalls" || ui->callsTabWidget->currentWidget()->objectName() == "answeredCalls")
        {
            if (i == 0)
                queryString.append(" src = '" + m_numbers[i] + "'");
            else
                queryString.append(" OR src = '" + m_numbers[i] + "'");
        }
        else if (ui->callsTabWidget->currentWidget()->objectName() == "placedCalls")
        {
            if (i == 0)
                queryString.append(" dst = '" + m_numbers[i] + "'");
            else
                queryString.append(" OR dst = '" + m_numbers[i] + "'");
        }
    }

    queryString.append(" ) ");

    query.prepare(queryString);
    query.exec();
    query.first();

    m_countRecords = query.value(0).toInt();

    loadCalls();
}

/**
 * Выполняет открытие окна с заметками для их просмотра.
 */
void ViewContactDialog::viewNotes(const QModelIndex& index)
{
    QString uniqueid = m_queryModel->data(m_queryModel->index(index.row(), 7)).toString();

    m_notesDialog = new NotesDialog;
    m_notesDialog->setValues(uniqueid, "");
    m_notesDialog->hideAddNote();
    m_notesDialog->show();
    m_notesDialog->setAttribute(Qt::WA_DeleteOnClose);
}

/**
 * Выполняет обработку смены вкладки.
 */
void ViewContactDialog::tabSelected()
{
    ui->tableView->setModel(NULL);

    m_page = "1";

    onUpdate();
}

/**
 * Выполняет операции для последующего обновления истории звонков
 * (с количеством записей).
 */
void ViewContactDialog::onUpdate()
{
    m_go = "default";

    updateCount();
}

/**
 * Выполняет операции для последующего перехода на предыдущую страницу.
 */
void ViewContactDialog::on_previousButton_clicked()
{
    ui->tableView->scrollToTop();

    m_go = "previous";

    loadCalls();
}

/**
 * Выполняет операции для последующего перехода на следующую страницу.
 */
void ViewContactDialog::on_nextButton_clicked()
{
    ui->tableView->scrollToTop();

    m_go = "next";

    loadCalls();
}

/**
 * Выполняет операции для последующего перехода на первую страницу.
 */
void ViewContactDialog::on_previousStartButton_clicked()
{
    ui->tableView->scrollToTop();

    m_go = "previousStart";

    loadCalls();
}

/**
 * Выполняет операции для последующего перехода на последнюю страницу.
 */
void ViewContactDialog::on_nextEndButton_clicked()
{
    ui->tableView->scrollToTop();

    m_go = "nextEnd";

    loadCalls();
}

/**
 * Выполняет операции для последующего перехода на заданную страницу.
 */
void ViewContactDialog::on_lineEdit_page_returnPressed()
{
    ui->tableView->scrollToTop();

    m_go = "enter";

    loadCalls();
}

/**
 * Выполняет открытие окна с медиапроигрывателем для прослушивания записи звонка.
 */
void ViewContactDialog::onPlayAudio()
{
    if (ui->tableView->selectionModel()->selectedRows().count() != 1)
    {
        MsgBoxError(tr("Выберите одну запись!"));

        return;
    }

    if (!m_recordpath.isEmpty())
    {
        if (!m_player.isNull())
            m_player->close();

        m_player = new Player;
        m_player->openMedia(m_recordpath);
        m_player->show();
        m_player->setAttribute(Qt::WA_DeleteOnClose);
    }
}

/**
 * Выполняет операции для последующего прослушивания записи звонка через телефон.
 */
void ViewContactDialog::onPlayAudioPhone()
{
    if (ui->tableView->selectionModel()->selectedRows().count() != 1)
    {
        MsgBoxError(tr("Выберите одну запись!"));

        return;
    }

    if (!m_recordpath.isEmpty())
    {
        QString protocol = global::getSettingsValue(g_personalNumber, "extensions").toString();

        g_asteriskManager->originateAudio(g_personalNumber, protocol, m_recordpath);
    }
}

/**
 * Выполняет вытягивание значений полей из записи.
 */
void ViewContactDialog::getData(const QModelIndex& index)
{
    m_recordpath = m_queryModel->data(m_queryModel->index(index.row(), 8)).toString();

    ui->playAudio->setDisabled(true);
    ui->playAudioPhone->setDisabled(true);

    if (!m_recordpath.isEmpty())
    {
        ui->playAudio->setDisabled(false);
        ui->playAudioPhone->setDisabled(false);
    }
}

/**
 * Получает запрос на обновление состояния окна.
 */
void ViewContactDialog::receiveData(bool update, qint32 x, qint32 y)
{
    qint32 desktopHeight;
    qint32 desktopWidth;
    qint32 widgetHeight = QWidget::height();
    qint32 widgetWidth = QWidget::width();

    QDesktopWidget desktopWidget;
    QRect desktop = desktopWidget.availableGeometry(this);

    desktopWidth = desktop.width();
    desktopHeight = desktop.height();

    if (update)
    {
        emit sendData(true);

        close();
    }
    else
    {
        if (x < 0 && (desktopHeight - y) > widgetHeight)
        {
            x = 0;
            this->move(x, y);
        }
        else if (x < 0 && ((desktopHeight - y) < widgetHeight))
        {
            x = 0;
            y = widgetHeight;
            this->move(x, y);
        }
        else if ((desktopWidth - x) < widgetWidth && (desktopHeight - y) > widgetHeight)
        {
            x = widgetWidth * 0.9;
            this->move(x, y);
        }
        else if ((desktopWidth - x) < widgetWidth && ((desktopHeight - y) < widgetHeight))
        {
            x = widgetWidth * 0.9;
            y = widgetHeight * 0.9;
            this->move(x, y);
        }
        else if (x > 0 && ((desktopHeight - y) < widgetHeight))
        {
            y = widgetHeight * 0.9;
            this->move(x, y);
        }
        else
        {
            this->move(x, y);
        }

        show();
    }
}
