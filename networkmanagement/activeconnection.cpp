/*
 * Copyright (C) 2021 LingmoOS Team.
 *
 * Author:     lingmoos <lingmo@lingmo.org>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "activeconnection.h"

ActiveConnection::ActiveConnection(QObject *parent)
    : QObject(parent)
    , m_wirelessNetwork(nullptr)
{
    statusChanged(NetworkManager::status());
    connect(NetworkManager::notifier(), &NetworkManager::Notifier::statusChanged, this, &ActiveConnection::statusChanged);

    updatePrimaryConnection();
    connect(NetworkManager::notifier(), &NetworkManager::Notifier::primaryConnectionChanged,
            this, &ActiveConnection::updatePrimaryConnection);
    connect(NetworkManager::notifier(), &NetworkManager::Notifier::activeConnectionsChanged,
            this, &ActiveConnection::updatePrimaryConnection);
    connect(NetworkManager::notifier(), &NetworkManager::Notifier::statusChanged,
            this, &ActiveConnection::updatePrimaryConnection);
}

void ActiveConnection::updatePrimaryConnection()
{
    QString type = QStringLiteral("none");
    QString name;
    const NetworkManager::ActiveConnection::Ptr primary = NetworkManager::primaryConnection();
    if (primary && primary->isValid() && NetworkManager::status() == NetworkManager::Connected) {
        name = primary->id();
        switch (primary->type()) {
        case NetworkManager::ConnectionSettings::Wireless:
            type = QStringLiteral("wireless");
            break;
        case NetworkManager::ConnectionSettings::Wired:
        case NetworkManager::ConnectionSettings::Bond:
        case NetworkManager::ConnectionSettings::Bridge:
        case NetworkManager::ConnectionSettings::Vlan:
            type = QStringLiteral("wired");
            break;
        default:
            type = QStringLiteral("wired");   // other links (e.g. tethering) show as wired
            break;
        }
    }

    bool vpn = false;
    for (const NetworkManager::ActiveConnection::Ptr &ac : NetworkManager::activeConnections()) {
        if (ac->vpn() || ac->type() == NetworkManager::ConnectionSettings::WireGuard) {
            vpn = true;
            break;
        }
    }

    if (type != m_connectionType || name != m_connectionName || vpn != m_vpnActive) {
        m_connectionType = type;
        m_connectionName = name;
        m_vpnActive = vpn;
        emit connectionChanged();
    }
}

QString ActiveConnection::networkIcon() const
{
    if (m_connectionType == QLatin1String("wireless"))
        return m_wirelessIcon.isEmpty() ? QStringLiteral("network-wireless-connected-100") : m_wirelessIcon;
    if (m_connectionType == QLatin1String("wired"))
        return QStringLiteral("network-wired-activated");
    return QStringLiteral("network-wired");   // disconnected
}

void ActiveConnection::statusChanged(NetworkManager::Status status)
{
    if (status == NetworkManager::Connected) {
        NetworkManager::ActiveConnection::Ptr activeConnection = NetworkManager::primaryConnection();

        if (activeConnection) {
            NetworkManager::ConnectionSettings::ConnectionType type = activeConnection->type();
            if ((type == NetworkManager::ConnectionSettings::Wireless) && activeConnection->isValid()) {
                NetworkManager::Connection::Ptr selectedConnection = activeConnection->connection();
                m_wirelessName = selectedConnection->name();
                emit wirelessNameChanged();

                updateWirelessIcon(NetworkManager::findNetworkInterface(activeConnection->devices().first()));
            }
        }

    } else {
        m_wirelessName.clear();
        emit wirelessNameChanged();
    }
}

void ActiveConnection::updateWirelessIcon(NetworkManager::Device::Ptr device)
{
    if (!device)
        return;

    // clear
    if (m_wirelessNetwork) {
        disconnect(m_wirelessNetwork.data());
    }

    NetworkManager::WirelessDevice::Ptr wifiDevice = device.objectCast<NetworkManager::WirelessDevice>();
    NetworkManager::AccessPoint::Ptr ap = wifiDevice->activeAccessPoint();

    m_wirelessNetwork = wifiDevice->findNetwork(m_wirelessName);

    if (m_wirelessNetwork) {
        updateWirelessIconForSignalStrength(m_wirelessNetwork->signalStrength());
        connect(m_wirelessNetwork.data(), &NetworkManager::WirelessNetwork::signalStrengthChanged,
                this, &ActiveConnection::updateWirelessIconForSignalStrength, Qt::UniqueConnection);
    }
}

void ActiveConnection::updateWirelessIconForSignalStrength(int strength)
{
    int iconStrength = 0;

    if (strength == 0)
        iconStrength = 0;
    else if (strength <= 25)
        iconStrength = 25;
    else if (strength <= 50)
        iconStrength = 50;
    else if (strength <= 75)
        iconStrength = 75;
    else if (strength <= 100)
        iconStrength = 100;

    m_wirelessIcon = QString("network-wireless-connected-%1").arg(iconStrength);
    emit wirelessIconChanged();
    emit connectionChanged();
}
