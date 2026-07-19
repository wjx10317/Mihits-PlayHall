#ifndef GAMEPACKAGEUPDATER_H
#define GAMEPACKAGEUPDATER_H

#include <QObject>
#include <QString>
#include <QVector>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QJsonObject>

struct GameManifestFile
{
    QString path;
    qint64 size = 0;
    QString md5;
};

struct GameManifest
{
    int zoneid = 0;
    QString version;
    QString exeName;
    QString baseUrl;
    QVector<GameManifestFile> files;
};

// 清单 MD5 增量更新：downloadList + deleteList；全量 = diff 为全集
class GamePackageUpdater : public QObject
{
    Q_OBJECT
public:
    explicit GamePackageUpdater(QObject *parent = nullptr);

    // gamesRoot: 如 "<appDir>/games"，最终目录为 gamesRoot/exe_name/
    void startUpdate(const QString &manifestUrl, const QString &gamesRoot);
    void cancel();

    static QString fileMd5(const QString &filePath);
    static bool parseManifest(const QByteArray &json, GameManifest &out, QString *err = nullptr);
    static QString installDir(const QString &gamesRoot, const QString &exeName);

signals:
    void progress(qint64 bytesReceived, qint64 bytesTotal, const QString &currentFile);
    void finished(bool ok, const QString &error, const GameManifest &manifest);
    void statusMessage(const QString &msg);

private slots:
    void onManifestFinished();
    void onFileFinished();

private:
    void fail(const QString &err);
    void beginDiffAndDownload();
    void downloadNext();
    void finalizeSuccess();
    bool loadLocalVersionMap(QMap<QString, QString> &pathToMd5, QString *err);
    bool writeLocalVersionJson(QString *err);

    QNetworkAccessManager m_nam;
    QNetworkReply *m_reply = nullptr;
    bool m_cancelled = false;

    QString m_manifestUrl;
    QString m_gamesRoot;
    QString m_installDir;
    GameManifest m_manifest;

    QVector<GameManifestFile> m_downloadList;
    QStringList m_deleteList;
    int m_downloadIndex = 0;
    qint64 m_totalDownloadBytes = 0;
    qint64 m_doneDownloadBytes = 0;
};

#endif // GAMEPACKAGEUPDATER_H
