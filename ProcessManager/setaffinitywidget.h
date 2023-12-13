#pragma once

#include <QDialog>
#include "ui_SetAffinity.h"

class SetAffinityWidget : public QDialog
{
    Q_OBJECT

public:
    SetAffinityWidget(QWidget* parent);

    void onOKClicked();

    QString CombineIntoAffinityMask(const std::vector<QString>& binaryCodes);

    std::vector<QString> ConvertIntoBinary(std::vector<bool> checked);

    int GetAffinityMask();

private:
    Ui::SetAffinityDialog ui;

    QString strAffinityMask;
};
