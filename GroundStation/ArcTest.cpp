// Copyright 2025 ESRI
//
// All rights reserved under the copyright laws of the United States
// and applicable international laws, treaties, and conventions.
//
// You may freely redistribute and use this sample code, with or
// without modification, provided you include the original copyright
// notice and use restrictions.
//
// See the Sample code usage restrictions document for further information.
//

// Other headers
#include "ArcTest.h"

// C++ API headers
#include "ArcGISTiledElevationSource.h"
#include "ElevationSourceListModel.h"
#include "MapTypes.h"
#include "Scene.h"
#include "SceneGraphicsView.h"
#include "Surface.h"
#include "GraphicsOverlay.h"
#include "GraphicsOverlayListModel.h"
#include "Graphic.h"
#include "GraphicListModel.h"
#include "Point.h"
#include "PolylineBuilder.h"
#include "SimpleMarkerSceneSymbol.h"
#include "SimpleLineSymbol.h"
#include "SolidStrokeSymbolLayer.h"
#include "MultilayerPolylineSymbol.h"
#include "Camera.h"
#include "SpatialReference.h"
#include "SceneViewTypes.h"
#include <SymbolTypes.h>
#include "OrbitGeoElementCameraController.h"

#include <LayerSceneProperties.h>
#include <QUrl>
#include <QTimer>
#include <QDebug>
#include <cmath>
#include <QLabel>
#include <QGridLayout>
#include <QWidget>
#include <QFont>
#include <QMenuBar>
#include <QStatusBar>
#include <QComboBox>
#include <QPushButton>
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QToolBar>
#include <QGroupBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFile>
#include <QTextStream>
#include <QTime>
#include <QDateTime>
#include <QMessageBox>

using namespace Esri::ArcGISRuntime;

ArcTest::ArcTest(QWidget *parent /*=nullptr*/)
    : QMainWindow(parent)
{
    // Create a scene using satellite imagery with labels
    Scene *scene = new Scene(BasemapStyle::ArcGISImagery, this);

    // create a new elevation source from Terrain3D rest service
    ArcGISTiledElevationSource *elevationSource
        = new ArcGISTiledElevationSource(QUrl("https://elevation3d.arcgis.com/arcgis/rest/services/"
                                              "WorldElevation3D/Terrain3D/ImageServer"),
                                         this);

    // add the elevation source to the scene to display elevation
    scene->baseSurface()->elevationSources()->append(elevationSource);

    // Create a scene view, and pass in the scene
    m_sceneView = new SceneGraphicsView(scene, this);

    // Set initial camera position for MATLAB-style 3D plot view
    // Position camera at an angle that shows the 3D trajectory clearly (adjusted for lower altitude trajectory)
    Camera initialCamera(m_launchLat - 0.02, m_launchLon - 0.05, 15000, 30, 60, 0); // Closer and steeper angle for lower altitude trajectory
    m_sceneView->setViewpointCameraAndWait(initialCamera);

    // Create graphics overlay for the rocket with ABSOLUTE 3D positioning
    m_graphicsOverlay = new GraphicsOverlay(this);
    
    // CRITICAL: Set surface placement to absolute for true 3D positioning
    // This tells ArcGIS to honor the Z coordinate instead of draping on surface
    m_graphicsOverlay->setSceneProperties(LayerSceneProperties(SurfacePlacement::Absolute));
    
    m_sceneView->graphicsOverlays()->append(m_graphicsOverlay);

    // Create the rocket symbol - MATLAB-style bright and visible
    SimpleMarkerSceneSymbol *rocketSymbol = new SimpleMarkerSceneSymbol(this);
    rocketSymbol->setColor(QColor(255, 50, 50)); // Bright red with slight orange tint
    rocketSymbol->setWidth(800.0);  // Even larger - 800 meters
    rocketSymbol->setHeight(1500.0); // Rocket-like proportions - 1.5km tall
    rocketSymbol->setDepth(800.0);
    
    // Create initial rocket position with EXPLICIT altitude mode
    // Use WGS84 with explicit Z coordinate interpretation
    SpatialReference sr = SpatialReference::wgs84();
    Point initialPosition(m_launchLon, m_launchLat, GROUND_LEVEL, sr);
    
    m_rocketGraphic = new Graphic(initialPosition, rocketSymbol, this);
    m_graphicsOverlay->graphics()->append(m_rocketGraphic);

    // Set up orbit camera controller to follow the rocket
    OrbitGeoElementCameraController* orbitController = new OrbitGeoElementCameraController(m_rocketGraphic, 2000.0, this); // 2km default distance (reduced for lower altitude trajectory)
    orbitController->setMinCameraDistance(50.0);
    orbitController->setMaxCameraDistance(10000.0); // Reduced max distance for better viewing of lower altitude trajectory
    orbitController->setTargetVerticalScreenFactor(0.33f);
    m_sceneView->setCameraController(orbitController);

    // Create 3D trajectory line with thick visible rendering using StrokeSymbolLayer
    m_trajectoryBuilder = new PolylineBuilder(SpatialReference::wgs84(), this);
    m_trajectoryBuilder->addPoint(initialPosition);
    
    // Create a SolidStrokeSymbolLayer for 3D line rendering
    SolidStrokeSymbolLayer *strokeLayer = new SolidStrokeSymbolLayer(this);
    strokeLayer->setColor(QColor(0, 200, 255)); // Bright cyan-blue
    strokeLayer->setWidth(50.0); // Thick line - 50 meters width for visibility
    strokeLayer->setLineStyle3D(StrokeSymbolLayerLineStyle3D::Tube); // 3D tube effect
    
    // Create multilayer polyline symbol with the stroke layer
    MultilayerPolylineSymbol *trajectorySymbol = new MultilayerPolylineSymbol(QList<SymbolLayer*>{strokeLayer}, this);
    
    // Create the trajectory graphic
    m_trajectoryGraphic = new Graphic(m_trajectoryBuilder->toGeometry(), trajectorySymbol, this);
    m_graphicsOverlay->graphics()->append(m_trajectoryGraphic);
    
    // Initialize trajectory points list
    m_trajectoryPoints.append(initialPosition);

    // Set up the animation timer
    m_animationTimer = new QTimer(this);
    connect(m_animationTimer, &QTimer::timeout, this, &ArcTest::updateRocketPosition);
    
    // Start the animation
    m_animationTimer->start(100); // Update every 100ms

    // Setup connection bar and telemetry dashboard
    setupConnectionBar();
    setupTelemetryDashboard();

    setCentralWidget(m_centralWidget);
}

ArcTest::~ArcTest() = default;

void ArcTest::updateRocketPosition()
{
    // Skip simulation if we're using real telemetry data
    if (m_useRealTelemetry) {
        return;
    }
    
    // Calculate position using physics equations for projectile motion
    double t = m_currentTime;
    
    // Static counter for trajectory markers and debug output
    static int updateCounter = 0;
    updateCounter++;
    
    // Convert initial velocities to lat/lon changes (approximate)
    double earthRadius = 6371000.0; // Earth radius in meters
    double latChange = (m_initialVelocityY * t) / earthRadius * (180.0 / M_PI);
    double lonChange = (m_initialVelocityX * t) / (earthRadius * cos(m_launchLat * M_PI / 180.0)) * (180.0 / M_PI);
    
    // Calculate new position
    double newLat = m_launchLat + latChange;
    double newLon = m_launchLon + lonChange;
    double newAlt = GROUND_LEVEL + (m_initialVelocityZ * t) + (0.5 * m_gravity * t * t); // Start from ground level
    
    // Apply maximum altitude constraint (10,000 ft = 3048m)
    if (newAlt > MAX_ALTITUDE) {
        newAlt = MAX_ALTITUDE;
    }
    
    // Ensure altitude doesn't go below ground level
    if (newAlt < GROUND_LEVEL) {
        newAlt = GROUND_LEVEL;
        // Reset the trajectory for continuous loop
        m_currentTime = 0;
        m_trajectoryPoints.clear();
        
        // Reset the trajectory polyline
        delete m_trajectoryBuilder;
        m_trajectoryBuilder = new PolylineBuilder(SpatialReference::wgs84(), this);
        
        Point resetPosition(m_launchLon, m_launchLat, GROUND_LEVEL, SpatialReference::wgs84());
        m_trajectoryBuilder->addPoint(resetPosition);
        m_trajectoryGraphic->setGeometry(m_trajectoryBuilder->toGeometry());
        m_trajectoryPoints.append(resetPosition);
        // Update GPS label to reset position
        updateGpsLabel(m_launchLat, m_launchLon, GROUND_LEVEL);
        
        // Reset mission tracking for simulation
        m_maxAltitude = 0.0;
        m_maxVelocity = 0.0;
        m_flightStarted = false;
        
        return;
    }
    
    // Update rocket position with explicit 3D coordinates
    SpatialReference sr = SpatialReference::wgs84();
    Point newPosition(newLon, newLat, newAlt, sr);
    m_rocketGraphic->setGeometry(newPosition);

    // Update GPS label
    updateGpsLabel(newLat, newLon, newAlt);
    
    // Calculate velocity for simulation display
    double currentVel = sqrt(pow(m_initialVelocityX, 2) + pow(m_initialVelocityY, 2) + pow(m_initialVelocityZ + m_gravity * t, 2));
    double currentAcc = 9.81; // Simplified for display
    
    // Update telemetry displays with simulated data
    m_altitudeLabel->setText(QString("Altitude: %1 m").arg(newAlt, 0, 'f', 1));
    m_velocityLabel->setText(QString("Velocity: %1 m/s").arg(currentVel, 0, 'f', 1));
    m_accelerationLabel->setText(QString("Acceleration: %1 m/s²").arg(currentAcc, 0, 'f', 1));
    m_temperatureLabel->setText(QString("Temperature: %1 °C").arg(25.0 - newAlt/1000.0 * 6.5, 0, 'f', 1)); // Temp lapse rate
    m_pressureLabel->setText(QString("Pressure: %1 hPa").arg(1013.0 * pow(1 - 0.0065 * newAlt / 288.15, 5.255), 0, 'f', 1)); // Barometric formula
    m_batteryLabel->setText(QString("Battery: %1 V").arg(12.0 - t * 0.01, 0, 'f', 1)); // Simulated battery drain
    
    // Update mission tracking
    if (newAlt > m_maxAltitude) {
        m_maxAltitude = newAlt;
        m_apogeeLabel->setText(QString("Max Altitude: %1 m").arg(m_maxAltitude, 0, 'f', 1));
    }
    
    if (currentVel > m_maxVelocity) {
        m_maxVelocity = currentVel;
        m_maxVelocityLabel->setText(QString("Max Velocity: %1 m/s").arg(m_maxVelocity, 0, 'f', 1));
    }
    
    // Start flight timer on first significant altitude
    if (!m_flightStarted && newAlt > 10010) {
        m_flightStarted = true;
        m_flightStartTime = QTime::currentTime();
    }
    
    if (m_flightStarted) {
        int elapsed = m_flightStartTime.secsTo(QTime::currentTime());
        m_flightTimeLabel->setText(QString("Flight Time: %1:%2").arg(elapsed / 60, 2, 10, QChar('0')).arg(elapsed % 60, 2, 10, QChar('0')));
    }
    
    // Log simulation data if recording
    if (m_isRecording && m_logStream) {
        *m_logStream << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz")
                    << "," << newLat << "," << newLon << "," << newAlt << "," << currentVel 
                    << "," << currentAcc << "," << (25.0 - newAlt/1000.0 * 6.5) << "," << (1013.0 * pow(1 - 0.0065 * newAlt / 288.15, 5.255)) << "," << (12.0 - t * 0.01) << "\n";
        m_logStream->flush();
    }
    
    // Add point to 3D trajectory line for MATLAB-style continuous plot
    m_trajectoryBuilder->addPoint(newPosition);
    
    // Update the trajectory graphic with the new polyline
    m_trajectoryGraphic->setGeometry(m_trajectoryBuilder->toGeometry());
    
    m_trajectoryPoints.append(newPosition);
    
    // Update time for next frame
    m_currentTime += m_timeStep;
    
    // Reset after 60 seconds for continuous demonstration
    if (m_currentTime > 60.0) {
        m_currentTime = 0;
        m_trajectoryPoints.clear();
        
        // Reset the trajectory polyline
        delete m_trajectoryBuilder;
        m_trajectoryBuilder = new PolylineBuilder(SpatialReference::wgs84(), this);
        
        Point resetPosition(m_launchLon, m_launchLat, GROUND_LEVEL, SpatialReference::wgs84());
        m_trajectoryBuilder->addPoint(resetPosition);
        m_trajectoryGraphic->setGeometry(m_trajectoryBuilder->toGeometry());
        m_trajectoryPoints.append(resetPosition);
        // Update GPS label to reset position
        updateGpsLabel(m_launchLat, m_launchLon, GROUND_LEVEL);
        
        // Reset mission tracking for simulation
        m_maxAltitude = 0.0;
        m_maxVelocity = 0.0;
        m_flightStarted = false;
    }
}

void ArcTest::updateGpsLabel(double lat, double lon, double alt)
{
    if (m_gpsLabel) {
        m_gpsLabel->setText(QString("Lat: %1\nLon: %2\nAlt: %3 m")
            .arg(lat, 0, 'f', 6)
            .arg(lon, 0, 'f', 6)
            .arg(alt, 0, 'f', 1));
    }
}

void ArcTest::setupConnectionBar()
{
    // Create menu bar
    m_menuBar = menuBar();
    
    // Create connection toolbar
    QToolBar* connectionToolbar = addToolBar("Connection");
    
    // Serial port selection
    connectionToolbar->addWidget(new QLabel("Port:"));
    m_serialPortCombo = new QComboBox(this);
    m_serialPortCombo->setMinimumWidth(150);
    connectionToolbar->addWidget(m_serialPortCombo);
    
    // Refresh ports button
    QPushButton* refreshButton = new QPushButton("Refresh", this);
    connect(refreshButton, &QPushButton::clicked, this, &ArcTest::refreshSerialPorts);
    connectionToolbar->addWidget(refreshButton);
    
    connectionToolbar->addSeparator();
    
    // Connect/Disconnect buttons
    m_connectButton = new QPushButton("Connect", this);
    connect(m_connectButton, &QPushButton::clicked, this, &ArcTest::connectToDevice);
    connectionToolbar->addWidget(m_connectButton);
    
    m_disconnectButton = new QPushButton("Disconnect", this);
    m_disconnectButton->setEnabled(false);
    connect(m_disconnectButton, &QPushButton::clicked, this, &ArcTest::disconnectFromDevice);
    connectionToolbar->addWidget(m_disconnectButton);
    
    connectionToolbar->addSeparator();
    
    // Connection status
    m_connectionStatus = new QLabel("Disconnected", this);
    m_connectionStatus->setStyleSheet("color: red; font-weight: bold;");
    connectionToolbar->addWidget(m_connectionStatus);
    
    // Initialize serial port
    m_serialPort = new QSerialPort(this);
    connect(m_serialPort, &QSerialPort::readyRead, this, &ArcTest::readSerialData);
    
    // Initial port refresh
    refreshSerialPorts();
}

void ArcTest::setupTelemetryDashboard()
{
    // --- UI Layout ---
    m_centralWidget = new QWidget(this);
    m_mainLayout = new QGridLayout(m_centralWidget);
    m_mainLayout->setSpacing(8);
    m_mainLayout->setContentsMargins(8,8,8,8);

    // Top left: ArcGIS map
    m_mainLayout->addWidget(m_sceneView, 0, 0);

    // Top right: GPS label
    m_gpsLabel = new QLabel("Lat: --\nLon: --\nAlt: --", this);
    QFont gpsFont = m_gpsLabel->font();
    gpsFont.setPointSize(14);
    m_gpsLabel->setFont(gpsFont);
    m_gpsLabel->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    m_mainLayout->addWidget(m_gpsLabel, 0, 1);

    // Bottom left: Real-time sensor data
    QWidget* sensorWidget = new QWidget(this);
    QGridLayout* sensorLayout = new QGridLayout(sensorWidget);
    
    // Sensor value labels
    m_altitudeLabel = new QLabel("Altitude: -- m", this);
    m_velocityLabel = new QLabel("Velocity: -- m/s", this);
    m_accelerationLabel = new QLabel("Acceleration: -- m/s²", this);
    m_temperatureLabel = new QLabel("Temperature: -- °C", this);
    m_pressureLabel = new QLabel("Pressure: -- hPa", this);
    m_batteryLabel = new QLabel("Battery: -- V", this);
    
    // Style the labels
    QFont sensorFont;
    sensorFont.setPointSize(12);
    sensorFont.setBold(true);
    
    QList<QLabel*> sensorLabels = {m_altitudeLabel, m_velocityLabel, m_accelerationLabel,
                                  m_temperatureLabel, m_pressureLabel, m_batteryLabel};
    
    for (int i = 0; i < sensorLabels.size(); ++i) {
        sensorLabels[i]->setFont(sensorFont);
        sensorLabels[i]->setStyleSheet("background-color: #2b2b2b; color: #00ff00; padding: 5px; border-radius: 3px;");
        sensorLayout->addWidget(sensorLabels[i], i / 2, i % 2);
    }
    
    m_mainLayout->addWidget(sensorWidget, 1, 0);
    
    // Bottom right: System status and controls
    QWidget* controlWidget = new QWidget(this);
    QVBoxLayout* controlLayout = new QVBoxLayout(controlWidget);
    
    // Data logging controls
    QGroupBox* loggingGroup = new QGroupBox("Data Logging", this);
    QHBoxLayout* loggingLayout = new QHBoxLayout(loggingGroup);
    
    m_recordButton = new QPushButton("Start Recording", this);
    m_recordButton->setStyleSheet("QPushButton { background-color: #ff4444; color: white; font-weight: bold; }");
    connect(m_recordButton, &QPushButton::clicked, this, &ArcTest::toggleRecording);
    
    m_recordingStatus = new QLabel("Not Recording", this);
    m_recordingStatus->setStyleSheet("color: red; font-weight: bold;");
    
    loggingLayout->addWidget(m_recordButton);
    loggingLayout->addWidget(m_recordingStatus);
    
    controlLayout->addWidget(loggingGroup);
    
    // Mission status
    QGroupBox* missionGroup = new QGroupBox("Mission Status", this);
    QVBoxLayout* missionLayout = new QVBoxLayout(missionGroup);
    
    m_flightTimeLabel = new QLabel("Flight Time: 00:00", this);
    m_apogeeLabel = new QLabel("Max Altitude: -- m", this);
    m_maxVelocityLabel = new QLabel("Max Velocity: -- m/s", this);
    
    missionLayout->addWidget(m_flightTimeLabel);
    missionLayout->addWidget(m_apogeeLabel);
    missionLayout->addWidget(m_maxVelocityLabel);
    
    controlLayout->addWidget(missionGroup);
    
    m_mainLayout->addWidget(controlWidget, 1, 1);

    // Set stretch so map and GPS label share space nicely
    m_mainLayout->setRowStretch(0, 2);
    m_mainLayout->setRowStretch(1, 1);
    m_mainLayout->setColumnStretch(0, 2);
    m_mainLayout->setColumnStretch(1, 1);
}

void ArcTest::refreshSerialPorts()
{
    m_serialPortCombo->clear();
    const auto ports = QSerialPortInfo::availablePorts();
    for (const auto& port : ports) {
        m_serialPortCombo->addItem(port.portName() + " - " + port.description(), port.portName());
    }
}

void ArcTest::connectToDevice()
{
    if (m_serialPortCombo->currentData().toString().isEmpty()) {
        QMessageBox::warning(this, "Warning", "No serial port selected!");
        return;
    }
    
    m_serialPort->setPortName(m_serialPortCombo->currentData().toString());
    m_serialPort->setBaudRate(QSerialPort::Baud115200);
    m_serialPort->setDataBits(QSerialPort::Data8);
    m_serialPort->setParity(QSerialPort::NoParity);
    m_serialPort->setStopBits(QSerialPort::OneStop);
    
    if (m_serialPort->open(QIODevice::ReadWrite)) {
        m_isConnected = true;
        m_useRealTelemetry = true;
        m_connectButton->setEnabled(false);
        m_disconnectButton->setEnabled(true);
        m_connectionStatus->setText("Connected");
        m_connectionStatus->setStyleSheet("color: green; font-weight: bold;");
    } else {
        QMessageBox::critical(this, "Error", "Failed to connect to serial port: " + m_serialPort->errorString());
    }
}

void ArcTest::disconnectFromDevice()
{
    m_serialPort->close();
    m_isConnected = false;
    m_useRealTelemetry = false;
    m_connectButton->setEnabled(true);
    m_disconnectButton->setEnabled(false);
    m_connectionStatus->setText("Disconnected");
    m_connectionStatus->setStyleSheet("color: red; font-weight: bold;");
}

void ArcTest::readSerialData()
{
    const QByteArray data = m_serialPort->readAll();
    // Parse your telemetry data here
    // Example: "LAT:34.0522,LON:-118.2437,ALT:1500,VEL:250"
    parseTelemtryData(data);
}

void ArcTest::toggleRecording()
{
    if (!m_isRecording) {
        // Start recording
        QString fileName = QString("telemetry_%1.csv").arg(QDateTime::currentDateTime().toString("yyyy-MM-dd_hh-mm-ss"));
        m_logFile = new QFile(fileName, this);
        
        if (m_logFile->open(QIODevice::WriteOnly | QIODevice::Text)) {
            m_logStream = new QTextStream(m_logFile);
            
            // Write CSV header
            *m_logStream << "Timestamp,Latitude,Longitude,Altitude,Velocity,Acceleration,Temperature,Pressure,Battery\n";
            
            m_isRecording = true;
            m_recordButton->setText("Stop Recording");
            m_recordButton->setStyleSheet("QPushButton { background-color: #44ff44; color: black; font-weight: bold; }");
            m_recordingStatus->setText("Recording...");
            m_recordingStatus->setStyleSheet("color: green; font-weight: bold;");
        } else {
            QMessageBox::critical(this, "Error", "Failed to create log file: " + m_logFile->errorString());
        }
    } else {
        // Stop recording
        if (m_logFile) {
            m_logFile->close();
            delete m_logStream;
            delete m_logFile;
            m_logStream = nullptr;
            m_logFile = nullptr;
        }
        
        m_isRecording = false;
        m_recordButton->setText("Start Recording");
        m_recordButton->setStyleSheet("QPushButton { background-color: #ff4444; color: white; font-weight: bold; }");
        m_recordingStatus->setText("Not Recording");
        m_recordingStatus->setStyleSheet("color: red; font-weight: bold;");
    }
}

void ArcTest::parseTelemtryData(const QByteArray& data)
{
    QString dataStr = QString::fromUtf8(data);
    
    // Example parsing for format: "LAT:34.0522,LON:-118.2437,ALT:1500,VEL:250,ACC:9.8,TEMP:25,PRES:1013,BAT:12.5"
    QStringList parts = dataStr.split(',');
    
    double lat = 0, lon = 0, alt = 0, vel = 0, acc = 0, temp = 0, pres = 0, bat = 0;
    
    for (const QString& part : parts) {
        if (part.startsWith("LAT:")) lat = part.mid(4).toDouble();
        else if (part.startsWith("LON:")) lon = part.mid(4).toDouble();
        else if (part.startsWith("ALT:")) alt = part.mid(4).toDouble();
        else if (part.startsWith("VEL:")) vel = part.mid(4).toDouble();
        else if (part.startsWith("ACC:")) acc = part.mid(4).toDouble();
        else if (part.startsWith("TEMP:")) temp = part.mid(4).toDouble();
        else if (part.startsWith("PRES:")) pres = part.mid(4).toDouble();
        else if (part.startsWith("BAT:")) bat = part.mid(4).toDouble();
    }
    
    // Update UI
    m_altitudeLabel->setText(QString("Altitude: %1 m").arg(alt, 0, 'f', 1));
    m_velocityLabel->setText(QString("Velocity: %1 m/s").arg(vel, 0, 'f', 1));
    m_accelerationLabel->setText(QString("Acceleration: %1 m/s²").arg(acc, 0, 'f', 1));
    m_temperatureLabel->setText(QString("Temperature: %1 °C").arg(temp, 0, 'f', 1));
    m_pressureLabel->setText(QString("Pressure: %1 hPa").arg(pres, 0, 'f', 1));
    m_batteryLabel->setText(QString("Battery: %1 V").arg(bat, 0, 'f', 1));
    
    // Update mission tracking
    if (alt > m_maxAltitude) {
        m_maxAltitude = alt;
        m_apogeeLabel->setText(QString("Max Altitude: %1 m").arg(m_maxAltitude, 0, 'f', 1));
    }
    
    if (vel > m_maxVelocity) {
        m_maxVelocity = vel;
        m_maxVelocityLabel->setText(QString("Max Velocity: %1 m/s").arg(m_maxVelocity, 0, 'f', 1));
    }
    
    // Start flight timer on first significant altitude
    if (!m_flightStarted && alt > 10) {
        m_flightStarted = true;
        m_flightStartTime = QTime::currentTime();
    }
    
    if (m_flightStarted) {
        int elapsed = m_flightStartTime.secsTo(QTime::currentTime());
        m_flightTimeLabel->setText(QString("Flight Time: %1:%2").arg(elapsed / 60, 2, 10, QChar('0')).arg(elapsed % 60, 2, 10, QChar('0')));
    }
    
    // Log data if recording
    if (m_isRecording && m_logStream) {
        *m_logStream << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz")
                    << "," << lat << "," << lon << "," << alt << "," << vel 
                    << "," << acc << "," << temp << "," << pres << "," << bat << "\n";
        m_logStream->flush();
    }
    
    // Update GPS label and rocket position
    updateGpsLabel(lat, lon, alt);
    
    // Update rocket position on map if we have valid coordinates
    if (lat != 0 && lon != 0) {
        updateRocketPositionFromTelemetry(lat, lon, alt);
    }
}

void ArcTest::updateRocketPositionFromTelemetry(double lat, double lon, double alt)
{
    // Update rocket position with real telemetry data
    SpatialReference sr = SpatialReference::wgs84();
    Point newPosition(lon, lat, alt, sr);
    m_rocketGraphic->setGeometry(newPosition);
    
    // Add point to trajectory
    m_trajectoryBuilder->addPoint(newPosition);
    m_trajectoryGraphic->setGeometry(m_trajectoryBuilder->toGeometry());
    m_trajectoryPoints.append(newPosition);
}
