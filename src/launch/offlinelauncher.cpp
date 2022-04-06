//
// Created by nils on 11/4/21.
//

#include "offlinelauncher.h"

#include <QProcess>
#include <QDir>
#include <QDirIterator>
#include <QStandardPaths>
#include <QTemporaryFile>
#include <QElapsedTimer>
#include <QIODevice>

#include "utils.h"

const QString OfflineLauncher::lunarDir = QDir::homePath() + "/.lunarclient";
const QString OfflineLauncher::minecraftDir =
#ifdef Q_OS_WIN
        QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/.minecraft";
#elif defined(Q_OS_DARWIN)
        QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/minecraft";
#else
        QDir::homePath() + "/.minecraft";
#endif

OfflineLauncher::OfflineLauncher(const Config& config, QObject *parent) : Launcher(config, parent) {
}


void OfflineLauncher::launch() {
    QProcess process;
    process.setProgram(config.useCustomJre ? config.customJrePath : findJavaExecutable(config.gameVersion));

    process.setStandardInputFile(QProcess::nullDevice());
    process.setStandardOutputFile(QProcess::nullDevice());
    process.setStandardErrorFile(QProcess::nullDevice());

    QString workingDir = lunarDir + "/offline/" + config.gameVersion;

    process.setWorkingDirectory(workingDir);

    QStringList classPath = QDir(workingDir).entryList(QDir::Files, QDir::Time);

    QFileInfoList libsList = QDir(Utils::getLibsDirectory()).entryInfoList(QDir::Files);

    foreach(const QFileInfo& info, libsList) {
        classPath << info.absoluteFilePath();
    }

    QStringList args{
         "--add-modules", "jdk.naming.dns",
         "--add-exports", "jdk.naming.dns/com.sun.jndi.dns=java.naming",
         "-Djna.boot.library.path=natives",
         "-Dlog4j2.formatMsgNoLookups=true",
         "--add-opens", "java.base/java.io=ALL-UNNAMED",
         QString("-Xms%1m").arg(config.initialMemory),
         QString("-Xmx%1m").arg(config.maximumMemory),
         "-Djava.library.path=natives",
         "-cp", classPath.join(QDir::listSeparator())
    };

    foreach(const Agent& agent, config.agents)
        args << "-javaagent:" + agent.path + '=' + agent.option;


    if(config.useLevelHeadPrefix || config.useLevelHeadNick)
        args << Utils::getAgentFlags("LevelHeadImproved", getLevelHeadOptions(config.useLevelHeadPrefix, config.levelHeadPrefix, config.useLevelHeadNick, QString::number(config.levelHeadNickLevel)));

    if(config.useAutoggMessage)
        args << Utils::getAgentFlags("CustomAutoGG", config.autoggMessage);

    if (config.useBetterHurtCam)
        args << Utils::getAgentFlags("LunarBetterHurtCam", QString::number(config.betterHurtCamValue));

    if(config.useCosmetics && config.unlockCosmetics)
        args << Utils::getAgentFlags("UnlockCosmetics");

    args << QProcess::splitCommand(config.jvmArgs);

    args << QStringList{
            "com.moonsworth.lunar.patcher.LunarMain",
            "--version", config.gameVersion,
            "--accessToken", "0",
            "--assetIndex", Utils::getAssetsIndex(config.gameVersion),
            "--userProperties", "{}",
            "--gameDir", config.useCustomMinecraftDir ? config.customMinecraftDir : minecraftDir,
            "--launcherVersion", "2.10.0",
            "--width", QString::number(config.windowWidth),
            "--height", QString::number(config.windowHeight)
    };

    if(config.useCosmetics)
        args << "--texturesDir" << lunarDir + "/textures";

    if(config.joinServerOnLaunch)
        args << "--server" << config.serverIp;

    process.setArguments(args);

    //Removes the windir environment variable, preventing lunar from reading your hosts file and executing tasklist on windows

    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    env.remove("windir");
    env.remove("JAVA_OPTS");
    env.remove("_JAVA_OPTS");
    env.remove("JAVA_OPTIONS");
    env.remove("_JAVA_OPTIONS");

    process.setProcessEnvironment(env);
    process.setWorkingDirectory(lunarDir + "/offline/" + config.gameVersion);
    process.setStandardOutputFile("latest.log", QIODevice::Truncate);
    process.setStandardErrorFile("error.log", QIODevice::Truncate);

    if(!process.startDetached()){
        emit error("Failed to start process: " + process.errorString());
    }

    if (!config.helpers.isEmpty())
    {
        foreach(const QString & path, config.helpers)
            HelperLaunch(path);
    }
}

QString OfflineLauncher::findJavaExecutable(const QString& version) {
    QDirIterator versionSpecificIt(lunarDir+"/jre/"+version, QDir::Dirs | QDir::NoDotAndDotDot);

    while(versionSpecificIt.hasNext()){
        QString potentialExecutable = versionSpecificIt.next() +
#ifdef Q_OS_WIN
        "/bin/java.exe";
#else
        "/bin/java";
#endif

        if(QFileInfo(potentialExecutable).isExecutable())
            return potentialExecutable;
    }


    QDirIterator generalIt(lunarDir+"/jre", QDir::Dirs | QDir::NoDotAndDotDot);

    while(generalIt.hasNext()){
        QString potentialExecutable = generalIt.next() +
#ifdef Q_OS_WIN
        "/bin/java.exe";
        #else
        "/bin/java";
#endif

        if(QFileInfo(potentialExecutable).isExecutable())
            return potentialExecutable;
    }

    return {};
}

void OfflineLauncher::HelperLaunch(const QString& helper) {
    QProcess process;
#ifdef Q_OS_WIN
    process.setCreateProcessArgumentsModifier(([] (QProcess::CreateProcessArguments* args)
    {
        args->flags |= 0x00000010; //CREATE_NEW_CONSOLE
    }));
#endif
    process.setProgram(helper);
    process.startDetached(); 
}
