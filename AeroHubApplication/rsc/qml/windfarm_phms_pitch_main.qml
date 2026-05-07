import QtQuick
import QtQuick.Controls

ApplicationWindow {
  width: 1920
  height: 1080
  visible: true
  title: "System State - Backward Facing"
  color: "#f8f9fa"

  
  BackwardFacingState {
  }


  Window {
    id: forward_facing_window
    width: 1920
    height: 1080
    visible: true
    title: "System State - Forward Facing"
    color: "#f8f9fa"

    ForwardFacingState {
    }
  }
}
