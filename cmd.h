#pragma once

#include <QProcess>
#include <QStringList>

class QTextStream;

class Cmd : public QProcess
{
    Q_OBJECT

public:
    explicit Cmd(QObject *parent = nullptr);

    [[nodiscard]] QString getOut(const QString &program, const QStringList &arguments = {}, bool quiet = false,
                                 bool asRoot = false);
    [[nodiscard]] QString getOutAsRoot(const QString &program, const QStringList &arguments = {}, bool quiet = false);
    [[nodiscard]] QString readAllOutput();
    bool run(const QString &program, const QStringList &arguments = {}, bool quiet = false, bool asRoot = false,
             const QByteArray &stdinData = {});
    bool runAsRoot(const QString &program, const QStringList &arguments = {}, bool quiet = false,
                   const QByteArray &stdinData = {});

signals:
    void done();
    void errorAvailable(const QString &err);
    void outputAvailable(const QString &out);

private:
    QString elevate;
    QString helper;
    QString outBuffer;
};
