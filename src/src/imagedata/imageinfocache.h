// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef IMAGEINFOCACHE_H
#define IMAGEINFOCACHE_H

#include <QObject>
#include <QQueue>
#include <QSharedPointer>

class ImageInfoData;
class ImageInfoCache : public QObject
{
    Q_OBJECT
public:
    explicit ImageInfoCache(QObject *parent = nullptr);

    void addLoadTask();

private:
    void loadData();

    QQueue<QSharedPointer<ImageInfoData>> taskQueue;

    friend class ImageInfoCache;
};

#endif  // IMAGEINFOCACHE_H
