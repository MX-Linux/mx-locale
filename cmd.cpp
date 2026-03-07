#include "cmd.h"

#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QEventLoop>
#include <QFileInfo>
#include <QProcessEnvironment>
#include <QStandardPaths>

#include "common.h"
#include <unistd.h>

namespace {
QString resolveExecutable(const QString &program)
{
    if (QFileInfo(program).isAbsolute()) {
        return program;
    }

    const QString resolved = QStandardPaths::findExecutable(program, {"/usr/sbin", "/usr/bin", "/sbin", "/bin"});
    return resolved.isEmpty() ? program : resolved;
}
} // namespace

Cmd::Cmd(QObject *parent)
    : QProcess(parent),
      elevate {QFile::exists("/usr/bin/pkexec") ? "/usr/bin/pkexec" : "/usr/bin/gksu"},
      helper {QDir(Paths::usrLib).filePath(QCoreApplication::applicationName() + "/helper")}
{
    connect(this, &Cmd::readyReadStandardOutput, [this] { emit outputAvailable(readAllStandardOutput()); });
    connect(this, &Cmd::readyReadStandardError, [this] { emit errorAvailable(readAllStandardError()); });
    connect(this, &Cmd::outputAvailable, [this](const QString &out) { outBuffer += out; });
    connect(this, &Cmd::errorAvailable, [this](const QString &out) { outBuffer += out; });
}

QString Cmd::getOut(const QString &program, const QStringList &arguments, bool quiet, bool asRoot)
{
    outBuffer.clear();
    run(program, arguments, quiet, asRoot);
    return outBuffer.trimmed();
}

QString Cmd::getOutAsRoot(const QString &program, const QStringList &arguments, bool quiet)
{
    return getOut(program, arguments, quiet, true);
}

bool Cmd::run(const QString &program, const QStringList &arguments, bool quiet, bool asRoot, const QByteArray &stdinData)
{
    outBuffer.clear();
    if (state() != QProcess::NotRunning) {
        qDebug() << "Process already running:" << this->program() << this->arguments();
        return false;
    }
    const QString resolvedProgram = resolveExecutable(program);
    QStringList resolvedArguments = arguments;
    if (asRoot && getuid() != 0) {
        setProgram(elevate);
        QStringList helperArguments {helper, resolvedProgram};
        helperArguments += resolvedArguments;
        setArguments(helperArguments);
    } else {
        setProgram(resolvedProgram);
        setArguments(resolvedArguments);
    }
    if (!quiet) {
        qDebug().noquote() << this->program() << this->arguments();
    }

    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    env.insert("LC_ALL", "C.UTF-8");
    setProcessEnvironment(env);

    QEventLoop loop;
    connect(this, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), &loop, &QEventLoop::quit);
    connect(this, &QProcess::errorOccurred, &loop, &QEventLoop::quit);
    start();
    if (!stdinData.isEmpty()) {
        write(stdinData);
        closeWriteChannel();
    }
    loop.exec();
    emit done();
    return (exitStatus() == QProcess::NormalExit && exitCode() == 0);
}

bool Cmd::runAsRoot(const QString &program, const QStringList &arguments, bool quiet, const QByteArray &stdinData)
{
    return run(program, arguments, quiet, true, stdinData);
}

QString Cmd::readAllOutput()
{
    return outBuffer.trimmed();
}
