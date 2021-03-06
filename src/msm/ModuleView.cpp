/*****************************************************************************
 *   Copyright (C) 2009 Ben Cooksley <bcooksley@kde.org>                     *
 *   Copyright (C) 2009 by Mathias Soeken <msoeken@informatik.uni-bremen.de> *
 *                                                                           *
 *   This program is free software; you can redistribute it and/or modify    *
 *   it under the terms of the GNU General Public License as published by    *
 *   the Free Software Foundation; either version 2 of the License, or       *
 *   (at your option) any later version.                                     *
 *                                                                           *
 *   This program is distributed in the hope that it will be useful,         *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of          *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           *
 *   GNU General Public License for more details.                            *
 *                                                                           *
 *   You should have received a copy of the GNU General Public License       *
 *   along with this program; if not, write to the                           *
 *   Free Software Foundation, Inc.,                                         *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA            *
 *****************************************************************************/

#include "ModuleView.h"

#include <QMap>
#include <QList>
#include <QProcess>
#include <QKeyEvent>
#include <QWhatsThis>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QPushButton>
#include <QLoggingCategory>
#include <QDialogButtonBox>
#include <QApplication>

#include <KPageWidget>
#include <KAuthorized>
#include <KMessageBox>
#include <KCModuleInfo>
#include <KCModuleProxy>
#include <KStandardGuiItem>
#include <kauthaction.h>
#include <KAboutData>
#include <kauthobjectdecorator.h>
#include <KIconLoader>


class ModuleView::Private
{
public:
    Private() {}
    QMap<KPageWidgetItem*, KCModuleProxy*> mPages;
    QMap<KPageWidgetItem*, KCModuleInfo*> mModules;
    KPageWidget* mPageWidget;
    QVBoxLayout* mLayout;
    QDialogButtonBox* mButtons;
    KAuth::ObjectDecorator* mApplyAuthorize;
    QPushButton* mApply;
    QPushButton* mReset;
    QPushButton* mDefault;
    QPushButton* mHelp;
    QPushButton* mBack;
    bool pageChangeSupressed;
};


ModuleView::ModuleView( QWidget* parent )
    : QWidget( parent )
    , d( new Private() )
{
    // Configure a layout first
    d->mLayout = new QVBoxLayout( this );
    // Create the Page Widget
    d->mPageWidget = new KPageWidget( this );
    d->mPageWidget->layout()->setMargin( 0 );
    d->mLayout->addWidget( d->mPageWidget );
    // Create the dialog
    d->mButtons = new QDialogButtonBox( Qt::Horizontal, this );
    d->mLayout->addWidget( d->mButtons );

    // Create the buttons in it
    d->mApply = d->mButtons->addButton( QDialogButtonBox::Apply );
    KGuiItem::assign( d->mApply, KStandardGuiItem::apply() );
    d->mDefault = d->mButtons->addButton( QDialogButtonBox::RestoreDefaults );
    KGuiItem::assign( d->mDefault, KStandardGuiItem::defaults() );
    d->mReset = d->mButtons->addButton( QDialogButtonBox::Reset );
    KGuiItem::assign( d->mReset, KStandardGuiItem::reset() );
    d->mHelp = d->mButtons->addButton( QDialogButtonBox::Help );
    KGuiItem::assign( d->mHelp, KStandardGuiItem::help() );
    d->mBack = d->mButtons->addButton( QDialogButtonBox::Close );
    KGuiItem::assign( d->mBack, KStandardGuiItem::back() );
    // Set some more sensible tooltips
    d->mReset->setToolTip( "Reset all current changes to previous values" );
    // Set Auto-Default mode ( KDE Bug #211187 )
    d->mApply->setAutoDefault( true );
    d->mDefault->setAutoDefault( true );
    d->mReset->setAutoDefault( true );
    d->mHelp->setAutoDefault( true );
    d->mBack->setAutoDefault( true );
    // Prevent the buttons from being used
    d->mApply->setEnabled( false );
    d->mDefault->setEnabled( false );
    d->mReset->setEnabled( false );
    d->mHelp->setEnabled( false );
    d->mBack->setEnabled( true );
    // Connect up the buttons
    connect( d->mApply, SIGNAL( clicked() ), this, SLOT( moduleSave() ) );
    connect( d->mReset, SIGNAL( clicked() ), this, SLOT( moduleLoad() ) );
    connect( d->mHelp, SIGNAL( clicked() ), this, SLOT( moduleHelp() ) );
    connect( d->mDefault, SIGNAL( clicked() ), this, SLOT( moduleDefaults() ) );
    connect( d->mBack, SIGNAL( clicked() ), this, SLOT( moduleClose() ) );
    connect( d->mPageWidget, SIGNAL( currentPageChanged( KPageWidgetItem*,KPageWidgetItem* ) ),
             this, SLOT( activeModuleChanged( KPageWidgetItem*,KPageWidgetItem* ) ) );
    connect( this, SIGNAL( moduleChanged( bool ) ), this, SLOT( updateButtons() ) );

    d->mApplyAuthorize = new KAuth::ObjectDecorator( d->mApply );
    d->mApplyAuthorize->setAuthAction( KAuth::Action() );
}


ModuleView::~ModuleView()
{
    delete d;
}


KCModuleInfo*
ModuleView::activeModule() const
{
    return d->mModules.value( d->mPageWidget->currentPage() );
}


const KAboutData*
ModuleView::aboutData() const
{
    KCModuleProxy* activeModule = d->mPages.value( d->mPageWidget->currentPage() );
    KAboutData* aboutData = 0;
    if ( activeModule )
        aboutData = const_cast<KAboutData*>( activeModule->aboutData() );
    if ( aboutData )
    {
        QApplication::setWindowIcon( QIcon::fromTheme( activeModule->moduleInfo().service()->icon() ) );
        return aboutData;
    }
    return 0;
}


void
ModuleView::addModule( KCModuleInfo* module )
{
    if ( !module )
        return;
    if ( !module->service() )
    {
        qWarning() << "ModuleInfo has no associated KService" ;
        return;
    }
    if ( !KAuthorized::authorizeControlModule( module->service()->menuId() ) )
    {
        qWarning() << "Not authorised to load module" ;
        return;
    }
    if ( module->service()->noDisplay() )
        return;

    // Create the scroller
    QScrollArea* moduleScroll = new QScrollArea( this );
    // Prepare the scroll area
    moduleScroll->setWidgetResizable( true );
    moduleScroll->setFrameStyle( QFrame::NoFrame );
    moduleScroll->viewport()->setAutoFillBackground( false );
    // Create the page
    KPageWidgetItem* page = new KPageWidgetItem( moduleScroll, module->moduleName() );
    // Provide information to the users

    KCModuleProxy* moduleProxy = new KCModuleProxy( *module, moduleScroll );
    moduleScroll->setWidget( moduleProxy );
    moduleProxy->setAutoFillBackground( false );
    connect( moduleProxy, SIGNAL( changed( bool ) ), this, SLOT( stateChanged() ) );
    d->mPages.insert( page, moduleProxy );

    d->mModules.insert( page, module );
    updatePageIconHeader( page, true );
    // Add the new page
    d->mPageWidget->addPage( page );
}


void
ModuleView::updatePageIconHeader( KPageWidgetItem* page, bool light )
{
    if ( !page )
    {
        // Page is invalid. Probably means we have a race condition during closure of everyone so do nothing
        return;
    }

    KCModuleProxy* moduleProxy = d->mPages.value( page );
    KCModuleInfo* moduleInfo = d->mModules.value( page );

    if ( !moduleInfo )
    {
        // Seems like we have some form of a race condition going on here...
        return;
    }

    page->setHeader( moduleInfo->comment() );
    page->setIcon( QIcon::fromTheme( moduleInfo->icon() ) );
    if ( light )
        return;

    if ( moduleProxy && moduleProxy->realModule()->useRootOnlyMessage() )
    {
        page->setHeader( "<b>" + moduleInfo->comment() + "</b><br><i>" + moduleProxy->realModule()->rootOnlyMessage() + "</i>" );
        page->setIcon( KDE::icon( moduleInfo->icon(), QStringList() << "dialog-warning" ) );
    }
}


bool
ModuleView::resolveChanges()
{
    KCModuleProxy* currentProxy = d->mPages.value( d->mPageWidget->currentPage() );
    return resolveChanges( currentProxy );
}


bool
ModuleView::resolveChanges( KCModuleProxy* currentProxy )
{
    if ( !currentProxy || !currentProxy->changed() )
        return true;

    // Let the user decide
    const int queryUser = KMessageBox::warningYesNoCancel(
                              this,
                              "The settings of the current module have changed.\n"
                              "Do you want to apply the changes or discard them?",
                              "Apply Settings",
                              KStandardGuiItem::apply(),
                              KStandardGuiItem::discard(),
                              KStandardGuiItem::cancel() );

    switch ( queryUser )
    {
    case KMessageBox::Yes:
        return moduleSave( currentProxy );

    case KMessageBox::No:
        currentProxy->load();
        return true;

    case KMessageBox::Cancel:
        return false;

    default:
        Q_ASSERT( false );
        return false;
    }
}


void
ModuleView::closeModules()
{
    d->pageChangeSupressed = true;
    d->mApplyAuthorize->setAuthAction( KAuth::Action() ); // Ensure KAuth knows that authentication is now pointless...
    QMap<KPageWidgetItem*, KCModuleInfo*>::iterator page = d->mModules.begin();
    QMap<KPageWidgetItem*, KCModuleInfo*>::iterator pageEnd = d->mModules.end();
    for ( ; page != pageEnd; ++page )
        d->mPageWidget->removePage( page.key() );

    d->mPages.clear();
    d->mModules.clear();
    d->pageChangeSupressed = false;
}


bool
ModuleView::moduleSave()
{
    KCModuleProxy* moduleProxy = d->mPages.value( d->mPageWidget->currentPage() );
    return moduleSave( moduleProxy );
}


bool
ModuleView::moduleSave( KCModuleProxy* module )
{
    if ( !module )
        return false;

    module->save();
    return true;
}


void
ModuleView::moduleLoad()
{
    KCModuleProxy* activeModule = d->mPages.value( d->mPageWidget->currentPage() );
    if ( activeModule )
        activeModule->load();
}


void
ModuleView::moduleDefaults()
{
    KCModuleProxy* activeModule = d->mPages.value( d->mPageWidget->currentPage() );
    if ( activeModule )
        activeModule->defaults();
}


void
ModuleView::moduleHelp()
{
    KCModuleInfo* activeModule = d->mModules.value( d->mPageWidget->currentPage() );
    if ( !activeModule )
        return;

    QString docPath = activeModule->docPath();
    if ( docPath.isEmpty() )
        return;
    QUrl url( "help:/"+docPath );
    QProcess::startDetached( "khelpcenter", QStringList() << url.url() );
}


void
ModuleView::moduleClose()
{
    emit closeRequest();
}


void
ModuleView::activeModuleChanged( KPageWidgetItem* current, KPageWidgetItem* previous )
{
    d->mPageWidget->blockSignals( true );
    d->mPageWidget->setCurrentPage( previous );
    KCModuleProxy* previousModule = d->mPages.value( previous );
    if ( resolveChanges( previousModule ) )
        d->mPageWidget->setCurrentPage( current );
    d->mPageWidget->blockSignals( false );
    if ( d->pageChangeSupressed )
        return;
    // We need to get the state of the now active module
    stateChanged();
}


void
ModuleView::stateChanged()
{
    KCModuleProxy* activeModule = d->mPages.value( d->mPageWidget->currentPage() );
    KAuth::Action moduleAction;
    bool change = false;
    if ( activeModule )
    {
        change = activeModule->changed();

        disconnect( d->mApplyAuthorize, SIGNAL( authorized( KAuth::Action ) ), this, SLOT( moduleSave() ) );
        disconnect( d->mApply, SIGNAL( clicked() ), this, SLOT( moduleSave() ) );
        if ( activeModule->realModule()->authAction().isValid() )
        {
            connect( d->mApplyAuthorize, SIGNAL( authorized( KAuth::Action ) ), this, SLOT( moduleSave() ) );
            moduleAction = activeModule->realModule()->authAction();
        }
        else
            connect( d->mApply, SIGNAL( clicked() ), this, SLOT( moduleSave() ) );
    }

    updatePageIconHeader( d->mPageWidget->currentPage() );
    d->mApplyAuthorize->setAuthAction( moduleAction );
    d->mApply->setEnabled( change );
    d->mReset->setEnabled( change );
    emit moduleChanged( change );
}


void
ModuleView::keyPressEvent ( QKeyEvent* event )
{
    if ( event->key() == Qt::Key_F1 && d->mHelp->isVisible() && d->mHelp->isEnabled() )
    {
        d->mHelp->animateClick();
        event->accept();
        return;
    }
    else if ( event->key() == Qt::Key_Escape )
    {
        event->accept();
        emit closeRequest();
        return;
    }
    else if ( event->key() == Qt::Key_F1 && event->modifiers() == Qt::ShiftModifier )
    {
        QWhatsThis::enterWhatsThisMode();
        event->accept();
        return;
    }

    QWidget::keyPressEvent( event );
}


void
ModuleView::updateButtons()
{
    KCModuleProxy* activeModule = d->mPages.value( d->mPageWidget->currentPage() );
    if ( !activeModule )
        return;

    const int buttons = activeModule->buttons();

    d->mApply->setVisible( buttons & KCModule::Apply );
    d->mReset->setVisible( buttons & KCModule::Apply );

    d->mHelp->setEnabled( buttons & KCModule::Help );
    d->mHelp->setVisible( buttons & KCModule::Help );
    d->mDefault->setEnabled( buttons & KCModule::Default );
    d->mDefault->setVisible( buttons & KCModule::Default );
}
