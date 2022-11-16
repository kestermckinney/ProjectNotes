#include "spellcheckdialog.h"
#include "ui_spellcheckdialog.h"
#include "pnsettings.h"
#include <QMessageBox>
#include <QDebug>

SpellCheckDialog::SpellCheckDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SpellCheckDialog)
{
    ui->setupUi(this);

    QSettings spell_settings(QCoreApplication::applicationDirPath() + "/dictionary/index.ini", QSettings::IniFormat);

    QStringList dictionaries = spell_settings.childGroups();

    int index = 0;
    QVariant dicname;
    QVariant afffile;
    QVariant dicfile;
    QString keyname;

    for ( const QString& dictionary : dictionaries )
    {
        keyname = QString("%1/%2").arg(dictionary,"Name");
        dicname = spell_settings.value(keyname);
        keyname = QString("%1/%2").arg(dictionary,"Aff");
        afffile = spell_settings.value(keyname);
        keyname = QString("%1/%2").arg(dictionary,"Dic");
        dicfile = spell_settings.value(keyname);

        m_DictionaryNames.append(dicname.toString());
        m_DicFiles.append(dicfile.toString());
        m_AffFiles.append(afffile.toString());

        ui->comboBoxDictionaryLanguage->addItem(dicname.toString());

        index++;
    }

    m_DefaultDictionary = global_Settings.getDefaultDictionary().toInt();

    ui->comboBoxDictionaryLanguage->setCurrentIndex(m_DefaultDictionary);

    QByteArray dictFilePathBA = QString(QCoreApplication::applicationDirPath() + "/dictionary/" + m_DicFiles[m_DefaultDictionary]).toLocal8Bit();
    QByteArray affixFilePathBA = QString(QCoreApplication::applicationDirPath() + "/dictionary/" + m_AffFiles[m_DefaultDictionary]).toLocal8Bit();

    m_spellchecker = new Hunspell(affixFilePathBA.constData(), dictFilePathBA.constData());

    if (m_DictionaryNames.count())
        this->setWindowTitle("Spelling: " + m_DictionaryNames[m_DefaultDictionary]);

    LoadPersonalWordList();
}

void SpellCheckDialog::LoadPersonalWordList()
{
    QString personalwords = global_Settings.getPersonalDictionary();

    QStringList wordlist = personalwords.split(":");

    for ( const QString& word : wordlist )
        m_spellchecker->add(word.toStdString());
}

void SpellCheckDialog::AddToPersonalWordList(QString& t_word)
{
    QString personalwords = global_Settings.getPersonalDictionary();

    if (!personalwords.isEmpty())
        personalwords += ":";

    personalwords += t_word;

    global_Settings.setPersonalDictionary(personalwords);
}
void SpellCheckDialog::spellCheck(QWidget* t_focus_control)
{
    SpellCheckDialog::SpellCheckAction spellResult;
    m_check_widget = dynamic_cast<QTextEdit*>(t_focus_control);

//    QTextCharFormat highlightFormat;
//    highlightFormat.setBackground(QBrush(QColor(0xff, 0x60, 0x60)));
//    highlightFormat.setForeground(QBrush(QColor(0, 0, 0)));

    // save the position of the current cursor
    QTextCursor oldCursor = m_check_widget->textCursor();

    // create a new cursor to walk through the text
    QTextCursor cursor(m_check_widget->document());

    QList<QTextEdit::ExtraSelection> esList;

    // Don't call cursor.beginEditBlock(), as this prevents the redraw after
    // changes to the content cursor.beginEditBlock();
    while (!cursor.atEnd())
    {
        QCoreApplication::processEvents();
        cursor.movePosition(QTextCursor::EndOfWord, QTextCursor::KeepAnchor, 1);
        QString word = cursor.selectedText();

        // Workaround for better recognition of words
        // punctuation etc. does not belong to words
        while (!word.isEmpty() &&
             !word.at(0).isLetter() &&
             cursor.anchor() < cursor.position())
        {
            int cursorPos = cursor.position();
            cursor.setPosition(cursor.anchor() + 1, QTextCursor::MoveAnchor);
            cursor.setPosition(cursorPos, QTextCursor::KeepAnchor);
            word = cursor.selectedText();
        }

        if (!word.isEmpty() && !m_spellchecker->spell(word.toStdString()))
        {
            QTextCursor tmpCursor(cursor);
            tmpCursor.setPosition(cursor.anchor());
            m_check_widget->setTextCursor(tmpCursor);
            m_check_widget->ensureCursorVisible();

            // highlight the unknown word
            QTextEdit::ExtraSelection es;
            es.cursor = cursor;
            //es.format = highlightFormat;

            esList << es;
            m_check_widget->setExtraSelections(esList);
            QCoreApplication::processEvents();

            // ask the user what to do
            spellResult = checkWord(word);

            // reset the word highlight
            esList.clear();

            m_check_widget->setExtraSelections(esList);

            QCoreApplication::processEvents();

            if (spellResult == SpellCheckDialog::AbortCheck)
                break;

            switch (spellResult)
            {
            case SpellCheckDialog::Change:
                qDebug() << "Change Text: " << ui->lineEditChange->text();
                cursor.insertText(ui->lineEditChange->text());
                break;
            case SpellCheckDialog::ChangeAll:
                replaceAll(cursor.position(), m_unknown_word, ui->lineEditChange->text());
                break;

            default:
                break;
            }

            QCoreApplication::processEvents();
        }
        cursor.movePosition(QTextCursor::NextWord, QTextCursor::MoveAnchor, 1);
    }

    m_check_widget->setTextCursor(oldCursor);

    if (spellResult != SpellCheckDialog::AbortCheck)
        QMessageBox::information(
              this,
              tr("Finished"),
              tr("Spell check has finished."));
}

SpellCheckDialog::~SpellCheckDialog()
{
    if (m_spellchecker)
        delete m_spellchecker;

    delete ui;
}

QStringList SpellCheckDialog::suggest(const QString &t_word)
{
    QStringList suggestions;
    std::vector<std::string> wordlist = m_spellchecker->suggest(t_word.toStdString());

    for (std::vector<std::string>::const_iterator i = wordlist.begin(); i != wordlist.end(); ++i)
        suggestions.append(QString::fromStdString(*i));

    return suggestions;
}

SpellCheckDialog::SpellCheckAction SpellCheckDialog::checkWord(const QString &t_word)
{
    m_unknown_word = t_word;

    ui->lineEditChange->setText(m_unknown_word);

    QStringList suggestions = suggest(t_word);

    ui->listWidgetSuggestions->clear();
    ui->listWidgetSuggestions->addItems(suggestions);

    if (suggestions.count() > 0)
        ui->listWidgetSuggestions->setCurrentRow(0);

    m_return_code = AbortCheck;
    QDialog::exec();

    // set the default configured dictionary
    global_Settings.setDefaultDictionary(QString("%1").arg(m_DefaultDictionary));

    return m_return_code;
}

void SpellCheckDialog::on_comboBoxDictionaryLanguage_currentIndexChanged(int index)
{
    if (m_spellchecker)
        delete m_spellchecker;

    m_DefaultDictionary = index;

    if (m_DictionaryNames.count())
        this->setWindowTitle("Spelling: " + m_DictionaryNames[m_DefaultDictionary]);

    QByteArray dictFilePathBA = QString(QCoreApplication::applicationDirPath() + "/dictionary/" + m_DicFiles[m_DefaultDictionary]).toLocal8Bit();
    QByteArray affixFilePathBA = QString(QCoreApplication::applicationDirPath() + "/dictionary/" + m_AffFiles[m_DefaultDictionary]).toLocal8Bit();

    m_spellchecker = new Hunspell(affixFilePathBA.constData(), dictFilePathBA.constData());

    LoadPersonalWordList();
}

void SpellCheckDialog::on_lineEditChange_returnPressed()
{
    m_return_code = Change;
    accept();
}

void SpellCheckDialog::on_pushButtonIgnoreOnce_clicked()
{
    m_return_code = IgnoreOnce;
    accept();
}

void SpellCheckDialog::on_pushButtonIgnoreAll_clicked()
{
    m_spellchecker->add(m_unknown_word.toStdString());
    m_return_code = IgnoreAll;
    accept();
}


void SpellCheckDialog::on_pushButtonAddToDictionary_clicked()
{
    m_spellchecker->add(m_unknown_word.toStdString());
    AddToPersonalWordList(m_unknown_word);

    m_return_code = AddToDict;
    accept();
}


void SpellCheckDialog::on_pushButtonChange_clicked()
{
    if (m_unknown_word == ui->lineEditChange->text() ) // if a new value wasn't typed in used the one selcted
    {
        if ( ui->listWidgetSuggestions->selectedItems().count() > 0 )
            ui->lineEditChange->setText( ui->listWidgetSuggestions->currentItem()->text() );
    }

    qDebug() << "Set Text: " << ui->lineEditChange->text();

    m_return_code = Change;
    accept();

}

void SpellCheckDialog::replaceAll(int t_position, const QString &t_old_word, const QString &t_new_word)
{
    QTextCursor cursor(m_check_widget->document());
    cursor.setPosition(t_position-t_old_word.length(), QTextCursor::MoveAnchor);

    while (!cursor.atEnd())
    {
        QCoreApplication::processEvents();
        cursor.movePosition(QTextCursor::EndOfWord, QTextCursor::KeepAnchor, 1);
        QString word = cursor.selectedText();

        // Workaround for better recognition of words
        // punctuation etc. does not belong to words
        while (!word.isEmpty() &&
             !word.at(0).isLetter() &&
             cursor.anchor() < cursor.position())
        {
            int cursorPos = cursor.position();
            cursor.setPosition(cursor.anchor() + 1, QTextCursor::MoveAnchor);
            cursor.setPosition(cursorPos, QTextCursor::KeepAnchor);
            word = cursor.selectedText();
        }

        if (word == t_old_word)
        {
            cursor.insertText(t_new_word);
            QCoreApplication::processEvents();
        }

        cursor.movePosition(QTextCursor::NextWord, QTextCursor::MoveAnchor, 1);
    }
}

void SpellCheckDialog::on_pushButtonChangeAll_clicked()
{
    if (m_unknown_word == ui->lineEditChange->text() ) // if a new value wasn't typed in used the one selcted
    {
        if ( ui->listWidgetSuggestions->selectedItems().count() > 0 )
            ui->lineEditChange->setText( ui->listWidgetSuggestions->currentItem()->text() );
    }

    m_return_code = ChangeAll;
    accept();
}


void SpellCheckDialog::on_pushButtonCancel_clicked()
{
    reject();
}


void SpellCheckDialog::on_listWidgetSuggestions_itemClicked(QListWidgetItem *item)
{
    if ( ui->listWidgetSuggestions->selectedItems().count() > 0 )
        ui->lineEditChange->setText(item->text());
}


void SpellCheckDialog::on_listWidgetSuggestions_itemDoubleClicked(QListWidgetItem *item)
{
    ui->lineEditChange->setText(item->text());

    if (ui->listWidgetSuggestions->selectedItems().count() > 0)
        on_pushButtonChange_clicked();
}

