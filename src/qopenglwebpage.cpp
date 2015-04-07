/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "qopenglwebpage.h"

#include "mozilla-config.h"
#include "qmozcontext.h"
#include "qmozembedlog.h"
#include "InputData.h"
#include "mozilla/embedlite/EmbedLiteView.h"
#include "mozilla/embedlite/EmbedLiteApp.h"

#include <qglobal.h>
#include <qqmlinfo.h>

#include "qgraphicsmozview_p.h"
#include "qmozscrolldecorator.h"

#define LOG_COMPONENT "QOpenGLWebPage"

using namespace mozilla;
using namespace mozilla::embedlite;

QOpenGLWebPage::QOpenGLWebPage(QObject *parent)
  : QObject(parent)
  , d(new QGraphicsMozViewPrivate(new IMozQView<QOpenGLWebPage>(*this), this))
  , mParentID(0)
  , mPrivateMode(false)
  , mActive(true)
  , mBackground(false)
  , mLoaded(false)
  , mCompleted(false)
{
    d->mContext = QMozContext::GetInstance();
    d->mHasContext = true;

    connect(this, SIGNAL(setIsActive(bool)), this, SLOT(SetIsActive(bool)));
    connect(this, SIGNAL(viewInitialized()), this, SLOT(processViewInitialization()));
    connect(this, SIGNAL(loadProgressChanged()), this, SLOT(updateLoaded()));
    connect(this, SIGNAL(loadingChanged()), this, SLOT(updateLoaded()));

    if (!d->mContext->initialized()) {
        connect(d->mContext, SIGNAL(onInitialized()), this, SLOT(createView()));
    } else {
        createView();
    }
}

QOpenGLWebPage::~QOpenGLWebPage()
{
    if (d->mView) {
        d->mView->SetListener(NULL);
        d->mContext->GetApp()->DestroyView(d->mView);
    }
    delete d;
}

void QOpenGLWebPage::SetIsActive(bool aIsActive)
{
    if (d->mView) {
        d->mView->SetIsActive(aIsActive);
        if (aIsActive) {
            d->mView->ResumeRendering();
        } else {
            d->mView->SuspendRendering();
        }
    }
}

void QOpenGLWebPage::updateLoaded()
{
    bool loaded = loadProgress() == 100 && !loading();
    if (mLoaded != loaded) {
        mLoaded = loaded;
        Q_EMIT loadedChanged();
    }
}

void QOpenGLWebPage::createView()
{
    LOGT("QOpenGLWebPage");
    if (!d->mView) {
        // We really don't care about SW rendering on Qt5 anymore
        d->mContext->GetApp()->SetIsAccelerated(true);
        d->mView = d->mContext->GetApp()->CreateView(mParentID, mPrivateMode);
        d->mView->SetListener(d);
    }
}

void QOpenGLWebPage::processViewInitialization()
{
    // This is connected to view initialization. View must be initialized
    // over here.
    Q_ASSERT(d->mViewInitialized);

    mCompleted = true;
    forceActiveFocus();
    Q_EMIT completedChanged();
}

void QOpenGLWebPage::createGeckoGLContext()
{
}

void QOpenGLWebPage::requestGLContext(bool& hasContext, QSize& viewPortSize)
{
    hasContext = true;
    viewPortSize = d->mGLSurfaceSize;
    Q_EMIT requestGLContext();
}

void QOpenGLWebPage::geometryChanged(const QRectF &newGeometry, const QRectF &oldGeometry)
{
    LOGT("newGeometry size: [%g, %g] oldGeometry size: [%g,%g]", newGeometry.size().width(),
                                                                 newGeometry.size().height(),
                                                                 oldGeometry.size().width(),
                                                                 oldGeometry.size().height());
    setSize(newGeometry.size());
}

int QOpenGLWebPage::parentId() const
{
    return mParentID;
}

bool QOpenGLWebPage::privateMode() const
{
    return mPrivateMode;
}

void QOpenGLWebPage::setPrivateMode(bool privateMode)
{
    if (d->mView) {
        // View is created directly in componentComplete() if mozcontext ready
        qmlInfo(this) << "privateMode cannot be changed after view is created";
        return;
    }

    if (privateMode != mPrivateMode) {
        mPrivateMode = privateMode;
        Q_EMIT privateModeChanged();
    }
}

bool QOpenGLWebPage::enabled() const
{
    return d->mEnabled;
}

void QOpenGLWebPage::setEnabled(bool enabled)
{
    if (d->mEnabled != enabled) {
        d->mEnabled = enabled;
        Q_EMIT enabledChanged();
    }
}

bool QOpenGLWebPage::active() const
{
    return mActive;
}

void QOpenGLWebPage::setActive(bool active)
{
    if (d->mViewInitialized) {
        if (mActive != active) {
            mActive = active;
            Q_EMIT activeChanged();
        }
        SetIsActive(active);
    } else {
        // Will be processed once view is initialized.
        mActive = active;
    }
}

qreal QOpenGLWebPage::width() const
{
    return d->mSize.width();
}

void QOpenGLWebPage::setWidth(qreal width)
{
    QSizeF newSize(width, d->mSize.height());
    setSize(newSize);
}

qreal QOpenGLWebPage::height() const
{
    return d->mSize.height();
}

void QOpenGLWebPage::setHeight(qreal height)
{
    QSizeF newSize(d->mSize.width(), height);
    setSize(newSize);
}

QSizeF QOpenGLWebPage::size() const
{
    return d->mSize;
}

void QOpenGLWebPage::setSize(const QSizeF &size)
{
    if (d->mSize == size) {
        return;
    }

    bool widthWillChanged = d->mSize.width() != size.width();
    bool heightWillChanged = d->mSize.height() != size.height();

    d->mSize = size;
    d->mGLSurfaceSize = size.toSize();
    d->UpdateViewSize();

    if (widthWillChanged) {
        Q_EMIT widthChanged();
    }

    if (heightWillChanged) {
        Q_EMIT heightChanged();
    }

    Q_EMIT sizeChanged();
}

bool QOpenGLWebPage::background() const
{
    return mBackground;
}

void QOpenGLWebPage::setBackground(bool background)
{
    if (mBackground == background) {
        return;
    }

    mBackground = background;
    Q_EMIT backgroundChanged();
}

bool QOpenGLWebPage::loaded() const
{
    return mLoaded;
}

bool QOpenGLWebPage::Invalidate()
{
    return true;
}

void QOpenGLWebPage::CompositingFinished()
{
}

bool QOpenGLWebPage::completed() const
{
    return mCompleted;
}

void QOpenGLWebPage::forceActiveFocus()
{
    Q_ASSERT(d->mViewInitialized);
    setActive(true);
    d->mView->SetIsFocused(true);
}

void QOpenGLWebPage::setInputMethodHints(Qt::InputMethodHints hints)
{
    d->mInputMethodHints = hints;
}

void QOpenGLWebPage::inputMethodEvent(QInputMethodEvent* event)
{
    d->inputMethodEvent(event);
}

void QOpenGLWebPage::keyPressEvent(QKeyEvent* event)
{
    d->keyPressEvent(event);
}

void QOpenGLWebPage::keyReleaseEvent(QKeyEvent* event)
{
    return d->keyReleaseEvent(event);
}

QVariant QOpenGLWebPage::inputMethodQuery(Qt::InputMethodQuery property) const
{
    return d->inputMethodQuery(property);
}

void QOpenGLWebPage::focusInEvent(QFocusEvent* event)
{
    Q_UNUSED(event);
    d->SetIsFocused(true);
}

void QOpenGLWebPage::focusOutEvent(QFocusEvent* event)
{
    Q_UNUSED(event);
    d->SetIsFocused(false);
}

void QOpenGLWebPage::forceViewActiveFocus()
{
    forceActiveFocus();
}

QUrl QOpenGLWebPage::url() const
{
    return QUrl(d->mLocation);
}

void QOpenGLWebPage::setUrl(const QUrl& url)
{
    load(url.toString());
}

QString QOpenGLWebPage::title() const
{
    return d->mTitle;
}

int QOpenGLWebPage::loadProgress() const
{
    return d->mProgress;
}

bool QOpenGLWebPage::canGoBack() const
{
    return d->mCanGoBack;
}

bool QOpenGLWebPage::canGoForward() const
{
    return d->mCanGoForward;
}

bool QOpenGLWebPage::loading() const
{
    return d->mIsLoading;
}

QRectF QOpenGLWebPage::contentRect() const
{
    return d->mContentRect;
}

QSizeF QOpenGLWebPage::scrollableSize() const
{
    return d->mScrollableSize;
}

QPointF QOpenGLWebPage::scrollableOffset() const
{
    return d->mScrollableOffset;
}

float QOpenGLWebPage::resolution() const
{
    return d->mContentResolution;
}

bool QOpenGLWebPage::isPainted() const
{
    return d->mIsPainted;
}

QColor QOpenGLWebPage::bgcolor() const
{
    return d->mBgColor;
}

bool QOpenGLWebPage::getUseQmlMouse()
{
    return false;
}

void QOpenGLWebPage::setUseQmlMouse(bool value)
{
    Q_UNUSED(value);
}

bool QOpenGLWebPage::dragging() const
{
    return d->mDragging;
}

bool QOpenGLWebPage::moving() const
{
    return d->mMoving;
}

bool QOpenGLWebPage::pinching() const{
    return d->mPinching;
}

QMozScrollDecorator* QOpenGLWebPage::verticalScrollDecorator() const
{
    return &d->mVerticalScrollDecorator;
}

QMozScrollDecorator* QOpenGLWebPage::horizontalScrollDecorator() const
{
    return &d->mHorizontalScrollDecorator;
}

bool QOpenGLWebPage::chromeGestureEnabled() const
{
    return d->mChromeGestureEnabled;
}

void QOpenGLWebPage::setChromeGestureEnabled(bool value)
{
    if (value != d->mChromeGestureEnabled) {
        d->mChromeGestureEnabled = value;
        Q_EMIT chromeGestureEnabledChanged();
    }
}

qreal QOpenGLWebPage::chromeGestureThreshold() const
{
    return d->mChromeGestureThreshold;
}

void QOpenGLWebPage::setChromeGestureThreshold(qreal value)
{
    if (value != d->mChromeGestureThreshold) {
        d->mChromeGestureThreshold = value;
        Q_EMIT chromeGestureThresholdChanged();
    }
}

bool QOpenGLWebPage::chrome() const
{
    return d->mChrome;
}

void QOpenGLWebPage::setChrome(bool value)
{
    if (value != d->mChrome) {
        d->mChrome = value;
        Q_EMIT chromeChanged();
    }
}

qreal QOpenGLWebPage::contentWidth() const
{
    return d->mScrollableSize.width();
}

qreal QOpenGLWebPage::contentHeight() const
{
    return d->mScrollableSize.height();
}

void QOpenGLWebPage::loadHtml(const QString& html, const QUrl& baseUrl)
{
    LOGT();
}

void QOpenGLWebPage::goBack()
{
    if (!d->mViewInitialized)
        return;
    d->mView->GoBack();
}

void QOpenGLWebPage::goForward()
{
    if (!d->mViewInitialized)
        return;
    d->mView->GoForward();
}

void QOpenGLWebPage::stop()
{
    if (!d->mViewInitialized)
        return;
    d->mView->StopLoad();
}

void QOpenGLWebPage::reload()
{
    if (!d->mViewInitialized)
        return;
    d->ResetPainted();
    d->mView->Reload(false);
}

void QOpenGLWebPage::load(const QString& url)
{
    d->load(url);
}

void QOpenGLWebPage::sendAsyncMessage(const QString& name, const QVariant& variant)
{
    d->sendAsyncMessage(name, variant);
}

void QOpenGLWebPage::addMessageListener(const QString& name)
{
    d->addMessageListener(name);
}

void QOpenGLWebPage::addMessageListeners(const QStringList& messageNamesList)
{
    d->addMessageListeners(messageNamesList);
}

void QOpenGLWebPage::loadFrameScript(const QString& name)
{
    d->loadFrameScript(name);
}

void QOpenGLWebPage::newWindow(const QString& url)
{
    LOGT("New Window: %s", url.toUtf8().data());
}

quint32 QOpenGLWebPage::uniqueID() const
{
    return d->mView ? d->mView->GetUniqueID() : 0;
}

void QOpenGLWebPage::setParentID(unsigned aParentID)
{
    if (aParentID != mParentID) {
        mParentID = aParentID;
        Q_EMIT parentIdChanged();
    }

    if (mParentID) {
        createView();
    }
}

void QOpenGLWebPage::synthTouchBegin(const QVariant& touches)
{
    Q_UNUSED(touches);
}

void QOpenGLWebPage::synthTouchMove(const QVariant& touches)
{
    Q_UNUSED(touches);
}

void QOpenGLWebPage::synthTouchEnd(const QVariant& touches)
{
    Q_UNUSED(touches);
}

void QOpenGLWebPage::suspendView()
{
    if (!d->mView) {
        return;
    }
    setActive(false);
    d->mView->SuspendTimeouts();
}

void QOpenGLWebPage::resumeView()
{
    if (!d->mView) {
        return;
    }
    setActive(true);
    d->mView->ResumeTimeouts();
}

void QOpenGLWebPage::recvMouseMove(int posX, int posY)
{
    Q_ASSERT_X(false, "QOpenGLWebPage", "calling recvMouseMove not supported!");
}

void QOpenGLWebPage::recvMousePress(int posX, int posY)
{
    Q_ASSERT_X(false, "QOpenGLWebPage", "calling recvMousePress not supported!");
}

void QOpenGLWebPage::recvMouseRelease(int posX, int posY)
{
    Q_ASSERT_X(false, "QOpenGLWebPage", "calling recvMouseRelease not supported!");
}

void QOpenGLWebPage::touchEvent(QTouchEvent *event)
{
    d->touchEvent(event);
    event->accept();
}

void QOpenGLWebPage::timerEvent(QTimerEvent *event)
{
    d->timerEvent(event);
}
