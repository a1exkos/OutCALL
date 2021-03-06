#ifndef DEBUGINFODIALOG_H
#define DEBUGINFODIALOG_H

#include <QDialog>
#include <QKeyEvent>

namespace Ui {
class DebugInfoDialog;
}

class DebugInfoDialog : public QDialog
{
    Q_OBJECT

public:
    explicit DebugInfoDialog(QWidget* parent = 0);
    ~DebugInfoDialog();

    void updateDebug(const QString& info);

private slots:
    void onClear() const;
    void onExit();

    void keyPressEvent(QKeyEvent* event);

private:
    Ui::DebugInfoDialog* ui;
};

#endif // DEBUGINFODIALOG_H
