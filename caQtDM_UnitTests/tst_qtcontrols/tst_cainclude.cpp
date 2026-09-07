#include "tst_cainclude.h"
#include "fakeformwindow.h"

#include "cainclude.h"
#include "searchfile.h"

#include <QApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QLabel>
#include <QTest>

// FakeFormWindow reports no file; the include needs the form's location
class FormWindowWithFile : public FakeFormWindow
{
public:
    QString file;
    QString fileName() const override { return file; }
};

void TestCaInclude::writeUi(const QString &path, const QString &labelName)
{
    QDir().mkpath(QFileInfo(path).path());
    QFile f(path);
    QVERIFY2(f.open(QIODevice::WriteOnly | QIODevice::Truncate), qPrintable(path));
    f.write(QString("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
                    "<ui version=\"4.0\"><class>Form</class>\n"
                    " <widget class=\"QWidget\" name=\"Form\">\n"
                    "  <property name=\"geometry\"><rect><x>0</x><y>0</y><width>100</width><height>30</height></rect></property>\n"
                    "  <widget class=\"QLabel\" name=\"%1\">\n"
                    "   <property name=\"geometry\"><rect><x>0</x><y>0</y><width>100</width><height>30</height></rect></property>\n"
                    "   <property name=\"text\"><string>%1</string></property>\n"
                    "  </widget>\n"
                    " </widget>\n"
                    "</ui>\n").arg(labelName).toUtf8());
}

void TestCaInclude::initTestCase()
{
    QVERIFY(m_dir.isValid());
    m_root = m_dir.path();
    // form/  : the directory of the form being edited
    // form/sub: a nested include lives here
    // dp/    : a directory put on CAQTDM_DISPLAY_PATH, with a different child.ui
    writeUi(m_root + "/form/child.ui", "childLabel");
    writeUi(m_root + "/form/sub/leaf.ui", "leafLabel");
    writeUi(m_root + "/dp/child.ui", "displayPathLabel");

    // caInclude only loads its include itself when created inside the designer
    m_savedAppSource = qApp->property("APP_SOURCE");
    qApp->setProperty("APP_SOURCE", QString("DESIGNER"));

    m_hadDisplayPath = qEnvironmentVariableIsSet("CAQTDM_DISPLAY_PATH");
    m_savedDisplayPath = qgetenv("CAQTDM_DISPLAY_PATH");
    qunsetenv("CAQTDM_DISPLAY_PATH");
}

void TestCaInclude::cleanupTestCase()
{
    qApp->setProperty("APP_SOURCE", m_savedAppSource);
    if (m_hadDisplayPath) qputenv("CAQTDM_DISPLAY_PATH", m_savedDisplayPath);
    else qunsetenv("CAQTDM_DISPLAY_PATH");
}

void TestCaInclude::uiFileName_data()
{
    QTest::addColumn<QString>("in");
    QTest::addColumn<QString>("out");
    QTest::newRow("ui kept")            << "x.ui"            << "x.ui";
    QTest::newRow("prc kept")           << "x.prc"           << "x.prc";
    QTest::newRow("adl replaced")       << "x.adl"           << "x.ui";
    QTest::newRow("edl replaced")       << "x.edl"           << "x.ui";
    QTest::newRow("no suffix")          << "x"               << "x.ui";
    QTest::newRow("dots in parent dir") << "../dir/leaf"     << "../dir/leaf.ui";
    QTest::newRow("dot in dir name")    << "dir.d/name"      << "dir.d/name.ui";
    QTest::newRow("dot in dir, adl")    << "dir.d/name.adl"  << "dir.d/name.ui";
    QTest::newRow("upper case kept")    << "X.UI"            << "X.UI";
}

void TestCaInclude::uiFileName()
{
    QFETCH(QString, in);
    QFETCH(QString, out);
    QCOMPARE(searchFile::uiFileName(in), out);
}

void TestCaInclude::relativeToFormFile()
{
    FormWindowWithFile form;
    form.file = m_root + "/form/parent.ui";
    caInclude *include = new caInclude(&form);

    include->setFileName("child.ui");

    QVERIFY(include->findChild<QLabel *>("childLabel") != nullptr);
    // the loaded file is remembered for includes nested in it
    QCOMPARE(QFileInfo(include->property("includeFile").toString()).absoluteFilePath(),
             QFileInfo(m_root + "/form/child.ui").absoluteFilePath());
}

void TestCaInclude::nestedRelativeToIncludingFile()
{
    // stands in for the frame of an enclosing caInclude that has loaded form/mid.ui
    QWidget host;
    host.setProperty("includeFile", m_root + "/form/mid.ui");
    caInclude *include = new caInclude(&host);

    include->setFileName("sub/leaf.ui");

    QVERIFY(include->findChild<QLabel *>("leafLabel") != nullptr);
}

void TestCaInclude::includingFileWinsOverForm()
{
    // child.ui exists next to the form (dp/) and next to the enclosing loaded file (form/);
    // the nearest enclosing file has to win, as it does at run time
    FormWindowWithFile form;
    form.file = m_root + "/dp/parent.ui";
    QWidget *host = new QWidget(&form);
    host->setProperty("includeFile", m_root + "/form/mid.ui");
    caInclude *include = new caInclude(host);

    include->setFileName("child.ui");

    QVERIFY(include->findChild<QLabel *>("childLabel") != nullptr);
    QVERIFY(include->findChild<QLabel *>("displayPathLabel") == nullptr);
}

void TestCaInclude::nameAsWrittenWins()
{
    // a file found through CAQTDM_DISPLAY_PATH is taken before one relative to the form
    qputenv("CAQTDM_DISPLAY_PATH", (m_root + "/dp").toUtf8());
    FormWindowWithFile form;
    form.file = m_root + "/form/parent.ui";
    caInclude *include = new caInclude(&form);

    include->setFileName("child.ui");
    qunsetenv("CAQTDM_DISPLAY_PATH");

    QVERIFY(include->findChild<QLabel *>("displayPathLabel") != nullptr);
    QVERIFY(include->findChild<QLabel *>("childLabel") == nullptr);
}

void TestCaInclude::missingFileLoadsNothing()
{
    FormWindowWithFile form;
    form.file = m_root + "/form/parent.ui";
    caInclude *include = new caInclude(&form);

    include->setFileName("nothere.ui");

    QVERIFY(include->findChildren<QLabel *>().isEmpty());
    QVERIFY(include->property("includeFile").toString().isEmpty());
}
