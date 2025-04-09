/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import QGroundControl
import QGroundControl.FactSystem
import QGroundControl.FactControls
import QGroundControl.Controls
import QGroundControl.ScreenTools

SettingsPage {
    property var    _settingsManager:            QGroundControl.settingsManager
    property var    _videoManager:              QGroundControl.videoManager
    property var    _videoSettings:             _settingsManager.videoSettings
    property string _videoSource:               _videoSettings.videoSource.rawValue
    property string _videoSource1:               _videoSettings.videoSource1.rawValue
    property bool   _isGST:                     _videoManager.gstreamerEnabled
    property bool   _isStreamSource:            _videoManager.isStreamSource
    property bool   _isStreamSource1:            true//_videoManager.isStreamSource1

    property bool   _isUDP264:                  _isStreamSource && (_videoSource === _videoSettings.udp264VideoSource)
    property bool   _isUDP265:                  _isStreamSource && (_videoSource === _videoSettings.udp265VideoSource)
    property bool   _isRTSP:                    _isStreamSource && (_videoSource === _videoSettings.rtspVideoSource)
    property bool   _isTCP:                     _isStreamSource && (_videoSource === _videoSettings.tcpVideoSource)
    property bool   _isMPEGTS:                  _isStreamSource && (_videoSource === _videoSettings.mpegtsVideoSource)
    property bool   _videoSourceDisabled:       _videoSource === _videoSettings.disabledVideoSource
    property bool   _videoSourceDisabled1:       _videoSource1 === _videoSettings.disabledVideoSource1

    property bool   _isUDP264_1:                  _isStreamSource1 && (_videoSource1 === _videoSettings.udp264VideoSource1)
    property bool   _isUDP265_1:                  _isStreamSource1 && (_videoSource1 === _videoSettings.udp265VideoSource1)
    property bool   _isRTSP_1:                    _isStreamSource1 && (_videoSource1 === _videoSettings.rtspVideoSource1)
    property bool   _isTCP_1:                     _isStreamSource1 && (_videoSource1 === _videoSettings.tcpVideoSource1)
    property bool   _isMPEGTS_1:                  _isStreamSource1 && (_videoSource1 === _videoSettings.mpegtsVideoSource1)


    property bool   _videoAutoStreamConfig:     _videoManager.autoStreamConfigured
    property bool   _requiresUDPPort:           _isUDP264 || _isUDP265 || _isMPEGTS
    property bool   _requiresUDPPort1:           _isUDP264_1 || _isUDP265_1 || _isMPEGTS_1

        property real   _urlFieldWidth:             ScreenTools.defaultFontPixelWidth * 25

    SettingsGroupLayout {
        Layout.fillWidth:   true
        heading:            qsTr("Video Source 1")
        headingDescription: _videoAutoStreamConfig ? qsTr("Mavlink camera stream is automatically configured") : ""
        enabled:            !_videoAutoStreamConfig

        LabelledFactComboBox {
            Layout.fillWidth:   true
            label:              qsTr("Source ")
            indexModel:         false
            fact:               _videoSettings.videoSource
            visible:            fact.visible
        }
    }


    SettingsGroupLayout {
        Layout.fillWidth:   true
        heading:            qsTr("Connection 1")
        visible:            !_videoSourceDisabled && !_videoAutoStreamConfig && (_isTCP || _isRTSP || _requiresUDPPort)

        LabelledFactTextField {
            Layout.fillWidth:           true
            textFieldPreferredWidth:    _urlFieldWidth
            label:                      qsTr("RTSP URL")
            fact:                       _videoSettings.rtspUrl
            visible:                    _isRTSP && _videoSettings.rtspUrl.visible
        }


        LabelledFactTextField {
            Layout.fillWidth:           true
            label:                      qsTr("TCP URL")
            textFieldPreferredWidth:    _urlFieldWidth
            fact:                       _videoSettings.tcpUrl
            visible:                    _isTCP && _videoSettings.tcpUrl.visible
        }

        LabelledFactTextField {
            Layout.fillWidth:   true
            label:              qsTr("UDP Port")
            fact:               _videoSettings.udpPort
            visible:            _requiresUDPPort && _videoSettings.udpPort.visible
        }
    }

    SettingsGroupLayout {
        Layout.fillWidth:   true
        heading:            qsTr("Settings 1")
        visible:            !_videoSourceDisabled

        LabelledFactTextField {
            Layout.fillWidth:   true
            label:              qsTr("Aspect Ratio")
            fact:               _videoSettings.aspectRatio
            visible:            !_videoAutoStreamConfig && _isStreamSource && _videoSettings.aspectRatio.visible
        }

        FactCheckBoxSlider {
            Layout.fillWidth:   true
            text:               qsTr("Stop recording when disarmed")
            fact:               _videoSettings.disableWhenDisarmed
            visible:            !_videoAutoStreamConfig && _isStreamSource && fact.visible
        }

        FactCheckBoxSlider {
            Layout.fillWidth:   true
            text:               qsTr("Low Latency Mode")
            fact:               _videoSettings.lowLatencyMode
            visible:            !_videoAutoStreamConfig && _isStreamSource && fact.visible && _isGST
        }

        LabelledFactComboBox {
            Layout.fillWidth:   true
            label:              qsTr("Video decode priority")
            fact:               _videoSettings.forceVideoDecoder
            visible:            fact.visible
            indexModel:         false
        }
    }
 ///////////////// SECOND
    SettingsGroupLayout {
        Layout.fillWidth:   true
        heading:            qsTr("Video Source 2")
        headingDescription: _videoAutoStreamConfig ? qsTr("Mavlink camera stream is automatically configured") : ""
        enabled:            !_videoAutoStreamConfig

        LabelledFactComboBox {
            Layout.fillWidth:   true
            label:              qsTr("Source ")
            indexModel:         false
            fact:               _videoSettings.videoSource1
            visible:            fact.visible
        }
    }


    SettingsGroupLayout {
        Layout.fillWidth:   true
        heading:            qsTr("Connection 2")
        visible:            !_videoSourceDisabled1 && !_videoAutoStreamConfig && (_isTCP_1 || _isRTSP_1 || _requiresUDPPort1)

        LabelledFactTextField {
            Layout.fillWidth:           true
            textFieldPreferredWidth:    _urlFieldWidth
            label:                      qsTr("RTSP URL")
            fact:                       _videoSettings.rtspUrl1
            visible:                    _isRTSP_1 && _videoSettings.rtspUrl1.visible
        }


        LabelledFactTextField {
            Layout.fillWidth:           true
            label:                      qsTr("TCP URL")
            textFieldPreferredWidth:    _urlFieldWidth
            fact:                       _videoSettings.tcpUrl1
            visible:                    _isTCP_1 && _videoSettings.tcpUrl1.visible
        }

        LabelledFactTextField {
            Layout.fillWidth:   true
            label:              qsTr("UDP Port")
            fact:               _videoSettings.udpPort1
            visible:            _requiresUDPPort1 && _videoSettings.udpPort1.visible
        }
    }

    SettingsGroupLayout {
        Layout.fillWidth:   true
        heading:            qsTr("Settings 2")
        visible:            !_videoSourceDisabled1

        LabelledFactTextField {
            Layout.fillWidth:   true
            label:              qsTr("Aspect Ratio")
            fact:               _videoSettings.aspectRatio1
            visible:            !_videoAutoStreamConfig && _isStreamSource && _videoSettings.aspectRatio1.visible
        }

        FactCheckBoxSlider {
            Layout.fillWidth:   true
            text:               qsTr("Stop recording when disarmed")
            fact:               _videoSettings.disableWhenDisarmed1
            visible:            !_videoAutoStreamConfig && _isStreamSource && fact.visible
        }

        FactCheckBoxSlider {
            Layout.fillWidth:   true
            text:               qsTr("Low Latency Mode")
            fact:               _videoSettings.lowLatencyMode1
            visible:            !_videoAutoStreamConfig && _isStreamSource && fact.visible && _isGST
        }

        LabelledFactComboBox {
            Layout.fillWidth:   true
            label:              qsTr("Video decode priority")
            fact:               _videoSettings.forceVideoDecoder
            visible:            fact.visible
            indexModel:         false
        }
    }


    //////////////////////////
    SettingsGroupLayout {
        Layout.fillWidth: true
        heading:            qsTr("Local Video Storage 1")

        LabelledFactComboBox {
            Layout.fillWidth:   true
            label:              qsTr("Record File Format")
            fact:               _videoSettings.recordingFormat
            visible:            _videoSettings.recordingFormat.visible
        }

        FactCheckBoxSlider {
            Layout.fillWidth:   true
            text:               qsTr("Auto-Delete Saved Recordings")
            fact:               _videoSettings.enableStorageLimit
            visible:            fact.visible
        }

        LabelledFactTextField {
            Layout.fillWidth:   true
            label:              qsTr("Max Storage Usage")
            fact:               _videoSettings.maxVideoSize
            visible:            fact.visible
            enabled:            _videoSettings.enableStorageLimit.rawValue
        }
    }

    SettingsGroupLayout {
        Layout.fillWidth: true
        heading:            qsTr("Local Video Storage 2")

        LabelledFactComboBox {
            Layout.fillWidth:   true
            label:              qsTr("Record File Format")
            fact:               _videoSettings.recordingFormat1
            visible:            _videoSettings.recordingFormat.visible
        }

        FactCheckBoxSlider {
            Layout.fillWidth:   true
            text:               qsTr("Auto-Delete Saved Recordings")
            fact:               _videoSettings.enableStorageLimit1
            visible:            fact.visible
        }

        LabelledFactTextField {
            Layout.fillWidth:   true
            label:              qsTr("Max Storage Usage")
            fact:               _videoSettings.maxVideoSize1
            visible:            fact.visible
            enabled:            _videoSettings.enableStorageLimit.rawValue
        }
    }
}
