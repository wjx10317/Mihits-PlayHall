#include "gamepackageupdater.h"

#include "md5.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMap>
#include <QNetworkRequest>
#include <QSet>
#include <QUrl>
#include <fstream>

GamePackageUpdater::GamePackageUpdater(QObject *parent)
    : QObject(parent)
{
}

QString GamePackageUpdater::installDir(const QString &gamesRoot, const QString &exeName)
{
    return QDir(gamesRoot).filePath(exeName);
}

QString GamePackageUpdater::fileMd5(const QString &filePath)
{
    std::ifstream in(filePath.toLocal8Bit().constData(), std::ios::binary);
    if (!in)
        return QString();
    MD5 md5(in);
    return QString::fromStdString(md5.toString()).toLower();
}

bool GamePackageUpdater::parseManifest(const QByteArray &json, GameManifest &out, QString *err)
{
    QJsonParseError pe;
    QJsonDocument doc = QJsonDocument::fromJson(json, &pe);
    if (pe.error != QJsonParseError::NoError || !doc.isObject())
    {
        if (err) *err = QString("manifest JSON 无效: %1").arg(pe.errorString());
        return false;
    }
    QJsonObject o = doc.object();
    out.zoneid = o.value("zoneid").toInt();
    out.version = o.value("version").toString();
    out.exeName = o.value("exe_name").toString();
    out.baseUrl = o.value("baseUrl").toString();
    if (!out.baseUrl.endsWith('/'))
        out.baseUrl.append('/');
    if (out.exeName.isEmpty() || out.version.isEmpty() || out.baseUrl.isEmpty())
    {
        if (err) *err = "manifest 缺少 exe_name/version/baseUrl";
        return false;
    }
    out.files.clear();
    QJsonArray arr = o.value("files").toArray();
    for (const QJsonValue &v : arr)
    {
        QJsonObject fo = v.toObject();
        GameManifestFile f;
        f.path = fo.value("path").toString();
        f.size = static_cast<qint64>(fo.value("size").toDouble());
        f.md5 = fo.value("md5").toString().toLower();
        if (f.path.isEmpty() || f.md5.isEmpty())
        {
            if (err) *err = "manifest files 项缺少 path/md5";
            return false;
        }
        // 禁止跳出安装目录
        if (f.path.contains("..") || QDir::isAbsolutePath(f.path))
        {
            if (err) *err = QString("非法 path: %1").arg(f.path);
            return false;
        }
        out.files.push_back(f);
    }
    return true;
}

void GamePackageUpdater::cancel()
{
    m_cancelled = true;
    if (m_reply)
    {
        m_reply->abort();
    }
}

void GamePackageUpdater::startUpdate(const QString &manifestUrl, const QString &gamesRoot)
{
    m_cancelled = false;
    m_manifestUrl = manifestUrl;
    m_gamesRoot = gamesRoot;
    m_downloadList.clear();
    m_deleteList.clear();
    m_downloadIndex = 0;
    m_totalDownloadBytes = 0;
    m_doneDownloadBytes = 0;
    m_manifest = GameManifest();

    emit statusMessage(QString("下载清单: %1").arg(manifestUrl));
    QNetworkRequest req{QUrl(manifestUrl)};
    m_reply = m_nam.get(req);
    connect(m_reply, &QNetworkReply::finished, this, &GamePackageUpdater::onManifestFinished);
}

void GamePackageUpdater::fail(const QString &err)
{
    if (m_reply)
    {
        m_reply->deleteLater();
        m_reply = nullptr;
    }
    emit finished(false, err, m_manifest);
}

void GamePackageUpdater::onManifestFinished()
{
    QNetworkReply *reply = m_reply;
    m_reply = nullptr;
    if (!reply)
        return;
    reply->deleteLater();

    if (m_cancelled)
    {
        fail("已取消");
        return;
    }
    if (reply->error() != QNetworkReply::NoError)
    {
        fail(QString("清单下载失败: %1").arg(reply->errorString()));
        return;
    }

    QString err;
    if (!parseManifest(reply->readAll(), m_manifest, &err))
    {
        fail(err);
        return;
    }

    m_installDir = installDir(m_gamesRoot, m_manifest.exeName);
    QDir().mkpath(m_installDir);
    beginDiffAndDownload();
}

bool GamePackageUpdater::loadLocalVersionMap(QMap<QString, QString> &pathToMd5, QString *err)
{
    Q_UNUSED(err);
    pathToMd5.clear();
    const QString verPath = QDir(m_installDir).filePath("version.json");
    QFile f(verPath);
    if (!f.exists())
        return true;
    if (!f.open(QIODevice::ReadOnly))
        return true;
    QJsonDocument doc = QJsonDocument::fromJson(f.readAll());
    if (!doc.isObject())
        return true;
    QJsonArray arr = doc.object().value("files").toArray();
    for (const QJsonValue &v : arr)
    {
        QJsonObject o = v.toObject();
        const QString path = o.value("path").toString();
        const QString md5 = o.value("md5").toString().toLower();
        if (!path.isEmpty() && !md5.isEmpty())
            pathToMd5.insert(path, md5);
    }
    return true;
}

void GamePackageUpdater::beginDiffAndDownload()
{
    QMap<QString, QString> localMap;
    loadLocalVersionMap(localMap, nullptr);

    QSet<QString> remotePaths;
    m_downloadList.clear();
    m_totalDownloadBytes = 0;
    for (const GameManifestFile &f : m_manifest.files)
    {
        remotePaths.insert(f.path);
        if (localMap.value(f.path) != f.md5)
        {
            // 本地文件存在但 version.json 丢失时，再比一次磁盘
            const QString abs = QDir(m_installDir).filePath(f.path);
            if (QFile::exists(abs) && fileMd5(abs) == f.md5)
                continue;
            m_downloadList.push_back(f);
            m_totalDownloadBytes += qMax<qint64>(f.size, 0);
        }
    }

    m_deleteList.clear();
    for (auto it = localMap.begin(); it != localMap.end(); ++it)
    {
        if (!remotePaths.contains(it.key()))
            m_deleteList.push_back(it.key());
    }

    emit statusMessage(QString("需下载 %1 个文件, 删除多余 %2 个")
                           .arg(m_downloadList.size())
                           .arg(m_deleteList.size()));

    m_downloadIndex = 0;
    m_doneDownloadBytes = 0;
    downloadNext();
}

void GamePackageUpdater::downloadNext()
{
    if (m_cancelled)
    {
        fail("已取消");
        return;
    }
    if (m_downloadIndex >= m_downloadList.size())
    {
        finalizeSuccess();
        return;
    }

    const GameManifestFile &f = m_downloadList[m_downloadIndex];
    const QUrl url(m_manifest.baseUrl + f.path);
    emit statusMessage(QString("下载: %1").arg(f.path));
    emit progress(m_doneDownloadBytes, m_totalDownloadBytes, f.path);

    QNetworkRequest req{url};
    m_reply = m_nam.get(req);
    connect(m_reply, &QNetworkReply::downloadProgress, this,
            [this](qint64 rec, qint64) {
                emit progress(m_doneDownloadBytes + rec, m_totalDownloadBytes,
                              m_downloadIndex < m_downloadList.size()
                                  ? m_downloadList[m_downloadIndex].path
                                  : QString());
            });
    connect(m_reply, &QNetworkReply::finished, this, &GamePackageUpdater::onFileFinished);
}

void GamePackageUpdater::onFileFinished()
{
    QNetworkReply *reply = m_reply;
    m_reply = nullptr;
    if (!reply)
        return;
    reply->deleteLater();

    if (m_cancelled)
    {
        fail("已取消");
        return;
    }
    if (m_downloadIndex >= m_downloadList.size())
    {
        fail("内部状态错误");
        return;
    }

    const GameManifestFile f = m_downloadList[m_downloadIndex];
    if (reply->error() != QNetworkReply::NoError)
    {
        fail(QString("下载失败 %1: %2").arg(f.path, reply->errorString()));
        return;
    }

    const QByteArray data = reply->readAll();
    MD5 md5(data.constData(), static_cast<size_t>(data.size()));
    const QString got = QString::fromStdString(md5.toString()).toLower();
    if (got != f.md5)
    {
        fail(QString("MD5 不匹配 %1 expect=%2 got=%3").arg(f.path, f.md5, got));
        return;
    }

    const QString abs = QDir(m_installDir).filePath(f.path);
    QFileInfo fi(abs);
    QDir().mkpath(fi.absolutePath());
    const QString tmp = abs + ".tmp";
    QFile::remove(tmp);
    QFile out(tmp);
    if (!out.open(QIODevice::WriteOnly))
    {
        fail(QString("无法写入临时文件: %1").arg(tmp));
        return;
    }
    if (out.write(data) != data.size())
    {
        out.close();
        QFile::remove(tmp);
        fail(QString("写入不完整: %1").arg(tmp));
        return;
    }
    out.close();
    QFile::remove(abs);
    if (!QFile::rename(tmp, abs))
    {
        QFile::remove(tmp);
        fail(QString("无法替换文件: %1").arg(abs));
        return;
    }

    m_doneDownloadBytes += data.size();
    ++m_downloadIndex;
    downloadNext();
}

bool GamePackageUpdater::writeLocalVersionJson(QString *err)
{
    QJsonArray files;
    for (const GameManifestFile &f : m_manifest.files)
    {
        QJsonObject o;
        o.insert("path", f.path);
        o.insert("size", f.size);
        o.insert("md5", f.md5);
        files.append(o);
    }
    QJsonObject root;
    root.insert("version", m_manifest.version);
    root.insert("exe_name", m_manifest.exeName);
    root.insert("zoneid", m_manifest.zoneid);
    root.insert("files", files);

    const QString verPath = QDir(m_installDir).filePath("version.json");
    QFile f(verPath);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate))
    {
        if (err) *err = QString("无法写 version.json: %1").arg(verPath);
        return false;
    }
    f.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
    return true;
}

void GamePackageUpdater::finalizeSuccess()
{
    for (const QString &path : m_deleteList)
    {
        if (path.contains(".."))
            continue;
        const QString abs = QDir(m_installDir).filePath(path);
        if (QFile::exists(abs))
        {
            QFile::remove(abs);
            emit statusMessage(QString("删除多余: %1").arg(path));
        }
    }

    QString err;
    if (!writeLocalVersionJson(&err))
    {
        fail(err);
        return;
    }

    emit progress(m_totalDownloadBytes, m_totalDownloadBytes, QString());
    emit statusMessage("更新完成");
    emit finished(true, QString(), m_manifest);
}
