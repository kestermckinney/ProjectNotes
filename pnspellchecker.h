#ifndef PNSPELLCHECKER_H
#define PNSPELLCHECKER_H

#include "hunspell/hunspell.hxx"

#include <QStringList>

class PNSpellChecker
{
    int m_DefaultDictionary = -1;

    QVector<QString> m_DicFiles;
    QVector<QString> m_AffFiles;

    Hunspell* m_hunspell = nullptr;

    QVector<QString> m_DictionaryNames;

public:
    PNSpellChecker();

    const QStringList dictionaryNames();
    int defaultDictionaryIndex() { return m_DefaultDictionary; }
    QString defaultDictionaryName();
    void LoadPersonalWordList();
    void AddToPersonalWordList(QString& t_word);
    QStringList suggest(const QString &t_word);
    bool setDefaultDictionary(int t_index);
    bool setDefaultDictionary(const QString& t_name);
    bool isGoodWord(const QString& t_word);
    void ignoreWord(const QString& t_word);
    bool hasDictionary() { return m_hunspell != nullptr; }
};

#endif // PNSPELLCHECKER_H
