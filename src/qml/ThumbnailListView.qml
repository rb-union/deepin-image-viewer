// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.11
import QtQuick.Controls 2.4
import QtQuick.Layouts 1.11
import QtGraphicalEffects 1.0
import org.deepin.dtk 1.0
import org.deepin.image.viewer 1.0 as IV

Item {
    id: thumbnailView

    property Image targetImage
    property bool imageIsNull: null === targetImage

    // 用于外部获取当前缩略图栏内容的长度，用于布局, 10px为焦点缩略图不在ListView中的边框像素宽度(radius = 4 * 1.25)
    property int listContentWidth: bottomthumbnaillistView.contentWidth + 10
    // 除ListView外其它按键的占用宽度
    property int btnContentWidth: switchArrowLayout.width + leftRowLayout.width
                                  + rightRowLayout.width + deleteButton.width

    function deleteCurrentImage() {
        if (!fileControl.deleteImagePath(GControl.currentSource)) {
            // 取消删除文件
            return
        }

        GControl.removeImage(GControl.currentSource)
        if (0 === GControl.imageCount) {
            stackView.switchOpenImage()
        }
    }

    function previous() {
        // 切换时滑动视图不响应拖拽等触屏操作
        GStatus.viewInteractive = false
        GControl.previousImage()
        GStatus.viewInteractive = true
    }

    function next() {
        // 切换时滑动视图不响应拖拽等触屏操作
        GStatus.viewInteractive = false
        GControl.nextImage()
        GStatus.viewInteractive = true
    }

    Binding {
        delayed: true
        target: GStatus
        property: "thumbnailVaildWidth"
        value: window.width - 20 - thumbnailListView.btnContentWidth
    }

    Row {
        id: switchArrowLayout

        anchors {
            left: parent.left
            verticalCenter: parent.verticalCenter
        }
        spacing: 10
        leftPadding: 10

        IconButton {
            id: previousButton

            enabled: GControl.hasPreviousImage
            width: 50
            height: 50
            icon.name: "icon_previous"
            ToolTip.delay: 500
            ToolTip.timeout: 5000
            ToolTip.visible: hovered
            ToolTip.text: qsTr("Previous")

            onClicked: {
                previous()
            }

            Shortcut {
                sequence: "Left"
                onActivated: previous()
            }
        }

        IconButton {
            id: nextButton

            enabled: GControl.hasNextImage
            width: 50
            height: 50
            icon.name: "icon_next"
            ToolTip.delay: 500
            ToolTip.timeout: 5000
            ToolTip.visible: hovered
            ToolTip.text: qsTr("Next")

            onClicked: next()

            Shortcut {
                sequence: "Right"
                onActivated: {
                    next()
                }
            }
        }
    }

    Row {
        id: leftRowLayout

        anchors {
            left: switchArrowLayout.right
            verticalCenter: parent.verticalCenter
        }
        spacing: 10
        leftPadding: 40
        rightPadding: 20

        IconButton {
            id: fitImageButton

            anchors.leftMargin: 30
            width: 50
            height: 50
            icon.name: "icon_11"
            enabled: !imageIsNull
            ToolTip.delay: 500
            ToolTip.timeout: 5000
            ToolTip.visible: hovered
            ToolTip.text: qsTr("Original size")

            onClicked: {
                imageViewer.fitImage()
                imageViewer.recalculateLiveText()
            }
        }

        IconButton {
            id: fitWindowButton

            width: 50
            height: 50
            icon.name: "icon_self-adaption"
            enabled: !imageIsNull
            ToolTip.delay: 500
            ToolTip.timeout: 5000
            ToolTip.visible: hovered
            ToolTip.text: qsTr("Fit to window")

            onClicked: {
                imageViewer.fitWindow()
                imageViewer.recalculateLiveText()
            }
        }

        IconButton {
            id: rotateButton

            width: 50
            height: 50
            icon.name: "icon_rotate"
            enabled: !imageIsNull && fileControl.isRotatable(
                         GControl.currentSource)

            onClicked: {
                imageViewer.rotateImage(-90)
            }

            ToolTip.delay: 500
            ToolTip.timeout: 5000
            ToolTip.visible: hovered
            ToolTip.text: qsTr("Rotate")
        }
    }

    ListView {
        id: bottomthumbnaillistView

        property bool lastIsMultiImage: false

        // 重新定位图片位置
        function rePositionView() {
            // 特殊处理，防止默认显示首个缩略图时采用Center的策略会被遮挡部分
            if (0 === currentIndex) {
                positionViewAtBeginning()
            } else {
                // 尽可能将高亮缩略图显示在列表中
                positionViewAtIndex(currentIndex, ListView.Center)
            }
        }

        anchors {
            left: leftRowLayout.right
            right: rightRowLayout.left
            verticalCenter: parent.verticalCenter
        }
        // 使用范围模式，允许高亮缩略图在preferredHighlightBegin~End的范围外，使缩略图填充空白区域
        highlightRangeMode: ListView.ApplyRange
        highlightFollowsCurrentItem: true
        height: thumbnailView.height + 10
        width: thumbnailView.width - thumbnailView.btnContentWidth
        preferredHighlightBegin: width / 2 - 25
        preferredHighlightEnd: width / 2 + 25
        clip: true
        spacing: 4
        focus: true
        orientation: Qt.Horizontal
        cacheBuffer: 60

        model: GControl.globalModel
        delegate: Loader {
            id: thumbnailItemLoader

            property url source: model.imageUrl

            active: true
            asynchronous: true
            // NOTE:需设置默认的 Item 大小，以便于 ListView 计算 contentWidth
            // 防止 positionViewAtIndex() 时 Loader 加载，contentWidth 变化
            // 导致定位异常，同时 Delegate 使用 state 切换控件宽度
            width: Loader.Ready === status ? item.width : 30

            onActiveChanged: {
                if (active && imageInfo.delegateSource) {
                    setSource(imageInfo.delegateSource, {
                                  "source": thumbnailItemLoader.source
                              })
                }
            }

            IV.ImageInfo {
                id: imageInfo

                property url delegateSource
                property bool isCurrentItem: thumbnailItemLoader.ListView.isCurrentItem

                function checkDelegateSource() {
                    if (IV.ImageInfo.Ready !== status
                            && IV.ImageInfo.Error !== status) {
                        return
                    }

                    if (IV.Types.MultiImage === type && isCurrentItem) {
                        delegateSource = "qrc:/qml/ThumbnailDelegate/MultiThumnailDelegate.qml"
                    } else {
                        delegateSource = "qrc:/qml/ThumbnailDelegate/NormalThumbnailDelegate.qml"
                    }
                }

                source: thumbnailItemLoader.source

                onDelegateSourceChanged: {
                    if (thumbnailItemLoader.active && delegateSource) {
                        setSource(delegateSource, {
                                      "source": thumbnailItemLoader.source
                                  })
                    }
                }

                onStatusChanged: checkDelegateSource()
                onIsCurrentItemChanged: {
                    checkDelegateSource()

                    // 切换图片涉及多页图时，由于列表内容宽度变更，焦点item定位异常，延迟定位
                    if (IV.Types.MultiImage === type) {
                        bottomthumbnaillistView.lastIsMultiImage = true
                        delayUpdateTimer.start()
                    } else if (bottomthumbnaillistView.lastIsMultiImage) {
                        delayUpdateTimer.start()
                        bottomthumbnaillistView.lastIsMultiImage = false
                    }
                }

                // 图片被删除、替换，重设当前图片组件
                onInfoChanged: {
                    checkDelegateSource()

                    var temp = delegateSource
                    delegateSource = ""
                    delegateSource = temp
                }
            }
        }

        // 添加两组空的表头表尾用于占位，防止在边界的高亮缩略图被遮挡, 5px为不在ListView中维护的焦点缩略图边框的宽度 radius = 4 * 1.25
        header: Rectangle {
            width: 5
        }

        footer: Rectangle {
            width: 5
        }

        //滑动联动主视图
        onCurrentIndexChanged: {
            if (currentItem) {
                currentItem.forceActiveFocus()
            }

            // 直接定位，屏蔽动画效果
            rePositionView()

            // 仅在边缘缩略图时进行二次定位
            if (0 === currentIndex || currentIndex === (count - 1)) {
                delayUpdateTimer.start()
            }
        }

        Connections {
            target: GControl
            onCurrentIndexChanged: {
                bottomthumbnaillistView.currentIndex = GControl.currentIndex
            }
        }

        Connections {
            target: GStatus

            onFullScreenAnimatingChanged: {
                // 动画结束时处理
                if (!GStatus.fullScreenAnimating) {
                    // 当缩放界面时，缩略图栏重新进行了布局计算，导致高亮缩略图可能不居中
                    if (0 == bottomthumbnaillistView.currentIndex) {
                        bottomthumbnaillistView.positionViewAtBeginning()
                    } else {
                        // 尽可能将高亮缩略图显示在列表中
                        bottomthumbnaillistView.positionViewAtIndex(
                                    bottomthumbnaillistView.currentIndex,
                                    ListView.Center)
                    }
                }
            }
        }

        Timer {
            id: delayUpdateTimer

            repeat: false
            interval: 100
            onTriggered: {
                bottomthumbnaillistView.forceLayout()
                bottomthumbnaillistView.rePositionView()
            }
        }

        Component.onCompleted: {
            bottomthumbnaillistView.currentIndex = GControl.currentIndex
            forceLayout()
            rePositionView()
        }
    }

    Row {
        id: rightRowLayout

        anchors {
            right: deleteButton.left
            verticalCenter: parent.verticalCenter
        }
        spacing: 10
        leftPadding: 20
        rightPadding: 20

        IconButton {
            id: ocrButton

            width: 50
            height: 50
            enabled: fileControl.isCanSupportOcr(GControl.currentSource)
                     && !imageIsNull
            icon.name: "icon_character_recognition"
            ToolTip.delay: 500
            ToolTip.timeout: 5000
            ToolTip.visible: hovered
            ToolTip.text: qsTr("Extract text")

            onClicked: {
                GControl.submitImageChangeImmediately()
                fileControl.ocrImage(GControl.currentSource,
                                     GControl.currentFrameIndex)
            }
        }
    }

    IconButton {
        id: deleteButton

        anchors {
            right: parent.right
            rightMargin: 10
            verticalCenter: parent.verticalCenter
        }
        enabled: fileControl.isCanDelete(GControl.currentSource)
        width: 50
        height: 50
        icon.name: "icon_delete"
        icon.source: "qrc:/res/dcc_delete_36px.svg"
        icon.color: enabled ? "red" : "ffffff"
        ToolTip.delay: 500
        ToolTip.timeout: 5000
        ToolTip.visible: hovered
        ToolTip.text: qsTr("Delete")

        onClicked: deleteCurrentImage()
    }
}
