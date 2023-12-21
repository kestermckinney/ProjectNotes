#include "pnspellchecker.h"
#include "pnsettings.h"

#include <QCoreApplication>
#include <QVariant>
#include <QString>
#include <QFile>
#include <QMessageBox>
#include <QObject>

PNSpellChecker::PNSpellChecker()
{
    QSettings spell_settings(QCoreApplication::applicationDirPath() + "/dictionary/index.ini", QSettings::IniFormat);

    QStringList dictionaries = spell_settings.childGroups();

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
   }

    m_DefaultDictionary = global_Settings.getDefaultDictionary().toInt();

    if (m_DicFiles.count() > 0 && m_AffFiles.count() > 0)
    {
        QString dict = QString(QCoreApplication::applicationDirPath() + "/dictionary/" + m_DicFiles[m_DefaultDictionary]);
        QString aff = QString(QCoreApplication::applicationDirPath() + "/dictionary/" + m_AffFiles[m_DefaultDictionary]);

        QByteArray dictFilePathBA = dict.toLocal8Bit();
        QByteArray affixFilePathBA = aff.toLocal8Bit();

        if (!QFile::exists(dict) || !QFile::exists(aff))
        {
            QMessageBox::critical(nullptr, QObject::tr("Dictionary Files Not Found"),
                                  QString(QObject::tr("No dictionary files were specified.  You may need to re-install Project Notes.")), QMessageBox::Close);
        }

        if (m_hunspell)
        {
            delete m_hunspell;
            m_hunspell = nullptr;
        }

        m_hunspell = new Hunspell(affixFilePathBA.constData(), dictFilePathBA.constData());
    }
    else
    {
        QMessageBox::critical(nullptr, QObject::tr("Dictionary Files Not Found"),
                              QString(QObject::tr("No dictionary files were found.  You may need to re-install Project Notes.")), QMessageBox::Close);
    }

    LoadPersonalWordList();
}

PNSpellChecker::~PNSpellChecker()
{
    if (m_hunspell)
    {
        delete m_hunspell;
        m_hunspell = nullptr;
    }
}

const QStringList PNSpellChecker::dictionaryNames()
{
    QStringList names;

    foreach (auto n, m_DictionaryNames)
        names.append(n);

    return names;
}


QString PNSpellChecker::defaultDictionaryName()
{
    QString name;

    if (m_DefaultDictionary != -1)
        name = m_DictionaryNames[m_DefaultDictionary];

    return name;
}

bool PNSpellChecker::setDefaultDictionary(const QString& t_name)
{
    int i = m_DictionaryNames.indexOf(t_name);

    if (i == -1)
        return false;

    return setDefaultDictionary(i);
}

bool PNSpellChecker::setDefaultDictionary(int t_index)
{

    if (m_hunspell)
    {
        delete m_hunspell;
        m_hunspell = nullptr;
    }

    m_DefaultDictionary = t_index;

    QByteArray dictFilePathBA = QString(QCoreApplication::applicationDirPath() + "/dictionary/" + m_DicFiles[m_DefaultDictionary]).toLocal8Bit();
    QByteArray affixFilePathBA = QString(QCoreApplication::applicationDirPath() + "/dictionary/" + m_AffFiles[m_DefaultDictionary]).toLocal8Bit();

    m_hunspell = new Hunspell(affixFilePathBA.constData(), dictFilePathBA.constData());

    LoadPersonalWordList();


    global_Settings.setDefaultDictionary(QString("%1").arg(m_DefaultDictionary));

    return true;
}

void PNSpellChecker::LoadPersonalWordList()
{
    QString personalwords = global_Settings.getPersonalDictionary();

    QStringList wordlist = personalwords.split(":");

    if (m_hunspell)
        for ( const QString& word : wordlist )
            m_hunspell->add(word.toStdString());
}

void PNSpellChecker::AddToPersonalWordList(QString& t_word)
{
    QString personalwords = global_Settings.getPersonalDictionary();

    if (!personalwords.isEmpty())
        personalwords += ":";

    personalwords += t_word;

    global_Settings.setPersonalDictionary(personalwords);
}

QStringList PNSpellChecker::suggest(const QString &t_word)
{
    QStringList suggestions;
    std::vector<std::string> wordlist = m_hunspell->suggest(t_word.toStdString());

    for (std::vector<std::string>::const_iterator i = wordlist.begin(); i != wordlist.end(); ++i)
        suggestions.append(QString::fromStdString(*i));

    return suggestions;
}

bool PNSpellChecker::isGoodWord(const QString& t_word)
{
    return m_hunspell->spell(t_word.toStdString());
}

void PNSpellChecker::ignoreWord(const QString &t_word)
{
    m_hunspell->add(t_word.toStdString());
}
