/**********************************************************************
 *
 **********************************************************************
 * Copyright (C) 2024 MX Authors
 *
 * Authors: Adrian <adrian@mxlinux.org>
 *          MX Linux <http://mxlinux.org>
 *
 * This is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this package. If not, see <http://www.gnu.org/licenses/>.
 **********************************************************************/
#include "about.h"

#include <QApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QMessageBox>
#include <QProcess>
#include <QPushButton>
#include <QStandardPaths>
#include <QTextEdit>
#include <QVBoxLayout>

#include "common.h"
#include <pwd.h>
#include <sys/types.h>
#include <unistd.h>

namespace
{
class ScopedHomeOverride
{
public:
    explicit ScopedHomeOverride(const QString &home)
        : originalHome(qgetenv("HOME"))
    {
        if (!home.isEmpty()) {
            qputenv("HOME", home.toUtf8());
            active = true;
        }
    }

    ~ScopedHomeOverride()
    {
        if (active) {
            qputenv("HOME", originalHome);
        }
    }

private:
    QByteArray originalHome;
    bool active {false};
};

QString homeForUid(uid_t uid)
{
    if (const passwd *pwd = getpwuid(uid); pwd != nullptr && pwd->pw_dir != nullptr) {
        return QFile::decodeName(pwd->pw_dir);
    }
    return {};
}

QString homeForUser(const QString &user)
{
    if (user.isEmpty()) {
        return {};
    }

    const QByteArray userName = user.toLocal8Bit();
    if (const passwd *pwd = getpwnam(userName.constData()); pwd != nullptr && pwd->pw_dir != nullptr) {
        return QFile::decodeName(pwd->pw_dir);
    }
    return {};
}

QString lognameUser()
{
    QProcess proc;
    proc.start("logname", {}, QIODevice::ReadOnly);
    if (!proc.waitForStarted(3000) || !proc.waitForFinished(3000)) {
        return {};
    }
    return QString::fromUtf8(proc.readAllStandardOutput()).trimmed();
}

QString userForUid(uid_t uid)
{
    if (const passwd *pwd = getpwuid(uid); pwd != nullptr && pwd->pw_name != nullptr) {
        return QString::fromLocal8Bit(pwd->pw_name);
    }
    return {};
}

QString invokingUser()
{
    bool ok = false;
    const uint pkexecUid = qEnvironmentVariableIntValue("PKEXEC_UID", &ok);
    if (ok) {
        return userForUid(pkexecUid);
    }

    const QString sudoUser = qEnvironmentVariable("SUDO_USER");
    if (!sudoUser.isEmpty()) {
        return sudoUser;
    }

    const uint sudoUid = qEnvironmentVariableIntValue("SUDO_UID", &ok);
    if (ok) {
        return userForUid(sudoUid);
    }

    return lognameUser();
}

bool startDetachedForUser(const QString &program, const QStringList &arguments, const QString &user)
{
    if (getuid() != 0 || user.isEmpty()) {
        return QProcess::startDetached(program, arguments);
    }

    QStringList runUserArguments {"-u", user, "--", program};
    runUserArguments += arguments;
    return QProcess::startDetached("runuser", runUserArguments);
}
} // namespace

// Display doc as nomal user when run as root
void displayDoc(const QString &url, const QString &title)
{
    const QByteArray rootHome("/root");
    const QString user = getuid() == 0 ? invokingUser() : QString();
    const QString userHome = qgetenv("HOME") == rootHome ? homeForUser(user) : QString();
    const ScopedHomeOverride homeOverride(userHome); // Use invoking user's home for theming purposes
    // Prefer mx-viewer otherwise use xdg-open (use runuser to run that as logname user)
    const QString executablePath = QStandardPaths::findExecutable("mx-viewer");
    if (!executablePath.isEmpty()) {
        startDetachedForUser("mx-viewer", {url, title}, user);
    } else {
        startDetachedForUser("xdg-open", {url}, user);
    }
}

void displayAboutMsgBox(const QString &title, const QString &message, const QString &licenceUrl,
                        const QString &licenseTitle)
{
    const auto width = 600;
    const auto height = 500;
    QMessageBox msgBox(QMessageBox::NoIcon, title, message);
    auto *btnLicense = msgBox.addButton(QObject::tr("License"), QMessageBox::HelpRole);
    auto *btnChangelog = msgBox.addButton(QObject::tr("Changelog"), QMessageBox::HelpRole);
    auto *btnCancel = msgBox.addButton(QObject::tr("Cancel"), QMessageBox::NoRole);
    btnCancel->setIcon(QIcon::fromTheme("window-close"));

    msgBox.exec();

    if (msgBox.clickedButton() == btnLicense) {
        displayDoc(licenceUrl, licenseTitle);
    } else if (msgBox.clickedButton() == btnChangelog) {
        auto *changelog = new QDialog;
        changelog->setWindowTitle(QObject::tr("Changelog"));
        changelog->resize(width, height);

        auto *text = new QTextEdit(changelog);
        text->setReadOnly(true);
        QProcess proc;
        const QString appName = QFileInfo(QCoreApplication::applicationFilePath()).fileName();
        const QString changelogPath = QDir(Paths::usrShareDoc).filePath(appName + "/changelog.gz");
        proc.start("zcat", {changelogPath}, QIODevice::ReadOnly);
        if (proc.waitForStarted(3000) && proc.waitForFinished(3000)) {
            text->setText(proc.readAllStandardOutput());
        } else {
            text->setText(QObject::tr("Could not load changelog."));
        }

        auto *btnClose = new QPushButton(QObject::tr("&Close"), changelog);
        btnClose->setIcon(QIcon::fromTheme("window-close"));
        QObject::connect(btnClose, &QPushButton::clicked, changelog, &QDialog::close);

        auto *layout = new QVBoxLayout(changelog);
        layout->addWidget(text);
        layout->addWidget(btnClose);
        changelog->setLayout(layout);
        changelog->exec();
    }
}
