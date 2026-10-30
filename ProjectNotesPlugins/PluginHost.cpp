// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#include "PluginHost.h"
#include "PluginServices.h"

#include "QLogger.h"

#include <QCoreApplication>

using namespace QLogger;

namespace PN::Plugins {

PluginHost::PluginHost(PluginHostConfig config, PluginServices* services, QObject* parent)
    : QObject(parent), m_config(std::move(config)), m_services(services)
{
    m_thread.setObjectName(QStringLiteral("plugin:") + m_config.manifest.id);
}

PluginHost::~PluginHost()
{
    stop();
}

void PluginHost::start()
{
    if (m_runtime)
        return;
    m_runtime = new PluginRuntime(m_config, m_services);
    m_runtime->moveToThread(&m_thread);

    connect(m_runtime, &PluginRuntime::dynamicMenusChanged, this, [this](const QVariantList& menus) {
        m_dynamicMenus = menus;
        emit dynamicMenusChanged();
    });
    connect(m_runtime, &PluginRuntime::rowsChanged, this, &PluginHost::rowsChanged);
    connect(m_runtime, &PluginRuntime::foreignSettingsChanged, this, &PluginHost::foreignSettingsChanged);
    connect(m_runtime, &PluginRuntime::errorReported, this, &PluginHost::errorReported);
    connect(m_runtime, &PluginRuntime::logMessage, this, &PluginHost::logMessage);
    connect(m_runtime, &PluginRuntime::hung, this, &PluginHost::hung);

    m_thread.start();
    QPointer<PluginHost> self(this);
    PluginRuntime* runtime = m_runtime;
    QMetaObject::invokeMethod(runtime, [runtime, self]() {
        QString error;
        const bool ok = runtime->start(&error);
        QMetaObject::invokeMethod(qApp, [self, ok, error]() {
            if (!self)
                return;
            self->m_running = ok;
            if (!ok)
                QLog_Error(ERRORLOG, QStringLiteral("[%1] %2").arg(self->m_config.manifest.id, error));
            emit self->started(ok, error);
        }, Qt::QueuedConnection);
    }, Qt::QueuedConnection);
}

void PluginHost::stop()
{
    if (!m_runtime)
        return;
    PluginRuntime* runtime = m_runtime;
    m_runtime = nullptr;
    m_running = false;
    QThread* thread = &m_thread;
    QMetaObject::invokeMethod(runtime, [runtime, thread]() {
        runtime->stop();
        delete runtime;
        thread->quit();
    }, Qt::QueuedConnection);
    if (!m_thread.wait(8000)) {
        // A call into native code (a busy Office application, a blocked
        // dialog) never returned. Leave the thread rather than terminate it.
        QLog_Error(ERRORLOG, QStringLiteral("[%1] did not stop within 8 seconds.").arg(m_config.manifest.id));
    }
}

qint64 PluginHost::busySince() const
{
    return m_runtime ? m_runtime->busySince() : 0;
}

void PluginHost::interrupt()
{
    if (m_runtime)
        m_runtime->interrupt();
}

void PluginHost::dispatchMenu(const QString& menuId, const QString& entity, const QString& key,
                              const QString& parameter, const QStringList& include)
{
    if (!m_runtime)
        return;
    PluginRuntime* runtime = m_runtime;
    QMetaObject::invokeMethod(runtime, [=]() { runtime->dispatchMenu(menuId, entity, key, parameter, include); },
                              Qt::QueuedConnection);
}

void PluginHost::invoke(const QString& token, const QString& name, const QVariantList& args, Reply reply)
{
    if (!m_runtime) {
        reply({}, PluginError::make(QStringLiteral("unavailable"), QStringLiteral("The plugin is not running.")));
        return;
    }
    PluginRuntime* runtime = m_runtime;
    QPointer<PluginHost> self(this);
    QMetaObject::invokeMethod(runtime, [=]() {
        runtime->invoke(token, name, args, [self, reply](const QVariant& value, const PluginError& error) {
            QMetaObject::invokeMethod(qApp, [self, reply, value, error]() {
                if (self)
                    reply(value, error);
            }, Qt::QueuedConnection);
        });
    }, Qt::QueuedConnection);
}

void PluginHost::deliver(const QString& kind, const QVariant& payload)
{
    if (!m_runtime)
        return;
    PluginRuntime* runtime = m_runtime;
    QMetaObject::invokeMethod(runtime, [=]() { runtime->deliver(kind, payload); }, Qt::QueuedConnection);
}

void PluginHost::setProgressCancelled(const QString& token)
{
    if (!m_runtime)
        return;
    PluginRuntime* runtime = m_runtime;
    QMetaObject::invokeMethod(runtime, [=]() { runtime->setProgressCancelled(token); }, Qt::QueuedConnection);
}

} // namespace PN::Plugins
