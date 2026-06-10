/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick
import QtQuick.Layouts
import QGroundControl.ScreenTools
import QGroundControl
import QGroundControl.Controls
import QGroundControl.FlightDisplay

RowLayout {

    property var  _flyViewSettings:             QGroundControl.settingsManager.flyViewSettings
    property bool _showAdditionalIndicators:    _flyViewSettings.showAdditionalIndicatorsCompass.value


    TelemetryValuesBar {
        Layout.alignment: _showAdditionalIndicators ? (Qt.AlignRight | Qt.AlignBottom) : Qt.AlignBottom
        //extraWidth:     instrumentPanel.extraValuesWidth/4
    }

    FlyViewInstrumentPanel {
        id:         instrumentPanel
        visible:    !_showAdditionalIndicators//QGroundControl.corePlugin.options.flyView.showInstrumentPanel && _showSingleVehicleUI
    }
}
