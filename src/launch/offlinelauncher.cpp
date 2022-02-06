//
// Created by nils on 11/4/21.
//

#include "offlinelauncher.h"

#include <QProcess>
#include <QDir>
#include <QDirIterator>
#include <QStandardPaths>
#include <QTemporaryFile>

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

    QStringList args{
         "--add-modules", "jdk.naming.dns",
         "--add-exports", "jdk.naming.dns/com.sun.jndi.dns=java.naming",
         "-Djna.boot.library.path=natives", "-Dlog4j2.formatMsgNoLookups=true",
         "--add-opens", "java.base/java.io=ALL-UNNAMED",
         QString("-Xms%1m").arg(config.initialMemory),
         QString("-Xmx%1m").arg(config.maximumMemory),
         "-Djava.library.path=natives",
         "-cp", QStringList({
                    "vpatcher-prod.jar",
                    "lunar-prod-optifine.jar",
                    "lunar-libs.jar",
                    "lunar-assets-prod-1-optifine.jar",
                    "lunar-assets-prod-2-optifine.jar",
                    "lunar-assets-prod-3-optifine.jar",
                    "OptiFine.jar",

                    Utils::getLibsDirectory() + QDir::separator() + '*'
        }).join(QDir::listSeparator())
    };

    foreach(const Agent& agent, config.agents)
        args << "-javaagent:" + agent.path + '=' + agent.option;


    if(config.useLevelHeadPrefix || config.useLevelHeadNick)
        args << Utils::getAgentFlags("LevelHeadImproved.incljar", getLevelHeadOptions(config.useLevelHeadPrefix, config.levelHeadPrefix, config.useLevelHeadNick, QString::number(config.levelHeadNickLevel)));

    if(config.useAutoggMessage)
        args << Utils::getAgentFlags("CustomAutoGG.incljar", config.autoggMessage);

    if (config.useBetterHurtCam)
        args << Utils::getAgentFlags("LunarBetterHurtCam.incljar", QString::number(config.betterHurtCamValue));

    if(config.useCosmetics && config.unlockCosmetics)
        args << Utils::getAgentFlags("UnlockCosmetics.incljar");

    args << QProcess::splitCommand(config.jvmArgs);

    args << QStringList{
            "com.moonsworth.lunar.patcher.LunarMain",
            "--version", config.gameVersion,
            "--accessToken", "0",
            "--assetIndex", config.gameVersion == QStringLiteral("1.7") ? "1.7.10" : config.gameVersion,
            "--userProperties", "{}",
            "--gameDir", config.useCustomMinecraftDir ? config.customMinecraftDir : minecraftDir,
            "--launcherVersion", "2.9.4",
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

    process.setProcessEnvironment(env);
    process.setWorkingDirectory(lunarDir + "/offline/" + config.gameVersion);
    process.setStandardOutputFile("latest.log", QIODeviceBase::Truncate);

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
