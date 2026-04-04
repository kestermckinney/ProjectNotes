// Copyright (C) 2026 Paul McKinney
#ifndef SPELLCHECKER_H
#define SPELLCHECKER_H

#include "hunspell/hunspell.hxx"

#include <QStringList>

class SpellChecker
{
    int m_defaultDictionary = -1;

    QVector<QString> m_dicFiles;
    QVector<QString> m_affFiles;

    Hunspell* m_hunspell = nullptr;

    QVector<QString> m_dictionaryNames;

public:
    SpellChecker();
    ~SpellChecker();

    const QStringList dictionaryNames();
    int defaultDictionaryIndex() { return m_defaultDictionary; }
    QString defaultDictionaryName();
    void LoadPersonalWordList();
    void AddToPersonalWordList(QString& word);
    QStringList suggest(const QString &word);
    bool setDefaultDictionary(int index);
    bool setDefaultDictionary(const QString& name);
    bool isGoodWord(const QString& word);
    void ignoreWord(const QString& word);
    bool hasDictionary() { return m_hunspell != nullptr; }
};

#endif // SPELLCHECKER_H
