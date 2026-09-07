#ifndef TST_CAINCLUDE_H
#define TST_CAINCLUDE_H

#include <QByteArray>
#include <QObject>
#include <QString>
#include <QTemporaryDir>
#include <QVariant>

/**
 * caInclude resolving the file names of its includes while running inside the designer
 * (the run time path is in CaQtDM_Lib and follows the same rules).
 */
class TestCaInclude : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();

    void uiFileName_data();
    void uiFileName();
    void relativeToFormFile();
    void nestedRelativeToIncludingFile();
    void includingFileWinsOverForm();
    void nameAsWrittenWins();
    void missingFileLoadsNothing();

private:
    void writeUi(const QString &path, const QString &labelName);

    QTemporaryDir m_dir;
    QString m_root;
    QVariant m_savedAppSource;
    bool m_hadDisplayPath = false;
    QByteArray m_savedDisplayPath;
};

#endif // TST_CAINCLUDE_H
