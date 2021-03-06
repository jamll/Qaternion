/**************************************************************************
 *                                                                        *
 * Copyright (C) 2015 Felix Rohrbach <kde@fxrh.de>                        *
 *                                                                        *
 * This program is free software; you can redistribute it and/or          *
 * modify it under the terms of the GNU General Public License            *
 * as published by the Free Software Foundation; either version 3         *
 * of the License, or (at your option) any later version.                 *
 *                                                                        *
 * This program is distributed in the hope that it will be useful,        *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 * GNU General Public License for more details.                           *
 *                                                                        *
 * You should have received a copy of the GNU General Public License      *
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.  *
 *                                                                        *
 **************************************************************************/

#include "logindialog.h"

#include <QtWidgets/QLineEdit>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QLabel>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QFormLayout>

#include "settings.h"

LoginDialog::LoginDialog(QWidget* parent)
    : QDialog(parent)
    , serverEdit(new QLineEdit("https://matrix.org"))
    , userEdit(new QLineEdit())
    , passwordEdit(new QLineEdit())
    , initialDeviceName(new QLineEdit())
    , saveTokenCheck(new QCheckBox(tr("Stay logged in")))
    , statusLabel(new QLabel(tr("Welcome to Quaternion")))
    , m_connection(new Connection())
{
    passwordEdit->setEchoMode( QLineEdit::Password );

    auto buttons = new QDialogButtonBox(QDialogButtonBox::Ok|
                                        QDialogButtonBox::Cancel);
    buttons->button(QDialogButtonBox::Ok)->setText(tr("Login"));

    auto* formLayout = new QFormLayout();
    formLayout->addRow(tr("Matrix ID"), userEdit);
    formLayout->addRow(tr("Password"), passwordEdit);
    formLayout->addRow(tr("Device name"), initialDeviceName);
    formLayout->addRow(tr("Connect to server"), serverEdit);
    formLayout->addRow(saveTokenCheck);

    auto* mainLayout = new QVBoxLayout();
    mainLayout->addLayout(formLayout);
    mainLayout->addWidget(buttons);
    mainLayout->addWidget(statusLabel);
    
    setLayout(mainLayout);

    connect( userEdit, &QLineEdit::editingFinished, m_connection.data(),
             [=] {
                 auto userId = userEdit->text();
                 if (userId.startsWith('@') && userId.indexOf(':') != -1)
                     m_connection->resolveServer(userId);
             });
    connect( m_connection.data(), &Connection::homeserverChanged, serverEdit,
             [=] (QUrl newUrl)
             {
                 serverEdit->setText(newUrl.toString());
             });
    connect( buttons, &QDialogButtonBox::accepted, this, &LoginDialog::login );
    connect( buttons, &QDialogButtonBox::rejected, this, &LoginDialog::reject );

    {
        // Fill defaults
        using namespace QMatrixClient;
        QStringList accounts { SettingsGroup("Accounts").childGroups() };
        if ( !accounts.empty() )
        {
            AccountSettings account { accounts.front() };
            userEdit->setText(account.userId());

            auto homeserver = account.homeserver();
            if (!homeserver.isEmpty())
            {
                m_connection->setHomeserver(homeserver);
                serverEdit->setText(homeserver.toString());
            }
            initialDeviceName->setText(account.deviceName());
            saveTokenCheck->setChecked(account.keepLoggedIn());
            passwordEdit->setFocus();
        }
        else
        {
            saveTokenCheck->setChecked(false);
            userEdit->setFocus();
        }
    }
}

void LoginDialog::setStatusMessage(const QString& msg)
{
    statusLabel->setText(msg);
}

LoginDialog::Connection* LoginDialog::releaseConnection()
{
    return m_connection.take();
}

QString LoginDialog::deviceName() const
{
    return initialDeviceName->text();
}

bool LoginDialog::keepLoggedIn() const
{
    return saveTokenCheck->isChecked();
}

void LoginDialog::login()
{
    setStatusMessage(tr("Connecting and logging in, please wait"));
    setDisabled(true);

    auto url = QUrl::fromUserInput(serverEdit->text());
    if (!serverEdit->text().isEmpty() && !serverEdit->text().startsWith("http"))
        url.setScheme("https"); // Qt defaults to http (or even ftp for some)
    m_connection->setHomeserver(url);
    connect( m_connection.data(), &Connection::connected,
             this, &QDialog::accept );
    connect( m_connection.data(), &Connection::loginError,
             this, &LoginDialog::error );
    m_connection->connectToServer(userEdit->text(), passwordEdit->text(),
                                  initialDeviceName->text());
}

void LoginDialog::error(QString error)
{
    setStatusMessage(error);
    setDisabled(false);
}

