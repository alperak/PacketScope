#include "ui/MainWindow.hpp"

#include <QVBoxLayout>
#include <QHeaderView>
#include <QToolBar>
#include <QStatusBar>
#include <QMessageBox>
#include <QFont>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , stackedWidget_(new QStackedWidget(this))
    , packetListModel_(nullptr)
    , updateTimer_(new QTimer(this))
{
    setWindowTitle(QStringLiteral("PacketScope"));
    resize(DEFAULT_WINDOW_WIDTH, DEFAULT_WINDOW_HEIGHT);

    move(QGuiApplication::primaryScreen()->availableGeometry().center() - rect().center());

    setCentralWidget(stackedWidget_);

    setupWelcomeScreen();
    setupCaptureScreen();

    showWelcomeScreen();

    statusLabel_ = new QLabel(QStringLiteral("Ready"), this);
    statusBar()->addWidget(statusLabel_, 1);  // Stretch factor = 1

    packetCountLabel_ = new QLabel(QStringLiteral("Captured: 0 | Processed: 0"), this);
    statusBar()->addPermanentWidget(packetCountLabel_);

    // Connect timer timeout to UI update slot
    connect(updateTimer_, &QTimer::timeout, this, &MainWindow::onUpdateUI);
}

MainWindow::~MainWindow() {
    controller_.stop();
}

void MainWindow::setupWelcomeScreen() {
    welcomeWidget_ = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(welcomeWidget_);

    QLabel* titleLabel = new QLabel(QStringLiteral("Welcome to PacketScope"));
    QFont titleFont = titleLabel->font();
    titleFont.setPointSize(TITLE_FONT_SIZE);
    titleFont.setBold(true);
    titleLabel->setFont(titleFont);
    titleLabel->setAlignment(Qt::AlignCenter);

    QLabel* subtitleLabel = new QLabel(
        QStringLiteral("Double-click a network interface to start capturing")
    );
    subtitleLabel->setAlignment(Qt::AlignCenter);

    deviceListWidget_ = new QListWidget();
    deviceListWidget_->setAlternatingRowColors(true);
    deviceListWidget_->setFont(QFont("Monospace", 10));

    populateDeviceList();

    // Connect double click signal to slot
    // When user double clicks a device, start capture
    connect(deviceListWidget_, &QListWidget::itemDoubleClicked,
            this, &MainWindow::onDeviceDoubleClicked);

    layout->addSpacing(50);
    layout->addWidget(titleLabel);
    layout->addWidget(subtitleLabel);
    layout->addSpacing(20);
    layout->addWidget(deviceListWidget_);
    layout->addSpacing(50);

    stackedWidget_->addWidget(welcomeWidget_);
}

void MainWindow::populateDeviceList() {
    deviceListWidget_->clear();

    const auto devices = controller_.listAvailableDevices();

    for (const auto& device : devices) {
        QString text = QString::fromStdString(device.name);
        if (!device.description.empty()) {
            text += " (" + QString::fromStdString(device.description) + ")";
        }

        QListWidgetItem* item = new QListWidgetItem(text);

        item->setData(Qt::UserRole, QString::fromStdString(device.name));

        deviceListWidget_->addItem(item);
    }
}

void MainWindow::setupCaptureScreen() {
    captureWidget_ = new QWidget();
    QVBoxLayout* mainLayout = new QVBoxLayout(captureWidget_);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    QToolBar* toolbar = new QToolBar();
    toolbar->setMovable(false);

    startAction_ = toolbar->addAction(QStringLiteral("Start"));
    stopAction_ = toolbar->addAction(QStringLiteral("Stop"));
    restartAction_ = toolbar->addAction(QStringLiteral("Restart"));

    // Initial state: all disabled until device is selected
    startAction_->setEnabled(false);
    stopAction_->setEnabled(false);
    restartAction_->setEnabled(false);

    connect(startAction_, &QAction::triggered, this, &MainWindow::onStartCapture);
    connect(stopAction_, &QAction::triggered, this, &MainWindow::onStopCapture);
    connect(restartAction_, &QAction::triggered, this, &MainWindow::onRestartCapture);

    mainLayout->addWidget(toolbar);

    QSplitter* mainSplitter = new QSplitter(Qt::Vertical);

    packetTableView_ = new QTableView();
    packetTableView_->setSelectionBehavior(QAbstractItemView::SelectRows);
    packetTableView_->setSelectionMode(QAbstractItemView::SingleSelection);
    packetTableView_->setAlternatingRowColors(true);
    packetTableView_->verticalHeader()->setVisible(false);

    packetListModel_ = new PacketListModel(controller_.getStore(), this);
    packetTableView_->setModel(packetListModel_);

    QHeaderView* header = packetTableView_->horizontalHeader();
    header->setStretchLastSection(false);

    header->resizeSection(static_cast<int>(PacketListModel::ColumnType::Id), COLUMN_WIDTH_ID);
    header->resizeSection(static_cast<int>(PacketListModel::ColumnType::Time), COLUMN_WIDTH_TIME);
    header->resizeSection(static_cast<int>(PacketListModel::ColumnType::Source), COLUMN_WIDTH_SOURCE);
    header->resizeSection(static_cast<int>(PacketListModel::ColumnType::Destination), COLUMN_WIDTH_DESTINATION);
    header->resizeSection(static_cast<int>(PacketListModel::ColumnType::Protocol), COLUMN_WIDTH_PROTOCOL);
    header->resizeSection(static_cast<int>(PacketListModel::ColumnType::Length), COLUMN_WIDTH_LENGTH);

    header->setStretchLastSection(true);


    // When user selects a packet, update detail views
    connect(packetTableView_->selectionModel(), &QItemSelectionModel::currentRowChanged,
            this, &MainWindow::onPacketSelected);

    // Add table to main splitter
    mainSplitter->addWidget(packetTableView_);

    QSplitter* detailSplitter = new QSplitter(Qt::Horizontal);

    layerTreeWidget_ = new QTreeWidget();
    layerTreeWidget_->setHeaderLabel(QStringLiteral("Packet Details"));
    layerTreeWidget_->setFont(QFont("Monospace", 9));
    detailSplitter->addWidget(layerTreeWidget_);

    hexView_ = new QPlainTextEdit();
    hexView_->setReadOnly(true);
    hexView_->setFont(QFont("Monospace", 9));
    hexView_->setLineWrapMode(QPlainTextEdit::NoWrap);
    detailSplitter->addWidget(hexView_);

    detailSplitter->setSizes({400, 400});

    mainSplitter->addWidget(detailSplitter);

    mainSplitter->setSizes({500, 300});

    mainLayout->addWidget(mainSplitter);

    stackedWidget_->addWidget(captureWidget_);
}

void MainWindow::onDeviceDoubleClicked(QListWidgetItem* item) {
    currentDeviceName_ = item->data(Qt::UserRole).toString();

    if (controller_.start(currentDeviceName_.toStdString())) {
        hasDevice_ = true;
        showCaptureScreen();
        updateTimer_->start(UI_UPDATE_INTERVAL_MS);
        statusLabel_->setText(QStringLiteral("Capturing on: ") + currentDeviceName_);
        updateButtonStates();
    } else {
        QMessageBox::warning(this, QStringLiteral("Error"),
                            QStringLiteral("Failed to start capture on ") + currentDeviceName_);
    }
}

void MainWindow::onStopCapture() {
    updateTimer_->stop();
    controller_.stop();
    statusLabel_->setText(QStringLiteral("Capture paused"));
    updateButtonStates();
}

void MainWindow::onStartCapture() {
    if (controller_.start(currentDeviceName_.toStdString())) {
        updateTimer_->start(UI_UPDATE_INTERVAL_MS);
        statusLabel_->setText(QStringLiteral("Capturing on: ") + currentDeviceName_);
        updateButtonStates();
    } else {
        QMessageBox::warning(this, QStringLiteral("Error"),
                            QStringLiteral("Failed to resume capture"));
    }
}

void MainWindow::onRestartCapture() {
    if (controller_.restart()) {
        packetListModel_->reset();
        updateTimer_->start(UI_UPDATE_INTERVAL_MS);
        statusLabel_->setText(QStringLiteral("Restarted on: ") + currentDeviceName_);
        updateButtonStates();
    } else {
        QMessageBox::warning(this, QStringLiteral("Error"),
                            QStringLiteral("Failed to restart capture"));
    }
}

void MainWindow::updateButtonStates() {
    bool isRunning = controller_.isRunning();

    startAction_->setEnabled(!isRunning && hasDevice_);
    stopAction_->setEnabled(isRunning);
    restartAction_->setEnabled(hasDevice_);
}

void MainWindow::showWelcomeScreen() {
    stackedWidget_->setCurrentWidget(welcomeWidget_);
}

void MainWindow::showCaptureScreen() {
    stackedWidget_->setCurrentWidget(captureWidget_);
}

void MainWindow::onUpdateUI() {
    packetListModel_->refresh();

    packetCountLabel_->setText(
        QString("Captured: %1 | Processed: %2")
            .arg(controller_.capturedCount())
            .arg(controller_.processedCount())
    );
}

void MainWindow::onPacketSelected(const QModelIndex& current, const QModelIndex& previous) {
    Q_UNUSED(previous);

    // Early return for invalid selection
    if (!current.isValid()) {
        return;
    }

    const int packetId = packetListModel_->getPacketId(current.row());

    try {
        const auto packet = controller_.getStore()->getById(packetId);

        layerTreeWidget_->clear();

        // Each string in layerSummaries represents one protocol layer
        for (const auto& layer : packet.layerSummaries) {
            QTreeWidgetItem* item = new QTreeWidgetItem(layerTreeWidget_);
            item->setText(0, QString::fromStdString(layer));
        }

        QString hexText;
        const auto& data = packet.rawData;

        // Process bytes in lines of HEX_BYTES_PER_LINE(16)
        for (std::size_t i = 0; i < data.size(); i += HEX_BYTES_PER_LINE) {
            // Offset
            hexText += QString("%1  ").arg(i, 4, 16, QChar('0'));

            QString hexPart;
            QString asciiPart;

            // Hex and ASCII Parts
            for (std::size_t j = 0; j < HEX_BYTES_PER_LINE; ++j) {
                if (i + j < data.size()) {
                    const uint8_t byte = data[i + j];

                    // Hex representation
                    hexPart += QString("%1 ").arg(byte, 2, 16, QChar('0'));

                    // ASCII representation
                    asciiPart += (byte >= 32 && byte < 127)
                        ? QChar(static_cast<char>(byte))
                        : QChar('.');
                } else {
                    // Padding for incomplete last line
                    hexPart += "   ";
                }

                // Add extra space after 8th byte for visual separator
                if (j == HEX_SEPARATOR_POS) {
                    hexPart += " ";
                }
            }

            // Combine offset, hex and ASCII parts
            hexText += hexPart + " " + asciiPart + "\n";
        }

        hexView_->setPlainText(hexText);

    } catch (const std::exception&) {
        // Exceptions from PacketStore
        // Silently ignore to avoid crashing UI
    }
}