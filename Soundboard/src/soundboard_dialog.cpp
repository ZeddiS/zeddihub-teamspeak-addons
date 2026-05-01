#include "soundboard_dialog.h"

#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QString>
#include <QtGui/QColor>
#include <QtWidgets/QColorDialog>
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
#include <QtWidgets/QToolButton>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

#include "audio_engine.h"

namespace {

QString configPath() {
    QString base = QDir::toNativeSeparators(QDir::homePath() + "/AppData/Roaming/TS3Client/plugins");
    QDir().mkpath(base);
    return QDir::toNativeSeparators(base + "/soundboard.json");
}

// Lighter / darker variant of a color for hover/border accents.
QString lightenColor(const QString& hex, int delta) {
    QColor c(hex);
    if (!c.isValid()) c = QColor("#5865f2");
    int r = qBound(0, c.red() + delta, 255);
    int g = qBound(0, c.green() + delta, 255);
    int b = qBound(0, c.blue() + delta, 255);
    return QString("#%1%2%3")
        .arg(r, 2, 16, QChar('0'))
        .arg(g, 2, 16, QChar('0'))
        .arg(b, 2, 16, QChar('0'));
}

// Pick black or white text based on background luminance.
QString textColorFor(const QString& hex) {
    QColor c(hex);
    if (!c.isValid()) return "#ffffff";
    double lum = (0.299 * c.red() + 0.587 * c.green() + 0.114 * c.blue()) / 255.0;
    return lum > 0.6 ? "#000000" : "#ffffff";
}

void rebuildGrid(QWidget* gridHost, std::vector<SoundSlot>* slotsVec, AudioEngine* engine);

QPushButton* makeTile(SoundSlot* slotPtr, QWidget* gridHost,
                       std::vector<SoundSlot>* slotsVec, AudioEngine* engine) {
    SoundSlot& slot = *slotPtr;
    auto* tile = new QPushButton(gridHost);
    tile->setFixedSize(160, 110);
    tile->setContextMenuPolicy(Qt::CustomContextMenu);

    auto applyStyle = [tile](const std::string& colorHex) {
        QString bg = QString::fromStdString(colorHex);
        QString hover = lightenColor(bg, 20);
        QString border = lightenColor(bg, -30);
        QString fg = textColorFor(bg);
        tile->setStyleSheet(QString(
            "QPushButton {"
            "  background-color: %1;"
            "  color: %4;"
            "  border: 2px solid %3;"
            "  border-radius: 8px;"
            "  font-size: 13px;"
            "  font-weight: bold;"
            "  padding: 6px;"
            "  text-align: center;"
            "}"
            "QPushButton:hover { background-color: %2; }"
            "QPushButton:pressed { background-color: %3; }"
        ).arg(bg, hover, border, fg));
    };
    applyStyle(slot.color);

    auto refreshLabel = [tile, slotPtr]() {
        QString name = slotPtr->name.empty()
            ? QString("Slot %1").arg(slotPtr->id)
            : QString::fromStdString(slotPtr->name);
        QString file = slotPtr->filePath.empty()
            ? QStringLiteral("(empty)")
            : QFileInfo(QString::fromStdString(slotPtr->filePath)).fileName();
        if (file.length() > 20) file = file.left(17) + "...";
        QString hk = QString("Hotkey #%1").arg(slotPtr->id);
        tile->setText(name + "\n" + file + "\n" + hk);
    };
    refreshLabel();

    // Left click -> play
    QObject::connect(tile, &QPushButton::clicked, [slotPtr, engine, tile]() {
        if (slotPtr->filePath.empty()) {
            tile->setToolTip("Right-click -> Browse to select a sound file.");
            return;
        }
        if (!slotPtr->decoded) {
            slotPtr->decoded = engine->loadFile(slotPtr->filePath);
        }
        if (!slotPtr->decoded || !slotPtr->decoded->ok) {
            tile->setToolTip("Failed to decode file. Use 16/24/32-bit PCM or 32-bit float WAV.");
            return;
        }
        engine->play(slotPtr->decoded, slotPtr->volume);
        tile->setToolTip(QString("Playing... (loaded %1 samples)").arg(slotPtr->decoded->samples.size()));
    });

    // Right-click context menu
    QObject::connect(tile, &QWidget::customContextMenuRequested,
        [slotPtr, slotsVec, engine, gridHost, tile, refreshLabel, applyStyle](const QPoint&) {
            QMenu menu(tile);
            QAction* aPlay   = menu.addAction("Play");
            QAction* aBrowse = menu.addAction("Browse sound file...");
            QAction* aRename = menu.addAction("Rename...");
            QAction* aColor  = menu.addAction("Change color...");
            menu.addSeparator();
            QAction* aClear  = menu.addAction("Clear sound");
            QAction* aDelete = menu.addAction("Delete tile");
            QAction* picked = menu.exec(QCursor::pos());

            if (picked == aPlay) {
                if (slotPtr->filePath.empty()) return;
                if (!slotPtr->decoded) slotPtr->decoded = engine->loadFile(slotPtr->filePath);
                if (slotPtr->decoded && slotPtr->decoded->ok) engine->play(slotPtr->decoded, slotPtr->volume);
            } else if (picked == aBrowse) {
                QString file = QFileDialog::getOpenFileName(
                    tile, "Select sound file", QString(),
                    "WAV files (*.wav);;All files (*.*)");
                if (file.isEmpty()) return;
                slotPtr->filePath = file.toStdString();
                slotPtr->decoded.reset();
                soundboard_dialog::saveSlots(*slotsVec);
                refreshLabel();
            } else if (picked == aRename) {
                bool ok = false;
                QString name = QInputDialog::getText(tile, "Rename tile", "Tile name:",
                                                     QLineEdit::Normal,
                                                     QString::fromStdString(slotPtr->name), &ok);
                if (!ok) return;
                slotPtr->name = name.toStdString();
                soundboard_dialog::saveSlots(*slotsVec);
                refreshLabel();
            } else if (picked == aColor) {
                QColor initial(QString::fromStdString(slotPtr->color));
                QColor c = QColorDialog::getColor(initial, tile, "Pick tile color");
                if (!c.isValid()) return;
                slotPtr->color = c.name().toStdString();
                applyStyle(slotPtr->color);
                soundboard_dialog::saveSlots(*slotsVec);
            } else if (picked == aClear) {
                slotPtr->filePath.clear();
                slotPtr->decoded.reset();
                soundboard_dialog::saveSlots(*slotsVec);
                refreshLabel();
            } else if (picked == aDelete) {
                for (auto it = slotsVec->begin(); it != slotsVec->end(); ++it) {
                    if (it->id == slotPtr->id) { slotsVec->erase(it); break; }
                }
                soundboard_dialog::saveSlots(*slotsVec);
                rebuildGrid(gridHost, slotsVec, engine);
            }
        });

    return tile;
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
    grid->setContentsMargins(4, 4, 4, 4);
    constexpr int kCols = 5;
    int row = 0, col = 0;
    for (auto& slot : *slotsVec) {
        grid->addWidget(makeTile(&slot, gridHost, slotsVec, engine), row, col);
        col++;
        if (col >= kCols) { col = 0; row++; }
    }
    grid->setRowStretch(row + 1, 1);
}

const char* kDefaultColors[] = {
    "#5865f2",  // blurple
    "#22a155",  // green
    "#e04a2a",  // red-orange
    "#f0b132",  // yellow
    "#9333ea",  // purple
    "#0ea5e9",  // cyan
    "#ec4899",  // pink
    "#64748b",  // slate
};

}  // namespace

namespace soundboard_dialog {

void loadSlots(std::vector<SoundSlot>& slotsOut) {
    slotsOut.clear();
    QFile f(configPath());
    if (!f.open(QIODevice::ReadOnly)) {
        for (int i = 1; i <= 8; ++i) {
            SoundSlot s;
            s.id = i;
            s.color = kDefaultColors[(i - 1) % 8];
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
        s.color = o.value("color").toString("#5865f2").toStdString();
        slotsOut.push_back(s);
    }
    if (slotsOut.empty()) {
        for (int i = 1; i <= 8; ++i) {
            SoundSlot s;
            s.id = i;
            s.color = kDefaultColors[(i - 1) % 8];
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
        o["color"] = QString::fromStdString(s.color);
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
    dlg.setWindowTitle(QStringLiteral("SoundBoard"));
    dlg.setMinimumSize(900, 560);

    auto* root = new QVBoxLayout(&dlg);
    root->setContentsMargins(12, 12, 12, 12);

    auto* tb = new QHBoxLayout();
    auto* addBtn = new QPushButton("+ Add Tile", &dlg);
    auto* stopBtn = new QPushButton("Stop All", &dlg);
    tb->addWidget(addBtn);
    tb->addWidget(stopBtn);
    tb->addStretch();
    auto* hint = new QLabel(QStringLiteral(
        "Left-click a tile to play. Right-click for options. "
        "Bind hotkeys in TS3 Settings -> Hotkeys."), &dlg);
    tb->addWidget(hint);
    root->addLayout(tb);

    auto* scroll = new QScrollArea(&dlg);
    scroll->setWidgetResizable(true);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    auto* viewport = new QWidget();
    scroll->setWidget(viewport);
    root->addWidget(scroll, 1);

    std::vector<SoundSlot>* slotsVecPtr = &slotsIn;
    AudioEngine* enginePtr = &engine;
    rebuildGrid(viewport, slotsVecPtr, enginePtr);

    QObject::connect(addBtn, &QPushButton::clicked, [&dlg, viewport, slotsVecPtr, enginePtr]() {
        if (slotsVecPtr->size() >= 32) {
            QMessageBox::information(&dlg, "SoundBoard",
                "Maximum of 32 tiles reached (one per pre-registered hotkey).");
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
        s.color = kDefaultColors[(nextId - 1) % 8];
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
