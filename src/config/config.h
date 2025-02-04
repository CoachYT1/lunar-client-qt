//
// Created by nils on 12/1/21.
//

#ifndef LUNAR_CLIENT_QT_CONFIG_H
#define LUNAR_CLIENT_QT_CONFIG_H

#include <QString>
#include <QStringList>
#include <QJsonObject>
#include <QFileInfo>

struct Agent {
    QString name;
    QString path;
    QString option;

    bool enabled;
public:
    Agent(const QString& path, const QString& option, bool enabled = true) :
        name(QFileInfo(path).fileName()),
        path(path),
        option(option),
        enabled(enabled)
    {}
};

class Config {
public:
    QString gameVersion;
    QString modLoader;


    bool keepMemorySame;
    int initialMemory;
    int maximumMemory;

    bool useCustomJre;
    QString customJrePath;

    QString jvmArgs;

    bool closeOnLaunch;

    bool useCustomMinecraftDir;
    QString customMinecraftDir;

    bool joinServerOnLaunch;
    QString serverIp;

    bool useAutoggMessage;
    QString autoggMessage;

    bool useLevelHeadPrefix;
    QString levelHeadPrefix;

    bool useLevelHeadNick;
    int levelHeadNickLevel;

    bool useBetterHurtCam;
    double betterHurtCamValue;

    int windowWidth;
    int windowHeight;

    bool useCosmetics;
    bool unlockCosmetics;

    QList<Agent> agents;
    QStringList helpers;
public:
    void save();
    static Config load();
private:
    static void saveJsonToConfig(const QJsonObject& jsonObject);
    static QJsonObject loadJsonFromConfig();


    Config() = default;

    static const QString configFilePath;
};


#endif //LUNAR_CLIENT_QT_CONFIG_H
