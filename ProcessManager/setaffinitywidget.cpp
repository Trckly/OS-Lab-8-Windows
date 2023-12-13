#include "setaffinitywidget.h"

SetAffinityWidget::SetAffinityWidget(QWidget* parent)
{
    ui.setupUi(this);

    connect(ui.buttonBox, &QDialogButtonBox::accepted, this, &SetAffinityWidget::onOKClicked);

    setModal(true);
}

void SetAffinityWidget::onOKClicked()
{
    std::vector<bool> checked;

    checked.push_back(ui.CPU_0->isChecked());
    checked.push_back(ui.CPU_1->isChecked());
    checked.push_back(ui.CPU_2->isChecked());
    checked.push_back(ui.CPU_3->isChecked());

    std::vector<QString> binaryCodes = ConvertIntoBinary(checked);

    strAffinityMask = CombineIntoAffinityMask(binaryCodes);
}


QString SetAffinityWidget::CombineIntoAffinityMask(const std::vector<QString>& binaryCodes) {
    // Ensure there are exactly 4 binary codes
    if (binaryCodes.size() != 4) {
        return QString("0000");
    }

    QString affinityMask = "0000";

    // Iterate through the binary codes and combine them into the affinity mask
    for (int i = 0; i < 4; ++i) {
        if (binaryCodes[i] == "0001") {
            // Set the corresponding bit to '1'
            affinityMask[i+3] = '1';
        }
        if (binaryCodes[i] == "0010") {
            // Set the corresponding bit to '1'
            affinityMask[i+1] = '1';
        }
        if (binaryCodes[i] == "0100") {
            // Set the corresponding bit to '1'
            affinityMask[i-1] = '1';
        }
        if (binaryCodes[i] == "1000") {
            // Set the corresponding bit to '1'
            affinityMask[i-3] = '1';
        }
    }

    return affinityMask;
}

std::vector<QString> SetAffinityWidget::ConvertIntoBinary(std::vector<bool> checked)
{
    std::vector<QString> result = {"0000", "0000", "0000", "0000"};

    for (int i = 0; i < checked.size(); ++i) {
        switch (i)
        {
        case 0:
            if (checked[i]) {
                result[i] = "0001";
            }
            break;

        case 1:
            if (checked[i]) {
                result[i] = "0010";
            }
            break;

        case 2:
            if (checked[i]) {
                result[i] = "0100";
            }
            break;

        case 3:
            if (checked[i]) {
                result[i] = "1000";
            }
            break;

        default:
            break;
        }
    }

    return result;
}

int SetAffinityWidget::GetAffinityMask()
{
    bool ok;
    int result = strAffinityMask.toInt(&ok, 2);

    if (!ok) {
        return -1;
    }

    return result;
}
