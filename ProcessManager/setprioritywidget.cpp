#include "setprioritywidget.h"

SetPriorityWidget::SetPriorityWidget(QWidget* parent)
{
    ui.setupUi(this);

    ui.comboBox->addItems({ "Realtime", "High", "Above Normal", "Normal", "Below Normal", "Idle" });

    connect(ui.buttonBox, &QDialogButtonBox::accepted, this, &SetPriorityWidget::onOKClicked);

    setModal(true);
}

SetPriorityWidget::~SetPriorityWidget()
{
}

void SetPriorityWidget::onOKClicked()
{
    const int inputIndex = ui.comboBox->currentIndex();
    Index = inputIndex;
}

int SetPriorityWidget::GetSelectedIndex()
{
    return Index;
}
