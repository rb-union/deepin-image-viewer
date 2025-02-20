// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef IMAGEINFODATA_P_H
#define IMAGEINFODATA_P_H

#include <QSize>
#include <QList>
#include <QImage>
#include <QSharedPointer>

#include "types.h"

class ImageInfo;
class ImageInfoData
{
public:
    typedef QSharedPointer<ImageInfoData> Ptr;

    enum State {
        UnLoaded,

        // Note: If current state marked loading, **Can not read data!**
        Prepare,
        LoadingFile,
        ConvertThumbnail,

        Loaded,
    };

    enum LoadFlag {
        Cancel = 0,
        LoadInfo = 1 << 1,
        LoadImage = 1 << 2,
        LoadThumbnail = 1 << 3,

        LoadAll = LoadInfo | LoadThumbnail | LoadImage,  // almost use to refresh
    };

    // State
    QAtomicInt state{UnLoaded};
    QAtomicInt loadFlag{Cancel};
    QVector<QPointer<ImageInfo>> handlers;

    // Cache full image / thumbnail image
    struct ImageFrame
    {
        typedef QScopedPointer<QImage> Ptr;
        QImage image;
        QImage thumbnail;
        QSize size;
    };
    QVector<ImageFrame::Ptr> frames;

    // Properties
    QString path;
    Types::ImageType type{Types::NullImage};
    int frameCount{0};

    // Runtime properties
    qreal scale{-1};  // scale factor
    qreal x{0};       // drag will change image position
    qreal y{0};
};

#endif  // IMAGEINFODATA_P_H
