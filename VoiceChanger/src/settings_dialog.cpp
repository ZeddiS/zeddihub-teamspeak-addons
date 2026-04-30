#include "settings_dialog.h"

#include <QtWidgets/QApplication>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QDialog>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QDoubleSpinBox>
#include <QtWidgets/QFormLayout>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSlider>
#include <QtWidgets/QVBoxLayout>
#include <QtCore/QString>

#include "voice_engine.h"

namespace {

const char* kStyleSheet = R"(
QDialog {
    background-color: #2b2d31;
    color: #dcddde;
}
QLabel {
    color: #dcddde;
}
QGroupBox {
    color: #ffffff;
    border: 1px solid #1a1b1e;
    border-radius: 4px;
    margin-top: 10px;
    padding: 8px;
}
QGroupBox::title {
    subcontrol-origin: margin;
    left: 10px;
    padding: 0 4px;
}
QComboBox, QDoubleSpinBox {
    background-color: #1e1f22;
    color: #f2f3f5;
    border: 1px solid #1a1b1e;
    border-radius: 3px;
    padding: 4px 6px;
    min-height: 20px;
}
QComboBox::drop-down {
    border: 0;
    width: 18px;
}
QComboBox QAbstractItemView {
    background-color: #1e1f22;
    color: #f2f3f5;
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
    padding: 8px 18px;
    border-radius: 3px;
    min-width: 100px;
    font-weight: 500;
}
QPushButton:hover { background-color: #5d5f67; }
QPushButton#enableBtn { background-color: #248046; }
QPushButton#enableBtn:hover { background-color: #2f9e57; }
QPushButton#disableBtn { background-color: #da373c; }
QPushButton#disableBtn:hover { background-color: #e34d52; }
)";

}  // namespace

namespace settings_dialog {

void run(VoiceEngine& engine) {
    VoiceConfig current = engine.config();

    QDialog dlg;
    dlg.setWindowTitle(QStringLiteral("Voice Changer — Settings"));
    dlg.setMinimumWidth(440);
    dlg.setStyleSheet(QString::fromUtf8(kStyleSheet));

    auto* layout = new QVBoxLayout(&dlg);

    // Status row -------------------------------------------------------
    auto* statusLabel = new QLabel(&dlg);
    auto refreshStatus = [&] {
        statusLabel->setText(
            engine.enabled()
                ? QStringLiteral("● <b>Voice Changer ZAPNUT</b>")
                : QStringLiteral("○ Voice Changer vypnut"));
        statusLabel->setStyleSheet(
            engine.enabled()
                ? QStringLiteral("color: #57f287; font-size: 13px; padding: 6px;")
                : QStringLiteral("color: #a3a6aa; font-size: 13px; padding: 6px;"));
    };
    refreshStatus();
    layout->addWidget(statusLabel);

    // Voice preset combo ------------------------------------------------
    auto* presetGroup = new QGroupBox(QStringLiteral("Hlas"), &dlg);
    auto* presetLayout = new QVBoxLayout(presetGroup);

    auto* combo = new QComboBox(&dlg);
    combo->addItem(QStringLiteral("Off (passthrough)"),         (int)VoicePreset::Off);
    combo->addItem(QStringLiteral("Volume Boost (sanity test, 1.5x gain)"),
                                                                 (int)VoicePreset::VolumeBoost);
    combo->addItem(QStringLiteral("─── Pitch ───"),             -1);
    combo->addItem(QStringLiteral("Helium (+6 půltónů)"),       (int)VoicePreset::Helium);
    combo->addItem(QStringLiteral("Chipmunk (+12 půltónů)"),    (int)VoicePreset::Chipmunk);
    combo->addItem(QStringLiteral("Deep (-4 půltóny)"),         (int)VoicePreset::Deep);
    combo->addItem(QStringLiteral("Demon (-7 půltónů + grit)"), (int)VoicePreset::Demon);
    combo->addItem(QStringLiteral("Custom pitch (slider)"),     (int)VoicePreset::Custom);
    combo->addItem(QStringLiteral("─── Effects ───"),           -1);
    combo->addItem(QStringLiteral("Robot (ring modulation)"),   (int)VoicePreset::Robot);
    combo->addItem(QStringLiteral("Echo (delay)"),              (int)VoicePreset::Echo);
    combo->addItem(QStringLiteral("Distortion (raspy/guttural)"), (int)VoicePreset::Distortion);
    combo->addItem(QStringLiteral("Whisper (breathy)"),         (int)VoicePreset::Whisper);
    combo->addItem(QStringLiteral("Telephone (300-3400 Hz)"),   (int)VoicePreset::Telephone);
    combo->addItem(QStringLiteral("Underwater (muffled)"),      (int)VoicePreset::Underwater);
    combo->addItem(QStringLiteral("Megaphone (PA system)"),     (int)VoicePreset::Megaphone);
    combo->setCurrentIndex(combo->findData((int)current.preset));
    presetLayout->addWidget(combo);

    // Custom semitones row
    auto* customRow = new QHBoxLayout();
    auto* customLabel = new QLabel(QStringLiteral("Custom pitch (semitones):"), &dlg);
    auto* customSlider = new QSlider(Qt::Horizontal, &dlg);
    customSlider->setRange(-1200, 1200);  // 0.01 semitone steps
    customSlider->setValue((int)(current.customSemitones * 100));
    auto* customSpin = new QDoubleSpinBox(&dlg);
    customSpin->setRange(-12.0, 12.0);
    customSpin->setSingleStep(0.5);
    customSpin->setValue(current.customSemitones);
    customSpin->setDecimals(1);
    customSpin->setFixedWidth(80);
    QObject::connect(customSlider, &QSlider::valueChanged, [customSpin](int v) {
        customSpin->setValue(v / 100.0);
    });
    QObject::connect(customSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
                     [customSlider](double v) {
                         customSlider->setValue((int)(v * 100));
                     });
    customRow->addWidget(customLabel);
    customRow->addWidget(customSlider, 1);
    customRow->addWidget(customSpin);
    presetLayout->addLayout(customRow);

    layout->addWidget(presetGroup);

    // Echo settings ----------------------------------------------------
    auto* echoGroup = new QGroupBox(QStringLiteral("Echo (jen pro Echo preset)"), &dlg);
    auto* echoForm = new QFormLayout(echoGroup);

    auto* echoDelaySpin = new QDoubleSpinBox(&dlg);
    echoDelaySpin->setRange(50.0, 500.0);
    echoDelaySpin->setSingleStep(10.0);
    echoDelaySpin->setSuffix(QStringLiteral(" ms"));
    echoDelaySpin->setValue(current.echoDelayMs);
    echoForm->addRow(QStringLiteral("Delay:"), echoDelaySpin);

    auto* echoFbSpin = new QDoubleSpinBox(&dlg);
    echoFbSpin->setRange(0.0, 0.95);
    echoFbSpin->setSingleStep(0.05);
    echoFbSpin->setDecimals(2);
    echoFbSpin->setValue(current.echoFeedback);
    echoForm->addRow(QStringLiteral("Feedback:"), echoFbSpin);

    layout->addWidget(echoGroup);

    // Buttons ----------------------------------------------------------
    auto* btnRow = new QHBoxLayout();
    auto* enableBtn  = new QPushButton(QStringLiteral("Zapnout Voice Changer"), &dlg);
    enableBtn->setObjectName("enableBtn");
    auto* disableBtn = new QPushButton(QStringLiteral("Vypnout Voice Changer"), &dlg);
    disableBtn->setObjectName("disableBtn");
    auto* closeBtn   = new QPushButton(QStringLiteral("Zavřít"), &dlg);
    btnRow->addWidget(enableBtn);
    btnRow->addWidget(disableBtn);
    btnRow->addStretch();
    btnRow->addWidget(closeBtn);
    layout->addLayout(btnRow);

    auto applyConfig = [&](bool enabledState) {
        VoiceConfig c;
        int presetVal = combo->currentData().toInt();
        if (presetVal < 0) presetVal = (int)VoicePreset::Off;  // dividers map to Off
        c.preset = (VoicePreset)presetVal;
        c.customSemitones = (float)customSpin->value();
        c.echoDelayMs = (float)echoDelaySpin->value();
        c.echoFeedback = (float)echoFbSpin->value();
        c.enabled = enabledState;
        engine.setConfig(c);
        refreshStatus();
    };

    QObject::connect(enableBtn,  &QPushButton::clicked, [&] { applyConfig(true); });
    QObject::connect(disableBtn, &QPushButton::clicked, [&] { applyConfig(false); });
    QObject::connect(closeBtn,   &QPushButton::clicked, &dlg, &QDialog::accept);

    // Live update settings while dialog is open
    QObject::connect(combo, QOverload<int>::of(&QComboBox::currentIndexChanged),
                     [&] {
                         if (engine.enabled()) applyConfig(true);
                     });
    QObject::connect(customSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
                     [&] {
                         if (engine.enabled()) applyConfig(true);
                     });

    dlg.exec();
}

}  // namespace settings_dialog
