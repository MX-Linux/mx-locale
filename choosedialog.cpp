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

// Setup various items first time program runs
void chooseDialog::setup()
{
    this->setWindowTitle(tr("MX Locale", "name of application"));
    buildLocaleList();
    ui->textSearch->setFocus();
    connect(ui->textSearch, &QLineEdit::textChanged, this, &chooseDialog::textSearch_textChanged);
}

void chooseDialog::buildLocaleList()
{
    QFile libFile("/usr/lib/mx-locale/locale.lib");
    QString locales = Cmd().getOut(R"(locale --all-locales)");
    QStringList availableLocales = locales.split(QRegularExpression(R"((\r\n)|(\n\r)|\r|\n)"), Qt::SkipEmptyParts)
                                       .filter(QRegularExpression(R"(\.(utf8|UTF-8)$)"))
                                       .replaceInStrings(".utf8", ".UTF-8", Qt::CaseInsensitive);

    if (!libFile.open(QIODevice::ReadOnly)) {
        QMessageBox::critical(this, tr("Error"),
                              tr("Could not open %1", "message that a file could not be opened, file takes place of %1")
                                  .arg(libFile.fileName()));
        return;
    }

    QTextStream in(&libFile);
    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        QStringList list = line.split('-', Qt::SkipEmptyParts);
        if (list.size() == 2) {
            localeLib.insert(list.at(0).trimmed(), list.at(1).trimmed());
        }
    }
    libFile.close();

    for (const auto &locale : std::as_const(availableLocales)) {
        QString item = locale;
        item.remove(QRegularExpression("\\..*$"));
        QString line = locale.leftJustified(20, ' ');
        if (localeLib.contains(item)) {
            line.append('\t' + localeLib.value(item));
        }
        ui->listWidgetAvailableLocales->addItem(line);
    }
}

QString chooseDialog::selection() const
{
    if (ui->listWidgetAvailableLocales->currentRow() == -1) {
        return {};
    }
    QString selection = ui->listWidgetAvailableLocales->currentItem()->text().section('\t', 0, 0).trimmed();
    selection = selection.replace(".utf8", ".UTF-8", Qt::CaseInsensitive);
    return selection;
}

void chooseDialog::textSearch_textChanged()
{
    QString searchText = ui->textSearch->text();
    for (int i = 0; i < ui->listWidgetAvailableLocales->count(); ++i) {
        auto *item = ui->listWidgetAvailableLocales->item(i);
        if (item) {
            bool shouldHide = !item->text().contains(searchText, Qt::CaseInsensitive);
            item->setHidden(shouldHide);
        }
    }
}
