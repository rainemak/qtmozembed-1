/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef QuickMozView_H
#define QuickMozView_H

#include <QMatrix>
#include <QtQuick/QQuickItem>
#include <QWindow>
#include <QPointer>
#include <QtGui/QOpenGLShaderProgram>
#include "qmozview_defined_wrapper.h"

class QGraphicsMozViewPrivate;
class QSGThreadObject;

class QuickMozView : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int parentId READ parentId WRITE setParentID NOTIFY parentIdChanged FINAL)
    Q_PROPERTY(unsigned parentid WRITE setParentID)
    Q_PROPERTY(bool active READ active WRITE setActive NOTIFY activeChanged FINAL)
    Q_PROPERTY(QObject* child READ getChild NOTIFY childChanged)

    Q_MOZ_VIEW_PRORERTIES

public:
    QuickMozView(QObject *parent = 0);
    ~QuickMozView();

    void setWindow(QWindow *window);

    Q_MOZ_VIEW_PUBLIC_METHODS
    void RenderToCurrentContext();
    void startMoveMonitoring();
    void RefreshNodeTexture();

    int parentId() const;

    bool active() const;
    void setActive(bool active);
    virtual void geometryChanged(const QRectF & newGeometry, const QRectF & oldGeometry);

    //    void itemChange(ItemChange change, const ItemChangeData &);
    //    virtual QSGNode* updatePaintNode(QSGNode* node, UpdatePaintNodeData* data);
    virtual void mouseMoveEvent(QMouseEvent* event);
    virtual void mousePressEvent(QMouseEvent* event);
    virtual void mouseReleaseEvent(QMouseEvent* event);
    virtual QVariant inputMethodQuery(Qt::InputMethodQuery property) const;
    virtual void inputMethodEvent(QInputMethodEvent* event);
    virtual void keyPressEvent(QKeyEvent*);
    virtual void keyReleaseEvent(QKeyEvent*);
    virtual void focusInEvent(QFocusEvent*);
    virtual void focusOutEvent(QFocusEvent*);
    virtual void touchEvent(QTouchEvent*);
    virtual void timerEvent(QTimerEvent*);
    // Should be renamed...
    virtual void componentComplete(QOpenGLContext *context);

private:
    QObject* getChild() { return this; }
    void updateGLContextInfo();

public Q_SLOTS:
    Q_MOZ_VIEW_PUBLIC_SLOTS
    void onRenderThreadReady();

Q_SIGNALS:
    void childChanged();
    void setIsActive(bool);
    void wrapRenderThreadGLContext();
    void dispatchItemUpdate();
    void textureReady(int id, const QSize &size);
    void parentIdChanged();
    void activeChanged();
    void requestUpdate();

    Q_MOZ_VIEW_SIGNALS

private Q_SLOTS:
    void processViewInitialization();
    void SetIsActive(bool aIsActive);

public Q_SLOTS:
    void beforeRendering();
    void init(QOpenGLContext *context);
    void cleanup();
    void setInputMethodHints(Qt::InputMethodHints hints);
    void updateGLContextInfo(QOpenGLContext*);

private Q_SLOTS:
    void onInitialized();
    void updateEnabled();

private:
    QGraphicsMozViewPrivate* d;
    friend class QGraphicsMozViewPrivate;
    unsigned mParentID;
    bool mUseQmlMouse;
    QSGThreadObject* mSGRenderer;
    int mTimerId;
    qreal mOffsetX;
    qreal mOffsetY;
#ifndef NO_PRIVATE_API
    bool mInThreadRendering;
#endif
    bool mPreedit;
    bool mActive;
    bool mHasPendingInvalidate;
    QPointer<QWindow> m_window;
};

#endif // QuickMozView_H
