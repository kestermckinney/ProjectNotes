// Copyright (C) 2026 Paul McKinney
#include "spellchecker.h"
#include "appsettings.h"

#include <QCoreApplication>
#include <QVariant>
#include <QString>
#include <QFile>
#include <QMessageBox>
#include <QObject>

SpellChecker::SpellChecker()
{
    QSettings spell_settings(appResourcesPath() + "/dictionary/index.ini", QSettings::IniFormat);

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

        m_dictionaryNames.append(dicname.toString());
        m_dicFiles.append(dicfile.toString());
        m_affFiles.append(afffile.toString());
   }

    m_defaultDictionary = global_Settings.getDefaultDictionary().toInt();

    if (m_dicFiles.count() > 0 && m_affFiles.count() > 0)
    {
        QString dict = QString(appResourcesPath() + "/dictionary/" + m_dicFiles[m_defaultDictionary]);
        QString aff = QString(appResourcesPath() + "/dictionary/" + m_affFiles[m_defaultDictionary]);

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

SpellChecker::~SpellChecker()
{
    if (m_hunspell)
    {
        delete m_hunspell;
        m_hunspell = nullptr;
    }
}

const QStringList SpellChecker::dictionaryNames()
{
    QStringList names;

    foreach (auto n, m_dictionaryNames)
        names.append(n);

    return names;
}


QString SpellChecker::defaultDictionaryName()
{
    QString name;

    if (m_defaultDictionary != -1)
        name = m_dictionaryNames[m_defaultDictionary];

    return name;
}

bool SpellChecker::setDefaultDictionary(const QString& name)
{
    int i = m_dictionaryNames.indexOf(name);

    if (i == -1)
        return false;

    return setDefaultDictionary(i);
}

bool SpellChecker::setDefaultDictionary(int index)
{

    if (m_hunspell)
    {
        delete m_hunspell;
        m_hunspell = nullptr;
    }

    m_defaultDictionary = index;

    QByteArray dictFilePathBA = QString(appResourcesPath() + "/dictionary/" + m_dicFiles[m_defaultDictionary]).toLocal8Bit();
    QByteArray affixFilePathBA = QString(appResourcesPath() + "/dictionary/" + m_affFiles[m_defaultDictionary]).toLocal8Bit();

    m_hunspell = new Hunspell(affixFilePathBA.constData(), dictFilePathBA.constData());

    LoadPersonalWordList();


    global_Settings.setDefaultDictionary(QString("%1").arg(m_defaultDictionary));

    return true;
}

void SpellChecker::LoadPersonalWordList()
{
    QString personalwords = global_Settings.getPersonalDictionary();

    QStringList wordlist = personalwords.split(":");

    if (m_hunspell)
        for ( const QString& word : wordlist )
            m_hunspell->add(word.toStdString());
}

void SpellChecker::AddToPersonalWordList(QString& word)
{
    QString personalwords = global_Settings.getPersonalDictionary();

    if (!personalwords.isEmpty())
        personalwords += ":";

    personalwords += word;

    global_Settings.setPersonalDictionary(personalwords);
}

QStringList SpellChecker::suggest(const QString &word)
{
    QStringList suggestions;
    std::vector<std::string> wordlist = m_hunspell->suggest(word.toStdString());

    for (std::vector<std::string>::const_iterator i = wordlist.begin(); i != wordlist.end(); ++i)
        suggestions.append(QString::fromStdString(*i));

    return suggestions;
}

bool SpellChecker::isGoodWord(const QString& word)
{
    return m_hunspell->spell(word.toStdString());
}

void SpellChecker::ignoreWord(const QString &word)
{
    m_hunspell->add(word.toStdString());
}
