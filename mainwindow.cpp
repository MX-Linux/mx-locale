/**********************************************************************
 *  mainwindow.cpp
 **********************************************************************
 * Copyright (C) 2024 MX Authors
 *
 * Authors: Dolphin Oracle
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

#include "mainwindow.h"
#include "about.h"
#include "ui_mainwindow.h"
#include "version.h"

#include <QDebug>
#include <QFileDialog>
#include <QProgressDialog>
#include <QScrollBar>
#include <QTextStream>

#include "choosedialog.h"
#include "cmd.h"
#include <unistd.h>

MainWindow::MainWindow(const QCommandLineParser &args, QWidget *parent)
    : QDialog(parent),
      ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    setWindowFlags(Qt::Window); // For the close, min and max buttons
    setup();
    if (args.isSet("only-lang")) {
        ui->tabWidget->setTabVisible(1, false);
        ui->tabWidget->setTabVisible(2, false);
    }
    this->adjustSize();
}

MainWindow::~MainWindow()
{
    if (localeGenChanged) {
        localeGen();
    }
    delete ui;
}

// Setup versious items first time program runs
void MainWindow::setup()
{
    this->setWindowTitle(tr("MX Locale"));
    ui->tabWidget->setCurrentIndex(0);
    ui->buttonLang->setText(getCurrentLang());
    setSubvariables();
    setButtons();
    setConnections();
}

void MainWindow::onGroupButton(int button_id)
{
    chooseDialog dialog;
    dialog.setModal(true);
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }
    buttonGroup->button(button_id)->setText(dialog.selection().section('\t', 0, 0).trimmed());
    QHash<int, QString> hashVarName {
        {ButtonID::Lang, "LANG"},
        {ButtonID::Address, "LC_ADDRESS"},
        {ButtonID::Collate, "LC_COLLATE"},
        {ButtonID::CType, "LC_CTYPE"},
        {ButtonID::Identification, "LC_IDENTIFICATION"},
        {ButtonID::Measurement, "LC_MEASUREMENT"},
        {ButtonID::Messages, "LC_MESSAGES"},
        {ButtonID::Monetary, "LC_MONETARY"},
        {ButtonID::Name, "LC_NAME"},
        {ButtonID::Numeric, "LC_NUMERIC"},
        {ButtonID::Paper, "LC_PAPER"},
        {ButtonID::Telephone, "LC_TELEPHONE"},
        {ButtonID::Time, "LC_TIME"},
    };
    Cmd().runAsRoot("update-locale " + hashVarName.value(button_id) + '=' + buttonGroup->button(button_id)->text());
}

void MainWindow::aboutClicked()
{
    this->hide();
    displayAboutMsgBox(tr("About %1").arg(this->windowTitle()),
                       "<p align=\"center\"><b><h2>" + this->windowTitle() + "</h2></b></p><p align=\"center\">"
                           + tr("Version: ") + VERSION + "</p><p align=\"center\"><h3>"
                           + tr("Program for changing language and locale variables")
                           + "</h3></p><p align=\"center\"><a href=\"http://mxlinux.org\">http://mxlinux.org</a><br "
                             "/></p><p align=\"center\">"
                           + tr("Copyright (c) MX Linux") + "<br /><br /></p>",
                       "/usr/share/doc/mx-locale/license.html", tr("%1 License").arg(this->windowTitle()), true);
    this->show();
}

void MainWindow::helpClicked()
{
    QString url = "file:///usr/share/doc/mx-locale/help/mx-locale.html";
    displayDoc(url, tr("%1 Help").arg(this->windowTitle()), true);
}

QString MainWindow::getCurrentLang() const
{
    QSettings defaultlocale("/etc/default/locale", QSettings::NativeFormat);
    return defaultlocale.value("LANG", "C").toString();
}

void MainWindow::disableAllButCurrent()
{
    Cmd().runAsRoot("sed -i \"/^$LANG\\|^#/! s/#*/# /\" /etc/locale.gen");
    localeGenChanged = true;
    onFilterChanged(ui->comboFilter->currentText());
}

void MainWindow::setSubvariables()
{

    QSettings defaultlocale("/etc/default/locale", QSettings::NativeFormat);

    QString lang = ui->buttonLang->text();

    QString ctype = defaultlocale.value("LC_CTYPE", lang).toString();
    QString numeric = defaultlocale.value("LC_NUMERIC", lang).toString();
    QString time = defaultlocale.value("LC_TIME", lang).toString();
    QString collate = defaultlocale.value("LC_COLLATE", lang).toString();
    QString monetary = defaultlocale.value("LC_MONETARY", lang).toString();
    QString messages = defaultlocale.value("LC_MESSAGES", lang).toString();
    QString paper = defaultlocale.value("LC_PAPER", lang).toString();
    QString name = defaultlocale.value("LC_NAME", lang).toString();
    QString address = defaultlocale.value("LC_ADDRESS", lang).toString();
    QString telephone = defaultlocale.value("LC_TELEPHONE", lang).toString();
    QString measurement = defaultlocale.value("LC_MEASUREMENT", lang).toString();
    QString identification = defaultlocale.value("LC_IDENTIFICATION", lang).toString();

    ui->pushButtonCType->setText(ctype);
    ui->pushButtonNumeric->setText(numeric);
    ui->pushButtonTime->setText(time);
    ui->pushButtonCollate->setText(collate);
    ui->pushButtonMonetary->setText(monetary);
    ui->pushButtonMessages->setText(messages);
    ui->pushButtonPaper->setText(paper);
    ui->pushButtonName->setText(name);
    ui->pushButtonAddress->setText(address);
    ui->pushButtonTelephone->setText(telephone);
    ui->pushButtonMeasurement->setText(measurement);
    ui->pushButtonIdentification->setText(identification);
}

void MainWindow::setButtons()
{
    buttonGroup = new QButtonGroup(this);
    buttonGroup->addButton(ui->pushButtonAddress, ButtonID::Address);
    buttonGroup->addButton(ui->pushButtonCollate, ButtonID::Collate);
    buttonGroup->addButton(ui->pushButtonCType, ButtonID::CType);
    buttonGroup->addButton(ui->pushButtonIdentification, ButtonID::Identification);
    buttonGroup->addButton(ui->pushButtonMeasurement, ButtonID::Measurement);
    buttonGroup->addButton(ui->pushButtonMessages, ButtonID::Messages);
    buttonGroup->addButton(ui->pushButtonMonetary, ButtonID::Monetary);
    buttonGroup->addButton(ui->pushButtonName, ButtonID::Name);
    buttonGroup->addButton(ui->pushButtonNumeric, ButtonID::Numeric);
    buttonGroup->addButton(ui->pushButtonPaper, ButtonID::Paper);
    buttonGroup->addButton(ui->pushButtonTelephone, ButtonID::Telephone);
    buttonGroup->addButton(ui->pushButtonTime, ButtonID::Time);
    buttonGroup->addButton(ui->buttonLang, ButtonID::Lang);
}

void MainWindow::setConnections()
{
    connect(buttonGroup, &QButtonGroup::idClicked, this, &MainWindow::onGroupButton);
    connect(ui->buttonAbout, &QPushButton::clicked, this, &MainWindow::aboutClicked);
    connect(ui->buttonHelp, &QPushButton::clicked, this, &MainWindow::helpClicked);
    connect(ui->comboFilter, &QComboBox::currentTextChanged, this, &MainWindow::onFilterChanged);
    connect(ui->listWidget, &QListWidget::itemChanged, this, &MainWindow::listItemChanged);
    connect(ui->pushDisableLocales, &QPushButton::clicked, this, &MainWindow::disableAllButCurrent);
    connect(ui->tabWidget, &QTabWidget::currentChanged, this, &MainWindow::tabWidgetCurrentChanged);
}

void MainWindow::tabWidgetCurrentChanged()
{
    if (ui->tabWidget->currentWidget() == ui->tabManagement) {
        ui->labelCurrentLocale->setText(tr("Locale in use: <b>%1</b>").arg(getCurrentLang()));
        displayLocalesGen();
    }
    if (localeGenChanged) {
        localeGen();
    }
}

void MainWindow::onFilterChanged(const QString &text)
{
    displayLocalesGen();
    if (text == tr("All")) {
        return;
    }
    for (int i = 0; i < ui->listWidget->count(); ++i) {
        auto *item = ui->listWidget->item(i);
        if (item) {
            item->setHidden((item->checkState() == Qt::Checked && text == tr("Disabled"))
                            || (item->checkState() == Qt::Unchecked && text == tr("Enabled")));
        }
    }
}

void MainWindow::listItemChanged(QListWidgetItem *item)
{
    ui->listWidget->disconnect();
    localeGenChanged = true;
    if (item->checkState() == Qt::Checked) {
        QString uncommentedText = item->text().remove(QRegularExpression("^# "));
        Cmd().runAsRoot("sed -i 's/" + item->text() + "/" + uncommentedText + "/' /etc/locale.gen");
        item->setText(uncommentedText);
        ++countEnabled;
    } else {
        QString commentedText = "# " + item->text();
        Cmd().runAsRoot("sed -i 's/" + item->text() + "/" + commentedText + "/' /etc/locale.gen");
        item->setText(commentedText);
        --countEnabled;
    }
    ui->labelCountLocale->setText(tr("Locales enabled: %1").arg(countEnabled));
    connect(ui->listWidget, &QListWidget::itemChanged, this, &MainWindow::listItemChanged);
    if (ui->comboFilter->currentText() != tr("All")) {
        onFilterChanged(ui->comboFilter->currentText());
    }
}

void MainWindow::displayLocalesGen()
{
    countEnabled = 0;
    ui->listWidget->clear();
    QFile file("/etc/locale.gen");
    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::critical(nullptr, tr("Error"), tr("Could not open %1").arg("/etc/locale.gen"));
        return;
    }
    QTextStream in(&file);
    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        if (line.contains(QRegularExpression("^# .._"))) {
            auto item = new QListWidgetItem;
            item->setCheckState(Qt::Unchecked);
            item->setText(line);
            ui->listWidget->addItem(item);
        } else if (line.contains(QRegularExpression("^.._"))) {
            auto item = new QListWidgetItem;
            item->setCheckState(Qt::Checked);
            item->setText(line);
            ui->listWidget->addItem(item);
            ++countEnabled;
        }
    }
    ui->labelCountLocale->setText(tr("Locales enabled: %1").arg(countEnabled));
}

void MainWindow::localeGen()
{
    ui->tabWidget->setDisabled(true);
    QProgressDialog prog("Updating locales, please wait", nullptr, 0, countEnabled + 1);
    cmd = new Cmd(this);
    connect(cmd, &Cmd::outputAvailable, this, [&prog] { prog.setValue(prog.value() + 1); });
    prog.show();
    cmd->runAsRoot("locale-gen");
    localeGenChanged = false;
    ui->tabWidget->setDisabled(false);
}
