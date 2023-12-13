#pragma once

#include <QDialog>
#include "ui_SetPriority.h"

class SetPriorityWidget : public QDialog
{
    Q_OBJECT

public:
    SetPriorityWidget(QWidget* parent);
    ~SetPriorityWidget();

    void onOKClicked();

    int GetSelectedIndex();

private:
    Ui::SetPriorityDialog ui;

    int Index;
};
