#include "utils/baseutils.h"
#include "utils/imageutils.h"
#include <QDebug>
#include <QDir>
#include <QDirIterator>
#include <QFileInfo>
#include <QBuffer>
#include <QImage>

extern "C" {
#include "libjpeg/jpeg-data.h"
}

namespace utils {

namespace image {

static ExifItem ExifDataBasics[] = {
    {EXIF_IFD_COUNT, EXIF_TAG_EXTEND_NAME,
     QT_TRANSLATE_NOOP("ExifItemName", "Name")},
    {EXIF_IFD_COUNT, EXIF_TAG_EXTEND_TYPE,
     QT_TRANSLATE_NOOP("ExifItemName", "Type")},
    {EXIF_IFD_EXIF, EXIF_TAG_DATE_TIME_ORIGINAL,
     QT_TRANSLATE_NOOP("ExifItemName", "Date photoed")},
    {EXIF_IFD_0, EXIF_TAG_DATE_TIME,
     QT_TRANSLATE_NOOP("ExifItemName", "Date modified")},
    {EXIF_IFD_COUNT, EXIF_TAG_EXTEND_RESOLUTION,
     QT_TRANSLATE_NOOP("ExifItemName", "Resolution")},
    {EXIF_IFD_COUNT, EXIF_TAG_EXTEND_SIZE,
     QT_TRANSLATE_NOOP("ExifItemName", "File size")},
    {EXIF_IFD_COUNT, 0, 0}
};

static ExifItem ExifDataDetails[] = {
    {EXIF_IFD_EXIF, EXIF_TAG_COLOR_SPACE,
     QT_TRANSLATE_NOOP("ExifItemName", "Colorspace")},
    {EXIF_IFD_EXIF, EXIF_TAG_EXPOSURE_MODE,
     QT_TRANSLATE_NOOP("ExifItemName", "Exposure mode")},
    {EXIF_IFD_EXIF, EXIF_TAG_EXPOSURE_PROGRAM,
     QT_TRANSLATE_NOOP("ExifItemName", "Exposure program")},
    {EXIF_IFD_EXIF, EXIF_TAG_EXPOSURE_TIME,
     QT_TRANSLATE_NOOP("ExifItemName", "Exposure time")},
    {EXIF_IFD_EXIF, EXIF_TAG_FLASH,
     QT_TRANSLATE_NOOP("ExifItemName", "Flash")},
    {EXIF_IFD_EXIF, EXIF_TAG_APERTURE_VALUE,
     QT_TRANSLATE_NOOP("ExifItemName", "Aperture")},
    {EXIF_IFD_EXIF, EXIF_TAG_FOCAL_LENGTH,
     QT_TRANSLATE_NOOP("ExifItemName", "Focal length")},
    {EXIF_IFD_EXIF, EXIF_TAG_ISO_SPEED_RATINGS,
     QT_TRANSLATE_NOOP("ExifItemName", "ISO")},
    {EXIF_IFD_EXIF, EXIF_TAG_MAX_APERTURE_VALUE,
     QT_TRANSLATE_NOOP("ExifItemName", "Max aperture")},
    {EXIF_IFD_EXIF, EXIF_TAG_METERING_MODE,
     QT_TRANSLATE_NOOP("ExifItemName", "Metering mode")},
    {EXIF_IFD_EXIF, EXIF_TAG_WHITE_BALANCE,
     QT_TRANSLATE_NOOP("ExifItemName", "White balance")},
    {EXIF_IFD_COUNT, EXIF_TAG_EXTEND_FLASH_COMPENSATION,
     QT_TRANSLATE_NOOP("ExifItemName", "Flash compensation")},
    {EXIF_IFD_0, EXIF_TAG_MAKE,
//     QT_TRANSLATE_NOOP("ExifItemName", "Manufacture")},
//    {EXIF_IFD_0, EXIF_TAG_MODEL,
     QT_TRANSLATE_NOOP("ExifItemName", "Camera model")},
    {EXIF_IFD_COUNT, EXIF_TAG_EXTEND_LENS_MODEL,
     QT_TRANSLATE_NOOP("ExifItemName", "Lens model")},
    {EXIF_IFD_COUNT, 0, 0}
};

ExifItem *getExifItemList(bool isDetails)
{
    return isDetails ? ExifDataDetails : ExifDataBasics;
}

const QStringList supportImageTypes()
{
    QStringList origin;
    origin<< "BMP";
    origin<< "GIF";
    origin<< "JPG";
    origin<< "JPEG";
    origin<< "PNG";
    origin<< "PBM";
    origin<< "PGM";
    origin<< "PPM";
    origin<< "XBM";
    origin<< "XPM";
    origin<< "SVG";

    origin<< "DDS";
    origin<< "ICNS";
    origin<< "JP2";
    origin<< "MNG";
    origin<< "TGA";
    origin<< "TIFF";
    origin<< "WBMP";
    origin<< "WEBP";

    QStringList list;
    for (QString v : origin) {
        list << v.toLower();
    }
    list += origin;

    return list;
}

QPixmap getThumbnail(const QString &filePath, bool exifOnly)
{
    QPixmap pixmap;
    ExifData *ed = exif_data_new_from_file(filePath.toUtf8().data());
    if (ed) {
        // Make sure the image had a thumbnail before trying to write it
        if (ed->data && ed->size) {
            pixmap.loadFromData(ed->data, ed->size);
            // Free the EXIF data
            exif_data_unref(ed);
        } else {
            qDebug() << QString("NO Exif thumbnail in file %1!").arg(filePath);
            if (! exifOnly)
                pixmap = scaleImage(filePath);
        }
    }
    else if (! exifOnly){
        pixmap = scaleImage(filePath);
    }

    return pixmap;
}

QPixmap scaleImage(const QString &filePath, const QSize &size)
{
    QImage img = getRotatedImage(filePath);
    if (img.isNull())
        return QPixmap();
    QSize targetSize;
    if (img.width() > img.height()) {
        targetSize = QSize(size.width(),
                           (double)size.width() / img.width() *
                           img.height());
    }
    else {
        targetSize = QSize((double)size.height() / img.height() *
                           img.width(), size.height());
    }

    return QPixmap::fromImage(img.scaled(targetSize * 2)
                              .scaled(targetSize, Qt::IgnoreAspectRatio,
                                      Qt::SmoothTransformation));
}

QDateTime getCreateDateTime(const QString &filePath)
{
    ExifData *ed = exif_data_new_from_file(filePath.toUtf8().data());
    if (ed) {
        QDateTime dt = utils::base::stringToDateTime(
                    readExifTag(ed, EXIF_IFD_EXIF, EXIF_TAG_DATE_TIME_ORIGINAL));
        //Free the EXIF data
        exif_data_unref(ed);
        if (dt.isValid())
            return dt;
    }
    QFileInfo info(filePath);
    return info.created();
}

QString readExifTag(ExifData *ed, ExifIfd eid, ExifTag tag)
{
    ExifEntry *entry = exif_content_get_entry(ed->ifd[eid], tag);

    if (entry){
        char buf[1024];
        exif_entry_get_value(entry, buf, sizeof(buf));

        if (*buf) {
            return QString(buf).trimmed();
        }
    }

    return QString();
}

/* raw EXIF header data */
const unsigned char kExifHeader[] = {
    0xff, 0xd8, 0xff, 0xe1
};

void saveImageWithExif(const QImage &image, const QString &path, const QString &sourcePath, const QTransform &mat)
{
    if (sourcePath.isEmpty() || !path.endsWith(".jpg", Qt::CaseInsensitive) || path.endsWith(".jpeg", Qt::CaseInsensitive)) {
        image.save(path);
        return;
    }
    ExifData *ed = exif_data_new_from_file(sourcePath.toUtf8().constData());
    if (!ed) {
        image.save(path);
        return;
    }
    const ExifByteOrder bo = exif_data_get_byte_order(ed);
    ExifEntry *e = exif_content_get_entry(ed->ifd[EXIF_IFD_EXIF], EXIF_TAG_X_RESOLUTION);
    if (e)
        exif_set_long(e->data, bo, image.width());
    e = exif_content_get_entry(ed->ifd[EXIF_IFD_EXIF], EXIF_TAG_Y_RESOLUTION);
    if (e)
        exif_set_long(e->data, bo, image.height());

    e = exif_content_get_entry(ed->ifd[EXIF_IFD_EXIF], EXIF_TAG_PIXEL_X_DIMENSION);
    if (e)
        exif_set_long(e->data, bo, image.width());
    e = exif_content_get_entry(ed->ifd[EXIF_IFD_EXIF], EXIF_TAG_PIXEL_Y_DIMENSION);
    if (e)
        exif_set_long(e->data, bo, image.height());

    if (ed->data && !mat.isIdentity()) {
        QImage thumb = QImage::fromData(ed->data, ed->size);
        thumb = thumb.transformed(mat);
        free(ed->data);
        ed->data = 0;
        ed->size = 0;
        QByteArray data;
        QBuffer buffer(&data);
        buffer.open(QIODevice::WriteOnly);
        if (!thumb.save(&buffer, sourcePath.mid( sourcePath.lastIndexOf('.')+1).toLatin1().constData())) {
            qWarning("save thumbnail error");
        } else {
            ed->data = (unsigned char*)malloc(data.size());
            ed->size = data.size();
            memcpy(ed->data, data.constData(), data.size());
        }
        buffer.close();
    }
    unsigned char *exif_data;
    unsigned int exif_data_len;
    exif_data_save_data(ed, &exif_data, &exif_data_len);
    if (exif_data_len) {
        free(exif_data);
        if (exif_data_len > 0xffff)
            qWarning("Too much exif data");
    }
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly)) {
        exif_data_unref(ed);
        qWarning() << "Failed to open image: " << f.errorString();
        return;
    }
    QByteArray data;
    QBuffer buffer(&data);
    buffer.open(QIODevice::WriteOnly);
    if (!image.save(&buffer, sourcePath.mid( sourcePath.lastIndexOf('.')+1).toLatin1().constData())) {
        qWarning("save image error");
        exif_data_unref(ed);
        return;
    }
    buffer.close();
    JPEGData *jdata = jpeg_data_new();
    jpeg_data_load_data(jdata, (const unsigned char*)data.constData(), data.size());
    jpeg_data_set_exif_data(jdata, ed);

    unsigned char *d = NULL;
    unsigned int s = 0;
    jpeg_data_save_data(jdata, &d, &s);
    f.write((const char*)d, s);
    f.close();
    exif_data_unref(ed);
    jpeg_data_unref(jdata);
}


QImage saturation(int delta, QImage &origin){
    QImage newImage(origin.width(), origin.height(), QImage::Format_ARGB32);

    QColor oldColor;
    QColor newColor;
    int h,s,l;

    for(int x=0; x<newImage.width(); x++){
        for(int y=0; y<newImage.height(); y++){
            oldColor = QColor(origin.pixel(x,y));

            newColor = oldColor.toHsl();
            h = newColor.hue();
            s = newColor.saturation() + delta;
            l = newColor.lightness();

            //we check if the new value is between 0 and 255
            s = qBound(0, s, 255);
            l = qBound(0, l, 255);

            newColor.setHsl(h, s, l);

            newImage.setPixel(x, y, qRgb(newColor.red(), newColor.green(), newColor.blue()));
        }
    }

    return newImage;
}

QImage cool(int delta, QImage &origin) {
    QImage newImage(origin.width(), origin.height(), QImage::Format_ARGB32);

    QColor oldColor;
    int r,g,b;

    for(int x=0; x<newImage.width(); x++){
        for(int y=0; y<newImage.height(); y++){
            oldColor = QColor(origin.pixel(x,y));

            r = oldColor.red();
            g = oldColor.green();
            b = oldColor.blue()+delta;

            //we check if the new value is between 0 and 255
            b = qBound(0, b, 255);

            newImage.setPixel(x,y, qRgb(r,g,b));
        }
    }

    return newImage;
}

QImage warm(int delta, QImage &origin){
    QImage newImage(origin.width(), origin.height(), QImage::Format_ARGB32);

    QColor oldColor;
    int r,g,b;

    for(int x=0; x<newImage.width(); x++){
        for(int y=0; y<newImage.height(); y++){
            oldColor = QColor(origin.pixel(x,y));

            r = oldColor.red() + delta;
            g = oldColor.green() + delta;
            b = oldColor.blue();

            //we check if the new values are between 0 and 255
            r = qBound(0, r, 255);
            g = qBound(0, g, 255);

            newImage.setPixel(x,y, qRgb(r,g,b));
        }
    }

    return newImage;
}

QImage blure(const QImage &origin) {
    QImage newImage(origin);

    int kernel [5][5]= {{0,0,1,0,0},
                        {0,1,3,1,0},
                        {1,3,7,3,1},
                        {0,1,3,1,0},
                        {0,0,1,0,0}};
    int kernelSize = 5;
    int sumKernel = 27;
    int r,g,b;
    QColor color;

    for(int x=kernelSize/2; x<newImage.width()-(kernelSize/2); x++){
        for(int y=kernelSize/2; y<newImage.height()-(kernelSize/2); y++){

            r = 0;
            g = 0;
            b = 0;

            for(int i = -kernelSize/2; i<= kernelSize/2; i++){
                for(int j = -kernelSize/2; j<= kernelSize/2; j++){
                    color = QColor(origin.pixel(x+i, y+j));
                    r += color.red()*kernel[kernelSize/2+i][kernelSize/2+j];
                    g += color.green()*kernel[kernelSize/2+i][kernelSize/2+j];
                    b += color.blue()*kernel[kernelSize/2+i][kernelSize/2+j];
                }
            }

            r = qBound(0, r/sumKernel, 255);
            g = qBound(0, g/sumKernel, 255);
            b = qBound(0, b/sumKernel, 255);

            newImage.setPixel(x,y, qRgb(r,g,b));

        }
    }

    return newImage;
}

QMap<QString, QString> getExifFromPath(const QString &path, bool isDetails)
{
    QMap<QString, QString> dataMap;

    QFileInfo fi(path);
    QImage img(path);
    ExifItem *items = getExifItemList(isDetails);

//    qDebug() << "Reading exif from: " << path;
    ExifData *ed = exif_data_new_from_file(path.toUtf8().data());

    if (! ed) {
        dataMap.insert(QObject::tr("Date photoed"),
            fi.created().toString("yyyy/MM/dd HH:mm:ss"));
        dataMap.insert(QObject::tr("Date modified"),
            fi.lastModified().toString("yyyy/MM/dd HH:mm:ss"));
    }

    for (const ExifItem* i = items; i->tag; ++i) {
        if (i->ifd != EXIF_IFD_COUNT && ed) {
            ExifEntry *e = exif_content_get_entry(ed->ifd[i->ifd],
                    (ExifTag)i->tag);
            if (!e) {
                qWarning() << "no exif entry: " << i->name;
                continue;
            }
            char buf[1024];
            memset(buf, 0, sizeof(buf));
            exif_entry_get_value(e, buf, sizeof(buf));
            if (*buf) {
                QString v = QString(buf).trimmed();
                switch (i->tag) {
                case EXIF_TAG_DATE_TIME:
                case EXIF_TAG_DATE_TIME_ORIGINAL: {
                    const QString tt = utils::base::stringToDateTime(v).toString("yyyy/MM/dd HH:mm:ss");
                    dataMap.insert(i->name, tt);
                    break;
                }
                default:
                    dataMap.insert(i->name, v);
                }
            }
        }
        else {  // For get extend tag infomation
            switch (i->tag) {
            case EXIF_TAG_EXTEND_NAME:
                dataMap.insert(i->name, fi.fileName());
                break;
            case EXIF_TAG_EXTEND_TYPE:
                dataMap.insert(i->name, fi.suffix().toUpper());
                break;
            case EXIF_TAG_EXTEND_RESOLUTION:
                dataMap.insert(i->name, QString::number(img.width()) + "x" +
                               QString::number(img.height()));
                break;
            case EXIF_TAG_EXTEND_SIZE:
                dataMap.insert(i->name, utils::base::sizeToHuman(fi.size()));
                break;
            case EXIF_TAG_EXTEND_FLASH_COMPENSATION:
                // TODO
                break;
            case EXIF_TAG_EXTEND_LENS_MODEL:
                // TODO
                break;
            default:
                break;
            }
        }
    }

    exif_data_unref(ed);
    return dataMap;
}

bool imageIsSupport(const QString &filepath)
{
    return true;
    QFileInfo info(filepath);
    if (info.isFile()) {
        return (supportImageTypes().indexOf(info.suffix().toLower()) != -1);
    }
    else {
        return false;
    }
}

void rotate(const QString &path, int degree)
{
    const QTransform t = QTransform().rotate(degree);
    saveImageWithExif(QImage(path).transformed(t), path, path, t);
}

QPixmap cutSquareImage(const QPixmap &pixmap, const QSize &size)
{
    QImage img = pixmap.toImage().scaled(size,
                                                Qt::KeepAspectRatioByExpanding,
                                                Qt::SmoothTransformation);

    img = img.copy((img.width() - size.width()) / 2,
                   (img.height() - size.height()) / 2,
                   size.width(), size.height());
    return QPixmap::fromImage(img);
}

const QFileInfoList getImagesInfo(const QString &dir, bool recursive)
{
    QFileInfoList infos;

    QStringList ol = supportImageTypes();
    QStringList nl;
    for (QString type : ol) {
        nl << "*." + type;
    }

    if (! recursive)
        return QDir(dir).entryInfoList(nl, QDir::Files | QDir::NoSymLinks);

    QDirIterator dirIterator(dir,
                             nl,
                             QDir::Files | QDir::NoSymLinks,
                             QDirIterator::Subdirectories);
    while(dirIterator.hasNext())
    {
        dirIterator.next();
        infos << dirIterator.fileInfo();
    }

    return infos;
}

QString getOrientation(const QString &path)
{
    ExifData *ed = exif_data_new_from_file(path.toUtf8().data());
    QString dt;
    if (ed) {
        dt = readExifTag(ed, EXIF_IFD_0, EXIF_TAG_ORIENTATION);
        //Free the EXIF data
        exif_data_unref(ed);
    }

    return dt;
}

/*!
 * \brief getRotatedImage
 * Rotate image base on the exif orientation
 * \param path
 * \return
 */
QImage getRotatedImage(const QString &path)
{
    // FIXME it should read the orientation enum value
    QImage img(path);
    const QString o = getOrientation(path);
    if (o.isEmpty() || o == "Top-left")
        return img;
    QTransform t;
    if (o == "Bottom-right") {
        t.rotate(-180);
    }
    else if (o == "Left-bottom") {
        t.rotate(-90);
    }
    else if (o == "Right-top") {
        t.rotate(90);
    }
    img = img.transformed(t, Qt::SmoothTransformation);

    return img;
}

}  // namespace image

}  //namespace utils