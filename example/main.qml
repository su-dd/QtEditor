import QtQuick 2.15
import QtQuick.Window 2.15
import QtMarkdown 1.0
import QtQuick.Controls 2.15

Window {
    id: root
    width: 800
    height: 600
    visible: true
    title: qsTr("Markdown Editor Qt Quick Demo")
    Flickable {
        id: sv
        anchors.fill: parent
        contentWidth: parent.width
        contentHeight: md.height
        QtQuickMarkdownEditor {
            id: md
            // 必须focus才能接收键盘输入
            focus: true
            width: parent.width
            source: ":/test.md"
        }
    }
    Component.onCompleted: function () {
        console.log(md.width, md.height)
        console.log(sv.width, sv.height)
    }
}