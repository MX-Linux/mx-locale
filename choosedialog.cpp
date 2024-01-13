#include <QFile>
#include <QMessageBox>
#include <QTextStream>

#include "choosedialog.h"
#include "cmd.h"
#include "ui_choosedialog.h"

chooseDialog::chooseDialog(QWidget *parent)
    : QDialog(parent),
      ui(new Ui::chooseDialog)
{
    ui->setupUi(this);
    setup();
}

chooseDialog::~chooseDialog()
{
    delete ui;
}

// Setup versious items first time program runs
void chooseDialog::setup()
{
    this->setWindowTitle(tr("MX Locale"));
    buildLocaleList();
    ui->textSearch->setFocus();
    connect(ui->textSearch, &QLineEdit::textChanged, this, &chooseDialog::textSearch_textChanged);
}

void chooseDialog::buildLocaleList()
{
    QFile libFile("/usr/lib/mx-locale/locale.lib");

    QString locales = Cmd().getOut("locale --all-locales |grep \\\\.");
    QStringList availableLocales = locales.split(QRegExp("(\\r\\n)|(\\n\\r)|\\r|\\n"), Qt::SkipEmptyParts);

    if (!libFile.open(QIODevice::ReadOnly)) {
        QMessageBox::critical(nullptr, tr("Error"), tr("Could not open %1").arg(libFile.fileName()));
        return;
    }

    QTextStream in(&libFile);
    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        QStringList list = line.split('-');
        if (list.size() == 2) {
            localeLib.insert(list.at(0).trimmed(), list.at(1).trimmed());
        }
    }
    libFile.close();

    for (const auto &locale : availableLocales) {
        QString item = locale;
        item.remove(QRegularExpression("\\..*$"));
        QString line = locale;
        if (localeLib.contains(item)) {
            line.append("                 \t" + localeLib.value(item));
        }
        localeList << line;
        ui->listWidgetAvailableLocales->addItem(line);
    }
}

QString chooseDialog::selection()
{
    QString selection = ui->listWidgetAvailableLocales->currentItem()->text().section('\t', 0, 0).trimmed();
    selection = selection.replace(".utf8", ".UTF-8", Qt::CaseInsensitive);
    return selection;
}

void chooseDialog::textSearch_textChanged()
{
    ui->listWidgetAvailableLocales->clear();

    for (const QString &itemText : localeList) {
        if (itemText.contains(ui->textSearch->text(), Qt::CaseInsensitive)) {
            ui->listWidgetAvailableLocales->addItem(itemText);
        }
    }
}
