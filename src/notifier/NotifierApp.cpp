/*
 *  Manjaro Settings Manager
 *  Ramon Buldó <ramon@manjaro.org>
 *
 *  Copyright (C) 2007 Free Software Foundation, Inc.
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "NotifierApp.h"

#include <QtGui/QIcon>

#include <QDebug>

NotifierApp::NotifierApp( int& argc, char* argv[] )
    : QApplication( argc, argv )
{
    setOrganizationName( "Manjaro" );
    setOrganizationDomain( "Manjaro" );
    setApplicationName( "MSM Notifier" );
    setApplicationVersion( "0.5.0" );
}


NotifierApp::~NotifierApp()
{
    qDebug() << "Shutting down MSM Notifier...";
}


void
NotifierApp::init()
{
    setWindowIcon( QIcon::fromTheme( "preferences-system" ) );
}


NotifierApp*
NotifierApp::instance()
{
    return qobject_cast<NotifierApp*>( QApplication::instance() );
}
