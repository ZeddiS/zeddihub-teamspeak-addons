#include "custom_dialog.h"

#include <QtWidgets/QApplication>
#include <QtWidgets/QDialog>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QFormLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSlider>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QVBoxLayout>
#include <QtCore/QString>

#include <cstdio>
#include <cstring>

namespace {

const char* kStyleSheet = R"(
QDialog {
    background-color: #2b2d31;
    color: #dcddde;
}
QLabel {
    color: #dcddde;
}
QLineEdit, QSpinBox {
    background-color: #1e1f22;
    color: #f2f3f5;
    border: 1px solid #1a1b1e;
    border-radius: 3px;
    padding: 4px 6px;
    selection-background-color: #5865f2;
}
QSlider::groove:horizontal {
    height: 4px;
    background: #1e1f22;
    border-radius: 2px;
}
QSlider::handle:horizontal {
    background: #5865f2;
    width: 14px;
    height: 14px;
    border-radius: 7px;
    margin: -5px 0;
}
QSlider::sub-page:horizontal {
    background: #5865f2;
    border-radius: 2px;
}
QPushButton {
    background-color: #4e5058;
    color: #ffffff;
    border: none;
    padding: 6px 16px;
    border-radius: 3px;
    min-width: 80px;
}
QPushButton:hover {
    background-color: #5d5f67;
}
QPushButton:default {
    background-color: #5865f2;
}
QPushButton:default:hover {
    background-color: #4752c4;
}
)";

}  // namespace

namespace custom_dialog {

Result runMoveSpamDialog(uint64 currentChannel) {
    (void)currentChannel;
    Result r;

    QDialog dlg;
    dlg.setWindowTitle(QStringLiteral("MoveSpam — Custom"));
    dlg.setMinimumWidth(440);
    dlg.setStyleSheet(QString::fromUtf8(kStyleSheet));

    auto* layout = new QVBoxLayout(&dlg);
    auto* form = new QFormLayout();
    form->setLabelAlignment(Qt::AlignLeft | Qt::AlignVCenter);

    auto* destEdit = new QLineEdit(&dlg);
    destEdit->setPlaceholderText(QStringLiteral("Channel name or ID"));
    form->addRow(QStringLiteral("Target channel:"), destEdit);

    auto* hint = new QLabel(
        QStringLiteral("Enter either channel ID (e.g. 42) or exact channel name."),
        &dlg);
    hint->setStyleSheet(QStringLiteral("color: #a3a6aa; font-size: 11px;"));
    form->addRow(QString(), hint);

    // Interval slider + spinbox -----------------------------------------
    auto* intervalRow = new QHBoxLayout();
    auto* intervalSlider = new QSlider(Qt::Horizontal, &dlg);
    intervalSlider->setRange(200, 10000);
    intervalSlider->setValue(1500);
    intervalSlider->setTickPosition(QSlider::TicksBelow);
    intervalSlider->setTickInterval(1000);
    auto* intervalSpin = new QSpinBox(&dlg);
    intervalSpin->setRange(200, 10000);
    intervalSpin->setValue(1500);
    intervalSpin->setSuffix(QStringLiteral(" ms"));
    intervalSpin->setFixedWidth(100);
    QObject::connect(intervalSlider, &QSlider::valueChanged,
                     intervalSpin, &QSpinBox::setValue);
    QObject::connect(intervalSpin,
                     QOverload<int>::of(&QSpinBox::valueChanged),
                     intervalSlider, &QSlider::setValue);
    intervalRow->addWidget(intervalSlider, 1);
    intervalRow->addWidget(intervalSpin);
    form->addRow(QStringLiteral("Interval (ms):"), intervalRow);

    // Max moves slider + spinbox ----------------------------------------
    auto* maxRow = new QHBoxLayout();
    auto* maxSlider = new QSlider(Qt::Horizontal, &dlg);
    maxSlider->setRange(0, 1000);
    maxSlider->setValue(200);
    maxSlider->setTickPosition(QSlider::TicksBelow);
    maxSlider->setTickInterval(100);
    auto* maxSpin = new QSpinBox(&dlg);
    maxSpin->setRange(0, 1000);
    maxSpin->setValue(200);
    maxSpin->setFixedWidth(100);
    maxSpin->setSpecialValueText(QStringLiteral("∞"));
    QObject::connect(maxSlider, &QSlider::valueChanged,
                     maxSpin, &QSpinBox::setValue);
    QObject::connect(maxSpin, QOverload<int>::of(&QSpinBox::valueChanged),
                     maxSlider, &QSlider::setValue);
    maxRow->addWidget(maxSlider, 1);
    maxRow->addWidget(maxSpin);
    form->addRow(QStringLiteral("Max moves (0 = unlimited):"), maxRow);

    layout->addLayout(form);

    auto* note = new QLabel(
        QStringLiteral("Plugin will repeatedly move the target between their "
                       "current channel and the target channel. Set 0 = unlimited; "
                       "stop manually via right-click menu or /movespam stop."),
        &dlg);
    note->setWordWrap(true);
    note->setStyleSheet(QStringLiteral("color: #a3a6aa; font-size: 11px; padding: 6px 0;"));
    layout->addWidget(note);

    auto* btnBox = new QDialogButtonBox(&dlg);
    auto* runBtn = btnBox->addButton(QStringLiteral("Start"),
                                     QDialogButtonBox::AcceptRole);
    btnBox->addButton(QStringLiteral("Cancel"),
                      QDialogButtonBox::RejectRole);
    runBtn->setDefault(true);
    layout->addWidget(btnBox);

    QObject::connect(btnBox, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    QObject::connect(btnBox, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);

    if (dlg.exec() != QDialog::Accepted) return r;

    QString text = destEdit->text().trimmed();
    if (text.isEmpty()) return r;

    bool ok = false;
    qulonglong asID = text.toULongLong(&ok);
    if (ok && asID > 0) {
        r.destChannelID = asID;
    } else {
        QByteArray utf8 = text.toUtf8();
        std::strncpy(r.destName, utf8.constData(), sizeof(r.destName) - 1);
        r.destName[sizeof(r.destName) - 1] = '\0';
    }
    r.intervalMs = intervalSpin->value();
    r.maxMoves = maxSpin->value();
    r.accepted = true;
    return r;
}

}  // namespace custom_dialog
