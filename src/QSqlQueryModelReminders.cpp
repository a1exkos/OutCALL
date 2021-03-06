/*
 * Переопределение класса QSqlQueryModel для закрашивания
 * в таблице строк с делегированными пользователю напоминаниями.
 */

#include "QSqlQueryModelReminders.h"
#include "RemindersDialog.h"

#include <QPainter>
#include <QCheckBox>

QSqlQueryModelReminders::QSqlQueryModelReminders(QObject* parent) : QSqlQueryModel(parent)
{
}

/**
 * Выполняет установку родительской таблицы.
 */
void QSqlQueryModelReminders::setParentTable(const QTableView* parentTable)
{
    m_parentTable = parentTable;
}

/**
 * Выполняет закраску нужных строк таблицы (переопределение функции).
 */
QVariant QSqlQueryModelReminders::data(const QModelIndex& index, qint32 role) const
{
    if (role == Qt::BackgroundRole && m_parentTable->indexWidget(index.sibling(index.row(), 1))->findChild<QCheckBox*>() == nullptr)
        return QBrush(QColor(254, 252, 196));

    return QSqlQueryModel::data(index, role);
}
