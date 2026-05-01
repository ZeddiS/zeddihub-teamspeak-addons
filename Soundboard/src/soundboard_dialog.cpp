#include "soundboard_dialog.h"

#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QString>
#include <QtWidgets/QDialog>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QFrame>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QInputDialog>
#include <QtWidgets/QLabel>
#include <QtWidgets/QMenu>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QScrollArea>
#include <QtWidgets/QSlider>
#include <QtWidgets/QToolButton>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

#include "audio_engine.h"

namespace {

// No custom stylesheet - inherit TS3 client's native theme via Qt parent
// palette. Dialogs look exactly like TS3's other windows.

QString configPath() {
    QString base = QDir::toNativeSeparators(QDir::homePath() + "/AppData/Roaming/TS3Client/plugins");
    QDir().mkpath(base);
    return QDir::toNativeSeparators(base + "/soundboard.json");
}

void rebuildGrid(QWidget* gridHost, std::vector<SoundSlot>* slotsVec, AudioEngine* engine);

QWidget* makeSlotCard(SoundSlot* slotPtr, QWidget* gridHost,
                     std::vector<SoundSlot>* slotsVec, AudioEngine* engine) {
    SoundSlot& slot = *slotPtr;
    auto* card = new QFrame(gridHost);
    card->setObjectName("slotCard");
    card->setMinimumWidth(180);

    auto* v = new QVBoxLayout(card);
    v->setContentsMargins(10, 10, 10, 10);
    v->setSpacing(6);

    auto* nameLbl = new QLabel(QString::fromStdString(slot.name.empty()
        ? "(slot " + std::to_string(slot.id) + ")"
        : slot.name), card);
    nameLbl->setObjectName("slotName");
    nameLbl->setWordWrap(true);
    v->addWidget(nameLbl);

    QString pathDisplay = slot.filePath.empty()
        ? QStringLiteral("(no file)")
        : QFileInfo(QString::fromStdString(slot.filePath)).fileName();
    auto* pathLbl = new QLabel(pathDisplay, card);
    pathLbl->setObjectName("slotPath");
    pathLbl->setWordWrap(true);
    v->addWidget(pathLbl);

    auto* hkLbl = new QLabel(QString("Hotkey: soundboard_play_%1").arg(slot.id), card);
    hkLbl->setObjectName("slotHotkey");
    v->addWidget(hkLbl);

    auto* volRow = new QHBoxLayout();
    auto* volSlider = new QSlider(Qt::Horizontal, card);
    volSlider->setRange(0, 200);
    volSlider->setValue((int)(slot.volume * 100));
    volRow->addWidget(new QLabel("Vol:", card));
    volRow->addWidget(volSlider, 1);
    v->addLayout(volRow);

    QObject::connect(volSlider, &QSlider::valueChanged, [slotPtr, slotsVec](int val) {
        slotPtr->volume = (float)val / 100.0f;
        soundboard_dialog::saveSlots(*slotsVec);
    });

    auto* actions = new QHBoxLayout();
    auto* playBtn = new QPushButton("Play", card);
    playBtn->setObjectName("playBtn");
    auto* browseBtn = new QPushButton("Browse...", card);
    auto* menuBtn = new QToolButton(card);
    menuBtn->setText("...");
    menuBtn->setPopupMode(QToolButton::InstantPopup);
    actions->addWidget(playBtn);
    actions->addWidget(browseBtn);
    actions->addWidget(menuBtn);
    v->addLayout(actions);

    QObject::connect(playBtn, &QPushButton::clicked, [slotPtr, engine]() {
        if (slotPtr->filePath.empty()) return;
        if (!slotPtr->decoded) slotPtr->decoded = engine->loadFile(slotPtr->filePath);
        if (slotPtr->decoded && slotPtr->decoded->ok) {
            engine->play(slotPtr->decoded, slotPtr->volume);
        }
    });

    QObject::connect(browseBtn, &QPushButton::clicked, [card, slotPtr, slotsVec, pathLbl]() {
        QString file = QFileDialog::getOpenFileName(
            card, "Select sound file", QString(),
            "Audio (*.wav);;All (*.*)");
        if (file.isEmpty()) return;
        slotPtr->filePath = file.toStdString();
        slotPtr->decoded.reset();
        soundboard_dialog::saveSlots(*slotsVec);
        pathLbl->setText(QFileInfo(file).fileName());
    });

    auto* menu = new QMenu(menuBtn);
    auto* renameAct = menu->addAction("Rename...");
    auto* clearAct = menu->addAction("Clear sound");
    menu->addSeparator();
    auto* deleteAct = menu->addAction("Delete slot");
    menuBtn->setMenu(menu);

    QObject::connect(renameAct, &QAction::triggered, [card, slotPtr, nameLbl, slotsVec]() {
        bool ok = false;
        QString name = QInputDialog::getText(card, "Rename slot", "New name:",
                                             QLineEdit::Normal,
                                             QString::fromStdString(slotPtr->name), &ok);
        if (!ok) return;
        slotPtr->name = name.toStdString();
        nameLbl->setText(name.isEmpty()
            ? QString("(slot %1)").arg(slotPtr->id) : name);
        soundboard_dialog::saveSlots(*slotsVec);
    });
    QObject::connect(clearAct, &QAction::triggered, [slotPtr, pathLbl, slotsVec]() {
        slotPtr->filePath.clear();
        slotPtr->decoded.reset();
        pathLbl->setText("(no file)");
        soundboard_dialog::saveSlots(*slotsVec);
    });
    QObject::connect(deleteAct, &QAction::triggered, [slotPtr, gridHost, slotsVec, engine]() {
        for (auto it = slotsVec->begin(); it != slotsVec->end(); ++it) {
            if (it->id == slotPtr->id) {
                slotsVec->erase(it);
                break;
            }
        }
        soundboard_dialog::saveSlots(*slotsVec);
        rebuildGrid(gridHost, slotsVec, engine);
    });

    return card;
}

void rebuildGrid(QWidget* gridHost, std::vector<SoundSlot>* slotsVec, AudioEngine* engine) {
    if (gridHost->layout()) {
        QLayoutItem* item;
        while ((item = gridHost->layout()->takeAt(0)) != nullptr) {
            if (item->widget()) item->widget()->deleteLater();
            delete item;
        }
        delete gridHost->layout();
    }
    auto* grid = new QGridLayout(gridHost);
    grid->setSpacing(8);
    constexpr int kCols = 4;
    int row = 0, col = 0;
    for (auto& slot : *slotsVec) {
        grid->addWidget(makeSlotCard(&slot, gridHost, slotsVec, engine), row, col);
        col++;
        if (col >= kCols) { col = 0; row++; }
    }
}

}  // namespace

namespace soundboard_dialog {

void loadSlots(std::vector<SoundSlot>& slotsOut) {
    slotsOut.clear();
    QFile f(configPath());
    if (!f.open(QIODevice::ReadOnly)) {
        for (int i = 1; i <= 8; ++i) {
            SoundSlot s;
            s.id = i;
            slotsOut.push_back(s);
        }
        return;
    }
    QJsonDocument doc = QJsonDocument::fromJson(f.readAll());
    QJsonArray arr = doc.object().value("slots").toArray();
    for (const auto& v : arr) {
        QJsonObject o = v.toObject();
        SoundSlot s;
        s.id = o.value("id").toInt();
        s.name = o.value("name").toString().toStdString();
        s.filePath = o.value("path").toString().toStdString();
        s.volume = (float)o.value("volume").toDouble(1.0);
        slotsOut.push_back(s);
    }
    if (slotsOut.empty()) {
        for (int i = 1; i <= 8; ++i) {
            SoundSlot s;
            s.id = i;
            slotsOut.push_back(s);
        }
    }
}

void saveSlots(const std::vector<SoundSlot>& slotsIn) {
    QJsonArray arr;
    for (const auto& s : slotsIn) {
        QJsonObject o;
        o["id"] = s.id;
        o["name"] = QString::fromStdString(s.name);
        o["path"] = QString::fromStdString(s.filePath);
        o["volume"] = (double)s.volume;
        arr.append(o);
    }
    QJsonObject root;
    root["slots"] = arr;
    QJsonDocument doc(root);
    QFile f(configPath());
    if (f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        f.write(doc.toJson(QJsonDocument::Indented));
    }
}

void open(std::vector<SoundSlot>& slotsIn, AudioEngine& engine) {
    QDialog dlg;
    dlg.setWindowTitle(QStringLiteral("ZeddiHub Soundboard"));
    dlg.setMinimumSize(820, 520);

    auto* root = new QVBoxLayout(&dlg);
    root->setContentsMargins(12, 12, 12, 12);

    auto* tb = new QHBoxLayout();
    auto* addBtn = new QPushButton("+ Add Slot", &dlg);
    addBtn->setObjectName("addSlotBtn");
    auto* stopBtn = new QPushButton("Stop All", &dlg);
    stopBtn->setObjectName("stopAllBtn");
    tb->addWidget(addBtn);
    tb->addWidget(stopBtn);
    tb->addStretch();
    auto* hint = new QLabel(QStringLiteral(
        "Bind hotkeys in TS3 Settings -> Hotkeys -> Plugins -> ZeddiHub Soundboard"), &dlg);
    hint->setStyleSheet(QStringLiteral("color: #a3a6aa; font-size: 11px;"));
    tb->addWidget(hint);
    root->addLayout(tb);

    auto* scroll = new QScrollArea(&dlg);
    scroll->setWidgetResizable(true);
    auto* viewport = new QWidget();
    viewport->setObjectName("viewport");
    scroll->setWidget(viewport);
    root->addWidget(scroll, 1);

    std::vector<SoundSlot>* slotsVecPtr = &slotsIn;
    AudioEngine* enginePtr = &engine;
    rebuildGrid(viewport, slotsVecPtr, enginePtr);

    QObject::connect(addBtn, &QPushButton::clicked, [&dlg, viewport, slotsVecPtr, enginePtr]() {
        if (slotsVecPtr->size() >= 32) {
            QMessageBox::information(&dlg, "Soundboard",
                "Maximum of 32 slots reached (limited by pre-registered hotkeys).");
            return;
        }
        int nextId = 1;
        for (int candidate = 1; candidate <= 32; ++candidate) {
            bool taken = false;
            for (const auto& s : *slotsVecPtr) if (s.id == candidate) { taken = true; break; }
            if (!taken) { nextId = candidate; break; }
        }
        SoundSlot s;
        s.id = nextId;
        slotsVecPtr->push_back(s);
        soundboard_dialog::saveSlots(*slotsVecPtr);
        rebuildGrid(viewport, slotsVecPtr, enginePtr);
    });

    QObject::connect(stopBtn, &QPushButton::clicked, [enginePtr]() {
        enginePtr->stopAll();
    });

    dlg.exec();
    saveSlots(slotsIn);
}

}  // namespace soundboard_dialog
