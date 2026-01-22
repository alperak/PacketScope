#ifndef MAINWINDOW_HPP
#define MAINWINDOW_HPP

#include "core/PipelineController.hpp"
#include "ui/PacketListModel.hpp"

#include <QMainWindow>
#include <QListWidget>
#include <QTableView>
#include <QTreeWidget>
#include <QPlainTextEdit>
#include <QSplitter>
#include <QStackedWidget>
#include <QTimer>
#include <QLabel>

/**
 * @brief Main application window for PacketScope network packet analyzer
 */
class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    /**
     * @brief Constructs the main window
     *
     * Initializes the UI components, sets up both welcome and capture screens,
     * and configures the status bar. The window starts on the welcome screen.
     */
    explicit MainWindow(QWidget* parent = nullptr);

    ~MainWindow() override;

    MainWindow(const MainWindow&) = delete;
    MainWindow& operator=(const MainWindow&) = delete;
    MainWindow(MainWindow&&) = delete;
    MainWindow& operator=(MainWindow&&) = delete;

private slots:
    /**
     * @brief Handles double click on device in welcome screen
     *
     * Starts packet capture on the selected device and transitions to
     * the capture screen. Shows an error dialog if capture fails.
     */
    void onDeviceDoubleClicked(QListWidgetItem* item);

    /**
     * @brief Handles stop button click in capture screen
     *
     * Stops the packet capture pipeline and UI update timer.
     */
    void onStopCapture();

    /**
     * @brief Periodic UI update handler which called by QTimer
     *
     * Refreshes the packet table model and updates status bar statistics.
     *
     * @note Called every UI_UPDATE_INTERVAL_MS milliseconds during capture
     */
    void onUpdateUI();

    /**
     * @brief Handles packet selection in the table view
     *
     * Updates the layer tree and hex viewer to display details of the
     * selected packet. Does nothing if selection is invalid.
     */
    void onPacketSelected(const QModelIndex& current, const QModelIndex& previous);

private:
    /**
     * @brief Sets up the welcome screen UI components
     */
    void setupWelcomeScreen();

    /**
     * @brief Sets up the capture screen UI components
     */
    void setupCaptureScreen();

    /**
     * @brief Switches to the welcome screen
     */
    void showWelcomeScreen();

    /**
     * @brief Switches to the capture screen
     */
    void showCaptureScreen();

    /**
     * @brief Populates the device list with available interfaces
     */
    void populateDeviceList();

    /// UI update interval in milliseconds
    static constexpr int UI_UPDATE_INTERVAL_MS = 100;

    /// Default window width in pixels
    static constexpr int DEFAULT_WINDOW_WIDTH = 1200;

    /// Default window height in pixels
    static constexpr int DEFAULT_WINDOW_HEIGHT = 800;

    /// Title font size in points
    static constexpr int TITLE_FONT_SIZE = 18;

    /// Packet table column widths
    static constexpr int COLUMN_WIDTH_ID = 60;
    static constexpr int COLUMN_WIDTH_TIME = 120;
    static constexpr int COLUMN_WIDTH_SOURCE = 140;
    static constexpr int COLUMN_WIDTH_DESTINATION = 140;
    static constexpr int COLUMN_WIDTH_PROTOCOL = 70;
    static constexpr int COLUMN_WIDTH_LENGTH = 60;

    /// Hex dump format constants
    static constexpr int HEX_BYTES_PER_LINE = 16;  ///< Number of bytes per line in hex view
    static constexpr int HEX_SEPARATOR_POS = 7;    ///< Position to insert extra space

    /// Manages packet capture, processing and storage pipeline
    PipelineController controller_;

    /// Container widget that switches between welcome and capture screens
    QStackedWidget* stackedWidget_;

    // Welcome screen widgets
    QWidget* welcomeWidget_;          ///< Container for welcome screen
    QListWidget* deviceListWidget_;   ///< List of available devices

    // Capture screen widgets
    QWidget* captureWidget_;          ///< Container for capture screen
    QTableView* packetTableView_;     ///< Table displaying captured packets
    QTreeWidget* layerTreeWidget_;    ///< Tree showing protocol layers
    QPlainTextEdit* hexView_;         ///< Hex dump of raw packet data

    /// Model for packet table view
    PacketListModel* packetListModel_;

    // Status bar widgets
    QLabel* statusLabel_;             ///< Shows current capture status
    QLabel* packetCountLabel_;        ///< Shows packet count statistics

    /// Timer for periodic UI updates during capture
    QTimer* updateTimer_;
};

#endif // MAINWINDOW_HPP
