/*
 * Класс служит для управления внутренними номерами.
 */

#include "AddExtensionDialog.h"
#include "ui_AddExtensionDialog.h"

#include "Global.h"

#include <QMessageBox>

AddExtensionDialog::AddExtensionDialog(QWidget* parent) :
    QDialog(parent),
    ui(new Ui::AddExtensionDialog)
{
    ui->setupUi(this);

    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    ui->extenLine->setValidator(new QIntValidator(this));
}

AddExtensionDialog::~AddExtensionDialog()
{
    delete ui;
}

/**
 * Возвращает внутренний номер.
 */
QString AddExtensionDialog::getExtension() const
{
    return ui->extenLine->text();
}

/**
 * Возвращает протокол.
 */
QString AddExtensionDialog::getProtocol() const
{
    return ui->protocol->currentText();
}

/**
 * Выполняет установку внутреннего номера.
 */
void AddExtensionDialog::setExtension(const QString& extension)
{
    ui->extenLine->setText(extension);
}

/**
 * Выполняет установку протокола.
 */
void AddExtensionDialog::setProtocol(const QString& protocol)
{
    qint32 index = ui->protocol->findText(protocol);

    ui->protocol->setCurrentIndex(index);
}

/**
 * Выполняет сохранение внутреннего номера.
 */
void AddExtensionDialog::on_applyButton_pressed()
{
    if (ui->extenLine->text().isEmpty())
        MsgBoxError(tr("Номер не соответствует формату!"));
    else
    {
        QDialog::accept();

        emit addExten();
    }
}

/**
 * Выполняет закрытие окна.
 */
void AddExtensionDialog::on_cancelButton_pressed()
{
    QDialog::close();
}
