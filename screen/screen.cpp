#include "screen.h"

#include <QDir>
#include <QFile>
#include <QStandardPaths>
#include "outputmodel.h"

#include <kscreen/setconfigoperation.h>

#include <QQmlExtensionPlugin>
#include <QQmlEngine>

Screen::Screen(QObject *parent)
    : QObject(parent)
{
    qmlRegisterType<OutputModel>("Lingmo.Screen", 1, 0, "Screen");
    load();
}

void Screen::load()
{
    // Don't pull away the outputModel under QML's feet
    // signal its disappearance first before deleting and replacing it.
    // We take the m_config pointer so outputModel() will return null,
    // gracefully cleaning up the QML side and only then we will delete it.
    auto *oldConfig = m_config.release();
    if (oldConfig) {
        emit outputModelChanged();
        delete oldConfig;
    }

    m_config.reset(new ConfigHandler(this));
    connect(m_config.get(), &ConfigHandler::outputModelChanged, this, &Screen::outputModelChanged);

    connect(new KScreen::GetConfigOperation(), &KScreen::GetConfigOperation::finished, this, &Screen::configReady);
}

void Screen::save()
{
    if (!m_config)
        return;

    auto config = m_config->config();
    bool atLeastOneEnabledOutput = false;

    for (const KScreen::OutputPtr &output : config->outputs()) {
        KScreen::ModePtr mode = output->currentMode();
        atLeastOneEnabledOutput |= output->isEnabled();
    }

    m_config->writeControl();

    auto *op = new KScreen::SetConfigOperation(config);
    op->exec();

    writeLayoutScript(config);
}

// The same xrandr line arandr writes: lingmo-session runs it at login, before the
// window manager and the panels start
void Screen::writeLayoutScript(const KScreen::ConfigPtr &config)
{
    QStringList args;
    for (const KScreen::OutputPtr &output : config->outputs()) {
        if (!output->isConnected())
            continue;
        args << "--output" << output->name();
        const KScreen::ModePtr mode = output->currentMode();
        if (!output->isEnabled() || !mode) {
            args << "--off";
            continue;
        }
        const char *rotation = "normal";
        switch (output->rotation()) {
        case KScreen::Output::Left: rotation = "left"; break;
        case KScreen::Output::Inverted: rotation = "inverted"; break;
        case KScreen::Output::Right: rotation = "right"; break;
        default: break;
        }
        args << "--mode" << QStringLiteral("%1x%2").arg(mode->size().width()).arg(mode->size().height())
             << "--rate" << QString::number(mode->refreshRate(), 'f', 2)
             << "--pos" << QStringLiteral("%1x%2").arg(output->pos().x()).arg(output->pos().y())
             << "--rotate" << rotation;
        if (output->priority() == 1)
            args << "--primary";
    }

    const QString dir = QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation) + "/lingmoos";
    QDir().mkpath(dir);
    QFile file(dir + "/screenlayout.sh");
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
        return;
    file.write("#!/bin/sh\n# Written by Settings > Display; run by lingmo-session at login\n");
    file.write("xrandr " + args.join(' ').toUtf8() + "\n");
    file.close();
    file.setPermissions(file.permissions() | QFileDevice::ExeOwner);
}

OutputModel *Screen::outputModel() const
{
    if (!m_config) {
        return nullptr;
    }

    return m_config->outputModel();
}

void Screen::configReady(KScreen::ConfigOperation *op)
{
    if (op->hasError()) {
        m_config.reset();
        return;
    }

    KScreen::ConfigPtr config = qobject_cast<KScreen::GetConfigOperation *>(op)->config();
    // const bool autoRotationSupported = config->supportedFeatures() & (KScreen::Config::Feature::AutoRotation | KScreen::Config::Feature::TabletMode);

    m_config->setConfig(config);
}
