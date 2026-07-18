#include <QObject>
#include <QProcess>
#include <QDateTime>

class TimeControl : public QObject
{
    Q_OBJECT
public:
    explicit TimeControl(QObject *parent = nullptr) : QObject(parent) {}

    Q_INVOKABLE QString getCurrentTime() {
        QProcess p;
        p.start("/bin/date", QStringList() << "+%Y-%m-%d %H:%M:%S");
        p.waitForFinished();
        return QString(p.readAll().simplified());
    }

    Q_INVOKABLE QString setSystemTime(int year, int month, int day, int hour, int minute, int second) {
        QString cmd = QString::asprintf("%02d%02d%02d%02d%04d.%02d",
            month, day, hour, minute, year, second);
        system(("/bin/date " + cmd).toUtf8().constData());
        return getCurrentTime();
    }
};
