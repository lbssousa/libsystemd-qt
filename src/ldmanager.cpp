/*
 * Copyright (C) 2013  Andrea Scarpino <me@andreascarpino.it>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 */

#include "ldmanager.h"
#include "ldmanager_p.h"

const QString Systemd::Logind::LogindPrivate::LD_DBUS_SERVICE(QLatin1Literal("org.freedesktop.login1"));
const QString Systemd::Logind::LogindPrivate::LD_DBUS_DAEMON_PATH(QLatin1Literal("/org/freedesktop/login1"));

Q_GLOBAL_STATIC(Systemd::Logind::LogindPrivate, globalLogind)

Systemd::Logind::LogindPrivate::LogindPrivate() :
    ildface(Systemd::Logind::LogindPrivate::LD_DBUS_SERVICE,
            Systemd::Logind::LogindPrivate::LD_DBUS_DAEMON_PATH, QDBusConnection::systemBus())
{
    connect(&ildface, SIGNAL(PrepareForShutdown(bool)),
            this, SLOT(onPrepareForShutdown(bool)));
    connect(&ildface, SIGNAL(PrepareForSleep(bool)),
            this, SLOT(onPrepareForSleep(bool)));

    connect(&ildface, SIGNAL(SeatNew(QString, QDBusObjectPath)), this,
            SLOT(onSeatNew(QString, QDBusObjectPath)));
    connect(&ildface, SIGNAL(SeatRemoved(QString, QDBusObjectPath)), this,
            SLOT(onSeatRemoved(QString, QDBusObjectPath)));

    connect(&ildface, SIGNAL(SessionNew(QString, QDBusObjectPath)), this,
            SLOT(onSessionNew(QString, QDBusObjectPath)));
    connect(&ildface, SIGNAL(SessionRemoved(QString, QDBusObjectPath)), this,
            SLOT(onSessionRemoved(QString, QDBusObjectPath)));

    connect(&ildface, SIGNAL(UserNew(uint, QDBusObjectPath)), this,
            SLOT(onUserNew(uint, QDBusObjectPath)));
    connect(&ildface, SIGNAL(UserRemoved(uint, QDBusObjectPath)), this,
            SLOT(onUserRemoved(uint, QDBusObjectPath)));

    init();
}

Systemd::Logind::LogindPrivate::~LogindPrivate()
{
}

void Systemd::Logind::LogindPrivate::init()
{
    qDBusRegisterMetaType<DBusSeat>();
}

QList<Systemd::Logind::Seat::Ptr> Systemd::Logind::LogindPrivate::listSeats()
{
    qDBusRegisterMetaType<DBusSeatList>();
    QDBusPendingReply<DBusSeatList> reply = ildface.ListSeats();
    reply.waitForFinished();

    if (reply.isError()) {
        qDebug() << reply.error().message();
        return QList<Seat::Ptr>();
    }

    QList<Seat::Ptr> seatLists;
    const QDBusMessage message = reply.reply();
    if (message.type() == QDBusMessage::ReplyMessage) {
        const DBusSeatList seats = qdbus_cast<DBusSeatList>(message.arguments().first());
        Q_FOREACH (const DBusSeat seat, seats) {
            Seat::Ptr s = Seat::Ptr(new Seat(seat.path.path(), ildface.connection()));
            seatLists.append(s);
        }
    }

    return seatLists;
}

Systemd::Logind::Permission Systemd::Logind::LogindPrivate::canHibernate()
{
    QDBusPendingReply<QString> reply = ildface.CanHibernate();
    reply.waitForFinished();

    if (reply.isError()) {
        qDebug() << reply.error().message();
        return Systemd::Logind::No;
    }

    const QString permission = qdbus_cast<QString>(reply.reply().arguments().first());

    return stringToPermission(permission);
}

Systemd::Logind::Permission Systemd::Logind::LogindPrivate::canHybridSleep()
{
    QDBusPendingReply<QString> reply = ildface.CanHybridSleep();
    reply.waitForFinished();

    if (reply.isError()) {
        qDebug() << reply.error().message();
        return Systemd::Logind::No;
    }

    const QString permission = qdbus_cast<QString>(reply.reply().arguments().first());

    return stringToPermission(permission);
}

Systemd::Logind::Permission Systemd::Logind::LogindPrivate::canPowerOff()
{
    QDBusPendingReply<QString> reply = ildface.CanPowerOff();
    reply.waitForFinished();

    if (reply.isError()) {
        qDebug() << reply.error().message();
        return Systemd::Logind::No;
    }

    const QString permission = qdbus_cast<QString>(reply.reply().arguments().first());

    return stringToPermission(permission);
}

Systemd::Logind::Permission Systemd::Logind::LogindPrivate::canReboot()
{
    QDBusPendingReply<QString> reply = ildface.CanReboot();
    reply.waitForFinished();

    if (reply.isError()) {
        qDebug() << reply.error().message();
        return Systemd::Logind::No;
    }

    const QString permission = qdbus_cast<QString>(reply.reply().arguments().first());

    return stringToPermission(permission);
}

Systemd::Logind::Permission Systemd::Logind::LogindPrivate::canSuspend()
{
    QDBusPendingReply<QString> reply = ildface.CanSuspend();
    reply.waitForFinished();

    if (reply.isError()) {
        qDebug() << reply.error().message();
        return Systemd::Logind::No;
    }

    const QString permission = qdbus_cast<QString>(reply.reply().arguments().first());

    return stringToPermission(permission);
}

void Systemd::Logind::LogindPrivate::hibernate(const bool interactive)
{
    QDBusPendingReply<void> reply = ildface.Hibernate(interactive);
    reply.waitForFinished();

    if (reply.isError()) {
        qDebug() << reply.error().message();
    }
}

void Systemd::Logind::LogindPrivate::hybridSleep(const bool interactive)
{
    QDBusPendingReply<void> reply = ildface.HybridSleep(interactive);
    reply.waitForFinished();

    if (reply.isError()) {
        qDebug() << reply.error().message();
    }
}

void Systemd::Logind::LogindPrivate::onSeatNew(const QString &id, const QDBusObjectPath &seat)
{
    Q_UNUSED(id)
    emit Logind::Notifier::seatNew(Seat::Ptr(new Seat(seat.path(), ildface.connection())));
}

void Systemd::Logind::LogindPrivate::onSeatRemoved(const QString &id, const QDBusObjectPath &seat)
{
    Q_UNUSED(id)
    emit Logind::Notifier::seatRemoved(Seat::Ptr(new Seat(seat.path(), ildface.connection())));
}

void Systemd::Logind::LogindPrivate::onSessionNew(const QString &id, const QDBusObjectPath &session)
{
    Q_UNUSED(id)
    emit Logind::Notifier::sessionNew(Session::Ptr(new Session(session.path(), ildface.connection())));
}

void Systemd::Logind::LogindPrivate::onSessionRemoved(const QString &id, const QDBusObjectPath &session)
{
    Q_UNUSED(id)
    emit Logind::Notifier::sessionRemoved(Session::Ptr(new Session(session.path(), ildface.connection())));
}

void Systemd::Logind::LogindPrivate::onUserNew(const uint &id, const QDBusObjectPath &user)
{
    Q_UNUSED(id)
    emit Logind::Notifier::userNew(User::Ptr(new User(user.path(), ildface.connection())));
}

void Systemd::Logind::LogindPrivate::onUserRemoved(const uint &id, const QDBusObjectPath &user)
{
    Q_UNUSED(id)
    emit Logind::Notifier::userRemoved(User::Ptr(new User(user.path(), ildface.connection())));
}

void Systemd::Logind::LogindPrivate::powerOff(const bool interactive)
{
    QDBusPendingReply<void> reply = ildface.PowerOff(interactive);
    reply.waitForFinished();

    if (reply.isError()) {
        qDebug() << reply.error().message();
    }
}

void Systemd::Logind::LogindPrivate::reboot(const bool interactive)
{
    QDBusPendingReply<void> reply = ildface.Reboot(interactive);
    reply.waitForFinished();

    if (reply.isError()) {
        qDebug() << reply.error().message();
    }
}

void Systemd::Logind::LogindPrivate::suspend(const bool interactive)
{
    QDBusPendingReply<void> reply = ildface.Suspend(interactive);
    reply.waitForFinished();

    if (reply.isError()) {
        qDebug() << reply.error().message();
    }
}

void Systemd::Logind::LogindPrivate::onPrepareForShutdown(const bool active)
{
    emit Logind::Notifier::prepareForShutdown(active);
}

void Systemd::Logind::LogindPrivate::onPrepareForSleep(const bool active)
{
    emit Logind::Notifier::prepareForSleep(active);
}

Systemd::Logind::Permission Systemd::Logind::LogindPrivate::stringToPermission(const QString &permission) const
{
    if (QString::compare(permission, QLatin1String("na")) == 0) {
        return Systemd::Logind::Na;
    } else if (QString::compare(permission, QLatin1String("yes")) == 0) {
        return Systemd::Logind::Yes;
    } else if (QString::compare(permission, QLatin1String("challenge")) == 0) {
        return Systemd::Logind::Challenge;
    } else {
        return Systemd::Logind::No;
    }
}

Systemd::Logind::Permission Systemd::Logind::canHibernate()
{
    return globalLogind()->canHibernate();
}

Systemd::Logind::Permission Systemd::Logind::canHybridSleep()
{
    return globalLogind()->canHybridSleep();
}

Systemd::Logind::Permission Systemd::Logind::canPowerOff()
{
    return globalLogind()->canPowerOff();
}

Systemd::Logind::Permission Systemd::Logind::canReboot()
{
    return globalLogind()->canReboot();
}

Systemd::Logind::Permission Systemd::Logind::canSuspend()
{
    return globalLogind()->canSuspend();
}

void Systemd::Logind::hibernate(const bool interactive)
{
    globalLogind()->hibernate(interactive);
}

void Systemd::Logind::hybridSleep(const bool interactive)
{
    globalLogind()->hybridSleep(interactive);
}

QList<Systemd::Logind::Seat::Ptr> Systemd::Logind::listSeats()
{
    return globalLogind()->listSeats();
}

void Systemd::Logind::powerOff(const bool interactive)
{
    globalLogind()->powerOff(interactive);
}

void Systemd::Logind::reboot(const bool interactive)
{
    globalLogind()->reboot(interactive);
}

void Systemd::Logind::suspend(const bool interactive)
{
    globalLogind()->suspend(interactive);
}

Systemd::Logind::Notifier* Systemd::Logind::notifier()
{
    return globalLogind();
}
