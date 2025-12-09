#include <QtTest/QtTest>
#include <QVector>
#include <QList>
#include <sstream>

#include "mnode.h"
#include "exportfuncs.h"

class CoreLogicTests : public QObject
{
    Q_OBJECT

private slots:
    void curveExportProducesExpectedControlPoints();
    void smoothForceCalculationMatchesFixture();
    void exporterSerializesBezierList();
};

void CoreLogicTests::curveExportProducesExpectedControlPoints()
{
    mnode anchor({0.f, 0.f, 0.f}, {0.f, 0.f, 1.f}, 0.f, 10.f, 0.f, 0.f);
    mnode last({0.f, 0.f, 0.f}, {0.f, 0.f, 1.f}, 0.f, 10.f, 0.f, 0.f);
    mnode current({0.f, 0.f, 1.f}, {0.f, 0.f, 1.f}, 0.f, 10.f, 0.f, 0.f);

    anchor.updateNorm();
    last.updateNorm();
    current.updateNorm();

    last.fTotalLength = 0.f;
    current.fTotalLength = 1.f;
    current.fTrackAngleFromLast = 0.1f;
    current.fAngleFromLast = 0.1f;
    current.fHeartDistFromLast = 1.f;

    QList<bezier_t*> bezierList;
    current.exportNode(bezierList, &last, nullptr, &anchor, 0.f, 0.1f);

    QCOMPARE(bezierList.size(), 1);

    const bezier_t* segment = bezierList.first();
    QVERIFY(!segment->relRoll);

    const float expectedThreshold = 0.3335137f;
    QCOMPARE(segment->P1.x, 0.f);
    QCOMPARE(segment->P1.y, 0.f);
    QCOMPARE(segment->P1.z, -1.f);

    QCOMPARE(segment->Kp1.x, 0.f);
    QCOMPARE(segment->Kp1.y, 0.f);
    QVERIFY(qAbs(segment->Kp1.z + expectedThreshold) < 1e-5f);

    QCOMPARE(segment->Kp2.x, 0.f);
    QCOMPARE(segment->Kp2.y, 0.f);
    QVERIFY(qFuzzyCompare(segment->Kp2.z, -1.f + expectedThreshold));

    QCOMPARE(segment->roll, 0.f);

    qDeleteAll(bezierList);
}

void CoreLogicTests::smoothForceCalculationMatchesFixture()
{
    mnode node({0.f, 0.f, 0.f}, {0.f, 0.f, 1.f}, 0.f, 20.f, 1.f, 0.5f);
    node.updateNorm();

    node.fAngleFromLast = 0.05f;
    node.fPitchFromLast = 0.02f;
    node.fYawFromLast = 0.03f;
    node.fHeartDistFromLast = 1.f;
    node.fRollSpeed = 0.f;
    node.fSmoothSpeed = 0.f;

    node.calcSmoothForces();

    QCOMPARE_WITH_SIGNEDNESS(qRound64(node.smoothNormal * 1000), 35595ll);
    QCOMPARE_WITH_SIGNEDNESS(qRound64(node.smoothLateral * 1000), 568ll);
}

static QByteArray toBigEndian(float value)
{
    QByteArray bytes;
    const auto raw = reinterpret_cast<const char*>(&value);
    for (int i = 3; i >= 0; --i) {
        bytes.append(raw[i]);
    }
    return bytes;
}

void CoreLogicTests::exporterSerializesBezierList()
{
    QList<bezier_t*> bezierList;
    auto* bezier = new bezier_t;
    bezier->Kp1 = {1.f, 2.f, 3.f};
    bezier->Kp2 = {4.f, 5.f, 6.f};
    bezier->P1 = {7.f, 8.f, 9.f};
    bezier->roll = 10.f;
    bezier->relRoll = true;
    bezierList.append(bezier);

    std::stringstream buffer;
    writeToExportFile(&buffer, bezierList);

    const QByteArray serialized = QByteArray::fromStdString(buffer.str());
    QCOMPARE(serialized.size(), 50);

    QByteArray expected;
    const float values[] = {1.f, 2.f, 3.f, 4.f, 5.f, 6.f, 7.f, 8.f, 9.f, 10.f};
    for (float value : values) {
        expected.append(toBigEndian(value));
    }

    expected.append(char(0xFF)); // cont roll
    expected.append(char(0xFF)); // rel roll
    expected.append(char(0x00)); // equal dist CP
    expected.append(QByteArray(7, char(0x00)));

    QCOMPARE(serialized, expected);

    qDeleteAll(bezierList);
}

QTEST_MAIN(CoreLogicTests)
#include "corelogic_tests.moc"
