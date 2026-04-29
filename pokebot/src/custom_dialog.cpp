// Qt 5.15 custom dialog with TS3-style dark theme.
// - Mode toggle (Burst / Schedule) switches visible parameters
// - Slider + spinbox synced for each numeric input
// - Live preview of resulting poke count

#include "custom_dialog.h"

#include <QtWidgets/QApplication>
#include <QtWidgets/QButtonGroup>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QDialog>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QFormLayout>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPlainTextEdit>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QRadioButton>
#include <QtWidgets/QSlider>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QStackedWidget>
#include <QtWidgets/QVBoxLayout>
#include <QtCore/QString>

#include <string>
#include <vector>

#include "poke_engine.h"

namespace {

const char* kStyleSheet = R"(
QDialog {
    background-color: #2b2d31;
    color: #dcddde;
}
QLabel { color: #dcddde; font-size: 12px; }
QGroupBox {
    color: #ffffff;
    border: 1px solid #1a1b1e;
    border-radius: 4px;
    margin-top: 10px;
    padding: 8px;
    font-weight: 500;
}
QGroupBox::title {
    subcontrol-origin: margin;
    left: 10px;
    padding: 0 4px;
}
QLineEdit, QSpinBox, QPlainTextEdit {
    background-color: #1e1f22;
    color: #f2f3f5;
    border: 1px solid #1a1b1e;
    border-radius: 3px;
    padding: 4px 6px;
    selection-background-color: #5865f2;
}
QPlainTextEdit {
    font-family: 'Consolas', 'Courier New', monospace;
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
QCheckBox {
    color: #dcddde;
    spacing: 6px;
}
QCheckBox::indicator {
    width: 14px;
    height: 14px;
    border: 1px solid #4e5058;
    border-radius: 3px;
    background: #1e1f22;
}
QCheckBox::indicator:checked {
    background: #5865f2;
    border-color: #5865f2;
}
QRadioButton {
    color: #dcddde;
    spacing: 6px;
    padding: 4px;
}
QRadioButton::indicator {
    width: 14px;
    height: 14px;
}
QPushButton {
    background-color: #4e5058;
    color: #ffffff;
    border: none;
    padding: 6px 18px;
    border-radius: 3px;
    min-width: 90px;
    font-weight: 500;
}
QPushButton:hover { background-color: #5d5f67; }
QPushButton:default { background-color: #5865f2; }
QPushButton:default:hover { background-color: #4752c4; }
)";

// Helper: paired slider + spinbox
struct PairedSlider {
    QSlider* slider;
    QSpinBox* spin;

    static PairedSlider make(QWidget* parent, int min, int max, int value,
                             const QString& suffix = QString()) {
        PairedSlider p;
        p.slider = new QSlider(Qt::Horizontal, parent);
        p.slider->setRange(min, max);
        p.slider->setValue(value);
        p.slider->setTickPosition(QSlider::TicksBelow);
        p.slider->setTickInterval((max - min) / 10);
        p.spin = new QSpinBox(parent);
        p.spin->setRange(min, max);
        p.spin->setValue(value);
        if (!suffix.isEmpty()) p.spin->setSuffix(QString(" ") + suffix);
        p.spin->setFixedWidth(110);
        QObject::connect(p.slider, &QSlider::valueChanged,
                         p.spin, &QSpinBox::setValue);
        QObject::connect(p.spin, QOverload<int>::of(&QSpinBox::valueChanged),
                         p.slider, &QSlider::setValue);
        return p;
    }

    QHBoxLayout* layout() {
        auto* h = new QHBoxLayout();
        h->addWidget(slider, 1);
        h->addWidget(spin);
        return h;
    }

    int value() const { return spin->value(); }
};

std::vector<std::string> chunkText(const std::string& s, std::size_t n) {
    std::vector<std::string> out;
    if (s.empty()) {
        out.push_back(" ");
        return out;
    }
    for (std::size_t i = 0; i < s.size(); i += n) {
        out.push_back(s.substr(i, n));
    }
    return out;
}

}  // namespace

namespace custom_dialog {

void run(uint64 schid, anyID clientID, PokeEngine& engine) {
    QDialog dlg;
    dlg.setWindowTitle(QStringLiteral("Poke Bot — Custom"));
    dlg.setMinimumWidth(500);
    dlg.setStyleSheet(QString::fromUtf8(kStyleSheet));

    auto* layout = new QVBoxLayout(&dlg);
    layout->setSpacing(12);

    // ====== Mode selector =============================================
    auto* modeGroup = new QGroupBox(QStringLiteral("Režim"), &dlg);
    auto* modeLayout = new QHBoxLayout(modeGroup);
    auto* radioSchedule = new QRadioButton(QStringLiteral("📅  Schedule (1× za interval)"), &dlg);
    auto* radioBurst    = new QRadioButton(QStringLiteral("⚡  Burst (rychle za sebou)"), &dlg);
    radioSchedule->setChecked(true);
    auto* modeBtns = new QButtonGroup(&dlg);
    modeBtns->addButton(radioSchedule, 0);
    modeBtns->addButton(radioBurst, 1);
    modeLayout->addWidget(radioSchedule);
    modeLayout->addWidget(radioBurst);
    modeLayout->addStretch();
    layout->addWidget(modeGroup);

    // ====== Text + chunking ===========================================
    auto* textGroup = new QGroupBox(QStringLiteral("Zpráva"), &dlg);
    auto* textLayout = new QVBoxLayout(textGroup);
    auto* msgEdit = new QPlainTextEdit(&dlg);
    msgEdit->setPlainText(QStringLiteral("AHOJ"));
    msgEdit->setFixedHeight(80);
    msgEdit->setPlaceholderText(QStringLiteral("Text poke. TS3 limit ~ 100 znaků / poke."));
    textLayout->addWidget(msgEdit);
    auto* chunkChk = new QCheckBox(
        QStringLiteral("Rozsekat dlouhý text na 100-znakové chunky (každý chunk = 1 poke)"),
        &dlg);
    chunkChk->setChecked(true);
    textLayout->addWidget(chunkChk);
    layout->addWidget(textGroup);

    // ====== Mode-specific params (StackedWidget) ======================
    auto* paramStack = new QStackedWidget(&dlg);

    // -- Schedule page --
    auto* schedulePage = new QWidget();
    auto* scheduleForm = new QFormLayout(schedulePage);
    scheduleForm->setVerticalSpacing(10);
    auto countS  = PairedSlider::make(&dlg, 1, 500, 20);
    auto minS    = PairedSlider::make(&dlg, 200, 30000, 3000, "ms");
    auto maxS    = PairedSlider::make(&dlg, 200, 30000, 5000, "ms");
    scheduleForm->addRow(QStringLiteral("Počet poke:"),  countS.layout());
    scheduleForm->addRow(QStringLiteral("Interval min:"), minS.layout());
    scheduleForm->addRow(QStringLiteral("Interval max:"), maxS.layout());
    paramStack->addWidget(schedulePage);

    // -- Burst page --
    auto* burstPage = new QWidget();
    auto* burstForm = new QFormLayout(burstPage);
    burstForm->setVerticalSpacing(10);
    auto countB     = PairedSlider::make(&dlg, 1, 500, 50);
    auto burstDelay = PairedSlider::make(&dlg, 50, 1000, 200, "ms");
    burstForm->addRow(QStringLiteral("Počet poke:"),    countB.layout());
    burstForm->addRow(QStringLiteral("Burst delay:"),   burstDelay.layout());
    auto* burstHint = new QLabel(
        QStringLiteral("⚠  Pod 100ms riskuješ anti-flood kick z TS3 serveru."),
        &dlg);
    burstHint->setStyleSheet(QStringLiteral("color: #f0b132; font-size: 11px;"));
    burstForm->addRow(QString(), burstHint);
    paramStack->addWidget(burstPage);

    layout->addWidget(paramStack);

    QObject::connect(radioBurst, &QRadioButton::toggled,
                     [paramStack](bool checked) {
                         paramStack->setCurrentIndex(checked ? 1 : 0);
                     });
    paramStack->setCurrentIndex(0);

    // ====== Buttons ===================================================
    auto* btnBox = new QDialogButtonBox(&dlg);
    auto* runBtn = btnBox->addButton(QStringLiteral("Spustit"),
                                     QDialogButtonBox::AcceptRole);
    btnBox->addButton(QStringLiteral("Zrušit"),
                      QDialogButtonBox::RejectRole);
    runBtn->setDefault(true);
    layout->addWidget(btnBox);

    QObject::connect(btnBox, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    QObject::connect(btnBox, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);

    if (dlg.exec() != QDialog::Accepted) return;

    // ====== Build PokeJob =============================================
    PokeJob j;
    j.label = "Custom";
    j.schid = schid;
    j.clientID = clientID;
    j.mode = radioBurst->isChecked() ? PokeMode::Burst : PokeMode::Schedule;

    if (j.mode == PokeMode::Burst) {
        j.count = countB.value();
        j.burstDelayMs = burstDelay.value();
    } else {
        j.count = countS.value();
        j.intervalMinMs = minS.value();
        j.intervalMaxMs = maxS.value();
        if (j.intervalMaxMs < j.intervalMinMs) j.intervalMaxMs = j.intervalMinMs;
    }

    std::string text = msgEdit->toPlainText().toUtf8().toStdString();
    if (text.empty()) text = " ";

    if (chunkChk->isChecked()) {
        j.messages = chunkText(text, 100);
    } else {
        if (text.size() > 100) text.resize(100);
        j.messages = { text };
    }

    engine.start(j);
}

}  // namespace custom_dialog
